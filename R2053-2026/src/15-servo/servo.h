/*******************************************************************************************************************************//**
 *
 * @file		servo.h
 * @brief		Servo SG90 manejado con el SCTimer en modo PWM (50 Hz, pulso de 1 a 2 ms).
 * @details		Periferico NO visto en la cursada (cuenta como desafio).
 *
 *				Idea: el SCT cuenta de 0 a 19999 a 1 MHz (1 cuenta = 1 us -> periodo de 20 ms = 50 Hz).
 *				  - Evento 0: la cuenta llega a MATCH0 (19999, fin del periodo) -> pone la salida en 1.
 *				  - Evento 1: la cuenta llega a MATCH1 (ancho del pulso)         -> pone la salida en 0.
 *				Cambiar el angulo es solo cambiar MATCH1 (a traves de MATCHREL1, que el SCT copia
 *				al empezar el proximo periodo, asi nunca sale un pulso cortado).
 *				Todo lo hace el hardware: no hay interrupciones ni trabajo de la CPU.
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef SERVO_H_
#define SERVO_H_

#include "LPC845.h"

/***********************************************************************************************************************************
 *** REGISTROS DEL SCT (el LPC845.h de la catedra no los trae; offsets segun UM11029, cap. SCTimer/PWM)
 **********************************************************************************************************************************/
typedef struct
{
	__IO uint32_t CONFIG;				// 0x000 bit0 UNIFY (contador de 32 bits), bit17 AUTOLIMIT_L (MATCH0 reinicia la cuenta)
	__IO uint32_t CTRL;					// 0x004 bit2 HALT_L, bit3 CLRCTR_L, bits 12:5 PRE_L (prescaler)
	__IO uint32_t LIMIT;				// 0x008
	__IO uint32_t HALT;					// 0x00C
	__IO uint32_t STOP;					// 0x010
	__IO uint32_t START;				// 0x014
	     uint32_t RESERVADO0[ 10 ];		// 0x018..0x03C
	__IO uint32_t COUNT;				// 0x040
	__IO uint32_t STATE;				// 0x044
	__I  uint32_t INPUT;				// 0x048
	__IO uint32_t REGMODE;				// 0x04C
	__IO uint32_t OUTPUT;				// 0x050
	__IO uint32_t OUTPUTDIRCTRL;		// 0x054
	__IO uint32_t RES;					// 0x058
	__IO uint32_t DMAREQ0;				// 0x05C
	__IO uint32_t DMAREQ1;				// 0x060
	     uint32_t RESERVADO1[ 35 ];		// 0x064..0x0EC
	__IO uint32_t EVEN;					// 0x0F0
	__IO uint32_t EVFLAG;				// 0x0F4
	__IO uint32_t CONEN;				// 0x0F8
	__IO uint32_t CONFLAG;				// 0x0FC
	__IO uint32_t MATCH[ 8 ];			// 0x100
	     uint32_t RESERVADO2[ 56 ];		// 0x120..0x1FC
	__IO uint32_t MATCHREL[ 8 ];		// 0x200
	     uint32_t RESERVADO3[ 56 ];		// 0x220..0x2FC
	struct {
		__IO uint32_t STATE;			// en que estados esta habilitado el evento
		__IO uint32_t CTRL;				// que dispara el evento
	} EV[ 8 ];							// 0x300
	     uint32_t RESERVADO4[ 112 ];	// 0x340..0x4FC
	struct {
		__IO uint32_t SET;				// eventos que ponen la salida en 1
		__IO uint32_t CLR;				// eventos que ponen la salida en 0
	} OUT[ 7 ];							// 0x500
} SCT_Type;

#define SCT0_BASE				(0x50004000u)
#define SCT0					((SCT_Type *) SCT0_BASE)

/***********************************************************************************************************************************
 *** PARAMETROS
 **********************************************************************************************************************************/
#define SCT_FREQ_ENTRADA		12000000UL	// SCTCLKSEL = FRO (12 MHz). VERIFICAR midiendo 50 Hz con osciloscopio.
#define SERVO_PERIODO_US		20000		// 50 Hz
#define SERVO_PULSO_MIN_US		1000		// 1 ms -> 0 grados   (ajustar con el servo real: muchos SG90 usan 0,5..2,4 ms)
#define SERVO_PULSO_MAX_US		2000		// 2 ms -> 180 grados

/***********************************************************************************************************************************
 *** CLASE
 **********************************************************************************************************************************/
class servo
{
	public:
		servo( uint8_t port , uint8_t pin );

		void	Pulso( uint32_t us );			// ancho de pulso directo, en microsegundos
		void	Angulo( uint8_t grados );		// 0..180
};

#endif /* SERVO_H_ */
