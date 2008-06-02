//********************************************************************************************
//* Egoboo - module.c
//*
//* Handles ingame maps and levels, called Modules
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

#include "module.h"
#include "Log.h"
#include "sound.h"
#include "Mesh.h"
#include "Particle.h"
#include "script.h"
#include "menu.h"
#include "enchant.h"
#include "object.h"
#include "Client.h"
#include "Server.h"
#include "game.h"

#include "egoboo_strutil.h"
#include "egoboo_utility.h"
#include "egoboo.h"

#include "graphic.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

GLtexture TxTitleImage[MAXMODULE];      /* title images */

//--------------------------------------------------------------------------------------------

static void   module_load_all_objects( GameState * gs, char * szModname );
static bool_t module_load_all_waves( SoundState * ss, char *modname );

//--------------------------------------------------------------------------------------------
void release_bumplist(void)
{
  bumplist_renew( &bumplist );
};

//--------------------------------------------------------------------------------------------
void module_release( GameState * gs )
{
  // ZZ> This function frees up memory used by the module

  bool_t client_running = bfalse, server_running = bfalse, local_running = bfalse;
  ModState * ms = &(gs->modstate);

  if(!ms->Active) return;

  client_running = ClientState_Running(gs->al_cs);
  server_running = sv_Running(gs->al_ss);
  local_running  = !client_running && !server_running;

  // if the client has shut down, release all client dependent resources
  if(local_running || !client_running)
  {
    release_all_textures(gs);
    release_all_icons(gs);
  }

  // if the server has shut down, release all server dependent resources
  if(local_running || !server_running)
  {
    release_map();
    release_bumplist();
  }

  if(local_running)
  {
    CapList_renew(gs);
    ChrList_renew(gs);

    PipList_renew(gs);
    PrtList_renew(gs);

    EveList_renew(gs);
    EncList_renew(gs);

    MadList_renew(gs);

    PlaList_renew(gs);
  }

}

//--------------------------------------------------------------------------------------------
bool_t module_reference_matches( char *szLoadName, IDSZ idsz )
{
  // ZZ> This function returns btrue if the named module has the required IDSZ

  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz;
  int cnt;

  if ( 0 == strcmp(szLoadName, "NONE") ) return btrue;
  if ( idsz == IDSZ_NONE ) return btrue;

  foundidsz = bfalse;
  snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.mnu_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return bfalse;

  // Read basic data
  globalname = szLoadName;
  fgoto_colon( fileread );   // Name of module...  Doesn't matter
  fgoto_colon( fileread );   // Reference directory...
  fgoto_colon( fileread );   // Reference IDSZ...
  fgoto_colon( fileread );   // Import...
  fgoto_colon( fileread );   // Export...
  fgoto_colon( fileread );   // Min players...
  fgoto_colon( fileread );   // Max players...
  fgoto_colon( fileread );   // Respawn...
  fgoto_colon( fileread );   // RTS... (OUTDATED)
  fgoto_colon( fileread );   // Rank...


  // Summary...
  cnt = 0;
  while ( cnt < SUMMARYLINES )
  {
    fgoto_colon( fileread );
    cnt++;
  }


  // Now check expansions
  while ( fgoto_colon_yesno( fileread ) && !foundidsz )
  {
    newidsz = fget_idsz( fileread );
    if ( newidsz == idsz )
    {
      foundidsz = btrue;
    }
  }


  fs_fileClose( fileread );

  return foundidsz;
}

//--------------------------------------------------------------------------------------------
void module_add_idsz( char *szLoadName, IDSZ idsz )
{
  // ZZ> This function appends an IDSZ to the module's menu.txt file

  FILE *filewrite;
  STRING newloadname;

  // Only add if there isn't one already
  if ( !module_reference_matches( szLoadName, idsz ) )
  {
    // Try to open the file in append mode
    snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.mnu_file );
    filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "a" );
    if ( filewrite )
    {
      fprintf( filewrite, "\n:[%4s]\n", undo_idsz( idsz ) );
      fs_fileClose( filewrite );
    }
  }
}

//--------------------------------------------------------------------------------------------
int module_find( char *smallname, MOD_INFO * mi_ary, size_t mi_size )
{
  // ZZ> This function returns -1 if the module does not exist locally, the module
  //     index otherwise

  int cnt, index;
  cnt = 0;
  index = -1;
  while ( cnt < mi_size )
  {
    if ( strcmp( smallname, mi_ary[cnt].loadname ) == 0 )
    {
      index = cnt;
      cnt = mi_size;
    }
    cnt++;
  }
  return index;
}

