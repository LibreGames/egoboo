//********************************************************************************************
//* Egoboo - game.c
//*
//* The main game loop and functions
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


#define DECLARE_GLOBALS

#include "game.h"
#include "Ui.h"
#include "Font.h"
#include "Clock.h"
#include "Log.h"
#include "Client.h"
#include "Server.h"
#include "System.h"
#include "id_md2.h"
#include "Menu.h"
#include "script.h"
#include "enchant.h"
#include "camera.h"
#include "sound.h"
#include "object.inl"
#include "network.inl"

#include "egoboo_utility.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include <SDL_endian.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "input.inl"
#include "particle.inl"
#include "Md2.inl"
#include "char.inl"
#include "graphic.inl"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

static MachineState _macState = { bfalse };

static MachineState * MachineState_new( MachineState * ms );
static bool_t         MachineState_delete( MachineState * ms );


static GameState * GameState_new(GameState * gs);
static bool_t      GameState_delete(GameState * gs);

static bool_t      GuiState_startUp();

//---------------------------------------------------------------------------------------------

#define RELEASE(x) if (x) {x->Release(); x=NULL;}

#define PROFILE_DECLARE(XX) ClockState * clkstate_##XX = NULL; double clkcount_##XX = 0.0; double clktime_##XX = 0.0;
#define PROFILE_INIT(XX)    { clkstate_##XX  = ClockState_create(#XX, -1); }
#define PROFILE_FREE(XX)    { ClockState_destroy(&(clkstate_##XX)); }
#define PROFILE_QUERY(XX)   ( (double)clktime_##XX / (double)clkcount_##XX )

#define PROFILE_BEGIN(XX) ClockState_frameStep(clkstate_##XX); {
#define PROFILE_END(XX)   ClockState_frameStep(clkstate_##XX);  clkcount_##XX = clkcount_##XX*0.9 + 0.1*1.0; clktime_##XX = clktime_##XX*0.9 + 0.1*ClockState_getFrameDuration(clkstate_##XX); }

//---------------------------------------------------------------------------------------------

char cActionName[MAXACTION][2];

MESSAGE     GMsg = {0,0};
NET_MESSAGE GNetMsg = {20,0,0,{'\0'}};
GuiState   _gui_state = { bfalse };

PROFILE_DECLARE( resize_characters );
PROFILE_DECLARE( keep_weapons_with_holders );
PROFILE_DECLARE( let_ai_think );
PROFILE_DECLARE( do_weather_spawn );
PROFILE_DECLARE( enc_spawn_particles );
PROFILE_DECLARE( ClientState_unbufferLatches );
PROFILE_DECLARE( sv_unbufferLatches );
PROFILE_DECLARE( check_respawn );
PROFILE_DECLARE( move_characters );
PROFILE_DECLARE( move_particles );
PROFILE_DECLARE( make_character_matrices );
PROFILE_DECLARE( attach_particles );
PROFILE_DECLARE( make_onwhichfan );
PROFILE_DECLARE( do_bumping );
PROFILE_DECLARE( stat_return );
PROFILE_DECLARE( pit_kill );
PROFILE_DECLARE( animate_tiles );
PROFILE_DECLARE( move_water );
PROFILE_DECLARE( figure_out_what_to_draw );
PROFILE_DECLARE( draw_main );

PROFILE_DECLARE( pre_update_game );
PROFILE_DECLARE( update_game );
PROFILE_DECLARE( main_loop );

// When operating as a server, there may be a need to host multiple games
// this "stack" holds all valid game states
typedef struct GSStack_t
{
  bool_t      initialized;
  int         count;
  GameState * data[256];
} GSStack;

static GSStack _gs_stack = { bfalse };

GSStack * GSStack_new(GSStack * stk);
bool_t GSStack_delete(GSStack * stk);
bool_t GSStack_push(GSStack * stk, GameState * gs);
GameState * GSStack_pop(GSStack * stk);
GameState * GSStack_get(GSStack * stk, int i);
GameState * GSStack_remove(GSStack * stk, int i);

GSStack * Get_GSStack()
{
  // BB > a function to get the global game state stack.
  //      Acts like a singleton, will initialize _gs_stack if not already initialized

  if(!_gs_stack.initialized)
  {
    GSStack_new(&_gs_stack);
  }

  return &_gs_stack; 
};

//---------------------------------------------------------------------------------------------
static void update_looped_sounds( GameState * gs );
static bool_t load_all_music_sounds(SoundState * ss, ConfigData * cd);

static void sdlinit( GraphicState * g );

static void update_game(GameState * gs, float dFrame, Uint32 * rand_idx);
static void memory_cleanUp(void);

GSStack * Get_GSStack();

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ACTION what_action( char cTmp )
{
  // ZZ> This function changes a letter into an action code

  ACTION action = ACTION_DA;

  switch ( toupper( cTmp ) )
  {
    case 'U':  action = ACTION_UA; break;
    case 'T':  action = ACTION_TA; break;
    case 'S':  action = ACTION_SA; break;
    case 'C':  action = ACTION_CA; break;
    case 'B':  action = ACTION_BA; break;
    case 'L':  action = ACTION_LA; break;
    case 'X':  action = ACTION_XA; break;
    case 'F':  action = ACTION_FA; break;
    case 'P':  action = ACTION_PA; break;
    case 'Z':  action = ACTION_ZA; break;
  };

  return action;
}

//------------------------------------------------------------------------------
void memory_cleanUp()
{
  // ZF> This function releases all loaded things in memory and cleans up everything properly

  GameState * gs;
  MachineState * mach_state;
  GSStack * stk = Get_GSStack();

  log_info("memory_cleanUp() - Attempting to clean up loaded things in memory... ");

  // kill all game states
  while(stk->count > 0)
  {
    gs = GSStack_pop(stk);
    if(NULL == gs) continue;

    gs->proc.KillMe = btrue;
    GameState_destroy(&gs);
  }

  // shut down all game systems
  ui_shutdown();			          //Shutdown various support systems
  GuiState_shutDown();
  mach_state = Get_MachineState();
  MachineState_delete( mach_state );
  sys_shutdown();

  log_message("Succeeded!\n");
  log_shutdown();

  snd_quit( &sndState );
}

//------------------------------------------------------------------------------
//Random Things-----------------------------------------------------------------
//------------------------------------------------------------------------------
void make_newloadname( char *modname, char *appendname, char *newloadname )
{
  // ZZ> This function takes some names and puts 'em together

  strcpy(newloadname, modname);
  strcat(newloadname, appendname);
}

//--------------------------------------------------------------------------------------------
//void module_load_all_waves( char *modname )
//{
//  // ZZ> This function loads the global waves
//
//  STRING tmploadname, newloadname;
//  int cnt;
//
//  if ( sndState.mixer_loaded )
//  {
//    // load in the sounds local to this module
//    snprintf( tmploadname, sizeof( tmploadname ), "%s%s" SLASH_STRING, modname, CData.gamedat_dir );
//    for ( cnt = 0; cnt < MAXWAVE; cnt++ )
//    {
//      snprintf( newloadname, sizeof( newloadname ), "%ssound%d.wav", tmploadname, cnt );
//      sndState.mc_list[cnt] = Mix_LoadWAV( newloadname );
//    };
//
//    //These sounds are always standard, but DO NOT override sounds that were loaded local to this module
//    if ( NULL == sndState.mc_list[GSOUND_COINGET] )
//    {
//      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinget_sound );
//      sndState.mc_list[GSOUND_COINGET] = Mix_LoadWAV( CStringTmp1 );
//    };
//
//    if ( NULL == sndState.mc_list[GSOUND_DEFEND] )
//    {
//      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_sound );
//      sndState.mc_list[GSOUND_DEFEND] = Mix_LoadWAV( CStringTmp1 );
//    }
//
//    if ( NULL == sndState.mc_list[GSOUND_COINFALL] )
//    {
//      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinfall_sound );
//      sndState.mc_list[GSOUND_COINFALL] = Mix_LoadWAV( CStringTmp1 );
//    };
//  }
//
//  /*  The Global Sounds
//  * 0 - Pick up coin
//  * 1 - Defend clank
//  * 2 - Weather Effect
//  * 3 - Hit Water tile (Splash)
//  * 4 - Coin falls on ground
//
//  //These new values todo should determine sound and particle effects (examples below)
//  Weather Type: DROPS, RAIN, SNOW, LAVABUBBLE (Which weather effect to spawn)
//  Water Type: LAVA, WATER, DARK (To determine sound and particle effects)
//
//  //We shold also add standard particles that can be used everywhere (Located and
//  //loaded in globalparticles folder) such as these below.
//  Particle Effect: REDBLOOD, SMOKE, HEALCLOUD
//  */
//}
//
//
//---------------------------------------------------------------------------------------------
void export_one_character( GameState * gs, CHR_REF character, Uint16 owner, int number )
{
  // ZZ> This function exports a character

  int tnc, profile;
  char fromdir[128];
  char todir[128];
  char fromfile[128];
  char tofile[128];
  char todirname[16];
  char todirfullname[64];

  // Don't export enchants
  disenchant_character( gs, character );

  profile = gs->ChrList[character].model;
  if (( gs->CapList[profile].cancarrytonextmodule || !gs->CapList[profile].isitem ) && gs->modstate.exportvalid )
  {
    STRING tmpname;
    // TWINK_BO.OBJ
    snprintf( todirname, sizeof( todirname ), "%s" SLASH_STRING, CData.players_dir );

    str_convert_spaces( todirname, sizeof( tmpname ), gs->ChrList[owner].name );
    strncat( todirname, ".obj", sizeof( tmpname ) );

    // Is it a character or an item?
    if ( owner != character )
    {
      // Item is a subdirectory of the owner directory...
      snprintf( todirfullname, sizeof( todirfullname ), "%s" SLASH_STRING "%d.obj", todirname, number );
    }
    else
    {
      // Character directory
      strncpy( todirfullname, todirname, sizeof( todirfullname ) );
    }


    // players/twink.obj or players/twink.obj/sword.obj
    snprintf( todir, sizeof( todir ), "%s" SLASH_STRING "%s", CData.players_dir, todirfullname );

    // modules/advent.mod/objects/advent.obj
    strncpy( fromdir, gs->MadList[profile].name, sizeof( fromdir ) );

    // Delete all the old items
    if ( owner == character )
    {
      tnc = 0;
      while ( tnc < 8 )
      {
        snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%d.obj", todir, tnc );   /*.OBJ*/
        fs_removeDirectoryAndContents( tofile );
        tnc++;
      }
    }


    // Make the directory
    fs_createDirectory( todir );


    // Build the DATA.TXT file
    snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.data_file );    /*DATA.TXT*/
    export_one_character_profile( gs, tofile, character );


    // Build the SKIN.TXT file
    snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.skin_file );    /*SKIN.TXT*/
    export_one_character_skin( gs, tofile, character );


    // Build the NAMING.TXT file
    snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.naming_file );    /*NAMING.TXT*/
    export_one_character_name( gs, tofile, character );


    // Copy all of the misc. data files
    snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.message_file );   /*MESSAGE.TXT*/
    snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.message_file );   /*MESSAGE.TXT*/
    fs_copyFile( fromfile, tofile );

    snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "tris.md2", fromdir );    /*TRIS.MD2*/
    snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "tris.md2", todir );    /*TRIS.MD2*/
    fs_copyFile( fromfile, tofile );

    snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.copy_file );    /*COPY.TXT*/
    snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.copy_file );    /*COPY.TXT*/
    fs_copyFile( fromfile, tofile );

    snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.script_file );
    snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.script_file );
    fs_copyFile( fromfile, tofile );

    snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.enchant_file );
    snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.enchant_file );
    fs_copyFile( fromfile, tofile );

    snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.credits_file );
    snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.credits_file );
    fs_copyFile( fromfile, tofile );


    snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.quest_file );
    snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.quest_file );
    fs_copyFile( fromfile, tofile );

    // Copy all of the particle files
    tnc = 0;
    while ( tnc < PRTPIP_PEROBJECT_COUNT )
    {
      snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "part%d.txt", fromdir, tnc );
      snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "part%d.txt", todir,   tnc );
      fs_copyFile( fromfile, tofile );
      tnc++;
    }


    // Copy all of the sound files

    for ( tnc = 0; tnc < MAXWAVE; tnc++ )
    {
      snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "sound%d.wav", fromdir, tnc );
      snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "sound%d.wav", todir,   tnc );
      fs_copyFile( fromfile, tofile );
    }


    // Copy all of the image files
    tnc = 0;
    while ( tnc < 4 )
    {
      snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "tris%d.bmp", fromdir, tnc );
      snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "tris%d.bmp", todir,   tnc );
      fs_copyFile( fromfile, tofile );

      snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "icon%d.bmp", fromdir, tnc );
      snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "icon%d.bmp", todir,   tnc );
      fs_copyFile( fromfile, tofile );
      tnc++;
    }
  }
}

//---------------------------------------------------------------------------------------------
void export_all_local_players( GameState * gs )
{
  // ZZ> This function saves all the local players in the
  //     PLAYERS directory

  int number;
  CHR_REF character, item;
  PLA_REF ipla;

  // Check each player
  if ( !gs->modstate.exportvalid ) return;

  for ( ipla = 0; ipla < MAXPLAYER; ipla++ )
  {
    if ( !VALID_PLA( gs->PlaList,  ipla ) || INBITS_NONE == gs->PlaList[ipla].device ) continue;

    // Is it alive?
    character = PlaList_get_character( gs, ipla );
    if ( !VALID_CHR( gs->ChrList, character ) || !gs->ChrList[character].alive ) continue;

    // Export the character
    export_one_character( gs, character, character, 0 );

    // Export all held items
    for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
    {
      item = chr_get_holdingwhich( gs->ChrList, MAXCHR, character, _slot );
      if ( VALID_CHR( gs->ChrList, item ) && gs->ChrList[item].isitem )
      {
        SLOT loc_slot = (_slot == SLOT_SADDLE ? _slot : SLOT_LEFT);
        export_one_character( gs, item, character, loc_slot );
      };
    }

    // Export the inventory
    number = 3;
    item  = chr_get_nextinpack( gs->ChrList, MAXCHR, character );
    while ( VALID_CHR( gs->ChrList, item ) )
    {
      if ( gs->ChrList[item].isitem ) export_one_character( gs, item, character, number );
      item  = chr_get_nextinpack( gs->ChrList, MAXCHR, item );
      number++;
    }

  }

}


//--------------------------------------------------------------------------------------------
void quit_game( GameState * gs )
{
  // ZZ> This function exits the game entirely

  if(!gs->proc.Active) return;

  log_info( "Exiting Egoboo %s the good way...\n", VERSION );

  module_quit(gs);

  gs->proc.Active = bfalse;
  gs->proc.Paused = bfalse;
}




//--------------------------------------------------------------------------------------------
int get_free_message(void)
{
  // This function finds the best message to use
  // Pick the first one
  int tnc = GMsg.start;
  GMsg.start++;
  GMsg.start = GMsg.start % CData.maxmessage;
  return tnc;
}

//--------------------------------------------------------------------------------------------
void display_message( GameState * gs, int message, CHR_REF chr_ref )
{
  // ZZ> This function sticks a message in the display queue and sets its timer

  int slot, read, write, cnt;
  char *eread;
  STRING szTmp;
  char cTmp, lTmp;
  
  ScriptInfo * slist = GameState_getScriptInfo(gs);

  CHR_REF target = chr_get_aitarget( gs->ChrList, MAXCHR, gs->ChrList + chr_ref );
  CHR_REF owner  = chr_get_aiowner( gs->ChrList, MAXCHR, gs->ChrList + chr_ref );

  Chr * pchr    = MAXCHR == chr_ref ? NULL : gs->ChrList + chr_ref;
  AI_STATE * pstate = &(pchr->aistate);
  Cap * pchr_cap = gs->CapList + pchr->model;

  Chr * ptarget  = MAXCHR == target  ? NULL : gs->ChrList + target;
  Cap * ptrg_cap = NULL   == ptarget ? NULL : gs->CapList + ptarget->model;

  Chr * powner   = MAXCHR == owner  ? NULL : gs->ChrList + owner;
  Cap * pown_cap = NULL   == powner ? NULL : gs->CapList + powner->model;


  if ( message >= GMsg.total ) return;

  slot = get_free_message();
  GMsg.list[slot].time = DELAY_MESSAGE;

  // Copy the message
  read = GMsg.index[message];
  cnt = 0;
  write = 0;
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
      cTmp = *eread;  eread++;
      while ( cTmp != 0 && write < MESSAGESIZE - 1 )
      {
        GMsg.list[slot].textdisplay[write] = cTmp;
        cTmp = *eread;  eread++;
        write++;
      }
    }
    else
    {
      // Copy the letter
      if ( write < MESSAGESIZE - 1 )
      {
        GMsg.list[slot].textdisplay[write] = cTmp;
        write++;
      }
    }
    cTmp = GMsg.text[read];  read++;
    cnt++;
  }
  GMsg.list[slot].textdisplay[write] = 0;

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void load_action_names( char* loadname )
{
  // ZZ> This function loads all of the 2 letter action names

  FILE* fileread;
  ACTION cnt;
  char first, second;

  log_info( "load_action_names() - loading all 2 letter action names from %s.\n", loadname );

  fileread = fs_fileOpen( PRI_WARN, NULL, loadname, "r" );
  if ( NULL == fileread )
  {
    log_warning( "Problems loading model action codes (%s)\n", loadname );
    return;
  }

  for ( cnt = ACTION_ST; cnt < MAXACTION; cnt = ( ACTION )( cnt + 1 ) )
  {
    fgoto_colon( fileread );
    fscanf( fileread, "%c%c", &first, &second );
    cActionName[cnt][0] = first;
    cActionName[cnt][1] = second;
  }
  fs_fileClose( fileread );

}




