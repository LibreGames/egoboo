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
#include "file_common.h"

#include "object.inl"
#include "Network.inl"
#include "mesh.inl"

#include "egoboo_rpc.h"
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
#include "egoboo_math.inl"
#include "egoboo_types.inl"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

static MachineState_t * MachineState_new( MachineState_t * ms );
static bool_t         MachineState_delete( MachineState_t * ms );


static Game_t * Game_new(Game_t * gs, Net_t * net, Client_t * cl, Server_t * sv);
static bool_t   Game_delete(Game_t * gs);

static bool_t   Gui_startUp();

static retval_t main_doGameGraphics();
static void     game_handleKeyboard();


//---------------------------------------------------------------------------------------------

static MachineState_t _macState = { bfalse };
static ConfigData_t   CData_default;
ConfigData_t CData;

char cActionName[MAXACTION][2];

KeyboardBuffer_t GNetMsg = {20,0,0,NULL_STRING};
Gui_t            _gui_state = { bfalse };

PROFILE_DECLARE( resize_characters );
PROFILE_DECLARE( keep_weapons_with_holders );
PROFILE_DECLARE( run_all_scripts );
PROFILE_DECLARE( do_weather_spawn );
PROFILE_DECLARE( enc_spawn_particles );
PROFILE_DECLARE( Client_unbufferLatches );
PROFILE_DECLARE( CServer_unbufferLatches );
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

PROFILE_DECLARE( ChrHeap );
PROFILE_DECLARE( EncHeap );
PROFILE_DECLARE( PrtHeap );
PROFILE_DECLARE( ekey );

// When operating as a server, there may be a need to host multiple games
// this "stack" holds all valid game states
struct sGameStack
{
  egoboo_key_t  ekey;
  int         count;
  Game_t * data[256];
};

typedef struct sGameStack GameStack_t;

static GameStack_t _game_stack = { bfalse };

GameStack_t * GameStack_new(GameStack_t * stk);
bool_t        GameStack_delete(GameStack_t * stk);
bool_t        GameStack_push(GameStack_t * stk, Game_t * gs);
Game_t *      GameStack_pop(GameStack_t * stk);
Game_t *      GameStack_get(GameStack_t * stk, int i);
Game_t *      GameStack_remove(GameStack_t * stk, int i);

GameStack_t * Get_GameStack()
{
  // BB > a function to get the global game state stack.
  //      Acts like a singleton, will initialize _game_stack if not already initialized

  if(!EKEY_VALID(_game_stack))
  {
    GameStack_new(&_game_stack);
  }

  return &_game_stack;
};

//---------------------------------------------------------------------------------------------
static void update_looped_sounds( Game_t * gs );
static bool_t load_all_music_sounds(ConfigData_t * cd);

static void sdlinit( Graphics_t * g );

static void update_game(Game_t * gs, float dFrame, Uint32 * rand_idx);
static void memory_cleanUp(void);

GameStack_t * Get_GameStack();

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

  Game_t * gs;
  MachineState_t * mach_state;
  GameStack_t * stk = Get_GameStack();

  log_info("memory_cleanUp() - \n\tAttempting to clean up loaded things in memory... ");

  // kill all game states
  while(stk->count > 0)
  {
    gs = GameStack_pop(stk);
    if( !EKEY_PVALID(gs) ) continue;

    gs->proc.KillMe = btrue;
    Game_destroy(&gs);
  }

  // shut down all game systems
  snd_quit();
  ui_shutdown();			          //Shutdown various support systems
  CGui_shutDown();
  mach_state = get_MachineState();
  MachineState_delete( mach_state );
  sys_shutdown();

  log_message("Succeeded!\n");
  log_shutdown();


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

//---------------------------------------------------------------------------------------------
bool_t export_all_enchants( Game_t * gs, CHR_REF ichr, const char * todirname )
{
  // BB> Export all of the enchant info associated with an enchant to a file called rechantXXXX.txt
  //     Note that this does not save the sounds, particles, or any other asset associated with the enchant

  int cnt;
  ENC_REF ienc;
  STRING  exportname, fname, rechant_name;
  FILE *  rechant_file;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  strncpy( exportname, todirname, sizeof(exportname));
  str_append_slash(exportname, sizeof(exportname));
  
  snprintf( rechant_name, sizeof(rechant_name), "%srechant.txt", exportname );

  rechant_file = fs_fileOpen( PRI_WARN, "export_all_enchants()", fname, "w" );
  if(NULL == rechant_file) return bfalse;

  fprintf( rechant_file, "// Stores the data for enchants that are to be re-applied to the character\n" );
  fprintf( rechant_file, "// Enchant number - remaining time\n" );

  cnt = 0;
  ienc = chrlst[ichr].firstenchant;
  while ( INVALID_ENC != ienc )
  {
    EVE_REF ieve;

    ienc = chrlst[ichr].firstenchant;
    ieve = gs->EncList[ienc].eve;

    if( !gs->EveList[ieve].endifcantpay ||  gs->EveList[ieve].stayifnoowner )
    {
      // store the info in the rechant.txt file
      fprintf( rechant_file, ":%d %d %d\n", cnt,  gs->EncList[ienc].time, gs->EncList[ienc].spawntime );

      // create the new filename
      snprintf( fname, sizeof(fname), "%senchant%03d.txt", exportname, cnt );
      cnt++;

      // copy the file
      fs_copyFile(gs->EveList[ieve].loadname, fname);
    };

    ienc = gs->EncList[ienc].nextenchant;
  }

  fs_fileClose(rechant_file);

  return btrue;
}


//---------------------------------------------------------------------------------------------
bool_t export_one_character( Game_t * gs, CHR_REF ichr, CHR_REF iowner, int number, bool_t export_profile )
{
  // ZZ> This function exports a character

  // BB> If export_profile is true, we have just exited a starter module. We should just export the base values
  //     directly from the profile. Otherwise, we have just beaten some module or exited a town, and we SHOULD
  //     carry all enchants, etc. with us. I'm not sure how to do this other than exporting all enchants to the
  //     *.obj directory?

  int tnc;
  STRING tmpname;
  STRING fromdir;
  STRING todir;
  STRING fromfile;
  STRING tofile;
  STRING todirname;
  STRING todirfullname;

  PChr_t chrlst     = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PObj_t objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  //PCap_t caplst      = gs->CapList;
  //size_t caplst_size = CAPLST_COUNT;

  //PMad_t madlst      = gs->MadList;
  //size_t madlst_size = MADLST_COUNT;

  OBJ_REF iobj;
  Obj_t  * pobj;

  //CAP_REF icap;
  Cap_t  * pcap;

  //MAD_REF imad;
  Mad_t  * pmad;

  Chr_t * pchr;

  if( !ACTIVE_CHR(chrlst, ichr) ) return bfalse;
  pchr = ChrList_getPChr(gs, ichr);

  iobj = ChrList_getRObj(gs, ichr);
  if( INVALID_OBJ == iobj ) return bfalse;
  pobj = gs->ObjList + iobj;

  pcap = ChrList_getPCap(gs, ichr);
  if(NULL == pcap) return bfalse;

  pmad = ChrList_getPMad(gs, ichr);
  if(NULL == pmad) return bfalse;

  if( !gs->modstate.exportvalid ) return bfalse;
  if( !pcap->cancarrytonextmodule && pcap->prop.isitem ) return bfalse;

  // TWINK_BO.OBJ
  snprintf( todirname, sizeof( todirname ), "%s" SLASH_STRING, CData.players_dir );
  str_encode( todirname, sizeof( tmpname ), chrlst[iowner].name );
  strncat( todirname, ".obj", sizeof( tmpname ) );

  if( !export_profile )
  {
    // make the enchants stick to the character after this module
    export_all_enchants(gs, ichr, todirname);
  }
 
  // Don't export enchants
  disenchant_character( gs, ichr );

  // Is it a character or an item?
  if ( iowner != ichr )
  {
    // Item - make a subdirectory of the owner directory...
    snprintf( todirfullname, sizeof( todirfullname ), "%s" SLASH_STRING "%d.obj", todirname, number );
  }
  else
  {
    // Character - make directory
    strncpy( todirfullname, todirname, sizeof( todirfullname ) );
  }

  // players/twink.obj or players/twink.obj/sword.obj
  snprintf( todir, sizeof( todir ), "%s" SLASH_STRING "%s", CData.players_dir, todirfullname );

  // modules/advent.mod/objects/advent.obj
  strncpy( fromdir, pobj->name, sizeof( fromdir ) );

  // Delete all the old items
  if ( iowner == ichr )
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
  CapList_save_one( gs, tofile, ichr );

  // Build the SKIN.TXT file
  snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.skin_file );    /*SKIN.TXT*/
  export_one_character_skin( gs, tofile, ichr );

  // Build the NAMING.TXT file
  snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.naming_file );    /*NAMING.TXT*/
  export_one_character_name( gs, tofile, ichr );

  // Copy all the messages
  snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.message_file );   /*MessageData_t.TXT*/
  snprintf( tofile, sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.message_file );   /*MessageData_t.TXT*/
  fs_copyFile( fromfile, tofile );

  // Copy all the model
  snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "tris.md2", fromdir );    /*TRIS.MD2*/
  snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "tris.md2", todir );    /*TRIS.MD2*/
  fs_copyFile( fromfile, tofile );

  // Copy "copy.txt"
  snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.copy_file );    /*COPY.TXT*/
  snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.copy_file );    /*COPY.TXT*/
  fs_copyFile( fromfile, tofile );

  // Copy all the script
  snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.script_file );
  snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.script_file );
  fs_copyFile( fromfile, tofile );

  // Copy the enchant file
  snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.enchant_file );
  snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.enchant_file );
  fs_copyFile( fromfile, tofile );

  // Copy any credits
  snprintf( fromfile, sizeof( fromfile ), "%s" SLASH_STRING "%s", fromdir, CData.credits_file );
  snprintf( tofile,   sizeof( tofile ), "%s" SLASH_STRING "%s", todir, CData.credits_file );
  fs_copyFile( fromfile, tofile );

  // Copy the quest info file
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

  return btrue;
}

//---------------------------------------------------------------------------------------------
void export_all_local_players( Game_t * gs, bool_t export_profile )
{
  // ZZ> This function saves all the local players in the
  //     PLAYERS directory

  int number;
  CHR_REF character, item;
  PLA_REF ipla;

  // Check each player
  if ( !gs->modstate.exportvalid ) return;

  for ( ipla = 0; ipla < PLALST_COUNT; ipla++ )
  {
    if ( !VALID_PLA( gs->PlaList,  ipla ) || INBITS_NONE == gs->PlaList[ipla].device ) continue;

    // Is it alive?
    character = PlaList_getRChr( gs, ipla );
    if ( !ACTIVE_CHR( gs->ChrList, character ) || !gs->ChrList[character].alive ) continue;

    // Export the character
    export_one_character( gs, character, character, 0, export_profile  );

    // Export all held items
    for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
    {
      item = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, character, _slot );
      if ( ACTIVE_CHR( gs->ChrList, item ) && gs->ChrList[item].prop.isitem )
      {
        SLOT loc_slot = (_slot == SLOT_SADDLE ? _slot : SLOT_LEFT);
        export_one_character( gs, item, character, loc_slot, export_profile  );
      };
    }

    // Export the inventory
    number = 3;
    item  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, character );
    while ( ACTIVE_CHR( gs->ChrList, item ) )
    {
      if ( gs->ChrList[item].prop.isitem ) export_one_character( gs, item, character, number, export_profile  );
      item  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, item );
      number++;
    }

  }

}


//--------------------------------------------------------------------------------------------
void quit_game( Game_t * gs )
{
  // ZZ> This function exits the game entirely

  if(!gs->proc.Active) return;

  log_info( "Exiting Egoboo %s the good way...\n", VERSION );

  module_quit(gs);

  gs->proc.Active = bfalse;
  gs->proc.Paused = bfalse;
}




//--------------------------------------------------------------------------------------------
int MessageQueue_get_free(MessageQueue_t * mq)
{
  // BB > This function finds the first free message

  int cnt, tnc;
  bool_t found = bfalse;

  tnc = CData.maxmessage;
  for(cnt=0; cnt<CData.maxmessage && !found; cnt++)
  {
    if(mq->list[tnc].time <= 0)
    {
      tnc = mq->start;
      found = btrue;
    }

    mq->start = (mq->start + 1) % CData.maxmessage;
  }

  return tnc;
}

//--------------------------------------------------------------------------------------------
bool_t decode_escape_sequence( Game_t * gs, char * buffer, size_t buffer_size, const char * message, CHR_REF chr_ref )
{
  // BB> expands escape sequences

  STRING szTmp;
  char esc_code;
  char lTmp;

  CHR_REF target = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + chr_ref );
  CHR_REF owner  = chr_get_aiowner( gs->ChrList, CHRLST_COUNT, gs->ChrList + chr_ref );

  Chr_t * pchr    = ChrList_getPChr(gs, chr_ref);
  AI_STATE * pstate = &(pchr->aistate);
  Cap_t * pchr_cap = ChrList_getPCap(gs, chr_ref);

  Chr_t * ptarget  = ChrList_getPChr(gs, target);
  Cap_t * ptrg_cap = ChrList_getPCap(gs, target);

  Chr_t * powner   = ChrList_getPChr(gs, owner);
  Cap_t * pown_cap = ChrList_getPCap(gs, owner);

  char * dst_pos, * src_pos;
  char * dst_end;

  dst_pos = buffer;
  dst_end = buffer + buffer_size - 1;
  src_pos = message;
  while(dst_pos < dst_end && EOS != *src_pos)
  {
    bool_t handled;

    while('%' != *src_pos && EOS != *src_pos && dst_pos < dst_end)
    {
      *dst_pos++ = *src_pos++;
    }

    if('%' != *src_pos) { *dst_pos = EOS; break; }

    // go to the esc_code character
    src_pos++;
    esc_code = *src_pos;
    src_pos++;

    handled = bfalse;
    if ( esc_code >= '0' && esc_code <= '0' + ( MAXSKIN - 1 ) )  // Target's skin name
    {
      strncpy( szTmp, ptrg_cap->skin[esc_code-'0'].name, sizeof( STRING ) );
      handled = btrue;
    }
    else
    {
      handled = btrue;

      switch(esc_code)
      {
      case 'n': // Name
        {
          if ( pchr->prop.nameknown )
            strncpy( szTmp, pchr->name, sizeof( STRING ) );
          else
          {
            lTmp = pchr_cap->classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", pchr_cap->classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", pchr_cap->classname );
          }
          if ( (dst_pos == buffer) && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        break;

      case 'c': // Class name
        {
          strncpy( szTmp, pchr_cap->classname, sizeof(szTmp));
        }
        break;

      case 't': // Target name
        {
          if ( ptarget->prop.nameknown )
            strncpy( szTmp, ptarget->name, sizeof( STRING ) );
          else
          {
            lTmp = ptrg_cap->classname[0];
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", ptrg_cap->classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", ptrg_cap->classname );
          }
          if ( (dst_pos == buffer) && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        break;

      case 'o': // Owner name
        {
          if ( powner->prop.nameknown )
            strncpy( szTmp, powner->name, sizeof( STRING ) );
          else
          {
            lTmp = toupper(pown_cap->classname[0]);
            if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
              snprintf( szTmp, sizeof( szTmp ), "an %s", pown_cap->classname );
            else
              snprintf( szTmp, sizeof( szTmp ), "a %s", pown_cap->classname );
          }
          if ( (dst_pos == buffer) && szTmp[0] == 'a' )  szTmp[0] = 'A';
        }
        break;

      case 's': // Target class name
        {
          strncpy( szTmp, ptrg_cap->classname, sizeof(szTmp));
        }
        break;

      case 'd': // tmpdistance value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", pstate->tmpdistance );
        }
        break;

      case 'x': // tmpx value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", pstate->tmpx );
        }
        break;

      case 'y': // tmpy value
        {
          snprintf( szTmp, sizeof( szTmp ), "%d", pstate->tmpy );
        }
        break;

      case 'D': // tmpdistance value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", pstate->tmpdistance );
        }
        break;

      case 'X': // tmpx value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", pstate->tmpx );
        }
        break;

      case 'Y': // tmpy value
        {
          snprintf( szTmp, sizeof( szTmp ), "%2d", pstate->tmpy );
        }
        break;

      case 'a': // Character's ammo
        {
          if ( pchr->ammoknown )
            snprintf( szTmp, sizeof( szTmp ), "%d", pchr->ammo );
          else
            snprintf( szTmp, sizeof( szTmp ), "?" );
        }
        break;

      case 'k': // Kurse state
        {
          if ( pchr->prop.iskursed )
            snprintf( szTmp, sizeof( szTmp ), "kursed" );
          else
            snprintf( szTmp, sizeof( szTmp ), "unkursed" );
        }
        break;

      case 'p': // Character's possessive
        {
          if ( pchr->gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else if ( pchr->gender == GEN_MALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "his" );
          }
          else
          {
            snprintf( szTmp, sizeof( szTmp ), "its" );
          }
        }
        break;

      case 'm': // Character's gender
        {
          if ( pchr->gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "female " );
          }
          else if ( pchr->gender == GEN_MALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "male " );
          }
          else
          {
            snprintf( szTmp, sizeof( szTmp ), " " );
          }
        }
        break;

      case 'g': // Target's possessive
        {
          if ( ptarget->gender == GEN_FEMALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "her" );
          }
          else if ( ptarget->gender == GEN_MALE )
          {
            snprintf( szTmp, sizeof( szTmp ), "his" );
          }
          else
          {
            snprintf( szTmp, sizeof( szTmp ), "its" );
          }

        }

        break;

      default:
        handled = bfalse;
        break;
      }
    }

    if(!handled)
    {
      strcpy( szTmp, "####" );
    }

    dst_pos += snprintf(dst_pos, (size_t)(dst_end-dst_pos), szTmp);
  }

  return btrue;
}


