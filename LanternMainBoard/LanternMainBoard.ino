/*==============================*/
/*======Liste des features======*/
/*==============================*/

//bluetooth in/out OK
//Controle des NeoPixels IN PROGRESS
//Communication avec la secondary Board OK
//Open/close detection OK
//Detecteur infrarouge TO DO
//Controle du volume IN PROGRESS
//Led d'état OK
//Led state on/off IN PROGRESS

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

TimeManager time;

//Definition des storyCode
#define STORY_NO_STORY 'n'
#define STORY_DRAGON_ENOSHIMA 'd'
#define STORY_BOUCLE_OR 'o'

#define DURATION_1_MIN 60000
#define DURATION_2_MIN 120000
#define DURATION_3_MIN 180000
#define DURATION_4_MIN 240000
#define DURATION_5_MIN 300000
#define DURATION_6_MIN 360000
#define DURATION_7_MIN 420000
#define DURATION_8_MIN 480000
#define DURATION_9_MIN 540000
#define DURATION_10_MIN 600000
#define DURATION_11_MIN 660000
#define DURATION_12_MIN 720000
#define DURATION_13_MIN 780000
#define DURATION_14_MIN 840000
#define DURATION_15_MIN 900000

char currentStoryCode, previousLoopStoryCode, previousStoryCode, currentPage, previousPage, previousLoopPage;
unsigned long transitionToNightDuration;
boolean transitionToNightStarted;

void setPage(char newPage){
  previousPage = currentPage;
  currentPage = newPage;
}

void setStory(char newStoryCode){
  previousStoryCode = currentStoryCode;
  currentStoryCode = newStoryCode;
}

//Definition des modes de la veilleuse
#define LANTERN_MODE_INIT 0
#define LANTERN_MODE_PREVIEW 1
#define LANTERN_MODE_STORY 2
#define LANTERN_MODE_TRANSITION 3
#define LANTERN_MODE_NIGHT 4
#define LANTERN_MODE_TUTO_BEGIN 5
#define LANTERN_MODE_TUTO_END 6
#define LANTERN_MODE_LEDS_TEST 7

byte currentLanternMode, previousLanternMode, previousLoopLanternMode;
void setLanternMode(byte newLanternMode){
  previousLanternMode = currentLanternMode;
  currentLanternMode = newLanternMode;
}

/*==================================================*/
/*=========Communication avec la secondary board=========*/
/*==================================================*/

#define MAIN_BOARD_ADDRESS 4
#define SECONDARY_BOARD_ADDRESS 9

#define WIRE_ACTION_START_LANTERN 0
#define WIRE_ACTION_PLAY_SOUND 1
#define WIRE_ACTION_CHANGE_VOLUME 2

EasyTransferI2C ET;

struct WireDatas {
    byte actionIdentifier;
    byte audioVolume;
    byte soundFileIdentifier;
} wireDatas;

void launchActionOnSecondaryBoard(byte actionIdentifier){
  wireDatas.actionIdentifier = actionIdentifier;
  ET.sendData(SECONDARY_BOARD_ADDRESS);
}

/*================================*/
/*======Gestion des couleurs======*/
/*================================*/

#define COLOR_TRANSITION_ORANGE Colors(255, 108, 0)
#define COLOR_NIGHT_ORANGE Colors(152, 38, 0)
#define COLOR_BLACK Colors(0, 0, 0)
#define COLOR_PREVIEW_DRAGON_ENOSHIMA Colors(0, 50, 155)
#define COLOR_PREVIEW_BOUCLE_OR Colors(245, 195, 10)
#define COLOR_WHITE Colors(255, 255, 255)
#define COLOR_LIGHT_WHITE Colors(220, 220, 220)

#define COLOR_DRAGON_RED Colors(235,0,0)
#define COLOR_PAGE_ONE_BOTTOM Colors(0,0,20)
#define COLOR_PAGE_ONE_TOP Colors(0,45,245)
#define COLOR_PAGE_TWO_BOTTOM Colors(0,0,20)
#define COLOR_PAGE_TWO_TOP Colors(0,85,185)
#define COLOR_PAGE_THREE_BOTTOM Colors(0,25,255)
#define COLOR_PAGE_THREE_TOP Colors(175,175,175)
#define COLOR_PAGE_FOUR_BOTTOM Colors(0,165,38)
#define COLOR_PAGE_FOUR_TOP Colors(0,235,45)

