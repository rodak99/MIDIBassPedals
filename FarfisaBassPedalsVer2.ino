/*
   MIDI controller for Farfisa Compact Bass Pedals
   Basically, it's just a 13-note MIDI keyboard, could be wired up to work with any pedals or even an analog keybord with keyswitches
*/
#include <MIDI.h>
#include <midi_defs.h>
#include <midi_message.h>
#include <midi_namespace.h>
#include <midi_settings.h>

/* Adafruit backpack stuff */
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
Adafruit_7segment matrix = Adafruit_7segment();

MIDI_CREATE_INSTANCE(HardwareSerial,Serial, midiOut); // create a MIDI object called midiOut

const byte midiCh = 1;
const byte numKeys = 13;
const int rotarysw_min = 0;
const int rotarysw_max = 1023;

byte i;
int actualreading;
double reading;
double mapped_reading;
int octave = 0;
int octavesw_analogval;
int octavesw_val;
int octavelast = 99;
unsigned long octave_elapsed = 0;
unsigned long octavelastDebounceTime;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastkeyoffsetDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay =75;    // the debounce time; increase if the output flickers

int keyoffsetsw_analogval;
int keyoffsetsw_val;
int keyoffsetlast = 99;
int keyoffset;
unsigned long elapsed_time = 0;

/*
Debounce variables
*/
byte  keyCState[numKeys+2] = {0};  // key Current state
byte  keyPState[numKeys+2] = {0};  // key Previous state
unsigned long lastDebounceTime[numKeys+2] = {0};
unsigned long debounceTime = 50;

//
//  Key/Pin definitions.  numKeys is number of Keys connected (duh)
//  KeyPin is the Arduino pin each Key is connected to
//  KeyMIDIcc is the MIDI control change that will be associated with that Key
//  KeyMIDIStateOff/On are the on/off states.  These Keyles are only necessary because the Reverb Key on the Compact is NormallyClosed, and when depressed
//    switches to Open - All the other Keys are NormallyOpen, switching to Closed when depressed.
//

const byte PinKey[numKeys]          =   {2, 3, 4, 5, 6, 7, 8, 9,10,11,12,14,15};
const byte PinNote[numKeys]      =  {36,37,38,39,40,41,42,43,44,45,46,47,48}; 
// const byte PinNote[numKeys]         =  {23,24,25,26,27,28,29,30,31,32,33,34,35};

//
//  Table to convert numeric "key" to alpha display in the 7-segment matrix
//
//                           C     Db    D     Eb    E     F     Gb    G     Ab    A     Bb    B
const byte KeyDisp1[12]  = {0x39, 0x3F, 0X3f, 0x79, 0x79, 0x71, 0x7D, 0x7D, 0x77, 0x77, 0x7F, 0x7F};
const byte KeyDisp2[12]  = {0x00, 0x7C, 0x00, 0x7C, 0x00, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x7C, 0x00};


void setup() {
  matrix.begin(0x70);  /* ADAFruit backpack init */
  matrix.writeDigitNum(2,1,false);  
  matrix.setBrightness(2);
  matrix.writeDisplay();
  
  Serial.begin(31250);  // Open MIDI serial port
     
  // initialize the pins as an input with pullup resistor:
    for (i = 0; i < numKeys; i++) {
      pinMode(PinKey[i], INPUT_PULLUP);
    }
  pinMode(13,OUTPUT);   /* Turn on power LED */
  digitalWrite(13,HIGH); 
  
  pinMode(A2,INPUT);   /* Octave switch */
  pinMode(A3,INPUT);   /* Pitch offset switch */

  lastkeyoffsetDebounceTime = millis();
  octavelastDebounceTime = millis();
    
//
// Start with all notes off 
//for (i = 0; i < 127; i++)  
//  {
//    midiOut.sendNoteOff(i,0,1);
//  }
//
  }
  
void loop() {
  
  // Get state of all keys, reset Debounce timers
  for (i = 0; i < numKeys; i++) {
    keyCState[i] = digitalRead(PinKey[i]);
    lastDebounceTime[i] = 0;
  }
  /*
  /* Check Octave Switch */
  
  reading = analogRead(A2);
  if (reading > 1000) 
  {
    reading = 1023;   // Correct for flakiness in the #4 position, bouncing all around 1012-1023, so map command returned 3 or 4
  }
  octavesw_val = map(reading,rotarysw_min,rotarysw_max,0,4);
  octave = octavesw_val * 12 ;
  octave_elapsed = millis() - octavelastDebounceTime ;
  if (octavesw_val != octavelast) 
  {
    if (octave_elapsed > debounceDelay)
    {
    matrix.writeDigitNum(4,octavesw_val+1,false);
    matrix.writeDisplay();
          //Serial.print("Octave: ");
          //Serial.print(octave);
          //Serial.print("  Reading: ");
          //Serial.print(reading);
          //Serial.print("  Current Octave: ");
          //Serial.print(octavesw_val);
          //Serial.print("  Previous Octave: ");
          //Serial.print(octavelast);
          //Serial.print(" elapsed time: ");
          //Serial.println(octave_elapsed);
    octavelast = octavesw_val;
    }
  }
  else
  {
    octavelastDebounceTime = millis();
  }
  
  /* Check Pitch Offset Switch */
  reading = analogRead(A3);
  if (reading > 1000) 
  {
    reading = 1022;   // Correct for flakiness in the #12 position, bouncing all around 1012-1023, so map command returned 3 or 4
  }
  if ((reading > 80) && (reading < 170))
  {
    reading = 92;   // Correct for flakiness in the #2 position
  }
  mapped_reading = map(reading,0,1023,0,12);
  keyoffset = mapped_reading;
    elapsed_time = millis() - lastkeyoffsetDebounceTime ;
  if (keyoffset != keyoffsetlast)
    {
    if (elapsed_time > debounceDelay) 
      {
      lastkeyoffsetDebounceTime = millis();
          matrix.writeDigitRaw(0,KeyDisp1[keyoffset]);
          matrix.writeDigitRaw(1,KeyDisp2[keyoffset]);
          matrix.writeDisplay();
          keyoffsetlast = keyoffset;
          lastkeyoffsetDebounceTime = millis();
       }
     } 
     else
     {
       lastkeyoffsetDebounceTime = millis();
     }
  //Check state of all keys if debounce timer has expired
  //key pressed equals LOW, key released equals HIGH
  for (i = 0; i < numKeys; i++) {
   
    if (keyCState[i] != keyPState[i]) {
      
      if ((millis() - lastDebounceTime[i]) > debounceTime) {
        lastDebounceTime[i] = millis();
        if (keyCState[i] == LOW)
        {
          midiOut.sendNoteOn(PinNote[i]+octave+keyoffset,127,1);
        }
        else
        {
        midiOut.sendNoteOff(PinNote[i]+octave+keyoffset,0,1);
        }

        keyPState[i] = keyCState[i];
    
    }
  }
 }
}
