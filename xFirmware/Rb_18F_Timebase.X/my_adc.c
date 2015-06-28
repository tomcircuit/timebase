/*************************************************************
* ADC interface for the  PIC18F44K22 
*************************************************************/

#include <xc.h>
#include "my_adc.h"

void adc_init(void)
{
   ADCON2 = 0b10101111;   // Right justified, Frc, 12 Tad acquisition
   ADCON1 = 0b00000000;   // ADC ref = Vdd,Vss
   ADCON0 = 0b00000001;   // ADC on, ch0, do not start conversion
}

unsigned int adc_measure(char channel)
{
   unsigned int retval;

	// Clear the channel selection in adcon0 (bits 2...6)
	ADCON0 &= 0b10000011;

	// Shift the channel value to the appropriate position
	channel = (channel & 0x1f) << 2;

	// And move it to the appropriate position in adcon0
	ADCON0 |= channel;

	// Start the conversion
        ADCON0 |= 0b00000010;

	// Wait until it is done
	while ((ADCON0 & 0b00000010) != 0);

	retval = (unsigned int)ADRESH;
	retval = retval << 8;
	retval |= ADRESL;

	return retval;
}