/*===============================*/
/*=====Gestion des neopixels=====*/
/*===============================*/

#define LEDS_PIN 9
#define NUMBER_OF_LEDS 29
#define NUMBER_OF_LEDS_PER_ROW 7
#define NUMBER_OF_LEDS_PER_COLUMN 4
#define NUMBER_OF_LIGHT_ROWS 5
#define NUMBER_OF_LIGHT_COLUMNS 7

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

void rgbGroup(byte red, byte green, byte blue, byte *group, byte quantity){
  byte i;
  
  for(i=0;i<quantity;i++){
    rgb(group[i], red, green, blue);
  }
}

Colors currentColorOfLedAtIndex(byte index){
  uint32_t rawColor = leds.getPixelColor(index);

  uint8_t
      green = (uint8_t)(rawColor >> 16),
      red = (uint8_t)(rawColor >>  8),
      blue = (uint8_t)rawColor;

  return Colors((byte)red, (byte)green, (byte)blue);
}

byte getRowIndexOfLed(byte ledIndex){
  if(ledIndex == 0){
    return 0;
  }

  if(ledIndex%NUMBER_OF_LEDS_PER_ROW == 0){
    return (byte)(ledIndex/NUMBER_OF_LEDS_PER_ROW);
  }

  return (byte)(ledIndex/NUMBER_OF_LEDS_PER_ROW)+1;
}

byte getColumnsIndexOf(byte ledIndex){
  if(ledIndex == 0){
    return 3;
  }

  byte row = getRowIndexOfLed(ledIndex);

  return (byte)((int)ledIndex - (int)row*NUMBER_OF_LEDS_PER_ROW + NUMBER_OF_LEDS_PER_ROW - 1);
}

#define LIGHT_ANIMATION_AMBIANT_NONE 0
#define LIGHT_ANIMATION_AMBIANT_SEA 1

#define LIGHT_ANIMATION_SEQUENCE_NONE 0
#define LIGHT_ANIMATION_SEQUENCE_DRAGON 1

boolean ambiantIsTransitionning;
byte currentAmbiant, currentSequence;
Tween ambiantTransitionTween, lightBreathTween;

void launchAmbiantTransition(unsigned long transitionDuration){
  ambiantIsTransitionning = true;
  ambiantTransitionTween.transition(0, 100, transitionDuration);  
}

void updateAmbiantTransition(){
  if(ambiantTransitionTween.isEnded()){
      ambiantIsTransitionning = false;
  }

  ambiantTransitionTween.update(time.delta());
}

Colors ledsOriginColor[NUMBER_OF_LEDS];
Colors ledsTargetColor[NUMBER_OF_LEDS];

void updateLightBreath(){
  if(!ambiantIsTransitionning){
    lightBreathTween.update(time.delta());
  }
}

float getLightBreathAlpha(){
  return (float)lightBreathTween.easeInOutQuartValue() / 255.0;
}

void breath(byte min, byte max, unsigned long duration, unsigned long delay){
  if(min > max){
      byte temp = max;
      max = min;
      min = temp;
  }

  lightBreathTween.transition(min, max, duration, delay);
  lightBreathTween.loopWithDelay();
  lightBreathTween.reverseLoop();
}

void justRoundBreath(){
  breath(0, 148, 3900, 150);
}

void simpleAmbiantBreath(){
  breath(0, 182, 6200, 248);
}

void tutoBreath(){
  breath(0, 220, 1500, 150);
}

void noBreath(){
  breath(0, 0, 0, 0);
}

void ambiantTransition(Colors *newLedsTargetColors, unsigned long transitionDuration){
  byte i;
  
  for(i=0;i<NUMBER_OF_LEDS;i++){
    ledsOriginColor[i] = currentColorOfLedAtIndex(i);
    ledsTargetColor[i] = newLedsTargetColors[i];    
  }

  launchAmbiantTransition(transitionDuration);
}

