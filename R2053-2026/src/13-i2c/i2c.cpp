/*******************************************************************************************************************************//**
 *
 * @file		i2c.cpp
 * @brief		Driver I2C maestro no bloqueante (ver i2c.h)
 * @date		26 nov. 2026
 *
 **********************************************************************************************************************************/
#include "i2c.h"
#include "swm.h"

/***********************************************************************************************************************************
 *** DEFINES PRIVADOS
 **********************************************************************************************************************************/
// Bits de STAT
#define STAT_MSTPENDING		(1 << 0)		// el maestro termino un paso y espera que le digamos que hacer
#define STAT_MSTSTATE(s)	(((s) >> 1) & 0x7)
#define STAT_MSTARBLOSS		(1 << 4)		// perdio el arbitraje (otro maestro en el bus)
#define STAT_MSTSTSTPERR	(1 << 6)		// START o STOP fuera de lugar

// Valores de MSTSTATE
#define MST_IDLE			0
#define MST_RX_LISTO		1				// llego un byte, esta en MSTDAT
#define MST_TX_LISTO		2				// se puede escribir el proximo byte
#define MST_NACK_DIR		3				// nadie respondio a la direccion
#define MST_NACK_DATO		4				// el esclavo no acepto un dato

// Bits de MSTCTL
#define MSTCTL_CONTINUE		(1 << 0)
#define MSTCTL_START		(1 << 1)
#define MSTCTL_STOP			(1 << 2)

// Clock: FCLKSEL = 0 elige el FRO (12 MHz, el mismo que asume la clase uart de la catedra).
//   12 MHz / (CLKDIV + 1) = 12 MHz / 15 = 800 kHz
//   cada periodo de SCL = (MSTSCLLOW + 2) + (MSTSCLHIGH + 2) = 4 + 4 = 8 clocks -> 100 kHz (modo estandar)
#define I2C_CLKDIV			14
#define I2C_MSTTIME			( (2 << 0) | (2 << 4) )

// Tablas de indices del IOCON (definidas en gpio.cpp)
extern const uint8_t IOCON_INDEX_PIO0[];
extern const uint8_t IOCON_INDEX_PIO1[];

/***********************************************************************************************************************************
 *** METODOS
 **********************************************************************************************************************************/
/**
	\brief	Configura el I2C numero num (1, 2 o 3) como maestro a 100 kHz
	\param	num		1..3 (el I2C0 tiene pines fijos que estan ocupados por el LCD)
*/
i2c::i2c( uint8_t num , uint8_t portSDA , uint8_t pinSDA , uint8_t portSCL , uint8_t pinSCL )
{
	// Datos que cambian segun el numero de I2C
	const struct {
		uint8_t		bitClk;		// bit en SYSAHBCLKCTRL0 y en PRESETCTRL0
		uint8_t		paSDA;		// funcion movible de la Switch Matrix
		uint8_t		paSCL;
		uint32_t	base;
	} tabla[] = {
		{ 21 , PA_I2C1_SDA , PA_I2C1_SCL , I2C1_BASE },
		{ 22 , PA_I2C2_SDA , PA_I2C2_SCL , I2C2_BASE },
		{ 23 , PA_I2C3_SDA , PA_I2C3_SCL , I2C3_BASE },
	};

	if ( num < 1 || num > 3 )
		num = 1;
	const uint8_t i = num - 1;

	m_i2c	 = (I2C_Type *) tabla[i].base;
	m_estado = LIBRE;
	m_paso	 = P_REPOSO;
	m_tx = nullptr; m_rx = nullptr;
	m_ntx = m_nrx = m_idx = m_dir = 0;

	SYSCON->SYSAHBCLKCTRL0 |= (1 << 7) | (1 << 18);			// SWM e IOCON
	SYSCON->SYSAHBCLKCTRL0 |= (1 << tabla[i].bitClk);		// clock del I2C
	SYSCON->PRESETCTRL0 &= ~(1 << tabla[i].bitClk);			// reset del periferico...
	SYSCON->PRESETCTRL0 |=  (1 << tabla[i].bitClk);			// ...y se libera
	SYSCON->FCLKSEL[ 5 + num ] = 0;							// FCLKSEL[5..8] = I2C0..I2C3. 0 = FRO

	// Pines: open-drain y sin pull-up interno (el modulo DS3231 ya trae los pull-up)
	PinOpenDrain( portSDA , pinSDA );
	PinOpenDrain( portSCL , pinSCL );
	PINASSIGN_Config( tabla[i].paSDA , portSDA , pinSDA );
	PINASSIGN_Config( tabla[i].paSCL , portSCL , pinSCL );

	m_i2c->CLKDIV  = I2C_CLKDIV;
	m_i2c->MSTTIME = I2C_MSTTIME;
	m_i2c->CFG	   = 1;										// MSTEN: modo maestro
}

