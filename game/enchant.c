//********************************************************************************************
//* Egoboo - enchant.c
//*
//* 
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

#include "enchant.h"
#include "Log.h"
#include "passage.h"
#include "script.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>

#include "particle.inl"
#include "Md2.inl"
#include "char.inl"
#include "mesh.inl"

//--------------------------------------------------------------------------------------------
void enc_spawn_particles( CGame * gs, float dUpdate )
{
  // ZZ> This function lets enchantments spawn particles

  ENC_REF enc_cnt;
  int tnc;
  Uint16 facing;
  CHR_REF target_ref;

  PObj objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  OBJ_REF iobj;
  CObj  * pobj;

  CEve  * peve;

  for ( enc_cnt = 0; enc_cnt < enclst_size; enc_cnt++ )
  {
    prt_spawn_info si;
    CEnc * penc = enclst + enc_cnt;
    if ( !penc->active ) continue;

    iobj = penc->profile = VALIDATE_OBJ(gs->ObjList, penc->profile);
    if( INVALID_OBJ == iobj ) continue;
    pobj = objlst + iobj;

    peve = ObjList_getPEve(gs, iobj);
    if(NULL == peve) continue;

    if ( peve->contspawnamount <= 0 ) continue;

    penc->spawntime -= dUpdate;
    if ( penc->spawntime <= 0 ) penc->spawntime = 0;
    if ( penc->spawntime > 0 ) continue;

    target_ref = penc->target = VALIDATE_CHR(chrlst, penc->target);
    if( !ACTIVE_CHR(chrlst, target_ref) ) continue;

    penc->spawntime = peve->contspawntime;
    facing = chrlst[target_ref].ori.turn_lr;
    for ( tnc = 0; tnc < peve->contspawnamount; tnc++)
    {
      prt_spawn_info_init( &si, gs, 1.0f, chrlst[target_ref].ori.pos, facing, iobj, peve->contspawnpip,
        INVALID_CHR, GRIP_LAST, chrlst[penc->owner].team, penc->owner, tnc, INVALID_CHR );

      req_spawn_one_particle( si );

      facing += peve->contspawnfacingadd;
    }

  }
}


//--------------------------------------------------------------------------------------------
void disenchant_character( CGame * gs, CHR_REF cnt )
{
  // ZZ> This function removes all enchantments from a character

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  while ( INVALID_ENC != chrlst[cnt].firstenchant )
  {
    remove_enchant( gs, chrlst[cnt].firstenchant );
  }
}

//--------------------------------------------------------------------------------------------
void spawn_poof( CGame * gs, CHR_REF character, OBJ_REF profile )
{
  // ZZ> This function spawns a character poof

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  prt_spawn_info si;
  Uint16 sTmp;
  CHR_REF origin;
  int iTmp;
  CChr * pchr;
  CCap * pcap = ObjList_getPCap(gs, profile);

  sTmp = chrlst[character].ori.turn_lr;
  iTmp = 0;
  origin = chr_get_aiowner( chrlst, CHRLST_COUNT, chrlst + character );
  pchr = ChrList_getPChr(gs, origin);
  while ( iTmp < pcap->gopoofprtamount )
  {
    prt_spawn_info_init( &si, gs, 1.0f, pchr->ori_old.pos,
                        sTmp, profile, pcap->gopoofprttype,
                        INVALID_CHR, GRIP_LAST, pchr->team, origin, iTmp, INVALID_CHR );

    req_spawn_one_particle( si );

    sTmp += pcap->gopoofprtfacingadd;
    iTmp++;
  }
}

//--------------------------------------------------------------------------------------------
char * naming_generate( CGame * gs, CObj * pobj )
{
  // ZZ> This function generates a random name

  static STRING name; // The name returned by the function
  Uint32 loc_rand;

  size_t  objlst_size = OBJLST_COUNT;

  int read, write, section, mychop;
  char cTmp;

  strcpy(name, "Blah");

  if ( 0 == pobj->sectionsize[0] ) return name;

  loc_rand = gs->randie_index;
  write = 0;
  section = 0;
  while ( section < MAXSECTION )
  {
    if ( pobj->sectionsize[section] != 0 )
    {
      mychop = pobj->sectionstart[section] + RAND(&loc_rand, 0, pobj->sectionsize[section]);
      read = gs->chop.start[mychop];
      cTmp = gs->chop.text[read];
      while ( cTmp != 0 && write < MAXCAPNAMESIZE - 1 )
      {
        name[write] = cTmp;
        write++;
        read++;
        cTmp = gs->chop.text[read];
      }
    }
    section++;
  }
  if ( write >= MAXCAPNAMESIZE ) write = MAXCAPNAMESIZE - 1;
  name[write] = 0;

  return name;
}

//--------------------------------------------------------------------------------------------
void naming_read( CGame * gs, const char * szModpath, const char * szObjectname, CObj * pobj)
{
  // ZZ> This function reads a naming file

  PObj objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;
  ChopData * pchop;

  FILE *fileread;
  int section, chopinsection, cnt;
  char mychop[32], cTmp;

  pchop = &(gs->chop);

  fileread = fs_fileOpen( PRI_NONE, NULL, inherit_fname(szModpath, szObjectname, CData.naming_file), "r" );
  if ( NULL == fileread ) return;

  section = 0;
  chopinsection = 0;
  while ( fget_next_string( fileread, mychop, sizeof( mychop ) ) && section < MAXSECTION )
  {
    if ( 0 != strcmp( mychop, "STOP" ) )
    {
      if ( pchop->write >= CHOPDATACHUNK )  pchop->write = CHOPDATACHUNK - 1;
      pchop->start[pchop->count] = pchop->write;

      cnt = 0;
      cTmp = mychop[0];
      while ( cTmp != 0 && cnt < 31 && pchop->write < CHOPDATACHUNK )
      {
        if ( cTmp == '_' ) cTmp = ' ';
        pchop->text[pchop->write] = cTmp;
        cnt++;
        pchop->write++;
        cTmp = mychop[cnt];
      }

      if ( pchop->write >= CHOPDATACHUNK )  pchop->write = CHOPDATACHUNK - 1;
      pchop->text[pchop->write] = 0;  pchop->write++;
      chopinsection++;
      pchop->count++;
    }
    else
    {
      pobj->sectionsize[section]  = chopinsection;
      pobj->sectionstart[section] = pchop->count - chopinsection;
      section++;
      chopinsection = 0;
    }
  }
  fs_fileClose( fileread );

}

//--------------------------------------------------------------------------------------------
void naming_prime( CGame * gs )
{
  // ZZ> This function prepares the name chopper for use

  PObj objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  OBJ_REF obj_cnt;
  int tnc;

  gs->chop.count = 0;
  gs->chop.write = 0;
  for ( obj_cnt = 0; obj_cnt < OBJLST_COUNT; obj_cnt++ )
  {
    for( tnc = 0; tnc < MAXSECTION; tnc++ )
    {
      objlst[obj_cnt].sectionstart[tnc] = MAXCHOP;
      objlst[obj_cnt].sectionsize[tnc]  = 0;
    }
  }
}

//--------------------------------------------------------------------------------------------
void tilt_character(CGame * gs, CHR_REF ichr)
{
  CChr * pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return;

  if ( pchr->prop.stickybutt && INVALID_FAN != pchr->onwhichfan)
  {
    Uint8 twist = mesh_get_twist( gs->Mesh_Mem.fanlst, pchr->onwhichfan );
    pchr->ori.mapturn_lr = twist_table[twist].lr;
    pchr->ori.mapturn_ud = twist_table[twist].ud;
  }
}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain(CGame * gs)
{
  // ZZ> This function sets all of the character's starting tilt values

  CHR_REF chr_cnt;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  
  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    if( !VALID_CHR(chrlst, chr_cnt) ) continue;

    tilt_character(gs, chr_cnt);
  }
}


