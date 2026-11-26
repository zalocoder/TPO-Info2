/*******************************************************************************************************************************//**
 *
 * @file		uart.cpp
 *
 **********************************************************************************************************************************/

#include "ColaCircular.h"
#if CC_MODE_WORK != 5

/***********************************************************************************************************************************
 *** INCLUDES
 **********************************************************************************************************************************/
#include "uart.h"
#include "swm.h"



/***********************************************************************************************************************************
 *** VARIABLES GLOBALES PRIVADAS AL MODULO
 **********************************************************************************************************************************/
static uart *g_usart[ 5 ];

/***********************************************************************************************************************************
 *** IMPLEMENTACION DE LOS METODODS DE LA CLASE
 **********************************************************************************************************************************/

// SCAI 26/11/2026: se agrandaron los buffers (eran 32 y 64). Una trama $EVT tiene ~70 caracteres
// y si no entra en el buffer de TX, push() descarta lo que sobra y la trama sale cortada.
// Las respuestas del ESP8266 (+IPD, lineas de estado) tambien superan los 64 bytes.
#define TX_BUFF_SZ 128
#define RX_BUFF_SZ 128

// SCAI 26/11/2026: clock de las UART por el generador fraccional FRG0.
// Con el FRO de 12 MHz directo, 115200 baudios da BRG = 12e6/(16*115200) - 1 = 5,5 -> 5 -> 125000 baudios
// (8,5 % de error: el PN532 y el ESP8266 no lo toleran). El FRG0 baja el clock a
//   12 MHz / (1 + MULT/256) = 12 MHz / (1 + 22/256) = 11,0504 MHz
// que es casi exactamente 6 * 16 * 115200 = 11,0592 MHz -> 115200 baudios con 0,08 % de error.
// El mismo clock sirve para 9600 (BRG = 71, 0,08 % de error).
#define UART_FRG_MULT		22
#define UART_FREQ_CLK		( ( 12000000ULL * 256 ) / ( 256 + UART_FRG_MULT ) )	// = 11050359 Hz

uart::uart(	uint8_t  numUart, uint8_t portTx ,uint8_t pinTx , uint8_t portRx , uint8_t pinRx ,
            uint32_t baudrate, bits_datos_t BitsDeDatos, paridad_t paridad)
           : m_buffRx (RX_BUFF_SZ),m_buffTx (TX_BUFF_SZ)
{

	const struct {
		uint8_t ctrl;
		uint8_t iser;
		uint8_t pa_txd;
		uint8_t pa_rxd;
		USART_Type *pUart;
	} prxUart[]={
			{14, 3,PA_U0_TXD,PA_U0_RXD,USART0},
			{15, 4,PA_U1_TXD,PA_U1_RXD,USART1},
			{16, 5,PA_U2_TXD,PA_U2_RXD,USART2},
			{30,30,PA_U3_TXD,PA_U3_RXD,USART3},
			{31,31,PA_U4_TXD,PA_U4_RXD,USART4}
	};

	x_num=numUart;

//==================================================
// ---- configuracion de parametros intrinsecos ----
//    parametros predefinidos por número de UART
// -------------------------------------------------

	SYSCON->SYSAHBCLKCTRL0 |= ( 1 << prxUart[numUart].ctrl );		// habilita el clock

	SYSCON->PRESETCTRL0&= ~( 1 <<  prxUart[numUart].ctrl );			// se reinicia
	SYSCON->PRESETCTRL0|=  ( 1 <<  prxUart[numUart].ctrl );

	SYSCON->FRG[ 0 ].FRGCLKSEL = 0;									// FRG0 toma el FRO (12 MHz)
	SYSCON->FRG[ 0 ].FRGDIV	   = 255;								// DIV tiene que ser 255 (manual)
	SYSCON->FRG[ 0 ].FRGMULT   = UART_FRG_MULT;
	SYSCON->FCLKSEL[ numUart ] = 2;									// 2 = FRG0 como clock de esta UART

	PINASSIGN_Config(prxUart[numUart].pa_txd, portTx, pinTx);		// asignación de pines
	PINASSIGN_Config(prxUart[numUart].pa_rxd, portRx, pinRx);

	m_usart=prxUart[numUart].pUart; 								// USART seleccionada
	g_usart[ numUart ] = this ;	 	 //  interrupción

//==================================================
//---------- parametros de comunicación,
//                	interrupciones y otros -----------
// -------------------------------------------------

	m_flagTx = false ;			// flag de fin de Tx

	m_usart->CFG = ( BitsDeDatos << 2 )	| ( paridad << 4 );

	// Divisor redondeado al entero mas cercano (sumar la mitad del divisor antes de dividir)
	{
		uint32_t div = baudrate * ( m_usart->OSR + 1 );
		m_usart->BRG = (uint32_t)( ( UART_FREQ_CLK + div / 2 ) / div ) - 1;
	}

	m_usart->INTENSET = DATAREADY;			// RX interrupcion

	NVIC->ISER[0] = ( 1 << prxUart[numUart].iser ); 	// habilitamos UART_IRQ

	m_usart->CFG |= ( 1 << 0 );			// habilitamos USART



}


