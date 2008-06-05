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

static Uint16   numchop = 0;               // The number of name parts
static Uint32   chopwrite = 0;             // The data pointer
static char     chopdata[CHOPDATACHUNK];    // The name parts
static Uint16   chopstart[MAXCHOP];         // The first character of each part
STRING          namingnames;// The name returned by the function

//--------------------------------------------------------------------------------------------
void enc_spawn_particles( CGame * gs, float dUpdate )
{
  // ZZ> This function lets enchantments spawn particles

  int cnt, tnc;
  Uint16 facing;
  EVE_REF eve;
  CHR_REF target_ref;

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;
  
  for ( cnt = 0; cnt < enclst_size; cnt++ )
  {
    if ( !enclst[cnt].on ) continue;

    eve = enclst[cnt].eve;
    if ( evelst[eve].contspawnamount <= 0 ) continue;

    enclst[cnt].spawntime -= dUpdate;
    if ( enclst[cnt].spawntime <= 0 ) enclst[cnt].spawntime = 0;
    if ( enclst[cnt].spawntime > 0 ) continue;

    target_ref = enclst[cnt].target;
    if( !VALID_CHR(chrlst, target_ref) ) continue;

    enclst[cnt].spawntime = evelst[eve].contspawntime;
    facing = chrlst[target_ref].turn_lr;
    for ( tnc = 0; tnc < evelst[eve].contspawnamount; tnc++)
    {
      spawn_one_particle( gs, 1.0f, chrlst[target_ref].pos, facing, eve, evelst[eve].contspawnpip,
        MAXCHR, GRIP_LAST, chrlst[enclst[cnt].owner].team, enclst[cnt].owner, tnc, MAXCHR );

      facing += evelst[eve].contspawnfacingadd;
    }

  }
}


//--------------------------------------------------------------------------------------------
void disenchant_character( CGame * gs, Uint16 cnt )
{
  // ZZ> This function removes all enchantments from a character

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  while ( chrlst[cnt].firstenchant != MAXENCHANT )
  {
    remove_enchant( gs, chrlst[cnt].firstenchant );
  }
}

//--------------------------------------------------------------------------------------------
void spawn_poof( CGame * gs, CHR_REF character, Uint16 profile )
{
  // ZZ> This function spawns a character poof

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 sTmp;
  Uint16 origin;
  int iTmp;

  sTmp = chrlst[character].turn_lr;
  iTmp = 0;
  origin = chr_get_aiowner( chrlst, MAXCHR, chrlst + character );
  while ( iTmp < caplst[profile].gopoofprtamount )
  {
    spawn_one_particle( gs, 1.0f, chrlst[character].pos_old,
                        sTmp, profile, caplst[profile].gopoofprttype,
                        MAXCHR, GRIP_LAST, chrlst[character].team, origin, iTmp, MAXCHR );

    sTmp += caplst[profile].gopoofprtfacingadd;
    iTmp++;
  }
}

//--------------------------------------------------------------------------------------------
void naming_names( CGame * gs, int profile )
{
  // ZZ> This function generates a random name

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  int read, write, section, mychop;
  char cTmp;

  if ( caplst[profile].sectionsize[0] == 0 )
  {
    strcpy(namingnames, "Blah");
  }
  else
  {
    write = 0;
    section = 0;
    while ( section < MAXSECTION )
    {
      if ( caplst[profile].sectionsize[section] != 0 )
      {
        mychop = caplst[profile].sectionstart[section] + ( rand() % caplst[profile].sectionsize[section] );
        read = chopstart[mychop];
        cTmp = chopdata[read];
        while ( cTmp != 0 && write < MAXCAPNAMESIZE - 1 )
        {
          namingnames[write] = cTmp;
          write++;
          read++;
          cTmp = chopdata[read];
        }
      }
      section++;
    }
    if ( write >= MAXCAPNAMESIZE ) write = MAXCAPNAMESIZE - 1;
    namingnames[write] = 0;
  }
}