//--------------------------------------------------------------------------------------------
bool_t display_message( Game_t * gs, int message, CHR_REF chr_ref )
{
  // ZZ> This function sticks a message in the display queue and sets its timer

  int slot;

  MessageData_t  * msglst = &(gs->MsgList);
  Gui_t         * gui    = gui_getState();
  MessageQueue_t * mq     = &(gui->msgQueue);
  MESSAGE_ELEMENT * msg;
  char * message_src;

  if ( message >= msglst->total ) return bfalse;

  slot = MessageQueue_get_free( mq );
  if(CData.maxmessage == slot) return bfalse;

  msg = mq->list + slot;

  // Copy the message
  message_src = msglst->text + msglst->index[message];
  decode_escape_sequence(gs, msg->textdisplay, sizeof(msg->textdisplay), message_src, chr_ref);
  msg->time = DELAY_MESSAGE;

  return btrue;
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

  log_info( "load_action_names() - \n\tloading all 2 letter action names from %s.\n", loadname );

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

  ConfigFilePtr_t lConfigSetup;
  char lCurSectionName[64];
  bool_t lTempBool;
  Sint32 lTmpInt;
  char lTmpStr[24];

  lConfigSetup = OpenConfigFile( filename );
  if ( NULL == lConfigSetup  )
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
void print_status( Game_t * gs, Uint16 statindex )
{
  // ZZ> This function shows the more specific stats for a character

  CHR_REF character;
  char gender[8];
  Status_t * lst = gs->cl->StatList;
  size_t   lst_size = MAXSTAT;

  if ( lst[statindex].delay == 0 )
  {
    if ( statindex < lst_size )
    {
      character = lst[statindex].chr_ref;

      // Name
      debug_message( 1, "=%s=", gs->ChrList[character].name );

      // Level and gender and class
      gender[0] = EOS;
      if ( gs->ChrList[character].alive )
      {
        switch ( gs->ChrList[character].gender )
        {
          case GEN_MALE: snprintf( gender, sizeof( gender ), "Male" ); break;
          case GEN_FEMALE: snprintf( gender, sizeof( gender ), "Female" ); break;
        };

        debug_message( 1, " %s %s", gender, ChrList_getPCap(gs, character)->classname );
      }
      else
      {
        debug_message( 1, " Dead %s", ChrList_getPCap(gs, character)->classname );
      }

      // Stats
      debug_message( 1, " STR:%2.1f ~WIS:%2.1f ~INT:%2.1f", FP8_TO_FLOAT( gs->ChrList[character].stats.strength_fp8 ), FP8_TO_FLOAT( gs->ChrList[character].stats.wisdom_fp8 ), FP8_TO_FLOAT( gs->ChrList[character].stats.intelligence_fp8 ) );
      debug_message( 1, " DEX:%2.1f ~LVL:%4.1f ~DEF:%2.1f", FP8_TO_FLOAT( gs->ChrList[character].stats.dexterity_fp8 ), calc_chr_level( gs, character ), FP8_TO_FLOAT( gs->ChrList[character].skin.defense_fp8 ) );

      lst[statindex].delay = 10;
    }
  }
}


//--------------------------------------------------------------------------------------------
void draw_chr_info( Game_t * gs )
{
  // ZZ> This function lets the players check character stats

  size_t cnt;
  Status_t * lst      = gs->cl->StatList;
  size_t   lst_size = gs->cl->StatList_count;

  if ( !keyb.mode )
  {
    for(cnt=0; cnt<8; cnt++)
    {
      if ( SDLKEYDOWN( SDLK_1 + cnt ) && cnt<lst_size )  print_status( gs, cnt );
    }

    // Debug cheat codes (Gives xp to stat characters)
    if ( CData.DevMode && SDLKEYDOWN( SDLK_x ) )
    {
      PLA_REF pla_cnt;
      for(pla_cnt=0; pla_cnt<MIN(8,PLALST_COUNT); pla_cnt++)
      {
        if ( SDLKEYDOWN( SDLK_1 + pla_cnt ) && pla_cnt<lst_size && ACTIVE_CHR( gs->ChrList, gs->PlaList[pla_cnt].chr_ref ) )
        {
          give_experience( gs, gs->PlaList[pla_cnt].chr_ref, 25, XP_DIRECT );
          lst[ REF_TO_INT(pla_cnt) ].delay = 0;
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

//--------------------------------------------------------------------------------------------
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


  if ( NULL == temp  )
    return bfalse;

  pixels = EGOBOO_NEW_ARY( Uint8, 3 * screen->w * screen->h );
  if ( NULL == pixels  )
  {
    SDL_FreeSurface( temp );
    return bfalse;
  }

  glReadPixels( 0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels );

  for ( i = 0; i < screen->h; i++ )
  {
    memcpy((( char * ) temp->pixels ) + temp->pitch * i, pixels + 3 * screen->w * ( screen->h - i - 1 ), screen->w * 3 );
  }
  EGOBOO_DELETE( pixels );

  // find the next EGO??.BMP file for writing
  i = 0;
  test = NULL;
  do
  {
    if ( NULL != test  )
      fs_fileClose( test );

    snprintf( buff, sizeof( buff ), "ego%02d.bmp", i );

    // lame way of checking if the file already exists...
    test = fs_fileOpen( PRI_NONE, NULL, buff, "rb" );
    i++;
  }
  while (( NULL != test  ) && ( i < 100 ) );

  SDL_SaveBMP( temp, buff );
  SDL_FreeSurface( temp );

  debug_message( 1, "Saved to %s", buff );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t add_status( Game_t * gs, CHR_REF character )
{
  // ZZ> This function adds a status display to the do list

  bool_t was_added;
  size_t old_size = gs->cl->StatList_count;
  size_t new_size;

  // try to add the character
  new_size = StatList_add( gs->cl->StatList, gs->cl->StatList_count, character );
  was_added = (old_size != new_size);

  gs->ChrList[character].staton = was_added;
  gs->cl->StatList_count        = new_size;

  return was_added;
}



//--------------------------------------------------------------------------------------------
bool_t remove_stat( Game_t * gs, Chr_t * pchr )
{
  size_t i, icount;
  bool_t bfound;

  Status_t * statlst      = gs->cl->StatList;
  size_t   statlst_size = gs->cl->StatList_count;

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
      memmove(statlst + i, statlst + i + 1, icount*sizeof(Status_t));
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

  gs->cl->StatList_count = statlst_size;

  return btrue;
}

//--------------------------------------------------------------------------------------------
void sort_statlist( Game_t * gs )
{
  // ZZ> This function puts all of the local players on top of the statlst

  PLA_REF pla_cnt;
  Status_t * statlst;
  size_t   statlst_size;

  if( !EKEY_PVALID(gs) || NULL == gs->cl) return;

  statlst      = gs->cl->StatList;
  statlst_size = gs->cl->StatList_count;

  for ( pla_cnt = 0; pla_cnt < gs->PlaList_count; pla_cnt++ )
  {
    if ( !VALID_PLA( gs->PlaList,  pla_cnt ) || INBITS_NONE == gs->PlaList[pla_cnt].device ) continue;

    StatList_move_to_top( statlst, statlst_size, gs->PlaList[pla_cnt].chr_ref );
  }
}

//--------------------------------------------------------------------------------------------
void move_water( float dUpdate )
{
  // ZZ> This function animates the water overlays

  int layer;
  Game_t * gs = Graphics_requireGame(&gfxState);

  for ( layer = 0; layer < MAXWATERLAYER; layer++ )
  {
    gs->Water.layer[layer].u += gs->Water.layer[layer].uadd * dUpdate;
    gs->Water.layer[layer].v += gs->Water.layer[layer].vadd * dUpdate;
    if ( gs->Water.layer[layer].u > 1.0 )  gs->Water.layer[layer].u -= 1.0;
    if ( gs->Water.layer[layer].v > 1.0 )  gs->Water.layer[layer].v -= 1.0;
    if ( gs->Water.layer[layer].u < -1.0 )  gs->Water.layer[layer].u += 1.0;
    if ( gs->Water.layer[layer].v < -1.0 )  gs->Water.layer[layer].v += 1.0;
    gs->Water.layer[layer].frame = (( int )( gs->Water.layer[layer].frame + gs->Water.layer[layer].frameadd * dUpdate ) ) & WATERFRAMEAND;
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF search_best_leader( Game_t * gs, TEAM_REF team, CHR_REF exclude )
{
  // BB > find the best (most experienced) character other than the sissy to be a team leader

  CHR_REF chr_cnt;
  CHR_REF best_leader = INVALID_CHR;
  int     best_experience = 0;
  bool_t  bfound = bfalse;
  CHR_REF exclude_sissy = team_get_sissy( gs, team );

  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    if ( !ACTIVE_CHR( gs->ChrList, chr_cnt ) || chr_cnt == exclude || chr_cnt == exclude_sissy || gs->ChrList[chr_cnt].team != team ) continue;

    if ( !bfound || gs->ChrList[chr_cnt].experience > best_experience )
    {
      best_leader     = chr_cnt;
      best_experience = gs->ChrList[chr_cnt].experience;
      bfound = btrue;
    }
  }

  return bfound ? best_leader : INVALID_CHR;
}

//--------------------------------------------------------------------------------------------
void call_for_help( Game_t * gs, CHR_REF character )
{
  // ZZ> This function issues a call for help to all allies

  TEAM_REF team;
  CHR_REF chr_cnt;

  team = gs->ChrList[character].team;


  // make the character in who needs help the sissy
  gs->TeamList[team].leader = search_best_leader( gs, team, character );
  gs->TeamList[team].sissy  = character;


  // send the help message
  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    if ( !ACTIVE_CHR( gs->ChrList, chr_cnt ) || chr_cnt == character ) continue;
    if ( !gs->TeamList[gs->ChrList[chr_cnt].team_base].hatesteam[REF_TO_INT(team)] )
    {
      gs->ChrList[chr_cnt].aistate.alert |= ALERT_CALLEDFORHELP;
    };
  }

  // TODO: make a yelping sound

}

//--------------------------------------------------------------------------------------------
void give_experience( Game_t * gs, CHR_REF character, int amount, EXPERIENCE xptype )
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function

  int newamount;
  int curlevel, nextexperience;
  int number;
  OBJ_REF iobj;
  SoundState_t * snd;
  Chr_t * pchr;
  Cap_t * pcap;
  Uint32 loc_rand;

  loc_rand = gs->randie_index;

  // Figure out how much experience to give
  pchr = ChrList_getPChr(gs, character);
  if(NULL == pchr)
    return;

  pcap = ChrList_getPCap(gs, character);
  iobj = ChrList_getRObj(gs, character);
  newamount = amount;
  if ( xptype < XP_COUNT )
  {
    newamount = amount * pcap->experiencerate[xptype];
  }
  newamount += pchr->experience;
  if ( newamount > MAXXP )  newamount = MAXXP;
  pchr->experience = newamount;


  // Do level ups and stat changes
  curlevel       = pchr->experiencelevel;
  nextexperience = pcap->experiencecoeff * ( curlevel + 1 ) * ( curlevel + 1 ) + pcap->experienceconst;
  while ( pchr->experience >= nextexperience )
  {
    // The character is ready to advance...
    if ( chr_is_player(gs, character) )
    {
      snd = snd_getState(gs->cd);
      debug_message( 1, "%s gained a level!!!", pchr->name );
      snd_play_sound(gs, 1.0f, pchr->ori.pos, snd->mc_list[GSOUND_LEVELUP], 0, INVALID_OBJ, GSOUND_LEVELUP);
    }
    pchr->experiencelevel++;

    // Size
    if (( pchr->sizegoto + pcap->sizeperlevel ) < 1 + ( pcap->sizeperlevel*10 ) ) pchr->sizegoto += pcap->sizeperlevel;
    pchr->sizegototime += DELAY_RESIZE * 100;

    // Strength
    number = generate_unsigned( &loc_rand, &pcap->statdata.strengthperlevel_pair );
    number += pchr->stats.strength_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    pchr->stats.strength_fp8 = number;

    // Wisdom
    number = generate_unsigned( &loc_rand, &pcap->statdata.wisdomperlevel_pair );
    number += pchr->stats.wisdom_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    pchr->stats.wisdom_fp8 = number;

    // Intelligence
    number = generate_unsigned( &loc_rand, &pcap->statdata.intelligenceperlevel_pair );
    number += pchr->stats.intelligence_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    pchr->stats.intelligence_fp8 = number;

    // Dexterity
    number = generate_unsigned( &loc_rand, &pcap->statdata.dexterityperlevel_pair );
    number += pchr->stats.dexterity_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    pchr->stats.dexterity_fp8 = number;

    // Life
    number = generate_unsigned( &loc_rand, &pcap->statdata.lifeperlevel_pair );
    number += pchr->stats.lifemax_fp8;
    if ( number > PERFECTBIG ) number = PERFECTBIG;
    pchr->stats.life_fp8 += ( number - pchr->stats.lifemax_fp8 );
    pchr->stats.lifemax_fp8 = number;

    // Mana
    number = generate_unsigned( &loc_rand, &pcap->statdata.manaperlevel_pair );
    number += pchr->stats.manamax_fp8;
    if ( number > PERFECTBIG ) number = PERFECTBIG;
    pchr->stats.mana_fp8 += ( number - pchr->stats.manamax_fp8 );
    pchr->stats.manamax_fp8 = number;

    // Mana Return
    number = generate_unsigned( &loc_rand, &pcap->statdata.manareturnperlevel_pair );
    number += pchr->stats.manareturn_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    pchr->stats.manareturn_fp8 = number;

    // Mana Flow
    number = generate_unsigned( &loc_rand, &pcap->statdata.manaflowperlevel_pair );
    number += pchr->stats.manaflow_fp8;
    if ( number > PERFECTSTAT ) number = PERFECTSTAT;
    pchr->stats.manaflow_fp8 = number;

    curlevel       = pchr->experiencelevel;
    nextexperience = pcap->experiencecoeff * ( curlevel + 1 ) * ( curlevel + 1 ) + pcap->experienceconst;
  }
}


//--------------------------------------------------------------------------------------------
void give_team_experience( Game_t * gs, TEAM_REF team, int amount, EXPERIENCE xptype )
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function

  CHR_REF chr_cnt;

  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    if( !ACTIVE_CHR(gs->ChrList, chr_cnt) ) continue;

    if ( gs->ChrList[chr_cnt].team == team )
    {
      give_experience( gs, chr_cnt, amount, xptype );
    }
  }
}


//--------------------------------------------------------------------------------------------
void setup_alliances( Game_t * gs, char *modname )
{
  // ZZ> This function reads the alliance file

  STRING newloadname, szTemp;
  TEAM_REF teama, teamb;
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

      gs->TeamList[teama].hatesteam[REF_TO_INT(teamb)] = bfalse;
    }
    fs_fileClose( fileread );
  }
}

//--------------------------------------------------------------------------------------------
void check_respawn( Game_t * gs )
{
  CHR_REF chr_cnt;

  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    // Let players respawn
    if ( gs->modstate.respawnvalid && !gs->ChrList[chr_cnt].alive && HAS_SOME_BITS( gs->ChrList[chr_cnt].aistate.latch.b, LATCHBUTTON_RESPAWN ) )
    {
      respawn_character( gs, chr_cnt );
      gs->TeamList[gs->ChrList[chr_cnt].team].leader = chr_cnt;
      gs->ChrList[chr_cnt].aistate.alert |= ALERT_CLEANEDUP;

      // Cost some experience for doing this...
      gs->ChrList[chr_cnt].experience *= EXPKEEP;
    }

    gs->ChrList[chr_cnt].aistate.latch.b &= ~LATCHBUTTON_RESPAWN;
  }

};



//--------------------------------------------------------------------------------------------
void begin_integration( Game_t * gs )
{
  CHR_REF chr_cnt;
  Chr_t * pchr;

  PRT_REF prt_cnt;
  Prt_t * pprt;

  for( chr_cnt=0; chr_cnt<CHRLST_COUNT; chr_cnt++)
  {
    if( !ACTIVE_CHR(gs->ChrList, chr_cnt) ) continue;
    pchr = gs->ChrList + chr_cnt;

    CPhysAccum_clear( &(pchr->accum) );
  };

  for( prt_cnt=0; prt_cnt<PRTLST_COUNT; prt_cnt++)
  {
    if( !ACTIVE_PRT(gs->PrtList, prt_cnt) ) continue;
    pprt = gs->PrtList + prt_cnt;

    CPhysAccum_clear( &(pprt->accum) );

    prt_calculate_bumpers(gs, prt_cnt);
  };

};

