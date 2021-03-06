#include <SPI.h>
#include <Adafruit_BLE_UART.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <EasyTransferI2C.h>

#include <Tween.h>
#include <TimeManager.h>
#include <Colors.h>

#include <ByteBitList.h>

ByteBitList bitList;
#define BIT_LIST_INDEX_TRANSITION_TO_NIGHT_STARTED 0
#define BIT_LIST_INDEX_AMBIANT_IS_TRANSITIONNING 1
#define BIT_LIST_INDEX_BLUETOOTH_PREVIOUS_LOOP_CONNECTED 2
#define BIT_LIST_INDEX_FIRST_LOOP 3
#define BIT_LIST_INDEX_LANTERN_READY 4
#define BIT_LIST_INDEX_TRANSITION_ENDED 5

TimeManager time;

#define TIME_BEFORE_LANTERN_IS_READY 5000

byte transitionNumberOfMinutesPast;

unsigned long minuteDuration(byte numberOfMinutes){
  return (unsigned long)numberOfMinutes * 60000;
}

//Definition des storyCode
#define STORY_NO_STORY 'n'
#define STORY_DRAGON_ENOSHIMA 'd'
#define STORY_RENARD_POULE 'r'
#define STORY_SYLVIE_HIBOU 's'
#define STORY_BOUCLE_OR 'o'

char currentStoryCode, previousLoopStoryCode, previousStoryCode, currentPage, previousPage, previousLoopPage;
//unsigned long transitionToNightDuration;
boolean transitionEnded(){
  bitList.getValue(BIT_LIST_INDEX_TRANSITION_ENDED);
}

void transitionEnded(boolean value){
  bitList.setValue(BIT_LIST_INDEX_TRANSITION_ENDED, value);
}

boolean transitionToNightStarted(){
  bitList.getValue(BIT_LIST_INDEX_TRANSITION_TO_NIGHT_STARTED);
}

void transitionToNightStarted(boolean value){
  bitList.setValue(BIT_LIST_INDEX_TRANSITION_TO_NIGHT_STARTED, value);
}

void setPage(char newPage){
  previousPage = currentPage;
  currentPage = newPage;
}

void setStory(char newStoryCode){
  previousStoryCode = currentStoryCode;
  currentStoryCode = newStoryCode;
}

//Definition des modes de la veilleuse
#define LANTERN_MODE_INIT 'i'
#define LANTERN_MODE_PREVIEW 'p'
#define LANTERN_MODE_STORY 's'
#define LANTERN_MODE_TRANSITION 't'
#define LANTERN_MODE_NIGHT 'n'
#define LANTERN_MODE_TUTO_BEGIN 'b'
#define LANTERN_MODE_TUTO_END 'e'
#define LANTERN_MODE_LEDS_TEST 'd'
#define LANTERN_MODE_PRE_TRANSITION 'r'

char currentLanternMode, previousLanternMode, previousLoopLanternMode;
void setLanternMode(char newLanternMode){
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
#define WIRE_ACTION_STOP_SOUND 2
#define WIRE_ACTION_CHANGE_VOLUME_UP 3
#define WIRE_ACTION_CHANGE_VOLUME_DOWN 4
#define WIRE_ACTION_LOOP_ON 5
#define WIRE_ACTION_LOOP_OFF 6
#define WIRE_ACTION_START_TRANSITION 7
//#define WIRE_ACTION_REACTIVATE_TRANSITION 8

EasyTransferI2C ET;

struct WireDatas {
    byte actionIdentifier;
    char soundFileIdentifier;
    byte minuteTransitionDuration;
} wireDatas;

void launchActionOnSecondaryBoard(byte actionIdentifier){
  wireDatas.actionIdentifier = actionIdentifier;
  ET.sendData(SECONDARY_BOARD_ADDRESS);
}

void playSound(char soundFileIdentifier, boolean loop){
  wireDatas.soundFileIdentifier = soundFileIdentifier;
  //delay(25);//Maybe to delete
  launchActionOnSecondaryBoard(WIRE_ACTION_PLAY_SOUND);
  launchActionOnSecondaryBoard((loop ? WIRE_ACTION_LOOP_ON : WIRE_ACTION_LOOP_OFF));
}

void playSound(char soundFileIdentifier){
  playSound(soundFileIdentifier, false);
}

void stopSound(){
  launchActionOnSecondaryBoard(WIRE_ACTION_LOOP_OFF);
  //delay(25);//Maybe to delete
  launchActionOnSecondaryBoard(WIRE_ACTION_STOP_SOUND);
}

/*================================*/
/*======Gestion des couleurs======*/
/*================================*/

#define COLOR_BLACK Colors(0, 0, 0)
#define COLOR_WHITE Colors(255, 255, 255)
#define COLOR_LIGHT_WHITE Colors(220, 220, 220)

#define COLOR_PRE_TRANSITION_ORANGE Colors(255, 112, 0)
#define COLOR_TRANSITION_ORANGE Colors(225, 85, 0)
#define COLOR_NIGHT_ORANGE Colors(108, 18, 0)

#define COLOR_PREVIEW_DRAGON_ENOSHIMA Colors(25, 65, 155)
#define COLOR_PREVIEW_BOUCLE_OR Colors(245, 195, 10)
#define COLOR_PREVIEW_RENARD_POULE Colors(240, 85, 65)
#define COLOR_PREVIEW_SYLVIE_HIBOU Colors(12, 225, 45)

#define COLOR_DRAGON_RED Colors(235,12,0)

#define COLOR_FOREST_GREEN Colors(0,255,25)
#define COLOR_PAGE_ONE_BOTTOM Colors(63,89,128)
#define COLOR_PAGE_ONE_TOP Colors(23,43,63)

#define COLOR_PAGE_TWO_BOTTOM Colors(12,12,35)
#define COLOR_PAGE_TWO_TOP Colors(45,45,185)

#define COLOR_PAGE_THREE_BOTTOM Colors(0,8,108)
#define COLOR_PAGE_THREE_TOP Colors(60,60,60)

#define COLOR_PAGE_FOUR_BOTTOM Colors(0,135,34)
#define COLOR_PAGE_FOUR_TOP Colors(0,205,32)

/*===============================*/
/*=====Gestion des neopixels=====*/
/*===============================*/

#define LED_VOID 255

#define LEDS_PIN 9
#define NUMBER_OF_LEDS 29
#define NUMBER_OF_LEDS_PER_ROW 7
#define NUMBER_OF_LEDS_PER_COLUMN 4
#define NUMBER_OF_LIGHT_ROWS 5
#define NUMBER_OF_LIGHT_COLUMNS 7

Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUMBER_OF_LEDS, LEDS_PIN, NEO_GRB + NEO_KHZ800);

