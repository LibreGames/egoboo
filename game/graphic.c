//********************************************************************************************
//* Egoboo - graphic.c
//*
//* A mish-mosh of code related to drawing the game.
//* Some code, such as data loading, should be moved elsewhere.
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

#include "graphic.inl"

#include "Log.h"
#include "Ui.h"
#include "Menu.h"
#include "camera.h"
#include "script.h"
#include "passage.h"
#include "Client.h"
#include "Server.h"
#include "sound.h"
#include "file_common.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "Network.inl"
#include "input.inl"
#include "char.inl"
#include "particle.inl"
#include "input.inl"
#include "mesh.inl"
#include "Physics.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"

#include <assert.h>
#include <stdarg.h>
#include <time.h>

#ifdef __unix__
#include <unistd.h>
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct sGame;

RENDERLIST         renderlist;
Graphics_t         gfxState;
static bool_t      gfx_initialized = bfalse;

//--------------------------------------------------------------------------------------------
struct sStatus;
struct s_egoboo_video_parameters;

static int  draw_wrap_string( BMFont_t * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... );
static int  draw_status( BMFont_t *  pfnt , struct sStatus * pstat );
static void draw_text( BMFont_t *  pfnt  );

static SDL_Surface * RequestVideoMode( struct s_egoboo_video_parameters * v );

void render_particles();

//--------------------------------------------------------------------------------------------
bool_t gfx_find_anisotropy( Graphics_t * g )
{
  // BB> get the maximum anisotropy supported by the video vard
  //     OpenGL and SDL must be loaded for this to work.

  if(NULL == g) return bfalse;

  g->maxAnisotropy  = 0;
  g->log2Anisotropy = 0;
  g->texturefilter  = TX_TRILINEAR_2;

  if(!SDL_WasInit(SDL_INIT_VIDEO) || NULL ==g->surface) return bfalse;

  glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &(g->maxAnisotropy) );
  g->log2Anisotropy = ( g->maxAnisotropy == 0 ) ? 0 : floor( log( g->maxAnisotropy + 1e-6 ) / log( 2.0f ) );

  if ( g->maxAnisotropy == 0.0f && g->texturefilter >= TX_ANISOTROPIC )
  {
    g->texturefilter = TX_TRILINEAR_2;
  }

  return btrue;
}

bool_t gfx_initialize(Graphics_t * g, ConfigData_t * cd)
{
  if(NULL == g || NULL == cd) return bfalse;

  if(gfx_initialized) return btrue;

  // set the graphics state
  Graphics_new(g, cd);
  g->rnd_lst = &renderlist;
  gfx_find_anisotropy(g);
  gfx_initialized = btrue;

  return btrue;
}

void EnableTexturing()
{
  //if ( !gfxState.texture_on )
  //{
  //  glEnable( GL_TEXTURE_2D );
  //  gfxState.texture_on = btrue;
  //}
}

void DisableTexturing()
{
  //if ( gfxState.texture_on )
  //{
  //  glDisable( GL_TEXTURE_2D );
  //  gfxState.texture_on = bfalse;
  //}
}



//TODO: This needs work
static GLint threeDmode_begin_level = 0;
void Begin3DMode()
{
  assert( 0 == threeDmode_begin_level );

  ATTRIB_GUARD_OPEN( threeDmode_begin_level );
  ATTRIB_PUSH( "Begin3DMode", GL_TRANSFORM_BIT | GL_CURRENT_BIT );

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadMatrixf( GCamera.mProjection.v );

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadMatrixf( GCamera.mView.v );

  glColor4f( 1, 1, 1, 1 );
}

void End3DMode()
{
  GLint threeDmode_end_level;

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  ATTRIB_POP( "End3DMode" );

  ATTRIB_GUARD_CLOSE( threeDmode_begin_level, threeDmode_end_level );
  threeDmode_begin_level = 0;
}

/********************> Begin2DMode() <*****/
static GLint twoD_begin_level = 0;

void Begin2DMode( void )
{
  assert( 0 == twoD_begin_level );

  ATTRIB_GUARD_OPEN( twoD_begin_level );
  ATTRIB_PUSH( "Begin2DMode", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_TRANSFORM_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();         // Reset The Projection Matrix
  glOrtho( 0, gfxState.scrx, 0, gfxState.scry, 1, -1 );   // Set up an orthogonal projection

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();

  glDisable( GL_DEPTH_TEST );

  glColor4f( 1, 1, 1, 1 );
};

/********************> End2DMode() <*****/
void End2DMode( void )
{
  GLint twoD_end_level;

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  ATTRIB_POP( "End2DMode" );

  ATTRIB_GUARD_CLOSE( twoD_begin_level, twoD_end_level );
  twoD_begin_level = 0;
}

//--------------------------------------------------------------------------------------------
static GLint text_begin_level = 0;
void BeginText( GLtexture * pfnt )
{
  assert( 0 == text_begin_level );

  ATTRIB_GUARD_OPEN( text_begin_level );
  ATTRIB_PUSH( "BeginText", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );

  GLtexture_Bind( pfnt, &gfxState );

  glEnable( GL_ALPHA_TEST );
  glAlphaFunc( GL_GREATER, 0 );

  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  glDisable( GL_DEPTH_TEST );
  glDisable( GL_CULL_FACE );

  glColor4f( 1, 1, 1, 1 );
}

//--------------------------------------------------------------------------------------------
void EndText()
{
  GLint text_end_level;

  ATTRIB_POP( "EndText" );

  ATTRIB_GUARD_CLOSE( text_begin_level, text_end_level );
  text_begin_level = 0;
}



//---------------------------------------------------------------------------------------------
void release_all_textures(Game_t * gs)
{
  // ZZ> This function clears out all of the textures

  int cnt;

  for ( cnt = 0; cnt < MAXTEXTURE; cnt++ )
  {
    GLtexture_Release( gs->TxTexture + cnt );
  }
}

//--------------------------------------------------------------------------------------------
Uint32 load_one_icon( Game_t * gs, char * szPathname, const char * szObjectname, char * szFilename )
{
  // ZZ> This function is used to load an icon.  Most icons are loaded
  //     without this function though...

  Uint32 retval = MAXICONTX;

  if ( INVALID_TEXTURE != GLtexture_Load( GL_TEXTURE_2D,  gs->TxIcon + gs->TxIcon_count,  inherit_fname(szPathname, szObjectname, szFilename), INVALID_KEY ) )
  {
    retval = gs->TxIcon_count;
    gs->TxIcon_count++;
  }

  return retval;
}

//---------------------------------------------------------------------------------------------
void mnu_prime_titleimage(MenuProc_t * mproc)
{
  // ZZ> This function sets the title image pointers to NULL

  mnu_free_all_titleimages(mproc);

  if(NULL != mproc->sv)
  {
    ModInfo_clear_all_titleimages( mproc->sv->loc_mod, MAXMODULE );
  }

  if(NULL != mproc->cl)
  {
    ModInfo_clear_all_titleimages( mproc->cl->rem_mod, MAXMODULE );
  }

}

//---------------------------------------------------------------------------------------------
void prime_icons(Game_t * gs)
{
  // ZZ> This function sets the icon pointers to NULL

  int cnt;

  gs->TxIcon_count = 0;
  for ( cnt = 0; cnt < MAXICONTX; cnt++ )
  {
    gs->TxIcon[cnt].textureID = INVALID_TEXTURE;
    gs->skintoicon[cnt] = MAXICONTX;
  }

  iconrect.left = 0;
  iconrect.right = 32;
  iconrect.top = 0;
  iconrect.bottom = 32;

  gs->ico_lst[ICO_NULL] = MAXICONTX;
  gs->ico_lst[ICO_KEYB] = MAXICONTX;
  gs->ico_lst[ICO_MOUS] = MAXICONTX;
  gs->ico_lst[ICO_JOYA] = MAXICONTX;
  gs->ico_lst[ICO_JOYB] = MAXICONTX;
  gs->ico_lst[ICO_BOOK_0] = MAXICONTX;
}

//---------------------------------------------------------------------------------------------
void release_all_icons(Game_t * gs)
{
  // ZZ> This function clears out all of the icons

  int cnt;

  for ( cnt = 0; cnt < MAXICONTX; cnt++ )
  {
    GLtexture_Release( gs->TxIcon + cnt );
    gs->TxIcon[cnt].textureID = INVALID_TEXTURE;
    gs->skintoicon[cnt]       = MAXICONTX;
  }
  gs->TxIcon_count = 0;
}

//---------------------------------------------------------------------------------------------
void init_all_models(Game_t * gs)
{
  // ZZ> This function initializes the models

  CapList_delete(gs);
  PipList_delete(gs);
  MadList_delete(gs);
}

//---------------------------------------------------------------------------------------------
void release_all_models(Game_t * gs)
{
  // ZZ> This function clears out all of the models

  CapList_delete(gs);
  MadList_delete(gs);
}

//--------------------------------------------------------------------------------------------
static bool_t write_debug_message( int time, const char *format, va_list args )
{
  // ZZ> This function sticks a message in the display queue and sets its timer

  STRING buffer;

  Gui_t            * gui = gui_getState();
  MessageQueue_t    * mq  = &(gui->msgQueue);
  MESSAGE_ELEMENT * msg;

  int slot = MessageQueue_get_free(mq);
  if(CData.maxmessage == slot) return bfalse;

  msg = mq->list + slot;

  // print the formatted messafe into the buffer
  vsnprintf( buffer, sizeof( buffer ) - 1, format, args );

  // Copy the message
  strncpy( msg->textdisplay, buffer, sizeof( msg->textdisplay ) );
  msg->time = time * DELAY_MESSAGE;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t debug_message( int time, const char *format, ... )
{
  bool_t retval;
  va_list args;

  if( !VALID_CSTR(format) ) return bfalse;

  va_start( args, format );
  retval = write_debug_message( time, format, args );
  va_end( args );

  return retval;
};


//--------------------------------------------------------------------------------------------
void reset_end_text( Game_t * gs )
{
  // ZZ> This function resets the end-module text

  // blank the text
  gs->endtext[0] = EOS;

  if ( gs->PlaList_count == 0 )
  {
    // No players???
    snprintf( gs->endtext, sizeof( gs->endtext ), "The game has ended..." );
  }
  else if ( gs->PlaList_count == 1 )
  {
    // One player
    snprintf( gs->endtext, sizeof( gs->endtext ), "Sadly, no trace was ever found..." );
  }
  else if ( gs->PlaList_count > 1 )
  {
    snprintf( gs->endtext, sizeof( gs->endtext ), "Sadly, they were never heard from again..." );
  }

}

//--------------------------------------------------------------------------------------------
void append_end_text( Game_t * gs, int message, CHR_REF chr_ref )
{
  // ZZ> This function appends a message to the end-module text

  char * message_src;

  if ( message < gs->MsgList.total ) return;

  message_src = gs->MsgList.text + gs->MsgList.index[message];
  decode_escape_sequence(gs, gs->endtext, sizeof(gs->endtext), message_src, chr_ref);
}

//--------------------------------------------------------------------------------------------
void make_textureoffset( void )
{
  // ZZ> This function sets up for moving textures

  int cnt;
  for ( cnt = 0; cnt < 256; cnt++ )
  {
    textureoffset[cnt] = FP8_TO_FLOAT( cnt );
  }
}

//--------------------------------------------------------------------------------------------
void figure_out_what_to_draw()
{
  // ZZ> This function determines the things that need to be drawn

  // Make the render list for the mesh
  make_renderlist(&renderlist);

  GCamera.turn_lr_one   = GCamera.turn_lr / (float)(1<<16);

  // Request matrices needed for local machine
  dolist_make();
  dolist_sort();
}

//--------------------------------------------------------------------------------------------
void animate_tiles( TILE_ANIMATED * t, float dUpdate )
{
  // This function changes the animated tile frame

  t->framefloat += dUpdate / ( float ) t->updateand;
  while ( t->framefloat >= 1.0f )
  {
    t->framefloat -= 1.0f;
    t->frameadd = ( t->frameadd + 1 ) & t->frameand;
  };
}

//--------------------------------------------------------------------------------------------
bool_t load_particle_texture( Game_t * gs, const char *szModPath  )
{
  // BB> Load the particle bitmap. Check the gamedat dir first for a module override

  STRING szTemp;
  ConfigData_t * cd;

  if( !EKEY_PVALID(gs) ) return bfalse;

  cd = gs->cd;
  if(NULL == cd) cd = &CData;

  // release any old texture
  if(MAXTEXTURE != particletexture)
  {
    GLtexture_delete( gs->TxTexture + particletexture );
    particletexture = MAXTEXTURE;
  }

  if( VALID_CSTR(szModPath) )
  {
    // try to load it from the module's gamedat dir
    if( MAXTEXTURE == particletexture )
    {
      snprintf( szTemp, sizeof( szTemp ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, "particle.bmp" );
      if ( INVALID_TEXTURE != GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_PARTICLE, szTemp, TRANSCOLOR ) )
      {
        particletexture = TX_PARTICLE;
      }
    }

    if( MAXTEXTURE == particletexture )
    {
      snprintf( szTemp, sizeof( szTemp ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, "particle.png" );
      if ( INVALID_TEXTURE != GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_PARTICLE, szTemp, TRANSCOLOR ) )
      {
        particletexture = TX_PARTICLE;
      }
    }

    if( MAXTEXTURE == particletexture )
    {
      snprintf( szTemp, sizeof( szTemp ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->particle_bitmap );
      if ( INVALID_TEXTURE != GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_PARTICLE, szTemp, TRANSCOLOR ) )
      {
        particletexture = TX_PARTICLE;
      }
    }
  }

  // load the default
  if( MAXTEXTURE == particletexture )
  {
    snprintf( szTemp, sizeof( szTemp ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->globalparticles_dir, cd->particle_bitmap );
    if ( INVALID_TEXTURE != GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_PARTICLE, szTemp, TRANSCOLOR ) )
    {
      particletexture = TX_PARTICLE;
    }
  }


  if( MAXTEXTURE == particletexture )
  {
    log_warning( "!!!!Particle bitmap could not be found!!!! Missing File = \"%s\"\n", szTemp );
  };

  return (MAXTEXTURE != particletexture);
}


//--------------------------------------------------------------------------------------------
bool_t load_basic_textures( Game_t * gs, const char *szModPath )
{
  // ZZ> This function loads the standard textures for a module
  // BB> In each case, try to load one stored with the module first.

  ConfigData_t * cd;

  if( !EKEY_PVALID(gs) ) return bfalse;

  cd = gs->cd;
  if(NULL == cd) cd = cd;

  load_particle_texture( gs, szModPath );

  // Module background tiles
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->tile0_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_0, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->tile0_bitmap );
    if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_0, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 0 could not be found. Missing File = \"%s\"\n", cd->tile0_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->tile1_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,   gs->TxTexture + TX_TILE_1, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->tile1_bitmap );
    if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_1, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 1 could not be found. Missing File = \"%s\"\n", cd->tile1_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->tile2_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_2, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->tile2_bitmap );
    if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_2, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 2 could not be found. Missing File = \"%s\"\n", cd->tile2_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->tile3_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_3, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->tile3_bitmap );
    if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_3, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 3 could not be found. Missing File = \"%s\"\n", cd->tile3_bitmap );
    }
  };


  // Water textures
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->watertop_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_TOP, CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->watertop_bitmap );
    if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_TOP, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 1 could not be found. Missing File = \"%s\"\n", cd->watertop_bitmap );
    }
  };

  // This is also used as far background
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->waterlow_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_LOW, CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->waterlow_bitmap );
    if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_LOW, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 0 could not be found. Missing File = \"%s\"\n", cd->waterlow_bitmap );
    }
  };


  // BB > this is handled differently now and is not needed
  // Texture 7 is the phong map
  //snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s" SLASH_STRING "%s", szModPath, cd->gamedat_dir, cd->phong_bitmap);
  //if(INVALID_TEXTURE==GLtexture_Load(GL_TEXTURE_2D,  gs->TxTexture + TX_PHONG, CStringTmp1, INVALID_KEY))
  //{
  //  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->phong_bitmap);
  //  GLtexture_Load(GL_TEXTURE_2D,  gs->TxTexture + TX_PHONG, CStringTmp1, TRANSCOLOR );
  //  {
  //    log_warning("Phong Bitmap Layer 1 could not be found. Missing File = \"%s\"\n", cd->phong_bitmap);
  //  }
  //};

  return btrue;

}

//--------------------------------------------------------------------------------------------
bool_t load_bars( char* szBitmap )
{
  // ZZ> This function loads the status bar bitmap
  Gui_t * gui = gui_getState();
  int cnt;

  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D, &gui->TxBars, szBitmap, 0 ) )
  {
    return bfalse;
  }


  // Make the blit rectangles
  for ( cnt = 0; cnt < NUMBAR; cnt++ )
  {
    tabrect[cnt].left = 0;
    tabrect[cnt].right = TABX;
    tabrect[cnt].top = cnt * BARY;
    tabrect[cnt].bottom = ( cnt + 1 ) * BARY;
    barrect[cnt].left = TABX;
    barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
    barrect[cnt].top = tabrect[cnt].top;
    barrect[cnt].bottom = tabrect[cnt].bottom;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
void load_map( Game_t * gs, char* szModule )
{
  // ZZ> This function loads the map bitmap and the blip bitmap

  // Turn it all off
  mapon = bfalse;
  youarehereon = bfalse;
  numblip = 0;

  // Load the images
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModule, CData.gamedat_dir, CData.plan_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D, &gs->TxMap, CStringTmp1, INVALID_KEY ) )
    log_warning( "Cannot load map: %s\n", CStringTmp1 );

  // Set up the rectangles
  mapscale = MIN(( float ) gfxState.scrx / (float)DEFAULT_SCREEN_W, ( float ) gfxState.scry / (float)DEFAULT_SCREEN_H );
  maprect.left   = 0;
  maprect.right  = MAPSIZE * mapscale;
  maprect.top    = gfxState.scry - MAPSIZE * mapscale;
  maprect.bottom = gfxState.scry;

}

