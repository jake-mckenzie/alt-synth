#ifndef ALT_SYNTH_UI_STYLE_H
#define ALT_SYNTH_UI_STYLE_H

#include <raylib.h>

extern const Color background;
extern const Color border;
extern const Color text_color;
extern const Color muted;
extern const Color accent;
extern const Color signal_color;

// center a label using the shared UI font metrics
void centered( const char *label, Rectangle bounds, float size, Color color );

#endif
