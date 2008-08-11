//********************************************************************************************
//* Egoboo - Ui.c
//*
//* A basic library for implementing user interfaces, based off of Casey Muratori's IMGUI.
//* (https://mollyrocket.com/forums/viewtopic.php?t=134)
//*
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

#include "Ui.h"

#include "Log.h"

#include "egoboo.h"

#include <string.h>
#include <SDL_opengl.h>
#include <assert.h>

#include "graphic.inl"

GLfloat ui_active_color[]  = {0.00f, 0.00f, 0.90f, 0.60f};
GLfloat ui_hot_color[]     = {0.54f, 0.00f, 0.00f, 1.00f};
GLfloat ui_normal_color[]  = {0.66f, 0.00f, 0.00f, 0.60f};

GLfloat ui_active_color2[] = {0.00f, 0.45f, 0.45f, 0.60f};
GLfloat ui_hot_color2[]    = {0.00f, 0.28f, 0.28f, 1.00f};
GLfloat ui_normal_color2[] = {0.33f, 0.00f, 0.33f, 0.60f};

struct s_ui_Context
{
  // Tracking control focus stuff
  ui_id_t active;
  ui_id_t hot;

  // Basic mouse state
  int mouseX, mouseY;
  int mouseReleased;
  int mousePressed;
  bool_t mouseVisible;

  // truetype font
  TTFont_t *defaultFont;
  TTFont_t *activeFont;

  // bitmapped font
  BMFont_t bmfont;
};

static ui_Context_t ui_context;

//********************************************************************************************
// Core functions
//********************************************************************************************

static ui_Context_t * UiContext_new(ui_Context_t *pui)
{
  //fprintf( stdout, "UiContext_new()\n");

  if(NULL == pui) return pui;

  memset( pui, 0, sizeof( ui_context ) );

  pui->active = pui->hot = UI_Nothing;
  pui->mouseVisible = btrue;

  BMFont_new( &(pui->bmfont) );

  return pui;
};

//--------------------------------------------------------------------------------------------
static bool_t UiContext_delete(ui_Context_t *pui)
{
  //fprintf( stdout, "UiContext_new()\n");

  if(NULL == pui) return bfalse;

  BMFont_delete( &(pui->bmfont) );

  memset( pui, 0, sizeof( ui_context ) );

  return btrue;
};


//--------------------------------------------------------------------------------------------
void ui_Reset()
{
  ui_context.active = ui_context.hot = UI_Nothing;
};


//--------------------------------------------------------------------------------------------
int ui_initialize( const char *default_font, int default_font_size )
{
  UiContext_new( &ui_context );

  ui_context.defaultFont = fnt_loadFont( default_font, default_font_size );

  return 1;
}

//--------------------------------------------------------------------------------------------
void ui_shutdown()
{
  UiContext_delete( &ui_context );
}

//--------------------------------------------------------------------------------------------
bool_t ui_handleSDLEvent( SDL_Event *evt )
{
  bool_t handled = bfalse;

  if ( NULL == evt ) return handled;


  switch ( evt->type )
  {
    case SDL_MOUSEBUTTONDOWN:
      ui_context.mouseReleased = 0;
      ui_context.mousePressed++;
      handled = btrue;
      break;

    case SDL_MOUSEBUTTONUP:
      ui_context.mousePressed = 0;
      ui_context.mouseReleased++;
      handled = btrue;
      break;

    case SDL_MOUSEMOTION:
      ui_context.mouseX = evt->motion.x;
      ui_context.mouseY = evt->motion.y;
      handled = btrue;
      break;

    case SDL_ACTIVEEVENT:
      {
        assert(evt->active.type == SDL_ACTIVEEVENT);

        if( evt->active.state == SDL_APPMOUSEFOCUS )
        {
          ui_context.mouseVisible = (1 == evt->active.gain);
        };
      };
      handled = btrue;
      break;
  }

  return handled;
}

