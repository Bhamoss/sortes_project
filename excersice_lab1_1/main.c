#define __AVR_ATmega32U4__
// define which mcu for the IDE to know 


// u should communicate with this through minicom on linux
// dont forget to turn on local echo so you can see what you send also, no only receive

// including the avr io library
#include <avr/io.h>

// NOTE
// I am defining constants before imports because they load libraries differently when certain
// constants are set


// the speed of our cpu has to be set for the delay to work like it should
// you have to define this before importing the library
#define F_CPU 8000000
// defining the baud rate at which our serial connection runs
#define BAUD 9600


// including for delay
#include <util/delay.h>
// include for easy oscilator calculations
#include <util/setbaud.h>
// include the following for setting the input and output stream for 
// the input and output functions we would normally use
// see just before main how we do it
#include <stdio.h>



// macros for more clear bit operations
#define BV(x)   (1 << x)
#define setBit(P,B) P |= BV(B)
#define clearBit(P,B) P &= ~BV(B)
#define toggleBit(P,B) P ^= BV(B)

/******************************************************************************
 * 
 * Using this guid: https://appelsiini.net/2011/simple-usart-with-avr-libc/
 * and the data sheet and the youtube series.
 * 
 * See p209-213 in the full datasheet for the meaning of the register constants.
 * 
 * U(S)ART communication is controlled and monitored through 3 bytes:
 * The UCSRn A, B and C bytes where n is a number.
 * The A registry contains flags for knowing if we can receive or send and if errors occured when sending.
 * The B registry contains bits for enablin various interupts, enabling receiving or sending and settin frame size
 * The C registry contains bits for configuring the serial connections: operating mode, parity, stop bits, frame size
 * It stands for USART control and status registry.
 * Setting the baud rate is done through 2 registers: the UBRR registers, see page 213
 * The receiving is done in the  RXD1 byte and
 * when something is transmitted, it is done through the TXD1 byte.
 * 
 ******************************************************************************/

/*

The USART Transmitter has two flags that indicate its state: USART Data Register Empty (UDREn) and
Transmit Complete (TXCn). Both flags can be used for generating interrupts.
The Receive Complete (RXCn) Flag indicates if there are unread data present in the receive buffer. This flag is
one when unread data exist in the receive buffer, and zero when the receive buffer is empty.
When the Receive Complete Interrupt Enable (RXCIEn) in UCSRnB is set, the USART Receive Complete
interrupt will be executed as long as the RXCn Flag is set (provided that global interrupts are enabled). When
interrupt-driven data reception is used, the receive complete routine must read the received data from UDRn in
order to clear the RXCn Flag, otherwise a new interrupt will occur once the interrupt routine terminates.
-> Super handig om te slapen tot we input krijgen en dan weer te gaan slapen (daarvoor waarschuwde hij in de les).
*/

/**
 * Function which sets up the serial connection with the right parameters.
 * 
 * Serial communication as in arduino IDE uses UART. See the USART section of the full data sheet.
 **/
void serial_Init(void){
    //TODO: when using interupts, disable them before the setup.

    // the serial import defines constant for us when we set the baud rate and clock speed to 
    // set the speed of the uart connection
    UBRR1H = UBRRH_VALUE; // many tutorials have a 0 instead of a 1 here, I don't know why but for us it is 1
    UBRR1L = UBRRL_VALUE;    // I think it is because the 0 registers are used for LORA on our board

    // The control registers are initialised to zero
    // which means we can leave them on zero default settings.
    // E.g. leaving the operating mode bits as 0 select UART as operating mode

    // setting 8 bit frames
    UCSR1C = BV(UCSZ11) | BV(UCSZ10); /* 8-bit data */

    // we want to send and to receive:
    UCSR1B = BV(RXEN1) | BV(TXEN1);   /* Enable RX and TX */
}


/**
 * See the full datasheet where I copy pasted this from
 **/
void serial_Transmit( char data )
{
    /* Wait for empty transmit buffer by waiting for the Data Resgister Empty flag to be set*/
    // We do this by comparing the the status registry UCSR1A and ANDing it with a byte 
    // where only the bit on the same place of the empty flag set to one.
    // We stay in the loop for until the flag is raised.
    loop_until_bit_is_set(UCSR1A, UDRE1)   ;

    /* Put data into buffer, sends the data */
    // When you load this sending registry, it automatically starts transmitting.
    UDR1 = data;
}

/*

The transmit buffer can only be written when the UDREn (Data Registry Empty) Flag in the UCSRnA Register is set. Data written to
UDRn when the UDREn Flag is not set, will be ignored by the USART Transmitter. 

When data is written to the transmit buffer, and the Transmitter is enabled, 
the Transmitter will load the data into the Transmit Shift Register when the Shift Register is empty. 
Then the data will be serially transmitted on the TxDn pin.

The receive buffer consists of a two level FIFO. The FIFO will change its state whenever the receive buffer is
accessed. Due to this behavior of the receive buffer, do not use Read-Modify-Write instructions (SBI and CBI)
on this location. Be careful when using bit test instructions (SBIC and SBIS), since these also will change the
state of the FIFO.

*/

char serial_Receive( void )
{
    /* Wait for data to be received */
    loop_until_bit_is_set(UCSR1A, RXC1) ;

    /* Get and return received data from buffer */
    return UDR1;
}

/*

If you wonder why the read and write registry are the same, read this from the datasheet:
The USART Transmit Data Buffer Register and USART Receive Data Buffer Registers share the same I/O
address referred to as USART Data Register or UDRn. The Transmit Data Buffer Register (TXB) will be the
destination for data written to the UDRn Register location. Reading the UDRn Register location will return the
contents of the Receive Data Buffer Register (RXB).

*/

// To redirect input I will redefine the above funtions a little.
// however, I will leave the above because I do not know the behaviour of this
// which could mess up our sleeping maybe.

void uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        uart_putchar('\r', stream);
    }
    loop_until_bit_is_set(UCSR1A, UDRE1);
    UDR1 = c;
}

char uart_getchar(FILE *stream) {
    loop_until_bit_is_set(UCSR1A, RXC1); /* Wait until data exists. */
    return UDR1;
}

// creating streams for redirecting input and ouput
FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);




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


        serial_init();

        // redirecting stdio 
        stdout = &uart_output;
        stdin  = &uart_input;
        
        char input;

        //------- loop ---------//
        while (1) {
            // wait a second
            puts("Hello world!");
            input = getchar();
            printf("You wrote %c\n", input);
            

            
        }


        // this line is never reached
        return(0);

}



