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
#include <Tween.h>

TimeManager time;

/*==================================================*/
/*=========Communication avec la main board=========*/
/*==================================================*/

#define MAIN_BOARD_ADDRESS 4
#define SECONDARY_BOARD_ADDRESS 9

EasyTransferI2C ET;

struct WireDatas {
    char actionIdentifier;
    char targetSound; 
} wireDatas;

void receive(int numBytes) {}

/*================================*/
/*=========Gestion du son=========*/
/*================================*/

#define SPEAKER_PIN 9
#define SD_CARD_PIN 10
#define MAX_VOLUME 6
#define MIN_VOLUME 0

#define SOUND_IDENTIFIER_NONE 'n'

TMRpcm audio;
Tween volumeTransitionTween;
byte currentVolume;
boolean transitionningInProgress;
char currentSoundIdentifier;
boolean soundOff;

void stopAudio(){
  audio.setVolume(0);
  audio.volume(0);
  audio.stopPlayback();

  soundOff = true;
}

void playAudio(char soundIdentifier){
  currentSoundIdentifier = soundIdentifier;

  switch(currentSoundIdentifier){
    case SOUND_IDENTIFIER_NONE:
      stopAudio();
    break;

    default:
      stopAudio();
    break;
  }
}

void playAudio(){
  playAudio(currentSoundIdentifier);
}

void changeVolume(byte newVolume){
  currentVolume = newVolume;
  currentVolume = currentVolume < MIN_VOLUME ? MIN_VOLUME : currentVolume;
  currentVolume = currentVolume > MAX_VOLUME ? MAX_VOLUME : currentVolume;
  
  if (currentVolume <= 0)
  {
    stopAudio();
  }
  else{
    soundOff = false;
    audio.setVolume(currentVolume-1);
    audio.volume(1);
  }
}

void volumeTransitionTo(byte newVolume){
  volumeTransitionTween.transition(currentVolume, newVolume, 800);
  transitionningInProgress = true;
}

void updateVolume(){
  if(transitionningInProgress){
    volumeTransitionTween.update(time.delta());
    
    changeVolume((byte)volumeTransitionTween.linearValue());
  }

  if (volumeTransitionTween.isEnded())
  {
    transitionningInProgress = false;
  }
}

/*---------------*/

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
    Serial.println("error SD");
    return;
  }

  changeVolume(MAX_VOLUME);
  audio.quality(1);

  firstLoop = true;
  transitionningInProgress = false;

  soundOff = false;

  time.init();
}

boolean testTransition;

void loop()
{
  time.loopStart();

  if(firstLoop){
    testTransition = false;
    firstLoop = false;
  }

  updateVolume();

  if (time.total() > 10000 && !testTransition)
  {
    volumeTransitionTo(MIN_VOLUME);
    testTransition = true;
  }

  if (!audio.isPlaying() && !soundOff)
  {
    audio.play("klax.wav");
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
