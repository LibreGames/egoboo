#pragma once

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

/// @file font_ttf.h
/// @details True-type font drawing functionality.  Uses the SDL_ttf module
/// to do its business. This depends on Freetype 2 & OpenGL.

#include <SDL.h>
#include <SDL_ttf.h>

#include "egoboo_display_list.h"

#if defined(__cplusplus)
extern "C"
{
#endif


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define FNT_NUM_FONT_CHARACTERS 94
#define FNT_SMALL_FONT_SIZE 12
#define FNT_NORMAL_FONT_SIZE 16
#define FNT_LARGE_FONT_SIZE 20
#define FNT_MAX_FONTS 8

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    extern int      fnt_init();

    extern TTF_Font * fnt_loadFont( const char *fileName, int pointSize );
    extern void       fnt_freeFont( TTF_Font *font );

/// Convert the formatted text to an ogl texture
    extern display_item_t * fnt_convertText( display_item_t *tx_ptr, TTF_Font * ttf_ptr, const char *format, ... );
/// The variable argument version of fnt_convertText()
    extern display_item_t * fnt_vconvertText( display_item_t *tx_ptr, TTF_Font * ttf_ptr, const char *format, va_list args );
/// The not formatted version of fnt_convertText()
    extern display_item_t * fnt_convertText_literal( display_item_t *tx_ptr, TTF_Font * ttf_ptr, const char *text );

/// Convert the formatted text to an ogl texture for each line of the text
    extern int fnt_convertTextBox( display_list_t * tx_lst, TTF_Font * ttf_ptr, int width, int height, int spacing, const char *format, ... );
/// The variable argument version of fnt_convertTextBox()
    extern int fnt_vconvertTextBox( display_list_t * tx_lst, TTF_Font * ttf_ptr, int x, int y, int spacing, const char *format, va_list args );
/// Convert un-formatted text to an ogl texture for each line of the text
    extern int fnt_convertTextBox_literal( display_list_t * tx_lst, TTF_Font * ttf_ptr, int width, int height, int spacing, const char *text );

/// Only works properly on a single line of text
    extern const char * fnt_getTextSize( TTF_Font * ttf_ptr, int *pwidth, int *pheight, const char *format, ... );
/// The variable argument version of fnt_getTextSize()
    extern const char * fnt_vgetTextSize( TTF_Font * ttf_ptr, int *pwidth, int *pheight, const char *format, va_list args );

/// Works for multiple-line strings, using the user-supplied spacing
    extern const char * fnt_getTextBoxSize( TTF_Font * ttf_ptr, int spacing, int *pwidth, int *pheight, const char *format, ... );
/// The variable argument version of fnt_getTextBoxSize()
    extern const char * fnt_vgetTextBoxSize( TTF_Font * ttf_ptr, int spacing, int *pwidth, int *pheight, const char *format, va_list args );

/// handle variable arguments to print text to a GL texture
    extern display_item_t * fnt_vprintf( display_item_t * tx_ptr, TTF_Font * ttf_ptr, SDL_Color color, const char *format, va_list args );

/// handle print text to a GL texture
    extern display_item_t * fnt_print( display_item_t * tx_ptr, TTF_Font * ttf_ptr, SDL_Color color, const char *text );

    extern display_list_t * fnt_vappend_text( display_list_t * tx_lst, TTF_Font * ttf_ptr, int x, int y, const char *format, va_list args );
    extern display_list_t * fnt_append_text( display_list_t * tx_lst, TTF_Font * ttf_ptr, int x, int y, const char *format, ... );
    extern display_list_t * fnt_append_text_literal( display_list_t * tx_lst, TTF_Font * ttf_ptr, int x, int y, const char *text );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
};
#endif

#define _font_ttf_h