void rgb(byte index, byte red, byte green, byte blue){
  if(index != LED_VOID){
     leds.setPixelColor(index, green, red, blue); 
  }
}

void rgb(byte red, byte green, byte blue){
  byte i;
  for(i=0;i<NUMBER_OF_LEDS;i++){
    rgb(i, red, green, blue);
  }
}

void applyColorOnLedAtIndex(byte index, Colors color){
  rgb(index, color.red(), color.green(), color.blue());
}

Colors currentColorOfLedAtIndex(byte index){
  uint32_t rawColor = leds.getPixelColor(index);

  //On utilise le mode GRB, du coup, l'ordre des composantes est inversé
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

/*byte getColumnsIndexOf(byte ledIndex){
  if(ledIndex == 0){
    return 3;
  }

  byte row = getRowIndexOfLed(ledIndex);

  return (byte)((int)ledIndex - (int)row*NUMBER_OF_LEDS_PER_ROW + NUMBER_OF_LEDS_PER_ROW - 1);
}*/

/*================================*/
/*=====Gestion des animations=====*/
/*================================*/

//Animations ponctuelles 
#define LIGHT_ANIMATION_NONE 'n'
#define LIGHT_ANIMATION_DRAGON 'd'
#define LIGHT_ANIMATION_FOREST 'f'
#define LIGHT_ANIMATION_SPARKLES 's'

Tween lightAnimationTween;
char currentLightAnimation;

/*Dragon*/
#define LIGHT_ANIMATION_DRAGON_LIGHT_PATH_LENGTH 16
#define LIGHT_ANIMATION_DRAGON_HEAD_SIZE 4
#define LIGHT_ANIMATION_DRAGON_TAIL_SIZE 7

byte lightPath(byte pathIndex){
  if(pathIndex == 7 || pathIndex == 8){
    return LED_VOID;
  }
  else if(pathIndex > 8){
    return pathIndex%2 == 0 ? pathIndex+6 : pathIndex+13;
  }
  return pathIndex%2 == 0 ? pathIndex+1 : pathIndex+8;
}

void lightAnimationInit_Dragon(){
  lightAnimationTween.transition(-4,LIGHT_ANIMATION_DRAGON_LIGHT_PATH_LENGTH+3, 3200);
}

void lightAnimationUpdate_Dragon(){
  int traceStart, traceEnd;
  byte distanceFromMain;
  float gradientCursor;

  Colors
    dragonColor = COLOR_DRAGON_RED,
    backgroundColor = COLOR_BLACK,
    finalColor = COLOR_BLACK;

  byte i, currentLedIndex;

  int currentLightPosition = (int)lightAnimationTween.easeInOutQuadValue();
  //Main
  if(currentLightPosition >= 0 && currentLightPosition < LIGHT_ANIMATION_DRAGON_LIGHT_PATH_LENGTH){
      finalColor = dragonColor;

      currentLedIndex = lightPath(currentLightPosition);
      if(currentLedIndex != LED_VOID){
          applyColorOnLedAtIndex(currentLedIndex, finalColor);
      }
  }

  //Head
  traceStart = currentLightPosition+1;
  traceEnd = currentLightPosition + LIGHT_ANIMATION_DRAGON_HEAD_SIZE;
  distanceFromMain = 0;
  gradientCursor = 0.0;

  for(i=traceStart;i<=traceEnd;i++){
    if(i >= 0 && i < LIGHT_ANIMATION_DRAGON_LIGHT_PATH_LENGTH){
        currentLedIndex = lightPath(i);
        if(currentLedIndex != LED_VOID){
          distanceFromMain = (byte)abs(currentLightPosition-i);
          gradientCursor = (1.0/((float)distanceFromMain*0.45)) * 0.98;
          
          backgroundColor = currentColorOfLedAtIndex(currentLedIndex);
          finalColor = backgroundColor.getGradientStep(gradientCursor, dragonColor);

          applyColorOnLedAtIndex(currentLedIndex, finalColor);
        }
    }
  }

  //Tail
  traceStart = currentLightPosition-1;
  traceEnd = currentLightPosition - LIGHT_ANIMATION_DRAGON_TAIL_SIZE;
  distanceFromMain = 0;
  gradientCursor = 0.0;

  for(i=traceStart;i<=traceEnd;i++){
    if(i >= 0 && i < LIGHT_ANIMATION_DRAGON_LIGHT_PATH_LENGTH){
        currentLedIndex = lightPath(i);
        if(currentLedIndex != LED_VOID){
          distanceFromMain = (byte)abs(currentLightPosition-i);
          gradientCursor = (1.0/((float)distanceFromMain*0.65)) * 0.95;
          
          backgroundColor = currentColorOfLedAtIndex(currentLedIndex);
          finalColor = backgroundColor.getGradientStep(gradientCursor, dragonColor);

          applyColorOnLedAtIndex(currentLedIndex, finalColor);
        }
    }
  }
}

/*---------*/
/*---------*/

/*Forest*/
#define LIGHT_ANIMATION_FOREST_STEP_NUMBER 6
#define LIGHT_ANIMATION_FOREST_TRACE_AFTER_SIZE 3
#define FOREST_CENTER_LED 11

//On
byte lightStepLeftOfCenterAtDistance(byte distance){
  return distance > 3 ? LED_VOID : FOREST_CENTER_LED-distance;
}
byte lightStepRightOfCenterAtDistance(byte distance){
  return distance > 3 ? LED_VOID : FOREST_CENTER_LED+distance;
}
byte lightStepTopOfCenterAtDistance(byte distance){
  if(distance <= 2){
    return FOREST_CENTER_LED+(7*distance);
  }

  return distance > 5 ? LED_VOID : FOREST_CENTER_LED+12+distance;
}
byte lightStepDownOfCenterAtDistance(byte distance){
  if(distance == 2){return 0;}
  if(distance <= 1){
    return FOREST_CENTER_LED-(7*distance);
  }

  return distance > 5 ? LED_VOID : 25-distance+2;
}
byte lightStepTopLeftOfCenterAtDistance(byte distance){
  if(distance == 1 || distance > 4){return LED_VOID;}

  return FOREST_CENTER_LED+7-distance+1;
}
byte lightStepTopRightOfCenterAtDistance(byte distance){
  if(distance == 1 || distance > 4){return LED_VOID;}

  return FOREST_CENTER_LED+7+distance-1;
}
byte lightStepDownLeftOfCenterAtDistance(byte distance){
  if(distance == 1 || distance > 4){return LED_VOID;}

  return FOREST_CENTER_LED-7-distance+1;
}
byte lightStepDownRightOfCenterAtDistance(byte distance){
  if(distance == 1 || distance > 4){return LED_VOID;}

  return FOREST_CENTER_LED-7+distance-1;
}

void lightAnimationInit_Forest(){
  lightAnimationTween.transition(0,LIGHT_ANIMATION_FOREST_STEP_NUMBER+LIGHT_ANIMATION_FOREST_TRACE_AFTER_SIZE, 1050+(100*LIGHT_ANIMATION_FOREST_TRACE_AFTER_SIZE));
}

void lightAnimationUpdate_Forest(){
  int traceStart, traceEnd;
  byte distanceFromMain;
  float gradientCursor;

  byte i, currentLedIndex;

  Colors
    forestColor = COLOR_FOREST_GREEN,
    backgroundColor = COLOR_BLACK,
    finalColor = COLOR_BLACK;

    int currentLightPosition = (int)lightAnimationTween.linearValue();
    byte red, green, blue;
    byte ledTop, ledDown, ledLeft, ledRight, ledTopLeft, ledTopRight, ledDownLeft, ledDownRight;
    
    //Main circle
    if(currentLightPosition >= 0){
      finalColor = forestColor;

      red = finalColor.red();
      green = finalColor.green();
      blue = finalColor.blue();

      if (currentLightPosition == 0)
      {
        rgb(FOREST_CENTER_LED, red, green, blue);
      }
      else{
        if(currentLightPosition < LIGHT_ANIMATION_FOREST_STEP_NUMBER){
          rgb(lightStepLeftOfCenterAtDistance(currentLightPosition), red, green, blue);
          rgb(lightStepRightOfCenterAtDistance(currentLightPosition), red, green, blue);
          rgb(lightStepTopOfCenterAtDistance(currentLightPosition), red, green, blue);
          rgb(lightStepDownOfCenterAtDistance(currentLightPosition), red, green, blue);
          rgb(lightStepTopLeftOfCenterAtDistance(currentLightPosition), red, green, blue);
          rgb(lightStepTopRightOfCenterAtDistance(currentLightPosition), red, green, blue);
          rgb(lightStepDownLeftOfCenterAtDistance(currentLightPosition), red, green, blue);
          rgb(lightStepDownRightOfCenterAtDistance(currentLightPosition), red, green, blue);
        }

        //trace inner circle start Here
        red *= 0.55;
        green *= 0.55;
        blue *= 1.25;

        if(currentLightPosition==1){
          rgb(FOREST_CENTER_LED, red, green, blue);
        }

        if(currentLightPosition>1){
          rgb(lightStepLeftOfCenterAtDistance(currentLightPosition-1), red, green, blue);
          rgb(lightStepRightOfCenterAtDistance(currentLightPosition-1), red, green, blue);
          rgb(lightStepTopOfCenterAtDistance(currentLightPosition-1), red, green, blue);
          rgb(lightStepDownOfCenterAtDistance(currentLightPosition-1), red, green, blue);
          rgb(lightStepTopLeftOfCenterAtDistance(currentLightPosition-1), red, green, blue);
          rgb(lightStepTopRightOfCenterAtDistance(currentLightPosition-1), red, green, blue);
          rgb(lightStepDownLeftOfCenterAtDistance(currentLightPosition-1), red, green, blue);
          rgb(lightStepDownRightOfCenterAtDistance(currentLightPosition-1), red, green, blue);
        }
        
        red *= 0.55;
        green *= 0.65;
        blue *= 0.85;

        if(currentLightPosition==2){
          rgb(FOREST_CENTER_LED, red, green, blue);
        }
        
        if(currentLightPosition > 2){
            rgb(lightStepLeftOfCenterAtDistance(currentLightPosition-2), red, green, blue);
            rgb(lightStepRightOfCenterAtDistance(currentLightPosition-2), red, green, blue);
            rgb(lightStepTopOfCenterAtDistance(currentLightPosition-2), red, green, blue);
            rgb(lightStepDownOfCenterAtDistance(currentLightPosition-2), red, green, blue);
            rgb(lightStepTopLeftOfCenterAtDistance(currentLightPosition-2), red, green, blue);
            rgb(lightStepTopRightOfCenterAtDistance(currentLightPosition-2), red, green, blue);
            rgb(lightStepDownLeftOfCenterAtDistance(currentLightPosition-2), red, green, blue);
            rgb(lightStepDownRightOfCenterAtDistance(currentLightPosition-2), red, green, blue);
        }

        red *= 0.55;
        green *= 0.55;
        blue *= 0.55;

        if(currentLightPosition==3){
          rgb(FOREST_CENTER_LED, red, green, blue);
        }
        
        if(currentLightPosition > 3){
            rgb(lightStepLeftOfCenterAtDistance(currentLightPosition-3), red, green, blue);
            rgb(lightStepRightOfCenterAtDistance(currentLightPosition-3), red, green, blue);
            rgb(lightStepTopOfCenterAtDistance(currentLightPosition-3), red, green, blue);
            rgb(lightStepDownOfCenterAtDistance(currentLightPosition-3), red, green, blue);
            rgb(lightStepTopLeftOfCenterAtDistance(currentLightPosition-3), red, green, blue);
            rgb(lightStepTopRightOfCenterAtDistance(currentLightPosition-3), red, green, blue);
            rgb(lightStepDownLeftOfCenterAtDistance(currentLightPosition-3), red, green, blue);
            rgb(lightStepDownRightOfCenterAtDistance(currentLightPosition-3), red, green, blue);
        }
      }
    }
}

/*---------*/
/*---------*/

/*Sparkles*/
#define LIGHT_ANIMATION_SPARKLES_NUMBER_OF_STEPS 3

byte lightAnimationSparklesCurrentStep;

byte lightAnimationSparklesIndexOfLedSparklingForStep(byte step){
  if(step == 2){
    return 18;
  }

  if(step == 3){
    return 26;
  }

  return step == 0 ? 10 : 12;
}

void lightAnimationSparklesNextStep(boolean firstStep){
  lightAnimationSparklesCurrentStep = firstStep ? 0 : lightAnimationSparklesCurrentStep+1;

  lightAnimationTween.transition(0,5,660);
}

void lightAnimationSparklesNextStep(){
  lightAnimationSparklesNextStep(/*first step*/false);
}

void lightAnimationInit_Sparkles(){
  lightAnimationSparklesNextStep(/*first step*/true);
}

byte lightAnimationSparkles_topLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex + NUMBER_OF_LEDS_PER_ROW*distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

byte lightAnimationSparkles_bottomLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex - NUMBER_OF_LEDS_PER_ROW*distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

byte lightAnimationSparkles_leftLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex - distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

byte lightAnimationSparkles_rightLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex + distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

byte lightAnimationSparkles_topLeftLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex - distance + NUMBER_OF_LEDS_PER_ROW*distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

byte lightAnimationSparkles_topRightLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex + distance + NUMBER_OF_LEDS_PER_ROW*distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

byte lightAnimationSparkles_bottomLeftLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex - distance - NUMBER_OF_LEDS_PER_ROW*distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

byte lightAnimationSparkles_bottomRightLedIndexAtDistanceForCenter(int distance, byte centerIndex){
  int ledIndex = (int)centerIndex + distance - NUMBER_OF_LEDS_PER_ROW*distance;
  return (ledIndex > 0 && ledIndex < NUMBER_OF_LEDS) ? (byte)ledIndex : LED_VOID; 
}

void lightAnimationUpdate_Sparkles(){
  byte currentCenterLedIndex =  lightAnimationSparklesIndexOfLedSparklingForStep(lightAnimationSparklesCurrentStep);
  float gradientCursor;

  int currentSparklePosition = (int)lightAnimationTween.linearValue();
  float whiteIntensity = 1.12-(float)lightAnimationTween.linearCursor();
  whiteIntensity = whiteIntensity < 0 ? 0 : whiteIntensity;

  Colors
    backgroundColor = COLOR_BLACK,
    finalColor = COLOR_BLACK;

  byte red, green, blue;
  byte ledTop, ledDown, ledLeft, ledRight, ledTopLeft, ledTopRight, ledDownLeft, ledDownRight;

  if(currentSparklePosition == 0){
    backgroundColor = currentColorOfLedAtIndex(currentCenterLedIndex);
    finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);

    applyColorOnLedAtIndex(currentCenterLedIndex, finalColor);
  }
  else if(currentSparklePosition > 0){

    ledTop = lightAnimationSparkles_topLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
    backgroundColor = currentColorOfLedAtIndex(ledTop);
    finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
    applyColorOnLedAtIndex(ledTop, finalColor);

    ledDown = lightAnimationSparkles_bottomLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
    backgroundColor = currentColorOfLedAtIndex(ledDown);
    finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
    applyColorOnLedAtIndex(ledDown, finalColor);

    ledLeft = lightAnimationSparkles_leftLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
    backgroundColor = currentColorOfLedAtIndex(ledLeft);
    finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
    applyColorOnLedAtIndex(ledLeft, finalColor);

    ledRight = lightAnimationSparkles_rightLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
    backgroundColor = currentColorOfLedAtIndex(ledRight);
    finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
    applyColorOnLedAtIndex(ledRight, finalColor);

    whiteIntensity *= 0.9;

    if(currentSparklePosition > 1){
      ledTopLeft = lightAnimationSparkles_topLeftLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledTopLeft);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledTopLeft, finalColor);

      ledTopRight = lightAnimationSparkles_topRightLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledTopRight);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledTopRight, finalColor);

      ledDownLeft = lightAnimationSparkles_bottomLeftLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledDownLeft);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledDownLeft, finalColor);

      ledDownRight = lightAnimationSparkles_bottomRightLedIndexAtDistanceForCenter(currentSparklePosition, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledDownRight);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledDownRight, finalColor);
    }

    whiteIntensity *= 0.65;

    if(currentSparklePosition == 1){
      backgroundColor = currentColorOfLedAtIndex(currentCenterLedIndex);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(currentCenterLedIndex, finalColor);
    }
    if(currentSparklePosition > 1){
      ledTop = lightAnimationSparkles_topLedIndexAtDistanceForCenter(currentSparklePosition-1, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledTop);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledTop, finalColor);

      ledDown = lightAnimationSparkles_bottomLedIndexAtDistanceForCenter(currentSparklePosition-1, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledDown);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledDown, finalColor);

      ledLeft = lightAnimationSparkles_leftLedIndexAtDistanceForCenter(currentSparklePosition-1, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledLeft);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledLeft, finalColor);

      ledRight = lightAnimationSparkles_rightLedIndexAtDistanceForCenter(currentSparklePosition-1, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledRight);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledRight, finalColor);
    }

    whiteIntensity *= 0.45;

    if(currentSparklePosition == 2){
      backgroundColor = currentColorOfLedAtIndex(currentCenterLedIndex);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(currentCenterLedIndex, finalColor);
    }
    if(currentSparklePosition > 2){
      ledTop = lightAnimationSparkles_topLedIndexAtDistanceForCenter(currentSparklePosition-2, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledTop);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledTop, finalColor);

      ledDown = lightAnimationSparkles_bottomLedIndexAtDistanceForCenter(currentSparklePosition-2, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledDown);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledDown, finalColor);

      ledLeft = lightAnimationSparkles_leftLedIndexAtDistanceForCenter(currentSparklePosition-2, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledLeft);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledLeft, finalColor);

      ledRight = lightAnimationSparkles_rightLedIndexAtDistanceForCenter(currentSparklePosition-2, currentCenterLedIndex);
      backgroundColor = currentColorOfLedAtIndex(ledRight);
      finalColor = backgroundColor.getGradientStep(whiteIntensity, COLOR_WHITE);
      applyColorOnLedAtIndex(ledRight, finalColor);
    }

  }

  if(lightAnimationTween.isEnded() && lightAnimationSparklesCurrentStep <= LIGHT_ANIMATION_SPARKLES_NUMBER_OF_STEPS){
    lightAnimationSparklesNextStep();
  }
}

