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

unsigned long minuteDuration(byte numberOfMinutes){
  return (unsigned long)numberOfMinutes * 60000;
}

/*==================================================*/
/*=========Communication avec la main board=========*/
/*==================================================*/

#define MAIN_BOARD_ADDRESS 4
#define SECONDARY_BOARD_ADDRESS 9

#define WIRE_ACTION_START_LANTERN 0
#define WIRE_ACTION_PLAY_SOUND 1
#define WIRE_ACTION_STOP_SOUND 2
#define WIRE_ACTION_CHANGE_VOLUME_UP 3
#define WIRE_ACTION_CHANGE_VOLUME_DOWN 4
#define WIRE_ACTION_LOOP_ON 5
#define WIRE_ACTION_LOOP_OFF 6
#define WIRE_ACTION_START_TRANSITION 7
#define WIRE_ACTION_REACTIVATE_TRANSITION 8

boolean audioLoop;

void setLoop(boolean loopValue){
  audioLoop = loopValue;
}

EasyTransferI2C ET;

struct WireDatas {
    byte actionIdentifier;
    char soundFileIdentifier;
    byte minuteTransitionDuration;
} wireDatas;

void receive(int numBytes) {}

/*================================*/
/*=========Gestion du son=========*/
/*================================*/

#define SPEAKER_PIN 9
#define SD_CARD_PIN 10
#define MAX_VOLUME 5
#define MIN_VOLUME 0

#define SOUND_IDENTIFIER_NONE 'n'

#define SOUND_IDENTIFIER_CONNECT 'c'
#define SOUND_IDENTIFIER_INTERACTION_REQUEST 'r'
#define SOUND_IDENTIFIER_INTERACTION_DONE 'o'

#define SOUND_IDENTIFIER_LOOP_FOREST 'f'
#define SOUND_IDENTIFIER_LOOP_WAVE 'w'
#define SOUND_IDENTIFIER_LOOP_MUSIC 'm'

#define SOUND_IDENTIFIER_EVENT_DRAGON 'd'
#define SOUND_IDENTIFIER_EVENT_SPARKLES 's'
#define SOUND_IDENTIFIER_EVENT_TREES 't'

TMRpcm audio;
Tween volumeTransitionTween;
byte currentVolume;
boolean transitionningInProgress;
char currentSoundIdentifier;

void stopAudio(){
  setLoop(false);
  audio.setVolume(0);
  audio.volume(0);
  audio.stopPlayback();
}

void changeVolume(byte newVolume, boolean force){
  if(currentVolume != newVolume || force){
    currentVolume = newVolume;
    currentVolume = currentVolume < MIN_VOLUME ? MIN_VOLUME : currentVolume;
    currentVolume = currentVolume > MAX_VOLUME ? MAX_VOLUME : currentVolume;
    
    if (currentVolume <= 0)
    {
      stopAudio();
    }
    else{
      audio.volume(1);
      audio.setVolume(currentVolume-1);
    }
  }
}

void changeVolume(byte newVolume){
  changeVolume(newVolume, false);
}

void playAudio(char soundIdentifier){
  boolean volumeZero = false;
  currentSoundIdentifier = soundIdentifier;

  audio.disable();

  switch(currentSoundIdentifier){
    case SOUND_IDENTIFIER_NONE:
      volumeZero = true;
      stopAudio();
    break;

    case SOUND_IDENTIFIER_CONNECT:
      audio.play("icon.wav");
    break;

    case SOUND_IDENTIFIER_INTERACTION_REQUEST:
      audio.play("ibeg.wav");
    break;

    case SOUND_IDENTIFIER_INTERACTION_DONE:
      audio.play("isto.wav");
    break;

    case SOUND_IDENTIFIER_LOOP_MUSIC:
      audio.play("lmus.wav");
    break;

    case SOUND_IDENTIFIER_LOOP_FOREST:
      audio.play("lfor.wav");
    break;

    case SOUND_IDENTIFIER_LOOP_WAVE:
      audio.play("lmer.wav");
    break;

    case SOUND_IDENTIFIER_EVENT_TREES:
      audio.play("efor.wav");
    break;

    case SOUND_IDENTIFIER_EVENT_DRAGON:
      audio.play("edra.wav");
    break;

    case SOUND_IDENTIFIER_EVENT_SPARKLES:
      audio.play("efee.wav");
    break;

    default:
      volumeZero = true;
      stopAudio();
    break;
  }

  if (!volumeZero){
    changeVolume(currentVolume, true);
  }
}

