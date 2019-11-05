#include "Arduino.h"
#include "communication_serial.h"


/**
 * This assumes Serial.begin was called either in setup or in loop
 * Will write the message to the minicom
*/
void writeStringToSerial(const char* str){
    Serial.println(str);
}

/**
 * Will wait until a string is provided by user with endmarker
 * will return a pointer to allocated memory. Memory needs to be freed after usage.
*/
void readStringFromSerial(char * str , const byte numChars){
    byte ndx = 0;
    char endMarker = '\n';
    char rc;
    boolean msg_received = false;

    while(!msg_received){
        if (Serial.available() > 0){
            rc = Serial.read();

            if (rc != endMarker) {
                str[ndx] = rc;
                ndx++;
                if( ndx >= numChars){
                    ndx = ndx -1;
                }
            }
            else {
                msg_received = true;
            }
        }
    }
}





