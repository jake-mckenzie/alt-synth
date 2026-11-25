#ifndef ALT_SYNTH_H
#define ALT_SYNTH_H

#include <stdbool.h>
#include <stddef.h>

#define MIDI_NOTES 128
#define SAMPLE_RATE 44100

typedef enum { TONE_PIANO, TONE_WARM, TONE_BELL, TONE_SOFT, TONE_CHIPTUNE, TONE_COUNT } Tone;

// Mutable voice state; input changes and rendering are owned by the main thread.
typedef struct
{
    double phase[ MIDI_NOTES ];
    float level[ MIDI_NOTES ];
    bool sounding[ MIDI_NOTES ];
    bool held[ MIDI_NOTES ];
    double age[ MIDI_NOTES ];
    float volume;
    Tone tone;
    float tone_from[ TONE_COUNT ]; // preset weights at the start of the current fade
    float tone_mix;

} Synth;

// Convert a MIDI note number to equal-tempered frequency with A4 at 440 Hz.
double note_frequency( int midi );

// Write a sharp-spelled note and octave, or -- for an invalid MIDI note.
void note_name( int midi, char *text, size_t size );

// Report whether a MIDI pitch class is a black piano key.
bool note_is_black( int midi );

// Return the filter label for a valid tone, or Unknown for an invalid value.
const char *tone_name( Tone tone );

// Start a short crossfade to a different valid tone without releasing notes.
void synth_set_tone( Synth *synth, Tone tone );

// Initialize a silent synth with the default Chiptune tone and volume.
void synth_init( Synth *synth );

// Apply held notes and pedal state, restarting the attack on each new press.
void synth_notes( Synth *synth, const bool held[ MIDI_NOTES ], bool sustain );

// Immediately clear voice gates and levels while retaining tone and volume.
void synth_silence( Synth *synth );

// Fill mono float samples and advance voice phases, envelopes, and preset crossfade.
void synth_render( Synth *synth, float *buffer, size_t frames );

#endif
