/* Egoboo - TTF_Font.h
 * True-type font drawing functionality.  Uses Freetype 2 & OpenGL
 * to do it's business.
 */

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#pragma once

#include "ogl_texture.h"

#include <SDL.h>

#define NUMFONTX                        16          // Number of fonts in the bitmap
#define NUMFONTY                        6           //
#define NUMFONT                         (NUMFONTX*NUMFONTY)
#define FONTADD                         4           // Gap between letters

struct bmfont_t
{
  GLtexture tex;                    // ogl texture
  int       offset;                 // Line up fonts from top of screen
  SDL_Rect  rect[NUMFONT];          // The font rectangles
  Uint8     spacing_x[NUMFONT];     // The spacing stuff
  Uint8     spacing_y;              //
  Uint8     ascii_table[256];       // Conversion table
};
typedef struct bmfont_t BMFont;

extern BMFont bmfont;

#define FNT_NUM_FONT_CHARACTERS 94
#define FNT_SMALL_FONT_SIZE 12
#define FNT_NORMAL_FONT_SIZE 16
#define FNT_LARGE_FONT_SIZE 20
#define FNT_MAX_FONTS 8

typedef struct ttfont_t TTFont;

extern struct ttfont_t *fnt_loadFont( const char *fileName, int pointSize );
extern bool_t  fnt_freeFont( struct ttfont_t *font );

extern void  fnt_drawText( struct ttfont_t *font, int x, int y, const char *text );
extern void  fnt_drawTextFormatted( struct ttfont_t *font, int x, int y, const char *format, ... );
extern void  fnt_drawTextBox( struct ttfont_t *font, const char *text, int x, int y, int width, int height, int spacing );

// Only works properly on a single line of text
extern void  fnt_getTextSize( struct ttfont_t *font, const char *text, int *width, int *height );

// Works for multiple-line strings, using the user-supplied spacing
extern void  fnt_getTextBoxSize( struct ttfont_t *font, const char *text, int spacing, int *width, int *height );