//--------------------------------------------------------------------------------------------
bool_t make_water(WATER_INFO * wi)
{
  // ZZ> This function sets up water movements

  int layer, frame, point, mode, cnt;
  float tmp_sin, tmp_cos, tmp;
  Uint8 spek;

  layer = 0;
  while ( layer < wi->layer_count )
  {
    wi->layer[layer].u = 0;
    wi->layer[layer].v = 0;
    frame = 0;
    while ( frame < MAXWATERFRAME )
    {
      // Do first mode
      mode = 0;
      for ( point = 0; point < WATERPOINTS; point++ )
      {
        tmp_sin = sin(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        tmp_cos = cos(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        wi->layer[layer].zadd[frame][mode][point]  = tmp_sin * wi->layer[layer].amp;
      }

      // Now mirror and copy data to other three modes
      mode++;
      wi->layer[layer].zadd[frame][mode][0] = wi->layer[layer].zadd[frame][0][1];
      wi->layer[layer].zadd[frame][mode][1] = wi->layer[layer].zadd[frame][0][0];
      wi->layer[layer].zadd[frame][mode][2] = wi->layer[layer].zadd[frame][0][3];
      wi->layer[layer].zadd[frame][mode][3] = wi->layer[layer].zadd[frame][0][2];
      mode++;

      wi->layer[layer].zadd[frame][mode][0] = wi->layer[layer].zadd[frame][0][3];
      wi->layer[layer].zadd[frame][mode][1] = wi->layer[layer].zadd[frame][0][2];
      wi->layer[layer].zadd[frame][mode][2] = wi->layer[layer].zadd[frame][0][1];
      wi->layer[layer].zadd[frame][mode][3] = wi->layer[layer].zadd[frame][0][0];
      mode++;

      wi->layer[layer].zadd[frame][mode][0] = wi->layer[layer].zadd[frame][0][2];
      wi->layer[layer].zadd[frame][mode][1] = wi->layer[layer].zadd[frame][0][3];
      wi->layer[layer].zadd[frame][mode][2] = wi->layer[layer].zadd[frame][0][0];
      wi->layer[layer].zadd[frame][mode][3] = wi->layer[layer].zadd[frame][0][1];
      frame++;
    }
    layer++;
  }


  // Calculate specular highlights
  spek = 0;
  for ( cnt = 0; cnt < 256; cnt++ )
  {
    tmp = FP8_TO_FLOAT( cnt );
    spek = 255 * tmp * tmp;

    wi->spek[cnt] = spek;

    // [claforte] Probably need to replace this with a
    //            glColor4f( FP8_TO_FLOAT(spek), FP8_TO_FLOAT(spek), FP8_TO_FLOAT(spek), 1.0f) call:
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t setup_lighting( LIGHTING_INFO * li )
{
  if(NULL ==li) return bfalse;

  // process the lighting info

  li->spek = DotProduct( li->spekdir, li->spekdir );
  if ( 0 != li->spek )
  {
    li->spek = sqrt( li->spek );
    li->spekdir.x /= li->spek;
    li->spekdir.y /= li->spek;
    li->spekdir.z /= li->spek;

    li->spekdir_stt = li->spekdir;
    li->spek *= li->ambi;
  }

  li->spekcol.r =
  li->spekcol.g =
  li->spekcol.b = li->spek;

  li->ambicol.r =
  li->ambicol.g =
  li->ambicol.b = li->ambi;

  make_speklut();
  make_lighttospek();
  make_spektable( li->spekdir );

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t read_wawalite( Game_t * gs, char *modname )
{
  // ZZ> This function sets up water and lighting for the module

  Mesh_t * pmesh;
  FILE* fileread;
  Uint32 loc_rand;
  bool_t found_expansion, found_idsz;

  if( !EKEY_PVALID(gs) ) return bfalse;

  pmesh = Game_getMesh(gs);

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.wawalite_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( NULL == fileread ) return bfalse;


  loc_rand = gs->randie_index;

  fgoto_colon( fileread );
  //  !!!BAD!!!
  //  Random map...
  //  If someone else wants to handle this, here are some thoughts for approaching
  //  it.  The .MPD file for the level should give the basic size of the map.  Use
  //  a standard tile set like the Palace modules.  Only use objects that are in
  //  the module's object directory, and only use some of them.  Imagine several Rock
  //  Moles eating through a stone filled level to make a path from the entrance to
  //  the exit.  Door placement will be difficult.
  //  !!!BAD!!!


  // Read water data first
  gs->Water.layer_count = fget_next_int( fileread );
  gs->Water.spekstart = fget_next_int( fileread );
  gs->Water.speklevel_fp8 = fget_next_int( fileread );
  gs->Water.douselevel = fget_next_int( fileread );
  gs->Water.surfacelevel = fget_next_int( fileread );
  gs->Water.light = fget_next_bool( fileread );
  gs->Water.iswater = fget_next_bool( fileread );
  gfxState.render_overlay    = fget_next_bool( fileread ) && CData.overlayvalid;
  gfxState.render_background = fget_next_bool( fileread ) && CData.backgroundvalid;
  gs->Water.layer[0].distx = fget_next_float( fileread );
  gs->Water.layer[0].disty = fget_next_float( fileread );
  gs->Water.layer[1].distx = fget_next_float( fileread );
  gs->Water.layer[1].disty = fget_next_float( fileread );
  foregroundrepeat = fget_next_int( fileread );
  backgroundrepeat = fget_next_int( fileread );


  gs->Water.layer[0].z = fget_next_int( fileread );
  gs->Water.layer[0].alpha_fp8 = fget_next_int( fileread );
  gs->Water.layer[0].frameadd = fget_next_int( fileread );
  gs->Water.layer[0].lightlevel_fp8 = fget_next_int( fileread );
  gs->Water.layer[0].lightadd_fp8 = fget_next_int( fileread );
  gs->Water.layer[0].amp = fget_next_float( fileread );
  gs->Water.layer[0].uadd = fget_next_float( fileread );
  gs->Water.layer[0].vadd = fget_next_float( fileread );

  gs->Water.layer[1].z = fget_next_int( fileread );
  gs->Water.layer[1].alpha_fp8 = fget_next_int( fileread );
  gs->Water.layer[1].frameadd = fget_next_int( fileread );
  gs->Water.layer[1].lightlevel_fp8 = fget_next_int( fileread );
  gs->Water.layer[1].lightadd_fp8 = fget_next_int( fileread );
  gs->Water.layer[1].amp = fget_next_float( fileread );
  gs->Water.layer[1].uadd = fget_next_float( fileread );
  gs->Water.layer[1].vadd = fget_next_float( fileread );

  gs->Water.layer[0].u = 0;
  gs->Water.layer[0].v = 0;
  gs->Water.layer[1].u = 0;
  gs->Water.layer[1].v = 0;
  gs->Water.layer[0].frame = RAND( &loc_rand, 0, WATERFRAMEAND );
  gs->Water.layer[1].frame = RAND( &loc_rand, 0, WATERFRAMEAND );

  // Read light data second
  gs->Light.on        = bfalse;
  gs->Light.spekdir.x = fget_next_float( fileread );
  gs->Light.spekdir.y = fget_next_float( fileread );
  gs->Light.spekdir.z = fget_next_float( fileread );
  gs->Light.ambi      = fget_next_float( fileread );
  gs->Light.spek      = 0;

  // Read tile data third
  gs->phys.hillslide = fget_next_float( fileread );
  gs->phys.slippyfriction = fget_next_float( fileread );
  gs->phys.airfriction = fget_next_float( fileread );
  gs->phys.waterfriction = fget_next_float( fileread );
  gs->phys.noslipfriction = fget_next_float( fileread );
  gs->phys.gravity = fget_next_float( fileread );
  gs->phys.slippyfriction = MAX( gs->phys.slippyfriction, sqrt( gs->phys.noslipfriction ) );
  gs->phys.airfriction    = MAX( gs->phys.airfriction,    sqrt( gs->phys.slippyfriction ) );
  gs->phys.waterfriction  = MIN( gs->phys.waterfriction,  pow( gs->phys.airfriction, 4.0f ) );

  gs->Tile_Anim.updateand = fget_next_int( fileread );
  gs->Tile_Anim.frameand = fget_next_int( fileread );
  gs->Tile_Anim.bigframeand = ( gs->Tile_Anim.frameand << 1 ) + 1;
  gs->Tile_Dam.amount = fget_next_int( fileread );
  gs->Tile_Dam.type = fget_next_damage( fileread );

  // Read weather data fourth
  gs->Weather.require_water = fget_next_bool( fileread );
  gs->Weather.timereset     = fget_next_int( fileread );
  gs->Weather.time          = gs->Weather.timereset;
  gs->Weather.player        = 0;
  gs->Weather.active        = btrue;
  if(0 == gs->Weather.timereset)
  {
    gs->Weather.active = bfalse;
    gs->Weather.player = INVALID_PLA;
  }

  // Read extra data
  pmesh->Info.exploremode = fget_next_bool( fileread );
  usefaredge = fget_next_bool( fileread );
  GCamera.swing = 0;
  GCamera.swingrate = fget_next_float( fileread );
  GCamera.swingamp = fget_next_float( fileread );

  // test for expansions (a bit tricky because of the fog stuff)
  fog_info_reset( &(gs->Fog) );
  tile_damage_reset( &(gs->Tile_Dam) );

  found_expansion = fgoto_colon_yesno( fileread );
  if( found_expansion )
  {
    found_idsz = ftest_idsz( fileread );

    if( !found_idsz )
    {
      // Read unnecessary data...  Only read if it exists...
      found_expansion = bfalse;
      gs->Fog.on           = CData.fogallowed;
      gs->Fog.top          = fget_float( fileread );
      gs->Fog.bottom       = fget_next_float( fileread );
      gs->Fog.red          = fget_next_fixed( fileread );
      gs->Fog.grn          = fget_next_fixed( fileread );
      gs->Fog.blu          = fget_next_fixed( fileread );
      gs->Fog.affectswater = fget_next_bool( fileread );

      gs->Fog.distance = ( gs->Fog.top - gs->Fog.bottom );

      // Read extra stuff for damage tile particles...
      found_expansion = fgoto_colon_yesno( fileread );
      if ( found_expansion )
      {
        found_idsz = ftest_idsz( fileread );
        if(!found_idsz)
        {
          found_expansion = bfalse;
          gs->Tile_Dam.parttype = fget_int( fileread );
          gs->Tile_Dam.partand  = fget_next_int( fileread );
          gs->Tile_Dam.sound    = fget_next_int( fileread );
        }
      }
    };

    if( !found_expansion ) { found_expansion = fgoto_colon_yesno( fileread ); if(found_expansion) { found_idsz = ftest_idsz( fileread ); } }
    if( found_expansion && found_idsz )
    {
      // we must have already found_expansion a ':' use the post-test do...while loop
      do 
      {
        IDSZ idsz;
        int iTmp;

        idsz = fget_idsz( fileread );
        iTmp = fget_int( fileread );

        // "MOON" == you can see the moon, so lycanthropy... mwa ha ha ha ha!
        // Also, it just means that it is outdoors
        if ( MAKE_IDSZ( "MOON" ) == idsz ) gs->Light.on = INT_TO_BOOL( iTmp );

      } while( fgoto_colon_yesno( fileread ) );
    }
  }

  // Allow slow machines to ignore the fancy stuff
  if ( !CData.twolayerwateron && gs->Water.layer_count > 1 )
  {
    int iTmp;
    gs->Water.layer_count = 1;
    iTmp = gs->Water.layer[0].alpha_fp8;
    iTmp = FP8_MUL( gs->Water.layer[1].alpha_fp8, iTmp ) + iTmp;
    if ( iTmp > 255 ) iTmp = 255;
    gs->Water.layer[0].alpha_fp8 = iTmp;
  }


  fs_fileClose( fileread );

  // Do it
  setup_lighting( &(gs->Light) );
  make_water( &(gs->Water) );

  return btrue;
};

//--------------------------------------------------------------------------------------------
void render_background( Uint16 texture )
{
  // ZZ> This function draws the large background

  Game_t * gs = Graphics_requireGame(&gfxState);

  GLVertex vtlist[4];
  float size;
  float sinsize, cossize;
  float x, y, z, u, v;
  int i;

  float loc_backgroundrepeat;


  // Figure out the screen coordinates of its corners
  x = gfxState.scrx << 6;
  y = gfxState.scry << 6;
  z = -100;
  u = gs->Water.layer[1].u;
  v = gs->Water.layer[1].v;
  size = x + y + 1;
  sinsize = turntosin[( 3*2047 ) & TRIGTABLE_MASK] * size;   // why 3/8 of a turn???
  cossize = turntocos[( 3*2047 ) & TRIGTABLE_MASK] * size;   // why 3/8 of a turn???
  loc_backgroundrepeat = backgroundrepeat * MIN( x / gfxState.scrx, y / gfxState.scrx );


  vtlist[0].pos.x = x + cossize;
  vtlist[0].pos.y = y - sinsize;
  vtlist[0].pos.z = z;
  vtlist[0].tx.s = 0 + u;
  vtlist[0].tx.t = 0 + v;

  vtlist[1].pos.x = x + sinsize;
  vtlist[1].pos.y = y + cossize;
  vtlist[1].pos.z = z;
  vtlist[1].tx.s = loc_backgroundrepeat + u;
  vtlist[1].tx.t = 0 + v;

  vtlist[2].pos.x = x - cossize;
  vtlist[2].pos.y = y + sinsize;
  vtlist[2].pos.z = z;
  vtlist[2].tx.s = loc_backgroundrepeat + u;
  vtlist[2].tx.t = loc_backgroundrepeat + v;

  vtlist[3].pos.x = x - sinsize;
  vtlist[3].pos.y = y - cossize;
  vtlist[3].pos.z = z;
  vtlist[3].tx.s = 0 + u;
  vtlist[3].tx.t = loc_backgroundrepeat + v;

  //-------------------------------------------------
  ATTRIB_PUSH( "render_background", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT );
  {
    glShadeModel( GL_FLAT );  // Flat shade this
    glDepthMask( GL_FALSE );
    glDepthFunc( GL_ALWAYS );

    glDisable( GL_CULL_FACE );

    GLtexture_Bind( gs->TxTexture + texture, &gfxState );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glBegin( GL_TRIANGLE_FAN );
    for ( i = 0; i < 4; i++ )
    {
      glTexCoord2fv( vtlist[i].tx._v );
      glVertex3fv( vtlist[i].pos.v );
    }
    glEnd();
  };
  ATTRIB_POP( "render_background" );
  //-------------------------------------------------
}



//--------------------------------------------------------------------------------------------
void render_foreground_overlay( Uint16 texture )
{
  Game_t * gs = Graphics_requireGame(&gfxState);

  GLVertex vtlist[4];
  float size;
  float sinsize, cossize;
  float x, y, z, u, v;
  int i;
  Uint16 rotate;
  float loc_foregroundrepeat;

  // Figure out the screen coordinates of its corners
  x = gfxState.scrx << 6;
  y = gfxState.scry << 6;
  z = 0;
  u = gs->Water.layer[1].u;
  v = gs->Water.layer[1].v;
  size = x + y + 1;
  rotate = 16384 + 8192;
  rotate >>= 2;
  sinsize = turntosin[rotate & TRIGTABLE_MASK] * size;
  cossize = turntocos[rotate & TRIGTABLE_MASK] * size;

  loc_foregroundrepeat = foregroundrepeat * MIN( x / gfxState.scrx, y / gfxState.scrx ) / 4.0;


  vtlist[0].pos.x = x + cossize;
  vtlist[0].pos.y = y - sinsize;
  vtlist[0].pos.z = z;
  vtlist[0].tx.s = 0 + u;
  vtlist[0].tx.t = 0 + v;

  vtlist[1].pos.x = x + sinsize;
  vtlist[1].pos.y = y + cossize;
  vtlist[1].pos.z = z;
  vtlist[1].tx.s = loc_foregroundrepeat + u;
  vtlist[1].tx.t = v;

  vtlist[2].pos.x = x - cossize;
  vtlist[2].pos.y = y + sinsize;
  vtlist[2].pos.z = z;
  vtlist[2].tx.s = loc_foregroundrepeat + u;
  vtlist[2].tx.t = loc_foregroundrepeat + v;

  vtlist[3].pos.x = x - sinsize;
  vtlist[3].pos.y = y - cossize;
  vtlist[3].pos.z = z;
  vtlist[3].tx.s = u;
  vtlist[3].tx.t = loc_foregroundrepeat + v;

  //-------------------------------------------------
  ATTRIB_PUSH( "render_forground_overlay", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_HINT_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT );
  {
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );  // make sure that the texture is as smooth as possible

    glShadeModel( GL_FLAT );  // Flat shade this

    glDepthMask( GL_FALSE );   // do not write into the depth buffer
    glDepthFunc( GL_ALWAYS );  // make it appear over the top of everything

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // make the texture a filter

    glDisable( GL_CULL_FACE );

    GLtexture_Bind( gs->TxTexture + texture, &gfxState );

    glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
    glBegin( GL_TRIANGLE_FAN );
    for ( i = 0; i < 4; i++ )
    {
      glTexCoord2fv( vtlist[i].tx._v );
      glVertex3fv( vtlist[i].pos.v );
    }
    glEnd();
  }
  ATTRIB_POP( "render_forground_overlay" );
  //-------------------------------------------------
}

//--------------------------------------------------------------------------------------------
void render_shadow( CHR_REF character )
{
  // ZZ> This function draws a NIFTY shadow

  Game_t * gs    = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);

  GLVertex v[4];

  float x, y;
  float level, height;
  float size_umbra_x,size_umbra_y, size_penumbra_x,size_penumbra_y;
  float height_factor, ambient_factor, tile_factor;
  float alpha_umbra, alpha_penumbra, alpha_character, light_character;
  Sint8 hide;
  int i;

  Uint16 chrlightambi, chrlightspek;
  float  globlightambi = 0, globlightspek = 0;
  float  lightambi, lightspek;

  chrlightambi  = gs->ChrList[character].tlight.ambi_fp8.r + gs->ChrList[character].tlight.ambi_fp8.g + gs->ChrList[character].tlight.ambi_fp8.b;
  chrlightspek  = gs->ChrList[character].tlight.spek_fp8.r + gs->ChrList[character].tlight.spek_fp8.g + gs->ChrList[character].tlight.spek_fp8.b;

  if( gs->Light.on )
  {
    globlightambi = gs->Light.ambicol.r + gs->Light.ambicol.g + gs->Light.ambicol.b;
    globlightspek = gs->Light.spekcol.r + gs->Light.spekcol.g + gs->Light.spekcol.b;
  };

  lightambi = chrlightambi + 255 * globlightambi;
  lightspek = chrlightspek + 255 * globlightspek;

  if( lightambi + lightspek < 1.0 ) return;

  hide = ChrList_getPCap(gs, character)->hidestate;
  if ( hide != NOHIDE && hide == gs->ChrList[character].aistate.state ) return;

  // Original points
  level = gs->ChrList[character].level;
  level += SHADOWRAISE;
  height = gs->ChrList[character].matrix.CNV( 3, 2 ) - level;
  if ( height < 0 ) height = 0;

  tile_factor = mesh_has_some_bits( pmesh->Mem.tilelst, gs->ChrList[character].onwhichfan, MPDFX_WATER ) ? 0.5 : 1.0;

  height_factor   = MAX( MIN(( 5 * gs->ChrList[character].bmpdata.calc_size / height ), 1 ), 0 );
  ambient_factor  = ( lightambi ) / ( lightambi + lightspek );
  alpha_character = FP8_TO_FLOAT( gs->ChrList[character].alpha_fp8 );
  if ( gs->ChrList[character].light_fp8 == 255 )
  {
    light_character = 1.0f;
  }
  else
  {
    light_character = ( float ) chrlightspek / 3.0f / ( float ) gs->ChrList[character].light_fp8;
    light_character =  MIN( 1, MAX( 0, light_character ) );
  };


  size_umbra_x    = ( gs->ChrList[character].bmpdata.cv.x_max - gs->ChrList[character].bmpdata.cv.x_min - height / 30.0 );
  size_umbra_y    = ( gs->ChrList[character].bmpdata.cv.y_max - gs->ChrList[character].bmpdata.cv.y_min - height / 30.0 );
  size_penumbra_x = ( gs->ChrList[character].bmpdata.cv.x_max - gs->ChrList[character].bmpdata.cv.x_min + height / 30.0 );
  size_penumbra_y = ( gs->ChrList[character].bmpdata.cv.y_max - gs->ChrList[character].bmpdata.cv.y_min + height / 30.0 );

  alpha_umbra    = alpha_character * height_factor * ambient_factor * light_character * tile_factor;
  alpha_penumbra = alpha_character * height_factor * ambient_factor * light_character * tile_factor;

  if ( FLOAT_TO_FP8( alpha_umbra ) == 0 && FLOAT_TO_FP8( alpha_penumbra ) == 0 ) return;

  x = gs->ChrList[character].matrix.CNV( 3, 0 );
  y = gs->ChrList[character].matrix.CNV( 3, 1 );

  //GOOD SHADOW
  v[0].tx.s = CALCULATE_PRT_U0( 238 );
  v[0].tx.t = CALCULATE_PRT_V0( 238 );

  v[1].tx.s = CALCULATE_PRT_U1( 255 );
  v[1].tx.t = CALCULATE_PRT_V0( 238 );

  v[2].tx.s = CALCULATE_PRT_U1( 255 );
  v[2].tx.t = CALCULATE_PRT_V1( 255 );

  v[3].tx.s = CALCULATE_PRT_U0( 238 );
  v[3].tx.t = CALCULATE_PRT_V1( 255 );

  if ( size_penumbra_x > 0 && size_penumbra_y > 0 )
  {
    v[0].pos.x = x + size_penumbra_x;
    v[0].pos.y = y - size_penumbra_y;
    v[0].pos.z = level;

    v[1].pos.x = x + size_penumbra_x;
    v[1].pos.y = y + size_penumbra_y;
    v[1].pos.z = level;

    v[2].pos.x = x - size_penumbra_x;
    v[2].pos.y = y + size_penumbra_y;
    v[2].pos.z = level;

    v[3].pos.x = x - size_penumbra_x;
    v[3].pos.y = y - size_penumbra_y;
    v[3].pos.z = level;

    ATTRIB_PUSH( "render_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT );
    {
      glEnable( GL_BLEND );
      glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

      glDepthMask( GL_FALSE );

      glEnable( GL_DEPTH_TEST );
      glDepthFunc( GL_LESS );

      // Choose texture.
      if( MAXTEXTURE == particletexture)
      {
        GLtexture_Bind( NULL, &gfxState );
      }
      else
      {
        GLtexture_Bind( gs->TxTexture + particletexture, &gfxState );
      };

      glBegin( GL_TRIANGLE_FAN );
      glColor4f( alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0 );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2fv( v[i].tx._v );
        glVertex3fv( v[i].pos.v );
      }
      glEnd();
    }
    ATTRIB_POP( "render_shadow" );
  };

  if ( size_umbra_x > 0 && size_umbra_y > 0 )
  {
    v[0].pos.x = x + size_umbra_x;
    v[0].pos.y = y - size_umbra_y;
    v[0].pos.z = level + 0.1;

    v[1].pos.x = x + size_umbra_x;
    v[1].pos.y = y + size_umbra_y;
    v[1].pos.z = level + 0.1;

    v[2].pos.x = x - size_umbra_x;
    v[2].pos.y = y + size_umbra_y;
    v[2].pos.z = level + 0.1;

    v[3].pos.x = x - size_umbra_x;
    v[3].pos.y = y - size_umbra_y;
    v[3].pos.z = level + 0.1;

    ATTRIB_PUSH( "render_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT );
    {
      glDisable( GL_CULL_FACE );

      glEnable( GL_BLEND );
      glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

      glDepthMask( GL_FALSE );
      glDepthFunc( GL_LEQUAL );

      // Choose texture.
      if( MAXTEXTURE == particletexture)
      {
        GLtexture_Bind( NULL, &gfxState );
      }
      else
      {
        GLtexture_Bind( gs->TxTexture + particletexture, &gfxState );
      };

      glBegin( GL_TRIANGLE_FAN );
      glColor4f( alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0 );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2fv( v[i].tx._v );
        glVertex3fv( v[i].pos.v );
      }
      glEnd();
    }
    ATTRIB_POP( "render_shadow" );
  };
};

//--------------------------------------------------------------------------------------------
void calc_chr_lighting( vect3 pos, Uint16 tl, Uint16 tr, Uint16 bl, Uint16 br, Uint16 * spek, Uint16 * minv, Uint16 * maxv )
{
  float dx,dy;
  Uint16 minval, maxval;
  float top, bot;

  dx = MESH_FLOAT_TO_FAN(pos.x); dx -= (int)dx;
  dy = MESH_FLOAT_TO_FAN(pos.y); dy -= (int)dy;

  minval = MIN( MIN( tl, tr ), MIN( bl, br ) );
  if(NULL != minv)
  {
    (*minv) = minval;
  }

  maxval = MAX( MAX( tl, tr ), MAX( bl, br ) );
  if(NULL != maxv)
  {
    (*maxv) = maxval;
  }

  // Interpolate lighting level using tile corners
  if(maxval <= minval)
  {
    (*spek) = 0;
  }
  else
  {
    top = dx*tr + (1-dx)*tl;
    bot = dx*br + (1-dx)*bl;
    if(NULL != spek)
    {
      (*spek) = (dy*bot + (1.0f-dy)*top) - minval;
    };
  }
};

//--------------------------------------------------------------------------------------------
void do_chr_dynalight(Game_t * gs)
{
  // ZZ> This function figures out character dynamic lighting from the mesh

  int cnt;
  CHR_REF chr_tnc;
  Uint16 i0, i1, i2, i3;
  Uint16 spek, minval, maxval;
  Uint32 vrtstart;
  Uint32 fan;

  Mesh_t * pmesh;
  CHR_TLIGHT * tlight;
  
  if(NULL == gs) gs = Graphics_requireGame( &gfxState );
  pmesh = Game_getMesh(gs);

  for ( cnt = 0; cnt < numdolist; cnt++ )
  {
    chr_tnc = dolist[cnt];
    tlight = &(gs->ChrList[chr_tnc].tlight);
    fan = gs->ChrList[chr_tnc].onwhichfan;

    if(INVALID_FAN == fan) continue;

    vrtstart = pmesh->Mem.tilelst[gs->ChrList[chr_tnc].onwhichfan].vrt_start;

    //----------------------------------------
    // Red lighting
    i0 = pmesh->Mem.vrt_lr_fp8[vrtstart + 0];
    i1 = pmesh->Mem.vrt_lr_fp8[vrtstart + 1];
    i2 = pmesh->Mem.vrt_lr_fp8[vrtstart + 2];
    i3 = pmesh->Mem.vrt_lr_fp8[vrtstart + 3];
    calc_chr_lighting( gs->ChrList[chr_tnc].ori.pos, i0, i1, i2, i3, &spek, &minval, &maxval );
    tlight->ambi_fp8.r = minval;
    tlight->spek_fp8.r = spek;

    tlight->turn_lr.r = 0;
    if ( spek > 0 || !pmesh->Info.exploremode )
    {
      float scale = 255.0f / maxval;

      // Look up spek direction using corners again
      i0 = (( ((int)(scale*i0)) & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( ((int)(scale*i1)) & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( ((int)(scale*i3)) & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( ((int)(scale*i2)) & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      tlight->turn_lr.r = ( lightdirectionlookup[i0] << 8 );
    }

    //----------------------------------------
    // Green lighting
    i0 = pmesh->Mem.vrt_lg_fp8[vrtstart + 0];
    i1 = pmesh->Mem.vrt_lg_fp8[vrtstart + 1];
    i3 = pmesh->Mem.vrt_lg_fp8[vrtstart + 2];
    i2 = pmesh->Mem.vrt_lg_fp8[vrtstart + 3];
    calc_chr_lighting( gs->ChrList[chr_tnc].ori.pos, i0, i1, i2, i3, &spek, &minval, &maxval );
    tlight->ambi_fp8.g = minval;
    tlight->spek_fp8.g = spek;

    tlight->turn_lr.g = 0;
    if ( spek > 0 && !pmesh->Info.exploremode )
    {
      float scale = 255.0f / maxval;

      // Look up spek direction using corners again
      i0 = (( ((int)(scale*i0)) & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( ((int)(scale*i1)) & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( ((int)(scale*i3)) & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( ((int)(scale*i2)) & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      tlight->turn_lr.g = ( lightdirectionlookup[i0] << 8 );
    }

    //----------------------------------------
    // Blue lighting
    i0 = pmesh->Mem.vrt_lb_fp8[vrtstart + 0];
    i1 = pmesh->Mem.vrt_lb_fp8[vrtstart + 1];
    i3 = pmesh->Mem.vrt_lb_fp8[vrtstart + 2];
    i2 = pmesh->Mem.vrt_lb_fp8[vrtstart + 3];
    calc_chr_lighting( gs->ChrList[chr_tnc].ori.pos, i0, i1, i2, i3, &spek, &minval, &maxval );
    tlight->ambi_fp8.b = minval;
    tlight->spek_fp8.b = spek;

    tlight->turn_lr.b = 0;
    if ( spek > 0 || !pmesh->Info.exploremode )
    {
      float scale = 255.0f / maxval;

      // Look up spek direction using corners again
      i0 = (( ((int)(scale*i0)) & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( ((int)(scale*i1)) & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( ((int)(scale*i3)) & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( ((int)(scale*i2)) & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      tlight->turn_lr.b = ( lightdirectionlookup[i0] << 8 );
    }

  }
}

//--------------------------------------------------------------------------------------------
void do_prt_dynalight(Game_t * gs)
{
  // ZZ> This function figures out particle lighting

  Mesh_t * pmesh = NULL;

  PRT_REF prt_cnt;
  CHR_REF character;

  if(NULL == gs) gs = Graphics_requireGame( &gfxState );
  pmesh = Game_getMesh(gs);

  for ( prt_cnt = 0; prt_cnt < PRTLST_COUNT; prt_cnt++ )
  {
    if ( !ACTIVE_PRT(gs->PrtList,  prt_cnt ) ) continue;

    switch ( gs->PrtList[prt_cnt].type )
    {
      case PRTTYPE_LIGHT:
        {
          float ftmp = gs->PrtList[prt_cnt].dyna.level * ( 127 * gs->PrtList[prt_cnt].dyna.falloff ) / FP8_TO_FLOAT( FP8_MUL( gs->PrtList[prt_cnt].size_fp8, gs->PrtList[prt_cnt].size_fp8 ) );
          if ( ftmp > 255 ) ftmp = 255;

          gs->PrtList[prt_cnt].lightr_fp8 =
          gs->PrtList[prt_cnt].lightg_fp8 =
          gs->PrtList[prt_cnt].lightb_fp8 = ftmp;
        }
        break;

      case PRTTYPE_ALPHA:
      case PRTTYPE_SOLID:
        {
          character = prt_get_attachedtochr( gs, prt_cnt );
          if ( ACTIVE_CHR( gs->ChrList,  character ) )
          {
            gs->PrtList[prt_cnt].lightr_fp8 = gs->ChrList[character].tlight.spek_fp8.r + gs->ChrList[character].tlight.ambi_fp8.r;
            gs->PrtList[prt_cnt].lightg_fp8 = gs->ChrList[character].tlight.spek_fp8.g + gs->ChrList[character].tlight.ambi_fp8.g;
            gs->PrtList[prt_cnt].lightb_fp8 = gs->ChrList[character].tlight.spek_fp8.b + gs->ChrList[character].tlight.ambi_fp8.b;
          }
          else if ( INVALID_FAN != gs->PrtList[prt_cnt].onwhichfan )
          {
            gs->PrtList[prt_cnt].lightr_fp8 = pmesh->Mem.vrt_lr_fp8[pmesh->Mem.tilelst[gs->PrtList[prt_cnt].onwhichfan].vrt_start];
            gs->PrtList[prt_cnt].lightg_fp8 = pmesh->Mem.vrt_lg_fp8[pmesh->Mem.tilelst[gs->PrtList[prt_cnt].onwhichfan].vrt_start];
            gs->PrtList[prt_cnt].lightb_fp8 = pmesh->Mem.vrt_lb_fp8[pmesh->Mem.tilelst[gs->PrtList[prt_cnt].onwhichfan].vrt_start];
          }
          else
          {
            gs->PrtList[prt_cnt].lightr_fp8 =
            gs->PrtList[prt_cnt].lightg_fp8 =
            gs->PrtList[prt_cnt].lightb_fp8 = 0;
          }
        }
        break;

      default:
        gs->PrtList[prt_cnt].lightr_fp8 =
        gs->PrtList[prt_cnt].lightg_fp8 =
        gs->PrtList[prt_cnt].lightb_fp8 = 0;
    };
  }

}

//--------------------------------------------------------------------------------------------
void render_water()
{
  // ZZ> This function draws all of the water fans

  int cnt;
  Game_t * gs = Graphics_requireGame(&gfxState);

  // Bottom layer first
  if ( !gfxState.render_background && gs->Water.layer_count > 1 )
  {
    cnt = 0;
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan( renderlist.watr[cnt], 1, renderlist.watr_mode[cnt]);
      cnt++;
    }
  }

  // Top layer second
  if ( !gfxState.render_overlay && gs->Water.layer_count > 0 )
  {
    cnt = 0;
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan( renderlist.watr[cnt], 0, renderlist.watr_mode[cnt] );
      cnt++;
    }
  }
}

void render_water_lit()
{
  // BB> This function draws the hilites for water tiles using global lighting

  int cnt;
  Game_t * gs = Graphics_requireGame(&gfxState);

  // Bottom layer first
  if ( !gfxState.render_background && gs->Water.layer_count > 1 )
  {
    float ambi_level = FP8_TO_FLOAT( gs->Water.layer[1].lightadd_fp8 + gs->Water.layer[1].lightlevel_fp8 );
    float spek_level =  FP8_TO_FLOAT( gs->Water.speklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( gs->Water.light )
    {
      // self-lit water provides its own light
      glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, mat_ambient );
    }
    else
    {
      // non-self-lit water needs an external lightsource
      glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, mat_none );
    }

    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess );

    cnt = 0;
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan_lit( renderlist.watr[cnt], 1, renderlist.watr_mode[cnt] );
      cnt++;
    }
  }

  // Top layer second
  if ( !gfxState.render_overlay && gs->Water.layer_count > 0 )
  {
    float ambi_level = ( gs->Water.layer[1].lightadd_fp8 + gs->Water.layer[1].lightlevel_fp8 ) / 255.0;
    float spek_level =  FP8_TO_FLOAT( gs->Water.speklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;

    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( gs->Water.light )
    {
      // self-lit water provides its own light
      glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, mat_ambient );
    }
    else
    {
      // non-self-lit water needs an external lightsource
      glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, mat_none );
    }

    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, mat_diffuse );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess );

    cnt = 0;
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan_lit( renderlist.watr[cnt], 0, renderlist.watr_mode[cnt] );
      cnt++;
    }
  }

}

//--------------------------------------------------------------------------------------------
void render_good_shadows()
{
  int cnt;
  CHR_REF chr_tnc;

  Game_t * gs    = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);

  // Good shadows for me
  ATTRIB_PUSH( "render_good_shadows", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDisable( GL_ALPHA_TEST );
    //glAlphaFunc(GL_GREATER, 0);

    glDisable( GL_CULL_FACE );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

    glDepthMask( GL_FALSE );
    glDepthFunc( GL_LEQUAL );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      chr_tnc = dolist[cnt];
      if ( gs->ChrList[chr_tnc].bmpdata.shadow != 0 || gs->ChrList[chr_tnc].prop.forceshadow && mesh_has_no_bits( pmesh->Mem.tilelst, gs->ChrList[chr_tnc].onwhichfan, MPDFX_SHINY ) )
        render_shadow( chr_tnc );
    }
  }
  ATTRIB_POP( "render_good_shadows" );
}

//--------------------------------------------------------------------------------------------
void render_character_reflections()
{
  int cnt;
  CHR_REF chr_tnc;

  Game_t * gs    = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);

  // Render reflections of characters
  ATTRIB_PUSH( "render_character_reflections", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CCW );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      chr_tnc = dolist[cnt];
      if ( mesh_has_some_bits( pmesh->Mem.tilelst, gs->ChrList[chr_tnc].onwhichfan, MPDFX_SHINY ) )
        render_refmad( chr_tnc, gs->ChrList[chr_tnc].alpha_fp8 / 2 );
    }
  }
  ATTRIB_POP( "render_character_reflections" );

};

//--------------------------------------------------------------------------------------------
void render_normal_fans()
{
  Game_t * gs    = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);

  int cnt, tnc, fan, texture;

  ATTRIB_PUSH( "render_normal_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    // gfxState.shading stuff
    glShadeModel( gfxState.shading );

    // alpha stuff
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    // backface culling
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    glCullFace( GL_BACK );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
      texture = cnt + TX_TILE_0;
      pmesh->Info.last_texture = texture;
      GLtexture_Bind( gs->TxTexture + texture, &gfxState );
      for ( tnc = 0; tnc < renderlist.num_norm; tnc++ )
      {
        fan = renderlist.norm[tnc];
        render_fan( fan, texture );
      };
    }
  }
  ATTRIB_POP( "render_normal_fans" );
};

//--------------------------------------------------------------------------------------------
void render_shiny_fans()
{
  Game_t * gs    = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);

  int cnt, tnc, fan, texture;

  ATTRIB_PUSH( "render_shiny_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    // gfxState.shading stuff
    glShadeModel( gfxState.shading );

    // alpha stuff
    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    // backface culling
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    glCullFace( GL_BACK );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
      texture = cnt + TX_TILE_0;
      pmesh->Info.last_texture = texture;
      GLtexture_Bind( gs->TxTexture + texture, &gfxState );
      for ( tnc = 0; tnc < renderlist.num_shine; tnc++ )
      {
        fan = renderlist.shine[tnc];
        render_fan( fan, texture );
      };
    }
  }
  ATTRIB_POP( "render_shiny_fans" );
};

