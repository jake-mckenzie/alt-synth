#include "synth.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_keyboard(void) {
    KeyRect bounds = {20, 30, 900, 240};
    // Every key is selectable at its label area, including the last C.
    for (int i = 0; i < KEY_COUNT; ++i) {
        KeyRect key = keyboard_rect(i, bounds);
        assert(keyboard_hit(key.x + key.width / 2, key.y + key.height - 10, bounds) == i);
    }
    KeyRect sharp = keyboard_rect(1, bounds);
    assert(keyboard_hit(sharp.x + 1, sharp.y + 1, bounds) == 1);
    assert(keyboard_hit(19, 50, bounds) == -1);
    assert(keyboard_hit(920, 50, bounds) == -1);
    assert(keyboard_hit(40, 270, bounds) == -1);
}

static void test_sound(void) {
    Synth synth;
    synth_init(&synth);
    bool held[MIDI_NOTES] = {false};
    float samples[4410];
    synth_render(&synth, samples, 4410);
    for (int i = 0; i < 4410; ++i)
        assert(samples[i] == 0);
    held[69] = true;
    synth_notes(&synth, held, false);
    synth_render(&synth, samples, 4410);
    int crossings = 0;
    for (int i = 1; i < 4410; ++i)
        if (samples[i - 1] <= 0 && samples[i] > 0)
            ++crossings;
    assert(crossings >= 43 && crossings <= 45); // A4: 440 Hz over 100 ms.
    assert(synth.level[69] > .99f);
    held[69] = false;
    synth_notes(&synth, held, true);
    assert(synth.sounding[69]);
    synth_notes(&synth, held, false);
    assert(!synth.sounding[69]);
    synth_render(&synth, samples, 4410);
    assert(fabsf(samples[4409]) < .002f);
    // All voices at once must still produce finite, bounded samples.
    for (int i = 0; i < MIDI_NOTES; ++i)
        held[i] = true;
    synth_notes(&synth, held, false);
    synth_render(&synth, samples, 4410);
    for (int i = 0; i < 4410; ++i)
        assert(isfinite(samples[i]) && fabsf(samples[i]) <= synth.volume);
    synth_silence(&synth);
    synth_render(&synth, samples, 4410);
    for (int i = 0; i < 4410; ++i)
        assert(samples[i] == 0);
    for (int i = 0; i < MIDI_NOTES; ++i)
        assert(!synth.sounding[i]);
}

int main(void) {
    char name[8];
    assert(fabs(note_frequency(69) - 440) < .00001);
    assert(fabs(note_frequency(60) - 261.625565) < .00001);
    note_name(60, name, sizeof(name));
    assert(strcmp(name, "C4") == 0);
    note_name(61, name, sizeof(name));
    assert(strcmp(name, "C#4") == 0);
    test_keyboard();
    test_sound();
    puts("All pitch, keyboard, polyphony, sustain, release, and silence tests "
         "passed.");
    return 0;
}