//--------------------------------------------------------------------------------------------
bool_t ui_frame_enabled = bfalse;
int    ui_begin_level = 0;
void ui_beginFrame()
{

  SDL_Surface *screen;

  screen = SDL_GetVideoSurface();

  assert( 0 == ui_begin_level );

  glGetIntegerv( GL_ATTRIB_STACK_DEPTH, &ui_begin_level );
  ATTRIB_PUSH( "ui_beginFrame", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_VIEWPORT_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );

  glShadeModel( GL_FLAT );                                  /* GL_LIGHTING_BIT */

  glDepthMask( GL_FALSE );                                  /* GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT */
  glDisable( GL_DEPTH_TEST );                               /* GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT */
  glDisable( GL_CULL_FACE );                                /* GL_ENABLE_BIT|GL_POLYGON_BIT */
  glEnable( GL_TEXTURE_2D );                                /* GL_ENABLE_BIT|GL_TEXTURE_BIT */

  //glEnable(GL_ALPHA_TEST);                                  /* GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT */
  //glAlphaFunc(GL_GREATER, 0);                               /* GL_COLOR_BUFFER_BIT */

  glEnable( GL_BLEND );                                     /* GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT */
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );      /* GL_COLOR_BUFFER_BIT */

  glViewport( 0, 0, screen->w, screen->h );                 /* GL_VIEWPORT_BIT */

  // Set up an ortho projection for the gui to use.  Controls are free to modify this
  // later, but most of them will need this, so it's done by default at the beginning
  // of a frame
  glMatrixMode( GL_PROJECTION );                            /* GL_TRANSFORM_BIT */
  glPushMatrix();
  glLoadIdentity();
  glOrtho( 0, screen->w, screen->h, 0, -1, 1 );

  glMatrixMode( GL_MODELVIEW );                             /* GL_TRANSFORM_BIT */
  glPushMatrix();
  glLoadIdentity();

  // hotness gets reset at the start of each frame
  ui_context.hot = UI_Nothing;

  ui_frame_enabled = btrue;
}

//--------------------------------------------------------------------------------------------
void ui_endFrame()
{
  GLint ui_end_level;

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  // Re-enable any states disabled by ui_beginFrame
  ATTRIB_POP( "ui_endFrame" );  /* GL_ENABLE_BIT|GL_TRANSFORM_BIT|GL_VIEWPORT_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_POLYGON_BIT|GL_TEXTURE_BIT */


  glGetIntegerv( GL_ATTRIB_STACK_DEPTH, &ui_end_level );
  assert( ui_begin_level == ui_end_level );

  // Clear input states at the end of the frame
  ui_context.mousePressed = ui_context.mouseReleased = 0;

  ui_frame_enabled = bfalse;

  ui_doCursor();
}

//********************************************************************************************
// Utility functions
//********************************************************************************************