//--------------------------------------------------------------------------------------------
void render_reflected_fans_ref()
{
  Game_t * gs    = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);

  int cnt, tnc, fan, texture;

  ATTRIB_PUSH( "render_reflected_fans_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // gfxState.shading stuff
    glShadeModel( gfxState.shading );

    // alpha stuff
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    // backface culling
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CCW );
    glCullFace( GL_BACK );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
      texture = cnt + TX_TILE_0;
      pmesh->Info.last_texture = texture;
      GLtexture_Bind( gs->TxTexture + texture, &gfxState );
      for ( tnc = 0; tnc < renderlist.num_shine; tnc++ )
      {
        fan = renderlist.reflc[tnc];
        render_fan_ref( fan, texture, gs->PlaList_level );
      };
    }
  }
  ATTRIB_POP( "render_reflected_fans_ref" );
};

//--------------------------------------------------------------------------------------------
void render_solid_characters()
{
  int cnt;
  CHR_REF chr_tnc;

  Game_t * gs = Graphics_requireGame(&gfxState);

  // Render the normal characters
  ATTRIB_PUSH( "render_solid_characters", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glFrontFace( GL_CW );

    // must be here even for solid characters because of stuff like the spikemace
    // and a lot of other items that just use a simple texturemapped plane with
    // transparency for their shape.  Maybe it should be converted to blend?
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      chr_tnc = dolist[cnt];
      if ( gs->ChrList[chr_tnc].alpha_fp8 == 255 && gs->ChrList[chr_tnc].light_fp8 == 255 )
        render_mad( chr_tnc, 255 );
    }
  }
  ATTRIB_POP( "render_solid_characters" );

};

