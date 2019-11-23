



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


/*******************************
 * Timer setup
 */

// wait twice and save remaing x - 8,6 seconds for second round of waiting

void setTimer(uint16_t msecs){
  TCNT1 = 2^16 -  (uint16_t) (7,8125 * msecs);
}


/*****************************************
 * Sleeping setup
 **/

SemaphoreHandle_t sleep_semaphore;

void ultra_low_power();
void low_power();


/******************************************
 * FreeRTOS setup 
 */

        /*************** task 1 setup 20s receive ********************/

int8_t nb_beacons = 0;
SemaphoreHandle_t receiving_interruptSemaphore;

ISR(TIMER1_OVF_vect) {
  /**
   * Give semaphore in the interrupt handler
   * https://www.freertos.org/a00124.html
   */
  
  xSemaphoreGiveFromISR(receiving_interruptSemaphore, NULL);
}


/* 
 * receiving 
 */
void receiving(void *pvParameters)
{
  (void) pvParameters;

  //pinMode(LED_BUILTIN, OUTPUT);

  for (;;) {
    /**
     * Take the semaphore.
     * https://www.freertos.org/a00122.html
     */
    if (xSemaphoreTake(receiving_interruptSemaphore, portMAX_DELAY) == pdPASS) {
      //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      // the receiving task, listening, writing, sending, sleeping

      // bij startup, genereer manueel timer overflow interrupt
      // DUS zet timer op die interrupt

      // na 20 keer zet de timer overflow interrupt naar 0
      while(!LoRa.available());
      onReceive(LoRa.parsePacket());
      nb_beacons++;
      if(nb_beacons == 20){
        // zet de interupt flag af en disable lora module
        //TIMSK1 &= ~(_BV(TOIE1));
        power_timer1_disable() ;
      }
    }
    
  }
}

void onReceive(int packetSize){
   if (packetSize == 0) return;
   String msg = "";
   while(LoRa.available()) {
      msg += (char) LoRa.read();
   }
   int seconds = msg.substring(4).toInt();
   int temp = get_temp();
   LogEvent record;
   record.temp = static_cast<char>(temp);
   record.next_wake_up_time = seconds;
   //Take sem here
   EDB_Status status = db.appendRec(EDB_REC record);
   if (status == EDB_TABLE_FULL){
    db.deleteRec(0);
    db.appendRec(EDB_REC record);
   }
   // give sem
   sendMessage(record.temp);
}


void sendMessage(char temp){
  LoRa.beginPacket();
  LoRa.print(temp);
  LoRa.endPacket();
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
ISR(WAKEUPI) {
  /**
   * Give semaphore in the interrupt handler
   * https://www.freertos.org/a00124.html
   */
  
  xSemaphoreGiveFromISR(serial_interruptSemaphore, NULL);
}


/* 
 * receiving 
 */
void serialTask(void *pvParameters)
{
  (void) pvParameters;

  //pinMode(LED_BUILTIN, OUTPUT);

  for (;;) {
    
    /**
     * Take the semaphore.
     * https://www.freertos.org/a00122.html
     */
    if (xSemaphoreTake(receiving_interruptSemaphore, portMAX_DELAY) == pdPASS) {
      //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      // the read in 1 2 or 3 and do it
      if(Serial.available()){ // zou overbodig moeten zijn
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
      ultra_low_power();
    }
    
  }
}







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
SemaphoreHandle_t database_semaphore;

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

  #if (POWER_OPT_LEVEL >= 5)
    clock_prescale_set(clock_div_128);
  #endif

  #if (POWER_OPT_LEVEL >= 3)
      //wdt disable
      wdt_disable();
    #endif

  #if (POWER_OPT_LEVEL >= 3)
      //jtag disable
      MCUCR &= ~(_BV(JTD))
    #endif
  
  // http://www.gammon.com.au/forum/?id=11497
  // https://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
  // TODO disable everything you do not need!
  #if (POWER_OPT_LEVEL >= 2)
    power_all_disable();
  //power_usb_enable() ; not necessary in phase 1
    power_timer1_enable() ;
    power_spi_enable();
    power_adc_enable();
  #endif

  
  /***********************************************
   * FreeRTOS setup
   */

  /************************** receiving **************************/


  // Create task for receiving 
  xTaskCreate(receiving, // Task function
              "Receiving", // Task name
              1024, // Stack size 
              NULL, 
              0, // Priority
              NULL );


  //TODO manually configure timer

  
  /**
   * Create a binary semaphore.
   * https://www.freertos.org/xSemaphoreCreateBinary.html
   */
  receiving_interruptSemaphore = xSemaphoreCreateBinary();
  if (receiving_interruptSemaphore != NULL) {
    // TODO Attach interrupt for timer
    //attachInterrupt(digitalPinToInterrupt(2), receiving_interruptHandler, LOW);
    // this is now done by the isr
  }

  /************************** serial **************************/
  /**
   * Create a binary semaphore.
   * https://www.freertos.org/xSemaphoreCreateBinary.html
   */


  // Create task for receiving 
  xTaskCreate(serialTask, // Task function
              "Serial", // Task name
              1024, // Stack size 
              NULL, 
              0, // Priority
              NULL );

   
  serial_interruptSemaphore = xSemaphoreCreateBinary();
  if (serial_interruptSemaphore != NULL) {
    // TODO Attach interrupt for serial ASYNCHRONOUS
    //attachInterrupt(digitalPinToInterrupt(2), serial_interruptHandler, LOW);
  }


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
  
  database_semaphore = xSemaphoreCreateBinary(); // sempahore always created empty so give
  xSemaphoreGive(database_semaphore);
  /**********************************************
   * LoRa setup
   */
   
   //LoRa.begin(OUR_FREQ);


  // start serial only when you need it?

  Serial.begin(9600);
  while(!Serial);
  UDIEN |= _BV(WAKEUPE); // for the WAKEUPE interrupt
  // usb_disable()

  

  sleep_semaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(database_semaphore);

  /**************************************
   * Timer setup
   */

   TCCR1A = 0; // geen compare mode
   TCCR1B =  _BV(CS12)| _BV(CS10) ;//  CS12/1/0 = 101 -> 1024 prescaler en 8 Mhz (125 nanoseconds voor 1 wave zonder prescaler) -> 125*1024 nanoseconds voor 1 increment -> 128 micro seconds = 0,128 ms
   // 7.8125 ticks per ms -> 7812,5 per seconde en 65536 increments -> ongeveer 8,6 seconden max
   // dus als x aantal seconden  = max{0, 65536 - 7812,5 * x}  -> sowieso wakker na 8,6 seconden.
   TIMSK1 = _BV(TOIE1); // timer overflow bit interrupt enabled
  
  //TODO set timer op 1 seconde om tijd te geven om in slaap te vallen
  // setTimer(1000ms);

  setTimer(100);
  
  // low power has interupts enabled when it exits (no need to call it here)
  low_power();
}

void loop() {
  ultra_low_power();
}


void low_power(){
  if (xSemaphoreTake(sleep_semaphore, portMAX_DELAY) == pdTRUE) {
    LoRa.sleep();
    set_sleep_mode(SLEEP_MODE_STANDBY); // we have interrupt on the clock running
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
    
  }
  xSemaphoreGive(sleep_semaphore);
}


// zie pagina  45
void ultra_low_power(){
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  //ultra_low_power();
}
