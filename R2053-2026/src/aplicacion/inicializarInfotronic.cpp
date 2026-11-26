/*******************************************************************************************************************************//**
 *
 * @file		inicializarInfotronic.cpp
 * @brief		SCAI - Inicializacion de la placa (ver inicializarInfotronic.h)
 * @details		Cambios respecto de la version de la catedra:
 *				  - Se saco el display de 7 segmentos, el teclado 2x3, el ADC y la UART4 de 9600:
 *				    el SCAI no los usa y sus pines hacen falta (ver pinout.h).
 *				  - Se agregaron: teclado 4x4, UART a PC / PN532 / ESP8266, I2C + DS3231, servo, reed y buzzer.
 *				  - Todos los pines salen de pinout.h.
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/

/***********************************************************************************************************************************
 *** INCLUDES
 **********************************************************************************************************************************/
#include "inicializarInfotronic.h"

/***********************************************************************************************************************************
 *** DEFINES PRIVADOS AL MODULO
 **********************************************************************************************************************************/
#define FREQ_SYSTICK	(1)		// periodo del SysTick en ms

/***********************************************************************************************************************************
 *** VARIABLES GLOBALES
 **********************************************************************************************************************************/
// Lista de objetos que el SysTick atiende cada 1 ms (timers, teclado, LCD, LEDs, reed, buzzer).
// IMPORTANTE: tiene que estar ANTES que los objetos globales de este archivo que se enganchan en ella
// (dentro de un mismo archivo C++ construye las globales en el orden en que aparecen).
list <Callback*> g_CallbackList;

// Mapa de teclas del 4x4. El codigo que entrega el teclado es columna + fila * 4.     [VERIFICAR con el teclado real]
const char TECLAS_4x4[ 16 ] = {
	'1' , '2' , '3' , 'A' ,
	'4' , '5' , '6' , 'B' ,
	'7' , '8' , '9' , 'C' ,
	'*' , '0' , '#' , 'D'
};

//!<	LEDs: el RGB de la BRK es activo en bajo -> gpio::low
static gpio g_ledVerde( LED_VERDE_PORT , LED_VERDE_PIN , gpio::pushpull , gpio::output , gpio::low );
static gpio g_ledAzul ( LED_AZUL_PORT  , LED_AZUL_PIN  , gpio::pushpull , gpio::output , gpio::low );
static gpio g_ledRojo ( LED_ROJO_PORT  , LED_ROJO_PIN  , gpio::pushpull , gpio::output , gpio::low );
static vector <gpio *> VectorDeLeds;

Led LedVerde( LED_N_VERDE , Callback_Leds_gpio );
Led LedAzul ( LED_N_AZUL  , Callback_Leds_gpio , 500 );	// 500 ms prendido / 500 ms apagado
Led LedRojo ( LED_N_ROJO  , Callback_Leds_gpio );

//!<	TECLADO 4x4
static vector <gpio *> scn;		// filas (salidas)
static vector <gpio *> ret;		// columnas (entradas con pull-up)
teclado *Teclado;

//!<	LCD
static vector <gpio *> BusLcd;
Lcd *lcd;

//!<	Comunicaciones y perifericos del SCAI
uart		*uartPC;
uart		*uartNFC;
uart		*uartWiFi;
i2c			*busI2C;
rtc_ds3231	*rtc;
servo		*Servo;
reed		*Puerta;
buzzer		*Buzzer;

/***********************************************************************************************************************************
 *** FUNCIONES GLOBALES AL MODULO
 **********************************************************************************************************************************/
/**
	\brief	La llama la ISR del SysTick cada 1 ms: le da un "tick" a cada objeto enganchado
*/
void systick_callback ( void  )
{
	for (Callback* q : g_CallbackList )
		q->inBackGround();
}

/**
	\brief	La clase Led no sabe de pines: llama a esta funcion para prender o apagar el LED numero n
*/
void Callback_Leds_gpio ( uint8_t NumeroDeLed , Led::led_t Estado )
{
	if ( NumeroDeLed < VectorDeLeds.size( ) )
	{
		if ( Estado )
			VectorDeLeds[ NumeroDeLed ]->SetPin();
		else
			VectorDeLeds[ NumeroDeLed ]->ClrPin();
	}
}

