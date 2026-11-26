/*******************************************************************************************************************************//**
 *
 * @file		buzzer.h
 * @brief		Buzzer con patrones de pitidos, sin bloquear (lo maneja el SysTick).
 * @details		Se asume un buzzer ACTIVO (suena con solo darle tension). Con un buzzer activo no se
 *				puede cambiar el tono, asi que "aceptado" y "rechazado" se distinguen por el patron:
 *				  - aceptado:  1 pitido largo        -> Pitidos( 1 , 300 , 0 )
 *				  - rechazado: 3 pitidos cortos      -> Pitidos( 3 , 80 , 80 )
 *				  - alarma:    continuo              -> Continuo()
 *				[PENDIENTE] Confirmar con el esquematico de la Infotronic si el buzzer es activo o pasivo.
 *				Si es pasivo habria que generar la frecuencia con el SCT (otra salida del mismo SCT0).
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef BUZZER_H_
#define BUZZER_H_

#include "callback.h"
#include "systick.h"
#include "gpio.h"

class buzzer : public Callback
{
	private:
		gpio				m_pin;
		volatile uint8_t	m_restantes;	// pitidos que faltan (0 = quieto)
		volatile bool		m_continuo;
		volatile bool		m_sonando;
		volatile uint16_t	m_cuenta;		// ms que faltan para el proximo cambio
		uint16_t			m_msOn;
		uint16_t			m_msOff;

	public:
		buzzer( uint8_t port , uint8_t pin , bool activoAlto );

		void	Pitidos( uint8_t cantidad , uint16_t msOn , uint16_t msOff );
		void	Continuo( void );
		void	Apagar( void );
		void	inBackGround( void );		// SysTick, cada 1 ms
};

#endif /* BUZZER_H_ */
