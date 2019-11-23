#include <SPI.h>
#include <LoRa.h>

// uncomment the section corresponding to your board
// BSFrance 2017 contact@bsrance.fr 

//  //LoR32u4 433MHz V1.0 (white board)
//  #define SCK     15
//  #define MISO    14
//  #define MOSI    16
//  #define SS      1
//  #define RST     4
//  #define DI0     7
//  #define BAND    433E6 
//  #define PABOOST true

//  //LoR32u4 433MHz V1.2 (white board)
//  #define SCK     15
//  #define MISO    14
//  #define MOSI    16
//  #define SS      8
//  #define RST     4
//  #define DI0     7
//  #define BAND    433E6 
//  #define PABOOST true 

  //LoR32u4II 868MHz or 915MHz (black board)
  #define SCK     15
  #define MISO    14
  #define MOSI    16
  #define SS      8
  #define RST     4
  #define DI0     7
  #define BAND    868E6  // 915E6
  #define PABOOST true 

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Ping Receiver");
  LoRa.setPins(SS,RST,DI0);
  if (!LoRa.begin(BAND,PABOOST )) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  onReceive(LoRa.parsePacket());
}

/**
 * Reads incoming message from Lora and prints it to serial com
 * does returns 0 if no message was received.
 * returns 1 if a message received
 */
void onReceive(int packetSize){
   if (packetSize == 0) return;
   Serial.print("Received message : ");
   String msg = "";
   while(LoRa.available()) {
      msg += (char) LoRa.read();
   }
   Serial.print(msg);
   Serial.print("'\n");
   sendMessage("Pong!");
  }

void sendMessage(String msg){
  delay(500);
  Serial.print("Sending msg.\n");
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
}
