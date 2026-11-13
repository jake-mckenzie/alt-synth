#include "font_data.h"
#include "raylib.h"
#include "synth.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_FRAMES 1024

static const int bindings[KEY_COUNT] = {KEY_Z, KEY_S,     KEY_X, KEY_D, KEY_C,    KEY_V, KEY_G,
                                        KEY_B, KEY_H,     KEY_N, KEY_J, KEY_M,    KEY_Q, KEY_TWO,
                                        KEY_W, KEY_THREE, KEY_E, KEY_R, KEY_FIVE, KEY_T, KEY_SIX,
                                        KEY_Y, KEY_SEVEN, KEY_U, KEY_I};
static const char *labels[KEY_COUNT] = {"Z", "S", "X", "D", "C", "V", "G", "B", "H",
                                        "N", "J", "M", "Q", "2", "W", "3", "E", "R",
                                        "5", "T", "6", "Y", "7", "U", "I"};
static const Color background = {17, 19, 23, 255};
static const Color panel_color = {25, 28, 33, 255};
static const Color border = {54, 59, 66, 255};
static const Color text_color = {228, 230, 224, 255};
static const Color muted = {147, 154, 160, 255};
static const Color accent = {255, 182, 77, 255};
static const Color signal_color = {120, 218, 191, 255};
static Font font;

static void text(const char *label, float x, float y, float size, Color color) {
    DrawTextEx(font, label, (Vector2){x, y}, size, 1, color);
}
static void centered(const char *label, Rectangle bounds, float size, Color color) {
    Vector2 extent = MeasureTextEx(font, label, size, 1);
    text(label, bounds.x + (bounds.width - extent.x) / 2, bounds.y + (bounds.height - extent.y) / 2,
         size, color);
}
static void panel(Rectangle r, const char *title) {
    DrawRectangleRounded(r, 0.035f, 8, panel_color);
    DrawRectangleRoundedLinesEx(r, 0.035f, 8, 1, border);
    text(title, r.x + 20, r.y + 15, 18, muted);
}
static Rectangle rectangle(KeyRect r) { return (Rectangle){r.x, r.y, r.width, r.height}; }

static void draw_keyboard(KeyRect bounds, int base, const bool held[MIDI_NOTES],
                          const Synth *synth) {
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < KEY_COUNT; ++i) {
            bool black = note_is_black(i);
            if (black != (pass == 1))
                continue;
            Rectangle r = rectangle(keyboard_rect(i, bounds));
            int note = base + i;
            bool latched = synth->sounding[note];
            Color fill = black ? (Color){30, 33, 39, 255} : (Color){224, 225, 217, 255};
            Color ink = black ? text_color : background;
            if (held[note])
                fill = accent;
            else if (latched)
                fill = signal_color;
            if (held[note] || latched)
                ink = background;
            DrawRectangleRec(r, fill);
            DrawRectangleLinesEx(r, black ? 2 : 1, background);
            if (black) {
                DrawRectangle((int)(r.x + 4), (int)(r.y + 4), (int)(r.width - 8), 3,
                              Fade(WHITE, .12f));
            }
            centered(labels[i], (Rectangle){r.x, r.y + r.height - 64, r.width, 26}, 20, ink);
            char name[8];
            note_name(note, name, sizeof(name));
            centered(name, (Rectangle){r.x, r.y + r.height - 33, r.width, 22}, 14, Fade(ink, .7f));
            if (held[note])
                DrawRectangle((int)(r.x + 5), (int)(r.y + r.height - 6), (int)(r.width - 10), 3,
                              ink);
        }
    }
}