//--------------------------------------------------------------------------------------------
void render_alpha_characters()
{
  int cnt;
  CHR_REF chr_tnc;
  Uint8 trans;

  Game_t * gs = Graphics_requireGame(&gfxState);

  // Now render the transparent characters
  ATTRIB_PUSH( "render_alpha_characters", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );
    glCullFace( GL_BACK );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_COLOR );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      chr_tnc = dolist[cnt];
      if ( gs->ChrList[chr_tnc].alpha_fp8 != 255 )
      {
        trans = gs->ChrList[chr_tnc].alpha_fp8;

        if (( gs->ChrList[chr_tnc].alpha_fp8 + gs->ChrList[chr_tnc].light_fp8 ) < SEEINVISIBLE &&  gs->cl->seeinvisible && chr_is_player(gs, chr_tnc) && gs->PlaList[gs->ChrList[chr_tnc].whichplayer].is_local )
          trans = SEEINVISIBLE - gs->ChrList[chr_tnc].light_fp8;

        render_mad( chr_tnc, trans );
      }
    }

  }
  ATTRIB_POP( "render_alpha_characters" );

};

//--------------------------------------------------------------------------------------------
// render the water hilights, etc. using global lighting
void render_water_highlights()
{
  Game_t * gs = Graphics_requireGame(&gfxState);

  ATTRIB_PUSH( "render_water_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT );
  {
    GLfloat light_position[] = { 10000*gs->Light.spekdir.x, 10000*gs->Light.spekdir.y, 10000*gs->Light.spekdir.z, 1.0 };
    GLfloat lmodel_ambient[] = { gs->Light.ambicol.r, gs->Light.ambicol.g, gs->Light.ambicol.b, 1.0 };
    GLfloat light_diffuse[]  = { gs->Light.spekcol.r, gs->Light.spekcol.g, gs->Light.spekcol.b, 1.0 };

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );

    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );

    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, lmodel_ambient );

    //glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv( GL_LIGHT0, GL_DIFFUSE, light_diffuse );
    glLightfv( GL_LIGHT0, GL_SPECULAR, light_diffuse );
    glLightfv( GL_LIGHT0, GL_POSITION, light_position );

    render_water_lit();

    glDisable( GL_LIGHT0 );
    glDisable( GL_LIGHTING );
  }
  ATTRIB_POP( "render_water_highlights" );
};

//--------------------------------------------------------------------------------------------
void render_alpha_water()
{

  ATTRIB_PUSH( "render_alpha_water", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_POLYGON_BIT );
  {
    glDisable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glFrontFace( GL_CW );

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    render_water();
  }
  ATTRIB_POP( "render_alpha_water" );
};

//--------------------------------------------------------------------------------------------
void render_light_water()
{
  ATTRIB_PUSH( "render_light_water", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
  {
    glDisable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glFrontFace( GL_CW );

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );
    render_water();
  }
  ATTRIB_POP( "render_light_water" );
};

//--------------------------------------------------------------------------------------------
void render_character_highlights()
{
  int cnt;
  CHR_REF chr_tnc;

  Game_t * gs = Graphics_requireGame(&gfxState);

  ATTRIB_PUSH( "render_character_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT );
  {
    GLfloat light_none[]     = {0, 0, 0, 0};
    GLfloat light_position[] = { 10000*gs->Light.spekdir.x, 10000*gs->Light.spekdir.y, 10000*gs->Light.spekdir.z, 1.0 };
    GLfloat lmodel_ambient[] = { gs->Light.ambicol.r, gs->Light.ambicol.g, gs->Light.ambicol.b, 1.0 };
    GLfloat light_specular[] = { gs->Light.spekcol.r, gs->Light.spekcol.g, gs->Light.spekcol.b, 1.0 };

    glDisable( GL_CULL_FACE );

    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_LESS, 1 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );

    glLightfv( GL_LIGHT1, GL_AMBIENT,  light_none );
    glLightfv( GL_LIGHT1, GL_DIFFUSE,  light_none );
    glLightfv( GL_LIGHT1, GL_SPECULAR, light_specular );
    glLightfv( GL_LIGHT1, GL_POSITION, light_position );

    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, light_none );
    glLightModeli( GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE );
    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );

    glEnable( GL_LIGHTING );
    glDisable( GL_LIGHT0 );
    glEnable( GL_LIGHT1 );

    for ( cnt = 0; cnt < numdolist; cnt++ )
    {
      chr_tnc = dolist[cnt];

      if ( gs->ChrList[chr_tnc].sheen_fp8 == 0 && gs->ChrList[chr_tnc].light_fp8 == 255 && gs->ChrList[chr_tnc].alpha_fp8 == 255 ) continue;

      render_mad_lit( chr_tnc );
    }

    glDisable( GL_LIGHT0 );
    glDisable( GL_LIGHTING );
  }
  ATTRIB_POP( "render_character_highlights" );
};

GLint inp_attrib_stack, out_attrib_stack;
void draw_scene_zreflection()
{
  // ZZ> This function draws 3D objects
  // do all the rendering of reflections

  Game_t * gs = Graphics_requireGame(&gfxState);

  if ( CData.refon )
  {
    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_reflected_fans_ref( );
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    //render_sha_fans_ref( );
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_character_reflections();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_particle_reflections();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
  }

  //---- render the non-reflective fans ----
  {
    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_normal_fans();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    render_shiny_fans();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

    ATTRIB_GUARD_OPEN( inp_attrib_stack );
    //render_reflected_fans();
    ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
  }

  // Render the shadows
  if ( CData.shaon )
  {
    if ( CData.shasprite )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      //render_bad_shadows();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    }
    else
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_good_shadows();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    }
  }

  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_solid_characters();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

  //---- Render transparent objects ----
  {
    render_alpha_characters();

    // And alpha water
    if ( !gs->Water.light )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_alpha_water();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    };

    // Do self-lit water
    if ( gs->Water.light )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_light_water();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    };

  };

  // do highlights
  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_character_highlights();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );


  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_water_highlights();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

  // render the sprites
  ATTRIB_GUARD_OPEN( inp_attrib_stack );
  render_particles();
  ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );

  // render the collision volumes
#if defined(DEBUG_CVOLUME) && defined(_DEBUG)

  if(CData.DevMode)
  {
    cv_list_draw();
  };

#endif

//#if defined(DEBUG_BBOX) && defined(_DEBUG)
//  if(CData.DevMode)
//  {
//    int i;
//
//    for(i=0; i<CHRLST_COUNT; i++)
//    {
//      if( !ACTIVE_CHR(gs->ChrList, i) ) continue;
//
//      mad_display_bbox_tree(2, gs->ChrList[i].matrix, gs->MadList + gs->ChrList[i].model, gs->ChrList[i].anim.last, gs->ChrList[i].anim.next );
//    }
//  };
//#endif

};


bool_t draw_texture_box( GLtexture * ptx, FRect_t * tx_rect, FRect_t * sc_rect )
{
  FRect_t rtmp;

  if( NULL == sc_rect ) return bfalse;

  GLtexture_Bind( ptx, &gfxState );

  if(NULL == tx_rect)
  {
    rtmp.left = 0; rtmp.right = 1;
    rtmp.top  = 0; rtmp.bottom = 1;
    tx_rect = &rtmp;
  };

  glBegin( GL_QUADS );
    glTexCoord2f( tx_rect->left,  tx_rect->bottom );   glVertex2f( sc_rect->left,  sc_rect->bottom );
    glTexCoord2f( tx_rect->right, tx_rect->bottom );   glVertex2f( sc_rect->right, sc_rect->bottom );
    glTexCoord2f( tx_rect->right, tx_rect->top    );   glVertex2f( sc_rect->right, sc_rect->top    );
    glTexCoord2f( tx_rect->left,  tx_rect->top    );   glVertex2f( sc_rect->left,  sc_rect->top    );
  glEnd();

  return btrue;
}

//--------------------------------------------------------------------------------------------
void draw_blip( COLR color, float x, float y)
{
  // ZZ> This function draws a blip

  Gui_t * gui = gui_getState();

  FRect_t tx_rect, sc_rect;
  float width, height;

  width  = 3.0*mapscale*0.5f;
  height = 3.0*mapscale*0.5f;

  if ( x < -width || x > gfxState.scrx + width || y < -height || y > gfxState.scry + height ) return;

  ATTRIB_PUSH( "draw_blip", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // gfxState.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // backface culling
    glDisable( GL_CULL_FACE );

    tx_rect.left   = (( float ) BlipList[color].rect.left   ) / ( float ) GLtexture_GetTextureWidth(&gui->TxBlip)  + 0.01;
    tx_rect.right  = (( float ) BlipList[color].rect.right  ) / ( float ) GLtexture_GetTextureWidth(&gui->TxBlip)  - 0.01;
    tx_rect.top    = (( float ) BlipList[color].rect.top    ) / ( float ) GLtexture_GetTextureHeight(&gui->TxBlip) + 0.01;
    tx_rect.bottom = (( float ) BlipList[color].rect.bottom ) / ( float ) GLtexture_GetTextureHeight(&gui->TxBlip) - 0.01;

    sc_rect.left   = x - width;
    sc_rect.right  = x + width;
    sc_rect.top    = gfxState.scry - ( y - height );
    sc_rect.bottom = gfxState.scry - ( y + height );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    draw_texture_box(&gui->TxBlip, &tx_rect, &sc_rect);
  }
  ATTRIB_POP( "draw_blip" );
}

//--------------------------------------------------------------------------------------------
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle )
{
  // ZZ> This function draws an icon

  Game_t * gs = Graphics_requireGame(&gfxState);

  int position, blipx, blipy;
  int width, height;
  FRect_t tx_rect, sc_rect;

  if (MAXICONTX== icontype || INVALID_TEXTURE == gs->TxIcon[icontype].textureID ) return;

  ATTRIB_PUSH( "draw_one_icon", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // gfxState.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_BLEND );

    // backface culling
    glDisable( GL_CULL_FACE );

    tx_rect.left   = (( float ) iconrect.left   ) / GLtexture_GetTextureWidth(gs->TxIcon + icontype);
    tx_rect.right  = (( float ) iconrect.right  ) / GLtexture_GetTextureWidth(gs->TxIcon + icontype);
    tx_rect.top    = (( float ) iconrect.top    ) / GLtexture_GetTextureHeight(gs->TxIcon + icontype);
    tx_rect.bottom = (( float ) iconrect.bottom ) / GLtexture_GetTextureHeight(gs->TxIcon + icontype);

    width  = iconrect.right  - iconrect.left;
    height = iconrect.bottom - iconrect.top;

    sc_rect.left   = x;
    sc_rect.right  = x + width;
    sc_rect.top    = gfxState.scry - y;
    sc_rect.bottom = gfxState.scry - (y + height);

    draw_texture_box( gs->TxIcon + icontype, &tx_rect, &sc_rect);
  }
  ATTRIB_POP( "draw_one_icon" );

  if ( sparkle != NOSPARKLE )
  {
    position = gs->wld_frame & 31;
    position = ( SPARKLESIZE * position >> 5 );

    blipx = x + SPARKLEADD + position;
    blipy = y + SPARKLEADD;
    draw_blip( (COLR)sparkle, blipx, blipy );

    blipx = x + SPARKLEADD + SPARKLESIZE;
    blipy = y + SPARKLEADD + position;
    draw_blip( (COLR)sparkle, blipx, blipy );

    blipx -= position;
    blipy = y + SPARKLEADD + SPARKLESIZE;
    draw_blip( (COLR)sparkle, blipx, blipy );

    blipx = x + SPARKLEADD;
    blipy -= position;
    draw_blip( (COLR)sparkle, blipx, blipy );
  }
}

//--------------------------------------------------------------------------------------------
void BMFont_draw_one( BMFont_t * pfnt, int fonttype, float x, float y )
{
  // ZZ> This function draws a letter or number
  // GAC> Very nasty version for starters.  Lots of room for improvement.

  GLfloat dx, dy, border;
  FRect_t tx_rect, sc_rect;

  dx = 2.0f / 512.0f;
  dy = 1.0f / 256.0f;
  border = 1.0f / 512.0f;

  tx_rect.left   = pfnt->rect[fonttype].x * dx + border;
  tx_rect.right  = ( pfnt->rect[fonttype].x + pfnt->rect[fonttype].w ) * dx - border;
  tx_rect.top    = pfnt->rect[fonttype].y * dy + border;
  tx_rect.bottom = ( pfnt->rect[fonttype].y + pfnt->rect[fonttype].h ) * dy - border;

  sc_rect.left   = x;
  sc_rect.right  = x + pfnt->rect[fonttype].w;
  sc_rect.top    = gfxState.scry - y;
  sc_rect.bottom = gfxState.scry - (y + pfnt->rect[fonttype].h);

  draw_texture_box( &(pfnt->tex), &tx_rect, &sc_rect);
}