//--------------------------------------------------------------------------------------------
bool_t module_load( GameState * gs, char *smallname )
{
  // ZZ> This function loads a module

  STRING szModpath;

  if(NULL == gs || NULL == smallname || '\0' == smallname[0]) return bfalse;

  gs->modstate.beat = bfalse;
  timeron = bfalse;
  snprintf( szModpath, sizeof( szModpath ), "%s" SLASH_STRING "%s" SLASH_STRING, CData.modules_dir, smallname );

  make_randie();
  gs->randie_index = 0;

  reset_characters(gs);
  reset_teams( gs );
  reset_messages( gs );
  prime_names( gs );
  reset_particles( gs, szModpath );
  reset_ai_script( gs );
  mad_clear_pips( gs );

  load_one_icon( CData.basicdat_dir, NULL, CData.nullicon_bitmap );

  module_load_all_waves( &sndState, szModpath );

  read_wawalite( gs, szModpath );

  make_twist();

  load_basic_textures( gs, szModpath );

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s", szModpath, CData.gamedat_dir );
  if ( MAXAI == load_ai_script( GameState_getScriptInfo(gs), CStringTmp1, NULL ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s", CData.basicdat_dir, CData.script_file );
    load_ai_script( GameState_getScriptInfo(gs), CStringTmp1, NULL );
  };

  release_all_models( gs );

  EncList_delete( gs );

  module_load_all_objects( gs, szModpath );

  if ( !load_mesh( gs, szModpath ) )
  {
    log_error( "Load problems with the gs->mesh.\n" );
  }

  setup_particles( gs );

  PassList_load( gs, szModpath );

  reset_players( gs );

  setup_characters( gs, szModpath );

  reset_end_text( gs );

  setup_alliances( gs, szModpath );

  // Load fonts and bars after other images, as not to hog videomem
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModpath, CData.gamedat_dir, CData.font_bitmap );
  snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s%s" SLASH_STRING "%s", szModpath, CData.gamedat_dir, CData.fontdef_file );
  if ( !load_font( CStringTmp1, CStringTmp2 ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.font_bitmap );
    snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.fontdef_file );
    if ( !load_font( CStringTmp1, CStringTmp2 ) )
    {
      log_warning( "Fonts not loaded.  Files missing from %s directory\n", CData.basicdat_dir );
    }
  };

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING "%s", szModpath, CData.gamedat_dir, CData.bars_bitmap );
  if ( !load_bars( CStringTmp1 ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.bars_bitmap );
    if ( !load_bars( CStringTmp1 ) )
    {
      log_warning( "Could not load status bars. File missing = \"%s\"\n", CStringTmp1 );
    }
  };

  load_map( gs, szModpath );
  load_blip_bitmap( szModpath );

  if ( CData.DevMode )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.debug_file );
    MadList_log_used( gs, CData.debug_file );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t module_read_data( MOD_INFO * pmod, char *szLoadName )
{
  // ZZ> This function loads the module data file

  FILE *fileread;
  char reference[128];
  STRING playername;
  Uint32 idsz;
  int iTmp;
  bool_t playerhasquest;

  if(NULL == pmod) return bfalse;

  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadName, "r" );
  if ( NULL == fileread ) return bfalse;

  // Read basic data
  globalname = szLoadName;
  fget_next_name( fileread, pmod->longname, sizeof( pmod->longname ) );
  fget_next_string( fileread, reference, sizeof( reference ) );
  idsz = fget_next_idsz( fileread );

  //Check all selected players directories
  playerhasquest = bfalse;
  iTmp = 0;
  while ( !playerhasquest && iTmp < loadplayer_count)
  {
    snprintf( playername, sizeof( playername ), "%s", loadplayer[iTmp].dir );
    if( check_player_quest( playername, idsz ) >= 0 ) playerhasquest = btrue;
    iTmp++;
  }

  //Check for unlocked modules (Both in Quest IDSZ and Module IDSZ). Skip this if in DevMode.
  if( CData.DevMode || playerhasquest || module_reference_matches( reference, idsz ) )
  {
    globalname = szLoadName;
    pmod->importamount = fget_next_int( fileread );
    pmod->allowexport  = fget_next_bool( fileread );
    pmod->minplayers   = fget_next_int( fileread );
    pmod->maxplayers   = fget_next_int( fileread );
    pmod->respawnmode  = fget_next_respawn( fileread );
    pmod->rts_control  = fget_next_bool( fileread ) ;
    fget_next_string( fileread, generictext, sizeof( generictext ) );
    iTmp = 0;
    while ( iTmp < RANKSIZE - 1 )
    {
      pmod->rank[iTmp] = generictext[iTmp];
      iTmp++;
    }
    pmod->rank[iTmp] = 0;

    // Read the expansions
    return btrue;
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t module_read_summary( char *szLoadName, MOD_SUMMARY * ms )
{
  // ZZ> This function gets the quest description out of the module's menu file

  FILE *fileread;
  char szLine[160];
  int cnt;
  bool_t result = bfalse;

  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadName, "r" );
  if ( NULL != fileread )
  {
    // Skip over basic data
    globalname = szLoadName;
    fgoto_colon( fileread );   // Name...
    fgoto_colon( fileread );   // Reference...
    fgoto_colon( fileread );   // IDSZ...
    fgoto_colon( fileread );   // Import...
    fgoto_colon( fileread );   // Export...
    fgoto_colon( fileread );   // Min players...
    fgoto_colon( fileread );   // Max players...
    fgoto_colon( fileread );   // Respawn...
    fgoto_colon( fileread );   // Not Used
    fgoto_colon( fileread );   // Rank...


    // Read the summary
    cnt = 0;
    while ( cnt < SUMMARYLINES )
    {
      fget_next_string( fileread, szLine, sizeof( szLine ) );
      str_convert_underscores( ms->summary[cnt], sizeof( ms->summary[cnt] ), szLine );
      cnt++;
    }
    result = btrue;
  }

  fs_fileClose( fileread );
  return result;
}