//--------------------------------------------------------------------------------------------
Uint16 change_armor( CGame * gs, CHR_REF character, Uint16 iskin )
{
  // ZZ> This function changes the armor of the character

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PCap caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  ENC_REF enchant;
  OBJ_REF sTmp;
  int iTmp, cnt;
  CCap * pcap;

  // Remove armor enchantments
  enchant = chrlst[character].firstenchant;
  while ( INVALID_ENC != enchant )
  {
    for ( cnt = SETSLASHMODIFIER; cnt <= SETZAPMODIFIER; cnt++ )
    {
      unset_enchant_value( gs, enchant, cnt );
    }
    enchant = enclst[enchant].nextenchant;
  }


  // Change the skin

  sTmp = chrlst[character].model;
  if ( iskin > gs->ObjList[sTmp].skins )  iskin = 0;
  chrlst[character].skin_ref  = iskin;


  // Change stats associated with skin
  pcap = ChrList_getPCap(gs, character);
  chrlst[character].skin.defense_fp8 = pcap->skin[iskin].defense_fp8;
  iTmp = 0;
  while ( iTmp < MAXDAMAGETYPE )
  {
    chrlst[character].skin.damagemodifier_fp8[iTmp] = pcap->skin[iskin].damagemodifier_fp8[iTmp];
    iTmp++;
  }
  chrlst[character].skin.maxaccel = pcap->skin[iskin].maxaccel;


  // Reset armor enchantments
  // These should really be done in reverse order ( Start with last enchant ), but
  // I don't care at this point !!!BAD!!!
  enchant = chrlst[character].firstenchant;
  while ( INVALID_ENC != enchant )
  {
    for ( cnt = SETSLASHMODIFIER; cnt <= SETZAPMODIFIER; cnt++ )
    {
      set_enchant_value( gs, enchant, cnt, enclst[enchant].eve );
    };
    add_enchant_value( gs, enchant, ADDACCEL, enclst[enchant].eve );
    add_enchant_value( gs, enchant, ADDDEFENSE, enclst[enchant].eve );
    enchant = enclst[enchant].nextenchant;
  }

  return iskin;
}

//--------------------------------------------------------------------------------------------
void change_character( CGame * gs, CHR_REF ichr, OBJ_REF new_profile, Uint8 new_skin, Uint8 leavewhich )
{
  // ZZ> This function polymorphs a character, changing stats, dropping weapons

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PObj objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PMad madlst      = gs->MadList;
  size_t madlst_size = MADLST_COUNT;

  PCap caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  int tnc;
  ENC_REF enchant;
  CHR_REF sTmp, item, imount;
  CChr * pchr;
  CObj * pobj;
  CMad * pmad;
  CCap * pcap;

  if( !ACTIVE_CHR( chrlst, ichr) ) return;
  pchr = ChrList_getPChr(gs, ichr);

  pobj = ObjList_getPObj(gs, new_profile);
  if ( NULL == pobj ) return;
  
  pmad = ObjList_getPMad(gs, new_profile);
  if( NULL == pmad ) return;

  pcap = ObjList_getPCap(gs, new_profile);
  if( NULL == pcap ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    sTmp = chr_get_holdingwhich( chrlst, CHRLST_COUNT, ichr, _slot );
    if ( !pcap->slotvalid[_slot] )
    {
      if ( detach_character_from_mount( gs, sTmp, btrue, btrue ) )
      {
        if ( _slot == SLOT_SADDLE )
        {
          chrlst[sTmp].accum.vel.z += DISMOUNTZVEL;
          chrlst[sTmp].accum.pos.z += DISMOUNTZVEL;
          chrlst[sTmp].jumptime  = DELAY_JUMP;
        };
      }
    }
  }

  // Remove particles
  disaffirm_attached_particles( gs, ichr );

  switch ( leavewhich )
  {
    case LEAVE_FIRST:
      {
        // Remove all enchantments except top one
        enchant = pchr->firstenchant;
        if ( INVALID_ENC != enchant )
        {
          while ( INVALID_ENC != enclst[enchant].nextenchant )
          {
            remove_enchant( gs, enclst[enchant].nextenchant );
          }
        }
      }
      break;

    case LEAVE_NONE:
      {
        // Remove all enchantments
        disenchant_character( gs, ichr );
      }
      break;
  }

  // Stuff that must be set
  pchr->model     = new_profile;
  VData_Blended_destruct( &(pchr->vdata) );
  VData_Blended_construct( &(pchr->vdata) );
  VData_Blended_Allocate( &(pchr->vdata), md2_get_numVertices(pmad->md2_ptr) );

  pchr->stoppedby = pcap->stoppedby;
  pchr->stats.lifeheal_fp8  = pcap->statdata.lifeheal_fp8;
  pchr->manacost_fp8  = pcap->manacost_fp8;

  // Ammo
  pchr->ammomax = pcap->ammomax;
  pchr->ammo = pcap->ammo;
  // Gender
  if ( pcap->gender != GEN_RANDOM ) // GEN_RANDOM means keep old gender
  {
    pchr->gender = pcap->gender;
  }

  // AI stuff
  pchr->aistate.type = pobj->ai;
  pchr->aistate.state = 0;
  pchr->aistate.time = 0;
  CLatch_clear( &(pchr->aistate.latch) );
  pchr->aistate.turnmode = TURNMODE_VELOCITY;

  // Flags
  pchr->prop.stickybutt = pcap->prop.stickybutt;
  pchr->prop.canopenstuff = pcap->prop.canopenstuff;
  pchr->transferblend = pcap->transferblend;
  pchr->enviro = pcap->enviro;
  pchr->prop.isplatform = pcap->prop.isplatform;
  pchr->prop.isitem = pcap->prop.isitem;
  pchr->prop.invictus = pcap->prop.invictus;
  pchr->prop.ismount = pcap->prop.ismount;
  pchr->prop.cangrabmoney = pcap->prop.cangrabmoney;
  pchr->jumptime = DELAY_JUMP;

  // Character size and bumping
  pchr->bmpdata_save.shadow  = pcap->shadowsize;
  pchr->bmpdata_save.size    = pcap->bumpsize;
  pchr->bmpdata_save.sizebig = pcap->bumpsizebig;
  pchr->bmpdata_save.height  = pcap->bumpheight;

  pchr->bmpdata.shadow   = pcap->shadowsize * pchr->fat;
  pchr->bmpdata.size     = pcap->bumpsize * pchr->fat;
  pchr->bmpdata.sizebig  = pcap->bumpsizebig * pchr->fat;
  pchr->bmpdata.height   = pcap->bumpheight * pchr->fat;
  pchr->bumpstrength     = pcap->bumpstrength * FP8_TO_FLOAT( pcap->alpha_fp8 );

  pchr->bumpdampen = pcap->bumpdampen;
  pchr->weight = pcap->weight * pchr->fat * pchr->fat * pchr->fat;     // preserve density
  pchr->scale = pchr->fat;


  // Character scales...  Magic numbers
  imount = chr_get_attachedto( chrlst, CHRLST_COUNT, ichr );
  if ( ACTIVE_CHR( chrlst,  imount ) )
  {
    CMad * tmppmad;
    Uint16 vrtoffset = slot_to_offset( pchr->inwhichslot );

    tmppmad = ChrList_getPMad(gs, imount);


    if( NULL == pmad )
    {
      pchr->attachedgrip[0] = 0;
      pchr->attachedgrip[1] = 0xFFFF;
      pchr->attachedgrip[2] = 0xFFFF;
      pchr->attachedgrip[3] = 0xFFFF;
    }
    else if ( tmppmad->vertices > vrtoffset && vrtoffset > 0 )
    {
      tnc = tmppmad->vertices - vrtoffset;
      pchr->attachedgrip[0] = tnc + 0;
      pchr->attachedgrip[1] = tnc + 1;
      pchr->attachedgrip[2] = tnc + 2;
      pchr->attachedgrip[3] = tnc + 3;
    }
    else
    {
      pchr->attachedgrip[0] = tmppmad->vertices - 1;
      pchr->attachedgrip[1] = 0xFFFF;
      pchr->attachedgrip[2] = 0xFFFF;
      pchr->attachedgrip[3] = 0xFFFF;
    }
  }

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    item = chr_get_holdingwhich( chrlst, CHRLST_COUNT, ichr, _slot );
    if ( ACTIVE_CHR( chrlst,  item ) )
    {
      int i, grip_offset;
      int vrtcount = pmad->vertices;

      grip_offset = slot_to_grip( _slot );

      if(vrtcount >= grip_offset + GRIP_SIZE)
      {
        // valid grip
        grip_offset = vrtcount - grip_offset;
        for(i=0; i<GRIP_SIZE; i++)
        {
          chrlst[item].attachedgrip[i] = grip_offset + i;
        };
      }
      else
      {
        // invalid grip.
        for(i=0; i<GRIP_SIZE; i++)
        {
          chrlst[item].attachedgrip[i] = 0xFFFF;
        };
      }
    }
  }

  // Image rendering
  pchr->uoffset_fp8 = 0;
  pchr->voffset_fp8 = 0;
  pchr->uoffvel = pcap->uoffvel;
  pchr->voffvel = pcap->voffvel;

  // Movement
  pchr->spd_sneak = pcap->spd_sneak;
  pchr->spd_walk  = pcap->spd_walk;
  pchr->spd_run   = pcap->spd_run;

  // AI and action stuff
  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  pchr->holdingweight = 0;
  pchr->onwhichplatform = INVALID_CHR;

  // Set the new_skin
  change_armor( gs, ichr, new_skin );

  // Reaffirm them particles...
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
  reaffirm_attached_particles( gs, ichr );

  // calculate the matrix
  make_one_character_matrix(chrlst, CHRLST_COUNT, chrlst + ichr);

  // calculate a basic bounding box
  chr_calculate_bumpers(gs, chrlst + ichr, 0);
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( CGame * gs, CHR_REF chr_ref, int amount, Uint16 killer )
{
  // ZZ> This function takes mana from a character ( or gives mana ),
  //     and returns btrue if the character had enough to pay, or bfalse
  //     otherwise

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  int iTmp;

  iTmp = chrlst[chr_ref].stats.mana_fp8 - amount;
  if ( iTmp < 0 )
  {
    chrlst[chr_ref].stats.mana_fp8 = 0;
    if ( chrlst[chr_ref].prop.canchannel )
    {
      chrlst[chr_ref].stats.life_fp8 += iTmp;
      if ( chrlst[chr_ref].stats.life_fp8 <= 0 )
      {
        kill_character( gs, chr_ref, chr_ref );
      }
      return btrue;
    }
    return bfalse;
  }
  else
  {
    chrlst[chr_ref].stats.mana_fp8 = iTmp;
    if ( iTmp > chrlst[chr_ref].stats.manamax_fp8 )
    {
      chrlst[chr_ref].stats.mana_fp8 = chrlst[chr_ref].stats.manamax_fp8;
    }
  }
  return btrue;
}

