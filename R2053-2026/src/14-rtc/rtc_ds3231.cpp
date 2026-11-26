/*******************************************************************************************************************************//**
 *
 * @file		rtc_ds3231.cpp
 * @brief		Primitiva del RTC DS3231 (ver rtc_ds3231.h)
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#include "rtc_ds3231.h"

rtc_ds3231::rtc_ds3231( i2c &bus ) : m_bus( bus )
{
	m_operacion = NINGUNA;
	m_horaNueva = false;
	m_error		= false;
	m_hora.seg = m_hora.min = m_hora.hora = 0;
	m_hora.dia = m_hora.mes = 1;
	m_hora.anio = 2000;
}

/**
	\brief	Arranca la lectura de los registros 0x00..0x06 (segundos a anio)
	\return	false si el bus esta ocupado; se reintenta en la proxima llamada
*/
bool rtc_ds3231::PedirLectura( void )
{
	if ( m_operacion != NINGUNA )
		return false;

	m_tx[ 0 ] = 0x00;						// "quiero leer a partir del registro 0"
	if ( !m_bus.Transferir( DS3231_DIRECCION , m_tx , 1 , m_rx , 7 ) )
		return false;

	m_operacion = LEYENDO;
	return true;
}

/**
	\brief	Pone en hora el DS3231. Escribe los 7 registros de una sola vez para que no quede una hora a medias.
*/
bool rtc_ds3231::PedirEscritura( const fecha_hora_t &fh )
{
	if ( m_operacion != NINGUNA )
		return false;

	m_tx[ 0 ] = 0x00;								// registro inicial
	m_tx[ 1 ] = BinaBCD( fh.seg );					// 0x00 segundos (bit 7 en 0)
	m_tx[ 2 ] = BinaBCD( fh.min );					// 0x01 minutos
	m_tx[ 3 ] = BinaBCD( fh.hora );					// 0x02 horas, bit 6 = 0 -> modo 24 h
	m_tx[ 4 ] = 1;									// 0x03 dia de la semana: el SCAI no lo usa
	m_tx[ 5 ] = BinaBCD( fh.dia );					// 0x04 dia del mes
	m_tx[ 6 ] = BinaBCD( fh.mes );					// 0x05 mes, bit 7 (siglo) = 0
	m_tx[ 7 ] = BinaBCD( (uint8_t)( fh.anio % 100 ) );	// 0x06 anio 00..99

	if ( !m_bus.Transferir( DS3231_DIRECCION , m_tx , 8 , nullptr , 0 ) )
		return false;

	m_operacion = ESCRIBIENDO;
	return true;
}

/**
	\brief	Hace avanzar el bus y, cuando termina una lectura, convierte de BCD a binario
*/
void rtc_ds3231::Run( void )
{
	m_bus.Run( );

	if ( m_operacion == NINGUNA )
		return;

	i2c::estado_t e = m_bus.Estado( );
	if ( e == i2c::OCUPADO )
		return;

	m_error = ( e == i2c::ERROR );

	if ( m_operacion == LEYENDO && !m_error )
	{
		m_hora.seg	= BCDaBin( m_rx[ 0 ] & 0x7F );
		m_hora.min	= BCDaBin( m_rx[ 1 ] & 0x7F );
		m_hora.hora	= BCDaBin( m_rx[ 2 ] & 0x3F );		// en modo 24 h los bits 5:0 son la hora
		m_hora.dia	= BCDaBin( m_rx[ 4 ] & 0x3F );
		m_hora.mes	= BCDaBin( m_rx[ 5 ] & 0x1F );		// bit 7 = siglo, se ignora
		m_hora.anio	= 2000 + BCDaBin( m_rx[ 6 ] );
		m_horaNueva = true;
	}

	m_bus.Liberar( );
	m_operacion = NINGUNA;
}

bool rtc_ds3231::HayHoraNueva( void )
{
	bool h = m_horaNueva;
	m_horaNueva = false;
	return h;
}
