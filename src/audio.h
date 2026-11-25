#ifndef ALT_SYNTH_AUDIO_H
#define ALT_SYNTH_AUDIO_H

#include "synth.h"

#include <raylib.h>

#define AUDIO_FRAMES 1024

// Raylib output stream and the latest samples available to the waveform
typedef struct
{
  AudioStream stream;
  float samples[ AUDIO_FRAMES ];
  double sample_remainder; // fractional sample carried between silent updates
  bool ready;
} AudioOutput;

// initialize output, with silent waveform mode if the device or stream fails
void audio_init( AudioOutput *audio );

// advance the synth and update the latest samples, submitting audio when ready
void audio_update( AudioOutput *audio, Synth *synth );

// release resources acquired by audio_init
void audio_shutdown( AudioOutput *audio );

#endif
