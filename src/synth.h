#ifndef ALT_SYNTH_H
#define ALT_SYNTH_H

#include <stdbool.h>
#include <stddef.h>

#define MIDI_NOTES 128
#define SAMPLE_RATE 44100
#define KEY_COUNT 18
#define WHITE_KEY_COUNT 11

typedef enum { TONE_PIANO, TONE_WARM, TONE_BELL, TONE_SOFT, TONE_COUNT } Tone;

typedef struct {
    double phase[MIDI_NOTES];
    float level[MIDI_NOTES];
    bool sounding[MIDI_NOTES];
    bool held[MIDI_NOTES];
    double age[MIDI_NOTES];
    float volume;
    Tone tone, previous_tone;
    float tone_mix;
} Synth;

typedef struct {
    float x, y, width, height;
} KeyRect;

double note_frequency(int midi);
void note_name(int midi, char *text, size_t size);
bool note_is_black(int midi);
KeyRect keyboard_rect(int index, KeyRect bounds);
int keyboard_hit(float x, float y, KeyRect bounds);
const char *tone_name(Tone tone);
void synth_set_tone(Synth *synth, Tone tone);
void synth_init(Synth *synth);
void synth_notes(Synth *synth, const bool held[MIDI_NOTES], bool sustain);
void synth_silence(Synth *synth);
void synth_render(Synth *synth, float *buffer, size_t frames);

#endif
