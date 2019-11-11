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



#include <EDB.h>
#include <EEPROM.h>

// 1024 bytes eeprom
#define TABLE_SIZE 1024

unsigned long start_time;

// 512 - header = 506 entries possible this way,
// could reduce to a byte be left most byte indicating event and using
// the 7 rest for 128 possible timestamps
struct LogEvent {
  // max of 255 secs
  unsigned char time_stamp;
  char event;
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
    db.create(0, TABLE_SIZE, sizeof(logEvent));;
  }
  else {
    db.open(0);
  }
  // reads input into a String, until it doesnt receive input for some time = timeout
  // do this to empty buffer
  Serial.readString();
  
  pinMode(LED_BUILTIN, OUTPUT);

  
  Serial.print("Record Count: "); Serial.println(db.count());
  
  /// if connected on setup, first connect / deconnect will be ignored

  start_time = millis();
}



void loop() {

  if (Serial.available()) {
    input = Serial.readStringUntil('\n');
    if(String("delete").equals(input)){
      db.clear();
    }
    
    // reads input into a String, until it doesnt receive input for some time = timeout
    // do this to empty buffer
    Serial.readString();

    Serial.println();
    if (db.count() == 0){
      Serial.println("Empty database.");
    } else {
      Serial.println("Printing database.");
    }
    Serial.println();

    for (int recno = 1; recno <= db.count(); recno++)
    {
      db.readRec(recno, EDB_REC logEvent);
      Serial.print(""); Serial.print(logEvent.time_stamp);
      Serial.print(" -> ");
      if (logEvent.event == 'o') {
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
     
     logEvent.time_stamp = (unsigned char) ((millis() - start_time) / 1000 ); 
     logEvent.event = 'o';
     if (db.appendRec(EDB_REC logEvent) == EDB_TABLE_FULL){
        Serial.println("Database full!!!!");
     }
     
     Serial.println(logEvent.time_stamp);

     digitalWrite(LED_BUILTIN, HIGH); 
  }
  else if(sensorValue <= 950 && connec) {
     Serial.println("Pin connection removed.");
     connec = false;

     logEvent.time_stamp = (unsigned char) ((millis() - start_time) / 1000 ); 
     logEvent.event = 'c';
     if (db.appendRec(EDB_REC logEvent) == EDB_TABLE_FULL){
        Serial.println("Database full!!!!");
     }
     
     Serial.println(logEvent.time_stamp);

     digitalWrite(LED_BUILTIN, LOW);
  }
  

  
  
  // wait a bit to not get insane amount of output
  delay(20);
}
