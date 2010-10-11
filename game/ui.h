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

/// @file ui.h
/// @details A basic library for implementing user interfaces, based off of Casey Muratori's
/// IMGUI.  (https://mollyrocket.com/forums/viewtopic.php?t=134)

#include "font_ttf.h"
#include "ogl_texture.h"
#include "egoboo_typedef.h"

#include <SDL.h>

#define UI_Nothing (ui_id_t)(-1)
#define UI_Invalid (ui_id_t)(-2)

typedef struct s_ui_Context ui_Context_t;
typedef Uint32 ui_id_t;

/// Possible UI button states
enum e_ui_button_values
{
    BUTTON_NOCHANGE = 0,
    BUTTON_DOWN,
    BUTTON_UP
};
typedef enum e_ui_button_values ui_buttonValues;

/// Possible UI button behaviors
enum e_ui_behavior_bits
{
    UI_LATCH_NONE      = 0,
    UI_LATCH_MOUSEOVER = 1 << 0,
    UI_LATCH_CLICKED   = 1 << 1,

    UI_LATCH_ALL       = UI_LATCH_MOUSEOVER | UI_LATCH_CLICKED
};

/// Possible UI widget display
enum e_ui_display_bits
{
    UI_DISPLAY_NONE   = 0,
    UI_DISPLAY_BUTTON = 1 << 0,
    UI_DISPLAY_IMAGE  = 1 << 1,
    UI_DISPLAY_TEXT   = 1 << 2
};

#define MAX_WIDGET_TEXT 20

/// The data describing the state of a UI widget
struct s_ui_Widget
{
    ui_id_t         id;

    // text info
    display_list_t * tx_lst;

    // image info
    oglx_texture_t *img;

    // which latches to keep track of
    BIT_FIELD       latch_mask;          
    Uint32          latch_state;

    BIT_FIELD       display_mask;        // which elements to display
    Uint32          timeout;

    // virtual screen coordinates
    float         vx, vy;
    float         vwidth, vheight;
};
typedef struct s_ui_Widget ui_Widget_t;

bool_t ui_Widget_free( ui_Widget_t * pw );
bool_t ui_Widget_copy( ui_Widget_t * pw2, ui_Widget_t * pw1 );
bool_t ui_Widget_shrink( ui_Widget_t * pw2, ui_Widget_t * pw1, float pixels );
bool_t ui_Widget_init( ui_Widget_t * pw, ui_id_t id, TTF_Font * ttf_ptr, const char *text, oglx_texture_t *img, float x, float y, float width, float height );

bool_t    ui_Widget_LatchMaskAdd( ui_Widget_t * pw, BIT_FIELD mbits );
bool_t    ui_Widget_LatchMaskRemove( ui_Widget_t * pw, BIT_FIELD mbits );
bool_t    ui_Widget_LatchMaskSet( ui_Widget_t * pw, BIT_FIELD mbits );
BIT_FIELD ui_Widget_LatchMaskTest( ui_Widget_t * pw, BIT_FIELD mbits );

bool_t    ui_Widget_DisplayMaskAdd( ui_Widget_t * pw, BIT_FIELD mbits );
bool_t    ui_Widget_DisplayMaskRemove( ui_Widget_t * pw, BIT_FIELD mbits );
bool_t    ui_Widget_DisplayMaskSet( ui_Widget_t * pw, BIT_FIELD mbits );
BIT_FIELD ui_Widget_DisplayMaskTest( ui_Widget_t * pw, BIT_FIELD mbits );

bool_t ui_Widget_set_text( ui_Widget_t * pw, TTF_Font * ttf_ptr, int voffset, const char * format, ... );
bool_t ui_Widget_set_image( ui_Widget_t * pw, oglx_texture_t * img );
bool_t ui_Widget_set_bound( ui_Widget_t * pw, float x, float y, float w, float h );
bool_t ui_Widget_set_button( ui_Widget_t * pw, float x, float y, float w, float h );
bool_t ui_Widget_set_pos( ui_Widget_t * pw, float x, float y );
bool_t ui_Widget_set_id( ui_Widget_t * pw, ui_id_t id );
bool_t ui_Widget_set_img( ui_Widget_t * pw, oglx_texture_t *img );

// Initialize or shut down the ui system
int  ui_begin( const char *default_font, int default_font_size );
void ui_end();
void ui_Reset();

// Pass input data from SDL to the ui
bool_t ui_handleSDLEvent( SDL_Event *evt );

// Allow the ui to do work that needs to be done before and after each frame
void ui_beginFrame( float deltaTime );
void ui_endFrame();

// UI controls
ui_buttonValues ui_doWidget( ui_Widget_t * pWidget );
ui_buttonValues ui_doButton( ui_id_t id, display_item_t * tx_ptr, const char *text, float x, float y, float width, float height );
ui_buttonValues ui_doImageButton( ui_id_t id, oglx_texture_t *img, float x, float y, float width, float height, GLXvector3f image_tint );
ui_buttonValues ui_doImageButtonWithText( ui_id_t id, display_item_t * tx_ptr, oglx_texture_t *img, const char *text, float x, float y, float width, float height );
// int  ui_doTextBox(ui_id_t id, const char *text, float x, float y, float width, float height);

// Utility functions
int  ui_mouseInside( float x, float y, float width, float height );
// void ui_setActive( ui_id_t id );
// void ui_setHot( ui_id_t id );
TTF_Font * ui_getFont();
TTF_Font * ui_setFont( TTF_Font * font );

/*****************************************************************************/
// Most users won't need to worry about stuff below here; it's mostly for
// implementing new controls.
/*****************************************************************************/

// Behaviors
ui_buttonValues  ui_buttonBehavior( ui_id_t id, float x, float y, float width, float height );
ui_buttonValues  ui_Widget_Behavior( ui_Widget_t * pw );

// Drawing
void   ui_drawButton( ui_id_t id, float x, float y, float width, float height, GLXvector4f pcolor );
void   ui_drawImage( ui_id_t id, oglx_texture_t *img, float x, float y, float width, float height, GLXvector4f image_tint );
bool_t ui_drawText( display_item_t * tx_ptr, float vx, float vy );
int    ui_drawTextBox( display_list_t * tx_lst, float vx, float vy, float vwidth, float vheight );

display_item_t *     ui_updateText( display_item_t * tx_ptr, TTF_Font * ttf_ptr, float vx, float vy, const char *format, ... );
display_list_t * ui_updateTextBox( display_list_t * tx_lst, TTF_Font * ttf_ptr, float vx, float vy, float vspacing, const char * format, ... );
display_list_t * ui_updateTextBox_literal( display_list_t * tx_lst, TTF_Font * ttf_ptr, float vx, float vy, float vspacing, const char * text );

display_item_t *     ui_vupdateText( display_item_t * tx_ptr, TTF_Font * ttf_ptr, float vx, float vy, const char *format, va_list args );
display_list_t * ui_vupdateTextBox( display_list_t * tx_lst, TTF_Font * ttf_ptr, float vx, float vy, float vspacing, const char * format, va_list args );

float ui_drawTextBoxImmediate( TTF_Font * ttf_ptr, float vx, float vy, float spacing, const char *format, ... );

// virtual screen
void ui_set_virtual_screen( float vw, float vh, float ww, float wh );
TTF_Font * ui_loadFont( const char * font_name, float vpointSize );

#define egoboo_ui_h

