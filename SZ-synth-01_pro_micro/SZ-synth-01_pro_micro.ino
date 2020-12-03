#include <Wire.h>
#include "Adafruit_MCP23017.h"
#include "MIDIUSB.h"

#define midiSwitch 7
#define envelopeButton 5
#define pitchAxis A0
#define octaveAxis A1
#define modulationAxis A2
#define breathAxis A3
#define attackPot A6
#define decayPot A7
#define sustainPot A8
#define releasePot A9
#define dutyCPot A10

Adafruit_MCP23017 mcp0; // Address 000
Adafruit_MCP23017 mcp1; // Address 001

bool keyStates[25];
bool canSwitchOctave;
bool octaveKeyHeld = true;
unsigned long octaveMillis = 0;
bool volumeChange = false;
bool portamentoChange = false;
bool breathKeyHeld = false;
bool envelopeKeyHeld = false;
byte currentOctave = 4;
byte previousOctave = 4;
unsigned int currentVolume = 240; // this will be divided by 1000 in the ESP32 code, as the maximum amplitude is 1.0
unsigned int currentPortamento = 0;
byte currentWaveform = 0;


uint8_t expDigitalRead(uint8_t pin) { // digitalRead from expansion ICs
  if (pin < 16) {
    return mcp0.digitalRead(pin);
  } else if (pin < 32) {
    return mcp1.digitalRead(pin - 16);
  }
}

void serialNoteOn(byte noteNum) { // sends a message to the ESP32 to create an instance of the Oscillator class
  String tempString = "[N"; // note o[N]

  tempString += String(noteNum) + "]";

  Serial1.write(tempString.c_str());
}

void serialNoteOff(byte noteNum) { // sends a message to the ESP32 to delete an instance of the Oscillator class
  String tempString = "[F"; // note of[F]

  tempString += String(noteNum) + "]";

  Serial1.write(tempString.c_str());
}

void serialPitchChange(int value) { // sends a message to the ESP32 to change the frequency of all active oscillators
  String tempString = "[P"; // [P]itch

  tempString += String(value) + "]";

  Serial1.write(tempString.c_str());
}

void serialWaveformChange(int value) { // sends a message to the ESP32 to change the current waveform
  String tempString = "[W"; // [W]aveform

  tempString += String(value) + "]";

  Serial1.write(tempString.c_str());
}

void serialVolumeChange(int value) { // sends a message to the ESP32 to change the volume
  String tempString = "[V"; // [V]olume

  tempString += String(value) + "]";

  Serial1.write(tempString.c_str());
}

void serialPortamentoChange(int value) { // sends a message to the ESP32 to change the volume
  String tempString = "[O"; // portament[O]

  tempString += String(value) + "]";

  Serial1.write(tempString.c_str());
}

void serialSendADSR() {
  String tempString = "[A"; // [A]ttack
  uint16_t tempVar = uint16_t(map(analogRead(attackPot), 0, 1023, 1, 40000));

  tempString += String(tempVar) + "]";

  Serial1.write(tempString.c_str());
  delay(10);


  tempString = "[D"; // [D]ecay
  tempVar = uint16_t(map(analogRead(decayPot), 0, 1023, 1, 40000));

  tempString += String(tempVar) + "]";

  Serial1.write(tempString.c_str());
  delay(10);


  tempString = "[S"; // [S]ustain
  tempVar = uint16_t(map(analogRead(sustainPot), 0, 1023, 0, 1000));

  tempString += String(tempVar) + "]";

  Serial1.write(tempString.c_str());
  delay(10);


  tempString = "[R"; // [R]elease
  tempVar = uint16_t(map(analogRead(releasePot), 0, 1023, 1, 40000));

  tempString += String(tempVar) + "]";

  Serial1.write(tempString.c_str());
  delay(10);


  tempString = "[C"; // duty [C]ycle
  tempVar = uint16_t(map(analogRead(dutyCPot), 0, 1023, 1, 2046));

  tempString += String(tempVar) + "]";

  Serial1.write(tempString.c_str());
  delay(10);
}


void midiNoteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void midiNoteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void midiControlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void midiPitchBendChange(byte channel, int value) {
  byte lowValue = value & 0x7F;
  byte highValue = value >> 7;
  midiEventPacket_t event = {0x0E, 0xE0 | channel, lowValue, highValue};
  MidiUSB.sendMIDI(event);
}


