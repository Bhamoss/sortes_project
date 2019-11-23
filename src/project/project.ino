



// Include Arduino FreeRTOS library
#include <Arduino_FreeRTOS.h>
// Include semaphore supoport
#include <semphr.h>
#include <EDB.h>
#include <EEPROM.h>
#include <SPI.h>              
#include <LoRa.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#define POWER_OPT_LEVEL 0
#define DEBUGGING 1


// define to fit which entry
struct LogEvent {
  int8_t temp; // -128 tot 127 (denkik)
  int8_t next_wake_up_time;
} 
logEvent;
SemaphoreHandle_t database_semaphore;


/*******************************
 * Timer setup
 */

// wait twice and save remaing x - 8,6 seconds for second round of waiting

void setTimer(uint16_t msecs){
  TCNT1 = (2^16 -   (uint16_t)((7.8125) * ((double) msecs)));
}


/*****************************************
 * Sleeping setup
 **/

SemaphoreHandle_t sleep_semaphore;

void ultra_low_power();
void low_power();







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
//SemaphoreHandle_t database_semaphore;

// 1024 bytes eeprom
#define TABLE_SIZE 1024
#define NEW_DATABASE 1

// define to fit which entry
//struct LogEvent {
//  int8_t temp; // -128 tot 127 (denkik)
//  int8_t next_wake_up_time;
//} 
//logEvent;

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






/******************************************
 * FreeRTOS setup 
 */

        /*************** task 1 setup 20s receive ********************/

#define EPSILON 1000
int8_t nb_beacons = 0;
SemaphoreHandle_t receiving_interruptSemaphore;

ISR(TIMER1_OVF_vect) {

  // if remaining > 0 : setTimer(remaining); sleep (when coming in idle task?; else:
  #if (DEBUGGING == 1)
    Serial.println("In timer vector");
  #endif
  xSemaphoreGiveFromISR(receiving_interruptSemaphore, NULL);
}


/* 
 * receiving 
 */
void receiving(void *pvParameters)
{
  (void) pvParameters;

  for (;;) {

    if (xSemaphoreTake(receiving_interruptSemaphore, portMAX_DELAY) == pdPASS) {
      #if (DEBUGGING == 1)
        Serial.println("In lora");
      #endif
      while(LoRa.parsePacket() == 0);
      #if (DEBUGGING == 1)
        Serial.println("Lora received packet");
        //delay(1000);
      #endif
      onReceive(LoRa.parsePacket());
      #if (DEBUGGING == 1)
        Serial.println("Lora sent packet");
      #endif
      nb_beacons++;
      
      if(nb_beacons == 20){
        #if (DEBUGGING == 1)
        Serial.println("20 beacons");
        delay(500);
      #endif
        TIMSK1 = 0; // DISABLE THE TIMER INTERRUPT of //TIMSK1 &= ~(_BV(TOIE1));  _BV(TOIE1); // timer overflow bit interrupt enabled
        LoRa.end(); // Stop LoRa completely
        Serial.begin(9600);
        while(!Serial);
      }
      if(nb_beacons < 20)
      {
        low_power();
      }
      else
      {
        ultra_low_power(); // transition 
      }
    }
    
    
  }
}

void onReceive(int packetSize){

   String msg = LoRa.readString();
   int8_t seconds = (int8_t) msg.substring(4).toInt();

   setTimer(seconds*1000 - EPSILON);

   
   int temp = get_temp();

  // Database write
  #if DEBUGGING == 1
    Serial.print("temp "); Serial.print(temp);Serial.print(", seconds "); Serial.print(seconds); Serial.print(", waiting ms "); Serial.println(seconds*1000 - EPSILON);
    //delay(500);
  #endif
   
   logEvent.temp = (int8_t)(temp);
   logEvent.next_wake_up_time = seconds;
   if (xSemaphoreTake(database_semaphore, portMAX_DELAY) == pdTRUE) {
     EDB_Status status = db.appendRec(EDB_REC logEvent);
     if (status == EDB_TABLE_FULL){
      db.deleteRec(0); //TODO optimise (hou pointer naar oudste en overschrijf die met nieuwst O(n) -> O(1)
      db.appendRec(EDB_REC logEvent);
     }
   }
   xSemaphoreGive(database_semaphore);

  // sending packet back with temp
  
  LoRa.beginPacket();
  LoRa.print(logEvent.temp);
  LoRa.endPacket(); // busy wait until module done sending
}





         /*************** task 2 serial input ********************/