//--------------------------------------------------------------------------------------------
void read_setup( char* filename )
{
  // ZZ> This function loads the setup file

  ConfigFilePtr lConfigSetup;
  char lCurSectionName[64];
  bool_t lTempBool;
  Sint32 lTmpInt;
  char lTmpStr[24];

  lConfigSetup = OpenConfigFile( filename );
  if ( lConfigSetup == NULL )
  {
    //Major Error
    log_error( "Could not find setup file (%s).\n", filename );
    return;
  }


  globalname = filename; // heu!?

  /*********************************************

  GRAPHIC Section

  *********************************************/

  strcpy( lCurSectionName, "GRAPHIC" );

  //Draw z reflection?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "Z_REFLECTION", &CData.zreflect ) == 0 )
  {
    CData.zreflect = CData_default.zreflect;
  }

  //Max number of vertices (Should always be 100!)
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MAX_NUMBER_VERTICES", &lTmpInt ) == 0 )
  {
    CData.maxtotalmeshvertices = CData_default.maxtotalmeshvertices;
  }
  else
  {
    CData.maxtotalmeshvertices = lTmpInt * 1024;
  }

  //Do CData.fullscreen?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "FULLSCREEN", &CData.fullscreen ) == 0 )
  {
    CData.fullscreen = CData_default.fullscreen;
  }

  //Screen Size
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "SCREENSIZE_X", &lTmpInt ) == 0 )
  {
    CData.scrx = CData_default.scrx;
  }
  else
  {
    CData.scrx = lTmpInt;
  };

  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "SCREENSIZE_Y", &lTmpInt ) == 0 )
  {
    CData.scry = CData_default.scry;
  }
  else
  {
    CData.scry = lTmpInt;
  };

  //Color depth
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "COLOR_DEPTH", &lTmpInt ) == 0 )
  {
    CData.scrd = CData_default.scrd;
  }
  else
  {
    CData.scrd = lTmpInt;
  };

  //The z depth
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "Z_DEPTH", &lTmpInt ) == 0 )
  {
    CData.scrz = CData_default.scrz;
  }
  else
  {
    CData.scrz = lTmpInt;
  };

  //Max number of messages displayed
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MAX_TEXT_MESSAGE", &lTmpInt ) == 0 )
  {
    CData.messageon  = CData_default.messageon;
    CData.maxmessage = CData_default.maxmessage;
  }
  else
  {
    CData.messageon = btrue;
    CData.maxmessage = lTmpInt;
  };

  if ( CData.maxmessage < 1 )  { CData.maxmessage = 1;  CData.messageon = bfalse; }
  if ( CData.maxmessage > MAXMESSAGE )  { CData.maxmessage = MAXMESSAGE; }

  //Show status bars? (Life, mana, character icons, etc.)
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "STATUS_BAR", &CData.staton ) == 0 )
  {
    CData.staton = CData_default.staton;
  }

  CData.wraptolerance = 32;

  if ( CData.staton )
  {
    CData.wraptolerance = 90;
  }

  //Perspective correction
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "PERSPECTIVE_CORRECT", &lTempBool ) == 0 )
  {
    CData.perspective = CData_default.perspective;
  }
  else
  {
    CData.perspective = lTempBool ? GL_NICEST : GL_FASTEST;
  };

  //Enable dithering?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "DITHERING", &CData.dither ) == 0 )
  {
    CData.dither = CData_default.dither;
  }

  //Reflection fadeout
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "FLOOR_REFLECTION_FADEOUT", &lTempBool ) == 0 )
  {
    CData.reffadeor = CData_default.reffadeor;
  }
  else
  {
    CData.reffadeor = (lTempBool ? 0 : 255);
  };


  //Draw Reflection?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "REFLECTION", &CData.refon ) == 0 )
  {
    CData.refon = CData_default.refon;
  }

  //Draw shadows?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "SHADOWS", &CData.shaon ) == 0 )
  {
    CData.shaon = CData_default.shaon;
  }

  //Draw good shadows
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "SHADOW_AS_SPRITE", &CData.shasprite ) == 0 )
  {
    CData.shasprite = CData_default.shasprite;
  }

  //Draw phong mapping?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "PHONG", &CData.phongon ) == 0 )
  {
    CData.phongon = CData_default.phongon;
  }

  //Draw water with more layers?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "MULTI_LAYER_WATER", &CData.twolayerwateron ) == 0 )
  {
    CData.twolayerwateron = CData_default.twolayerwateron;
  }

  //TODO: This is not implemented
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "OVERLAY", &CData.overlayvalid ) == 0 )
  {
    CData.overlayvalid = CData_default.overlayvalid;
  }

  //Allow backgrounds?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "BACKGROUND", &CData.backgroundvalid ) == 0 )
  {
    CData.backgroundvalid = CData_default.backgroundvalid;
  }

  //Enable fog?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "FOG", &CData.fogallowed ) == 0 )
  {
    CData.fogallowed = CData_default.fogallowed;
  }

  //Do gourad CData.shading?
  CData.shading = CData_default.shading;
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "GOURAUD_SHADING", &lTempBool ) != 0 )
  {
    CData.shading = lTempBool ? GL_SMOOTH : GL_FLAT;
  }

  //Enable CData.antialiasing?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "ANTIALIASING", &CData.antialiasing ) == 0 )
  {
    CData.antialiasing = CData_default.antialiasing;
  }

  //Do we do texture filtering?

  if ( GetConfigValue( lConfigSetup, lCurSectionName, "TEXTURE_FILTERING", lTmpStr, 24 ) == 0 )
  {
    CData.texturefilter = 1;
  }
  else if ( isdigit( lTmpStr[0] ) )
  {
    sscanf( lTmpStr, "%d", &CData.texturefilter );
    if ( CData.texturefilter >= TX_ANISOTROPIC )
    {
      int tmplevel = CData.texturefilter - TX_ANISOTROPIC + 1;
      gfxState.userAnisotropy = 1 << tmplevel;
    }
  }
  else if ( toupper(lTmpStr[0]) == 'L')  CData.texturefilter = TX_LINEAR;
  else if ( toupper(lTmpStr[0]) == 'B')  CData.texturefilter = TX_BILINEAR;
  else if ( toupper(lTmpStr[0]) == 'T')  CData.texturefilter = TX_TRILINEAR_2;
  else if ( toupper(lTmpStr[0]) == 'A')  CData.texturefilter = TX_ANISOTROPIC + gfxState.log2Anisotropy;

  if ( GetConfigValue( lConfigSetup, lCurSectionName, "PARTICLE_EFFECTS", lTmpStr, 24 ) == 0 )
  {
    CData.particletype = PART_NORMAL; //Default
  }
  else if ( toupper(lTmpStr[0]) == 'N' )  CData.particletype = PART_NORMAL;
  else if ( toupper(lTmpStr[0]) == 'S' )  CData.particletype = PART_SMOOTH;
  else if ( toupper(lTmpStr[0]) == 'F' )  CData.particletype = PART_FAST;

  //Do vertical sync?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "VERTICAL_SYNC", &CData.backgroundvalid ) == 0 )
  {
    CData.vsync = CData_default.vsync;
  }

  //Force openGL hardware acceleration
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "FORCE_GFX_ACCEL", &CData.backgroundvalid ) == 0 )
  {
    CData.gfxacceleration = CData_default.gfxacceleration;
  }

  /*********************************************

  SOUND Section

  *********************************************/

  strcpy( lCurSectionName, "SOUND" );

  //Enable sound
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "SOUND", &CData.allow_sound ) == 0 )
  {
    CData.allow_sound = CData.allow_sound;
  }

  //Enable music
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "MUSIC", &CData.allow_music ) == 0 )
  {
    CData.allow_music = CData_default.allow_music;
  }

  //Music volume
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MUSIC_VOLUME", &CData.musicvolume ) == 0 )
  {
    CData.musicvolume = CData_default.musicvolume;
  }

  //Sound volume
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "SOUND_VOLUME", &CData.soundvolume ) == 0 )
  {
    CData.soundvolume = CData_default.soundvolume;
  }

  //Max number of sound channels playing at the same time
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "MAX_SOUND_CHANNEL", &CData.maxsoundchannel ) == 0 )
  {
    CData.maxsoundchannel = CData_default.maxsoundchannel;
  }
  CData.maxsoundchannel = CLIP(CData.maxsoundchannel, 8, 128);

  //The output buffer size
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "OUPUT_BUFFER_SIZE", &CData.buffersize ) == 0 )
  {
    CData.buffersize = CData_default.buffersize;
  }
  if ( CData.buffersize < 512 ) CData.buffersize = 512;
  if ( CData.buffersize > 8196 ) CData.buffersize = 8196;


  /*********************************************

  CONTROL Section

  *********************************************/

  strcpy( lCurSectionName, "CONTROL" );

  //Camera control mode
  if ( GetConfigValue( lConfigSetup, lCurSectionName, "AUTOTURN_CAMERA", lTmpStr, 24 ) == 0 )
  {
    CData.autoturncamera = CData_default.autoturncamera;
  }
  else
  {
    switch( toupper(lTmpStr[0]) )
    {
      case 'G': CData.autoturncamera = 255;    break;
      case 'T': CData.autoturncamera = btrue;  break;

      default:
      case 'F': CData.autoturncamera = bfalse; break;
    }
  }


  /*********************************************

  NETWORK Section

  *********************************************/

  strcpy( lCurSectionName, "NETWORK" );

  //Enable GNet.working systems?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "NETWORK_ON", &CData.request_network ) == 0 )
  {
    CData.request_network = CData_default.request_network;
  }

  //Max CData.lag
  if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "LAG_TOLERANCE", &lTmpInt ) == 0 )
  {
    CData.lag = CData_default.lag;
  }
  else
  {
    CData.lag = lTmpInt;
  };

  //Name or IP of the host(s) to join
  memset( CData.net_hosts, 0, MAXNETPLAYER*sizeof(STRING) );
  if ( GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME", CData.net_hosts[0], sizeof(STRING) ) == 0 )
  {
    strcpy( CData.net_hosts[0], CData_default.net_hosts[0] );
  }
  GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME_2", CData.net_hosts[1], sizeof(STRING) );
  GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME_3", CData.net_hosts[2], sizeof(STRING) );
  GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME_4", CData.net_hosts[3], sizeof(STRING) );
  GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME_5", CData.net_hosts[4], sizeof(STRING) );
  GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME_6", CData.net_hosts[5], sizeof(STRING) );
  GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME_7", CData.net_hosts[6], sizeof(STRING) );
  GetConfigValue( lConfigSetup, lCurSectionName, "HOST_NAME_8", CData.net_hosts[7], sizeof(STRING) );

  //Multiplayer name
  if ( GetConfigValue( lConfigSetup, lCurSectionName, "MULTIPLAYER_NAME", CData.net_messagename, sizeof(STRING) ) == 0 )
  {
    strcpy( CData.net_messagename, CData_default.net_messagename );
  }


  /*********************************************

  DEBUG Section

  *********************************************/

  strcpy( lCurSectionName, "DEBUG" );

  //Show the FPS counter?
  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "DISPLAY_FPS", &lTempBool ) == 0 )
  {
    CData.fpson = CData_default.fpson;
  }
  else
  {
    CData.fpson = lTempBool;
  };

  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "HIDE_MOUSE", &CData.HideMouse ) == 0 )
  {
    CData.HideMouse = CData_default.HideMouse;
  }

  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "GRAB_MOUSE", &lTempBool ) == 0 )
  {
    CData.GrabMouse = CData_default.GrabMouse;
  }
  else
  {
    CData.GrabMouse = lTempBool ? SDL_GRAB_ON : SDL_GRAB_OFF;
  };

  if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "DEVELOPER_MODE", &CData.DevMode ) == 0 )
  {
    CData.DevMode = CData_default.DevMode;
  }

  CloseConfigFile( lConfigSetup );

}


//---------------------------------------------------------------------------------------------
void make_lightdirectionlookup()
{
  // ZZ> This function builds the lighting direction table
  //     The table is used to find which direction the light is coming
  //     from, based on the four corner vertices of a mesh tile.

  Uint32 cnt;
  Uint16 tl, tr, br, bl;
  int x, y;

  for ( cnt = 0; cnt < UINT16_SIZE; cnt++ )
  {
    tl = ( cnt & 0xf000 ) >> 12;
    tr = ( cnt & 0x0f00 ) >> 8;
    br = ( cnt & 0x00f0 ) >> 4;
    bl = ( cnt & 0x000f );
    x = br + tr - bl - tl;
    y = br + bl - tl - tr;
    lightdirectionlookup[cnt] = ( atan2( -y, x ) + PI ) * RAD_TO_BYTE;
  }
}

//--------------------------------------------------------------------------------------------
void make_enviro( void )
{
  // ZZ> This function sets up the environment mapping table

  int cnt;
  float z;
  float x, y;

  // Find the environment map positions
  for ( cnt = 0; cnt < MD2LIGHTINDICES; cnt++ )
  {
    x = -kMd2Normals[cnt][0];
    y = kMd2Normals[cnt][1];
    x = 1.0f + atan2( y, x ) / PI;
    x--;

    if ( x < 0 )
      x--;

    indextoenvirox[cnt] = x;
  }

  for ( cnt = 0; cnt < 256; cnt++ )
  {
    z = FP8_TO_FLOAT( cnt );   // Z is between 0 and 1
    lighttoenviroy[cnt] = z;
  }
}

//--------------------------------------------------------------------------------------------
void print_status( GameState * gs, Uint16 statindex )
{
  // ZZ> This function shows the more specific stats for a character

  CHR_REF character;
  char gender[8];
  Status * lst = gs->al_cs->StatList;
  size_t   lst_size = MAXSTAT;

  if ( lst[statindex].delay == 0 )
  {
    if ( statindex < lst_size )
    {
      character = lst[statindex].chr_ref;

      // Name
      debug_message( 1, "=%s=", gs->ChrList[character].name );

      // Level and gender and class
      gender[0] = '\0';
      if ( gs->ChrList[character].alive )
      {
        switch ( gs->ChrList[character].gender )
        {
          case GEN_MALE: snprintf( gender, sizeof( gender ), "Male" ); break;
          case GEN_FEMALE: snprintf( gender, sizeof( gender ), "Female" ); break;
        };

        debug_message( 1, " %s %s", gender, gs->CapList[gs->ChrList[character].model].classname );
      }
      else
      {
        debug_message( 1, " Dead %s", gs->CapList[gs->ChrList[character].model].classname );
      }

      // Stats
      debug_message( 1, " STR:%2.1f ~WIS:%2.1f ~INT:%2.1f", FP8_TO_FLOAT( gs->ChrList[character].strength_fp8 ), FP8_TO_FLOAT( gs->ChrList[character].wisdom_fp8 ), FP8_TO_FLOAT( gs->ChrList[character].intelligence_fp8 ) );
      debug_message( 1, " DEX:%2.1f ~LVL:%4.1f ~DEF:%2.1f", FP8_TO_FLOAT( gs->ChrList[character].dexterity_fp8 ), calc_chr_level( gs, character ), FP8_TO_FLOAT( gs->ChrList[character].skin.defense_fp8 ) );

      lst[statindex].delay = 10;
    }
  }
}


//--------------------------------------------------------------------------------------------
void draw_chr_info( GameState * gs )
{
  // ZZ> This function lets the players check character stats

  int cnt;
  Status * lst      = gs->al_cs->StatList;
  size_t   lst_size = gs->al_cs->StatList_count;

  if ( !Get_GuiState()->net_messagemode )
  {
    for(cnt=0; cnt<8; cnt++)
    {
      if ( SDLKEYDOWN( SDLK_1 + cnt ) && cnt<lst_size )  print_status( gs, cnt );
    }

    // Debug cheat codes (Gives xp to stat characters)
    if ( CData.DevMode && SDLKEYDOWN( SDLK_x ) )
    {
      for(cnt=0; cnt<8; cnt++)
      {
        if ( SDLKEYDOWN( SDLK_1 + cnt ) && cnt<lst_size && VALID_CHR( gs->ChrList, gs->PlaList[0].chr_ref ) )
        {
          give_experience( gs, gs->PlaList[cnt].chr_ref, 25, XP_DIRECT ); 
          lst[cnt].delay = 0;
        }
      }
    }

    //CTRL+M - show or hide map
    if ( CData.DevMode && SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN (SDLK_LCTRL ) )
    {
      mapon        = !mapon;
      youarehereon = mapon;
    }
  }
}

bool_t do_screenshot()
{
  // dumps the current screen (GL context) to a new bitmap file
  // right now it dumps it to whatever the current directory is

  // returns btrue if successful, bfalse otherwise

  SDL_Surface *screen, *temp;
  Uint8 *pixels;
  STRING buff;
  int i;
  FILE *test;

  screen = SDL_GetVideoSurface();
  temp   = SDL_CreateRGBSurface( SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                               0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
                               0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
                             );


  if ( temp == NULL )
    return bfalse;

  pixels = (Uint8*)calloc( 3 * screen->w * screen->h, sizeof(Uint8) );
  if ( pixels == NULL )
  {
    SDL_FreeSurface( temp );
    return bfalse;
  }

  glReadPixels( 0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels );

  for ( i = 0; i < screen->h; i++ )
  {
    memcpy((( char * ) temp->pixels ) + temp->pitch * i, pixels + 3 * screen->w * ( screen->h - i - 1 ), screen->w * 3 );
  }
  FREE( pixels );

  // find the next EGO??.BMP file for writing
  i = 0;
  test = NULL;
  do
  {
    if ( test != NULL )
      fs_fileClose( test );

    snprintf( buff, sizeof( buff ), "ego%02d.bmp", i );

    // lame way of checking if the file already exists...
    test = fs_fileOpen( PRI_NONE, NULL, buff, "rb" );
    i++;
  }
  while (( test != NULL ) && ( i < 100 ) );

  SDL_SaveBMP( temp, buff );
  SDL_FreeSurface( temp );

  debug_message( 1, "Saved to %s", buff );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t add_status( GameState * gs, CHR_REF character )
{
  // ZZ> This function adds a status display to the do list

  bool_t was_added;
  int old_size = gs->al_cs->StatList_count;
  int new_size;

  // try to add the character
  new_size = StatList_add( gs->al_cs->StatList, gs->al_cs->StatList_count, character );
  was_added = (old_size != new_size);

  gs->ChrList[character].staton = was_added;
  gs->al_cs->StatList_count        = new_size;

  return was_added;
}



//--------------------------------------------------------------------------------------------
bool_t remove_stat( GameState * gs, Chr * pchr )
{
  int i, icount;
  bool_t bfound;

  Status * statlst      = gs->al_cs->StatList;
  size_t   statlst_size = gs->al_cs->StatList_count;

  if(NULL == pchr) return bfalse;

  pchr->staton = bfalse;
  bfound       = bfalse;
  for ( i=0;  i < statlst_size; i++ )
  {
    if ( (gs->ChrList + statlst[i].chr_ref) != pchr ) continue;

    icount = statlst_size-i-1;
    if(icount > 0)
    {
      // move the data block to fill in the hole
      memmove(statlst + i, statlst + i + 1, icount*sizeof(Status));
    }

    // turn off the bad stat at the end of the list
    if(statlst_size<MAXSTAT)
    {
      statlst[statlst_size].on = bfalse;
    }

    // correct the statlst_size
    statlst_size--;

    break;
  }

  gs->al_cs->StatList_count = statlst_size;

  return btrue;
}

//--------------------------------------------------------------------------------------------
void sort_statlist( GameState * gs )
{
  // ZZ> This function puts all of the local players on top of the statlst

  int cnt;

  Status * statlst      = gs->al_cs->StatList;
  size_t   statlst_size = gs->al_cs->StatList_count;

  for ( cnt = 0; cnt < gs->PlaList_count; cnt++ )
  {
    if ( !VALID_PLA( gs->PlaList,  cnt ) || INBITS_NONE == gs->PlaList[cnt].device ) continue;

    StatList_move_to_top( statlst, statlst_size, gs->PlaList[cnt].chr_ref );
  }
}

//--------------------------------------------------------------------------------------------
void move_water( float dUpdate )
{
  // ZZ> This function animates the water overlays

  int layer;
  GameState * gs = gfxState.gs;

  for ( layer = 0; layer < MAXWATERLAYER; layer++ )
  {
    gs->water.layer[layer].u += gs->water.layer[layer].uadd * dUpdate;
    gs->water.layer[layer].v += gs->water.layer[layer].vadd * dUpdate;
    if ( gs->water.layer[layer].u > 1.0 )  gs->water.layer[layer].u -= 1.0;
    if ( gs->water.layer[layer].v > 1.0 )  gs->water.layer[layer].v -= 1.0;
    if ( gs->water.layer[layer].u < -1.0 )  gs->water.layer[layer].u += 1.0;
    if ( gs->water.layer[layer].v < -1.0 )  gs->water.layer[layer].v += 1.0;
    gs->water.layer[layer].frame = (( int )( gs->water.layer[layer].frame + gs->water.layer[layer].frameadd * dUpdate ) ) & WATERFRAMEAND;
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF search_best_leader( GameState * gs, TEAM team, CHR_REF exclude )
{
  // BB > find the best (most experienced) character other than the sissy to be a team leader
  CHR_REF cnt;
  Uint16  best_leader = MAXCHR;
  int     best_experience = 0;
  bool_t  bfound = bfalse;
  Uint16  exclude_sissy = team_get_sissy( gs, team );

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( gs->ChrList, cnt ) || cnt == exclude || cnt == exclude_sissy || gs->ChrList[cnt].team != team ) continue;

    if ( !bfound || gs->ChrList[cnt].experience > best_experience )
    {
      best_leader     = cnt;
      best_experience = gs->ChrList[cnt].experience;
      bfound = btrue;
    }
  }

  return bfound ? best_leader : MAXCHR;
}

//--------------------------------------------------------------------------------------------
void call_for_help( GameState * gs, CHR_REF character )
{
  // ZZ> This function issues a call for help to all allies

  TEAM team;
  Uint16 cnt;

  team = gs->ChrList[character].team;


  // make the character in who needs help the sissy
  gs->TeamList[team].leader = search_best_leader( gs, team, character );
  gs->TeamList[team].sissy  = character;


  // send the help message
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( gs->ChrList, cnt ) || cnt == character ) continue;
    if ( !gs->TeamList[gs->ChrList[cnt].baseteam].hatesteam[team] )
    {
      gs->ChrList[cnt].aistate.alert |= ALERT_CALLEDFORHELP;
    };
  }

  // TODO: make a yelping sound

}

//--------------------------------------------------------------------------------------------
void give_experience( GameState * gs, CHR_REF character, int amount, EXPERIENCE xptype )
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function

  int newamount;
  int curlevel, nextexperience;
  int number;
  int profile;


  // Figure out how much experience to give
  profile = gs->ChrList[character].model;
  newamount = amount;
  if ( xptype < XP_COUNT )
  {
    newamount = amount * gs->CapList[profile].experiencerate[xptype];
  }
  newamount += gs->ChrList[character].experience;
  if ( newamount > MAXXP )  newamount = MAXXP;
  gs->ChrList[character].experience = newamount;


  // Do level ups and stat changes
  curlevel       = gs->ChrList[character].experiencelevel;
  nextexperience = gs->CapList[profile].experiencecoeff * ( curlevel + 1 ) * ( curlevel + 1 ) + gs->CapList[profile].experienceconst;
  while ( gs->ChrList[character].experience >= nextexperience )
  {
    // The character is ready to advance...
    if ( chr_is_player(gs, character) )
    {
      debug_message( 1, "%s gained a level!!!", gs->ChrList[character].name );
      snd_play_sound(gs, 1.0f, gs->ChrList[character].pos, sndState.mc_list[GSOUND_LEVELUP], 0, character, GSOUND_LEVELUP);
    }
    gs->ChrList[character].experiencelevel++;

    // Size
    if (( gs->ChrList[character].sizegoto + gs->CapList[profile].sizeperlevel ) < 1 + ( gs->CapList[profile].sizeperlevel*10 ) ) gs->ChrList[character].sizegoto += gs->CapList[profile].sizeperlevel;
    gs->ChrList[character].sizegototime += DELAY_RESIZE * 100;

    // Strength
    number = generate_unsigned( &gs->CapList[profile].strengthperlevel_fp8 );
    number += gs->ChrList[character].strength_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    gs->ChrList[character].strength_fp8 = number;

    // Wisdom
    number = generate_unsigned( &gs->CapList[profile].wisdomperlevel_fp8 );
    number += gs->ChrList[character].wisdom_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    gs->ChrList[character].wisdom_fp8 = number;

    // Intelligence
    number = generate_unsigned( &gs->CapList[profile].intelligenceperlevel_fp8 );
    number += gs->ChrList[character].intelligence_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    gs->ChrList[character].intelligence_fp8 = number;

    // Dexterity
    number = generate_unsigned( &gs->CapList[profile].dexterityperlevel_fp8 );
    number += gs->ChrList[character].dexterity_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    gs->ChrList[character].dexterity_fp8 = number;

    // Life
    number = generate_unsigned( &gs->CapList[profile].lifeperlevel_fp8 );
    number += gs->ChrList[character].lifemax_fp8;
    if ( number > PERFECTBIG ) number = PERFECTBIG;
    gs->ChrList[character].life_fp8 += ( number - gs->ChrList[character].lifemax_fp8 );
    gs->ChrList[character].lifemax_fp8 = number;

    // Mana
    number = generate_unsigned( &gs->CapList[profile].manaperlevel_fp8 );
    number += gs->ChrList[character].manamax_fp8;
    if ( number > PERFECTBIG ) number = PERFECTBIG;
    gs->ChrList[character].mana_fp8 += ( number - gs->ChrList[character].manamax_fp8 );
    gs->ChrList[character].manamax_fp8 = number;

    // Mana Return
    number = generate_unsigned( &gs->CapList[profile].manareturnperlevel_fp8 );
    number += gs->ChrList[character].manareturn_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    gs->ChrList[character].manareturn_fp8 = number;

    // Mana Flow
    number = generate_unsigned( &gs->CapList[profile].manaflowperlevel_fp8 );
    number += gs->ChrList[character].manaflow_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    gs->ChrList[character].manaflow_fp8 = number;

    curlevel       = gs->ChrList[character].experiencelevel;
    nextexperience = gs->CapList[profile].experiencecoeff * ( curlevel + 1 ) * ( curlevel + 1 ) + gs->CapList[profile].experienceconst;
  }
}


//--------------------------------------------------------------------------------------------
void give_team_experience( GameState * gs, TEAM team, int amount, EXPERIENCE xptype )
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function

  int cnt;

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( gs->ChrList[cnt].team == team && gs->ChrList[cnt].on )
    {
      give_experience( gs, cnt, amount, xptype );
    }
  }
}


//--------------------------------------------------------------------------------------------
void setup_alliances( GameState * gs, char *modname )
{
  // ZZ> This function reads the alliance file

  STRING newloadname, szTemp;
  TEAM teama, teamb;
  FILE *fileread;

  // Load the file
  snprintf( newloadname, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.alliance_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL != fileread )
  {
    while ( fget_next_string( fileread, szTemp, sizeof( szTemp ) ) )
    {
      teama = ( szTemp[0] - 'A' ) % TEAM_COUNT;

      fget_string( fileread, szTemp, sizeof( szTemp ) );
      teamb = ( szTemp[0] - 'A' ) % TEAM_COUNT;

      gs->TeamList[teama].hatesteam[teamb] = bfalse;
    }
    fs_fileClose( fileread );
  }
}

//grfx.c

//--------------------------------------------------------------------------------------------
void check_respawn( GameState * gs )
{
  int cnt;

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    // Let players respawn
    if ( gs->modstate.respawnvalid && !gs->ChrList[cnt].alive && HAS_SOME_BITS( gs->ChrList[cnt].aistate.latch.b, LATCHBUTTON_RESPAWN ) )
    {
      respawn_character( gs, cnt );
      gs->TeamList[gs->ChrList[cnt].team].leader = cnt;
      gs->ChrList[cnt].aistate.alert |= ALERT_CLEANEDUP;

      // Cost some experience for doing this...
      gs->ChrList[cnt].experience *= EXPKEEP;
    }
    gs->ChrList[cnt].aistate.latch.b &= ~LATCHBUTTON_RESPAWN;
  }

};