//--------------------------------------------------------------------------------------------
void module_load_all_objects( GameState * gs, char * szModpath )
{
  // ZZ> This function loads a module's objects

  const char *filehandle;
  STRING szObjectpath, szTempdir, tmpstr;
  int cnt;
  int skin;
  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  // Clear the import slots...
  import_info_clear( &(gs->modstate.import) );

  // Load the import directory
  skin = TX_LAST;  // Character skins start just after the last special texture
  if ( gs->modstate.import.valid )
  {
    for ( cnt = 0; cnt < MAXIMPORT; cnt++ )
    {
      STRING objectname;

      snprintf( objectname, sizeof( objectname ), "temp%04d.obj", cnt );

      // Make sure the object exists...
      snprintf( szTempdir, sizeof( szTempdir ), "%s" SLASH_STRING "%s", CData.import_dir, objectname );
      if ( !fs_fileIsDirectory(szTempdir) ) continue;

      // Load it...
      gs->modstate.import.player = cnt / 9;

      import_info_add( &(gs->modstate.import), cnt);

      skin += load_one_object( gs, skin, CData.import_dir, objectname );
    }
  }

  empty_import_directory();  // Free up that disk space...

  // If in Developer mode, create a new debug.txt file for debug info logging
  log_debug( "DEBUG INFORMATION FOR MODULE: \"%s\"\n", szModpath );
  log_debug( "This document logs extra debugging information for the last module loaded.\n");
  log_debug( "\nSpawning log after module has started...\n");
  log_debug( "-----------------------------------------------\n" );


  // Search for .obj directories and load them
  gs->modstate.import.object = -100;
  snprintf( szObjectpath, sizeof( szObjectpath ), "%s%s" SLASH_STRING, szModpath, CData.objects_dir );

  filehandle = fs_findFirstFile( &fs_finfo, szObjectpath, NULL, "*.obj" );
  while ( NULL != filehandle )
  {
    int skins_loaded;
    strcpy(tmpstr, filehandle);

    skins_loaded = load_one_object( gs, skin, szObjectpath, tmpstr );
    if(0 == skins_loaded)
    {
      log_warning("module_load_all_objects() - Could not find object %s" SLASH_STRING "%s", szObjectpath, tmpstr);
    }

    skin += skins_loaded;
    filehandle = fs_findNextFile(&fs_finfo);
  }

  fs_findClose(&fs_finfo);
}