int ui_mouseInside( int x, int y, int width, int height )
{
  int right, bottom;
  right = x + width;
  bottom = y + height;

  if ( x <= ui_context.mouseX && y <= ui_context.mouseY && ui_context.mouseX <= right && ui_context.mouseY <= bottom )
  {
    return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------------------------
void ui_setactive( ui_Widget_t * pw )
{
  if ( NULL == pw )
  {
    ui_context.active = UI_Nothing;
  }
  else
  {
    ui_context.active = pw->id;

    pw->timeout = SDL_GetTicks() + 100;
    if ( 0 != ( pw->mask & UI_BITS_CLICKED ) )
    {
      // use exclusive or to flip the bit
      pw->state ^= UI_BITS_CLICKED;
    };
  };
}

//--------------------------------------------------------------------------------------------
void ui_sethot( ui_Widget_t * pw )
{
  if ( NULL == pw )
  {
    ui_context.hot = UI_Nothing;
  }
  else if (( ui_context.active == pw->id || ui_context.active == UI_Nothing ) )
  {
    if ( pw->timeout < SDL_GetTicks() )
    {
      pw->timeout = SDL_GetTicks() + 100;

      if ( 0 != ( pw->mask & UI_BITS_MOUSEOVER ) && ui_context.hot != pw->id )
      {
        // use exclusive or to flip the bit
        pw->state ^= UI_BITS_MOUSEOVER;
      };
    };

    // Only allow hotness to be set if this control, or no control is active
    ui_context.hot = pw->id;
  }
}

//--------------------------------------------------------------------------------------------
TTFont_t* ui_getTTFont()
{
  return ( NULL != ui_context.activeFont ) ? ui_context.activeFont : ui_context.defaultFont;
}

//--------------------------------------------------------------------------------------------
BMFont_t* ui_getBMFont()
{
  BMFont_t * ret = NULL;

  if( INVALID_TEXTURE != ui_context.bmfont.tex.textureID )
  {
    ret = &(ui_context.bmfont);
  }

  return ret;
};

//--------------------------------------------------------------------------------------------
bool_t ui_load_BMFont( char* szBitmap, char* szSpacing )
{
  return BMFont_load( &(ui_context.bmfont), gfxState.scry, szBitmap, szSpacing );
};

//--------------------------------------------------------------------------------------------
bool_t ui_copyWidget( ui_Widget_t * pw2, ui_Widget_t * pw1 )
{
  if ( NULL == pw2 || NULL == pw1 ) return bfalse;
  return NULL != memcpy( pw2, pw1, sizeof( ui_Widget_t ) );
};

//--------------------------------------------------------------------------------------------
bool_t ui_shrinkWidget( ui_Widget_t * pw2, ui_Widget_t * pw1, int pixels )
{
  if ( NULL == pw2 || NULL == pw1 ) return bfalse;

  if ( !ui_copyWidget( pw2, pw1 ) ) return bfalse;

  pw2->x += pixels;
  pw2->y += pixels;
  pw2->width  -= 2 * pixels;
  pw2->height -= 2 * pixels;

  if ( pw2->width < 0 )  pw2->width   = 0;
  if ( pw2->height < 0 ) pw2->height = 0;

  return pw2->width > 0 && pw2->height > 0;
};

//--------------------------------------------------------------------------------------------
bool_t ui_initWidget( ui_Widget_t * pw, ui_id_t id, TTFont_t * pfont, const char *text, GLtexture *img, int x, int y, int width, int height )
{
  if ( NULL == pw ) return bfalse;

  pw->id      = id;
  pw->pfont   = pfont;
  pw->text    = text;
  pw->img     = img;
  pw->x       = x;
  pw->y       = y;
  pw->width   = width;
  pw->height  = height;
  pw->state   = 0;
  pw->mask    = 0;
  pw->timeout = 0;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t ui_widgetAddMask( ui_Widget_t * pw, Uint32 mbits )
{
  if ( NULL == pw ) return bfalse;

  pw->mask  |= mbits;
  pw->state &= ~mbits;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_widgetRemoveMask( ui_Widget_t * pw, Uint32 mbits )
{
  if ( NULL == pw ) return bfalse;

  pw->mask  &= ~mbits;
  pw->state &= ~mbits;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_widgetSetMask( ui_Widget_t * pw, Uint32 mbits )
{
  if ( NULL == pw ) return bfalse;

  pw->mask   = mbits;
  pw->state &= ~mbits;

  return btrue;
}


//********************************************************************************************
// Behaviors
//********************************************************************************************

ui_buttonValues ui_buttonBehavior( ui_Widget_t * pWidget )
{
  ui_buttonValues result = BUTTON_NOCHANGE;

  // If the mouse is over the button, try and set hotness so that it can be clicked
  if ( ui_mouseInside( pWidget->x, pWidget->y, pWidget->width, pWidget->height ) )
  {
    ui_sethot( pWidget );
  }

  // Check to see if the button gets clicked on
  if ( ui_context.active == pWidget->id )
  {
    if ( ui_context.mouseReleased == 1 )
    {
      // mouse button up
      if ( ui_context.active == pWidget->id ) result = BUTTON_UP;

      ui_setactive( NULL );
    }
  }
  else if ( ui_context.hot == pWidget->id )
  {
    if ( ui_context.mousePressed == 1 )
    {
      // mouse button down
      if ( ui_context.hot == pWidget->id ) result = BUTTON_DOWN;

      ui_setactive( pWidget );
    }
  }

  return result;
}

//********************************************************************************************
// Drawing
//********************************************************************************************

void ui_drawButton( ui_Widget_t * pWidget )
{
  bool_t bactive, bhot;
  if ( !ui_frame_enabled ) return;

  // Draw the button
  ATTRIB_PUSH( "ui_drawButton", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT );
  {
    glDisable( GL_TEXTURE_2D );                                /* GL_ENABLE_BIT|GL_TEXTURE_BIT */

    bactive = ui_context.active == pWidget->id && ui_context.hot == pWidget->id;
    bactive = bactive || 0 != ( pWidget->mask & pWidget->state & UI_BITS_CLICKED );
    bhot    = ui_context.hot == pWidget->id;
    bhot    = bhot || 0 != ( pWidget->mask & pWidget->state & UI_BITS_MOUSEOVER );

    if ( 0 != pWidget->mask )
    {
      if ( bactive )
      {
        glColor4fv( ui_normal_color2 );
      }
      else if ( bhot )
      {
        glColor4fv( ui_hot_color );
      }
      else
        glColor4fv( ui_normal_color );
    }
    else
    {
      if ( bactive )
      {
        glColor4fv( ui_active_color );
      }
      else if ( bhot )
      {
        glColor4fv( ui_hot_color );
      }
      else
        glColor4fv( ui_normal_color );
    }


    glBegin( GL_QUADS );
    glVertex2i( pWidget->x, pWidget->y );
    glVertex2i( pWidget->x, pWidget->y + pWidget->height );
    glVertex2i( pWidget->x + pWidget->width, pWidget->y + pWidget->height );
    glVertex2i( pWidget->x + pWidget->width, pWidget->y );
    glEnd();
  }
  ATTRIB_POP( "ui_drawButton" );
}

//--------------------------------------------------------------------------------------------
void ui_drawImage( ui_Widget_t * pWidget )
{
  int w, h;
  float x1, y1;

  if ( NULL == pWidget->img || !ui_frame_enabled ) return;

  ATTRIB_PUSH( "ui_drawImage", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT );
  {

    if ( pWidget->width == 0 || pWidget->height == 0 )
    {
      w = GLtexture_GetImageWidth( pWidget->img );
      h = GLtexture_GetImageHeight( pWidget->img );
    }
    else
    {
      w = pWidget->width;
      h = pWidget->height;
    }

    x1 = ( GLfloat ) GLtexture_GetImageWidth( pWidget->img )  / ( GLfloat ) GLtexture_GetTextureWidth( pWidget->img );
    y1 = ( GLfloat ) GLtexture_GetImageHeight( pWidget->img ) / ( GLfloat ) GLtexture_GetTextureHeight( pWidget->img );

    // Draw the image
    GLtexture_Bind( pWidget->img, &gfxState );
    glColor4f( 1, 1, 1, GLtexture_GetAlpha( pWidget->img ) );

    glBegin( GL_QUADS );
    glTexCoord2f( 0,  0 ); glVertex2i( pWidget->x,     pWidget->y );
    glTexCoord2f( 0, y1 ); glVertex2i( pWidget->x,     pWidget->y + h );
    glTexCoord2f( x1, y1 ); glVertex2i( pWidget->x + w, pWidget->y + h );
    glTexCoord2f( x1,  0 ); glVertex2i( pWidget->x + w, pWidget->y );
    glEnd();
  };

  ATTRIB_POP( "ui_drawImage" );
}


//--------------------------------------------------------------------------------------------
/** ui_drawTextBox
 * Draws a pWidget->text string into a box, splitting it into lines according to newlines in the string.
 *
 * pWidget->text    - The pWidget->text to draw
 * pWidget->x       - The pWidget->x position to start drawing at
 * pWidget->y       - The pWidget->y position to start drawing at
 * pWidget->width   - Maximum pWidget->width of the box (not implemented)
 * pWidget->height  - Maximum pWidget->height of the box (not implemented)
 * spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void ui_drawTextBox( ui_Widget_t * pWidget, int spacing )
{
  TTFont_t *font = ui_getTTFont();
  fnt_drawTextBox( font, pWidget->text, pWidget->x, pWidget->y, pWidget->width, pWidget->height, spacing );
}

//********************************************************************************************
// Controls
//********************************************************************************************

ui_buttonValues ui_doButton( ui_Widget_t * pWidget )
{
  ui_buttonValues result;
  int text_w, text_h;
  int text_x, text_y;
  TTFont_t *font;

  // Do all the logic type work for the button
  result = ui_buttonBehavior( pWidget );

  // Draw the button part of the button
  ui_drawButton( pWidget );

  // And then draw the pWidget->text that goes on top of the button
  font = pWidget->pfont;
  if(NULL == font) { font = ui_getTTFont(); };

  if ( NULL != font && NULL != pWidget->text )
  {
    // find the pWidget->width & pWidget->height of the pWidget->text to be drawn, so that it can be centered inside
    // the button

    fnt_getTextBoxSize( font, pWidget->text, 20, &text_w, &text_h );

    text_w = MIN(text_w, pWidget->width );
    text_h = MIN(text_h, pWidget->height);

    text_x = ( pWidget->width  - text_w ) / 2 + pWidget->x;
    text_y = ( pWidget->height - text_h ) / 2 + pWidget->y;

    glColor3f( 1, 1, 1 );
    fnt_drawTextBox( font, pWidget->text, text_x, text_y, text_w, text_h, 20 );
  }

  return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doImageButton( ui_Widget_t * pWidget )
{
  ui_buttonValues result;
  ui_Widget_t wtmp;

  // Do all the logic type work for the button
  result = ui_buttonBehavior( pWidget );

  // Draw the button part of the button
  ui_drawButton( pWidget );

  // And then draw the image on top of it
  glColor3f( 1, 1, 1 );
  if ( NULL != pWidget->img )
  {
    ui_shrinkWidget( &wtmp, pWidget, 5 );
    ui_drawImage( &wtmp );
  };

  return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doImageButtonWithText( ui_Widget_t * pWidget )
{
  ui_buttonValues result;
  TTFont_t *font;
  int text_x, text_y;
  int text_w, text_h;
  ui_Widget_t wtmp;

  // Do all the logic type work for the button
  result = ui_buttonBehavior( pWidget );

  // Draw the button part of the button
  ui_drawButton( pWidget );

  // Draw the image part
  glColor3f( 1, 1, 1 );
  ui_shrinkWidget( &wtmp, pWidget, 5 );
  wtmp.width = wtmp.height;
  ui_drawImage( &wtmp );

  // And draw the pWidget->text next to the image
  // And then draw the pWidget->text that goes on top of the button
  font = ui_getTTFont();
  if ( font )
  {
    // find the pWidget->width & pWidget->height of the pWidget->text to be drawn, so that it can be centered inside
    // the button
    fnt_getTextSize( font, pWidget->text, &text_w, &text_h );

    text_x = GLtexture_GetImageWidth( pWidget->img ) + 5 + pWidget->x;
    text_y = ( pWidget->height - text_h ) / 2 + pWidget->y;

    glColor3f( 1, 1, 1 );
    fnt_drawText( font, text_x, text_y, pWidget->text );
  }

  return result;
}


//--------------------------------------------------------------------------------------------
void ui_doCursor()
{
  // BB > Use the "normal" Egoboo cursor drawing routines to handle the cursor
  //      Turn off the mouse as a Game Controller while the menu is on.
  // !!!! This is a pretty ugly patch !!!!

  if(!ui_context.mouseVisible)
    return;

  // must use Begin2DMode() and BeginText() to get OpenGL in the right state
  Begin2DMode();
  BeginText( &(ui_context.bmfont.tex) );
  {
    BMFont_draw_one( &(ui_context.bmfont), 95, ui_context.mouseX - 5, ui_context.mouseY - 7 );
  };
  EndText();
  End2DMode();
};

//--------------------------------------------------------------------------------------------
int ui_getMouseX() { return ui_context.mouseX; };

//--------------------------------------------------------------------------------------------
int ui_getMouseY() { return ui_context.mouseY; };
