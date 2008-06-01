//********************************************************************************************
//* Egoboo - object.c
//*
//* Implements the object manager
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

#include "object.inl"
#include "graphic.h"
#include "particle.h"
#include "char.h"
#include "mad.h"
#include "enchant.h"
#include "game.h"

#include "egoboo_utility.h"
#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
int load_one_object( GameState * gs, int skin_count, char * szObjectpath, char* szObjectname )
{
  // ZZ> This function loads one object and returns the number of skins

  Uint16 iobj;
  int numskins, TxIcon_count, skin_index;
  STRING newloadname, loc_loadpath, wavename;
  int cnt;

  // generate an index for this object
  strcpy(newloadname, szObjectpath);
  str_append_slash(newloadname, sizeof(newloadname));
  if(NULL != szObjectname)
  {
    strcat(newloadname, szObjectname);
    str_append_slash(newloadname, sizeof(newloadname));
  }
  strcat(newloadname, CData.data_file);

  iobj = object_generate_index(gs, newloadname);
  if(MAXMODEL == iobj)
  {
    // could not find the object
    return 0;
  }

  // Append a slash to the szObjectname
  strncpy( loc_loadpath, szObjectpath, sizeof( loc_loadpath ) );
  str_append_slash( loc_loadpath, sizeof( loc_loadpath ) );
  strncat( loc_loadpath, szObjectname, sizeof( loc_loadpath ) );
  str_append_slash( loc_loadpath, sizeof( loc_loadpath ) );

  // Load the iobj data file
  CapList_load_one( gs, szObjectpath, szObjectname, iobj );

  // load the model data
  MadList_load_one( gs, szObjectpath, szObjectname, iobj );

  // Fix lighting if need be
  if ( gs->CapList[iobj].uniformlit )
  {
    make_mad_equally_lit( gs, iobj );
  }

  // Load the messages for this object
  load_all_messages( gs, szObjectpath, szObjectname, iobj );

  // Load the random naming table for this object
  read_naming( gs, szObjectpath, szObjectname, iobj );

  // Load the particles for this object
  for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
  {
    snprintf( newloadname, sizeof( newloadname ), "part%d.txt", cnt );
    gs->MadList[iobj].prtpip[cnt] = PipList_load_one( gs, szObjectpath, szObjectname, newloadname, -1 );
  }

  // Load the waves for this object
  for ( cnt = 0; cnt < MAXWAVE; cnt++ )
  {
    snprintf( wavename, sizeof( wavename ), "sound%d.wav", cnt );
    gs->CapList[iobj].wavelist[cnt] = Mix_LoadWAV( inherit_fname(szObjectpath, szObjectname, wavename) );
  }

  // Load the enchantment for this object
  EveList_load_one( gs, szObjectpath, szObjectname, iobj );

  // Load the skins and icons
  gs->MadList[iobj].skinstart = skin_count;
  numskins = 0;
  TxIcon_count = 0;
  for ( skin_index = 0; skin_index < MAXSKIN; skin_index++ )
  {
    STRING fname;
    Uint32 tx_index;

    // try various file types
    snprintf( fname, sizeof( newloadname ), "tris%d.png", skin_index );
    tx_index = GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + skin_count+numskins, inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );

    if(INVALID_TEXTURE == tx_index)
    {
      snprintf( fname, sizeof( newloadname ), "tris%d.bmp", skin_index );
      tx_index = GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + skin_count+numskins, inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );
    }

    if(INVALID_TEXTURE == tx_index)
    {
      snprintf( fname, sizeof( newloadname ), "tris%d.pcx", skin_index );
      tx_index = GLTexture_Load( GL_TEXTURE_2D,  gs->TxTexture + skin_count + numskins, inherit_fname(szObjectpath, szObjectname, fname), TRANSCOLOR );
    }

    if ( INVALID_TEXTURE != tx_index )
    {
      numskins++;

      snprintf( fname, sizeof( newloadname ), "icon%d.png", skin_index );
      tx_index = GLTexture_Load( GL_TEXTURE_2D,  gs->TxIcon + gs->TxIcon_count, inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );

      if ( INVALID_TEXTURE == tx_index )
      {
        snprintf( fname, sizeof( newloadname ), "icon%d.bmp", skin_index );
        tx_index = GLTexture_Load( GL_TEXTURE_2D,  gs->TxIcon + gs->TxIcon_count, inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );
      }

      if ( INVALID_TEXTURE == tx_index )
      {
        snprintf( fname, sizeof( newloadname ), "icon%d.pcx", skin_index );
        tx_index = GLTexture_Load( GL_TEXTURE_2D,  gs->TxIcon + gs->TxIcon_count, inherit_fname(szObjectpath, szObjectname, fname), INVALID_KEY );
      }

      if( INVALID_TEXTURE != tx_index )
      {
        if ( iobj == SPELLBOOK && gs->bookicon == 0 )
        {
          gs->bookicon = gs->TxIcon_count;
        }

        while ( TxIcon_count < numskins )
        {
          gs->skintoicon[skin_count+TxIcon_count] = gs->TxIcon_count;
          TxIcon_count++;
        }

        gs->TxIcon_count++;
      }
    }
  }

  if ( numskins == 0 )
  {
    // If we didn't get a skin_count, set it to the water texture
    numskins = 1;
    gs->MadList[iobj].skinstart = TX_WATER_TOP;
  }
  gs->MadList[iobj].skins = numskins;

  return numskins;
}

