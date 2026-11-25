#include "synth.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TAU 6.28318530717958647692

// Convert a MIDI note number to equal-tempered frequency with A4 at 440 Hz.
double note_frequency( int midi ) { return 440.0 * pow( 2.0, ( midi - 69 ) / 12.0 ); }

// Write a sharp-spelled note and octave, or -- for an invalid MIDI note.
void note_name( int midi, char *text, size_t size )
{
  static const char *names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

  if ( midi < 0 || midi >= MIDI_NOTES )
  {
    snprintf( text, size, "--" );
    return;
  }
  snprintf( text, size, "%s%d", names[ midi % 12 ], midi / 12 - 1 );
}

// Report whether a MIDI pitch class is a black piano key.
bool note_is_black( int midi )
{
  int pitch = midi % 12;
  return pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10;
}

// Return the filter label for a valid tone, or Unknown for an invalid value.
const char *tone_name( Tone tone )
{
  static const char *names[ TONE_COUNT ] = { "Piano", "Warm keys", "Bell", "Soft synth", "Chiptune" };
  return tone >= 0 && tone < TONE_COUNT ? names[ tone ] : "Unknown";
}

// Start a short crossfade to a different valid tone without releasing notes.
void synth_set_tone( Synth *synth, Tone tone )
{
  if ( tone < 0 || tone >= TONE_COUNT || tone == synth->tone )
  { return; }

  synth->previous_tone = synth->tone;
  synth->tone = tone;
  synth->tone_mix = 0;
}

// Initialize a silent synth with the default Chiptune tone and volume.
void synth_init( Synth *synth )
{
  memset( synth, 0, sizeof( *synth ));
  synth->volume = 0.65f;
  synth->tone = TONE_CHIPTUNE;
  synth->previous_tone = TONE_CHIPTUNE;
  synth->tone_mix = 1;
}

// Apply held notes and pedal state, restarting the attack on each new press.
void synth_notes( Synth *synth, const bool held[ MIDI_NOTES ], bool sustain )
{
  for ( int i = 0; i < MIDI_NOTES; ++i )
  {
    // Re-strike the string even if the sustain pedal still holds this note.
    if ( held[ i ] && !synth->held[ i ] )
    { synth->age[ i ] = 0; }

    synth->sounding[ i ] = held[ i ] || ( sustain && synth->sounding[ i ] );
    synth->held[ i ] = held[ i ];
  }
}

// Immediately clear voice gates and levels while retaining tone and volume.
void synth_silence( Synth *synth )
{
  memset( synth->sounding, 0, sizeof( synth->sounding ));
  memset( synth->level, 0, sizeof( synth->level ));
  memset( synth->held, 0, sizeof( synth->held ));
}

// Additive partials model struck strings and sustained synthesizer oscillators.
// Omit partials above Nyquist, including at the highest keyboard transpositions.
static double partial( double phase, double frequency, double ratio )
{ return frequency * ratio < SAMPLE_RATE * .45 ? sin( phase * ratio ) : 0; }