void ambiantTransition(Colors *newLedsTargetColors){
  ambiantTransition(newLedsTargetColors, 3600);
}

void applyAmbiantColor(boolean breathing, boolean justTheMiddle){
  byte i;

  if(ambiantIsTransitionning){
    for(i=0;i<NUMBER_OF_LEDS;i++){
      Colors ledColor = ledsOriginColor[i].getGradientStep(ambiantTransitionTween.easeInOutQuadCursor(), ledsTargetColor[i]);
      rgb(i, ledColor.red(), ledColor.green(), ledColor.blue());
    }
  }
  else if(breathing){
    float breathingAlpha = getLightBreathAlpha();
    boolean onMiddle = false;
    for(i=0;i<NUMBER_OF_LEDS;i++){
      Colors temp_ledColor = ledsTargetColor[i];

      onMiddle = (i == 0 || i == 4 || i == 11 || i == 18 || i == 25);
      Colors finalColor = (!justTheMiddle || onMiddle) ? temp_ledColor.getColorWithAlpha(breathingAlpha) : temp_ledColor;

      rgb(i, finalColor.red(), finalColor.green(), finalColor.blue());
    }
  }
}

void applyAmbiantColor(boolean breathing){
  applyAmbiantColor(breathing, false);
}

/*==============================*/
/*=====Gestion du bluetooth=====*/
/*==============================*/
#define TIME_WITHOUT_CONNECTION_BEFORE_RETURN_TO_NIGHT_MODE 60000
unsigned long timeWithoutConnection;

#define REQ_PIN 10
#define RDY_PIN 2
#define RST_PIN 7

Adafruit_BLE_UART bluetooth = Adafruit_BLE_UART(REQ_PIN, RDY_PIN, RST_PIN);

#define BLE_INSTRUCTION_SET_MODE 'm'
#define BLE_INSTRUCTION_SET_STORY 's'
#define BLE_INSTRUCTION_SET_STORY_PAGE 'p'
#define BLE_INSTRUCTION_SET_TRANSITION_TIME 't'

#define BLE_PARAM_MODE_PREVIEW 'p'
#define BLE_PARAM_MODE_STORY 's'
#define BLE_PARAM_MODE_TRANSITION 't'
#define BLE_PARAM_MODE_NIGHT 'n'
#define BLE_PARAM_MODE_TUTO_BEGIN 'b'
#define BLE_PARAM_MODE_TUTO_END 'e'
#define BLE_PARAM_MODE_LEDS_TEST 'd'

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
  char bleInstruction = (char)buffer[0];
  char bleParamOne = '0';

  if(len > 1){
    bleParamOne = (char)buffer[1];
  }

  if(bleInstruction == BLE_INSTRUCTION_SET_MODE){
      if(bleParamOne == BLE_PARAM_MODE_PREVIEW){
          setLanternMode(LANTERN_MODE_PREVIEW);
      }
      else if(bleParamOne == BLE_PARAM_MODE_STORY){
          setLanternMode(LANTERN_MODE_STORY);
      }
      else if(bleParamOne == BLE_PARAM_MODE_TRANSITION){
          setLanternMode(LANTERN_MODE_TRANSITION);
      }
      else if(bleParamOne == BLE_PARAM_MODE_NIGHT){
          setLanternMode(LANTERN_MODE_NIGHT);
      }
      else if(bleParamOne == BLE_PARAM_MODE_TUTO_BEGIN){
          setLanternMode(LANTERN_MODE_TUTO_BEGIN);
      }
      else if(bleParamOne == BLE_PARAM_MODE_TUTO_END){
          setLanternMode(LANTERN_MODE_TUTO_END);
      }
      else if(bleParamOne == BLE_PARAM_MODE_LEDS_TEST){
          setLanternMode(LANTERN_MODE_LEDS_TEST);
      }

      manageLanternMode((currentLanternMode != previousLanternMode), false, false);
  }
  else if(bleInstruction == BLE_INSTRUCTION_SET_STORY){
    setStory(bleParamOne);

    manageLanternMode(false, (currentStoryCode != previousStoryCode), false);
  }
  else if(bleInstruction == BLE_INSTRUCTION_SET_STORY_PAGE){
    setPage(bleParamOne);

    manageLanternMode(false, false, (currentPage != previousPage));
  }
  else if(bleInstruction == BLE_INSTRUCTION_SET_TRANSITION_TIME){
    switch (bleParamOne) {
        case '1':transitionToNightDuration = DURATION_1_MIN;break;
        case '2':transitionToNightDuration = DURATION_2_MIN;break;
        case '3':transitionToNightDuration = DURATION_3_MIN;break;
        case '4':transitionToNightDuration = DURATION_4_MIN;break;
        case '5':transitionToNightDuration = DURATION_5_MIN;break;
        case '6':transitionToNightDuration = DURATION_6_MIN;break;
        case '7':transitionToNightDuration = DURATION_7_MIN;break;
        case '8':transitionToNightDuration = DURATION_8_MIN;break;
        case '9':transitionToNightDuration = DURATION_9_MIN;break;
        case 'a':transitionToNightDuration = DURATION_10_MIN;break;
        case 'b':transitionToNightDuration = DURATION_11_MIN;break;
        case 'c':transitionToNightDuration = DURATION_12_MIN;break;
        case 'd':transitionToNightDuration = DURATION_13_MIN;break;
        case 'e':transitionToNightDuration = DURATION_14_MIN;break;
        default:transitionToNightDuration = DURATION_15_MIN;break;
    }
  }
}

