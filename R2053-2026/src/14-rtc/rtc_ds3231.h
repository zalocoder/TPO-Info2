/*******************************************************************************************************************************//**
 *
 * @file		rtc_ds3231.h
 * @brief		Primitiva del RTC DS3231 sobre el driver i2c (no bloqueante).
 * @details		El DS3231 guarda la hora en BCD en los registros 0x00..0x06 y sigue contando con su pila
 *				aunque se corte la alimentacion o se resetee el micro.
 *
 *				Uso:
 *				  rtc.PedirLectura();              // arranca la lectura (por ejemplo, una vez por segundo)
 *				  rtc.Run();                       // en cada vuelta del main
 *				  if ( rtc.HayHoraNueva() ) { const fecha_hora_t &h = rtc.Hora(); ... }
 *				  rtc.PedirEscritura( nueva );     // por ejemplo, al recibir $TIME de la PC
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef RTC_DS3231_H_
#define RTC_DS3231_H_

#include "i2c.h"

#define DS3231_DIRECCION	0x68		// direccion I2C de 7 bits, fija en el integrado

typedef struct
{
	uint8_t		seg;		// 0..59
	uint8_t		min;		// 0..59
	uint8_t		hora;		// 0..23 (se usa siempre el modo 24 h)
	uint8_t		dia;		// 1..31
	uint8_t		mes;		// 1..12
	uint16_t	anio;		// 2000..2099
} fecha_hora_t;

class rtc_ds3231
{
	private:
		enum operacion_t { NINGUNA , LEYENDO , ESCRIBIENDO };

		i2c				&m_bus;
		operacion_t		m_operacion;
		uint8_t			m_tx[ 8 ];				// registro inicial + hasta 7 datos
		uint8_t			m_rx[ 7 ];
		fecha_hora_t	m_hora;
		bool			m_horaNueva;
		bool			m_error;

	public:
		rtc_ds3231( i2c &bus );

		bool				PedirLectura( void );
		bool				PedirEscritura( const fecha_hora_t &fh );
		void				Run( void );

		bool				HayHoraNueva( void );				// true una sola vez por cada lectura terminada
		const fecha_hora_t&	Hora( void ) const { return m_hora; }
		bool				HuboError( void ) const { return m_error; }	// el DS3231 no respondio la ultima vez

	private:
		static uint8_t	BCDaBin( uint8_t bcd ) { return ( bcd >> 4 ) * 10 + ( bcd & 0x0F ); }
		static uint8_t	BinaBCD( uint8_t bin ) { return (uint8_t)( ( ( bin / 10 ) << 4 ) | ( bin % 10 ) ); }
};

#endif /* RTC_DS3231_H_ */
