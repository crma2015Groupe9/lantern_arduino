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

#define MIN_DURATION_BETWEEN_TWO_PLAY_LOOP 2000
unsigned int timeSinceLastPlayLoop;
boolean soundLoopWaitingToBePlayed;
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
//#define WIRE_ACTION_REACTIVATE_TRANSITION 8

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
byte currentVolume, currentVolumePotarModifier;
boolean transitionningInProgress;
char currentSoundIdentifier;

void stopAudio(){
  setLoop(false);
  audio.stopPlayback();
  audio.disable();
}


void changeVolume(byte newVolume, byte newVolumePotarModifier, boolean force){
  byte finalVolume = 0, currentVolumeToUse, currentVolumePotarModifierToUse;
  boolean newVolumeValid = (newVolume >= MIN_VOLUME && newVolume <= MAX_VOLUME);
  boolean newVolumePotarModifierValid = (newVolumePotarModifier >= MIN_VOLUME && newVolumePotarModifier <= MAX_VOLUME);

  if(force || ((newVolumeValid && newVolume != currentVolume) || (newVolumePotarModifier && newVolumePotarModifier != currentVolumePotarModifier))){
    currentVolumePotarModifier = newVolumePotarModifier;
    currentVolume = newVolume;

    currentVolumeToUse = currentVolume;
    currentVolumePotarModifierToUse = currentVolumePotarModifier;

    if(currentVolumePotarModifierToUse > MAX_VOLUME){currentVolumePotarModifierToUse = MAX_VOLUME;}
    if(currentVolumePotarModifierToUse < MIN_VOLUME){currentVolumePotarModifierToUse = MIN_VOLUME;}
    if(currentVolumeToUse > MAX_VOLUME){currentVolumeToUse = MAX_VOLUME;}
    if(currentVolumeToUse < MIN_VOLUME){currentVolumeToUse = MIN_VOLUME;}

    finalVolume = currentVolumePotarModifierToUse >= currentVolumeToUse ? MIN_VOLUME : currentVolumeToUse - currentVolumePotarModifierToUse;
    if(finalVolume <= MIN_VOLUME){
      audio.setVolume(MIN_VOLUME);
      //audio.volume(0);
    }
    else{
      //audio.volume(1);
      audio.setVolume(finalVolume > MAX_VOLUME ? MAX_VOLUME : finalVolume);
    }
  }
}

void changeVolume(byte newVolume, byte newVolumePotarModifier){
  changeVolume(newVolume, newVolumePotarModifier, /*force*/false);
}

void changeVolumeFromPotar(byte newVolumePotarModifier){
  changeVolume(currentVolume, newVolumePotarModifier);
}

void changeVolumeFromTransition(byte newVolume){
  changeVolume(newVolume, currentVolumePotarModifier, true);
  if(currentVolume <= MIN_VOLUME){
      stopAudio();
  }
}

void playAudio(char soundIdentifier){
  currentSoundIdentifier = soundIdentifier;

  audio.stopPlayback();
  audio.disable();

  if(timeSinceLastPlayLoop >= MIN_DURATION_BETWEEN_TWO_PLAY_LOOP){
      switch(currentSoundIdentifier){
        case SOUND_IDENTIFIER_NONE:
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
          stopAudio();
        break;
      }

      if(audioLoop){
          timeSinceLastPlayLoop = 0;
      }
  }
  else{
    soundLoopWaitingToBePlayed = true;
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

    changeVolumeFromTransition(
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
    //Serial.println("error SD");
    return;
  }
  
  audio.volume(1);
  currentVolume = MIN_VOLUME;
  currentVolumePotarModifier = 0;
  changeVolume(MAX_VOLUME, 0);
  audio.quality(1);

  firstLoop = true;
  transitionningInProgress = false;

  setLoop(false);

  transitionNumberOfMinutesPast = 0;

  timeSinceLastPlayLoop = MIN_DURATION_BETWEEN_TWO_PLAY_LOOP;
  soundLoopWaitingToBePlayed = false;

  time.init();
}

void loop()
{
  time.loopStart();

  if(firstLoop){
    firstLoop = false;
  }

  updateVolume();

  timeSinceLastPlayLoop += time.delta();
  if(timeSinceLastPlayLoop >= MIN_DURATION_BETWEEN_TWO_PLAY_LOOP){
    timeSinceLastPlayLoop = MIN_DURATION_BETWEEN_TWO_PLAY_LOOP;

    if(soundLoopWaitingToBePlayed){
        soundLoopWaitingToBePlayed = false;
        playAudio(wireDatas.soundFileIdentifier);
    }
  }

  if(!audio.isPlaying() && audioLoop){
    playAudio();
  }

  if(ET.receiveData()){
    switch(wireDatas.actionIdentifier){
      case WIRE_ACTION_START_LANTERN:
        setLoop(false);
        changeVolume(MAX_VOLUME, 0);
        playAudio(SOUND_IDENTIFIER_CONNECT);
      break;

      case WIRE_ACTION_PLAY_SOUND:
        playAudio(wireDatas.soundFileIdentifier);
      break;

      case WIRE_ACTION_STOP_SOUND:
        stopAudio();
        //volumeTransitionTo(MIN_VOLUME);
      break;

      case WIRE_ACTION_CHANGE_VOLUME_UP:
        changeVolumeFromPotar(currentVolumePotarModifier <= MIN_VOLUME ? currentVolumePotarModifier : currentVolumePotarModifier-1);
      break;
      
      case WIRE_ACTION_CHANGE_VOLUME_DOWN:
        changeVolumeFromPotar(currentVolumePotarModifier >= MAX_VOLUME ? currentVolumePotarModifier : currentVolumePotarModifier+1);
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
        changeVolume(MAX_VOLUME*2, 0, true);
        setLoop(true);
        if(!audio.isPlaying()){
          playAudio();
        }
        transitionNumberOfMinutesPast = 0;
        volumeTransitionTo(MIN_VOLUME, minuteDuration(1));
      break;

      /*case WIRE_ACTION_REACTIVATE_TRANSITION:
        changeVolume(MAX_VOLUME, 0);
        setLoop(true);
        playAudio();
        transitionNumberOfMinutesPast = 0;
        volumeTransitionTo(MIN_VOLUME, minuteDuration(1));
      break;*/
      
      default:
      break;
    }
  }

  time.loopEnd();
}
