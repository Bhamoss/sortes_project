// including the avr library
#include <avr/io.h>

// the speed of our cpu has to be set for the delay to work like it should
#define F_CPU 8000000
// including for delay
#include <util/delay.h>



//TODO: set to the correct ports and bits of the led
// so LED_BUILTIN printed 13
// the bsfrance datasheet has arduino pin 13 on PC7
#define LED PC7
#define LED_PORT PORTC
#define LED_DDR DDRC

// macros for more clear bit operations
#define BV(x)   (1 << x)
#define setBit(P,B) P |= BV(B)
#define clearBit(P,B) P &= ~BV(B)
#define toggleBit(P,B) P ^= BV(B)

char text[] = "Hello world!";
#define TEMPO 1000

void dot(void) {
	setBit(LED_PORT, LED);
	_delay_ms(1*TEMPO);
	clearBit(LED_PORT, LED);
	_delay_ms(1*TEMPO);
}

void dash(void) {
	setBit(LED_PORT, LED);
	_delay_ms(3*TEMPO);
	clearBit(LED_PORT, LED);
	_delay_ms(1*TEMPO);
}

// this method came from an internet repo
void morse(char letter){
	if (letter == 'a' || letter == 'A') {dot(); dash();}
	if (letter == 'b' || letter == 'B') {dash(); dot(); dot(); dot();}
	if (letter == 'c' || letter == 'C') {dash(); dot(); dash(); dot();}
	if (letter == 'd' || letter == 'D') {dash(); dot(); dot();}
	if (letter == 'e' || letter == 'E') {dot();}
	if (letter == 'f' || letter == 'F') {dot(); dot(); dash(); dot();}
	if (letter == 'g' || letter == 'G') {dash(); dash(); dot();}
	if (letter == 'h' || letter == 'H') {dot(); dot(); dot(); dot();}
	if (letter == 'i' || letter == 'I') {dot(); dot();}
	if (letter == 'j' || letter == 'J') {dot(); dash(); dash(); dash();}
	if (letter == 'k' || letter == 'K') {dash(); dot(); dash();}
	if (letter == 'l' || letter == 'L') {dot(); dash(); dot(); dot();}
	if (letter == 'm' || letter == 'M') {dash(); dash();}
	if (letter == 'n' || letter == 'N') {dash(); dot();}
	if (letter == 'o' || letter == 'O') {dash(); dash(); dash();}
	if (letter == 'p' || letter == 'P') {dot(); dash(); dash(); dot();}
	if (letter == 'q' || letter == 'Q') {dash(); dash(); dot(); dash();}
	if (letter == 'r' || letter == 'R') {dot(); dash(); dot();}
	if (letter == 's' || letter == 'S') {dot(); dot(); dot();}
	if (letter == 't' || letter == 'T') {dash();}
	if (letter == 'u' || letter == 'U') {dot(); dot(); dash();}
	if (letter == 'v' || letter == 'V') {dot(); dot(); dot(); dash();}
	if (letter == 'w' || letter == 'W') {dot(); dash(); dash();}
	if (letter == 'x' || letter == 'X') {dash(); dot(); dot(); dash();}
	if (letter == 'y' || letter == 'Y') {dash(); dot(); dash(); dash();}
	if (letter == 'z' || letter == 'Z') {dash(); dash(); dot(); dot();}
	if (letter == '1') {dot(); dash(); dash(); dash(); dash();}
	if (letter == '2') {dot(); dot(); dash(); dash(); dash();}
	if (letter == '3') {dot(); dot(); dot(); dash(); dash();}
	if (letter == '4') {dot(); dot(); dot(); dot(); dash();}
	if (letter == '5') {dot(); dot(); dot(); dot(); dot();}
	if (letter == '6') {dash(); dot(); dot(); dot(); dot();}
	if (letter == '7') {dash(); dash(); dot(); dot(); dot();}
	if (letter == '8') {dash(); dash(); dash(); dot(); dot();}
	if (letter == '9') {dash(); dash(); dash(); dash(); dot();}
	if (letter == '0') {dash(); dash(); dash(); dash(); dash();}
	if (letter == ' ') {_delay_ms(5 * TEMPO);}                        // This makes 7 * tempo for space
	// teken komt net na ! dus we wachten even voor we uitroepteken uitzenden
	if (letter == '!') {_delay_ms(5 * TEMPO); dash(); dot(); dash(); dot(); dash(); dash();}

	// and	now for some national/special letters
	// values were foundt with Serial.print(letter, DEC)

	if (letter == 166 || letter == 134) {dot(); dash(); dot(); dash();}     // æ/Æ is recognized as 166/134
	if (letter == 184 || letter == 152) {dash(); dash();dash();dot();}      // ø/Ø is recognized as 184/152
	if (letter == 165 || letter == 133) {dot();dash();dash();dot();dash();} // å/Å is recognized as 165/133

	_delay_ms(2 * TEMPO);      // this makes 3 * tempo for letter end, and 7 * tempo for space
}

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
        
	// setting the LED off for a start
	clearBit(LED_PORT, LED);

        //------- loop ---------//
        while (1) {

	    // saying we are refressing by flashing rapidly for 3 seconds
	    for(int i = 0; i < 30; i++){
	    	toggleBit(LED_PORT, LED);
                _delay_ms(100);
	    }

	    _delay_ms(2000);

	    for (int i = 0; i < sizeof(text); i++) {
		    morse(text[i]);
	    }
	    _delay_ms(2000);
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