//--------------------------------------------------------------------------------------------
void begin_integration( GameState * gs )
{
  int cnt;

  for( cnt=0; cnt<MAXCHR; cnt++)
  {
    if( !gs->ChrList[cnt].on ) continue;

    VectorClear( gs->ChrList[cnt].accum_acc.v );
    VectorClear( gs->ChrList[cnt].accum_vel.v );
    VectorClear( gs->ChrList[cnt].accum_pos.v );
  };

  for( cnt=0; cnt<MAXPRT; cnt++)
  {
    if( !gs->PrtList[cnt].on ) continue;

    VectorClear( gs->PrtList[cnt].accum_acc.v );
    VectorClear( gs->PrtList[cnt].accum_vel.v );
    VectorClear( gs->PrtList[cnt].accum_pos.v );
    prt_calculate_bumpers(gs, cnt);
  };

};

//--------------------------------------------------------------------------------------------
bool_t chr_collide_mesh(GameState * gs, CHR_REF ichr)
{
  float meshlevel;
  vect3 norm;
  bool_t hitmesh = bfalse;
  Chr * pchr;

  if( !VALID_CHR( gs->ChrList, ichr ) ) return hitmesh;

  pchr = gs->ChrList + ichr;

  if ( 0 != chr_hitawall( gs, gs->ChrList + ichr, &norm ) )
  {
    float dotprod = DotProduct(norm, pchr->vel);

    if(dotprod < 0.0f)
    {
      // do the reflection
      pchr->accum_vel.x += -(1.0f + pchr->dampen) * dotprod * norm.x - pchr->vel.x;
      pchr->accum_vel.y += -(1.0f + pchr->dampen) * dotprod * norm.y - pchr->vel.y;
      pchr->accum_vel.z += -(1.0f + pchr->dampen) * dotprod * norm.z - pchr->vel.z;

      // if there is reflection, go back to the last valid position
      if( 0.0f != norm.x ) pchr->accum_pos.x += pchr->pos_old.x - pchr->pos.x;
      if( 0.0f != norm.y ) pchr->accum_pos.y += pchr->pos_old.y - pchr->pos.y;
      if( 0.0f != norm.z ) pchr->accum_pos.z += pchr->pos_old.z - pchr->pos.z;

      hitmesh = btrue;
    };
  }

  meshlevel = mesh_get_level( gs, pchr->onwhichfan, pchr->pos.x, pchr->pos.y, gs->CapList[pchr->model].waterwalk );
  if( pchr->pos.z < meshlevel )
  {
    hitmesh = btrue;

    pchr->accum_pos.z += meshlevel + 0.001f - pchr->pos.z;

    if ( pchr->vel.z < -STOPBOUNCING )
    {
      pchr->accum_vel.z += -pchr->vel.z * ( 1.0f + pchr->dampen );
    }
    else if ( pchr->vel.z < STOPBOUNCING )
    {
      pchr->accum_vel.z += -pchr->vel.z;
    }

    if ( pchr->hitready )
    {
      pchr->aistate.alert |= ALERT_HITGROUND;
      pchr->hitready = bfalse;
    };
  }

  return hitmesh;
};

//--------------------------------------------------------------------------------------------
bool_t prt_collide_mesh(GameState * gs, PRT_REF iprt)
{
  float meshlevel, dampen;
  CHR_REF attached;
  vect3 norm;
  bool_t hitmesh = bfalse;
  Uint16 pip;

  if( !VALID_PRT( gs->PrtList, iprt) ) return hitmesh;


  attached = prt_get_attachedtochr(gs, iprt);
  pip      = gs->PrtList[iprt].pip;
  dampen   = gs->PipList[pip].dampen;

  if ( 0 != prt_hitawall( gs, iprt, &norm ) )
  {
    float dotprod = DotProduct(norm, gs->PrtList[iprt].vel);

    if(dotprod < 0.0f)
    {
      snd_play_particle_sound( gs, gs->PrtList[iprt].bumpstrength*MIN( 1.0f, gs->PrtList[iprt].vel.x / 10.0f ), iprt, gs->PipList[pip].soundwall );

      if ( gs->PipList[pip].endwall )
      {
        gs->PrtList[iprt].gopoof = btrue;
      }
      else if( !gs->PipList[pip].rotatetoface && !VALID_CHR(gs->ChrList, attached) )  // "rotate to face" gets it's own treatment
      {
        vect3 old_vel;
        float dotprodN;
        float dampen = gs->PipList[gs->PrtList[iprt].pip].dampen;

        old_vel = gs->PrtList[iprt].vel;

        // do the reflection
        dotprodN = DotProduct(norm, gs->PrtList[iprt].vel);
        gs->PrtList[iprt].accum_vel.x += -(1.0f + dampen) * dotprodN * norm.x - gs->PrtList[iprt].vel.x;
        gs->PrtList[iprt].accum_vel.y += -(1.0f + dampen) * dotprodN * norm.y - gs->PrtList[iprt].vel.y;
        gs->PrtList[iprt].accum_vel.z += -(1.0f + dampen) * dotprodN * norm.z - gs->PrtList[iprt].vel.z;

        // Change facing
        // determine how much the billboarded particle should rotate on reflection from
        // the rotation of the velocity vector about the vector to the camera
        {
          vect3 vec_out, vec_up, vec_right, wld_up;
          float old_vx, old_vy, new_vx, new_vy;

          Uint16 new_turn, old_turn;

          vec_out.x = gs->PrtList[iprt].pos.x - GCamera.pos.x;
          vec_out.y = gs->PrtList[iprt].pos.y - GCamera.pos.y;
          vec_out.z = gs->PrtList[iprt].pos.z - GCamera.pos.z;

          wld_up.x = 0;
          wld_up.y = 0;
          wld_up.z = 1;

          vec_right = Normalize( CrossProduct( wld_up, vec_out ) );
          vec_up    = Normalize( CrossProduct( vec_right, vec_out ) );

          old_vx = DotProduct(old_vel, vec_right);
          old_vy = DotProduct(old_vel, vec_up);
          old_turn = vec_to_turn(old_vx, old_vy);

          new_vx = DotProduct(gs->PrtList[iprt].vel, vec_right);
          new_vy = DotProduct(gs->PrtList[iprt].vel, vec_up);
          new_turn = vec_to_turn(new_vx, new_vy);

          gs->PrtList[iprt].facing += new_turn - old_turn;
        }
      }

      // if there is reflection, go back to the last valid position
      if( 0.0f != norm.x ) gs->PrtList[iprt].accum_pos.x += gs->PrtList[iprt].pos_old.x - gs->PrtList[iprt].pos.x;
      if( 0.0f != norm.y ) gs->PrtList[iprt].accum_pos.y += gs->PrtList[iprt].pos_old.y - gs->PrtList[iprt].pos.y;
      if( 0.0f != norm.z ) gs->PrtList[iprt].accum_pos.z += gs->PrtList[iprt].pos_old.z - gs->PrtList[iprt].pos.z;

      hitmesh = btrue;
    };
  }

  meshlevel = mesh_get_level( gs, gs->PrtList[iprt].onwhichfan, gs->PrtList[iprt].pos.x, gs->PrtList[iprt].pos.y, gs->CapList[gs->PrtList[iprt].model].waterwalk );
  if( gs->PrtList[iprt].pos.z < meshlevel )
  {
    hitmesh = btrue;

    if(gs->PrtList[iprt].vel.z < 0.0f)
    {
      snd_play_particle_sound( gs, MIN( 1.0f, -gs->PrtList[iprt].vel.z / 10.0f ), iprt, gs->PipList[pip].soundfloor );
    };

    if( gs->PipList[pip].endground )
    {
      gs->PrtList[iprt].gopoof = btrue;
    }
    else if( !VALID_CHR( gs->ChrList, attached ) )
    {
      gs->PrtList[iprt].accum_pos.z += meshlevel + 0.001f - gs->PrtList[iprt].pos.z;

      if ( gs->PrtList[iprt].vel.z < -STOPBOUNCING )
      {
        gs->PrtList[iprt].accum_vel.z -= gs->PrtList[iprt].vel.z * ( 1.0f + dampen );
      }
      else if ( gs->PrtList[iprt].vel.z < STOPBOUNCING )
      {
        gs->PrtList[iprt].accum_vel.z -= gs->PrtList[iprt].vel.z;
      }
    };

  }

  return hitmesh;
};



//--------------------------------------------------------------------------------------------
void do_integration(GameState * gs, float dFrame)
{
  int cnt, tnc;

  for( cnt=0; cnt<MAXCHR; cnt++)
  {
    if( !gs->ChrList[cnt].on ) continue;

    gs->ChrList[cnt].pos_old = gs->ChrList[cnt].pos;

    gs->ChrList[cnt].pos.x += gs->ChrList[cnt].vel.x * dFrame + gs->ChrList[cnt].accum_pos.x;
    gs->ChrList[cnt].pos.y += gs->ChrList[cnt].vel.y * dFrame + gs->ChrList[cnt].accum_pos.y;
    gs->ChrList[cnt].pos.z += gs->ChrList[cnt].vel.z * dFrame + gs->ChrList[cnt].accum_pos.z;

    gs->ChrList[cnt].vel.x += gs->ChrList[cnt].accum_acc.x * dFrame + gs->ChrList[cnt].accum_vel.x;
    gs->ChrList[cnt].vel.y += gs->ChrList[cnt].accum_acc.y * dFrame + gs->ChrList[cnt].accum_vel.y;
    gs->ChrList[cnt].vel.z += gs->ChrList[cnt].accum_acc.z * dFrame + gs->ChrList[cnt].accum_vel.z;

    VectorClear( gs->ChrList[cnt].accum_acc.v );
    VectorClear( gs->ChrList[cnt].accum_vel.v );
    VectorClear( gs->ChrList[cnt].accum_pos.v );

    // iterate through the integration routine until you force the new position to be valid
    // should only ever go through the loop twice
    tnc = 0;
    while( tnc < 20 && chr_collide_mesh(gs, cnt) )
    {
      gs->ChrList[cnt].pos.x += gs->ChrList[cnt].accum_pos.x;
      gs->ChrList[cnt].pos.y += gs->ChrList[cnt].accum_pos.y;
      gs->ChrList[cnt].pos.z += gs->ChrList[cnt].accum_pos.z;

      gs->ChrList[cnt].vel.x += gs->ChrList[cnt].accum_vel.x;
      gs->ChrList[cnt].vel.y += gs->ChrList[cnt].accum_vel.y;
      gs->ChrList[cnt].vel.z += gs->ChrList[cnt].accum_vel.z;

      VectorClear( gs->ChrList[cnt].accum_acc.v );
      VectorClear( gs->ChrList[cnt].accum_vel.v );
      VectorClear( gs->ChrList[cnt].accum_pos.v );

      tnc++;
    };
  };

  for( cnt=0; cnt<MAXPRT; cnt++)
  {
    if( !gs->PrtList[cnt].on ) continue;

    gs->PrtList[cnt].pos_old = gs->PrtList[cnt].pos;

    gs->PrtList[cnt].pos.x += gs->PrtList[cnt].vel.x * dFrame + gs->PrtList[cnt].accum_pos.x;
    gs->PrtList[cnt].pos.y += gs->PrtList[cnt].vel.y * dFrame + gs->PrtList[cnt].accum_pos.y;
    gs->PrtList[cnt].pos.z += gs->PrtList[cnt].vel.z * dFrame + gs->PrtList[cnt].accum_pos.z;

    gs->PrtList[cnt].vel.x += gs->PrtList[cnt].accum_acc.x * dFrame + gs->PrtList[cnt].accum_vel.x;
    gs->PrtList[cnt].vel.y += gs->PrtList[cnt].accum_acc.y * dFrame + gs->PrtList[cnt].accum_vel.y;
    gs->PrtList[cnt].vel.z += gs->PrtList[cnt].accum_acc.z * dFrame + gs->PrtList[cnt].accum_vel.z;

    // iterate through the integration routine until you force the new position to be valid
    // should only ever go through the loop twice
    tnc = 0;
    while( tnc < 20 && prt_collide_mesh(gs, cnt) )
    {
      gs->PrtList[cnt].pos.x += gs->PrtList[cnt].accum_pos.x;
      gs->PrtList[cnt].pos.y += gs->PrtList[cnt].accum_pos.y;
      gs->PrtList[cnt].pos.z += gs->PrtList[cnt].accum_pos.z;

      gs->PrtList[cnt].vel.x += gs->PrtList[cnt].accum_vel.x;
      gs->PrtList[cnt].vel.y += gs->PrtList[cnt].accum_vel.y;
      gs->PrtList[cnt].vel.z += gs->PrtList[cnt].accum_vel.z;

      VectorClear( gs->PrtList[cnt].accum_acc.v );
      VectorClear( gs->PrtList[cnt].accum_vel.v );
      VectorClear( gs->PrtList[cnt].accum_pos.v );

      tnc++;
    }
  };

};

////--------------------------------------------------------------------------------------------
//void update_game( float dUpdate )
//{
//  // ZZ> This function does several iterations of character movements and such
//  //     to keep the game in sync.
//
//  int    cnt, numdead;
//
//  // Check for all local players being dead
//  gs->allpladead  = bfalse;
//  gs->somepladead = bfalse;
//  gs->al_cs->seeinvisible = bfalse;
//  gs->al_cs->seekurse = bfalse;
//  numdead = 0;
//  for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
//  {
//    CHR_REF ichr;
//    if ( !VALID_PLA( gs->PlaList,  cnt ) || INBITS_NONE == gs->PlaList[cnt].device ) continue;
//
//    ichr = PlaList_get_character( gs->PlaList, gs->PlaList_count, cnt );
//    if ( !VALID_CHR( gs->ChrList, ichr ) ) continue;
//
//    if ( !gs->ChrList[ichr].alive && gs->ChrList[ichr].islocalplayer )
//    {
//      numdead++;
//    };
//
//    if ( gs->ChrList[ichr].canseeinvisible )
//    {
//      gs->al_cs->seeinvisible = btrue;
//    }
//
//    if ( gs->ChrList[ichr].canseekurse )
//    {
//      gs->al_cs->seekurse = btrue;
//    }
//
//  };
//
//  gs->somepladead = ( numdead > 0 );
//  gs->allpladead  = ( numdead >= gs->al_cs->loc_pla_count );
//
//  // This is the main game loop
//  gs->al_cs->msg_timechange = 0;
//
//  // [claforte Jan 6th 2001]
//  // TODO: Put that back in place once GNet.working is functional.
//  // Important stuff to keep in sync
//  while (( gs->wld_clock < gs->all_clock ) && ( AClientState.tlb.numtimes > 0 ) )
//  {
//    srand( gs->al_ss->rand_idx );
//
//    PROFILE_BEGIN( resize_characters );
//    resize_characters( dUpdate );
//    PROFILE_END( resize_characters );
//
//    PROFILE_BEGIN( keep_weapons_with_holders );
//    keep_weapons_with_holders();
//    PROFILE_END( keep_weapons_with_holders );
//
//    PROFILE_BEGIN( let_ai_think );
//    let_ai_think( dUpdate );
//    PROFILE_END( let_ai_think );
//
//    PROFILE_BEGIN( do_weather_spawn );
//    do_weather_spawn( dUpdate );
//    PROFILE_END( do_weather_spawn );
//
//    PROFILE_BEGIN( enc_spawn_particles );
//    enc_spawn_particles( dUpdate );
//    PROFILE_END( enc_spawn_particles );
//
//    // unbuffer the updated latches after let_ai_think() and before move_characters()
//    PROFILE_BEGIN( ClientState_unbufferLatches );
//    ClientState_unbufferLatches( gs->al_cs );
//    PROFILE_END( ClientState_unbufferLatches );
//
//    PROFILE_BEGIN( sv_unbufferLatches );
//    sv_unbufferLatches( gs->al_ss );
//    PROFILE_END( sv_unbufferLatches );
//
//
//    PROFILE_BEGIN( check_respawn );
//    check_respawn();
//    despawn_characters();
//    despawn_particles();
//    PROFILE_END( check_respawn );
//
//    PROFILE_BEGIN( make_onwhichfan );
//    make_onwhichfan();
//    PROFILE_END( make_onwhichfan );
//
//    begin_integration();
//    {
//      PROFILE_BEGIN( move_characters );
//      move_characters( dUpdate );
//      PROFILE_END( move_characters );
//
//      PROFILE_BEGIN( move_particles );
//      move_particles( dUpdate );
//      PROFILE_END( move_particles );
//
//      PROFILE_BEGIN( attach_particles );
//      attach_particles();
//      PROFILE_END( attach_particles );
//
//      PROFILE_BEGIN( do_bumping );
//      do_bumping( dUpdate );
//      PROFILE_END( do_bumping );
//
//    }
//    do_integration(dUpdate);
//
//    PROFILE_BEGIN( make_character_matrices );
//    make_character_matrices();
//    PROFILE_END( make_character_matrices );
//
//    PROFILE_BEGIN( stat_return );
//    stat_return( dUpdate );
//    PROFILE_END( stat_return );
//
//    PROFILE_BEGIN( pit_kill );
//    pit_kill( dUpdate );
//    PROFILE_END( pit_kill );
//
//    // Generate the new seed
//    gs->al_ss->rand_idx += * (( Uint32* ) & kMd2Normals[gs->wld_frame&127][0] );
//    gs->al_ss->rand_idx += * (( Uint32* ) & kMd2Normals[gs->al_ss->rand_idx&127][1] );
//
//    // Stuff for which sync doesn't matter
//    PROFILE_BEGIN( animate_tiles );
//    animate_tiles( dUpdate );
//    PROFILE_END( animate_tiles );
//
//    PROFILE_BEGIN( move_water );
//    move_water( dUpdate );
//    PROFILE_END( move_water );
//
//    // Timers
//    gs->wld_clock += UPDATESKIP;
//    gs->wld_frame++;
//    gs->al_cs->msg_timechange++;
//    if ( Stat_delay > 0 )  Stat_delay--;
//    cs->stat_clock++;
//  }
//
//  {
//    if ( gs->al_cs->tlb.numtimes == 0 )
//    {
//      // The remote ran out of messages, and is now twiddling its thumbs...
//      // Make it go slower so it doesn't happen again
//      gs->wld_clock += UPDATESKIP / 4.0f;
//    }
//    else if ( !cl_Started() && gs->al_cs->tlb.numtimes > 3 )
//    {
//      // The host has too many messages, and is probably experiencing control
//      // CData.lag...  Speed it up so it gets closer to sync
//      gs->wld_clock -= UPDATESKIP / 4.0f;
//    }
//  }
//}
//
//--------------------------------------------------------------------------------------------
void update_timers(GameState * gs)
{
  // ZZ> This function updates the game timers

  gs->lst_clock = gs->all_clock;

  if(gs->Single_frame)
  {
    if(gs->Do_frame) gs->all_clock = gs->wld_clock + UPDATESKIP;
  }
  else
  {
    gs->all_clock = SDL_GetTicks() - gs->stt_clock;
  }

  ups_clock += gs->all_clock - gs->lst_clock;
  fps_clock += gs->all_clock - gs->lst_clock;

  if ( ups_loops > 0 && ups_clock > 0)
  {
    stabilized_ups_sum    = stabilized_ups_sum * 0.99 + 0.01 * ( float ) ups_loops / (( float ) ups_clock / TICKS_PER_SEC );
    stabilized_ups_weight = stabilized_ups_weight * 0.99 + 0.01;
  };

  if ( fps_loops > 0 && fps_clock > 0 )
  {
    stabilized_fps_sum        = stabilized_fps_sum * 0.99 + 0.01 * ( float ) fps_loops / (( float ) fps_clock / TICKS_PER_SEC );
    stabilized_fps_weight = stabilized_fps_weight * 0.99 + 0.01;
  };

  if ( ups_clock >= TICKS_PER_SEC )
  {
    ups_clock = 0;
    ups_loops = 0;
    assert(stabilized_ups_weight>0);
    stabilized_ups = stabilized_ups_sum / stabilized_ups_weight;
  }

  if ( fps_clock >= TICKS_PER_SEC )
  {
    fps_clock = 0;
    fps_loops = 0;
    assert(stabilized_ups_weight>0);
    stabilized_fps = stabilized_fps_sum / stabilized_fps_weight;
  }

}




//--------------------------------------------------------------------------------------------
void reset_teams(GameState * gs)
{
  // ZZ> This function makes everyone hate everyone else

  int teama, teamb;

  teama = 0;
  while ( teama < TEAM_COUNT )
  {
    // Make the team hate everyone
    teamb = 0;
    while ( teamb < TEAM_COUNT )
    {
      gs->TeamList[teama].hatesteam[teamb] = btrue;
      teamb++;
    }
    // Make the team like itself
    gs->TeamList[teama].hatesteam[teama] = bfalse;

    // Set defaults
    gs->TeamList[teama].leader = MAXCHR;
    gs->TeamList[teama].sissy = 0;
    gs->TeamList[teama].morale = 0;
    teama++;
  }


  // Keep the null team neutral
  teama = 0;
  while ( teama < TEAM_COUNT )
  {
    gs->TeamList[teama].hatesteam[TEAM_NULL] = bfalse;
    gs->TeamList[TEAM_NULL].hatesteam[teama] = bfalse;
    teama++;
  }
}

//--------------------------------------------------------------------------------------------
void reset_messages(GameState * gs)
{
  // ZZ> This makes messages safe to use

  int cnt;

  GMsg.total = 0;
  GMsg.totalindex = 0;
  gs->al_cs->msg_timechange = 0;
  GMsg.start = 0;
  cnt = 0;
  while ( cnt < MAXMESSAGE )
  {
    GMsg.list[cnt].time = 0;
    cnt++;
  }
  cnt = 0;
  while ( cnt < MAXTOTALMESSAGE )
  {
    GMsg.index[cnt] = 0;
    cnt++;
  }
  GMsg.text[0] = 0;
}

//--------------------------------------------------------------------------------------------
void reset_timers(GameState * gs)
{
  // ZZ> This function resets the timers...

  gs->stt_clock  = SDL_GetTicks();
  gs->all_clock  = 0;
  gs->lst_clock  = 0;
  gs->wld_clock  = 0;
  gs->al_cs->stat_clock = 0;
  gs->pit_clock  = 0;  gs->pitskill = bfalse;
  gs->wld_frame  = 0;
  gs->all_frame  = 0;

  outofsync = bfalse;

  ups_loops = 0;
  ups_clock = 0;
  fps_loops = 0;
  fps_clock = 0;
}

extern int initMenus();