boolean bluetoothIsConnected(){
  return bluetooth.getState() == ACI_EVT_CONNECTED;
}

/*==============================*/
/*==============================*/
/*==============================*/

#define CLOSED_PIN 4

#define CHECKING_IF_LANTERN_IS_OPENED_TIME_INTERVAL 1000

unsigned long timeSinceLastLanternOpenedChecking;

boolean lanternIsClosed(){
  return digitalRead(CLOSED_PIN);
}

boolean lanternIsOpen(){
  return !lanternIsClosed();
}

/*==============================*/
/*======Gestion du volume=======*/
/*==============================*/
#include <Rotary.h>

#define ROTARY_LEFT_PIN A1
#define ROTARY_RIGHT_PIN A0
#define MIN_VOLUME 0
#define MAX_VOLUME 7

Rotary volumeRotaryEncoder = Rotary(ROTARY_LEFT_PIN, ROTARY_RIGHT_PIN);

void updateVolume(){
    unsigned char volumeRotaryEncoderDirection = volumeRotaryEncoder.process();
    
    if(volumeRotaryEncoderDirection){
      volumeRotaryEncoderDirection == DIR_CW ? wireDatas.audioVolume++ : wireDatas.audioVolume--;
      
      if(wireDatas.audioVolume < MIN_VOLUME){
        wireDatas.audioVolume = MIN_VOLUME;
      }
      
      if(wireDatas.audioVolume > MAX_VOLUME){
        wireDatas.audioVolume = MAX_VOLUME;
      }
      
      launchActionOnSecondaryBoard(WIRE_ACTION_CHANGE_VOLUME);     
    }
}

/*==============================*/
/*====Gestion des statuts=======*/
/*==============================*/

#define TIME_BEFORE_LANTERN_IS_READY 5000

#define RED_STATE_LED_PIN 3
#define GREEN_STATE_LED_PIN 5

#define STATE_LED_ON_OFF_PIN A2
boolean stateLedOn, previousSwitchStateLed;

//Le bouton du rotary encoder permet d'éteindre la led de statut
void updateSwitchLedState(){
  boolean switchStateLed = digitalRead(STATE_LED_ON_OFF_PIN);
  if(switchStateLed != previousSwitchStateLed && !switchStateLed){
    stateLedOn = !stateLedOn;
  }
  previousSwitchStateLed = switchStateLed;
}

Tween stateLedBreath;
byte stateLedIntensity;
void updateStateLedIntensity(){
  stateLedBreath.update(time.delta());
  stateLedIntensity = (byte)(stateLedBreath.easeInQuintValue());
}

