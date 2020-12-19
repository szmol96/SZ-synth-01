#include "oscillator.hpp"

Oscillator::Oscillator(oscType initType, unsigned char initNote, unsigned int initFreq, unsigned int initDC, float initAmplitudeMax, unsigned int initAttack, unsigned int initDecay, float initSustainLevel, unsigned int initRelease, unsigned char initPortamentoNote, unsigned int initPortamento) {
    this->toDelete = false;
    
    if (initType < sine || initType > square) this->type = sine; // max. is 3 for now, will be 4 later (when the noise waveform is added)
    else this->type = initType;

    this->note = initNote; // needed for handling note off commands

    if (initPortamento > 1) { // the oscillator will have portamento
        this->frequency = midiFrequencies[initPortamentoNote]; // the oscillator will begin with the frequency of its portamento note
    } else { // the oscillator won't have portamento applied at all
        if (initFreq < 0 || initFreq > 22050) this->toDelete = true; // delete, it won't be audible anyway
        else this->frequency = initFreq; // the oscillator will begin with its usual frequency
    }

    this->dutyCycle = initDC;
    if (this->dutyCycle < 1) this->dutyCycle = 1;
    else if (this->dutyCycle > 2046) this->dutyCycle = 2046;
    
    if (initAmplitudeMax <= 0.0) this->toDelete = true; // it would be silent anyway
    else if (initAmplitudeMax > 1.0) this->amplitudeMax = 1.0; // limit the maximum amplitude
    else this->amplitudeMax = initAmplitudeMax;
    
    this->phase = 0;
    this->increment = 0;

    
    this->amplitude = 0; // the oscillator begins completely silent,...

    if (initAttack < 1) this->attack = 1;
    else this->attack = initAttack; // then it rises to its maximum amplitude

    if (initDecay < 1) this ->decay = 1;
    else this->decay = initDecay; // after that, it falls to...

    if (initSustainLevel < 0.0) this->sustainLevel = 0;
    else if (initSustainLevel > 1.0) this->sustainLevel = 1.0;
    else this->sustainLevel = initSustainLevel; // ...its sustained amplitude
    this->absoluteSustainLevel = this->amplitudeMax * this->sustainLevel;
    
    if (initRelease < 1) this->release = 1;
    else this->release = initRelease; // and finally it falls completely silent

    this->ADSRState = 0;

    this->portamentoNote = initPortamentoNote;
    this->portamento = initPortamento;
    this->portamentoPos = 0;
}

Oscillator::~Oscillator() {
    delete &this->type;
    delete &this->note;
    delete &this->frequency;
    delete &this->dutyCycle;
    delete &this->amplitudeMax;
    delete &this->amplitude;
    delete &this->phase;
    delete &this->increment;
    delete &this->toDelete;
    delete &this->attack;
    delete &this->decay;
    delete &this->sustainLevel;
    delete &this->absoluteSustainLevel;
    delete &this->release;
    delete &this->ADSRState;
    delete &this->portamentoNote;
    delete &this->portamento;
    delete &this->portamentoPos;
}

void Oscillator::update() {
    this->increment = (float)(frequency) * (float)(wavetableSize) / (float)(44100);
    this->phase = phase + increment;
    if (phase > wavetableSize - 1) phase = phase - (float)(wavetableSize); // phase will wrap around the wavetable

    if (this->note != this->portamentoNote && this->portamento > 1 && this->frequency != midiFrequencies[this->note]) {
        this->portamentoPos = this->portamentoPos + 1;
        this->frequency = map(this->portamentoPos, 0, this->portamento, midiFrequencies[this->portamentoNote], midiFrequencies[this->note]); // slide from the starting frequency to the target frequency
    }

    if (this->ADSRState == 0) { // attack state
        this->amplitude = this->amplitude + this->amplitudeMax / this->attack;
        if (this->amplitude >= this->amplitudeMax) {
            this->amplitude = this->amplitudeMax;
            if (this->sustainLevel > 0) this->ADSRState = this->ADSRState + 1;
            else this->ADSRState = 3; // skip decay and sustain completely
        }
    } else if (this->ADSRState == 1) { // decay state
        this->amplitude = this->amplitude - (this->amplitudeMax - this->absoluteSustainLevel) / this->decay;
        if (this->amplitude <= this->absoluteSustainLevel) {
            this->amplitude = this->absoluteSustainLevel;
            this->ADSRState = this->ADSRState + 1;
        }
    } else if (this->ADSRState == 3) { // release state
        if (this->sustainLevel > 0) this->amplitude = this->amplitude - this->absoluteSustainLevel / this->release;
        else this->amplitude = this->amplitude - this->amplitudeMax / this->release;

        if (this->amplitude <= 0) this->toDelete = true;
    }
}


Mixer::Mixer() {
    this->oscillators.reserve(8);
}

Mixer::~Mixer() {
 delete &this->oscillators;
 delete &this->mixedSample;
}

void Mixer::doYourMagic() {
    this->mixedSample = 0;

    for (int i = 0; i < this->oscillators.size(); i++) { // iterate through all the oscillators
        this->oscillators[i]->update(); // update the oscillator first

        if (this->oscillators[i]->type == square) { // the square waveform needs special treatment because of duty cycle and also it's not retrieved from lookup-tables
            this->mixedSample = this->mixedSample + (unsigned int)(((unsigned int)(oscillators[i]->phase + 0.5) > this->oscillators[i]->dutyCycle) ? 0 : 65535 * this->oscillators[i]->amplitude);
        } else {
            this->mixedSample = this->mixedSample + (unsigned int)((float)(wavetables[this->oscillators[i]->type][(unsigned int)(oscillators[i]->phase + 0.5)]) * this->oscillators[i]->amplitude); // get the wave sample from the wavetables depending on waveform and phase, then add it to the output sample
        }

        if (this->oscillators[i]->toDelete == true) {
            this->oscillators.erase(this->oscillators.begin() + i);
            continue;
        }
    }

    if (this->mixedSample > 65535) this->mixedSample = 65535; // hard clipping, an easy solution; be sure to choose oscillator amplitudes accordingly! :)
}