//--------------------------------------------------------------------------------------------
void draw_map( float x, float y )
{
  // ZZ> This function draws the map

  Game_t * gs = Graphics_requireGame(&gfxState);
  FRect_t tx_rect, sc_rect;

  ATTRIB_PUSH( "draw_map", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // gfxState.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );
    glDisable( GL_BLEND );

    // backface culling
    glDisable( GL_CULL_FACE );

    tx_rect.left   = 0.0f;
    tx_rect.right  = (float)GLtexture_GetImageWidth(&gs->TxMap) / (float)GLtexture_GetTextureWidth(&gs->TxMap);
    tx_rect.top    = 0.0f;
    tx_rect.bottom = (float)GLtexture_GetImageHeight(&gs->TxMap) / (float)GLtexture_GetTextureHeight(&gs->TxMap);

    sc_rect.left   = x + maprect.left;
    sc_rect.right  = x + maprect.right;
    sc_rect.top    = maprect.bottom - y;
    sc_rect.bottom = maprect.top    - y;

    draw_texture_box( &gs->TxMap, &tx_rect, &sc_rect );
  }
  ATTRIB_POP( "draw_map" );
}

//--------------------------------------------------------------------------------------------
int draw_one_bar( int bartype, int x, int y, int ticks, int maxticks )
{
  // ZZ> This function draws a bar and returns the y position for the next one

  Gui_t * gui = gui_getState();

  int noticks;
  FRect_t tx_rect, sc_rect;
  int width, height;
  int ystt = y;

  ATTRIB_PUSH( "draw_one_bar", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // depth buffer stuff
    glDepthMask( GL_FALSE );

    // gfxState.shading stuff
    glShadeModel( GL_FLAT );

    // alpha stuff
    glDisable( GL_ALPHA_TEST );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // backface culling
    glDisable( GL_CULL_FACE );

    if ( maxticks > 0 && ticks >= 0 )
    {
      // Draw the tab
      GLtexture_Bind( &gui->TxBars, &gfxState );
      tx_rect.left   = ( float ) tabrect[bartype].left   / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
      tx_rect.right  = ( float ) tabrect[bartype].right  / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
      tx_rect.top    = ( float ) tabrect[bartype].top    / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );
      tx_rect.bottom = ( float ) tabrect[bartype].bottom / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );

      width  = tabrect[bartype].right  - tabrect[bartype].left;
      height = tabrect[bartype].bottom - tabrect[bartype].top;

      sc_rect.left   = x;
      sc_rect.right  = x + width;
      sc_rect.top    = gfxState.scry - y;
      sc_rect.bottom = gfxState.scry - (y + height);

      draw_texture_box(&gui->TxBars, &tx_rect, &sc_rect);

      // Error check
      if ( maxticks > MAXTICK ) maxticks = MAXTICK;
      if ( ticks > maxticks ) ticks = maxticks;

      // Draw the full rows of ticks
      x += TABX;
      while ( ticks >= NUMTICK )
      {
        barrect[bartype].right = BARX;

        tx_rect.left   = ( float ) barrect[bartype].left   / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[bartype].right  / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[bartype].top    / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[bartype].bottom / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );

        width  = barrect[bartype].right  - barrect[bartype].left;
        height = barrect[bartype].bottom - barrect[bartype].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = gfxState.scry - y;
        sc_rect.bottom = gfxState.scry - (y + height);

        draw_texture_box(&gui->TxBars, &tx_rect, &sc_rect);

        y += BARY;
        ticks -= NUMTICK;
        maxticks -= NUMTICK;
      }


      // Draw any partial rows of ticks
      if ( maxticks > 0 )
      {
        // Draw the filled ones
        barrect[bartype].right = ( ticks << 3 ) + TABX;

        tx_rect.left   = ( float ) barrect[bartype].left   / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[bartype].right  / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[bartype].top    / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[bartype].bottom / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );

        width  = barrect[bartype].right  - barrect[bartype].left;
        height = barrect[bartype].bottom - barrect[bartype].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = gfxState.scry - y;
        sc_rect.bottom = gfxState.scry - (y + height);

        draw_texture_box(&gui->TxBars, &tx_rect, &sc_rect);

        // Draw the empty ones
        noticks = maxticks - ticks;
        if ( noticks > ( NUMTICK - ticks ) ) noticks = ( NUMTICK - ticks );
        barrect[0].right = ( noticks << 3 ) + TABX;

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );

        width  = barrect[0].right  - barrect[0].left;
        height = barrect[0].bottom - barrect[0].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = gfxState.scry - y;
        sc_rect.bottom = gfxState.scry - (y + height);

        draw_texture_box(&gui->TxBars, &tx_rect, &sc_rect);

        maxticks -= NUMTICK;
        y += BARY;
      }


      // Draw full rows of empty ticks
      while ( maxticks >= NUMTICK )
      {
        barrect[0].right = BARX;

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );

        width  = barrect[0].right  - barrect[0].left;
        height = barrect[0].bottom - barrect[0].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = gfxState.scry - y;
        sc_rect.bottom = gfxState.scry - (y + height);

        draw_texture_box(&gui->TxBars, &tx_rect, &sc_rect);

        y += BARY;
        maxticks -= NUMTICK;
      }


      // Draw the last of the empty ones
      if ( maxticks > 0 )
      {
        barrect[0].right = ( maxticks << 3 ) + TABX;

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLtexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLtexture_GetTextureHeight( &gui->TxBars );

        width  = barrect[0].right  - barrect[0].left;
        height = barrect[0].bottom - barrect[0].top;

        sc_rect.left   = x;
        sc_rect.right  = x + width;
        sc_rect.top    = gfxState.scry - y;
        sc_rect.bottom = gfxState.scry - (y + height);

        draw_texture_box(&gui->TxBars, &tx_rect, &sc_rect);

        y += BARY;
      }
    }
  }
  ATTRIB_POP( "draw_one_bar" );

  return y - ystt;
}

//--------------------------------------------------------------------------------------------
int draw_string( BMFont_t * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... )
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer
  char cTmp;
  GLfloat current_tint[4], temp_tint[4];
  STRING szText;
  va_list args;
  int cnt = 1;

  if(NULL ==pfnt || NULL == szFormat) return 0;

  // write the string to the buffer
  va_start( args, szFormat );
  vsnprintf( szText, sizeof(STRING), szFormat, args );
  va_end( args );

  // set the tint of the text
  glGetFloatv(GL_CURRENT_COLOR, current_tint);

  BeginText( &(pfnt->tex) );
  {
    if(NULL != tint)
    {
      temp_tint[0] = current_tint[0] * tint[0];
      temp_tint[1] = current_tint[1] * tint[1];
      temp_tint[2] = current_tint[2] * tint[2];
      temp_tint[3] = current_tint[3];
      glColor4fv(temp_tint);
    }

    cTmp = szText[0];
    while ( cTmp != 0 )
    {
      // Convert ASCII to our own little font
      if ( cTmp == '~' )
      {
        // Use squiggle for tab
        x = ((int)(x)) | TABAND;
      }
      else if ( cTmp == '\n' )
      {
        break;

      }
      else
      {
        // Normal letter
        cTmp = pfnt->ascii_table[cTmp];
        BMFont_draw_one( ui_getBMFont(), cTmp, x, y );
        x += pfnt->spacing_x[cTmp];
      }
      cTmp = szText[cnt];
      cnt++;
    }
  }
  EndText();

  // restore the old tint
  glColor4fv(current_tint);

  return pfnt->spacing_y;
}

//--------------------------------------------------------------------------------------------
int BMFont_word_size( BMFont_t * pfnt, char *szText )
{
  // ZZ> This function returns the number of pixels the
  //     next word will take on screen in the x direction

  // Count all preceeding spaces
  int x = 0;
  int cnt = 0;
  Uint8 cTmp = szText[cnt];

  while ( cTmp == ' ' || cTmp == '~' || cTmp == '\n' )
  {
    if ( cTmp == ' ' )
    {
      x += pfnt->spacing_x[pfnt->ascii_table[cTmp]];
    }
    else if ( cTmp == '~' )
    {
      x += TABAND + 1;
    }
    cnt++;
    cTmp = szText[cnt];
  }


  while ( cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0 )
  {
    x += pfnt->spacing_x[pfnt->ascii_table[cTmp]];
    cnt++;
    cTmp = szText[cnt];
  }
  return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string( BMFont_t * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... )
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer,
  //     wrapping over the right side and returning the new y value

  va_list args;
  STRING szText;
  int xstt, ystt, newy, cnt;
  char cTmp;
  bool_t newword;
  GLfloat current_tint[4], temp_tint[4];


  // write the string to the buffer
  va_start( args, szFormat );
  vsnprintf( szText, sizeof(STRING), szFormat, args );
  va_end( args );

  // set the tint of the text
  glGetFloatv(GL_CURRENT_COLOR, current_tint);
  if(NULL != tint)
  {
    temp_tint[0] = 1 - (1.0f - current_tint[0])*(1.0f - tint[0]);
    temp_tint[1] = 1 - (1.0f - current_tint[1])*(1.0f - tint[1]);
    temp_tint[2] = 1 - (1.0f - current_tint[2])*(1.0f - tint[2]);
    temp_tint[3] = current_tint[3];
    glColor4fv(tint);
  }

  BeginText( &(pfnt->tex) );
  {
    newword = btrue;
    xstt = x;
    ystt = y;
    maxx += xstt;
    newy = y + pfnt->spacing_y;

    cnt = 1;
    cTmp = szText[0];
    while ( cTmp != 0 )
    {
      // Check each new word for wrapping
      if ( newword )
      {
        int endx = x + BMFont_word_size( pfnt, szText + cnt - 1 );

        newword = bfalse;
        if ( endx > maxx )
        {
          // Wrap the end and cut off spaces and tabs
          x = xstt + pfnt->spacing_y;
          y += pfnt->spacing_y;
          newy += pfnt->spacing_y;
          while ( cTmp == ' ' || cTmp == '~' )
          {
            cTmp = szText[cnt];
            cnt++;
          }
        }
      }
      else
      {
        if ( cTmp == '~' )
        {
          // Use squiggle for tab
          x = ((int)(x)) | TABAND;
        }
        else if ( cTmp == '\n' )
        {
          x = xstt;
          y += pfnt->spacing_y;
          newy += pfnt->spacing_y;
        }
        else
        {
          // Normal letter
          cTmp = pfnt->ascii_table[cTmp];
          BMFont_draw_one( ui_getBMFont(), cTmp, x, y );
          x += pfnt->spacing_x[cTmp];
        }
        cTmp = szText[cnt];
        if ( cTmp == '~' || cTmp == ' ' )
        {
          newword = btrue;
        }
        cnt++;
      }
    }
  }
  EndText();

  // restore the old tint
  glColor4fv(current_tint);

  return newy-ystt;
}

//--------------------------------------------------------------------------------------------
int draw_status( BMFont_t * pfnt, Status_t * pstat )
{
  // ZZ> This function shows a character's icon, status and inventory
  //     The x,y coordinates are the top left point of the image to draw

  STRING szTmp;
  char cTmp;
  char *readtext;
  int  ix,iy, iystt;
  float life, lifemax;
  float mana, manamax;
  int cnt;
  CHR_REF item;

  PObj_t objlst;
  PChr_t chrlst;

  Game_t * gs;

  CHR_REF ichr;
  Chr_t * pchr;

  OBJ_REF iobj;
  Obj_t * pobj;
  Mad_t * pmad;
  Cap_t * pcap;

  gs = Graphics_requireGame(&gfxState);
  if(NULL == gs) return 0;

  objlst = gs->ObjList;
  chrlst = gs->ChrList;
  //madlst = gs->MadList;
  //caplst = gs->CapList;

  if(NULL == pstat) return 0;
  ichr = pstat->chr_ref;
  ix = pstat->pos.x;
  iy = pstat->pos.y;
  iystt = iy;

  pchr = ChrList_getPChr(gs, ichr);
  if( NULL == pchr ) return 0;

  iobj = ChrList_getRObj(gs, ichr);
  if( INVALID_OBJ == iobj ) return 0;
  pobj = gs->ObjList + iobj;

  pmad = ChrList_getPMad(gs, ichr);
  if(NULL == pmad) return 0;

  pcap = ChrList_getPCap(gs, ichr);
  if(NULL == pcap) return 0;

  life    = FP8_TO_FLOAT( pchr->stats.life_fp8 );
  lifemax = FP8_TO_FLOAT( pchr->stats.lifemax_fp8 );
  mana    = FP8_TO_FLOAT( pchr->stats.mana_fp8 );
  manamax = FP8_TO_FLOAT( pchr->stats.manamax_fp8 );

  // Write the character's first name
  if ( pchr->prop.nameknown )
    readtext = pchr->name;
  else
    readtext = pcap->classname;

  for ( cnt = 0; cnt < 6; cnt++ )
  {
    cTmp = readtext[cnt];
    if ( cTmp == ' ' || cTmp == 0 )
    {
      szTmp[cnt] = 0;
      break;

    }
    else
      szTmp[cnt] = cTmp;
  }
  szTmp[6] = 0;
  iy += draw_string( pfnt, ix + 8, iy, NULL, szTmp );

  // Write the character's money
  iy += 8 + draw_string( pfnt, ix + 8, iy, NULL, "$%4d", pchr->money );

  // Draw the icons
  draw_one_icon( gs->skintoicon[pchr->skin_ref + pobj->skinstart], ix + 40, iy, pchr->sparkle );
  item = chr_get_holdingwhich( chrlst, CHRLST_COUNT, ichr, SLOT_LEFT );
  if ( ACTIVE_CHR( chrlst,  item ) && VALID_OBJ( objlst, chrlst[item].model) )
  {
    Chr_t * tmppchr = ChrList_getPChr(gs, item);
    Obj_t * tmppobj = ChrList_getPObj(gs, item);
    Cap_t * tmppcap = ChrList_getPCap(gs, item);

    if ( tmppchr->prop.icon )
    {
      draw_one_icon( gs->skintoicon[tmppchr->skin_ref + tmppobj->skinstart], ix + 8, iy, tmppchr->sparkle );
      if ( tmppchr->ammomax != 0 && tmppchr->ammoknown )
      {
        if ( !tmppcap->prop.isstackable || tmppchr->ammo > 1 )
        {
          // Show amount of ammo left
          draw_string( pfnt, ix + 8, iy - 8, NULL, "%2d", tmppchr->ammo );
        }
      }
    }
    else
      draw_one_icon( gs->ico_lst[ICO_BOOK_0] + ( tmppchr->money % MAXSKIN ), ix + 8, iy, tmppchr->sparkle );
  }
  else
    draw_one_icon( gs->ico_lst[ICO_NULL], ix + 8, iy, NOSPARKLE );

  item = chr_get_holdingwhich( chrlst, CHRLST_COUNT, ichr, SLOT_RIGHT );
  if ( ACTIVE_CHR( chrlst,  item ) && VALID_OBJ( objlst, chrlst[item].model) )
  {
    Chr_t * tmppchr = ChrList_getPChr(gs, item);
    Obj_t * tmppobj = ChrList_getPObj(gs, item);
    Cap_t * tmppcap = ChrList_getPCap(gs, item);

    if ( tmppchr->prop.icon )
    {
      draw_one_icon( gs->skintoicon[tmppchr->skin_ref + tmppobj->skinstart], ix + 72, iy, tmppchr->sparkle );
      if ( tmppchr->ammomax != 0 && tmppchr->ammoknown )
      {
        if ( !tmppcap->prop.isstackable || tmppchr->ammo > 1 )
        {
          // Show amount of ammo left
         draw_string( pfnt, ix + 72, iy - 8, NULL, "%2d", tmppchr->ammo );
        }
      }
    }
    else
      draw_one_icon( gs->ico_lst[ICO_BOOK_0] + ( chrlst[item].money % MAXSKIN ), ix + 72, iy, chrlst[item].sparkle );
  }
  else
    draw_one_icon( gs->ico_lst[ICO_NULL], ix + 72, iy, NOSPARKLE );

  iy += 32;

  // Draw the bars
  if ( pchr->alive )
  {
    iy += draw_one_bar( pchr->lifecolor, ix, iy, life, lifemax );
    iy += draw_one_bar( pchr->manacolor, ix, iy, mana, manamax );
  }
  else
  {
    iy += draw_one_bar( 0, ix, iy, 0, lifemax ); // Draw a black bar
    iy += draw_one_bar( 0, ix, iy, 0, manamax ); // Draw a black bar
  };


  return iy - iystt;
}