void redLed(){
  analogWrite(RED_STATE_LED_PIN, stateLedOn ? stateLedIntensity : 0);
  analogWrite(GREEN_STATE_LED_PIN, 0);
}

void greenLed(){
  analogWrite(RED_STATE_LED_PIN, 0);
  analogWrite(GREEN_STATE_LED_PIN, stateLedOn ? stateLedIntensity : 0);
}

void yellowLed(){
  analogWrite(RED_STATE_LED_PIN, stateLedOn ? stateLedIntensity/1.5 : 0);
  analogWrite(GREEN_STATE_LED_PIN, stateLedOn ? stateLedIntensity/1.5 : 0);
}

void orangeLed(){
  analogWrite(RED_STATE_LED_PIN, stateLedOn ? stateLedIntensity/2 : 0);
  analogWrite(GREEN_STATE_LED_PIN, stateLedOn ? stateLedIntensity/8 : 0);
}

boolean firstLoop, lanternReady;

/*==============================*/
/*==============================*/
/*==============================*/

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
  
  pinMode(CLOSED_PIN, INPUT);
  pinMode(STATE_LED_ON_OFF_PIN, INPUT);
  
  firstLoop = true;
  lanternReady = false;
  stateLedOn = true;
  previousSwitchStateLed = LOW;
  
  previousLanternMode = LANTERN_MODE_INIT;
  currentLanternMode = LANTERN_MODE_INIT;
  previousLoopLanternMode = LANTERN_MODE_INIT;
  
  wireDatas.audioVolume = 0;
  stateLedIntensity = 0;
  
  stateLedBreath.transition(20, 200, 3500, 1000);
  stateLedBreath.loopWithDelay();
  stateLedBreath.reverseLoop();
  
  ambiantIsTransitionning = false;

  setStory(STORY_NO_STORY);

  timeSinceLastLanternOpenedChecking = 0;
  timeWithoutConnection = 0;

  currentPage = '0';
  previousPage = '0';

  transitionToNightDuration = DURATION_15_MIN;
  transitionToNightStarted = false;

  time.init();
}

void loop()
{
  boolean modeChanged, storyChanged, pageChanged;
    
  if(!bluetoothIsConnected() && (currentLanternMode == LANTERN_MODE_TUTO_BEGIN || currentLanternMode == LANTERN_MODE_TUTO_END)){
      setLanternMode(LANTERN_MODE_NIGHT);
  }

  modeChanged = false;
  storyChanged = currentStoryCode != previousLoopStoryCode;
  pageChanged = currentPage != previousLoopPage;

  time.loopStart();
  
  updateStateLedIntensity();
  updateSwitchLedState();

  updateAmbiantTransition();
  updateLightBreath();
  
  //On attend un peu pour etre sur que la secondary board est prete à recevoir des données
  if(!lanternReady){
    redLed();
    
    if(time.total() > TIME_BEFORE_LANTERN_IS_READY){
      lanternReady = true;
    }
  }
  
  if(firstLoop){
    if(lanternReady){
      bluetooth.begin();
      
      wireDatas.audioVolume = 7;
      launchActionOnSecondaryBoard(WIRE_ACTION_START_LANTERN);
      setLanternMode(LANTERN_MODE_NIGHT);

      firstLoop = false;
    }
  }
  
  if(lanternReady){
    modeChanged = currentLanternMode != previousLoopLanternMode;
    
    manageLanternMode(modeChanged, storyChanged, pageChanged);
    
    updateVolume();
  
    bluetooth.pollACI();
  
    leds.show();
  }

  if(!bluetoothIsConnected() && currentLanternMode != LANTERN_MODE_TRANSITION && currentLanternMode != LANTERN_MODE_NIGHT){
    timeWithoutConnection += time.delta();
  }
  else{
    timeWithoutConnection = 0;
  }
  if(timeWithoutConnection >= TIME_WITHOUT_CONNECTION_BEFORE_RETURN_TO_NIGHT_MODE){
      setLanternMode(LANTERN_MODE_NIGHT);
      setStory(STORY_NO_STORY);
  }
  
  time.loopEnd();
}