//--------------------------------------------------------------------------------------------
CEnc * CEnc_new(CEnc *penc) 
{ 
  //fprintf( stdout, "CEnc_new()\n");

  if(NULL ==penc) return penc; 

  CEnc_delete( penc );

  memset(penc, 0, sizeof(CEnc));

  EKEY_PNEW( penc, CEnc );

  penc->active = bfalse;

  return penc; 
};

//--------------------------------------------------------------------------------------------
bool_t CEnc_delete( CEnc * penc)
{
  if(NULL == penc) return bfalse;
  if(!EKEY_PVALID(penc))  return btrue;

  EKEY_PINVALIDATE( penc );

  penc->active = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
CEnc * CEnc_renew( CEnc * penc )
{
  CEnc_delete( penc );
  return CEnc_new( penc );
}

//--------------------------------------------------------------------------------------------
Uint32 fget_damage_modifier( FILE * fileread )
{
  Uint32 iTmp = 0, tTmp;
  char cTmp;
  if ( NULL == fileread || feof( fileread ) ) return iTmp;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'F': iTmp = 0;             break;
    case 'T': iTmp = DAMAGE_INVERT; break;
    case 'C': iTmp = DAMAGE_CHARGE; break;
    case 'M': iTmp = DAMAGE_MANA;   break;
  };
  tTmp = fget_int( fileread );

  return iTmp | tTmp;
};

//--------------------------------------------------------------------------------------------
EVE_REF EveList_load_one( CGame * gs, const char * szObjectpath, const char * szObjectname, EVE_REF irequest )
{
  // ZZ> This function loads the enchantment associated with an object

  FILE* fileread;
  char cTmp;
  int iTmp;
  int num;
  IDSZ idsz;

  PObj objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  EVE_REF  ieve;
  CEve   * peve;
  
  if(!VALID_EVE_RANGE(irequest)) return INVALID_EVE;
  ieve = irequest;

  // Make sure we don't load over an existing model
  if( LOADED_EVE(evelst, ieve) )
  {
    log_error( "Enchant template (eve) %i is already used. (%s%s)\n", ieve, szObjectpath, szObjectname );
  }

  peve = evelst + ieve;

  // initialize the model template
  Eve_new(peve);

  globalname = szObjectname;
  fileread = fs_fileOpen( PRI_NONE, NULL, inherit_fname(szObjectpath, szObjectname, CData.enchant_file), "r" );
  if ( NULL == fileread ) return INVALID_EVE;

  // btrue/bfalse values
  peve->retarget = fget_next_bool( fileread );
  peve->override = fget_next_bool( fileread );
  peve->removeoverridden = fget_next_bool( fileread );
  peve->killonend = fget_next_bool( fileread );
  peve->poofonend = fget_next_bool( fileread );

  // More stuff
  peve->time = fget_next_int( fileread );
  peve->endmessage = fget_next_int( fileread );
  // make -1 the "never-ending enchant" marker
  if ( peve->time == 0 ) peve->time = -1;

  // Drain stuff
  peve->ownermana_fp8  = fget_next_fixed( fileread );
  peve->targetmana_fp8 = fget_next_fixed( fileread );
  peve->endifcantpay   = fget_next_bool( fileread );
  peve->ownerlife_fp8  = fget_next_fixed( fileread );
  peve->targetlife_fp8 = fget_next_fixed( fileread );


  // Specifics
  peve->dontdamagetype = fget_next_damage( fileread );
  peve->onlydamagetype = fget_next_damage( fileread );
  peve->removedbyidsz  = fget_next_idsz( fileread );


  // Now the set values
  num = 0;
  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_damage_modifier( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_int( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_bool( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_bool( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  cTmp = fget_first_letter( fileread );
  peve->setvalue[num] = MIS_NORMAL;
  switch ( toupper( cTmp ) )
  {
    case 'R': peve->setvalue[num] = MIS_REFLECT; break;
    case 'D': peve->setvalue[num] = MIS_DEFLECT; break;
  };
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_float( fileread ) * 16;
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_bool( fileread );
  num++;

  peve->setyesno[num] = fget_next_bool( fileread );
  peve->setvalue[num] = fget_bool( fileread );
  num++;


  // Now read in the add values
  num = 0;
  peve->addvalue[num] = fget_next_float( fileread ) * 16;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 127;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 127;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 127;
  num++;

  peve->addvalue[num] = fget_next_int( fileread );
  num++;

  peve->addvalue[num] = fget_next_int( fileread );
  num++;

  peve->addvalue[num] = fget_next_int( fileread );
  num++;

  peve->addvalue[num] = fget_next_int( fileread );
  num++;

  peve->addvalue[num] = fget_next_int( fileread );
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 4;
  num++;

  peve->addvalue[num] = fget_next_float( fileread ) * 4;
  num++;


  // Clear expansions...
  peve->contspawntime = 0;
  peve->contspawnamount = 0;
  peve->contspawnfacingadd = 0;
  peve->contspawnpip = 0;
  peve->endsound = INVALID_SOUND;
  peve->stayifnoowner = 0;
  peve->overlay = 0;
  peve->canseekurse = bfalse;

  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );

    if ( MAKE_IDSZ( "AMOU" ) == idsz )  peve->contspawnamount = iTmp;
    else if ( MAKE_IDSZ( "TYPE" ) == idsz )  peve->contspawnpip = iTmp;
    else if ( MAKE_IDSZ( "TIME" ) == idsz )  peve->contspawntime = iTmp;
    else if ( MAKE_IDSZ( "FACE" ) == idsz )  peve->contspawnfacingadd = iTmp;
    else if ( MAKE_IDSZ( "SEND" ) == idsz )  peve->endsound = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "STAY" ) == idsz )  peve->stayifnoowner = INT_TO_BOOL(iTmp);
    else if ( MAKE_IDSZ( "OVER" ) == idsz )  peve->overlay = iTmp;
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  peve->canseekurse = INT_TO_BOOL(iTmp);
  }

  // All done ( finally )
  fs_fileClose( fileread );

  peve->Loaded = btrue;

  return ieve;
}