//--------------------------------------------------------------------------------------------
bool_t do_map()
{
  PLA_REF ipla;
  CHR_REF ichr;
  int cnt;

  Game_t * gs    = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);

  if(!mapon) return bfalse;

  draw_map( maprect.left, maprect.top );

  for ( cnt = 0; cnt < numblip; cnt++ )
  {
    draw_blip( BlipList[cnt].c, maprect.left + BlipList[cnt].x, maprect.top + BlipList[cnt].y );
  };

  if ( youarehereon && 0 == ( ( gfxState.fps_clock >> 3) & 1 ) )
  {
    for ( ipla = 0; ipla < PLALST_COUNT; ipla++ )
    {
      if ( !VALID_PLA( gs->PlaList, ipla ) || INBITS_NONE == gs->PlaList[ipla].device ) continue;

      ichr = PlaList_getRChr( gs, ipla );
      if ( !ACTIVE_CHR( gs->ChrList,  ichr ) || !gs->ChrList[ichr].alive ) continue;

      draw_blip( (COLR)0, maprect.left + MAPSIZE * mesh_fraction_x( &(pmesh->Info), gs->ChrList[ichr].ori.pos.x ) * mapscale, maprect.top + MAPSIZE * mesh_fraction_y( &(pmesh->Info), gs->ChrList[ichr].ori.pos.y ) * mapscale );
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
int do_messages( BMFont_t * pfnt, int x, int y )
{
  int cnt, tnc;
  int ystt = y;

  Game_t * gs  = Graphics_requireGame(&gfxState);
  Gui_t  * gui = gui_getState();
  MessageQueue_t * mq = &(gui->msgQueue);
  MESSAGE_ELEMENT * msg;

  if ( NULL ==pfnt || !CData.messageon ) return 0;

  // Display the messages
  tnc = mq->start;
  for ( cnt = 0; cnt < CData.maxmessage; cnt++ )
  {
    msg = mq->list + tnc;

    // mesages with negative times never time out!
    if ( msg->time != 0 )
    {
      y += draw_wrap_string( pfnt, x, y, NULL, gfxState.scrx - CData.wraptolerance - x, msg->textdisplay );

      if ( msg->time > 0 )
      {
        msg->time -= mq->timechange;
        if ( msg->time < 0 ) msg->time = 0;
      };
    }
    tnc++;
    tnc %= CData.maxmessage;
  }

  mq->timechange = 0;

  return y - ystt;
}

//--------------------------------------------------------------------------------------------
int do_status( Client_t * cs, BMFont_t * pfnt, int x, int y)
{
  Uint32 cnt;
  int ystt = y;

  if ( !CData.staton ) return 0;

  for ( cnt = 0; cnt < cs->StatList_count && y < gfxState.scry; cnt++ )
  {
    cs->StatList[cnt].pos.x = x;
    cs->StatList[cnt].pos.y = y;

    y += draw_status( pfnt, cs->StatList + cnt );
  };

  return y - ystt;
};

//--------------------------------------------------------------------------------------------
void draw_text( BMFont_t *  pfnt )
{
  // ZZ> This function spits out some words

  char text[512];
  int y, fifties, seconds, minutes;

  KeyboardBuffer_t * kbuffer = KeyboardBuffer_getState();
  Game_t * gs  = Graphics_requireGame(&gfxState);
  Gui_t  * gui = gui_getState();

  Begin2DMode();
  {
    // Status_t bars
    y = 0;
    y += do_status( gs->cl, pfnt, gfxState.scrx - BARX, y);

    // Map display
    do_map();

    y = 0;
    if ( outofsync )
    {
      y += draw_string( pfnt, 0, y, NULL, "OUT OF SYNC!" );
    }

    if ( parseerror && CData.DevMode )
    {
      y += draw_string( pfnt, 0, y, NULL, "SCRIPT ERROR ( SEE LOG.TXT )" );
    }

    if ( CData.fpson )
    {
      CHR_REF pla_chr = PlaList_getRChr( gs, PLA_REF(0) );

      y += draw_string( pfnt, 0, y, NULL, "%2.3f FPS, %2.3f UPS", gfxState.stabilized_fps, stabilized_ups );

      if( CData.DevMode )
      {
        y += draw_string( pfnt, 0, y, NULL, "estimated max FPS %2.3f", gfxState.est_max_fps );
        //y += draw_string( pfnt, 0, y, NULL, "wld_frame %d, gs->wld_clock %d, all_clock %d", gs->wld_frame, gs->wld_clock, gs->all_clock );
        //y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", gs->ChrList[pla_chr].ori.pos.x, gs->ChrList[pla_chr].ori.pos.y, gs->ChrList[pla_chr].ori.pos.z );
        //y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", gs->ChrList[pla_chr].ori.vel.x, gs->ChrList[pla_chr].ori.vel.y, gs->ChrList[pla_chr].ori.vel.z );
        y += draw_string( pfnt, 0, y, NULL, "character collision frac %1.3f", (float)chr_collisions / (float)CHR_MAX_COLLISIONS );
      }


      {
        MachineState_t * mac = get_MachineState();
        MachineState_update( mac );

        y += draw_string( pfnt, 0, y, NULL, "YLB %d, %d, %d, %2.2f ", mac->i_bishop_time_y, mac->i_bishop_time_m, mac->i_bishop_time_d, mac->f_bishop_time_h );
      }

    }

    {
      CHR_REF ichr, iref;
      GLvector tint = {0.5, 1.0, 1.0, 1.0};

      ichr = PlaList_getRChr( gs, PLA_REF(0) );
      if( ACTIVE_CHR( gs->ChrList,  ichr) )
      {
        iref = chr_get_attachedto( gs->ChrList, CHRLST_COUNT,ichr);
        if( ACTIVE_CHR( gs->ChrList, iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 holder == %s(%s)", gs->ChrList[iref].name, ChrList_getPCap(gs, iref)->classname );
        };

        iref = chr_get_inwhichpack( gs->ChrList, CHRLST_COUNT,ichr);
        if( ACTIVE_CHR( gs->ChrList, iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 packer == %s(%s)", gs->ChrList[iref].name, ChrList_getPCap(gs, iref)->classname );
        };

        iref = chr_get_onwhichplatform( gs->ChrList, CHRLST_COUNT,ichr);
        if( ACTIVE_CHR( gs->ChrList, iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 platform == %s(%s)", gs->ChrList[iref].name, ChrList_getPCap(gs, iref)->classname );
        };

      };
    }

    if ( SDLKEYDOWN( SDLK_F1 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!MOUSE HELP!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );
      y += draw_string( pfnt, 0, y, NULL, "~Left Click to use an item" );
      y += draw_string( pfnt, 0, y, NULL, "~Left and Right Click to grab" );
      y += draw_string( pfnt, 0, y, NULL, "~Middle Click to jump" );
      y += draw_string( pfnt, 0, y, NULL, "~A and S keys do stuff" );
      y += draw_string( pfnt, 0, y, NULL, "~Right Drag to move camera" );
    }

    if ( SDLKEYDOWN( SDLK_F2 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!JOYSTICK HELP!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );
      y += draw_string( pfnt, 0, y, NULL, "~Hit the buttons" );
      y += draw_string( pfnt, 0, y, NULL, "~You'll figure it out" );
    }

    if ( SDLKEYDOWN( SDLK_F3 ) )
    {
      // In-Game help
      y += draw_string( pfnt, 0, y, NULL, "!!!KEYBOARD HELP!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~Edit CONTROLS.TXT to change" );
      y += draw_string( pfnt, 0, y, NULL, "~TGB controls left hand" );
      y += draw_string( pfnt, 0, y, NULL, "~YHN controls right hand" );
      y += draw_string( pfnt, 0, y, NULL, "~Keypad to move and jump" );
      y += draw_string( pfnt, 0, y, NULL, "~Number keys for stats" );
    }


    // Player_t DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F5 ) && CData.DevMode )
    {
      CHR_REF pla_chr = PlaList_getRChr( gs, PLA_REF(0) );

      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-5!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f", GCamera.pos.x, GCamera.pos.y );

      y += draw_string( pfnt, 0, y, NULL, "  PLA0DEF %d %d %d %d %d %d %d %d",
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[0]&DAMAGE_SHIFT,
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[1]&DAMAGE_SHIFT,
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[2]&DAMAGE_SHIFT,
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[3]&DAMAGE_SHIFT,
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[4]&DAMAGE_SHIFT,
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[5]&DAMAGE_SHIFT,
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[6]&DAMAGE_SHIFT,
                gs->ChrList[pla_chr].skin.damagemodifier_fp8[7]&DAMAGE_SHIFT );

      y += draw_string( pfnt, 0, y, NULL, "~PLA0 %5.1f %5.1f", gs->ChrList[pla_chr].ori.pos.x / 128.0, gs->ChrList[pla_chr].ori.pos.y / 128.0  );

      pla_chr = PlaList_getRChr( gs, PLA_REF(1) );
      y += draw_string( pfnt, 0, y, NULL, "~PLA1 %5.1f %5.1f", gs->ChrList[pla_chr].ori.pos.x / 128.0, gs->ChrList[pla_chr].ori.pos.y / 128.0 );
    }

    if(ups_loops > 0)
    {
      y += draw_string( pfnt, 0, y, NULL, "ChrHeap - %3.3lf : %3.3lf", 1e6 * clktime_ChrHeap / (double)ups_loops, clkcount_ChrHeap / (double)ups_loops );
      y += draw_string( pfnt, 0, y, NULL, "EncHeap - %3.3lf : %3.3lf", 1e6 * clktime_EncHeap / (double)ups_loops, clkcount_EncHeap / (double)ups_loops );
      y += draw_string( pfnt, 0, y, NULL, "PrtHeap - %3.3lf : %3.3lf", 1e6 * clktime_PrtHeap / (double)ups_loops, clkcount_PrtHeap / (double)ups_loops );
      y += draw_string( pfnt, 0, y, NULL, "ekey    - %3.3lf : %3.3lf", 1e6 * clktime_ekey / (double)ups_loops,    clkcount_ekey / (double)ups_loops );
    };

    // GLOBAL DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F6 ) && CData.DevMode )
    {
      char * net_status;

      net_status = "? invalid ?";
      if ( Game_isClientServer(gs) )
      {
        net_status = "client + server";
      }
      else if ( Game_isClient(gs) )
      {
        net_status = "client";
      }
      else if ( Game_isServer(gs) )
      {
        net_status = "server";
      }
      else if( Game_isLocal(gs) )
      {
        net_status = "local game";
      };

      // More debug information
      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-6!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~GAME TYPE - %s", net_status );
      y += draw_string( pfnt, 0, y, NULL, "~NETPLAYERS - %d", gs->sv->num_loaded );
      y += draw_string( pfnt, 0, y, NULL, "~FREEPRT - %d", gs->PrtHeap.free_count );
      y += draw_string( pfnt, 0, y, NULL, "~FREECHR - %d", gs->ChrHeap.free_count );
      y += draw_string( pfnt, 0, y, NULL, "~EXPORT - %s", gs->modstate.exportvalid ? "TRUE" : "FALSE" );
      y += draw_string( pfnt, 0, y, NULL, "~FOG&WATER - %s", gs->Fog.affectswater ? "TRUE" : "FALSE"  );
      y += draw_string( pfnt, 0, y, NULL, "~SHOP - %d", gs->ShopList_count );
      y += draw_string( pfnt, 0, y, NULL, "~PASSAGE - %d", gs->PassList_count );
      y += draw_string( pfnt, 0, y, NULL, "~DAMAGEPART - %d", gs->Tile_Dam.parttype );
    }

    // Camera_t DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F7 ) && CData.DevMode )
    {
      // White debug mode
      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-7!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ).CNV( 0, 0 ), ( GCamera.mView ).CNV( 1, 0 ), ( GCamera.mView ).CNV( 2, 0 ), ( GCamera.mView ).CNV( 3, 0 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ).CNV( 0, 1 ), ( GCamera.mView ).CNV( 1, 1 ), ( GCamera.mView ).CNV( 2, 1 ), ( GCamera.mView ).CNV( 3, 1 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ).CNV( 0, 2 ), ( GCamera.mView ).CNV( 1, 2 ), ( GCamera.mView ).CNV( 2, 2 ), ( GCamera.mView ).CNV( 3, 2 ) );
      y += draw_string( pfnt, 0, y, NULL, "~CAM %f %f %f %f", ( GCamera.mView ).CNV( 0, 3 ), ( GCamera.mView ).CNV( 1, 3 ), ( GCamera.mView ).CNV( 2, 3 ), ( GCamera.mView ).CNV( 3, 3 ) );
      y += draw_string( pfnt, 0, y, NULL, "~x %f, %f", GCamera.centerpos.x, GCamera.trackpos.x );
      y += draw_string( pfnt, 0, y, NULL, "~y %f %f", GCamera.centerpos.y, GCamera.trackpos.y );
      y += draw_string( pfnt, 0, y, NULL, "~turn %d %d", CData.autoturncamera, doturntime );
    }

    //Draw paused text
    if ( gs->proc.Paused && !SDLKEYDOWN( SDLK_F11 ) )
    {
      snprintf( text, sizeof( text ), "GAME PAUSED" );
      draw_string( pfnt, -90 + gfxState.scrx / 2, 0 + gfxState.scry / 2, NULL, text  );
    }

    // TIMER
    if ( timeron )
    {
      fifties = ( timervalue % 50 ) << 1;
      seconds = (( timervalue / 50 ) % 60 );
      minutes = ( timervalue / 3000 );
      y += draw_string( pfnt, 0, y, NULL, "=%d:%02d:%02d=", minutes, seconds, fifties );
    }

    // WAITING TEXT
    if ( gs->cl->waiting )
    {
      y += draw_string( pfnt, 0, y, NULL, "Waiting for players... " );
    }

    // MODULE EXIT TEXT
    if ( gs->modstate.beat )
    {
      y += draw_string( pfnt, 0, y, NULL, "VICTORY!  PRESS ESCAPE" );
    }
    else if ( gs->allpladead && !gs->modstate.respawnvalid )
    {
      y += draw_string( pfnt, 0, y, NULL, "PRESS ESCAPE TO QUIT" );
    }
    else if (( gs->allpladead && gs->modstate.respawnvalid ) || ( gs->somepladead && gs->modstate.respawnanytime ) )
    {
      y += draw_string( pfnt, 0, y, NULL, "JUMP TO RESPAWN" );
    }

    // Network message input
    if ( keyb.mode )
    {
      y += draw_wrap_string( pfnt, 0, y, NULL, gfxState.scrx - CData.wraptolerance, kbuffer->buffer );
    }

    // Messages
    y += do_messages( pfnt, 0, y );
  }
  End2DMode();
}

//--------------------------------------------------------------------------------------------
bool_t query_clear()
{
  return gfxState.clear_requested;
};

//--------------------------------------------------------------------------------------------
bool_t query_pageflip()
{
  return gfxState.pageflip_requested;
};

//--------------------------------------------------------------------------------------------
bool_t request_pageflip()
{
  bool_t retval = !gfxState.pageflip_requested;
  gfxState.pageflip_requested = btrue;

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t request_pageflip_pause()
{
  bool_t retval = !gfxState.pageflip_paused;
  gfxState.pageflip_pause_requested = btrue;

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t request_pageflip_unpause()
{
  bool_t retval = gfxState.pageflip_paused;
  gfxState.pageflip_unpause_requested = btrue;

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t do_pageflip()
{
  // BB > actualy attempt a pageflip. It is possible to freeze the whole pageflipping
  //      process by calling request_pageflip_pause(). It will freeze the screen after the next
  //      actual pageflip. This can be overriden by calling request_pageflip_unpause() at any time.

  bool_t retval = gfxState.pageflip_requested;

  if ( gfxState.pageflip_requested && !gfxState.pageflip_paused )
  {
    SDL_GL_SwapBuffers();

    // only pause on a pageflip
    if(gfxState.pageflip_pause_requested && !gfxState.pageflip_unpause_requested)
    {
      gfxState.pageflip_paused = btrue;
      gfxState.pageflip_pause_requested = bfalse;
    }

    // only clear if the pageflip is not paused
    // otherwise, what's the point?
    if(!gfxState.pageflip_paused)
    {
      gfxState.clear_requested = btrue;
    };
  }

  // update the fps_loops and pageflip_requested variables 
  // just AS IF the page was flipping
  if( gfxState.pageflip_requested )
  {
    gfxState.fps_loops++;
    gfxState.pageflip_requested = bfalse;
  }

  // unpause at any time
  if(gfxState.pageflip_unpause_requested)
  {
    gfxState.pageflip_paused = bfalse;
    gfxState.pageflip_unpause_requested = bfalse;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t do_clear()
{
  bool_t retval = gfxState.clear_requested;

  if ( gfxState.clear_requested )
  {
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    gfxState.clear_requested = bfalse;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t draw_scene(Game_t * gs)
{
  if(NULL == gs) gs = Graphics_getGame(&gfxState);
  if(NULL == gs) return bfalse;

  Begin3DMode();

  make_prtlist(gs);
  do_dyna_light(gs);
  do_chr_dynalight(gs);
  do_prt_dynalight(gs);

  // Render the background
  if ( gfxState.render_background )
  {
    render_background( TX_WATER_LOW );   // TX_WATER_LOW == 6 is the texture for waterlow.bmp
  }

  draw_scene_zreflection();

  //Foreground overlay
  if ( gfxState.render_overlay )
  {
    render_foreground_overlay( TX_WATER_TOP );   // TX_WATER_TOP ==  5 is watertop.bmp
  }

  End3DMode();

  return btrue;
}

//--------------------------------------------------------------------------------------------
void draw_main( float frameDuration )
{
  // ZZ> This function does all the drawing stuff

  draw_scene( NULL );

  draw_text( ui_getBMFont() );

  request_pageflip();
}

//--------------------------------------------------------------------------------------------
void load_blip_bitmap( char * modname )
{
  //This function loads the blip bitmaps
  Gui_t * gui = gui_getState();
  int cnt;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.blip_bitmap );
  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  &gui->TxBlip, CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.blip_bitmap );
    if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D,  &gui->TxBlip, CStringTmp1, INVALID_KEY ) )
    {
      log_warning( "Blip bitmap not loaded. Missing file = \"%s\"\n", CStringTmp1 );
    }
  };



  // Set up the rectangles
  blipwidth  = gui->TxBlip.imgW / NUMBAR;
  blipheight = gui->TxBlip.imgH;
  for ( cnt = 0; cnt < NUMBAR; cnt++ )
  {
    BlipList[cnt].rect.left   = ( cnt + 0 ) * blipwidth;
    BlipList[cnt].rect.right  = ( cnt + 1 ) * blipwidth;
    BlipList[cnt].rect.top    = 0;
    BlipList[cnt].rect.bottom = blipheight;
  }
}

//--------------------------------------------------------------------------------------------
void load_menu()
{
  // ZZ> This function loads all of the menu data...  Images are loaded into system
  // memory

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.font_bitmap );
  snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.fontdef_file );
  ui_load_BMFont( CStringTmp1, CStringTmp2 );
}



/********************> Reshape3D() <*****/
void Reshape3D( int w, int h )
{
  glViewport( 0, 0, w, h );
}
//--------------------------------------------------------------------------------------------
bool_t glinit( Graphics_t * g, ConfigData_t * cd )
{
  if(NULL == g || NULL == cd) return bfalse;

  // get the maximum anisotropy fupported by the video vard
  glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &(g->maxAnisotropy) );
  g->log2Anisotropy = ( g->maxAnisotropy == 0 ) ? 0 : floor( log( g->maxAnisotropy + 1e-6 ) / log( 2.0f ) );

  if ( g->maxAnisotropy == 0.0f && g->texturefilter >= TX_ANISOTROPIC )
  {
    g->texturefilter = TX_TRILINEAR_2;
    gfxState.texturefilter = g->texturefilter;
  }
  g->userAnisotropy = MIN( g->maxAnisotropy, g->userAnisotropy );

  /* Depth testing stuff */
  glClearDepth( 1.0 );
  glDepthFunc( GL_LESS );
  glEnable( GL_DEPTH_TEST );

  //Load the current graphical settings
  gl_set_mode(g);

  //Check which particle image to load
  if      ( cd->particletype == PART_NORMAL ) strncpy( cd->particle_bitmap, "particle_normal.png" , sizeof( STRING ) );
  else if ( cd->particletype == PART_SMOOTH ) strncpy( cd->particle_bitmap, "particle_smooth.png" , sizeof( STRING ) );
  else if ( cd->particletype == PART_FAST  )  strncpy( cd->particle_bitmap, "particle_fast.png" , sizeof( STRING ) );

  // set up environment mapping
  glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
  glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

  EnableTexturing();  // Enable texture mapping

  return btrue;
}



//--------------------------------------------------------------------------------------------
bool_t CGraphics_delete(Graphics_t * g)
{
  if(NULL == g) return bfalse;
  if( !EKEY_PVALID(g) ) return btrue;

  EKEY_PINVALIDATE(g);

  return btrue;
}

//--------------------------------------------------------------------------------------------
Graphics_t * Graphics_new(Graphics_t * g, ConfigData_t * cd)
{
  //fprintf( stdout, "Graphics_new()\n");

  if(NULL == g) return g;

  CGraphics_delete( g );

  memset(g, 0, sizeof(Graphics_t));

  if( NULL == cd ) return NULL;

  EKEY_PNEW( g, Graphics_t );

  Graphics_synch(g, cd);

  // put a reasonable value in here
  g->est_max_fps           = 30;


  g->stabilized_fps        = TARGETFPS;
  g->stabilized_fps_sum    = TARGETFPS;
  g->stabilized_fps_weight = 1;
  g->pageflip_requested    = bfalse;
  g->clear_requested       = btrue;

  return g;
};

//--------------------------------------------------------------------------------------------
Game_t * Graphics_getGame(Graphics_t * g)
{
  if(NULL == g) return NULL;
  return g->gs;
}

//--------------------------------------------------------------------------------------------
Game_t * Graphics_requireGame(Graphics_t * g)
{
  assert(NULL != g && NULL != g->gs);
  return g->gs;
}

//--------------------------------------------------------------------------------------------
bool_t Graphics_hasGame(Graphics_t * g)
{
  return (NULL != g) && (NULL != g->gs);
}

//--------------------------------------------------------------------------------------------
bool_t Graphics_matchesGame(Graphics_t * g, Game_t * gs)
{
  if(NULL == g) return bfalse;
  return g->gs == gs;
}

//--------------------------------------------------------------------------------------------
retval_t Graphics_ensureGame(Graphics_t * g, Game_t * gs)
{
  if(NULL == g) return rv_error;

  if(NULL == g->gs) g->gs = gs;

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
retval_t Graphics_removeGame(Graphics_t * g, Game_t * gs)
{
  if(NULL == g) return rv_error;

  if(gs == g->gs) g->gs = NULL;

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
bool_t Graphics_synch(Graphics_t * g, ConfigData_t * cd)
{
  bool_t changed = bfalse;

  if(NULL == g || NULL == cd) return bfalse;

  g->GrabMouse = cd->GrabMouse;
  g->HideMouse = cd->HideMouse;

  g->shading            = cd->shading;
  g->render_overlay     = cd->render_overlay;
  g->render_background  = cd->render_background;
  g->phongon            = cd->phongon;

  if(g->perspective != cd->perspective)
  {
    g->perspective = cd->perspective;
    changed = btrue;
  }

  if(g->antialiasing != cd->antialiasing)
  {
    g->antialiasing = cd->antialiasing;
    changed = btrue;
  }


  if(g->dither != cd->dither)
  {
    g->dither = cd->dither;
    changed = btrue;
  }

  if(g->vsync != cd->vsync)
  {
    g->vsync = cd->vsync;
    changed = btrue;
  }

  if(g->texturefilter != cd->texturefilter)
  {
    g->texturefilter = cd->texturefilter;
    changed = btrue;
  }

  if(g->gfxacceleration != cd->gfxacceleration)
  {
    g->gfxacceleration = cd->gfxacceleration;
    changed = btrue;
  }

  if(g->fullscreen != cd->fullscreen)
  {
    g->fullscreen = cd->fullscreen;
    changed = btrue;
  }

  if(g->scrd != cd->scrd)
  {
    g->scrd = cd->scrd;
    changed = btrue;
  }

  if(g->scrx != cd->scrx)
  {
    g->scrx = cd->scrx;
    changed = btrue;
  }

  if(g->scry != cd->scry)
  {
    g->scry = cd->scry;
    changed = btrue;
  }

  if(g->scrz != cd->scrz)
  {
    g->scrz = cd->scrz;
    changed = btrue;
  }

  if(changed)
  {
    // to get this to work properly, you need to reload all textures!
    gl_set_mode(g);
  }

  return btrue;
}



/* obsolete graphics functions */
////--------------------------------------------------------------------------------------------
//void draw_titleimage(int image, int x, int y)
//{
//  // ZZ> This function draws a title image on the backbuffer
//
//  GLfloat txWidth, txHeight;
//
//  if ( INVALID_TEXTURE != GLtexture_GetTextureID(mproc->TxTitleImage + image) )
//  {
//    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//    Begin2DMode();
//    glNormal3f( 0, 0, 1 ); // glNormal3f( 0, 1, 0 );
//
//    /* Calculate the texture width & height */
//    txWidth = ( GLfloat )( GLtexture_GetImageWidth( mproc->TxTitleImage + image )/GLtexture_GetTextureWidth( mproc->TxTitleImage + image ) );
//    txHeight = ( GLfloat )( GLtexture_GetImageHeight( mproc->TxTitleImage + image )/GLtexture_GetTextureHeight( mproc->TxTitleImage + image ) );
//
//    /* Bind the texture */
//    GLtexture_Bind( mproc->TxTitleImage + image, &gfxState );
//
//    /* Draw the quad */
//    glBegin( GL_QUADS );
//    glTexCoord2f( 0, 1 ); glVertex2f( x, gfxState.scry - y - GLtexture_GetImageHeight( mproc->TxTitleImage + image ) );
//    glTexCoord2f( txWidth, 1 ); glVertex2f( x + GLtexture_GetImageWidth( mproc->TxTitleImage + image ), gfxState.scry - y - GLtexture_GetImageHeight( mproc->TxTitleImage + image ) );
//    glTexCoord2f( txWidth, 1-txHeight ); glVertex2f( x + GLtexture_GetImageWidth( mproc->TxTitleImage + image ), gfxState.scry - y );
//    glTexCoord2f( 0, 1-txHeight ); glVertex2f( x, gfxState.scry - y );
//    glEnd();
//
//    End2DMode();
//  }
//}


//--------------------------------------------------------------------------------------------
void dolist_add( CHR_REF chr_ref )
{
  // This function puts a character in the list
  int fan;

  Game_t * gs = Graphics_requireGame(&gfxState);

  if ( !ACTIVE_CHR( gs->ChrList,  chr_ref ) || gs->ChrList[chr_ref].indolist ) return;

  fan = gs->ChrList[chr_ref].onwhichfan;
  //if ( mesh_fan_remove_renderlist( fan ) )
  {
    //gs->ChrList[chr_ref].lightspek_fp8 = Mesh[meshvrtstart[fan]].vrtl_fp8;
    dolist[numdolist] = chr_ref;
    gs->ChrList[chr_ref].indolist = btrue;
    numdolist++;


    // Do flashing
    if (( gfxState.fps_loops & gs->ChrList[chr_ref].flashand ) == 0 && gs->ChrList[chr_ref].flashand != DONTFLASH )
    {
      flash_character( gs, chr_ref, 255 );
    }
    // Do blacking
    if (( gfxState.fps_loops&SEEKURSEAND ) == 0 && gs->cl->seekurse && gs->ChrList[chr_ref].prop.iskursed )
    {
      flash_character( gs, chr_ref, 0 );
    }
  }
  //else
  //{
  //  OBJ_REF model = gs->ChrList[chr_ref].model;
  //  assert( MADLST_COUNT != VALIDATE_MDL( model ) );

  //  // Double check for large/special objects
  //  if ( gs->CapList[model].prop.alwaysdraw )
  //  {
  //    dolist[numdolist] = chr_ref;
  //    gs->ChrList[chr_ref].indolist = btrue;
  //    numdolist++;
  //  }
  //}

  // Add its weapons too
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    dolist_add( chr_get_holdingwhich(  gs->ChrList, CHRLST_COUNT, chr_ref, _slot ) );
  };

}

//--------------------------------------------------------------------------------------------
void dolist_sort( void )
{
  // ZZ> This function orders the dolist based on distance from camera,
  //     which is needed for reflections to properly clip themselves.
  //     Order from closest to farthest

  Game_t * gs = Graphics_requireGame(&gfxState);

  CHR_REF chr_ref, olddolist[CHRLST_COUNT];
  int cnt, tnc, order;
  int dist[CHRLST_COUNT];

  // Figure the distance of each
  cnt = 0;
  while ( cnt < numdolist )
  {
    chr_ref = dolist[cnt];  olddolist[cnt] = chr_ref;
    if ( gs->ChrList[chr_ref].light_fp8 != 255 || gs->ChrList[chr_ref].alpha_fp8 != 255 )
    {
      // This makes stuff inside an invisible character visible...
      // A key inside a Jellcube, for example
      dist[cnt] = 0x7fffffff;
    }
    else
    {
      dist[cnt] = ABS( gs->ChrList[chr_ref].ori.pos.x - GCamera.pos.x ) + ABS( gs->ChrList[chr_ref].ori.pos.y - GCamera.pos.y );
    }
    cnt++;
  }


  // Put em in the right order
  cnt = 0;
  while ( cnt < numdolist )
  {
    chr_ref = olddolist[cnt];
    order = 0;  // Assume this chr_ref is closest
    tnc = 0;
    while ( tnc < numdolist )
    {
      // For each one closer, increment the order
      order += ( dist[cnt] > dist[tnc] );
      order += ( dist[cnt] == dist[tnc] ) && ( cnt < tnc );
      tnc++;
    }
    dolist[order] = chr_ref;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void dolist_make( void )
{
  // ZZ> This function finds the characters that need to be drawn and puts them in the list

  Game_t * gs = Graphics_requireGame(&gfxState);

  int cnt;
  CHR_REF chr_ref, chr_cnt;


  // Remove everyone from the dolist
  cnt = 0;
  while ( cnt < numdolist )
  {
    chr_ref = dolist[cnt];
    gs->ChrList[chr_ref].indolist = bfalse;
    cnt++;
  }
  numdolist = 0;


  // Now fill it up again
  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++)
  {
    if( !ACTIVE_CHR(gs->ChrList, chr_cnt) ) continue;

    if ( !chr_in_pack( gs->ChrList, CHRLST_COUNT, chr_cnt ) )
    {
      dolist_add( chr_cnt );
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t CVolume_draw( CVolume_t * cv, bool_t draw_square, bool_t draw_diamond  )
{
  bool_t retval = bfalse;

  if(NULL == cv) return bfalse;

  ATTRIB_PUSH( "CVolume_draw", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT );
  {
    // don't write into the depth buffer
    glDepthMask( GL_FALSE );
    glEnable(GL_DEPTH_TEST);

    // fix the poorly chosen normals...
    glDisable( GL_CULL_FACE );

    // make them transparent
    glEnable(GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // choose a "white" texture
    GLtexture_Bind(NULL, &gfxState);

    //------------------------------------------------
    // DIAGONAL BBOX

    if(cv->lod > 1 && draw_diamond)
    {
      float p1_x, p1_y;
      float p2_x, p2_y;

      glColor4f(0.5, 1, 1, 0.1);

      p1_x = 0.5f * (cv->xy_max - cv->yx_max);
      p1_y = 0.5f * (cv->xy_max + cv->yx_max);
      p2_x = 0.5f * (cv->xy_max - cv->yx_min);
      p2_y = 0.5f * (cv->xy_max + cv->yx_min);

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_max);
        glVertex3f(p1_x, p1_y, cv->z_max);
      glEnd();

      p1_x = 0.5f * (cv->xy_max - cv->yx_min);
      p1_y = 0.5f * (cv->xy_max + cv->yx_min);
      p2_x = 0.5f * (cv->xy_min - cv->yx_min);
      p2_y = 0.5f * (cv->xy_min + cv->yx_min);

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_max);
        glVertex3f(p1_x, p1_y, cv->z_max);
      glEnd();

      p1_x = 0.5f * (cv->xy_min - cv->yx_min);
      p1_y = 0.5f * (cv->xy_min + cv->yx_min);
      p2_x = 0.5f * (cv->xy_min - cv->yx_max);
      p2_y = 0.5f * (cv->xy_min + cv->yx_max);

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_max);
        glVertex3f(p1_x, p1_y, cv->z_max);
      glEnd();

      p1_x = 0.5f * (cv->xy_min - cv->yx_max);
      p1_y = 0.5f * (cv->xy_min + cv->yx_max);
      p2_x = 0.5f * (cv->xy_max - cv->yx_max);
      p2_y = 0.5f * (cv->xy_max + cv->yx_max);

      glBegin(GL_QUADS);
        glVertex3f(p1_x, p1_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_min);
        glVertex3f(p2_x, p2_y, cv->z_max);
        glVertex3f(p1_x, p1_y, cv->z_max);
      glEnd();

      retval = btrue;
    }

    //------------------------------------------------
    // SQUARE BBOX
    if(draw_square)
    {
      glColor4f(1, 0.5, 1, 0.1);

      // XZ FACE, min Y
      glBegin(GL_QUADS);
        glVertex3f(cv->x_min, cv->y_min, cv->z_min);
        glVertex3f(cv->x_min, cv->y_min, cv->z_max);
        glVertex3f(cv->x_max, cv->y_min, cv->z_max);
        glVertex3f(cv->x_max, cv->y_min, cv->z_min);
      glEnd();

      // YZ FACE, min X
      glBegin(GL_QUADS);
        glVertex3f(cv->x_min, cv->y_min, cv->z_min);
        glVertex3f(cv->x_min, cv->y_min, cv->z_max);
        glVertex3f(cv->x_min, cv->y_max, cv->z_max);
        glVertex3f(cv->x_min, cv->y_max, cv->z_min);
      glEnd();

      // XZ FACE, max Y
      glBegin(GL_QUADS);
        glVertex3f(cv->x_min, cv->y_max, cv->z_min);
        glVertex3f(cv->x_min, cv->y_max, cv->z_max);
        glVertex3f(cv->x_max, cv->y_max, cv->z_max);
        glVertex3f(cv->x_max, cv->y_max, cv->z_min);
      glEnd();

      // YZ FACE, max X
      glBegin(GL_QUADS);
        glVertex3f(cv->x_max, cv->y_min, cv->z_min);
        glVertex3f(cv->x_max, cv->y_min, cv->z_max);
        glVertex3f(cv->x_max, cv->y_max, cv->z_max);
        glVertex3f(cv->x_max, cv->y_max, cv->z_min);
      glEnd();

      // XY FACE, min Z
      glBegin(GL_QUADS);
        glVertex3f(cv->x_min, cv->y_min, cv->z_min);
        glVertex3f(cv->x_min, cv->y_max, cv->z_min);
        glVertex3f(cv->x_max, cv->y_max, cv->z_min);
        glVertex3f(cv->x_max, cv->y_min, cv->z_min);
      glEnd();

      // XY FACE, max Z
      glBegin(GL_QUADS);
        glVertex3f(cv->x_min, cv->y_min, cv->z_max);
        glVertex3f(cv->x_min, cv->y_max, cv->z_max);
        glVertex3f(cv->x_max, cv->y_max, cv->z_max);
        glVertex3f(cv->x_max, cv->y_min, cv->z_max);
      glEnd();

      retval = btrue;
    }

  }
  ATTRIB_POP( "CVolume_draw" );

  return retval;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_egoboo_video_parameters
{
  int    doublebuffer;     // btrue
  bool_t opengl;           // btrue
  bool_t fullscreen;       // bfalse

  int multibuffers;      // 1
  int multisamples;      // 16
  int glacceleration;    // 1

  vect3_si32 colordepth;  // 8,8,8

  int width;       // DEFAULT_SCREEN_W == 640
  int height;      // DEFAULT_SCREEN_H == 480
  int depth;       // 32
};

typedef struct s_egoboo_video_parameters video_parameters_t;

//--------------------------------------------------------------------------------------------
bool_t video_parameters_default(video_parameters_t * v)
{
  if(NULL == v) return bfalse;

  v->doublebuffer = btrue;
  v->opengl       = btrue;
  v->fullscreen   = bfalse;

  v->multibuffers   = 1;
  v->multisamples   = 16;
  v->glacceleration = 1;

  v->colordepth.r = 8;
  v->colordepth.g = 8;
  v->colordepth.b = 8;

  v->width  = DEFAULT_SCREEN_W;
  v->height = DEFAULT_SCREEN_H;
  v->depth  =  32;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t video_parameters_download(video_parameters_t * p, Graphics_t * g )
{
  if(NULL == g)
  {
    return video_parameters_default(p);
  };

  if(g->scrd > 24)
  {
    g->colordepth = g->scrd / 4;
  }
  else
  {
    g->colordepth = g->scrd / 3;
  }

  if(NULL == p) return bfalse;

  p->fullscreen     = g->fullscreen;
  p->glacceleration = g->gfxacceleration;

  if(g->antialiasing)
  {
    p->multisamples = 1;
    p->multibuffers = 16;
  }
  else
  {
    p->multisamples = 0;
    p->multibuffers = 0;
  }

  p->colordepth.r = g->colordepth;
  p->colordepth.g = g->colordepth;
  p->colordepth.b = g->colordepth;
  p->depth        = g->scrd;
  p->width        = g->scrx;
  p->height       = g->scry;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t video_parameters_upload( video_parameters_t * p, Graphics_t * g )
{
  if(NULL == p || NULL == g) return bfalse;

  g->fullscreen     = p->fullscreen;
  g->gfxacceleration = p->glacceleration;

  g->colordepth = p->colordepth.r;
  g->scrd       = p->depth;
  g->scrx       = p->width;
  g->scry       = p->height;

  return btrue;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SDL_Surface * RequestVideoMode( video_parameters_t * v )
{

  Uint32 flags;
  SDL_Surface * ret = NULL;

  if(NULL == v) return ret;

  if(v->opengl)
  {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, v->multibuffers  );
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, v->multisamples  );
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, v->glacceleration);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,       v->doublebuffer  );

#ifndef __unix__
    // Under Unix we cannot specify these, we just get whatever format
    // the framebuffer has, specifying depths > the framebuffer one
    // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"

    SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   v->colordepth.r );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, v->colordepth.g );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  v->colordepth.b );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, v->depth );
#endif
  }

  flags = 0;
  if(0 != v->doublebuffer) flags |= SDL_DOUBLEBUF;
  if( v->opengl          ) flags |= SDL_OPENGL;
  if( v->fullscreen      ) flags |= SDL_FULLSCREEN;

  ret = SDL_SetVideoMode( v->width, v->height, v->depth, flags );

  if(NULL == ret)
  {
    log_warning("SDL unable to set video mode with current parameters - \n\t\"%s\"\n", SDL_GetError() );

    if(v->opengl)
    {
      log_info( "\tUsing OpenGL\n" );
      log_info( "\tSDL_GL_MULTISAMPLEBUFFERS == %d\n", v->multibuffers);
      log_info( "\tSDL_GL_MULTISAMPLESAMPLES == %d\n", v->multisamples);
      log_info( "\tSDL_GL_ACCELERATED_VISUAL == %d\n", v->glacceleration);
      log_info( "\tSDL_GL_DOUBLEBUFFER       == %d\n", v->doublebuffer );

#ifndef __unix__
      // Under Unix we cannot specify these, we just get whatever format
      // the framebuffer has, specifying depths > the framebuffer one
      // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"

      log_info( "\tSDL_GL_RED_SIZE   == %d\n", v->colordepth.r );
      log_info( "\tSDL_GL_GREEN_SIZE == %d\n", v->colordepth.g );
      log_info( "\tSDL_GL_BLUE_SIZE  == %d\n", v->colordepth.b );
      log_info( "\tSDL_GL_DEPTH_SIZE == %d\n", v->depth );
#endif
    }

    log_info( "\t%s\n", ((0 != v->doublebuffer) ? "DOUBLE BUFFER" : "SINGLE BUFFER") );
    log_info( "\t%s\n", (v->fullscreen   ? "FULLSCREEN"    : "WINDOWED"     ) );
    log_info( "\twidth == %d, height == %d, depth == %d\n", v->width, v->height, v->depth );

  }
  else
  {
    // report current graphics values

    log_info("SDL set video mode to the current parameters\n", SDL_GetError() );

    v->opengl = HAS_SOME_BITS(ret->flags, SDL_OPENGL);

    if( v->opengl )
    {
      log_info( "\tUsing OpenGL\n" );

      if( 0 == SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &(v->multibuffers)) )
      {
        log_info("\tGL_MULTISAMPLEBUFFERS == %d\n", v->multibuffers);
      };

      if( 0 == SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &(v->multisamples)) )
      {
        log_info("\tSDL_GL_MULTISAMPLESAMPLES == %d\n", v->multisamples);
      };

      if( 0 == SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &(v->glacceleration)) )
      {
        log_info("\tSDL_GL_ACCELERATED_VISUAL == %d\n", v->glacceleration);
      };

      if( 0 == SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &(v->doublebuffer)) )
      {
        log_info("\tSDL_GL_DOUBLEBUFFER == %d\n", v->doublebuffer);
      };

      SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &(v->colordepth.r) );
      {
        log_info("\tSDL_GL_RED_SIZE == %d\n", v->colordepth.r);
      };

      SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &(v->colordepth.g) );
      {
        log_info("\tSDL_GL_GREEN_SIZE == %d\n", v->colordepth.g);
      };

      SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE,  &(v->colordepth.b) );
      {
        log_info("\tSDL_GL_BLUE_SIZE == %d\n", v->colordepth.b);
      };

      SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &(v->depth) );
      {
        log_info("\tSDL_GL_DEPTH_SIZE == %d\n", v->depth);
      };

    }

    log_info( "\t%s\n", (HAS_SOME_BITS(ret->flags, SDL_DOUBLEBUF)  ? "SDL DOUBLE BUFFER" : "SDL SINGLE BUFFER") );
    log_info( "\t%s\n", (HAS_SOME_BITS(ret->flags, SDL_FULLSCREEN)  ? "FULLSCREEN"    : "WINDOWED"     ) );

    v->width  = ret->w;
    v->height = ret->h;
    v->depth  = ret->format->BitsPerPixel;
    log_info( "\twidth == %d, height == %d, depth == %d\n", v->width, v->height, v->depth );
  }

  return ret;

};