// Evaluate one preset oscillator and its age-dependent timbre at the given phase.
static double voice( Tone tone, double phase, double age, double frequency )
{
  double fundamental = sin( phase );
  switch ( tone )
  {
    case TONE_PIANO:
    {
      // A hammer excites a broad harmonic spectrum; upper partials die first.
      // Two decay rates leave a quiet string tail without an organ-like sustain floor.
      double damping = pow( frequency / 261.625565, .3 );
      double body = .78 * exp( -age * 1.8 * damping ) + .22 * exp( -age * .45 * damping );
      double sample = .7 * fundamental;
      static const double weights[] = { 0, 0, .42, .25, .15, .10, .07 };

      for ( int harmonic = 2; harmonic <= 6; ++harmonic )
      { 
        sample += weights[harmonic] * partial(phase, frequency, harmonic) *
          exp(-age * (1.0 + harmonic * 1.4) * damping);
      }

      double hammer = .08 * (partial(phase, frequency, 8) +
          partial(phase, frequency, 9)) * exp(-age * 90);

      return sample * body + hammer;
    }
    case TONE_WARM:
    {
      double decay = exp( -age * 1.6 );
      return ( .9 * fundamental + .1 * decay * partial( phase, frequency, 2 )) * ( .4 + .6 * decay );
    }
    case TONE_BELL:
    { 
      return ( .65 * fundamental + .35 * partial( phase, frequency, 3 ) * exp( -age * 2 ) +
              .18 * partial( phase, frequency, 7 ) * exp( -age * 5.0 )) * exp( -age * 1.1 );
    }
    case TONE_SOFT:
    {
      // Rounded saw waves, with opposite detuning for slow beating on held notes.
      double sample = 0, rolloff = 1;
      for ( int harmonic = 1; harmonic <= 10; ++harmonic )
      {
        double detune = .0025;

        if ( frequency * harmonic * ( 1 + detune ) >= SAMPLE_RATE * .45 )
        { break; }

        double drift = TAU * age * frequency * harmonic * detune;
        double weight = rolloff / harmonic;
        rolloff *= .78;
        sample += weight * .5 * ( sin( phase * harmonic + drift ) + sin( phase * harmonic - drift ));
      }

      return sample * .85 * ( 1 - exp( -age * 35 ));
    }
    case TONE_CHIPTUNE:
    {
      // A classic 50% pulse wave. Sum only odd harmonics below Nyquist instead
      // of switching samples abruptly, which would alias at high pitches.
      double pulse = 0;

      for ( int harmonic = 1; harmonic <= 31; harmonic += 2 )
      {
        if ( frequency * harmonic >= SAMPLE_RATE * .45 )
        { break; }

        pulse += sin( phase * harmonic ) / harmonic;
      }

      return pulse * ( 4.0 / ( TAU / 2 )) * .8 * ( .65 + .35 * exp( -age * 16 ));
    }
    default:
    {
      return 0;
    }
  }
}

// Fill mono float samples and advance voice phases, envelopes, and preset crossfade.
void synth_render( Synth *synth, float *buffer, size_t frames )
{
  memset( buffer, 0, frames * sizeof( *buffer ));
  float initial_mix = synth->tone_mix;
  for ( int note = 0; note < MIDI_NOTES; ++note )
  {
    if ( !synth->sounding[ note ] && synth->level[ note ] < 0.00001f)
    {
      synth->level[ note ] = 0;
      continue;
    }

    double frequency = note_frequency( note );
    double step = TAU * frequency / SAMPLE_RATE;
    float target = synth->sounding[ note ] ? 1.0f : 0.0f;
    float rate = synth->sounding[ note ] ? 0.006f : 
      ( synth->tone == TONE_SOFT ? 0.0003f : 0.0012f );
    float mix = initial_mix;

    for ( size_t frame = 0; frame < frames; ++frame )
    {
      synth->level[ note ] += ( target - synth->level[ note ]) * rate;
      mix = fminf( 1, mix + 1.0f / ( SAMPLE_RATE * .025f ));
      double sample = voice( synth->tone, synth->phase[ note ], synth->age[ note ], frequency );

      if ( mix < 1 )
      {
        double previous = voice( synth->previous_tone, synth->phase[ note ],
            synth->age[ note ], frequency );
        sample = previous + mix * ( sample - previous );
      }

      buffer[ frame ] += ( float )sample * synth->level[ note ] * .22f;
      synth->phase[ note ] += step;
      synth->age[ note ] += 1.0 / SAMPLE_RATE;

      if ( synth->phase[ note ] >= TAU )
      { 
        synth->phase[ note ] -= TAU;
      }
    }
  }

  synth->tone_mix = fminf( 1, initial_mix + ( float )frames / ( SAMPLE_RATE * .025f ));

  for ( size_t frame = 0; frame < frames; ++frame )
  {
    buffer[ frame ] = tanhf( buffer[ frame ]) * synth->volume;
  }
}
