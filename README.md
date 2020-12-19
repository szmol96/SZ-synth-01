# SZ-synth-01

## What is it?
It's a simple chiptune synthesizer using table-lookup synthesis. It also doubles as a USB MIDI keyboard.
The project is based on a cheap Pro Micro and a WeMos LoLin32 board.

## What are the current features?
* 25 note buttons
* Synth/MIDI mode switch
* 4 waveforms: sine, triangle, sawtooth, pulse (square)
  * Adjustable duty cycle for the pulse waveform
* Adjustable attack, decay, sustain level, and release
  * Skipping the decay and sustain phases when sustain is zero
* Pitch bending (one semitone range)

## What are the possible future features? Things to be fixed?
* Stereo sound output
* Pitch modulation
* Portamento
* Noise waveform (mainly for producing percussion sounds)
* Being able to produce more complex waveforms (modulating one wave by another, filters and stuff?)
* Getting rid of the crackling that occurs sometimes on note triggers

## Things you need to put this together:
* 1 x Pro Micro (preferably a 3.3V one, it will make things a bit easier)
* 1 x WeMos LoLin32 (almost any other ESP32 development board will do, I just happened to use this one)
* 2 x MCP23017 I/O expansion ICs (for the buttons)
* 1 x PCM5102 I2S DAC (one of those purple ones with a 3.5mm jack)
* [OPTIONAL] 1 x TXB0104 logic level shifter (only if you have development boards with different logic voltages)
* some perfboard (the larger piece(s), the better; it depends on your design)
* 26-30 x momentary switches aka. buttons (I suggest using those gray colored silent silicone ones, but any will do)
* 5 x 1kΩ potentiometers (for the knobs, can be up to 10kΩ i guess)
* 2 x 2-axis analog sticks (for pitch bending, modulation and stuff; can be replaced with more potentiometers)
* 1 x female Micro USB breakout board
* 2 x Micro USB data cables (they will be cut to short pieces, you can get away with cheaper ones)
* an enclosure that will best fit your design
* the source code for both development boards, of course!
