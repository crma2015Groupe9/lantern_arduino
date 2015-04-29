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

/*===============================*/
/*=====Gestion des neopixels=====*/
/*===============================*/

#define LEDS_PIN 9
#define NUMBER_OF_LEDS 2

Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUMBER_OF_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800);


void rgb(byte index, byte red, byte green, byte blue){
  leds.setPixelColor(index, green, red, blue);
}

void rgb(byte red, byte green, byte blue){
  byte i;
  for(i=0;i<NUMBER_OF_LEDS;i++){
    rgb(i, red, green, blue);
  }  
}
/*==============================*/
/*=====Gestion du bluetooth=====*/
/*==============================*/

#define REQ_PIN 10
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
  if((char)buffer[0] == 'a'){
    //Serial.println("red and green");
    rgb(0, 255, 0, 0);
    rgb(1, 0, 255, 0);
    leds.show();
  }
  
  if((char)buffer[0] == 'b'){
    //Serial.println("purple and blue");
    rgb(0, 255, 0, 250);
    rgb(1, 0, 0, 255);
    leds.show();
  }

  if((char)buffer[0] == 'c'){
    sendMessage("hello", 5);
  }
}

/*==============================*/
/*==============================*/
/*==============================*/

TimeManager time;

void setup(void)
{ 
  Serial.begin(9600);
  while(!Serial);
  
  leds.begin();
  leds.show();
  
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
  
  rgb(15, 15, 0);
  leds.show();

  time.loopEnd();
}
