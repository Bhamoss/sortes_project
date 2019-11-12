#define __AVR_ATmega32U4__

#include <avr/io.h>

#define F_CPU 8000000
#define BAUD 9600
#define BRC ((F_CPU/16/BAUD)-1)


#include <util/delay.h>

/*
#define LED PC7
#define LED_PORT PORTC
#define LED_DDR DDRC

#define BV(x)   (1 << x)
#define setBit(P,B) P |= BV(B)
#define clearBit(P,B) P &= ~BV(B)
#define toggleBit(P,B) P ^= BV(B)
*/

void serial_Init(void){

    UBRR1H = (BRC >> 8); // many tutorials have a 0 instead of a 1 here, I don't know why but for us it is 1
    UBRR1L = BRC;    

    UCSR1C = ( 1 << UCSZ11) | (1 << UCSZ10); /* 8-bit data */

    // we want to send
    UCSR1B = (1 << TXEN1);   /* Enable  TX */
}



void serial_Transmit( char data )
{
    loop_until_bit_is_set(UCSR1A, UDRE1)   ;
    UDR1 = data;
}


int main(void) {
        //------- Setup --------//

        

        serial_Init();

        // setting up led as output
        //setBit(LED_DDR, LED);
        

        //------- loop ---------//
        while (1) {
            /*
            clearBit(LED_PORT, LED);
            // signaling start
            for(int i; i < 30; i++){
                toggleBit(LED_PORT, LED);
                _delay_ms(500);
            }
            clearBit(LED_PORT, LED);
            */

            //serial_Transmit('c');
            
            serial_Transmit('c');
            _delay_ms(1000);

            /*
            // rapidly flashin for 5 seconds to indicate we got the string
            for(int i; i < 100; i++){
                toggleBit(LED_PORT, LED);
                _delay_ms(50);
            }
            */
            

            
        }
        return(0);
}



