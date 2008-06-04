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
#include "Network.h"
#include "Client.h"
#include "Server.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "input.inl"
#include "particle.inl"
#include "input.inl"
#include "mesh.inl"
#include "egoboo_math.inl"

#include <assert.h>
#include <stdarg.h>

#ifdef __unix__
#include <unistd.h>
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct CGame_t;

RENDERLIST           renderlist;
GraphicState         gfxState;
GLOBAL_LIGHTING_INFO GLight = {bfalse};
static bool_t        gfx_initialized = bfalse;

//--------------------------------------------------------------------------------------------
struct Status_t;

static int  draw_wrap_string( BMFont * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... );
static int  draw_status( BMFont *  pfnt , struct Status_t * pstat );
static void draw_text( BMFont *  pfnt  );

void render_particles();

//--------------------------------------------------------------------------------------------
bool_t gfx_find_anisotropy( GraphicState * g )
{
  // BB> get the maximum anisotropy supported by the video vard
  //     OpenGL and SDL must be loaded for this to work.

  if(NULL == g) return bfalse;

  g->maxAnisotropy  = 0;
  g->log2Anisotropy = 0;
  g->texturefilter  = TX_TRILINEAR_2;

  if(!SDL_WasInit(SDL_INIT_VIDEO) || NULL==g->surface) return bfalse;

  glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &(g->maxAnisotropy) );
  g->log2Anisotropy = ( g->maxAnisotropy == 0 ) ? 0 : floor( log( g->maxAnisotropy + 1e-6 ) / log( 2.0f ) );

  if ( g->maxAnisotropy == 0.0f && g->texturefilter >= TX_ANISOTROPIC )
  {
    g->texturefilter = TX_TRILINEAR_2;
  }

  return btrue;
}

bool_t gfx_initialize(GraphicState * g, ConfigData * cd)
{
  if(NULL == g || NULL == cd) return bfalse;

  if(gfx_initialized) return btrue;

  // set the graphics state
  GraphicState_new(g, cd);
  g->rnd_lst = &renderlist;
  gfx_find_anisotropy(g);
  gfx_initialized = btrue;

  GLight.spek      = 0.0f;
  GLight.spekdir.x = GLight.spekdir.y = GLight.spekdir.z = 0.0f;
  GLight.spekcol.r = GLight.spekcol.g = GLight.spekcol.b = 1.0f;
  GLight.ambi      = 0.0f;
  GLight.ambicol.r = GLight.ambicol.g = GLight.ambicol.b = 1.0f;

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



// This needs work
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

  GLTexture_Bind( pfnt, &gfxState );

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
void release_all_textures(CGame * gs)
{
  // ZZ> This function clears out all of the textures

  int cnt;

  for ( cnt = 0; cnt < MAXTEXTURE; cnt++ )
  {
    GLTexture_Release( gs->TxTexture + cnt );
  }
}

//--------------------------------------------------------------------------------------------
Uint32 load_one_icon( char * szModname, char * szObjectname, char * szFilename )
{
  // ZZ> This function is used to load an icon.  Most icons are loaded
  //     without this function though...

  Uint32 retval = MAXICONTX;
  CGame * gs = gfxState.gs;

  if ( INVALID_TEXTURE != GLTexture_Load( GL_TEXTURE_2D,  gs->TxIcon + gs->TxIcon_count,  inherit_fname(szModname, szObjectname, szFilename), INVALID_KEY ) )
  {
    retval = gs->TxIcon_count;
    gs->TxIcon_count++;
  }

  return retval;
}

//---------------------------------------------------------------------------------------------
void mnu_prime_titleimage(MenuProc * mproc)
{
  // ZZ> This function sets the title image pointers to NULL
  int cnt;

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
void prime_icons(CGame * gs)
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

  gs->nullicon = MAXICONTX;
  gs->keybicon = MAXICONTX;
  gs->mousicon = MAXICONTX;
  gs->joyaicon = MAXICONTX;
  gs->joybicon = MAXICONTX;
  gs->bookicon = MAXICONTX;
}

//---------------------------------------------------------------------------------------------
void release_all_icons(CGame * gs)
{
  // ZZ> This function clears out all of the icons

  int cnt;
  gs->TxIcon_count = 0;
  for ( cnt = 0; cnt < MAXICONTX; cnt++ )
  {
    GLTexture_Release( gs->TxIcon + cnt );
    gs->TxIcon[cnt].textureID = INVALID_TEXTURE;
    gs->skintoicon[cnt]       = MAXICONTX;
  }
}

//---------------------------------------------------------------------------------------------
void init_all_models(CGame * gs)
{
  // ZZ> This function initializes the models

  CapList_delete(gs);
  PipList_delete(gs);
  MadList_delete(gs);
}

//---------------------------------------------------------------------------------------------
void release_all_models(CGame * gs)
{
  // ZZ> This function clears out all of the models

  CapList_delete(gs);
  MadList_delete(gs);
}

//--------------------------------------------------------------------------------------------
void release_map( CGame * gs )
{
  // ZZ> This function releases all the map images

  GLTexture_Release( &gs->TxMap );
}

//--------------------------------------------------------------------------------------------
static void write_debug_message( int time, const char *format, va_list args )
{
  // ZZ> This function sticks a message in the display queue and sets its timer

  STRING buffer;
  int slot = get_free_message();

  // print the formatted messafe into the buffer
  vsnprintf( buffer, sizeof( buffer ) - 1, format, args );

  // Copy the message
  strncpy( GMsg.list[slot].textdisplay, buffer, sizeof( GMsg.list[slot].textdisplay ) );
  GMsg.list[slot].time = time * DELAY_MESSAGE;
}

//--------------------------------------------------------------------------------------------
void debug_message( int time, const char *format, ... )
{
  va_list args;

  va_start( args, format );
  write_debug_message( time, format, args );
  va_end( args );
};


//--------------------------------------------------------------------------------------------
void reset_end_text( CGame * gs )
{
  // ZZ> This function resets the end-module text

  if ( gs->PlaList_count > 1 )
  {
    snprintf( endtext, sizeof( endtext ), "Sadly, they were never heard from again..." );
    endtextwrite = 42;  // Where to append further text
  }
  else
  {
    if ( gs->PlaList_count == 0 )
    {
      // No players???
      snprintf( endtext, sizeof( endtext ), "The game has ended..." );
      endtextwrite = 21;
    }
    else
    {
      // One player
      snprintf( endtext, sizeof( endtext ), "Sadly, no trace was ever found..." );
      endtextwrite = 33;  // Where to append further text
    }
  }
}

//--------------------------------------------------------------------------------------------
void append_end_text( CGame * gs, int message, CHR_REF chr_ref )
{
  // ZZ> This function appends a message to the end-module text

  int read, cnt;
  char *eread;
  STRING szTmp;
  char cTmp, lTmp;

  Uint16 target = chr_get_aitarget( gs->ChrList, MAXCHR, gs->ChrList + chr_ref );
  Uint16 owner = chr_get_aiowner( gs->ChrList, MAXCHR, gs->ChrList + chr_ref );

  Chr * pchr    = MAXCHR == chr_ref ? NULL : gs->ChrList + chr_ref;
  AI_STATE * pstate = &(pchr->aistate);
  Cap * pchr_cap = gs->CapList + pchr->model;

  Chr * ptarget  = MAXCHR == target  ? NULL : gs->ChrList + target;
  Cap * ptrg_cap = NULL   == ptarget ? NULL : gs->CapList + ptarget->model;

  Chr * powner   = MAXCHR == owner  ? NULL : gs->ChrList + owner;
  Cap * pown_cap = NULL   == powner ? NULL : gs->CapList + powner->model;

  if ( message < GMsg.total )
  {
    // Copy the message
    read = GMsg.index[message];
    cnt = 0;
    cTmp = GMsg.text[read];  read++;
    while ( cTmp != 0 )
    {
      if ( cTmp == '%' )
      {
        // Escape sequence
        eread = szTmp;
        szTmp[0] = 0;
        cTmp = GMsg.text[read];  read++;
        if ( cTmp == 'n' ) // Name
        {
          if ( pchr->nameknown )
            strncpy( szTmp, pchr->name, sizeof( STRING ) );
          else
          {
            lTmp = pchr_cap->classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", pchr_cap->classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", pchr_cap->classname );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 'c' ) // Class name
        {
          eread = pchr_cap->classname;
        }
        if ( cTmp == 't' ) // Target name
        {
          if ( ptarget->nameknown )
            strncpy( szTmp, ptarget->name, sizeof( STRING ) );
          else
          {
            lTmp = ptrg_cap->classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", ptrg_cap->classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", ptrg_cap->classname );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 'o' ) // Owner name
        {
          if ( powner->nameknown )
            strncpy( szTmp, powner->name, sizeof( STRING ) );
          else
          {
            lTmp = pown_cap->classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", pown_cap->classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", pown_cap->classname );
          }
          if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        if ( cTmp == 's' ) // Target class name
        {
          eread = ptrg_cap->classname;
        }
        if ( cTmp >= '0' && cTmp <= '0' + ( MAXSKIN - 1 ) )  // Target's skin name
        {
          eread = ptrg_cap->skin[cTmp-'0'].name;
        }
        if ( cTmp == 'd' ) // tmpdistance value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", pstate->tmpdistance );
        }
        if ( cTmp == 'x' ) // tmpx value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", pstate->tmpx );
        }
        if ( cTmp == 'y' ) // tmpy value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", pstate->tmpy );
        }
        if ( cTmp == 'D' ) // tmpdistance value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", pstate->tmpdistance );
        }
        if ( cTmp == 'X' ) // tmpx value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", pstate->tmpx );
        }
        if ( cTmp == 'Y' ) // tmpy value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", pstate->tmpy );
        }
        if ( cTmp == 'a' ) // Character's ammo
        {
          if ( pchr->ammoknown )
            snprintf( szTmp, sizeof( szTmp ), "%d", pchr->ammo );
          else
            snprintf( szTmp, sizeof( szTmp ), "?" );
        }
        if ( cTmp == 'k' ) // Kurse state
        {
          if ( pchr->iskursed )
            snprintf( szTmp, sizeof( szTmp ), "kursed" );
          else
            snprintf( szTmp, sizeof( szTmp ), "unkursed" );
        }
        if ( cTmp == 'p' ) // Character's possessive
        {
          if ( pchr->gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else
          {
            if ( pchr->gender == GEN_MALE )
            {
              snprintf( szTmp, sizeof( szTmp ), "his" );
            }
            else
            {
              snprintf( szTmp, sizeof( szTmp ), "its" );
            }
          }
        }
        if ( cTmp == 'm' ) // Character's gender
        {
          if ( pchr->gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "female " );
          }
          else
          {
            if ( pchr->gender == GEN_MALE )
            {
              snprintf( szTmp, sizeof( szTmp ), "male " );
            }
            else
            {
              snprintf( szTmp, sizeof( szTmp ), " " );
            }
          }
        }
        if ( cTmp == 'g' ) // Target's possessive
        {
          if ( ptarget->gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else
          {
            if ( ptarget->gender == GEN_MALE )
            {
              snprintf( szTmp, sizeof( szTmp ), "his" );
            }
            else
            {
              snprintf( szTmp, sizeof( szTmp ), "its" );
            }
          }
        }
        // Copy the generated text
        cTmp = *eread;  eread++;
        while ( cTmp != 0 && endtextwrite < MAXENDTEXT - 1 )
        {
          endtext[endtextwrite] = cTmp;
          cTmp = *eread;  eread++;
          endtextwrite++;
        }
      }
      else
      {
        // Copy the letter
        if ( endtextwrite < MAXENDTEXT - 1 )
        {
          endtext[endtextwrite] = cTmp;
          endtextwrite++;
        }
      }
      cTmp = GMsg.text[read];  read++;
      cnt++;
    }
  }
  endtext[endtextwrite] = 0;
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
void animate_tiles( float dUpdate )
{
  // This function changes the animated tile frame

  GTile_Anim.framefloat += dUpdate / ( float ) GTile_Anim.updateand;
  while ( GTile_Anim.framefloat >= 1.0f )
  {
    GTile_Anim.framefloat -= 1.0f;
    GTile_Anim.frameadd = ( GTile_Anim.frameadd + 1 ) & GTile_Anim.frameand;
  };
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( CGame * gs, char *modname )
{
  // ZZ> This function loads the standard textures for a module
  // BB> In each case, try to load one stored with the module first.

  // Particle sprites
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, "particle.bmp" );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_PARTICLE, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.particle_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_PARTICLE, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "!!!!Particle bitmap could not be found!!!! Missing File = \"%s\"\n", CStringTmp1 );
    }
  };

  // Module background tiles
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.tile0_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_0, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.tile0_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_0, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 0 could not be found. Missing File = \"%s\"\n", CData.tile0_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.tile1_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,   gs->TxTexture + TX_TILE_1, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.tile1_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_1, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 1 could not be found. Missing File = \"%s\"\n", CData.tile1_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.tile2_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_2, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.tile2_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_2, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 2 could not be found. Missing File = \"%s\"\n", CData.tile2_bitmap );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.tile3_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_3, CStringTmp1, TRANSCOLOR ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.tile3_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_TILE_3, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Tile 3 could not be found. Missing File = \"%s\"\n", CData.tile3_bitmap );
    }
  };


  // Water textures
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.watertop_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_TOP, CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.watertop_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_TOP, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 1 could not be found. Missing File = \"%s\"\n", CData.watertop_bitmap );
    }
  };

  // This is also used as far background
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.waterlow_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_LOW, CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.waterlow_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + TX_WATER_LOW, CStringTmp1, TRANSCOLOR ) )
    {
      log_warning( "Water Layer 0 could not be found. Missing File = \"%s\"\n", CData.waterlow_bitmap );
    }
  };


  // BB > this is handled differently now and is not needed
  // Texture 7 is the phong map
  //snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.phong_bitmap);
  //if(INVALID_TEXTURE==GLTexture_Load(GL_TEXTURE_2D,  gs->TxTexture + TX_PHONG, CStringTmp1, INVALID_KEY))
  //{
  //  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.phong_bitmap);
  //  GLTexture_Load(GL_TEXTURE_2D,  gs->TxTexture + TX_PHONG, CStringTmp1, TRANSCOLOR );
  //  {
  //    log_warning("Phong Bitmap Layer 1 could not be found. Missing File = \"%s\"", CData.phong_bitmap);
  //  }
  //};


}

