#define __AVR_ATmega32U4__
// define which mcu for the IDE to know 

// including the avr io library
#include <avr/io.h>

// the speed of our cpu has to be set for the delay to work like it should
// you have to define this before importing the library
#define F_CPU 8000000
// including for delay
#include <util/delay.h>




// LED is on PC7
#define LED PC7
#define LED_PORT PORTC
#define LED_DDR DDRC

// macros for more clear bit operations
#define BV(x)   (1 << x)
#define setBit(P,B) P |= BV(B)
#define clearBit(P,B) P &= ~BV(B)
#define toggleBit(P,B) P ^= BV(B)

/******************************************************************************
 * 
 * Using this guid: https://appelsiini.net/2011/simple-usart-with-avr-libc/
 * 
 ******************************************************************************/

/**
 * main with no parameters
 */
int main(void) {
        //------- Setup --------//

        /**
         * You communicate by ports.
         * Each port has a letter.
         * Each port has 8 output pins and is represented as a byte.
         * If you want to write a 0 or 1 to a pin you write it to the corresponding bit of that byte.
         * Every port has a corresponding DDR (Data Direction Register) byte on which you specify whether a pin is input(0) or output (1).
         * In the setup you have to specify this by using the global variables of avr
         **/ 

        // setting the LED bit as output
        setBit(LED_DDR, LED);
        

        //------- loop ---------//
        while (1) {
            // toggle the led
            toggleBit(LED_PORT, LED);
            // wait a second
            _delay_ms(5000);
        }


        // this line is never reached
        return(0);

}