//--------------------------------------------------------------------------------------------
ENC_REF EncList_get_free( CGame * gs, ENC_REF irequest )
{
  // ZZ> This function returns the next free enchantment or ENCLST_COUNT if there are none

  int i;
  ENC_REF retval = INVALID_ENC;

  if(!EKEY_PVALID(gs)) return INVALID_ENC;

  if(INVALID_ENC == irequest)
  {
    // request is not specified. just give them the next one
    if(gs->EncFreeList_count>0)
    {
      gs->EncFreeList_count--;
      retval = gs->EncFreeList[gs->EncFreeList_count];
    }
  }
  else
  {
    // search for the requested reference in the free list
    for(i=0; i<gs->EncFreeList_count; i++)
    {
      if(gs->EncFreeList[i] == irequest)
      {
        // found it, so pop it off the list
        gs->EncFreeList[i] = gs->EncFreeList[gs->EncFreeList_count-1];
        gs->EncFreeList_count--;
      }
    }

    retval = irequest;
  }

  // initialize the data
  if(INVALID_ENC != retval)
  {
    CEnc_new(gs->EncList + retval);
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value( CGame * gs, ENC_REF enchantindex, Uint8 valueindex )
{
  // ZZ> This function unsets a set value

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CHR_REF character;

  if ( enclst[enchantindex].setyesno[valueindex] )
  {
    character = enclst[enchantindex].target;
    switch ( valueindex )
    {
      case SETDAMAGETYPE:
        chrlst[character].damagetargettype = (DAMAGE)enclst[enchantindex].setsave[valueindex];
        break;

      case SETNUMBEROFJUMPS:
        chrlst[character].jumpnumberreset = enclst[enchantindex].setsave[valueindex];
        break;

      case SETLIFEBARCOLOR:
        chrlst[character].lifecolor = enclst[enchantindex].setsave[valueindex];
        break;

      case SETMANABARCOLOR:
        chrlst[character].manacolor = enclst[enchantindex].setsave[valueindex];
        break;

      case SETSLASHMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_SLASH] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETCRUSHMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_CRUSH] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETPOKEMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_POKE] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETHOLYMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_HOLY] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETEVILMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_EVIL] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETFIREMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_FIRE] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETICEMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_ICE] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETZAPMODIFIER:
        chrlst[character].skin.damagemodifier_fp8[DAMAGE_ZAP] = enclst[enchantindex].setsave[valueindex];
        break;

      case SETFLASHINGAND:
        chrlst[character].flashand = enclst[enchantindex].setsave[valueindex];
        break;

      case SETLIGHTBLEND:
        chrlst[character].light_fp8 = enclst[enchantindex].setsave[valueindex];
        break;

      case SETALPHABLEND:
        chrlst[character].alpha_fp8 = enclst[enchantindex].setsave[valueindex];
        break;

      case SETSHEEN:
        chrlst[character].sheen_fp8 = enclst[enchantindex].setsave[valueindex];
        break;

      case SETFLYTOHEIGHT:
        chrlst[character].flyheight = enclst[enchantindex].setsave[valueindex];
        break;

      case SETWALKONWATER:
        chrlst[character].prop.waterwalk = INT_TO_BOOL(enclst[enchantindex].setsave[valueindex]);
        break;

      case SETCANSEEINVISIBLE:
        chrlst[character].prop.canseeinvisible = INT_TO_BOOL(enclst[enchantindex].setsave[valueindex]);
        break;

      case SETMISSILETREATMENT:
        chrlst[character].missiletreatment = (MISSLE_TYPE)enclst[enchantindex].setsave[valueindex];
        break;

      case SETCOSTFOREACHMISSILE:
        chrlst[character].missilecost = enclst[enchantindex].setsave[valueindex];
        chrlst[character].missilehandler = character;
        break;

      case SETMORPH:
        // Need special handler for when this is removed
        change_character( gs, character, chrlst[character].model_base, enclst[enchantindex].setsave[valueindex], LEAVE_ALL );
        chrlst[character].aistate.morphed = btrue;
        break;

      case SETCHANNEL:
        chrlst[character].prop.canchannel = INT_TO_BOOL(enclst[enchantindex].setsave[valueindex]);
        break;

    }
    enclst[enchantindex].setyesno[valueindex] = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value( CGame * gs, ENC_REF enchantindex, Uint8 valueindex )
{
  // ZZ> This function undoes cumulative modification to character stats

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  float fvaluetoadd;
  int valuetoadd;
  CHR_REF character;
  CChr * pchr;
  CEnc * penc;

  penc = enclst + enchantindex;
  character = penc->target;

  if( !ACTIVE_CHR( chrlst, character) ) return;
  pchr = ChrList_getPChr(gs, character);

  switch ( valueindex )
  {
    case ADDJUMPPOWER:
      fvaluetoadd = penc->addsave[valueindex] / 16.0;
      pchr->jump -= fvaluetoadd;
      break;

    case ADDBUMPDAMPEN:
      fvaluetoadd = penc->addsave[valueindex] / 128.0;
      pchr->bumpdampen -= fvaluetoadd;
      break;

    case ADDBOUNCINESS:
      fvaluetoadd = penc->addsave[valueindex] / 128.0;
      pchr->dampen -= fvaluetoadd;
      break;

    case ADDDAMAGE:
      valuetoadd = penc->addsave[valueindex];
      pchr->damageboost -= valuetoadd;
      break;

    case ADDSIZE:
      fvaluetoadd = penc->addsave[valueindex] / 128.0;
      pchr->sizegoto -= fvaluetoadd;
      pchr->sizegototime = DELAY_RESIZE;
      break;

    case ADDACCEL:
      fvaluetoadd = penc->addsave[valueindex] / 1000.0;
      pchr->skin.maxaccel -= fvaluetoadd;
      break;

    case ADDRED:
      valuetoadd = penc->addsave[valueindex];
      pchr->redshift -= valuetoadd;
      break;

    case ADDGRN:
      valuetoadd = penc->addsave[valueindex];
      pchr->grnshift -= valuetoadd;
      break;

    case ADDBLU:
      valuetoadd = penc->addsave[valueindex];
      pchr->blushift -= valuetoadd;
      break;

    case ADDDEFENSE:
      valuetoadd = penc->addsave[valueindex];
      pchr->skin.defense_fp8 -= valuetoadd;
      break;

    case ADDMANA:
      valuetoadd = penc->addsave[valueindex];
      pchr->stats.manamax_fp8 -= valuetoadd;
      pchr->stats.mana_fp8 -= valuetoadd;
      if ( pchr->stats.mana_fp8 < 0 ) pchr->stats.mana_fp8 = 0;
      break;

    case ADDLIFE:
      valuetoadd = penc->addsave[valueindex];
      pchr->stats.lifemax_fp8 -= valuetoadd;
      pchr->stats.life_fp8 -= valuetoadd;
      if ( pchr->stats.life_fp8 < 1 ) pchr->stats.life_fp8 = 1;
      break;

    case ADDSTRENGTH:
      valuetoadd = penc->addsave[valueindex];
      pchr->stats.strength_fp8 -= valuetoadd;
      break;

    case ADDWISDOM:
      valuetoadd = penc->addsave[valueindex];
      pchr->stats.wisdom_fp8 -= valuetoadd;
      break;

    case ADDINTELLIGENCE:
      valuetoadd = penc->addsave[valueindex];
      pchr->stats.intelligence_fp8 -= valuetoadd;
      break;

    case ADDDEXTERITY:
      valuetoadd = penc->addsave[valueindex];
      pchr->stats.dexterity_fp8 -= valuetoadd;
      break;

  }
}

