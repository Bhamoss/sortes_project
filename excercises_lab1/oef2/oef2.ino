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

int sensorPin = A1;    // select the input pin 
int sensorValue = 0;  // variable to store the value coming from the sensor
boolean connec = false;

void setup() {
  // declare the ledPin as an OUTPUT:
  Serial.begin(9600);
  while(!Serial){;}
  
  pinMode(LED_BUILTIN, OUTPUT);
  sensorValue = analogRead(sensorPin);
  // maps 0-3,3 or 5 to 0 to 1023
  // if sufficiently high
  if(sensorValue > 950){
    Serial.println("Pin connection detected.");
    connec = true;
  }
}

void loop() {
  // read the value from the sensor:
  sensorValue = analogRead(sensorPin);
  // maps 0-3,3 or 5 to 0 to 1023
  // if sufficiently high
  if(sensorValue > 950 && !connec){
     Serial.println("Pin connection detected.");
     connec = true;
  }
  else if(sensorValue <= 950 && connec) {
     Serial.println("Pin connection removed.");
     connec = false;
  }
  
  // wait a bit to not get insane amount of output
  delay(100);
}