#define DO_CONFIGSTRING_COMPARE(XX) if(0 == strncmp(#XX, szin, strlen(#XX))) { if(NULL!=szout) *szout = szin + strlen(#XX); return cd->XX; }
//--------------------------------------------------------------------------------------------
char * get_config_string(ConfigData * cd, char * szin, char ** szout)
{
  // BB > localize a string by converting the name of the string to the string itself

  DO_CONFIGSTRING_COMPARE(basicdat_dir)
  else DO_CONFIGSTRING_COMPARE(gamedat_dir)
  else DO_CONFIGSTRING_COMPARE(mnu_dir)
  else DO_CONFIGSTRING_COMPARE(globalparticles_dir)
  else DO_CONFIGSTRING_COMPARE(modules_dir)
  else DO_CONFIGSTRING_COMPARE(music_dir)
  else DO_CONFIGSTRING_COMPARE(objects_dir)
  else DO_CONFIGSTRING_COMPARE(import_dir)
  else DO_CONFIGSTRING_COMPARE(players_dir)
  else DO_CONFIGSTRING_COMPARE(nullicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(keybicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(mousicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(joyaicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(joybicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile0_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile1_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile2_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile3_bitmap)
  else DO_CONFIGSTRING_COMPARE(watertop_bitmap)
  else DO_CONFIGSTRING_COMPARE(waterlow_bitmap)
  else DO_CONFIGSTRING_COMPARE(phong_bitmap)
  else DO_CONFIGSTRING_COMPARE(plan_bitmap)
  else DO_CONFIGSTRING_COMPARE(blip_bitmap)
  else DO_CONFIGSTRING_COMPARE(font_bitmap)
  else DO_CONFIGSTRING_COMPARE(icon_bitmap)
  else DO_CONFIGSTRING_COMPARE(bars_bitmap)
  else DO_CONFIGSTRING_COMPARE(particle_bitmap)
  else DO_CONFIGSTRING_COMPARE(title_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_main_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_advent_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_sleepy_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_gnome_bitmap)
  //else DO_CONFIGSTRING_COMPARE(slotused_file)
  else DO_CONFIGSTRING_COMPARE(passage_file)
  else DO_CONFIGSTRING_COMPARE(aicodes_file)
  else DO_CONFIGSTRING_COMPARE(actions_file)
  else DO_CONFIGSTRING_COMPARE(alliance_file)
  else DO_CONFIGSTRING_COMPARE(fans_file)
  else DO_CONFIGSTRING_COMPARE(fontdef_file)
  //else DO_CONFIGSTRING_COMPARE(mnu_file)
  else DO_CONFIGSTRING_COMPARE(money1_file)
  else DO_CONFIGSTRING_COMPARE(money5_file)
  else DO_CONFIGSTRING_COMPARE(money25_file)
  else DO_CONFIGSTRING_COMPARE(money100_file)
  else DO_CONFIGSTRING_COMPARE(weather1_file)
  else DO_CONFIGSTRING_COMPARE(weather2_file)
  else DO_CONFIGSTRING_COMPARE(script_file)
  else DO_CONFIGSTRING_COMPARE(ripple_file)
  else DO_CONFIGSTRING_COMPARE(scancode_file)
  else DO_CONFIGSTRING_COMPARE(playlist_file)
  else DO_CONFIGSTRING_COMPARE(spawn_file)
  else DO_CONFIGSTRING_COMPARE(wawalite_file)
  else DO_CONFIGSTRING_COMPARE(defend_file)
  else DO_CONFIGSTRING_COMPARE(splash_file)
  else DO_CONFIGSTRING_COMPARE(mesh_file)
  else DO_CONFIGSTRING_COMPARE(setup_file)
  else DO_CONFIGSTRING_COMPARE(log_file)
  else DO_CONFIGSTRING_COMPARE(controls_file)
  else DO_CONFIGSTRING_COMPARE(data_file)
  else DO_CONFIGSTRING_COMPARE(copy_file)
  else DO_CONFIGSTRING_COMPARE(enchant_file)
  else DO_CONFIGSTRING_COMPARE(message_file)
  else DO_CONFIGSTRING_COMPARE(naming_file)
  else DO_CONFIGSTRING_COMPARE(modules_file)
  else DO_CONFIGSTRING_COMPARE(setup_file)
  else DO_CONFIGSTRING_COMPARE(skin_file)
  else DO_CONFIGSTRING_COMPARE(credits_file)
  else DO_CONFIGSTRING_COMPARE(quest_file)
  else DO_CONFIGSTRING_COMPARE(uifont_ttf)
  else DO_CONFIGSTRING_COMPARE(coinget_sound)
  else DO_CONFIGSTRING_COMPARE(defend_sound)
  else DO_CONFIGSTRING_COMPARE(coinfall_sound)
  else DO_CONFIGSTRING_COMPARE(lvlup_sound);

  return NULL;
}


//--------------------------------------------------------------------------------------------
char * get_config_string_name(ConfigData * cd, STRING * pconfig_string)
{
  // BB > localize a string by converting the name of the string to the string itself

  if(pconfig_string == &(cd->basicdat_dir)) return "basicdat_dir";
  else if(pconfig_string == &(cd->gamedat_dir)) return "gamedat_dir";
  else if(pconfig_string == &(cd->mnu_dir)) return "mnu_dir";
  else if(pconfig_string == &(cd->globalparticles_dir)) return "globalparticles_dir";
  else if(pconfig_string == &(cd->modules_dir)) return "modules_dir";
  else if(pconfig_string == &(cd->music_dir)) return "music_dir";
  else if(pconfig_string == &(cd->objects_dir)) return "objects_dir";
  else if(pconfig_string == &(cd->import_dir)) return "import_dir";
  else if(pconfig_string == &(cd->players_dir)) return "players_dir";
  else if(pconfig_string == &(cd->nullicon_bitmap)) return "nullicon_bitmap";
  else if(pconfig_string == &(cd->keybicon_bitmap)) return "keybicon_bitmap";
  else if(pconfig_string == &(cd->mousicon_bitmap)) return "mousicon_bitmap";
  else if(pconfig_string == &(cd->joyaicon_bitmap)) return "joyaicon_bitmap";
  else if(pconfig_string == &(cd->joybicon_bitmap)) return "joybicon_bitmap";
  else if(pconfig_string == &(cd->tile0_bitmap)) return "tile0_bitmap";
  else if(pconfig_string == &(cd->tile1_bitmap)) return "tile1_bitmap";
  else if(pconfig_string == &(cd->tile2_bitmap)) return "tile2_bitmap";
  else if(pconfig_string == &(cd->tile3_bitmap)) return "tile3_bitmap";
  else if(pconfig_string == &(cd->watertop_bitmap)) return "watertop_bitmap";
  else if(pconfig_string == &(cd->waterlow_bitmap)) return "waterlow_bitmap";
  else if(pconfig_string == &(cd->phong_bitmap)) return "phong_bitmap";
  else if(pconfig_string == &(cd->plan_bitmap)) return "plan_bitmap";
  else if(pconfig_string == &(cd->blip_bitmap)) return "blip_bitmap";
  else if(pconfig_string == &(cd->font_bitmap)) return "font_bitmap";
  else if(pconfig_string == &(cd->icon_bitmap)) return "icon_bitmap";
  else if(pconfig_string == &(cd->bars_bitmap)) return "bars_bitmap";
  else if(pconfig_string == &(cd->particle_bitmap)) return "particle_bitmap";
  else if(pconfig_string == &(cd->title_bitmap)) return "title_bitmap";
  else if(pconfig_string == &(cd->menu_main_bitmap)) return "menu_main_bitmap";
  else if(pconfig_string == &(cd->menu_advent_bitmap)) return "menu_advent_bitmap";
  else if(pconfig_string == &(cd->menu_sleepy_bitmap)) return "menu_sleepy_bitmap";
  else if(pconfig_string == &(cd->menu_gnome_bitmap)) return "menu_gnome_bitmap";
  //else if(pconfig_string == &(cd->slotused_file)) return "slotused_file";
  else if(pconfig_string == &(cd->passage_file)) return "passage_file";
  else if(pconfig_string == &(cd->aicodes_file)) return "aicodes_file";
  else if(pconfig_string == &(cd->actions_file)) return "actions_file";
  else if(pconfig_string == &(cd->alliance_file)) return "alliance_file";
  else if(pconfig_string == &(cd->fans_file)) return "fans_file";
  else if(pconfig_string == &(cd->fontdef_file)) return "fontdef_file";
  //else if(pconfig_string == &(cd->mnu_file)) return "mnu_file";
  else if(pconfig_string == &(cd->money1_file)) return "money1_file";
  else if(pconfig_string == &(cd->money5_file)) return "money5_file";
  else if(pconfig_string == &(cd->money25_file)) return "money25_file";
  else if(pconfig_string == &(cd->money100_file)) return "money100_file";
  else if(pconfig_string == &(cd->weather1_file)) return "weather1_file";
  else if(pconfig_string == &(cd->weather2_file)) return "weather2_file";
  else if(pconfig_string == &(cd->script_file)) return "script_file";
  else if(pconfig_string == &(cd->ripple_file)) return "ripple_file";
  else if(pconfig_string == &(cd->scancode_file)) return "scancode_file";
  else if(pconfig_string == &(cd->playlist_file)) return "playlist_file";
  else if(pconfig_string == &(cd->spawn_file)) return "spawn_file";
  else if(pconfig_string == &(cd->wawalite_file)) return "wawalite_file";
  else if(pconfig_string == &(cd->defend_file)) return "defend_file";
  else if(pconfig_string == &(cd->splash_file)) return "splash_file";
  else if(pconfig_string == &(cd->mesh_file)) return "mesh_file";
  else if(pconfig_string == &(cd->setup_file)) return "setup_file";
  else if(pconfig_string == &(cd->log_file)) return "log_file";
  else if(pconfig_string == &(cd->controls_file)) return "controls_file";
  else if(pconfig_string == &(cd->data_file)) return "data_file";
  else if(pconfig_string == &(cd->copy_file)) return "copy_file";
  else if(pconfig_string == &(cd->enchant_file)) return "enchant_file";
  else if(pconfig_string == &(cd->message_file)) return "message_file";
  else if(pconfig_string == &(cd->naming_file)) return "naming_file";
  else if(pconfig_string == &(cd->modules_file)) return "modules_file";
  else if(pconfig_string == &(cd->setup_file)) return "setup_file";
  else if(pconfig_string == &(cd->skin_file)) return "skin_file";
  else if(pconfig_string == &(cd->credits_file)) return "credits_file";
  else if(pconfig_string == &(cd->quest_file)) return "quest_file";
  else if(pconfig_string == &(cd->uifont_ttf)) return "uifont_ttf";
  else if(pconfig_string == &(cd->coinget_sound)) return "coinget_sound";
  else if(pconfig_string == &(cd->defend_sound)) return "defend_sound";
  else if(pconfig_string == &(cd->coinfall_sound)) return "coinfall_sound";
  else if(pconfig_string == &(cd->lvlup_sound)) return "lvlup_sound";

  return NULL;
}


//--------------------------------------------------------------------------------------------
void set_default_config_data(ConfigData * pcon)
{
  if(NULL==pcon) return;

  strncpy( pcon->basicdat_dir, "basicdat", sizeof( STRING ) );
  strncpy( pcon->gamedat_dir, "gamedat" , sizeof( STRING ) );
  strncpy( pcon->mnu_dir, "menu" , sizeof( STRING ) );
  strncpy( pcon->globalparticles_dir, "globalparticles" , sizeof( STRING ) );
  strncpy( pcon->modules_dir, "modules" , sizeof( STRING ) );
  strncpy( pcon->music_dir, "music" , sizeof( STRING ) );
  strncpy( pcon->objects_dir, "objects" , sizeof( STRING ) );
  strncpy( pcon->import_dir, "import" , sizeof( STRING ) );
  strncpy( pcon->players_dir, "players", sizeof( STRING ) );

  strncpy( pcon->nullicon_bitmap, "gs->nullicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->keybicon_bitmap, "gs->keybicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->mousicon_bitmap, "gs->mousicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->joyaicon_bitmap, "gs->joyaicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->joybicon_bitmap, "gs->joybicon.bmp" , sizeof( STRING ) );

  strncpy( pcon->tile0_bitmap, "tile0.bmp" , sizeof( STRING ) );
  strncpy( pcon->tile1_bitmap, "tile1.bmp" , sizeof( STRING ) );
  strncpy( pcon->tile2_bitmap, "tile2.bmp" , sizeof( STRING ) );
  strncpy( pcon->tile3_bitmap, "tile3.bmp" , sizeof( STRING ) );
  strncpy( pcon->watertop_bitmap, "watertop.bmp" , sizeof( STRING ) );
  strncpy( pcon->waterlow_bitmap, "waterlow.bmp" , sizeof( STRING ) );
  strncpy( pcon->phong_bitmap, "phong.bmp" , sizeof( STRING ) );
  strncpy( pcon->plan_bitmap, "plan.bmp" , sizeof( STRING ) );
  strncpy( pcon->blip_bitmap, "blip9.png" , sizeof( STRING ) );
  strncpy( pcon->font_bitmap, "font.png" , sizeof( STRING ) );
  strncpy( pcon->icon_bitmap, "icon.bmp" , sizeof( STRING ) );
  strncpy( pcon->bars_bitmap, "bars.png" , sizeof( STRING ) );
  strncpy( pcon->particle_bitmap, "particle_normal.png" , sizeof( STRING ) );
  strncpy( pcon->title_bitmap, "title.bmp" , sizeof( STRING ) );

  strncpy( pcon->menu_main_bitmap, "menu_main.png" , sizeof( STRING ) );
  strncpy( pcon->menu_advent_bitmap, "menu_advent.png" , sizeof( STRING ) );
  strncpy( pcon->menu_sleepy_bitmap, "menu_sleepy.png" , sizeof( STRING ) );
  strncpy( pcon->menu_gnome_bitmap, "menu_gnome.png" , sizeof( STRING ) );


  strncpy( pcon->debug_file, "debug.txt" , sizeof( STRING ) );
  strncpy( pcon->passage_file, "passage.txt" , sizeof( STRING ) );
  strncpy( pcon->aicodes_file, "aicodes.txt" , sizeof( STRING ) );
  strncpy( pcon->actions_file, "actions.txt" , sizeof( STRING ) );
  strncpy( pcon->alliance_file, "alliance.txt" , sizeof( STRING ) );
  strncpy( pcon->fans_file, "fans.txt" , sizeof( STRING ) );
  strncpy( pcon->fontdef_file, "font.txt" , sizeof( STRING ) );
  strncpy( pcon->mnu_file, "menu.txt" , sizeof( STRING ) );
  strncpy( pcon->money1_file, "1money.txt" , sizeof( STRING ) );
  strncpy( pcon->money5_file, "5money.txt" , sizeof( STRING ) );
  strncpy( pcon->money25_file, "25money.txt" , sizeof( STRING ) );
  strncpy( pcon->money100_file, "100money.txt" , sizeof( STRING ) );
  strncpy( pcon->weather1_file, "weather4.txt" , sizeof( STRING ) );
  strncpy( pcon->weather2_file, "weather5.txt" , sizeof( STRING ) );
  strncpy( pcon->script_file, "script.txt" , sizeof( STRING ) );
  strncpy( pcon->ripple_file, "ripple.txt" , sizeof( STRING ) );
  strncpy( pcon->scancode_file, "scancode.txt" , sizeof( STRING ) );
  strncpy( pcon->playlist_file, "playlist.txt" , sizeof( STRING ) );
  strncpy( pcon->spawn_file, "spawn.txt" , sizeof( STRING ) );
  strncpy( pcon->wawalite_file, "wawalite.txt" , sizeof( STRING ) );
  strncpy( pcon->defend_file, "defend.txt" , sizeof( STRING ) );
  strncpy( pcon->splash_file, "splash.txt" , sizeof( STRING ) );
  strncpy( pcon->mesh_file, "level.mpd" , sizeof( STRING ) );
  strncpy( pcon->setup_file, "setup.txt" , sizeof( STRING ) );
  strncpy( pcon->log_file, "log.txt", sizeof( STRING ) );
  strncpy( pcon->controls_file, "controls.txt", sizeof( STRING ) );
  strncpy( pcon->data_file, "data.txt", sizeof( STRING ) );
  strncpy( pcon->copy_file, "copy.txt", sizeof( STRING ) );
  strncpy( pcon->enchant_file, "enchant.txt", sizeof( STRING ) );
  strncpy( pcon->message_file, "message.txt", sizeof( STRING ) );
  strncpy( pcon->naming_file, "naming.txt", sizeof( STRING ) );
  strncpy( pcon->modules_file, "modules.txt", sizeof( STRING ) );
  strncpy( pcon->setup_file, "setup.txt", sizeof( STRING ) );
  strncpy( pcon->skin_file, "skin.txt", sizeof( STRING ) );
  strncpy( pcon->credits_file, "credits.txt", sizeof( STRING ) );
  strncpy( pcon->quest_file, "quest.txt", sizeof( STRING ) );


  pcon->uifont_points  = 20;
  pcon->uifont_points2 = 18;
  strncpy( pcon->uifont_ttf, "Negatori.ttf" , sizeof( STRING ) );

  strncpy( pcon->coinget_sound, "coinget.wav" , sizeof( STRING ) );
  strncpy( pcon->defend_sound, "defend.wav" , sizeof( STRING ) );
  strncpy( pcon->coinfall_sound, "coinfall.wav" , sizeof( STRING ) );
  strncpy( pcon->lvlup_sound, "lvlup.wav" , sizeof( STRING ) );

  pcon->fullscreen = bfalse;
  pcon->zreflect = bfalse;
  pcon->maxtotalmeshvertices = 256 * 256 * 6;
  pcon->scrd = 8;                   // Screen bit depth
  pcon->scrx = 320;                 // Screen X size
  pcon->scry = 200;                 // Screen Y size
  pcon->scrz = 16;                  // Screen z-buffer depth ( 8 unsupported )
  pcon->maxmessage = MAXMESSAGE;    //
  pcon->messageon  = btrue;           // Messages?
  pcon->wraptolerance = 80;     // Status bar
  pcon->staton = btrue;                 // Draw the status bars?
  pcon->render_overlay = bfalse;
  pcon->render_background = bfalse;
  pcon->perspective = GL_FASTEST;
  pcon->dither = bfalse;
  pcon->shading = GL_FLAT;
  pcon->antialiasing = bfalse;
  pcon->refon = bfalse;
  pcon->shaon = bfalse;
  pcon->texturefilter = TX_LINEAR;
  pcon->wateron = btrue;
  pcon->shasprite = bfalse;
  pcon->phongon = btrue;                // Do phong overlay? OUTDATED?
  pcon->zreflect = bfalse;
  pcon->reffadeor = 255;              // 255 = Don't fade reflections
  pcon->twolayerwateron = bfalse;        // Two layer water?
  pcon->overlayvalid = bfalse;               // Allow large overlay?
  pcon->backgroundvalid = bfalse;            // Allow large background?
  pcon->fogallowed = btrue;          //
  pcon->particletype = PART_NORMAL;
  pcon->vsync = bfalse;
  pcon->gfxacceleration = bfalse;
  pcon->allow_sound = bfalse;     //Allow playing of sound?
  pcon->allow_music = bfalse;     // Allow music and loops?
  pcon->request_network = btrue;              // Try to connect?
  pcon->lag        = 3;                                // Lag tolerance

  // clear out the net hosts
  memset(pcon->net_hosts, 0, 8 * sizeof(STRING) );
  strncpy( pcon->net_hosts[0], "no host", sizeof(STRING) );                        // Name for hosting session
  strncpy( pcon->net_messagename, "little Raoul", sizeof( pcon->net_messagename ) );             // Name for messages
  pcon->fpson = btrue;               // FPS displayed?

  // Debug option
  pcon->GrabMouse = SDL_GRAB_ON;
  pcon->HideMouse = bfalse;
  pcon->DevMode  = btrue;
  // Debug option
};

//--------------------------------------------------------------------------------------------

typedef enum ProcessStates_e
{
  PROC_Begin,
  PROC_Entering,
  PROC_Running,
  PROC_Leaving,
  PROC_Finish,
} ProcessStates;

int proc_mainLoop( ProcState * ego_proc, int argc, char **argv );
int proc_gameLoop( ProcState * gproc, GameState * gs);
int proc_menuLoop( MenuProc  * mproc );

//--------------------------------------------------------------------------------------------
ProcessStates main_handleKeyboard(ProcState * proc, GameState * gs)
{
  // do screenshot
  if ( SDLKEYDOWN( SDLK_F11 ) )
  {
    if(!do_screenshot())
    {
      debug_message( 1, "Error writing screenshot" );
      log_warning( "Error writing screenshot\n" );    //Log the error in log.txt
    }
  }

  // do in-game-menu
  if(SDLKEYDOWN(SDLK_F9) && CData.DevMode)
  {
    gs->igm.proc.Active    = btrue;
    gs->igm.whichMenu = mnu_Inventory;

    SDL_WM_GrabInput(SDL_GRAB_OFF);
    SDL_ShowCursor(SDL_DISABLE);
    mous.on = bfalse;
  }

  // do frame step in single-frame mode
  if( gs->Do_frame )
  {
    gs->Do_frame = bfalse;
  }
  else if( SDLKEYDOWN( SDLK_F10 ) )
  {
    keyb.state[SDLK_F10] = 0;
    gs->Do_frame = btrue;
  }

  //Pressed panic button
  if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
  {
    log_info( "User pressed escape button (LCTRL+Q)... Quitting game gracefully.\n" );
    proc->State = PROC_Leaving;

    // <BB> memory_cleanUp() should be kept in PROC_Finish, so that we can make sure to deallocate
    //      the memory for any active menus, and/or modules.  Alternately, we could
    //      register all of the memory deallocation routines with atexit() so they will
    //      be called automatically on a return from the main function or a call to exit()
  }

  return proc->State;
};

//--------------------------------------------------------------------------------------------
void main_doGameGraphics()
{
  GameState * gs = gfxState.gs;

  move_camera( UPDATESCALE );

  if ( gs->dFrame >= 1.0 )
  {
    bool_t outdoors = bfalse; // GLight.spek > 0;

    // animate the light source
    if( outdoors )
    {
      // BB > simulate sunlight

      float sval, cval, xval, yval, ftmp;

      sval = sin( gs->dFrame/50.0f * TWO_PI / 30.0f);
      cval = cos( gs->dFrame/50.0f * TWO_PI / 30.0f);

      xval = GLight.spekdir.y;
      yval = GLight.spekdir.z;

      GLight.spekdir.y = xval*cval + yval*sval;
      GLight.spekdir.z =-xval*sval + yval*cval;

      if ( GLight.spekdir.z > 0 )
      {
        // do the filtering
        GLight.spekcol.r = exp(( 1.0f -  1.0f  / GLight.spekdir.z ) * 0.1f );
        GLight.spekcol.g = exp(( 5.0f -  5.0f  / GLight.spekdir.z ) * 0.1f );
        GLight.spekcol.b = exp(( 16.0f - 16.0f / GLight.spekdir.z ) * 0.1f );

        GLight.ambicol.r = ( 1.0f - GLight.spekcol.r );
        GLight.ambicol.g = ( 1.0f - GLight.spekcol.g );
        GLight.ambicol.b = ( 1.0f - GLight.spekcol.b );

        // do the intensity
        GLight.spekcol.r *= ABS( GLight.spekdir.z ) * GLight.spek;
        GLight.spekcol.g *= ABS( GLight.spekdir.z ) * GLight.spek;
        GLight.spekcol.b *= ABS( GLight.spekdir.z ) * GLight.spek;

        if(GLight.ambicol.r + GLight.ambicol.g + GLight.ambicol.b > 0)
        {
          ftmp = ( GLight.spekcol.r + GLight.spekcol.g + GLight.spekcol.b ) / (GLight.ambicol.r + GLight.ambicol.g + GLight.ambicol.b);
          GLight.ambicol.r = 0.025f + (1.0f-0.025f) * ftmp * GLight.ambicol.r + ABS( GLight.spekdir.z ) * GLight.ambi;
          GLight.ambicol.g = 0.044f + (1.0f-0.044f) * ftmp * GLight.ambicol.g + ABS( GLight.spekdir.z ) * GLight.ambi;
          GLight.ambicol.b = 0.100f + (0.9f-0.100f) * ftmp * GLight.ambicol.b + ABS( GLight.spekdir.z ) * GLight.ambi;
        };
      }
      else
      {
        GLight.spekcol.r = 0.0f;
        GLight.spekcol.g = 0.0f;
        GLight.spekcol.b = 0.0f;

        GLight.ambicol.r = 0.025 + GLight.ambi;
        GLight.ambicol.g = 0.044 + GLight.ambi;
        GLight.ambicol.b = 0.100 + GLight.ambi;
      };

      make_spektable( GLight.spekdir );
    };

    // Do the display stuff
    PROFILE_BEGIN( figure_out_what_to_draw );
    figure_out_what_to_draw();
    PROFILE_END( figure_out_what_to_draw );

    PROFILE_BEGIN( draw_main );
    draw_main( gs->dFrame );
    PROFILE_END( draw_main );

    draw_chr_info( gs );

    gs->dFrame = 0.0f;
  }
}
//--------------------------------------------------------------------------------------------
// the main loop. Processes all graphics
retval_t main_doGraphics()
{
  GameState * gs;
  GuiState  * gui;
  ProcState * game_proc;
  MenuProc  * mnu_proc, * ig_mnu_proc;
  MachineState * mach_state = Get_MachineState();

  double frameDuration, frameTicks;
  bool_t do_menu_frame = bfalse;

  gui = Get_GuiState();
  if (NULL == gui) return rv_error;
  mnu_proc = &(gui->mnu_proc);

  if (NULL == gfxState.gs) return rv_error;
  gs          = gfxState.gs;
  ig_mnu_proc = GameState_getMenuProc(gs);
  game_proc   = GameState_getProcedure(gs);

  // Clock updates each frame
  ClockState_frameStep( gui->clk );
  frameDuration = ClockState_getFrameDuration( gui->clk );
  frameTicks    = frameDuration * TICKS_PER_SEC;
  gui->dUpdate  += frameTicks / UPDATESKIP;

  // read the input
  input_read();

  // handle the keyboard input
  main_handleKeyboard(game_proc, gs);

  // blank the screen, if required
  do_clear();

  // update the game, if active
  if ( game_proc->Active )
  {
    main_doGameGraphics();
  }

  // figure out if we need to process the menus
  if(game_proc->Active)
  {
    // the game has written into the frame buffer
    // put the menus on top
    do_menu_frame = query_pageflip();
  }
  else
  {
    // the frame timer has timed out
    do_menu_frame = gui->dUpdate > 1.0f;
  }

  if( do_menu_frame )
  {
    // do a frame for the menu ie EITHER the game has written into the frame buffer
    // OR the game is not active and it is time for an update

    // Do the in-game menu, if it is active
    if ( game_proc->Active && ig_mnu_proc->proc.Active )
    {
      ui_beginFrame();
      {
        mnu_RunIngame( ig_mnu_proc, gs );
        switch ( ig_mnu_proc->MenuResult  )
        {
          case 1: /* nothing */
            break;

          case - 1:
            // Done with the menu
            ig_mnu_proc->proc.Active = bfalse;
            mnu_exitMenuMode();
            break;
        }

      }
      ui_endFrame();

      request_pageflip();
    }

    // Do the out-of-game menu, if it is active
    if ( mnu_proc->proc.Active && !mnu_proc->proc.Paused )
    {
      proc_menuLoop( mnu_proc );

      switch ( mnu_proc->proc.returnValue )
      {
        case  1:
          // start the game
          game_proc->State      = PROC_Begin;
          game_proc->Active     = btrue;
          game_proc->Paused     = bfalse;
          game_proc->Terminated = bfalse;

          // pause the menu
          mnu_proc->proc.Paused = btrue;

          mnu_exitMenuMode();
          break;

        case - 1:
          // the menu should have terminated
          mnu_proc->proc.Active = btrue;
          mnu_proc->proc.Paused = bfalse;

          mnu_exitMenuMode();
          break;
      }
    }

    gui->dUpdate -= 1.0f;
    if(gui->dUpdate < 0) gui->dUpdate = 0;
  }

  // do pageflip, if required
  do_pageflip();

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
int proc_mainLoop( ProcState * ego_proc, int argc, char **argv )
{
  // ZZ> This is where the program starts and all the high level stuff happens
  static double frameDuration, frameTicks;
  GameState    * gs;
  GSStack      * stk;
  MachineState * mach_state;
  int i;

  mach_state = Get_MachineState();
  stk        = Get_GSStack();

  if(ego_proc->KillMe && ego_proc->State < PROC_Leaving)
  {
    ego_proc->Active = btrue;
    ego_proc->Paused = bfalse;
    ego_proc->State  = PROC_Leaving;
  }

  ego_proc->returnValue = 0;
  if(!ego_proc->Active || ego_proc->Paused) return ego_proc->returnValue;

  switch ( ego_proc->State )
  {
    case PROC_Begin:
      {
        // use CData_default gives the default values for CData
        // when we read the CData values from a file, the CData_default will be used
        // for any non-existant tags
        set_default_config_data(&CData_default);
        memcpy(&CData, &CData_default, sizeof(ConfigData));

        // start initializing the various subsystems
        log_message( "Starting Egoboo %s...\n", VERSION );

        // make sure the arrays are initialized to some specific initial value
        memset(BlipList, 0, MAXBLIP * sizeof(BLIP));

        read_setup( CData.setup_file );

        snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.scancode_file );
        read_all_tags( CStringTmp1 );

        read_controls( CData.controls_file );

        snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.aicodes_file );
        load_ai_codes( CStringTmp1 );

        snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.actions_file );
        load_action_names( CStringTmp1 );

        // create a new game state (automatically linked into the game state stack)
        GameState_create();

        // start the GuiState
        GuiState_startUp();

        // start the network
        net_startUp(&CData);

        // initialize the graphics state
        gfx_initialize( &gfxState, &CData );

        // initialize the SDL and OpenGL
        sdlinit( &gfxState );

        // initialize the OpenGL state
        glinit( &gfxState, &CData );


        // initialize the user interface
        snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.uifont_ttf);
        ui_initialize(CStringTmp1, CData.uifont_points);

        // initialize the sound system
        snd_initialize(&sndState, &CData);
        load_all_music_sounds(&sndState, &CData);

        // allocate the maximum amount of mesh memory
        load_mesh_fans();

        // Make lookup tables
        make_textureoffset();
        make_lightdirectionlookup();
        make_turntosin();
        make_enviro();

        //DEBUG
        CData.scrd = 32;    //We need to force 32 bit, since SDL_image crashes without it
        //DEBUG END

        // force memory_cleanUp() on any exit. placing this last makes it go first.
        atexit(memory_cleanUp);
      }
      ego_proc->State = PROC_Entering;
      break;

    case PROC_Entering:
      {
        GuiState     * gui;
        MenuProc     * mnu_proc;

        gui        = Get_GuiState();               // automatically starts the GuiState
        mnu_proc   = &(gui->mnu_proc);

        PROFILE_INIT( resize_characters );
        PROFILE_INIT( keep_weapons_with_holders );
        PROFILE_INIT( let_ai_think );
        PROFILE_INIT( do_weather_spawn );
        PROFILE_INIT( enc_spawn_particles );
        PROFILE_INIT( ClientState_unbufferLatches );
        PROFILE_INIT( sv_unbufferLatches );
        PROFILE_INIT( check_respawn );
        PROFILE_INIT( move_characters );
        PROFILE_INIT( move_particles );
        PROFILE_INIT( make_character_matrices );
        PROFILE_INIT( attach_particles );
        PROFILE_INIT( make_onwhichfan );
        PROFILE_INIT( do_bumping );
        PROFILE_INIT( stat_return );
        PROFILE_INIT( pit_kill );
        PROFILE_INIT( animate_tiles );
        PROFILE_INIT( move_water );
        PROFILE_INIT( figure_out_what_to_draw );
        PROFILE_INIT( draw_main );
        PROFILE_INIT( pre_update_game );
        PROFILE_INIT( update_game );
        PROFILE_INIT( main_loop );

        if( mnu_proc->proc.Active )
        {
          mnu_enterMenuMode();
          snd_play_music( 0, 500, -1 ); //Play the menu music
        };
      }
      ego_proc->State = PROC_Running;
      break;

    case PROC_Running:
      {
        GuiState * gui;
        MenuProc * mnu_proc;
        bool_t     no_games_active;

        gui        = Get_GuiState();               // automatically starts the GuiState
        mnu_proc   = &(gui->mnu_proc);

        no_games_active = btrue;
        for(i=0; i<stk->count; i++)
        {
          ProcState * proc;

          gs   = GSStack_get(stk, i);
          proc = GameState_getProcedure(gs);

          // a kludge to make sure the the gfxState is connected to something
          if(NULL == gfxState.gs) gfxState.gs = gs;

          proc_gameLoop( proc, gs ) ;

          if( proc->Active )
          {
            no_games_active = bfalse;
          }
        }

        // automatically restart the main menu
        if ( no_games_active && (!mnu_proc->proc.Active || mnu_proc->proc.Paused ) && !mnu_proc->proc.Terminated )
        {
          mnu_proc->proc.Active = btrue;
          mnu_proc->proc.Paused = bfalse;
          mnu_proc->lastMenu = mnu_proc->whichMenu = mnu_Main;
          mnu_enterMenuMode();
          snd_play_music( 0, 500, -1 ); //Play the menu music
        }

        if ( no_games_active && mnu_proc->proc.Terminated )
        {
          ego_proc->KillMe = btrue;
        }

        main_doGraphics();

        // let the OS breathe
        {
          //SDL_Delay( 1 );
          int ms_leftover = MAX(0, UPDATESKIP - frameDuration * TICKS_PER_SEC);
          if(ms_leftover / 4 > 0)
          {
            SDL_Delay( ms_leftover / 4 );
          }
        }
      }

      break;

    case PROC_Leaving:
      {
        GuiState     * gui;
        MenuProc     * mnu_proc;
        bool_t all_games_finished;

        gui        = Get_GuiState();               // automatically starts the GuiState
        mnu_proc   = &(gui->mnu_proc);

        ClockState_frameStep( mach_state->clk );
        frameDuration = ClockState_getFrameDuration( mach_state->clk );

        // request that the menu loop to clean itself.
        // the menu loop is only connected to the GameState that
        // the gfxState is connected to
        if ( !mnu_proc->proc.Terminated )
        {
          mnu_proc->proc.KillMe = btrue;
          proc_menuLoop(mnu_proc);
        };

        // terminate every single game state
        all_games_finished = btrue;
        for(i=0; i<stk->count; i++)
        {
          gs = GSStack_get(stk, i);

          // force the main loop to clean itself
          if ( !gs->proc.Terminated )
          {
            gs->proc.KillMe = btrue;
            proc_gameLoop( GameState_getProcedure(gs), gs);
            switch ( gs->proc.returnValue )
            {
              case -1: gs->proc.Active = bfalse; gs->proc.Terminated = btrue; break;
            };

            all_games_finished = all_games_finished && !gs->proc.Terminated;
          };
        }
        
        if ( all_games_finished && mnu_proc->proc.Terminated ) ego_proc->State = PROC_Finish;
      }
      break;

    case PROC_Finish:
      {
        log_info( "============ PROFILE =================\n" );
        log_info( "times in microseconds\n" );
        log_info( "======================================\n" );
        log_info( "\tmain_loop  - %lf\n", 1e6*PROFILE_QUERY( main_loop ) );
        log_info( "======================================\n" );
        log_info( "break down of main loop sub-functions\n" );
        log_info( "\tpre_update_game - %lf\n", 1e6*PROFILE_QUERY( pre_update_game ) );
        log_info( "\tupdate_game - %lf\n", 1e6*PROFILE_QUERY( update_game ) );
        log_info( "\tfigure_out_what_to_draw - %lf\n", 1e6*PROFILE_QUERY( figure_out_what_to_draw ) );
        log_info( "\tdraw_main - %lf\n", 1e6*PROFILE_QUERY( draw_main ) );
        log_info( "======================================\n" );
        log_info( "break down of update_game() sub-functions\n" );
        log_info( "\tresize_characters - %lf\n", 1e6*PROFILE_QUERY( resize_characters ) );
        log_info( "\tkeep_weapons_with_holders - %lf\n", 1e6*PROFILE_QUERY( keep_weapons_with_holders ) );
        log_info( "\tlet_ai_think - %lf\n", 1e6*PROFILE_QUERY( let_ai_think ) );
        log_info( "\tdo_weather_spawn - %lf\n", 1e6*PROFILE_QUERY( do_weather_spawn ) );
        log_info( "\tdo_enchant_spawn - %lf\n", 1e6*PROFILE_QUERY( enc_spawn_particles ) );
        log_info( "\tcl_unbufferLatches - %lf\n", 1e6*PROFILE_QUERY( ClientState_unbufferLatches ) );
        log_info( "\tsv_unbufferLatches - %lf\n", 1e6*PROFILE_QUERY( sv_unbufferLatches ) );
        log_info( "\tcheck_respawn - %lf\n", 1e6*PROFILE_QUERY( check_respawn ) );
        log_info( "\tmove_characters - %lf\n", 1e6*PROFILE_QUERY( move_characters ) );
        log_info( "\tmove_particles - %lf\n", 1e6*PROFILE_QUERY( move_particles ) );
        log_info( "\tmake_character_matrices - %lf\n", 1e6*PROFILE_QUERY( make_character_matrices ) );
        log_info( "\tattach_particles - %lf\n", 1e6*PROFILE_QUERY( attach_particles ) );
        log_info( "\tmake_onwhichfan - %lf\n", 1e6*PROFILE_QUERY( make_onwhichfan ) );
        log_info( "\tbump_characters - %lf\n", 1e6*PROFILE_QUERY( do_bumping ) );
        log_info( "\tstat_return - %lf\n", 1e6*PROFILE_QUERY( stat_return ) );
        log_info( "\tpit_kill - %lf\n", 1e6*PROFILE_QUERY( pit_kill ) );
        log_info( "\tanimate_tiles - %lf\n", 1e6*PROFILE_QUERY( animate_tiles ) );
        log_info( "\tmove_water - %lf\n", 1e6*PROFILE_QUERY( move_water ) );
        log_info( "======================================\n\n" );

        PROFILE_FREE( resize_characters );
        PROFILE_FREE( keep_weapons_with_holders );
        PROFILE_FREE( let_ai_think );
        PROFILE_FREE( do_weather_spawn );
        PROFILE_FREE( enc_spawn_particles );
        PROFILE_FREE( ClientState_unbufferLatches );
        PROFILE_FREE( sv_unbufferLatches );
        PROFILE_FREE( check_respawn );
        PROFILE_FREE( move_characters );
        PROFILE_FREE( move_particles );
        PROFILE_FREE( make_character_matrices );
        PROFILE_FREE( attach_particles );
        PROFILE_FREE( make_onwhichfan );
        PROFILE_FREE( do_bumping );
        PROFILE_FREE( stat_return );
        PROFILE_FREE( pit_kill );
        PROFILE_FREE( animate_tiles );
        PROFILE_FREE( move_water );
        PROFILE_FREE( figure_out_what_to_draw );
        PROFILE_FREE( draw_main );
        PROFILE_FREE( pre_update_game );
        PROFILE_FREE( update_game );
        PROFILE_FREE( main_loop );

        for(i=0; i<stk->count; i++)
        {
          gs = GSStack_get(stk, i);
          quit_game(gs);
        }
      }

      ego_proc->Active     = bfalse;
      ego_proc->Terminated = btrue;

      ego_proc->returnValue = -1;
      ego_proc->State = PROC_Begin;
      break;
  };

  return ego_proc->returnValue;
};

//--------------------------------------------------------------------------------------------
ProcessStates game_handleKeyboard(GameState * gs, ProcessStates procIn)
{
  ProcessStates procOut = procIn;
  GuiState * gui = Get_GuiState();

  // do game pause if needed
  if (!SDLKEYDOWN(SDLK_F8))  gui->can_pause = btrue;
  if (SDLKEYDOWN(SDLK_F8) && gs->proc.Active && keyb.on && gui->can_pause)
  {
    gs->proc.Paused = !gs->proc.Paused;
    gui->can_pause = bfalse;
  }

  // Check for quitters
  if ( (gs->proc.Active && SDLKEYDOWN(SDLK_ESCAPE)) || (gs->modstate.Active && gs->allpladead) )
  {
    gs->igm.proc.Active = btrue;
    gs->igm.whichMenu  = mnu_Quit;
  }

  return procOut;
}

//--------------------------------------------------------------------------------------------
void game_handleIO(GameState * gs)
{
  set_local_latches( gs );                   // client function

  // NETWORK PORT
  if(net_Started())
  {
    // buffer the existing latches
    input_net_message(gs);        // client function
  }

  // this needs to be done even if the network is not on
  ClientState_bufferLatches(gs->al_cs);     // client function

  if(net_Started())
  {
    ServerState_bufferLatches(gs->al_ss);     // server function

    // upload the information
    ClientState_talkToHost(gs->al_cs);        // client function
    sv_talkToRemotes(gs->al_ss);     // server function
  } 
};

//--------------------------------------------------------------------------------------------
void cl_update_game(GameState * gs, float dUpdate, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.
  int cnt;

  // exactly one iteration
  RANDIE(gs);

  count_players(gs);

  // This is the main game loop
  gs->al_cs->msg_timechange = 0;

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync
  while ( (gs->wld_clock<gs->all_clock) && (gs->al_cs->tlb.numtimes > 0))
  {
    srand(gs->al_ss->rand_idx);                          // client/server function

    PROFILE_BEGIN( resize_characters );
    resize_characters( gs, dUpdate );                 // client function
    PROFILE_END( resize_characters );

    PROFILE_BEGIN( keep_weapons_with_holders );
    keep_weapons_with_holders( gs );                  // client-ish function
    PROFILE_END( keep_weapons_with_holders );

    // unbuffer the updated latches after let_ai_think() and before move_characters()
    PROFILE_BEGIN( ClientState_unbufferLatches );
    ClientState_unbufferLatches( gs->al_cs );              // client function
    PROFILE_END( ClientState_unbufferLatches );

    PROFILE_BEGIN( check_respawn );
    check_respawn( gs );                         // client function
    despawn_characters( gs );
    despawn_particles( gs );
    PROFILE_END( check_respawn );

    PROFILE_BEGIN( make_onwhichfan );
    make_onwhichfan( gs );                      // client/server function
    PROFILE_END( make_onwhichfan );

    // client/server block
    begin_integration( gs );
    {
      PROFILE_BEGIN( move_characters );
      move_characters( gs, dUpdate );
      PROFILE_END( move_characters );

      PROFILE_BEGIN( move_particles );
      move_particles( gs, dUpdate );
      PROFILE_END( move_particles );

      PROFILE_BEGIN( attach_particles );
      attach_particles( gs );                      // client/server function
      PROFILE_END( attach_particles );

      PROFILE_BEGIN( do_bumping );
      do_bumping( gs, dUpdate );                   // client/server function
      PROFILE_END( do_bumping );

    }
    do_integration(gs, dUpdate);

    PROFILE_BEGIN( make_character_matrices );
    make_character_matrices( gs->ChrList, MAXCHR );                // client function
    PROFILE_END( make_character_matrices );

    PROFILE_BEGIN( stat_return );
    stat_return( gs, dUpdate );                           // client/server function
    PROFILE_END( stat_return );

    PROFILE_BEGIN( pit_kill );
    pit_kill( gs, dUpdate );                           // client/server function
    PROFILE_END( pit_kill );

    {
      // Generate the new seed
      gs->al_ss->rand_idx += *((Uint32*) & kMd2Normals[gs->wld_frame&127][0]);
      gs->al_ss->rand_idx += *((Uint32*) & kMd2Normals[gs->al_ss->rand_idx&127][1]);
    }

    // Stuff for which sync doesn't matter
    //flash_select();                                    // client function

    PROFILE_BEGIN( animate_tiles );
    animate_tiles( dUpdate );                          // client function
    PROFILE_END( animate_tiles );

    PROFILE_BEGIN( move_water );
    move_water( dUpdate );                            // client function
    PROFILE_END( move_water );


    // Timers
    gs->wld_clock += FRAMESKIP;
    gs->wld_frame++;

    gs->al_cs->msg_timechange++;

    for(cnt=0; cnt<MAXSTAT; cnt++)
    {
      if (gs->al_cs->StatList[cnt].delay > 0)  
      {
        gs->al_cs->StatList[cnt].delay--;
      }
    };

    gs->al_cs->stat_clock++;
  }

  if (net_Started())
  {
    if (gs->al_cs->tlb.numtimes == 0)
    {
      // The remote ran out of messages, and is now twiddling its thumbs...
      // Make it go slower so it doesn't happen again
      gs->wld_clock += FRAMESKIP/4.0f;
    }
    else if (cl_Started() && gs->al_cs->tlb.numtimes > 3)
    {
      // The host has too many messages, and is probably experiencing control
      // gs->cd->lag...  Speed it up so it gets closer to sync
      gs->wld_clock -= FRAMESKIP/4.0f;
    }
  }
}

//--------------------------------------------------------------------------------------------
void sv_update_game(GameState * gs, float dUpdate, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.


  RANDIE(gs);

  count_players(gs);

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync

  // gs->wld_clock<gs->all_clock  will this work with multiple game states?
  while ( gs->wld_clock<gs->all_clock )
  {
    srand(gs->al_ss->rand_idx);                          // client/server function

    PROFILE_BEGIN( keep_weapons_with_holders );
    keep_weapons_with_holders( gs );                  // client-ish function
    PROFILE_END( keep_weapons_with_holders );

    PROFILE_BEGIN( let_ai_think );
    let_ai_think( gs, dUpdate );                      // server function
    PROFILE_END( let_ai_think );

    PROFILE_BEGIN( do_weather_spawn );
    do_weather_spawn( gs, dUpdate );                       // server function
    PROFILE_END( do_weather_spawn );

    PROFILE_BEGIN( enc_spawn_particles );
    enc_spawn_particles( gs, dUpdate );                       // server function
    PROFILE_END( enc_spawn_particles );

    // unbuffer the updated latches after let_ai_think() and before move_characters()
    PROFILE_BEGIN( sv_unbufferLatches );
    sv_unbufferLatches( gs->al_ss );              // server function
    PROFILE_END( sv_unbufferLatches );

    PROFILE_BEGIN( make_onwhichfan );
    make_onwhichfan( gs );                      // client/server function
    PROFILE_END( make_onwhichfan );


   // client/server block
    begin_integration( gs );
    {
      PROFILE_BEGIN( move_characters );
      move_characters( gs, dUpdate );
      PROFILE_END( move_characters );

      PROFILE_BEGIN( move_particles );
      move_particles( gs, dUpdate );
      PROFILE_END( move_particles );

      PROFILE_BEGIN( attach_particles );
      attach_particles( gs );                      // client/server function
      PROFILE_END( attach_particles );

      PROFILE_BEGIN( do_bumping );
      do_bumping( gs, dUpdate );                   // client/server function
      PROFILE_END( do_bumping );

    }
    do_integration(gs, dUpdate);

    PROFILE_BEGIN( stat_return );
    stat_return( gs, dUpdate );                           // client/server function
    PROFILE_END( stat_return );

    PROFILE_BEGIN( pit_kill );
    pit_kill( gs, dUpdate );                           // client/server function
    PROFILE_END( pit_kill );

    {
      // Generate the new seed
      gs->al_ss->rand_idx += *((Uint32*) & kMd2Normals[gs->wld_frame&127][0]);
      gs->al_ss->rand_idx += *((Uint32*) & kMd2Normals[gs->al_ss->rand_idx&127][1]);
    }

    // Timers
    gs->wld_clock += FRAMESKIP;
    gs->wld_frame++;
  }

}

//--------------------------------------------------------------------------------------------
void update_game(GameState * gs, float dUpdate, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.

  int cnt;
  ClientState * cs = gs->al_cs;
  ServerState * ss = gs->al_ss;

  RANDIE(gs);

  count_players(gs);

  // This is the main game loop
  cs->msg_timechange = 0;

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync

  // gs->wld_clock<gs->all_clock  will this work with multiple game states?
  while ( (gs->wld_clock<gs->all_clock) &&  ( cs->tlb.numtimes > 0 ) )
  {
    srand(ss->rand_idx);                          // client/server function

    PROFILE_BEGIN( resize_characters );
    resize_characters( gs, dUpdate );                 // client function
    PROFILE_END( resize_characters );

    PROFILE_BEGIN( keep_weapons_with_holders );
    keep_weapons_with_holders( gs );                  // client-ish function
    PROFILE_END( keep_weapons_with_holders );

    PROFILE_BEGIN( let_ai_think );
    let_ai_think( gs, dUpdate );                      // server function
    PROFILE_END( let_ai_think );

    PROFILE_BEGIN( do_weather_spawn );
    do_weather_spawn( gs, dUpdate );                       // server function
    PROFILE_END( do_weather_spawn );

    PROFILE_BEGIN( enc_spawn_particles );
    enc_spawn_particles( gs, dUpdate );                       // server function
    PROFILE_END( enc_spawn_particles );

    // unbuffer the updated latches after let_ai_think() and before move_characters()
    PROFILE_BEGIN( ClientState_unbufferLatches );
    ClientState_unbufferLatches( cs );              // client function
    PROFILE_END( ClientState_unbufferLatches );

    PROFILE_BEGIN( sv_unbufferLatches );
    sv_unbufferLatches( ss );              // server function
    PROFILE_END( sv_unbufferLatches );

    PROFILE_BEGIN( check_respawn );
    check_respawn( gs );                         // client function
    despawn_characters(gs);
    despawn_particles(gs);
    PROFILE_END( check_respawn );

    PROFILE_BEGIN( make_onwhichfan );
    make_onwhichfan( gs );                      // client/server function
    PROFILE_END( make_onwhichfan );

    // client/server block
    begin_integration( gs );
    {
      PROFILE_BEGIN( move_characters );
      move_characters( gs, dUpdate );
      PROFILE_END( move_characters );

      PROFILE_BEGIN( move_particles );
      move_particles( gs, dUpdate );
      PROFILE_END( move_particles );

      PROFILE_BEGIN( attach_particles );
      attach_particles( gs );                      // client/server function
      PROFILE_END( attach_particles );

      PROFILE_BEGIN( do_bumping );
      do_bumping( gs, dUpdate );                   // client/server function
      PROFILE_END( do_bumping );
    }
    do_integration(gs, dUpdate);

    PROFILE_BEGIN( make_character_matrices );
    make_character_matrices( gs->ChrList, MAXCHR );                // client function
    PROFILE_END( make_character_matrices );

    PROFILE_BEGIN( stat_return );
    stat_return( gs, dUpdate );                           // client/server function
    PROFILE_END( stat_return );

    PROFILE_BEGIN( pit_kill );
    pit_kill( gs, dUpdate );                           // client/server function
    PROFILE_END( pit_kill );

    {
      // Generate the new seed
      ss->rand_idx += *((Uint32*) & kMd2Normals[gs->wld_frame&127][0]);
      ss->rand_idx += *((Uint32*) & kMd2Normals[ss->rand_idx&127][1]);
    }

    // Stuff for which sync doesn't matter
    //flash_select();                          // client function

    PROFILE_BEGIN( animate_tiles );
    animate_tiles( dUpdate );                          // client function
    PROFILE_END( animate_tiles );

    PROFILE_BEGIN( move_water );
    move_water( dUpdate );                          // client function
    PROFILE_END( move_water );

    // Timers
    gs->wld_clock += FRAMESKIP;
    gs->wld_frame++;

    cs->msg_timechange++;

    for(cnt=0; cnt<MAXSTAT; cnt++)
    {
      if (cs->StatList[cnt].delay > 0)  
      {
        cs->StatList[cnt].delay--;
      }
    };

    cs->stat_clock++;
  }

  if (net_Started())
  {
    if (cs->tlb.numtimes == 0)
    {
      // The remote ran out of messages, and is now twiddling its thumbs...
      // Make it go slower so it doesn't happen again
      gs->wld_clock += FRAMESKIP/4.0f;
    }
    else if (cl_Started() && cs->tlb.numtimes > 3)
    {
      // The host has too many messages, and is probably experiencing control
      // gs->cd->lag...  Speed it up so it gets closer to sync
      gs->wld_clock -= FRAMESKIP/4.0f;
    }
  }
}

//--------------------------------------------------------------------------------------------
ProcessStates game_doRun(GameState * gs, ProcessStates procIn)
{
  ProcessStates procOut = procIn;
  static double dTimeUpdate, dTimeFrame;

  bool_t client_running = bfalse, server_running = bfalse, local_running = bfalse;

  if (NULL==gs)  return procOut;

  client_running = ClientState_Running(gs->al_cs);
  server_running = sv_Running(gs->al_ss);
  local_running  = !client_running && !server_running;

  update_timers( gs );

#if DEBUG_UPDATE_CLAMP && defined(_DEBUG)
  if(CData.DevMode)
  {
    log_info( "gs->wld_frame == %d\t dframe == %2.4f\n", gs->wld_frame, gs->dUpdate );
  };
#endif

  procOut = game_handleKeyboard(gs, procOut);

  while ( gs->dUpdate >= 1.0 )
  {
    // Do important things
    if ( !local_running && !client_running && !server_running )
    {
      gs->wld_clock = gs->all_clock;
    }
    else if ( gs->Single_frame || (gs->proc.Paused && !server_running && (local_running || client_running)) )
    {
      gs->wld_clock = gs->all_clock;
    }
    else
    {
      // all of this stupp should only be done for the gfxState.gs game
      if( gs == gfxState.gs)
      {
        PROFILE_BEGIN( pre_update_game );
        {
          update_timers( gs );

          check_passage_music( gs );
          update_looped_sounds( gs );

          // handle the game and network IO
          game_handleIO(gs);
        }
        PROFILE_END( pre_update_game );
      }

      PROFILE_BEGIN( update_game );
      if(local_running)
      {
        // non-networked updates
        update_game(gs, EMULATEUPS / TARGETUPS, &(gs->randie_index) );
      }
      else if(server_running)
      {
        // server-only updated
        sv_update_game(gs, EMULATEUPS / TARGETUPS, &(gs->randie_index));
      }
      else if(client_running)
      {
        // client only updates
        cl_update_game(gs, EMULATEUPS / TARGETUPS, &(gs->randie_index));
      }
      else
      {
        log_warning("game_doRun() - invalid server/client/local game configuration?");
      }
      PROFILE_END( update_game );
    }

#if DEBUG_UPDATE_CLAMP && defined(_DEBUG)
    if(CData.DevMode)
    {
      log_info( "\t\twldframe == %d\t dframe == %2.4f\n", gs->wld_frame, gs->dUpdate );
    }
#endif

    gs->dUpdate -= 1.0f;
    ups_loops++;
  };

  return procOut;
}

//--------------------------------------------------------------------------------------------
// TODO : this should be re-factored so that graphics updates are in a different loop
int proc_gameLoop( ProcState * gproc, GameState * gs )
{
  // if we are being told to exit, jump to PROC_Leaving
  double frameDuration, frameTicks;  
  GuiState * gui = Get_GuiState();

  if(NULL == gproc || gproc->Terminated)
  {
    gproc->returnValue = -1;
    return gproc->returnValue;
  }

  if(gproc->KillMe && gproc->State < PROC_Leaving) 
  {
    gproc->Active = btrue;
    gproc->Paused = bfalse;
    gproc->State = PROC_Leaving;
  }

  gproc->returnValue = 0;
  if(!gproc->Active || gproc->Paused)
  {
    return gproc->returnValue;
  }

  // update this game state's clock clock
  ClockState_frameStep( gproc->clk );
  frameDuration = ClockState_getFrameDuration( gproc->clk );
  frameTicks    = frameDuration * TICKS_PER_SEC;

  // frameDuration is given in seconds, convert to ticks (ticks == milliseconds on win32)
  frameTicks = frameDuration * TICKS_PER_SEC;

  gproc->returnValue = 0;
  switch ( gproc->State )
  {
    case PROC_Begin:
      {
        retval_t mod_return;

        // Start a new module
        srand( time(NULL) );

        // set up the MD2 models
        init_all_models(gs);

        log_info( "proc_gameLoop() - Starting to load module %s.\n", gs->mod.loadname);
        mod_return = module_load( gs, gs->mod.loadname );
        log_info( "proc_gameLoop() - Loading module %s... ", gs->mod.loadname);
        if( mod_return )
        {
          log_info("Succeeded!\n");
        }
        else
        {
          log_info("Failed!\n");
          gproc->KillMe = btrue;
          gproc->returnValue = 0;
          return gproc->returnValue;
        }

        make_onwhichfan( gs );
        reset_camera();
        reset_timers( gs );
        figure_out_what_to_draw();
        make_character_matrices( gs->ChrList, MAXCHR );
        attach_particles( gs );

        if ( net_Started() )
        {
          log_info( "SDL_main: Loading module %s...\n", gs->mod.loadname );
          gui->net_messagemode = bfalse;
          GNetMsg.delay = 20;
          net_sayHello( gs->ns );
        }
      }
      gproc->State = PROC_Entering;
      break;

    case PROC_Entering:
      {
        // Let the game go
        gs->modstate.Active = btrue;
        gs->modstate.Paused = bfalse;

        // seed the random number
        gs->al_ss->rand_idx = time(NULL);      // TODO - for a network game, this needs to be set by the server...
        srand( -(Sint32)gs->al_ss->rand_idx );

        // reset the timers
        gs->dFrame = gs->dUpdate = 0.0f;
      }
      gproc->State = PROC_Running;
      break;

    case PROC_Running:

      // update the times

      gs->dFrame += frameTicks / FRAMESKIP;
      if(gs->Single_frame) gs->dFrame = 1.0f;

      gs->dUpdate += frameTicks / UPDATESKIP;
      if(gs->Single_frame) gs->dUpdate = 1.0f;

      if ( !gproc->Active )
      {
        gproc->State = PROC_Leaving;
      }
      else
      {
        PROFILE_BEGIN( main_loop );
        gproc->State = game_doRun(gs, gproc->State);
        PROFILE_END( main_loop );

        gfxState.est_max_fps = 0.9 * gfxState.est_max_fps + 0.1 * (1.0f / PROFILE_QUERY(main_loop) );
      }
      break;

    case PROC_Leaving:
      {
        module_quit(gs);
      }
      gproc->State = PROC_Finish;
      break;

    case PROC_Finish:
      {
        gproc->Active     = bfalse;
        gproc->Terminated = btrue;

        // Let the normal OS mouse cursor work
        SDL_WM_GrabInput( CData.GrabMouse );
        //SDL_ShowCursor(SDL_ENABLE);
      }
      gproc->returnValue = -1;
      gproc->State = PROC_Begin;
      break;
  };

  return gproc->returnValue;
};

//--------------------------------------------------------------------------------------------
int proc_menuLoop( MenuProc  * mproc )
{ 
  GameState * gs;
  ProcState * proc;

  if(NULL == mproc || mproc->proc.Terminated) return -1;

  proc = &(mproc->proc);
  gs   = gfxState.gs;

  if(proc->KillMe && proc->State < PROC_Leaving)
  {
    proc->Active = btrue;
    proc->Paused = bfalse;
    proc->State  = PROC_Leaving;
  }

  proc->returnValue = 0;
  if(!proc->Active || proc->Paused) return proc->returnValue;

  switch ( proc->State )
  {
    case PROC_Begin:
      {
        snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.uifont_ttf );
        ui_initialize( CStringTmp1, CData.uifont_points );

        //Load stuff into memory
        prime_icons();
        prime_titleimage(NULL, 0);
        make_textureoffset();
        initMenus();             //Start the game menu

        // initialize the bitmap font so we can use the cursor
        snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.font_bitmap );
        snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.fontdef_file );
        if ( !load_font( CStringTmp1, CStringTmp2 ) )
        {
          log_warning( "UI unable to use load bitmap font for cursor. Files missing from %s directory\n", CData.basicdat_dir );
        };

        // reset the update timer
        mproc->dUpdate = 0;

        // Let the normal OS mouse cursor work
        SDL_WM_GrabInput( SDL_GRAB_OFF );
        SDL_ShowCursor( SDL_DISABLE );
        mous.on = bfalse;
      }
      proc->State = PROC_Entering;
      break;

    case PROC_Entering:
      {
      }
      proc->State = PROC_Running;
      break;

    case PROC_Running:
      {
        // do menus
        ui_beginFrame();

        mnu_Run( mproc, gs );

        ui_endFrame();

        request_pageflip();
      };
      break;

    case PROC_Leaving:
      {
      }
      proc->State = PROC_Finish;
      break;

    case PROC_Finish:
      {
        proc->Active     = bfalse;
        proc->Terminated = btrue;

        // Let the normal OS mouse cursor work
        SDL_WM_GrabInput( CData.GrabMouse );
        //SDL_ShowCursor(SDL_ENABLE);
      }
      proc->returnValue = -1;
      proc->State = PROC_Begin;
      break;
  };

  return proc->returnValue;
};



