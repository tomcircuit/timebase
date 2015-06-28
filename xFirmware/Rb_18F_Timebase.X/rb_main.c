/*
 * Rb Oscillator Control Board Firmware
 * File:   rb_main.c
 * Author: tomcircuit
 *
 * Created on April 8, 2015
 * Last Built on June 28, 2015
 */

#define VERSION_MAJ 1
#define VERSION_MIN 0

#define _XTAL_FREQ 16000000
#define USE_AND_MASKS

#include "my_config.h"
#include <stdio.h>
#include <stdlib.h>
#include "my_adc.h"
#include "my_usart.h"
#include "ws2812.h"

#include "timebase.h"

#define TRUE (1)
#define FALSE (0)

enum t_state {RESET = 0, PROVE, WARMUP, OPERATE, STANDBY, FAULT};
enum t_cgrate {CG10M = 0, CG5M = 1, CG1M = 2, CG500K = 3, CG100K = 4, CGOFF = 5};
enum t_fault {NORM = 0, V24_FAULT, BITE_TIMEOUT, V24SW_FAULT, BITE_FAULT, LAMPV_FAULT};

enum t_state state, prev_state;
enum t_cgrate comb_rate;
enum t_fault fault_code;

void putch(char data)
{
    usart1_putchar(data);
}

