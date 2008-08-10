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

struct sBMFont
{
  egoboo_key_t ekey;

  GLtexture tex;                    // ogl texture
  int       offset;                 // Line up fonts from top of screen
  SDL_Rect  rect[NUMFONT];          // The font rectangles
  Uint8     spacing_x[NUMFONT];     // The spacing stuff
  Uint8     spacing_y;              //
  Uint8     ascii_table[256];       // Conversion table
};
typedef struct sBMFont BMFont_t;

BMFont_t * BMFont_new( BMFont_t * pfnt );
bool_t     BMFont_delete( BMFont_t * pfnt );

bool_t BMFont_load( BMFont_t * pbmp, int scr_height, char* szBitmap, char* szSpacing );
void   BMFont_draw_one( BMFont_t * pfnt, int fonttype, float x, float y );
int    BMFont_word_size( BMFont_t * pfnt, char *szText );

#define FNT_NUM_FONT_CHARACTERS 94
#define FNT_SMALL_FONT_SIZE 12
#define FNT_NORMAL_FONT_SIZE 16
#define FNT_LARGE_FONT_SIZE 20
#define FNT_MAX_FONTS 8

typedef struct sTTFont TTFont_t;

extern struct sTTFont *fnt_loadFont( const char *fileName, int pointSize );
extern bool_t  fnt_freeFont( struct sTTFont *font );

extern void  fnt_drawText( struct sTTFont *font, int x, int y, const char *text );
extern void  fnt_drawTextFormatted( struct sTTFont *font, int x, int y, const char *format, ... );
extern void  fnt_drawTextBox( struct sTTFont *font, const char *text, int x, int y, int width, int height, int spacing );

// Only works properly on a single line of text
extern void  fnt_getTextSize( struct sTTFont *font, const char *text, int *width, int *height );

// Works for multiple-line strings, using the user-supplied spacing
extern void  fnt_getTextBoxSize( struct sTTFont *font, const char *text, int spacing, int *width, int *height );
