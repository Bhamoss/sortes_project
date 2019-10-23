// including the avr library
#include <avr/io.h>

// including for delay
#include <util/delay.h>


// the speed of our cpu has to be set for the delay to work like it should
#define F_CPU 8000000

//TODO: set to the correct ports and bits of the led
// so LED_BUILTIN printed 13
// the bsfrance datasheet has arduino pin 13 on PC7
#define LED PB0
#define LED_PORT PORTB
#define LED_DDR DDRB

// macros for more clear bit operations
#define BV(x)   (1 << x)
#define setBit(P,B) P |= BV(B)
#define clearBit(P,B) P &= ~BV(B)
#define toggleBit(P,B) P ^= BV(B)


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
            _delay_ms(100);
        }


        // this line is never reached
        return(0);

}


/**
 * Things i learned about the board:
 * 
 *  ATMegas have 3 timers/counters.
 * 
 * It has built in hardware for all comm protocols we have seen.
 * ADC = convert analog to digital
 * It has EEPROM which is non volatile.
 * Our MCU runs at 8 MHz and is 3.3
 **/