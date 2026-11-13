#include "synth.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TAU 6.28318530717958647692

double note_frequency(int midi) { return 440.0 * pow(2.0, (midi - 69) / 12.0); }

void note_name(int midi, char *text, size_t size) {
    static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (midi < 0 || midi >= MIDI_NOTES) {
        snprintf(text, size, "--");
        return;
    }
    snprintf(text, size, "%s%d", names[midi % 12], midi / 12 - 1);
}

bool note_is_black(int midi) {
    int pitch = midi % 12;
    return pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10;
}

KeyRect keyboard_rect(int index, KeyRect bounds) {
    float white_width = bounds.width / 15.0f;
    int whites = 0;
    for (int i = 0; i < index; ++i)
        if (!note_is_black(i))
            ++whites;
    if (note_is_black(index)) {
        return (KeyRect){bounds.x + whites * white_width - white_width * 0.31f, bounds.y,
                         white_width * 0.62f, bounds.height * 0.62f};
    }
    return (KeyRect){bounds.x + whites * white_width, bounds.y, white_width, bounds.height};
}

int keyboard_hit(float x, float y, KeyRect bounds) {
    // Black keys overlap white keys, so they take precedence for mouse input.
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < KEY_COUNT; ++i) {
            if (note_is_black(i) != (pass == 0))
                continue;
            KeyRect r = keyboard_rect(i, bounds);
            if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height)
                return i;
        }
    }
    return -1;
}

void synth_init(Synth *synth) {
    memset(synth, 0, sizeof(*synth));
    synth->volume = 0.65f;
}

void synth_notes(Synth *synth, const bool held[MIDI_NOTES], bool sustain) {
    for (int i = 0; i < MIDI_NOTES; ++i)
        synth->sounding[i] = held[i] || (sustain && synth->sounding[i]);
}

void synth_silence(Synth *synth) {
    memset(synth->sounding, 0, sizeof(synth->sounding));
    memset(synth->level, 0, sizeof(synth->level));
}

void synth_render(Synth *synth, float *buffer, size_t frames) {
    memset(buffer, 0, frames * sizeof(*buffer));
    for (int note = 0; note < MIDI_NOTES; ++note) {
        if (!synth->sounding[note] && synth->level[note] < 0.00001f) {
            synth->level[note] = 0;
            continue;
        }
        double step = TAU * note_frequency(note) / SAMPLE_RATE;
        float target = synth->sounding[note] ? 1.0f : 0.0f;
        // Short attack and longer release prevent clicks, retaining each
        // oscillator's phase.
        float rate = synth->sounding[note] ? 0.006f : 0.0012f;
        for (size_t frame = 0; frame < frames; ++frame) {
            synth->level[note] += (target - synth->level[note]) * rate;
            buffer[frame] += (float)sin(synth->phase[note]) * synth->level[note] * 0.18f;
            synth->phase[note] += step;
            if (synth->phase[note] >= TAU)
                synth->phase[note] -= TAU;
        }
    }
    // Soft saturation bounds even large sustained chords without gain jumps on
    // note changes.
    for (size_t frame = 0; frame < frames; ++frame)
        buffer[frame] = tanhf(buffer[frame]) * synth->volume;
}