//--------------------------------------------------------------------------------------------
ModState * ModState_new(ModState * ms, MOD_INFO * mi, Uint32 seed)
{
  //fprintf( stdout, "ModState_new()\n");

  if(NULL == ms) return NULL;

  memset(ms, 0, sizeof(ModState));

  // initialize the the non-zero, non NULL, non-false values
  ms->seed            = seed;

  if(NULL != mi)
  {
    ms->import.amount  = mi->importamount;
    ms->import.min_pla = mi->maxplayers;
    ms->import.max_pla = mi->maxplayers;
    ms->import.valid   = (mi->importamount > 0) || (mi->maxplayers > 0);

    ms->exportvalid    = mi->allowexport;
    ms->rts_control    = mi->rts_control;
    ms->respawnvalid   = (RESPAWN_NONE != mi->respawnmode);
    ms->respawnanytime = (RESPAWN_ANYTIME == mi->respawnmode);
  };

  ms->initialized = btrue;

  return ms;
};


//--------------------------------------------------------------------------------------------
bool_t ModState_delete(ModState * ms)
{
  if(NULL == ms) return bfalse;
  if(!ms->initialized) return btrue;

  ms->Active = bfalse;
  ms->Paused = btrue;

  ms->initialized = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
ModState * ModState_renew(ModState * ms, MOD_INFO * mi, Uint32 seed)
{
  ModState_delete(ms);
  return ModState_new(ms, mi, seed);
}

//--------------------------------------------------------------------------------------------
bool_t module_load_all_waves( SoundState * ss, char *modname )
{
  // ZZ> This function loads the global waves used in a given modules

  STRING tmploadname, newloadname;
  Uint8 cnt;

  if ( NULL == ss || NULL == modname || '\0' == modname[0] ) return bfalse;
  
  if( !ss->soundActive || !ss->mixer_loaded ) return bfalse;

  // load in the sounds local to this module
  snprintf( tmploadname, sizeof( tmploadname ), "%s%s" SLASH_STRING, modname, CData.gamedat_dir );
  for ( cnt = 0; cnt < MAXWAVE; cnt++ )
  {
    snprintf( newloadname, sizeof( newloadname ), "%ssound%d.wav", tmploadname, cnt );
    ss->mc_list[cnt] = Mix_LoadWAV( newloadname );
  };

  // These sounds are always standard, but DO NOT override sounds that were loaded local to this module
  if ( NULL == ss->mc_list[GSOUND_COINGET] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinget_sound );
    ss->mc_list[GSOUND_COINGET] = Mix_LoadWAV( CStringTmp1 );
  };

  if ( NULL == ss->mc_list[GSOUND_DEFEND] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_sound );
    ss->mc_list[GSOUND_DEFEND] = Mix_LoadWAV( CStringTmp1 );
  }

  if ( NULL == ss->mc_list[GSOUND_COINFALL] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinfall_sound );
    ss->mc_list[GSOUND_COINFALL] = Mix_LoadWAV( CStringTmp1 );
  };

  if ( NULL == ss->mc_list[GSOUND_LEVELUP] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.globalparticles_dir, CData.lvlup_sound );
    ss->mc_list[GSOUND_LEVELUP] = Mix_LoadWAV( CStringTmp1 );
  };


  /*  The Global Sounds
  * 0 - Pick up coin
  * 1 - Defend clank
  * 2 - Weather Effect
  * 3 - Hit Water tile (Splash)
  * 4 - Coin falls on ground
  * 5 - Level up

  //TODO: These new values should determine sound and particle effects (examples below)
  Weather Type: DROPS, RAIN, SNOW, LAVABUBBLE (Which weather effect to spawn)
  Water Type: LAVA, WATER, DARK (To determine sound and particle effects)

  //We shold also add standard particles that can be used everywhere (Located and
  //loaded in globalparticles folder) such as these below.
  Particle Effect: REDBLOOD, SMOKE, HEALCLOUD
  */

  return btrue;
}

//---------------------------------------------------------------------------------------------
void module_quit( GameState * gs )
{
  // ZZ> This function forces a return to the menu

  bool_t client_running = bfalse, server_running = bfalse, local_running = bfalse;
  ModState * ms = &(gs->modstate);

  if(!ms->Active) return;

  client_running = ClientState_Running(gs->al_cs);
  server_running = sv_Running(gs->al_ss);
  local_running  = !client_running && !server_running;

  // only export players if it makes sense
  if( (ms->beat && ms->exportvalid) || ms->respawnanytime )
  {
    export_all_local_players( gs );
  }

  if(local_running)
  {
    // reset both the client and server data
    ClientState_renew(gs->al_cs);
    ServerState_renew(gs->al_ss);
  }
  else if(net_Started() && NULL != gs->al_cs)
  {
    // reset only the client data. let the server keep running.
    ClientState_unjoinGame(gs->al_cs);
    ClientState_shutDown(gs->al_cs);
  }

  // deallocate any memory
  module_release( gs );

  // Stop all sounds that are playing
  snd_stop_sound( -1 ); 

  // officiallly tell the game that we are offline
  ms->Active = bfalse;

  // clear any pause state for the game
  gs->proc.Paused = bfalse;
}