//--------------------------------------------------------------------------------------------
void read_naming( CGame * gs, char * szModpath, char * szObjectname, int profile)
{
  // ZZ> This function reads a naming file

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  FILE *fileread;
  int section, chopinsection, cnt;
  char mychop[32], cTmp;

  fileread = fs_fileOpen( PRI_NONE, NULL, inherit_fname(szModpath, szObjectname, CData.naming_file), "r" );
  if ( NULL == fileread ) return;

  section = 0;
  chopinsection = 0;
  while ( fget_next_string( fileread, mychop, sizeof( mychop ) ) && section < MAXSECTION )
  {
    if ( 0 != strcmp( mychop, "STOP" ) )
    {
      if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;
      chopstart[numchop] = chopwrite;

      cnt = 0;
      cTmp = mychop[0];
      while ( cTmp != 0 && cnt < 31 && chopwrite < CHOPDATACHUNK )
      {
        if ( cTmp == '_' ) cTmp = ' ';
        chopdata[chopwrite] = cTmp;
        cnt++;
        chopwrite++;
        cTmp = mychop[cnt];
      }

      if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;
      chopdata[chopwrite] = 0;  chopwrite++;
      chopinsection++;
      numchop++;
    }
    else
    {
      caplst[profile].sectionsize[section] = chopinsection;
      caplst[profile].sectionstart[section] = numchop - chopinsection;
      section++;
      chopinsection = 0;
    }
  }
  fs_fileClose( fileread );

}