/*---------*/
/*---------*/
/*---------*/

void applyLightAnimation(){
  switch (currentLightAnimation) {
      case LIGHT_ANIMATION_DRAGON:
        if(!lightAnimationTween.isEnded()){
          lightAnimationUpdate_Dragon();
        }
      break;

      case LIGHT_ANIMATION_FOREST:
        if(!lightAnimationTween.isEnded()){
          lightAnimationUpdate_Forest();
        }
      break;

      case LIGHT_ANIMATION_SPARKLES:
        if(!lightAnimationTween.isEnded() || lightAnimationSparklesCurrentStep <= LIGHT_ANIMATION_SPARKLES_NUMBER_OF_STEPS){
          lightAnimationUpdate_Sparkles();
        }
      break;

      default:
      break;
  }
}

void updateLightAnimation(){
  if(!lightAnimationTween.isEnded()){
    lightAnimationTween.update(time.delta());
  }
}

void playLightAnimation(char lightAnimationCode){
  currentLightAnimation = lightAnimationCode;

  switch (currentLightAnimation) {
      case LIGHT_ANIMATION_DRAGON:
        lightAnimationInit_Dragon();
      break;

      case LIGHT_ANIMATION_FOREST:
        lightAnimationInit_Forest();
      break;

      case LIGHT_ANIMATION_SPARKLES:
        lightAnimationInit_Sparkles();
      break;
      
      default:
        lightAnimationTween.transition(0,0,0);
      break;
  }
}

