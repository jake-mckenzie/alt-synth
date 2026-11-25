#include "audio.h"

#include <math.h>
#include <string.h>

// initialize output and start the mono stream if the device and stream are ready
void audio_init( AudioOutput *audio )
{
  *audio = ( AudioOutput ){ 0 };
  InitAudioDevice();
  if ( !IsAudioDeviceReady() )
  { return; }

  SetAudioStreamBufferSizeDefault( AUDIO_FRAMES );
  audio->stream = LoadAudioStream( SAMPLE_RATE, 32, 1 );
  audio->ready = IsAudioStreamValid( audio->stream );
  if ( audio->ready )
  { PlayAudioStream( audio->stream ); }
}

// advance the silent synth by elapsed time and retain its latest waveform samples
static void update_silent_waveform( AudioOutput *audio, Synth *synth )
{
  float *samples = audio->samples;
  // bound catch-up work after the window has been paused
  double elapsed_samples = fmin( .25, fmax( 0, GetFrameTime() )) * SAMPLE_RATE + audio->sample_remainder;
  size_t frames = ( size_t )elapsed_samples;
  audio->sample_remainder = elapsed_samples - frames;

  if ( frames == 0 )
  { return; }

  while ( frames > AUDIO_FRAMES )
  {
    size_t excess = frames - AUDIO_FRAMES;
    size_t chunk = excess < AUDIO_FRAMES ? excess : AUDIO_FRAMES;
    synth_render( synth, samples, chunk );
    frames -= chunk;
  }

  size_t retained = AUDIO_FRAMES - frames;
  memmove( samples, samples + frames, retained * sizeof( *samples ));
  synth_render( synth, samples + retained, frames );
}

// render requested audio buffers, or update the waveform when audio is unavailable
void audio_update( AudioOutput *audio, Synth *synth )
{
  if ( !audio->ready )
  {
    update_silent_waveform( audio, synth );
    return;
  }

  while ( IsAudioStreamProcessed( audio->stream ) )
  {
    synth_render( synth, audio->samples, AUDIO_FRAMES );
    UpdateAudioStream( audio->stream, audio->samples, AUDIO_FRAMES );
  }
}

// stop and unload the stream before closing any initialized audio device
void audio_shutdown( AudioOutput *audio )
{
  if ( audio->ready )
  {
    StopAudioStream( audio->stream );
    UnloadAudioStream( audio->stream );
    audio->ready = false;
  }

  if ( IsAudioDeviceReady() )
  { CloseAudioDevice(); }
}