//--------------------------------------------------------------------------------------------
void switch_team( GameState * gs, CHR_REF chr_ref, TEAM team )
{
  // ZZ> This function makes a chr_ref join another team...

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  if ( team < TEAM_COUNT )
  {
    if ( !chrlst[chr_ref].invictus )
    {
      gs->TeamList[chrlst[chr_ref].baseteam].morale--;
      gs->TeamList[team].morale++;
    }
    if (( !chrlst[chr_ref].ismount || !chr_using_slot( chrlst, MAXCHR, chr_ref, SLOT_SADDLE ) ) &&
        ( !chrlst[chr_ref].isitem  || !chr_attached( chrlst, MAXCHR, chr_ref ) ) )
    {
      chrlst[chr_ref].team = team;
    }

    chrlst[chr_ref].baseteam = team;
    if ( !VALID_CHR( chrlst, team_get_leader( gs, team ) ) )
    {
      gs->TeamList[team].leader = chr_ref;
    }
  }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( GameState * gs, CHR_REF chr_ref, IDSZ idsz )
{
  // ZZ> This function restocks the characters ammo, if it needs ammo and if
  //     either its parent or type idsz match the given idsz.  This
  //     function returns the amount of ammo given.

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;
  int amount, model;

  amount = 0;
  if ( VALID_CHR(chrlst, chr_ref) )
  {
    model = chrlst[chr_ref].model;
    if ( CAP_INHERIT_IDSZ( gs,  model, idsz ) )
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
void issue_clean( GameState * gs, CHR_REF chr_ref )
{
  // ZZ> This function issues a clean up order to all teammates

  TEAM team;
  Uint16 cnt;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  team = chrlst[chr_ref].team;
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( chrlst[cnt].team == team && !chrlst[cnt].alive )
    {
      chrlst[cnt].aistate.time = 2;  // Don't let it think too much...
      chrlst[cnt].aistate.alert = ALERT_CLEANEDUP;
    }
    cnt++;
  }
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
TeamInfo * TeamInfo_new(TeamInfo *pteam) 
{ 
  fprintf( stdout, "TeamInfo_new()\n");

  if(NULL==pteam) return pteam; 
  
  memset(pteam, 0, sizeof(TeamInfo)); 
  
  return pteam; 
};

//--------------------------------------------------------------------------------------------
bool_t TeamInfo_delete(TeamInfo *pteam) 
{ 
  if(NULL==pteam) return bfalse; 

  memset(pteam, 0, sizeof(TeamInfo)); 
   
  return btrue; 
};

//--------------------------------------------------------------------------------------------
TeamInfo * TeamInfo_renew(TeamInfo *pteam) 
{ 
  TeamInfo_delete(pteam);
  return TeamInfo_new(pteam);
};