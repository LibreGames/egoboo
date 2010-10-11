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

/// @file ui.c
/// @brief The Egoboo GUI
/// @details A basic library for implementing user interfaces, based off of Casey Muratori's
/// IMGUI.  (https://mollyrocket.com/forums/viewtopic.php?t=134)

#include "ui.h"
#include "graphic.h"
#include "font_ttf.h"
#include "texture.h"

#include "ogl_debug.h"
#include "SDL_extensions.h"

#include <string.h>
#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define UI_MAX_JOYSTICKS 8
#define UI_CONTROL_TIMER 100

struct s_ui_control_info
{
    int    timer;
    float  scr[2];
    float  vrt[2];
};
typedef struct s_ui_control_info ui_control_info_t;

/// The data to describe the UI state
struct UiContext
{
    // Tracking control focus stuff
    ui_id_t active;
    ui_id_t hot;

    // info on the mouse control
    ui_control_info_t mouse;
    ui_control_info_t joy;
    ui_control_info_t joys[UI_MAX_JOYSTICKS];

    // Basic cursor state
    float cursor_X, cursor_Y;
    int   cursor_Released;
    int   cursor_Pressed;

    STRING defaultFontName;
    float  defaultFontSize;
    TTF_Font  *defaultFont;
    TTF_Font  *activeFont;

    // virtual window
    float vw, vh, ww, wh;

    // define the forward transform
    float aw, ah, bw, bh;

    // define the inverse transform
    float iaw, iah, ibw, ibh;
};

static struct UiContext ui_context;

static const GLfloat ui_white_color[]  = {1.00f, 1.00f, 1.00f, 1.00f};

static const GLfloat ui_active_color[]  = {0.00f, 0.00f, 0.90f, 0.60f};
static const GLfloat ui_hot_color[]     = {0.54f, 0.00f, 0.00f, 1.00f};
static const GLfloat ui_normal_color[]  = {0.66f, 0.00f, 0.00f, 0.60f};

static const GLfloat ui_active_color2[] = {0.00f, 0.45f, 0.45f, 0.60f};
static const GLfloat ui_hot_color2[]    = {0.00f, 0.28f, 0.28f, 1.00f};
static const GLfloat ui_normal_color2[] = {0.33f, 0.00f, 0.33f, 0.60f};

static void ui_virtual_to_screen_abs( float vx, float vy, float *rx, float *ry );
static void ui_screen_to_virtual_abs( float rx, float ry, float *vx, float *vy );

static void ui_virtual_to_screen_rel( float vx, float vy, float *rx, float *ry );
static void ui_screen_to_virtual_rel( float rx, float ry, float *vx, float *vy );

static void ui_joy_init();
static void ui_cursor_update();
static bool_t ui_joy_set( SDL_JoyAxisEvent * evt_ptr );

static bool_t ui_Widget_update_text_pos( ui_Widget_t * pw );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Core functions
int ui_begin( const char *default_font, int default_font_size )
{
    // initialize the font handler
    fnt_init();

    memset( &ui_context, 0, sizeof( ui_context ) );

    ui_context.active = ui_context.hot = UI_Nothing;

    ui_context.defaultFontSize = default_font_size;
    strncpy( ui_context.defaultFontName, default_font, SDL_arraysize( ui_context.defaultFontName ) );

    ui_set_virtual_screen( sdl_scr.x, sdl_scr.y, sdl_scr.x, sdl_scr.y );

    ui_joy_init();

    return 1;
}

//--------------------------------------------------------------------------------------------
void ui_end()
{
    // clear out the default font
    if ( NULL != ui_context.defaultFont )
    {
        fnt_freeFont( ui_context.defaultFont );
        ui_context.defaultFont = NULL;
    }

    // clear out the active font
    ui_context.activeFont = NULL;

    memset( &ui_context, 0, sizeof( ui_context ) );
}

//--------------------------------------------------------------------------------------------
void ui_Reset()
{
    ui_context.active = ui_context.hot = UI_Nothing;
}