uart::~uart() {
	// TODO Auto-generated destructor stub
}








void uart::Transmit ( char val)
{

	m_buffTx.push(val );

	if ( m_flagTx == false )
	{
		m_flagTx = true ;
		Tx_EnableInterupt (  );
	}

}



void uart::Transmit ( const char * msg)
{
	while ( *msg )
	{
		m_buffTx.push(*msg );
		msg++;

		if ( m_flagTx == false )
		{
			m_flagTx = true ;
			Tx_EnableInterupt (  );
		}
	}
}


void uart::Transmit ( void * msg , uint32_t n )
{
	for ( uint32_t i = 0 ; i < n ; i++ )
	{
		m_buffTx.push ( ((uint8_t*)msg)[ i ] );

		if ( m_flagTx == false )
		{
			m_flagTx = true ;
			Tx_EnableInterupt (  );
		}
	}
}


void uart::Tx_EnableInterupt (  void )
{
	m_usart->INTENSET = (1 << 2);
}

void uart::Tx_DisableInterupt (  void )
{
	m_usart->INTENCLR = (1 << 2);
}



void uart::UART_IRQHandler ( void )
{
uint8_t datoTx ;
uint8_t datoRx;
uint32_t stat = m_usart->STAT ;

	//--- Proceso de Recepcion
	if( stat & (DATAREADY) )
	{
		datoRx = ( uint8_t ) m_usart->RXDAT;

		m_buffRx.push(datoRx);
	}

	//--- Proceso de Transmision

	if( stat & TX_READY )
	{
		if( m_buffTx.pop(datoTx) )
			m_usart->TXDAT = datoTx ;
		else
		{
			Tx_DisableInterupt ( );
			m_flagTx = false ;
		}
	}

}


char * uart::Receive ( char * msg , int n)
{

	if (m_buffRx.isNewLine()||m_buffRx.qtty()>= n-1)
	{
		int i;
		uint8_t dato;

		for (i=0;i<n-1;i++)
		{

			if ( m_buffRx.pop ( dato ) )
				msg [i] = dato;
			else
				break;


		}


		m_buffRx.newLineClear();	//Corresponde hacerlo siempre??

		msg[i]=0;
		return msg;
	}
return nullptr;
}






void UART0_IRQHandler ( void )
{
	g_usart[ 0 ]->UART_IRQHandler();
}
void UART1_IRQHandler ( void )
{
	g_usart[ 1 ]->UART_IRQHandler();
}
void UART2_IRQHandler ( void )
{
	g_usart[ 2 ]->UART_IRQHandler();
}
void PININT6_IRQHandler ( void )
{
	g_usart[ 3 ]->UART_IRQHandler();
}
void  PININT7_IRQHandler ( void )
{
	g_usart[ 4 ]->UART_IRQHandler();

}

#endif
