/* Egoboo - graphic.h
 *
 *
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
#include "Font.h"
#include "mesh.h"
#include "Menu.h"
#include "game.h"

#include "egoboo_types.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define TABAND              31                      // Tab size

#define TRANSCOLOR                      0           // Transparent color

//--------------------------------------------------------------------------------------------

struct ConfigData_t;
struct CGame_t;

//--------------------------------------------------------------------------------------------
struct renderlist_t
{
  int     num_totl;                            // Number to render, total
  Uint32  totl[MAXMESHRENDER];                 // List of which to render, total

  int     num_shine;                           // ..., reflective
  Uint32  shine[MAXMESHRENDER];                // ..., reflective

  int     num_reflc;                           // ..., has reflection
  Uint32  reflc[MAXMESHRENDER];                // ..., has reflection

  int     num_norm;                            // ..., not reflective, has no reflection
  Uint32  norm[MAXMESHRENDER];                 // ..., not reflective, has no reflection

  int     num_watr;                            // ..., water
  Uint32  watr[MAXMESHRENDER];                 // ..., water
  Uint32  watr_mode[MAXMESHRENDER];
};
typedef struct renderlist_t RENDERLIST;

//--------------------------------------------------------------------------------------------
// Global lighting stuff
struct global_lighting_info_t
{
  bool_t on;
  float  spek;
  vect3  spekdir;
  vect3  spekcol;
  float  ambi;
  vect3  ambicol;
};
typedef struct global_lighting_info_t GLOBAL_LIGHTING_INFO;

extern GLOBAL_LIGHTING_INFO GLight;

//--------------------------------------------------------------------------------------------
struct CGraphics_t
{
  egoboo_key ekey;

  // JF - Added so that the video mode might be determined outside of the graphics code
  SDL_Surface * surface;

  bool_t        texture_on;
  SDL_GrabMode  GrabMouse;
  bool_t        HideMouse;

  STRING        szDriver;                   // graphics driver name;

  bool_t        fullscreen;                 // current value of the fullscreen
  bool_t        gfxacceleration;            // current value of the gfx acceleration

  int           colordepth;
  int           scrd;                       // current screen bit depth
  int           scrx;                       // current screen X size
  int           scry;                       // current screen Y size
  int           scrz;                       // current screen z-buffer depth ( 8 unsupported )

  bool_t        antialiasing;               // current antialiasing value
  int           texturefilter;              // current texture filter
  GLenum        perspective;                // current correction hint
  bool_t        dither;                     // current dithering flag
  GLenum        shading;                    // current shading type
  bool_t        vsync;                      // current vsync flag
  bool_t        phongon;                    // current phongon flag
  
  bool_t        render_overlay;              // current overlayvalid flag
  bool_t        render_background;           // current overlayvalid flag
  bool_t        render_fog;                  // current fogallowed flag

  float         maxAnisotropy;                     // Max anisotropic filterings (Between 1.00 and 16.00)
  int           userAnisotropy;                    // Requested anisotropic level
  int           log2Anisotropy;                    // Max levels of anisotropy

  SDL_Rect **   video_mode_list;

  RENDERLIST *  rnd_lst;

  float         est_max_fps;

  // the game state that we are plugged into
  struct CGame_t  * gs;

};
typedef struct CGraphics_t CGraphics;

CGraphics * CGraphics_new(CGraphics * g, struct ConfigData_t * cd);
bool_t      CGraphics_synch(CGraphics * g, struct ConfigData_t * cd);

extern CGraphics gfxState;

//--------------------------------------------------------------------------------------------
struct ClockState_t;

typedef struct CGui_t 
{
  egoboo_key ekey;

  MenuProc mnu_proc;

  bool_t can_pause;          //Pause button avalible?

  GLtexture TxBars;                                         /* status bars */
  GLtexture TxBlip;                                         /* you are here texture */

  struct ClockState_t * clk;
  float                 dUpdate;

  MessageQueue msgQueue;
} CGui;

CGui * gui_getState();
bool_t CGui_shutDown();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct mod_data_t;
struct CChr_t;

bool_t glinit(  CGraphics * g, struct ConfigData_t * cd  );
bool_t gfx_initialize(CGraphics * g, struct ConfigData_t * cd);

int draw_string( BMFont * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... );
bool_t draw_texture_box( GLtexture * ptx, FRect * tx_rect, FRect * sc_rect );

void BeginText( GLtexture * pfnt );
void EndText( void );

void Begin2DMode( void );
void End2DMode( void );

INLINE const bool_t bbox_gl_draw(AA_BBOX * pbbox);

bool_t gfx_set_mode(CGraphics * g);
bool_t gfx_find_anisotropy( CGraphics * g );

bool_t make_renderlist(RENDERLIST * prlst);

bool_t query_clear();
bool_t query_pageflip();
bool_t request_pageflip();
bool_t do_pageflip();
bool_t do_clear();

void md2_blend_vertices(struct CChr_t * pchr, Sint32 vrtmin, Sint32 vrtmax);
void md2_blend_lighting(struct CChr_t * pchr);

void prime_icons( struct CGame_t * gs);