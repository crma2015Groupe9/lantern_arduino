/*==============================*/
/*======Liste des features======*/
/*==============================*/

//bluetooth in/out OK
//Controle des NeoPixels TO DO
//Open/close detection TO DO
//Detecteur infrarouge TO DO
//Lecture du son TO DO
//Controle du volume TO DO
//Led d'état red/green TO DO

/*==============================*/
/*==============================*/
/*==============================*/

#include <SPI.h>
#include <Adafruit_BLE_UART.h>
//#include <SD.h>
#include <Adafruit_NeoPixel.h>
//#include <TMRpcm.h>

#include <Tween.h>
#include <TimeManager.h>
#include <Colors.h>

//#define SPEAKER_PIN 6
//#define SD_CARD_PIN 10
//TMRpcm audio;

/*==============================*/
/*=====Gestion du bluetooth=====*/
/*==============================*/

#define REQ_PIN 8
#define RDY_PIN 2
#define RST_PIN 7

Adafruit_BLE_UART bluetooth = Adafruit_BLE_UART(REQ_PIN, RDY_PIN, RST_PIN);

void sendMessage(char *messageToSend, uint8_t lengthOfMessageToSend){
  uint8_t *buffer;
  buffer[0] = 0;
  buffer[1] = 0;
  
  for(byte i=0;i<lengthOfMessageToSend;i++){
    buffer[i+2] = messageToSend[i];
  }
  
  bluetooth.write(buffer, lengthOfMessageToSend+2);
}

void rxCallback(uint8_t *buffer, uint8_t len)
{
  /*if((char)buffer[0] == 'a'){
    playSound();
  }

  if((char)buffer[0] == 'b'){
    sendMessage("hello", 5);
  }*/
}

/*==============================*/
/*==============================*/
/*==============================*/

TimeManager time;

void setup(void)
{ 
  Serial.begin(9600);
  while(!Serial);
  
  /*pinMode(SPEAKER_PIN, OUTPUT);

  audio.speakerPin = SPEAKER_PIN;
  if(!SD.begin(SD_CARD_PIN)){
    return;
  }
  audio.volume(1);
  audio.setVolume(7);
  audio.play("klax");

  delay(2000);*/

  bluetooth.setRXcallback(rxCallback);
  bluetooth.setDeviceName("LANTERN");
  bluetooth.begin();

  time.init();
}

void loop()
{
  time.loopStart();

  bluetooth.pollACI();

  time.loopEnd();
}