//--------------------------------------------------------------------------------------------
Graphics_t * sdl_set_mode(Graphics_t * g_old, Graphics_t * g_new, bool_t update_ogl)
{
  // BB > let SDL try to set a new video mode.

  video_parameters_t param_old, param_new;
  Graphics_t * retval = NULL;
  bool_t success = btrue;                      // hope for the best

  if(!gfx_initialized) return g_old;

  // save the old parameters
  video_parameters_default( &param_old );
  video_parameters_download( &param_old, g_old );

  // load the param structure function with the video info
  video_parameters_default( &param_new );
  video_parameters_download( &param_new, g_new );

  // assume any problem with setting the graphics mode is with the multisampling
  while( param_new.multisamples > 0 )
  {
    g_new->surface = RequestVideoMode(&param_new);
    if(NULL != g_new->surface) break;
    param_new.multisamples >>= 1;
  };

  if( NULL == g_new->surface )
  {
    // we can't have any multi...

    param_new.multibuffers = 0;
    param_new.multisamples = 0;

    g_new->surface = RequestVideoMode(&param_new);
  }

  if( NULL != g_new->surface )
  {
    // apply the new OpenGL settings?

    video_parameters_upload( &param_new, g_new );

    if(update_ogl)
    {
      gl_set_mode( g_new );
    };

    retval = g_new;
  }
  else
  {
    log_warning("I can't get SDL to set the new video mode.\n");
    if( NULL == g_old )
    {
      log_warning("Trying to start a default mode.\n");
    }
    else
    {
      log_warning("Trying to reset the old mode.\n");
    };

    g_old->surface = RequestVideoMode(&param_old);
    if(NULL == g_old->surface)
    {
      log_error("Could not restore the old video mode. Terminating.\n");
      exit(-1);
    }

    retval = g_old;

    video_parameters_upload( &param_old, g_old );

    // need to re-establish all of the old OpenGL settings?
    if(update_ogl)
    {
      gl_set_mode( g_old );
    }
  }

  return retval;
};