void playLightAnimationIfPossible(char lightAnimationCode){
  if(lightAnimationTween.isEnded() && currentLanternMode == LANTERN_MODE_STORY){
      playLightAnimation(lightAnimationCode);
  }
}

//Animations d'ambiance
boolean ambiantIsTransitionning(){
  return bitList.getValue(BIT_LIST_INDEX_AMBIANT_IS_TRANSITIONNING);
}

void ambiantIsTransitionning(boolean value){
  bitList.setValue(BIT_LIST_INDEX_AMBIANT_IS_TRANSITIONNING, value);
}

Tween ambiantTransitionTween, lightBreathTween;

Colors ledsOriginColor[NUMBER_OF_LEDS];
Colors ledsTargetColor[NUMBER_OF_LEDS];

void launchAmbiantTransition(unsigned long transitionDuration){
  ambiantIsTransitionning(true);
  ambiantTransitionTween.transition(0, 100, (unsigned int)transitionDuration);
}

void updateAmbiantTransition(){
  if(ambiantTransitionTween.isEnded()){
      ambiantIsTransitionning(false);
  }

  ambiantTransitionTween.update(time.delta());
}

void updateLightBreath(){
  if(!ambiantIsTransitionning()){
    lightBreathTween.update(time.delta());
  }
}

