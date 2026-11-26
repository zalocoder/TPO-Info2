/*******************************************************************************************************************************//**
 *
 * @file		servo.cpp
 * @brief		Servo SG90 con SCT en modo PWM (ver servo.h)
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#include "servo.h"
#include "swm.h"

#define SCT_BIT_CLK			8					// bit del SCT en SYSAHBCLKCTRL0 y PRESETCTRL0
#define SCT_PRESCALER		( SCT_FREQ_ENTRADA / 1000000UL - 1 )	// 12 MHz / 12 = 1 MHz -> 1 cuenta = 1 us

// CTRL
#define CTRL_HALT			( 1 << 2 )
#define CTRL_CLRCTR			( 1 << 3 )
#define CTRL_PRE(x)			( (x) << 5 )
// CONFIG
#define CONFIG_UNIFY		( 1 << 0 )
#define CONFIG_AUTOLIMIT	( 1 << 17 )
// EV[n].CTRL
#define EV_MATCHSEL(m)		( (m) << 0 )
#define EV_SOLO_MATCH		( 1 << 12 )			// COMBMODE = 1: el evento depende solo del match

/**
	\brief	Configura SCT0 como PWM de 50 Hz en SCT_OUT0 y deja el servo en 0 grados
*/
servo::servo( uint8_t port , uint8_t pin )
{
	SYSCON->SYSAHBCLKCTRL0 |= ( 1 << 7 );					// SWM
	SYSCON->SYSAHBCLKCTRL0 |= ( 1 << SCT_BIT_CLK );			// clock del SCT
	SYSCON->PRESETCTRL0 &= ~( 1 << SCT_BIT_CLK );
	SYSCON->PRESETCTRL0 |=  ( 1 << SCT_BIT_CLK );
	SYSCON->SCTCLKSEL = 0;									// fuente: FRO
	SYSCON->SCTCLKDIV = 1;									// sin division (0 apagaria el clock)

	PINASSIGN_Config( PA_SCT_OUT0 , port , pin );			// SCT_OUT0 sale por el pin elegido

	SCT0->CONFIG = CONFIG_UNIFY | CONFIG_AUTOLIMIT;			// un solo contador de 32 bits; MATCH0 = limite
	SCT0->CTRL	 = CTRL_HALT | CTRL_CLRCTR | CTRL_PRE( SCT_PRESCALER );	// detenido mientras se configura

	SCT0->MATCH[ 0 ]	= SERVO_PERIODO_US - 1;				// fin de periodo
	SCT0->MATCHREL[ 0 ]	= SERVO_PERIODO_US - 1;
	SCT0->MATCH[ 1 ]	= SERVO_PULSO_MIN_US;				// ancho de pulso inicial
	SCT0->MATCHREL[ 1 ]	= SERVO_PULSO_MIN_US;

	SCT0->EV[ 0 ].STATE = 1;								// habilitado en el estado 0 (el unico que se usa)
	SCT0->EV[ 0 ].CTRL	= EV_MATCHSEL( 0 ) | EV_SOLO_MATCH;
	SCT0->EV[ 1 ].STATE = 1;
	SCT0->EV[ 1 ].CTRL	= EV_MATCHSEL( 1 ) | EV_SOLO_MATCH;

	SCT0->OUT[ 0 ].SET	= ( 1 << 0 );						// evento 0 (inicio de periodo) -> salida en 1
	SCT0->OUT[ 0 ].CLR	= ( 1 << 1 );						// evento 1 (fin del pulso)     -> salida en 0
	SCT0->OUTPUT		= 0;

	SCT0->CTRL &= ~CTRL_HALT;								// arranca
}

/**
	\brief	Cambia el ancho de pulso. Se aplica al comenzar el proximo periodo (via MATCHREL).
*/
void servo::Pulso( uint32_t us )
{
	if ( us < SERVO_PULSO_MIN_US ) us = SERVO_PULSO_MIN_US;
	if ( us > SERVO_PULSO_MAX_US ) us = SERVO_PULSO_MAX_US;
	SCT0->MATCHREL[ 1 ] = us;
}

/**
	\brief	Posiciona el servo en un angulo de 0 a 180 grados (interpolacion lineal entre MIN y MAX)
*/
void servo::Angulo( uint8_t grados )
{
	if ( grados > 180 )
		grados = 180;
	Pulso( SERVO_PULSO_MIN_US + ( (uint32_t) grados * ( SERVO_PULSO_MAX_US - SERVO_PULSO_MIN_US ) ) / 180 );
}
