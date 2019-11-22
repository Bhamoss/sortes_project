



// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>
// Include semaphore supoport
#include <semphr.h>
#include <EDB.h>
#include <EEPROM.h>
#include <SPI.h>              
#include <LoRa.h>
#include <avr/sleep.h>

/******************************************
 * FreeRTOS setup 
 */


/*******************************************
 * Temp setup
 * Temp assumes 
 */
#define ROOM_TEMP 21
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




//     logEvent.temp = temp; 
//     logEvent.next_wake_up_time = next_wake_up_time;
//     if (db.appendRec(EDB_REC logEvent) == EDB_TABLE_FULL){
//        Serial.println("Database full!!!!");
//     }



/****************************
   * LoRa setup
   */



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

void setup() {
  /***********************************************
   * FreeRTOS setup
   */


  /**********************************************
   * temp setup
   */

   temp_setup();


  /**********************************************
   * Database setup
   */

  #if NEW_DATABASE == 1
    db.create(0, TABLE_SIZE, sizeof(logEvent));;
  #else 
    db.open(0);
  #endif
  /**********************************************
   * LoRa setup
   */
   
   //LoRa.begin(OUR_FREQ);


  // start serial only when you need it?

  //TODO go to sleep after setup
}

void loop() {
  ultra_low_power();
}


void low_power(){
  //TODO figure out how to make this as low as possible
  //TODO turn of usb during setup?
  // TODO peripherals disable
  // TODO mutex op in slaap gaan?
  //TODO turn of ADC of temp an reenable when reading temp
  LoRa.sleep();
  set_sleep_mode(SLEEP_MODE_STANDBY); // we have interrupt on the clock running
  // DISABLE OTHER TASK SO YOU DO NOT GO INTO POWER DOWN AND MISS CLOCK
  cli(); // safely disable interrupts
  sleep_enable();
//  sleep_bod_disable(); // brown out disable
  sei(); // enable interrupts AFTER execution of next line
  sleep_cpu();
  sleep_disable();
}


// zie pagina  45
void ultra_low_power(){
  //TODO figure out how to make this as low as possible
  //TODO turn of usb during setup?
  // TODO peripherals disable
  // TODO mutex op in slaap gaan?
  /**
   *  Turn it off if you are not using it, e.g. LEDs, ADC, Clocks, Brown out
Detection (BoD) and Peripherals.
 Deterministic state beats undefined state. Floating pins leach energy, so set
their state in sleep mode.

   */
  LoRa.sleep();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli(); // safely disable interrupts
  sleep_enable();
  byte adcsra = ADCSRA;                     //save ADCSRA
  byte adcsrb = ADCSRB;                     //save ADCSRB
  ADCSRA &= ~_BV(ADEN);                     //disable ADC
  //sleep_bod_disable(); // brown out disable
  sei(); // enable interrupts AFTER execution of next line
  sleep_cpu();
  // the next is executed when you come back to sleep
  // so shit that always needs to be reenabled you have to put here
  sleep_disable();
  ADCSRA = adcsra;                          //restore ADCSRA
  ADCSRB = adcsrb;                          //restore ADCSRB
}