//--------------------------------------------------------------------------------------------
Uint32 module_load_one_image(int titleimage, char *szLoadName)
{
  // ZZ> This function loads a title in the specified image slot, forcing it into
  //     system memory.  Returns btrue if it worked
  Uint32 retval = MAXMODULE;

  if(INVALID_TEXTURE != GLTexture_Load(GL_TEXTURE_2D,  TxTitleImage + titleimage, szLoadName, INVALID_KEY))
  {
    retval = titleimage;
  }

  return retval;
}


//--------------------------------------------------------------------------------------------
size_t module_load_all_data(MOD_INFO * mi_ary, size_t mi_len)
{
  // ZZ> This function loads the title image for each module.  Modules without a
  //     title are marked as invalid

  char searchname[15];
  STRING loadname;
  const char *FileName;
  FILE* filesave;
  size_t modcount;

  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  // Convert searchname
  strcpy( searchname, "modules" SLASH_STRING "*.mod" );

  // Log a directory list
  filesave = fs_fileOpen( PRI_NONE, NULL, CData.modules_file, "w" );
  if ( filesave != NULL )
  {
    fprintf( filesave, "This file logs all of the modules found\n" );
    fprintf( filesave, "** Denotes an invalid module (Or locked)\n\n" );
  }
  else
  {
    log_warning( "Could not write to %s\n", CData.modules_file );
  }

  // Search for .mod directories
  FileName = fs_findFirstFile( &fs_finfo, CData.modules_dir, NULL, "*.mod" );
  modcount = 0;
  while ( FileName && modcount < mi_len )
  {
    strncpy( mi_ary[modcount].loadname, FileName, sizeof( mi_ary[modcount].loadname ) );
    snprintf( loadname, sizeof( loadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.mnu_file );
    if ( module_read_data( mi_ary + modcount, loadname ) )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.title_bitmap );
      mi_ary[modcount].tx_title_idx = module_load_one_image( modcount, CStringTmp1 );
      if ( MAXMODULE != mi_ary[modcount].tx_title_idx )
      {
        fprintf( filesave, "%02d.  %s\n", modcount, mi_ary[modcount].longname );
        modcount++;
      }
      else
      {
        fprintf( filesave, "**.  %s\n", FileName );
      }
    }
    else
    {
      fprintf( filesave, "**.  %s\n", FileName );
    }
    FileName = fs_findNextFile(&fs_finfo);
  }
  fs_findClose(&fs_finfo);
  if ( filesave != NULL ) fs_fileClose( filesave );

  return modcount;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
MOD_SUMMARY * ModSummary_new( MOD_SUMMARY * ms )
{
  int i;

  //fprintf( stdout, "ModSummary_new()\n");

  if(NULL == ms) return ms;

  ms->numlines = 0;
  for(i=0; i<SUMMARYLINES; i++)
  {
    ms->summary[i][0] = '\0';
  }

  return ms;
};

//--------------------------------------------------------------------------------------------
bool_t ModSummary_delete( MOD_SUMMARY * ms )
{
  if(NULL == ms) return bfalse;

  ms->numlines = 0;

  return bfalse;
}

//--------------------------------------------------------------------------------------------
MOD_SUMMARY * ModSummary_renew( MOD_SUMMARY * ms )
{
  ModSummary_delete(ms);

  return ModSummary_new(ms);
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
MOD_INFO * ModInfo_new( MOD_INFO * pmi )
{
  if(NULL == pmi) return pmi;

  //fprintf( stdout, "ModInfo_new()\n");

  memset(pmi, 0, sizeof(MOD_INFO));
  pmi->tx_title_idx = MAXMODULE;

  return pmi;
}

//--------------------------------------------------------------------------------------------
bool_t ModInfo_delete( MOD_INFO * pmi )
{
  if(NULL == pmi) return bfalse;

  memset(pmi, 0, sizeof(MOD_INFO));
  pmi->tx_title_idx = MAXMODULE;

  return btrue;
}

//--------------------------------------------------------------------------------------------
MOD_INFO * ModInfo_renew( MOD_INFO * pmi )
{
  ModInfo_delete(pmi);
  return ModInfo_new(pmi);
}