//--------------------------------------------------------------------------------------------
bool_t load_bars( char* szBitmap )
{
  // ZZ> This function loads the status bar bitmap
  CGui * gui = Get_CGui();
  int cnt;

  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D, &gui->TxBars, szBitmap, 0 ) )
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
void load_map( CGame * gs, char* szModule )
{
  // ZZ> This function loads the map bitmap and the blip bitmap

  // Turn it all off
  mapon = bfalse;
  youarehereon = bfalse;
  numblip = 0;

  // Load the images
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModule, CData.gamedat_dir, CData.plan_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D, &gs->TxMap, CStringTmp1, INVALID_KEY ) )
    log_warning( "Cannot load map: %s\n", CStringTmp1 );

  // Set up the rectangles
  mapscale = MIN(( float ) gfxState.scrx / 640.0f, ( float ) gfxState.scry / 480.0f );
  maprect.left   = 0;
  maprect.right  = MAPSIZE * mapscale;
  maprect.top    = gfxState.scry - MAPSIZE * mapscale;
  maprect.bottom = gfxState.scry;

}

//--------------------------------------------------------------------------------------------
bool_t load_font( char* szBitmap, char* szSpacing )
{
  // ZZ> This function loads the font bitmap and sets up the coordinates
  //     of each font on that bitmap...  Bitmap must have 16x6 fonts

  int cnt, y, xsize, ysize, xdiv, ydiv;
  int xstt, ystt;
  int xspacing, yspacing;
  Uint8 cTmp;
  FILE *fileread;


  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D, &(bmfont.tex), szBitmap, 0 ) ) return bfalse;


  // Clear out the conversion table
  for ( cnt = 0; cnt < 256; cnt++ )
    bmfont.ascii_table[cnt] = 0;


  // Get the size of the bitmap
  xsize = GLTexture_GetImageWidth( &(bmfont.tex) );
  ysize = GLTexture_GetImageHeight( &(bmfont.tex) );
  if ( xsize == 0 || ysize == 0 )
    log_warning( "Bad font size! (basicdat" SLASH_STRING "%s) - X size: %i , Y size: %i\n", szBitmap, xsize, ysize );


  // Figure out the general size of each font
  ydiv = ysize / NUMFONTY;
  xdiv = xsize / NUMFONTX;


  // Figure out where each font is and its spacing
  fileread = fs_fileOpen( PRI_NONE, NULL, szSpacing, "r" );
  if ( fileread == NULL ) return bfalse;

  globalname = szSpacing;

  // Uniform font height is at the top
  yspacing = fget_next_int( fileread );
  bmfont.offset = gfxState.scry - yspacing;

  // Mark all as unused
  for ( cnt = 0; cnt < 255; cnt++ )
    bmfont.ascii_table[cnt] = 255;


  cnt = 0;
  y = 0;
  xstt = 0;
  ystt = 0;
  while ( cnt < 255 && fgoto_colon_yesno( fileread ) )
  {
    cTmp = fgetc( fileread );
    xspacing = fget_int( fileread );
    if ( bmfont.ascii_table[cTmp] == 255 )
    {
      bmfont.ascii_table[cTmp] = cnt;
    }

    if ( xstt + xspacing + 1 >= xsize )
    {
      xstt = 0;
      ystt += yspacing;
    }

    bmfont.rect[cnt].x = xstt;
    bmfont.rect[cnt].w = xspacing;
    bmfont.rect[cnt].y = ystt;
    bmfont.rect[cnt].h = yspacing - 1;
    bmfont.spacing_x[cnt] = xspacing;

    xstt += xspacing + 1;

    cnt++;
  }
  fs_fileClose( fileread );


  // Space between lines
  bmfont.spacing_y = ( yspacing >> 1 ) + FONTADD;

  return btrue;
}

