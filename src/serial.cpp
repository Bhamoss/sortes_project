
#include "Arduino.h"
#include "communication_serial.h"

int blinkRate = 0;
boolean blink = false;


/**
 * Initialisation code for arduino
*/
void setup()
{
    Serial.begin(9600);
    //Makes sure that initial setup that requires user input is fulfilled.
    while (!Serial);

    writeStringToSerial("Enter LED status (on/off):");
    char * answer = (char *) malloc(2);
    readStringFromSerial(answer,2);
    char final_message[100];
    
    int result = strcmp(answer,"on");
    sprintf(final_message,"answer = %s\n comparison = %d",answer,result);
    writeStringToSerial(final_message);
    if ( result == 0){
        
        writeStringToSerial("Enter the blink rate (1-60 sec):");
        char * blinkrateS = (char *) malloc(2);
        readStringFromSerial(blinkrateS,2);
        blinkRate = atoi(blinkrateS);
        
        sprintf(final_message,"You have selected LED on. Blink rate is %d sec",blinkRate);
        writeStringToSerial(final_message);
        pinMode(LED_BUILTIN, OUTPUT);
        blink = true;
        free(blinkrateS);


    }else{
        sprintf(final_message,"You have selected LED off. Blink rate is %d sec",blinkRate);
        writeStringToSerial(final_message);
        
    }

    free(answer);
}

/**
 * Function that will be called over and over
 * Has local scope any variable declared in the function will be lost next time loop is called
*/
void loop()
{
    if (blink) {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);

        // wait for a second
        delay(1000);

        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);

        // wait for a second
        delay(blinkRate * 1000);
    }
    
}