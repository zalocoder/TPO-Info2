/*******************************************************************************************************************************//**
 *
 * @file		inicializarInfotronic.h
 * @brief		SCAI - Inicializacion de la placa (cumple el rol de Board_Init()).
 * @details		Crea y configura todos los perifericos del nucleo del SCAI y deja los objetos
 *				disponibles para la aplicacion. Los pines salen de pinout.h.
 *				Basado en el inicializarInfotronic de la catedra (Ing. Marcelo Trujillo).
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef INICIALIZAR_H_
#define INICIALIZAR_H_

/***********************************************************************************************************************************
 *** INCLUDES GLOBALES
 **********************************************************************************************************************************/
#include <vector>
#include <list>

#include "LPC845.h"
#include "dr_pll.h"
#include "systick.h"
#include "gpio.h"
#include "timer.h"
#include "teclado.h"
#include "Lcd.h"
#include "Led.h"
#include "uart.h"
#include "i2c.h"
#include "rtc_ds3231.h"
#include "servo.h"
#include "reed.h"
#include "buzzer.h"
#include "pinout.h"

using namespace std;

/***********************************************************************************************************************************
 *** DEFINES GLOBALES
 **********************************************************************************************************************************/
// Numero de cada LED dentro de VectorDeLeds (lo usa Callback_Leds_gpio)
#define LED_N_VERDE		0
#define LED_N_AZUL		1
#define LED_N_ROJO		2

/***********************************************************************************************************************************
 *** VARIABLES GLOBALES (objetos de la placa, creados en InicializarInfotronic)
 **********************************************************************************************************************************/
extern teclado		*Teclado;		// 4x4. GetKey() devuelve 0..15 o NO_KEY; traducir con TECLAS_4x4[]
extern Lcd			*lcd;			// 16x2. lcd->Set( "texto" , fila , columna )
extern Led			LedVerde;		// acceso aceptado
extern Led			LedRojo;		// acceso rechazado
extern Led			LedAzul;		// latido: parpadea si el programa esta corriendo
extern uart			*uartPC;		// USART0 -> PC por USB (VCOM)
extern uart			*uartNFC;		// USART1 -> PN532 (HSU)
extern uart			*uartWiFi;		// USART2 -> ESP8266
extern i2c			*busI2C;		// I2C1
extern rtc_ds3231	*rtc;			// DS3231 sobre busI2C
extern servo		*Servo;			// SG90 en SCT_OUT0
extern reed			*Puerta;		// reed switch MC-38 con PINT
extern buzzer		*Buzzer;

extern const char	TECLAS_4x4[ 16 ];	// codigo de tecla (0..15) -> caracter impreso en el teclado

/***********************************************************************************************************************************
 *** PROTOTIPOS GLOBALES
 **********************************************************************************************************************************/
void InicializarInfotronic ( void ) ;
void systick_callback ( void  );
void Callback_Leds_gpio ( uint8_t nLed , Led::led_t estado );

#endif /* INICIALIZAR_H_ */
