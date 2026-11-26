/*******************************************************************************************************************************//**
 *
 * @file		reed.cpp
 * @brief		Reed switch con PINT + antirrebote (ver reed.h)
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#include "reed.h"

#define PINT_CANALES_UTILES		6

// Objeto asociado a cada canal, para que la ISR sepa a quien avisar
static reed *g_reed[ PINT_CANALES_UTILES ] = { nullptr };

/**
	\brief	Configura el pin como entrada con pull-up y el canal de PINT para ambos flancos
	\details Con pull-up: contacto abierto (sin iman = puerta abierta) -> pin en 1.
			 Contacto cerrado (iman cerca = puerta cerrada) -> pin en 0.
*/
reed::reed( uint8_t port , uint8_t pin , uint8_t canal )
	: m_pin( port , pin , gpio::pullup , gpio::input , gpio::high )
{
	if ( canal >= PINT_CANALES_UTILES )
		canal = 0;
	m_canal	 = canal;
	m_cuenta = 0;
	m_cambio = false;
	m_abierta = m_pin.GetPin( );				// estado inicial, sin esperar un flanco

	g_reed[ canal ] = this;
	g_CallbackList.push_back( this );			// para que el SysTick llame a inBackGround()

	SYSCON->SYSAHBCLKCTRL0 |= SYSCON_SYSAHBCLKCTRL0_GPIOINT_MASK;	// clock del bloque PINT
	SYSCON->PINTSEL[ canal ] = ( port == 1 ) ? 32 + pin : pin;		// que pin mira este canal

	PINT->ISEL	&= ~( 1 << canal );				// por flanco
	PINT->SIENR	 =  ( 1 << canal );				// flanco ascendente
	PINT->SIENF	 =  ( 1 << canal );				// flanco descendente
	PINT->IST	 =  ( 1 << canal );				// borra cualquier flanco viejo

	NVIC->ISER[ 0 ] = ( 1 << ( PININT0_IRQn + canal ) );
}

/**
	\brief	Llamada desde la ISR: solo (re)arranca la cuenta del antirrebote
	\details Si el contacto rebota, cada rebote vuelve a cargar la cuenta; recien cuando pasan
			 REED_ANTIRREBOTE_MS sin flancos se lee el pin.
*/
void reed::Flanco( void )
{
	m_cuenta = REED_ANTIRREBOTE_MS;
}

/**
	\brief	SysTick (1 ms): descuenta el antirrebote y, al llegar a 0, toma el estado estable
*/
void reed::inBackGround( void )
{
	if ( m_cuenta == 0 )
		return;

	if ( --m_cuenta == 0 )
	{
		bool abierta = m_pin.GetPin( );
		if ( abierta != m_abierta )
		{
			m_abierta = abierta;
			m_cambio  = true;
		}
	}
}

bool reed::HuboCambio( void )
{
	bool c = m_cambio;
	m_cambio = false;
	return c;
}

/***********************************************************************************************************************************
 *** ISR del PINT: borran el flag del hardware y avisan al objeto. Nada mas.
 **********************************************************************************************************************************/
static void PINT_Atender( uint8_t canal )
{
	PINT->IST = ( 1 << canal );
	if ( g_reed[ canal ] )
		g_reed[ canal ]->Flanco( );
}

void PININT0_IRQHandler( void ) { PINT_Atender( 0 ); }
void PININT1_IRQHandler( void ) { PINT_Atender( 1 ); }
void PININT2_IRQHandler( void ) { PINT_Atender( 2 ); }
void PININT3_IRQHandler( void ) { PINT_Atender( 3 ); }
void PININT4_IRQHandler( void ) { PINT_Atender( 4 ); }
void PININT5_IRQHandler( void ) { PINT_Atender( 5 ); }
