/*==============================*/
/*======Liste des features======*/
/*==============================*/

//bluetooth in/out OK
//Controle des NeoPixels IN PROGRESS
//Communication avec la secondary Board OK
//Open/close detection TO DO
//Detecteur infrarouge TO DO
//Controle du volume TO DO
//Led d'état red/green TO DO

/*==============================*/
/*==============================*/
/*==============================*/

#include <SPI.h>
#include <Adafruit_BLE_UART.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <EasyTransferI2C.h>

#include <Tween.h>
#include <TimeManager.h>
#include <Colors.h>

/*==================================================*/
/*=========Communication avec la secondary board=========*/
/*==================================================*/

#define MAIN_BOARD_ADDRESS 4
#define SECONDARY_BOARD_ADDRESS 9

EasyTransferI2C ET;

struct WireDatas {
    int actionIdentifier;
} wireDatas;

void launchActionOnSecondaryBoard(int actionIdentifier){
  wireDatas.actionIdentifier = actionIdentifier;
  ET.sendData(SECONDARY_BOARD_ADDRESS);
}

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
    //Serial.println(F("red and green"));
    rgb(0, 255, 0, 0);
    rgb(1, 0, 255, 0);
  }
  
  if((char)buffer[0] == 'b'){
    //Serial.println(F("purple and blue"));
    rgb(0, 255, 0, 250);
    rgb(1, 0, 0, 255);
  }

  if((char)buffer[0] == 'c'){
    sendMessage("hello", 5);
  }
}

/*==============================*/
/*==============================*/
/*==============================*/

TimeManager time;
boolean firstLoop;

void setup(void)
{ 
  Serial.begin(9600);
  while(!Serial);
  
  leds.begin();
  leds.show();
  
  //Communication entre les deux boards
  Wire.begin(MAIN_BOARD_ADDRESS);
  ET.begin(details(wireDatas), &Wire);
  /*----------------------------------*/

  bluetooth.setRXcallback(rxCallback);
  bluetooth.setDeviceName("LANTERN");
  bluetooth.begin();
  
  firstLoop = true;
  
  time.init();
}

void loop()
{
  time.loopStart();
                                       
  if(firstLoop){
    rgb(15, 15, 0);
    
    firstLoop = false;
  }

  bluetooth.pollACI();

  leds.show();

  time.loopEnd();
}
