#include <Vector.h>
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();
#include "MCP_DAC.h"  
MCP4921 myDAC;

/*
Pinout:
  - Pin 9:  CLOCK
  - Pin 10: SS
  - Pin 13: SCLK
  - Pin 11: MOSI

  - Pin A0: Volume
  - Pin A1: Glide Time

  - MIDI 5 - yellow wire
  - MIDI 4 - orange wire

*/

const int CLKPin = 9;

double currentFreq;

bool gliding = false;

int glideTime = 50; //in milliseconds. Should range from 1 - 150ms
int glidePin = A1;

int volume = 1000; //from 0-1023
int volPin = A0;

int arr[10];
Vector<int> notes(arr);

int debugLED = 7;


void setup() {
  //Set up DAC to output charge voltage
  myDAC.begin(10);

  pinMode(LED_BUILTIN, OUTPUT); // for debugging

  //Set up pin 9 for variable frequency clock signal
  pinMode(9, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) ; // phase and frequency correct mode. NON-inverted mode
  TCCR1B = _BV(WGM13) | _BV(CS11); // Select mode 8 and select divide by 8 on main clock.

  //Set up MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  pinMode(debugLED, OUTPUT); // for debugging
}

void loop() {
    MIDI.read();
  if (notes.size() == 0) {
    setClockFreq(0);
  }
  
}

//___________________________________________________________

void handleNoteOn(byte channel, byte pitch, byte velocity) {
  //First, set glide time and volume
  glideTime = 1 + analogRead(glidePin) * 150.0 / 1024.0; //should then range from 1-150
  volume = analogRead(volPin);
  
  if (velocity > 0) { //if note on
    notes.push_back(pitch); //add note to vector
    glideToFreq(currentFreq, findFreq(notes.back()));
    currentFreq = findFreq(notes.back());
  } else { //if note off
    for (int i = 0; i < notes.size(); i++) {
      if (notes.at(i) == pitch) {
        notes.remove(i); //remove it
      }
    }
    if (notes.size() > 0) {
      glideToFreq(currentFreq, findFreq(notes.back()));
      currentFreq = findFreq(notes.back());
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity) {
  digitalWrite(debugLED, LOW);
  for (int i = 0; i < notes.size(); i++) {
    if (notes.at(i) == pitch) {
      notes.remove(i); //remove that pitch from the notes vector
    }
  }
  if (notes.size() > 0) {
    glideToFreq(currentFreq, findFreq(notes.back()));
    currentFreq = findFreq(notes.back());
  }
}

void glideToFreq(double startFreq, double endFreq) {
  double freqDistance = abs(endFreq - startFreq);
  double glideSpeed = freqDistance / glideTime; //in hz/ms
  
  if (startFreq < endFreq) {
    gliding = true;
    while (startFreq < endFreq - glideSpeed) {
      setOscillatorFreq(startFreq);
      delay(1);
      startFreq+= glideSpeed;
    }
    setOscillatorFreq(endFreq);
    gliding = false;
  } else if (startFreq > endFreq) {
    gliding = true;
    while (startFreq > endFreq + glideSpeed) {
      setOscillatorFreq(startFreq);
      delay(1);
      startFreq-= glideSpeed;
    }
    setOscillatorFreq(endFreq);
    gliding = false;
  } else {
    //if same freq
    setOscillatorFreq(endFreq);
  }
}

void playNote(byte note) {
  double freq = pow(2, (note - 69) / 12.0) * 440.0;

  setOscillatorFreq(freq);
  currentFreq = freq;
}


void setOscillatorFreq(double freq) {
  setClockFreq(freq);
  setChargeVoltage(((double) freq / 1000.0) * (volume / 1024.0));
  currentFreq = freq;
}

void setChargeVoltage(double voltage) {
  myDAC.analogWrite(4096 * (voltage / 5.0));
}

void setClockFreq(double f) {
  ICR1 = 1000000 / f;
  OCR1B = 50;
  analogWrite(9, 50);
}

double findFreq(byte note) {
  return pow(2, (note - 69) / 12.0) * 440.0;
}