/**
	\brief	Arranca una transaccion: escribe ntx bytes y luego lee nrx bytes (START repetido entre medio)
	\return	false si el bus esta ocupado (no se arranca nada)
	\note	Los buffers tx y rx tienen que seguir existiendo hasta que la transaccion termine.
*/
bool i2c::Transferir( uint8_t dir7 , const uint8_t *tx , uint8_t ntx , uint8_t *rx , uint8_t nrx )
{
	if ( m_estado == OCUPADO || ( ntx == 0 && nrx == 0 ) )
		return false;

	m_dir = dir7; m_tx = tx; m_ntx = ntx; m_rx = rx; m_nrx = nrx; m_idx = 0;
	m_estado = OCUPADO;

	if ( ntx )
	{
		m_i2c->MSTDAT = ( dir7 << 1 ) | 0;		// direccion + bit R/W = 0 (escritura)
		m_paso = P_DIR_ESCRITURA;
	}
	else
	{
		m_i2c->MSTDAT = ( dir7 << 1 ) | 1;		// direccion + bit R/W = 1 (lectura)
		m_paso = P_DIR_LECTURA;
	}
	m_i2c->MSTCTL = MSTCTL_START;
	return true;
}

/**
	\brief	Avanza un paso la transaccion en curso. Se llama en cada vuelta del main loop.
	\details Si el hardware todavia no termino el paso anterior (MSTPENDING = 0) no hace nada y vuelve.
*/
void i2c::Run( void )
{
	if ( m_paso == P_REPOSO )
		return;

	uint32_t stat = m_i2c->STAT;
	if ( !( stat & STAT_MSTPENDING ) )
		return;										// el hardware sigue trabajando

	if ( stat & ( STAT_MSTARBLOSS | STAT_MSTSTSTPERR ) )
	{
		m_i2c->STAT = STAT_MSTARBLOSS | STAT_MSTSTSTPERR;	// se borran escribiendo 1
		m_paso = P_REPOSO;
		m_estado = ERROR;							// el maestro ya quedo libre, no hace falta STOP
		return;
	}

	switch ( m_paso )
	{
		case P_DIR_ESCRITURA:
		case P_ESCRIBIENDO:
			if ( STAT_MSTSTATE( stat ) != MST_TX_LISTO )
			{
				Stop( ERROR );						// NACK: el esclavo no respondio o no acepto el dato
				break;
			}
			if ( m_idx < m_ntx )
			{
				m_i2c->MSTDAT = m_tx[ m_idx++ ];
				m_i2c->MSTCTL = MSTCTL_CONTINUE;
				m_paso = P_ESCRIBIENDO;
			}
			else if ( m_nrx )
			{
				m_idx = 0;							// ahora se cuenta lo leido
				m_i2c->MSTDAT = ( m_dir << 1 ) | 1;
				m_i2c->MSTCTL = MSTCTL_START;		// START repetido, sin STOP en el medio
				m_paso = P_DIR_LECTURA;
			}
			else
				Stop( TERMINADO );
			break;

		case P_DIR_LECTURA:
		case P_LEYENDO:
			if ( STAT_MSTSTATE( stat ) != MST_RX_LISTO )
			{
				Stop( ERROR );
				break;
			}
			m_rx[ m_idx++ ] = (uint8_t) m_i2c->MSTDAT;
			if ( m_idx < m_nrx )
			{
				m_i2c->MSTCTL = MSTCTL_CONTINUE;	// CONTINUE = ACK, se pide otro byte
				m_paso = P_LEYENDO;
			}
			else
				Stop( TERMINADO );					// STOP despues del ultimo byte = NACK + STOP
			break;

		default:
			break;
	}
}

void i2c::Liberar( void )
{
	if ( m_estado != OCUPADO )
		m_estado = LIBRE;
}

void i2c::Stop( estado_t final )
{
	m_i2c->MSTCTL = MSTCTL_STOP;
	m_paso = P_REPOSO;
	m_estado = final;
}

/**
	\brief	Deja un pin en open-drain y sin resistencias internas (lo que necesita un bus I2C)
	\details IOCON: bits 4:3 MODE = 0 (sin pull-up/pull-down), bit 10 OD = 1 (open-drain)
*/
void i2c::PinOpenDrain( uint8_t port , uint8_t pin )
{
	uint8_t idx = ( port == 0 ) ? IOCON_INDEX_PIO0[ pin ] : IOCON_INDEX_PIO1[ pin ];

	IOCON->PIO[ idx ] &= ~( 0x3 << 3 );
	IOCON->PIO[ idx ] |=  ( 1 << 10 );
}