SemaphoreHandle_t serial_interruptSemaphore;

//TODO which flag
// there are 2 async usb flags, VBUSI = voltage over bus change aka unplugging or plugging I think
// and WAKEUPI
/***
 * There are no relationship between the SUSPI interrupt and the WAKEUPI interrupt: the WAKEUPI interrupt is
triggered as soon as there are non-idle patterns on the data lines. Thus, the WAKEUPI interrupt can occurs
even if the controller is not in the “suspend” mode.
When the WAKEUPI interrupt is triggered, if the SUSPI interrupt bit was already set, it is cleared by hardware.
When the SUSPI interrupt is triggered, if the WAKEUPI interrupt bit was already set, it is cleared by hardware.
Set by hardware when the USB controller is re-activated by a filtered non-idle signal from the lines (not by an
upstream resume). This triggers an interrupt if WAKEUPE is set.
Shall be cleared by software (USB clock inputs must be enabled before). Setting by software has no effect.

 */


// USB_COM_vect
// USB_GEN_vect
//#define LOL USB_COM_vect

  ISR(TIMER1_CMP_vect) {
    #if (DEBUGGING == 1)
        Serial.println("serial isr");
        //delay(2000);
    #else
     xSemaphoreGiveFromISR(serial_interruptSemaphore, NULL);
    #endif
  }



/* 
 * receiving 
 */
void serialTask(void *pvParameters)
{
  (void) pvParameters;

  for (;;) {
    
    if (xSemaphoreTake(receiving_interruptSemaphore, portMAX_DELAY) == pdPASS) {
      while(Serial.available()){ 
        char command = Serial.read();
        if(command == '1'){
          if (xSemaphoreTake(database_semaphore, portMAX_DELAY) == pdTRUE) {
            if(db.count() != 0){
              db.readRec(db.count(), EDB_REC logEvent);
              Serial.print(logEvent.temp); Serial.print(" degrees at "); Serial.println(logEvent.next_wake_up_time);
              if(db.limit() == db.count()){
                Serial.println("WARNING: database full.");
              }
            }
            else
            {
              Serial.println("Table empty");
            }
          }
          xSemaphoreGive(database_semaphore);
        }
        else if(command == '2'){
          if (xSemaphoreTake(database_semaphore, portMAX_DELAY) == pdTRUE) {
            if(db.count() == 0){
              Serial.println("Database empty.");
            }
            for (int recno = 1; recno <= db.count(); recno++)
            {
              db.readRec(recno, EDB_REC logEvent);
              Serial.print(logEvent.temp); Serial.print(" degrees at "); Serial.println(logEvent.next_wake_up_time);
            }
            if(db.limit() == db.count()){
              Serial.println("WARNING: database full.");
            }
          }
          xSemaphoreGive(database_semaphore);
        }
        else if(command == '3'){
          Serial.println("Entering ultra low power mode.");
          // this happens always?
        }
        else if(command == '\n'){
          Serial.println("EOL character included in transmission."); // TODO delete this line
        }
        else{
          Serial.println("Illegal command.");
        }
      }
      #if (DEBUGGING == 1)
        Serial.println("serial ulta deep");
        delay(2000);
      #endif
      //ultra_low_power();
    }
    
  }
}














