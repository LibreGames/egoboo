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
/// @brief Egoboo object manager
/// @details Implements the Egoboo object manager

#include "object.inl"

#include "Mad.h"
#include "enchant.h"
#include "game.h"
#include "Log.h"
#include "sound.h"
#include "file_common.h"
#include "Script_compile.h"

#include "egoboo_utility.h"
#include "egoboo_strutil.h"

#include "graphic.inl"
#include "particle.inl"
#include "char.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
int load_one_object( Game_t * gs, int skin_count, EGO_CONST char * szObjectpath, char* szObjectname, OBJ_REF slot_override )
{
  /// @details ZZ@> This function loads one object and returns the number of skins

  int numskins, TxIcon_count, skin_index;
  STRING newloadname, loc_loadpath, wavename;
  int cnt;

  Graphics_Data_t * gfx = Game_getGfx( gs );

  OBJ_REF iobj;
  Obj_t  * pobj;

  // generate an index for this object
  strcpy(newloadname, szObjectpath);
  str_append_slash(newloadname, sizeof(newloadname));
  if(NULL != szObjectname)
  {
    strcat(newloadname, szObjectname);
    str_append_slash(newloadname, sizeof(newloadname));
  }
  strcat(newloadname, CData.data_file);

  // grab the requested object
  if( INVALID_OBJ != slot_override )
  {
    // we have already been given the slot to use
    iobj = slot_override;
  }
  else
  {
    // grab the slot from the data.txt file
    iobj = object_generate_index(newloadname);
  }

  // check see if the object is available
  iobj = ObjList_get_free(gs, iobj);
  if(INVALID_OBJ == iobj)
  {
    // could not obtain a valid object
    return 0;
  }

  pobj = gs->ObjList +  iobj;

  // Make up a name for the model...  IMPORT\TEMP0000.OBJ
  strncpy( pobj->name, szObjectpath, sizeof( pobj->name ) );
  if(NULL != szObjectname)
  {
    strncat( pobj->name, szObjectname, sizeof( pobj->name ) );
  }

  // Append a slash to the szObjectname
  strncpy( loc_loadpath, szObjectpath, sizeof( loc_loadpath ) );
  str_append_slash( loc_loadpath, sizeof( loc_loadpath ) );
  strncat( loc_loadpath, szObjectname, sizeof( loc_loadpath ) );
  str_append_slash( loc_loadpath, sizeof( loc_loadpath ) );

  // Load the iobj data file
  pobj->cap = CapList_load_one( gs, szObjectpath, szObjectname, CAP_REF(REF_TO_INT(iobj)) );

  // Load the AI script for this object
  pobj->ai = load_ai_script( Game_getScriptInfo(gs), szObjectpath, szObjectname );
  if ( INVALID_AI == pobj->ai )
  {
    // use the default script
    pobj->ai = 0;
  }

  // load the model data
  pobj->mad = MadList_load_one( gs, szObjectpath, szObjectname, MAD_REF(REF_TO_INT(iobj)) );

  // Fix lighting if need be
  if ( gs->CapList[pobj->cap].uniformlit )
  {
    make_mad_equally_lit( gs, pobj->mad );
  }

  // Load the messages for this object
  pobj->msg_start = load_all_messages( gs, szObjectpath, szObjectname );

  // Load the random naming table for this object
  naming_read( gs, szObjectpath, szObjectname, pobj );

  // Load the particles for this object
  for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
  {
    snprintf( newloadname, sizeof( newloadname ), "part%d.txt", cnt );
    pobj->prtpip[cnt] = PipList_load_one( gs, szObjectpath, szObjectname, newloadname, INVALID_PIP );
  }

  // Load the waves for this object
  for ( cnt = 0; cnt < MAXWAVE; cnt++ )
  {
    snprintf( wavename, sizeof( wavename ), "sound%d.wav", cnt );
    pobj->wavelist[cnt] = Mix_LoadWAV( inherit_fname(szObjectpath, szObjectname, wavename) );
  }

  // Load the enchantment for this object
  pobj->eve = EveList_load_one( gs, szObjectpath, szObjectname, EVE_REF(REF_TO_INT(iobj)) );

  // Load the skins and icons
  pobj->skinstart = skin_count;
  numskins = 0;
  TxIcon_count = 0;
  for ( skin_index = 0; skin_index < MAXSKIN; skin_index++ )
  {
    STRING fname;
    Uint32 tx_index;

    // try various file types
    snprintf( fname, sizeof( newloadname ), "tris%d.png", skin_index );
    tx_index = GLtexture_Load( GL_TEXTURE_2D,  gfx->TxTexture + skin_count+numskins, inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );

    if(INVALID_TX_ID == tx_index)
    {
      snprintf( fname, sizeof( newloadname ), "tris%d.bmp", skin_index );
      tx_index = GLtexture_Load( GL_TEXTURE_2D,  gfx->TxTexture + skin_count+numskins, inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );
    }

    if(INVALID_TX_ID == tx_index)
    {
      snprintf( fname, sizeof( newloadname ), "tris%d.pcx", skin_index );
      tx_index = GLtexture_Load( GL_TEXTURE_2D,  gfx->TxTexture + skin_count + numskins, inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );
    }

    if ( INVALID_TX_ID != tx_index )
    {
      numskins++;

      snprintf( fname, sizeof( newloadname ), "icon%d.png", skin_index );
      tx_index = GLtexture_Load( GL_TEXTURE_2D,  gfx->TxIcon + gfx->TxIcon_count, inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );

      if ( INVALID_TX_ID == tx_index )
      {
        snprintf( fname, sizeof( newloadname ), "icon%d.bmp", skin_index );
        tx_index = GLtexture_Load( GL_TEXTURE_2D,  gfx->TxIcon + gfx->TxIcon_count, inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );
      }

      if ( INVALID_TX_ID == tx_index )
      {
        snprintf( fname, sizeof( newloadname ), "icon%d.pcx", skin_index );
        tx_index = GLtexture_Load( GL_TEXTURE_2D,  gfx->TxIcon + gfx->TxIcon_count, inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );
      }

      if( INVALID_TX_ID != tx_index )
      {
        if ( iobj == SPELLBOOK && MAXICONTX == gfx->ico_lst[ICO_BOOK_0] )
        {
          gfx->ico_lst[ICO_BOOK_0] = gfx->TxIcon_count;
        }

        while ( TxIcon_count < numskins )
        {
          gs->skintoicon[skin_count+TxIcon_count] = gfx->TxIcon_count;
          TxIcon_count++;
        }

        gfx->TxIcon_count++;
      }
    }
  }

  if ( numskins == 0 )
  {
    // If we didn't get a skin_count, set it to the water texture
    numskins = 1;
    pobj->skinstart = TX_WATER_TOP;
  }
  pobj->skins = numskins;

  pobj->Active = btrue;

  return numskins;
}

