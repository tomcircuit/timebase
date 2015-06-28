/*
    Rb Oscillator Control Board Firmware
          tomcircuit April 1 2015 
        
            Application Defines
*/        
 

#ifndef _TIMEBASE_H_
#define _TIMEBASE_H_

/* OSCCON reset value (16 MHz CPU clock) */
// internal oscillator 16 MHz
// use primary block as source (allows PLL)
#define RESET_OSCCON (0b01110000)
#define RESET_OSCCON2 (0b00000000)

/* INTCON reset value (no interrupts) */
#define RESET_INTCON (0)

/* PORT A GPIO assignments */
#define RESET_PORTA (0)
#define RESET_TRISA (0x0f)
#define RESET_ANSELA (0b00001110)
// BITE and AN1,2,3 as inputs
#define RB_BITE (1)
#define EN_V24SW (16)

/* PORT B GPIO assignments */
#define RESET_PORTB (0)
#define RESET_TRISB (0x1f)
#define RESET_ANSELB (0b00010000)
// V24_GOOD, switches, AN11 as inputs
#define V24_GOOD (1)
#define SW_COMB (2)
#define SW_CMOS (4)
#define SW_DIST (8)

/* PORT C GIPO assignments */
#define RESET_PORTC (0)
#define RESET_TRISC (0xC1)
#define RESET_ANSELC (0)
// 100K square, RXD as inputs
#define SQ_100K (1)
#define PIXEL (4)

/* PORT D GIPO assignments */
#define RESET_PORTD (0x07)
#define RESET_TRISD (0)
#define RESET_ANSELD (0)
// all outputs
#define CG_SW0 (1)
#define CG_SW1 (2)
#define CG_SW2 (4)
#define EN_V9SW (8)
#define RD4_DEBUG (16)

/* PORT E GIPO assignments */
#define RESET_PORTE (0)
#define RESET_TRISE (0)
#define RESET_ANSELE (0)
// all outputs
#define EN_DIG (1)
#define EN_DIV (2)

/* USART1 constants */
// 207 is from table 16-5 in datasheet, for 19.2 kbps
//#define USART_BRG_VALUE (207)
// 68 is from table 16-5 in datasheet, for 57.6 kbps
#define USART_BRG_VALUE (68)

/* TIMER0 constants */
#define RESET_T0CON (0b00001001)
#define RUN_T0 (0x80)

/* ANALOG assignments */
#define RESET_ANSEL (0b00001110)
// AN1,2,3 configured as analog inputs
#define RESET_ANSELH (0b00001000)
// AN11 configured as analog input
#define ADC_CH_V24SW (1)
#define ADC_CH_V24 (2)
#define ADC_CH_LAMPV (3)
#define ADC_CH_CASET (11)

/* timing constants */
// following is absolute ms delay at end of each FSM iteration
#define ITER_DELAY_MS (10)
#define ITER_COUNT_MAX (100)
#define TMR0_RELOAD (65536-40000)

// following are all scalars against ITER_DELAY_MS
#define ITER_PROVE (200)
#define ITER_WARMUP_EXP (30000)
#define ITER_WARM_BITE (6000)
#define ITER_STABILIZE (1000)
#define ITER_REPORT (1000)
#define ITER_FLASH (200)
#define SW_OFF (0)
#define SW_ON (7)
#define CSW_ON (7)
#define BITE_FAIL (0)
#define BITE_OK (20)

/* ADC bounds */
// minimum voltage input is 16V
#define ADC_V24_MIN (158)
// maximum voltage input is 26V
#define ADC_V24_MAX (345)

// minimum voltage input is 16V
#define ADC_V24SW_MIN (158)
// maximum voltage input is 26V
#define ADC_V24SW_MAX (345)

// minimum lamp voltage is 5V
#define ADC_LAMP_MIN (180)
//#define ADC_LAMP_MIN (0)
// maximum lamp voltage is 14V
#define ADC_LAMP_MAX (756)
//#define ADC_LAMP_MAX (8192)

#endif //_TIMEBASE_H_

