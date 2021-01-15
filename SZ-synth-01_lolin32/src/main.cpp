#include <Arduino.h>
#include "driver/i2s.h"
#include "driver/dac.h"
#include "freertos/queue.h"
#include <HardwareSerial.h>
#include "oscillator.hpp"

#define DEBUG

oscType gWaveform = sine;
float gVolume = 0.24; // the peak volume or amplitude of any oscillator created (maximum is 1)
unsigned int envelopeA = 1;
unsigned int envelopeD = 1;
unsigned int envelopeS = 1000;
unsigned int envelopeR = 1;
unsigned int gDutyCycle = 1023; // only relevant for the square waveform
unsigned int gPortamento = 1; // the amount of portamento
unsigned int lastNote = 10; // needed to be known to have portamento


TaskHandle_t mixerHandle;

i2s_port_t i2s_num = I2S_NUM_0; // i2s port number
size_t bytesWritten = 0;

HardwareSerial serialReader(2); // serial connection with Pro Micro

/*static const i2s_config_t i2s_config = { // internal DAC config
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 44100,
    .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT, // the DAC module will only take the 8bits from MSB
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 64
};*/ // internal DAC config end

static const i2s_config_t i2s_config = { // external DAC config
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0, // default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
};

static const i2s_pin_config_t pin_config = {
    .bck_io_num = 26,
    .ws_io_num = 25,
    .data_out_num = 22,
    .data_in_num = I2S_PIN_NO_CHANGE
}; // external DAC config end

Mixer masterMix;

void runMixer(void * pvParameters) {
    while (true) {
        masterMix.doYourMagic();
        i2s_write(i2s_num, &masterMix.mixedSample, sizeof(unsigned int), &bytesWritten, portMAX_DELAY);
    } 
}

void setup() {
    #ifdef DEBUG
    Serial.begin(115200); // USB serial, only for debugging
    #endif

    serialReader.begin(115200, SERIAL_8N1, 33, 32); // hardware serial for control by keyboard component (Pro Micro)

    i2s_driver_install(i2s_num, &i2s_config, 0, NULL); // install and start i2s driver
    //i2s_set_pin(i2s_num, NULL); // for internal DAC, this will enable both of the internal channels
    i2s_set_pin(i2s_num, &pin_config); // for extrenal DAC
    i2s_set_sample_rates(i2s_num, 44100);
    // i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN); // enable only right channel, mapped to GPIO25
    i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE); // disable output to internal DACs

    xTaskCreatePinnedToCore(runMixer, "Mixer", 10000, NULL, 1, &mixerHandle, 0);

    randomSeed(5);
}

void loop() {
    String inputString = "";

    while (serialReader.available() >= 1) {
        #ifdef DEBUG
        char tempChar = serialReader.read();
        Serial.println(tempChar);
        inputString += tempChar;
        #else
        inputString += serialReader.read();
        #endif
    }

    if (inputString[0] == '[' && inputString[inputString.length() - 1] == ']') {
        if (inputString[1] == 'N') { // note on command
            uint8_t inNote = inputString.substring(2, inputString.length()).toInt();

            masterMix.oscillators.push_back(new Oscillator(gWaveform, inNote, midiFrequencies[inNote], gDutyCycle, gVolume, envelopeA, envelopeD, (float)(envelopeS) / (float)(1000), envelopeR, lastNote, gPortamento));
            lastNote = inNote; // this note will be the portamento note for the next one
        } else if (inputString[1] == 'F') { // note off command
            uint8_t inNote = inputString.substring(2, inputString.length()).toInt();

            for (int i = 0; i < masterMix.oscillators.size(); i++) {
                if (masterMix.oscillators[i]->note == inNote && masterMix.oscillators[i]->sustainLevel > 0) masterMix.oscillators[i]->ADSRState = 3; // only try to force the oscillator into the release state when it has sustain
            }
        } else if (inputString[1] == 'P') { // pitch change command
            int inChange = inputString.substring(2, inputString.length()).toInt();

            for (int i = 0; i < masterMix.oscillators.size(); i++) {
                masterMix.oscillators[i]->frequency = midiFrequencies[masterMix.oscillators[i]->note] + map(inChange, -512, 511, midiFrequencies[masterMix.oscillators[i]->note - 1] - midiFrequencies[masterMix.oscillators[i]->note], midiFrequencies[masterMix.oscillators[i]->note + 1] - midiFrequencies[masterMix.oscillators[i]->note]);
            }
        } else if (inputString[1] == 'A') { // attack change command
            envelopeA = inputString.substring(2, inputString.length()).toInt();
        } else if (inputString[1] == 'D') { // decay change command
            envelopeD = inputString.substring(2, inputString.length()).toInt();
        } else if (inputString[1] == 'S') { // sustain change command
            envelopeS = inputString.substring(2, inputString.length()).toInt();
        } else if (inputString[1] == 'R') { // release change command
            envelopeR = inputString.substring(2, inputString.length()).toInt();
        } else if (inputString[1] == 'W') { // waveform change command
            gWaveform = static_cast<oscType>(inputString.substring(2, inputString.length()).toInt());
        } else if (inputString[1] == 'C') { // duty cycle change command
            gDutyCycle = inputString.substring(2, inputString.length()).toInt();
        } else if (inputString[1] == 'O') { // portamento change command
            gPortamento = inputString.substring(2, inputString.length()).toInt();
            if (gPortamento < 1) gPortamento = 1; // cannot be less than 1
        } else if (inputString[1] == 'V') { // volume change command
            gVolume = (float)(inputString.substring(2, inputString.length()).toInt()) / (float)(1000);
            for (int i = 0; i < masterMix.oscillators.size(); i++) {
                masterMix.oscillators[i]->toDelete = true; // delete all active oscillators
            }
            masterMix.oscillators.push_back(new Oscillator(triangle, 65, midiFrequencies[65], 1023, gVolume, 5000, 100, 0, 5000, 65, 0)); // trigger a non-sustained note to test the volume
        }
    }
}