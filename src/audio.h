#ifndef ALT_SYNTH_AUDIO_H
#define ALT_SYNTH_AUDIO_H

#include "synth.h"

#include <raylib.h>

#define AUDIO_FRAMES 1024 // samples per mono audio buffer

// Raylib output stream and the latest samples available to the waveform
typedef struct
{
  AudioStream stream;
  float samples[ AUDIO_FRAMES ]; // latest samples, retained for the waveform
  double sample_remainder; // fractional sample carried between silent updates
  bool ready; // true when the device and stream can play audio
} AudioOutput;

// reset the output state and try to open a stream
// if setup fails, subsequent updates maintain the waveform without audio
void audio_init( AudioOutput *audio );

// advance the synth and update the latest samples, submitting audio when ready
void audio_update( AudioOutput *audio, Synth *synth );

// release resources acquired by audio_init
void audio_shutdown( AudioOutput *audio );

#endif