//--------------------------------------------------------------------------------------------
bool_t ui_handleSDLEvent( SDL_Event *evt )
{
    bool_t handled;

    if ( NULL == evt ) return bfalse;

    handled = btrue;
    switch ( evt->type )
    {
        case SDL_JOYBUTTONDOWN:
        case SDL_MOUSEBUTTONDOWN:
            ui_context.cursor_Released = 0;
            ui_context.cursor_Pressed = 1;
            break;

        case SDL_JOYBUTTONUP:
        case SDL_MOUSEBUTTONUP:
            ui_context.cursor_Pressed = 0;
            ui_context.cursor_Released = 1;
            break;

        case SDL_MOUSEMOTION:
            // convert the screen coordinates to our "virtual coordinates"
            ui_context.mouse.scr[0] = evt->motion.x;
            ui_context.mouse.vrt[1] = evt->motion.y;
            ui_screen_to_virtual_abs( ui_context.mouse.scr[0], ui_context.mouse.vrt[1], &( ui_context.mouse.vrt[0] ), &( ui_context.mouse.vrt[1] ) );
            ui_context.mouse.timer  = 2 * UI_CONTROL_TIMER;
            break;

        case SDL_JOYAXISMOTION:
            ui_joy_set( &( evt->jaxis ) );
            break;

        case SDL_VIDEORESIZE:
            if ( SDL_VIDEORESIZE == evt->resize.type )
            {
                // the video has been re-sized, if the game is active, the
                // view matrix needs to be recalculated and possibly the
                // auto-formatting for the menu system and the ui system must be
                // recalculated

                // grab all the new SDL screen info
                SDLX_Get_Screen_Info( &sdl_scr, bfalse );

                // set the ui's virtual screen size based on the graphic system's
                // configuration
                gfx_set_virtual_screen( &gfx );
            }
            break;

        default:
            handled = bfalse;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
void ui_beginFrame( float deltaTime )
{
    ATTRIB_PUSH( "ui_beginFrame", GL_ENABLE_BIT );
    GL_DEBUG( glDisable )( GL_DEPTH_TEST );
    GL_DEBUG( glDisable )( GL_CULL_FACE );
    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    GL_DEBUG( glEnable )( GL_BLEND );
    GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    GL_DEBUG( glViewport )( 0, 0, sdl_scr.x, sdl_scr.y );

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadIdentity )();
    GL_DEBUG( glOrtho )( 0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glLoadIdentity )();

    // hotness gets reset at the start of each frame
    ui_context.hot = UI_Nothing;

    // update the cursor position
    ui_cursor_update();
}

//--------------------------------------------------------------------------------------------
void ui_draw_cursor_icon( TX_REF icon_ref )
{
    float x, y;

    ui_virtual_to_screen_abs( ui_context.cursor_X - 5, ui_context.cursor_Y - 5, &x, &y );

    //Draw the cursor, but only if it inside the screen
    draw_one_icon( icon_ref, x, y, NOSPARKLE );
}

//--------------------------------------------------------------------------------------------
void ui_draw_cursor_ogl()
{
    const float cursor_h = 15.0f;
    const float cursor_w  = cursor_h * INV_SQRT_TWO;

    const float cursor_d1  = 0.92387953251128675612818318939679f;
    const float cursor_d2  = 0.3826834323650897717284599840304f;

    float x1, y1, x2, y2, x3, y3;

    ATTRIB_PUSH( "ui_draw_cursor_ogl", GL_ENABLE_BIT | GL_CURRENT_BIT | GL_HINT_BIT | GL_COLOR_BUFFER_BIT );
    {
        // mmm.... anti-aliased cursor....

        GL_DEBUG( glDisable )( GL_TEXTURE_2D );                           // GL_ENABLE_BIT

        GL_DEBUG( glEnable )( GL_LINE_SMOOTH );                           // GL_ENABLE_BIT
        GL_DEBUG( glEnable )( GL_POLYGON_SMOOTH );                        // GL_ENABLE_BIT

        GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        GL_DEBUG( glHint )( GL_LINE_SMOOTH_HINT, GL_NICEST );             // GL_HINT_BIT
        GL_DEBUG( glHint )( GL_POLYGON_SMOOTH_HINT, GL_NICEST );          // GL_HINT_BIT

        ui_virtual_to_screen_abs( ui_context.cursor_X,          ui_context.cursor_Y,          &x1, &y1 );
        ui_virtual_to_screen_abs( ui_context.cursor_X + cursor_w, ui_context.cursor_Y + cursor_w, &x2, &y2 );
        ui_virtual_to_screen_abs( ui_context.cursor_X,          ui_context.cursor_Y + cursor_h, &x3, &y3 );

        // Draw the head
        GL_DEBUG( glColor4f )( 1, 1, 1, 1 );                   // GL_CURRENT_BIT
        GL_DEBUG( glBegin )( GL_POLYGON );
        {
            GL_DEBUG( glVertex2f )( x1, y1 );
            GL_DEBUG( glVertex2f )( x2, y2 );
            GL_DEBUG( glVertex2f )( x3, y3 );
        }
        GL_DEBUG_END();

        // Draw the tail
        {
            float dx1 = SQRT_TWO * cursor_h * cursor_d2;
            float dy1 = SQRT_TWO * cursor_h * cursor_d1;

            float dx2 = cursor_h / 10.0f * cursor_d1;
            float dy2 = -cursor_h / 10.0f * cursor_d2;

            ui_virtual_to_screen_abs( ui_context.cursor_X + dx1 + dx2, ui_context.cursor_Y + dy1 + dy2, &x2, &y2 );
            ui_virtual_to_screen_abs( ui_context.cursor_X + dx1 - dx2, ui_context.cursor_Y + dy1 - dy2, &x3, &y3 );
        }

        GL_DEBUG( glBegin )( GL_POLYGON );
        {
            GL_DEBUG( glVertex2f )( x1, y1 );
            GL_DEBUG( glVertex2f )( x2, y2 );
            GL_DEBUG( glVertex2f )( x3, y3 );
        }
        GL_DEBUG_END();
    }
    ATTRIB_POP( "ui_draw_cursor_ogl" );
}

//--------------------------------------------------------------------------------------------
void ui_draw_cursor()
{
    // see if the icon cursor is available
    TX_REF           ico_ref = ( TX_REF )TX_CURSOR;
    oglx_texture_t * ptex    = TxTexture_get_ptr( ico_ref );

    if ( NULL == ptex )
    {
        ui_draw_cursor_ogl();
    }
    else
    {
        ui_draw_cursor_icon( ico_ref );
    }
}

//--------------------------------------------------------------------------------------------
void ui_endFrame()
{
    // Draw the cursor last
    ui_draw_cursor();

    // Restore the OpenGL matrices to what they were
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glLoadIdentity )();

    // Re-enable any states disabled by gui_beginFrame
    ATTRIB_POP( "ui_endFrame" );

    // Clear input states at the end of the frame
    ui_context.cursor_Pressed = ui_context.cursor_Released = 0;
}