//--------------------------------------------------------------------------------------------
bool_t chr_collide_mesh(Game_t * gs, CHR_REF ichr)
{
  float meshlevel;
  vect3 norm;
  bool_t hitmesh = bfalse;
  Chr_t * pchr;

  Mesh_t * pmesh = Game_getMesh(gs);

  if( !ACTIVE_CHR( gs->ChrList, ichr ) ) return hitmesh;

  pchr = gs->ChrList + ichr;

  if ( 0 != chr_hitawall( gs, gs->ChrList + ichr, &norm ) )
  {
    float dotprod = DotProduct(norm, pchr->ori.vel);

    if(dotprod < 0.0f)
    {
      // do the reflection
      pchr->accum.vel.x += -(1.0f + pchr->dampen) * dotprod * norm.x - pchr->ori.vel.x;
      pchr->accum.vel.y += -(1.0f + pchr->dampen) * dotprod * norm.y - pchr->ori.vel.y;
      pchr->accum.vel.z += -(1.0f + pchr->dampen) * dotprod * norm.z - pchr->ori.vel.z;

      // if there is reflection, go back to the last valid position
      if( 0.0f != norm.x ) pchr->accum.pos.x += pchr->ori_old.pos.x - pchr->ori.pos.x;
      if( 0.0f != norm.y ) pchr->accum.pos.y += pchr->ori_old.pos.y - pchr->ori.pos.y;
      if( 0.0f != norm.z ) pchr->accum.pos.z += pchr->ori_old.pos.z - pchr->ori.pos.z;

      hitmesh = btrue;
    };
  }

  meshlevel = mesh_get_level( &(pmesh->Mem), pchr->onwhichfan, pchr->ori.pos.x, pchr->ori.pos.y, gs->ChrList[ichr].prop.waterwalk, &(gs->Water) );
  if( pchr->ori.pos.z < meshlevel )
  {
    hitmesh = btrue;

    pchr->accum.pos.z += meshlevel + 0.001f - pchr->ori.pos.z;

    if ( pchr->ori.vel.z < -STOPBOUNCING )
    {
      pchr->accum.vel.z += -pchr->ori.vel.z * ( 1.0f + pchr->dampen );
    }
    else if ( pchr->ori.vel.z < STOPBOUNCING )
    {
      pchr->accum.vel.z += -pchr->ori.vel.z;
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
bool_t prt_collide_mesh(Game_t * gs, PRT_REF iprt)
{
  float meshlevel, dampen;
  CHR_REF attached;
  vect3 norm;
  bool_t hitmesh = bfalse;
  PIP_REF pip;

  Mesh_t * pmesh = Game_getMesh(gs);

  if( !ACTIVE_PRT( gs->PrtList, iprt) ) return hitmesh;

  attached = prt_get_attachedtochr(gs, iprt);
  pip      = gs->PrtList[iprt].pip;
  dampen   = gs->PipList[pip].dampen;

  if ( 0 != prt_hitawall( gs, iprt, &norm ) )
  {
    float dotprod = DotProduct(norm, gs->PrtList[iprt].ori.vel);

    if(dotprod < 0.0f)
    {
      float vel_xy = ABS(gs->PrtList[iprt].ori.vel.x) + ABS(gs->PrtList[iprt].ori.vel.y);
      snd_play_particle_sound( gs, gs->PrtList[iprt].bumpstrength*MIN( 1.0f, vel_xy / 10.0f ), iprt, gs->PipList[pip].soundwall );

      if ( gs->PipList[pip].endwall )
      {
        gs->PrtList[iprt].gopoof = btrue;
      }
      else if( !gs->PipList[pip].rotatetoface && !ACTIVE_CHR(gs->ChrList, attached) )  // "rotate to face" gets it's own treatment
      {
        vect3 old_vel;
        float dotprodN;
        float dampen = gs->PipList[gs->PrtList[iprt].pip].dampen;

        old_vel = gs->PrtList[iprt].ori.vel;

        // do the reflection
        dotprodN = DotProduct(norm, gs->PrtList[iprt].ori.vel);
        gs->PrtList[iprt].accum.vel.x += -(1.0f + dampen) * dotprodN * norm.x - gs->PrtList[iprt].ori.vel.x;
        gs->PrtList[iprt].accum.vel.y += -(1.0f + dampen) * dotprodN * norm.y - gs->PrtList[iprt].ori.vel.y;
        gs->PrtList[iprt].accum.vel.z += -(1.0f + dampen) * dotprodN * norm.z - gs->PrtList[iprt].ori.vel.z;

        // Change facing
        // determine how much the billboarded particle should rotate on reflection from
        // the rotation of the velocity vector about the vector to the camera
        {
          vect3 vec_out, vec_up, vec_right, wld_up;
          float old_vx, old_vy, new_vx, new_vy;

          Uint16 new_turn, old_turn;

          vec_out.x = gs->PrtList[iprt].ori.pos.x - GCamera.pos.x;
          vec_out.y = gs->PrtList[iprt].ori.pos.y - GCamera.pos.y;
          vec_out.z = gs->PrtList[iprt].ori.pos.z - GCamera.pos.z;

          wld_up.x = 0;
          wld_up.y = 0;
          wld_up.z = 1;

          vec_right = Normalize( CrossProduct( wld_up, vec_out ) );
          vec_up    = Normalize( CrossProduct( vec_right, vec_out ) );

          old_vx = DotProduct(old_vel, vec_right);
          old_vy = DotProduct(old_vel, vec_up);
          old_turn = vec_to_turn(old_vx, old_vy);

          new_vx = DotProduct(gs->PrtList[iprt].ori.vel, vec_right);
          new_vy = DotProduct(gs->PrtList[iprt].ori.vel, vec_up);
          new_turn = vec_to_turn(new_vx, new_vy);

          gs->PrtList[iprt].facing += new_turn - old_turn;
        }
      }

      // if there is reflection, go back to the last valid position
      if( 0.0f != norm.x ) gs->PrtList[iprt].accum.pos.x += gs->PrtList[iprt].ori_old.pos.x - gs->PrtList[iprt].ori.pos.x;
      if( 0.0f != norm.y ) gs->PrtList[iprt].accum.pos.y += gs->PrtList[iprt].ori_old.pos.y - gs->PrtList[iprt].ori.pos.y;
      if( 0.0f != norm.z ) gs->PrtList[iprt].accum.pos.z += gs->PrtList[iprt].ori_old.pos.z - gs->PrtList[iprt].ori.pos.z;

      hitmesh = btrue;
    };
  }

  meshlevel = mesh_get_level( &(pmesh->Mem), gs->PrtList[iprt].onwhichfan, gs->PrtList[iprt].ori.pos.x, gs->PrtList[iprt].ori.pos.y, bfalse, &(gs->Water) );
  if( gs->PrtList[iprt].ori.pos.z < meshlevel )
  {
    hitmesh = btrue;

    if(gs->PrtList[iprt].ori.vel.z < 0.0f)
    {
      snd_play_particle_sound( gs, MIN( 1.0f, -gs->PrtList[iprt].ori.vel.z / 10.0f ), iprt, gs->PipList[pip].soundfloor );
    };

    if( gs->PipList[pip].endground )
    {
      gs->PrtList[iprt].gopoof = btrue;
    }
    else if( !ACTIVE_CHR( gs->ChrList, attached ) )
    {
      gs->PrtList[iprt].accum.pos.z += meshlevel + 0.001f - gs->PrtList[iprt].ori.pos.z;

      if ( gs->PrtList[iprt].ori.vel.z < -STOPBOUNCING )
      {
        gs->PrtList[iprt].accum.vel.z -= gs->PrtList[iprt].ori.vel.z * ( 1.0f + dampen );
      }
      else if ( gs->PrtList[iprt].ori.vel.z < STOPBOUNCING )
      {
        gs->PrtList[iprt].accum.vel.z -= gs->PrtList[iprt].ori.vel.z;
      }
    };

  }

  return hitmesh;
};

//--------------------------------------------------------------------------------------------
void do_integration(Game_t * gs, float dFrame)
{
  // BB > Integrate the position of the characters/items and particles.
  //      Handle the interaction with the walls. Make sure that all positions
  //      are on valid mesh tiles. This routine could be fooled if the character
  //      was standing on a tile that became invalid since the last frame.

  int tnc;
  bool_t collide;
  Orientation_t ori_tmp;

  CHR_REF chr_cnt;
  Chr_t   *pchr;

  PRT_REF prt_cnt;
  Prt_t   *pprt;

  for( chr_cnt=0; chr_cnt<CHRLST_COUNT; chr_cnt++)
  {
    if( !ACTIVE_CHR(gs->ChrList, chr_cnt) ) continue;

    pchr = gs->ChrList + chr_cnt;

    ori_tmp = pchr->ori_old;
    phys_integrate( &(pchr->ori), &(pchr->ori_old), &(pchr->accum), dFrame );

    // iterate through the integration routine until you force the new position to be valid
    // should only ever go through the loop twice

    collide = bfalse;
    for( tnc = 0; tnc < 20; tnc++ )
    {
      collide = chr_collide_mesh(gs, chr_cnt);
      if(!collide) break;

      pchr->ori.pos.x += pchr->accum.pos.x;
      pchr->ori.pos.y += pchr->accum.pos.y;
      pchr->ori.pos.z += pchr->accum.pos.z;

      pchr->ori.vel.x += pchr->accum.vel.x;
      pchr->ori.vel.y += pchr->accum.vel.y;
      pchr->ori.vel.z += pchr->accum.vel.z;

      CPhysAccum_clear( &(pchr->accum) );
    };

    if(collide)
    {
      pchr->ori     = pchr->ori_old;
      pchr->ori_old = ori_tmp;
    }
  };

  for( prt_cnt=0; prt_cnt<PRTLST_COUNT; prt_cnt++)
  {
    if( !ACTIVE_PRT(gs->PrtList, prt_cnt) ) continue;

    pprt = gs->PrtList + prt_cnt;

    ori_tmp = pprt->ori_old;
    phys_integrate( &(pprt->ori), &(pprt->ori_old), &(pprt->accum), dFrame );

    // iterate through the integration routine until you force the new position to be valid
    // should only ever go through the loop twice

    collide = bfalse;
    for( tnc = 0; tnc < 20; tnc++ )
    {
      collide = prt_collide_mesh(gs, prt_cnt);
      if(!collide) break;

      pprt->ori.pos.x += pprt->accum.pos.x;
      pprt->ori.pos.y += pprt->accum.pos.y;
      pprt->ori.pos.z += pprt->accum.pos.z;

      pprt->ori.vel.x += pprt->accum.vel.x;
      pprt->ori.vel.y += pprt->accum.vel.y;
      pprt->ori.vel.z += pprt->accum.vel.z;

      CPhysAccum_clear( &(pprt->accum) );
    };

    if(collide)
    {
      pprt->ori     = pprt->ori_old;
      pprt->ori_old = ori_tmp;
    }
  };

};

//--------------------------------------------------------------------------------------------
void update_timers(Game_t * gs)
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
  gfxState.fps_clock += gs->all_clock - gs->lst_clock;

  if ( ups_loops > 0 && ups_clock > 0)
  {
    stabilized_ups_sum    = stabilized_ups_sum * 0.99 + 0.01 * ( float ) ups_loops / (( float ) ups_clock / TICKS_PER_SEC );
    stabilized_ups_weight = stabilized_ups_weight * 0.99 + 0.01;
  };

  if ( gfxState.fps_loops > 0 && gfxState.fps_clock > 0 )
  {
    gfxState.stabilized_fps_sum    = gfxState.stabilized_fps_sum * 0.99 + 0.01 * ( float ) gfxState.fps_loops / (( float ) gfxState.fps_clock / TICKS_PER_SEC );
    gfxState.stabilized_fps_weight = gfxState.stabilized_fps_weight * 0.99 + 0.01;
  };

  if ( ups_clock >= TICKS_PER_SEC )
  {
    ups_clock = 0;
    ups_loops = 0;
    assert(stabilized_ups_weight>0);
    stabilized_ups = stabilized_ups_sum / stabilized_ups_weight;
  }

  if ( gfxState.fps_clock >= TICKS_PER_SEC )
  {
    gfxState.fps_clock = 0;
    gfxState.fps_loops = 0;
    assert(gfxState.stabilized_fps_weight>0);
    gfxState.stabilized_fps = gfxState.stabilized_fps_sum / gfxState.stabilized_fps_weight;
  }

}




//--------------------------------------------------------------------------------------------
void reset_teams(Game_t * gs)
{
  // ZZ> This function makes everyone hate everyone else

  TEAM_REF teama, teamb;

  teama = 0;
  while ( teama < TEAM_COUNT )
  {
    // Make the team hate everyone
    teamb = 0;
    while ( teamb < TEAM_COUNT )
    {
      gs->TeamList[teama].hatesteam[REF_TO_INT(teamb)] = btrue;
      teamb++;
    }
    // Make the team like itself
    gs->TeamList[teama].hatesteam[REF_TO_INT(teama)] = bfalse;

    // Set defaults
    gs->TeamList[teama].leader = INVALID_CHR;
    gs->TeamList[teama].sissy = 0;
    gs->TeamList[teama].morale = 0;
    teama++;
  }

  // Keep the null team neutral
  teama = 0;
  while ( teama < TEAM_COUNT )
  {
    gs->TeamList[teama].hatesteam[TEAM_NULL] = bfalse;
    gs->TeamList[TEAM_REF(TEAM_NULL)].hatesteam[REF_TO_INT(teama)] = bfalse;
    teama++;
  }
}

//--------------------------------------------------------------------------------------------
void reset_messages(Game_t * gs)
{
  // ZZ> This makes messages safe to use
  Gui_t * gui = gui_getState();

  clear_messages( &(gs->MsgList) );
  clear_message_queue( &(gui->msgQueue) );
}

//--------------------------------------------------------------------------------------------
void reset_timers(Game_t * gs)
{
  // ZZ> This function resets the timers...

  gs->stt_clock  = SDL_GetTicks();
  gs->all_clock  = 0;
  gs->lst_clock  = 0;
  gs->wld_clock  = 0;
  gs->cl->stat_clock = 0;
  gs->pits_clock  = 0;  gs->pits_kill = bfalse;
  gs->wld_frame  = 0;

  outofsync = bfalse;

  ups_loops = 0;
  ups_clock = 0;
  gfxState.fps_loops = 0;
  gfxState.fps_clock = 0;
}

extern int initMenus();

#define DO_CONFIGSTRING_COMPARE(XX) if(0 == strncmp(#XX, szin, strlen(#XX))) { if(NULL !=szout) *szout = szin + strlen(#XX); return cd->XX; }
//--------------------------------------------------------------------------------------------
char * get_config_string(ConfigData_t * cd, char * szin, char ** szout)
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
  else DO_CONFIGSTRING_COMPARE(spawn2_file)
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
char * get_config_string_name(ConfigData_t * cd, STRING * pconfig_string)
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
  else if(pconfig_string == &(cd->spawn2_file)) return "spawn2_file";
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
void set_default_config_data(ConfigData_t * pcon)
{
  if(NULL ==pcon) return;

  strncpy( pcon->basicdat_dir, "basicdat", sizeof( STRING ) );
  strncpy( pcon->gamedat_dir, "gamedat" , sizeof( STRING ) );
  strncpy( pcon->mnu_dir, "menu" , sizeof( STRING ) );
  strncpy( pcon->globalparticles_dir, "globalparticles" , sizeof( STRING ) );
  strncpy( pcon->modules_dir, "modules" , sizeof( STRING ) );
  strncpy( pcon->music_dir, "music" , sizeof( STRING ) );
  strncpy( pcon->objects_dir, "objects" , sizeof( STRING ) );
  strncpy( pcon->import_dir, "import" , sizeof( STRING ) );
  strncpy( pcon->players_dir, "players", sizeof( STRING ) );

  strncpy( pcon->nullicon_bitmap, "nullicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->keybicon_bitmap, "keybicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->mousicon_bitmap, "mousicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->joyaicon_bitmap, "joyaicon.bmp" , sizeof( STRING ) );
  strncpy( pcon->joybicon_bitmap, "joybicon.bmp" , sizeof( STRING ) );

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
  strncpy( pcon->spawn2_file, "spawn2.txt" , sizeof( STRING ) );
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


  pcon->uifont_points_large = 22;
  pcon->uifont_points_small = 17;
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
  pcon->wraptolerance = 80;     // Status_t bar
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

int proc_mainLoop( ProcState_t * ego_proc, int argc, char **argv );
int proc_gameLoop( ProcState_t * gproc, Game_t * gs);
int proc_menuLoop( MenuProc_t  * mproc );

//--------------------------------------------------------------------------------------------
void main_handleKeyboard()
{
  bool_t control, alt, shift, mod;

  control = (SDLKEYDOWN(SDLK_RCTRL) || SDLKEYDOWN(SDLK_LCTRL));
  alt     = (SDLKEYDOWN(SDLK_RALT) || SDLKEYDOWN(SDLK_LALT));
  shift   = (SDLKEYDOWN(SDLK_RSHIFT) || SDLKEYDOWN(SDLK_LSHIFT));
  mod     = control || alt || shift;

  // do screenshot
  if ( SDLKEYDOWN( SDLK_F11 ) && !mod )
  {
    if(!do_screenshot())
    {
      debug_message( 1, "Error writing screenshot" );
      log_warning( "Error writing screenshot\n" );    //Log the error in log.txt
    }
  }

};

//--------------------------------------------------------------------------------------------
void do_sunlight(LIGHTING_INFO * info)
{
  // BB > simulate sunlight

  static bool_t first_time = btrue;
  static time_t i_time_last;
  int    dtime;
  vect3 transmit;
  vect3 night_ambi;
  vect3 day_spek;

  MachineState_t * mac = get_MachineState();

  // check to see if we're outdoors
  if(!info->on)
  {
    glClearColor( 0,0,0,1 );
    return;
  };

  // the color of the night sky
  night_ambi.r = 0.025;
  night_ambi.g = 0.044;
  night_ambi.b = 0.100;

  // the transmission of sunlight through the atmosphere at mid-day
  day_spek.r = 0.666f;
  day_spek.g = 0.628f;
  day_spek.b = 0.345f;

  // update the sun position every 10 seconds
  MachineState_update(mac);
  dtime = mac->i_actual_time - i_time_last;
  if( first_time || dtime > 3 )
  {
    float  f_sky_pos;
    float sval, cval, ftmp;

    first_time  = bfalse;
    i_time_last = mac->i_actual_time;
    f_sky_pos = (mac->f_bishop_time_h - 12.0f) / 24.0f;

    sval = sin( f_sky_pos * TWO_PI );
    cval = cos( f_sky_pos * TWO_PI );

    //info->spekdir.y = sval;
    //info->spekdir.z = cval;
    info->spekdir.z = 0.1;
    info->spekdir.y = sqrt( 1.0f - pow(info->spekdir.z,2));

    if ( info->spekdir.z > 0 )
    {
      // do the filtering

      // do the "sunset effect"
      ftmp = (1 - info->spekdir.z) * 2.0;
      transmit.r = exp(-ftmp * 0.381f );
      transmit.g = exp(-ftmp * 0.437f );
      transmit.b = exp(-ftmp * 1.000f );

      // do the "blue sky" effect
      // everything that doesn't come from the sun comes from the blue sky
      info->spekcol.r = day_spek.r * transmit.r;
      info->spekcol.g = day_spek.g * transmit.g;
      info->spekcol.b = day_spek.b * transmit.b;

      info->ambicol.r = ( 1.0f - day_spek.r ) * transmit.r;
      info->ambicol.g = ( 1.0f - day_spek.g ) * transmit.r;
      info->ambicol.b = ( 1.0f - day_spek.b ) * transmit.r;

      // add in the "night sky" effect
      // smoothly interpolate down to the "night" setting
      info->ambicol.r += night_ambi.r;
      info->ambicol.g += night_ambi.g;
      info->ambicol.b += night_ambi.b;
    }
    else
    {
      info->spekcol.r = 0.0f;
      info->spekcol.g = 0.0f;
      info->spekcol.b = 0.0f;

      info->ambicol.r = night_ambi.r;
      info->ambicol.g = night_ambi.g;
      info->ambicol.b = night_ambi.b;
    };

    glClearColor( info->ambicol.r, info->ambicol.g, info->ambicol.b, 1 );

    // update the spektable since the sunlight may have changes
    make_spektable( info->spekdir );
  }

};

//--------------------------------------------------------------------------------------------
retval_t main_doGameGraphics()
{
  Game_t * gs = Graphics_requireGame(&gfxState);


  if( !EKEY_PVALID(gs) ) return rv_error;
  if( !gs->proc.Active ) { gs->dFrame = 0; return rv_succeed; };

  move_camera( UPDATESCALE );

  // simulate sunlight. runs on its own clock.
  do_sunlight(&gs->Light);

  if ( gs->dFrame >= 1.0 )
  {
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

  return rv_succeed;
}
//--------------------------------------------------------------------------------------------
// the main loop. Processes all graphics

retval_t main_doGraphics()
{
  Gui_t * gui;
  MenuProc_t * mnu_proc;

  double frameDuration, frameTicks;
  bool_t do_menu_frame = bfalse;
  bool_t do_game_frame = bfalse;

  gui = gui_getState();
  if (NULL == gui) return rv_error;
  mnu_proc = &(gui->mnu_proc);

  // Clock updates each frame
  ClockState_frameStep( gui->clk );
  frameDuration = ClockState_getFrameDuration( gui->clk );
  frameTicks    = frameDuration * TICKS_PER_SEC;
  gui->dUpdate  += frameTicks / UPDATESKIP;

  // read the input
  input_read();

  // handle the keyboard input
  main_handleKeyboard();

  // blank the screen, if required
  do_clear();

  do_game_frame = Graphics_hasGame(&gfxState) && Graphics_requireGame(&gfxState)->proc.Active;
  if(do_game_frame)
  {
    // update the game graphics
    main_doGameGraphics();
  };

  // figure out if we need to process the menus
  if(do_menu_frame)
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
    if ( do_game_frame )
    {
      Game_t * gs               = Graphics_requireGame(&gfxState);
      MenuProc_t  * ig_mnu_proc = Game_getMenuProc(gs);
      ProcState_t * game_proc   = Game_getProcedure(gs);

      if(ig_mnu_proc->proc.Active)
      {
        ui_beginFrame();
        {
          mnu_RunIngame( ig_mnu_proc );
          switch ( ig_mnu_proc->MenuResult  )
          {
            case 1: /* nothing */
              break;

            case - 1:
              // Done with the menu
              ig_mnu_proc->proc.Active = bfalse;
              mous.game = btrue;
              break;
          }

        }
        ui_endFrame();

        request_pageflip();
      }
    }

    // Do the out-of-game menu, if it is active
    if ( mnu_proc->proc.Active && !mnu_proc->proc.Paused )
    {
      proc_menuLoop( mnu_proc );

      switch ( mnu_proc->proc.returnValue )
      {
        case  1:

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
int proc_mainLoop( ProcState_t * ego_proc, int argc, char **argv )
{
  // ZZ> This is where the program starts and all the high level stuff happens
  static double frameDuration, frameTicks;
  Game_t    * gs;
  GameStack_t      * stk;
  MachineState_t * mach_state;
  int i;

  mach_state = get_MachineState();
  stk        = Get_GameStack();

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
        memcpy(&CData, &CData_default, sizeof(ConfigData_t));

        // start initializing the various subsystems
        log_info( "proc_mainLoop() - \n\tStarting Egoboo %s...\n", VERSION );

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

        // start the Gui_t
        Gui_startUp();

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
        ui_initialize(CStringTmp1, CData.uifont_points_large);

        // initialize the sound system
        snd_initialize(&CData);
        load_all_music_sounds( &CData);

        // allocate the maximum amount of mesh memory
        TileDictionary_load(&gTileDict);

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
        Gui_t     * gui;
        MenuProc_t     * mnu_proc;

        gui        = gui_getState();               // automatically starts the Gui_t
        mnu_proc   = &(gui->mnu_proc);

        PROFILE_INIT( resize_characters );
        PROFILE_INIT( keep_weapons_with_holders );
        PROFILE_INIT( run_all_scripts );
        PROFILE_INIT( do_weather_spawn );
        PROFILE_INIT( enc_spawn_particles );
        PROFILE_INIT( Client_unbufferLatches );
        PROFILE_INIT( CServer_unbufferLatches );
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

        PROFILE_INIT( ChrHeap );
        PROFILE_INIT( EncHeap );
        PROFILE_INIT( PrtHeap );


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
        Gui_t * gui;
        MenuProc_t * mnu_proc;
        bool_t     no_games_active;

        PROFILE_BEGIN( main_loop );

        gui        = gui_getState();               // automatically starts the Gui_t
        mnu_proc   = &(gui->mnu_proc);

        main_handleKeyboard();

        no_games_active = btrue;
        for(i=0; i<stk->count; i++)
        {
          ProcState_t * proc;

          gs   = GameStack_get(stk, i);
          proc = Game_getProcedure(gs);

          // a kludge to make sure the the gfxState is connected to something
          Graphics_ensureGame(&gfxState, gs);

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

        PROFILE_END( main_loop );

        gfxState.est_max_fps = 0.9 * gfxState.est_max_fps + 0.1 * (1.0f / PROFILE_QUERY(main_loop) );

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
        Gui_t     * gui;
        MenuProc_t     * mnu_proc;
        bool_t all_games_finished;

        gui        = gui_getState();               // automatically starts the Gui_t
        mnu_proc   = &(gui->mnu_proc);

        ClockState_frameStep( mach_state->clk );
        frameDuration = ClockState_getFrameDuration( mach_state->clk );

        // request that the menu loop to clean itself.
        // the menu loop is only connected to the Game_t that
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
          gs = GameStack_get(stk, i);

          // force the main loop to clean itself
          if ( !gs->proc.Terminated )
          {
            gs->proc.KillMe = btrue;
            proc_gameLoop( Game_getProcedure(gs), gs);
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

#if defined(DEBUG_PROFILE)
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
        log_info( "\trun_all_scripts - %lf\n", 1e6*PROFILE_QUERY( run_all_scripts ) );
        log_info( "\tdo_weather_spawn - %lf\n", 1e6*PROFILE_QUERY( do_weather_spawn ) );
        log_info( "\tdo_enchant_spawn - %lf\n", 1e6*PROFILE_QUERY( enc_spawn_particles ) );
        log_info( "\tCClient_unbufferLatches - %lf\n", 1e6*PROFILE_QUERY( Client_unbufferLatches ) );
        log_info( "\tCServer_unbufferLatches - %lf\n", 1e6*PROFILE_QUERY( CServer_unbufferLatches ) );
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
#endif

        PROFILE_FREE( resize_characters );
        PROFILE_FREE( keep_weapons_with_holders );
        PROFILE_FREE( run_all_scripts );
        PROFILE_FREE( do_weather_spawn );
        PROFILE_FREE( enc_spawn_particles );
        PROFILE_FREE( Client_unbufferLatches );
        PROFILE_FREE( CServer_unbufferLatches );
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
          gs = GameStack_get(stk, i);
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
void game_handleKeyboard()
{
  Game_t      * gs = Graphics_requireGame(&gfxState);
  ProcState_t * gproc = Game_getProcedure(gs);
  Gui_t       * gui = gui_getState();
  bool_t control, alt, shift, mod;

  if( !EKEY_PVALID(gs) ) return;

  control = (SDLKEYDOWN(SDLK_RCTRL) || SDLKEYDOWN(SDLK_LCTRL));
  alt     = (SDLKEYDOWN(SDLK_RALT) || SDLKEYDOWN(SDLK_LALT));
  shift   = (SDLKEYDOWN(SDLK_RSHIFT) || SDLKEYDOWN(SDLK_LSHIFT));
  mod     = control || alt || shift;

  // do game pause if needed
  if (!SDLKEYDOWN(SDLK_F8) && !mod)  gui->can_pause = btrue;
  if (SDLKEYDOWN(SDLK_F8) && !mod && gs->proc.Active && keyb.on && gui->can_pause)
  {
    gs->proc.Paused = !gs->proc.Paused;
    gui->can_pause = bfalse;
  }

  // Check for quitters
  if ((gs->proc.Active && SDLKEYDOWN(SDLK_ESCAPE) && !mod) || (gs->modstate.Active && gs->allpladead) )
  {
    gs->igm.proc.Active = btrue;
    gs->igm.whichMenu   = mnu_Quit;
  }

  // do in-game-menu
  if(SDLKEYDOWN(SDLK_F9) && !mod && CData.DevMode)
  {
    gs->igm.proc.Active = btrue;
    gs->igm.whichMenu   = mnu_Inventory;
  }

  // force the module to be beaten
  if(SDLKEYDOWN(SDLK_F9) && control && CData.DevMode)
  {
    // make it so that we truely beat the module
    gs->modstate.beat        = btrue;
    gs->modstate.exportvalid = gs->modstate.exportvalid || (1 == gs->modstate.import_max_pla);

    gs->igm.proc.Active = btrue;
    gs->igm.whichMenu   = mnu_EndGame;
  }

  // do frame step in single-frame mode
  if( gs->Do_frame )
  {
    gs->Do_frame = bfalse;
  }
  else if( SDLKEYDOWN( SDLK_F10 ) && !mod )
  {
    keyb.state[SDLK_F10] = 0;
    gs->Do_frame = btrue;
  }

  //Pressed panic button
  if ( SDLKEYDOWN( SDLK_q )  && control )
  {
    log_info( "User pressed escape button (CTRL+Q)... Quitting game gracefully.\n" );
    gproc->State = PROC_Leaving;

    // <BB> memory_cleanUp() should be kept in PROC_Finish, so that we can make sure to deallocate
    //      the memory for any active menus, and/or modules.  Alternately, we could
    //      register all of the memory deallocation routines with atexit() so they will
    //      be called automatically on a return from the main function or a call to exit()
  }
}

//--------------------------------------------------------------------------------------------
void game_handleIO(Game_t * gs)
{
  set_local_latches( gs );                   // client function

  // NETWORK PORT
  //if(net_Started())
  {
    // buffer the existing latches
    do_chat_input();        // client function
  }

  // this needs to be done even if the network is not on
  Client_bufferLatches(gs->cl);     // client function

  if(net_Started())
  {
    CServer_bufferLatches(gs->sv);     // server function

    // upload the information
    Client_talkToHost(gs->cl);        // client function
    sv_talkToRemotes(gs->sv);     // server function
  }
};

//--------------------------------------------------------------------------------------------
void cl_update_game(Game_t * gs, float dUpdate, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.
  int cnt;

  Gui_t * gui = gui_getState();

  count_players(gs);

  // This is the main game loop
  gui->msgQueue.timechange = 0;

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync
  while ( (gs->wld_clock<gs->all_clock) && (gs->cl->tlb.numtimes > 0))
  {
    srand(gs->sv->rand_idx);                          // client/server function

    PROFILE_BEGIN( resize_characters );
    resize_characters( gs, dUpdate );                 // client function
    PROFILE_END( resize_characters );

    PROFILE_BEGIN( keep_weapons_with_holders );
    keep_weapons_with_holders( gs );                  // client-ish function
    PROFILE_END( keep_weapons_with_holders );

    // unbuffer the updated latches after run_all_scripts() and before move_characters()
    PROFILE_BEGIN( Client_unbufferLatches );
    Client_unbufferLatches( gs->cl );              // client function
    PROFILE_END( Client_unbufferLatches );

    PROFILE_BEGIN( check_respawn );
    check_respawn( gs );                         // client function
    ChrList_resynch( gs );
    PrtList_resynch( gs );
    EncList_resynch( gs );
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
    make_character_matrices( gs );                // client function
    PROFILE_END( make_character_matrices );

    PROFILE_BEGIN( stat_return );
    stat_return( gs, dUpdate );                           // client/server function
    PROFILE_END( stat_return );

    PROFILE_BEGIN( pit_kill );
    pit_kill( gs, dUpdate );                           // client/server function
    PROFILE_END( pit_kill );

    {
      // Generate the new seed
      gs->sv->rand_idx += *((Uint32*) & kMd2Normals[gs->wld_frame&127][0]);
      gs->sv->rand_idx += *((Uint32*) & kMd2Normals[gs->sv->rand_idx&127][1]);
    }

    // Stuff for which sync doesn't matter
    //flash_select();                                    // client function

    PROFILE_BEGIN( animate_tiles );
    animate_tiles( &(gs->Tile_Anim), dUpdate );                          // client function
    PROFILE_END( animate_tiles );

    PROFILE_BEGIN( move_water );
    move_water( dUpdate );                            // client function
    PROFILE_END( move_water );

    gui->msgQueue.timechange += dUpdate;

    for(cnt=0; cnt<MAXSTAT; cnt++)
    {
      if (gs->cl->StatList[cnt].delay > 0)
      {
        gs->cl->StatList[cnt].delay--;
      }
    };

    gs->cl->stat_clock++;
  }

  if (net_Started())
  {
    if (gs->cl->tlb.numtimes == 0)
    {
      // The remote ran out of messages, and is now twiddling its thumbs...
      // Make it go slower so it doesn't happen again
      gs->wld_clock += FRAMESKIP/4.0f;
    }
    else if (cl_Started() && gs->cl->tlb.numtimes > 3)
    {
      // The host has too many messages, and is probably experiencing control
      // gs->cd->lag...  Speed it up so it gets closer to sync
      gs->wld_clock -= FRAMESKIP/4.0f;
    }
  }
}

//--------------------------------------------------------------------------------------------
void sv_update_game(Game_t * gs, float dUpdate, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.


  count_players(gs);

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync

  // gs->wld_clock<gs->all_clock  will this work with multiple game states?
  while ( gs->wld_clock<gs->all_clock )
  {
    srand(gs->sv->rand_idx);                          // client/server function

    PROFILE_BEGIN( keep_weapons_with_holders );
    keep_weapons_with_holders( gs );                  // client-ish function
    PROFILE_END( keep_weapons_with_holders );

    PROFILE_BEGIN( run_all_scripts );
    run_all_scripts( gs, dUpdate );                      // server function
    PROFILE_END( run_all_scripts );

    PROFILE_BEGIN( do_weather_spawn );
    do_weather_spawn( gs, dUpdate );                       // server function
    PROFILE_END( do_weather_spawn );

    PROFILE_BEGIN( enc_spawn_particles );
    enc_spawn_particles( gs, dUpdate );                       // server function
    PROFILE_END( enc_spawn_particles );

    // unbuffer the updated latches after run_all_scripts() and before move_characters()
    PROFILE_BEGIN( CServer_unbufferLatches );
    CServer_unbufferLatches( gs->sv );              // server function
    PROFILE_END( CServer_unbufferLatches );

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
      gs->sv->rand_idx += *((Uint32*) & kMd2Normals[gs->wld_frame&127][0]);
      gs->sv->rand_idx += *((Uint32*) & kMd2Normals[gs->sv->rand_idx&127][1]);
    }

  }

}

//--------------------------------------------------------------------------------------------
void update_game(Game_t * gs, float dUpdate, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.

  int cnt;
  Gui_t    * gui = gui_getState();
  Client_t * cs = gs->cl;
  Server_t * ss = gs->sv;

  count_players(gs);

  // This is the main game loop
  gui->msgQueue.timechange = 0;

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync

  // gs->wld_clock<gs->all_clock  will this work with multiple game states?
  //while ( (gs->wld_clock<gs->all_clock) &&  ( cs->tlb.numtimes > 0 ) )
  {
    srand(ss->rand_idx);                          // client/server function

    PROFILE_BEGIN( resize_characters );
    resize_characters( gs, dUpdate );                 // client function
    PROFILE_END( resize_characters );

    PROFILE_BEGIN( keep_weapons_with_holders );
    keep_weapons_with_holders( gs );                  // client-ish function
    PROFILE_END( keep_weapons_with_holders );

    PROFILE_BEGIN( run_all_scripts );
    run_all_scripts( gs, dUpdate );                      // server function
    PROFILE_END( run_all_scripts );

    PROFILE_BEGIN( do_weather_spawn );
    do_weather_spawn( gs, dUpdate );                       // server function
    PROFILE_END( do_weather_spawn );

    PROFILE_BEGIN( enc_spawn_particles );
    enc_spawn_particles( gs, dUpdate );                       // server function
    PROFILE_END( enc_spawn_particles );

    // unbuffer the updated latches after run_all_scripts() and before move_characters()
    PROFILE_BEGIN( Client_unbufferLatches );
    Client_unbufferLatches( cs );              // client function
    PROFILE_END( Client_unbufferLatches );

    PROFILE_BEGIN( CServer_unbufferLatches );
    CServer_unbufferLatches( ss );              // server function
    PROFILE_END( CServer_unbufferLatches );

    PROFILE_BEGIN( check_respawn );
    check_respawn( gs );                         // client function
    ChrList_resynch(gs);
    PrtList_resynch(gs);
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
    make_character_matrices( gs );                // client function
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
    animate_tiles( &(gs->Tile_Anim), dUpdate );                          // client function
    PROFILE_END( animate_tiles );

    PROFILE_BEGIN( move_water );
    move_water( dUpdate );                          // client function
    PROFILE_END( move_water );

    gui->msgQueue.timechange += dUpdate;

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
ProcessStates game_doRun(Game_t * gs, ProcessStates procIn)
{
  ProcessStates procOut = procIn;
  static double dTimeUpdate, dTimeFrame;

  if (NULL == gs)  return procOut;

#if DEBUG_UPDATE_CLAMP && defined(_DEBUG)
  if(CData.DevMode)
  {
    log_info( "wld_frame == %d\t dframe == %2.4f\n", gs->wld_frame, gs->dUpdate );
  };
#endif

  while ( gs->dUpdate >= 1.0 )
  {
    update_timers( gs );

    // Do important things
    //if ( !local_running && !client_running && !server_running )
    //{
    //  gs->wld_clock = gs->all_clock;
    //}
    //else
    if ( gs->Single_frame || (gs->proc.Paused && Game_isLocal(gs)) )
    {
      gs->wld_clock = gs->all_clock;
    }
    else
    {
      // all of this stuff should only be done for the game we are watching game
      if( Graphics_matchesGame(&gfxState, gs) )
      {
        PROFILE_BEGIN( pre_update_game );
        {
          passage_check_music( gs );
          update_looped_sounds( gs );

          // handle the game and network IO
          game_handleIO(gs);
        }
        PROFILE_END( pre_update_game );
      }

      PROFILE_BEGIN( update_game );
      if( Game_isLocal(gs) || Game_isClientServer(gs) )
      {
        // non-networked updates
        update_game(gs, EMULATEUPS / TARGETUPS, &(gs->randie_index) );
      }
      else if( Game_isServer(gs) )
      {
        // server-only updated
        sv_update_game(gs, EMULATEUPS / TARGETUPS, &(gs->randie_index));
      }
      else if( Game_isClient(gs) )
      {
        // client only updates
        cl_update_game(gs, EMULATEUPS / TARGETUPS, &(gs->randie_index));
      }
      else
      {
        log_warning("game_doRun() - \n\tinvalid server/client/local game configuration?");
      }
      PROFILE_END( update_game );
    }

#if DEBUG_UPDATE_CLAMP && defined(_DEBUG)
    if(CData.DevMode)
    {
      log_info( "\t\twldframe == %d\t dframe == %2.4f\n", gs->wld_frame, gs->dUpdate );
    }
#endif

    // Timers
    gs->wld_clock += FRAMESKIP;
    gs->wld_frame++;
    ups_loops++;

    gs->dUpdate -= 1.0f;
  };

  return procOut;
}

//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
// TODO : this should be re-factored so that graphics updates are in a different loop
int proc_gameLoop( ProcState_t * gproc, Game_t * gs )
{
  // if we are being told to exit, jump to PROC_Leaving
  double frameDuration, frameTicks;
  static SDL_Thread       * pThread = NULL;
  static SDL_Callback_Ptr   pCallback = NULL;

  Gui_t * gui = gui_getState();

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

        // clear out any old icons and load the global icons
        load_global_icons( gs );

        // set up the MD2 models
        init_all_models(gs);

        log_info( "proc_gameLoop() - \n\tStarting to load module %s.\n", gs->mod.loadname);
        mod_return = module_load( gs, gs->mod.loadname ) ? rv_succeed : rv_fail;
        log_info( "proc_gameLoop() - \n\tLoading module %s... ", gs->mod.loadname);
        if( mod_return )
        {
          log_message("Succeeded!\n");
        }
        else
        {
          log_message("Failed!\n");
          gproc->KillMe = btrue;
          gproc->returnValue = 0;
          return gproc->returnValue;
        }

        make_onwhichfan( gs );
        reset_camera();
        reset_timers( gs );
        figure_out_what_to_draw();
        make_character_matrices( gs );
        recalc_character_bumpers( gs );
        attach_particles( gs );


        if ( cl_Running(gs->cl) || sv_Running(gs->sv) )
        {
          log_info( "SDL_main: Loading module %s...\n", gs->mod.loadname );
          keyb.mode = bfalse;
          keyb.delay = 20;
          net_sayHello( gs );
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
        gs->sv->rand_idx = time(NULL);      // TODO - for a network game, this needs to be set by the server...
        srand( -(Sint32)gs->sv->rand_idx );

        // reset the timers
        gs->dFrame = gs->dUpdate = 0.0f;

        // force the game to start flipping pages again
        request_pageflip_unpause();
      }
      gproc->State = PROC_Running;
      break;

    case PROC_Running:

      // update the times

      gs->dFrame += frameTicks / FRAMESKIP;
      if(gs->Single_frame) gs->dFrame = 1.0f;

      gs->dUpdate += frameTicks / UPDATESKIP;
      if(gs->Single_frame) gs->dUpdate = 1.0f;

      game_handleKeyboard();

      if ( !gproc->Active )
      {
        gproc->State = PROC_Leaving;
      }
      else
      {
        PROFILE_BEGIN( main_loop );
        game_doRun(gs, gproc->State);
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
        //SDL_WM_GrabInput( CData.GrabMouse );
        //SDL_ShowCursor(SDL_ENABLE);

        // this game is done. stop looking at it.
        Graphics_removeGame( &gfxState, gs );
      }
      gproc->returnValue = -1;
      gproc->State = PROC_Begin;
      break;
  };

  return gproc->returnValue;
};

//--------------------------------------------------------------------------------------------
int proc_menuLoop( MenuProc_t  * mproc )
{
  ProcState_t * proc;

  if(NULL == mproc || mproc->proc.Terminated) return -1;

  proc = &(mproc->proc);

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
        ui_initialize( CStringTmp1, CData.uifont_points_large );

        //Load stuff into memory
        mnu_free_all_titleimages(mproc);
        initMenus();             //Start the game menu

        // initialize the bitmap font so we can use the cursor
        snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.font_bitmap );
        snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.fontdef_file );
        if ( !ui_load_BMFont( CStringTmp1, CStringTmp2 ) )
        {
          log_warning( "UI unable to use load bitmap font for cursor. Files missing from %s directory\n", CData.basicdat_dir );
        };

        // reset the update timer
        mproc->dUpdate = 0;

        // Let the normal OS mouse cursor work
        SDL_WM_GrabInput( SDL_GRAB_OFF );
        SDL_ShowCursor( SDL_DISABLE );
        mous.game = bfalse;
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

        mnu_Run( mproc );

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
  ProcState_t EgoProc = {bfalse };

  int program_state = 0, i;
  MachineState_t * mach_state;
  GameStack_t * stk;
  Game_t * gs;

  PROFILE_INIT( ekey );

  // Initialize logging first, so that we can use it everywhere.
  log_init();
  log_setLoggingLevel( 2 );

  // force the initialization of the machine state
  mach_state = get_MachineState();

  // create the main loop "process"
  ProcState_new(&EgoProc);

  // For each game state, run the main loop.
  // Delete any game states that have been killed
  stk = Get_GameStack();
  do
  {
    proc_mainLoop( &EgoProc, argc, argv );

    // always keep 1 game state on the stack
    for(i=1; i<stk->count; i++)
    {
      gs = GameStack_get(stk, i);
      if(gs->proc.Terminated)
      {
        // only kill one per loop, since the deletion process invalidates the stack
        Game_delete(gs);
        break;
      }
    }

  }
  while ( -1 != EgoProc.returnValue );

  return btrue;
}

//--------------------------------------------------------------------------------------------
int load_all_messages( Game_t * gs, const char *szObjectpath, const char *szObjectname )
{
  // ZZ> This function loads all of an objects messages

  int retval;
  FILE *fileread;
  MessageData_t * msglst = &(gs->MsgList);

  retval = 0;
  fileread = fs_fileOpen( PRI_NONE, NULL, inherit_fname(szObjectpath, szObjectname, CData.message_file), "r" );
  if ( NULL != fileread )
  {
    retval = msglst->total;
    while ( fget_next_message( fileread, &(gs->MsgList) ) ) {};

    fs_fileClose( fileread );
  }

  return retval;
}


//--------------------------------------------------------------------------------------------
void update_looped_sounds( Game_t * gs )
{
  CHR_REF ichr;

  for(ichr=0; ichr<CHRLST_COUNT; ichr++)
  {
    if( !ACTIVE_CHR(gs->ChrList, ichr) ) continue;
    if( INVALID_CHANNEL == gs->ChrList[ichr].loopingchannel ) continue;

    snd_apply_mods( gs->ChrList[ichr].loopingchannel, gs->ChrList[ichr].loopingvolume, gs->ChrList[ichr].ori.pos, GCamera.trackpos, GCamera.turn_lr);
  };

}


//--------------------------------------------------------------------------------------------
SearchInfo_t * SearchInfo_new(SearchInfo_t * psearch)
{
  //fprintf( stdout, "SearchInfo_new()\n");

  if(NULL == psearch) return NULL;

  psearch->initialize = btrue;

  psearch->bestdistance = 9999999;
  psearch->besttarget   = INVALID_CHR;
  psearch->bestangle    = 32768;

  return psearch;
};

//--------------------------------------------------------------------------------------------
bool_t prt_search_wide( Game_t * gs, SearchInfo_t * psearch, PRT_REF iprt, Uint16 facing,
                        Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                        TEAM_REF team, CHR_REF donttarget, CHR_REF oldtarget )
{
  // This function finds the best target for the given parameters

  int block_x, block_y;

  if( !ACTIVE_PRT( gs->PrtList, iprt) ) return bfalse;

  block_x = MESH_FLOAT_TO_BLOCK( gs->PrtList[iprt].ori.pos.x );
  block_y = MESH_FLOAT_TO_BLOCK( gs->PrtList[iprt].ori.pos.y );

  // initialize the search
  SearchInfo_new(psearch);

  prt_search_block( gs, psearch, block_x + 0, block_y + 0, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );

  if ( !ACTIVE_CHR( gs->ChrList, psearch->besttarget ) )
  {
    prt_search_block( gs, psearch, block_x + 1, block_y + 0, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x - 1, block_y + 0, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x + 0, block_y + 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x + 0, block_y - 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  }

  if( !ACTIVE_CHR( gs->ChrList, psearch->besttarget ) )
  {
    prt_search_block( gs, psearch, block_x + 1, block_y + 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x + 1, block_y - 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x - 1, block_y + 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
    prt_search_block( gs, psearch, block_x - 1, block_y - 1, iprt, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  }

  return ACTIVE_CHR( gs->ChrList, psearch->besttarget );
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_block( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                         bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                         bool_t invert_idsz )
{
  // ZZ> This is a good little helper. returns btrue if a suitable target was found

  int cnt;
  CHR_REF charb;
  Uint32 fanblock, blnode_b;
  TEAM_REF team;
  vect3 diff;
  float dist;

  Mesh_t * pmesh   = Game_getMesh(gs);
  MeshInfo_t * mi  = &(pmesh->Info);

  BUMPLIST  * pbump  = &(mi->bumplist);

  bool_t require_friends =  ask_friends && !ask_enemies;
  bool_t require_enemies = !ask_friends &&  ask_enemies;
  bool_t require_alive   = !ask_dead;
  bool_t require_noitems = !ask_items;
  bool_t ballowed;

  if ( !ACTIVE_CHR( gs->ChrList, character ) || !pbump->filled ) return bfalse;
  team     = gs->ChrList[character].team;

  fanblock = mesh_convert_block( &(pmesh->Info), block_x, block_y );
  if(INVALID_FAN == fanblock) return bfalse;

  for ( cnt = 0, blnode_b = bumplist_get_chr_head( pbump, fanblock );
        cnt < bumplist_get_chr_count(pbump, fanblock) && INVALID_BUMPLIST_NODE != blnode_b;
        cnt++, blnode_b = bumplist_get_next_chr( gs, pbump, blnode_b ) )
  {
    charb = bumplist_get_ref( pbump, blnode_b );
    assert( ACTIVE_CHR( gs->ChrList, charb ) );

    // don't find stupid stuff
    if ( !ACTIVE_CHR( gs->ChrList, charb ) || 0.0f == gs->ChrList[charb].bumpstrength ) continue;

    // don't find yourself or any of the items you're holding
    if ( character == charb || gs->ChrList[charb].attachedto == character || gs->ChrList[charb].inwhichpack == character ) continue;

    // don't find your mount or your master
    if ( gs->ChrList[character].attachedto == charb || gs->ChrList[character].inwhichpack == charb ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( gs->ChrList, CHRLST_COUNT, charb ) ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, charb ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && gs->TeamList[team].hatesteam[gs->ChrList[charb].REF_TO_INT(team)] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !gs->TeamList[team].hatesteam[gs->ChrList[charb].REF_TO_INT(team)] || gs->ChrList[charb].prop.invictus ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !gs->ChrList[charb].alive ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && gs->ChrList[charb].prop.isitem ) continue;

    ballowed = bfalse;
    if ( IDSZ_NONE == idsz )
    {
      ballowed = btrue;
    }
    else
    {
      bool_t found_idsz = CAP_INHERIT_IDSZ( gs, ChrList_getRCap(gs, charb), idsz );
      ballowed = (invert_idsz != found_idsz);
    }

    if ( ballowed )
    {
      diff.x = gs->ChrList[character].ori.pos.x - gs->ChrList[charb].ori.pos.x;
      diff.y = gs->ChrList[character].ori.pos.y - gs->ChrList[charb].ori.pos.y;
      diff.z = gs->ChrList[character].ori.pos.z - gs->ChrList[charb].ori.pos.z;

      dist = DotProduct(diff, diff);
      if ( psearch->initialize || dist < psearch->bestdistance )
      {
        psearch->bestdistance = dist;
        psearch->besttarget   = charb;
        psearch->initialize   = bfalse;
      }
    }

  }

  return ACTIVE_CHR( gs->ChrList, psearch->besttarget);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_nearby( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, bool_t ask_items,
                          bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz )
{
  // ZZ> This function finds a target from the blocks that the character is overlapping. Returns btrue if found.

  int ix,ix_min,ix_max, iy,iy_min,iy_max;
  bool_t seeinvisible;

  Mesh_t * pmesh = Game_getMesh(gs);

  if ( !ACTIVE_CHR( gs->ChrList, character ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, character ) ) return bfalse;

  // initialize the search
  SearchInfo_new(psearch);

  // grab seeinvisible from the character
  seeinvisible = gs->ChrList[character].prop.canseeinvisible;

  // Current fanblock
  ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( &(pmesh->Info), gs->ChrList[character].bmpdata.cv.x_min ) );
  ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( &(pmesh->Info), gs->ChrList[character].bmpdata.cv.x_max ) );
  iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( &(pmesh->Info), gs->ChrList[character].bmpdata.cv.y_min ) );
  iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( &(pmesh->Info), gs->ChrList[character].bmpdata.cv.y_max ) );

  for( ix = ix_min; ix<=ix_max; ix++ )
  {
    for( iy=iy_min; iy<=iy_max; iy++ )
    {
      chr_search_block( gs, psearch, ix, iy, character, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, ask_idsz, bfalse );
    };
  };

  if ( psearch->besttarget==character )
  {
    psearch->besttarget = INVALID_CHR;
  }

  return ACTIVE_CHR( gs->ChrList, psearch->besttarget);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_distant( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead )
{
  // ZZ> This function finds a target, or it returns bfalse if it can't find one...
  //     maxdist should be the square of the actual distance you want to use
  //     as the cutoff...

  CHR_REF charb;
  int xdist, ydist, zdist;
  bool_t require_alive = !ask_dead;
  TEAM_REF team;

  if ( !ACTIVE_CHR( gs->ChrList, character ) ) return bfalse;

  SearchInfo_new(psearch);

  team = gs->ChrList[character].team;
  psearch->bestdistance = maxdist;
  for ( charb = 0; charb < CHRLST_COUNT; charb++ )
  {
    // don't find stupid items
    if ( !ACTIVE_CHR( gs->ChrList, charb ) || 0.0f == gs->ChrList[charb].bumpstrength ) continue;

    // don't find yourself or items you are carrying
    if ( character == charb || gs->ChrList[charb].attachedto == character || gs->ChrList[charb].inwhichpack == character ) continue;

    // don't find thigs you can't see
    if (( !gs->ChrList[character].prop.canseeinvisible && chr_is_invisible( gs->ChrList, CHRLST_COUNT, charb ) ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, charb ) ) continue;

    // don't find dead things if not asked for
    if ( require_alive && ( !gs->ChrList[charb].alive || gs->ChrList[charb].prop.isitem ) ) continue;

    // don't find enemies unless asked for
    if ( ask_enemies && ( !gs->TeamList[team].hatesteam[gs->ChrList[charb].REF_TO_INT(team)] || gs->ChrList[charb].prop.invictus ) ) continue;

    xdist = gs->ChrList[charb].ori.pos.x - gs->ChrList[character].ori.pos.x;
    ydist = gs->ChrList[charb].ori.pos.y - gs->ChrList[character].ori.pos.y;
    zdist = gs->ChrList[charb].ori.pos.z - gs->ChrList[character].ori.pos.z;
    psearch->bestdistance = xdist * xdist + ydist * ydist + zdist * zdist;

    if ( psearch->bestdistance < psearch->bestdistance )
    {
      psearch->bestdistance = psearch->bestdistance;
      psearch->besttarget   = charb;
    };
  }

  return ACTIVE_CHR( gs->ChrList, psearch->besttarget);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_block_nearest( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, CHR_REF chra_ref, bool_t ask_items,
                               bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz )
{
  // ZZ> This is a good little helper

  float dis, xdis, ydis, zdis;
  int   cnt;
  TEAM_REF    team;
  CHR_REF chrb_ref;
  Uint32  fanblock, blnode_b;

  Mesh_t     * pmesh = Game_getMesh(gs);
  MeshInfo_t * mi    = &(pmesh->Info);
  BUMPLIST   * pbump = &(mi->bumplist);

  bool_t require_friends =  ask_friends && !ask_enemies;
  bool_t require_enemies = !ask_friends &&  ask_enemies;
  bool_t require_alive   = !ask_dead;
  bool_t require_noitems = !ask_items;

  // if chra_ref is not defined, return
  if ( !ACTIVE_CHR( gs->ChrList, chra_ref ) || !pbump->filled ) return bfalse;

  // blocks that are off the mesh are not stored
  fanblock = mesh_convert_block( &(pmesh->Info), block_x, block_y );
  if(INVALID_FAN == fanblock) return bfalse;

  team = gs->ChrList[chra_ref].team;
  for ( cnt = 0, blnode_b = bumplist_get_chr_head(pbump, fanblock);
        cnt < bumplist_get_chr_count(pbump, fanblock) && INVALID_BUMPLIST_NODE != blnode_b;
        cnt++, blnode_b = bumplist_get_next_chr(gs, pbump, blnode_b) )
  {
    chrb_ref = bumplist_get_ref(pbump, blnode_b);
    ACTIVE_CHR( gs->ChrList, chrb_ref );

    // don't find stupid stuff
    if ( !ACTIVE_CHR( gs->ChrList, chrb_ref ) || 0.0f == gs->ChrList[chrb_ref].bumpstrength ) continue;

    // don't find yourself or any of the items you're holding
    if ( chra_ref == chrb_ref || gs->ChrList[chrb_ref].attachedto == chra_ref || gs->ChrList[chrb_ref].inwhichpack == chra_ref ) continue;

    // don't find your mount or your master
    if ( gs->ChrList[chra_ref].attachedto == chrb_ref || gs->ChrList[chra_ref].inwhichpack == chrb_ref ) continue;

    // don't find anything you can't see
    if (( !seeinvisible && chr_is_invisible( gs->ChrList, CHRLST_COUNT, chrb_ref ) ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, chrb_ref ) ) continue;

    // if we need to find friends, don't find enemies
    if ( require_friends && gs->TeamList[team].hatesteam[gs->ChrList[chrb_ref].REF_TO_INT(team)] ) continue;

    // if we need to find enemies, don't find friends or invictus
    if ( require_enemies && ( !gs->TeamList[team].hatesteam[gs->ChrList[chrb_ref].REF_TO_INT(team)] || gs->ChrList[chrb_ref].prop.invictus ) ) continue;

    // if we require being alive, don't accept dead things
    if ( require_alive && !gs->ChrList[chrb_ref].alive ) continue;

    // if we require not an item, don't accept items
    if ( require_noitems && gs->ChrList[chrb_ref].prop.isitem ) continue;

    if ( IDSZ_NONE == idsz || CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, chrb_ref), idsz ) )
    {
      xdis = gs->ChrList[chra_ref].ori.pos.x - gs->ChrList[chrb_ref].ori.pos.x;
      ydis = gs->ChrList[chra_ref].ori.pos.y - gs->ChrList[chrb_ref].ori.pos.y;
      zdis = gs->ChrList[chra_ref].ori.pos.z - gs->ChrList[chrb_ref].ori.pos.z;
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
bool_t chr_search_wide_nearest( Game_t * gs, SearchInfo_t * psearch, CHR_REF chr_ref, bool_t ask_items,
                                 bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz )
{
  // ZZ> This function finds an target, or it returns CHRLST_COUNT if it can't find one

  int x, y;
  bool_t seeinvisible = gs->ChrList[chr_ref].prop.canseeinvisible;

  if ( !ACTIVE_CHR( gs->ChrList, chr_ref ) ) return bfalse;

  // Current fanblock
  x = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].ori.pos.x );
  y = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].ori.pos.y );

  // initialize the search
  SearchInfo_new(psearch);

  chr_search_block_nearest( gs, psearch, x + 0, y + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );

  if ( !ACTIVE_CHR( gs->ChrList, psearch->nearest ) )
  {
    chr_search_block_nearest( gs, psearch, x - 1, y + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 1, y + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 0, y - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 0, y + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( !ACTIVE_CHR( gs->ChrList, psearch->nearest ) )
  {
    chr_search_block_nearest( gs, psearch, x - 1, y + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 1, y - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x - 1, y - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
    chr_search_block_nearest( gs, psearch, x + 1, y + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz );
  };

  if ( psearch->nearest == chr_ref )
    psearch->nearest = INVALID_CHR;

  return ACTIVE_CHR( gs->ChrList, psearch->nearest);
}

//--------------------------------------------------------------------------------------------
bool_t chr_search_wide( Game_t * gs, SearchInfo_t * psearch, CHR_REF chr_ref, bool_t ask_items,
                        bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid )
{
  // ZZ> This function finds an object, or it returns bfalse if it can't find one

  int     ix, iy;
  bool_t  seeinvisible;
  bool_t  found;

  if ( !ACTIVE_CHR( gs->ChrList, chr_ref ) ) return bfalse;

  // make sure the search context is clear
  SearchInfo_new(psearch);

  // get seeinvisible from the character
  seeinvisible = gs->ChrList[chr_ref].prop.canseeinvisible;

  // Current fanblock
  ix = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].ori.pos.x );
  iy = MESH_FLOAT_TO_BLOCK( gs->ChrList[chr_ref].ori.pos.y );

  chr_search_block( gs, psearch, ix + 0, iy + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
  found = ACTIVE_CHR( gs->ChrList, psearch->besttarget ) && (psearch->besttarget != chr_ref);

  if( !found )
  {
    chr_search_block( gs, psearch, ix - 1, iy + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 1, iy + 0, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 0, iy - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 0, iy + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    found = ACTIVE_CHR( gs->ChrList, psearch->besttarget ) && (psearch->besttarget != chr_ref);
  }

  if( !found )
  {
    chr_search_block( gs, psearch, ix - 1, iy + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 1, iy - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix - 1, iy - 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    chr_search_block( gs, psearch, ix + 1, iy + 1, chr_ref, ask_items, ask_friends, ask_enemies, ask_dead, seeinvisible, idsz, excludeid );
    found = ACTIVE_CHR( gs->ChrList, psearch->besttarget ) && (psearch->besttarget != chr_ref);
  }

  return found;
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( Game_t * gs, PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset )
{
  // ZZ> This function sets one particle's position to be attached to a character.
  //     It will kill the particle if the character is no longer around

  Uint16 vertex;
  float flip;
  GLvector point, nupoint;

  bool_t prt_valid;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  Prt_t * pprt;
  Chr_t * pchr;

  prt_valid = btrue;
  if ( !ACTIVE_CHR( chrlst, chr_ref ) )
  {
    pchr = NULL;
    prt_valid = bfalse;
  }
  else
  {
    pchr = ChrList_getPChr(gs, chr_ref);
  }

  if ( !ACTIVE_PRT( prtlst, particle ) )
  {
    pprt = NULL;
    prt_valid = bfalse;
  }
  else
  {
    pprt = prtlst + particle;
  }


  // Check validity of attachment
  if ( !prt_valid )
  {
    return;
  }

  if( chr_in_pack( chrlst, CHRLST_COUNT, chr_ref ) )
  {
    pprt->gopoof = btrue;
    return;
  }

  // Do we have a matrix???
  if ( !pchr->matrix_valid )
  {
    // No matrix, so just wing it...

    pprt->ori.pos.x = pchr->ori.pos.x;
    pprt->ori.pos.y = pchr->ori.pos.y;
    pprt->ori.pos.z = pchr->ori.pos.z;
  }
  else if ( vertoffset == GRIP_ORIGIN )
  {
    // Transform the origin to world space

    pprt->ori.pos.x = pchr->matrix.CNV( 3, 0 );
    pprt->ori.pos.y = pchr->matrix.CNV( 3, 1 );
    pprt->ori.pos.z = pchr->matrix.CNV( 3, 2 );
  }
  else
  {
    // Transform the grip vertex position to world space

    Mad_t * pmad = ChrList_getPMad(gs, chr_ref);

    Uint32      ilast, inext;
    MD2_Model_t * pmdl;
    const MD2_Frame_t * plast, * pnext;

    inext = pchr->anim.next;
    ilast = pchr->anim.last;
    flip  = pchr->anim.flip;

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

    pprt->ori.pos.x = nupoint.x;
    pprt->ori.pos.y = nupoint.y;
    pprt->ori.pos.z = nupoint.z;


  }



}


//--------------------------------------------------------------------------------------------
bool_t load_all_music_sounds(ConfigData_t * cd)
{
  //ZF> This function loads all of the music sounds
  STRING songname;
  FILE *playlist;
  Uint8 cnt;

  SoundState_t * snd = snd_getState(cd);

  if( NULL == snd || NULL == cd ) return bfalse;

  // reset the music
  if(snd->music_loaded)
  {
    snd_unload_music();
  }

  //Open the playlist listing all music files
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", cd->basicdat_dir, cd->music_dir, cd->playlist_file );
  playlist = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( NULL == playlist  )
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
    snd->mus_list[cnt] = Mix_LoadMUS( CStringTmp1 );
    cnt++;
  }

  fs_fileClose( playlist );

  snd->music_loaded = btrue;

  return snd->music_loaded;
}

//--------------------------------------------------------------------------------------------
void sdlinit( Graphics_t * g )
{
  int cnt;
  SDL_Surface * tmp_surface;
  STRING strbuffer = NULL_STRING;

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

  /* Setup the cute windows manager icon */
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.icon_bitmap );
  tmp_surface = SDL_LoadBMP( CStringTmp1 );
  if ( NULL == tmp_surface  )
  {
    log_warning( "Unable to load icon (%s)\n", CStringTmp1 );
  }
  SDL_WM_SetIcon( tmp_surface, NULL );

  // Set the window name
  SDL_WM_SetCaption( "Egoboo", "Egoboo" );

  // Get us a video mode
  if( NULL == sdl_set_mode(NULL, g, bfalse) )
  {
    log_error("I can't get SDL to set any video mode.\n");
    exit(-1);
  }

  // We can now do a valid anisotropy calculation
  gfx_find_anisotropy( g );

  //Grab all the available video modes
  g->video_mode_list = SDL_ListModes( g->surface->format, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN | SDL_OPENGL | SDL_HWACCEL | SDL_SRCALPHA );
  log_info( "Detecting available full-screen video modes...\n" );
  for ( cnt = 0; NULL != g->video_mode_list[cnt]; ++cnt )
  {
    log_info( "\tVideo Mode - %d x %d\n", g->video_mode_list[cnt]->w, g->video_mode_list[cnt]->h );
  };



  // set the mouse cursor
  SDL_WM_GrabInput( g->GrabMouse );
  //if (g->HideMouse) SDL_ShowCursor(SDL_DISABLE);

  input_setup();
}

//--------------------------------------------------------------------------------------------
retval_t MachineState_update(MachineState_t * mac)
{
  // BB > Update items related to this computer.
  //        - Calclate the Bishopia date

  // seconds from Jan 1, 1970 to "birth" of Bishopia
  const time_t i_dif  = 10030 * 24 * 60 * 60;

  // 1 game day every 15 minutes
  const double clock_magnification = 96;

  // seconds in one day
  const double secs_per_day = 24.0 * 60.0 * 60.0;

  if ( !EKEY_PVALID(mac) ) return rv_error;

  // store the real time
  mac->i_actual_time = time(NULL);

  // Bishopia has 360 day years, 30 day months, and 24 hour days
  mac->f_bishop_time   = (double)(mac->i_actual_time - i_dif) * clock_magnification / secs_per_day;
  mac->i_bishop_time_y = mac->f_bishop_time / 360;
  mac->i_bishop_time_m = (mac->f_bishop_time - mac->i_bishop_time_y * 360) / 30;
  mac->i_bishop_time_d = (mac->f_bishop_time - mac->i_bishop_time_y * 360 - mac->i_bishop_time_m * 30);
  mac->f_bishop_time_h = (mac->f_bishop_time - (int)mac->f_bishop_time) * 24.0;

  return rv_succeed;
};

//--------------------------------------------------------------------------------------------
MachineState_t * get_MachineState()
{
  // BB > give access to the machine state singleton

  if(!EKEY_VALID(_macState))
  {
    MachineState_new(&_macState);
  }

  return &_macState;
}

//--------------------------------------------------------------------------------------------
MachineState_t * MachineState_new( MachineState_t * ms )
{
  //fprintf( stdout, "MachineState_new()\n");

  if(NULL == ms) return ms;

  MachineState_delete( ms );

  memset(ms, 0, sizeof(MachineState_t));

  EKEY_PNEW( ms, MachineState_t );

  // initialize the system-dependent functions
  sys_initialize();

  // initialize the filesystem
  fs_init();

  // set up the clock
  ms->clk = ClockState_create("MachineState_t", -1);

  return ms;
}

//--------------------------------------------------------------------------------------------
bool_t MachineState_delete( MachineState_t * ms )
{
  if(NULL == ms) return bfalse;
  if(!EKEY_PVALID(ms)) return btrue;

  EKEY_PINVALIDATE(ms);

  ClockState_destroy( &(ms->clk) );

  sys_shutdown();

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_chr_spawn_info(Game_t * gs, FILE * pfile, CHR_SPAWN_INFO * psi)
{
  STRING myname = NULL_STRING;
  bool_t found;
  int slot;
  vect3 pos;
  char cTmp;

  if(NULL == pfile || NULL == psi) return bfalse;
  if( feof(pfile) ) return bfalse;

  // initialize the spawn info for this object
  chr_spawn_info_new(psi, gs);

  // the name
  found = fget_next_string( pfile, myname, sizeof( myname ) );
  if( !found ) return bfalse;

  str_decode( myname, sizeof( myname ), myname );

  if ( 0 == strcmp( "NONE", myname ) )
  {
    // Random name
    psi->name[0] = EOS;
  }
  else
  {
    strncpy(psi->name, myname, sizeof(psi->name));
  }

  // the slot number
  fscanf( pfile, "%d", &slot );
  psi->iobj = (slot < 0) ? INVALID_OBJ : OBJ_REF(slot);

  // the position info
  pos.x = fget_float( pfile );
  pos.y = fget_float( pfile );
  pos.z = fget_float( pfile );

  psi->pos.x = MESH_FAN_TO_FLOAT( pos.x );
  psi->pos.y = MESH_FAN_TO_FLOAT( pos.y );
  psi->pos.z = MESH_FAN_TO_FLOAT( pos.z );

  // attachment and facing info
  cTmp = fget_first_letter( pfile );
  switch ( toupper( cTmp ) )
  {
    case 'N':  psi->facing = NORTH; break;
    case 'S':  psi->facing = SOUTH; break;
    case 'E':  psi->facing = EAST; break;
    case 'W':  psi->facing = WEST; break;
    case 'L':  psi->slot   = SLOT_LEFT;      break;
    case 'R':  psi->slot   = SLOT_RIGHT;     break;
    case 'I':  psi->slot   = SLOT_INVENTORY; break;
  };

  // misc info
  psi->money   = fget_int( pfile );
  psi->iskin   = fget_int( pfile );
  psi->passage = fget_int( pfile );
  psi->content = fget_int( pfile );
  psi->level   = fget_int( pfile );
  psi->stat    = fget_bool( pfile );
  psi->ghost   = fget_bool( pfile );

  // team info
  psi->iteam = TEAM_REF( fget_first_letter( pfile ) - 'A' );
  if ( psi->iteam > TEAM_COUNT ) psi->iteam = TEAM_REF( TEAM_NULL );

  return btrue;
}

//--------------------------------------------------------------------------------------------
struct s_chr_setup_info
{
  egoboo_key_t ekey;

  Game_t * gs;
  CHR_REF  last_chr;
  vect3    last_chr_pos;
  CHR_REF  last_item;
  int tnc, localnumber;

};

typedef struct s_chr_setup_info CHR_SETUP_INFO;

bool_t chr_setup_info_delete(CHR_SETUP_INFO * pi)
{
  if(NULL == pi) return bfalse;
  if( !EKEY_PVALID(pi) ) return btrue;

  EKEY_PINVALIDATE(pi);

  return btrue;
}

CHR_SETUP_INFO * chr_setup_info_new(CHR_SETUP_INFO * pi, Game_t * gs)
{
  if(NULL == pi) return NULL;

  chr_setup_info_delete( pi );

  memset( pi, 0, sizeof(CHR_SETUP_INFO) );

  EKEY_PNEW( pi, CHR_SETUP_INFO );

  pi->gs = gs;

  pi->last_chr = INVALID_CHR;
  pi->last_item = INVALID_CHR;

  pi->tnc = 0;
  pi->localnumber = 0;

  return pi;
}


//--------------------------------------------------------------------------------------------
bool_t do_setup_chracter(CHR_SETUP_INFO * pinfo, CHR_SPAWN_INFO * psi)
{
  CHR_REF attach, tmp_item;
  GRIP    grip;

  Game_t * gs;
  PChr_t chrlst;
  ModState_t * pmod;
  Chr_t * pitem;

  if( !EKEY_PVALID(pinfo) || !EKEY_PVALID(psi) ) return bfalse;

  gs     = psi->gs;
  chrlst = gs->ChrList;
  pmod   = &(gs->modstate);

  // if the character/object has requested to be in a certain slot...
  attach = INVALID_CHR;
  grip   = GRIP_NONE;
  if(SLOT_NONE != psi->slot)
  {
    attach = pinfo->last_chr;
    grip = slot_to_grip(psi->slot);
  }

  tmp_item = force_chr_spawn( *psi );
  if ( !VALID_CHR( chrlst, tmp_item ) ) return bfalse;
  pitem = chrlst + tmp_item;
  pinfo->last_item = tmp_item;

  // handle attachments
  if ( SLOT_NONE == psi->slot )
  {
    // The item is actually a character
    pinfo->last_chr     = pinfo->last_item;
    pinfo->last_item    = INVALID_CHR;

    if( VALID_CHR(chrlst, pinfo->last_chr) )
    {
      pinfo->last_chr_pos = chrlst[pinfo->last_chr].ori.pos;
    }
    else
    {
      VectorClear( pinfo->last_chr_pos.v );
    }
  }
  else if ( SLOT_INVENTORY == psi->slot )
  {
    // Place pinfo->last_item into the inventory of pinfo->last_chr.
    // Pretend that the item was grabbed by the left hand first and run the
    // item's script to handle any code related to ALERT_GRABBED
    CHR_REF itmp;

    // save whatever may be in the character's left hand
    itmp = chrlst[pinfo->last_chr].holdingwhich[SLOT_LEFT];

    // put the item into the left hand
    chrlst[pinfo->last_chr].holdingwhich[SLOT_LEFT] = pinfo->last_item;
    pitem->inwhichslot  = SLOT_LEFT;
    pitem->attachedto   = pinfo->last_chr;

    // tell the chr that it has been grabbed
    pitem->aistate.alert |= ALERT_GRABBED;                    // Make spellbooks change
    run_script( pinfo->gs, pinfo->last_item, 1.0f );                     // Empty the grabbed messages

    // restore any old item
    chrlst[pinfo->last_chr].holdingwhich[SLOT_LEFT] = itmp;
    pitem->inwhichslot  = SLOT_NONE;
    pitem->attachedto   = INVALID_CHR;

    // actually insert intot the inventory
    pack_add_item( pinfo->gs, pinfo->last_item, pinfo->last_chr );

    // set the item position to the holder's position
    pitem->ori.pos = pinfo->last_chr_pos;
  }
  else
  {
    // Attach the pinfo->last_item to the appropriate slot of pinfo->last_chr
    if ( attach_character_to_mount( pinfo->gs, pinfo->last_item, pinfo->last_chr, psi->slot ) )
    {
      run_script( pinfo->gs, pinfo->last_item, 1.0f );   // Empty the grabbed messages
    };

    // set the item position to the holder's position
    pitem->ori.pos = pinfo->last_chr_pos;
  }

  // Set the starting level
  if ( psi->level > 0 )
  {
    Cap_t * pcap = ChrList_getPCap(gs, pinfo->last_chr);

    // make sure that the character/item CAN level
    if( NULL!=pcap && pcap->experiencecoeff > 0)
    {
      // Let the character gain levels
      psi->level -= 1;
      while ( pitem->experiencelevel < psi->level && pitem->experience < MAXXP )
      {
        give_experience( pinfo->gs, pinfo->last_chr, 100, XP_DIRECT );
      }
    }

  }

  if ( psi->ghost )  // Outdated, should be removed.
  {
    // Make the character a ghost !!!BAD!!!  Can do with enchants
    pitem->alpha_fp8 = 128;
    pitem->bumpstrength = ChrList_getPCap(pinfo->gs, pinfo->last_item)->bumpstrength * FP8_TO_FLOAT( pitem->alpha_fp8 );
    pitem->light_fp8 = 255;
  }

  // make sure that a basic bounding box is calculated for every object
  chr_calculate_bumpers( gs, chrlst + pinfo->last_item, 0);

  // TODO : Fix tilting trees problem
  tilt_character(gs, pinfo->last_item);

  return btrue;
}

void do_setup_inputs(CHR_SETUP_INFO * pinfo, CHR_SPAWN_INFO * psi)
{
  Gui_t * gui = gui_getState();
  Game_t * gs = pinfo->gs;
  PChr_t chrlst = gs->ChrList;
  Client_t * cl = gs->cl;
  ModState_t * pmod = &(gs->modstate);

  if( !Graphics_matchesGame(&gfxState, gs) ) return;

  // Turn on player input devices
  if ( psi->stat )
  {
    if ( pmod->import_amount == 0 )
    {
      if ( cl->StatList_count == 0 )
      {
        // Single player module
        add_player( pinfo->gs, pinfo->last_chr, INBITS_MOUS | INBITS_KEYB | INBITS_JOYA | INBITS_JOYB );
      };
    }
    else if ( cl->StatList_count < pmod->import_amount )
    {
      // Multiplayer module
      int tnc, localnumber=0;
      bool_t itislocal = bfalse;
      for ( tnc = 0; tnc < localplayer_count; tnc++ )
      {
        if ( REF_TO_INT(chrlst[pinfo->last_chr].model) == localplayer_slot[tnc] )
        {
          itislocal = btrue;
          localnumber = tnc;
          break;
        }
      }

      if ( itislocal )
      {
        // It's a local player
        add_player( pinfo->gs, pinfo->last_chr, localplayer_control[localnumber] );
      }
      else
      {
        // It's a remote player
        add_player( pinfo->gs, pinfo->last_chr, INBITS_NONE );
      }
    }

    // Turn on the stat display
    add_status( pinfo->gs, pinfo->last_chr );
  }

}

//--------------------------------------------------------------------------------------------
void setup_characters( Game_t * gs, char *modname )
{
  // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
  STRING newloadname;
  FILE * fileread;
  Gui_t * gui = NULL;

  CHR_SPAWN_INFO si;
  CHR_SETUP_INFO info;

  if( Graphics_matchesGame(&gfxState, gs) )
  {
    gui = gui_getState();
  }

  // if we are not in control of our own spawning,
  // we have to wait for the server to tell us to spawn
  if( !Game_hasServer(gs) ) return;

  // Turn some back on
  snprintf( newloadname, sizeof( newloadname ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.spawn_file );
  fileread = fs_fileOpen( PRI_WARN, "setup_characters()", newloadname, "r" );
  if ( NULL == fileread )
  {
    log_error( "Error loading module: %s\n", newloadname );  //Something went wrong
    return;
  }

  // initialize the character setup
  chr_setup_info_new(&info, gs);

  while ( fget_next_chr_spawn_info(gs, fileread, &si)  )
  {
    // reserve a valid character
    if(INVALID_CHR == ChrList_reserve(&si)) continue;

    // send the character setup to all clients
    sv_send_chr_setup( gs->sv, &si );

    do_setup_chracter(&info, &si);
    do_setup_inputs(&info, &si);
  }
  fs_fileClose( fileread );

  // if we are spawning into the game that the gui is connected to,
  // clear the messsage queue
  if(NULL != gui)
  {
    // clear out any junk messages that are being displayed
    clear_message_queue( &(gui->msgQueue) );
  }

  // Make sure local players are displayed first
  sort_statlist( gs );
}


//--------------------------------------------------------------------------------------------
int cl_proc_setup_character(Game_t * gs, CHR_SETUP_INFO * pinfo, ProcState_t * proc)
{
  Client_t * cl;
  if( !EKEY_PVALID(gs)    ) return -1;
  if( !EKEY_PVALID(pinfo) ) -1;
  if( !EKEY_PVALID(proc)  ) return -1;

  cl = gs->cl;

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
        chr_setup_info_new(pinfo, gs);
      }
      proc->State = PROC_Entering;
      break;

    case PROC_Entering:
      {
        // do nothing
      }
      proc->State = PROC_Running;
      break;

    case PROC_Running:
      {
        CHR_SPAWN_INFO * psi;

        // spawn all of the characters that we have received up to this point
        for (;;)
        {
          psi = chr_spawn_queue_pop( &(cl->chr_queue) );
          if(NULL == psi) break;

          do_setup_chracter(pinfo, psi);
          do_setup_inputs(pinfo, psi);
        }

      }
      break;

    case PROC_Leaving:
      {
        // do nothing
      }
      proc->State = PROC_Finish;
      break;

    case PROC_Finish:
      {
      }
      proc->Active     = bfalse;
      proc->Terminated = btrue;

      proc->returnValue = -1;
      proc->State = PROC_Begin;
      break;
  }

  return proc->returnValue;
}

//--------------------------------------------------------------------------------------------
void ChrList_resynch(Game_t * gs)
{
  // BB > handle all allocation and deallocation requests

  CHR_REF chr_ref;

  // poof all characters that have reserved poof requests
  for ( chr_ref = 0; chr_ref < CHRLST_COUNT; chr_ref++ )
  {
    if ( !VALID_CHR( gs->ChrList, chr_ref ) || !gs->ChrList[chr_ref].gopoof ) continue;

    // detach from any imount
    detach_character_from_mount( gs, chr_ref, btrue, bfalse );

    // Drop all possesions
    for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
    {
      if ( chr_using_slot( gs->ChrList, CHRLST_COUNT, chr_ref, _slot ) )
      {
        detach_character_from_mount( gs, chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, chr_ref, _slot ), btrue, bfalse );
      }
    };

    chr_free_inventory( gs->ChrList, CHRLST_COUNT, chr_ref );
    gs->ChrList[chr_ref].freeme = btrue;
  };

  // free all characters that requested destruction last round
  for ( chr_ref = 0; chr_ref < CHRLST_COUNT; chr_ref++ )
  {
    if ( !VALID_CHR( gs->ChrList, chr_ref ) || !gs->ChrList[chr_ref].freeme ) continue;
    ChrList_free_one( gs, chr_ref );
  }

  // activate all characters that are requested for next round
  for ( chr_ref = 0; chr_ref < CHRLST_COUNT; chr_ref++ )
  {
    if ( !PENDING_CHR( gs->ChrList, chr_ref ) ) continue;

    gs->ChrList[chr_ref].req_active = bfalse;
    gs->ChrList[chr_ref].reserved   = bfalse;
    gs->ChrList[chr_ref].active     = btrue;
  };

};


//--------------------------------------------------------------------------------------------
void PrtList_resynch(Game_t * gs)
{
  int tnc;
  Uint16 facing;
  PRT_REF iprt;
  PIP_REF pip;
  CHR_REF prt_target, prt_owner, prt_attachedto;

  PPrt_t prtlst = gs->PrtList;
  PPip_t piplst = gs->PipList;
  Prt_t * pprt;

  // actually destroy all particles that requested destruction last time through the loop
  for ( iprt = 0; iprt < PRTLST_COUNT; iprt++ )
  {
    if ( !VALID_PRT( prtlst,  iprt ) ) continue;
    pprt = prtlst + iprt;

    if( !pprt->gopoof ) continue;

    // To make it easier
    pip = pprt->pip;
    facing = pprt->facing;
    prt_target = prt_get_target( gs, iprt );
    prt_owner = prt_get_owner( gs, iprt );
    prt_attachedto = prt_get_attachedtochr( gs, iprt );

    for ( tnc = 0; tnc < piplst[pip].endspawnamount; tnc++ )
    {
      prt_spawn( gs, 1.0f, pprt->ori.pos, pprt->ori.vel,
                          facing, pprt->model, piplst[pip].endspawnpip,
                          INVALID_CHR, GRIP_LAST, pprt->team, prt_owner, tnc, prt_target );

      facing += piplst[pip].endspawnfacingadd;
    }

    end_one_particle( gs, iprt );
  }

  // turn on all particles requested in the last turn
  for(iprt = 0; iprt < PRTLST_COUNT; iprt++)
  {
    if( !PENDING_PRT(prtlst, iprt) ) continue;
    pprt = prtlst + iprt;

    pprt->req_active = bfalse;
    pprt->reserved   = bfalse;
    pprt->active     = btrue;

    // Do sound effect on activation
    if( VALID_PIP(piplst, pprt->spinfo.ipip) )
    {
      Pip_t * ppip = piplst + pprt->spinfo.ipip;

      snd_play_particle_sound( pprt->spinfo.gs, pprt->spinfo.intensity, pprt->spinfo.iprt, ppip->soundspawn );
    }

  }

};

//--------------------------------------------------------------------------------------------
bool_t prt_search_block( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, PRT_REF prt_ref, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, TEAM_REF team,
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

  Mesh_t     * pmesh = Game_getMesh(gs);
  MeshInfo_t * mi    = &(pmesh->Info);
  BUMPLIST   * pbump = &(mi->bumplist);

  if( !ACTIVE_PRT( gs->PrtList, prt_ref) ) return bfalse;

  // Current fanblock
  fanblock = mesh_convert_block( &(pmesh->Info), block_x, block_y );
  if ( INVALID_FAN == fanblock ) return bfalse;

  for ( cnt = 0, blnode_b = bumplist_get_chr_head(pbump, fanblock);
        cnt < bumplist_get_chr_count(pbump, fanblock) && INVALID_BUMPLIST_NODE != blnode_b;
        cnt++, blnode_b = bumplist_get_next_chr(gs, pbump, blnode_b) )
  {
    search_ref = bumplist_get_ref(pbump, blnode_b);
    assert( ACTIVE_CHR( gs->ChrList, search_ref ) );

    // don't find stupid stuff
    if ( !ACTIVE_CHR( gs->ChrList, search_ref ) || 0.0f == gs->ChrList[search_ref].bumpstrength ) continue;

    if ( !gs->ChrList[search_ref].alive || gs->ChrList[search_ref].prop.invictus || chr_in_pack( gs->ChrList, CHRLST_COUNT, search_ref ) ) continue;

    if ( search_ref == donttarget_ref || search_ref == oldtarget_ref ) continue;

    if (   allow_anyone ||
         ( request_friends && !gs->TeamList[team].hatesteam[gs->ChrList[search_ref].REF_TO_INT(team)] ) ||
         ( request_enemies &&  gs->TeamList[team].hatesteam[gs->ChrList[search_ref].REF_TO_INT(team)] ) )
    {
      local_distance = ABS( gs->ChrList[search_ref].ori.pos.x - gs->PrtList[prt_ref].ori.pos.x ) + ABS( gs->ChrList[search_ref].ori.pos.y - gs->PrtList[prt_ref].ori.pos.y );
      if ( psearch->initialize || local_distance < psearch->bestdistance )
      {
        Uint16 test_angle;
        local_angle = facing - vec_to_turn( gs->ChrList[search_ref].ori.pos.x - gs->PrtList[prt_ref].ori.pos.x, gs->ChrList[search_ref].ori.pos.y - gs->PrtList[prt_ref].ori.pos.y );

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
GameStack_t * GameStack_new(GameStack_t * stk)
{
  //fprintf( stdout, "GameStack_new()\n");

  if(NULL == stk) return stk;
  GameStack_delete(stk);

  memset(stk, 0, sizeof(GameStack_t));

  EKEY_PNEW(stk, GameStack_t);

  stk->count = 0;

  return stk;
};

//--------------------------------------------------------------------------------------------
bool_t GameStack_delete(GameStack_t * stk)
{
  int i;

  if(NULL == stk) return bfalse;
  if( !EKEY_PVALID(stk) ) return btrue;

  EKEY_PINVALIDATE(stk);

  for(i=0; i<stk->count; i++)
  {
    if( NULL != stk->data[i] )
    {
      Game_delete(stk->data[i]);
      EGOBOO_DELETE(stk->data[i]);
    }
  }
  stk->count = 0;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t GameStack_push(GameStack_t * stk, Game_t * gs)
{
  if(!EKEY_PVALID(stk) || stk->count + 1 >= 256) return bfalse;
  if(!EKEY_PVALID(gs)) return bfalse;

  stk->data[stk->count] = gs;
  stk->count++;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Game_t * GameStack_pop(GameStack_t * stk)
{
  if(!EKEY_PVALID(stk) || stk->count - 1 < 0) return NULL;

  stk->count--;
  return stk->data[stk->count];
}

//--------------------------------------------------------------------------------------------
Game_t * GameStack_get(GameStack_t * stk, int i)
{
  if(!EKEY_PVALID(stk)) return NULL;
  if( i >=  stk->count) return NULL;

  return stk->data[i];
}

//--------------------------------------------------------------------------------------------
bool_t GameStack_add(GameStack_t * stk, Game_t * gs)
{
  // BB> Adds a new game state into the stack.
  //     Does not add a new wntry if the gamestate is already in the stack.

  int i;
  bool_t bfound = bfalse, retval = bfalse;

  if(!EKEY_PVALID(stk)) return bfalse;
  if(!EKEY_PVALID(gs))  return bfalse;

  // check to see it it already exists
  for(i=0; i<stk->count; i++)
  {
    if( gs == GameStack_get(stk, i) )
    {
      bfound = btrue;
      break;
    }
  }

  if(!bfound)
  {
    // not on the stack, so push it
    retval = GameStack_push(stk, gs);
  }

  return retval;

}

//--------------------------------------------------------------------------------------------
Game_t * GameStack_remove(GameStack_t * stk, int i)
{
  if(!EKEY_PVALID(stk)) return NULL;
  if( i >= stk->count ) return NULL;

  // float value to remove to the top
  if( stk->count > 1 && i < stk->count -1 )
  {
    Game_t * ptmp;
    int j = stk->count - 1;

    // swap the value with the value at the top of the stack
    ptmp = stk->data[i];
    stk->data[i] = stk->data[j];
    stk->data[j] = ptmp;
  };

  return GameStack_pop(stk);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void Game_new_textures( Game_t * gs )
{
  int i;

  for(i=0; i<MAXTEXTURE; i++)
  {
    GLtexture_new( gs->TxTexture + i );
  };

  for(i=0; i<MAXICONTX; i++)
  {
    GLtexture_new( gs->TxIcon + i );
  };
  gs->TxIcon_count = 0;

  GLtexture_new( &(gs->TxMap) );

  gs->ico_lst[ICO_NULL] = MAXICONTX;
  gs->ico_lst[ICO_KEYB] = MAXICONTX;
  gs->ico_lst[ICO_MOUS] = MAXICONTX;
  gs->ico_lst[ICO_JOYA] = MAXICONTX;
  gs->ico_lst[ICO_JOYB] = MAXICONTX;
  gs->ico_lst[ICO_BOOK_0] = MAXICONTX;                      // The first book icon

  for(i=0; i<MAXTEXTURE; i++)
  {
    gs->skintoicon[i] = MAXICONTX;
  };
};


//--------------------------------------------------------------------------------------------
Game_t * Game_new(Game_t * gs, Net_t * ns, Client_t * cl, Server_t * sv)
{
  fprintf(stdout, "Game_new()\n");

  if( NULL == gs ) return gs;

  Game_delete( gs );

  memset(gs, 0, sizeof(Game_t));

  EKEY_PNEW(gs, Game_t);

  gs->net_status = NET_STATUS_LOCAL;

  // initialize the main loop process
  ProcState_init( &(gs->proc) );
  gs->proc.Active = bfalse;

  // initialize the menu process
  MenuProc_init( &(gs->igm) );
  gs->igm.proc.Active = bfalse;

  // initialize the sub-game-state values and link them
  ModState_new(&(gs->modstate), NULL, (Uint32)-1);
  gs->cd = &CData;

  // if the Net_t state doesn't exist, create it
  if(NULL == ns) ns = CNet_create( gs );
  ns->parent = gs;
  gs->ns     = ns;

  // add a link to the already created client and server.
  // WARNING: We own these now and must destroy them when we're finished.
  if(NULL != cl) cl->parent = gs;
  gs->cl = cl;

  if(NULL != sv) sv->parent = gs;
  gs->sv = sv;

  // module parameters
  ModInfo_new( &(gs->mod) );
  ModState_new( &(gs->modstate), NULL, (Uint32)-1 );
  ModSummary_new( &(gs->modtxt) );
  Mesh_new( &(gs->Mesh) );
  tile_animated_reset( &(gs->Tile_Anim) );
  tile_damage_reset( &(gs->Tile_Dam) );

  // profiles
  CapList_new( gs );
  EveList_new( gs );
  MadList_new( gs );
  PipList_new( gs );

  ObjList_new( gs );
  ChrList_new( gs );
  EncList_new( gs );
  PrtList_new( gs );

  PassList_new( gs );
  ShopList_new( gs );
  TeamList_new( gs );
  PlaList_new ( gs );

  // weather
  Weather_init( &(gs->Weather) );

  // lighting
  lighting_info_reset( &(gs->Light) );

  // fog
  fog_info_reset( &(gs->Fog) );

  //textures
  Game_new_textures( gs );

  // the the graphics state is not linked into anyone, link it to us
  Graphics_ensureGame(&gfxState, gs);

  prime_icons( gs );

  Game_updateNetStatus( gs );

  return gs;
}

//--------------------------------------------------------------------------------------------
static void Game_delete_textures( Game_t * gs )
{
  int i;

  for(i=0; i<MAXTEXTURE; i++)
  {
    GLtexture_delete( gs->TxTexture + i );
  };

  for(i=0; i<MAXICONTX; i++)
  {
    GLtexture_delete( gs->TxIcon + i );
  };
  gs->TxIcon_count = 0;

  GLtexture_delete( &(gs->TxMap) );

  gs->ico_lst[ICO_NULL] = MAXICONTX;
  gs->ico_lst[ICO_KEYB] = MAXICONTX;
  gs->ico_lst[ICO_MOUS] = MAXICONTX;
  gs->ico_lst[ICO_JOYA] = MAXICONTX;
  gs->ico_lst[ICO_JOYB] = MAXICONTX;
  gs->ico_lst[ICO_BOOK_0] = MAXICONTX;                      // The first book icon

  for(i=0; i<MAXTEXTURE; i++)
  {
    gs->skintoicon[i] = MAXICONTX;
  };
};

//--------------------------------------------------------------------------------------------
bool_t Game_delete(Game_t * gs)
{
  GameStack_t * stk;

  fprintf(stdout, "Game_delete()\n");

  if(NULL == gs) return bfalse;
  if(!EKEY_PVALID(gs))  return btrue;

  // only get rid of this data if we have been asked to be terminated
  if(!gs->proc.KillMe)
  {
    // if someone called this function out of turn, wait until the
    // the "thread" releases the data
    gs->proc.KillMe = btrue;
    return btrue;
  }

  EKEY_PINVALIDATE( gs );

  // free the process
  ProcState_delete( &(gs->proc) );

  // initialize the sub-game-state values and link them
  ModState_delete( &(gs->modstate) );
  CNet_destroy( &(gs->ns) );
  Client_destroy(  &(gs->cl) );
  CServer_destroy(  &(gs->sv) );

  // module parameters
  ModInfo_delete( &(gs->mod) );
  ModState_delete( &(gs->modstate) );
  ModSummary_delete( &(gs->modtxt) );

  Mesh_delete( &(gs->Mesh) );

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

  // delete/release all textures
  Game_delete_textures( gs );

  // the the graphics state is linked to us, unlink it
  Graphics_removeGame(&gfxState, gs);

  // unlink this from the game state stack
  stk = Get_GameStack();
  if(NULL != stk)
  {
    int i;
    for(i=0; i<stk->count; i++)
    {
      if( gs == GameStack_get(stk, i) )
      {
        // found it, so remove and exit
        GameStack_remove(stk, i);
        break;
      }
    }
  }

  // blank out everything (sets all bools to false, all pointers to NULL, ...)
  memset(gs, 0, sizeof(Game_t));

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t Game_renew(Game_t * gs)
{
  fprintf(stdout, "Game_renew()\n");

  if(!EKEY_PVALID(gs)) return bfalse;

  // re-initialize the proc state
  ProcState_renew( Game_getProcedure(gs) );
  ProcState_init( Game_getProcedure(gs) );
  gs->proc.Active = bfalse;

  Client_renew(gs->cl);
  CServer_renew(gs->sv);

  // module parameters
  ModInfo_renew( &(gs->mod) );
  ModState_renew( &(gs->modstate), NULL, (Uint32)-1 );
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
void set_alerts( Game_t * gs, CHR_REF ichr, float dUpdate )
{
  // ZZ> This function polls some alert conditions

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  AI_STATE * pstate;
  Chr_t      * pchr;

  if( !ACTIVE_CHR( chrlst, ichr) ) return;

  pchr   = ChrList_getPChr(gs, ichr);
  pstate = &(pchr->aistate);

  if ( ABS( pchr->ori.pos.x - wp_list_x(&(pstate->wp)) ) < WAYTHRESH &&
       ABS( pchr->ori.pos.y - wp_list_y(&(pstate->wp)) ) < WAYTHRESH )
  {
    ai_state_advance_wp(pstate, !chrlst[ichr].prop.isequipment);
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Game_t * Game_create(Net_t * net, Client_t * cl, Server_t * sv)
{
  GameStack_t   * stk;
  Game_t * ret;

  ret = Game_new( EGOBOO_NEW( Game_t ), net, cl, sv );

  // automatically link it into the game state stack
  stk = Get_GameStack();
  GameStack_add(stk, ret);

  return ret;
};

//--------------------------------------------------------------------------------------------
bool_t Game_destroy(Game_t ** gs )
{
  bool_t ret = Game_delete(*gs);
  EGOBOO_DELETE(*gs);
  return ret;
};

//--------------------------------------------------------------------------------------------
retval_t Game_registerNetwork( Game_t * gs, Net_t * net, bool_t destroy )
{
  if( !EKEY_PVALID(gs) ) return rv_fail;

  if(gs->ns != net )
  {
    if(destroy) CNet_destroy( &(gs->ns) );
    gs->ns = net;
    if(NULL !=gs->ns) gs->ns->parent = gs;
  }

  Game_updateNetStatus( gs );

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
retval_t Game_registerClient ( Game_t * gs, Client_t * cl, bool_t destroy  )
{
  if( !EKEY_PVALID(gs) ) return rv_fail;

  if(gs->cl != cl )
  {
    if(destroy) Client_destroy( &(gs->cl) );
    gs->cl = cl;
    if(NULL !=gs->cl) gs->cl->parent = gs;
  }

  Game_updateNetStatus( gs );

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
retval_t Game_registerServer ( Game_t * gs, Server_t * sv, bool_t destroy )
{
  if( !EKEY_PVALID(gs) ) return rv_fail;

  if(gs->sv != sv )
  {
    if(destroy) CServer_destroy( &(gs->sv) );
    gs->sv = sv;
    if(NULL !=gs->sv) gs->sv->parent = gs;
  }

  Game_updateNetStatus( gs );

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
retval_t Game_updateNetStatus( Game_t * gs )
{
  // BB > update the network status variable so that we can know what kind of game we are playing

  if( !EKEY_PVALID(gs) ) return rv_error;

  gs->net_status = NET_STATUS_LOCAL;

  if ( cl_Running(gs->cl) )
  {
    gs->net_status |= NET_STATUS_CLIENT;
  }

  if ( sv_Running(gs->sv) )
  {
    gs->net_status |= NET_STATUS_SERVER;
  }

  return rv_succeed;
};


//--------------------------------------------------------------------------------------------
bool_t Game_isLocal( Game_t * gs )
{
  if( rv_succeed != Game_updateNetStatus(gs) ) return bfalse;

  return (NET_STATUS_LOCAL == gs->net_status);
}

//--------------------------------------------------------------------------------------------
bool_t Game_isNetwork( Game_t * gs )
{
  if( rv_succeed != Game_updateNetStatus(gs) ) return bfalse;

  return (NET_STATUS_LOCAL != gs->net_status);
}

//--------------------------------------------------------------------------------------------
bool_t Game_isClientServer( Game_t * gs )
{
  if( rv_succeed != Game_updateNetStatus(gs) ) return bfalse;

  return ((NET_STATUS_CLIENT | NET_STATUS_SERVER) == gs->net_status);
};

//--------------------------------------------------------------------------------------------
bool_t Game_isServer ( Game_t * gs )
{
  if( rv_succeed != Game_updateNetStatus(gs) ) return bfalse;

  return (NET_STATUS_SERVER == gs->net_status);
};

//--------------------------------------------------------------------------------------------
bool_t Game_isClient ( Game_t * gs )
{
  if( rv_succeed != Game_updateNetStatus(gs) ) return bfalse;

  return (NET_STATUS_CLIENT == gs->net_status);
};

//--------------------------------------------------------------------------------------------
bool_t Game_hasServer ( Game_t * gs )
{
  if( rv_succeed != Game_updateNetStatus(gs) ) return bfalse;

  return (NET_STATUS_LOCAL == gs->net_status) || (NET_STATUS_SERVER == gs->net_status);
}

//--------------------------------------------------------------------------------------------
bool_t Game_hasClient ( Game_t * gs )
{
  if( rv_succeed != Game_updateNetStatus(gs) ) return bfalse;

  return (NET_STATUS_LOCAL == gs->net_status) || (NET_STATUS_CLIENT == gs->net_status);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ShopList_new( Game_t * gs )
{
  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->ShopList_count = 0;

  for(i=0; i<PASSLST_COUNT; i++)
  {
    Shop_new(gs->ShopList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ShopList_delete( Game_t * gs )
{
  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->ShopList_count = 0;

  for(i=0; i<PASSLST_COUNT; i++)
  {
    Shop_delete(gs->ShopList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ShopList_renew( Game_t * gs )
{
  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->ShopList_count = 0;

  for(i=0; i<PASSLST_COUNT; i++)
  {
    Shop_renew(gs->ShopList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PassList_new( Game_t * gs )
{
  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PassList_count = 0;

  for(i=0; i<PASSLST_COUNT; i++)
  {
    Passage_new(gs->PassList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PassList_delete( Game_t * gs )
{
  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PassList_count = 0;

  for(i=0; i<PASSLST_COUNT; i++)
  {
    Passage_delete(gs->PassList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PassList_renew( Game_t * gs )
{
  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PassList_count = 0;

  for(i=0; i<PASSLST_COUNT; i++)
  {
    Passage_renew(gs->PassList + i);
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t CapList_new( Game_t * gs )
{
  CAP_REF icap;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(icap=0; icap<CAPLST_COUNT; icap++)
  {
    Cap_new(gs->CapList + icap);
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t CapList_delete( Game_t * gs )
{
  CAP_REF icap;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(icap=0; icap<PASSLST_COUNT; icap++)
  {
    Cap_delete(gs->CapList + icap);
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t CapList_renew( Game_t * gs )
{
  CAP_REF icap;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(icap=0; icap<PASSLST_COUNT; icap++)
  {
    Cap_renew(gs->CapList + icap);
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t MadList_new( Game_t * gs )
{
  MAD_REF imad;

  for(imad=0; imad<MADLST_COUNT; imad++)
  {
    Mad_new(gs->MadList + imad);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t MadList_delete( Game_t * gs )
{
  MAD_REF imad;

  for(imad=0; imad<MADLST_COUNT; imad++)
  {
    Mad_delete(gs->MadList + imad);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t MadList_renew( Game_t * gs )
{
  MAD_REF imad;

  for(imad=0; imad<MADLST_COUNT; imad++)
  {
    Mad_renew(gs->MadList + imad);
  }

  return btrue;
}





//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t EncList_new( Game_t * gs )
{
  // ZZ> This functions frees all of the enchantments

  ENC_REF enc_cnt;

  if( !EKEY_PVALID(gs) ) return bfalse;

  // initialize the heap
  EncHeap_new( &(gs->EncHeap) );

  // make sure that all the enchants are deallocated
  for ( enc_cnt = 0; enc_cnt < ENCLST_COUNT; enc_cnt++ )
  {
    Enc_delete( gs->EncList + enc_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_delete( Game_t * gs )
{
  // ZZ> This functions frees all of the enchantments

  ENC_REF enc_cnt;

  if(!EKEY_PVALID(gs)) return bfalse;

  // delete the heap
  EncHeap_delete( &(gs->EncHeap) );

  // make sure that all the enchants are deallocated
  for ( enc_cnt = 0; enc_cnt < ENCLST_COUNT; enc_cnt++ )
  {
    Enc_delete( gs->EncList + enc_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_renew( Game_t * gs )
{
  // ZZ> This functions frees all of the enchantments

  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  // don't touch the heap. just renew each used enchant
  for ( i = 0; i < gs->EncHeap.used_count; i++ )
  {
    Enc_renew( gs->EncList + gs->EncHeap.used_list[ i ] );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PlaList_new( Game_t * gs )
{
  // ZZ> This functions frees all of the plahantments

  PLA_REF pla_cnt;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PlaList_count      = 0;
  if(NULL != gs->cl)
  {
    gs->cl->loc_pla_count  = 0;
    gs->cl->StatList_count = 0;
  };

  for ( pla_cnt = 0; pla_cnt < ENCLST_COUNT; pla_cnt++ )
  {
    Player_new( gs->PlaList + pla_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PlaList_delete( Game_t * gs )
{
  // ZZ> This functions frees all of the plahantments

  PLA_REF pla_cnt;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PlaList_count      = 0;
  if(NULL != gs->ns && NULL != gs->cl)
  {
    gs->cl->loc_pla_count  = 0;
    gs->cl->StatList_count = 0;
  };

  for ( pla_cnt = 0; pla_cnt < ENCLST_COUNT; pla_cnt++ )
  {
    Player_delete( gs->PlaList + pla_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PlaList_renew( Game_t * gs )
{
  // ZZ> This functions frees all of the plahantments

  PLA_REF pla_cnt;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PlaList_count      = 0;
  if(NULL != gs->cl)
  {
    gs->cl->loc_pla_count  = 0;
    gs->cl->StatList_count = 0;
  }

  for ( pla_cnt = 0; pla_cnt < PLALST_COUNT; pla_cnt++ )
  {
    Player_renew( gs->PlaList + pla_cnt );
  }

  return btrue;
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ChrList_new( Game_t * gs )
{
  // ZZ> This functions frees all of the chrhantments

  CHR_REF chr_cnt;

  if(!EKEY_PVALID(gs)) return bfalse;

  // initialize the heap
  ChrHeap_new( &(gs->ChrHeap) );

  // make sure that all the chrhants are deallocated
  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    Chr_delete( gs->ChrList + chr_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_delete( Game_t * gs )
{
  // ZZ> This functions frees all of the chrhantments

  CHR_REF chr_cnt;

  if(!EKEY_PVALID(gs)) return bfalse;

  // delete the heap
  ChrHeap_delete( &(gs->ChrHeap) );

  // make sure that all the chrhants are deallocated
  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    Chr_delete( gs->ChrList + chr_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_renew( Game_t * gs )
{
  // ZZ> This functions frees all of the chrhantments

  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  // don't touch the heap. just renew each used chrhant
  for ( i = 0; i < gs->ChrHeap.used_count; i++ )
  {
    Chr_renew( gs->ChrList + gs->ChrHeap.used_list[ i ] );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t EveList_new( Game_t * gs )
{
  EVE_REF ieve;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(ieve = 0; ieve<EVELST_COUNT; ieve++)
  {
    Eve_new( gs->EveList + ieve );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EveList_delete( Game_t * gs )
{
  EVE_REF ieve;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(ieve = 0; ieve<EVELST_COUNT; ieve++)
  {
    Eve_delete( gs->EveList + ieve );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EveList_renew( Game_t * gs )
{
  EVE_REF ieve;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(ieve = 0; ieve<EVELST_COUNT; ieve++)
  {
    Eve_renew( gs->EveList + ieve );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PrtList_new( Game_t * gs )
{
  // ZZ> This functions frees all of the particles

  PRT_REF prt_cnt;

  if( !EKEY_PVALID(gs) ) return bfalse;

  // initialize the heap
  PrtHeap_new( &(gs->PrtHeap) );

  // make sure that all the particles are deallocated
  for ( prt_cnt = 0; prt_cnt < PRTLST_COUNT; prt_cnt++ )
  {
    Prt_delete( gs->PrtList + prt_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_delete( Game_t * gs )
{
  // ZZ> This functions frees all of the particles

  PRT_REF prt_cnt;

  if(!EKEY_PVALID(gs)) return bfalse;

  // delete the heap
  PrtHeap_delete( &(gs->PrtHeap) );

  // make sure that all the particles are deallocated
  for ( prt_cnt = 0; prt_cnt < PRTLST_COUNT; prt_cnt++ )
  {
    Prt_delete( gs->PrtList + prt_cnt );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_renew( Game_t * gs )
{
  // ZZ> This function renews all the particles

  int i;

  if(!EKEY_PVALID(gs)) return bfalse;

  // don't touch the heap. just renew each used prthant
  for ( i = 0; i < gs->PrtHeap.used_count; i++ )
  {
    Prt_renew( gs->PrtList + gs->PrtHeap.used_list[ i ] );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t TeamList_new( Game_t * gs )
{
  TEAM_REF iteam;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(iteam = 0; iteam<TEAM_COUNT; iteam++)
  {
    CTeam_new( gs->TeamList + iteam );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t TeamList_delete( Game_t * gs )
{
  TEAM_REF iteam;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(iteam = 0; iteam<TEAM_COUNT; iteam++)
  {
    CTeam_delete( gs->TeamList + iteam );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t TeamList_renew( Game_t * gs )
{
  TEAM_REF iteam;

  if(!EKEY_PVALID(gs)) return bfalse;

  for(iteam = 0; iteam<TEAM_COUNT; iteam++)
  {
    CTeam_renew( gs->TeamList + iteam );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t reset_characters( Game_t * gs )
{
  // ZZ> This function resets all the character data

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->allpladead = btrue;

  ChrList_renew(gs);
  StatList_renew(gs->cl);
  PlaList_renew(gs);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t PipList_new( Game_t * gs )
{
  PIP_REF ipip;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PipList_count = 0;
  for(ipip = 0; ipip<PIPLST_COUNT; ipip++)
  {
    Pip_new( gs->PipList + ipip );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PipList_delete( Game_t * gs )
{
  PIP_REF ipip;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PipList_count = 0;
  for(ipip = 0; ipip<PIPLST_COUNT; ipip++)
  {
    Pip_delete( gs->PipList + ipip );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PipList_renew( Game_t * gs )
{
  PIP_REF ipip;

  if(!EKEY_PVALID(gs)) return bfalse;

  gs->PipList_count = 0;
  for(ipip = 0; ipip<PIPLST_COUNT; ipip++)
  {
    Pip_renew( gs->PipList + ipip );
  };

  PipList_load_global(gs);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ProcState_t * ProcState_new(ProcState_t * ps)
{
  //fprintf( stdout, "ProcState_new()\n");

  if(NULL == ps) return ps;

  ProcState_delete(ps);

  memset(ps, 0, sizeof(ProcState_t));

  EKEY_PNEW(ps, ProcState_t);

  ps->Active = btrue;
  ps->State  = PROC_Begin;
  ps->clk    = ClockState_create("ProcState_t", -1);

  return ps;
};

//--------------------------------------------------------------------------------------------
bool_t ProcState_delete(ProcState_t * ps)
{
  if(NULL == ps) return bfalse;
  if(!EKEY_PVALID(ps)) return btrue;

  if(!ps->KillMe)
  {
    ps->KillMe = btrue;
    ps->State = PROC_Leaving;
    return btrue;
  }

  EKEY_PINVALIDATE(ps);

  ps->Active      = bfalse;
  ClockState_destroy( &(ps->clk) );

  return btrue;
}

//--------------------------------------------------------------------------------------------
ProcState_t * ProcState_renew(ProcState_t * ps)
{
  ProcState_delete(ps);
  return ProcState_new(ps);
};

//--------------------------------------------------------------------------------------------
bool_t ProcState_init(ProcState_t * ps)
{
  if(NULL == ps) return bfalse;

  if(!EKEY_PVALID(ps))
  {
    ProcState_new(ps);
  };

  ps->State       = PROC_Begin;
  ps->Active      = btrue;
  ps->Paused      = bfalse;
  ps->Terminated  = bfalse;
  ps->KillMe      = bfalse;
  ps->returnValue = 0;

  return btrue;
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static Gui_t * CGui_new( Gui_t * g );
static bool_t     CGui_delete( Gui_t * g );

//--------------------------------------------------------------------------------------------
Gui_t * gui_getState()
{
  if(!EKEY_VALID(_gui_state))
  {
    CGui_new(&_gui_state);
  }

  return &_gui_state;
}

//--------------------------------------------------------------------------------------------
bool_t Gui_startUp()
{
  gui_getState();

  return EKEY_VALID(_gui_state);
}

//--------------------------------------------------------------------------------------------
bool_t CGui_shutDown()
{
  return CGui_delete( &_gui_state );
}

//--------------------------------------------------------------------------------------------
Gui_t * CGui_new( Gui_t * gui )
{
  //fprintf( stdout, "CGui_new()\n");

  if(NULL == gui) return gui;

  CGui_delete( gui );

  memset(gui, 0, sizeof(Gui_t));

  EKEY_PNEW(gui, Gui_t);

  MenuProc_init( &(gui->mnu_proc) );

  //Pause button avalible?
  gui->can_pause = btrue;

  // initialize the textures
  gui->TxBars.textureID = INVALID_TEXTURE;
  gui->TxBlip.textureID = INVALID_TEXTURE;

  gui->clk = ClockState_create("Gui_t", -1);

  return gui;
}

//--------------------------------------------------------------------------------------------
bool_t CGui_delete( Gui_t * gui )
{
  if(NULL == gui) return bfalse;
  if(!EKEY_PVALID(gui)) return btrue;

  EKEY_PINVALIDATE(gui);

  MenuProc_delete( &(gui->mnu_proc) );

  ClockState_destroy( &(gui->clk) );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_player( Game_t * gs, CHR_REF character)
{
  if( !EKEY_PVALID(gs) || !ACTIVE_CHR(gs->ChrList, character)) return bfalse;

  return VALID_PLA(gs->PlaList, gs->ChrList[character].whichplayer);
}

//--------------------------------------------------------------------------------------------
bool_t count_players(Game_t * gs)
{
  int numdead;
  PLA_REF pla_cnt;
  CHR_REF ichr;
  PPla_t plalist;
  PChr_t chrlist;
  Client_t * cs;
  Server_t * ss;

  if( !EKEY_PVALID(gs) ) return bfalse;

  cs = gs->cl;
  ss = gs->sv;
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
  for (pla_cnt = 0; pla_cnt < PLALST_COUNT; pla_cnt++)
  {
    if( !VALID_PLA(plalist, pla_cnt) ) continue;

    if( INBITS_NONE == plalist[pla_cnt].device )
    {
      ss->rem_pla_count++;
    }
    else
    {
      cs->loc_pla_count++;
    }

    ichr = PlaList_getRChr( gs, pla_cnt );
    if ( !VALID_CHR( chrlist, ichr ) ) continue;

    if ( !chrlist[ichr].alive )
    {
      numdead++;
    };

    if ( chrlist[ichr].prop.canseeinvisible )
    {
      cs->seeinvisible = btrue;
    }

    if ( chrlist[ichr].prop.canseekurse )
    {
      cs->seekurse = btrue;
    }
  }

  gs->somepladead = ( numdead > 0 );
  gs->allpladead  = ( numdead >= cs->loc_pla_count + ss->rem_pla_count );

  return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_message_queue(MessageQueue_t * q)
{
  // ZZ> This function empties the message queue

  int cnt;

  q->count = 0;
  for (cnt = 0; cnt < MAXMESSAGE; cnt++)
  {
    q->list[cnt].time = 0;
  }

  q->timechange = 0;
  q->start = 0;
}

//--------------------------------------------------------------------------------------------
void clear_messages( MessageData_t * md)
{
  int cnt;

  md->total = 0;
  md->totalindex = 0;

  for ( cnt = 0; cnt < MAXTOTALMESSAGE; cnt++ )
  {
    md->index[cnt] = 0;
  }

  md->text[0] = EOS;
}

//--------------------------------------------------------------------------------------------
void load_global_icons(Game_t * gs)
{
  release_all_icons(gs);

  gs->ico_lst[ICO_NULL] = load_one_icon( gs, CData.basicdat_dir, NULL, CData.nullicon_bitmap );
  gs->ico_lst[ICO_KEYB] = load_one_icon( gs, CData.basicdat_dir, NULL, CData.keybicon_bitmap );
  gs->ico_lst[ICO_MOUS] = load_one_icon( gs, CData.basicdat_dir, NULL, CData.mousicon_bitmap );
  gs->ico_lst[ICO_JOYA] = load_one_icon( gs, CData.basicdat_dir, NULL, CData.joyaicon_bitmap );
  gs->ico_lst[ICO_JOYB] = load_one_icon( gs, CData.basicdat_dir, NULL, CData.joybicon_bitmap );
}


//--------------------------------------------------------------------------------------------

void recalc_character_bumpers( Game_t * gs )
{
  CHR_REF chr_ref;

  for(chr_ref = 0; chr_ref<CHRLST_COUNT; chr_ref++)
  {
    if( !ACTIVE_CHR(gs->ChrList, chr_ref) ) continue;

    chr_calculate_bumpers( gs, gs->ChrList + chr_ref, 0);
  };
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ObjList_new( Game_t * gs )
{
  OBJ_REF iobj;
  if(!EKEY_PVALID(gs)) return bfalse;

  for(iobj=0; iobj<OBJLST_COUNT; iobj++)
  {
    CProfile_new( gs->ObjList + iobj );

    gs->ObjFreeList[REF_TO_INT(iobj)]  = REF_TO_INT(iobj);
  }
  gs->ObjFreeList_count = OBJLST_COUNT;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ObjList_delete(Game_t * gs)
{
  OBJ_REF iobj;
  if(!EKEY_PVALID(gs)) return bfalse;

  for(iobj=0; iobj<OBJLST_COUNT; iobj++)
  {
    CProfile_delete( gs->ObjList + iobj );

    gs->ObjFreeList[REF_TO_INT(iobj)]  = REF_TO_INT(iobj);
  }
  gs->ObjFreeList_count = OBJLST_COUNT;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ObjList_renew( Game_t * gs )
{
  OBJ_REF iobj;
  if(!EKEY_PVALID(gs)) return bfalse;

  for(iobj=0; iobj<OBJLST_COUNT; iobj++)
  {
    CProfile_renew( gs->ObjList + iobj );

    gs->ObjFreeList[REF_TO_INT(iobj)]  = REF_TO_INT(iobj);
  }
  gs->ObjFreeList_count = OBJLST_COUNT;

  return btrue;
}

//--------------------------------------------------------------------------------------------
OBJ_REF ObjList_get_free( Game_t * gs, OBJ_REF request )
{
  int i;
  OBJ_REF retval = INVALID_OBJ;

  if(!EKEY_PVALID(gs)) return INVALID_OBJ;

  if(INVALID_OBJ == request)
  {
    // request is not specified. just give them the next one
    if(gs->ObjFreeList_count>0)
    {
      gs->ObjFreeList_count--;
      retval = gs->ObjFreeList[gs->ObjFreeList_count];
    }
  }
  else
  {
    // search for the requested reference in the free list
    for(i=0; i<gs->ObjFreeList_count; i++)
    {
      if(gs->ObjFreeList[i] == request)
      {
        // found it, so pop it off the list
        gs->ObjFreeList[i] = gs->ObjFreeList[gs->ObjFreeList_count-1];
        gs->ObjFreeList_count--;
        break;
      }
    }

    retval = request;
  }

  // initialize the data
  CProfile_new(gs->ObjList + request);

  return retval;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t Weather_init(WEATHER_INFO * w)
{
  if(NULL == w) return bfalse;

  w->active        = bfalse;
  w->require_water = bfalse;
  w->timereset     = 10;
  w->time          = 10;
  w->player        = INVALID_PLA;

  return btrue;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t tile_animated_reset(TILE_ANIMATED * t)
{
  if(NULL == t) return bfalse;

  t->updateand   =  7;                  // New tile every 7 frames

  t->frameand    =  3;                  // Only 4 frames
  t->baseand     =  ~(t->frameand);     //

  t->bigframeand =  7;                  // For big tiles
  t->bigbaseand  =  ~(t->bigframeand);  //

  t->framefloat  =  0;                  // Current frame
  t->frameadd    =  0;                  // Current frame rate

  return btrue;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fog_info_reset(FOG_INFO * f)
{
  if(NULL == f) return bfalse;

  f->on           = bfalse;          // Do ground fog?
  f->bottom       = 0.0;             //
  f->top          = 100;             //
  f->distance     = 100;             //
  f->red          = 255;             //  Fog color
  f->grn          = 255;             //
  f->blu          = 255;             //
  f->affectswater = bfalse;

  return btrue;
};