//--------------------------------------------------------------------------------------------
bool_t gl_set_mode(Graphics_t * g)
{
  // BB > this function applies OpenGL settings. Must have a valid SDL_Surface to do any good.

  if(NULL == g || NULL == g->surface) return bfalse;

  if( g->antialiasing )
  {
    glEnable(GL_MULTISAMPLE_ARB);
    //glEnable(GL_MULTISAMPLE);
  };

  //Check if the computer graphic driver supports anisotropic filtering
  if ( g->texturefilter >= TX_ANISOTROPIC )
  {
    if ( 0 == strstr(( char* ) glGetString( GL_EXTENSIONS ), "GL_EXT_texture_filter_anisotropic" ) )
    {
      log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
      g->texturefilter = TX_TRILINEAR_2; //Set filtering to trillienar instead
    }
  }

  //Enable perspective correction?
  glHint( GL_PERSPECTIVE_CORRECTION_HINT, g->perspective );

  //Enable dithering?
  if ( g->dither ) glEnable( GL_DITHER );
  else glDisable( GL_DITHER );

  //Enable gourad g->shading? (Important!)
  glShadeModel( g->shading );

  //Enable g->antialiasing?
  if ( g->antialiasing )
  {
    glEnable( GL_LINE_SMOOTH );
    glHint( GL_LINE_SMOOTH_HINT,    GL_NICEST );

    glEnable( GL_POINT_SMOOTH );
    glHint( GL_POINT_SMOOTH_HINT,   GL_NICEST );

    glDisable( GL_POLYGON_SMOOTH );
    glHint( GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

    // PLEASE do not turn this on unless you use
    //  glEnable (GL_BLEND);
    //  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // before every single draw command
    //
    //glEnable(GL_POLYGON_SMOOTH);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  }
  else
  {
    glDisable( GL_POINT_SMOOTH );
    glDisable( GL_LINE_SMOOTH );
    glDisable( GL_POLYGON_SMOOTH );
  }

  // Wait for vertical synchronization?
  if( g->vsync )
  {
    // Fedora 7 doesn't suuport SDL_GL_SWAP_CONTROL, but we use this  nvidia extension instead.
#if defined(__unix__)
      SDL_putenv("__GL_SYNC_TO_VBLANK=1");
#else
    /* Turn on vsync, this works on Windows. */
      if(SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1) < 0)
      {
        log_warning( "SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1) failed - \n\t\"%s\"\n", SDL_GetError());
      }
#endif
  }

  //fill mode
  glPolygonMode( GL_FRONT, GL_FILL );
  glPolygonMode( GL_BACK,  GL_FILL );

  /* Disable OpenGL lighting */
  glDisable( GL_LIGHTING );

  /* Backface culling */
  glEnable( GL_CULL_FACE );  // This seems implied - DDOI
  glCullFace( GL_BACK );

  glViewport(0, 0, g->surface->w, g->surface->h);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t lighting_info_reset(LIGHTING_INFO * li)
{
  li->on        = bfalse;

  li->spek      = 0.0f;
  li->spekdir.x = li->spekdir.y = li->spekdir.z = 0.0f;
  li->spekcol.r = li->spekcol.g = li->spekcol.b = 1.0f;

  li->ambi      = 0.0f;
  li->ambicol.r = li->ambicol.g = li->ambicol.b = 1.0f;

  li->spekdir_stt = li->spekdir;

  return btrue;
};



//--------------------------------------------------------------------------------------------
//void render_bad_shadow(CHR_REF character)
//{
//  // ZZ> This function draws a sprite shadow
//
//  Game_t * gs = Graphics_requireGame(&gfxState);
//
//  GLVertex v[4];
//  float size, x, y;
//  Uint8 ambi;
//  //DWORD light;
//  float level; //, z;
//  int height;
//  Sint8 hide;
//  Uint8 trans;
//  int i;
//
//
//  hide = ChrList_getPCap(gs, character)->hidestate;
//  if (hide == NOHIDE || hide != gs->ChrList[character].aistate.state)
//  {
//    // Original points
//    level = gs->ChrList[character].level;
//    level += SHADOWRAISE;
//    height = gs->ChrList[character].matrix.CNV(3, 2) - level;
//    if (height > 255)  return;
//    if (height < 0) height = 0;
//    size = gs->ChrList[character].bmpdata.calc_shadowsize - FP8_MUL(height, gs->ChrList[character].bmpdata.calc_shadowsize);
//    if (size < 1) return;
//    ambi = gs->ChrList[character].lightspek_fp8 >> 4;  // LUL >>3;
//    trans = ((255 - height) >> 1) + 64;
//
//    x = gs->ChrList[character].matrix.CNV(3, 0);
//    y = gs->ChrList[character].matrix.CNV(3, 1);
//    v[0].pos.x = (float) x + size;
//    v[0].pos.y = (float) y - size;
//    v[0].pos.z = (float) level;
//
//    v[1].pos.x = (float) x + size;
//    v[1].pos.y = (float) y + size;
//    v[1].pos.z = (float) level;
//
//    v[2].pos.x = (float) x - size;
//    v[2].pos.y = (float) y + size;
//    v[2].pos.z = (float) level;
//
//    v[3].pos.x = (float) x - size;
//    v[3].pos.y = (float) y - size;
//    v[3].pos.z = (float) level;
//
//
//    v[0].s = CALCULATE_PRT_U0(236);
//    v[0].t = CALCULATE_PRT_V0(236);
//
//    v[1].s = CALCULATE_PRT_U1(253);
//    v[1].t = CALCULATE_PRT_V0(236);
//
//    v[2].s = CALCULATE_PRT_U1(253);
//    v[2].t = CALCULATE_PRT_V1(253);
//
//    v[3].s = CALCULATE_PRT_U0(236);
//    v[3].t = CALCULATE_PRT_V1(253);
//
//    ATTRIB_PUSH("render_bad_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
//    {
//
//      glDisable(GL_CULL_FACE);
//
//      //glEnable(GL_ALPHA_TEST);
//      //glAlphaFunc(GL_GREATER, 0);
//
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
//
//      glDepthMask(GL_FALSE);
//      glDepthFunc(GL_LEQUAL);
//
//      // Choose texture.
//      if( MAXTEXTURE == particletexture)
//      {
//        GLtexture_Bind( NULL, &gfxState );
//      }
//      else
//      {
//        GLtexture_Bind( gs->TxTexture + particletexture, &gfxState );
//      };
//
//      glColor4f(FP8_TO_FLOAT(ambi), FP8_TO_FLOAT(ambi), FP8_TO_FLOAT(ambi), FP8_TO_FLOAT(trans));
//      glBegin(GL_TRIANGLE_FAN);
//      for (i = 0; i < 4; i++)
//      {
//        glTexCoord2f(v[i].s, v[i].t);
//        glVertex3fv(v[i].pos.v);
//      }
//      glEnd();
//    }
//    ATTRIB_POP("render_bad_shadow");
//  }
//}
//

//--------------------------------------------------------------------------------------------
//void render_bad_shadows()
//{
//  int cnt;
//  CHR_REF chr_tnc;
//
//  Game_t * gs = Graphics_requireGame(&gfxState);
//
//  // Bad shadows
//  ATTRIB_PUSH("render_bad_shadows", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
//  {
//    glDepthMask(GL_FALSE);
//
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      chr_tnc = dolist[cnt];
//      //if(INVALID_CHR == gs->ChrList[chr_tnc].attachedto )
//      //{
//      if (gs->ChrList[chr_tnc].bmpdata.calc_shadowsize != 0 || chrlst[chr_tnc].prop.forceshadow && HAS_NO_BITS(Mesh[gs->ChrList[chr_tnc].onwhichfan].fx, MPDFX_SHINY))
//        render_bad_shadow(chr_tnc);
//      //}
//    }
//  }
//  ATTRIB_POP("render_bad_shadows");
//}

//--------------------------------------------------------------------------------------------
//void render_reflected_fans()
//{
//  int cnt, tnc, fan, texture;
//
//  // Render the shadow floors
//  ATTRIB_PUSH("render_reflected_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
//  {
//    // depth buffer stuff
//    glDepthMask(GL_TRUE);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    // shading stuff
//    glShadeModel(gfxState.shading);
//
//    // alpha stuff
//    glDisable(GL_BLEND);
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    // backface culling
//    glEnable(GL_CULL_FACE);
//    glFrontFace(GL_CW);
//    glCullFace(GL_BACK);
//
//    for (cnt = 0; cnt < 4; cnt++)
//    {
//      texture = cnt + TX_TILE_0;
//      pmesh->Info.last_texture = texture;
//      GLtexture_Bind( gs->TxTexture + texture, &gfxState);
//      for (tnc = 0; tnc < renderlist.num_reflc; tnc++)
//      {
//        fan = renderlist.reflc[tnc];
//        render_fan(fan, texture);
//      };
//    }
//  }
//  ATTRIB_POP("render_reflected_fans");
//};
//

//--------------------------------------------------------------------------------------------
//void render_sha_fans_ref()
//{
//  int cnt, tnc, fan, texture;
//
//  // Render the shadow floors
//  ATTRIB_PUSH("render_sha_fans_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
//  {
//    // depth buffer stuff
//    glDepthMask(GL_TRUE);
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    // gfxState.shading stuff
//    glShadeModel(gfxState.shading);
//
//    // alpha stuff
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_ONE, GL_ONE);
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    // backface culling
//    glEnable(GL_CULL_FACE);
//    glFrontFace(GL_CCW);
//    glCullFace(GL_BACK);
//
//    for (cnt = 0; cnt < 4; cnt++)
//    {
//      texture = cnt + TX_TILE_0;
//      pmesh->Info.last_texture = texture;
//      GLtexture_Bind( gs->TxTexture + texture, &gfxState);
//      for (tnc = 0; tnc < renderlist.num_reflc; tnc++)
//      {
//        fan = renderlist.reflc[tnc];
//        render_fan_ref(fan, texture, gs->PlaList_level);
//      };
//    }
//
//  }
//  ATTRIB_POP("render_sha_fans_ref");
//};
//