//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
  ProcState EgoProc = {bfalse };

  int program_state = 0, i;
  MachineState * mach_state;
  GSStack * stk;
  GameState * gs;

  // Initialize logging first, so that we can use it everywhere.
  log_init();
  log_setLoggingLevel( 2 );

  // force the initialization of the machine state
  mach_state = Get_MachineState();

  // create the main loop "process"
  ProcState_new(&EgoProc);

  // For each game state, run the main loop.
  // Delete any game states that have been killed
  stk = Get_GSStack();
  do
  {
    proc_mainLoop( &EgoProc, argc, argv );

    // always keep 1 game state on the stack
    for(i=1; i<stk->count; i++)
    {
      gs = GSStack_get(stk, i);
      if(gs->proc.Terminated)
      {
        // only kill one per loop, since the deletion process invalidates the stack
        GameState_delete(gs);
        break;
      }
    }

  }
  while ( -1 != EgoProc.returnValue );

  return btrue;
}

//--------------------------------------------------------------------------------------------
void load_all_messages( GameState * gs, char *szObjectpath, char *szObjectname, Uint16 object )
{
  // ZZ> This function loads all of an objects messages

  FILE *fileread;

  gs->MadList[object].msg_start = 0;
  fileread = fs_fileOpen( PRI_NONE, NULL, inherit_fname(szObjectpath, szObjectname, CData.message_file), "r" );
  if ( NULL != fileread )
  {
    gs->MadList[object].msg_start = GMsg.total;
    while ( fget_next_message( fileread ) ) {};

    fs_fileClose( fileread );
  }
}


