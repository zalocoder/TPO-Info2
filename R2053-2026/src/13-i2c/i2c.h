/*******************************************************************************************************************************//**
 *
 * @file		i2c.h
 * @brief		Driver I2C maestro NO bloqueante para el LPC845 (I2C1..I2C3, pines por Switch Matrix).
 * @details		Periferico NO visto en la cursada (cuenta como desafio).
 *
 *				Como se usa:
 *				  1) i2c bus( 1 , portSDA , pinSDA , portSCL , pinSCL );     // configura y deja el bus listo
 *				  2) bus.Transferir( dir7 , tx , ntx , rx , nrx );          // arranca una transaccion
 *				  3) en cada vuelta del main: bus.Run();                     // avanza la transaccion
 *				  4) bus.Estado() pasa de OCUPADO a TERMINADO o ERROR.
 *
 *				Transferir() hace "escribir ntx bytes" y despues, con un START repetido, "leer nrx bytes".
 *				Es justo lo que pide el DS3231: se escribe el registro desde donde leer y despues se lee.
 *
 *				No usa interrupciones: Run() mira el bit MSTPENDING y avanza UN paso por llamada.
 *				A 100 kHz un byte tarda ~90 us, asi que el main loop nunca queda esperando.
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef I2C_H_
#define I2C_H_

#include "LPC845.h"

/***********************************************************************************************************************************
 *** REGISTROS (el LPC845.h de la catedra no trae el I2C, se definen aca segun el manual UM11029, cap. I2C)
 **********************************************************************************************************************************/
typedef struct
{
	__IO uint32_t CFG;			// 0x00 configuracion: bit0 MSTEN habilita el modo maestro
	__IO uint32_t STAT;			// 0x04 estado: bit0 MSTPENDING, bits 3:1 MSTSTATE, bit4 MSTARBLOSS, bit6 MSTSTSTPERR
	__IO uint32_t INTENSET;		// 0x08
	__O  uint32_t INTENCLR;		// 0x0C
	__IO uint32_t TIMEOUT;		// 0x10
	__IO uint32_t CLKDIV;		// 0x14 divisor del clock del I2C
	__I  uint32_t INTSTAT;		// 0x18
	     uint32_t RESERVADO0;	// 0x1C
	__IO uint32_t MSTCTL;		// 0x20 control: bit0 MSTCONTINUE, bit1 MSTSTART, bit2 MSTSTOP
	__IO uint32_t MSTTIME;		// 0x24 tiempo en alto y en bajo de SCL
	__IO uint32_t MSTDAT;		// 0x28 dato a enviar / dato recibido
} I2C_Type;

#define I2C1_BASE				(0x40054000u)
#define I2C2_BASE				(0x40030000u)
#define I2C3_BASE				(0x40034000u)

/***********************************************************************************************************************************
 *** CLASE
 **********************************************************************************************************************************/
class i2c
{
	public:
		enum estado_t { LIBRE , OCUPADO , TERMINADO , ERROR };

	private:
		// Pasos internos de la maquina de estados de una transaccion
		enum paso_t { P_REPOSO , P_DIR_ESCRITURA , P_ESCRIBIENDO , P_DIR_LECTURA , P_LEYENDO };

		I2C_Type 			*m_i2c;
		volatile estado_t	m_estado;
		paso_t				m_paso;

		uint8_t				m_dir;			// direccion de 7 bits del esclavo
		const uint8_t		*m_tx;			// datos a escribir (los provee quien llama)
		uint8_t				m_ntx;
		uint8_t				*m_rx;			// donde dejar lo leido
		uint8_t				m_nrx;
		uint8_t				m_idx;			// cuantos bytes van escritos / leidos

	public:
		i2c( uint8_t num , uint8_t portSDA , uint8_t pinSDA , uint8_t portSCL , uint8_t pinSCL );

		bool		Transferir( uint8_t dir7 , const uint8_t *tx , uint8_t ntx , uint8_t *rx , uint8_t nrx );
		void		Run( void );
		estado_t	Estado( void ) const { return m_estado; }
		void		Liberar( void );		// despues de leer el resultado (TERMINADO o ERROR) se vuelve a LIBRE

	private:
		void		Stop( estado_t final );
		static void	PinOpenDrain( uint8_t port , uint8_t pin );
};

#endif /* I2C_H_ */
