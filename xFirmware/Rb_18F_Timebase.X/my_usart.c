/*************************************************************
* USART interface for the PIC18F44K22 
*************************************************************/

#include <xc.h>
#include "my_usart.h"

/* usart_init enables EUSART #1 and establishes BRG */
void usart1_init(unsigned int brgval)
{
	// BRG16 = 1
	BAUDCON1 = 0b00001000;

	// SPBRG1:SPBRGH1 = brgval
	SPBRG1 = brgval & 0xff;
	SPBRGH1 = brgval >> 8;

	// set RC6 and RC7 to inputs
	TRISC |= 0b11000000;

	// SPEN=1;
	// CREN=1;
	RCSTA1 = 0b10010000;

 	// SYNC = 0
	// BRGH = 1
	TXSTA1 = 0b00100100;
}

char usart1_putchar(char ch)
{
	//Wait for TXREG Buffer to become available
	while(!PIR1bits.TX1IF);

	//Write data
	TXREG1=ch;
	
	return ch;
}


char usart1_getch()
{
	while(!PIR1bits.RC1IF);	//Wait for a byte
	return RCREG1;
}
