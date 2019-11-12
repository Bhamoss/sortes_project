/*
  Analog Input

  Demonstrates analog input by reading an analog sensor on analog pin 0 and
  turning on and off a light emitting diode(LED) connected to digital pin 13.
  The amount of time the LED will be on and off depends on the value obtained
  by analogRead().

  The circuit:
  - potentiometer
    center pin of the potentiometer to the analog input 0
    one side pin (either one) to ground
    the other side pin to +5V
  - LED
    anode (long leg) attached to digital output 13
    cathode (short leg) attached to ground

  - Note: because most Arduinos have a built-in LED attached to pin 13 on the
    board, the LED is optional.

  created by David Cuartielles
  modified 30 Aug 2011
  By Tom Igoe

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/AnalogInput
*/




#include <EEPROM.h>

// 1024 bytes eeprom
#define TABLE_SIZE 1024

unsigned long start_time;

// 512 - header = 506 entries possible this way,
// could reduce to a byte be left most byte indicating event and using
// the 7 rest for 128 possible timestamps
struct ll {
  // max of 255 secs
  unsigned char time_stamp;
  char event;
  int16_t pointer;
} 
ll;


String input;

int sensorPin = A1;    // select the input pin 
int sensorValue = 0;  // variable to store the value coming from the sensor
boolean connec = false;

void setup() {
  // declare the ledPin as an OUTPUT:
  Serial.begin(9600);
  while(!Serial){;}
  Serial.setTimeout(100); // when not receiving input for 0.1 second, the 
  // string read function will call it a day and return
  
  Serial.println("Send \"init\" to initiate a database, anything else to open an existing one.");
  while(!Serial.available()){;}
  input = Serial.readStringUntil('\n');
  if(String("init").equals(input)){
    ll.pointer = -1;
    EEPROM.put(0,ll);
    Serial.println("Initiliased new list.");
    EEPROM.get(0, ll);
  }
  else {
    EEPROM.get(0, ll);
    int i = 0;
    if (ll.pointer != -1) {
      i++;
      while(ll.pointer != 0) {
        i++;
        EEPROM.get(ll.pointer, ll);
      }
    }
    Serial.print(i); Serial.println(" entries");
  }
  // reads input into a String, until it doesnt receive input for some time = timeout
  // do this to empty buffer
  Serial.readString();
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  /// if connected on setup, first connect / deconnect will be ignored

  start_time = millis();
}



void loop() {
  if (Serial.available()) {
    input = Serial.readStringUntil('\n');
    if(String("delete").equals(input)){
      ll.pointer = -1;
      EEPROM.put(0,ll);
    }
    
    // reads input into a String, until it doesnt receive input for some time = timeout
    // do this to empty buffer
    Serial.readString();

    Serial.println();
    EEPROM.get(0, ll);
    if (ll.pointer == -1){
      Serial.println("Empty database.");
    } else {
      Serial.println("Printing database.");
    }
    Serial.println();

    
    while(ll.pointer != -1 && ll.pointer != 0)
    {
      Serial.print(""); Serial.print(ll.time_stamp);
      Serial.print(" -> ");
      if (ll.event == 'o') {
        Serial.println("Pin connection detected.");
      }
      else {
        Serial.println("Pin connection removed.");
      }  

      // get the next one
      EEPROM.get(ll.pointer, ll); 
    }
    if (ll.pointer == 0){
      Serial.print(""); Serial.print(ll.time_stamp);
      Serial.print(" -> ");
      if (ll.event == 'o') {
        Serial.println("Pin connection detected.");
      }
      else {
        Serial.println("Pin connection removed.");
      }  
    }

    Serial.println();
    
  }
  
  
  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  // maps 0-3,3 or 5 to 0 to 1023
  // if sufficiently high
  if(sensorValue > 950 && !connec){
     Serial.println("Pin connection detected.");
     connec = true;

    // dumb linked list which puts at the back like array
    // is just excercise so i wont implement all the LL functionality
    // but this is skeleton. LL shows its own when an entry in the middle is removed
    // and somthing else can take its place, put they dont ask for it
    EEPROM.get(0,ll);
    if (ll.pointer == -1){
      ll.time_stamp = (unsigned char) ((millis() - start_time) / 1000 ); 
      Serial.println(ll.time_stamp);
      ll.event = 'o';
      ll.pointer = 0;
      EEPROM.put(0, ll);
    }
    else {
      int16_t prev = 0;
      while(ll.pointer != 0){
        prev = ll.pointer;
        EEPROM.get(prev,ll);
      }
      // if you would delete in the list, at this point you could look for a free place in memory
      int16_t new_address = prev + sizeof(ll);
       ll.time_stamp = (unsigned char) ((millis() - start_time) / 1000 ); 
       Serial.println(ll.time_stamp);
       ll.event = 'o';
       if (new_address  + sizeof(ll) > TABLE_SIZE){
          Serial.println("EEPROM full!!!!");
       }
       else
       {
          EEPROM.put(new_address, ll);
          
          EEPROM.get(prev, ll);
          ll.pointer = new_address;
          EEPROM.put(prev, ll);
       }
    }
     

     digitalWrite(LED_BUILTIN, HIGH); 
  }
  else if(sensorValue <= 950 && connec) {
     Serial.println("Pin connection removed.");
     connec = false;

     EEPROM.get(0,ll);
    if (ll.pointer == -1){
      ll.time_stamp = (unsigned char) ((millis() - start_time) / 1000 );
      Serial.println(ll.time_stamp); 
      ll.event = 'c';
      ll.pointer = 0;
      EEPROM.put(0, ll);
    }
    else {
      int16_t prev = 0;
      while(ll.pointer != 0){
        prev = ll.pointer;
        EEPROM.get(prev,ll);
      }
      // if you would delete in the list, at this point you could look for a free place in memory
      int16_t new_address = prev + sizeof(ll);
       ll.time_stamp = (unsigned char) ((millis() - start_time) / 1000 ); 
       Serial.println(ll.time_stamp);
       ll.event = 'c';
       if (new_address + sizeof(ll)  > TABLE_SIZE){  // no -1 becase table_size counts from 1
          Serial.println("EEPROM full!!!!");
       }
       else
       {
          EEPROM.put(new_address, ll);
          
          EEPROM.get(prev, ll);
          ll.pointer = new_address;
          EEPROM.put(prev, ll);
       }
    }

     digitalWrite(LED_BUILTIN, LOW);
  }
  

  
  
  // wait a bit to not get insane amount of output
  delay(20);
}