static void draw_wave(Rectangle r, const float *samples) {
    panel(r, "OUTPUT / WAVEFORM");
    Rectangle plot = {r.x + 20, r.y + 52, r.width - 40, r.height - 80};
    for (int i = 0; i <= 4; ++i) {
        float y = plot.y + plot.height * i / 4;
        DrawLineEx((Vector2){plot.x, y}, (Vector2){plot.x + plot.width, y}, 1, border);
    }
    for (int i = 1; i < AUDIO_FRAMES; ++i) {
        Vector2 a = {plot.x + plot.width * (i - 1) / (AUDIO_FRAMES - 1),
                     plot.y + plot.height * (.5f - .45f * samples[i - 1])};
        Vector2 b = {plot.x + plot.width * i / (AUDIO_FRAMES - 1),
                     plot.y + plot.height * (.5f - .45f * samples[i])};
        DrawLineEx(a, b, 1.5f, signal_color);
    }
    text("0 ms", plot.x, r.y + r.height - 24, 13, muted);
    text("23.2 ms", plot.x + plot.width - 70, r.y + r.height - 24, 13, muted);
}

static void draw_pitches(Rectangle r, const Synth *synth) {
    panel(r, "VOICES / PITCH LEVELS");
    Rectangle plot = {r.x + 24, r.y + 52, r.width - 48, r.height - 80};
    float bottom = plot.y + plot.height;
    DrawLineEx((Vector2){plot.x, bottom}, (Vector2){plot.x + plot.width, bottom}, 1, border);
    // Fixed MIDI axis: the bars show oscillator envelopes, rather than an FFT
    // estimate.
    for (int note = 24; note <= 108; ++note) {
        float x = plot.x + plot.width * (note - 24) / 84;
        if (note % 12 == 0) {
            DrawLineEx((Vector2){x, plot.y}, (Vector2){x, bottom}, 1, border);
            char label[8];
            note_name(note, label, sizeof(label));
            text(label, x - 9, bottom + 6, 12, muted);
        }
        if (synth->level[note] < .001f)
            continue;
        float y = bottom - plot.height * synth->level[note];
        DrawLineEx((Vector2){x, bottom}, (Vector2){x, y}, 4, accent);
        DrawCircleV((Vector2){x, y}, 3, signal_color);
    }
}

