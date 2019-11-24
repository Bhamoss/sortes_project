#include <EDB.h>
#include <EEPROM.h>
#include <SPI.h>              
#include <LoRa.h>

#define ROOM_TEMP 21

//LoR32u4II 868MHz or 915MHz (black board)
#define SCK     15
#define MISO    14
#define MOSI    16
#define SS      8
#define RST     4
#define DI0     7
#define BAND    868E6  // 915E6
#define PABOOST true 

// Our frequency 869700000
#define OUR_FREQ 869700000


int calibration_one_point; //for saving callibration point

int get_temp(){
  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));
  uint16_t reading = ADC;
  return reading - 273 + calibration_one_point; // they say it returns kelvin, we will see.
}

void temp_setup(){
  // listening to temp
  ADMUX = (  _BV(MUX2) | _BV(MUX1) | _BV(MUX0));
  ADCSRB = ADCSRB | _BV(MUX5); // for MUX5
  ADMUX |= (_BV(REFS1) | _BV(REFS0)); // The internal 2.56V voltage as reference
  // Enable ADC conversions
  ADCSRA = (_BV(ADEN));
  delay(50); // wait a bit for the adc circuit to stabilize

  // you first need to do a read to get the garbage out

  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  uint16_t reading = ADCW;
  delay(50); // wait a bit for the adc circuit to stabilize

  // reading for calibration
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));
  reading = ADC;
  calibration_one_point = ROOM_TEMP - reading;
}


/******************************
 * Database setup
 */

// 1024 bytes eeprom
#define TABLE_SIZE 1024
#define NEW_DATABASE 1

// define to fit which entry
struct LogEvent {
  char temp; // -128 tot 127 (denkik)
  int16_t next_wake_up_time;
} 
logEvent;

// The read and write handlers for using the EEPROM Library
void writer(unsigned long address, byte data)
{
  EEPROM.write(address, data);
}

byte reader(unsigned long address)
{
  return EEPROM.read(address);
}

// Create an EDB object with the appropriate write and read handlers
EDB db(&writer, &reader);


void setup() {
     Serial.begin(9600);
     while(!Serial);
     temp_setup();

     
     #if NEW_DATABASE == 1
     db.create(0, TABLE_SIZE, sizeof(logEvent));;
     #else 
     db.open(0);
     #endif

     


}

void loop() {
  // put your main code here, to run repeatedly:

}