void setup() {
  #if (DEBUGGING == 1)
    Serial.begin(9600);
    while(!Serial);
    delay(500);
  #endif

  /***********************************************************
   *  Sleep and powersaving setup
   */

 // #if (POWER_OPT_LEVEL >= 5)
    //clock_prescale_set(clock_div_128); // WATCH OUT FOR TIMER
  //#endif

  //#if (POWER_OPT_LEVEL >= 4)
      //wdt disable
      //wdt_disable();
   // #endif

  //#if (POWER_OPT_LEVEL >= 3)
      //jtag disable
      //MCUCR &= ~(_BV(JTD))
   // #endif
  
  // http://www.gammon.com.au/forum/?id=11497
  // https://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
  // TODO disable everything you do not need!
  //#if (POWER_OPT_LEVEL >= 2)
    //power_all_disable();
  //power_usb_enable() ; not necessary in phase 1
    //power_timer1_enable() ;
    //power_spi_enable();
    //power_adc_enable();
  //#endif


  #if (DEBUGGING == 1)
    Serial.println("creating sleep semaphore");
    delay(500);
  #endif

  sleep_semaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(sleep_semaphore);

  
  
  /**********************************************
   * temp setup
   */

  #if (DEBUGGING == 1)
    Serial.println("setting up temp");
    delay(500);
  #endif

   temp_setup();


  /**********************************************
   * Database setup
   */

   #if (DEBUGGING == 1)
    Serial.println("Setting up database and semaphore");
    delay(500);
  #endif

  #if NEW_DATABASE == 1
    db.create(0, TABLE_SIZE, sizeof(logEvent));;
  #else 
    db.open(0);
  #endif
  
  database_semaphore = xSemaphoreCreateBinary(); // sempahore always created empty so give
  xSemaphoreGive(database_semaphore);
  
  /**********************************************
   * LoRa setup
   */

  #if (DEBUGGING == 1)
    Serial.println("LoRa setpins");
    delay(500);
  #endif

  LoRa.setPins(SS,RST,DI0);// set CS, reset, IRQ pin

  #if (DEBUGGING == 1)
    Serial.println("LoRa begin");
    delay(500);
  #endif
  
  LoRa.begin(OUR_FREQ,PABOOST);


  // TODO serial interrupt when we receive mail reply
  // Serial only necessary in phase 2

  #if (DEBUGGING == 1)
    Serial.println("USB flag");
    delay(500);
  #endif
  
  //UDIEN |= _BV(WAKEUPE); // for the WAKEUPE interrupt -> UDINT && _BV(WAKEUPI)
  // usb_disable()

  /**
   * 
   * 11
 $0014
 USB General
 USB General Interrupt request
12
 $0016
 USB Endpoint
 USB Endpoint Interrupt request

   */

  

  /**************************************
   * Timer setup
   */

   #if (DEBUGGING == 1)
    Serial.println("creating timer1");
  #endif

   TCCR1A = 0; // geen compare mode
   TCCR1B =  _BV(CS12)| _BV(CS10) ;//  CS12/1/0 = 101 -> 1024 prescaler en 8 Mhz (125 nanoseconds voor 1 wave zonder prescaler) -> 125*1024 nanoseconds voor 1 increment -> 128 micro seconds = 0,128 ms
   // 7.8125 ticks per ms -> 7812,5 per seconde en 65536 increments -> ongeveer 8,6 seconden max
   // dus als x aantal seconden  = max{0, 65536 - 7812,5 * x}  -> sowieso wakker na 8,6 seconden.
   TIMSK1 = _BV(TOIE1); // timer overflow bit interrupt enabled




  /***********************************************
   * FreeRTOS setup
   */

  /************************** receiving **************************/

  #if (DEBUGGING == 1)
    Serial.println("creating receiving task & semaphore");
  #endif

  // Create task for receiving 
  xTaskCreate(receiving, // Task function
              "Receiving", // Task name
              128, // Stack size 
              NULL, 
              1, // Priority
              NULL );
              
  receiving_interruptSemaphore = xSemaphoreCreateBinary();
  //attachInterrupt(digitalPinToInterrupt(2), interruptHandler, LOW);


  /************************** serial **************************/

  #if (DEBUGGING == 1)
    Serial.println("creating serial task ");
  #endif

  // Create task for serial  
  xTaskCreate(serialTask, // Task function
              "Serial", // Task name
              128, // Stack size 
              NULL, 
              1, // Priority
              NULL );
  #if (DEBUGGING == 1)
    Serial.println("creating serial semaphore");
  #endif
   
  serial_interruptSemaphore = xSemaphoreCreateBinary();















  
  /*********************************************
   * Start by going to sleep for 100 ms which will cause to busy wait for packet after you wake up in 100 ms.
   */

  #if (DEBUGGING == 1)
    Serial.println("setting timer for a second");
  #endif

  setTimer(5000);

  #if (DEBUGGING == 1)
    Serial.println("going into low power mode");
  #endif
  // low power has interupts enabled when it exits (no need to call it here)
  low_power();

  #if (DEBUGGING == 1)
    Serial.println("woke up from low power mode");
  #endif
}