//--------------------------------------------------------------------------------------------
void make_water(CGame * gs)
{
  // ZZ> This function sets up water movements

  int layer, frame, point, mode, cnt;
  float tmp_sin, tmp_cos, tmp;
  Uint8 spek;

  layer = 0;
  while ( layer < gs->water.layer_count )
  {
    gs->water.layer[layer].u = 0;
    gs->water.layer[layer].v = 0;
    frame = 0;
    while ( frame < MAXWATERFRAME )
    {
      // Do first mode
      mode = 0;
      for ( point = 0; point < WATERPOINTS; point++ )
      {
        tmp_sin = sin(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        tmp_cos = cos(( frame * TWO_PI / MAXWATERFRAME ) + ( PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
        gs->water.layer[layer].zadd[frame][mode][point]  = tmp_sin * gs->water.layer[layer].amp;
      }

      // Now mirror and copy data to other three modes
      mode++;
      gs->water.layer[layer].zadd[frame][mode][0] = gs->water.layer[layer].zadd[frame][0][1];
      //gs->water.layer[layer].color[frame][mode][0] = gs->water.layer[layer].color[frame][0][1];
      gs->water.layer[layer].zadd[frame][mode][1] = gs->water.layer[layer].zadd[frame][0][0];
      //gs->water.layer[layer].color[frame][mode][1] = gs->water.layer[layer].color[frame][0][0];
      gs->water.layer[layer].zadd[frame][mode][2] = gs->water.layer[layer].zadd[frame][0][3];
      //gs->water.layer[layer].color[frame][mode][2] = gs->water.layer[layer].color[frame][0][3];
      gs->water.layer[layer].zadd[frame][mode][3] = gs->water.layer[layer].zadd[frame][0][2];
      //gs->water.layer[layer].color[frame][mode][3] = gs->water.layer[layer].color[frame][0][2];
      mode++;

      gs->water.layer[layer].zadd[frame][mode][0] = gs->water.layer[layer].zadd[frame][0][3];
      //gs->water.layer[layer].color[frame][mode][0] = gs->water.layer[layer].color[frame][0][3];
      gs->water.layer[layer].zadd[frame][mode][1] = gs->water.layer[layer].zadd[frame][0][2];
      //gs->water.layer[layer].color[frame][mode][1] = gs->water.layer[layer].color[frame][0][2];
      gs->water.layer[layer].zadd[frame][mode][2] = gs->water.layer[layer].zadd[frame][0][1];
      //gs->water.layer[layer].color[frame][mode][2] = gs->water.layer[layer].color[frame][0][1];
      gs->water.layer[layer].zadd[frame][mode][3] = gs->water.layer[layer].zadd[frame][0][0];
      //gs->water.layer[layer].color[frame][mode][3] = gs->water.layer[layer].color[frame][0][0];
      mode++;

      gs->water.layer[layer].zadd[frame][mode][0] = gs->water.layer[layer].zadd[frame][0][2];
      //gs->water.layer[layer].color[frame][mode][0] = gs->water.layer[layer].color[frame][0][2];
      gs->water.layer[layer].zadd[frame][mode][1] = gs->water.layer[layer].zadd[frame][0][3];
      //gs->water.layer[layer].color[frame][mode][1] = gs->water.layer[layer].color[frame][0][3];
      gs->water.layer[layer].zadd[frame][mode][2] = gs->water.layer[layer].zadd[frame][0][0];
      //gs->water.layer[layer].color[frame][mode][2] = gs->water.layer[layer].color[frame][0][0];
      gs->water.layer[layer].zadd[frame][mode][3] = gs->water.layer[layer].zadd[frame][0][1];
      //gs->water.layer[layer].color[frame][mode][3] = gs->water.layer[layer].color[frame][0][1];
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

    gs->water.spek[cnt] = spek;

    // [claforte] Probably need to replace this with a
    //            glColor4f( FP8_TO_FLOAT(spek), FP8_TO_FLOAT(spek), FP8_TO_FLOAT(spek), 1.0f) call:
  }
}

//--------------------------------------------------------------------------------------------
void read_wawalite( CGame * gs, char *modname )
{
  // ZZ> This function sets up water and lighting for the module

  FILE* fileread;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.wawalite_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( NULL != fileread )
  {
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
    gs->water.layer_count = fget_next_int( fileread );
    gs->water.spekstart = fget_next_int( fileread );
    gs->water.speklevel_fp8 = fget_next_int( fileread );
    gs->water.douselevel = fget_next_int( fileread );
    gs->water.surfacelevel = fget_next_int( fileread );
    gs->water.light = fget_next_bool( fileread );
    gs->water.iswater = fget_next_bool( fileread );
    gfxState.render_overlay    = fget_next_bool( fileread ) && CData.overlayvalid;
    gfxState.render_background = fget_next_bool( fileread ) && CData.backgroundvalid;
    gs->water.layer[0].distx = fget_next_float( fileread );
    gs->water.layer[0].disty = fget_next_float( fileread );
    gs->water.layer[1].distx = fget_next_float( fileread );
    gs->water.layer[1].disty = fget_next_float( fileread );
    foregroundrepeat = fget_next_int( fileread );
    backgroundrepeat = fget_next_int( fileread );


    gs->water.layer[0].z = fget_next_int( fileread );
    gs->water.layer[0].alpha_fp8 = fget_next_int( fileread );
    gs->water.layer[0].frameadd = fget_next_int( fileread );
    gs->water.layer[0].lightlevel_fp8 = fget_next_int( fileread );
    gs->water.layer[0].lightadd_fp8 = fget_next_int( fileread );
    gs->water.layer[0].amp = fget_next_float( fileread );
    gs->water.layer[0].uadd = fget_next_float( fileread );
    gs->water.layer[0].vadd = fget_next_float( fileread );

    gs->water.layer[1].z = fget_next_int( fileread );
    gs->water.layer[1].alpha_fp8 = fget_next_int( fileread );
    gs->water.layer[1].frameadd = fget_next_int( fileread );
    gs->water.layer[1].lightlevel_fp8 = fget_next_int( fileread );
    gs->water.layer[1].lightadd_fp8 = fget_next_int( fileread );
    gs->water.layer[1].amp = fget_next_float( fileread );
    gs->water.layer[1].uadd = fget_next_float( fileread );
    gs->water.layer[1].vadd = fget_next_float( fileread );

    gs->water.layer[0].u = 0;
    gs->water.layer[0].v = 0;
    gs->water.layer[1].u = 0;
    gs->water.layer[1].v = 0;
    gs->water.layer[0].frame = rand() & WATERFRAMEAND;
    gs->water.layer[1].frame = rand() & WATERFRAMEAND;

    // Read light data second
    GLight.on        = btrue;
    GLight.spekdir.x = fget_next_float( fileread );
    GLight.spekdir.y = fget_next_float( fileread );
    GLight.spekdir.z = fget_next_float( fileread );
    GLight.ambi      = fget_next_float( fileread );

    GLight.spek = DotProduct( GLight.spekdir, GLight.spekdir );
    if ( 0 != GLight.spek )
    {
      GLight.spek = sqrt( GLight.spek );
      GLight.spekdir.x /= GLight.spek;
      GLight.spekdir.y /= GLight.spek;
      GLight.spekdir.z /= GLight.spek;

      GLight.spek *= GLight.ambi;
    }

    GLight.spekcol.r =
    GLight.spekcol.g =
    GLight.spekcol.b = GLight.spek;

    GLight.ambicol.r =
    GLight.ambicol.g =
    GLight.ambicol.b = GLight.ambi;

    // Read tile data third
    hillslide = fget_next_float( fileread );
    slippyfriction = fget_next_float( fileread );
    airfriction = fget_next_float( fileread );
    waterfriction = fget_next_float( fileread );
    noslipfriction = fget_next_float( fileread );
    gravity = fget_next_float( fileread );
    slippyfriction = MAX( slippyfriction, sqrt( noslipfriction ) );
    airfriction    = MAX( airfriction,    sqrt( slippyfriction ) );
    waterfriction  = MIN( waterfriction,  pow( airfriction, 4.0f ) );

    GTile_Anim.updateand = fget_next_int( fileread );
    GTile_Anim.frameand = fget_next_int( fileread );
    GTile_Anim.bigframeand = ( GTile_Anim.frameand << 1 ) + 1;
    GTile_Dam.amount = fget_next_int( fileread );
    GTile_Dam.type = fget_next_damage( fileread );

    // Read weather data fourth
    GWeather.overwater = fget_next_bool( fileread );
    GWeather.timereset = fget_next_int( fileread );
    GWeather.time = GWeather.timereset;
    GWeather.player = 0;

    // Read extra data
    gs->mesh.exploremode = fget_next_bool( fileread );
    usefaredge = fget_next_bool( fileread );
    GCamera.swing = 0;
    GCamera.swingrate = fget_next_float( fileread );
    GCamera.swingamp = fget_next_float( fileread );


    // Read unnecessary data...  Only read if it exists...
    GFog.on = bfalse;
    GFog.affectswater = btrue;
    GFog.top = 100;
    GFog.bottom = 0;
    GFog.distance = 100;
    GFog.red = 255;
    GFog.grn = 255;
    GFog.blu = 255;
    GTile_Dam.parttype = MAXPRTPIP;
    GTile_Dam.partand = 255;
    GTile_Dam.sound = INVALID_SOUND;

    if ( fgoto_colon_yesno( fileread ) )
    {
      GFog.on           = CData.fogallowed;
      GFog.top          = fget_next_float( fileread );
      GFog.bottom       = fget_next_float( fileread );
      GFog.red          = fget_next_fixed( fileread );
      GFog.grn          = fget_next_fixed( fileread );
      GFog.blu          = fget_next_fixed( fileread );
      GFog.affectswater = fget_next_bool( fileread );

      GFog.distance = ( GFog.top - GFog.bottom );
      if ( GFog.distance < 1.0 )  GFog.on = bfalse;

      // Read extra stuff for damage tile particles...
      if ( fgoto_colon_yesno( fileread ) )
      {
        GTile_Dam.parttype = fget_int( fileread );
        GTile_Dam.partand  = fget_next_int( fileread );
        GTile_Dam.sound    = fget_next_int( fileread );
      }
    }

    // Allow slow machines to ignore the fancy stuff
    if ( !CData.twolayerwateron && gs->water.layer_count > 1 )
    {
      int iTmp;
      gs->water.layer_count = 1;
      iTmp = gs->water.layer[0].alpha_fp8;
      iTmp = FP8_MUL( gs->water.layer[1].alpha_fp8, iTmp ) + iTmp;
      if ( iTmp > 255 ) iTmp = 255;
      gs->water.layer[0].alpha_fp8 = iTmp;
    }


    fs_fileClose( fileread );

    // Do it
    make_speklut();
    make_lighttospek();
    make_spektable( GLight.spekdir );
    make_water( gs );
  }

}

//--------------------------------------------------------------------------------------------
void render_background( Uint16 texture )
{
  // ZZ> This function draws the large background

  CGame * gs = gfxState.gs;

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
  u = gs->water.layer[1].u;
  v = gs->water.layer[1].v;
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
    glDepthMask( GL_ALWAYS );

    glDisable( GL_CULL_FACE );

    GLTexture_Bind( gs->TxTexture + texture, &gfxState );

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
  CGame * gs = gfxState.gs;

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
  u = gs->water.layer[1].u;
  v = gs->water.layer[1].v;
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

    GLTexture_Bind( gs->TxTexture + texture, &gfxState );

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

  CGame * gs = gfxState.gs;

  GLVertex v[4];

  float x, y;
  float level, height;
  float size_umbra_x,size_umbra_y, size_penumbra_x,size_penumbra_y;
  float height_factor, ambient_factor, tile_factor;
  float alpha_umbra, alpha_penumbra, alpha_character, light_character;
  Sint8 hide;
  int i;


  Uint16 chrlightambi  = gs->ChrList[character].tlight.ambi_fp8.r + gs->ChrList[character].tlight.ambi_fp8.g + gs->ChrList[character].tlight.ambi_fp8.b;
  Uint16 chrlightspek  = gs->ChrList[character].tlight.spek_fp8.r + gs->ChrList[character].tlight.spek_fp8.g + gs->ChrList[character].tlight.spek_fp8.b;
  float  globlightambi = GLight.ambicol.r + GLight.ambicol.g + GLight.ambicol.b;
  float  globlightspek = GLight.spekcol.r + GLight.spekcol.g + GLight.spekcol.b;

  hide = gs->CapList[gs->ChrList[character].model].hidestate;
  if ( hide != NOHIDE && hide == gs->ChrList[character].aistate.state ) return;

  // Original points
  level = gs->ChrList[character].level;
  level += SHADOWRAISE;
  height = gs->ChrList[character].matrix.CNV( 3, 2 ) - level;
  if ( height < 0 ) height = 0;

  tile_factor = mesh_has_some_bits( gs->Mesh_Mem.fanlst, gs->ChrList[character].onwhichfan, MPDFX_WATER ) ? 0.5 : 1.0;

  height_factor   = MAX( MIN(( 5 * gs->ChrList[character].bmpdata.calc_size / height ), 1 ), 0 );
  ambient_factor  = ( float )( chrlightspek ) / ( float )( chrlightambi + chrlightspek );
  ambient_factor  = 0.5f * ( ambient_factor + globlightspek / ( globlightambi + globlightspek ) );
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
      glDepthFunc( GL_LEQUAL );

      // Choose texture.
      GLTexture_Bind( gs->TxTexture + particletexture, &gfxState );

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
      GLTexture_Bind( gs->TxTexture + particletexture, &gfxState );

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
//void render_bad_shadow(CHR_REF character)
//{
//  // ZZ> This function draws a sprite shadow
//
//  CGame * gs = gfxState.gs;
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
//  hide = gs->CapList[gs->ChrList[character].model].hidestate;
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
//      GLTexture_Bind( gs->TxTexture + particletexture, &gfxState);
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
void calc_chr_lighting( int x, int y, Uint16 tl, Uint16 tr, Uint16 bl, Uint16 br, Uint16 * spek, Uint16 * ambi )
{
  ( *spek ) = 0;
  ( *ambi ) = MIN( MIN( tl, tr ), MIN( bl, br ) );

  // Interpolate lighting level using tile corners
  switch ( x )
  {
    case 0:
      ( *spek ) += ( tl - ( *ambi ) ) << 1;
      ( *spek ) += ( bl - ( *ambi ) ) << 1;
      break;

    case 1:
    case 2:
      ( *spek ) += ( tl - ( *ambi ) );
      ( *spek ) += ( tr - ( *ambi ) );
      ( *spek ) += ( bl - ( *ambi ) );
      ( *spek ) += ( br - ( *ambi ) );
      break;

    case 3:
      ( *spek ) += ( tr - ( *ambi ) ) << 1;
      ( *spek ) += ( br - ( *ambi ) ) << 1;
      break;
  }


  switch ( y )
  {
    case 0:
      ( *spek ) += ( tl - ( *ambi ) ) << 1;
      ( *spek ) += ( tr - ( *ambi ) ) << 1;
      break;

    case 1:
    case 2:
      ( *spek ) += ( tl - ( *ambi ) );
      ( *spek ) += ( tr - ( *ambi ) );
      ( *spek ) += ( bl - ( *ambi ) );
      ( *spek ) += ( br - ( *ambi ) );
      break;

    case 3:
      ( *spek ) += ( bl - ( *ambi ) ) << 1;
      ( *spek ) += ( br - ( *ambi ) ) << 1;
      break;
  }

  ( *spek ) >>= 3;
};

//--------------------------------------------------------------------------------------------
void light_characters()
{
  // ZZ> This function figures out character lighting

  int cnt, tnc, x, y;
  Uint16 i0, i1, i2, i3;
  Uint16 spek, ambi;
  Uint32 vrtstart;
  Uint32 fan;

  CGame * gs = gfxState.gs;

  for ( cnt = 0; cnt < numdolist; cnt++ )
  {
    tnc = dolist[cnt];
    fan = gs->ChrList[tnc].onwhichfan;

    if(INVALID_FAN == fan) continue;

    vrtstart = gs->Mesh_Mem.fanlst[gs->ChrList[tnc].onwhichfan].vrt_start;

    x = gs->ChrList[tnc].pos.x;
    y = gs->ChrList[tnc].pos.y;
    x = ( x & 127 ) >> 5;  // From 0 to 3
    y = ( y & 127 ) >> 5;  // From 0 to 3

    i0 = gs->Mesh_Mem.vrt_lr_fp8[vrtstart + 0];
    i1 = gs->Mesh_Mem.vrt_lr_fp8[vrtstart + 1];
    i2 = gs->Mesh_Mem.vrt_lr_fp8[vrtstart + 2];
    i3 = gs->Mesh_Mem.vrt_lr_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    gs->ChrList[tnc].tlight.ambi_fp8.r = ambi;
    gs->ChrList[tnc].tlight.spek_fp8.r = spek;

    if ( !gs->mesh.exploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      gs->ChrList[tnc].tlight.turn_lr.r = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      gs->ChrList[tnc].tlight.turn_lr.r = 0;
    }

    i0 = gs->Mesh_Mem.vrt_lg_fp8[vrtstart + 0];
    i1 = gs->Mesh_Mem.vrt_lg_fp8[vrtstart + 1];
    i3 = gs->Mesh_Mem.vrt_lg_fp8[vrtstart + 2];
    i2 = gs->Mesh_Mem.vrt_lg_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    gs->ChrList[tnc].tlight.ambi_fp8.g = ambi;
    gs->ChrList[tnc].tlight.spek_fp8.g = spek;

    if ( !gs->mesh.exploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      gs->ChrList[tnc].tlight.turn_lr.g = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      gs->ChrList[tnc].tlight.turn_lr.g = 0;
    }

    i0 = gs->Mesh_Mem.vrt_lb_fp8[vrtstart + 0];
    i1 = gs->Mesh_Mem.vrt_lb_fp8[vrtstart + 1];
    i3 = gs->Mesh_Mem.vrt_lb_fp8[vrtstart + 2];
    i2 = gs->Mesh_Mem.vrt_lb_fp8[vrtstart + 3];
    calc_chr_lighting( x, y, i0, i1, i2, i3, &spek, &ambi );
    gs->ChrList[tnc].tlight.ambi_fp8.b = ambi;
    gs->ChrList[tnc].tlight.spek_fp8.b = spek;

    if ( !gs->mesh.exploremode )
    {
      // Look up spek direction using corners again
      i0 = (( i0 & 0xf0 ) << 8 ) & 0xf000;
      i1 = (( i1 & 0xf0 ) << 4 ) & 0x0f00;
      i3 = (( i3 & 0xf0 ) << 0 ) & 0x00f0;
      i2 = (( i2 & 0xf0 ) >> 4 ) & 0x000f;
      i0 = i0 | i1 | i3 | i2;
      gs->ChrList[tnc].tlight.turn_lr.b = ( lightdirectionlookup[i0] << 8 );
    }
    else
    {
      gs->ChrList[tnc].tlight.turn_lr.b = 0;
    }
  }
}

//--------------------------------------------------------------------------------------------
void light_particles()
{
  // ZZ> This function figures out particle lighting

  CGame * gs = gfxState.gs;

  int cnt;
  CHR_REF character;

  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    if ( !VALID_PRT(gs->PrtList,  cnt ) ) continue;

    switch ( gs->PrtList[cnt].type )
    {
      case PRTTYPE_LIGHT:
        {
          float ftmp = gs->PrtList[cnt].dyna.level * ( 127 * gs->PrtList[cnt].dyna.falloff ) / FP8_TO_FLOAT( FP8_MUL( gs->PrtList[cnt].size_fp8, gs->PrtList[cnt].size_fp8 ) );
          if ( ftmp > 255 ) ftmp = 255;

          gs->PrtList[cnt].lightr_fp8 =
          gs->PrtList[cnt].lightg_fp8 =
          gs->PrtList[cnt].lightb_fp8 = ftmp;
        }
        break;

      case PRTTYPE_ALPHA:
      case PRTTYPE_SOLID:
        {
          character = prt_get_attachedtochr( gs, cnt );
          if ( VALID_CHR( gs->ChrList,  character ) )
          {
            gs->PrtList[cnt].lightr_fp8 = gs->ChrList[character].tlight.spek_fp8.r + gs->ChrList[character].tlight.ambi_fp8.r;
            gs->PrtList[cnt].lightg_fp8 = gs->ChrList[character].tlight.spek_fp8.g + gs->ChrList[character].tlight.ambi_fp8.g;
            gs->PrtList[cnt].lightb_fp8 = gs->ChrList[character].tlight.spek_fp8.b + gs->ChrList[character].tlight.ambi_fp8.b;
          }
          else if ( INVALID_FAN != gs->PrtList[cnt].onwhichfan )
          {
            gs->PrtList[cnt].lightr_fp8 = gs->Mesh_Mem.vrt_lr_fp8[gs->Mesh_Mem.fanlst[gs->PrtList[cnt].onwhichfan].vrt_start];
            gs->PrtList[cnt].lightg_fp8 = gs->Mesh_Mem.vrt_lg_fp8[gs->Mesh_Mem.fanlst[gs->PrtList[cnt].onwhichfan].vrt_start];
            gs->PrtList[cnt].lightb_fp8 = gs->Mesh_Mem.vrt_lb_fp8[gs->Mesh_Mem.fanlst[gs->PrtList[cnt].onwhichfan].vrt_start];
          }
          else
          {
            gs->PrtList[cnt].lightr_fp8 =
              gs->PrtList[cnt].lightg_fp8 =
                gs->PrtList[cnt].lightb_fp8 = 0;
          }
        }
        break;

      default:
        gs->PrtList[cnt].lightr_fp8 =
          gs->PrtList[cnt].lightg_fp8 =
            gs->PrtList[cnt].lightb_fp8 = 0;
    };
  }

}

//--------------------------------------------------------------------------------------------
void render_water()
{
  // ZZ> This function draws all of the water fans

  int cnt;
  CGame * gs = gfxState.gs;

  // Bottom layer first
  if ( !gfxState.render_background && gs->water.layer_count > 1 )
  {
    cnt = 0;
    while ( cnt < renderlist.num_watr )
    {
      render_water_fan( renderlist.watr[cnt], 1, renderlist.watr_mode[cnt]);
      cnt++;
    }
  }

  // Top layer second
  if ( !gfxState.render_overlay && gs->water.layer_count > 0 )
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
  CGame * gs = gfxState.gs;

  // Bottom layer first
  if ( !gfxState.render_background && gs->water.layer_count > 1 )
  {
    float ambi_level = FP8_TO_FLOAT( gs->water.layer[1].lightadd_fp8 + gs->water.layer[1].lightlevel_fp8 );
    float spek_level =  FP8_TO_FLOAT( gs->water.speklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( gs->water.light )
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
  if ( !gfxState.render_overlay && gs->water.layer_count > 0 )
  {
    float ambi_level = ( gs->water.layer[1].lightadd_fp8 + gs->water.layer[1].lightlevel_fp8 ) / 255.0;
    float spek_level =  FP8_TO_FLOAT( gs->water.speklevel_fp8 );
    float spekularity = MIN( 40, spek_level / ambi_level ) + 2;

    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if ( gs->water.light )
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
  int cnt, tnc;

  CGame * gs = gfxState.gs;

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
      tnc = dolist[cnt];
      if ( gs->ChrList[tnc].bmpdata.shadow != 0 || gs->CapList[gs->ChrList[tnc].model].forceshadow && mesh_has_no_bits( gs->Mesh_Mem.fanlst, gs->ChrList[tnc].onwhichfan, MPDFX_SHINY ) )
        render_shadow( tnc );
    }
  }
  ATTRIB_POP( "render_good_shadows" );
}

//--------------------------------------------------------------------------------------------
//void render_bad_shadows()
//{
//  int cnt, tnc;
//
//  CGame * gs = gfxState.gs;
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
//      tnc = dolist[cnt];
//      //if(gs->ChrList[tnc].attachedto == MAXCHR)
//      //{
//      if (gs->ChrList[tnc].bmpdata.calc_shadowsize != 0 || gs->CapList[gs->ChrList[tnc].model].forceshadow && HAS_NO_BITS(Mesh[gs->ChrList[tnc].onwhichfan].fx, MPDFX_SHINY))
//        render_bad_shadow(tnc);
//      //}
//    }
//  }
//  ATTRIB_POP("render_bad_shadows");
//}

//--------------------------------------------------------------------------------------------
void render_character_reflections()
{
  int cnt, tnc;

  CGame * gs = gfxState.gs;

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
      tnc = dolist[cnt];
      if ( mesh_has_some_bits( gs->Mesh_Mem.fanlst, gs->ChrList[tnc].onwhichfan, MPDFX_SHINY ) )
        render_refmad( tnc, gs->ChrList[tnc].alpha_fp8 / 2 );
    }
  }
  ATTRIB_POP( "render_character_reflections" );

};

//--------------------------------------------------------------------------------------------
void render_normal_fans()
{
  CGame * gs = gfxState.gs;

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
      gs->mesh.last_texture = texture;
      GLTexture_Bind( gs->TxTexture + texture, &gfxState );
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
  CGame * gs = gfxState.gs;

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
      gs->mesh.last_texture = texture;
      GLTexture_Bind( gs->TxTexture + texture, &gfxState );
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
//      gs->mesh.last_texture = texture;
//      GLTexture_Bind( gs->TxTexture + texture, &gfxState);
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
void render_reflected_fans_ref()
{
  CGame * gs = gfxState.gs;

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
      gs->mesh.last_texture = texture;
      GLTexture_Bind( gs->TxTexture + texture, &gfxState );
      for ( tnc = 0; tnc < renderlist.num_shine; tnc++ )
      {
        fan = renderlist.reflc[tnc];
        render_fan_ref( fan, texture, GCamera.tracklevel );
      };
    }
  }
  ATTRIB_POP( "render_reflected_fans_ref" );
};

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
//      gs->mesh.last_texture = texture;
//      GLTexture_Bind( gs->TxTexture + texture, &gfxState);
//      for (tnc = 0; tnc < renderlist.num_reflc; tnc++)
//      {
//        fan = renderlist.reflc[tnc];
//        render_fan_ref(fan, texture, GCamera.tracklevel);
//      };
//    }
//
//  }
//  ATTRIB_POP("render_sha_fans_ref");
//};
//
//--------------------------------------------------------------------------------------------
void render_solid_characters()
{
  int cnt, tnc;

  CGame * gs = gfxState.gs;

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
      tnc = dolist[cnt];
      if ( gs->ChrList[tnc].alpha_fp8 == 255 && gs->ChrList[tnc].light_fp8 == 255 )
        render_mad( tnc, 255 );
    }
  }
  ATTRIB_POP( "render_solid_characters" );

};