void playAudio(){
  playAudio(currentSoundIdentifier);
}

void volumeTransitionTo(byte newVolume, unsigned long transitionDuration){
  volumeTransitionTween.transition(currentVolume, newVolume, (unsigned int)transitionDuration);
  transitionningInProgress = true; 
}

byte transitionNumberOfMinutesPast;

void updateVolume(){
  if(transitionningInProgress){
    volumeTransitionTween.update(time.delta());

    changeVolume(
      (byte)(
        volumeTransitionTween.startValue() +
        (
          (volumeTransitionTween.endValue()-volumeTransitionTween.startValue())*
          ((float)((float)transitionNumberOfMinutesPast+volumeTransitionTween.linearCursor())/(float)wireDatas.minuteTransitionDuration)
        )
      )
    );
  }

  if (volumeTransitionTween.isEnded())
  {
    transitionNumberOfMinutesPast++;

    if(transitionNumberOfMinutesPast >= wireDatas.minuteTransitionDuration){
      transitionningInProgress = false;
      transitionNumberOfMinutesPast = 0;
    }
    else{
      volumeTransitionTween.replay();
    }
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
  
  currentVolume = MIN_VOLUME;
  changeVolume(MAX_VOLUME);
  audio.quality(1);

  firstLoop = true;
  transitionningInProgress = false;

  setLoop(false);

  transitionNumberOfMinutesPast = 0;

  time.init();
}

void loop()
{
  time.loopStart();

  if(firstLoop){
    firstLoop = false;
  }

  updateVolume();

  if(!audio.isPlaying() && audioLoop){
    playAudio();
  }

  if(ET.receiveData()){
    switch(wireDatas.actionIdentifier){
      case WIRE_ACTION_START_LANTERN:
        changeVolume(MAX_VOLUME);
        playAudio(SOUND_IDENTIFIER_CONNECT);
      break;

      case WIRE_ACTION_PLAY_SOUND:
        changeVolume(MAX_VOLUME);
        playAudio(wireDatas.soundFileIdentifier);
      break;

      case WIRE_ACTION_STOP_SOUND:
        stopAudio();
        //volumeTransitionTo(MIN_VOLUME);
      break;

      case WIRE_ACTION_CHANGE_VOLUME_UP:
        changeVolume(currentVolume+1);
      break;
      
      case WIRE_ACTION_CHANGE_VOLUME_DOWN:
        changeVolume(currentVolume-1);
      break;

      case WIRE_ACTION_LOOP_ON:
        setLoop(true);
        //audio.loop(1);
      break;

      case WIRE_ACTION_LOOP_OFF:
        setLoop(false);
        //audio.loop(0);
      break;

      case WIRE_ACTION_START_TRANSITION:
        changeVolume(MAX_VOLUME);
        transitionNumberOfMinutesPast = 0;
        volumeTransitionTo(MIN_VOLUME, minuteDuration(1));
      break;

      case WIRE_ACTION_REACTIVATE_TRANSITION:
        changeVolume(MAX_VOLUME);
        setLoop(true);
        playAudio();
        transitionNumberOfMinutesPast = 0;
        volumeTransitionTo(MIN_VOLUME, minuteDuration(1));
      break;
      
      default:
      break;
    }
  }

  time.loopEnd();
}
