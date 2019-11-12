/*
  DigitalReadSerial

  Reads a digital input on pin 2, prints the result to the Serial Monitor

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/DigitalReadSerial
*/

#include <stdio.h>

int blink_rate = 0;
boolean blink = false;

void setup() {
  // initialize serial communication at 9600 bits per second:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while(!Serial){;}

  
  char tbs[200];

  Serial.println("Enter LED status (on/off):");
  // The terminator character is discarded from the serial buffer.
  while (!Serial.available()){}
  String input = Serial.readStringUntil('\n');
  
  
  if (String("on").equals(input)) {
    
    Serial.println("Enter blink rate (1-60):");
    while (!Serial.available()){}
    blink_rate = Serial.parseInt();
    
    if(blink_rate >= 1 && blink_rate <= 60) {
        sprintf(tbs, "You have selected LED on. Blink rate is %d sec.", blink_rate);
        Serial.println(tbs);
        blink = true;
    }
    else{
      Serial.println("You have selected LED off (invalid blink rate).");
      blink = false;
    }
  }
  else {
    Serial.println("You have selected LED off.");
    blink = false;
  }
}

// the loop routine runs over and over again forever:
void loop() {
  if(blink){
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(blink_rate * 1000);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(blink_rate * 1000);  
  }
}