void setup() {
  mcp0.begin(0, &Wire);
  mcp1.begin(1, &Wire);

  Serial1.begin(115200);

  delay(1000);

  pinMode(midiSwitch, INPUT_PULLUP);
  pinMode(envelopeButton, INPUT_PULLUP);
  pinMode(pitchAxis, INPUT);
  pinMode(octaveAxis, INPUT);
  pinMode(modulationAxis, INPUT);
  pinMode(breathAxis, INPUT);
  pinMode(attackPot, INPUT);
  pinMode(decayPot, INPUT);
  pinMode(sustainPot, INPUT);
  pinMode(releasePot, INPUT);
  pinMode(dutyCPot, INPUT);

  for (uint8_t i = 0; i < 25; i++) {
    keyStates[i] = false;
    if (i < 16) {
      mcp0.pinMode(i, INPUT);
      mcp0.pullUp(i, HIGH);
      mcp1.pinMode(i, INPUT);
      mcp1.pullUp(i, HIGH);
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  unsigned int pitchValue = analogRead(pitchAxis);
  unsigned int octaveValue = analogRead(octaveAxis);
  unsigned int modulationValue = analogRead(modulationAxis);
  unsigned int breathValue = analogRead(breathAxis);

  if (!digitalRead(midiSwitch)) { // if the mode is set to MIDI mode
    for (int i = 0; i < 25; i++) {
      if (expDigitalRead(i) == LOW) {
        if (keyStates[i] == false) {
          midiNoteOn(0, currentOctave * 12 + i, 64);
          keyStates[i] = true;
        }
      } else {
        if (keyStates[i] == true) {
          midiNoteOff(0, currentOctave * 12 + i, 64);
          keyStates[i] = false;
        }
      }
    }

    if (pitchValue < 508 || pitchValue > 516) midiPitchBendChange(0, map(pitchValue, 0, 1023, 0, 16383));
    else midiPitchBendChange(0, 8192);

    MidiUSB.flush();
  } else { // if the mode is set to synth mode (ESP32)
    if (!volumeChange && !portamentoChange) {
      for (int i = 0; i < 25; i++) { // handling key presses and releases here (note on/off)
        if (expDigitalRead(i) == LOW) {
          if (keyStates[i] == false) {
            serialNoteOn(currentOctave * 12 + i);
            keyStates[i] = true;
          }
        } else {
          if (keyStates[i] == true) {
            serialNoteOff(currentOctave * 12 + i);
            keyStates[i] = false;
          }
        }
      }

      if (pitchValue < 488 || pitchValue > 536) {
        serialPitchChange(pitchValue - 512);
        delay(2);
      }

      if (breathValue < 256 && !breathKeyHeld && currentWaveform > 0) {
        currentWaveform--;
        serialWaveformChange(currentWaveform);
      }
      if (breathValue > 768 && !breathKeyHeld && currentWaveform < 3) {
        currentWaveform++;
        serialWaveformChange(currentWaveform);
      }

      if (!digitalRead(envelopeButton) && !envelopeKeyHeld) serialSendADSR(); // ADSR and duty cycle settings will be sent only on button press to relieve the CPU from unnecessary analogRead calls
    }
  }

  if (octaveValue < 256) { // right stick is moved to the far left
    if (!octaveKeyHeld) { // execute this only once until the stick is moved back to the middle
      canSwitchOctave = true;

      for (int i = 0; i < 25; i++) {
        if (keyStates[i] == true) { // if any of the note keys are held...
          canSwitchOctave = false; // ...the device won't switch octaves
          break;
        }
      }

      if (canSwitchOctave && currentOctave > 0) {
        previousOctave = currentOctave; // the currently selected octave gets saved into another variable
        currentOctave--;
        octaveMillis = currentMillis;
      }
    } else if (currentMillis - octaveMillis > 1000) { // if the octave stick is pushed to the left for more than one second
      currentOctave = previousOctave; // the selected octave gets reverted back to the previous one
      portamentoChange = true;

      if (expDigitalRead(0) == LOW && currentPortamento >= 100) { // decreasing portamento
        if (keyStates[0] == false) {
          currentPortamento -= 100;
          serialPortamentoChange(currentPortamento);
          keyStates[0] = true;
        }
      } else {
        if (keyStates[0] == true) keyStates[0] = false;
      }

      if (expDigitalRead(2) == LOW) { // increasing portamento
        if (keyStates[2] == false) {
          currentPortamento += 100;
          serialPortamentoChange(currentPortamento);
          keyStates[2] = true;
        }
      } else {
        if (keyStates[2] == true) keyStates[2] = false;
      }
    }
  } else portamentoChange = false;
  if (octaveValue > 768) { // right stick is moved to the far right
    if (!octaveKeyHeld) { // execute this only once until the stick is moved back to the middle
      canSwitchOctave = true;

      for (int i = 0; i < 25; i++) {
        if (keyStates[i] == true) { // if any of the note keys are held...
          canSwitchOctave = false; // ...the device won't switch octaves
          break;
        }
      }

      if (canSwitchOctave && currentOctave < 7) {
        previousOctave = currentOctave; // the currently selected octave gets saved into another variable
        currentOctave++;
        octaveMillis = currentMillis;
      }
    } else if (currentMillis - octaveMillis > 1000) { // if the octave stick is pushed to the left for more than one second
      currentOctave = previousOctave; // the selected octave gets reverted back to the previous one
      volumeChange = true;

      if (expDigitalRead(0) == LOW && currentVolume >= 20) { // decreasing volume
        if (keyStates[0] == false) {
          currentVolume -= 20;
          serialVolumeChange(currentVolume);
          keyStates[0] = true;
        }
      } else {
        if (keyStates[0] == true) keyStates[0] = false;
      }

      if (expDigitalRead(2) == LOW && currentVolume < 1000) { // increasing volume
        if (keyStates[2] == false) {
          currentVolume += 20;
          serialVolumeChange(currentVolume);
          keyStates[2] = true;
        }
      } else {
        if (keyStates[2] == true) keyStates[2] = false;
      }
    }
  } else volumeChange = false;

  octaveKeyHeld = (octaveValue < 256) || (octaveValue > 768);
  breathKeyHeld = (breathValue < 256) || (breathValue > 768);
  envelopeKeyHeld = !digitalRead(envelopeButton);
}