//--------------------------------------------------------------------------------------------
void prime_names( CGame * gs )
{
  // ZZ> This function prepares the name chopper for use

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  int cnt, tnc;

  numchop = 0;
  chopwrite = 0;
  cnt = 0;
  while ( cnt < MAXMODEL )
  {
    tnc = 0;
    while ( tnc < MAXSECTION )
    {
      caplst[cnt].sectionstart[tnc] = MAXCHOP;
      caplst[cnt].sectionsize[tnc] = 0;
      tnc++;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain(CGame * gs)
{
  // ZZ> This function sets all of the character's starting tilt values

  int cnt;
  Uint8 twist;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if( !VALID_CHR(chrlst, cnt) ) continue;

    if ( chrlst[cnt].stickybutt && INVALID_FAN != chrlst[cnt].onwhichfan)
    {
      twist = mesh_get_twist( gs->Mesh_Mem.fanlst, chrlst[cnt].onwhichfan );
      chrlst[cnt].mapturn_lr = twist_table[twist].lr;
      chrlst[cnt].mapturn_ud = twist_table[twist].ud;
    }
  }
}


//--------------------------------------------------------------------------------------------
Uint16 change_armor( CGame * gs, CHR_REF character, Uint16 iskin )
{
  // ZZ> This function changes the armor of the character

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 enchant, sTmp;
  int iTmp, cnt;

  // Remove armor enchantments
  enchant = chrlst[character].firstenchant;
  while ( enchant < MAXENCHANT )
  {
    for ( cnt = SETSLASHMODIFIER; cnt <= SETZAPMODIFIER; cnt++ )
    {
      unset_enchant_value( gs, enchant, cnt );
    }
    enchant = enclst[enchant].nextenchant;
  }


  // Change the skin
  sTmp = chrlst[character].model;
  if ( iskin > gs->MadList[sTmp].skins )  iskin = 0;
  chrlst[character].skin_ref  = iskin;


  // Change stats associated with skin
  chrlst[character].skin.defense_fp8 = caplst[sTmp].skin[iskin].defense_fp8;
  iTmp = 0;
  while ( iTmp < MAXDAMAGETYPE )
  {
    chrlst[character].skin.damagemodifier_fp8[iTmp] = caplst[sTmp].skin[iskin].damagemodifier_fp8[iTmp];
    iTmp++;
  }
  chrlst[character].skin.maxaccel = caplst[sTmp].skin[iskin].maxaccel;


  // Reset armor enchantments
  // These should really be done in reverse order ( Start with last enchant ), but
  // I don't care at this point !!!BAD!!!
  enchant = chrlst[character].firstenchant;
  while ( enchant < MAXENCHANT )
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
void change_character( CGame * gs, CHR_REF ichr, Uint16 new_profile, Uint8 new_skin, Uint8 leavewhich )
{
  // ZZ> This function polymorphs a character, changing stats, dropping weapons

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  int tnc, enchant;
  Uint16 sTmp;
  CHR_REF item, imount;
  Chr * pchr;
  Mad * pmad;
  Cap * pcap;

  if( !VALID_CHR( chrlst, ichr) ) return;

  pchr = chrlst + ichr;

  if ( !VALID_MDL_RANGE(new_profile) ) return;
  
  pmad = gs->MadList + new_profile;
  if( !pmad->used ) return;

  pcap = caplst + new_profile;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    sTmp = chr_get_holdingwhich( chrlst, MAXCHR, ichr, _slot );
    if ( !pcap->slotvalid[_slot] )
    {
      if ( detach_character_from_mount( gs, sTmp, btrue, btrue ) )
      {
        if ( _slot == SLOT_SADDLE )
        {
          chrlst[sTmp].accum_vel.z += DISMOUNTZVEL;
          chrlst[sTmp].accum_pos.z += DISMOUNTZVEL;
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
        if ( enchant != MAXENCHANT )
        {
          while ( enclst[enchant].nextenchant != MAXENCHANT )
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
  pchr->lifeheal  = pcap->lifeheal_fp8;
  pchr->manacost  = pcap->manacost_fp8;

  // Ammo
  pchr->ammomax = pcap->ammomax;
  pchr->ammo = pcap->ammo;
  // Gender
  if ( pcap->gender != GEN_RANDOM ) // GEN_RANDOM means keep old gender
  {
    pchr->gender = pcap->gender;
  }

  // AI stuff
  pchr->aistate.type = pmad->ai;
  pchr->aistate.state = 0;
  pchr->aistate.time = 0;
  pchr->aistate.latch.x = 0;
  pchr->aistate.latch.y = 0;
  pchr->aistate.latch.b = 0;
  pchr->aistate.turnmode = TURNMODE_VELOCITY;

  // Flags
  pchr->stickybutt = pcap->stickybutt;
  pchr->openstuff = pcap->canopenstuff;
  pchr->transferblend = pcap->transferblend;
  pchr->enviro = pcap->enviro;
  pchr->isplatform = pcap->isplatform;
  pchr->isitem = pcap->isitem;
  pchr->invictus = pcap->invictus;
  pchr->ismount = pcap->ismount;
  pchr->cangrabmoney = pcap->cangrabmoney;
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
  imount = chr_get_attachedto( chrlst, MAXCHR, ichr );
  if ( VALID_CHR( chrlst,  imount ) )
  {
    Uint16 imodel =  chrlst[imount].model;
    Uint16 vrtoffset = slot_to_offset( pchr->inwhichslot );

    if( !VALID_MDL(imodel) )
    {
      pchr->attachedgrip[0] = 0;
      pchr->attachedgrip[1] = 0xFFFF;
      pchr->attachedgrip[2] = 0xFFFF;
      pchr->attachedgrip[3] = 0xFFFF;
    }
    else if ( gs->MadList[imodel].vertices > vrtoffset && vrtoffset > 0 )
    {
      tnc = gs->MadList[imodel].vertices - vrtoffset;
      pchr->attachedgrip[0] = tnc;
      pchr->attachedgrip[1] = tnc + 1;
      pchr->attachedgrip[2] = tnc + 2;
      pchr->attachedgrip[3] = tnc + 3;
    }
    else
    {
      pchr->attachedgrip[0] = gs->MadList[imodel].vertices - 1;
      pchr->attachedgrip[1] = 0xFFFF;
      pchr->attachedgrip[2] = 0xFFFF;
      pchr->attachedgrip[3] = 0xFFFF;
    }
  }

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    item = chr_get_holdingwhich( chrlst, MAXCHR, ichr, _slot );
    if ( VALID_CHR( chrlst,  item ) )
    {
      int i, grip_offset;
      int vrtcount = gs->MadList[pchr->model].vertices;

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
  pchr->onwhichplatform = MAXCHR;

  // Set the new_skin
  change_armor( gs, ichr, new_skin );

  // Reaffirm them particles...
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
  reaffirm_attached_particles( gs, ichr );

  make_one_character_matrix(chrlst, MAXCHR, chrlst + ichr);
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( CGame * gs, CHR_REF chr_ref, int amount, Uint16 killer )
{
  // ZZ> This function takes mana from a character ( or gives mana ),
  //     and returns btrue if the character had enough to pay, or bfalse
  //     otherwise

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  int iTmp;

  iTmp = chrlst[chr_ref].mana_fp8 - amount;
  if ( iTmp < 0 )
  {
    chrlst[chr_ref].mana_fp8 = 0;
    if ( chrlst[chr_ref].canchannel )
    {
      chrlst[chr_ref].life_fp8 += iTmp;
      if ( chrlst[chr_ref].life_fp8 <= 0 )
      {
        kill_character( gs, chr_ref, chr_ref );
      }
      return btrue;
    }
    return bfalse;
  }
  else
  {
    chrlst[chr_ref].mana_fp8 = iTmp;
    if ( iTmp > chrlst[chr_ref].manamax_fp8 )
    {
      chrlst[chr_ref].mana_fp8 = chrlst[chr_ref].manamax_fp8;
    }
  }
  return btrue;
}

//--------------------------------------------------------------------------------------------
Enc * Enc_new(Enc *penc) 
{ 
  //fprintf( stdout, "Enc_new()\n");

  if(NULL==penc) return penc; 

  memset(penc, 0, sizeof(Enc));
  penc->on = bfalse;

  return penc; 
};

//--------------------------------------------------------------------------------------------
bool_t Enc_delete( Enc * penc)
{
  if(NULL == penc) return bfalse;

  penc->on = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Enc * Enc_renew( Enc * penc )
{
  Enc_delete( penc );
  return Enc_new( penc );
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
void EveList_load_one( CGame * gs, char * szObjectpath, char * szObjectname, Uint16 profile )
{
  // ZZ> This function loads the enchantment associated with an object

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  FILE* fileread;
  char cTmp;
  int iTmp;
  int num;
  IDSZ idsz;
  Eve * peve;
  
  if( !VALID_MDL_RANGE(profile) ) return;
  peve = evelst + profile;

  globalname = szObjectname;
  fileread = fs_fileOpen( PRI_NONE, NULL, inherit_fname(szObjectpath, szObjectname, CData.enchant_file), "r" );
  if ( NULL == fileread ) return;

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
    else if ( MAKE_IDSZ( "STAY" ) == idsz )  peve->stayifnoowner = 0 != iTmp;
    else if ( MAKE_IDSZ( "OVER" ) == idsz )  peve->overlay = iTmp;
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  peve->overlay = (bfalse != iTmp);
  }

  peve->used = btrue;

  // All done ( finally )
  fs_fileClose( fileread );
}

//--------------------------------------------------------------------------------------------
Uint16 EncList_get_free( CGame * gs )
{
  // ZZ> This function returns the next free enchantment or MAXENCHANT if there are none

  if ( gs->EncFreeList_count > 0 )
  {
    gs->EncFreeList_count--;
    return gs->EncFreeList[gs->EncFreeList_count];
  }
  return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value( CGame * gs, Uint16 enchantindex, Uint8 valueindex )
{
  // ZZ> This function unsets a set value

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  CHR_REF character;

  if ( enclst[enchantindex].setyesno[valueindex] )
  {
    character = enclst[enchantindex].target;
    switch ( valueindex )
    {
      case SETDAMAGETYPE:
        chrlst[character].damagetargettype = enclst[enchantindex].setsave[valueindex];
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
        chrlst[character].waterwalk = enclst[enchantindex].setsave[valueindex];
        break;

      case SETCANSEEINVISIBLE:
        chrlst[character].canseeinvisible = enclst[enchantindex].setsave[valueindex];
        break;

      case SETMISSILETREATMENT:
        chrlst[character].missiletreatment = enclst[enchantindex].setsave[valueindex];
        break;

      case SETCOSTFOREACHMISSILE:
        chrlst[character].missilecost = enclst[enchantindex].setsave[valueindex];
        chrlst[character].missilehandler = character;
        break;

      case SETMORPH:
        // Need special handler for when this is removed
        change_character( gs, character, chrlst[character].basemodel, enclst[enchantindex].setsave[valueindex], LEAVE_ALL );
        chrlst[character].aistate.morphed = btrue;
        break;

      case SETCHANNEL:
        chrlst[character].canchannel = enclst[enchantindex].setsave[valueindex];
        break;

    }
    enclst[enchantindex].setyesno[valueindex] = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value( CGame * gs, Uint16 enchantindex, Uint8 valueindex )
{
  // ZZ> This function undoes cumulative modification to character stats

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  float fvaluetoadd;
  int valuetoadd;
  CHR_REF character;
  Chr * pchr;
  Enc * penc;

  penc = enclst + enchantindex;
  character = penc->target;

  if( !VALID_CHR( chrlst, character) ) return;
  pchr = chrlst + character;

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
      pchr->manamax_fp8 -= valuetoadd;
      pchr->mana_fp8 -= valuetoadd;
      if ( pchr->mana_fp8 < 0 ) pchr->mana_fp8 = 0;
      break;

    case ADDLIFE:
      valuetoadd = penc->addsave[valueindex];
      pchr->lifemax_fp8 -= valuetoadd;
      pchr->life_fp8 -= valuetoadd;
      if ( pchr->life_fp8 < 1 ) pchr->life_fp8 = 1;
      break;

    case ADDSTRENGTH:
      valuetoadd = penc->addsave[valueindex];
      pchr->strength_fp8 -= valuetoadd;
      break;

    case ADDWISDOM:
      valuetoadd = penc->addsave[valueindex];
      pchr->wisdom_fp8 -= valuetoadd;
      break;

    case ADDINTELLIGENCE:
      valuetoadd = penc->addsave[valueindex];
      pchr->intelligence_fp8 -= valuetoadd;
      break;

    case ADDDEXTERITY:
      valuetoadd = penc->addsave[valueindex];
      pchr->dexterity_fp8 -= valuetoadd;
      break;

  }
}

//--------------------------------------------------------------------------------------------
Eve * Eve_new(Eve *peve) 
{ 
  //fprintf( stdout, "Eve_new()\n");

  if(NULL==peve) return peve; 

  memset(peve, 0, sizeof(Eve)); 
  peve->used = bfalse;

  return peve; 
};

//--------------------------------------------------------------------------------------------
bool_t Eve_delete( Eve * peve )
{
  if(NULL == peve) return bfalse;

  peve->used = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Eve * Eve_renew( Eve * peve )
{
  Eve_delete( peve );
  return Eve_new( peve );
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( CGame * gs, CHR_REF character )
{
  // ZZ> This function fixes an item's transparency

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 enchant, mount;

  if ( !VALID_CHR( chrlst,  character ) ) return;

  mount = chr_get_attachedto( chrlst, MAXCHR, character );
  if ( VALID_CHR( chrlst,  mount ) && chrlst[character].isitem && chrlst[mount].transferblend )
  {
    // Okay, reset transparency
    enchant = chrlst[character].firstenchant;
    while ( enchant < MAXENCHANT )
    {
      unset_enchant_value( gs, enchant, SETALPHABLEND );
      unset_enchant_value( gs, enchant, SETLIGHTBLEND );
      enchant = enclst[enchant].nextenchant;
    }
    chrlst[character].alpha_fp8 = caplst[chrlst[character].model].alpha_fp8;
    chrlst[character].bumpstrength = caplst[chrlst[character].model].bumpstrength * FP8_TO_FLOAT( chrlst[character].alpha_fp8 );
    chrlst[character].light_fp8 = caplst[chrlst[character].model].light_fp8;
    enchant = chrlst[character].firstenchant;
    while ( enchant < MAXENCHANT )
    {
      set_enchant_value( gs, enchant, SETALPHABLEND, enclst[enchant].eve );
      set_enchant_value( gs, enchant, SETLIGHTBLEND, enclst[enchant].eve );
      enchant = enclst[enchant].nextenchant;
    }
  }

}

//--------------------------------------------------------------------------------------------
void remove_enchant( CGame * gs, Uint16 enchantindex )
{
  // ZZ> This function removes a specific enchantment and adds it to the unused list

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;


  CHR_REF character, overlay;
  Uint16 eve;
  Uint16 lastenchant, currentenchant;
  int add, cnt;

  ScriptInfo g_scr;

  memset(&g_scr, 0, sizeof(ScriptInfo));

  if ( enchantindex >= MAXENCHANT || !enclst[enchantindex].on ) return;

  // grab the profile
  eve = enclst[enchantindex].eve;

  // Unsparkle the spellbook
  character = enclst[enchantindex].spawner;
  if ( VALID_CHR( chrlst,  character ) )
  {
    chrlst[character].sparkle = NOSPARKLE;
    // Make the spawner unable to undo the enchantment
    if ( chrlst[character].undoenchant == enchantindex )
    {
      chrlst[character].undoenchant = MAXENCHANT;
    }
  }


  // Play the end sound
  character = enclst[enchantindex].target;
  if ( INVALID_SOUND != evelst[eve].endsound )
  {
    snd_play_sound( gs, 1.0f, chrlst[character].pos_old, caplst[eve].wavelist[evelst[eve].endsound], 0, eve, evelst[eve].endsound );
  };


  // Unset enchant values, doing morph last (opposite order to spawn_enchant)
  unset_enchant_value( gs, enchantindex, SETCHANNEL );
  for ( cnt = SETCOSTFOREACHMISSILE; cnt >= SETCOSTFOREACHMISSILE; cnt-- )
  {
    unset_enchant_value( gs, enchantindex, cnt );
  }
  unset_enchant_value( gs, enchantindex, SETMORPH );


  // Remove all of the cumulative values
  add = 0;
  while ( add < EVE_ADD_COUNT )
  {
    remove_enchant_value( gs, enchantindex, add );
    add++;
  }


  // Unlink it
  if ( chrlst[character].firstenchant == enchantindex )
  {
    // It was the first in the list
    chrlst[character].firstenchant = enclst[enchantindex].nextenchant;
  }
  else
  {
    // Search until we find it
    lastenchant    = MAXENCHANT;
    currentenchant = chrlst[character].firstenchant;
    while ( currentenchant != enchantindex )
    {
      lastenchant = currentenchant;
      currentenchant = enclst[currentenchant].nextenchant;
    }

    // Relink the last enchantment
    enclst[lastenchant].nextenchant = enclst[enchantindex].nextenchant;
  }



  // See if we spit out an end message
  if ( evelst[eve].endmessage >= 0 )
  {
    display_message( gs, gs->MadList[eve].msg_start + evelst[eve].endmessage, enclst[enchantindex].target );
  }

  // Check to see if we spawn a poof
  if ( evelst[eve].poofonend )
  {
    spawn_poof( gs, enclst[enchantindex].target, eve );
  }
  // Check to see if the character dies
  if ( evelst[eve].killonend )
  {
    if ( chrlst[character].invictus )  gs->TeamList[chrlst[character].baseteam].morale++;
    chrlst[character].invictus = bfalse;
    kill_character( gs, character, MAXCHR );
  }
  // Kill overlay too...
  overlay = enclst[enchantindex].overlay;
  if ( VALID_CHR(chrlst, overlay) )
  {
    if ( chrlst[overlay].invictus )  gs->TeamList[chrlst[overlay].baseteam].morale++;
    chrlst[overlay].invictus = bfalse;
    kill_character( gs, overlay, MAXCHR );
  }



  // Now get rid of it
  enclst[enchantindex].on = bfalse;
  gs->EncFreeList[gs->EncFreeList_count] = enchantindex;
  gs->EncFreeList_count++;

  // Now fix dem weapons
  for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
  {
    reset_character_alpha( gs, chr_get_holdingwhich( chrlst, MAXCHR, character, _slot ) );
  }

  // And remove see kurse enchantment
  if(evelst[enchantindex].canseekurse && !caplst[character].canseekurse)
  {
    chrlst[character].canseekurse = bfalse;
  }


}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled( CGame * gs, Uint16 enchantindex, Uint8 valueindex )
{
  // ZZ> This function returns MAXENCHANT if the enchantment's target has no conflicting
  //     set values in its other enchantments.  Otherwise it returns the enchantindex
  //     of the conflicting enchantment

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  CHR_REF character;
  Uint16 currenchant;

  character = enclst[enchantindex].target;
  currenchant = chrlst[character].firstenchant;
  while ( currenchant != MAXENCHANT )
  {
    if ( enclst[currenchant].setyesno[valueindex] )
    {
      return currenchant;
    }
    currenchant = enclst[currenchant].nextenchant;
  }
  return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value( CGame * gs, Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype )
{
  // ZZ> This function sets and saves one of the ichr's stats

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 conflict, ichr;

  Eve * peve = evelst + enchanttype;
  Enc * penc = enclst + enchantindex;
  Chr * pchr;


  penc->setyesno[valueindex] = bfalse;
  if ( peve->setyesno[valueindex] )
  {
    conflict = enchant_value_filled( gs, enchantindex, valueindex );
    if ( conflict == MAXENCHANT || peve->override )
    {
      // Check for multiple enchantments
      if ( conflict < MAXENCHANT )
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
      pchr = chrlst + ichr;
      penc->setyesno[valueindex] = btrue;
      switch ( valueindex )
      {
        case SETDAMAGETYPE:
          penc->setsave[valueindex] = pchr->damagetargettype;
          pchr->damagetargettype = peve->setvalue[valueindex];
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
          pchr->bumpstrength = caplst[pchr->model].bumpstrength * FP8_TO_FLOAT( pchr->alpha_fp8 );
          break;

        case SETSHEEN:
          penc->setsave[valueindex] = pchr->sheen_fp8;
          pchr->sheen_fp8 = peve->setvalue[valueindex];
          break;

        case SETFLYTOHEIGHT:
          penc->setsave[valueindex] = pchr->flyheight;
          if ( pchr->flyheight == 0 && pchr->pos.z > -2 )
          {
            pchr->flyheight = peve->setvalue[valueindex];
          }
          break;

        case SETWALKONWATER:
          penc->setsave[valueindex] = pchr->waterwalk;
          if ( !pchr->waterwalk )
          {
            pchr->waterwalk = peve->setvalue[valueindex];
          }
          break;

        case SETCANSEEINVISIBLE:
          penc->setsave[valueindex] = pchr->canseeinvisible;
          pchr->canseeinvisible = peve->setvalue[valueindex];
          break;

        case SETMISSILETREATMENT:
          penc->setsave[valueindex] = pchr->missiletreatment;
          pchr->missiletreatment = peve->setvalue[valueindex];
          break;

        case SETCOSTFOREACHMISSILE:
          penc->setsave[valueindex] = pchr->missilecost;
          pchr->missilecost = peve->setvalue[valueindex];
          pchr->missilehandler = penc->owner;
          break;

        case SETMORPH:
          penc->setsave[valueindex] = pchr->skin_ref % MAXSKIN;
          // Special handler for morph
          change_character( gs, ichr, enchanttype, 0, LEAVE_ALL );
          pchr->aistate.morphed = btrue;
          break;

        case SETCHANNEL:
          penc->setsave[valueindex] = pchr->canchannel;
          pchr->canchannel = peve->setvalue[valueindex];
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
void add_enchant_value( CGame * gs, Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype )
{
  // ZZ> This function does cumulative modification to character stats

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  int valuetoadd, newvalue;
  float fvaluetoadd, fnewvalue;
  CHR_REF ichr;

  Chr * pchr;
  Eve * peve = evelst + enchantindex;
  Enc * penc = enclst + enchanttype;


  ichr = penc->target;
  if( !VALID_CHR( chrlst, ichr) ) return;
  pchr = chrlst + ichr;

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
      newvalue = pchr->manamax_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, HIGHSTAT, &valuetoadd );
      pchr->manamax_fp8 += valuetoadd;
      pchr->mana_fp8 += valuetoadd;
      if ( pchr->mana_fp8 < 0 )  pchr->mana_fp8 = 0;
      break;

    case ADDLIFE:
      newvalue = pchr->lifemax_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( LOWSTAT, newvalue, HIGHSTAT, &valuetoadd );
      pchr->lifemax_fp8 += valuetoadd;
      pchr->life_fp8 += valuetoadd;
      if ( pchr->life_fp8 < 1 )  pchr->life_fp8 = 1;
      break;

    case ADDSTRENGTH:
      newvalue = pchr->strength_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->strength_fp8 += valuetoadd;
      break;

    case ADDWISDOM:
      newvalue = pchr->wisdom_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->wisdom_fp8 += valuetoadd;
      break;

    case ADDINTELLIGENCE:
      newvalue = pchr->intelligence_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->intelligence_fp8 += valuetoadd;
      break;

    case ADDDEXTERITY:
      newvalue = pchr->dexterity_fp8;
      valuetoadd = peve->addvalue[valueindex] << 6;
      getadd( 0, newvalue, PERFECTSTAT, &valuetoadd );
      pchr->dexterity_fp8 += valuetoadd;
      break;
  }

  penc->addsave[valueindex] = valuetoadd;  // Save the value for undo
}


//--------------------------------------------------------------------------------------------
Uint16 spawn_enchant( CGame * gs, Uint16 owner, Uint16 target,
                      Uint16 spawner, ENC_REF enchantindex, Uint16 modeloptional )
{
  // ZZ> This function enchants a target, returning the enchantment index or MAXENCHANT
  //     if failed

  ENC_REF enc_ref = MAXENCHANT;

  Eve  * evelst      = gs->EveList;
  size_t evelst_size = MAXEVE;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Enc * penc;
  Eve * peve;

  EVE_REF eve_ref;
  CHR_REF overlay;
  int add, cnt;



  if ( MAXMODEL != modeloptional )
  {
    // The enchantment type is given explicitly
    eve_ref = modeloptional;
  }
  else
  {
    // The enchantment type is given by the spawner
    eve_ref = chrlst[spawner].model;
  }


  // Target and owner must both be alive and on and valid
  if ( VALID_CHR(chrlst, target) )
  {
    if ( !VALID_CHR( chrlst,  target ) || !chrlst[target].alive )
      return enc_ref;
  }
  else
  {
    // Invalid target
    return enc_ref;
  }
  if ( VALID_CHR(chrlst, owner) )
  {
    if ( !VALID_CHR( chrlst,  owner ) || !chrlst[owner].alive )
      return enc_ref;
  }
  else
  {
    // Invalid target
    return enc_ref;
  }


  if ( MAXEVE == eve_ref || !evelst[eve_ref].used ) return enc_ref;
  peve = evelst + eve_ref;

  enc_ref = enchantindex;
  if ( enc_ref == MAXENCHANT )
  {
    // Should it choose an inhand item?
    if ( peve->retarget )
    {
      bool_t bfound = bfalse;
      SLOT best_slot = SLOT_BEGIN;

      // Is at least one valid?
      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        if ( chr_using_slot( chrlst, MAXCHR, target, _slot ) )
        {
          bfound = btrue;
          if ( _slot > best_slot ) best_slot = _slot;  // choose SLOT_RIGHT above SLOT_LEFT
          break;
        }
      };

      if ( !bfound ) return enc_ref;

      target = chr_get_holdingwhich( chrlst, MAXCHR, target, best_slot );
    }


    // Make sure it's valid
    if ( peve->dontdamagetype != DAMAGE_NULL )
    {
      if (( chrlst[target].skin.damagemodifier_fp8[peve->dontdamagetype]&7 ) >= 3 )   // Invert | Shift = 7
      {
        return enc_ref;
      }
    }
    if ( peve->onlydamagetype != DAMAGE_NULL )
    {
      if ( chrlst[target].damagetargettype != peve->onlydamagetype )
      {
        return enc_ref;
      }
    }

    // Find one to use
    enc_ref = EncList_get_free(gs);
  }
  else
  {
    gs->EncFreeList_count--;  // To keep it in order
  }

  if(MAXENCHANT == enc_ref) return MAXENCHANT;
  penc = enclst + enc_ref;

  // Make a new one
  penc->on = btrue;
  penc->target = target;
  penc->owner = owner;
  penc->spawner = spawner;
  if ( VALID_CHR(chrlst, spawner) )
  {
    chrlst[spawner].undoenchant = enc_ref;
  }
  penc->eve            = eve_ref;
  penc->time           = peve->time;
  penc->spawntime      = 1;
  penc->ownermana_fp8  = peve->ownermana_fp8;
  penc->ownerlife_fp8  = peve->ownerlife_fp8;
  penc->targetmana_fp8 = peve->targetmana_fp8;
  penc->targetlife_fp8 = peve->targetlife_fp8;

  // Add it as first in the list
  penc->nextenchant = chrlst[target].firstenchant;
  chrlst[target].firstenchant      = enc_ref;


  // Now set all of the specific values, morph first
  set_enchant_value( gs, enc_ref, SETMORPH, eve_ref );
  for ( cnt = SETDAMAGETYPE; cnt <= SETCOSTFOREACHMISSILE; cnt++ )
  {
    set_enchant_value( gs, enc_ref, cnt, eve_ref );
  }
  set_enchant_value( gs, enc_ref, SETCHANNEL, eve_ref );


  // Now do all of the stat adds
  add = 0;
  while ( add < EVE_ADD_COUNT )
  {
    add_enchant_value( gs, enc_ref, add, eve_ref );
    add++;
  }

  //Enchant to allow see kurses?
  chrlst[target].canseekurse = chrlst[target].canseekurse || evelst[enc_ref].canseekurse;


  // Create an overlay character?
  penc->overlay = MAXCHR;
  if ( peve->overlay )
  {
    overlay = spawn_one_character( gs, chrlst[target].pos, eve_ref, chrlst[target].team, 0, chrlst[target].turn_lr, NULL, MAXCHR );
    if ( VALID_CHR(chrlst, overlay) )
    {
      penc->overlay = overlay;  // Kill this character on end...
      chrlst[overlay].aistate.target = target;
      chrlst[overlay].aistate.state = peve->overlay;
      chrlst[overlay].overlay = btrue;

      // Start out with ActionMJ...  Object activated
      if ( gs->MadList[chrlst[overlay].model].actionvalid[ACTION_MJ] )
      {
        chrlst[overlay].action.now = ACTION_MJ;
        chrlst[overlay].anim.ilip = 0;
        chrlst[overlay].anim.flip = 0.0f;
        chrlst[overlay].anim.next = gs->MadList[chrlst[overlay].model].actionstart[ACTION_MJ];
        chrlst[overlay].anim.last = chrlst[overlay].anim.next;
        chrlst[overlay].action.ready = bfalse;
      }

      chrlst[overlay].light_fp8 = 254;  // Assume it's transparent...
    }
  }


  return enc_ref;
}
