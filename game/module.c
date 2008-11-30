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

///
/// @file
/// @brief Egoboo Model Manager
/// @details Handles ingame maps and levels, called Modules

#include "module.h"

#include "Log.h"
#include "sound.h"
#include "Script_compile.h"
#include "Menu.h"
#include "enchant.h"
#include "Client.h"
#include "Server.h"
#include "game.h"
#include "Ui.h"
#include "file_common.h"

#include "egoboo_strutil.h"
#include "egoboo_utility.h"
#include "egoboo.h"

#include "particle.inl"
#include "object.inl"
#include "graphic.inl"
#include "mesh.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_spawn_info
{
  STRING   name;
  int      slot;
  vect3    pos;
  int      dir;
  int      money;
  int      skin;
  PASS_REF passage;
  int      content;
  int      level;
  bool_t   has_status;
  bool_t   is_ghost;
  TEAM_REF iteam;
};

typedef struct s_spawn_info spawn_info_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void   module_load_all_objects( Game_t * gs, char * szModname );
static bool_t module_load_all_waves( Game_t * gs, char *modname );
static bool_t module_read_egomap_extra( Game_t * gs, const char * szModPath );

//--------------------------------------------------------------------------------------------
void release_bumplist(MeshInfo_t * mi)
{
  bumplist_renew( &(mi->bumplist) );
}

//--------------------------------------------------------------------------------------------
void module_release( Game_t * gs )
{
  /// @details ZZ@> This function frees up memory used by the module

  Mesh_t     * pmesh = Game_getMesh(gs);
  ModState_t * ms    = &(gs->modstate);
  MeshInfo_t * mi    = &(pmesh->Info);
  Graphics_Data_t * gfx = Game_getGfx( gs );

  if(!ms->Active) return;

  // if the game is local or a client,
  // release all client dependent resources
  if( Game_hasClient(gs) )
  {
    // release client resources
    release_all_textures( gfx );
    release_all_icons( gfx  );
  }

  // if the game is local or a server,
  // release all server dependent resources
  if( Game_hasServer(gs) )
  {
    // release server resources
    release_map( gfx );
    release_bumplist( mi );
  }

  if( Game_isLocal(gs) )
  {
    CapList_renew(gs);
    ChrList_new(gs);

    PipList_renew(gs);
    PrtList_new(gs);

    EveList_renew(gs);
    EncList_new(gs);

    MadList_renew(gs);

    PlaList_renew(gs);
  }

}

