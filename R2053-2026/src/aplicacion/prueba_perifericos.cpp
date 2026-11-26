/*******************************************************************************************************************************//**
 *
 * @file		prueba_perifericos.cpp
 * @brief		SCAI - Prueba manual de perifericos (ver prueba_perifericos.h)
 * @details		No usa sprintf a proposito: la version de newlib ocupa varios KB de flash y el
 *				proyecto ya usa ~2/3 de los 64 KB. Se arman los textos con funciones chicas.
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#include "prueba_perifericos.h"
#include "inicializarInfotronic.h"

/***********************************************************************************************************************************
 *** TABLAS PRIVADAS
 **********************************************************************************************************************************/
// PN532 por HSU: preambulo para despertarlo + comando GetFirmwareVersion (0x02).
// Trama: 00 00 FF LEN LCS D4 CMD DCS 00   con LEN = 2 (D4 y CMD), LCS = 0x100 - LEN, DCS = 0x100 - (D4 + CMD)
static const uint8_t PN532_GET_VERSION[] = {
	0x55 , 0x55 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 ,	// despertar
	0x00 , 0x00 , 0xFF , 0x02 , 0xFE , 0xD4 , 0x02 , 0x2A , 0x00
};

/***********************************************************************************************************************************
 *** FUNCIONES PRIVADAS
 **********************************************************************************************************************************/
// Escribe n en dos digitos decimales (con cero a la izquierda) en dst
static void Dos_Digitos( char *dst , uint8_t n )
{
	dst[ 0 ] = '0' + ( n / 10 ) % 10;
	dst[ 1 ] = '0' + n % 10;
}

// Lee dos digitos decimales. Devuelve false si alguno no es un digito.
static bool Leer_Dos( const char *src , uint8_t &n )
{
	if ( src[ 0 ] < '0' || src[ 0 ] > '9' || src[ 1 ] < '0' || src[ 1 ] > '9' )
		return false;
	n = ( src[ 0 ] - '0' ) * 10 + ( src[ 1 ] - '0' );
	return true;
}

// Manda a la PC un byte como dos caracteres hexadecimales y un espacio
static void PC_Hex( uint8_t b )
{
	static const char HEX[] = "0123456789ABCDEF";
	char t[ 4 ] = { HEX[ b >> 4 ] , HEX[ b & 0x0F ] , ' ' , 0 };
	uartPC->Transmit( t );
}

/**
	\brief	Interpreta "$TIME,dd/mm/aaaa,hh:mm:ss" y pone en hora el DS3231
	\details Version minima solo para la prueba. El parser real de tramas de la PC esta PENDIENTE.
*/
static void PC_Linea( const char *l )
{
	// posiciones:            0123456789012345678901234
	//                        $TIME,dd/mm/aaaa,hh:mm:ss
	const char *p = "$TIME,";
	for ( uint8_t i = 0 ; p[ i ] ; i++ )
		if ( l[ i ] != p[ i ] )
		{
			uartPC->Transmit( "$NAK,?\r\n" );
			return;
		}

	fecha_hora_t fh;
	uint8_t a1 , a2;
	if ( Leer_Dos( l + 6 , fh.dia ) && Leer_Dos( l + 9 , fh.mes ) && Leer_Dos( l + 12 , a1 ) && Leer_Dos( l + 14 , a2 ) &&
		 Leer_Dos( l + 17 , fh.hora ) && Leer_Dos( l + 20 , fh.min ) && Leer_Dos( l + 23 , fh.seg ) )
	{
		fh.anio = a1 * 100 + a2;
		if ( rtc->PedirEscritura( fh ) )
			uartPC->Transmit( "$ACK,TIME\r\n" );
		else
			uartPC->Transmit( "$NAK,TIME,RTC ocupado\r\n" );
	}
	else
		uartPC->Transmit( "$NAK,TIME,formato\r\n" );
}

