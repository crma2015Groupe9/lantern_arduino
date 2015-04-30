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
  
  if((char)buffer[0] == 'v'){
    Serial.println(wireDatas.audioVolume);
  }
}

boolean bluetoothIsConnected(){
  return bluetooth.getState() == ACI_EVT_CONNECTED;
}

/*==============================*/
/*==============================*/
/*==============================*/

#define CLOSED_PIN 4

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

//Definition des modes de la veilleuse
#define LANTERN_MODE_INIT 0
#define LANTERN_MODE_PREVIEW 1
#define LANTERN_MODE_STORY 2
#define LANTERN_MODE_TRANSITION 3
#define LANTERN_MODE_NIGHT 4

byte currentLanternMode, previousLanternMode;

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
  
  wireDatas.audioVolume = 0;
  stateLedIntensity = 0;
  
  stateLedBreath.transition(20, 200, 3500, 1000);
  stateLedBreath.loopWithDelay();
  stateLedBreath.reverseLoop();
  
  time.init();
}

void loop()
{
  time.loopStart();
  
  updateStateLedIntensity();
  updateSwitchLedState();
  
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
    
      firstLoop = false;
    }
  }
  
  if(lanternReady){
    //Gestion des changement de mode
    if(currentLanternMode != previousLanternMode){
    
    }
    
    //On definit les events pour chaque mode
    switch(currentLanternMode){
      case LANTERN_MODE_INIT:
        bluetoothIsConnected() ? greenLed() : yellowLed();
      break;
      
      case LANTERN_MODE_PREVIEW:
        bluetoothIsConnected() ? greenLed() : yellowLed();
      break;
      
      case LANTERN_MODE_STORY:
        bluetoothIsConnected() ? greenLed() : yellowLed();
      break;
      
      case LANTERN_MODE_TRANSITION:
        orangeLed();
      break;
      
      case LANTERN_MODE_NIGHT:
        orangeLed();
      break;
      
      default:
      break;
    }
    
    updateVolume();
    
    /*
    if(lanternIsClosed()){
      rgb(255,0,0);
    }
    if(lanternIsOpen()){
      rgb(0,255,0);
    }*/
  
    bluetooth.pollACI();
  
    leds.show();
  }
  
  time.loopEnd();
}