void main()
{
   unsigned int st_counter = 0;
   unsigned int iter_counter = 0;
   unsigned long run_counter = 0;

   unsigned long led_color = WS2812_OFF;
   unsigned long prev_color = WS2812_OFF+1;

   unsigned char sw_dist_count = SW_OFF;
   unsigned char sw_cmos_count = SW_OFF;
   unsigned char sw_comb_count = SW_OFF;
   unsigned char sw_comb_ready = TRUE;

   unsigned char diag_bite_count = BITE_FAIL;

   unsigned char read_portd;

   unsigned int meas_v24 = 0;
   unsigned int meas_v24sw = 0;
   unsigned int meas_lampv = 0;
   unsigned int meas_caset = 0;

   // set CPU clock frequency to internal RC 16 MHz
   OSCCON = RESET_OSCCON;
   OSCCON2 = RESET_OSCCON2;

   // initialize Timer0
   // (T0 stopped, 16 bit mode, no prescalar)
   T0CON = RESET_T0CON;

   state = RESET;
   prev_state = RESET;

   while (1)
   {
      // set CPU clock frequency to internal RC 16 MHz
      OSCCON = RESET_OSCCON;
      OSCCON2 = RESET_OSCCON2;

      // load Timer0 with the FSM interval count
      TMR0H = TMR0_RELOAD / 256;
      TMR0L = TMR0_RELOAD % 256;

      // clear Timer0 overflow flag
      INTCON = RESET_INTCON;

      // start Timer0
      TMR0ON = 1;

      /* initialize GPIO directions */
      TRISA = RESET_TRISA;
      TRISB = RESET_TRISB;
      TRISC = RESET_TRISC;
      TRISD = RESET_TRISD;
      TRISE = RESET_TRISE;

      /* enable analog functions on appropriate pins */
      ANSELA = RESET_ANSELA;
      ANSELB = RESET_ANSELB;
      ANSELC = RESET_ANSELC;
      ANSELD = RESET_ANSELD;
      ANSELE = RESET_ANSELE;

      /* initialize USART */
      usart1_init(USART_BRG_VALUE);

      /* debounce the switch inputs each iteration*/

      /* simple up/down counter for each switch. each counter saturates at SW_ON (highest value)
         and at SW_OFF (lowest value). A counter value of SW_ON indicates that the switch is
         closed. */
      if ((0 == (PORTB & SW_DIST)) && (sw_dist_count < SW_ON))
         sw_dist_count++;
      if ((0 != (PORTB & SW_DIST)) && (sw_dist_count > SW_OFF))
         sw_dist_count--;

      if ((0 == (PORTB & SW_CMOS)) && (sw_cmos_count < SW_ON))
         sw_cmos_count++;
      if ((0 != (PORTB & SW_CMOS)) && (sw_cmos_count > SW_OFF))
         sw_cmos_count--;

      if ((0 == (PORTB & SW_COMB)) && (sw_comb_count < CSW_ON))
         sw_comb_count++;
      if ((0 != (PORTB & SW_COMB)) && (sw_comb_count > SW_OFF))
         sw_comb_count--;

      /* debounce the BITE input each iteration*/
      if ((0 == (PORTA & RB_BITE)) && (diag_bite_count < BITE_OK))
         diag_bite_count++;
      if ((0 != (PORTA & RB_BITE)) && (diag_bite_count > BITE_FAIL))
         diag_bite_count--;

      /* OPERATE state hander */

      /* OPERATE state is when the LPRO-101 is powered, at at least one output is enabled.
         In OPERATE state the 24VSW and BITE and LAMPV are all checked, and if any are abnormal
         we proceed to FAULT state. Otherwise, we wait for all of the outputs to be be disabled
         to bring us back into STANDBY state. */
      if (OPERATE == state)
      {
         /* upon first entry into OPERATE, clear st_counter */
         if (prev_state != OPERATE)
            st_counter = 0;
         prev_state = OPERATE;

         // pixel_color(GREEN);
         led_color = WS2812_GREEN;

         /* turn on the 24V supply to LPRO-101 */
         PORTA |= EN_V24SW;

         /* control the 8V supply to the dist amp */
         if (sw_dist_count >= SW_ON)
            PORTD |= EN_V9SW;
         else
            PORTD &= ~EN_V9SW;

         /* control the CMOS outputs */
         if (sw_cmos_count >= SW_ON)
            PORTE |= EN_DIG;
         else
            PORTE &= ~EN_DIG;

         /* handle the comb generator rate toggle switch here */
         if ((sw_comb_count >= CSW_ON) && (sw_comb_ready == TRUE))
         {
            sw_comb_ready = FALSE;
            if (comb_rate == CG10M)
               comb_rate = CGOFF;
            else
               comb_rate -= 1;
         }

         if (sw_comb_count == SW_OFF)
            sw_comb_ready = TRUE;

         /* drive comb generator rate selection lines accordingly */
         /* read portd and mask out MUX control bits */
         read_portd = PORTD & 0xf8;
         switch (comb_rate)
         {
           case CG10M :
             /* rate mux input 0 is 10 MHz */
             PORTD = read_portd;
             break;
           case CG5M :
             /* rate mux input 1 is 5 MHz */
             PORTD = read_portd + 1;
            break;
          case CG1M :
             /* rate mux input 2 is 1 MHz */
             PORTD = read_portd + 2;
             break;
          case CG500K :
             /* rate mux input 3 is 0.5 MHz */
             PORTD = read_portd + 3;
             break;
          case CG100K :
             /* rate mux input 4 is 0.1 MHz */
             PORTD = read_portd + 4;
             break;
          default :
             /* rate mux inputs 7 is  GND */
             PORTD = read_portd + 7;
             break;
        }

         /* enable 10MHz divider and rate mux if comb generator enabled */
         if (comb_rate != CGOFF)
            PORTE &= ~EN_DIV;
         else
            PORTE |= EN_DIV;

         /* if ALL of the outputs are disabled, go back to STANDBY mode */
         if ((sw_cmos_count == SW_OFF) && (sw_dist_count == SW_OFF) && (comb_rate == CGOFF) && (sw_comb_ready == TRUE))
            state = STANDBY;

         /* allow a few iterations through the FSM before checking
            the switched 24V rail, lamp voltage, and BITE state */
         if (st_counter > ITER_STABILIZE)
         {
             /* check switched 24V rail */
             if ((meas_v24sw < ADC_V24SW_MIN) || (meas_v24sw > ADC_V24SW_MAX))
             {
                state = FAULT;
                fault_code = V24SW_FAULT;
             }

             /* check BITE signal for a fault */
             if (diag_bite_count == BITE_FAIL)
             {
                state = FAULT;
                fault_code = BITE_FAULT;
             }

             /* check if LAMPV level is within expected value */
             if ((meas_lampv < ADC_LAMP_MIN) || (meas_lampv > ADC_LAMP_MAX))
             {
                state = FAULT;
                fault_code = LAMPV_FAULT;
             }
         } // if (st_counter > ITER_STABILIZE)

         st_counter += 1;
         if (st_counter == 0)
             st_counter -= 1;
      } // if (OPERATE == state)

      /* STANDBY state hander */

      /* STANDBY state is when the LPRO-101 is powered, but no outputs are enabled.
         In STANDBY state the 24VSW and BITE and LAMPV are all checked, and if any are abnormal
         we proceed to FAULT state. Otherwise, we wait for one/more of the output enable switches
         to close to bring us into OPERATE state. */
      if (STANDBY == state)
      {
         /* upon first entry into STANDBY, clear st_counter */
         if (prev_state != STANDBY)
            st_counter = 0;
         prev_state = STANDBY;

         // pixel_color(YELLOW);
         led_color = WS2812_YELLOW;

         /* turn on the 24V supply to LPRO-101 */
         PORTA |= EN_V24SW;

         /* turn off the 8V supply to the dist amp */
         PORTD &= ~EN_V9SW;

         /* turn off the 10MHz divider chain */
         PORTE |= EN_DIV;

         /* turn off the CMOS outputs */
         PORTE &= ~EN_DIG;
         
         /* always make sw_comb_ready TRUE */
         sw_comb_ready = TRUE;

         if ((sw_cmos_count >= SW_ON) || (sw_dist_count >= SW_ON) || (sw_comb_count >= CSW_ON))
         {
            state = OPERATE;
         }

         /* allow a few iterations through the FSM before checking
            the switched 24V rail, the lamp voltage, and BITE state */
         if (st_counter > ITER_STABILIZE)
         {
             /* check switched 24V rail */
             if ((meas_v24sw < ADC_V24SW_MIN) || (meas_v24sw > ADC_V24SW_MAX))
             {
                state = FAULT;
                fault_code = V24SW_FAULT;
             }

             /* check BITE signal for a fault */
             if (diag_bite_count == BITE_FAIL)
             {
                state = FAULT;
                fault_code = BITE_FAULT;
             }

             /* check if LAMPV level is within expected value */
             if ((meas_lampv < ADC_LAMP_MIN) || (meas_lampv > ADC_LAMP_MAX))
             {
                state = FAULT;
                fault_code = LAMPV_FAULT;
             }
         } // if (st_counter > ITER_STABILIZE)

         st_counter += 1;
         if (st_counter == 0)
             st_counter -= 1;
      } // if (STANDBY == state)

      /* WARMUP state hander */

      /* WARMUP state waits for the LPRO-101 BITE signal to assert (LOW) for proper operation
         If it does not do so within a set time, proceed to FAULT state. Otherwise, proceed to
         STANDBY state. */
      if (WARMUP == state)
      {
         /* upon first entry into WARMUP, clear st_counter */
         if (prev_state != WARMUP)
            st_counter = 0;
         prev_state = WARMUP;

         // pixel_color(ORANGE);
         led_color = WS2812_ORANGE;

         /* turn on the 24V supply to LPRO-101 */
         PORTA |= EN_V24SW;

         /* turn off the 8V supply to the dist amp */
         PORTD &= ~EN_V9SW;

         /* turn off the 10MHz divider chain */
         PORTE |= EN_DIV;

         /* turn off the CMOS outputs */
         PORTE &= ~EN_DIG;

         /* allow a few iterations through the FSM before checking
            the switched 24V rail voltage */
         if (st_counter > ITER_STABILIZE)
         {
            /* check switched 24V rail */
            if ((meas_v24sw < ADC_V24SW_MIN) || (meas_v24sw > ADC_V24SW_MAX))
            {
               state = FAULT;
               fault_code = V24SW_FAULT;
            }
         }

         if (st_counter > ITER_WARM_BITE)
         {
             /* check BITE signal to see if no longer failing */
             if (diag_bite_count != BITE_FAIL)
               state = STANDBY;
         }

         /* check to see if warmup timer has expired. If we haven't left WARMUP state
            by now, then time to move to FAULT state! */
         if (st_counter > ITER_WARMUP_EXP)
         {
            state = FAULT;
            fault_code = BITE_TIMEOUT;
         }

         st_counter += 1;
         if (st_counter == 0)
             st_counter -= 1;

      } // if (WARMUP == state)

      /* PROVE state handler */

      /* PROVE state delays for 500ms, then checks the 24V supply to
         ensure it is within expected limits. If so, proceed to WARMUP state.
         If not, proceed to FAULT state. */
      if (PROVE == state)
      {
         /* upon first entry into PROVE, clear st_counter */
         if (prev_state != PROVE)
            st_counter = 0;
         prev_state = PROVE;

         // pixel_color(VIOLET);
         led_color = WS2812_VIOLET;

         /* turn off the 24V supply to LPRO-101 */
         PORTA &= ~EN_V24SW;

         /* turn off the 8V supply to the dist amp */
         PORTD &= ~EN_V9SW;

         /* turn off the 10MHz divider chain */
         PORTE |= EN_DIV;

         /* turn off the CMOS outputs */
         PORTE &= ~EN_DIG;

         if (st_counter >= ITER_PROVE)
         {
            if ((meas_v24 < ADC_V24_MIN) || (meas_v24 > ADC_V24_MAX))
            {
               state = FAULT;
               fault_code = V24_FAULT;
            }
            else
               state = WARMUP;
         } // if (st_counter >= ITER_PROVE)

         st_counter += 1;
         if (st_counter == 0)
             st_counter -= 1;
      } // if (PROVE == state)

      /* FAULT state handler */

      /* FAULT state latches and never exits. The status LED is illuminated with
         specific color to reflect the active fault status. */

      if (FAULT == state)
      {
         /* upon first entry into PROVE, clear st_counter */
         if (prev_state != FAULT)
            st_counter = 0;
         prev_state = FAULT;

         // establish pixel_color
         if (st_counter <= ITER_FLASH/2)
            led_color = WS2812_RED;
         else {
             led_color = WS2812_OFF;
             if (fault_code == V24_FAULT)
                 led_color = WS2812_VIOLET;
             if (fault_code == V24SW_FAULT)
                 led_color = WS2812_VIOLET;
             if (fault_code == BITE_FAULT)
                 led_color = WS2812_YELLOW;
             if (fault_code == BITE_TIMEOUT)
                 led_color == WS2812_ORANGE;
             if (fault_code == LAMPV_FAULT)
                 led_color == WS2812_WHITE;
         }

         /* turn off the 24V supply to LPRO-101 */
         PORTA &= ~EN_V24SW;

         /* turn off the 8V supply to the dist amp */
         PORTD &= ~EN_V9SW;

         /* turn off the 10MHz divider chain */
         PORTE |= EN_DIV;

         /* turn off the CMOS outputs */
         PORTE &= ~EN_DIG;

         st_counter += 1;
         if (st_counter >= ITER_FLASH)
             st_counter = 0;

      } // if (FAULT == state)

      /* RESET state hander */
      if (RESET == state)
      {
         PORTA = RESET_PORTA;
         PORTB = RESET_PORTB;
         PORTC = RESET_PORTC;
         PORTD = RESET_PORTD;
         PORTE = RESET_PORTE;

         run_counter = 0;

         sw_dist_count = SW_OFF;
         sw_cmos_count = SW_OFF;
         sw_comb_count = SW_OFF;
         sw_comb_ready = FALSE;
         comb_rate = CGOFF;

         prev_state = RESET;
         state = PROVE;

         led_color = WS2812_OFF;
      } // if (RESET == state)

      /* If the LPRO-101 is powered, increment the run counter. This count saturates
         at 32b long storage value, which is about 1.3 years of continuous uptime! */
      if (PORTA & EN_V24SW)
      {
          run_counter += 1;
          if (run_counter == 0)
             run_counter--;
      }

      /* toggle RD4 debug pin each time through main loop */
      PORTD ^= RD4_DEBUG;

      /* initialize ADC */
      adc_init();

      /* measure these each iteration - these are checked so often it makes more
         sense to do this outside the loop, rather than inside each state handler. */
      meas_v24 = adc_measure(ADC_CH_V24);
      meas_v24sw = adc_measure(ADC_CH_V24SW);
      meas_lampv = adc_measure(ADC_CH_LAMPV);
      meas_caset = adc_measure(ADC_CH_CASET);

      /* increment main loop iteration counter */
      iter_counter += 1;
      if (iter_counter >= ITER_COUNT_MAX)
          iter_counter = 0;

      /* send out information via USART in small pieces, so as not to disturb
       * overall timing of main loop */
      if (iter_counter == 0)
      {
          printf("\n\n\r>>>>>\r\nState (%u) ",state);
          switch (state)
          {
              case RESET : printf("RESET");
              break;
              case PROVE : printf("PROVE");
              break;
              case WARMUP : printf("WARMUP");
              break;
              case OPERATE : printf("OPERATE");
              break;
              case STANDBY : printf("STANDBY");
              break;
              case FAULT : printf("FAULT");
              break;
          }
      }

      // Report Version and Running Time Counter (seconds)
      if (iter_counter == 1)
      {
          printf("\r\nFirmware Version = %d.%d",VERSION_MAJ,VERSION_MIN);
          printf("\r\nRB10 Uptime = %ld seconds",run_counter/(1000/ITER_DELAY_MS));
      }

      // Report Input Voltage (volts)
      if (iter_counter == 2)
      {
         /* take the ADC reading, add 2, multiply by 32, divide by 149, and add 30.
          * The result is a fixed-point binary value that is four times the input voltage */
         unsigned int temp_int;
         temp_int = (meas_v24 + 2) << 5;
         temp_int = temp_int / 149;
         temp_int += 30;
         printf("\r\nInput voltage = %u",temp_int >> 2);
         if (temp_int & 3 == 0)
             printf(".00 VDC");
         else if (temp_int & 3 == 1)
             printf(".25 VDC");
         else if (temp_int & 3 == 2)
             printf(".50 VDC");
         else printf(".75 VDC");
      }

      // Report LPRO Supply Voltage (volts)
      if (iter_counter == 3)
      {
         /* take the ADC reading, add 2, multiply by 32, divide by 149, and add 30.
          * The result is a fixed-point binary value that is four times the input voltage */
         unsigned int temp_int;
         temp_int = (meas_v24sw + 2) << 5;
         temp_int = temp_int / 149;
         temp_int += 30;
         printf("\r\nLPRO voltage = %u",temp_int >> 2);
         if (temp_int & 3 == 0)
             printf(".00 VDC");
         else if (temp_int & 3 == 1)
             printf(".25 VDC");
         else if (temp_int & 3 == 2)
             printf(".50 VDC");
         else printf(".75 VDC");
      }

      // Report 8V Supply Rail Status
      if (iter_counter == 4)
      {
         if ((PORTD & EN_V9SW) != 0)
             printf("\r\n8V Supply rail ON");
         else
             printf("\r\n8V Supply rail OFF");
      }

      // Report LPRO Lamp Voltage (volts)
      if (iter_counter == 5)
      {
         /* take the ADC reading, add 20, divide by 16, then add 8. The result
          * is a fixed-point binary value that is four times the lamp voltage */
         unsigned int temp_int;
         temp_int = (meas_lampv + 20) >> 4;
         temp_int += 8;
         printf("\r\nLamp voltage = %u",temp_int >> 2);
         if (temp_int & 3 == 0)
             printf(".00 VDC");
         else if (temp_int & 3 == 1)
             printf(".25 VDC");
         else if (temp_int & 3 == 2)
             printf(".50 VDC");
         else printf(".75 VDC");
      }

      // Report LPRO Case Temperature (degrees C)
      if (iter_counter == 6)
      {
         /* take the ADC reading, subtract 80, and divide by 4. The result
          * is an integer binary value that is the temperature in Celsius*/
         unsigned int temp_int;
         if ((meas_caset < 100) || (meas_caset > 400))
         {
             printf("\r\nTemp Sensor out-of-range");
         }
         else
         {
             temp_int = (meas_caset - 80) / 4;
             printf("\r\nLPRO Case temperature = %u C",temp_int);
         }
      }

      // Report CMOS Driver Status
      if (iter_counter == 7)
      {
         if ((PORTE & EN_DIG) != 0)
             printf("\r\nCMOS ON");
         else
             printf("\r\nCMOS OFF");
      }

      // Report Frequency Divider Status
      if (iter_counter == 8)
      {
         if ((PORTE & EN_DIV) != 0)
             printf("\r\nDivider OFF");
         else
             printf("\r\nDivider ON");
      }

      // Report Comb Generator Status (rate)
      if (iter_counter == 9)
      {
          printf("\r\nComb generator rate ");
          switch (comb_rate)
          {
              case CGOFF : printf("OFF");
              break;
              case CG10M : printf("10MHz");
              break;
              case CG5M : printf("5MHz");
              break;
              case CG1M : printf("1MHz");
              break;
              case CG500K : printf("500KHz");
              break;
              case CG100K : printf("100KHz");
              break;
          }
      }

      // Report Dist Switch Status (ticks)
      if (iter_counter == 10)
      {
         printf("\r\nDIST switch (%u) ", (unsigned int) sw_dist_count);
         if (sw_dist_count >= SW_ON)
             printf("ON");
         else if (sw_dist_count == SW_OFF)
             printf("OFF");
      }

      // Report CMOS Switch Status (ticks)
      if (iter_counter == 11)
      {
         printf("\r\nCMOS switch (%u) ", (unsigned int) sw_cmos_count);
         if (sw_cmos_count >= SW_ON)
             printf("ON");
         else if (sw_cmos_count == SW_OFF)
             printf("OFF");
      }

      // Report Comb Switch Status (ticks)
      if (iter_counter == 12)
      {
         printf("\r\nCOMB switch (%u) ", (unsigned int) sw_comb_count);
         if (sw_comb_count >= CSW_ON)
             printf("ON");
         else if (sw_comb_count == SW_OFF)
             printf("OFF");
      }

      // update the NeoPixel LED if necessary

      // warning: this uses MCU PLL, so the Timer0 period will be short
      // during color changes!

      if (prev_color != led_color)
      {
          /* increase CPU clock to 64 MHz and drive NEOPIXEL */
          OSCTUNEbits.PLLEN = 1;

          ws2812_send(led_color);

          /* reduce CPU clock back down to 16 MHz */
          OSCTUNEbits.PLLEN = 0;
          prev_color = led_color;
      }

      /* wait for the interval timer to expire before doing next iteration */
      while (TMR0IF == 0);

//      /* delay at end of each iteration through the main loop */
//      __delay_ms(ITER_DELAY_MS);

   } // while(1)
}