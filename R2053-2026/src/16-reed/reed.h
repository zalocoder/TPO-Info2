/*******************************************************************************************************************************//**
 *
 * @file		reed.h
 * @brief		Sensor de puerta (reed switch MC-38) con interrupcion por flanco (PINT) y antirrebote.
 * @details		- PINT: cualquier flanco del pin dispara una interrupcion. La ISR solo levanta un flag
 *				  y arranca la cuenta del antirrebote (regla de "ISR minima").
 *				- Antirrebote: el reed rebota al abrir/cerrar. Despues del ultimo flanco se esperan
 *				  REED_ANTIRREBOTE_MS sin cambios (contados en el SysTick) y recien ahi se lee el pin.
 *				- La aplicacion consulta Abierta() y HuboCambio() desde el main loop.
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef REED_H_
#define REED_H_

#include "LPC845.h"
#include "callback.h"
#include "systick.h"
#include "gpio.h"

/***********************************************************************************************************************************
 *** REGISTROS DEL PINT (el LPC845.h de la catedra no los trae; offsets segun UM11029, cap. Pin interrupt)
 **********************************************************************************************************************************/
typedef struct
{
	__IO uint32_t ISEL;		// 0x00 0 = por flanco, 1 = por nivel
	__IO uint32_t IENR;		// 0x04 habilitacion flanco ascendente
	__O  uint32_t SIENR;	// 0x08 set   de IENR
	__O  uint32_t CIENR;	// 0x0C clear de IENR
	__IO uint32_t IENF;		// 0x10 habilitacion flanco descendente
	__O  uint32_t SIENF;	// 0x14 set   de IENF
	__O  uint32_t CIENF;	// 0x18 clear de IENF
	__IO uint32_t RISE;		// 0x1C hubo flanco ascendente (se borra escribiendo 1)
	__IO uint32_t FALL;		// 0x20 hubo flanco descendente
	__IO uint32_t IST;		// 0x24 estado de la interrupcion (escribir 1 la borra)
} PINT_Type;

#define PINT_BASE				(0xA0004000u)
#define PINT					((PINT_Type *) PINT_BASE)
#define PININT0_IRQn			24			// numero de interrupcion del canal 0 en el NVIC

#define REED_ANTIRREBOTE_MS		30

/***********************************************************************************************************************************
 *** CLASE
 **********************************************************************************************************************************/
class reed : public Callback
{
	private:
		gpio				m_pin;
		uint8_t				m_canal;
		volatile uint16_t	m_cuenta;		// ms que faltan para dar por estable la senial
		volatile bool		m_abierta;
		volatile bool		m_cambio;

	public:
		// canal: 0..5 (6 y 7 comparten interrupcion con USART3 y USART4)
		reed( uint8_t port , uint8_t pin , uint8_t canal );

		bool	Abierta( void ) const { return m_abierta; }
		bool	HuboCambio( void );		// true una sola vez despues de cada cambio estable

		void	Flanco( void );			// la llama la ISR del PINT
		void	inBackGround( void );	// la llama el SysTick cada 1 ms
};

#if defined (__cplusplus)
extern "C" {
	void PININT0_IRQHandler( void );
	void PININT1_IRQHandler( void );
	void PININT2_IRQHandler( void );
	void PININT3_IRQHandler( void );
	void PININT4_IRQHandler( void );
	void PININT5_IRQHandler( void );
}
#endif

#endif /* REED_H_ */