//--------------------------------------------------------------------------------------------
void update_looped_sounds( GameState * gs )
{
  CHR_REF ichr;

  for(ichr=0; ichr<MAXCHR; ichr++)
  {
    if( !gs->ChrList[ichr].on || INVALID_CHANNEL == gs->ChrList[ichr].loopingchannel ) continue;

    snd_apply_mods( gs->ChrList[ichr].loopingchannel, gs->ChrList[ichr].loopingvolume, gs->ChrList[ichr].pos, GCamera.trackpos, GCamera.turn_lr);
  };

}


//--------------------------------------------------------------------------------------------
SearchInfo * SearchInfo_new(SearchInfo * psearch)
{
  //fprintf( stdout, "SearchInfo_new()\n");

  if(NULL == psearch) return NULL;

  psearch->initialize = btrue;

  psearch->bestdistance = 9999999;
  psearch->besttarget   = MAXCHR;
  psearch->bestangle    = 32768;

  return psearch;
};

//--------------------------------------------------------------------------------------------
bool_t prt_search_wide( GameState * gs, SearchInfo * psearch, PRT_REF iprt, Uint16 facing,
                        Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                        TEAM team, Uint16 donttarget, Uint16 oldtarget )
{
  // This function finds the best target for the given parameters

  int block_x, block_y;
  
  if( !VALID_PRT( gs->PrtList, iprt) ) return bfalse;

  block_x = MESH_FLOAT_TO_BLOCK( gs->PrtList[iprt].pos.x );
  block_y = MESH_FLOAT_TO_BLOCK( gs->PrtList[iprt].pos.y );

  // initialize the search
  SearchInfo_new(psearch);

  prt_search_block( gs, psearch, block_x + 0, block_y + 0, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );

  if ( !VALID_CHR( gs->ChrList, psearch->besttarget ) )
  {
    prt_search_block( gs, psearch, block_x + 1, block_y + 0, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x - 1, block_y + 0, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x + 0, block_y + 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x + 0, block_y - 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  }

  if( !VALID_CHR( gs->ChrList, psearch->besttarget ) )
  {
    prt_search_block( gs, psearch, block_x + 1, block_y + 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x + 1, block_y - 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x - 1, block_y + 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x - 1, block_y - 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  }

  return VALID_CHR( gs->ChrList, psearch->besttarget );
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_block( GameState * gs, SearchInfo * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                         bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                         bool_t invert_idsz )
{
  // ZZ> This is a good little helper. returns btrue if a suitable target was found

  int cnt;
  CHR_REF charb;
  Uint32 fanblock, blnode_b;
  TEAM team;
  vect3 diff;
  float dist;

  bool_t require_friends =  ask_friends && !ask_enemies;
  bool_t require_enemies = !ask_friends &&  ask_enemies;
  bool_t require_alive   = !ask_dead;
  bool_t require_noitems = !ask_items;
  bool_t ballowed;

  if ( !VALID_CHR( gs->ChrList, character ) || !bumplist.filled ) return bfalse;
  team     = gs->ChrList[character].team;

  fanblock = mesh_convert_block( &(gs->mesh), block_x, block_y );
  for ( cnt = 0, blnode_b = bumplist_get_chr_head( &bumplist, fanblock );
        cnt < bumplist_get_chr_count(&bumplist, fanblock) && INVALID_BUMPLIST_NODE != blnode_b;
        cnt++, blnode_b = bumplist_get_next_chr( gs, &bumplist, blnode_b ) )
  {
    charb = bumplist_get_ref( &bumplist, blnode_b );
    assert( VALID_CHR( gs->ChrList, charb ) );

    // don't find stupid stuff
    if ( !VALID_CHR( gs->ChrList, charb ) || 0.0f == gs->ChrList[charb].bumpstrength ) continue;

    // don't find yourself or any of the items you're holding
    if ( character == charb || gs->ChrList[charb].attachedto == character || gs->ChrList[charb].inwhichpack == character ) continue;

    // don't find your mount or your master
    if ( gs->ChrList[character].attachedto == charb || gs->ChrList[character].inwhichpack == charb ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( gs->ChrList, MAXCHR, charb ) ) || chr_in_pack( gs->ChrList, MAXCHR, charb ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && gs->TeamList[team].hatesteam[gs->ChrList[charb].team] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !gs->TeamList[team].hatesteam[gs->ChrList[charb].team] || gs->ChrList[charb].invictus ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !gs->ChrList[charb].alive ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && gs->ChrList[charb].isitem ) continue;

    ballowed = bfalse;
    if ( IDSZ_NONE == idsz )
    {
      ballowed = btrue;
    }
    else
    {
      bool_t found_idsz = CAP_INHERIT_IDSZ( gs,  gs->ChrList[charb].model, idsz );
      ballowed = (invert_idsz != found_idsz);
    }

    if ( ballowed )
    {
      diff.x = gs->ChrList[character].pos.x - gs->ChrList[charb].pos.x;
      diff.y = gs->ChrList[character].pos.y - gs->ChrList[charb].pos.y;
      diff.z = gs->ChrList[character].pos.z - gs->ChrList[charb].pos.z;

      dist = DotProduct(diff, diff);
      if ( psearch->initialize || dist < psearch->bestdistance )
      {
        psearch->bestdistance = dist;
        psearch->besttarget   = charb;
        psearch->initialize   = bfalse;
      }
    }

  }

  return VALID_CHR( gs->ChrList, psearch->besttarget);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_nearby( GameState * gs, SearchInfo * psearch, CHR_REF character, bool_t ask_items,
                          bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz )
{
  // ZZ> This function finds a target from the blocks that the character is overlapping. Returns btrue if found.

  int ix,ix_min,ix_max, iy,iy_min,iy_max;
  bool_t seeinvisible;

  if ( !VALID_CHR( gs->ChrList, character ) || chr_in_pack( gs->ChrList, MAXCHR, character ) ) return bfalse;

  // initialize the search
  SearchInfo_new(psearch);

  // grab seeinvisible from the character
  seeinvisible = gs->ChrList[character].canseeinvisible;

  // Current fanblock
  ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( &(gs->mesh), gs->ChrList[character].bmpdata.cv.x_min ) );
  ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( &(gs->mesh), gs->ChrList[character].bmpdata.cv.x_max ) );
  iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( &(gs->mesh), gs->ChrList[character].bmpdata.cv.y_min ) );
  iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( &(gs->mesh), gs->ChrList[character].bmpdata.cv.y_max ) );

  for( ix = ix_min; ix<=ix_max; ix++ )
  {
    for( iy=iy_min; iy<=iy_max; iy++ )
    {
      chr_search_block( gs, psearch, ix, iy, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, ask_idsz, bfalse );
    };
  };

  if ( psearch->besttarget==character )
  {
    psearch->besttarget = MAXCHR;
  }

  return VALID_CHR( gs->ChrList, psearch->besttarget);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_distant( GameState * gs, SearchInfo * psearch, CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead )
{
  // ZZ> This function finds a target, or it returns bfalse if it can't find one...
  //     maxdist should be the square of the actual distance you want to use
  //     as the cutoff...

  int charb, xdist, ydist, zdist;
  bool_t require_alive = !ask_dead;
  TEAM team;

  if ( !VALID_CHR( gs->ChrList, character ) ) return bfalse;

  SearchInfo_new(psearch);

  team = gs->ChrList[character].team;
  psearch->bestdistance = maxdist;
  for ( charb = 0; charb < MAXCHR; charb++ )
  {
    // don't find stupid items
    if ( !VALID_CHR( gs->ChrList, charb ) || 0.0f == gs->ChrList[charb].bumpstrength ) continue;

    // don't find yourself or items you are carrying
    if ( character == charb || gs->ChrList[charb].attachedto == character || gs->ChrList[charb].inwhichpack == character ) continue;

    // don't find thigs you can't see
    if (( !gs->ChrList[character].canseeinvisible && chr_is_invisible( gs->ChrList, MAXCHR, charb ) ) || chr_in_pack( gs->ChrList, MAXCHR, charb ) ) continue;

    // don't find dead things if not asked for
    if ( require_alive && ( !gs->ChrList[charb].alive || gs->ChrList[charb].isitem ) ) continue;

    // don't find enemies unless asked for
    if ( ask_enemies && ( !gs->TeamList[team].hatesteam[gs->ChrList[charb].team] || gs->ChrList[charb].invictus ) ) continue;

    xdist = gs->ChrList[charb].pos.x - gs->ChrList[character].pos.x;
    ydist = gs->ChrList[charb].pos.y - gs->ChrList[character].pos.y;
    zdist = gs->ChrList[charb].pos.z - gs->ChrList[character].pos.z;
    psearch->bestdistance = xdist * xdist + ydist * ydist + zdist * zdist;

    if ( psearch->bestdistance < psearch->bestdistance )
    {
      psearch->bestdistance = psearch->bestdistance;
      psearch->besttarget   = charb;
    };
  }

  return VALID_CHR( gs->ChrList, psearch->besttarget);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_block_nearest( GameState * gs, SearchInfo * psearch, int block_x, int block_y, CHR_REF chra_ref, bool_t ask_items,
                               bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz )
{
  // ZZ> This is a good little helper

  float dis, xdis, ydis, zdis;
  int   cnt;
  TEAM    team;
  CHR_REF chrb_ref;
  Uint32  fanblock, blnode_b;
  bool_t require_friends =  ask_friends && !ask_enemies;
  bool_t require_enemies = !ask_friends &&  ask_enemies;
  bool_t require_alive   = !ask_dead;
  bool_t require_noitems = !ask_items;

  // if chra_ref is not defined, return
  if ( !VALID_CHR( gs->ChrList, chra_ref ) || !bumplist.filled ) return bfalse;

  // blocks that are off the mesh are not stored
  fanblock = mesh_convert_block( &(gs->mesh), block_x, block_y );

  team = gs->ChrList[chra_ref].team;
  for ( cnt = 0, blnode_b = bumplist_get_chr_head(&bumplist, fanblock); 
        cnt < bumplist_get_chr_count(&bumplist, fanblock) && INVALID_BUMPLIST_NODE != blnode_b; 
        cnt++, blnode_b = bumplist_get_next_chr(gs, &bumplist, blnode_b) )
  {
    chrb_ref = bumplist_get_ref(&bumplist, blnode_b);
    VALID_CHR( gs->ChrList, chrb_ref );

    // don't find stupid stuff
    if ( !VALID_CHR( gs->ChrList, chrb_ref ) || 0.0f == gs->ChrList[chrb_ref].bumpstrength ) continue;

    // don't find yourself or any of the items you're holding
    if ( chra_ref == chrb_ref || gs->ChrList[chrb_ref].attachedto == chra_ref || gs->ChrList[chrb_ref].inwhichpack == chra_ref ) continue;

    // don't find your mount or your master
    if ( gs->ChrList[chra_ref].attachedto == chrb_ref || gs->ChrList[chra_ref].inwhichpack == chrb_ref ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( gs->ChrList, MAXCHR, chrb_ref ) ) || chr_in_pack( gs->ChrList, MAXCHR, chrb_ref ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && gs->TeamList[team].hatesteam[gs->ChrList[chrb_ref].team] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !gs->TeamList[team].hatesteam[gs->ChrList[chrb_ref].team] || gs->ChrList[chrb_ref].invictus ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !gs->ChrList[chrb_ref].alive ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && gs->ChrList[chrb_ref].isitem ) continue;

    if ( IDSZ_NONE == idsz || CAP_INHERIT_IDSZ( gs,  gs->ChrList[chrb_ref].model, idsz ) )
    {
      xdis = gs->ChrList[chra_ref].pos.x - gs->ChrList[chrb_ref].pos.x;
      ydis = gs->ChrList[chra_ref].pos.y - gs->ChrList[chrb_ref].pos.y;
      zdis = gs->ChrList[chra_ref].pos.z - gs->ChrList[chrb_ref].pos.z;
      xdis *= xdis;
      ydis *= ydis;
      zdis *= zdis;
      dis = xdis + ydis + zdis;
      if ( psearch->initialize || dis < psearch->distance )
      {
        psearch->nearest  = chrb_ref;
        psearch->distance = dis;
        psearch->initialize = bfalse;
      }
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_wide_nearest( GameState * gs, SearchInfo * psearch, CHR_REF chr_ref, bool_t ask_items,
                                 bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz )
{
  // ZZ> This function finds an target, or it returns MAXCHR if it can't find one

  int x, y;
  bool_t seeinvisible = gs->ChrList[chr_ref].canseeinvisible;

  if ( !VALID_CHR( gs->ChrList, chr_ref ) ) return bfalse;

  // Current fanblock
  x = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].pos.x );
  y = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].pos.y );

  // initialize the search
  SearchInfo_new(psearch);

  chr_search_block_nearest( gs, psearch, x + 0, y + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );

  if ( !VALID_CHR( gs->ChrList, psearch->nearest ) )
  {
    chr_search_block_nearest( gs, psearch, x - 1, y + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 1, y + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 0, y - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 0, y + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( !VALID_CHR( gs->ChrList, psearch->nearest ) )
  {
    chr_search_block_nearest( gs, psearch, x - 1, y + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 1, y - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x - 1, y - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 1, y + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( psearch->nearest == chr_ref )
    psearch->nearest = MAXCHR;

  return VALID_CHR( gs->ChrList, psearch->nearest);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_wide( GameState * gs, SearchInfo * psearch, CHR_REF chr_ref, bool_t ask_items,
                        bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid )
{
  // ZZ> This function finds an object, or it returns bfalse if it can't find one

  int     ix, iy;
  bool_t  seeinvisible;
  bool_t  found;

  if ( !VALID_CHR( gs->ChrList, chr_ref ) ) return MAXCHR;

  // make sure the search context is clear
  SearchInfo_new(psearch);

  // get seeinvisible from the chr_ref
  seeinvisible = gs->ChrList[chr_ref].canseeinvisible;

  // Current fanblock
  ix = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].pos.x );
  iy = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].pos.y );

  chr_search_block( gs, psearch, ix + 0, iy + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  found = VALID_CHR( gs->ChrList, psearch->besttarget ) && (psearch->besttarget != chr_ref);

  if( !found )
  {
    chr_search_block( gs, psearch, ix - 1, iy + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 1, iy + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 0, iy - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 0, iy + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    found = VALID_CHR( gs->ChrList, psearch->besttarget ) && (psearch->besttarget != chr_ref);
  }

  if( !found )
  {
    chr_search_block( gs, psearch, ix - 1, iy + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 1, iy - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix - 1, iy - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 1, iy + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    found = VALID_CHR( gs->ChrList, psearch->besttarget ) && (psearch->besttarget != chr_ref);
  }

  return found;
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( GameState * gs, PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset )
{
  // ZZ> This function sets one particle's position to be attached to a character.
  //     It will kill the particle if the character is no longer around

  Uint16 vertex, model;
  float flip;
  GLvector point, nupoint;
  Prt * pprt;
  Chr * pchr;

  // Check validity of attachment
  if ( !VALID_CHR( gs->ChrList, chr_ref ) || chr_in_pack( gs->ChrList, MAXCHR, chr_ref ) || !VALID_PRT( gs->PrtList,  particle ) )
  {
    gs->PrtList[particle].gopoof = btrue;
    return;
  }

  pprt = gs->PrtList + particle;
  pchr = gs->ChrList + chr_ref;

  // Do we have a matrix???
  if ( !pchr->matrixvalid )
  {
    // No matrix, so just wing it...

    pprt->pos.x = pchr->pos.x;
    pprt->pos.y = pchr->pos.y;
    pprt->pos.z = pchr->pos.z;
  }
  else   if ( vertoffset == GRIP_ORIGIN )
  {
    // Transform the origin to world space

    pprt->pos.x = pchr->matrix.CNV( 3, 0 );
    pprt->pos.y = pchr->matrix.CNV( 3, 1 );
    pprt->pos.z = pchr->matrix.CNV( 3, 2 );
  }
  else
  {
    // Transform the grip vertex position to world space

    Uint32      ilast, inext;
    Mad       * pmad;
    MD2_Model * pmdl;
    MD2_Frame * plast, * pnext;

    model = pchr->model;
    inext = pchr->anim.next;
    ilast = pchr->anim.last;
    flip  = pchr->anim.flip;

    assert( MAXMODEL != VALIDATE_MDL( model ) );

    pmad = gs->MadList + model;
    pmdl  = pmad->md2_ptr;
    plast = md2_get_Frame(pmdl, ilast);
    pnext = md2_get_Frame(pmdl, inext);

    //handle possible invalid values
    vertex = pmad->vertices - vertoffset;
    if(vertoffset >= pmad->vertices)
    {
      vertex = pmad->vertices - GRIP_LAST;
    }

    // Calculate grip point locations with linear interpolation and other silly things
    if ( inext == ilast )
    {
      point.x = plast->vertices[vertex].x;
      point.y = plast->vertices[vertex].y;
      point.z = plast->vertices[vertex].z;
      point.w = 1.0f;
    }
    else
    {
      point.x = plast->vertices[vertex].x + ( pnext->vertices[vertex].x - plast->vertices[vertex].x ) * flip;
      point.y = plast->vertices[vertex].y + ( pnext->vertices[vertex].y - plast->vertices[vertex].y ) * flip;
      point.z = plast->vertices[vertex].z + ( pnext->vertices[vertex].z - plast->vertices[vertex].z ) * flip;
      point.w = 1.0f;
    }

    // Do the transform
    Transform4_Full( 1.0f, 1.0f, &(pchr->matrix), &point, &nupoint, 1 );

    pprt->pos.x = nupoint.x;
    pprt->pos.y = nupoint.y;
    pprt->pos.z = nupoint.z;
  }



}


//--------------------------------------------------------------------------------------------
bool_t load_all_music_sounds(SoundState * ss, ConfigData * cd)
{
  //ZF> This function loads all of the music sounds
  STRING songname;
  FILE *playlist;
  Uint8 cnt;

  if( NULL == ss || NULL == cd ) return bfalse;

  // reset the music
  if(ss->music_loaded)
  {
    snd_unload_music(ss);
  }

  //Open the playlist listing all music files
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->music_dir, cd->playlist_file );
  playlist = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( playlist == NULL )
  {
    log_warning( "Error opening \"%s\"\n", cd->playlist_file );
    return bfalse;
  }

  // Load the music data into memory
  cnt = 0;
  while ( cnt < MAXPLAYLISTLENGTH && !feof( playlist ) )
  {
    fget_next_string( playlist, songname, sizeof( songname ) );
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->music_dir, songname );
    ss->mus_list[cnt] = Mix_LoadMUS( CStringTmp1 );
    cnt++;
  }

  fs_fileClose( playlist );

  ss->music_loaded = btrue;

  return ss->music_loaded;
}

//--------------------------------------------------------------------------------------------
void sdlinit( GraphicState * g )
{
  int cnt;
  SDL_Surface *theSurface;
  STRING strbuffer = { '\0' };

  log_info("Initializing main SDL services version %i.%i.%i... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) < 0 )
  {
    log_message("Failed!\n");
    log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
  }
  else
  {
    log_message("Succeeded!\n");
  }

  atexit( SDL_Quit );

  // log the video driver info
  SDL_VideoDriverName( strbuffer, 256 );
  log_info( "Using Video Driver - %s\n", strbuffer );

  g->colordepth = g->scrd / 3;

  /* Setup the cute windows manager icon */
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.icon_bitmap );
  theSurface = SDL_LoadBMP( CStringTmp1 );
  if ( theSurface == NULL )
  {
    log_warning( "Unable to load icon (%s)\n", CStringTmp1 );
  }
  SDL_WM_SetIcon( theSurface, NULL );

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

  // We can now do a valid anisotropy calculation
  gfx_find_anisotropy( g );

  //Grab all the available video modes
  g->video_mode_list = SDL_ListModes( g->surface->format, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN | SDL_OPENGL | SDL_HWACCEL | SDL_SRCALPHA );
  log_info( "Detecting avalible video modes...\n" );
  for ( cnt = 0; NULL != g->video_mode_list[cnt]; ++cnt )
  {
    log_info( "Video Mode - %d x %d\n", g->video_mode_list[cnt]->w, g->video_mode_list[cnt]->h );
  };

  // Set the window name
  SDL_WM_SetCaption( "Egoboo", "Egoboo" );

  // set the mouse cursor
  SDL_WM_GrabInput( g->GrabMouse );
  //if (g->HideMouse) SDL_ShowCursor(SDL_DISABLE);

  input_setup();
}

//--------------------------------------------------------------------------------------------
MachineState * Get_MachineState()
{
  if(!_macState.initialized)
  {
    MachineState_new(&_macState);
  }

  return &_macState;
}

//--------------------------------------------------------------------------------------------
MachineState * MachineState_new( MachineState * ms )
{
  //fprintf( stdout, "MachineState_new()\n");

  if(NULL == ms || ms->initialized) return ms;

  memset(ms, 0, sizeof(MachineState));

  // initialize the system-dependent functions
  sys_initialize();

  // initialize the filesystem
  fs_init();

  // set up the clock
  ms->clk = ClockState_create("MachineState", -1);

  ms->initialized = btrue;

  return ms;
}

//--------------------------------------------------------------------------------------------
bool_t MachineState_delete( MachineState * ms )
{
  if(NULL == ms) return bfalse;
  if(!ms->initialized) return btrue;

  ClockState_destroy( &(ms->clk) );

  sys_shutdown();

  ms->initialized = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
void setup_characters( GameState * gs, char *modname )
{
  // ZZ> This function sets up character data, loaded from "SPAWN.TXT"

  CHR_REF currentcharacter = MAXCHR, lastcharacter = MAXCHR, tmpchr = MAXCHR;
  int passage, content, money, level, skin, tnc, localnumber = 0;
  bool_t stat, ghost;
  TEAM team;
  Uint8 cTmp;
  char *name;
  bool_t itislocal;
  STRING myname, newloadname;
  Uint16 facing, attach;
  Uint32 slot;
  vect3 pos;
  FILE *fileread;
  GRIP grip;

  // Turn some back on
  currentcharacter = MAXCHR;
  snprintf( newloadname, sizeof( newloadname ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.spawn_file );
  fileread = fs_fileOpen( PRI_WARN, "setup_characters()", newloadname, "r" );
  if ( NULL == fileread )
  {
    log_error( "Error loading module: %s\n", newloadname );  //Something went wrong
    return;
  }


  while ( fget_next_string( fileread, myname, sizeof( myname ) ) )
  {
    str_convert_underscores( myname, sizeof( myname ), myname );

    name = myname;
    if ( 0 == strcmp( "NONE", myname ) )
    {
      // Random name
      name = NULL;
    }

    fscanf( fileread, "%d", &slot );

    fscanf( fileread, "%f%f%f", &pos.x, &pos.y, &pos.z );
    pos.x = MESH_FAN_TO_FLOAT( pos.x );
    pos.y = MESH_FAN_TO_FLOAT( pos.y );
    pos.z = MESH_FAN_TO_FLOAT( pos.z );

    cTmp = fget_first_letter( fileread );
    attach = MAXCHR;
    facing = NORTH;
    grip = GRIP_SADDLE;
    switch ( toupper( cTmp ) )
    {
      case 'N':  facing = NORTH; break;
      case 'S':  facing = SOUTH; break;
      case 'E':  facing = EAST; break;
      case 'W':  facing = WEST; break;
      case 'L':  attach = currentcharacter; grip = GRIP_LEFT;      break;
      case 'R':  attach = currentcharacter; grip = GRIP_RIGHT;     break;
      case 'I':  attach = currentcharacter; grip = GRIP_INVENTORY; break;
    };
    fscanf( fileread, "%d%d%d%d%d", &money, &skin, &passage, &content, &level );
    stat = fget_bool( fileread );
    ghost = fget_bool( fileread );
    team = fget_first_letter( fileread ) - 'A';
    if ( team > TEAM_COUNT ) team = TEAM_NULL;


    // Spawn the character
    tmpchr = spawn_one_character( gs, pos, slot, team, skin, facing, name, MAXCHR );
    if ( VALID_CHR( gs->ChrList, tmpchr ) )
    {
      lastcharacter = tmpchr;

      gs->ChrList[lastcharacter].money += money;
      if ( gs->ChrList[lastcharacter].money > MAXMONEY )  gs->ChrList[lastcharacter].money = MAXMONEY;
      if ( gs->ChrList[lastcharacter].money < 0 )  gs->ChrList[lastcharacter].money = 0;
      gs->ChrList[lastcharacter].aistate.content = content;
      gs->ChrList[lastcharacter].passage = passage;
      if ( !VALID_CHR( gs->ChrList, attach ) )
      {
        // Free character
        currentcharacter = lastcharacter;
      }
      else
      {
        // Attached character
        if ( grip == GRIP_INVENTORY )
        {
          // Inventory character
          if ( pack_add_item( gs, lastcharacter, currentcharacter ) )
          {
            // actually do the attachment to the inventory
            Uint16 tmpchr = chr_get_attachedto(gs->ChrList, MAXCHR, lastcharacter);
            gs->ChrList[lastcharacter].aistate.alert |= ALERT_GRABBED;                       // Make spellbooks change

            // fake that it was grabbed by the left hand
            gs->ChrList[lastcharacter].attachedto = VALIDATE_CHR(gs->ChrList, currentcharacter);  // Make grab work
            gs->ChrList[lastcharacter].inwhichslot = SLOT_INVENTORY;
            let_character_think( gs, lastcharacter, 1.0f );                     // Empty the grabbed messages

            // restore the proper attachment and slot variables
            gs->ChrList[lastcharacter].attachedto = MAXCHR;                          // Fix grab
            gs->ChrList[lastcharacter].inwhichslot = SLOT_NONE;
          };
        }
        else
        {
          // Wielded character
          if ( attach_character_to_mount( gs, lastcharacter, currentcharacter, grip_to_slot( grip ) ) )
          {
            let_character_think( gs, lastcharacter, 1.0f );   // Empty the grabbed messages
          };
        }
      }

      // Turn on player input devices
      if ( stat )
      {
        if ( gs->modstate.import.amount == 0 )
        {
          if ( gs->al_cs->StatList_count == 0 )
          {
            // Single player module
            add_player( gs, lastcharacter, INBITS_MOUS | INBITS_KEYB | INBITS_JOYA | INBITS_JOYB );
          };
        }
        else if ( gs->al_cs->StatList_count < gs->modstate.import.amount )
        {
          // Multiplayer module
          itislocal = bfalse;
          tnc = 0;
          while ( tnc < localplayer_count )
          {
            if ( gs->modstate.import.slot_lst[gs->ChrList[lastcharacter].model] == localplayer_slot[tnc] )
            {
              itislocal = btrue;
              localnumber = tnc;
              break;
            }
            tnc++;
          }

          if ( itislocal )
          {
            // It's a local player
            add_player( gs, lastcharacter, localplayer_control[localnumber] );
          }
          else
          {
            // It's a remote player
            add_player( gs, lastcharacter, INBITS_NONE );
          }
        }

        // Turn on the stat display
        add_status( gs, lastcharacter );
      }

      // Set the starting level
      if ( !chr_is_player(gs, lastcharacter) )
      {
        // Let the character gain levels
        level -= 1;
        while ( gs->ChrList[lastcharacter].experiencelevel < level && gs->ChrList[lastcharacter].experience < MAXXP )
        {
          give_experience( gs, lastcharacter, 100, XP_DIRECT );
        }
      }
      if ( ghost )  // Outdated, should be removed.
      {
        // Make the character a ghost !!!BAD!!!  Can do with enchants
        gs->ChrList[lastcharacter].alpha_fp8 = 128;
        gs->ChrList[lastcharacter].bumpstrength = gs->CapList[gs->ChrList[lastcharacter].model].bumpstrength * FP8_TO_FLOAT( gs->ChrList[lastcharacter].alpha_fp8 );
        gs->ChrList[lastcharacter].light_fp8 = 255;
      }
    }
  }
  fs_fileClose( fileread );



  clear_messages();


  // Make sure local players are displayed first
  sort_statlist( gs );


  // Fix tilting trees problem
  tilt_characters_to_terrain(gs);
}

//--------------------------------------------------------------------------------------------
void despawn_characters(GameState * gs)
{
  CHR_REF chr_ref;

  // poof all characters that have pending poof requests
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( gs->ChrList, chr_ref ) || !gs->ChrList[chr_ref].gopoof ) continue;

    // detach from any imount
    detach_character_from_mount( gs, chr_ref, btrue, bfalse );

    // Drop all possesions
    for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
    {
      if ( chr_using_slot( gs->ChrList, MAXCHR, chr_ref, _slot ) )
        detach_character_from_mount( gs, chr_get_holdingwhich( gs->ChrList, MAXCHR, chr_ref, _slot ), btrue, bfalse );
    };

    chr_free_inventory( gs->ChrList, MAXCHR, chr_ref );
    gs->ChrList[chr_ref].freeme = btrue;
  };

  // free all characters that requested destruction last round
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( gs->ChrList, chr_ref ) || !gs->ChrList[chr_ref].freeme ) continue;
    ChrList_free_one( gs, chr_ref );
  }
};


//--------------------------------------------------------------------------------------------
void despawn_particles(GameState * gs)
{
  int iprt, tnc;
  Uint16 facing, pip;
  CHR_REF prt_target, prt_owner, prt_attachedto;

  // actually destroy all particles that requested destruction last time through the loop
  for ( iprt = 0; iprt < MAXPRT; iprt++ )
  {
    if ( !VALID_PRT( gs->PrtList,  iprt ) || !gs->PrtList[iprt].gopoof ) continue;

    // To make it easier
    pip = gs->PrtList[iprt].pip;
    facing = gs->PrtList[iprt].facing;
    prt_target = prt_get_target( gs, iprt );
    prt_owner = prt_get_owner( gs, iprt );
    prt_attachedto = prt_get_attachedtochr( gs, iprt );

    for ( tnc = 0; tnc < gs->PipList[pip].endspawnamount; tnc++ )
    {
      spawn_one_particle( gs, 1.0f, gs->PrtList[iprt].pos,
                          facing, gs->PrtList[iprt].model, gs->PipList[pip].endspawnpip,
                          MAXCHR, GRIP_LAST, gs->PrtList[iprt].team, prt_owner, tnc, prt_target );

      facing += gs->PipList[pip].endspawnfacingadd;
    }

    PrtList_free_one( gs, iprt );
  }

};

//--------------------------------------------------------------------------------------------
bool_t prt_search_block( GameState * gs, SearchInfo * psearch, int block_x, int block_y, PRT_REF prt_ref, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, TEAM team,
                         CHR_REF donttarget_ref, CHR_REF oldtarget_ref )
{
  // ZZ> This function helps find a target, returning btrue if it found a decent target

  Uint32 cnt;
  Uint16 local_angle;
  CHR_REF search_ref;
  bool_t request_enemies = !request_friends;
  bool_t bfound = bfalse;
  Uint32 fanblock, blnode_b;
  int local_distance;

  if( !VALID_PRT( gs->PrtList, prt_ref) ) return bfalse;

  // Current fanblock
  fanblock = mesh_convert_block( &(gs->mesh), block_x, block_y );
  if ( INVALID_FAN == fanblock ) return bfalse;

  for ( cnt = 0, blnode_b = bumplist_get_chr_head(&bumplist, fanblock); 
        cnt < bumplist_get_chr_count(&bumplist, fanblock) && INVALID_BUMPLIST_NODE != blnode_b; 
        cnt++, blnode_b = bumplist_get_next_chr(gs, &bumplist, blnode_b) )
  {
    search_ref = bumplist_get_ref(&bumplist, blnode_b);
    assert( VALID_CHR( gs->ChrList, search_ref ) );

    // don't find stupid stuff
    if ( !VALID_CHR( gs->ChrList, search_ref ) || 0.0f == gs->ChrList[search_ref].bumpstrength ) continue;

    if ( !gs->ChrList[search_ref].alive || gs->ChrList[search_ref].invictus || chr_in_pack( gs->ChrList, MAXCHR, search_ref ) ) continue;

    if ( search_ref == donttarget_ref || search_ref == oldtarget_ref ) continue;

    if (   allow_anyone || 
         ( request_friends && !gs->TeamList[team].hatesteam[gs->ChrList[search_ref].team] ) || 
         ( request_enemies &&  gs->TeamList[team].hatesteam[gs->ChrList[search_ref].team] ) )
    {
      local_distance = ABS( gs->ChrList[search_ref].pos.x - gs->PrtList[prt_ref].pos.x ) + ABS( gs->ChrList[search_ref].pos.y - gs->PrtList[prt_ref].pos.y );
      if ( psearch->initialize || local_distance < psearch->bestdistance )
      {
        Uint16 test_angle;
        local_angle = facing - vec_to_turn( gs->ChrList[search_ref].pos.x - gs->PrtList[prt_ref].pos.x, gs->ChrList[search_ref].pos.y - gs->PrtList[prt_ref].pos.y );

        test_angle = local_angle;
        if(test_angle > 32768)
        {
          test_angle = 65535 - test_angle;
        }

        if ( psearch->initialize || test_angle < psearch->bestangle )
        {
          bfound = btrue;
          psearch->initialize   = bfalse;
          psearch->besttarget   = search_ref;
          psearch->bestdistance = local_distance;
          psearch->bestangle    = test_angle;
          psearch->useangle     = local_angle;
        }
      }
    }
  }

  return bfound;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GSStack * GSStack_new(GSStack * stk)
{
  //fprintf( stdout, "GSStack_new()\n");

  if(NULL == stk || stk->initialized) return stk;

  stk->count = 0;
  stk->initialized = btrue;

  return stk;
};

//--------------------------------------------------------------------------------------------
bool_t GSStack_delete(GSStack * stk)
{
  int i;

  if(NULL == stk) return bfalse;
  if( !stk->initialized ) return btrue;

  for(i=0; i<stk->count; i++)
  {
    if( NULL != stk->data[i] )
    {
      GameState_delete(stk->data[i]);
      FREE(stk->data[i]);
    }
  }
  stk->count = 0;

  stk->initialized = bfalse;
  stk->count = 0;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t GSStack_push(GSStack * stk, GameState * gs)
{
  if(NULL == stk || !stk->initialized || stk->count + 1 >= 256) return bfalse;
  if(NULL == gs  || !gs->initialized) return bfalse;

  stk->data[stk->count] = gs;
  stk->count++;

  return btrue;
}

//--------------------------------------------------------------------------------------------
GameState * GSStack_pop(GSStack * stk)
{
  if(NULL == stk || !stk->initialized || stk->count - 1 < 0) return NULL;

  stk->count--;
  return stk->data[stk->count];
}

//--------------------------------------------------------------------------------------------
GameState * GSStack_get(GSStack * stk, int i)
{
  if(NULL == stk || !stk->initialized) return NULL;
  if( i >=  stk->count) return NULL;

  return stk->data[i];
}

//--------------------------------------------------------------------------------------------
bool_t GSStack_add(GSStack * stk, GameState * gs)
{
  // BB> Adds a new game state into the stack.
  //     Does not add a new wntry if the gamestate is already in the stack.

  int i;
  bool_t bfound = bfalse, retval = bfalse;

  if(NULL == stk || !stk->initialized) return bfalse;
  if(NULL == gs  || !gs->initialized ) return bfalse;

  // check to see it it already exists
  for(i=0; i<stk->count; i++)
  {
    if( gs == GSStack_get(stk, i) )
    {
      bfound = btrue;
      break;
    }
  }

  if(!bfound)
  {
    // not on the stack, so push it
    retval = GSStack_push(stk, gs);
  }

  return retval;

}

//--------------------------------------------------------------------------------------------
GameState * GSStack_remove(GSStack * stk, int i)
{
  if(NULL == stk || !stk->initialized) return NULL;
  if( i >= stk->count ) return NULL;

  // float value to remove to the top
  if( stk->count > 1 && i < stk->count -1 )
  {
    GameState * ptmp;
    int j = stk->count - 1;

    // swap the value with the value at the top of the stack
    ptmp = stk->data[i];
    stk->data[i] = stk->data[j];
    stk->data[j] = ptmp;
  };

  return GSStack_pop(stk);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GameState * GameState_new(GameState * gs)
{
  fprintf(stdout, "GameState_new()\n");

  if(NULL == gs || gs->initialized) return gs;

  memset(gs, 0, sizeof(GameState));

  // initialize the main loop process
  ProcState_init( &(gs->proc) );
  gs->proc.Active = bfalse;

  // initialize the menu process
  MenuProc_init( &(gs->igm) );
  gs->igm.proc.Active = bfalse;

  // initialize the sub-game-state values and link them
  ModState_new(&(gs->modstate), NULL, -1);
  gs->cd = &CData;
  gs->ns = NetState_create( gs );

  // make aliases
  gs->al_cs = gs->ns->cs;
  gs->al_ss = gs->ns->ss;


 // module parameters
  ModInfo_new( &(gs->mod) );
  ModState_new( &(gs->modstate), NULL, -1 );
  ModSummary_new( &(gs->modtxt) );
  MeshMem_new( &(gs->Mesh_Mem), 0, 0 );


  // profiles
  CapList_new( gs );
  EveList_new( gs );
  MadList_new( gs );
  PipList_new( gs );

  ChrList_new( gs );
  EncList_new( gs );
  PrtList_new( gs );

  PassList_new( gs );
  ShopList_new( gs );
  TeamList_new( gs );
  PlaList_new ( gs );

  // the the graphics state is not linked into anyone, link it to us
  if(gfxState.gs == NULL) gfxState.gs = gs;

  gs->initialized = btrue;

  return gs;
}

//--------------------------------------------------------------------------------------------
bool_t GameState_delete(GameState * gs)
{
  GSStack * stk;

  fprintf(stdout, "GameState_delete()\n");

  if(NULL == gs) return bfalse;
  if(!gs->initialized) return btrue;

  // only get rid of this data if we have been asked to be terminated
  if(!gs->proc.KillMe)
  {
    // if someone called this function out of turn, wait until the
    // the "thread" releases the data
    gs->proc.KillMe = btrue;
    return btrue;
  }

  // free the process
  ProcState_delete( &(gs->proc) );

  // initialize the sub-game-state values and link them
  ModState_delete( &(gs->modstate) );
  NetState_destroy( &(gs->ns) );

  // module parameters
  ModInfo_delete( &(gs->mod) );
  ModState_delete( &(gs->modstate) );
  ModSummary_delete( &(gs->modtxt) );

  MeshMem_delete(&(gs->Mesh_Mem));				  //Free the mesh memory

  // profiles
  CapList_delete( gs );
  EveList_delete( gs );
  MadList_delete( gs );
  PipList_delete( gs );

  ChrList_delete( gs );
  EncList_delete( gs );
  PrtList_delete( gs );
  PassList_delete( gs );
  ShopList_delete( gs );
  TeamList_delete( gs );
  PlaList_delete( gs );

  // the the graphics state is linked to us, unlink it
  if(gfxState.gs == gs)
  {
    gfxState.gs = NULL;
  }

  // unlink this from the game state stack
  stk = Get_GSStack();
  if(NULL != stk)
  {
    int i;
    for(i=0; i<stk->count; i++)
    {
      if( gs == GSStack_get(stk, i) )
      {
        // found it, so remove and exit
        GSStack_remove(stk, i);
        break;
      }
    }
  }

  // blank out everything (sets all bools to false, all pointers to NULL, ...)
  memset(gs, 0, sizeof(GameState));

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t GameState_renew(GameState * gs)
{
  fprintf(stdout, "GameState_renew()\n");

  if(NULL == gs || !gs->initialized) return bfalse;

  // re-initialize the proc state
  ProcState_renew( GameState_getProcedure(gs) );
  ProcState_init( GameState_getProcedure(gs) );
  gs->proc.Active = bfalse;

  // module parameters
  ModInfo_renew( &(gs->mod) );
  ModState_renew( &(gs->modstate), NULL, -1 );
  ModSummary_renew( &(gs->modtxt) );

  // profiles
  CapList_renew( gs );
  EveList_renew( gs );
  MadList_renew( gs );
  PipList_renew( gs );

  ChrList_renew( gs );
  EncList_renew( gs );
  PrtList_renew( gs );
  PassList_renew( gs );
  ShopList_renew( gs );
  TeamList_renew( gs );
  PlaList_renew( gs );

  return btrue;
}

//--------------------------------------------------------------------------------------------
void set_alerts( GameState * gs, CHR_REF ichr, float dUpdate )
{
  // ZZ> This function polls some alert conditions

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  AI_STATE * pstate;
  Chr      * pchr;

  if( !VALID_CHR( chrlst, ichr) ) return;

  pchr   = chrlst + ichr;
  pstate = &(pchr->aistate);

  pstate->time -= dUpdate;
  if ( pstate->time < 0 ) pstate->time = 0.0f;

  if ( ABS( pchr->pos.x - wp_list_x(&(pstate->wp)) ) < WAYTHRESH &&
       ABS( pchr->pos.y - wp_list_y(&(pstate->wp)) ) < WAYTHRESH )
  {
    ai_state_advance_wp(pstate, !caplst[pchr->model].isequipment);
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GameState * GameState_create()  
{ 
  GSStack   * stk;
  GameState * ret;

  ret = GameState_new( (GameState*)calloc(1, sizeof(GameState)) ); 

  // automatically link it into the game state stack
  stk = Get_GSStack();
  GSStack_add(stk, ret);

  return ret;
};

//--------------------------------------------------------------------------------------------
bool_t GameState_destroy(GameState ** gs ) 
{ 
  bool_t ret = GameState_delete(*gs); 
  FREE(*gs);
  return ret; 
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ShopList_new( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->ShopList_count = 0;

  for(i=0; i<MAXPASS; i++)
  {
    Shop_new(gs->ShopList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ShopList_delete( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->ShopList_count = 0;

  for(i=0; i<MAXPASS; i++)
  {
    Shop_delete(gs->ShopList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ShopList_renew( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->ShopList_count = 0;

  for(i=0; i<MAXPASS; i++)
  {
    Shop_renew(gs->ShopList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PassList_new( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PassList_count = 0;

  for(i=0; i<MAXPASS; i++)
  {
    Passage_new(gs->PassList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PassList_delete( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PassList_count = 0;

  for(i=0; i<MAXPASS; i++)
  {
    Passage_delete(gs->PassList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PassList_renew( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PassList_count = 0;

  for(i=0; i<MAXPASS; i++)
  {
    Passage_renew(gs->PassList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t CapList_new( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(i=0; i<MAXPASS; i++)
  {
    Cap_new(gs->CapList + i);
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t CapList_delete( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(i=0; i<MAXPASS; i++)
  {
    Cap_delete(gs->CapList + i);
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t CapList_renew( GameState * gs )
{
  int i;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(i=0; i<MAXPASS; i++)
  {
    Cap_renew(gs->CapList + i);
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t MadList_new( GameState * gs )
{
  int i;

  for(i=0; i<MAXMODEL; i++)
  {
    Mad_new(gs->MadList + i);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t MadList_delete( GameState * gs )
{
  int i;

  for(i=0; i<MAXMODEL; i++)
  {
    if(!gs->MadList[i].used) continue;

    Mad_delete(gs->MadList + i);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t MadList_renew( GameState * gs )
{
  int i;

  for(i=0; i<MAXMODEL; i++)
  {
    if(!gs->MadList[i].used) continue;

    Mad_renew(gs->MadList + i);
  }

  return btrue;
}





//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t EncList_new( GameState * gs )
{
  // ZZ> This functions frees all of the enchantments

  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->EncFreeList_count = 0;
  for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
  {
    Enc_new( gs->EncList + cnt );

    gs->EncFreeList_count = cnt;
    gs->EncFreeList[cnt] = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_delete( GameState * gs )
{
  // ZZ> This functions frees all of the enchantments

  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->EncFreeList_count = 0;
  for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
  {
    Enc_delete( gs->EncList + cnt );

    gs->EncFreeList_count = cnt;
    gs->EncFreeList[cnt] = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_renew( GameState * gs )
{
  // ZZ> This functions frees all of the enchantments

  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->EncFreeList_count = 0;
  for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
  {
    Enc_renew( gs->EncList + cnt );

    gs->EncFreeList_count = cnt;
    gs->EncFreeList[cnt] = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PlaList_new( GameState * gs )
{
  // ZZ> This functions frees all of the plahantments

  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PlaList_count      = 0;
  gs->al_cs->loc_pla_count  = 0;
  gs->al_cs->StatList_count = 0;

  for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
  {
    Player_new( gs->PlaList + cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PlaList_delete( GameState * gs )
{
  // ZZ> This functions frees all of the plahantments

  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PlaList_count      = 0;
  if(NULL != gs->ns && NULL != gs->al_cs)
  {
    gs->al_cs->loc_pla_count  = 0;
    gs->al_cs->StatList_count = 0;
  };

  for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
  {
    Player_delete( gs->PlaList + cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PlaList_renew( GameState * gs )
{
  // ZZ> This functions frees all of the plahantments

  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PlaList_count      = 0;
  gs->al_cs->loc_pla_count  = 0;
  gs->al_cs->StatList_count = 0;

  for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
  {
    Player_renew( gs->PlaList + cnt );
  }

  return btrue;
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ChrList_new( GameState * gs )
{
  int cnt;
  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt=0; cnt<MAXCHR; cnt++)
  {
    Chr_new( gs->ChrList + cnt );

    gs->ChrFreeList_count = cnt;
    gs->ChrFreeList[cnt]  = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_delete(GameState * gs)
{
  int cnt;
  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt=0; cnt<MAXCHR; cnt++)
  {
    Chr_delete( gs->ChrList + cnt );

    gs->ChrFreeList_count = cnt;
    gs->ChrFreeList[cnt]  = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_renew( GameState * gs )
{
  int cnt;
  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt=0; cnt<MAXCHR; cnt++)
  {
    Chr_renew( gs->ChrList + cnt );

    gs->ChrFreeList_count = cnt;
    gs->ChrFreeList[cnt]  = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t EveList_new( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt = 0; cnt<MAXEVE; cnt++)
  {
    Eve_new( gs->EveList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EveList_delete( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt = 0; cnt<MAXEVE; cnt++)
  {
    Eve_delete( gs->EveList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EveList_renew( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt = 0; cnt<MAXEVE; cnt++)
  {
    Eve_renew( gs->EveList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PrtList_new( GameState * gs )
{
  // ZZ> This function resets the particle allocation lists

  int cnt;
  if(NULL == gs) return bfalse;

  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    Prt_new(gs->PrtList + cnt);

    gs->PrtFreeList_count = cnt;
    gs->PrtFreeList[cnt] = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_delete( GameState * gs )
{
  // ZZ> This function resets the particle allocation lists

  int cnt;
  if(NULL == gs) return bfalse;

  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    Prt_delete(gs->PrtList + cnt);

    gs->PrtFreeList_count = cnt;
    gs->PrtFreeList[cnt] = cnt;
  }

  return btrue;
}


//--------------------------------------------------------------------------------------------
bool_t PrtList_renew( GameState * gs )
{
  // ZZ> This function resets the particle allocation lists

  int cnt;
  if(NULL == gs) return bfalse;

  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    Prt_renew(gs->PrtList + cnt);

    gs->PrtFreeList_count = cnt;
    gs->PrtFreeList[cnt] = cnt;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t TeamList_new( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt = 0; cnt<TEAM_COUNT; cnt++)
  {
    TeamInfo_new( gs->TeamList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t TeamList_delete( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt = 0; cnt<TEAM_COUNT; cnt++)
  {
    TeamInfo_delete( gs->TeamList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t TeamList_renew( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  for(cnt = 0; cnt<TEAM_COUNT; cnt++)
  {
    TeamInfo_renew( gs->TeamList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t reset_characters( GameState * gs )
{
  // ZZ> This function resets all the character data

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->allpladead = btrue;

  ChrList_renew(gs);
  StatList_renew(gs->al_cs);
  PlaList_renew(gs);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PipList_new( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PipList_count = 0;
  for(cnt = 0; cnt<MAXPRTPIP; cnt++)
  {
    Pip_new( gs->PipList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PipList_delete( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PipList_count = 0;
  for(cnt = 0; cnt<MAXPRTPIP; cnt++)
  {
    Pip_delete( gs->PipList + cnt );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PipList_renew( GameState * gs )
{
  int cnt;

  if(NULL == gs || !gs->initialized) return bfalse;

  gs->PipList_count = 0;
  for(cnt = 0; cnt<MAXPRTPIP; cnt++)
  {
    Pip_renew( gs->PipList + cnt );
  };

  PipList_load_global(gs);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ProcState * ProcState_new(ProcState * ps)
{
  //fprintf( stdout, "ProcState_new()\n");

  if(NULL == ps || ps->initialized) return ps;

  memset(ps, 0, sizeof(ProcState));

  ps->Active = btrue;
  ps->State  = PROC_Begin;
  ps->clk    = ClockState_create("ProcState", -1);

  ps->initialized = btrue;

  return ps;
};

//--------------------------------------------------------------------------------------------
bool_t ProcState_delete(ProcState * ps)
{
  if(NULL == ps) return bfalse;
  if(!ps->initialized) return btrue;

  if(!ps->KillMe)
  {
    ps->KillMe = btrue;
    ps->State = PROC_Leaving;
    return btrue;
  }

  ps->Active      = bfalse;
  ps->initialized = bfalse;
  ClockState_destroy( &(ps->clk) );

  return btrue;
}

//--------------------------------------------------------------------------------------------
ProcState * ProcState_renew(ProcState * ps)
{
  ProcState_delete(ps);
  return ProcState_new(ps);
};

//--------------------------------------------------------------------------------------------
bool_t ProcState_init(ProcState * ps)
{
  if(NULL == ps) return bfalse;

  if(!ps->initialized)
  {
    ProcState_new(ps);
  };

  ps->Active     = btrue;
  ps->Paused     = bfalse;
  ps->Terminated = bfalse;

  return btrue;
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static GuiState * GuiState_new( GuiState * g );
static bool_t     GuiState_delete( GuiState * g );

//--------------------------------------------------------------------------------------------
GuiState * Get_GuiState()
{
  if(!_gui_state.initialized)
  {
    GuiState_new(&_gui_state);
  }

  return &_gui_state;
}

//--------------------------------------------------------------------------------------------
bool_t GuiState_startUp()
{
  Get_GuiState();

  return _gui_state.initialized;
}

//--------------------------------------------------------------------------------------------
bool_t GuiState_shutDown()
{
  return GuiState_delete( &_gui_state );
}

//--------------------------------------------------------------------------------------------
GuiState * GuiState_new( GuiState * gui )
{
  //fprintf( stdout, "GuiState_new()\n");

  if(NULL == gui || gui->initialized) return gui;

  memset(gui, 0, sizeof(GuiState));

  MenuProc_init( &(gui->mnu_proc) );

  //Pause button avalible?
  gui->can_pause = btrue;  

  // not in message mode
  gui->net_messagemode = bfalse;

  // initialize the textures
  gui->TxBars.textureID = INVALID_TEXTURE;
  gui->TxBlip.textureID = INVALID_TEXTURE;

  gui->clk = ClockState_create("GuiState", -1);

  gui->initialized = btrue;

  return gui;
}

//--------------------------------------------------------------------------------------------
bool_t GuiState_delete( GuiState * gui )
{
  if(NULL == gui) return bfalse;
  if(!gui->initialized) return btrue;

  MenuProc_delete( &(gui->mnu_proc) );

  ClockState_destroy( &(gui->clk) );

  gui->initialized = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_player( GameState * gs, CHR_REF character)
{
  if( NULL == gs || !VALID_CHR(gs->ChrList, character)) return bfalse;

  return VALID_PLA(gs->PlaList, gs->ChrList[character].whichplayer);
}

//--------------------------------------------------------------------------------------------
bool_t count_players(GameState * gs)
{
  int cnt, ichr, numdead;
  Player * plalist;
  Chr    * chrlist;
  ClientState * cs;
  ServerState * ss;

  if(NULL == gs) return bfalse;

  cs = gs->al_cs;
  ss = gs->al_ss;
  plalist = gs->PlaList;
  chrlist = gs->ChrList;

  // Check for all local players being dead
  gs->allpladead   = bfalse;
  gs->somepladead  = bfalse;
  cs->seeinvisible = bfalse;
  cs->seekurse     = bfalse;
  cs->loc_pla_count = 0;
  ss->rem_pla_count = 0;
  numdead = 0;
  for (cnt = 0; cnt < MAXPLAYER; cnt++)
  {
    if( !VALID_PLA(plalist, cnt) ) continue;

    if( INBITS_NONE == plalist[cnt].device )
    {
      ss->rem_pla_count++;
    }
    else
    {
      cs->loc_pla_count++;
    }

    ichr = PlaList_get_character( gs, cnt );
    if ( !VALID_CHR( chrlist, ichr ) ) continue;

    if ( !chrlist[ichr].alive )
    {
      numdead++;
    };

    if ( chrlist[ichr].canseeinvisible )
    {
      cs->seeinvisible = btrue;
    }

    if ( chrlist[ichr].canseekurse )
    {
      cs->seekurse = btrue;
    }
  }

  gs->somepladead = ( numdead > 0 );
  gs->allpladead  = ( numdead >= cs->loc_pla_count + ss->rem_pla_count );

  return btrue;
}