//--------------------------------------------------------------------------------------------
bool_t module_reference_matches( const char *szLoadName, IDSZ idsz )
{
  /// @details ZZ@> This function returns btrue if the named module has the required IDSZ

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
void module_add_idsz( const char *szLoadName, IDSZ idsz )
{
  /// @details ZZ@> This function appends an IDSZ to the module's menu.txt file

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
int module_find( const char *smallname, MOD_INFO * mi_ary, size_t mi_size )
{
  /// @details ZZ@> This function returns -1 if the module does not exist locally, the module
  ///     index otherwise

  size_t cnt, index;

  index = mi_size;
  for ( cnt=0; cnt < mi_size; cnt++ )
  {
    if ( strcmp( smallname, mi_ary[cnt].loadname ) == 0 )
    {
      index = cnt;
      break;
    }
  }

  return (index==mi_size) ? -1 : (int)index;
}

//--------------------------------------------------------------------------------------------
bool_t module_load( Game_t * gs, const char *smallname )
{
  /// @details ZZ@> This function loads a module

  STRING szModpath;
  Graphics_Data_t * gfx = Game_getGfx( gs );

  if(!EKEY_PVALID(gs) ||  !VALID_CSTR(smallname) ) return bfalse;

  gs->modstate.beat = bfalse;
  gs->timeron = bfalse;
  snprintf( szModpath, sizeof( szModpath ), "%s" SLASH_STRING "%s" SLASH_STRING, CData.modules_dir, smallname );

  make_randie();
  gs->randie_index = 0;

  reset_characters(gs);
  reset_teams( gs );
  reset_messages( gs );
  naming_prime( gs );
  reset_particles( gs, szModpath );
  reset_ai_script( gs );
  obj_clear_pips( gs );

  module_load_all_waves( gs, szModpath );

  read_wawalite( gs, szModpath );

  mesh_make_twist();

  load_basic_textures( gs, szModpath );

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s", szModpath, CData.gamedat_dir );
  if ( INVALID_AI == load_ai_script( Game_getScriptInfo(gs), CStringTmp1, NULL ) )
  {
    log_warning("Cannot find the requested script, using default.\n");
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s", CData.basicdat_dir, CData.script_file );
    load_ai_script( Game_getScriptInfo(gs), CStringTmp1, NULL );
  };

  release_all_models( gs );

  EncList_new( gs );

  module_load_all_objects( gs, szModpath );

  if ( !mesh_load( Game_getMesh(gs), szModpath ) )
  {
    log_error( "Load problems with the mesh.\n" );
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
  if ( !ui_load_BMFont( CStringTmp1, CStringTmp2 ) )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.font_bitmap );
    snprintf( CStringTmp2, sizeof( CStringTmp2 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.fontdef_file );
    if ( !ui_load_BMFont( CStringTmp1, CStringTmp2 ) )
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

  load_map( gfx, szModpath );
  load_blip_bitmap( gfx, szModpath );

  module_read_egomap_extra( gs, szModpath );

  if ( CData.DevMode )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.debug_file );
    ObjList_log_used( gs, CData.debug_file );
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t module_read_data( MOD_INFO * pmod, const char *szLoadName )
{
  /// @details ZZ@> This function loads the module data file

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
  while ( !playerhasquest && iTmp < loadplayer_count )
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
    fget_next_bool( fileread );
    fget_next_string( fileread, pmod->rank, sizeof( pmod->rank ) );
    pmod->rank[RANKSIZE-1] = EOS;

    // Read the expansions
    return btrue;
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t module_read_summary( char *szLoadName, ModSummary_t * ms )
{
  /// @details ZZ@> This function gets the quest description out of the module's menu file

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
      str_decode( ms->summary[cnt], sizeof( ms->summary[cnt] ), szLine );
      cnt++;
    }
    result = btrue;
  }

  fs_fileClose( fileread );
  return result;
}



//--------------------------------------------------------------------------------------------
void module_load_all_objects( Game_t * gs, char * szModpath )
{
  /// @details ZZ@> This function loads a module's objects

  EGO_CONST char *filehandle;
  STRING szObjectpath, szTempdir, tmpstr;
  int cnt;
  int skin;
  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  //// Clear the import slots...
  //import_info_clear( &(gs->modstate.import) );

  // Load the import directory
  skin = TX_LAST;  // Character skins start just after the last special texture
  if ( gs->modstate.import_valid )
  {
    for ( cnt = 0; cnt < MAXIMPORT; cnt++ )
    {
      STRING objectname;

      snprintf( objectname, sizeof( objectname ), "temp%04d.obj", cnt );

      // Make sure the object exists...
      snprintf( szTempdir, sizeof( szTempdir ), "%s" SLASH_STRING "%s", CData.import_dir, objectname );
      if ( !fs_fileIsDirectory(szTempdir) ) continue;

      //// Load it...
      //gs->modstate.import.player = cnt / MAXIMPORTPERCHAR;

      //import_info_add( &(gs->modstate.import), OBJ_REF(cnt) );

      skin += load_one_object( gs, skin, CData.import_dir, objectname, cnt );
    }
  }

  // If in Developer mode, create a new debug.txt file for debug info logging
  log_debug( "DEBUG INFORMATION FOR MODULE: \"%s\"\n", szModpath );
  log_debug( "This document logs extra debugging information for the last module loaded.\n");
  log_debug( "Spawning log after module has started...\n");
  log_debug( "-----------------------------------------------\n" );


  // Search for .obj directories and load them
  //gs->modstate.import.object = INVALID_OBJ;
  snprintf( szObjectpath, sizeof( szObjectpath ), "%s%s" SLASH_STRING, szModpath, CData.objects_dir );

  filehandle = fs_findFirstFile( &fs_finfo, szObjectpath, NULL, "*.obj" );
  while ( NULL != filehandle )
  {
    int skins_loaded;
    strcpy(tmpstr, filehandle);

    skins_loaded = load_one_object( gs, skin, szObjectpath, tmpstr, INVALID_OBJ );
    if(0 == skins_loaded)
    {
      log_warning("module_load_all_objects() - \n\tCould not load object %s" SLASH_STRING "%s", szObjectpath, tmpstr);
    }

    skin += skins_loaded;
    filehandle = fs_findNextFile(&fs_finfo);
  }

  fs_findClose(&fs_finfo);
}


//--------------------------------------------------------------------------------------------
ModState_t * ModState_new(ModState_t * ms, MOD_INFO * mi, Uint32 seed)
{
  //fprintf( stdout, "ModState_new()\n");

  if(NULL == ms) return NULL;

  ModState_delete( ms );

  memset(ms, 0, sizeof(ModState_t));

  EKEY_PNEW(ms, ModState_t);

  // initialize the the non-zero, non NULL, non-false values
  ms->seed            = seed;

  if(NULL != mi)
  {
    ms->import_amount  = mi->importamount;
    ms->import_min_pla = mi->maxplayers;
    ms->import_max_pla = mi->maxplayers;
    ms->import_valid   = (mi->importamount > 0) || (mi->maxplayers > 1);

    ms->exportvalid    = mi->allowexport;
    //ms->rts_control    = mi->rts_control;
    ms->respawnvalid   = (RESPAWN_NONE != mi->respawnmode);
    ms->respawnanytime = (RESPAWN_ANYTIME == mi->respawnmode);
  };

  return ms;
}


//--------------------------------------------------------------------------------------------
bool_t ModState_delete(ModState_t * ms)
{
  if(NULL == ms) return bfalse;
  if( !EKEY_PVALID(ms) ) return btrue;

  EKEY_PINVALIDATE(ms);

  ms->Active = bfalse;
  ms->Paused = btrue;

  return btrue;
}

//--------------------------------------------------------------------------------------------
ModState_t * ModState_renew(ModState_t * ms, MOD_INFO * mi, Uint32 seed)
{
  ModState_delete(ms);
  return ModState_new(ms, mi, seed);
}

//--------------------------------------------------------------------------------------------
bool_t module_load_all_waves( Game_t * gs, char *modname )
{
  /// @details ZZ@> This function loads the global waves used in a given modules

  STRING tmploadname, newloadname;
  Uint8 cnt;

  SoundState_t * snd = snd_getState(gs->cd);

  if ( NULL == snd ||  !VALID_CSTR(modname)  ) return bfalse;

  if( !snd->soundActive || !snd->mixer_loaded ) return bfalse;

  // load in the sounds local to this module
  snprintf( tmploadname, sizeof( tmploadname ), "%s%s" SLASH_STRING, modname, gs->cd->gamedat_dir );
  for ( cnt = 0; cnt < MAXWAVE; cnt++ )
  {
    snprintf( newloadname, sizeof( newloadname ), "%ssound%d.wav", tmploadname, cnt );
    snd->mc_list[cnt] = Mix_LoadWAV( newloadname );
  };

  // These sounds are always standard, but DO NOT override sounds that were loaded local to this module
  if ( NULL == snd->mc_list[GSOUND_COINGET] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", gs->cd->basicdat_dir, gs->cd->globalparticles_dir, gs->cd->coinget_sound );
    snd->mc_list[GSOUND_COINGET] = Mix_LoadWAV( CStringTmp1 );
  };

  if ( NULL == snd->mc_list[GSOUND_DEFEND] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", gs->cd->basicdat_dir, gs->cd->globalparticles_dir, gs->cd->defend_sound );
    snd->mc_list[GSOUND_DEFEND] = Mix_LoadWAV( CStringTmp1 );
  }

  if ( NULL == snd->mc_list[GSOUND_COINFALL] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", gs->cd->basicdat_dir, gs->cd->globalparticles_dir, gs->cd->coinfall_sound );
    snd->mc_list[GSOUND_COINFALL] = Mix_LoadWAV( CStringTmp1 );
  };

  if ( NULL == snd->mc_list[GSOUND_LEVELUP] )
  {
    snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", gs->cd->basicdat_dir, gs->cd->globalparticles_dir, gs->cd->lvlup_sound );
    snd->mc_list[GSOUND_LEVELUP] = Mix_LoadWAV( CStringTmp1 );
  };


  // The Global Sounds
  //   0 - Pick up coin
  //   1 - Defend clank
  //   2 - Weather Effect
  //   3 - Hit Water tile (Splash)
  //   4 - Coin falls on ground
  //   5 - Level up

  /// @todo These new values should determine sound and particle effects (examples below)
  /// Weather Type: DROPS, RAIN, SNOW, LAVABUBBLE (Which weather effect to spawn)
  /// Water Type: LAVA, WATER, DARK (To determine sound and particle effects)
  ///
  /// We shold also add standard particles that can be used everywhere (Located and
  /// loaded in globalparticles folder) such as these below.
  /// Particle Effect: REDBLOOD, SMOKE, HEALCLOUD

  return btrue;
}

//---------------------------------------------------------------------------------------------
void module_quit( Game_t * gs )
{
  /// @details ZZ@> This function forces a return to the menu

  ModState_t * ms = &(gs->modstate);

  if(!ms->Active) return;

  // only export players if it makes sense
  if( (ms->beat && ms->exportvalid) || ms->respawnanytime )
  {
    export_all_local_players( gs, ms->beat );
  }

  if( Game_isLocal(gs) )
  {
    // reset both the client and server data
    Client_renew(gs->cl);
    Server_renew(gs->sv);
  }
  else if( Game_hasClient(gs) )
  {
    // reset only the client data. let the server keep running.
    Client_unjoinGame(gs->cl);
    Client_shutDown(gs->cl);
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
//--------------------------------------------------------------------------------------------
ModSummary_t * ModSummary_new( ModSummary_t * ms )
{
  int i;

  //fprintf( stdout, "ModSummary_new()\n");

  if(NULL == ms) return ms;

  ModSummary_delete( ms );

  memset( ms, 0, sizeof(ModSummary_t) );

  EKEY_PNEW( ms, ModSummary_t );

  ms->numlines = 0;
  for(i=0; i<SUMMARYLINES; i++)
  {
    ms->summary[i][0] = EOS;
  }

  return ms;
}

//--------------------------------------------------------------------------------------------
bool_t ModSummary_delete( ModSummary_t * ms )
{
  if(NULL == ms) return bfalse;
  if( !EKEY_PVALID(ms) ) return btrue;

  EKEY_PINVALIDATE(ms);

  ms->numlines = 0;

  return bfalse;
}

//--------------------------------------------------------------------------------------------
ModSummary_t * ModSummary_renew( ModSummary_t * ms )
{
  ModSummary_delete(ms);

  return ModSummary_new(ms);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
MOD_INFO * ModInfo_new( MOD_INFO * pmi )
{
  if(NULL == pmi) return pmi;

  ModInfo_delete( pmi );

  //fprintf( stdout, "ModInfo_new()\n");

  memset(pmi, 0, sizeof(MOD_INFO));

  EKEY_PNEW( pmi, MOD_INFO );

  pmi->tx_title_idx = MAXMODULE;

  return pmi;
}

//--------------------------------------------------------------------------------------------
bool_t ModInfo_delete( MOD_INFO * pmi )
{
  if(NULL == pmi) return bfalse;
  if( !EKEY_PVALID(pmi) ) return btrue;

  EKEY_PINVALIDATE(pmi);

  pmi->tx_title_idx = MAXMODULE;

  return btrue;
}

//--------------------------------------------------------------------------------------------
MOD_INFO * ModInfo_renew( MOD_INFO * pmi )
{
  ModInfo_delete(pmi);
  return ModInfo_new(pmi);
}

//---------------------------------------------------------------------------------------------
void ModInfo_clear_all_titleimages( MOD_INFO * mi_ary, size_t mi_count )
{
  size_t cnt;

  if(NULL == mi_ary || 0 == mi_count) return;

  for (cnt = 0; cnt < mi_count; cnt++)
  {
    mi_ary[cnt].tx_title_idx = MAXMODULE;
  };

}


//---------------------------------------------------------------------------------------------
bool_t module_read_egomap_extra( Game_t * gs, const char * szModPath )
{
  FILE * ftmp;
  STRING fname;
  int file_version;
  int dynalight_max;
  int i;

  if( !VALID_CSTR(szModPath) ) return bfalse;

  // create the filename
  strncpy(fname, szModPath, sizeof(STRING));
  str_append_slash(fname, sizeof(STRING));
  strcat(fname, gs->cd->gamedat_dir);
  str_append_slash(fname, sizeof(STRING));
  strcat(fname, "EgoMapExtra.txt");

  // open the file
  ftmp = fs_fileOpen( PRI_NONE, NULL, fname, "r" );
  if(NULL == ftmp) return bfalse;

  file_version = fget_next_int(ftmp);

  dynalight_max = fget_next_int(ftmp);
  for(i=0; i<dynalight_max; i++)
  {
    float xpos, ypos, level, radius;
    DYNALIGHT_INFO di;

    fgoto_colon(ftmp);
    xpos = fget_float(ftmp);
    ypos = fget_float(ftmp);
    level = fget_next_float(ftmp);
    radius = fget_next_float(ftmp);;

    if(level <=0 || radius <=0) continue;

    di.pos.x = xpos * (1 << 7);
    di.pos.y = ypos * (1 << 7);
    di.pos.z = 1.0f * (1 << 7);

    /// @todo : I need to figure out the scaling for this!!!
    di.falloff = radius;
    di.level   = level;
    di.permanent = btrue;

    DLightList_add( Game_getGfx(gs), &di );
  };

  fs_fileClose( ftmp );

  return btrue;
}

//--------------------------------------------------------------------------------------------
void release_map( Graphics_Data_t * gfx )
{
  /// @details ZZ@> This function releases all the map images

  GLtexture_Release( &gfx->Map_tex );
}

//--------------------------------------------------------------------------------------------
static bool_t fread_spawn_info(FILE * fileread, spawn_info_t * pinfo)
{
  if(NULL == fileread || NULL == pinfo) return bfalse;

  fget_string(fileread, pinfo->name, sizeof(pinfo->name) );
  pinfo->slot       = fget_int(fileread);
  pinfo->pos.x      = fget_float(fileread);
  pinfo->pos.y      = fget_float(fileread);
  pinfo->pos.z      = fget_float(fileread);
  pinfo->dir        = fget_int(fileread);
  pinfo->money      = fget_int(fileread);
  pinfo->skin       = fget_int(fileread);
  pinfo->passage    = fget_int(fileread);
  pinfo->content    = fget_int(fileread);
  pinfo->level      = fget_int(fileread);
  pinfo->has_status = fget_bool(fileread);
  pinfo->is_ghost   = fget_bool(fileread);
  pinfo->iteam      = fget_int(fileread);

  return btrue;
}

//--------------------------------------------------------------------------------------------
static bool_t fwrite_spawn_info(FILE * filewrite, const char * objname, spawn_info_t * pinfo, const char * comment)
{
  if(NULL == filewrite || NULL == pinfo ) return bfalse;

  if( VALID_CSTR(objname) )
  {
    fprintf(filewrite, "%s : ", objname);
  }
  else
  {
    fprintf(filewrite, "UNKNOWN : ");
  }


  fput_string(filewrite, pinfo->name );
  fput_int  (filewrite, pinfo->slot      );
  fput_float(filewrite, pinfo->pos.x     );
  fput_float(filewrite, pinfo->pos.y     );
  fput_float(filewrite, pinfo->pos.z     );
  fput_int  (filewrite, pinfo->dir       );
  fput_int  (filewrite, pinfo->money     );
  fput_int  (filewrite, pinfo->skin      );
  fput_int  (filewrite, pinfo->passage   );
  fput_int  (filewrite, pinfo->content   );
  fput_int  (filewrite, pinfo->level     );
  fput_bool (filewrite, pinfo->has_status);
  fput_bool (filewrite, pinfo->is_ghost  );
  fput_int  (filewrite, pinfo->iteam     );

  if( VALID_CSTR(comment) )
  {
    fputs( comment, filewrite );
  }

  fputs( "\n", filewrite );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t translate_spawn_file( Game_t * gs, const char *modname )
{
  FILE * spawnfile, *spawnfile2;
  STRING spawnname, spawnname2;
  STRING objectname, comment;
  spawn_info_t spinfo;
  int cnt;

  // load all of the objects
  module_load_all_objects( gs, modname );

  // open the source file
  strncpy( spawnname, modname, sizeof(spawnname) );
  str_append_slash( spawnname, sizeof(spawnname) );
  strcat( spawnname, CData.spawn_file );
  spawnfile = fs_fileOpen(PRI_NONE, "translate_spawn_file()", spawnname, "r");
  if(NULL == spawnfile) return bfalse;

  // open the dest file
  strncpy( spawnname2, modname, sizeof(spawnname2) );
  str_append_slash( spawnname2, sizeof(spawnname2) );
  strcat( spawnname2, CData.spawn2_file );
  spawnfile2 = fs_fileOpen(PRI_NONE, "translate_spawn_file()", spawnname2, "r");
  if(NULL == spawnfile2)
  {
    fs_fileClose(spawnfile);
    return bfalse;
  }

  for(cnt = 0; !feof(spawnfile); cnt++ )
  {
    if( !fgoto_colon_yesno_buffer(spawnfile, objectname, sizeof(objectname)) )
    {
      break;
    }

    // read the spawning info for this character/item
    if( !fread_spawn_info(spawnfile, &spinfo) ) break;

    // read any comment
    fgets(comment, sizeof(comment), spawnfile);

    // make sure that we have a correct name for the object
    // this is the WHOLE POINT of the creation of spawn2.txt! :)
    if( spinfo.slot < 0 )
    {
      // this object is to be imported from the import dir when the mosule is loaded
      snprintf( objectname, sizeof(objectname), "import%02d.obj", cnt ); 
    }
    else if( NULL != ObjList_getPObj(gs, spinfo.slot) )
    {
      // we found the slot number in the object database
      Obj_t * pobj = ObjList_getPObj(gs, spinfo.slot);
      strncpy(objectname, pobj->name, sizeof(objectname));
    }
    else
    {
      // assume that the objectname that we loaded is the actual object name
      /* nothing */
    }

    // write out the new line of text
    fwrite_spawn_info(spawnfile2, objectname, &spinfo, comment);
  }

  fs_fileClose(spawnfile);
  fs_fileClose(spawnfile2);

  return btrue;
};