float getLightBreathAlpha(){
  return (float)lightBreathTween.easeInOutQuartValue() / 255.0;
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

void applyAmbiantColor(boolean breathing, boolean justTheMiddle, boolean useMinuteDuration){
  byte i;

  if(ambiantIsTransitionning()){
    if(useMinuteDuration){
        for(i=0;i<NUMBER_OF_LEDS;i++){
          Colors ledColor = ledsOriginColor[i].getGradientStep(
          (float)(((float)((float)(transitionNumberOfMinutesPast-1)+ambiantTransitionTween.easeInOutQuadCursor()))/(float)wireDatas.minuteTransitionDuration),
          ledsTargetColor[i]);
          applyColorOnLedAtIndex(i, ledColor);
        }
    }
    else{
      for(i=0;i<NUMBER_OF_LEDS;i++){
        Colors ledColor = ledsOriginColor[i].getGradientStep(ambiantTransitionTween.easeInOutQuadCursor(), ledsTargetColor[i]);
        applyColorOnLedAtIndex(i, ledColor);
      }
    }
  }
  else if(breathing){
    float breathingAlpha = getLightBreathAlpha();
    boolean onMiddle = false;
    for(i=0;i<NUMBER_OF_LEDS;i++){
      Colors temp_ledColor = ledsTargetColor[i];

      onMiddle = (i == 0 || i == 4 || i == 11 || i == 18 || i == 25);
      Colors finalColor = (!justTheMiddle || onMiddle) ? temp_ledColor.getColorWithAlpha(breathingAlpha) : temp_ledColor;

      applyColorOnLedAtIndex(i, finalColor);
    }
  }
}

void applyAmbiantColor(boolean breathing, boolean justTheMiddle){
  applyAmbiantColor(breathing, justTheMiddle, false);
}

void applyAmbiantColor(boolean breathing){
  applyAmbiantColor(breathing, false);
}

//Gestion des differents rythme de respiration
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

void noBreath(){
  breath(0, 0, 0, 0);
}

void justRoundBreath(){
  breath(0, 148, 3900, 150);
}

void simpleAmbiantBreath(){
  breath(0, 182, 6200, 248);
}

void tutoBreath(){
  breath(0, 220, 980, 150);
}

/*==============================*/
/*=====Gestion du bluetooth=====*/
/*==============================*/
#define TIME_WITHOUT_CONNECTION_BEFORE_RETURN_TO_NIGHT_MODE 60000
unsigned int timeWithoutConnection;

#define REQ_PIN 10
#define RDY_PIN 2
#define RST_PIN 7

Adafruit_BLE_UART bluetooth = Adafruit_BLE_UART(REQ_PIN, RDY_PIN, RST_PIN);

#define BLE_INSTRUCTION_APPLICATION_LAUNCH 'l'
#define BLE_INSTRUCTION_SET_MODE 'm'
#define BLE_INSTRUCTION_SET_STORY 's'
#define BLE_INSTRUCTION_SET_STORY_PAGE 'p'
#define BLE_INSTRUCTION_SET_TRANSITION_TIME 't'
#define BLE_INSTRUCTION_PLAY_ANIMATION 'a'
#define BLE_INSTRUCTION_PLAY_SOUND 'n'
#define BLE_INSTRUCTION_GET_CURRENT_LANTERN_MODE 'g'

boolean bluetoothPreviousLoopConnected(){
  return bitList.getValue(BIT_LIST_INDEX_BLUETOOTH_PREVIOUS_LOOP_CONNECTED);
}

void bluetoothPreviousLoopConnected(boolean value){
  bitList.setValue(BIT_LIST_INDEX_BLUETOOTH_PREVIOUS_LOOP_CONNECTED, value);
}

boolean bluetoothIsConnected(){
  return bluetooth.getState() == ACI_EVT_CONNECTED;
}

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
  char bleParamTwo = '0';

  if(len > 1){
    bleParamOne = (char)buffer[1];
  }
  if(len > 2){
    bleParamTwo = (char)buffer[2];
  }

  if(bleInstruction == BLE_INSTRUCTION_APPLICATION_LAUNCH){
      launchActionOnSecondaryBoard(WIRE_ACTION_START_LANTERN);
  }
  else if(bleInstruction == BLE_INSTRUCTION_SET_MODE){
      setLanternMode(bleParamOne);

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
    wireDatas.minuteTransitionDuration = (byte)bleParamOne;
  }
  else if(bleInstruction == BLE_INSTRUCTION_PLAY_ANIMATION){
    playLightAnimationIfPossible(bleParamOne);
  }
  else if(bleInstruction == BLE_INSTRUCTION_PLAY_SOUND){
    playSound(bleParamOne, (bleParamTwo == '1'));
  }
  else if(bleInstruction == BLE_INSTRUCTION_GET_CURRENT_LANTERN_MODE){
    sendMessage(&currentLanternMode, 1);
  }
}

