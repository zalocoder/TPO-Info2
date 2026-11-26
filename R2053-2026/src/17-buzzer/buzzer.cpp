/*******************************************************************************************************************************//**
 *
 * @file		buzzer.cpp
 * @brief		Buzzer con patrones de pitidos (ver buzzer.h)
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#include "buzzer.h"

// La clase gpio resuelve la polaridad: SetPin() = sonar, ClrPin() = callar, sea activo alto o bajo.
buzzer::buzzer( uint8_t port , uint8_t pin , bool activoAlto )
	: m_pin( port , pin , gpio::pushpull , gpio::output , activoAlto ? gpio::high : gpio::low )
{
	m_restantes = 0;
	m_continuo	= false;
	m_sonando	= false;
	m_cuenta	= 0;
	m_msOn = m_msOff = 0;
	m_pin.ClrPin( );
	g_CallbackList.push_back( this );
}

/**
	\brief	Arranca una serie de pitidos. Si ya habia un patron sonando, lo reemplaza.
*/
void buzzer::Pitidos( uint8_t cantidad , uint16_t msOn , uint16_t msOff )
{
	if ( cantidad == 0 || msOn == 0 )
	{
		Apagar( );
		return;
	}
	m_continuo	= false;
	m_msOn		= msOn;
	m_msOff		= msOff;
	m_restantes	= cantidad;
	m_sonando	= true;
	m_cuenta	= msOn;				// se escribe ultimo: recien ahi el SysTick empieza a contar
	m_pin.SetPin( );
}

void buzzer::Continuo( void )
{
	m_restantes = 0;
	m_cuenta	= 0;
	m_continuo	= true;
	m_pin.SetPin( );
}

void buzzer::Apagar( void )
{
	m_continuo	= false;
	m_restantes = 0;
	m_cuenta	= 0;
	m_sonando	= false;
	m_pin.ClrPin( );
}

/**
	\brief	SysTick: alterna entre sonar (msOn) y callar (msOff) hasta completar la cantidad pedida
*/
void buzzer::inBackGround( void )
{
	if ( m_continuo || m_cuenta == 0 )
		return;

	if ( --m_cuenta )
		return;

	if ( m_sonando )
	{
		m_pin.ClrPin( );
		m_sonando = false;
		if ( --m_restantes && m_msOff )
			m_cuenta = m_msOff;			// queda otro pitido: primero el silencio
		else
			m_restantes = 0;			// era el ultimo (o no hay silencio definido): fin
	}
	else
	{
		m_pin.SetPin( );
		m_sonando = true;
		m_cuenta  = m_msOn;
	}
}
