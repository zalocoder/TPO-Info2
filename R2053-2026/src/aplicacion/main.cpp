/*******************************************************************************************************************************//**
 *
 * @file		main.cpp
 * @brief		SCAI - Sistema de Control de Acceso Inteligente.
 * @details		Estado actual: perifericos configurados, SIN maquina de estados de la puerta (PENDIENTE).
 *				Con PRUEBA_PERIFERICOS en 1 corre una prueba manual (ver prueba_perifericos.h).
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/

/***********************************************************************************************************************************
 *** INCLUDES
 **********************************************************************************************************************************/
#include "inicializarInfotronic.h"
#include "timers.h"
#include "prueba_perifericos.h"

/***********************************************************************************************************************************
 *** DEFINES PRIVADOS AL MODULO
 **********************************************************************************************************************************/
#define PRUEBA_PERIFERICOS		1		// 1: corre la prueba manual. 0: solo inicializa (para cuando exista la MdE)

#define T_LECTURA_RTC_DEC		10		// cada cuanto se lee el DS3231, en decimas de segundo (10 = 1 s)

/***********************************************************************************************************************************
 *** VARIABLES PRIVADAS AL MODULO
 **********************************************************************************************************************************/
// Coleccion de timers cuyos handlers se ejecutan desde el main loop (no desde la ISR del SysTick).
static timers	Timers;
static timer	*pTmrRTC = nullptr;

/***********************************************************************************************************************************
 *** FUNCIONES PRIVADAS AL MODULO
 **********************************************************************************************************************************/
/**
	\brief	Vence una vez por segundo: pide la hora al DS3231 y se relanza (el timer es de un solo disparo)
	\details La lectura en si la va completando rtc->Run() en el main loop, sin bloquear.
*/
static void LecturaRTC_Handler( void )
{
	rtc->PedirLectura( );
	pTmrRTC->TimerStart( T_LECTURA_RTC_DEC );
}

/***********************************************************************************************************************************
 *** FUNCIONES GLOBALES AL MODULO
 **********************************************************************************************************************************/
int main( void )
{
	// Los timers se crean ANTES de InicializarInfotronic() a proposito:
	// 1) Cada timer se engancha en g_CallbackList (lista global de inicializarInfotronic.cpp).
	//    Si fuera una variable global de este archivo, C++ no garantiza que esa lista ya este
	//    construida (orden de inicializacion entre archivos) -> se crea aca, en tiempo de ejecucion.
	// 2) InicializarInfotronic() arranca el SysTick, y su ISR recorre g_CallbackList.
	//    Agregando los timers antes, la ISR nunca recorre la lista mientras se esta modificando.
	// main() no retorna nunca, asi que el objeto local vive todo el programa.
	timer TmrRTC( LecturaRTC_Handler , timer::DEC );
	pTmrRTC = &TmrRTC;
	Timers << TmrRTC;

	InicializarInfotronic( );		// PLL, LEDs, teclado, reed, buzzer, servo, UARTs, I2C/RTC, SysTick y LCD

	TmrRTC.TimerStart( T_LECTURA_RTC_DEC );

#if PRUEBA_PERIFERICOS
	Prueba_Inicio( );
#endif

	while ( 1 )
	{
		Timers.TmrEvent( );			// despacha los timers vencidos
		rtc->Run( );				// avanza la transaccion I2C en curso (si hay)

#if PRUEBA_PERIFERICOS
		Prueba_Run( );
#endif
		// PENDIENTE: PN532_Run();  PC_Run();  WiFi_Run();  MdE_Puerta();
	}

	return 0;
}