//--------------------------------------------------------------------------------------------
// Utility functions
int ui_mouseInside( float vx, float vy, float vwidth, float vheight )
{
    float vright, vbottom;

    vright  = vx + vwidth;
    vbottom = vy + vheight;
    if ( vx <= ui_context.cursor_X && vy <= ui_context.cursor_Y && ui_context.cursor_X <= vright && ui_context.cursor_Y <= vbottom )
    {
        return 1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
void ui_setactive( ui_id_t id )
{
    ui_context.active = id;
}

//--------------------------------------------------------------------------------------------
void ui_sethot( ui_id_t id )
{
    ui_context.hot = id;
}

//--------------------------------------------------------------------------------------------
void ui_Widget_setActive( ui_Widget_t * pw )
{
    if ( NULL == pw )
    {
        ui_context.active = UI_Nothing;
    }
    else
    {
        ui_context.active = pw->id;

        pw->timeout = egoboo_get_ticks() + 100;
        if ( ui_Widget_LatchMaskTest( pw, UI_LATCH_CLICKED ) )
        {
            // use exclusive or to flip the bit
            pw->latch_state ^= UI_LATCH_CLICKED;
        };
    };
}

//--------------------------------------------------------------------------------------------
void ui_Widget_setHot( ui_Widget_t * pw )
{
    if ( NULL == pw )
    {
        ui_context.hot = UI_Nothing;
    }
    else if (( ui_context.active == pw->id || ui_context.active == UI_Nothing ) )
    {
        if ( pw->timeout < egoboo_get_ticks() )
        {
            pw->timeout = egoboo_get_ticks() + 100;

            if ( ui_Widget_LatchMaskTest( pw, UI_LATCH_MOUSEOVER ) && ui_context.hot != pw->id )
            {
                pw->latch_state |= UI_LATCH_MOUSEOVER;
            };
        };

        // Only allow hotness to be set if this control, or no control is active
        ui_context.hot = pw->id;
    }
}

//--------------------------------------------------------------------------------------------
TTF_Font * ui_getFont()
{
    return ( NULL != ui_context.activeFont ) ? ui_context.activeFont : ui_context.defaultFont;
}

//--------------------------------------------------------------------------------------------
TTF_Font* ui_setFont( TTF_Font * font )
{
    ui_context.activeFont = font;

    return ui_context.activeFont;
}

//--------------------------------------------------------------------------------------------
// Behaviors
ui_buttonValues ui_buttonBehavior( ui_id_t id, float vx, float vy, float vwidth, float vheight )
{
    ui_buttonValues result = BUTTON_NOCHANGE;

    if ( id == UI_Nothing )
        return result;

    // If the mouse is over the button, try and set hotness so that it can be cursor_clicked
    if ( ui_mouseInside( vx, vy, vwidth, vheight ) )
    {
        ui_sethot( id );
    }

    // Check to see if the button gets cursor_clicked on
    if ( id == ui_context.active )
    {
        if ( 1 == ui_context.cursor_Released )
        {
            ui_setactive( UI_Nothing );
            result = BUTTON_UP;
        }
    }
    else if ( id == ui_context.hot )
    {
        if ( 1 == ui_context.cursor_Pressed )
        {
            ui_setactive( id );
            result = BUTTON_DOWN;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_Widget_Behavior( ui_Widget_t * pWidget )
{
    ui_buttonValues result = BUTTON_NOCHANGE;

    if ( NULL == pWidget ) return result;

    if ( UI_Nothing == pWidget->id ) return result;

    // If the mouse is over the button, try and set hotness so that it can be cursor_clicked
    if ( ui_mouseInside( pWidget->vx, pWidget->vy, pWidget->vwidth, pWidget->vheight ) )
    {
        ui_Widget_setHot( pWidget );
    }

    // Check to see if the button gets cursor_clicked on
    if ( ui_context.active == pWidget->id )
    {
        if ( 1 == ui_context.cursor_Released )
        {
            // mouse button up
            ui_Widget_setActive( NULL );
            result = BUTTON_UP;
        }
    }
    else if ( ui_context.hot == pWidget->id )
    {
        if ( 1 == ui_context.cursor_Pressed )
        {
            // mouse button down
            ui_Widget_setActive( pWidget );
            result = BUTTON_DOWN;
        }
    }
    else
    {
        pWidget->latch_state &= ~UI_LATCH_MOUSEOVER;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
// Drawing
void ui_drawButton( ui_id_t id, float vx, float vy, float vwidth, float vheight, GLXvector4f pcolor )
{
    float x1, x2, y1, y2;

    GLXvector4f color_1 = { 0.0f, 0.0f, 0.9f, 0.6f };
    GLXvector4f color_2 = { 0.54f, 0.0f, 0.0f, 1.0f };
    GLXvector4f color_3 = { 0.66f, 0.0f, 0.0f, 0.6f };

    // Draw the button
    GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    if ( NULL == pcolor )
    {
        if ( ui_context.active != UI_Nothing && ui_context.active == id && ui_context.hot == id )
        {
            pcolor = color_1;
        }
        else if ( ui_context.hot != UI_Nothing && ui_context.hot == id )
        {
            pcolor = color_2;
        }
        else
        {
            pcolor = color_3;
        }
    }

    // convert the virtual coordinates to screen coordinates
    ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );
    ui_virtual_to_screen_abs( vx + vwidth, vy + vheight, &x2, &y2 );

    GL_DEBUG( glColor4fv )( pcolor );
    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glVertex2f )( x1, y1 );
        GL_DEBUG( glVertex2f )( x1, y2 );
        GL_DEBUG( glVertex2f )( x2, y2 );
        GL_DEBUG( glVertex2f )( x2, y1 );
    }
    GL_DEBUG_END();

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void ui_drawImage( ui_id_t id, oglx_texture_t *img, float vx, float vy, float vwidth, float vheight, GLXvector4f image_tint )
{
    GLXvector4f tmp_tint = {1, 1, 1, 1};

    float vw, vh;
    float tx, ty;
    float x1, x2, y1, y2;

    // handle optional parameters
    if ( NULL == image_tint ) image_tint = tmp_tint;

    if ( img )
    {
        if ( vwidth == 0 || vheight == 0 )
        {
            vw = img->imgW;
            vh = img->imgH;
        }
        else
        {
            vw = vwidth;
            vh = vheight;
        }

        tx = ( float ) oglx_texture_GetImageWidth( img )  / ( float ) oglx_texture_GetTextureWidth( img );
        ty = ( float ) oglx_texture_GetImageHeight( img ) / ( float ) oglx_texture_GetTextureHeight( img );

        // convert the virtual coordinates to screen coordinates
        ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );
        ui_virtual_to_screen_abs( vx + vw, vy + vh, &x2, &y2 );

        // Draw the image
        oglx_texture_Bind( img );

        GL_DEBUG( glColor4fv )( image_tint );

        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( 0,  0 );  GL_DEBUG( glVertex2f )( x1, y1 );
            GL_DEBUG( glTexCoord2f )( tx,  0 );  GL_DEBUG( glVertex2f )( x2, y1 );
            GL_DEBUG( glTexCoord2f )( tx, ty );  GL_DEBUG( glVertex2f )( x2, y2 );
            GL_DEBUG( glTexCoord2f )( 0, ty );  GL_DEBUG( glVertex2f )( x1, y2 );
        }
        GL_DEBUG_END();
    }
}

//--------------------------------------------------------------------------------------------
void ui_Widget_drawButton( ui_Widget_t * pw )
{
    GLfloat * pcolor = NULL;
    bool_t ui_active, ui_hot;

    ui_active = ui_context.active == pw->id && ui_context.hot == pw->id;
    ui_hot    = ui_context.hot == pw->id;

    if ( UI_Nothing == pw->id )
    {
        // this is a label
        pcolor = ui_normal_color;
    }
    else if ( 0 != pw->latch_mask )
    {
        // this is a "normal" button

        bool_t st_active, st_hot;

        st_active = 0 != ( ui_Widget_LatchMaskTest( pw, UI_LATCH_CLICKED ) & pw->latch_state );
        st_hot    = 0 != ( ui_Widget_LatchMaskTest( pw, UI_LATCH_MOUSEOVER ) & pw->latch_state );

        if ( ui_active || st_active )
        {
            pcolor = ui_normal_color2;
        }
        else if ( ui_hot || st_hot )
        {
            pcolor = ui_hot_color;
        }
        else
        {
            pcolor = ui_normal_color;
        }
    }
    else
    {
        // this is a "presistent" button

        if ( ui_active )
        {
            pcolor = ui_active_color;
        }
        else if ( ui_hot )
        {
            pcolor = ui_hot_color;
        }
        else
        {
            pcolor = ui_normal_color;
        }
    }

    ui_drawButton( pw->id, pw->vx, pw->vy, pw->vwidth, pw->vheight, pcolor );
}

//--------------------------------------------------------------------------------------------
void ui_Widget_drawImage( ui_Widget_t * pw )
{
    if ( NULL != pw && NULL != pw->img )
    {
        ui_drawImage( pw->id, pw->img, pw->vx, pw->vy, pw->vwidth, pw->vheight, NULL );
    }
}

//--------------------------------------------------------------------------------------------
/** ui_vupdateTextBox
 * Draws a text string into a box, splitting it into lines according to newlines in the string.
 * @warning Doesn't pay attention to the width/height arguments yet.
 *
 * text    - The text to draw
 * x       - The x position to start drawing at
 * y       - The y position to start drawing at
 * width   - Maximum width of the box (not implemented)
 * height  - Maximum height of the box (not implemented)
 * spacing - Amount of space to move down between lines. (usually close to your font size)
 */

//--------------------------------------------------------------------------------------------
float ui_drawTextBoxImmediate( TTF_Font * ttf_ptr, float vx, float vy, float spacing, const char *format, ... )
{
    display_list_t * tmp_tx_lst = NULL;
    va_list args;

    // render the text to a display_list
    va_start( args, format );
    tmp_tx_lst = ui_vupdateTextBox( tmp_tx_lst, ttf_ptr, vx, vy, spacing, format, args );
    va_end( args );

    if ( NULL != tmp_tx_lst )
    {
        float x1, y1;

        frect_t bound;
        int line_count;

        // set the texture position
        display_list_bound( tmp_tx_lst, &bound );
        ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );
        display_list_adjust_bound( tmp_tx_lst, x1 - bound.x, y1 - bound.y );

        // output the lines
        line_count = display_list_draw( tmp_tx_lst );

        // adjust the vertical position
        vy += spacing * line_count;

        // get rid of the temporary list
        tmp_tx_lst = display_list_dtor( tmp_tx_lst, btrue );
    }

    // return the new vertical position
    return vy;
}

//--------------------------------------------------------------------------------------------
display_item_t * ui_vupdateText( display_item_t * tx_ptr, TTF_Font * ttf_ptr, float vx, float vy, const char *format, va_list args )
{
    float  x1, y1;
    bool_t local_dspl;
    display_item_t * retval = NULL;

    // use the default ui font?
    if ( NULL == ttf_ptr )
    {
        ttf_ptr = ui_getFont();
    }

    // make sure we have a list of some kind
    local_dspl = bfalse;
    if ( NULL == tx_ptr )
    {
        tx_ptr = display_item_new();
        local_dspl = btrue;
    }

    // convert the virtual coordinates to screen coordinates
    ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );

    // convert the text to a display_item using screen coordinates
    retval = fnt_vconvertText( tx_ptr, ttf_ptr, format, args );
    display_item_set_pos( tx_ptr, x1, y1 );

    // an error
    if ( NULL == retval )
    {
        // delete the display list data
        tx_ptr = display_item_free( tx_ptr, local_dspl );
    }

    return tx_ptr;
}

//--------------------------------------------------------------------------------------------
display_item_t * ui_updateText( display_item_t * tx_ptr, TTF_Font * ttf_ptr, float vx, float vy, const char *format, ... )
{
    va_list args;

    // use the default ui font?
    if ( NULL == ttf_ptr )
    {
        ttf_ptr = ui_getFont();
    }

    va_start( args, format );
    tx_ptr = ui_vupdateText( tx_ptr, ttf_ptr, vx, vy, format, args );
    va_end( args );

    return tx_ptr;
}

//--------------------------------------------------------------------------------------------
bool_t ui_drawText( display_item_t * tx_ptr, float vx, float vy )
{
    float x1, y1;

    if ( NULL == tx_ptr ) return bfalse;

    ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );

    display_item_set_pos( tx_ptr, x1, y1 );

    return display_item_draw( tx_ptr );
}

