/*==============================*/
/*======Liste des features======*/
/*==============================*/

//Lecture du son TO DO
//Communication avec la main Board OK

/*==============================*/
/*==============================*/
/*==============================*/

#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <Wire.h>
#include <EasyTransferI2C.h>

#include <TimeManager.h>

/*==================================================*/
/*=========Communication avec la main board=========*/
/*==================================================*/

#define MAIN_BOARD_ADDRESS 4
#define SECONDARY_BOARD_ADDRESS 9

EasyTransferI2C ET;

struct WireDatas {
    int actionIdentifier;
} wireDatas;

void receive(int numBytes) {}

/*================================*/
/*=========Gestion du son=========*/
/*================================*/

#define SPEAKER_PIN 9
#define SD_CARD_PIN 10
TMRpcm audio;

TimeManager time;
boolean firstLoop;

void setup(void)
{ 
  Serial.begin(9600);
  while(!Serial);
  
  //Communication avec la main board
  Wire.begin(SECONDARY_BOARD_ADDRESS);
  ET.begin(details(wireDatas), &Wire);
  Wire.onReceive(receive);
  /*------------------------------*/
  
  //Gestion de l'audio
  
  pinMode(SPEAKER_PIN, OUTPUT);

  audio.speakerPin = SPEAKER_PIN;
  if(!SD.begin(SD_CARD_PIN)){
    return;
  }
  audio.volume(1);
  audio.setVolume(7);
  
  tone(SPEAKER_PIN, 440, 1500);
  
  delay(2000);
  
  audio.play("klax.wav");

  firstLoop = true;

  time.init();
}

void loop()
{
  delay(2000);
  
  audio.play("klax.wav");
  
  time.loopStart();
  
  if(firstLoop){
  
    firstLoop = false;
  }
  
  if(ET.receiveData()){
    switch(wireDatas.actionIdentifier){
      case 1:
      break;
      
      case 2:
      break;
      
      default:
      break;
    }
  }

  time.loopEnd();
}
