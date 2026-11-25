#include "../src/synth.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Check piano key hit detection, overlap priority, and boundary exclusions.
static void test_keyboard( void )
{
  KeyRect bounds = { 20, 30, 900, 240 };

  // Every key is selectable at its label area, including the last F.
  for ( int i = 0; i < KEY_COUNT; ++i )
  {
    KeyRect key = keyboard_rect( i, bounds );
    assert( keyboard_hit( key.x + key.width / 2, key.y + key.height - 10, bounds ) == i );
  }

  KeyRect sharp = keyboard_rect( 1, bounds );
  assert( keyboard_hit( sharp.x + 1, sharp.y + 1, bounds ) == 1 );
  assert( keyboard_hit( 19, 50, bounds ) == -1 );
  assert( keyboard_hit( 920, 50, bounds ) == -1 );
  assert( keyboard_hit( 40, 270, bounds ) == -1 );
}

// Check all QWERTY note offsets, reserved controls, and octave-shift bounds.
static void test_qwerty_mapping( void )
{
  bool seen[ MIDI_NOTES ] = { false };
  int count = 0;

  for ( int row = 0; row < QWERTY_ROWS; ++row )
  {
    for ( int column = 0; column < QWERTY_COLUMNS; ++column )
    {
      int offset = qwerty_note_offset( row, column );

      if ( offset == NO_NOTE )
      { continue; }

      assert( offset >= QWERTY_MIN_OFFSET && offset <= QWERTY_MAX_OFFSET );
      assert( 24 + offset >= 0 && 84 + offset < MIDI_NOTES );
      assert( !seen[ 60 + offset ]);
      seen[ 60 + offset ] = true;
      ++count;
    }
  }
  assert( count == 55);
  assert( qwerty_note_offset( 2, 0 ) == NO_NOTE );  // Caps Lock is intentionally inactive.
  assert( qwerty_note_offset( 2, 1 ) == 0 );        // A remains middle C.
  assert( qwerty_note_offset( 1, 2 ) == 1 );        // W remains C sharp.
  assert( qwerty_note_offset( 3, 1 ) == -12 );      // Z extends the lower octave.
  assert( qwerty_note_offset( 0, 1 ) == 24 );       // Number keys extend the upper range.
  assert( qwerty_note_offset( 0, 11 ) == NO_NOTE ); // Volume down.
  assert( qwerty_note_offset( 0, 12 ) == NO_NOTE ); // Volume up.
  assert( qwerty_note_offset( 0, 13 ) == NO_NOTE ); // Panic.
  assert( qwerty_note_offset( 1, 0 ) == NO_NOTE );  // Filter.
  assert( qwerty_note_offset( 4, 3 ) == NO_NOTE );  // Sustain.
  assert( qwerty_note_offset( -1, 0 ) == NO_NOTE );
  assert( qwerty_note_offset( 0, QWERTY_COLUMNS ) == NO_NOTE );
}