//--------------------------------------------------------------------------------------------
CEve * Eve_new(CEve *peve) 
{ 
  //fprintf( stdout, "Eve_new()\n");

  if(NULL == peve) return peve; 

  Eve_delete(peve);

  memset(peve, 0, sizeof(CEve)); 

  EKEY_PNEW( peve, CEve );

  return peve; 
};

//--------------------------------------------------------------------------------------------
bool_t Eve_delete( CEve * peve )
{
  if(NULL == peve) return bfalse;
  if(!EKEY_PVALID(peve))  return btrue;

  EKEY_PINVALIDATE( peve );

  peve->Loaded = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
CEve * Eve_renew( CEve * peve )
{
  Eve_delete( peve );
  return Eve_new( peve );
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( CGame * gs, CHR_REF character )
{
  // ZZ> This function fixes an item's transparency

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PCap caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  ENC_REF enchant;
  CHR_REF mount;
  CChr * pchr;
  CCap * pcap;

  if ( !ACTIVE_CHR( chrlst,  character ) ) return;
  pchr = ChrList_getPChr(gs, character);
  pcap = ChrList_getPCap(gs, character);

  mount = chr_get_attachedto( chrlst, CHRLST_COUNT, character );
  if ( ACTIVE_CHR( chrlst,  mount ) && pchr->prop.isitem && chrlst[mount].transferblend )
  {
    // Okay, reset transparency
    enchant = pchr->firstenchant;
    while ( INVALID_ENC != enchant )
    {
      unset_enchant_value( gs, enchant, SETALPHABLEND );
      unset_enchant_value( gs, enchant, SETLIGHTBLEND );
      enchant = enclst[enchant].nextenchant;
    }

    pchr->alpha_fp8    = pcap->alpha_fp8;
    pchr->bumpstrength = pcap->bumpstrength * FP8_TO_FLOAT( pchr->alpha_fp8 );
    pchr->light_fp8    = pcap->light_fp8;
    enchant = pchr->firstenchant;
    while ( INVALID_ENC != enchant )
    {
      set_enchant_value( gs, enchant, SETALPHABLEND, enclst[enchant].eve );
      set_enchant_value( gs, enchant, SETLIGHTBLEND, enclst[enchant].eve );
      enchant = enclst[enchant].nextenchant;
    }
  }

}

//--------------------------------------------------------------------------------------------
void remove_enchant( CGame * gs, ENC_REF enchantindex )
{
  // ZZ> This function removes a specific enchantment and adds it to the unused list

  int add, cnt;

  PObj objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PCap caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CHR_REF character, overlay;
  ENC_REF lastenchant, currentenchant;

  CObj * pobj;

  OBJ_REF iobj;
  CEnc  * penc;

  CEve  * peve;
  CCap * pcap;

  ScriptInfo g_scr;

  memset(&g_scr, 0, sizeof(ScriptInfo));

  if ( !VALID_ENC(enclst, enchantindex) ) return;
  penc = enclst + enchantindex;

  // grab the profile
  iobj = penc->profile = VALIDATE_OBJ(objlst, penc->profile);
  if(INVALID_OBJ == iobj) return;
  pobj = gs->ObjList + iobj;

  // grab the eve
  peve = ObjList_getPEve(gs, iobj);
  if(NULL == peve) return;

  // Unsparkle the spellbook
  character = penc->spawner;
  if ( ACTIVE_CHR( chrlst,  character ) )
  {
    chrlst[character].sparkle = NOSPARKLE;
    // Make the spawner unable to undo the enchantment
    if ( chrlst[character].undoenchant == enchantindex )
    {
      chrlst[character].undoenchant = INVALID_ENC;
    }
  }


  // Play the end sound
  character = penc->target;
  if ( INVALID_SOUND != peve->endsound )
  {
    snd_play_sound( gs, 1.0f, chrlst[character].ori_old.pos, pobj->wavelist[peve->endsound], 0, iobj, peve->endsound );
  };

  // Unset enchant values, doing morph last (opposite order to enc_spawn_info_init)
  unset_enchant_value( gs, enchantindex, SETCHANNEL );
  for ( cnt = SETCOSTFOREACHMISSILE; cnt >= SETCOSTFOREACHMISSILE; cnt-- )
  {
    unset_enchant_value( gs, enchantindex, cnt );
  }
  unset_enchant_value( gs, enchantindex, SETMORPH );


  // Remove all of the cumulative values
  
  for ( add = 0; add < EVE_ADD_COUNT; add++ )
  {
    remove_enchant_value( gs, enchantindex, add );
  }


  // Unlink it
  if ( chrlst[character].firstenchant == enchantindex )
  {
    // It was the first in the list
    chrlst[character].firstenchant = penc->nextenchant;
  }
  else
  {
    // Search until we find it
    lastenchant    = INVALID_ENC;
    currentenchant = chrlst[character].firstenchant;
    while ( currentenchant != enchantindex )
    {
      lastenchant = currentenchant;
      currentenchant = enclst[currentenchant].nextenchant;
    }

    // Relink the last enchantment
    enclst[lastenchant].nextenchant = penc->nextenchant;
  }



  // See if we spit out an end message
  if ( peve->endmessage >= 0 )
  {
    display_message( gs, pobj->msg_start + peve->endmessage, penc->target );
  }

  // Check to see if we spawn a poof
  if ( peve->poofonend )
  {
    spawn_poof( gs, penc->target, iobj );
  }

  // Check to see if the character dies
  if ( peve->killonend )
  {
    if ( chrlst[character].prop.invictus )  gs->TeamList[chrlst[character].team_base].morale++;
    chrlst[character].prop.invictus = bfalse;
    kill_character( gs, character, INVALID_CHR );
  }
  // Kill overlay too...
  overlay = penc->overlay;
  if ( ACTIVE_CHR(chrlst, overlay) )
  {
    if ( chrlst[overlay].prop.invictus )  gs->TeamList[chrlst[overlay].team_base].morale++;
    chrlst[overlay].prop.invictus = bfalse;
    kill_character( gs, overlay, INVALID_CHR );
  }



  // Now get rid of it
  penc->active = bfalse;
  gs->EncFreeList[gs->EncFreeList_count] = enchantindex;
  gs->EncFreeList_count++;

  // Now fix dem weapons
  for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
  {
    reset_character_alpha( gs, chr_get_holdingwhich( chrlst, CHRLST_COUNT, character, _slot ) );
  }

  // And remove see kurse enchantment
  pcap = ChrList_getPCap(gs, character);
  if(peve->canseekurse && !pcap->prop.canseekurse)
  {
    chrlst[character].prop.canseekurse = bfalse;
  }


}

//--------------------------------------------------------------------------------------------
ENC_REF enchant_value_filled( CGame * gs, ENC_REF enchantindex, Uint8 valueindex )
{
  // ZZ> This function returns INVALID_ENC if the enchantment's target has no conflicting
  //     set values in its other enchantments.  Otherwise it returns the enchantindex
  //     of the conflicting enchantment

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CHR_REF character;
  ENC_REF currenchant;

  CEnc * penc;

  if( !VALID_ENC(enclst, enchantindex) ) return INVALID_ENC;
  penc = enclst + enchantindex;

  character = penc->target = VALIDATE_CHR(chrlst, penc->target);
  if( !ACTIVE_CHR(chrlst, character) ) return INVALID_ENC;

  currenchant = chrlst[character].firstenchant;
  while ( INVALID_ENC != currenchant )
  {
    if ( enclst[currenchant].setyesno[valueindex] )
    {
      return currenchant;
    }
    currenchant = enclst[currenchant].nextenchant;
  }

  return INVALID_ENC;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value( CGame * gs, ENC_REF enchantindex, Uint8 valueindex,
                        EVE_REF enchanttype )
{
  // ZZ> This function sets and saves one of the ichr's stats

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PCap caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  ENC_REF conflict;
  CHR_REF ichr;

  CEve * peve = evelst + enchanttype;
  CEnc * penc = enclst + enchantindex;
  CChr * pchr;
  CCap * pcap;


  penc->setyesno[valueindex] = bfalse;
  if ( peve->setyesno[valueindex] )
  {
    conflict = enchant_value_filled( gs, enchantindex, valueindex );
    if ( INVALID_ENC == conflict || peve->override )
    {
      // Check for multiple enchantments
      if ( INVALID_ENC != conflict )
      {
        // Multiple enchantments aren't allowed for sets
        if ( peve->removeoverridden )
        {
          // Kill the old enchantment
          remove_enchant( gs, conflict );
        }
        else
        {
          // Just unset the old enchantment's value
          unset_enchant_value( gs, conflict, valueindex );
        }
      }
      // Set the value, and save the ichr's real stat
      ichr = penc->target;
      pchr = ChrList_getPChr(gs, ichr);
      pcap = ChrList_getPCap(gs, ichr);
      penc->setyesno[valueindex] = btrue;
      switch ( valueindex )
      {
        case SETDAMAGETYPE:
          penc->setsave[valueindex] = pchr->damagetargettype;
          pchr->damagetargettype = (DAMAGE)peve->setvalue[valueindex];
          break;

        case SETNUMBEROFJUMPS:
          penc->setsave[valueindex] = pchr->jumpnumberreset;
          pchr->jumpnumberreset = peve->setvalue[valueindex];
          break;

        case SETLIFEBARCOLOR:
          penc->setsave[valueindex] = pchr->lifecolor;
          pchr->lifecolor = peve->setvalue[valueindex];
          break;

        case SETMANABARCOLOR:
          penc->setsave[valueindex] = pchr->manacolor;
          pchr->manacolor = peve->setvalue[valueindex];
          break;

        case SETSLASHMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_SLASH];
          pchr->skin.damagemodifier_fp8[DAMAGE_SLASH] = peve->setvalue[valueindex];
          break;

        case SETCRUSHMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH];
          pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH] = peve->setvalue[valueindex];
          break;

        case SETPOKEMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_POKE];
          pchr->skin.damagemodifier_fp8[DAMAGE_POKE] = peve->setvalue[valueindex];
          break;

        case SETHOLYMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_HOLY];
          pchr->skin.damagemodifier_fp8[DAMAGE_HOLY] = peve->setvalue[valueindex];
          break;

        case SETEVILMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_EVIL];
          pchr->skin.damagemodifier_fp8[DAMAGE_EVIL] = peve->setvalue[valueindex];
          break;

        case SETFIREMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_FIRE];
          pchr->skin.damagemodifier_fp8[DAMAGE_FIRE] = peve->setvalue[valueindex];
          break;

        case SETICEMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_ICE];
          pchr->skin.damagemodifier_fp8[DAMAGE_ICE] = peve->setvalue[valueindex];
          break;

        case SETZAPMODIFIER:
          penc->setsave[valueindex] = pchr->skin.damagemodifier_fp8[DAMAGE_ZAP];
          pchr->skin.damagemodifier_fp8[DAMAGE_ZAP] = peve->setvalue[valueindex];
          break;

        case SETFLASHINGAND:
          penc->setsave[valueindex] = pchr->flashand;
          pchr->flashand = peve->setvalue[valueindex];
          break;

        case SETLIGHTBLEND:
          penc->setsave[valueindex] = pchr->light_fp8;
          pchr->light_fp8 = peve->setvalue[valueindex];
          break;

        case SETALPHABLEND:
          penc->setsave[valueindex] = pchr->alpha_fp8;
          pchr->alpha_fp8 = peve->setvalue[valueindex];
          pchr->bumpstrength = pcap->bumpstrength * FP8_TO_FLOAT( pchr->alpha_fp8 );
          break;

        case SETSHEEN:
          penc->setsave[valueindex] = pchr->sheen_fp8;
          pchr->sheen_fp8 = peve->setvalue[valueindex];
          break;

        case SETFLYTOHEIGHT:
          penc->setsave[valueindex] = pchr->flyheight;
          if ( pchr->flyheight == 0 && pchr->ori.pos.z > -2 )
          {
            pchr->flyheight = peve->setvalue[valueindex];
          }
          break;

        case SETWALKONWATER:
          penc->setsave[valueindex] = pchr->prop.waterwalk;
          if ( !pchr->prop.waterwalk )
          {
            pchr->prop.waterwalk = INT_TO_BOOL(peve->setvalue[valueindex]);
          }
          break;

        case SETCANSEEINVISIBLE:
          penc->setsave[valueindex] = pchr->prop.canseeinvisible;
          pchr->prop.canseeinvisible = INT_TO_BOOL(peve->setvalue[valueindex]);
          break;

        case SETMISSILETREATMENT:
          penc->setsave[valueindex] = pchr->missiletreatment;
          pchr->missiletreatment    = (MISSLE_TYPE)peve->setvalue[valueindex];
          break;

        case SETCOSTFOREACHMISSILE:
          penc->setsave[valueindex] = pchr->missilecost;
          pchr->missilecost = peve->setvalue[valueindex];
          pchr->missilehandler = penc->owner;
          break;

        case SETMORPH:
          penc->setsave[valueindex] = pchr->skin_ref % MAXSKIN;
          // Special handler for morph
          change_character( gs, ichr, penc->profile, 0, LEAVE_ALL );
          pchr->aistate.morphed = btrue;
          break;

        case SETCHANNEL:
          penc->setsave[valueindex] = pchr->prop.canchannel;
          pchr->prop.canchannel = INT_TO_BOOL(peve->setvalue[valueindex]);
          break;

      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void getadd( int MIN, int value, int MAX, int* valuetoadd )
{
  // ZZ> This function figures out what value to add should be in order
  //     to not overflow the MIN and MAX bounds

  int newvalue;

  newvalue = value + ( *valuetoadd );
  if ( newvalue < MIN )
  {
    // Increase valuetoadd to fit
    *valuetoadd = MIN - value;
    if ( *valuetoadd > 0 )  *valuetoadd = 0;
    return;
  }


  if ( newvalue > MAX )
  {
    // Decrease valuetoadd to fit
    *valuetoadd = MAX - value;
    if ( *valuetoadd < 0 )  *valuetoadd = 0;
  }
}

