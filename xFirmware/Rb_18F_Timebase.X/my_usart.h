#ifndef __my_USART_H
#define __my_USART_H

/* usart1_init enables EUSART #1 and establishes BRG */
void usart1_init(unsigned int brgval);

/* usart1_putchar sends a single character via EUSART #1 */
char usart1_putchar(char ch);

/* usart1_getch reads a single character via EUSART #1 */
char usart1_getch();

#endif