//--------------------------------------------------------------------------------------------
void switch_team( Game_t * gs, CHR_REF chr_ref, TEAM_REF team )
{
  /// @details ZZ@> This function makes a character join another team...

  PChr_t chrlst      = gs->ChrList;

  if ( team < TEAM_COUNT )
  {
    if ( !chrlst[chr_ref].prop.invictus )
    {
      gs->TeamList[chrlst[chr_ref].team_base].morale--;
      gs->TeamList[team].morale++;
    }
    if (( !chrlst[chr_ref].prop.ismount || !chr_using_slot( chrlst, CHRLST_COUNT, chr_ref, SLOT_SADDLE ) ) &&
        ( !chrlst[chr_ref].prop.isitem  || !chr_attached( chrlst, CHRLST_COUNT, chr_ref ) ) )
    {
      chrlst[chr_ref].team = team;
    }

    chrlst[chr_ref].team_base = team;
    if ( !ACTIVE_CHR( chrlst, team_get_leader( gs, team ) ) )
    {
      gs->TeamList[team].leader = chr_ref;
    }
  }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( Game_t * gs, CHR_REF chr_ref, IDSZ idsz )
{
  /// @details ZZ@> This function restocks the characters ammo, if it needs ammo and if
  ///     either its parent or type idsz match the given idsz.  This
  ///     function returns the amount of ammo given.

  PChr_t chrlst      = gs->ChrList;

  int amount;
  OBJ_REF model;
  CAP_REF icap;

  amount = 0;
  if ( ACTIVE_CHR(chrlst, chr_ref) )
  {
    model = chrlst[chr_ref].model;
    icap = gs->ObjList[model].cap;
    if ( CAP_INHERIT_IDSZ( gs, icap, idsz ) )
    {
      if ( chrlst[chr_ref].ammo < chrlst[chr_ref].ammomax )
      {
        amount = chrlst[chr_ref].ammomax - chrlst[chr_ref].ammo;
        chrlst[chr_ref].ammo = chrlst[chr_ref].ammomax;
      }
    }
  }
  return amount;
}


//--------------------------------------------------------------------------------------------
void issue_clean( Game_t * gs, CHR_REF chr_ref )
{
  /// @details ZZ@> This function issues a clean up order to all teammates

  TEAM_REF team;
  CHR_REF chr_cnt;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  team = chrlst[chr_ref].team;

  for ( chr_cnt = 0; chr_cnt < chrlst_size; chr_cnt++ )
  {
    if ( chrlst[chr_cnt].team == team && !chrlst[chr_cnt].alive )
    {
      chrlst[chr_cnt].aistate.time  = 2;  // Don't let it think too much...
      chrlst[chr_cnt].aistate.alert = ALERT_CLEANEDUP;
    }
  }
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Team_t * Team_new(Team_t *pteam)
{
  //fprintf( stdout, "Team_new()\n");

  if(NULL ==pteam) return pteam;

  Team_delete( pteam );

  memset(pteam, 0, sizeof(Team_t));

  EKEY_PNEW( pteam, Team_t );


  return pteam;
}

//--------------------------------------------------------------------------------------------
bool_t Team_delete(Team_t *pteam)
{
  if(NULL ==pteam) return bfalse;
  if(!EKEY_PVALID( pteam )) return btrue;

  EKEY_PINVALIDATE(pteam);

  memset(pteam, 0, sizeof(Team_t));

  return btrue;
}

//--------------------------------------------------------------------------------------------
Team_t * Team_renew(Team_t *pteam)
{
  Team_delete(pteam);
  return Team_new(pteam);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ObjList_free_one( Game_t * gs, OBJ_REF obj_ref )
{
  /// @details BB@> This function sticks a profile back on the free profile stack

  PCap_t    caplst      = gs->CapList;

  Obj_t * pobj;

  pobj = ObjList_getPObj(gs, obj_ref);
  if(NULL == pobj) return;

  log_debug( "ObjList_free_one() - \n\tprofile == %d, caplst[pobj->cap].classname == \"%s\"\n", obj_ref, caplst[pobj->cap].classname );

  // deallocate the character
  CProfile_delete(pobj);

  // add it to the free list
  gs->ObjFreeList[gs->ObjFreeList_count] = obj_ref;
  gs->ObjFreeList_count++;
}


Profile_t * CProfile_new(Profile_t * p)
{
  int cnt;

  if(NULL == p) return p;

  CProfile_delete(p);

  memset(p, 0, sizeof(Profile_t));

  EKEY_PNEW(p, Profile_t);

  strcpy(p->name, "*NONE*");

  p->eve = INVALID_EVE;
  p->cap = INVALID_CAP;
  p->mad = INVALID_MAD;
  p->ai  = INVALID_AI;                              // AI for this model

  for(cnt=0; cnt<PRTPIP_PEROBJECT_COUNT; cnt++)
  {
    p->prtpip[cnt] = INVALID_PIP;
  }

  return p;
}

bool_t CProfile_delete(Profile_t * p)
{
  if(NULL == p) return bfalse;
  if( !EKEY_PVALID(p) ) return btrue;

  EKEY_PINVALIDATE(p);

  return btrue;
}

Profile_t * CProfile_renew(Profile_t * p)
{
  CProfile_delete(p);
  return CProfile_new(p);
}

//--------------------------------------------------------------------------------------------
void obj_clear_pips( Game_t * gs )
{
  int cnt;
  OBJ_REF object;

  // Now clear out the local pips
  for ( object = 0; object < OBJLST_COUNT; object++ )
  {
    for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
    {
      gs->ObjList[object].prtpip[cnt] = INVALID_PIP;
    }
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Obj_t * ObjList_getPObj(Game_t * gs, OBJ_REF iobj)
{
  if ( !VALID_OBJ( gs->ObjList, iobj ) ) return NULL;

  return gs->ObjList + iobj;
}

//--------------------------------------------------------------------------------------------
Eve_t * ObjList_getPEve(Game_t * gs, OBJ_REF iobj)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return NULL;

  pobj->eve = VALIDATE_EVE(gs->EveList, pobj->eve);
  if(!VALID_EVE(gs->EveList, pobj->eve)) return NULL;

  return gs->EveList + pobj->eve;
}

//--------------------------------------------------------------------------------------------
Cap_t * ObjList_getPCap(Game_t * gs, OBJ_REF iobj)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return NULL;

  pobj->cap = VALIDATE_CAP(gs->CapList, pobj->cap);
  if(!VALID_CAP(gs->CapList, pobj->cap)) return NULL;

  return gs->CapList + pobj->cap;
}

//--------------------------------------------------------------------------------------------
Mad_t * ObjList_getPMad(Game_t * gs, OBJ_REF iobj)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return NULL;

  pobj->mad = VALIDATE_MAD(gs->MadList, pobj->mad);
  if(!VALID_MAD(gs->MadList, pobj->mad)) return NULL;

  return gs->MadList + pobj->mad;
}

//--------------------------------------------------------------------------------------------
Pip_t * ObjList_getPPip(Game_t * gs, OBJ_REF iobj, int i)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return NULL;

  pobj->prtpip[i] = VALIDATE_PIP(gs->PipList, pobj->prtpip[i]);
  if(!VALID_PIP(gs->PipList, pobj->prtpip[i])) return NULL;

  return gs->PipList + pobj->prtpip[i];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
EVE_REF ObjList_getREve(Game_t * gs, OBJ_REF iobj)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return INVALID_EVE;

  pobj->eve = VALIDATE_EVE(gs->EveList, pobj->eve);
  return pobj->eve;
}

//--------------------------------------------------------------------------------------------
CAP_REF ObjList_getRCap(Game_t * gs, OBJ_REF iobj)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return INVALID_CAP;

  pobj->cap = VALIDATE_CAP(gs->CapList, pobj->cap);
  return pobj->cap;
}