// Check initial silence, A4 pitch, sustain release, polyphonic bounds, and panic.
static void test_sound( void )
{
  Synth synth;
  synth_init( &synth );
  bool held[ MIDI_NOTES ] = { false };
  float samples[ 4410 ];
  synth_render( &synth, samples, 4410 );

  for ( int i = 0; i < 4410; ++i )
  { assert(samples[i] == 0); }

  held[ 69 ] = true;
  synth_notes( &synth, held, false );
  synth_render( &synth, samples, 4410 );
  int crossings = 0;

  for ( int i = 1; i < 4410; ++i )
  { 
    if ( samples[ i - 1 ] <= 0 && samples[ i ] > 0)
    { ++crossings; }
  }

  assert( crossings >= 43 && crossings <= 45 ); // A4: 440 Hz over 100 ms.
  assert( synth.level[ 69 ] > .99f );
  held[ 69 ] = false;
  synth_notes( &synth, held, true );
  assert( synth.sounding[ 69 ] );
  synth_notes( &synth, held, false );
  assert( !synth.sounding[ 69 ] );
  synth_render( &synth, samples, 4410 );
  assert( fabsf( samples[ 4409 ] ) < .002f );

  // every voice must sound while the combined output remains finite and bounded
  for ( int i = 0; i < MIDI_NOTES; ++i )
  { held[i] = true; }

  synth_notes( &synth, held, false );
  synth_render( &synth, samples, 4410 );

  for ( int i = 0; i < 4410; ++i )
  { 
    assert( isfinite( samples[ i ] ) && fabsf( samples[ i ] ) <= synth.volume );
  }

  for ( int i = 0; i < MIDI_NOTES; ++i )
  {
    assert( synth.sounding[ i ] );
    assert( synth.level[ i ] > .99f );
  }

  synth_silence( &synth );
  synth_render( &synth, samples, 4410 );

  for ( int i = 0; i < 4410; ++i )
  { assert(samples[ i ] == 0); }

  for ( int i = 0; i < MIDI_NOTES; ++i )
  { assert( !synth.sounding[ i ] ); }
}

// Measure mean-square output energy for decay and silence assertions.
static double energy( const float *samples, size_t count )
{
  double total = 0;

  for ( size_t i = 0; i < count; ++i )
  { total += samples[ i ] * samples[ i ]; }

  return total / count;
}

// Check preset differences, output bounds, piano re-strikes, and tone transitions.
static void test_tones( void )
{
  float samples[ 4410 ], piano[ 4410 ];
  bool held[ MIDI_NOTES ] = { false };
  held[ 60 ] = true;

  for ( int tone = 0; tone < TONE_COUNT; ++tone )
  { 
    Synth synth;
    synth_init( &synth );
    synth_set_tone( &synth, ( Tone )tone );
    synth_notes( &synth, held, false );
    synth_render( &synth, samples, 4410 );
    assert( synth.tone_mix == 1 );
    assert( energy( samples, 4410 ) > .0001 );

    if ( tone == TONE_PIANO )
    { memcpy( piano, samples, sizeof( piano )); }
    else
    {
      double difference = 0;

      for ( int i = 0; i < 4410; ++i )
      { difference += fabs( samples[ i ] - piano[ i ] ); }

      assert( difference > 1 ); // Each preset must produce a different waveform.
    }

    // Dense chords remain bounded in every sound mode.
    bool all[ MIDI_NOTES ];

    for ( int i = 0; i < MIDI_NOTES; ++i )
    { all[ i ] = true; }

    synth_notes( &synth, all, false );
    synth_render( &synth, samples, 4410 );

    for ( int i = 0; i < 4410; ++i )
    { 
      assert( isfinite( samples[ i ] ) && fabsf( samples[ i ] ) <= synth.volume );
    }
  }

  Synth synth;
  synth_init( &synth );
  assert( synth.tone == TONE_CHIPTUNE );
  synth_set_tone( &synth, TONE_PIANO );
  synth_notes( &synth, held, false );
  synth_render( &synth, samples, 4410 );
  double attack_energy = energy( samples, 4410 );

  for ( int i = 0; i < 30; ++i )
  { synth_render( &synth, samples, 4410 ); }

  assert( energy( samples, 4410 ) < attack_energy * .3 ); // Piano notes decay while held.
  held[ 60 ] = false;
  synth_notes( &synth, held, true );
  assert( synth.sounding[ 60 ] );
  held[ 60 ] = true;
  synth_notes( &synth, held, true );
  assert( synth.age[ 60 ] == 0 ); // Pedal-held notes can be struck again.
  synth_render( &synth, samples, 4410 );
  assert( energy( samples, 4410 ) > attack_energy * .8 );
  synth_set_tone( &synth, TONE_SOFT );
  assert( synth.sounding[ 60 ] ); // Changing sound preserves held notes.
  synth_render( &synth, samples, 4410 );
  assert( synth.tone_mix == 1 );
}

