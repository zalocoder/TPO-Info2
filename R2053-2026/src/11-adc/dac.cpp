/*
 * dac.cpp

 */

#include <dac.h>

uint8_t dac::m_inicializar=0;

static dac *g_dac0;

//// DAC 0

void dac::initDac0 ( void )// Falta agregar Canal 1.
{
	//---- Pin 0,17

	//Energizo el periferico y le habilito el clk:
	SYSCON->PDRUNCFG &= ~(1<<13);
	SYSCON->SYSAHBCLKCTRL0 |= 1<<27;

	//Habilito el clk para la SWM y selecciono el bit del DAC en el
	//registro PINENABLE correspondiente para tener la salida en el pin P0.17:
	SYSCON->SYSAHBCLKCTRL0 |= 1<<7;
	SYSCON->SYSAHBCLKCTRL0 |= 1<<18;

	//PINENABLE0 &= ~(1<<26); XXXX
	PINENABLE_Config( 26 , 1 );
	IOCON->PIO[0] = (1<<16);

	//Registros del DAC:
	DAC0->CR = 0;
	DAC0->CTRL = 0;

	DAC0->CTRL = 1<<0 |	//Habilito interrupciones cuando haya que refrescar la salida
				1<<1 |	//Hablito el doble buffer
				0<<2 |	//Dejo deshabilitado el contador (lo habilito con start)
				0<<3;	//Deshabilito el DMA


	int A_Definir=5000;
	DAC0->CNTVAL = A_Definir;

	NVIC->ISER[0] |= (1 << 2);

	DAC0->CTRL |= 1<<2; 	//Habilito interrupciones cuando haya que refrescar la salida

}


dac::dac(uint8_t canal, uint32_t periodo, seniales_t tipo): m_senial(tipo)
{

	m_flag=false;

	if (canal > 1)	// solo 2 canales disponibles 0 o 1.
		return;

	if ((1<canal)& m_inicializar)	// canal en uso
		return;

	m_inicializar|=1<canal;
	m_NumeroCanal=canal;


	m_periodo=(periodo>100)?periodo:100;

	if (canal==0)
	{
		initDac0 ();
		m_flag=true;
		g_dac0 = this ;
	}

		// y el canal 1????
}



#define MAX_DAC_VALUE 0x3FF	//1024-1

void dac::DAC_IRQHandler(void)
{
static uint32_t cnt=0;
int val=0;

	switch (m_senial)
	{
	case cuadrada:
			val= (cnt<m_periodo/2)?0:MAX_DAC_VALUE;
			break;

	case diente:
			val= cnt * MAX_DAC_VALUE/m_periodo;
		break;


	case triangular:	// A resolver
		break;
	}

	if (++cnt >= m_periodo)
		cnt=0;

	//valor a colocar en el pin:
	DAC0->CR = ((val & 0x3FF)<<6);

}


dac::~dac() {
	// TODO Auto-generated destructor stub
}


void DAC0_IRQHandler(void)
{
	g_dac0->DAC_IRQHandler();
}