//--------------------------------------------------------------------------------------------
void render_alpha_characters()
{
  int cnt, tnc;
  Uint8 trans;

  CGame * gs = gfxState.gs;

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
      tnc = dolist[cnt];
      if ( gs->ChrList[tnc].alpha_fp8 != 255 )
      {
        trans = gs->ChrList[tnc].alpha_fp8;

        if (( gs->ChrList[tnc].alpha_fp8 + gs->ChrList[tnc].light_fp8 ) < SEEINVISIBLE &&  gs->cl->seeinvisible && chr_is_player(gs, tnc) && gs->PlaList[gs->ChrList[tnc].whichplayer].is_local )
          trans = SEEINVISIBLE - gs->ChrList[tnc].light_fp8;

        if ( trans > 0 )
        {
          render_mad( tnc, trans );
        };
      }
    }

  }
  ATTRIB_POP( "render_alpha_characters" );

};

//--------------------------------------------------------------------------------------------
// render the water hilights, etc. using global lighting
void render_water_highlights()
{
  ATTRIB_PUSH( "render_water_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT );
  {
    GLfloat light_position[] = { 10000*GLight.spekdir.x, 10000*GLight.spekdir.y, 10000*GLight.spekdir.z, 1.0 };
    GLfloat lmodel_ambient[] = { GLight.ambicol.r, GLight.ambicol.g, GLight.ambicol.b, 1.0 };
    GLfloat light_diffuse[]  = { GLight.spekcol.r, GLight.spekcol.g, GLight.spekcol.b, 1.0 };

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
  int cnt, tnc;

  CGame * gs = gfxState.gs;

  ATTRIB_PUSH( "render_character_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT );
  {
    GLfloat light_none[]     = {0, 0, 0, 0};
    GLfloat light_position[] = { 10000*GLight.spekdir.x, 10000*GLight.spekdir.y, 10000*GLight.spekdir.z, 1.0 };
    GLfloat lmodel_ambient[] = { GLight.ambicol.r, GLight.ambicol.g, GLight.ambicol.b, 1.0 };
    GLfloat light_specular[] = { GLight.spekcol.r, GLight.spekcol.g, GLight.spekcol.b, 1.0 };

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
      tnc = dolist[cnt];

      if ( gs->ChrList[tnc].sheen_fp8 == 0 && gs->ChrList[tnc].light_fp8 == 255 && gs->ChrList[tnc].alpha_fp8 == 255 ) continue;

      render_mad_lit( tnc );
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

  CGame * gs = gfxState.gs;

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
    if ( !gs->water.light )
    {
      ATTRIB_GUARD_OPEN( inp_attrib_stack );
      render_alpha_water();
      ATTRIB_GUARD_CLOSE( inp_attrib_stack, out_attrib_stack );
    };

    // Do self-lit water
    if ( gs->water.light )
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

#if defined(DEBUG_BBOX) && defined(_DEBUG)
  if(CData.DevMode)
  {
    int i;

    for(i=0; i<MAXCHR; i++)
    {
      if(!gs->ChrList[i].on) continue;

      mad_display_bbox_tree(2, gs->ChrList[i].matrix, gs->MadList + gs->ChrList[i].model, gs->ChrList[i].anim.last, gs->ChrList[i].anim.next );
    }
  };

#endif

};


//--------------------------------------------------------------------------------------------
//void draw_scene_zreflection()
//{
//  // ZZ> This function draws 3D objects
//
//  Uint16 cnt, tnc;
//  Uint8 trans;
//
//  CGame * gs = gfxState.gs;
//
//  // Clear the image if need be
//  // PORT: I don't think this is needed if(render_background) { clear_surface(lpDDSBack); }
//  // Zbuffer is cleared later
//
//  // Render the reflective floors
//  glDisable(GL_DEPTH_TEST);
//  glDepthMask(GL_FALSE);
//  glDisable(GL_BLEND);
//
//  // Renfer ref
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  gs->mesh.last_texture = 0;
//  for (cnt = 0; cnt < renderlist.num_shine; cnt++)
//    render_fan(renderlist.shine[cnt]);
//
//  // Renfer sha
//  // BAD: DRAW SHADOW STUFF TOO
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < renderlist.num_reflc; cnt++)
//    render_fan(renderlist.reflc[cnt]);
//
//  glEnable(GL_DEPTH_TEST);
//  glDepthMask(GL_TRUE);
//  if(CData.refon)
//  {
//    // Render reflections of characters
//    glFrontFace(GL_CCW);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//    glDepthFunc(GL_LEQUAL);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if((Mesh[gs->ChrList[tnc].onwhichfan].fx&MPDFX_SHINY))
//        render_refmad(tnc, gs->ChrList[tnc].alpha_fp8 & gs->ChrList[tnc].light_fp8);
//    }
//
//    // [claforte] I think this is wrong... I think we should choose some other depth func.
//    glDepthFunc(GL_ALWAYS);
//
//    // Render the reflected sprites
//    render_particle_reflections();
//  }
//
//  // Render the shadow floors
//  gs->mesh.last_texture = 0;
//
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < renderlist.num_reflc; cnt++)
//    render_fan(renderlist.reflc[cnt]);
//
//  // Render the shadows
//  if (CData.shaon)
//  {
//    if (CData.shasprite)
//    {
//      // Bad shadows
//      glDepthMask(GL_FALSE);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(gs->ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((gs->ChrList[tnc].light_fp8==255 && gs->ChrList[tnc].alpha_fp8==255) || gs->CapList[gs->ChrList[tnc].model].forceshadow) && gs->ChrList[tnc].bmpdata.calc_shadowsize!=0)
//            render_bad_shadow(tnc);
//        }
//      }
//      glDisable(GL_BLEND);
//      glDepthMask(GL_TRUE);
//    }
//    else
//    {
//      // Good shadows for me
//      glDepthMask(GL_FALSE);
//      glDepthFunc(GL_LEQUAL);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_COLOR, GL_ZERO);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(gs->ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((gs->ChrList[tnc].light_fp8==255 && gs->ChrList[tnc].alpha_fp8==255) || gs->CapList[gs->ChrList[tnc].model].forceshadow) && gs->ChrList[tnc].bmpdata.calc_shadowsize!=0)
//            render_shadow(tnc);
//        }
//      }
//
//      glDisable(GL_BLEND);
//      glDepthMask ( GL_TRUE );
//    }
//  }
//
//  // Render the normal characters
//  ATTRIB_PUSH("zref",GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_TRUE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LESS);
//
//    glDisable(GL_BLEND);
//
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(gs->ChrList[tnc].alpha_fp8==255 && gs->ChrList[tnc].light_fp8==255)
//        render_mad(tnc, 255);
//    }
//  }
//  ATTRIB_POP("zref");
//
//  //// Render the sprites
//  glDepthMask ( GL_FALSE );
//  glEnable(GL_BLEND);
//
//  // Now render the transparent characters
//  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_FALSE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(gs->ChrList[tnc].alpha_fp8!=255 && gs->ChrList[tnc].light_fp8==255)
//      {
//        trans = gs->ChrList[tnc].alpha_fp8;
//        if(trans < SEEINVISIBLE && (gs->cl->seeinvisible || gs->ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//
//  }
//
//  // And alpha water floors
//  if(!gs->water.light)
//    render_water();
//
//  // Then do the light characters
//  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_FALSE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(gs->ChrList[tnc].light_fp8!=255)
//      {
//        trans = FP8_TO_FLOAT(FP8_MUL(gs->ChrList[tnc].light_fp8, gs->ChrList[tnc].alpha_fp8)) * 0.5f;
//        if(trans < SEEINVISIBLE && (gs->cl->seeinvisible || gs->ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//  }
//
//  // Do phong highlights
//  if(CData.phongon && gs->ChrList[tnc].sheen_fp8 > 0)
//  {
//    Uint16 texturesave, envirosave;
//
//    ATTRIB_PUSH("zref", GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//    {
//      glDepthMask ( GL_FALSE );
//      glEnable(GL_DEPTH_TEST);
//      glDepthFunc(GL_LEQUAL);
//
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_ONE, GL_ONE);
//
//      envirosave = gs->ChrList[tnc].enviro;
//      texturesave = gs->ChrList[tnc].skin + gs->MadList[gs->ChrList[tnc].model].skinstart;
//      gs->ChrList[tnc].enviro = btrue;
//      gs->ChrList[tnc].skin + gs->MadList[gs->ChrList[tnc].model].skinstart = TX_PHONG;  // The phong map texture...
//      render_enviromad(tnc, (gs->ChrList[tnc].alpha_fp8 * spek_global[gs->ChrList[tnc].sheen_fp8][gs->ChrList[tnc].light_fp8]) / 2, GL_TEXTURE_2D);
//      gs->ChrList[tnc].skin + gs->MadList[gs->ChrList[tnc].model].skinstart = texturesave;
//      gs->ChrList[tnc].enviro = envirosave;
//    };
//    ATTRIB_POP("zref");
//  }
//
//
//  // Do light water
//  if(gs->water.light)
//    render_water();
//
//  // Turn Z buffer back on, alphablend off
//  render_particles();
//
//  // Done rendering
//};

bool_t draw_texture_box( GLtexture * ptx, FRect * tx_rect, FRect * sc_rect )
{
  FRect rtmp;

  if( NULL == sc_rect ) return bfalse;

  if(NULL != ptx)
  {
    GLTexture_Bind( ptx, &gfxState );
  }

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

  CGui * gui = Get_CGui();

  FRect tx_rect, sc_rect;
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

    tx_rect.left   = (( float ) BlipList[color].rect.left   ) / ( float ) GLTexture_GetTextureWidth(&gui->TxBlip)  + 0.01;
    tx_rect.right  = (( float ) BlipList[color].rect.right  ) / ( float ) GLTexture_GetTextureWidth(&gui->TxBlip)  - 0.01;
    tx_rect.top    = (( float ) BlipList[color].rect.top    ) / ( float ) GLTexture_GetTextureHeight(&gui->TxBlip) + 0.01;
    tx_rect.bottom = (( float ) BlipList[color].rect.bottom ) / ( float ) GLTexture_GetTextureHeight(&gui->TxBlip) - 0.01;

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

  CGame * gs = gfxState.gs;

  int position, blipx, blipy;
  int width, height;
  FRect tx_rect, sc_rect;

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

    tx_rect.left   = (( float ) iconrect.left   ) / GLTexture_GetTextureWidth(gs->TxIcon + icontype);
    tx_rect.right  = (( float ) iconrect.right  ) / GLTexture_GetTextureWidth(gs->TxIcon + icontype);
    tx_rect.top    = (( float ) iconrect.top    ) / GLTexture_GetTextureHeight(gs->TxIcon + icontype);
    tx_rect.bottom = (( float ) iconrect.bottom ) / GLTexture_GetTextureHeight(gs->TxIcon + icontype);

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
void draw_one_font( int fonttype, float x, float y )
{
  // ZZ> This function draws a letter or number
  // GAC> Very nasty version for starters.  Lots of room for improvement.

  GLfloat dx, dy;
  FRect tx_rect, sc_rect;

  dx = 2.0 / 512;
  dy = 1.0 / 256;

  tx_rect.left   = bmfont.rect[fonttype].x * dx + 0.001f;
  tx_rect.right  = ( bmfont.rect[fonttype].x + bmfont.rect[fonttype].w ) * dx - 0.001f;
  tx_rect.top    = bmfont.rect[fonttype].y * dy + 0.001f;
  tx_rect.bottom = ( bmfont.rect[fonttype].y + bmfont.rect[fonttype].h ) * dy;

  sc_rect.left   = x;
  sc_rect.right  = x + bmfont.rect[fonttype].w;
  sc_rect.top    = gfxState.scry - y;
  sc_rect.bottom = gfxState.scry - (y + bmfont.rect[fonttype].h);

  draw_texture_box(NULL, &tx_rect, &sc_rect);
}

//--------------------------------------------------------------------------------------------
void draw_map( float x, float y )
{
  // ZZ> This function draws the map

  CGame * gs = gfxState.gs;
  FRect tx_rect, sc_rect;

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
    tx_rect.right  = (float)GLTexture_GetImageWidth(&gs->TxMap) / (float)GLTexture_GetTextureWidth(&gs->TxMap);
    tx_rect.top    = 0.0f;
    tx_rect.bottom = (float)GLTexture_GetImageHeight(&gs->TxMap) / (float)GLTexture_GetTextureHeight(&gs->TxMap);

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

  CGui * gui = Get_CGui();

  int noticks;
  FRect tx_rect, sc_rect;
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
      GLTexture_Bind( &gui->TxBars, &gfxState );
      tx_rect.left   = ( float ) tabrect[bartype].left   / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
      tx_rect.right  = ( float ) tabrect[bartype].right  / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
      tx_rect.top    = ( float ) tabrect[bartype].top    / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );
      tx_rect.bottom = ( float ) tabrect[bartype].bottom / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );

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

        tx_rect.left   = ( float ) barrect[bartype].left   / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[bartype].right  / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[bartype].top    / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[bartype].bottom / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );

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

        tx_rect.left   = ( float ) barrect[bartype].left   / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[bartype].right  / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[bartype].top    / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[bartype].bottom / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );

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

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );

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

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );

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

        tx_rect.left   = ( float ) barrect[0].left   / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.right  = ( float ) barrect[0].right  / ( float ) GLTexture_GetTextureWidth( &gui->TxBars );
        tx_rect.top    = ( float ) barrect[0].top    / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );
        tx_rect.bottom = ( float ) barrect[0].bottom / ( float ) GLTexture_GetTextureHeight( &gui->TxBars );

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
int draw_string( BMFont * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... )
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer
  char cTmp;
  GLfloat current_tint[4], temp_tint[4];
  STRING szText;
  va_list args;
  int cnt = 1;

  if(NULL==pfnt || NULL == szFormat) return 0;

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
        draw_one_font( cTmp, x, y );
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
int length_of_word( char *szText )
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
      x += bmfont.spacing_x[bmfont.ascii_table[cTmp]];
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
    x += bmfont.spacing_x[bmfont.ascii_table[cTmp]];
    cnt++;
    cTmp = szText[cnt];
  }
  return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string( BMFont * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... )
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
    newy = y + bmfont.spacing_y;

    cnt = 1;
    cTmp = szText[0];
    while ( cTmp != 0 )
    {
      // Check each new word for wrapping
      if ( newword )
      {
        int endx = x + length_of_word( szText + cnt - 1 );

        newword = bfalse;
        if ( endx > maxx )
        {
          // Wrap the end and cut off spaces and tabs
          x = xstt + bmfont.spacing_y;
          y += bmfont.spacing_y;
          newy += bmfont.spacing_y;
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
          y += bmfont.spacing_y;
          newy += bmfont.spacing_y;
        }
        else
        {
          // Normal letter
          cTmp = bmfont.ascii_table[cTmp];
          draw_one_font( cTmp, x, y );
          x += bmfont.spacing_x[cTmp];
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
int draw_status( BMFont * pfnt, Status * pstat )
{
  // ZZ> This function shows a ichr's icon, status and inventory
  //     The x,y coordinates are the top left point of the image to draw

  Uint16 item, imdl;
  char cTmp;
  char *readtext;
  int  ix,iy, iystt;
  CHR_REF ichr;
  Chr * chrlst;
  Mad * madlst;

  Chr * pchr;
  Mad * pmad;
  Cap * pcap;
  CGame * gs;

  float life, lifemax;
  float mana, manamax;
  int cnt;

  if(NULL == gfxState.gs) return;
  gs = gfxState.gs;
  chrlst = gs->ChrList;
  madlst = gs->MadList;

  if(NULL == pstat) return 0;
  ichr = pstat->chr_ref;
  ix = pstat->pos.x;
  iy = pstat->pos.y;
  iystt = iy;

  if( !VALID_CHR( chrlst, ichr) ) return 0;
  pchr = chrlst + ichr;

  imdl = pchr->model;
  if( !VALID_MDL(imdl) ) return 0;

  pmad = madlst + imdl;
  pcap = gs->CapList + imdl;

  life    = FP8_TO_FLOAT( pchr->life_fp8 );
  lifemax = FP8_TO_FLOAT( pchr->lifemax_fp8 );
  mana    = FP8_TO_FLOAT( pchr->mana_fp8 );
  manamax = FP8_TO_FLOAT( pchr->manamax_fp8 );

  // Write the ichr's first name
  if ( pchr->nameknown )
    readtext = pchr->name;
  else
    readtext = pcap->classname;

  for ( cnt = 0; cnt < 6; cnt++ )
  {
    cTmp = readtext[cnt];
    if ( cTmp == ' ' || cTmp == 0 )
    {
      generictext[cnt] = 0;
      break;

    }
    else
      generictext[cnt] = cTmp;
  }
  generictext[6] = 0;
  iy += draw_string( pfnt, ix + 8, iy, NULL, generictext );

  // Write the character's money
  iy += 8 + draw_string( pfnt, ix + 8, iy, NULL, "$%4d", pchr->money );

  // Draw the icons
  draw_one_icon( gs->skintoicon[pchr->skin_ref + pmad->skinstart], ix + 40, iy, pchr->sparkle );
  item = chr_get_holdingwhich( chrlst, MAXCHR, ichr, SLOT_LEFT );
  if ( VALID_CHR( chrlst,  item ) )
  {
    if ( chrlst[item].icon )
    {
      draw_one_icon( gs->skintoicon[chrlst[item].skin_ref + madlst[chrlst[item].model].skinstart], ix + 8, iy, chrlst[item].sparkle );
      if ( chrlst[item].ammomax != 0 && chrlst[item].ammoknown )
      {
        if ( !gs->CapList[chrlst[item].model].isstackable || chrlst[item].ammo > 1 )
        {
          // Show amount of ammo left
          draw_string( pfnt, ix + 8, iy - 8, NULL, "%2d", chrlst[item].ammo );
        }
      }
    }
    else
      draw_one_icon( gs->bookicon + ( chrlst[item].money % MAXSKIN ), ix + 8, iy, chrlst[item].sparkle );
  }
  else
    draw_one_icon( gs->nullicon, ix + 8, iy, NOSPARKLE );

  item = chr_get_holdingwhich( chrlst, MAXCHR, ichr, SLOT_RIGHT );
  if ( VALID_CHR( chrlst,  item ) )
  {
    if ( chrlst[item].icon )
    {
      draw_one_icon( gs->skintoicon[chrlst[item].skin_ref + madlst[chrlst[item].model].skinstart], ix + 72, iy, chrlst[item].sparkle );
      if ( chrlst[item].ammomax != 0 && chrlst[item].ammoknown )
      {
        if ( !gs->CapList[chrlst[item].model].isstackable || chrlst[item].ammo > 1 )
        {
          // Show amount of ammo left
          draw_string( pfnt, ix + 72, iy - 8, NULL, "%2d", chrlst[item].ammo );
        }
      }
    }
    else
      draw_one_icon( gs->bookicon + ( chrlst[item].money % MAXSKIN ), ix + 72, iy, chrlst[item].sparkle );
  }
  else
    draw_one_icon( gs->nullicon, ix + 72, iy, NOSPARKLE );

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

  CGame * gs = gfxState.gs;

  if(!mapon) return bfalse;

  draw_map( maprect.left, maprect.top );

  for ( cnt = 0; cnt < numblip; cnt++ )
  {
    draw_blip( BlipList[cnt].c, maprect.left + BlipList[cnt].x, maprect.top + BlipList[cnt].y );
  };

  if ( youarehereon && ( gs->wld_frame&8 ) )
  {
    for ( ipla = 0; ipla < MAXPLAYER; ipla++ )
    {
      if ( !VALID_PLA( gs->PlaList, ipla ) || INBITS_NONE == gs->PlaList[ipla].device ) continue;

      ichr = PlaList_get_character( gs, ipla );
      if ( !VALID_CHR( gs->ChrList,  ichr ) || !gs->ChrList[ichr].alive ) continue;

      draw_blip( (COLR)0, maprect.left + MAPSIZE * mesh_fraction_x( &(gs->mesh), gs->ChrList[ichr].pos.x ) * mapscale, maprect.top + MAPSIZE * mesh_fraction_y( &(gs->mesh), gs->ChrList[ichr].pos.y ) * mapscale );
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
int do_messages( BMFont * pfnt, int x, int y )
{
  int cnt, tnc;
  int ystt = y;

  CGame * gs = gfxState.gs;

  if ( NULL==pfnt || !CData.messageon ) return 0;

  // Display the messages
  tnc = GMsg.start;
  for ( cnt = 0; cnt < CData.maxmessage; cnt++ )
  {
    // mesages with negative times never time out!
    if ( GMsg.list[tnc].time != 0 )
    {
      y += draw_wrap_string( pfnt, x, y, NULL, gfxState.scrx - CData.wraptolerance - x, GMsg.list[tnc].textdisplay );

      if ( GMsg.list[tnc].time > 0 )
      {
        GMsg.list[tnc].time -= gs->cl->msg_timechange;
        if ( GMsg.list[tnc].time < 0 ) GMsg.list[tnc].time = 0;
      };
    }
    tnc++;
    tnc %= CData.maxmessage;
  }

  return y - ystt;
}

//--------------------------------------------------------------------------------------------
int do_status( CClient * cs, BMFont * pfnt, int x, int y)
{
  int cnt;
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
void draw_text( BMFont *  pfnt )
{
  // ZZ> This function spits out some words

  char text[512];
  int y, fifties, seconds, minutes;

  CGame * gs = gfxState.gs;

  Begin2DMode();
  {
    // Status bars
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
      CHR_REF pla_chr = PlaList_get_character( gs, 0 );

      y += draw_string( pfnt, 0, y, NULL, "%2.3f FPS, %2.3f UPS", stabilized_fps, stabilized_ups );
      y += draw_string( pfnt, 0, y, NULL, "estimated max FPS %2.3f", gfxState.est_max_fps );


      if( CData.DevMode )
      {
        y += draw_string( pfnt, 0, y, NULL, "wld_frame %d, gs->wld_clock %d, all_clock %d", gs->wld_frame, gs->wld_clock, gs->all_clock );
        y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", gs->ChrList[pla_chr].pos.x, gs->ChrList[pla_chr].pos.y, gs->ChrList[pla_chr].pos.z );
        y += draw_string( pfnt, 0, y, NULL, "<%3.2f,%3.2f,%3.2f>", gs->ChrList[pla_chr].vel.x, gs->ChrList[pla_chr].vel.y, gs->ChrList[pla_chr].vel.z );
      }
    }

    {
      CHR_REF ichr, iref;
      GLvector tint = {0.5, 1.0, 1.0, 1.0};

      ichr = PlaList_get_character( gs, 0 );
      if( VALID_CHR( gs->ChrList,  ichr) )
      {
        iref = chr_get_attachedto( gs->ChrList, MAXCHR,ichr);
        if( VALID_CHR( gs->ChrList, iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 holder == %s(%s)", gs->ChrList[iref].name, gs->CapList[gs->ChrList[iref].model].classname );
        };

        iref = chr_get_inwhichpack( gs->ChrList, MAXCHR,ichr);
        if( VALID_CHR( gs->ChrList, iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 packer == %s(%s)", gs->ChrList[iref].name, gs->CapList[gs->ChrList[iref].model].classname );
        };

        iref = chr_get_onwhichplatform( gs->ChrList, MAXCHR,ichr);
        if( VALID_CHR( gs->ChrList, iref) )
        {
          y += draw_string( pfnt, 0, y, tint.v, "PLA0 platform == %s(%s)", gs->ChrList[iref].name, gs->CapList[gs->ChrList[iref].model].classname );
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


    // Player DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F5 ) && CData.DevMode )
    {
      CHR_REF pla_chr = PlaList_get_character( gs, 0 );

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

      y += draw_string( pfnt, 0, y, NULL, "~PLA0 %5.1f %5.1f", gs->ChrList[pla_chr].pos.x / 128.0, gs->ChrList[pla_chr].pos.y / 128.0  );

      pla_chr = PlaList_get_character( gs, 1 );
      y += draw_string( pfnt, 0, y, NULL, "~PLA1 %5.1f %5.1f", gs->ChrList[pla_chr].pos.x / 128.0, gs->ChrList[pla_chr].pos.y / 128.0 );
    }


    // GLOBAL DEBUG MODE
    if ( SDLKEYDOWN( SDLK_F6 ) && CData.DevMode )
    {
      char * net_status;
      bool_t client_running = bfalse, server_running = bfalse;
      client_running = CClient_Running(gs->cl);
      server_running = sv_Running(gs->sv);

      net_status = "local game";
      if (client_running && server_running)
      {
        net_status = "client + server";
      }
      else if (client_running && !server_running)
      {
        net_status = "client";
      }
      else if (!client_running && server_running)
      {
        net_status = "server";
      }

      // More debug information
      y += draw_string( pfnt, 0, y, NULL, "!!!DEBUG MODE-6!!!" );
      y += draw_string( pfnt, 0, y, NULL, "~FREEPRT %d", gs->PrtFreeList_count );
      y += draw_string( pfnt, 0, y, NULL, "~FREECHR %d",  gs->ChrFreeList_count );
      y += draw_string( pfnt, 0, y, NULL, "~MACHINE %s", net_status );
      y += draw_string( pfnt, 0, y, NULL, "~EXPORT %d", gs->modstate.exportvalid );
      y += draw_string( pfnt, 0, y, NULL, "~FOGAFF %d", GFog.affectswater );
      y += draw_string( pfnt, 0, y, NULL, "~PASS %d" SLASH_STRING "%d", gs->ShopList_count, gs->PassList_count );
      y += draw_string( pfnt, 0, y, NULL, "~NETPLAYERS %d", gs->sv->num_loaded );
      y += draw_string( pfnt, 0, y, NULL, "~DAMAGEPART %d", GTile_Dam.parttype );
    }

    // CAMERA DEBUG MODE
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
    if ( Get_CGui()->net_messagemode )
    {
      y += draw_wrap_string( pfnt, 0, y, NULL, gfxState.scrx - CData.wraptolerance, GNetMsg.buffer );
    }


    // Messages
    y += do_messages( pfnt, 0, y );
  }
  End2DMode();
}

//--------------------------------------------------------------------------------------------
static bool_t pageflip_requested = bfalse;
static bool_t clear_requested    = btrue;

//--------------------------------------------------------------------------------------------
bool_t query_clear()
{
  return clear_requested;
};

//--------------------------------------------------------------------------------------------
bool_t query_pageflip()
{
  return pageflip_requested;
};

//--------------------------------------------------------------------------------------------
bool_t request_pageflip()
{
  bool_t retval = !pageflip_requested;
  pageflip_requested = btrue;

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t do_pageflip()
{
  bool_t retval = pageflip_requested;
  CGame * gs = gfxState.gs;

  if ( pageflip_requested )
  {
    SDL_GL_SwapBuffers();
    gs->all_frame++;
    fps_loops++;
    pageflip_requested = bfalse;
    clear_requested    = btrue;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t do_clear()
{
  bool_t retval = clear_requested;

  if ( clear_requested )
  {
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    clear_requested = bfalse;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
void draw_scene()
{
  Begin3DMode();

  make_prtlist();
  do_dynalight();
  light_characters();
  light_particles();

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
}

//--------------------------------------------------------------------------------------------
void draw_main( float frameDuration )
{
  // ZZ> This function does all the drawing stuff

  draw_scene();

  draw_text( &bmfont );

  request_pageflip();
}

//--------------------------------------------------------------------------------------------
void load_blip_bitmap( char * modname )
{
  //This function loads the blip bitmaps
  CGui * gui = Get_CGui();
  int cnt;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.blip_bitmap );
  if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &gui->TxBlip, CStringTmp1, INVALID_KEY ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.blip_bitmap );
    if ( INVALID_TEXTURE == GLTexture_Load( GL_TEXTURE_2D,  &gui->TxBlip, CStringTmp1, INVALID_KEY ) )
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
  load_font( CStringTmp1, CStringTmp2 );
}



/********************> Reshape3D() <*****/
void Reshape3D( int w, int h )
{
  glViewport( 0, 0, w, h );
}

bool_t glinit( GraphicState * g, ConfigData * cd )
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
  gfx_set_mode(g);

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
bool_t gfx_set_mode(GraphicState * g)
{
  //This function loads all the graphics based on the game settings

  if(!gfx_initialized) return bfalse;


  // ---- reset the SDL video mode ----
  {
    //Enable antialiasing X16
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    //Force OpenGL hardware acceleration (This must be done before video mode)
    if(g->gfxacceleration)
    {
      SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    }

    /* Set the OpenGL Attributes */
  #ifndef __unix__
    // Under Unix we cannot specify these, we just get whatever format
    // the framebuffer has, specifying depths > the framebuffer one
    // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   g->colordepth );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, g->colordepth );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  g->colordepth );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, g->scrd );
  #endif
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    g->surface = SDL_SetVideoMode( g->scrx, g->scry, g->scrd, SDL_DOUBLEBUF | SDL_OPENGL | ( g->fullscreen ? SDL_FULLSCREEN : 0 ) );
    if ( NULL == g->surface )
    {
      log_error( "Unable to set video mode: %s\n", SDL_GetError() );
      exit( 1 );
    }
  }

  glEnable(GL_MULTISAMPLE_ARB);
  //glEnable(GL_MULTISAMPLE);

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
      SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
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

  glViewport(0, 0, gfxState.surface->w, gfxState.surface->h);

  return btrue;
}

//--------------------------------------------------------------------------------------------
GraphicState * GraphicState_new(GraphicState * g, ConfigData * cd)
{
  //fprintf( stdout, "GraphicState_new()\n");

  if(NULL == g || NULL == cd) return NULL;

  memset(g, 0, sizeof(GraphicState));

  GraphicState_synch(g, cd);

  // put a reasonable value in here
  g->est_max_fps = 30;

  return g;
};

//--------------------------------------------------------------------------------------------
bool_t GraphicState_synch(GraphicState * g, ConfigData * cd)
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
    gfx_set_mode(g);
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
//  if ( INVALID_TEXTURE != GLTexture_GetTextureID(mproc->TxTitleImage + image) )
//  {
//    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//    Begin2DMode();
//    glNormal3f( 0, 0, 1 ); // glNormal3f( 0, 1, 0 );
//
//    /* Calculate the texture width & height */
//    txWidth = ( GLfloat )( GLTexture_GetImageWidth( mproc->TxTitleImage + image )/GLTexture_GetTextureWidth( mproc->TxTitleImage + image ) );
//    txHeight = ( GLfloat )( GLTexture_GetImageHeight( mproc->TxTitleImage + image )/GLTexture_GetTextureHeight( mproc->TxTitleImage + image ) );
//
//    /* Bind the texture */
//    GLTexture_Bind( mproc->TxTitleImage + image, &gfxState );
//
//    /* Draw the quad */
//    glBegin( GL_QUADS );
//    glTexCoord2f( 0, 1 ); glVertex2f( x, gfxState.scry - y - GLTexture_GetImageHeight( mproc->TxTitleImage + image ) );
//    glTexCoord2f( txWidth, 1 ); glVertex2f( x + GLTexture_GetImageWidth( mproc->TxTitleImage + image ), gfxState.scry - y - GLTexture_GetImageHeight( mproc->TxTitleImage + image ) );
//    glTexCoord2f( txWidth, 1-txHeight ); glVertex2f( x + GLTexture_GetImageWidth( mproc->TxTitleImage + image ), gfxState.scry - y );
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

  CGame * gs = gfxState.gs;

  if ( !VALID_CHR( gs->ChrList,  chr_ref ) || gs->ChrList[chr_ref].indolist ) return;

  fan = gs->ChrList[chr_ref].onwhichfan;
  //if ( mesh_fan_remove_renderlist( fan ) )
  {
    //gs->ChrList[chr_ref].lightspek_fp8 = Mesh[meshvrtstart[fan]].vrtl_fp8;
    dolist[numdolist] = chr_ref;
    gs->ChrList[chr_ref].indolist = btrue;
    numdolist++;


    // Do flashing
    if (( gs->all_frame & gs->ChrList[chr_ref].flashand ) == 0 && gs->ChrList[chr_ref].flashand != DONTFLASH )
    {
      flash_character( gs, chr_ref, 255 );
    }
    // Do blacking
    if (( gs->all_frame&SEEKURSEAND ) == 0 && gs->cl->seekurse && gs->ChrList[chr_ref].iskursed )
    {
      flash_character( gs, chr_ref, 0 );
    }
  }
  //else
  //{
  //  Uint16 model = gs->ChrList[chr_ref].model;
  //  assert( MAXMODEL != VALIDATE_MDL( model ) );

  //  // Double check for large/special objects
  //  if ( gs->CapList[model].alwaysdraw )
  //  {
  //    dolist[numdolist] = chr_ref;
  //    gs->ChrList[chr_ref].indolist = btrue;
  //    numdolist++;
  //  }
  //}

  // Add its weapons too
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    dolist_add( chr_get_holdingwhich(  gs->ChrList, MAXCHR,chr_ref, _slot ) );
  };

}

//--------------------------------------------------------------------------------------------
void dolist_sort( void )
{
  // ZZ> This function orders the dolist based on distance from camera,
  //     which is needed for reflections to properly clip themselves.
  //     Order from closest to farthest

  CGame * gs = gfxState.gs;

  CHR_REF chr_ref, olddolist[MAXCHR];
  int cnt, tnc, order;
  int dist[MAXCHR];

  // Figure the distance of each
  cnt = 0;
  while ( cnt < numdolist )
  {
    chr_ref = dolist[cnt];  olddolist[cnt] = chr_ref;
    if ( gs->ChrList[chr_ref].light_fp8 != 255 || gs->ChrList[chr_ref].alpha_fp8 != 255 )
    {
      // This makes stuff inside an invisible chr_ref visible...
      // A key inside a Jellcube, for example
      dist[cnt] = 0x7fffffff;
    }
    else
    {
      dist[cnt] = ABS( gs->ChrList[chr_ref].pos.x - GCamera.pos.x ) + ABS( gs->ChrList[chr_ref].pos.y - GCamera.pos.y );
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

  CGame * gs = gfxState.gs;

  int cnt;
  CHR_REF chr_ref;

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
  
  for ( cnt = 0; cnt < MAXCHR; cnt++)
  {
    if( !VALID_CHR(gs->ChrList, cnt) ) continue;

    if ( !chr_in_pack( gs->ChrList, MAXCHR, cnt ) )
    {
      dolist_add( cnt );
    }
  }

}
