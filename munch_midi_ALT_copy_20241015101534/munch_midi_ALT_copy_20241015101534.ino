#include <HCSR04.h>
#include "MIDIUSB.h"
//REMEMBER TO ADD MIDIUSB LIBRARY VIA LIBRARY MANAGER
//SELECT BOARD "ARDUINO LEONARDO" or "ARDUINO MICRO"

// which inpouts are connected to knobs? 
bool activatedKnobs[8] = {true, false, false, false, false, false, false, false}; // A0, A1, A2, A3, A6, A7, A8, A9
bool activatedButtons[8] = {true, false, false, false, false, false, false}; // D2, D3, D5, D7, D14, D15, D16
bool useDistanceSensor = false;

int previousButtonState[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};   // for checking the state of a pushButton
int buttPins[7] = {2, 3, 5, 7, 14, 15, 16};
int knobPins[8] = {A0,A1,A2,A3,A6,A7,A8,A9};
int notes[8] = {67, 65, 66, 64, 68, 69, 70, 71};
int knobCCs[8] = {10,11,12,13,14,15,16,17};
unsigned long debounce[8] {0, 0, 0, 0, 0, 0, 0, 0};
int debounceT = 10;
int oldKnobs[8] = {0,0,0,0,0,0,0,0};

bool noteMode = false; //mode to store Notes or CC

byte triggerPin = 15;
byte echoPin = 16;

void setup() {
  // make the pushButton pin an input:
  for (byte i = 0; i < 8; i++) {
    pinMode(buttPins[i], INPUT_PULLUP);
  }
  if(useDistanceSensor){
    HCSR04.begin(triggerPin, echoPin);
  }
}




void loop() {
 handleButtons();
 handleKnobs();
 handleOtherSensors();
// handleNeoPixels();
}

int diffThresh = 10;
void handleKnobs(){
  bool wantToSend = false;
  for(int i = 0; i<8; i++){
    int thisKnob = analogRead(knobPins[i]);
    if(thisKnob < oldKnobs[i] - diffThresh | thisKnob > oldKnobs[i] + diffThresh ){
      oldKnobs[i] = thisKnob;
      int midiVal = thisKnob>>3;
      if(activatedKnobs[i]){
      /*
        //DEBUGGING
        Serial.print("knob ");
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(thisKnob);
        Serial.println();
      */
      controlChange(0,knobCCs[i],midiVal);  
      wantToSend = true;
      }
    }
  }
  if(wantToSend){
    MidiUSB.flush();
  }
}

void handleButtons(){
  for (byte i = 0; i < 7; i++) {
    if (debounce[i] < millis()) {
      bool buttonState = !digitalRead(buttPins[i]);
      if(!buttonState){

      }
      if (buttonState && !previousButtonState[i]) {
        debounce[i] = millis() + debounceT;
        if(noteMode){
          noteOn(0, notes[i], 64);   // Channel 0, middle C, normal velocity  
        } else {
          controlChange(0,127,notes[i]);
        }
        MidiUSB.flush();
      }
      if (!buttonState && previousButtonState[i]) {
        debounce[i] = millis() + debounceT;
        if(noteMode){
          noteOff(0, notes[i], 64);   // Channel 0, middle C, normal velocity  
        } else {
          controlChange(0,0,notes[i]);
        }
        
        MidiUSB.flush();
      }
      previousButtonState[i] = buttonState;
    }
  }
}


void handleOtherSensors(){
  if(useDistanceSensor){
    double* distances = HCSR04.measureDistanceCm();
    uint16_t distanceMidi = min((int)distances[0], 127);
    controlChange(0,100,distanceMidi);
    MidiUSB.flush();
  }
  
}




//MIDI MESSAGE DEFINITIONS
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}


// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
