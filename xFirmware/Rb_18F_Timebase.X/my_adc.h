/*************************************************************
* ADC interface for the internal PIC18F44K22 ADC
*************************************************************/

#ifndef _ADC_H_
#define _ADC_H_

void adc_init(void);
unsigned int adc_measure(char channel);

#endif  // _ADC_H_