/*==============================*/
/*==============================*/
/*==============================*/

//closed/opened
#define CLOSED_PIN 4

#define CHECKING_IF_LANTERN_IS_OPENED_TIME_INTERVAL 1000

unsigned int timeSinceLastLanternOpenedChecking;

boolean lanternIsClosed(){
  return digitalRead(CLOSED_PIN);
}

boolean lanternIsOpen(){
  return !lanternIsClosed();
}

//Infrared Sensor
#define INFRARED_SENSOR_PIN A3

boolean childReactiveLanternTransition(){
  if (lanternIsOpen())
  {
    return false;
  }

  return digitalRead(INFRARED_SENSOR_PIN) == 0 ? true : false;
}


/*==============================*/
/*======Gestion du volume=======*/
/*==============================*/
#include <Rotary.h>

#define ROTARY_LEFT_PIN A1
#define ROTARY_RIGHT_PIN A0

Rotary volumeRotaryEncoder = Rotary(ROTARY_LEFT_PIN, ROTARY_RIGHT_PIN);

void updateVolume(){
    unsigned char volumeRotaryEncoderDirection = volumeRotaryEncoder.process();
    
    if(volumeRotaryEncoderDirection){
      if(volumeRotaryEncoderDirection == DIR_CW){
        launchActionOnSecondaryBoard(WIRE_ACTION_CHANGE_VOLUME_UP);
      }
      else{
        launchActionOnSecondaryBoard(WIRE_ACTION_CHANGE_VOLUME_DOWN);
      }   
    }
}

