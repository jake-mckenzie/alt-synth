#ifndef ALT_SYNTH_H
#define ALT_SYNTH_H

#include <stdbool.h>
#include <stddef.h>

#define MIDI_NOTES 128 // number of MIDI pitches and synth voice slots
#define SAMPLE_RATE 44100 // audio sample rate in hertz

// playable presets, with TONE_COUNT marking the end of the valid range
typedef enum { TONE_PIANO, TONE_WARM, TONE_BELL, TONE_SOFT, TONE_CHIPTUNE, TONE_COUNT } Tone;

// per-note oscillator, envelope, and gate state owned by the main thread
typedef struct
{
    double phase[ MIDI_NOTES ]; // oscillator position in radians
    float level[ MIDI_NOTES ]; // envelope level, including release tails
    bool sounding[ MIDI_NOTES ]; // gate held open by a key or sustain
    bool held[ MIDI_NOTES ]; // last key state, used to detect a new strike
    double age[ MIDI_NOTES ]; // seconds since the last strike
    float volume; // output gain from 0 to 1
    Tone tone; // selected preset and current fade target
    float tone_from[ TONE_COUNT ]; // preset weights at the start of the current fade
    float tone_mix; // progress toward the target preset from 0 to 1

} Synth;

// Convert a MIDI note number to equal-tempered frequency with A4 at 440 Hz.
double note_frequency( int midi );

// Write a sharp-spelled note and octave, or -- for an invalid MIDI note.
void note_name( int midi, char *text, size_t size );

// Report whether a MIDI pitch class is a black piano key.
bool note_is_black( int midi );

// return the display label for a valid tone, or Unknown for an invalid value
const char *tone_name( Tone tone );

// Start a short crossfade to a different valid tone without releasing notes.
void synth_set_tone( Synth *synth, Tone tone );

// Initialize a silent synth with the default Chiptune tone and volume.
void synth_init( Synth *synth );

// Apply held notes and pedal state, restarting the attack on each new press.
void synth_notes( Synth *synth, const bool held[ MIDI_NOTES ], bool sustain );

// Immediately clear voice gates and levels while retaining tone and volume.
void synth_silence( Synth *synth );

// fill frames mono samples in the caller's buffer and advance the synth state
void synth_render( Synth *synth, float *buffer, size_t frames );

#endif
