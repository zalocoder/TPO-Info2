/*******************************************************************************************************************************//**
 *
 * @file		pinout.h
 * @brief		SCAI - Asignacion UNICA de pines y parametros de hardware.
 * @details		Todo pin que use el firmware se define aca y en ningun otro lado.
 *				Si hay que mover algo, se cambia solo este archivo.
 *
 *				ESTADO: PROVISORIO. Armado a partir de lo que ya usa inicializarInfotronic.cpp (KIT2022)
 *				y del manual de la LPC845-BRK. Los pines marcados [VERIFICAR] hay que confirmarlos
 *				con los esquematicos de la BRK y de la Infotronic antes de conectar nada.
 *
 *				Pines prohibidos: PIO0_2 / PIO0_3 (SWD, depurador) y PIO0_5 (RESET).
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef PINOUT_H_
#define PINOUT_H_

/***********************************************************************************************************************************
 *** Convencion: cada senial tiene un _PORT (0 o 1) y un _PIN (numero de bit).
 **********************************************************************************************************************************/

// ---------------------------------------------------------------------------------------------------------------------------------
// LCD 16x2 (Infotronic, pines fijos de la placa, modo 4 bits, RW a GND)
// ---------------------------------------------------------------------------------------------------------------------------------
#define LCD_D4_PORT			0
#define LCD_D4_PIN			9
#define LCD_D5_PORT			0
#define LCD_D5_PIN			10
#define LCD_D6_PORT			0
#define LCD_D6_PIN			11
#define LCD_D7_PORT			0
#define LCD_D7_PIN			13
#define LCD_RS_PORT			0
#define LCD_RS_PIN			1
#define LCD_EN_PORT			0
#define LCD_EN_PIN			14

// ---------------------------------------------------------------------------------------------------------------------------------
// Teclado matricial 4x4 externo                                                                     [VERIFICAR]
// No alcanzan los pines libres, asi que se reutilizan las lineas del display de 7 segmentos de la
// Infotronic (PIO0_18..23), que el SCAI no usa. Esas lineas llegan a ENTRADAS del 4511/4017,
// por eso:  - las FILAS son salidas del micro (manejar una entrada CMOS no molesta),
//           - las COLUMNAS son entradas con pull-up (el 4511 no maneja esas lineas).
// Efecto secundario: el display de 7 segmentos puede mostrar basura. No se usa en el TPO.
// ---------------------------------------------------------------------------------------------------------------------------------
#define TEC_FIL0_PORT		0
#define TEC_FIL0_PIN		18
#define TEC_FIL1_PORT		0
#define TEC_FIL1_PIN		19
#define TEC_FIL2_PORT		0
#define TEC_FIL2_PIN		20
#define TEC_FIL3_PORT		0
#define TEC_FIL3_PIN		21
#define TEC_COL0_PORT		0
#define TEC_COL0_PIN		22
#define TEC_COL1_PORT		0
#define TEC_COL1_PIN		23
#define TEC_COL2_PORT		0
#define TEC_COL2_PIN		29
#define TEC_COL3_PORT		0
#define TEC_COL3_PIN		7

// ---------------------------------------------------------------------------------------------------------------------------------
// LEDs de acceso: se usa el LED RGB de la BRK (activo en BAJO)                                      [VERIFICAR]
// ---------------------------------------------------------------------------------------------------------------------------------
#define LED_VERDE_PORT		1
#define LED_VERDE_PIN		0
#define LED_AZUL_PORT		1		// "latido": parpadea para mostrar que el programa corre
#define LED_AZUL_PIN		1
#define LED_ROJO_PORT		1
#define LED_ROJO_PIN		2

// ---------------------------------------------------------------------------------------------------------------------------------
// Buzzer de la Infotronic                                                       [PENDIENTE: pin real segun esquematico]
// Hasta ver el esquematico se deja en PIO0_0 (pin libre) para que el codigo compile.
// ---------------------------------------------------------------------------------------------------------------------------------
#define BUZZER_PORT			0
#define BUZZER_PIN			0
#define BUZZER_ACTIVO_ALTO	1		// 1: suena con el pin en 1. Cambiar a 0 si el transistor invierte.

// ---------------------------------------------------------------------------------------------------------------------------------
// Reed switch MC-38 (puerta). Entrada con pull-up + interrupcion PINT por ambos flancos.
// Con el iman cerca (puerta cerrada) el contacto cierra a GND -> pin en 0.                          [VERIFICAR]
// ---------------------------------------------------------------------------------------------------------------------------------
#define REED_PORT			0
#define REED_PIN			15
#define REED_CANAL_PINT		0		// canales 6 y 7 NO: comparten interrupcion con USART3/USART4

// ---------------------------------------------------------------------------------------------------------------------------------
// Servo SG90: salida PWM del SCTimer (SCT_OUT0, asignada por Switch Matrix)                         [VERIFICAR]
// ---------------------------------------------------------------------------------------------------------------------------------
#define SERVO_PORT			1
#define SERVO_PIN			3

// ---------------------------------------------------------------------------------------------------------------------------------
// USART0 -> PC por el puente USB-VCOM de la BRK                                                     [VERIFICAR]
// ---------------------------------------------------------------------------------------------------------------------------------
#define PC_UART_NUM			0
#define PC_TX_PORT			0
#define PC_TX_PIN			25
#define PC_RX_PORT			0
#define PC_RX_PIN			24
#define PC_BAUDIOS			115200

// ---------------------------------------------------------------------------------------------------------------------------------
// USART1 -> PN532 en modo HSU (llaves del modulo en UART: SEL0 = 0, SEL1 = 0)                       [VERIFICAR]
// TX del micro va a RX (SDA/RXD) del PN532 y viceversa.
// ---------------------------------------------------------------------------------------------------------------------------------
#define NFC_UART_NUM		1
#define NFC_TX_PORT			1
#define NFC_TX_PIN			8
#define NFC_RX_PORT			1
#define NFC_RX_PIN			9
#define NFC_BAUDIOS			115200

// ---------------------------------------------------------------------------------------------------------------------------------
// USART2 -> ESP8266 / ESP-01 (WiFi hacia la PC). El ESP-01 trabaja a 3,3 V: NO alimentarlo de 5 V.  [VERIFICAR]
// ---------------------------------------------------------------------------------------------------------------------------------
#define WIFI_UART_NUM		2
#define WIFI_TX_PORT		1
#define WIFI_TX_PIN			6
#define WIFI_RX_PORT		1
#define WIFI_RX_PIN			7
#define WIFI_BAUDIOS		115200	// valor de fabrica de la mayoria de los ESP-01 con firmware AT

// ---------------------------------------------------------------------------------------------------------------------------------
// I2C1 -> RTC DS3231 (asignado por Switch Matrix).                                                  [VERIFICAR]
// No se usa I2C0 porque sus pines fijos (PIO0_10 y PIO0_11) son D5 y D6 del LCD.
// El modulo DS3231 ya trae resistencias de pull-up: en el micro el pin queda en open-drain sin pull-up.
// ---------------------------------------------------------------------------------------------------------------------------------
#define RTC_I2C_NUM			1
#define RTC_SDA_PORT		1
#define RTC_SDA_PIN			4
#define RTC_SCL_PORT		1
#define RTC_SCL_PIN			5

#endif /* PINOUT_H_ */