//--------------------------------------------------------------------------------------------
display_list_t * ui_updateTextBox_literal( display_list_t * tx_lst, TTF_Font * ttf_ptr, float vx, float vy, float vspacing, const char * text )
{
    float   x1, y1;
    float   spacing;
    GLsizei line_count;
    bool_t  local_dspl_lst;

    // clear out any existing tx_lst
    if ( NULL != tx_lst ) tx_lst = display_list_clear( tx_lst );

    // use the default ui font?
    if ( NULL == ttf_ptr ) ttf_ptr = ui_getFont();

    // create a  tx_lst, if needed
    local_dspl_lst = bfalse;
    if ( NULL == tx_lst || 0 == display_list_used( tx_lst ) )
    {
        tx_lst = display_list_ctor( tx_lst, MAX_WIDGET_TEXT );
        local_dspl_lst = btrue;
    }

    // convert the virtual coordinates to screen coordinates
    ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );
    spacing = ui_context.ah * vspacing;

    // convert the text to a display_list using screen coordinates
    line_count = fnt_convertTextBox_literal( tx_lst, ttf_ptr, x1, y1, spacing, text );

    // an error
    if ( line_count > display_list_used( tx_lst ) )
    {
        // delete the display list data
        tx_lst = display_list_dtor( tx_lst, local_dspl_lst );
    }

    return tx_lst;
}