/**
	\brief	Configura toda la placa. Se llama una sola vez al principio de main().
	\details El orden importa:
			 1) PLL (clock del sistema).
			 2) Todo lo que se engancha en g_CallbackList, ANTES de arrancar el SysTick: asi la ISR
			    nunca recorre la lista mientras se le estan agregando elementos.
			 3) SysTick.
			 4) LCD al final: su constructor espera tiempos de arranque del HD44780 que cuenta el SysTick.
			 La memoria dinamica (new / vector) se usa solo aca, nunca dentro del while(1).
*/
void InicializarInfotronic ( void )
{
	Inicializar_PLL( );

	// ## LEDS ######################################################################################
	VectorDeLeds.push_back( &g_ledVerde );		// en el orden de LED_N_VERDE, LED_N_AZUL, LED_N_ROJO
	VectorDeLeds.push_back( &g_ledAzul );
	VectorDeLeds.push_back( &g_ledRojo );
	LedVerde = Led::OFF;
	LedRojo  = Led::OFF;
	LedAzul  = Led::BLINK;						// latido

	// ## TECLADO 4x4 ###############################################################################
	scn.push_back( new gpio( TEC_FIL0_PORT , TEC_FIL0_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	scn.push_back( new gpio( TEC_FIL1_PORT , TEC_FIL1_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	scn.push_back( new gpio( TEC_FIL2_PORT , TEC_FIL2_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	scn.push_back( new gpio( TEC_FIL3_PORT , TEC_FIL3_PIN , gpio::pushpull , gpio::output , gpio::high ) );

	ret.push_back( new gpio( TEC_COL0_PORT , TEC_COL0_PIN , gpio::pullup , gpio::input , gpio::low ) );
	ret.push_back( new gpio( TEC_COL1_PORT , TEC_COL1_PIN , gpio::pullup , gpio::input , gpio::low ) );
	ret.push_back( new gpio( TEC_COL2_PORT , TEC_COL2_PIN , gpio::pullup , gpio::input , gpio::low ) );
	ret.push_back( new gpio( TEC_COL3_PORT , TEC_COL3_PIN , gpio::pullup , gpio::input , gpio::low ) );

	Teclado = new teclado ( scn , ret );

	// ## SENSOR DE PUERTA Y BUZZER #################################################################
	Puerta = new reed( REED_PORT , REED_PIN , REED_CANAL_PINT );
	Buzzer = new buzzer( BUZZER_PORT , BUZZER_PIN , BUZZER_ACTIVO_ALTO );

	// ## SERVO (SCT) ###############################################################################
	Servo = new servo( SERVO_PORT , SERVO_PIN );	// arranca en 0 grados (puerta cerrada)

	// ## UARTs #####################################################################################
	uartPC	 = new uart( PC_UART_NUM   , PC_TX_PORT   , PC_TX_PIN   , PC_RX_PORT   , PC_RX_PIN   , PC_BAUDIOS );
	uartNFC	 = new uart( NFC_UART_NUM  , NFC_TX_PORT  , NFC_TX_PIN  , NFC_RX_PORT  , NFC_RX_PIN  , NFC_BAUDIOS );
	uartWiFi = new uart( WIFI_UART_NUM , WIFI_TX_PORT , WIFI_TX_PIN , WIFI_RX_PORT , WIFI_RX_PIN , WIFI_BAUDIOS );

	uartPC->setStringMode( LF );				// las tramas $...\n de la PC terminan en '\n'
	uartWiFi->setStringMode( CR_LF );			// el firmware AT del ESP8266 termina las lineas en "\r\n"

	// ## I2C + RTC #################################################################################
	busI2C = new i2c( RTC_I2C_NUM , RTC_SDA_PORT , RTC_SDA_PIN , RTC_SCL_PORT , RTC_SCL_PIN );
	rtc	   = new rtc_ds3231( *busI2C );

	// ## SYSTICK ###################################################################################
	SysTick_CallBack_Install( systick_callback );
	Inicializar_SysTick( FREQ_SYSTICK );

	// ## LCD (necesita el SysTick andando) ##########################################################
	BusLcd.push_back( new gpio( LCD_D4_PORT , LCD_D4_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	BusLcd.push_back( new gpio( LCD_D5_PORT , LCD_D5_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	BusLcd.push_back( new gpio( LCD_D6_PORT , LCD_D6_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	BusLcd.push_back( new gpio( LCD_D7_PORT , LCD_D7_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	BusLcd.push_back( new gpio( LCD_RS_PORT , LCD_RS_PIN , gpio::pushpull , gpio::output , gpio::high ) );
	BusLcd.push_back( new gpio( LCD_EN_PORT , LCD_EN_PIN , gpio::pushpull , gpio::output , gpio::high ) );

	// RIESGO CONOCIDO (heredado de la catedra): el constructor de Lcd se engancha en g_CallbackList
	// con el SysTick ya corriendo. No se pueden apagar las interrupciones alrededor del new porque
	// el propio constructor espera que el SysTick descuente sus demoras. Ver el informe.
	lcd = new Lcd( BusLcd );
}