// Measure harmonic content relative to the fundamental, independent of volume.
static double harmonic_ratio( const float *samples, size_t count, double frequency )
{
  double powers[ 4 ] = { 0 };

  for ( int harmonic = 1; harmonic <= 4; ++harmonic )
  {
    double real = 0, imaginary = 0;

    for ( size_t i = 0; i < count; ++i )
    {
      double phase = 6.283185307179586 * frequency * harmonic * i / SAMPLE_RATE;
      real += samples[ i ] * cos( phase );
      imaginary += samples[ i ] * sin( phase );
    }

    powers[ harmonic - 1 ] = real * real + imaginary * imaginary;
  }

  assert( powers[ 0 ] > .000001 );
  return ( powers[ 1 ] + powers[ 2 ] + powers[ 3 ]) / powers[ 0 ];
}

// Check that piano overtones decay while sustained synth harmonics remain.
static void test_piano_and_synth_character( void )
{
  double piano_tail = 0;
  for ( int mode = 0; mode < 2; ++mode )
  {
    Synth synth;
    synth_init( &synth );
    synth_set_tone( &synth, mode ? TONE_SOFT : TONE_PIANO );
    bool held[ MIDI_NOTES ] = { false };
    held[ 57 ] = true; // A3, 220 Hz: an exact number of cycles per buffer.
    synth_notes( &synth, held, false );
    float samples[ 4410 ];
    synth_render( &synth, samples, 4410 );
    double attack_harmonics = harmonic_ratio( samples, 4410, 220 );

    for ( int i = 0; i < 10; ++i )
    { synth_render( &synth, samples, 4410 ); }

    double tail_harmonics = harmonic_ratio( samples, 4410, 220 );

    if ( !mode )
    {
      assert( tail_harmonics < attack_harmonics * .1 );
      piano_tail = tail_harmonics;
    }
    else
    {
      assert( tail_harmonics > .02 );
      assert( tail_harmonics > piano_tail * 10 );
    }
  }
}

// Check the Chiptune label, square-wave harmonic content, sustain, and release.
static void test_chiptune( void )
{
  Synth synth;
  synth_init( &synth );
  synth_set_tone( &synth, TONE_CHIPTUNE );
  assert( strcmp( tone_name( synth.tone ), "Chiptune" ) == 0 );
  bool held[ MIDI_NOTES ] = { false };
  held[ 57 ] = true;
  synth_notes( &synth, held, false );
  float samples[ 4410 ];

  // Let the preset crossfade and attack settle before measuring a held note.
  for ( int i = 0; i < 10; ++i )
  { synth_render( &synth, samples, 4410 ); }

  double harmonics = harmonic_ratio( samples, 4410, 220 );
  assert( harmonics > .07 && harmonics < .15 ); // Square-wave third harmonic.
  assert( energy( samples, 4410 ) > .001 );     // A held chip note keeps sounding.
  held[ 57 ] = false;
  synth_notes( &synth, held, false );
  synth_render( &synth, samples, 4410 );
  assert( fabsf( samples[ 4409 ] ) < .001f );
}

// Run the display-independent pitch, keyboard, and synthesizer regression checks.
int main( void )
{
  char name[ 8 ];
  assert( fabs( note_frequency( 69 ) - 440 ) < .00001 );
  assert( fabs( note_frequency( 60 ) - 261.625565 ) < .00001 );
  note_name( 60, name, sizeof( name ) );
  assert( strcmp( name, "C4" ) == 0 );
  note_name( 61, name, sizeof( name ) );
  assert( strcmp( name, "C#4" ) == 0 );
  test_keyboard();
  test_qwerty_mapping();
  test_sound();
  test_tones();
  test_piano_and_synth_character();
  test_chiptune();
  puts( "All pitch, keyboard, tone, polyphony, sustain, release, and silence tests passed." );

  return 0;
}
