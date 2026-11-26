/*******************************************************************************************************************************//**
 *
 * @file		prueba_perifericos.h
 * @brief		SCAI - Prueba manual de perifericos (NO es la aplicacion final).
 * @details		Sirve para verificar cada componente a medida que se conecta, antes de que exista
 *				la maquina de estados de la puerta. Se habilita con PRUEBA_PERIFERICOS en main.cpp
 *				y se puede borrar entera cuando la aplicacion real este lista.
 *
 *				Teclas:   A = servo a 90 (abre) + LED verde + 1 pitido
 *				          B = servo a 0 (cierra) + apaga LED verde
 *				          C = LED rojo + 3 pitidos
 *				          D = apaga LED rojo y buzzer
 *				          * = pide la version de firmware al PN532 (prueba la UART del NFC)
 *				          # = manda "AT" al ESP8266 (prueba la UART del WiFi)
 *				          digitos = se muestran en el LCD y en la PC
 *				PC (USART0, 115200):  $TIME,dd/mm/aaaa,hh:mm:ss  -> pone en hora el DS3231
 *				Todo lo que llega del PN532 y del ESP8266 se reenvia a la PC.
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#ifndef PRUEBA_PERIFERICOS_H_
#define PRUEBA_PERIFERICOS_H_

void Prueba_Inicio( void );
void Prueba_Run( void );			// en cada vuelta del main loop

#endif /* PRUEBA_PERIFERICOS_H_ */