//--------------------------------------------------------------------------------------------
MAD_REF ObjList_getRMad(Game_t * gs, OBJ_REF iobj)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return INVALID_MAD;

  pobj->mad = VALIDATE_MAD(gs->MadList, pobj->mad);
  return pobj->mad;
}

//--------------------------------------------------------------------------------------------
PIP_REF ObjList_getRPip(Game_t * gs, OBJ_REF iobj, int i)
{
  Obj_t * pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj) return INVALID_PIP;

  pobj->prtpip[i] = VALIDATE_PIP(gs->PipList, pobj->prtpip[i]);

  return pobj->prtpip[i];

}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ObjList_log_used( Game_t * gs, char *savename )
{
  /// @details ZZ@> This is a debug function for checking model loads

  FILE* hFileWrite;
  OBJ_REF obj_cnt;

  hFileWrite = fs_fileOpen( PRI_NONE, NULL, savename, "a" );
  if ( NULL != hFileWrite )
  {
    fprintf( hFileWrite, "\n\nSlot usage for objects in last module loaded...\n" );
    fprintf( hFileWrite, "-----------------------------------------------\n" );


    for ( obj_cnt = 0; obj_cnt < OBJLST_COUNT; obj_cnt++ )
    {
      Cap_t * pcap;
      Mad_t * pmad;
      if( !gs->ObjList[obj_cnt].Active ) continue;

      pcap = ObjList_getPCap(gs, obj_cnt);
      pmad = ObjList_getPMad(gs, obj_cnt);

      if(NULL != pcap && NULL != pmad)
      {
        fprintf( hFileWrite, "%3d %32s %s\n", REF_TO_INT(obj_cnt), pcap->classname, pmad->name );
      }
      else if(NULL != pcap)
      {
        fprintf( hFileWrite, "%3d %32s ?\n", REF_TO_INT(obj_cnt), pcap->classname );
      }
      else if(NULL != pmad)
      {
        fprintf( hFileWrite, "%3d ? %s\n", REF_TO_INT(obj_cnt), pmad->name );
      }
    }

    fprintf( hFileWrite, "\n\nDebug info after initial spawning and loading is complete...\n" );
    fprintf( hFileWrite, "-----------------------------------------------\n" );

    fs_fileClose( hFileWrite );
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t tile_damage_reset(TILE_DAMAGE * t)
{
  if(NULL == t) return bfalse;

  t->parttype = INVALID_PIP;
  t->partand  = 0xFF;
  t->sound    = INVALID_SOUND;
  t->amount   = INT_TO_FP8(1);                    // Amount of damage
  t->type     = DAMAGE_FIRE;                      // Type of damage

  return btrue;
}