void manageLanternMode(boolean modeChanged, boolean storyChanged, boolean pageChanged){
  boolean checkIfLanternIsOpened;
  byte i, j;
  Colors newLedsTargetColors[NUMBER_OF_LEDS];
  Colors gradientBottomColor, gradientTopColor;

  if(currentLanternMode == LANTERN_MODE_LEDS_TEST){
    for(i=0;i<NUMBER_OF_LEDS;i++){
        rgb(0,0,0);
        rgb(i,255,255,255);
        leds.show();

        delay(800);
    }

    rgb(255,255,255);
    leds.show();
    delay(800);

    setLanternMode(LANTERN_MODE_NIGHT);
  }

  checkIfLanternIsOpened = false;

  timeSinceLastLanternOpenedChecking += time.delta();
  if(timeSinceLastLanternOpenedChecking >= CHECKING_IF_LANTERN_IS_OPENED_TIME_INTERVAL){
      checkIfLanternIsOpened = true;
      timeSinceLastLanternOpenedChecking = 0;
  }

  if(currentLanternMode != LANTERN_MODE_TRANSITION && currentLanternMode != LANTERN_MODE_NIGHT){
      transitionToNightStarted = false;
  }

  //On definit les events pour chaque mode
  switch(currentLanternMode){
    case LANTERN_MODE_INIT:
      bluetoothIsConnected() ? greenLed() : yellowLed();
      rgb(0,0,0);
    break;
    
    case LANTERN_MODE_PREVIEW:
      bluetoothIsConnected() ? greenLed() : yellowLed();

      if(modeChanged || storyChanged){
        boolean validStory = true;

        switch (currentStoryCode) {
            case STORY_NO_STORY:
              newLedsTargetColors[0] = COLOR_NIGHT_ORANGE;
              for(i=1;i<NUMBER_OF_LEDS;i++){
                newLedsTargetColors[i] = COLOR_BLACK;
              }
            break;

            case STORY_DRAGON_ENOSHIMA:
              newLedsTargetColors[0] = COLOR_PREVIEW_DRAGON_ENOSHIMA;
              for(i=1;i<NUMBER_OF_LEDS;i++){
                newLedsTargetColors[i] = COLOR_BLACK;
              }
            break;

            case STORY_BOUCLE_OR:
              newLedsTargetColors[0] = COLOR_PREVIEW_BOUCLE_OR;
              for(i=1;i<NUMBER_OF_LEDS;i++){
                newLedsTargetColors[i] = COLOR_BLACK;
              }
            break;

            default:
              validStory = false;
            break;
        }

        if(validStory){
          ambiantTransition(newLedsTargetColors);
          justRoundBreath();
        }
      }
      else{
        applyAmbiantColor(/*breathing*/true);
      }
      //PREVIEW MODE END
    break;
    
    case LANTERN_MODE_STORY:
      bluetoothIsConnected() ? greenLed() : yellowLed();

      gradientBottomColor = COLOR_BLACK;
      gradientTopColor = COLOR_BLACK;

      if(modeChanged || storyChanged || pageChanged){
        int topRowIndex = NUMBER_OF_LIGHT_ROWS - 1;
        float row = 0;
        float gradientCursor = 0.0;
        byte specialAmbiantPage = 0;

        if(currentStoryCode == STORY_DRAGON_ENOSHIMA){
            switch (currentPage) {
                case '1':
                  specialAmbiantPage = 1;
                  gradientBottomColor = COLOR_PAGE_ONE_BOTTOM;
                  gradientTopColor = COLOR_PAGE_ONE_TOP;
                break;

                case '2':
                  specialAmbiantPage = 2;

                  gradientBottomColor = COLOR_PAGE_TWO_BOTTOM;
                  gradientTopColor = COLOR_PAGE_TWO_TOP;
                break;

                case '3':
                  specialAmbiantPage = 3;

                  gradientBottomColor = COLOR_PAGE_THREE_BOTTOM;
                  gradientTopColor = COLOR_PAGE_THREE_TOP;
                break;

                case '4':
                  specialAmbiantPage = 4;

                  gradientBottomColor = COLOR_PAGE_FOUR_BOTTOM;
                  gradientTopColor = COLOR_PAGE_FOUR_TOP;
                break;

                default:
                break;
            }
        }

        float intervalPerRow = 1.0/(float)topRowIndex;

        for(i=0;i<NUMBER_OF_LEDS;i++){
          row = (float)getRowIndexOfLed(i);
          gradientCursor = intervalPerRow*row;

          newLedsTargetColors[i] = gradientBottomColor.getGradientStep(gradientCursor, gradientTopColor);
        }

        if(specialAmbiantPage == 2){
            for(i=22;i<=28;i++){
              newLedsTargetColors[i] = COLOR_DRAGON_RED;
            }
        }
        else if(specialAmbiantPage == 4){
          newLedsTargetColors[0] = COLOR_WHITE;
        }

        ambiantTransition(newLedsTargetColors);
        simpleAmbiantBreath();
      }
      else{
        applyAmbiantColor(/*breathing*/true);
      }
    break;
    
    case LANTERN_MODE_TRANSITION:
      orangeLed();

      if(modeChanged){
        transitionToNightStarted = true;

        for(i=0;i<NUMBER_OF_LEDS;i++){
          newLedsTargetColors[i] = COLOR_TRANSITION_ORANGE;
        }

        ambiantTransition(newLedsTargetColors);
        noBreath();
      }
      else{
        applyAmbiantColor(/*breathing*/false);

        if(ambiantTransitionTween.isEnded()){
          if(transitionToNightStarted){
              newLedsTargetColors[0] = COLOR_NIGHT_ORANGE;

              for(i=1;i<NUMBER_OF_LEDS;i++){
                newLedsTargetColors[i] = COLOR_BLACK;
              }
              ambiantTransition(newLedsTargetColors, transitionToNightDuration);

              transitionToNightStarted = false;
          }
          else{
            setLanternMode(LANTERN_MODE_NIGHT);
            modeChanged = true;
          }
        }
      }
    break;
    
    case LANTERN_MODE_NIGHT:
      orangeLed();

      if(modeChanged){
        newLedsTargetColors[0] = COLOR_NIGHT_ORANGE;

        for(i=1;i<NUMBER_OF_LEDS;i++){
          newLedsTargetColors[i] = COLOR_BLACK;
        }
  
        ambiantTransition(newLedsTargetColors);
        justRoundBreath();
      }
      else{
        applyAmbiantColor(/*breathing*/true);
      }

      //NIGHT MODE END
    break;

    case LANTERN_MODE_TUTO_BEGIN:
      bluetoothIsConnected() ? greenLed() : yellowLed();

      if(modeChanged){
        newLedsTargetColors[0] = COLOR_LIGHT_WHITE;

        for(i=1;i<NUMBER_OF_LEDS;i++){
          newLedsTargetColors[i] = COLOR_BLACK;
        }

        ambiantTransition(newLedsTargetColors);
        tutoBreath();
      }
      else{
        applyAmbiantColor(/*breathing*/true);
      }
    
      if(checkIfLanternIsOpened){
          if(lanternIsOpen()){
              sendMessage("opened", 6);
          }
      }
    break;

    case LANTERN_MODE_TUTO_END:
      bluetoothIsConnected() ? greenLed() : yellowLed();

      if(modeChanged){
        newLedsTargetColors[0] = COLOR_LIGHT_WHITE;

        for(i=1;i<NUMBER_OF_LEDS;i++){
          newLedsTargetColors[i] = ledsTargetColor[i];
        }

        ambiantTransition(newLedsTargetColors);
        tutoBreath();
      }
      else{
        applyAmbiantColor(/*breathing*/true, /*Just on Middle*/true);
      }
    
      if(checkIfLanternIsOpened){
          if(lanternIsClosed()){
              sendMessage("closed", 6);
          }
      }
    break;
    
    default:
    break;
  }

  previousLoopLanternMode = currentLanternMode;
  previousLoopStoryCode = currentStoryCode;
  previousLoopPage = currentPage;
}

