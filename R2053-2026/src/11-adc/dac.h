/*
 * dac.h
 *

 */

#ifndef SRC_11_ADC_DAC_H_
#define SRC_11_ADC_DAC_H_

#include "systick.h"
#include "gpio.h"
#include "LPC845.h"
#include "swm.h"


#if defined (__cplusplus)
extern "C" {

void DAC0_IRQHandler(void);
}
#endif


class dac {
public:
	enum seniales_t {cuadrada , diente, triangular };	// otras Senoidal???


private:
	uint8_t  m_NumeroCanal;
	bool  m_flag;
	uint32_t m_periodo;	//cuentas ??? frecuencia?? que es?
	seniales_t m_senial;
	static uint8_t m_inicializar;	//indica si el el canal ya esta en uso

	void initDac0();
public:


	dac(uint8_t canal, uint32_t periodo=100, seniales_t tipo=cuadrada);
	void DAC_IRQHandler (void);
	virtual ~dac();
};

#endif /* SRC_11_ADC_DAC_H_ */
