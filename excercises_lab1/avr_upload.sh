#!/bin/bash

# find the Arduino port
ARDUINO_UPLOAD_PORT="$(find /dev/ttyACM* | head -n 1)"

echo "Resetting board on ${ARDUINO_UPLOAD_PORT}."

# reset the Arduino
stty -F "${ARDUINO_UPLOAD_PORT}" 1200

# wait for it...
while :; do
    sleep 0.5
    [ -c "${ARDUINO_UPLOAD_PORT}" ] && break
done

echo "Board reset and back online."
echo "Starting upload."

# ...upload!
#avrdude "${OPTIONS[@]}"
avrdude -v -p atmega32u4 -c avr109 -P ${ARDUINO_UPLOAD_PORT}  -b 57600 -U flash:w:main.hex:i