static void draw_help(int width, int height) {
    DrawRectangle(0, 0, width, height, Fade(BLACK, .75f));
    Rectangle box = {(width - 760) / 2.0f, (height - 380) / 2.0f, 760, 380};
    panel(box, "PLAYING alt-synth");
    const char *lines[] = {"Lower octave: Z S X D C V G B H N J M",
                           "Upper octave: Q 2 W 3 E R 5 T 6 Y 7 U I",
                           "Hold multiple keys to play chords. Release to stop.",
                           "Click or drag across the piano to play with the mouse.",
                           "Space: sustain pedal     Left / right: shift octave",
                           "- / =: volume           Backspace: silence all notes",
                           "F1: help                Esc: close help / quit"};
    for (int i = 0; i < 7; ++i)
        text(lines[i], box.x + 24, box.y + 65 + i * 39, 18, text_color);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            puts("alt-synth: a polyphonic piano keyboard\n"
                 "Lower octave: Z S X D C V G B H N J M\n"
                 "Upper octave: Q 2 W 3 E R 5 T 6 Y 7 U I\n"
                 "Space sustain | arrows octave | -/= volume | Backspace silence | "
                 "F1 help | Esc quit");
            return 0;
        }
        fprintf(stderr, "Usage: %s [--help]\n", argv[0]);
        return 1;
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 760, "alt-synth");
    if (!IsWindowReady())
        return 1;
    SetWindowMinSize(960, 640);
    SetExitKey(KEY_NULL);
    SetTargetFPS(120);
    font = LoadFontFromMemory(".ttf", font_data, (int)sizeof(font_data), 48, NULL, 0);
    if (!IsFontValid(font)) {
        CloseWindow();
        return 1;
    }
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    Synth synth;
    synth_init(&synth);
    InitAudioDevice();
    bool audio_ready = IsAudioDeviceReady();
    AudioStream stream = {0};
    float samples[AUDIO_FRAMES] = {0};
    if (audio_ready) {
        SetAudioStreamBufferSizeDefault(AUDIO_FRAMES);
        stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
        if (!IsAudioStreamValid(stream))
            audio_ready = false;
        else
            PlayAudioStream(stream);
    }
    int base = 48;
    bool help = false, quit = false, sustain = false, blocked = false;
    while (!quit && !WindowShouldClose()) {
        int width = GetScreenWidth(), height = GetScreenHeight();
        float margin = 24, gap = 16;
        float keyboard_height = fminf(260, height * .36f);
        KeyRect keys = {margin + 16, height - keyboard_height - 48, width - 2 * (margin + 16),
                        keyboard_height};
        Rectangle keyboard_panel = {margin, keys.y - 56, width - 2 * margin, keyboard_height + 72};
        float graph_y = 128, graph_height = keyboard_panel.y - graph_y - gap;
        float graph_width = (width - 2 * margin - gap) / 2;
        bool held[MIDI_NOTES] = {false};
        if (IsKeyPressed(KEY_F1))
            help = !help;
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (help)
                help = false;
            else
                quit = true;
        }
        bool focused = IsWindowFocused() && !IsWindowMinimized() && !help;
        if (focused) {
            int old_base = base;
            if (IsKeyPressed(KEY_LEFT) && base > 24)
                base -= 12;
            if (IsKeyPressed(KEY_RIGHT) && base < 84)
                base += 12;
            if (old_base != base)
                synth_silence(&synth);
            if (IsKeyPressed(KEY_MINUS))
                synth.volume = fmaxf(0, synth.volume - .05f);
            if (IsKeyPressed(KEY_EQUAL))
                synth.volume = fminf(1, synth.volume + .05f);
            for (int i = 0; i < KEY_COUNT; ++i)
                held[base + i] = IsKeyDown(bindings[i]);
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                int key = keyboard_hit(mouse.x, mouse.y, keys);
                if (key >= 0)
                    held[base + key] = true;
            }
            sustain = IsKeyDown(KEY_SPACE);
        } else {
            blocked = true;
            synth_silence(&synth);
            sustain = false;
        }
        // Panic/focus loss suppresses existing holds until all playing controls are
        // released.
        if (focused && IsKeyPressed(KEY_BACKSPACE)) {
            blocked = true;
            synth_silence(&synth);
        }
        if (blocked) {
            bool any = IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            for (int i = 0; i < KEY_COUNT; ++i)
                any = any || IsKeyDown(bindings[i]);
            if (!any)
                blocked = false;
            memset(held, 0, sizeof(held));
            sustain = false;
        }
        synth_notes(&synth, held, sustain);
        if (audio_ready) {
            while (IsAudioStreamProcessed(stream)) {
                synth_render(&synth, samples, AUDIO_FRAMES);
                UpdateAudioStream(stream, samples, AUDIO_FRAMES);
            }
        } else
            synth_render(&synth, samples, AUDIO_FRAMES);

        BeginDrawing();
        ClearBackground(background);
        text("alt-synth", margin, 22, 36, accent);
        text("POLYPHONIC KEYBOARD", margin + 235, 34, 17, muted);
        char status[100], first[8], last[8];
        note_name(base, first, sizeof(first));
        note_name(base + KEY_COUNT - 1, last, sizeof(last));
        snprintf(status, sizeof(status), "%s - %s    VOLUME %d%%    SUSTAIN %s", first, last,
                 (int)lroundf(synth.volume * 100), sustain ? "ON" : "OFF");
        text(status, margin, 82, 18, text_color);
        if (!audio_ready)
            text("AUDIO UNAVAILABLE", width - 235, 36, 16, accent);
        else
            text("F1 HELP", width - 110, 36, 16, muted);
        draw_wave((Rectangle){margin, graph_y, graph_width, graph_height}, samples);
        draw_pitches((Rectangle){margin + graph_width + gap, graph_y, graph_width, graph_height},
                     &synth);
        panel(keyboard_panel, "HOLD KEYS TO PLAY / SPACE TO SUSTAIN");
        draw_keyboard(keys, base, held, &synth);
        text("ARROWS octave    - / = volume    BACKSPACE silence    ESC quit", margin, height - 23,
             14, muted);
        if (help)
            draw_help(width, height);
        EndDrawing();
    }
    if (audio_ready) {
        StopAudioStream(stream);
        UnloadAudioStream(stream);
    }
    if (IsAudioDeviceReady())
        CloseAudioDevice();
    UnloadFont(font);
    CloseWindow();
    return 0;
}
