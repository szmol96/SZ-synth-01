#include <vector>
#include "wavetables.hpp"

enum oscType{sine, triangle, saw, square, noise};
const unsigned int midiFrequencies[128] = {8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};

class Oscillator {
public:
    oscType type;
    unsigned char note; // needed for knowing which key triggered the oscillator
    float frequency; // the frequency the waveform is played at
    unsigned int dutyCycle; // what it says on the tin, only important for the square waveform
    float amplitudeMax; // the maximum amplitude the oscillator can reach (ranging from 0 to 1)
    float amplitude; // the amplitude the waveform is currently played at (aka. volume) (ranging from 0 to amplitudeMax) (ADSR)
    float phase; // an index for the waveform array for deciding which sample to write to buffer
    float increment; // the phase will be incremented by this value
    bool toDelete; // indicates to the Mixer class, whether the oscillator should be deleted or not

    unsigned int attack; // how long it takes the oscillator to reach its maximum amplitude
    unsigned int decay; // how long it takes the oscillator to reach its sustain amplitude
    float sustainLevel; // the sustain amplitude of the oscillator (ranging from 0 to 1, where 1 equals amplitudeMax), 0 means no sustain at all (will skip decay and sustain completely)
    float absoluteSustainLevel; // the absolute amplitude calculated from amplitudeMax and sustainLevel
    unsigned int release; // how long it takes the oscillator to reach complete silence from its sustain amplitude 
    unsigned char ADSRState; // which state of the ADSR envelope the oscillator is currently in (0: attack state, 1: decay state, 2: sustained state, 3: release state)

    unsigned char portamentoNote; // the note to slide to the current note from
    unsigned int portamento; // the amount of portamento (the time it takes to slide from one note to another)

    Oscillator(oscType initType, unsigned char initNote, unsigned int initFreq, unsigned int initDC, float initAmplitudeMax, unsigned int initAttack, unsigned int initDecay, float initSustainLevel, unsigned int initRelease, unsigned char initPortamentoNote, unsigned int initPortamento);
    ~Oscillator();

    void update(); // calculate the phase of the oscillator based on its frequency and sample rate, handle ADSR envelope
};

class Mixer {
public:
    std::vector<Oscillator*> oscillators;
    unsigned int mixedSample;

    Mixer();
    ~Mixer();

    void doYourMagic(); // updates oscillators, mixes samples and returns the final sample value to be written into I2S buffer
};