void loop() {
  //low_power();
}


void low_power(){
  if (xSemaphoreTake(sleep_semaphore, portMAX_DELAY) == pdTRUE) {

  #if (DEBUGGING == 1)
    Serial.println("putting lora to low sleep");
  #endif
    
    LoRa.sleep();


    
    set_sleep_mode(SLEEP_MODE_IDLE); // we have interrupt on the clock running
    // DISABLE OTHER TASK SO YOU DO NOT GO INTO POWER DOWN AND MISS CLOCK


    
    cli(); // safely disable interrupts
    
    //LoRa.sleep(); do it after you get the 20th packet
    #if (POWER_OPT_LEVEL >= 2)
      power_all_disable();
      power_timer1_enable() ;
      power_spi_enable();
      power_adc_enable();
    #endif


    
    sleep_enable();
    
    #if (POWER_OPT_LEVEL >= 1)
      //sleep_bod_disable(); // brown out disable
      // you have to set fuses with avrdude for bod disable
    #endif
    
    sei(); // enable interrupts AFTER execution of next line
    
    sleep_cpu();

  
    
    sleep_disable();
    #if (POWER_OPT_LEVEL >= 2)
      power_spi_enable();
      power_adc_enable();
    #endif
    #if (DEBUGGING == 1)
    Serial.println("uit low power");
    delay(500);
  #endif
  }
  xSemaphoreGive(sleep_semaphore);
}


// zie pagina  45
void ultra_low_power(){
  #if (DEBUGGING == 1)
        Serial.println("In ultra low power");
        delay(2000);
      #endif
  if (xSemaphoreTake(sleep_semaphore, portMAX_DELAY) == pdTRUE) {
    //TODO port pin registers should be 0 to not consume power
    // TODO disable brown out fuses
    /**
     *  Turn it off if you are not using it, e.g. LEDs, ADC, Clocks, Brown out
  Detection (BoD) and Peripherals.
  Deterministic state beats undefined state. Floating pins leach energy, so set
  their state in sleep mode.

    */

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli(); // safely disable interrupts

    //LoRa.sleep(); do it after you get the 20th packet
    #if (POWER_OPT_LEVEL >= 2)
      power_all_disable();
      power_usb_enable();
      //power_usi_enable() ;
    #endif
    
    sleep_enable();

    
    
    #if (POWER_OPT_LEVEL >= 1)
      //sleep_bod_disable(); // brown out disable does not work for our processor
      // you have to set fuses with avrdude for bod disable
    #endif
    sei(); // enable interrupts AFTER execution of next line
    sleep_cpu();
    // the next is executed when you come back to sleep
    // so shit that always needs to be reenabled you have to put here
    sleep_disable();
    //TODO save 2,56 votl?
    //ADCSRA = adcsra;                          //restore ADCSRA
    //ADCSRB = adcsrb;                          //restore ADCSRB
    #if (POWER_OPT_LEVEL >= 2)
    //byte adcsra = ADCSRA;                     //save ADCSRA
    //byte adcsrb = ADCSRB;                     //save ADCSRB
    //ADCSRA &= ~_BV(ADEN);                     //disable ADC
    #endif
  }
  xSemaphoreGive(sleep_semaphore);
}
// http://www.gammon.com.au/forum/?id=11497
//

void vApplicationIdleHook( void ){
  // should never be entered
  // could turn on led if we ever enter this to check
  

  low_power();
}
