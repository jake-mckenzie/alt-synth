#include "keyboard_layout.h"

// Lay out a chromatic key within a C-to-F piano range; indices must be valid.
KeyRect keyboard_rect( int index, KeyRect bounds )
{
  float white_width = bounds.width / WHITE_KEY_COUNT;
  int whites = 0;

  for ( int i = 0; i < index; ++i )
  {
    if ( !note_is_black( i ) )
    { ++whites; }
  }

  if ( note_is_black( index ) )
  {
    return ( KeyRect ){ bounds.x + whites * white_width - white_width * 0.31f, bounds.y,
                     white_width * 0.62f, bounds.height * 0.62f };
  }

  return ( KeyRect ){ bounds.x + whites * white_width, bounds.y, white_width, bounds.height };
}

// Return the piano key under a point, prioritizing overlapping black keys.
int keyboard_hit( float x, float y, KeyRect bounds )
{
  // Black keys overlap white keys, so they take precedence for mouse input.
  for ( int pass = 0; pass < 2; ++pass )
  {
    for ( int i = 0; i < KEY_COUNT; ++i )
    {
      if ( note_is_black( i ) != ( pass == 0 ))
      { continue; }

      KeyRect r = keyboard_rect( i, bounds );

      if ( x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height )
      { return i; }
    }
  }
  return -1;
}

// Return a QWERTY key's semitone offset, or NO_NOTE for a control, disabled key, or empty cell.
int qwerty_note_offset( int row, int column )
{
  // Home-row piano bindings stay familiar. Other rows extend the playable range.
  // Tab, Space, -/=, and Backspace retain their control actions.
  static const int offsets[ QWERTY_ROWS ][ QWERTY_COLUMNS ] =
  {
    { 34, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, NO_NOTE, NO_NOTE, NO_NOTE, NO_NOTE },
    { NO_NOTE, 18, 1, 3, 19, 6, 8, 10, 20, 13, 15, 21, 22, 23, NO_NOTE },
    { NO_NOTE, 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, -23, NO_NOTE, NO_NOTE },
    { -22, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -21, NO_NOTE, NO_NOTE, NO_NOTE },
    { -20, -19, -18, NO_NOTE, -17, -16, -15, -14, NO_NOTE, NO_NOTE, NO_NOTE, NO_NOTE, NO_NOTE,
     NO_NOTE, NO_NOTE }
  };

  if ( row < 0 || row >= QWERTY_ROWS || column < 0 || column >= QWERTY_COLUMNS )
  { return NO_NOTE; }

  return offsets[ row ][ column ];
}