/***********************************************************************************************************************************
 *** FUNCIONES GLOBALES
 **********************************************************************************************************************************/
void Prueba_Inicio( void )
{
	lcd->Set( "SCAI  prueba    " , 0 , 0 );
	lcd->Set( "perifericos     " , 1 , 0 );
	uartPC->Transmit( "\r\nSCAI - prueba de perifericos\r\n" );
}

void Prueba_Run( void )
{
	// ---- Teclado --------------------------------------------------------------------------------
	uint8_t codigo = Teclado->GetKey( );
	if ( codigo != NO_KEY && codigo < 16 )
	{
		char c = TECLAS_4x4[ codigo ];
		char txt[] = "Tecla: x        ";
		txt[ 7 ] = c;
		lcd->Set( txt , 1 , 0 );

		char pc[] = "TEC x\r\n";
		pc[ 4 ] = c;
		uartPC->Transmit( pc );

		switch ( c )
		{
			case 'A':	Servo->Angulo( 90 );	LedVerde = Led::ON;		Buzzer->Pitidos( 1 , 300 , 0 );		break;
			case 'B':	Servo->Angulo( 0 );		LedVerde = Led::OFF;										break;
			case 'C':	LedRojo = Led::ON;		Buzzer->Pitidos( 3 , 80 , 80 );								break;
			case 'D':	LedRojo = Led::OFF;		Buzzer->Apagar( );											break;
			case '*':	uartNFC->Transmit( (void *) PN532_GET_VERSION , sizeof( PN532_GET_VERSION ) );	break;
			case '#':	uartWiFi->Transmit( "AT\r\n" );													break;
			default:	break;
		}
	}

	// ---- Reed (puerta) --------------------------------------------------------------------------
	if ( Puerta->HuboCambio( ) )
	{
		if ( Puerta->Abierta( ) )
		{
			lcd->Set( "Puerta ABIERTA  " , 1 , 0 );
			uartPC->Transmit( "REED abierta\r\n" );
		}
		else
		{
			lcd->Set( "Puerta CERRADA  " , 1 , 0 );
			uartPC->Transmit( "REED cerrada\r\n" );
		}
	}

	// ---- RTC: hora en la primera fila del LCD ----------------------------------------------------
	static bool errorAnterior = false;
	if ( rtc->HayHoraNueva( ) )
	{
		const fecha_hora_t &h = rtc->Hora( );
		char txt[] = "dd/mm hh:mm:ss  ";
		Dos_Digitos( txt + 0 , h.dia );
		Dos_Digitos( txt + 3 , h.mes );
		Dos_Digitos( txt + 6 , h.hora );
		Dos_Digitos( txt + 9 , h.min );
		Dos_Digitos( txt + 12 , h.seg );
		lcd->Set( txt , 0 , 0 );
		errorAnterior = false;
	}
	else if ( rtc->HuboError( ) && !errorAnterior )
	{
		lcd->Set( "RTC sin respuest" , 0 , 0 );
		uartPC->Transmit( "RTC: el DS3231 no responde\r\n" );
		errorAnterior = true;
	}

	// ---- PN532: todo lo que llega se reenvia a la PC en hexa ------------------------------------
	uint8_t b;
	if ( uartNFC->Receive( b ) )
	{
		uartPC->Transmit( "NFC< " );
		do
			PC_Hex( b );
		while ( uartNFC->Receive( b ) );
		uartPC->Transmit( "\r\n" );
	}

	// ---- ESP8266: lineas completas a la PC --------------------------------------------------------
	char linea[ 64 ];
	if ( uartWiFi->Receive( linea , sizeof( linea ) ) )
	{
		uartPC->Transmit( "WIFI< " );
		uartPC->Transmit( linea );
		uartPC->Transmit( "\r\n" );
	}

	// ---- PC: lineas terminadas en '\n' -------------------------------------------------------------
	if ( uartPC->Receive( linea , sizeof( linea ) ) )
		PC_Linea( linea );
}