//--------------------------------------------------------------------------------------------
void fgetadd( float MIN, float value, float MAX, float* valuetoadd )
{
  // ZZ> This function figures out what value to add should be in order
  //     to not overflow the MIN and MAX bounds

  float newvalue;


  newvalue = value + ( *valuetoadd );
  if ( newvalue < MIN )
  {
    // Increase valuetoadd to fit
    *valuetoadd = MIN - value;
    if ( *valuetoadd > 0 )  *valuetoadd = 0;
    return;
  }


  if ( newvalue > MAX )
  {
    // Decrease valuetoadd to fit
    *valuetoadd = MAX - value;
    if ( *valuetoadd < 0 )  *valuetoadd = 0;
  }
}

//--------------------------------------------------------------------------------------------
void add_enchant_value( CGame * gs, ENC_REF enchantindex, Uint8 valueindex,
                        EVE_REF enchanttype )
{
  // ZZ> This function does cumulative modification to character stats

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  int valuetoadd, newvalue;
  float fvaluetoadd, fnewvalue;
  CHR_REF ichr;

  CChr * pchr;
  CEve * peve = evelst + enchanttype;
  CEnc * penc = enclst + enchantindex;


  ichr = penc->target;
  if( !ACTIVE_CHR( chrlst, ichr) ) return;
  pchr = ChrList_getPChr(gs, ichr);

  valuetoadd = 0;
  switch ( valueindex )
  {
    case ADDJUMPPOWER:
      fnewvalue = pchr->jump;
      fvaluetoadd = peve->addvalue[valueindex] / 16.0;
      fgetadd( 0, fnewvalue, 30.0, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 16.0; // Get save value
      fvaluetoadd = valuetoadd / 16.0;
      pchr->jump += fvaluetoadd;

      break;

    case ADDBUMPDAMPEN:
      fnewvalue = pchr->bumpdampen;
      fvaluetoadd = peve->addvalue[valueindex] / 128.0;
      fgetadd( 0, fnewvalue, 1.0, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 128.0; // Get save value
      fvaluetoadd = valuetoadd / 128.0;
      pchr->bumpdampen += fvaluetoadd;
      break;

    case ADDBOUNCINESS:
      fnewvalue = pchr->dampen;
      fvaluetoadd = peve->addvalue[valueindex] / 128.0;
      fgetadd( 0, fnewvalue, 0.95, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 128.0; // Get save value
      fvaluetoadd = valuetoadd / 128.0;
      pchr->dampen += fvaluetoadd;
      break;

    case ADDDAMAGE:
      newvalue = pchr->damageboost;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, 4096, &valuetoadd );
      pchr->damageboost += valuetoadd;
      break;

    case ADDSIZE:
      fnewvalue = pchr->sizegoto;
      fvaluetoadd = peve->addvalue[valueindex] / 128.0;
      fgetadd( 0.5, fnewvalue, 2.0, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 128.0; // Get save value
      fvaluetoadd = valuetoadd / 128.0;
      pchr->sizegoto += fvaluetoadd;
      pchr->sizegototime = DELAY_RESIZE;
      break;

    case ADDACCEL:
      fnewvalue = pchr->skin.maxaccel;
      fvaluetoadd = peve->addvalue[valueindex] / 25.0;
      fgetadd( 0, fnewvalue, 1.5, &fvaluetoadd );
      valuetoadd = fvaluetoadd * 1000.0; // Get save value
      fvaluetoadd = valuetoadd / 1000.0;
      pchr->skin.maxaccel += fvaluetoadd;
      break;

    case ADDRED:
      newvalue = pchr->redshift;
      valuetoadd = peve->addvalue[valueindex];
      getadd( 0, newvalue, 6, &valuetoadd );
      pchr->redshift += valuetoadd;
      break;

    case ADDGRN:
      newvalue = pchr->grnshift;
      valuetoadd = peve->addvalue[valueindex];
      getadd( 0, newvalue, 6, &valuetoadd );
      pchr->grnshift += valuetoadd;
      break;

    case ADDBLU:
      newvalue = pchr->blushift;
      valuetoadd = peve->addvalue[valueindex];
      getadd( 0, newvalue, 6, &valuetoadd );
      pchr->blushift += valuetoadd;
      break;

    case ADDDEFENSE:
      newvalue = pchr->skin.defense_fp8;
      valuetoadd = peve->addvalue[valueindex];
      getadd( 55, newvalue, 255, &valuetoadd );   // Don't fix again!
      pchr->skin.defense_fp8 += valuetoadd;
      break;

    case ADDMANA:
      newvalue = pchr->stats.manamax_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
      pchr->stats.manamax_fp8 += valuetoadd;
      pchr->stats.mana_fp8 += valuetoadd;
      if ( pchr->stats.mana_fp8 < 0 )  pchr->stats.mana_fp8 = 0;
      break;

    case ADDLIFE:
      newvalue = pchr->stats.lifemax_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( LOWSTAT, newvalue, HIGHSTAT, &valuetoadd );
      pchr->stats.lifemax_fp8 += valuetoadd;
      pchr->stats.life_fp8 += valuetoadd;
      if ( pchr->stats.life_fp8 < 1 )  pchr->stats.life_fp8 = 1;
      break;

    case ADDSTRENGTH:
      newvalue = pchr->stats.strength_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->stats.strength_fp8 += valuetoadd;
      break;

    case ADDWISDOM:
      newvalue = pchr->stats.wisdom_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->stats.wisdom_fp8 += valuetoadd;
      break;

    case ADDINTELLIGENCE:
      newvalue = pchr->stats.intelligence_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->stats.intelligence_fp8 += valuetoadd;
      break;

    case ADDDEXTERITY:
      newvalue = pchr->stats.dexterity_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->stats.dexterity_fp8 += valuetoadd;
      break;
  }

  penc->addsave[valueindex] = valuetoadd;  // Save the value for undo
}

//--------------------------------------------------------------------------------------------
enc_spawn_info * enc_spawn_info_new(enc_spawn_info * psi, CGame * gs)
{
  if(NULL == psi) return psi;

  memset(psi, 0, sizeof(enc_spawn_info));

  EKEY_PNEW( psi, enc_spawn_info );

  psi->gs = gs;
  psi->seed = (NULL==gs) ? -time(NULL) : gs->randie_index;

  psi->ienc = INVALID_ENC;
  psi->owner = INVALID_CHR;
  psi->target = INVALID_CHR;
  psi->spawner = INVALID_CHR;
  psi->enchantindex = INVALID_ENC;
  psi->iobj = INVALID_OBJ;
  psi->ieve = INVALID_EVE;

  return psi;
}

//--------------------------------------------------------------------------------------------
ENC_REF enc_spawn_info_init( enc_spawn_info * psi, CGame * gs, CHR_REF owner, CHR_REF target,
                         CHR_REF spawner, ENC_REF enc_request, OBJ_REF obj_optional )
{
  // ZZ> This function enchants a target, returning the enchantment index or INVALID_ENC
  //     if failed

  PEve evelst      = gs->EveList;
  size_t evelst_size = EVELST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  PChr chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CEve * peve;

  if(NULL == psi) return INVALID_ENC;

  enc_spawn_info_new(psi, gs);
  
  // grab the correct profile
  psi->iobj = INVALID_OBJ;
  if ( INVALID_OBJ != obj_optional )
  {
    // The enchantment type is given explicitly
    psi->iobj = VALIDATE_OBJ(gs->ObjList, obj_optional);
  }
  
  if(INVALID_OBJ == psi->iobj)
  {
    // The enchantment type is given by the spawner
    psi->iobj = ChrList_getRObj(gs, spawner);
  }
  if( INVALID_OBJ == psi->iobj ) return INVALID_ENC;

  // grab the correct eve
  psi->ieve = ObjList_getREve(gs, psi->iobj);
  if ( INVALID_EVE == psi->ieve ) return INVALID_ENC;
  peve = evelst + psi->ieve;

  // do some extra work if a specific enchant index is not requested
  if ( INVALID_ENC == enc_request )
  {
    // Should it choose an inhand item?
    if ( peve->retarget )
    {
      bool_t bfound = bfalse;
      SLOT best_slot = SLOT_BEGIN;

      // Is at least one valid?
      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        if ( chr_using_slot( chrlst, CHRLST_COUNT, target, _slot ) )
        {
          bfound = btrue;
          if ( _slot > best_slot ) best_slot = _slot;  // choose SLOT_RIGHT above SLOT_LEFT
          break;
        }
      };

      if ( !bfound ) return INVALID_ENC;

      target = chr_get_holdingwhich( chrlst, CHRLST_COUNT, target, best_slot );
    }


    // Make sure it's valid
    if ( peve->dontdamagetype != DAMAGE_NULL )
    {
      if (( chrlst[target].skin.damagemodifier_fp8[peve->dontdamagetype]&7 ) >= 3 )   // Invert | Shift = 7
      {
        return INVALID_ENC;
      }
    }

    if ( peve->onlydamagetype != DAMAGE_NULL )
    {
      if ( chrlst[target].damagetargettype != peve->onlydamagetype )
      {
        return INVALID_ENC;
      }
    }

  }

  // Owner must both be alive and active
  if ( !ACTIVE_CHR( chrlst, owner ) || !chrlst[owner].alive )
  {
    // Invalid owner
    return INVALID_ENC;
  }
  psi->owner = owner;

  // Target must both be alive and active
  if ( !ACTIVE_CHR(chrlst, target) || !chrlst[target].alive )
  {
    // Invalid target
    return INVALID_ENC;
  }
   psi->target = target;

  // do this absolutely last so that we will not need to free it if we hit a snag
  psi->ienc = EncList_get_free(gs, enc_request);
  if(INVALID_ENC == psi->ienc) return INVALID_ENC;

  return psi->ienc;
}


//--------------------------------------------------------------------------------------------
ENC_REF req_spawn_one_enchant( enc_spawn_info si )
{
  // ZZ> This function enchants a si.target, returning the enchantment index or INVALID_ENC
  //     if failed

  PEve evelst = si.gs->EveList;
  PEnc enclst = si.gs->EncList;
  PChr chrlst = si.gs->ChrList;
  PObj objlst = si.gs->ObjList;

  CEnc * penc;
  CEve * peve;
  CObj * pobj;

  int add, cnt;

  // grab the correct enchant
  if( !VALID_ENC(enclst, si.ienc) ) return INVALID_ENC;
  penc = enclst + si.ienc;

  // grab the correct profile
  if( !VALID_OBJ(objlst, si.iobj) ) return INVALID_ENC;
  pobj = ObjList_getPObj(si.gs, si.iobj);

  // grab the correct eve
  si.ieve = ObjList_getREve(si.gs, si.iobj);
  if ( !VALID_OBJ(objlst, si.ieve)  ) return INVALID_ENC;
  peve = ObjList_getPEve(si.gs, si.ieve);

  // Make a new one
  penc->target         = si.target;
  penc->owner          = si.owner;
  penc->eve            = si.ieve;
  penc->time           = peve->time;
  penc->spawntime      = 1;
  penc->ownermana_fp8  = peve->ownermana_fp8;
  penc->ownerlife_fp8  = peve->ownerlife_fp8;
  penc->targetmana_fp8 = peve->targetmana_fp8;
  penc->targetlife_fp8 = peve->targetlife_fp8;

  // link it to the spawner
  if ( ACTIVE_CHR(chrlst, si.spawner) )
  {
    chrlst[si.spawner].undoenchant = si.ienc;
    penc->spawner = si.spawner;
  }
  else
  {
    penc->spawner = INVALID_CHR;
  }

  // Now set all of the specific values, morph first
  set_enchant_value( si.gs, si.ienc, SETMORPH, si.ieve );
  for ( cnt = SETDAMAGETYPE; cnt <= SETCOSTFOREACHMISSILE; cnt++ )
  {
    set_enchant_value( si.gs, si.ienc, cnt, si.ieve );
  }
  set_enchant_value( si.gs, si.ienc, SETCHANNEL, si.ieve );


  // Now do all of the stat adds
  add = 0;
  while ( add < EVE_ADD_COUNT )
  {
    add_enchant_value( si.gs, si.ienc, add, si.ieve );
    add++;
  }

  penc->reserved = btrue;

  return si.ienc;
}

//--------------------------------------------------------------------------------------------
bool_t activate_enchant(CEnc * penc)
{
  PEve evelst;
  PEnc enclst;
  PChr chrlst;

  CGame * gs;
  ENC_REF ienc;
  CEve * peve;
  CChr * ptarget;

  if( EKEY_PVALID(penc) || penc->active ) return bfalse;

  gs   = penc->spinfo.gs;

  evelst = gs->EveList;
  enclst = gs->EncList;
  chrlst = gs->ChrList;

  ienc    = penc->spinfo.ienc;
  peve    = evelst + penc->eve;
  ptarget = chrlst + penc->target;

  // Link the enchant into tthe target's list
  penc->nextenchant = ptarget->firstenchant;
  ptarget->firstenchant      = ienc;

  //Enchant to allow see kurses?
  ptarget->prop.canseekurse = ptarget->prop.canseekurse || peve->canseekurse;

  // Create an overlay character?
  penc->overlay = INVALID_CHR;
  if ( peve->overlay )
  {
    penc->overlay = chr_spawn( gs, ptarget->ori.pos, ptarget->ori.vel, penc->spinfo.iobj, ptarget->team, 0, ptarget->ori.turn_lr, NULL, INVALID_CHR );
    if ( VALID_CHR(chrlst, penc->overlay) )
    {
      CChr * povl;
      CMad * povl_mad;

      povl = ChrList_getPChr(gs, penc->overlay);
      povl_mad = ChrList_getPMad(gs, penc->overlay);

      povl->aistate.target = penc->target;
      povl->aistate.state  = peve->overlay;
      povl->overlay        = btrue;

      // Start out with ActionMJ...  Object activated
      if ( povl_mad->actionvalid[ACTION_MJ] )
      {
        povl->action.now = ACTION_MJ;
        povl->anim.ilip = 0;
        povl->anim.flip = 0.0f;
        povl->anim.next = povl_mad->actionstart[ACTION_MJ];
        povl->anim.last = povl->anim.next;
        povl->action.ready = bfalse;
      }

      povl->light_fp8 = 254;  // Assume it's transparent...
    }
  }

  penc->active = btrue;

  return btrue;

}

//--------------------------------------------------------------------------------------------
void EncList_resynch( CGame * gs )
{
  ENC_REF ienc;
  CEnc  * penc;
  PEnc enclst = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  // turn on all enchants requested in the last turn
  for(ienc = 0; ienc < enclst_size; ienc++)
  {
    if( !PENDING_ENC(enclst, ienc) ) continue;
    penc = enclst + ienc;

    penc->req_active = bfalse;
    penc->reserved   = bfalse;

    penc->active = btrue;
  }
}