//--------------------------------------------------------------------------------------------
display_list_t * ui_vupdateTextBox( display_list_t * tx_lst, TTF_Font * ttf_ptr, float vx, float vy, float vspacing, const char * format, va_list args )
{
    display_list_t * retval;
    int vsnprintf_rv;

    char text[4096];

    // clear out any existing tx_lst
    if ( NULL != tx_lst ) tx_lst = display_list_clear( tx_lst );

    // convert the text string
    vsnprintf_rv = vsnprintf( text, SDL_arraysize( text ) - 1, format, args );

    // some problem printing the text?
    if ( vsnprintf_rv < 0 )
    {
        tx_lst = display_list_dtor( tx_lst, bfalse );
        retval = tx_lst;
    }
    else
    {
        retval = ui_updateTextBox_literal( tx_lst, ttf_ptr, vx, vy, vspacing, text );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
display_list_t * ui_updateTextBox( display_list_t * tx_lst, TTF_Font * ttf_ptr, float vx, float vy, float vspacing, const char * format, ... )
{
    // the normal entry function for ui_vupdateTextBox()

    va_list args;

    va_start( args, format );
    tx_lst = ui_vupdateTextBox( tx_lst, ttf_ptr, vx, vy, vspacing, format, args );
    va_end( args );

    return tx_lst;
}

//--------------------------------------------------------------------------------------------
int ui_drawTextBox( display_list_t * tx_lst, float vx, float vy, float vwidth, float vheight )
{
    int rv = 0;
    float x1, x2, y1, y2;
    size_t size;

    if ( NULL == tx_lst ) return 0;

    size = display_list_used( tx_lst );
    if ( 0 == size ) return 0;

    ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );

    // default size?
    x2 = y2 = -1.0f;
    if ( vwidth < 0.0f || vheight < 0.0f )
    {
        frect_t tmp;

        if ( display_list_bound( tx_lst, &tmp ) )
        {
            x2 = tmp.x + tmp.w;
            y2 = tmp.y + tmp.h;
        }
    }
    else
    {
        ui_virtual_to_screen_abs( vx + vwidth, vy + vheight, &x2, &y2 );
    }

    ATTRIB_PUSH( "ui_vupdateTextBox", GL_SCISSOR_BIT | GL_ENABLE_BIT );
    {
        //if( x2 > 0.0f && y2 > 0.0f )
        //{
        //    GL_DEBUG( glEnable )( GL_SCISSOR_TEST );
        //    GL_DEBUG( glScissor )( x1, y1, x2-x1, y2-y1 );
        //}

        rv = display_list_draw( tx_lst );
    }
    ATTRIB_POP( "ui_vupdateTextBox" );

    return rv;
}

//--------------------------------------------------------------------------------------------
// Controls
ui_buttonValues ui_doButton( ui_id_t id, display_item_t * tx_ptr, const char *text, float vx, float vy, float vwidth, float vheight )
{
    ui_buttonValues result;
    bool_t          convert_rv = bfalse;

    // Do all the logic type work for the button
    result = ui_buttonBehavior( id, vx, vy, vwidth, vheight );

    // Draw the button part of the button
    ui_drawButton( id, vx, vy, vwidth, vheight, NULL );

    // And then draw the text that goes on top of the button
    convert_rv = bfalse;
    if ( NULL != text && '\0' != text[0] )
    {
        int text_w, text_h;
        int text_x, text_y;
        float x1, x2, y1, y2;

        // convert the virtual coordinates to screen coordinates
        ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );
        ui_virtual_to_screen_abs( vx + vwidth, vy + vheight, &x2, &y2 );

        // find the vwidth & vheight of the text to be drawn, so that it can be centered inside
        // the button
        fnt_getTextSize( ui_getFont(), &text_w, &text_h, text );

        text_x = (( x2 - x1 ) - text_w ) / 2 + x1;
        text_y = (( y2 - y1 ) - text_h ) / 2 + y1;

        GL_DEBUG( glColor3f )( 1, 1, 1 );

        if ( NULL == tx_ptr )
        {
            float new_y = ui_drawTextBoxImmediate( ui_getFont(), text_x, text_y, 20, text );

            convert_rv = ( new_y != text_y );
        }
        else
        {
            tx_ptr = fnt_convertText( tx_ptr, ui_getFont(), text );
            if ( NULL != tx_ptr )
            {
                display_item_set_bound( tx_ptr, text_x, text_y, text_w, text_h );
            }

            convert_rv = display_item_draw( tx_ptr );
        }
    }

    // if there was an error, delete any existing tx_ptr
    if ( !convert_rv )
    {
        tx_ptr = display_item_free( tx_ptr, bfalse );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doImageButton( ui_id_t id, oglx_texture_t *img, float vx, float vy, float vwidth, float vheight, GLXvector3f image_tint )
{
    ui_buttonValues result;

    // Do all the logic type work for the button
    result = ui_buttonBehavior( id, vx, vy, vwidth, vheight );

    // Draw the button part of the button
    ui_drawButton( id, vx, vy, vwidth, vheight, NULL );

    // And then draw the image on top of it
    ui_drawImage( id, img, vx + 5, vy + 5, vwidth - 10, vheight - 10, image_tint );

    return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doImageButtonWithText( ui_id_t id, display_item_t * tx_ptr, oglx_texture_t *img, const char *text, float vx, float vy, float vwidth, float vheight )
{
    ui_buttonValues result;

    float text_x, text_y;
    int   text_w, text_h;
    bool_t loc_display = bfalse, retval = bfalse;

    //are we going to create a display here?
    loc_display = bfalse;
    if ( NULL == tx_ptr )
    {
        loc_display = ( NULL == tx_ptr );
    }

    // Do all the logic type work for the button
    result = ui_buttonBehavior( id, vx, vy, vwidth, vheight );

    // Draw the button part of the button
    ui_drawButton( id, vx, vy, vwidth, vheight, NULL );

    // Draw the image part
    ui_drawImage( id, img, vx + 5, vy + 5, 0, 0, NULL );

    // And draw the text next to the image
    // And then draw the text that goes on top of the button
    retval = bfalse;
    if ( NULL != tx_ptr )
    {
        float x1, x2, y1, y2;

        // convert the virtual coordinates to screen coordinates
        ui_virtual_to_screen_abs( vx, vy, &x1, &y1 );
        ui_virtual_to_screen_abs( vx + vwidth, vy + vheight, &x2, &y2 );

        // find the vwidth & vheight of the text to be drawn, so that it can be centered inside
        // the button
        fnt_getTextSize( ui_getFont(), &text_w, &text_h, text );

        text_x = ( img->imgW + 10 ) * ui_context.aw + x1;
        text_y = (( y2 - y1 ) - text_h ) / 2         + y1;

        GL_DEBUG( glColor3f )( 1, 1, 1 );

        tx_ptr = fnt_convertText( tx_ptr, ui_getFont(), text );
        if ( NULL != tx_ptr )
        {
            display_item_set_bound( tx_ptr, text_x, text_y, text_w, text_h );
        }

        retval = display_item_draw( tx_ptr );
    }

    if ( !retval && loc_display )
    {
        tx_ptr = display_item_free( tx_ptr, btrue );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doWidget( ui_Widget_t * pw )
{
    ui_buttonValues result;

    float img_w;

    // Do all the logic type work for the button
    result = ui_Widget_Behavior( pw );

    // Draw the button part of the button
    if ( 0 != ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_BUTTON ) )
    {
        ui_Widget_drawButton( pw );
    }

    // draw any image on the left hand side of the button
    img_w = 0;
    if ( 0 != ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_IMAGE ) && NULL != pw->img )
    {
        ui_Widget_t wtmp;

        // Draw the image part
        GL_DEBUG( glColor3f )( 1, 1, 1 );

        ui_Widget_shrink( &wtmp, pw, 5 );
        wtmp.vwidth = wtmp.vheight;

        ui_Widget_drawImage( &wtmp );

        // get the non-virtual image width
        img_w = pw->img->imgW * ui_context.aw;
    }

    // And draw the text on the right hand side of any image
    if ( 0 != ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_TEXT ) && NULL != pw->tx_lst )
    {
        GL_DEBUG( glColor3f )( 1, 1, 1 );
        display_list_draw( pw->tx_lst );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_copy( ui_Widget_t * pw2, ui_Widget_t * pw1 )
{
    if ( NULL == pw2 || NULL == pw1 ) return bfalse;
    return NULL != memcpy( pw2, pw1, sizeof( ui_Widget_t ) );
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_shrink( ui_Widget_t * pw2, ui_Widget_t * pw1, float pixels )
{
    if ( NULL == pw2 || NULL == pw1 ) return bfalse;

    if ( !ui_Widget_copy( pw2, pw1 ) ) return bfalse;

    pw2->vx += pixels;
    pw2->vy += pixels;
    pw2->vwidth  -= 2 * pixels;
    pw2->vheight -= 2 * pixels;

    if ( pw2->vwidth < 0 )  pw2->vwidth   = 0;
    if ( pw2->vheight < 0 ) pw2->vheight = 0;

    return pw2->vwidth > 0 && pw2->vheight > 0;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_init( ui_Widget_t * pw, ui_id_t id, TTF_Font * ttf_ptr, const char *text, oglx_texture_t *img, float vx, float vy, float vwidth, float vheight )
{
    int img_w = 0;

    if ( NULL == pw ) return bfalse;

    // use the default ui font?
    if ( NULL == ttf_ptr )
    {
        ttf_ptr = ui_getFont();
    }

    pw->id            = id;
    pw->vx            = vx;
    pw->vy            = vy;
    pw->vwidth        = vwidth;
    pw->vheight       = vheight;
    pw->latch_state   = 0;
    pw->timeout       = 0;
    pw->latch_mask    = 0;
    pw->display_mask  = UI_DISPLAY_BUTTON;

    // construct the text display list
    pw->tx_lst = display_list_ctor( pw->tx_lst, MAX_WIDGET_TEXT );

    // set any image
    ui_Widget_set_image( pw, img );

    // set any text
    img_w = ( NULL == pw->img ) ? 0 : pw->img->imgW;
    ui_Widget_set_text( pw, ttf_ptr, img_w, text );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_LatchMaskAdd( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    ADD_BITS( pw->latch_mask, mbits );
    REMOVE_BITS( pw->latch_state, mbits );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_LatchMaskRemove( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    REMOVE_BITS( pw->latch_mask, mbits );
    REMOVE_BITS( pw->latch_state, mbits );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_LatchMaskSet( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    pw->latch_mask  = mbits;
    REMOVE_BITS( pw->latch_state, mbits );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD ui_Widget_LatchMaskTest( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    return pw->latch_mask & mbits;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_DisplayMaskAdd( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    ADD_BITS( pw->display_mask, mbits );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_DisplayMaskRemove( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    REMOVE_BITS( pw->display_mask, mbits );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_DisplayMaskSet( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    pw->display_mask  = mbits;

    return btrue;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD ui_Widget_DisplayMaskTest( ui_Widget_t * pw, BIT_FIELD mbits )
{
    if ( NULL == pw ) return bfalse;

    return pw->display_mask & mbits;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_update_bound( ui_Widget_t * pw, frect_t * pbound )
{
    int   cnt_w = 0;
    float img_w, img_h;
    float txt_w, txt_h;
    float but_w, but_h;

    if ( NULL == pw || NULL == pbound ) return bfalse;

    // grab the existing image pbound
    img_w = 0.0f;
    img_h = 0.0f;
    if ( ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_IMAGE ) && NULL != pw->img )
    {
        img_w = pw->img->imgW;
        img_h = pw->img->imgH;

        cnt_w++;
    }

    // grab the existing text pbound
    txt_w = 0.0f;
    txt_h = 0.0f;
    if ( ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_TEXT ) && NULL != pw->tx_lst )
    {
        frect_t tmp;

        display_list_bound( pw->tx_lst, &tmp );

        txt_w = tmp.w;
        txt_h = tmp.h;

        cnt_w++;
    }

    // grab the existing button bound, if there is nothing else in the widget
    but_w = 30.0f;
    but_h = 30.0f;
    if ( 0 == cnt_w && ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_BUTTON ) )
    {
        but_w = MAX( but_w, pw->vwidth );
        but_h = MAX( but_h, pw->vheight );
    }

    // set the position
    pbound->x = pw->vx;
    pbound->y = pw->vy;

    // find the basic size
    if ( 0 == cnt_w )
    {
        // just copy the button size
        pbound->w = but_w;
        pbound->h = but_h;
    }
    else
    {
        // place a 5 pix border around the objects and a 5 pixel strip between
        pbound->w = 10 + img_w + txt_w + ( cnt_w - 1 ) * 5;
        pbound->h = 10 + img_h + txt_h;

        // make sure that it bigger than the default size
        pbound->w = MAX( but_w, pbound->w );
        pbound->h = MAX( but_h, pbound->h );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_bound( ui_Widget_t * pw, float x, float y, float w, float h )
{
    /// @details BB@> render the text string to a SDL_Surface

    frect_t size = {0, 0, 0, 0};

    if ( NULL == pw ) return bfalse;

    // set the basic size
    size.x = x;
    size.y = y;
    size.w = w;
    size.h = h;

    // get the "default" size
    if ( w <= 0 || h <= 0 )
    {
        frect_t tmp;

        ui_Widget_update_bound( pw, &tmp );

        if ( w <= 0 ) size.w = tmp.w;
        if ( h <= 0 ) size.h = tmp.h;
    }

    // store the button position
    pw->vx      = size.x;
    pw->vy      = size.y;
    pw->vwidth  = MAX( 30, size.w );
    pw->vheight = MAX( 30, size.h );

    // update the text position
    ui_Widget_update_text_pos( pw );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_button( ui_Widget_t * pw, float x, float y, float w, float h )
{
    bool_t rv;

    rv = bfalse;
    if ( ui_Widget_set_bound( pw, x, y, w, h ) )
    {
        // tell the button to display
        ui_Widget_DisplayMaskAdd( pw, UI_DISPLAY_BUTTON );

        rv = btrue;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_pos( ui_Widget_t * pw, float x, float y )
{
    float  dx, dy;

    if ( NULL == pw ) return bfalse;

    dx = x - pw->vx;
    dy = y - pw->vy;

    pw->vx = x;
    pw->vy = y;

    return display_list_adjust_bound( pw->tx_lst, dx, dy );
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_id( ui_Widget_t * pw, ui_id_t id )
{
    /// @details BB@>

    if ( NULL == pw ) return bfalse;

    pw->id = id;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_img( ui_Widget_t * pw, oglx_texture_t *img )
{
    if ( NULL == pw ) return bfalse;

    // tell the widget not to dosplay an image
    ui_Widget_DisplayMaskRemove( pw, UI_DISPLAY_IMAGE );

    if ( NULL == img ) return btrue;

    // set the image
    pw->img = img;

    // tell the ui to display the image
    ui_Widget_DisplayMaskAdd( pw, UI_DISPLAY_IMAGE );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_update_text_pos( ui_Widget_t * pw )
{
    float x1, x2, y1, y2;
    float w, h;
    float offset;
    float text_x, text_y;
    frect_t rect;

    if ( NULL == pw ) return bfalse;

    if ( NULL == pw->tx_lst ) return btrue;

    // determine if we need to move the text_ptr over for an image
    offset = 0;
    if ( 0 != ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_IMAGE ) && NULL != pw->img )
    {
        offset = pw->img->imgW * ui_context.aw;
    }

    // convert the virtual coordinates to screen coordinates
    ui_virtual_to_screen_abs( pw->vx, pw->vy, &x1, &y1 );
    ui_virtual_to_screen_abs( pw->vx + pw->vwidth, pw->vy + pw->vheight, &x2, &y2 );
    w = x2 - x1;
    h = y2 - y1;

    display_list_bound( pw->tx_lst, &rect );

    rect.w = MIN( rect.w, w );
    rect.h = MIN( rect.h, h );

    if ( w - ( offset + rect.w ) < 0 )
    {
        // not enough room or everything
        text_x = x1 + offset;
    }
    else
    {
        text_x = ( w - ( offset + rect.w ) ) * 0.5f + ( x1 + offset );
    }

    if ( h - rect.h < 0 )
    {
        text_y = y1;
    }
    else
    {
        text_y = ( h - rect.h ) * 0.5f + y1;
    }

    display_list_adjust_bound( pw->tx_lst, text_x - rect.x, text_y - rect.y );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_free( ui_Widget_t * pw )
{
    if ( NULL == pw ) return bfalse;

    if ( NULL != pw->tx_lst )
    {
        pw->tx_lst = display_list_dtor( pw->tx_lst, btrue );
    }

    memset( pw, 0, sizeof( *pw ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_vtext( ui_Widget_t * pw, TTF_Font * ttf_ptr, int voffset, const char * format, va_list args )
{
    float text_x, text_y;
    int   text_w, text_h;
    float offset;
    int lines = 0;

    if ( NULL == pw ) return bfalse;

    // use the default ui font?
    if ( NULL == ttf_ptr )
    {
        ttf_ptr = ui_getFont();
    }

    // remove any existing text data
    pw->tx_lst = display_list_dtor( pw->tx_lst, btrue );

    // tell the widget not to dosplay text
    ui_Widget_DisplayMaskRemove( pw, UI_DISPLAY_TEXT );

    // Create the image and position it on the right hand side of any image
    if ( NULL != format && '\0' != format[0] )
    {
        const char * text_ptr = NULL;
        float x1, x2, y1, y2;
        float w, h;

        // determine if we need to move the text_ptr over for an image
        offset = 0;
        if ( 0 != ui_Widget_DisplayMaskTest( pw, UI_DISPLAY_IMAGE ) )
        {
            offset = voffset * ui_context.aw;
        }

        // convert the virtual coordinates to screen coordinates
        ui_virtual_to_screen_abs( pw->vx, pw->vy, &x1, &y1 );
        ui_virtual_to_screen_abs( pw->vx + pw->vwidth, pw->vy + pw->vheight, &x2, &y2 );
        w = x2 - x1;
        h = y2 - y1;

        // find the width (x2-x1) & height (y2-y1) of the text_ptr to be drawn,
        // so that it can be centered inside the button
        text_ptr = fnt_vgetTextBoxSize( ttf_ptr, 20, &text_w, &text_h, format, args );

        text_w = MIN( text_w, w );
        text_h = MIN( text_h, h );

        if ( w - ( offset + text_w ) < 0 )
        {
            // not enough room or everything
            text_x = x1 + offset;
        }
        else
        {
            text_x = ( w - ( offset + text_w ) ) * 0.5f + ( x1 + offset );
        }

        if ( h - text_h < 0 )
        {
            text_y = y1;
        }
        else
        {
            text_y = ( h - text_h ) * 0.5f + y1;
        }

        // ensure that we hae a display list
        pw->tx_lst = display_list_ctor( pw->tx_lst, MAX_WIDGET_TEXT );

        // actually convert the text_ptr ( use the text_ptr returned from fnt_vgetTextBoxSize() )
        GL_DEBUG( glColor3f )( 1, 1, 1 );
        lines = fnt_convertTextBox_literal( pw->tx_lst, ttf_ptr, text_x, text_y, 20, text_ptr );

        ui_Widget_update_text_pos( pw );
    }

    // set the visibility of this component
    if ( lines > 0 )
    {
        ui_Widget_DisplayMaskAdd( pw, UI_DISPLAY_TEXT );
    }

    return lines > 0;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_text( ui_Widget_t * pw, TTF_Font * ttf_ptr, int voffset, const char * format, ... )
{
    /// @details BB@> render the text string to a ogl texture

    va_list args;
    bool_t retval;

    // use the default ui font?
    if ( NULL == ttf_ptr )
    {
        ttf_ptr = ui_getFont();
    }

    va_start( args, format );
    retval = ui_Widget_set_vtext( pw, ttf_ptr, voffset, format, args );
    va_end( args );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ui_Widget_set_image( ui_Widget_t * pw, oglx_texture_t * img )
{
    bool_t changed = bfalse;

    if ( NULL == pw ) return bfalse;

    changed = bfalse;
    if ( pw->img != img )
    {
        changed = btrue;
    }

    // get rid of the old image, if necessary
    if ( changed && NULL != pw->img )
    {
        oglx_texture_Release( pw->img );
        pw->img = NULL;

        ui_Widget_DisplayMaskRemove( pw, UI_DISPLAY_IMAGE );
    }

    // set the image
    pw->img = img;

    // turn on the display, if the image exists
    if ( NULL != pw->img )
    {
        ui_Widget_DisplayMaskAdd( pw, UI_DISPLAY_IMAGE );
    }

    return img == pw->img;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ui_virtual_to_screen_abs( float vx, float vy, float * rx, float * ry )
{
    /// @details BB@> convert "virtual" screen positions into "real" space

    *rx = ui_context.aw * vx + ui_context.bw;
    *ry = ui_context.ah * vy + ui_context.bh;
}

//--------------------------------------------------------------------------------------------
void ui_screen_to_virtual_abs( float rx, float ry, float *vx, float *vy )
{
    /// @details BB@> convert "real" mouse positions into "virtual" space

    *vx = ui_context.iaw * rx + ui_context.ibw;
    *vy = ui_context.iah * ry + ui_context.ibh;
}

//--------------------------------------------------------------------------------------------
void ui_virtual_to_screen_rel( float vx, float vy, float * rx, float * ry )
{
    /// @details BB@> convert "virtual" screen positions into "real" space

    *rx = ui_context.aw * vx;
    *ry = ui_context.ah * vy;
}

//--------------------------------------------------------------------------------------------
void ui_screen_to_virtual_rel( float rx, float ry, float *vx, float *vy )
{
    /// @details BB@> convert "real" mouse positions into "virtual" space

    *vx = ui_context.iaw * rx;
    *vy = ui_context.iah * ry;
}

//--------------------------------------------------------------------------------------------
void ui_set_virtual_screen( float vw, float vh, float ww, float wh )
{
    /// @details BB@> set up the ui's virtual screen

    float k;
    TTF_Font * old_defaultFont;

    // define the virtual screen
    ui_context.vw = vw;
    ui_context.vh = vh;
    ui_context.ww = ww;
    ui_context.wh = wh;

    // define the forward transform
    k = MIN( sdl_scr.x / ww, sdl_scr.y / wh );
    ui_context.aw = k;
    ui_context.ah = k;
    ui_context.bw = ( sdl_scr.x - k * ww ) * 0.5f;
    ui_context.bh = ( sdl_scr.y - k * wh ) * 0.5f;

    // define the inverse transform
    ui_context.iaw = 1.0f / ui_context.aw;
    ui_context.iah = 1.0f / ui_context.ah;
    ui_context.ibw = -ui_context.bw * ui_context.iaw;
    ui_context.ibh = -ui_context.bh * ui_context.iah;

    // make sure the font is sized right for the virtual screen
    old_defaultFont = ui_context.defaultFont;
    if ( NULL != ui_context.defaultFont )
    {
        fnt_freeFont( ui_context.defaultFont );
    }
    ui_context.defaultFont = ui_loadFont( ui_context.defaultFontName, ui_context.defaultFontSize );

    // fix the active font. in general, we do not own it, so do not delete
    if ( NULL == ui_context.activeFont || old_defaultFont == ui_context.activeFont )
    {
        ui_context.activeFont = ui_context.defaultFont;
    }
}

//--------------------------------------------------------------------------------------------
TTF_Font * ui_loadFont( const char * font_name, float vpointSize )
{
    float pointSize;

    pointSize = vpointSize * ui_context.aw;

    return fnt_loadFont( font_name, pointSize );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ui_joy_init()
{
    int cnt;

    for ( cnt = 0; cnt < UI_MAX_JOYSTICKS; cnt++ )
    {
        memset( ui_context.joys + cnt, 0, sizeof( ui_control_info_t ) );
    }

    memset( &( ui_context.joy ), 0, sizeof( ui_control_info_t ) );

    memset( &( ui_context.mouse ), 0, sizeof( ui_control_info_t ) );
}

//--------------------------------------------------------------------------------------------
void ui_cursor_update()
{
    int   cnt;

    ui_control_info_t * pctrl = NULL;

    // assume no one is in control
    pctrl = NULL;

    // find the best most controlling joystick
    for ( cnt = 0; cnt < UI_MAX_JOYSTICKS; cnt++ )
    {
        ui_control_info_t * pinfo = ui_context.joys + cnt;

        if ( pinfo->timer <= 0 ) continue;

        if (( NULL == pctrl ) || ( pinfo->timer > pctrl->timer ) )
        {
            pctrl = pinfo;
        }
    }

    // update the ui_context.joy device
    if ( NULL != pctrl )
    {
        ui_context.joy.timer   = pctrl->timer;

        ui_context.joy.vrt[0] += pctrl->vrt[0];
        ui_context.joy.vrt[1] += pctrl->vrt[1];
        ui_virtual_to_screen_abs( ui_context.joy.vrt[0], ui_context.joy.vrt[1], &( ui_context.joy.scr[0] ), &( ui_context.joy.scr[1] ) );

        pctrl = &( ui_context.joy );
    }

    // find out whether the mouse or the joystick is the better controller
    if ( NULL == pctrl )
    {
        pctrl = &( ui_context.mouse );
    }
    else if ( ui_context.mouse.timer >  pctrl->timer )
    {
        pctrl = &( ui_context.mouse );
    }

    if ( NULL != pctrl )
    {
        ui_context.cursor_X = 0.5f * ui_context.cursor_X + 0.5f * pctrl->vrt[0];
        ui_context.cursor_Y = 0.5f * ui_context.cursor_Y + 0.5f * pctrl->vrt[1];
    }

    // decrement the joy timers
    for ( cnt = 0; cnt < UI_MAX_JOYSTICKS; cnt++ )
    {
        ui_control_info_t * pinfo = ui_context.joys + cnt;

        if ( pinfo->timer > 0 )
        {
            pinfo->timer--;
        }
    }

    // decrement the mouse timer
    if ( ui_context.mouse.timer > 0 )
    {
        ui_context.mouse.timer--;
    }

    // decrement the joy timer
    if ( ui_context.joy.timer > 0 )
    {
        ui_context.joy.timer--;
    }

}

//--------------------------------------------------------------------------------------------
bool_t ui_joy_set( SDL_JoyAxisEvent * evt_ptr )
{
    const int   dead_zone = 0x8000 >> 4;
    const float sensitivity = 10.0f;

    ui_control_info_t * pctrl   = NULL;
    bool_t          updated = bfalse;
    int             value   = 0;

    if ( NULL == evt_ptr || SDL_JOYAXISMOTION != evt_ptr->type ) return bfalse;
    value   = evt_ptr->value;

    // check the correct range of the events
    if ( evt_ptr->which >= UI_MAX_JOYSTICKS ) return btrue;
    pctrl = ui_context.joys + evt_ptr->which;

    updated = bfalse;
    if ( evt_ptr->axis < 2 )
    {
        float old_diff, new_diff;

        // make a dead zone
        if ( value > dead_zone ) value -= dead_zone;
        else if ( value < -dead_zone ) value += dead_zone;
        else value = 0;

        // update the info
        old_diff = pctrl->scr[evt_ptr->axis];
        new_diff = ( float )value / ( float )( 0x8000 - dead_zone ) * sensitivity;
        pctrl->scr[evt_ptr->axis] = new_diff;

        updated = ( old_diff != new_diff );
    }

    if ( updated )
    {
        ui_screen_to_virtual_rel( pctrl->scr[0], pctrl->scr[1], &( pctrl->vrt[0] ), &( pctrl->vrt[1] ) );

        pctrl->timer = UI_CONTROL_TIMER;
    }

    return ( NULL != pctrl ) && ( pctrl->timer > 0 );
}