/*==============================*/
/*====Gestion des statuts=======*/
/*==============================*/

#define RED_STATE_LED_PIN 3
#define GREEN_STATE_LED_PIN 5

//#define STATE_LED_ON_OFF_PIN A2
//boolean stateLedOn, previousSwitchStateLed;

//Le bouton du rotary encoder permet d'éteindre la led de statut
/*void updateSwitchLedState(){
  boolean switchStateLed = digitalRead(STATE_LED_ON_OFF_PIN);
  if(switchStateLed != previousSwitchStateLed && !switchStateLed){
    stateLedOn = !stateLedOn;
  }
  previousSwitchStateLed = switchStateLed;
}*/

/*Tween stateLedBreathTween;
byte stateLedIntensity;
void updateStateLedIntensity(){
  stateLedBreathTween.update(time.delta());
  stateLedIntensity = (byte)(stateLedBreathTween.easeInQuintValue());
}*/

void redLed(){
  //analogWrite(RED_STATE_LED_PIN, stateLedOn ? stateLedIntensity : 0);
  analogWrite(RED_STATE_LED_PIN, 180);
  analogWrite(GREEN_STATE_LED_PIN, 0);
}

void greenLed(){
  analogWrite(RED_STATE_LED_PIN, 0);
  analogWrite(GREEN_STATE_LED_PIN, 180);
}

void yellowLed(){
  analogWrite(RED_STATE_LED_PIN, 120);
  analogWrite(GREEN_STATE_LED_PIN, 120);
  /*analogWrite(RED_STATE_LED_PIN, stateLedOn ? stateLedIntensity/1.5 : 0);
  analogWrite(GREEN_STATE_LED_PIN, stateLedOn ? stateLedIntensity/1.5 : 0);*/
}

void orangeLed(){
  analogWrite(RED_STATE_LED_PIN, 90);
  analogWrite(GREEN_STATE_LED_PIN, 23);
  /*analogWrite(RED_STATE_LED_PIN, stateLedOn ? stateLedIntensity/2 : 0);
  analogWrite(GREEN_STATE_LED_PIN, stateLedOn ? stateLedIntensity/8 : 0);*/
}

boolean firstLoop(){
  return bitList.getValue(BIT_LIST_INDEX_FIRST_LOOP);
}

void firstLoop(boolean value){
  bitList.setValue(BIT_LIST_INDEX_FIRST_LOOP, value);
}

boolean lanternReady(){
  return bitList.getValue(BIT_LIST_INDEX_LANTERN_READY);
}

void lanternReady(boolean value){
  bitList.setValue(BIT_LIST_INDEX_LANTERN_READY, value);
}

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
  pinMode(INFRARED_SENSOR_PIN, INPUT);

  firstLoop(true);
  lanternReady(false);

  //pinMode(STATE_LED_ON_OFF_PIN, INPUT);
  
  /*firstLoop = true;
  lanternReady = false;*/
  /*stateLedOn = true;
  previousSwitchStateLed = LOW;*/
  
  bluetoothPreviousLoopConnected(false);

  previousLanternMode = LANTERN_MODE_INIT;
  currentLanternMode = LANTERN_MODE_INIT;
  previousLoopLanternMode = LANTERN_MODE_INIT;
  
  wireDatas.soundFileIdentifier = 'n';

  //stateLedIntensity = 0;
  
  /*stateLedBreathTween.transition(20, 200, 3500, 1000);
  stateLedBreathTween.loopWithDelay();
  stateLedBreathTween.reverseLoop();*/
  
  ambiantIsTransitionning(false);

  setStory(STORY_NO_STORY);

  timeSinceLastLanternOpenedChecking = 0;
  timeWithoutConnection = 0;

  currentPage = '0';
  previousPage = '0';

  wireDatas.minuteTransitionDuration = 1;
  transitionToNightStarted(false);

  lightAnimationTween.transition(0,0,0);
  currentLightAnimation = LIGHT_ANIMATION_NONE;

  transitionNumberOfMinutesPast = 0;

  //randomSeed(analogRead(0));
  transitionEnded(false);
  time.init();
}

