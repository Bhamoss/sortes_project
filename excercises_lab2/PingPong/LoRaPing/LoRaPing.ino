#include <SPI.h>
#include <LoRa.h>
#include <EDB.h>
#include <EEPROM.h>

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

// define to fit which entry
struct LogEvent {
  char temp; 
  uint8_t  next_wake_up_time;
} 
logEvent;

int get_temp(){
  return 0;
}

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

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Ping sender");
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
   String msg = "";
   while(LoRa.available()) {
      msg += (char) LoRa.read();
   }
   int seconds = msg.substring(4).toInt();
   int temp = get_temp();
   LogEvent record;
   record.temp = static_cast<char>(temp);
   record.next_wake_up_time = seconds;
   EDB_Status status = db.appendRec(EDB_REC record);
   if (status ==EDB_TABLE_FULL){
    db.deleteRec(0);
    db.appendRec(EDB_REC record);
   }
   sendMessage(record.temp);
}


void sendMessage(char temp){
  LoRa.beginPacket();
  LoRa.print(temp);
  LoRa.endPacket();
}