void loop()
{
  boolean modeChanged, storyChanged, pageChanged;
  
  /*if(!bluetoothPreviousLoopConnected() && bluetoothIsConnected()){
    Serial.println(F("connexion"));
    launchActionOnSecondaryBoard(WIRE_ACTION_START_LANTERN);
  }*/

  
  //Test open and infrared sensor
  /*if(lanternIsOpen()){
    greenLed();
    rgb(0,255,0);
  }
  else{
    if(childReactiveLanternTransition()){
      playSound('n');
      yellowLed();
      rgb(0,0,255);
    }
    else{
      redLed();
      rgb(255,150,0);
    }

  }

  leds.show();

  return;*/

  if(!bluetoothIsConnected() && (currentLanternMode == LANTERN_MODE_TUTO_BEGIN || currentLanternMode == LANTERN_MODE_TUTO_END)){
    setLanternMode(LANTERN_MODE_NIGHT);
  }

  modeChanged = false;
  storyChanged = currentStoryCode != previousLoopStoryCode;
  pageChanged = currentPage != previousLoopPage;

  time.loopStart();

  /*updateStateLedIntensity();
  updateSwitchLedState();*/

  updateAmbiantTransition();
  updateLightAnimation();
  updateLightBreath();
  
  //On attend un peu pour etre sur que la secondary board est prete à recevoir des données
  if(!lanternReady()){
    redLed();
    
    if(time.total() > TIME_BEFORE_LANTERN_IS_READY){
      lanternReady(true);
    }
  }
  
  if(firstLoop()){
    if(lanternReady()){
      bluetooth.begin();
      
      //launchActionOnSecondaryBoard(WIRE_ACTION_START_LANTERN);
      setLanternMode(LANTERN_MODE_NIGHT);

      firstLoop(false);
    }
  }
  
  if(lanternReady()){
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
  
  bluetoothPreviousLoopConnected(bluetoothIsConnected());

  time.loopEnd();
}

/*=======================================*/
/*====Gestion des modes de la lantern====*/
/*=======================================*/

void manageLanternMode(boolean modeChanged, boolean storyChanged, boolean pageChanged){
  boolean checkIfLanternIsOpened, nextTurnNeedAModeChanged;
  byte i, j;
  Colors newLedsTargetColors[NUMBER_OF_LEDS];
  Colors gradientBottomColor, gradientTopColor;

  nextTurnNeedAModeChanged = false;

  if(currentLanternMode == LANTERN_MODE_LEDS_TEST){
    for(i=0;i<NUMBER_OF_LEDS;i++){
        rgb(0,0,0);
        applyColorOnLedAtIndex(i,COLOR_WHITE);
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
      transitionToNightStarted(false);
  }

  //On definit les events pour chaque mode
  switch(currentLanternMode){
    case LANTERN_MODE_INIT:
      bluetoothIsConnected() ? greenLed() : yellowLed();
      transitionEnded(false);
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

            case STORY_RENARD_POULE:
              newLedsTargetColors[0] = COLOR_PREVIEW_RENARD_POULE;
              for(i=1;i<NUMBER_OF_LEDS;i++){
                newLedsTargetColors[i] = COLOR_BLACK;
              }
            break;

            case STORY_SYLVIE_HIBOU:
              newLedsTargetColors[0] = COLOR_PREVIEW_SYLVIE_HIBOU;
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

      transitionEnded(false);

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

      
      applyLightAnimation();
    break;
    //END STORY MODE

    case LANTERN_MODE_PRE_TRANSITION:
      bluetoothIsConnected() ? greenLed() : yellowLed();
      
      if(modeChanged){
        for(i=0;i<NUMBER_OF_LEDS;i++){
          newLedsTargetColors[i] = COLOR_PRE_TRANSITION_ORANGE;
        }

        ambiantTransition(newLedsTargetColors);
        justRoundBreath();
      }
      else{
        applyAmbiantColor(/*breathing*/true);
      }
    break;
    
    case LANTERN_MODE_TRANSITION:
      orangeLed();

      if(modeChanged){
        transitionNumberOfMinutesPast = 0;
        transitionToNightStarted(true);

        for(i=0;i<NUMBER_OF_LEDS;i++){
          newLedsTargetColors[i] = COLOR_TRANSITION_ORANGE;
        }

        ambiantTransition(newLedsTargetColors, 800);
        noBreath();
        transitionEnded(false);
      }
      else{
        if(transitionToNightStarted() && transitionNumberOfMinutesPast == 0){
            applyAmbiantColor(/*breathing*/false);
        }
        else if(transitionNumberOfMinutesPast > 0){
          applyAmbiantColor(
            /*breathing*/false,
            /*justOnMidle*/false,
            /*useMinuteDuration*/true
          );
        }
        
        //Si la transition pour arriver au mode transition est finie, on lance la transition vers le mode nuit
        //Comme la durée de transition peut s'étaler sur plusieurs minutes, on repete le tween plusieurs fois
        if(ambiantTransitionTween.isEnded()){
          if(transitionToNightStarted()){
              if(transitionNumberOfMinutesPast == 0){
                  newLedsTargetColors[0] = COLOR_NIGHT_ORANGE;

                  for(i=1;i<NUMBER_OF_LEDS;i++){
                    newLedsTargetColors[i] = COLOR_BLACK;
                  }
                  ambiantTransition(newLedsTargetColors, minuteDuration(1));
                  launchActionOnSecondaryBoard(WIRE_ACTION_START_TRANSITION);
              }
              else{
                ambiantTransitionTween.replay();
              }

              transitionNumberOfMinutesPast++;

              if(transitionNumberOfMinutesPast >= wireDatas.minuteTransitionDuration){
                transitionToNightStarted(false);
              }
              else{
                ambiantIsTransitionning(true);
              }
          }
          else{
            transitionEnded(true);
            setLanternMode(LANTERN_MODE_NIGHT);
            nextTurnNeedAModeChanged = true;
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

      if(childReactiveLanternTransition() && transitionEnded()){
        setLanternMode(LANTERN_MODE_TRANSITION);
        //launchActionOnSecondaryBoard(WIRE_ACTION_REACTIVATE_TRANSITION);
        nextTurnNeedAModeChanged = true;
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

  if (!nextTurnNeedAModeChanged)
  {
    previousLoopLanternMode = currentLanternMode;
  }
  previousLoopStoryCode = currentStoryCode;
  previousLoopPage = currentPage;
}

