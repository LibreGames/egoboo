//********************************************************************************************
//* Egoboo - char.c
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

#include "char.inl"

#include "Network.h"
#include "Client.h"
#include "Server.h"
#include "Log.h"
#include "camera.h"
#include "enchant.h"
#include "passage.h"
#include "Menu.h"
#include "script.h"
#include "mesh.h"
#include "graphic.h"
#include "sound.h"

#include "egoboo_strutil.h"
#include "egoboo_utility.h"
#include "egoboo_rpc.h"
#include "egoboo.h"

#include <assert.h>

#include "object.inl"
#include "Physics.inl"
#include "input.inl"
#include "particle.inl"
#include "Md2.inl"
#include "mesh.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"

// pair-wise collision data
struct sCoData
{
  CHR_REF chra, chrb;
  PRT_REF prtb;
};

typedef struct sCoData CoData_t;

HashList_t * CoList;

int chr_collisions = 0;

static CHR_REF _chr_spawn( CHR_SPAWN_INFO si, bool_t activate );


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct sChrEnvironment
{
  egoboo_key_t ekey;

  bool_t  grounded;
  bool_t  flying;
  bool_t  inwater;

  float   horiz_friction;
  float   vert_friction;
  float   flydampen;
  float   traction;

  float   level;
  Uint8   twist;
  vect3   nrm;
  float   air_traction;
  float   buoyancy;

  PhysicsData_t phys;

};

typedef struct sChrEnvironment ChrEnviro_t;

ChrEnviro_t * CChrEnviro_new( ChrEnviro_t * cphys, PhysicsData_t * gphys);
bool_t       CChrEnviro_delete( ChrEnviro_t * cphys );
ChrEnviro_t * CChrEnviro_renew( ChrEnviro_t * cphys, PhysicsData_t * gphys);
bool_t       CChrEnviro_init( ChrEnviro_t * cphys, float dUpdate);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SLOT    _slot;

Uint16  numdolist = 0;
CHR_REF  dolist[CHRLST_COUNT];

Uint16  chrcollisionlevel = 2;

TILE_DAMAGE GTile_Dam;

//--------------------------------------------------------------------------------------------
Uint32  cv_list_count = 0;
CVolume_t cv_list[1000];

INLINE void cv_list_add( CVolume_t * cv);
INLINE void cv_list_clear();
INLINE void cv_list_draw();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void flash_character_height( Game_t * gs, CHR_REF chr_ref, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
  // ZZ> This function sets a chr_ref's lighting depending on vertex height...
  //     Can make feet dark and head light...

  int cnt;
  float z, flip;

  Obj_t * pobj;
  Mad_t  * pmad;
  Chr_t * pchr;

  Uint32 ilast, inext;
  MD2_Model_t * pmdl;
  const MD2_Frame_t * plast, * pnext;

  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return;

  inext = pchr->anim.next;
  ilast = pchr->anim.last;
  flip  = pchr->anim.flip;

  pobj = ChrList_getPObj(gs, chr_ref);
  if(NULL == pobj) return;

  pmad = ChrList_getPMad(gs, chr_ref);
  if(NULL == pmad) return;

  pmdl  = pmad->md2_ptr;
  plast = md2_get_Frame(pmdl, ilast);
  pnext = md2_get_Frame(pmdl, inext);

  for ( cnt = 0; cnt < pmad->transvertices; cnt++ )
  {
    z = pnext->vertices[cnt].z + (pnext->vertices[cnt].z - plast->vertices[cnt].z) * flip;

    if ( z < low )
    {
      pchr->vrta_fp8[cnt].r =
      pchr->vrta_fp8[cnt].g =
      pchr->vrta_fp8[cnt].b = valuelow;
    }
    else if ( z > high )
    {
      pchr->vrta_fp8[cnt].r =
      pchr->vrta_fp8[cnt].g =
      pchr->vrta_fp8[cnt].b = valuehigh;
    }
    else
    {
      float ftmp = (float)( z - low ) / (float)( high - low );
      pchr->vrta_fp8[cnt].r =
      pchr->vrta_fp8[cnt].g =
      pchr->vrta_fp8[cnt].b = valuelow + (valuehigh - valuelow) * ftmp;
    }
  }
}

//--------------------------------------------------------------------------------------------
void flash_character( Game_t * gs, CHR_REF chr_ref, Uint8 value )
{
  // ZZ> This function sets a chr_ref's lighting

  PChr_t chrlst      = gs->ChrList;
  PMad_t madlst      = gs->MadList;

  Obj_t * pobj;
  Mad_t  * pmad;
  Chr_t * pchr;

  int cnt;

  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return;

  pobj = ChrList_getPObj(gs, chr_ref);
  if(NULL == pobj) return;

  pmad = ChrList_getPMad(gs, chr_ref);
  if(NULL == pmad) return;

  for ( cnt = 0; cnt < pmad->transvertices; cnt++ )
  {
    pchr->vrta_fp8[cnt].r =
    pchr->vrta_fp8[cnt].g =
    pchr->vrta_fp8[cnt].b = value;
  }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders(Game_t * gs)
{
  // ZZ> This function keeps weapons near their holders

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;

  CHR_REF chr_cnt;
  CHR_REF holder_ref;

  Cap_t * pcap;
  Chr_t * pchr, *pholder;

  // !!!BAD!!!  May need to do 3 levels of attachment...
  for ( chr_cnt = 0; chr_cnt < chrlst_size; chr_cnt++ )
  {
    pchr = ChrList_getPChr(gs, chr_cnt);
    if(NULL == pchr) continue;

    pcap = ChrList_getPCap(gs, chr_cnt);

    holder_ref = chr_get_attachedto( chrlst, chrlst_size, chr_cnt );
    if ( !ACTIVE_CHR( chrlst, holder_ref ) )
    {
      // Keep inventory with character
      if ( !chr_in_pack( chrlst, chrlst_size, chr_cnt ) )
      {
        holder_ref = chr_get_nextinpack( chrlst, chrlst_size, chr_cnt );
        while ( ACTIVE_CHR( chrlst, holder_ref ) )
        {
          Chr_t * pinven   = ChrList_getPChr(gs, holder_ref);
          pinven->ori.pos = pchr->ori.pos;
          pinven->ori_old = pchr->ori_old;  // Copy olds to make SendMessageNear work
          holder_ref      = chr_get_nextinpack( chrlst, chrlst_size, holder_ref );
        }
      }
    }
    else
    {
      pholder = ChrList_getPChr(gs, holder_ref);

      // Keep in hand weapons with character
      if ( pholder->matrix_valid && pchr->matrix_valid )
      {
        pchr->ori.pos.x = pchr->matrix.CNV( 3, 0 );
        pchr->ori.pos.y = pchr->matrix.CNV( 3, 1 );
        pchr->ori.pos.z = pchr->matrix.CNV( 3, 2 );
      }
      else
      {
        pchr->ori.pos = pholder->ori.pos;
      }
      pchr->ori.turn_lr = pholder->ori.turn_lr;

      // Copy this stuff ONLY if it's a weapon, not for mounts
      if ( pholder->transferblend && pchr->prop.isitem )
      {
        if ( pholder->alpha_fp8 != 255 )
        {
          pchr->alpha_fp8 = pholder->alpha_fp8;
          pchr->bumpstrength = pcap->bumpstrength * FP8_TO_FLOAT( pchr->alpha_fp8 );
        }

        if ( pholder->light_fp8 != 255 )
        {
          pchr->light_fp8 = pholder->light_fp8;
        }
      }
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t make_one_character_matrix( ChrList_t chrlst, size_t chrlst_size, Chr_t * pchr )
{
  // ZZ> This function sets one character's matrix

  Chr_t * povl;

  CHR_REF chr_tnc;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  if ( !EKEY_PVALID(pchr) ) return bfalse;

  mat_old = pchr->matrix;
  pchr->matrix_valid = bfalse;

  if ( pchr->overlay )
  {
    // Overlays are kept with their target...
    chr_tnc = chr_get_aitarget( chrlst, chrlst_size, pchr );

    if ( ACTIVE_CHR( chrlst, chr_tnc ) )
    {
      povl = chrlst + chr_tnc;

      pchr->ori.pos.x = povl->matrix.CNV( 3, 0 );
      pchr->ori.pos.y = povl->matrix.CNV( 3, 1 );
      pchr->ori.pos.z = povl->matrix.CNV( 3, 2 );

      pchr->matrix = povl->matrix;

      pchr->matrix.CNV( 0, 0 ) *= pchr->pancakepos.x;
      pchr->matrix.CNV( 1, 0 ) *= pchr->pancakepos.x;
      pchr->matrix.CNV( 2, 0 ) *= pchr->pancakepos.x;

      pchr->matrix.CNV( 0, 1 ) *= pchr->pancakepos.y;
      pchr->matrix.CNV( 1, 1 ) *= pchr->pancakepos.y;
      pchr->matrix.CNV( 2, 1 ) *= pchr->pancakepos.y;

      pchr->matrix.CNV( 0, 2 ) *= pchr->pancakepos.z;
      pchr->matrix.CNV( 1, 2 ) *= pchr->pancakepos.z;
      pchr->matrix.CNV( 2, 2 ) *= pchr->pancakepos.z;

      pchr->matrix_valid = btrue;

      recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
    }
  }
  else
  {
    pchr->matrix = ScaleXYZRotateXYZTranslate( pchr->scale * pchr->pancakepos.x, pchr->scale * pchr->pancakepos.y, pchr->scale * pchr->pancakepos.z,
                     pchr->ori.turn_lr,
                     ( Uint16 )( pchr->ori.mapturn_ud + 32768 ),
                     ( Uint16 )( pchr->ori.mapturn_lr + 32768 ),
                     pchr->ori.pos.x, pchr->ori.pos.y, pchr->ori.pos.z );

    pchr->matrix_valid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
  }

  if(pchr->matrix_valid && recalc_bumper)
  {
    // invalidate the cached bumper data
    pchr->bmpdata.cv.lod = -1;
    pchr->vdata.needs_lighting = btrue;
  };

  return pchr->matrix_valid;
}

//--------------------------------------------------------------------------------------------
void ChrList_free_one( Game_t * gs, CHR_REF chr_ref )
{
  // ZZ> This function sticks a character back on the free character stack

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;

  CHR_REF chr_cnt;

  Chr_t * pchr;
  Cap_t * pcap;

  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return;

  pcap = ChrList_getPCap(gs, chr_ref);
  if(NULL == pcap) return;

  log_debug( "ChrList_free_one() - \n\tprofile == %d, pcap->classname == \"%s\", index == %d\n", pchr->model, pcap->classname, chr_ref );

  // Make sure everyone knows it died
  for ( chr_cnt = 0; chr_cnt < chrlst_size; chr_cnt++ )
  {
    Chr_t * ptmp = ChrList_getPChr(gs, chr_cnt);
    if ( NULL == ptmp ) continue;

    if ( ptmp->aistate.target == chr_ref )
    {
      ptmp->aistate.alert |= ALERT_TARGETKILLED;
      ptmp->aistate.target = chr_cnt;
    }
    if ( team_get_leader( gs, ptmp->team ) == chr_ref )
    {
      ptmp->aistate.alert |= ALERT_LEADERKILLED;
    }
  }

  // fix the team
  if ( pchr->alive && !pcap->prop.invictus )
  {
    gs->TeamList[pchr->team_base].morale--;
  }

  if ( team_get_leader( gs, pchr->team ) == chr_ref )
  {
    gs->TeamList[pchr->team].leader = INVALID_CHR;
  }

  // deallocate the character
  Chr_delete(pchr);

  // Remove from stat list
  if ( pchr->staton )
  {
    remove_stat( gs, pchr );
  }

  // add it to the free list
  gs->ChrFreeList[gs->ChrFreeList_count] = chr_ref;
  gs->ChrFreeList_count++;
}

//--------------------------------------------------------------------------------------------
void chr_free_inventory( ChrList_t chrlst, size_t chrlst_size, CHR_REF chr_ref )
{
  // ZZ> This function frees every item in the character's inventory

  CHR_REF cnt;

  cnt  = chr_get_nextinpack( chrlst, chrlst_size, chr_ref );
  while ( cnt < chrlst_size )
  {
    chrlst[cnt].freeme = btrue;
    cnt = chr_get_nextinpack( chrlst, chrlst_size, cnt );
  }
}

//--------------------------------------------------------------------------------------------
bool_t make_one_weapon_matrix( ChrList_t chrlst, size_t chrlst_size, CHR_REF chr_ref )
{
  // ZZ> This function sets one weapon's matrix, based on who it's attached to

  int cnt;
  Chr_t * pchr;
  CHR_REF mount_ref;
  Uint16 vertex;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  // check this character
  pchr = chrlst + chr_ref;

  // invalidate the matrix
  pchr->matrix_valid = bfalse;

  // check that the mount is valid
  mount_ref = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
  if ( !ACTIVE_CHR( chrlst, mount_ref ) )
  {
    pchr->matrix = ZeroMatrix();
    return bfalse;
  }

  mat_old = pchr->matrix;

  if(0xFFFF == pchr->attachedgrip[0])
  {
    // Calculate weapon's matrix
    pchr->matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, pchr->ori.turn_lr + chrlst[mount_ref].ori.turn_lr, chrlst[mount_ref].ori.pos.x, chrlst[mount_ref].ori.pos.y, chrlst[mount_ref].ori.pos.z);
    pchr->matrix_valid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
  }
  else if(0xFFFF == pchr->attachedgrip[1])
  {
    // do the linear interpolation
    vertex = pchr->attachedgrip[0];
    md2_blend_vertices(chrlst + mount_ref, vertex, vertex);

    // Calculate weapon's matrix
    pchr->matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, pchr->ori.turn_lr + chrlst[mount_ref].ori.turn_lr, chrlst[mount_ref].vdata.Vertices[vertex].x, chrlst[mount_ref].vdata.Vertices[vertex].y, chrlst[mount_ref].vdata.Vertices[vertex].z);
    pchr->matrix_valid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
  }
  else
  {
    GLvector point[GRIP_SIZE], nupoint[GRIP_SIZE];

    // do the linear interpolation
    vertex = pchr->attachedgrip[0];
    md2_blend_vertices(chrlst + mount_ref, vertex, vertex+GRIP_SIZE);

    for ( cnt = 0; cnt < GRIP_SIZE; cnt++ )
    {
      point[cnt].x = chrlst[mount_ref].vdata.Vertices[vertex+cnt].x;
      point[cnt].y = chrlst[mount_ref].vdata.Vertices[vertex+cnt].y;
      point[cnt].z = chrlst[mount_ref].vdata.Vertices[vertex+cnt].z;
      point[cnt].w = 1.0f;
    };

    // Do the transform
    Transform4_Full( 1.0f, 1.0f, &(chrlst[mount_ref].matrix), point, nupoint, GRIP_SIZE );

    // Calculate weapon's matrix based on positions of grip points
    // chrscale is recomputed at time of attachment
    pchr->matrix = FourPoints( nupoint[0], nupoint[1], nupoint[2], nupoint[3], 1.0 );
    pchr->ori.pos.x = (pchr->matrix).CNV(3,0);
    pchr->ori.pos.y = (pchr->matrix).CNV(3,1);
    pchr->ori.pos.z = (pchr->matrix).CNV(3,2);
    pchr->matrix_valid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
  };

  if(pchr->matrix_valid && recalc_bumper)
  {
    // invalidate the cached bumper data
    pchr->bmpdata.cv.lod = -1;
    pchr->vdata.needs_lighting = btrue;
  };

  return pchr->matrix_valid;
}

//--------------------------------------------------------------------------------------------
void make_character_matrices(Game_t * gs)
{
  // ZZ> This function makes all of the character's matrices

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CHR_REF chr_ref;
  bool_t  bfinished;

  // Forget about old matrices
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    chrlst[chr_ref].matrix_valid = bfalse;
  }

  // Do base characters
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    CHR_REF attached_ref;
    if ( !ACTIVE_CHR( chrlst, chr_ref ) ) continue;

    attached_ref = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
    if ( ACTIVE_CHR( chrlst, attached_ref ) ) continue;  // Skip weapons for now

    make_one_character_matrix( chrlst, chrlst_size, chrlst + chr_ref );
  }

  // Do all other levels of attachment
  bfinished = bfalse;
  while ( !bfinished )
  {
    bfinished = btrue;
    for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
    {
      CHR_REF attached_ref;
      if ( chrlst[chr_ref].matrix_valid || !ACTIVE_CHR( chrlst, chr_ref ) ) continue;

      attached_ref = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
      if ( !ACTIVE_CHR( chrlst, attached_ref ) ) continue;

      if ( !chrlst[attached_ref].matrix_valid )
      {
        bfinished = bfalse;
        continue;
      }

      make_one_weapon_matrix( chrlst, chrlst_size, chr_ref );
    }
  };

  // recalculate the basic bounding boxes for any characters that moved
  recalc_character_bumpers( gs );

}

//--------------------------------------------------------------------------------------------
CHR_REF ChrList_get_free( Game_t * gs, CHR_REF irequest )
{
  // ZZ> This function gets an unused character and returns its index

  CHR_REF retval;
  int tnc;

  // Get the first free index
  retval = INVALID_CHR;
  if ( gs->ChrFreeList_count > 0 )
  {
    // Just grab the next one
    gs->ChrFreeList_count--;
    retval = gs->ChrFreeList[gs->ChrFreeList_count];
  }

  if ( INVALID_CHR != irequest )
  {
    if ( retval != irequest )
    {
      // Picked the wrong one, so put this one back and find the right one
      for ( tnc = 0; tnc < gs->ChrFreeList_count; tnc++ )
      {
        if ( gs->ChrFreeList[tnc] == irequest )
        {
          gs->ChrFreeList[tnc] = retval;
          break;
        }
      }
      retval = irequest;
    }

    if ( retval != irequest )
    {
      log_debug( "WARNING: req_chr_spawn_one() - failed to spawn : cannot find irequest index %d\n", irequest );
      return INVALID_CHR;
    }
  }


  if ( INVALID_CHR == retval )
  {
    log_debug( "WARNING: ChrList_get_free() - could not get valid character\n");
    return INVALID_CHR;
  }

  return retval;

}

//--------------------------------------------------------------------------------------------
Chr_t * Chr_new(Chr_t * pchr)
{
  // BB > initialize the Chr_t data structure with safe values

  AI_STATE * pstate;

  //fprintf( stdout, "Chr_new()\n");

  if(NULL == pchr) return pchr;

  Chr_delete(pchr);

  // initialize the data
  memset(pchr, 0, sizeof(Chr_t));

  EKEY_PNEW( pchr, Chr_t );

  pstate = &(pchr->aistate);

  // IMPORTANT!!!
  pchr->sparkle = NOSPARKLE;
  pchr->missilehandler = INVALID_CHR;
  pchr->missiletreatment = MIS_NORMAL;

  // Set up model stuff
  pchr->inwhichslot = SLOT_NONE;
  pchr->inwhichpack = INVALID_CHR;
  pchr->nextinpack = INVALID_CHR;
  VData_Blended_construct( &(pchr->vdata) );

  pchr->whichplayer = INVALID_PLA;

  pchr->model_base  = INVALID_OBJ;
  pchr->stoppedby   = MPDFX_WALL | MPDFX_IMPASS;
  pchr->boretime    = 375;
  pchr->carefultime = DELAY_CAREFUL;

  //Ready for loop sound
  pchr->loopingchannel = INVALID_CHANNEL;

  // Enchant stuff
  pchr->firstenchant = INVALID_ENC;
  pchr->undoenchant = INVALID_ENC;

  // Gender
  pchr->gender = GEN_OTHER;

  // Team stuff
  pchr->team = TEAM_NULL;
  pchr->team_base = TEAM_NULL;

  // Life and Mana
  pchr->lifecolor = 0;
  pchr->manacolor = 1;

  // Jumping
  pchr->jumpready = btrue;
  pchr->jumpnumber = 1;
  pchr->jumptime = DELAY_JUMP;

  // Other junk

  // Character size and bumping
  pchr->fat = 1;
  pchr->sizegoto = pchr->fat;
  pchr->sizegototime = 0;

  pchr->bumpdampen = 0.1;

  // Grip info
  pchr->inwhichslot = SLOT_NONE;
  pchr->attachedto  = INVALID_CHR;
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    pchr->holdingwhich[_slot] = INVALID_CHR;
  }

  // Movement
  pchr->spd_sneak = 1;
  pchr->spd_walk  = 1;
  pchr->spd_run   = 1;

  // Set up position
  pchr->ori.mapturn_lr = 32768;  // These two mean on level surface
  pchr->ori.mapturn_ud = 32768;
  pchr->scale = pchr->fat;

  pchr->onwhichplatform = INVALID_CHR;

  // Name the character
  strncpy( pchr->name, "NONE", sizeof( pchr->name ) );

  pchr->pancakepos.x = pchr->pancakepos.y = pchr->pancakepos.z = 1.0;
  pchr->pancakevel.x = pchr->pancakevel.y = pchr->pancakevel.z = 0.0f;

  pchr->loopingchannel = INVALID_CHANNEL;

  // calculate the bumpers
  assert(NULL == pchr->bmpdata.cv_tree);
  chr_bdata_reinit( pchr, &(pchr->bmpdata) );
  pchr->matrix = IdentityMatrix();
  pchr->matrix_valid = bfalse;

  // ai stuff
  ai_state_new( pstate );

  // action stuff
  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  // set the default properties
  CProperties_new( &(pchr->prop) );

  // initialize a non-existant collision volume octree
  BData_new( &(pchr->bmpdata) );

  return pchr;
}

//--------------------------------------------------------------------------------------------
bool_t Chr_delete(Chr_t * pchr)
{
  int i;

  if(NULL ==pchr) return bfalse;
  if(!EKEY_PVALID(pchr))  return btrue;

  EKEY_PINVALIDATE( pchr );

  // reset some values
  pchr->alive = bfalse;
  pchr->staton = bfalse;
  pchr->matrix_valid = bfalse;
  pchr->model = INVALID_OBJ;
  VData_Blended_Deallocate(&(pchr->vdata));
  pchr->name[0] = EOS;

  // invalidate pack
  pchr->numinpack = 0;
  pchr->inwhichpack = INVALID_CHR;
  pchr->nextinpack = INVALID_CHR;

  // invalidate attachmants
  pchr->inwhichslot = SLOT_NONE;
  pchr->attachedto = INVALID_CHR;
  for ( i = 0; i < SLOT_COUNT; i++ )
  {
    pchr->holdingwhich[i] = INVALID_CHR;
  };

  // remove existing collision volume octree
  BData_delete( &(pchr->bmpdata) );

  // deallocate any vertex data
  VData_Blended_Deallocate(&(pchr->vdata));

  // silence all looping sounds
  if( INVALID_CHANNEL != pchr->loopingchannel )
  {
    snd_stop_sound( pchr->loopingchannel );
    pchr->loopingchannel = INVALID_CHANNEL;
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
Chr_t * Chr_renew(Chr_t * pchr)
{
  if(NULL == pchr) return pchr;

  Chr_delete(pchr);
  return Chr_new(pchr);
}

//--------------------------------------------------------------------------------------------
Uint32 chr_hitawall( Game_t * gs, Chr_t * pchr, vect3 * norm )
{
  // ZZ> This function returns nonzero if the character hit a wall that the
  //     chr_ref is not allowed to cross

  Uint32 retval;
  vect3  pos, size, test_norm, tmp_norm;

  Mesh_t * pmesh = Game_getMesh(gs);

  if ( !EKEY_PVALID(pchr) || 0.0f == pchr->bumpstrength ) return 0;

  VectorClear( tmp_norm.v );

  pos.x = ( pchr->bmpdata.cv.x_max + pchr->bmpdata.cv.x_min ) * 0.5f;
  pos.y = ( pchr->bmpdata.cv.y_max + pchr->bmpdata.cv.y_min ) * 0.5f;
  pos.z =   pchr->bmpdata.cv.z_min;

  size.x = ( pchr->bmpdata.cv.x_max - pchr->bmpdata.cv.x_min ) * 0.5f;
  size.y = ( pchr->bmpdata.cv.y_max - pchr->bmpdata.cv.y_min ) * 0.5f;
  size.z = ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min ) * 0.5f;

  retval = mesh_hitawall( pmesh, pos, size.x, size.y, pchr->stoppedby, NULL );
  test_norm.z = 0;

  if(0 != retval)
  {
    vect3 diff, pos2;

    diff.x = pchr->ori.pos.x - pchr->ori_old.pos.x;
    diff.y = pchr->ori.pos.y - pchr->ori_old.pos.y;
    diff.z = pchr->ori.pos.z - pchr->ori_old.pos.z;

    pos2.x = pos.x - diff.x;
    pos2.y = pos.y - diff.y;
    pos2.z = pos.z - diff.z;

    if( 0 != mesh_hitawall( pmesh, pos2, size.x, size.y, pchr->stoppedby, NULL ) )
    {
      // this is an "invalid" object. It's old position is also *inside* the wall
      // this could hapen if the object spawns inside a blocked region, if a script
      // turns a region off, or something silly like that. DO NOTHING

      if( NULL != norm && 0.0f == ABS(tmp_norm.x) + ABS(tmp_norm.y) + ABS(tmp_norm.z))
      {
        // !absolutely nothing worked!
        retval = 0;
        norm->x = 0.0f;
        norm->y = 0.0f;
        norm->z = 1.0f;
      }

      return 0;
    }

    // check for a wall in the y-z plane
    pos2.x = pos.x - diff.x;
    pos2.y = pos.y;
    pos2.z = pos.z;
    if( 0 == mesh_hitawall( pmesh, pos2, size.x, size.y, pchr->stoppedby, NULL ) )
    {
      tmp_norm.x += -diff.x;
    }

    // check for a wall in the x-z plane
    pos2.x = pos.x;
    pos2.y = pos.y - diff.y;
    pos2.z = pos.z;
    if( 0 == mesh_hitawall( pmesh, pos2, size.x, size.y, pchr->stoppedby, NULL ) )
    {
      tmp_norm.y += -diff.y;
    }

    // if the "simple" method doesn't work, then just bounce?
    if( ABS(tmp_norm.x) + ABS(tmp_norm.y) + ABS(tmp_norm.z) == 0.0f)
    {
      tmp_norm.x = -diff.x;
      tmp_norm.y = -diff.y;
    }

    // if the "simple" method doesn't work, then just bounce?
    if( ABS(tmp_norm.x) + ABS(tmp_norm.y) + ABS(tmp_norm.z) == 0.0f)
    {
      tmp_norm.x = -pchr->ori.vel.x;
      tmp_norm.y = -pchr->ori.vel.y;
    }

    // as an absolute last resort, use the average normal of the "wall" collision
    // likely not to work, since a lot of "walls" are just closed doorways...
    if( ABS(tmp_norm.x) + ABS(tmp_norm.y) + ABS(tmp_norm.z) == 0.0f)
    {
      tmp_norm.x = test_norm.x;
      tmp_norm.y = test_norm.y;
    }

  }

  if(NULL != norm)
  {
    if( ABS(tmp_norm.x) + ABS(tmp_norm.y) + ABS(tmp_norm.z) == 0.0f)
    {
      // !absolutely nothing worked!
      retval = 0;
      norm->x = 0.0f;
      norm->y = 0.0f;
      norm->z = 1.0f;
    }
    else
    {
      *norm = Normalize( tmp_norm );
    };
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void chr_reset_accel( Game_t * gs, CHR_REF chr_ref )
{
  // ZZ> This function fixes a character's MAX acceleration

  PChr_t chrlst      = gs->ChrList;
  PCap_t caplst      = gs->CapList;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  ENC_REF enchant;

  if ( !ACTIVE_CHR( chrlst, chr_ref ) ) return;

  // Okay, remove all acceleration enchants
  enchant = chrlst[chr_ref].firstenchant;
  while ( enchant < enclst_size )
  {
    remove_enchant_value( gs, enchant, ADDACCEL );
    enchant = enclst[enchant].nextenchant;
  }

  // Set the starting value
  assert( NULL != ChrList_getPMad(gs, chr_ref) );
  chrlst[chr_ref].skin.maxaccel = ChrList_getPCap(gs, chr_ref)->skin[chrlst[chr_ref].skin_ref % MAXSKIN].maxaccel;

  // Put the acceleration enchants back on
  enchant = chrlst[chr_ref].firstenchant;
  while ( enchant < enclst_size )
  {
    add_enchant_value( gs, enchant, ADDACCEL, enclst[enchant].eve );
    enchant = enclst[enchant].nextenchant;
  }

}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_mount( Game_t * gs, CHR_REF chr_ref, bool_t ignorekurse, bool_t doshop )
{
  // ZZ> This function drops an item
  Uint32 loc_rand;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  //size_t caplst_size = CAPLST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  CHR_REF imount, iowner = INVALID_CHR;
  ENC_REF enchant;
  PASS_REF passage;
  Uint16 cnt, price;
  bool_t inshop;

  Chr_t * pchr, * pmount;
  Cap_t * pcap;

  // Make sure the chr_ref is valid
  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return bfalse;

  pcap = ChrList_getPCap(gs, chr_ref);
  if(NULL == pcap) return bfalse;

  // Make sure the chr_ref is mounted
  imount = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
  if ( !ACTIVE_CHR( chrlst, imount ) ) return bfalse;
  pmount = ChrList_getPChr(gs, imount);

  // Don't allow living characters to drop kursed weapons
  if ( !ignorekurse && pchr->prop.iskursed && pmount->alive )
  {
    pchr->aistate.alert |= ALERT_NOTDROPPED;
    return bfalse;
  }

  loc_rand = gs->randie_index;

  // Rip 'em apart
  _slot = pchr->inwhichslot;
  if(_slot == SLOT_INVENTORY)
  {
    pchr->attachedto = INVALID_CHR;
    pchr->inwhichslot = SLOT_NONE;
  }
  else
  {
    assert(_slot != SLOT_NONE);
    assert(chr_ref == pmount->holdingwhich[_slot]);
    pchr->attachedto = INVALID_CHR;
    pchr->inwhichslot = SLOT_NONE;
    pmount->holdingwhich[_slot] = INVALID_CHR;
  }

  pchr->scale = pchr->fat; // * madlst[pchr->model].scale * 4;

  // Run the falling animation...
  play_action( gs, chr_ref, (ACTION)(ACTION_JB + ( pchr->inwhichslot % 2 )), bfalse );

  // Set the positions
  if ( pchr->matrix_valid )
  {
    pchr->ori.pos.x = pchr->matrix.CNV( 3, 0 );
    pchr->ori.pos.y = pchr->matrix.CNV( 3, 1 );
    pchr->ori.pos.z = pchr->matrix.CNV( 3, 2 );
  }
  else
  {
    pchr->ori.pos = pmount->ori.pos;
  }

  // Make sure it's not dropped in a wall...
  if ( 0 != chr_hitawall( gs, chrlst + chr_ref, NULL ) )
  {
    pchr->ori.pos.x = pmount->ori.pos.x;
    pchr->ori.pos.y = pmount->ori.pos.y;
  }

  // Check for shop passages
  inshop = bfalse;
  if ( pchr->prop.isitem && gs->ShopList_count != 0 && doshop )
  {
    for ( cnt = 0; cnt < gs->ShopList_count; cnt++ )
    {
      passage = gs->ShopList[cnt].passage;

      if ( passage_check_any( gs, chr_ref, passage, NULL ) )
      {
        iowner = gs->ShopList[passage].owner;
        inshop = ( NOOWNER != iowner );
        break;
      }
    }

    if ( doshop && inshop )
    {
      OBJ_REF model = pchr->model;

      assert( NULL != ChrList_getPMad(gs, chr_ref) );

      // Give the imount its money back, alert the shop iowner
      price = pcap->skin[pchr->skin_ref % MAXSKIN].cost;
      if ( pcap->prop.isstackable )
      {
        price *= pchr->ammo;
      }
      pmount->money += price;
      chrlst[iowner].money -= price;
      if ( chrlst[iowner].money < 0 )  chrlst[iowner].money = 0;
      if ( pmount->money > MAXMONEY )  pmount->money = MAXMONEY;

      chrlst[iowner].aistate.alert |= ALERT_SIGNALED;
      chrlst[iowner].message.type = SIGNAL_BUY;      // 0 for buying an item
      chrlst[iowner].message.data = price;  // Tell iowner how much...
    }
  }

  // Make sure it works right
  pchr->hitready = btrue;
  pchr->aistate.alert   |= ALERT_DROPPED;
  if ( inshop )
  {
    // Drop straight down to avoid theft
    pchr->ori.vel.x = 0;
    pchr->ori.vel.y = 0;
  }
  else
  {
    Uint16 sin_dir = IRAND(&loc_rand, 15);
    pchr->accum.vel.x += pmount->ori.vel.x + 0.5 * DROPXYVEL * turntocos[sin_dir>>2];
    pchr->accum.vel.y += pmount->ori.vel.y + 0.5 * DROPXYVEL * turntosin[sin_dir>>2];
  }
  pchr->accum.vel.z += DROPZVEL;

  // Turn looping off
  pchr->action.loop = bfalse;

  // Reset the team if it is a imount
  if ( pmount->prop.ismount )
  {
    pmount->team = pmount->team_base;
    pmount->aistate.alert |= ALERT_DROPPED;
  }
  pchr->team = pchr->team_base;
  pchr->aistate.alert |= ALERT_DROPPED;

  // Reset transparency
  if ( pchr->prop.isitem && pmount->transferblend )
  {
    OBJ_REF model = pchr->model;

    assert( NULL != ChrList_getPMad(gs, chr_ref) );

    // Okay, reset transparency
    enchant = pchr->firstenchant;
    while ( enchant < enclst_size )
    {
      unset_enchant_value( gs, enchant, SETALPHABLEND );
      unset_enchant_value( gs, enchant, SETLIGHTBLEND );
      enchant = enclst[enchant].nextenchant;
    }

    pchr->alpha_fp8 = pcap->alpha_fp8;
    pchr->bumpstrength = pcap->bumpstrength * FP8_TO_FLOAT( pchr->alpha_fp8 );
    pchr->light_fp8 = pcap->light_fp8;
    enchant = pchr->firstenchant;
    while ( enchant < enclst_size )
    {
      set_enchant_value( gs, enchant, SETALPHABLEND, enclst[enchant].eve );
      set_enchant_value( gs, enchant, SETLIGHTBLEND, enclst[enchant].eve );
      enchant = enclst[enchant].nextenchant;
    }
  }

  // Set twist
  pchr->ori.mapturn_lr = 32768;
  pchr->ori.mapturn_ud = 32768;

  if ( chr_is_player(gs, chr_ref) )
  {
    Cap_t * pcap1 = ChrList_getPCap(gs, chr_ref);
    Cap_t * pcap2 = ChrList_getPCap(gs, imount);
    debug_message( 1, "dismounted %s(%s) from (%s)", pchr->name, pcap1->classname, pcap2->classname );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_character_to_mount( Game_t * gs, CHR_REF chr_ref, CHR_REF mount_ref, SLOT slot )
{
  // ZZ> This function attaches one character to another ( the mount )
  //     at either the left or right grip

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PMad_t madlst      = gs->MadList;
  size_t madlst_size = MADLST_COUNT;

  int tnc;

  Chr_t * pchr, *pmount;

  // Make sure both are still around
  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return bfalse;

  if( !PENDING_CHR( chrlst, mount_ref ) ) return bfalse;
  pmount = ChrList_getPChr(gs, mount_ref);


  // the item may hit the floor if this fails
  pchr->hitready = bfalse;

  //make sure you're not trying to mount yourself!
  if ( chr_ref == mount_ref )
    return bfalse;

  // make sure that neither is in someone's pack
  if ( chr_in_pack( chrlst, chrlst_size, chr_ref ) || chr_in_pack( chrlst, chrlst_size, mount_ref ) )
    return bfalse;

  // Make sure the the slot is valid
  assert( NULL != ChrList_getPMad(gs, mount_ref) );
  if ( SLOT_NONE == slot || !ChrList_getPCap(gs, mount_ref)->slotvalid[slot] )
    return bfalse;

  // Put 'em together
  assert(slot != SLOT_NONE);
  pchr->inwhichslot = slot;
  pchr->attachedto  = mount_ref;
  pmount->holdingwhich[slot] = chr_ref;

  // handle the vertices
  {
    OBJ_REF iobj = ChrList_getRObj(gs, mount_ref);
    MAD_REF imad = ChrList_getRMad(gs, mount_ref);
    Uint16 vrtoffset = slot_to_offset( slot );

    assert( INVALID_MAD != VALIDATE_MAD( madlst, imad ) );
    if ( madlst[imad].vertices > vrtoffset && vrtoffset > 0 )
    {
      tnc = madlst[imad].vertices - vrtoffset;
      pchr->attachedgrip[0] = tnc;
      pchr->attachedgrip[1] = tnc + 1;
      pchr->attachedgrip[2] = tnc + 2;
      pchr->attachedgrip[3] = tnc + 3;
    }
    else
    {
      pchr->attachedgrip[0] = madlst[imad].vertices - 1;
      pchr->attachedgrip[1] = 0xFFFF;
      pchr->attachedgrip[2] = 0xFFFF;
      pchr->attachedgrip[3] = 0xFFFF;
    }
  }

  pchr->jumptime = DELAY_JUMP * 4;

  // Run the held animation
  if ( pmount->bmpdata.calc_is_mount && slot == SLOT_SADDLE )
  {
    // Riding mount_ref
    play_action( gs, chr_ref, ACTION_MI, btrue );
    pchr->action.loop = btrue;
  }
  else
  {
    play_action( gs, chr_ref, (ACTION)(ACTION_MM + slot), bfalse );
    if ( pchr->prop.isitem )
    {
      // Item grab
      pchr->action.keep = btrue;
    }
  }

  // Set the team
  if ( pchr->prop.isitem )
  {
    pchr->team = pmount->team;
    // Set the alert
    pchr->aistate.alert |= ALERT_GRABBED;
  }
  else if ( pmount->bmpdata.calc_is_mount )
  {
    pmount->team = pchr->team;
    // Set the alert
    if ( !pmount->prop.isitem )
    {
      pmount->aistate.alert |= ALERT_GRABBED;
    }
  }

  // It's not gonna hit the floor
  pchr->hitready = bfalse;

  if ( chr_is_player(gs, chr_ref) )
  {
    Cap_t * pcap1 = ChrList_getPCap(gs, chr_ref);
    Cap_t * pcap2 = ChrList_getPCap(gs, mount_ref);

    debug_message( 1, "mounted %s(%s) to (%s)", pchr->name, pcap1->classname, pcap2->classname );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF pack_find_stack( Game_t * gs, CHR_REF item_ref, CHR_REF pack_chr_ref )
{
  // ZZ> This function looks in the chraracter's pack for an item similar
  //     to the one given.  If it finds one, it returns the similar item's
  //     index number, otherwise it returns CHRLST_COUNT.

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  CHR_REF inpack_ref;
  Uint16 id;
  bool_t allok;

  OBJ_REF item_obj;
  CAP_REF item_cap;
  MAD_REF item_mad;

  OBJ_REF inpack_obj;
  CAP_REF inpack_cap;
  MAD_REF inpack_mad;

  item_obj = chrlst[item_ref].model;
  item_cap = gs->ObjList[item_obj].cap;
  item_mad = gs->ObjList[item_obj].mad;

  assert( VALID_OBJ(gs->ObjList, item_obj) );

  if ( caplst[item_cap].prop.isstackable )
  {
    inpack_ref = chr_get_nextinpack( chrlst, chrlst_size, pack_chr_ref );
    inpack_obj = chrlst[inpack_ref].model;
    inpack_cap = gs->ObjList[inpack_obj].cap;
    inpack_mad = gs->ObjList[inpack_obj].mad;

    allok = bfalse;
    while ( ACTIVE_CHR( chrlst, inpack_ref ) && !allok )
    {
      assert( VALID_OBJ(gs->ObjList, inpack_obj) );

      allok = btrue;
      if ( inpack_obj != item_obj )
      {
        if ( !caplst[inpack_cap].prop.isstackable )
        {
          allok = bfalse;
        }

        if ( chrlst[inpack_ref].ammomax != chrlst[item_ref].ammomax )
        {
          allok = bfalse;
        }

        id = 0;
        while ( id < IDSZ_COUNT && allok )
        {
          if ( caplst[inpack_cap].idsz[id] != caplst[item_cap].idsz[id] )
          {
            allok = bfalse;
          }
          id++;
        }
      }
      if ( !allok )
      {
        inpack_ref = chr_get_nextinpack( chrlst, chrlst_size, inpack_ref );
      }
    }

    if ( allok )
    {
      return inpack_ref;
    }
  }

  return INVALID_CHR;
}

//--------------------------------------------------------------------------------------------
static bool_t pack_push_front( ChrList_t chrlst, size_t chrlst_size, CHR_REF item_ref, CHR_REF pack_chr_ref )
{
  // make sure the item and character are valid
  if ( !ACTIVE_CHR( chrlst, item_ref ) || !ACTIVE_CHR( chrlst, pack_chr_ref ) ) return bfalse;

  // make sure the item is free to add
  if ( chr_attached( chrlst, chrlst_size, item_ref ) || chr_in_pack( chrlst, chrlst_size, item_ref ) ) return bfalse;

  // we cannot do packs within packs, so
  if ( chr_in_pack( chrlst, chrlst_size, pack_chr_ref ) ) return bfalse;

  // make sure there is space for the item
  if ( chrlst[pack_chr_ref].numinpack >= MAXNUMINPACK ) return bfalse;

  // insert at the front of the list
  chrlst[item_ref].nextinpack  = chr_get_nextinpack( chrlst, chrlst_size, pack_chr_ref );
  chrlst[pack_chr_ref].nextinpack = item_ref;
  chrlst[item_ref].inwhichpack = pack_chr_ref;
  chrlst[pack_chr_ref].numinpack++;

  return btrue;
};

//--------------------------------------------------------------------------------------------
static CHR_REF pack_pop_back( ChrList_t chrlst, size_t chrlst_size, CHR_REF pack_chr_ref )
{
  CHR_REF iitem = INVALID_CHR, itail = INVALID_CHR;

  // make sure the character is valid
  if ( !ACTIVE_CHR( chrlst, pack_chr_ref ) ) return INVALID_CHR;

  // if character is in a pack, it has no inventory of it's own
  if ( chr_in_pack( chrlst, chrlst_size, iitem ) ) return INVALID_CHR;

  // make sure there is something in the pack
  if ( !chr_has_inventory( chrlst, chrlst_size, pack_chr_ref ) ) return INVALID_CHR;

  // remove from the back of the list
  itail = pack_chr_ref;
  iitem = chr_get_nextinpack( chrlst, chrlst_size, pack_chr_ref );
  while ( ACTIVE_CHR( chrlst, chrlst[iitem].nextinpack ) )
  {
    // do some error checking
    assert( 0 == chrlst[iitem].numinpack );

    // go to the next element
    itail = iitem;
    iitem = chr_get_nextinpack( chrlst, chrlst_size, iitem );
  };

  // disconnect the item from the list
  chrlst[itail].nextinpack = INVALID_CHR;
  chrlst[pack_chr_ref].numinpack--;

  // do some error checking
  assert( ACTIVE_CHR( chrlst, iitem ) );

  // fix the removed item
  chrlst[iitem].numinpack   = 0;
  chrlst[iitem].nextinpack  = INVALID_CHR;
  chrlst[iitem].inwhichpack = INVALID_CHR;
  chrlst[iitem].isequipped = bfalse;

  return iitem;
};

//--------------------------------------------------------------------------------------------
bool_t pack_add_item( Game_t * gs, CHR_REF item_ref, CHR_REF pack_chr_ref )
{
  // ZZ> This function puts one pack_chr_ref inside the other's pack

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  Uint16 newammo;
  CHR_REF istack;

  // Make sure both objects exist
  if ( !ACTIVE_CHR( chrlst, pack_chr_ref ) || !ACTIVE_CHR( chrlst, item_ref ) ) return bfalse;

  // Make sure neither object is in a pack
  if ( chr_in_pack( chrlst, chrlst_size, pack_chr_ref ) || chr_in_pack( chrlst, chrlst_size, item_ref ) ) return bfalse;

  // make sure we the character IS NOT an item and the item IS an item
  if ( chrlst[pack_chr_ref].prop.isitem || !chrlst[item_ref].prop.isitem ) return bfalse;

  // make sure the item does not have an inventory of its own
  if ( chr_has_inventory( chrlst, chrlst_size, item_ref ) ) return bfalse;

  istack = pack_find_stack( gs, item_ref, pack_chr_ref );
  if ( ACTIVE_CHR( chrlst, istack ) )
  {
    // put out torches, etc.
    disaffirm_attached_particles( gs, item_ref );

    // We found a similar, stackable item_ref in the pack
    if ( chrlst[item_ref].prop.nameknown || chrlst[istack].prop.nameknown )
    {
      chrlst[item_ref].prop.nameknown = btrue;
      chrlst[istack].prop.nameknown = btrue;
    }
    if ( chrlst[item_ref].prop.usageknown || chrlst[istack].prop.usageknown )
    {
      chrlst[item_ref].prop.usageknown = btrue;
      chrlst[istack].prop.usageknown = btrue;
    }
    newammo = chrlst[item_ref].ammo + chrlst[istack].ammo;
    if ( newammo <= chrlst[istack].ammomax )
    {
      // All transfered, so kill the in hand item_ref
      chrlst[istack].ammo = newammo;
      detach_character_from_mount( gs, item_ref, btrue, bfalse );
      chrlst[item_ref].freeme = btrue;
    }
    else
    {
      // Only some were transfered,
      chrlst[item_ref].ammo += chrlst[istack].ammo - chrlst[istack].ammomax;
      chrlst[istack].ammo = chrlst[istack].ammomax;
      chrlst[pack_chr_ref].aistate.alert |= ALERT_TOOMUCHBAGGAGE;
    }
  }
  else
  {
    // Make sure we have room for another item_ref
    if ( chrlst[pack_chr_ref].numinpack >= MAXNUMINPACK )
    {
      chrlst[pack_chr_ref].aistate.alert |= ALERT_TOOMUCHBAGGAGE;
      return bfalse;
    }

    // Take the item out of hand
    if ( detach_character_from_mount( gs, item_ref, btrue, bfalse ) )
    {
      chrlst[item_ref].aistate.alert &= ~ALERT_DROPPED;
    }

    if ( pack_push_front( chrlst, chrlst_size, item_ref, pack_chr_ref ) )
    {
      // put out torches, etc.
      disaffirm_attached_particles( gs, item_ref );
      chrlst[item_ref].aistate.alert |= ALERT_ATLASTWAYPOINT;

      // Remove the item_ref from play
      chrlst[item_ref].hitready    = bfalse;
    };
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF pack_get_item( Game_t * gs, CHR_REF pack_chr_ref, SLOT slot, bool_t ignorekurse )
{
  // ZZ> This function takes the last item in the character's pack and puts
  //     it into the designated hand.  It returns the item number or CHRLST_COUNT.

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CHR_REF item;

  // dose the pack_chr_ref exist?
  if ( !ACTIVE_CHR( chrlst, pack_chr_ref ) )
    return INVALID_CHR;

  // make sure a valid inventory exists
  if ( !chr_has_inventory( chrlst, chrlst_size, pack_chr_ref ) )
    return INVALID_CHR;

  item = pack_pop_back( chrlst, chrlst_size, pack_chr_ref );

  // Figure out what to do with it
  if ( chrlst[item].prop.iskursed && chrlst[item].isequipped && !ignorekurse )
  {
    // Flag the last item as not removed
    chrlst[item].aistate.alert |= ALERT_NOTPUTAWAY;  // Doubles as IfNotTakenOut

    // push it back on the front of the list
    pack_push_front( chrlst, chrlst_size, item, pack_chr_ref );

    // return the "fail" value
    item = INVALID_CHR;
  }
  else
  {
    // Attach the item to the pack_chr_ref's hand
    attach_character_to_mount( gs, item, pack_chr_ref, slot );

    // fix some item values
    chrlst[item].aistate.alert &= ( ~ALERT_GRABBED );
    chrlst[item].aistate.alert |= ALERT_TAKENOUT;
    //chrlst[item].team   = chrlst[pack_chr_ref].team;
  }

  return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( Game_t * gs, CHR_REF chr_ref )
{
  // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a chr_ref's
  //     inventory ( Not hands ).

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PObj_t objlst = gs->ObjList;

  CHR_REF item, lastitem, nextitem;
  Uint16 direction, cosdir;
  IDSZ testa, testz;
  Uint32 loc_rand;

  if ( !ACTIVE_CHR( chrlst, chr_ref ) ) return;

  loc_rand = gs->randie_index;

  if ( chrlst[chr_ref].ori.pos.z > -2 ) // Don't lose keys in pits...
  {
    // The IDSZs to find
    testa = MAKE_IDSZ( "KEYA" );   // [KEYA]
    testz = MAKE_IDSZ( "KEYZ" );   // [KEYZ]

    lastitem = chr_ref;
    item = chr_get_nextinpack( chrlst, chrlst_size, chr_ref );
    while ( ACTIVE_CHR( chrlst, item ) )
    {
      nextitem = chr_get_nextinpack( chrlst, chrlst_size, item );
      if ( item != chr_ref ) // Should never happen...
      {
        if ( CAP_INHERIT_IDSZ_RANGE( gs, objlst[chrlst[item].model].cap, testa, testz ) )
        {
          // We found a key...
          chrlst[item].inwhichpack = INVALID_CHR;
          chrlst[item].isequipped = bfalse;

          chrlst[lastitem].nextinpack = nextitem;
          chrlst[item].nextinpack = INVALID_CHR;
          chrlst[chr_ref].numinpack--;

          chrlst[item].hitready = btrue;
          chrlst[item].aistate.alert |= ALERT_DROPPED;

          direction = IRAND(&loc_rand, 15);
          chrlst[item].ori.turn_lr = direction + 32768;
          cosdir = direction + 16384;
          chrlst[item].level = chrlst[chr_ref].level;
          chrlst[item].ori.pos.x = chrlst[chr_ref].ori.pos.x;
          chrlst[item].ori.pos.y = chrlst[chr_ref].ori.pos.y;
          chrlst[item].ori.pos.z = chrlst[chr_ref].ori.pos.z;
          chrlst[item].accum.vel.x += turntocos[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          chrlst[item].accum.vel.y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          chrlst[item].accum.vel.z += DROPZVEL;
          chrlst[item].team = chrlst[item].team_base;
        }
        else
        {
          lastitem = item;
        }
      }
      item = nextitem;
    }
  }

}

//--------------------------------------------------------------------------------------------
void drop_all_items( Game_t * gs, CHR_REF chr_ref )
{
  // ZZ> This function drops all of a character's items

  PChr_t chrlst        = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CHR_REF item;
  Uint16 direction, cosdir, diradd;

  if ( !ACTIVE_CHR( chrlst, chr_ref ) ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    detach_character_from_mount( gs, chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, _slot ), !chrlst[chr_ref].alive, bfalse );
  };

  if ( chr_has_inventory( chrlst, chrlst_size, chr_ref ) )
  {
    direction = chrlst[chr_ref].ori.turn_lr + 32768;
    diradd = (float)UINT16_SIZE / chrlst[chr_ref].numinpack;
    while ( chrlst[chr_ref].numinpack > 0 )
    {
      item = pack_get_item( gs, chr_ref, SLOT_NONE, !chrlst[chr_ref].alive );
      if ( detach_character_from_mount( gs, item, btrue, btrue ) )
      {
        chrlst[item].hitready = btrue;
        chrlst[item].aistate.alert |= ALERT_DROPPED;
        chrlst[item].ori.pos.x = chrlst[chr_ref].ori.pos.x;
        chrlst[item].ori.pos.y = chrlst[chr_ref].ori.pos.y;
        chrlst[item].ori.pos.z = chrlst[chr_ref].ori.pos.z;
        chrlst[item].level = chrlst[chr_ref].level;
        chrlst[item].ori.turn_lr = direction + 32768;

        cosdir = direction + 16384;
        chrlst[item].accum.vel.x += turntocos[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        chrlst[item].accum.vel.y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        chrlst[item].accum.vel.z += DROPZVEL;
        chrlst[item].team = chrlst[item].team_base;
      }

      direction += diradd;
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t chr_grab_stuff( Game_t * gs, CHR_REF chr_ref, SLOT slot, bool_t people )
{
  // ZZ> This function makes the character pick up an item if there's one around

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  Mesh_t * pmesh     = Game_getMesh(gs); 

  vect4 posa, point;
  vect3 posb, posc;
  float dist, mindist;
  CHR_REF object_ref, minchr_ref, holder_ref, packer_ref, owner_ref = INVALID_CHR;
  Uint16 vertex, passage, cnt, price;
  bool_t inshop, can_disarm, can_pickpocket, bfound, ballowed;
  GRIP grip;
  float grab_width, grab_height;

  CHR_REF trg_chr = INVALID_CHR;
  Sint16 trg_strength_fp8, trg_intelligence_fp8;
  TEAM_REF trg_team;

  Chr_t * pchr;
  Cap_t * pcap;
  Mad_t * pmad;

  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return bfalse;

  pcap = ChrList_getPCap(gs, chr_ref);
  if(NULL == pcap) return bfalse;

  pmad = ChrList_getPMad(gs, chr_ref);
  if(NULL == pmad) return bfalse;

  // Make sure the character doesn't have something already, and that it has hands

  if ( chr_using_slot( chrlst, chrlst_size, chr_ref, slot ) || !pcap->slotvalid[slot] )
    return bfalse;

  // Make life easier
  grip  = slot_to_grip( slot );

  // !!!!base the grab distance off of the character size!!!!
  if( !pchr->bmpdata.valid )
  {
    chr_calculate_bumpers( gs, chrlst + chr_ref, 0);
  }

  grab_width  = ( pchr->bmpdata.calc_size_big + pchr->bmpdata.calc_size ) / 2.0f * 1.5f;
  grab_height = pchr->bmpdata.calc_height / 2.0f * 1.5f;

  // Do we have a matrix???
  if ( pchr->matrix_valid )
  {
    // Transform the weapon grip from model to world space
    vertex = pchr->attachedgrip[0];

    if(0xFFFF == vertex)
    {
      point.x = pchr->ori.pos.x;
      point.y = pchr->ori.pos.y;
      point.z = pchr->ori.pos.z;
      point.w = 1.0f;
    }
    else
    {
      point.x = pchr->vdata.Vertices[vertex].x;
      point.y = pchr->vdata.Vertices[vertex].y;
      point.z = pchr->vdata.Vertices[vertex].z;
      point.w = 1.0f;
    }

    // Do the transform
    Transform4_Full( 1.0f, 1.0f, &(pchr->matrix), &point, &posa, 1 );
  }
  else
  {
    // Just wing it
    posa.x = pchr->ori.pos.x;
    posa.y = pchr->ori.pos.y;
    posa.z = pchr->ori.pos.z;
  }

  // Go through all characters to find the best match
  can_disarm     = check_skills( gs, chr_ref, MAKE_IDSZ( "DISA" ) );
  can_pickpocket = check_skills( gs, chr_ref, MAKE_IDSZ( "PICK" ) );
  bfound = bfalse;
  for ( object_ref = 0; object_ref < chrlst_size; object_ref++ )
  {
    // Don't mess with stuff that doesn't exist
    if ( !ACTIVE_CHR( chrlst, object_ref ) ) continue;

    holder_ref = chr_get_attachedto(chrlst, chrlst_size, object_ref);
    packer_ref = chr_get_inwhichpack(chrlst, chrlst_size, object_ref);

    // don't mess with yourself or anything you're already holding
    if ( object_ref == chr_ref || packer_ref == chr_ref || holder_ref == chr_ref ) continue;

    // don't mess with stuff you can't see
    if ( !pchr->prop.canseeinvisible && chr_is_invisible( chrlst, chrlst_size, object_ref ) ) continue;

    // if we can't pickpocket, don't mess with inventory items
    if ( !can_pickpocket && ACTIVE_CHR( chrlst, packer_ref ) ) continue;

    // if we can't disarm, don't mess with held items
    if ( !can_disarm && ACTIVE_CHR( chrlst, holder_ref ) ) continue;

    // if we can't grab people, don't mess with them
    if ( !people && !chrlst[object_ref].prop.isitem ) continue;

    // get the target object position
    if ( !ACTIVE_CHR( chrlst, packer_ref ) && !ACTIVE_CHR(chrlst, holder_ref) )
    {
      trg_strength_fp8     = chrlst[object_ref].stats.strength_fp8;
      trg_intelligence_fp8 = chrlst[object_ref].stats.intelligence_fp8;
      trg_team             = chrlst[object_ref].team;

      posb = chrlst[object_ref].ori.pos;
    }
    else if ( ACTIVE_CHR(chrlst, holder_ref) )
    {
      trg_chr              = holder_ref;
      trg_strength_fp8     = chrlst[holder_ref].stats.strength_fp8;
      trg_intelligence_fp8 = chrlst[holder_ref].stats.intelligence_fp8;

      trg_team = chrlst[holder_ref].team;
      posb     = chrlst[object_ref].ori.pos;
    }
    else // must be in a pack
    {
      trg_chr              = packer_ref;
      trg_strength_fp8     = chrlst[packer_ref].stats.strength_fp8;
      trg_intelligence_fp8 = chrlst[packer_ref].stats.intelligence_fp8;

      trg_team = chrlst[packer_ref].team;
      posb     = chrlst[packer_ref].ori.pos;
      posb.z  += chrlst[packer_ref].bmpdata.calc_height / 2;
    };

    // First check absolute value diamond
    posc.x = ABS( posa.x - posb.x );
    posc.y = ABS( posa.y - posb.y );
    posc.z = ABS( posa.z - posb.z );
    dist = posc.x + posc.y;

    // close enough to grab ?
    if ( dist > grab_width || posc.z > grab_height ) continue;

    if ( ACTIVE_CHR(chrlst, packer_ref) )
    {
      // check for pickpocket
      ballowed = pchr->stats.dexterity_fp8 >= trg_intelligence_fp8 && gs->TeamList[pchr->team].hatesteam[REF_TO_INT(trg_team)];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        chrlst[holder_ref].aistate.alert |= ALERT_ATTACKED;
        chrlst[holder_ref].aistate.bumplast = chr_ref;
      }
      else  // must be in a pack
      {
        // TODO : figure out a way to get the thing out of the pack!!
        //        pack_get_item() won't work?

      };
    }
    else if ( ACTIVE_CHR( chrlst, holder_ref ) )
    {
      // check for stealing item from hand
      ballowed = !chrlst[object_ref].prop.iskursed && pchr->stats.strength_fp8 > trg_strength_fp8 && gs->TeamList[pchr->team].hatesteam[REF_TO_INT(trg_team)];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        chrlst[holder_ref].aistate.alert |= ALERT_ATTACKED;
        chrlst[holder_ref].aistate.bumplast = chr_ref;
      }
      else
      {
        // TODO : do the dismount
      };
    }
    else
    {
      ballowed = btrue;
    }

    if ( ballowed || !bfound )
    {
      mindist = dist;
      minchr_ref  = object_ref;
      bfound  = btrue;
    };

  };

  if ( !bfound ) return bfalse;

  // Check for shop
  inshop = bfalse;
  ballowed = bfalse;
  if ( mesh_check( (&pmesh->Info), chrlst[minchr_ref].ori.pos.x, chrlst[minchr_ref].ori.pos.y ) )
  {
    if ( gs->ShopList_count == 0 )
    {
      ballowed = btrue;
    }
    else if ( chrlst[minchr_ref].prop.isitem )
    {

      // loop through just in case there are overlapping shops with one owner_ref deceased
      for ( cnt = 0; cnt < gs->ShopList_count && !inshop; cnt++ )
      {
        passage = gs->ShopList[cnt].passage;

        if ( passage_check_any( gs, minchr_ref, passage, NULL ) )
        {
          owner_ref  = gs->ShopList[passage].owner;
          inshop = ( NOOWNER != owner_ref );
        };
      };

    };
  }

  if ( inshop )
  {
    if ( pchr->prop.isitem )
    {
      ballowed = btrue; // As in NetHack, Pets can shop for free =]
    }
    else
    {
      // Pay the shop owner_ref, or don't allow grab...
      chrlst[owner_ref].aistate.alert |= ALERT_SIGNALED;
      price = ChrList_getPCap(gs, minchr_ref)->skin[chrlst[minchr_ref].skin_ref % MAXSKIN].cost;
      if ( chrlst[minchr_ref].prop.isstackable )
      {
        price *= chrlst[minchr_ref].ammo;
      }
      chrlst[owner_ref].message.data = price;  // Tell owner_ref how much...
      if ( pchr->money >= price )
      {
        // Okay to buy
        pchr->money  -= price;  // Skin 0 cost is price
        chrlst[owner_ref].money += price;
        if ( chrlst[owner_ref].money > MAXMONEY )  chrlst[owner_ref].money = MAXMONEY;

        ballowed = btrue;
        chrlst[owner_ref].message.type = SIGNAL_SELL;  // 1 for selling an item
      }
      else
      {
        // Don't allow purchase
        chrlst[owner_ref].message.type = SIGNAL_REJECT;  // 2 for "you can't afford that"
        ballowed = bfalse;
      }
    }
  }

  if ( ballowed )
  {
    // Stick 'em together and quit
    ballowed = attach_character_to_mount( gs, minchr_ref, chr_ref, slot );
    if ( ballowed && people )
    {
      // Do a bodyslam animation...  ( Be sure to drop!!! )
      play_action( gs, chr_ref, (ACTION)(ACTION_MC + slot), bfalse );
    };
  }
  else
  {
    // Lift the item a little and quit...
    chrlst[minchr_ref].accum.vel.z += DROPZVEL;
    chrlst[minchr_ref].hitready = btrue;
    chrlst[minchr_ref].aistate.alert |= ALERT_DROPPED;
  };

  return ballowed;
}

//--------------------------------------------------------------------------------------------
void chr_swipe( Game_t * gs, CHR_REF ichr, SLOT slot )
{
  // ZZ> This function spawns an attack particle

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  PPip_t piplst      = gs->PipList;
  size_t piplst_size = PIPLST_COUNT;

  CHR_REF iweapon, ithrown;
  Chr_t * pchr, * pweapon, *pthrown;

  PRT_REF particle;

  ACTION action;
  Uint16 tTmp;
  float dampen;
  GRIP  spawngrip;


  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return;


  // See if it's an unarmed attack...
  iweapon = chr_get_holdingwhich( chrlst, chrlst_size, ichr, slot );
  spawngrip = GRIP_LAST;
  pweapon = ChrList_getPChr(gs, iweapon);
  if ( NULL == pweapon )
  {
    iweapon   = ichr;
    pweapon   = pchr;
    spawngrip = slot_to_grip( slot );
  }

  action = pchr->action.now;

  if ( iweapon != ichr && (( pweapon->prop.isstackable && pweapon->ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
  {
    // Throw the weapon if it's stacked or a hurl animation

    ithrown = chr_spawn( gs, pchr->ori.pos, pchr->ori.vel, pweapon->model, pchr->team, 0, pchr->ori.turn_lr, pweapon->name, INVALID_CHR );
    if ( VALID_CHR(gs->ChrList, ithrown) )
    {
      float velocity;

      pthrown = gs->ChrList + ithrown;
      pthrown->prop.iskursed = bfalse;
      pthrown->ammo = 1;
      pthrown->aistate.alert |= ALERT_THROWN;

      velocity = 0.0f;
      if ( pchr->weight >= 0.0f )
      {
        velocity = pchr->stats.strength_fp8 / ( chrlst[ithrown].weight * THROWFIX );
      };

      velocity += MINTHROWVELOCITY;
      if ( velocity > MAXTHROWVELOCITY )
      {
        velocity = MAXTHROWVELOCITY;
      }
      tTmp = ( 0x7FFF + pchr->ori.turn_lr ) >> 2;
      pthrown->ori.vel.x += turntocos[( tTmp+8192 ) & TRIGTABLE_MASK] * velocity;
      pthrown->ori.vel.y += turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * velocity;
      pthrown->ori.vel.z += DROPZVEL;

      if ( pweapon->ammo <= 1 )
      {
        // Poof the item
        detach_character_from_mount( gs, iweapon, btrue, bfalse );
        pweapon->freeme = btrue;
      }
      else
      {
        pweapon->ammo--;
      }
    }
  }
  else if ( pweapon->ammomax == 0 || pweapon->ammo != 0 )
  {
    // Spawn an attack particle
    Cap_t * pweapon_cap = ChrList_getPCap(gs, iweapon);

    if ( pweapon->ammo > 0 && !pweapon->prop.isstackable )
    {
      pweapon->ammo--;  // Ammo usage
    }

    if ( INVALID_PIP != pweapon_cap->attackprttype )
    {
      Prt_t * pprt;
      PRT_SPAWN_INFO si;

      prt_spawn_info_init( &si, gs, 1.0f, pweapon->ori.pos, pchr->ori.turn_lr, pweapon->model, pweapon_cap->attackprttype, iweapon, spawngrip, pchr->team, ichr, 0, INVALID_CHR );
      particle = req_spawn_one_particle( si );
      if ( !RESERVED_PRT(gs->PrtList, particle) )
      {
        Pip_t *  ppip = PrtList_getPPip(gs, particle);
        CHR_REF prt_target = prt_get_target( gs, particle );
        pprt = gs->PrtList + particle;

        if ( !pweapon_cap->attackattached )
        {
          // Detach the particle
          if ( NULL!=ppip && !ppip->startontarget || !ACTIVE_CHR( chrlst, prt_target ) )
          {
            attach_particle_to_character( gs, particle, iweapon, spawngrip );

            // Correct Z spacing base, but nothing else...
            pprt->ori.pos.z += ppip->zspacing.ibase;
          }
          pprt->attachedtochr = INVALID_CHR;

          // Don't spawn in walls
          if ( 0 != prt_hitawall( gs, particle, NULL ) )
          {
            pprt->ori.pos.x = pweapon->ori.pos.x;
            pprt->ori.pos.y = pweapon->ori.pos.y;
            if ( 0 != prt_hitawall( gs, particle, NULL ) )
            {
              pprt->ori.pos.x = pchr->ori.pos.x;
              pprt->ori.pos.y = pchr->ori.pos.y;
            }
          }
        }
        else
        {
          // Attached particles get a strength bonus for reeling...
          if ( NULL!=ppip && ppip->causeknockback ) dampen = ( REELBASE + ( pchr->stats.strength_fp8 / REEL ) ) * 4; //Extra knockback?
          else dampen = REELBASE + ( pchr->stats.strength_fp8 / REEL );      // No, do normal

          pprt->accum.vel.x += -(1.0f - dampen) * pprt->ori.vel.x;
          pprt->accum.vel.y += -(1.0f - dampen) * pprt->ori.vel.y;
          pprt->accum.vel.z += -(1.0f - dampen) * pprt->ori.vel.z;
        }

        // Initial particles get a strength bonus, which may be 0.00
        pprt->damage.ibase += ( pchr->stats.strength_fp8 * pweapon_cap->strengthdampen );

        // Initial particles get an enchantment bonus
        pprt->damage.ibase += pweapon->damageboost;

        // Initial particles inherit damage type of iweapon
        pprt->damagetype = pweapon->damagetargettype;
      }
    }
  }
  else
  {
    pweapon->ammoknown = btrue;
  }




}
//--------------------------------------------------------------------------------------------
bool_t chr_do_animation( Game_t * gs, float dUpdate, CHR_REF chr_ref, ChrEnviro_t * enviro, Uint32 * prand )
{
  // BB > Animate the character

  Chr_t * pchr;
  Obj_t * pobj;
  Cap_t * pcap;
  Mad_t * pmad;
  AI_STATE * pstate;
  OBJ_REF iobj;
  CHR_REF imount;

  Uint8 speed, framelip;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;


  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return bfalse;
  pstate = &(pchr->aistate);

  iobj = ChrList_getRObj(gs, chr_ref);
  if(INVALID_OBJ == iobj) return bfalse;
  pobj = gs->ObjList + iobj;

  pmad = ChrList_getPMad(gs, chr_ref);
  if(NULL == pmad) return bfalse;

  pcap = ChrList_getPCap(gs, chr_ref);
  if(NULL == pcap) return bfalse;

  imount = chr_get_attachedto(chrlst, chrlst_size, chr_ref);

  // Texture animation
  pchr->uoffset_fp8 += pchr->uoffvel * dUpdate;
  pchr->voffset_fp8 += pchr->voffvel * dUpdate;

  // do pancake anim
  pchr->pancakevel.x *= 0.90;
  pchr->pancakevel.y *= 0.90;
  pchr->pancakevel.z *= 0.90;

  pchr->pancakepos.x += pchr->pancakevel.x * dUpdate;
  pchr->pancakepos.y += pchr->pancakevel.y * dUpdate;
  pchr->pancakepos.z += pchr->pancakevel.z * dUpdate;

  if ( pchr->pancakepos.x < 0 ) { pchr->pancakepos.x = 0.001; pchr->pancakevel.x *= -0.5f; };
  if ( pchr->pancakepos.y < 0 ) { pchr->pancakepos.y = 0.001; pchr->pancakevel.y *= -0.5f; };
  if ( pchr->pancakepos.z < 0 ) { pchr->pancakepos.z = 0.001; pchr->pancakevel.z *= -0.5f; };

  pchr->pancakevel.x += ( 1.0f - pchr->pancakepos.x ) * dUpdate / 10.0f;
  pchr->pancakevel.y += ( 1.0f - pchr->pancakepos.y ) * dUpdate / 10.0f;
  pchr->pancakevel.z += ( 1.0f - pchr->pancakepos.z ) * dUpdate / 10.0f;

  // so the model's animation
  pchr->anim.flip += dUpdate * 0.25;
  while ( pchr->anim.flip > 0.25f )
  {
    // convert flip into lip
    pchr->anim.flip -= 0.25f;
    pchr->anim.ilip += 64;

    // handle the mad fx
    if ( pchr->anim.ilip == 192 )
    {
      // Check frame effects
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_ACTLEFT ) )
        chr_swipe( gs, chr_ref, SLOT_LEFT );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_ACTRIGHT ) )
        chr_swipe( gs, chr_ref, SLOT_RIGHT );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_GRABLEFT ) )
        chr_grab_stuff( gs, chr_ref, SLOT_LEFT, bfalse );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_GRABRIGHT ) )
        chr_grab_stuff( gs, chr_ref, SLOT_RIGHT, bfalse );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_CHARLEFT ) )
        chr_grab_stuff( gs, chr_ref, SLOT_LEFT, btrue );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_CHARRIGHT ) )
        chr_grab_stuff( gs, chr_ref, SLOT_RIGHT, btrue );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_DROPLEFT ) )
        detach_character_from_mount( gs, chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_LEFT ), bfalse, btrue );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_DROPRIGHT ) )
        detach_character_from_mount( gs, chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_RIGHT ), bfalse, btrue );
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_POOF ) && !chr_is_player(gs, chr_ref) )
        pchr->gopoof = btrue;
      if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_FOOTFALL ) )
      {
        if ( NULL !=pcap && INVALID_SOUND != pcap->footfallsound )
        {
          float volume = ( ABS( pchr->ori.vel.x ) +  ABS( pchr->ori.vel.y ) ) / pcap->spd_sneak;
          snd_play_sound( gs, MIN( 1.0f, volume ), pchr->ori.pos, pobj->wavelist[pcap->footfallsound], 0, iobj, pcap->footfallsound );
        }
      }
    }

    // change frames
    if ( pchr->anim.ilip == 0 )
    {
      // Change frames
      pchr->anim.last = pchr->anim.next;
      pchr->anim.next++;

      if ( pchr->anim.next >= pmad->actionend[pchr->action.now] )
      {
        // Action finished
        if ( pchr->action.keep )
        {
          // Keep the last frame going
          pchr->anim.next = pchr->anim.last;
        }
        else if ( !pchr->action.loop )
        {
          // Go on to the next action
          pchr->action.now  = pchr->action.next;
          pchr->action.next = ACTION_DA;

          pchr->anim.next = pmad->actionstart[pchr->action.now];
        }
        else if ( ACTIVE_CHR(chrlst, imount) )
        {
          // See if the character is mounted...
          pchr->action.now = ACTION_MI;

          pchr->anim.next = pmad->actionstart[pchr->action.now];
        }

        pchr->action.ready = btrue;
      }
    }

  };



  // Get running, walking, sneaking, or dancing, from speed
  if ( !pchr->action.keep && !pchr->action.loop )
  {
    framelip = pmad->framelip[pchr->anim.next];  // 0 - 15...  Way through animation
    if ( pchr->action.ready && pchr->anim.ilip == 0 && enviro->grounded && !enviro->flying && ( framelip&7 ) < 2 )
    {
      // Do the motion stuff
      speed = ABS( pchr->ori.vel.x ) + ABS( pchr->ori.vel.y );
      if ( speed < pchr->spd_sneak )
      {
        //                        pchr->action.next = ACTION_DA;
        // Do boredom
        pchr->boretime -= dUpdate;
        if ( pchr->boretime <= 0 ) pchr->boretime = 0;

        if ( pchr->boretime <= 0 )
        {
          pstate->alert |= ALERT_BORED;
          pchr->boretime = DELAY_BORE(prand);
        }
        else
        {
          // Do standstill
          if ( pchr->action.now > ACTION_DD )
          {
            pchr->action.now = ACTION_DA;
            pchr->anim.next = pmad->actionstart[pchr->action.now];
          }
        }
      }
      else
      {
        pchr->boretime = DELAY_BORE(prand);
        if ( speed < pchr->spd_walk )
        {
          pchr->action.next = ACTION_WA;
          if ( pchr->action.now != ACTION_WA )
          {
            pchr->anim.next = pmad->frameliptowalkframe[LIPT_WA][framelip];
            pchr->action.now = ACTION_WA;
          }
        }
        else
        {
          if ( speed < pchr->spd_run )
          {
            pchr->action.next = ACTION_WB;
            if ( pchr->action.now != ACTION_WB )
            {
              pchr->anim.next = pmad->frameliptowalkframe[LIPT_WB][framelip];
              pchr->action.now = ACTION_WB;
            }
          }
          else
          {
            pchr->action.next = ACTION_WC;
            if ( pchr->action.now != ACTION_WC )
            {
              pchr->anim.next = pmad->frameliptowalkframe[LIPT_WC][framelip];
              pchr->action.now = ACTION_WC;
            }
          }
        }
      }
    }
  }

  // Characters with sticky butts lie on the surface of the mesh
  if ( enviro->grounded && ( pchr->prop.stickybutt || !pchr->alive ) )
  {
    pchr->ori.mapturn_lr = pchr->ori.mapturn_lr * 0.9 + twist_table[enviro->twist].lr * 0.1;
    pchr->ori.mapturn_ud = pchr->ori.mapturn_ud * 0.9 + twist_table[enviro->twist].ud * 0.1;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_do_environment(Game_t * gs, Chr_t * pchr, ChrEnviro_t * enviro)
{
  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PMad_t madlst      = gs->MadList;
  size_t madlst_size = MADLST_COUNT;

  Mesh_t * pmesh     = Game_getMesh(gs); 

  float wt;

  // get the level
  enviro->level = pchr->level;

  // check for flying
  enviro->flying = (0.0f != pchr->flyheight);

  // make characters slide downhill
  enviro->twist = mesh_get_twist( pmesh->Mem.tilelst, pchr->onwhichfan );

  enviro->inwater        = pchr->inwater;

  enviro->grounded       = bfalse;

  // calculate the normal dynamically from the mesh coordinates
  if ( !mesh_calc_normal( pmesh, &(gs->phys), pchr->ori.pos, &enviro->nrm ) )
  {
    enviro->nrm = twist_table[enviro->twist].nrm;
  };

  enviro->traction       = 0;
  enviro->horiz_friction = 0;
  enviro->vert_friction  = 0;
  wt = 0.0f;
  if ( enviro->inwater )
  {
    // we are partialy under water
    float lerp;

    if ( pchr->weight < 0.0f || pchr->holdingweight < 0.0f )
    {
      enviro->buoyancy = 0.0f;
    }
    else
    {
      float volume, weight;

      weight = pchr->weight + pchr->holdingweight;
      volume = ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min ) * ( pchr->bmpdata.cv.x_max - pchr->bmpdata.cv.x_min ) * ( pchr->bmpdata.cv.y_max - pchr->bmpdata.cv.y_min );

      // this adjusts the buoyancy so that the default adventurer gets a buoyancy of 0.3
      enviro->buoyancy = 0.3f * ( weight / volume ) * 1196.0f;
      if ( enviro->buoyancy < 0.0f ) enviro->buoyancy = 0.0f;
      if ( enviro->buoyancy > 1.0f ) enviro->buoyancy = 1.0f;
    };

    lerp = ( float )( gs->water.surfacelevel - pchr->ori.pos.z ) / ( float ) ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min );
    if ( lerp > 1.0f ) lerp = 1.0f;
    if ( lerp < 0.0f ) lerp = 0.0f;

    enviro->traction       += enviro->phys.waterfriction * lerp;
    enviro->horiz_friction += enviro->phys.waterfriction * lerp;
    enviro->vert_friction  += enviro->phys.waterfriction * lerp;
    enviro->buoyancy       *= lerp;

    wt += lerp;
  }

  if ( pchr->ori.pos.z < enviro->level + PLATTOLERANCE )
  {
    // we are close to something
    bool_t is_slippy;
    float lerp = ( enviro->level + PLATTOLERANCE - pchr->ori.pos.z ) / ( float ) PLATTOLERANCE;
    if ( lerp > 1.0f ) lerp = 1.0f;
    if ( lerp < 0.0f ) lerp = 0.0f;

    if ( ACTIVE_CHR( chrlst, pchr->onwhichplatform ) )
    {
      is_slippy = bfalse;
    }
    else
    {
      is_slippy = ( INVALID_FAN != pchr->onwhichfan ) && mesh_has_some_bits( pmesh->Mem.tilelst, pchr->onwhichfan, MPDFX_SLIPPY );
    }

    if ( is_slippy )
    {
      enviro->traction   += ( 1.0 - enviro->phys.slippyfriction ) * lerp;
      enviro->horiz_friction += enviro->phys.slippyfriction * lerp;
    }
    else
    {
      enviro->traction   += ( 1.0 - enviro->phys.noslipfriction ) * lerp;
      enviro->horiz_friction += enviro->phys.noslipfriction * lerp;
    };
    enviro->vert_friction += enviro->phys.airfriction * lerp;

    wt += lerp;
  };

  if ( wt < 1.0f )
  {
    // we are in clear air
    enviro->traction       += ( 1.0f - wt ) * enviro->air_traction;
    enviro->horiz_friction += ( 1.0f - wt ) * enviro->phys.airfriction;
    enviro->vert_friction  += ( 1.0f - wt ) * enviro->phys.airfriction;
  }
  else
  {
    enviro->traction       /= wt;
    enviro->horiz_friction /= wt;
    enviro->vert_friction  /= wt;
  };

  enviro->grounded = ( pchr->ori.pos.z < enviro->level + PLATTOLERANCE / 20.0f );

  // reset the jump
  pchr->jumpready  = enviro->grounded || enviro->inwater;
  if ( pchr->jumptime == 0.0f )
  {
    if ( enviro->grounded && pchr->jumpnumber < pchr->jumpnumberreset )
    {
      pchr->jumpnumber = pchr->jumpnumberreset;
      pchr->jumptime   = DELAY_JUMP;
    }
    else if ( enviro->inwater && pchr->jumpnumber < 1 )
    {
      // "Swimming"
      pchr->jumpready  = btrue;
      pchr->jumptime   = DELAY_JUMP / 3.0f;
      pchr->jumpnumber += 1;
    }
  };

  // override jumpready if the character is on a non-flat slippery surface
  if ( enviro->grounded )
  {
    // only slippy, non-flat surfaces don't allow jumps
    if ( INVALID_FAN != pchr->onwhichfan && mesh_has_some_bits( pmesh->Mem.tilelst, pchr->onwhichfan, MPDFX_SLIPPY ) )
    {
      if ( !twist_table[enviro->twist].flat )
      {
        pchr->jumpready = bfalse;
      };
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_do_latches( Game_t * gs, CHR_REF ichr, ChrEnviro_t * enviro, float dUpdate)
{
  // BB > do volontary movement

  CHR_REF weapon_ref, mount_ref, item_ref;
  float maxvel, dvx, dvy, dvmax;
  ACTION action;
  bool_t   ready, allowedtoattack, watchtarget;
  TURNMODE loc_turnmode;
  Uint32 loc_rand;

  float turnfactor;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PMad_t madlst      = gs->MadList;
  size_t madlst_size = MADLST_COUNT;

  PObj_t   objlst = gs->ObjList;
  size_t objlst_size = OBJLST_COUNT;

  Chr_t        * pchr;
  PhysAccum_t  * paccum;
  AI_STATE    * pstate;

  OBJ_REF       iobj;
  Obj_t        * pobj;
  Mad_t        * pmad;
  Cap_t        * pcap;

  loc_rand = gs->randie_index;

  pchr    = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return bfalse;
  if(!pchr->alive) return bfalse;

  paccum  = &(pchr->accum);
  pstate  = &(pchr->aistate);

  iobj = ChrList_getRObj(gs, ichr);
  if( INVALID_OBJ == iobj ) return bfalse;
  pobj = gs->ObjList + iobj;

  pmad = ChrList_getPMad(gs, ichr);
  if(NULL == pmad) return bfalse;

  pcap = ChrList_getPCap(gs, ichr);
  if(NULL == pcap) return bfalse;

  // get the mount_ref
  mount_ref = chr_get_attachedto(chrlst, chrlst_size, ichr);

  // TURNMODE_VELOCITY acts strange when someone is mounted on a "bucking" mount_ref, like the gelfeet
  loc_turnmode = pstate->turnmode;
  if ( ACTIVE_CHR( chrlst, mount_ref ) ) loc_turnmode = TURNMODE_NONE;

  // TODO : replace with line(s) below
  turnfactor = 2.0f;
  // scale the turn rate by the dexterity.
  // For zero dexterity, rate is half speed
  // For maximum dexterity, rate is 1.5 normal rate
  //turnfactor = (3.0f * (float)pchr->stats.dexterity_fp8 / (float)PERFECTSTAT + 1.0f) / 2.0f;


  // Apply the latches
  if ( !ACTIVE_CHR( chrlst, mount_ref ) )
  {
    // Character latches for generalized movement
    dvx = pstate->latch.x;
    dvy = pstate->latch.y;

    // Reverse movements for daze
    if ( pchr->dazetime > 0.0f )
    {
      dvx = -dvx;
      dvy = -dvy;
    }

    // Switch x and y for grog
    if ( pchr->grogtime > 0.0f )
    {
      dvmax = dvx;
      dvx = dvy;
      dvy = dvmax;
    }

    // Get direction from the DESIRED change in velocity
    if ( loc_turnmode == TURNMODE_WATCH )
    {
      if (( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
      {
        pchr->ori.turn_lr = terp_dir( pchr->ori.turn_lr, dvx, dvy, dUpdate * turnfactor );
      }
    }

    if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_STOP ) )
    {
      dvx = 0;
      dvy = 0;
    }

    // TODO : change to line(s) below
    maxvel = pchr->skin.maxaccel / ( 1.0 - gs->phys.noslipfriction );
    // set a minimum speed of 6 to fix some stupid slow speeds
    //maxvel = 1.5f * MAX(MAX(3,pchr->spd_run), MAX(pchr->spd_walk,pchr->spd_sneak));
    pstate->trgvel.x = dvx * maxvel;
    pstate->trgvel.y = dvy * maxvel;
    pstate->trgvel.z = 0;

    if ( pchr->skin.maxaccel > 0.0f )
    {
      dvx = ( pstate->trgvel.x - pchr->ori.vel.x );
      dvy = ( pstate->trgvel.y - pchr->ori.vel.y );

      // TODO : change to line(s) below
      dvmax = pchr->skin.maxaccel;
      // Limit to max acceleration
      //if(maxvel==0.0)
      //{
      //  dvmax = 2.0f * pchr->maxaccel;
      //}
      //else
      //{
      //  float ftmp;
      //  chrvel2 = pchr->ori.vel.x*pchr->ori.vel.x + pchr->ori.vel.y*pchr->ori.vel.y;
      //  ftmp = MIN(1.0 , chrvel2/maxvel/maxvel);
      //  dvmax   = 2.0f * pchr->maxaccel * (1.0-ftmp);
      //};

      if ( dvx < -dvmax ) dvx = -dvmax;
      if ( dvx >  dvmax ) dvx =  dvmax;
      if ( dvy < -dvmax ) dvy = -dvmax;
      if ( dvy >  dvmax ) dvy =  dvmax;

      enviro->traction *= 11.0f;                    // 11.0f corrects traction so that it gives full traction for non-slip floors in advent.mod
      enviro->traction = MIN( 1.0, enviro->traction );

      paccum->acc.x += dvx * enviro->traction * enviro->nrm.z;
      paccum->acc.y += dvy * enviro->traction * enviro->nrm.z;
    };
  }

  // Apply chrlst[].latch.x and chrlst[].latch.y
  if ( !ACTIVE_CHR( chrlst, mount_ref ) )
  {
    // Face the target
    watchtarget = ( loc_turnmode == TURNMODE_WATCHTARGET );
    if ( watchtarget )
    {
      CHR_REF ai_target = chr_get_aitarget( chrlst, chrlst_size, chrlst + ichr );
      if ( ACTIVE_CHR( chrlst, ai_target ) && ichr != ai_target )
      {
        pchr->ori.turn_lr = terp_dir( pchr->ori.turn_lr, chrlst[ai_target].ori.pos.x - pchr->ori.pos.x, chrlst[ai_target].ori.pos.y - pchr->ori.pos.y, dUpdate * turnfactor );
      };
    }

    // Get direction from ACTUAL change in velocity
    if ( loc_turnmode == TURNMODE_VELOCITY )
    {
      if ( chr_is_player(gs, ichr) )
        pchr->ori.turn_lr = terp_dir( pchr->ori.turn_lr, pstate->trgvel.x, pstate->trgvel.y, dUpdate * turnfactor );
      else
        pchr->ori.turn_lr = terp_dir( pchr->ori.turn_lr, pstate->trgvel.x, pstate->trgvel.y, dUpdate * turnfactor / 4.0f );
    }

    // Otherwise make it spin
    else if ( loc_turnmode == TURNMODE_SPIN )
    {
      pchr->ori.turn_lr += SPINRATE * dUpdate * turnfactor;
    }
  };

  // Character latches for generalized buttons
  if ( LATCHBUTTON_NONE != pstate->latch.b )
  {
    if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_JUMP ) && pchr->jumptime == 0.0f )
    {
      if ( detach_character_from_mount( gs, ichr, btrue, btrue ) )
      {
        pchr->jumptime = DELAY_JUMP;
        paccum->vel.z += !enviro->flying ? DISMOUNTZVEL : DISMOUNTZVELFLY;
        if ( pchr->jumpnumberreset != JUMPINFINITE && pchr->jumpnumber > 0 )
          pchr->jumpnumber -= dUpdate;

        // Play the jump sound
        if ( INVALID_SOUND != pcap->jumpsound )
        {
          snd_play_sound( gs, 1.0f, pchr->ori.pos, pobj->wavelist[pcap->jumpsound], 0, iobj, pcap->jumpsound );
        };
      }
      else if ( pchr->jumpnumber > 0 && ( pchr->jumpready || pchr->jumpnumberreset > 1 ) )
      {
        // Make the character jump
        if ( enviro->inwater && !enviro->grounded )
        {
          paccum->vel.z += WATERJUMP / 3.0f;
          pchr->jumptime = DELAY_JUMP / 3.0f;
        }
        else
        {
          paccum->vel.z += pchr->jump * 2.0f;
          pchr->jumptime = DELAY_JUMP;

          // Set to jump animation if not doing anything better
          if ( pchr->action.ready )    play_action( gs, ichr, ACTION_JA, btrue );

          // Play the jump sound (Boing!)
          if ( INVALID_SOUND != pcap->jumpsound )
          {
            snd_play_sound( gs, MIN( 1.0f, pchr->jump / 50.0f ), pchr->ori.pos, pobj->wavelist[pcap->jumpsound], 0, iobj, pcap->jumpsound );
          }
        };

        pchr->hitready  = btrue;
        pchr->jumpready = bfalse;
        if ( pchr->jumpnumberreset != JUMPINFINITE ) pchr->jumpnumber -= dUpdate;
      }
    }

    if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_ALTLEFT ) && pchr->action.ready && pchr->reloadtime == 0 )
    {
      pchr->reloadtime = DELAY_GRAB;
      if ( !chr_using_slot( chrlst, chrlst_size, ichr, SLOT_LEFT ) )
      {
        // Grab left
        play_action( gs, ichr, ACTION_ME, bfalse );
      }
      else
      {
        // Drop left
        play_action( gs, ichr, ACTION_MA, bfalse );
      }
    }

    if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_ALTRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
    {
      pchr->reloadtime = DELAY_GRAB;
      if ( !chr_using_slot( chrlst, chrlst_size, ichr, SLOT_RIGHT ) )
      {
        // Grab right
        play_action( gs, ichr, ACTION_MF, bfalse );
      }
      else
      {
        // Drop right
        play_action( gs, ichr, ACTION_MB, bfalse );
      }
    }

    if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_PACKLEFT ) && pchr->action.ready && pchr->reloadtime == 0 )
    {
      pchr->reloadtime = DELAY_PACK;
      item_ref = chr_get_holdingwhich( chrlst, chrlst_size, ichr, SLOT_LEFT );
      if ( ACTIVE_CHR( chrlst, item_ref ) )
      {
        if (( chrlst[item_ref].prop.iskursed || chrlst[item_ref].prop.istoobig ) && !chrlst[item_ref].prop.isequipment )
        {
          // The item_ref couldn't be put away
          chrlst[item_ref].aistate.alert |= ALERT_NOTPUTAWAY;
        }
        else
        {
          // Put the item_ref into the pack
          pack_add_item( gs, item_ref, ichr );
        }
      }
      else
      {
        // Get a new one out and put it in hand
        pack_get_item( gs, ichr, SLOT_LEFT, bfalse );
      }

      // Make it take a little time
      play_action( gs, ichr, ACTION_MG, bfalse );
    }

    if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_PACKRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
    {
      pchr->reloadtime = DELAY_PACK;
      item_ref = chr_get_holdingwhich( chrlst, chrlst_size, ichr, SLOT_RIGHT );
      if ( ACTIVE_CHR( chrlst, item_ref ) )
      {
        if (( chrlst[item_ref].prop.iskursed || chrlst[item_ref].prop.istoobig ) && !chrlst[item_ref].prop.isequipment )
        {
          // The item_ref couldn't be put away
          chrlst[item_ref].aistate.alert |= ALERT_NOTPUTAWAY;
        }
        else
        {
          // Put the item_ref into the pack
          pack_add_item( gs, item_ref, ichr );
        }
      }
      else
      {
        // Get a new one out and put it in hand
        pack_get_item( gs, ichr, SLOT_RIGHT, bfalse );
      }

      // Make it take a little time
      play_action( gs,  ichr, ACTION_MG, bfalse );
    }

    if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_LEFT ) && pchr->reloadtime == 0 )
    {
      // Which weapon_ref?
      weapon_ref = chr_get_holdingwhich( chrlst, chrlst_size, ichr, SLOT_LEFT );
      if ( !ACTIVE_CHR( chrlst, weapon_ref ) )
      {
        // Unarmed means character itself is the weapon_ref
        weapon_ref = ichr;
      }
      action = ChrList_getPCap(gs, weapon_ref)->weaponaction;

      // Can it do it?
      allowedtoattack = btrue;
      if ( !pmad->actionvalid[action] || chrlst[weapon_ref].reloadtime > 0 ||
        ( ChrList_getPCap(gs, weapon_ref)->needskillidtouse && !check_skills( gs, ichr, ChrList_getPCap(gs, weapon_ref)->idsz[IDSZ_SKILL] ) ) )
      {
        allowedtoattack = bfalse;
        if ( chrlst[weapon_ref].reloadtime == 0 )
        {
          // This character can't use this weapon_ref
          chrlst[weapon_ref].reloadtime = 50;
          if ( pchr->staton )
          {
            // Tell the player that they can't use this weapon_ref
            debug_message( 1, "%s can't use this item_ref...", pchr->name );
          }
        }
      }

      if ( action == ACTION_DA )
      {
        allowedtoattack = bfalse;
        if ( chrlst[weapon_ref].reloadtime == 0 )
        {
          chrlst[weapon_ref].aistate.alert |= ALERT_USED;
        }
      }

      if ( allowedtoattack )
      {
        // Rearing mount_ref
        if ( ACTIVE_CHR( chrlst, mount_ref ) )
        {
          allowedtoattack = chrlst[mount_ref].prop.ridercanattack;
          if ( chrlst[mount_ref].prop.ismount && chrlst[mount_ref].alive && !chr_is_player(gs, mount_ref) && chrlst[mount_ref].action.ready )
          {
            if (( action != ACTION_PA || !allowedtoattack ) && pchr->action.ready )
            {
              play_action( gs,  mount_ref, ( ACTION )( ACTION_UA + IRAND(&loc_rand, 1) ), bfalse );
              chrlst[mount_ref].aistate.alert |= ALERT_USED;
            }
            else
            {
              allowedtoattack = bfalse;
            }
          }
        }

        // Attack button
        if ( allowedtoattack )
        {
          if ( pchr->action.ready && pmad->actionvalid[action] )
          {
            // Check mana cost
            if ( pchr->stats.mana_fp8 >= chrlst[weapon_ref].manacost_fp8 || pchr->prop.canchannel )
            {
              cost_mana( gs, ichr, chrlst[weapon_ref].manacost_fp8, weapon_ref );
              // Check life healing
              pchr->stats.life_fp8 += chrlst[weapon_ref].stats.lifeheal_fp8;
              if ( pchr->stats.life_fp8 > pchr->stats.lifemax_fp8 )  pchr->stats.life_fp8 = pchr->stats.lifemax_fp8;
              ready = ( action == ACTION_PA );
              action = (ACTION)(action + IRAND(&loc_rand, 1));
              play_action( gs,  ichr, action, ready );
              if ( weapon_ref != ichr )
              {
                // Make the weapon_ref attack too
                play_action( gs,  weapon_ref, ACTION_MJ, bfalse );
                chrlst[weapon_ref].aistate.alert |= ALERT_USED;
              }
              else
              {
                // Flag for unarmed attack
                pstate->alert |= ALERT_USED;
              }
            }
          }
        }
      }
    }
    else if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_RIGHT ) && pchr->reloadtime == 0 )
    {
      // Which weapon_ref?
      weapon_ref = chr_get_holdingwhich( chrlst, chrlst_size, ichr, SLOT_RIGHT );
      if ( !ACTIVE_CHR( chrlst, weapon_ref ) )
      {
        // Unarmed means character itself is the weapon_ref
        weapon_ref = ichr;
      }
      action = (ACTION)(ChrList_getPCap(gs, weapon_ref)->weaponaction + 2);

      // Can it do it?
      allowedtoattack = btrue;
      if ( !pmad->actionvalid[action] || chrlst[weapon_ref].reloadtime > 0 ||
        ( ChrList_getPCap(gs, weapon_ref)->needskillidtouse && !check_skills( gs, ichr, ChrList_getPCap(gs, weapon_ref)->idsz[IDSZ_SKILL] ) ) )
      {
        allowedtoattack = bfalse;
        if ( chrlst[weapon_ref].reloadtime == 0 )
        {
          // This character can't use this weapon_ref
          chrlst[weapon_ref].reloadtime = 50;
          if ( pchr->staton )
          {
            // Tell the player that they can't use this weapon_ref
            debug_message( 1, "%s can't use this item_ref...", pchr->name );
          }
        }
      }
      if ( action == ACTION_DC )
      {
        allowedtoattack = bfalse;
        if ( chrlst[weapon_ref].reloadtime == 0 )
        {
          chrlst[weapon_ref].aistate.alert |= ALERT_USED;
        }
      }

      if ( allowedtoattack )
      {
        // Rearing mount_ref
        if ( ACTIVE_CHR( chrlst, mount_ref ) )
        {
          allowedtoattack = chrlst[mount_ref].prop.ridercanattack;
          if ( chrlst[mount_ref].prop.ismount && chrlst[mount_ref].alive && !chr_is_player(gs, mount_ref) && chrlst[mount_ref].action.ready )
          {
            if (( action != ACTION_PC || !allowedtoattack ) && pchr->action.ready )
            {
              play_action( gs,  mount_ref, ( ACTION )( ACTION_UC + IRAND(&loc_rand, 1) ), bfalse );
              chrlst[mount_ref].aistate.alert |= ALERT_USED;
            }
            else
            {
              allowedtoattack = bfalse;
            }
          }
        }

        // Attack button
        if ( allowedtoattack )
        {
          if ( pchr->action.ready && pmad->actionvalid[action] )
          {
            // Check mana cost
            if ( pchr->stats.mana_fp8 >= chrlst[weapon_ref].manacost_fp8 || pchr->prop.canchannel )
            {
              cost_mana( gs, ichr, chrlst[weapon_ref].manacost_fp8, weapon_ref );
              // Check life healing
              pchr->stats.life_fp8 += chrlst[weapon_ref].stats.lifeheal_fp8;
              if ( pchr->stats.life_fp8 > pchr->stats.lifemax_fp8 )  pchr->stats.life_fp8 = pchr->stats.lifemax_fp8;
              ready = ( action == ACTION_PC );
              action = (ACTION)(action + IRAND(&loc_rand, 1));
              play_action( gs,  ichr, action, ready );
              if ( weapon_ref != ichr )
              {
                // Make the weapon_ref attack too
                play_action( gs,  weapon_ref, ACTION_MJ, bfalse );
                chrlst[weapon_ref].aistate.alert |= ALERT_USED;
              }
              else
              {
                // Flag for unarmed attack
                pstate->alert |= ALERT_USED;
              }
            }
          }
        }
      }
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_do_physics( Game_t * gs, Chr_t * pchr, ChrEnviro_t * enviro, float dUpdate)
{
  PhysAccum_t * paccum = &(pchr->accum);

  Mesh_t * pmesh     = Game_getMesh(gs); 

  if(!EKEY_PVALID(gs) || !EKEY_PVALID(pchr)  || !EKEY_PVALID(enviro)) return bfalse;

  // Integrate the z direction
  if ( enviro->flying )
  {
    if ( enviro->level < 0 ) paccum->pos.z += enviro->level - pchr->ori.pos.z; // Don't fall in pits...
    paccum->acc.z += ( enviro->level + pchr->flyheight - pchr->ori.pos.z ) * FLYDAMPEN;

    enviro->vert_friction = 1.0;
  }
  else if ( pchr->ori.pos.z > enviro->level + PLATTOLERANCE )
  {
    paccum->acc.z += gs->phys.gravity;
  }
  else
  {
    float lerp_normal, lerp_tang;
    lerp_tang = ( enviro->level + PLATTOLERANCE - pchr->ori.pos.z ) / ( float ) PLATTOLERANCE;
    if ( lerp_tang > 1.0f ) lerp_tang = 1.0f;
    if ( lerp_tang < 0.0f ) lerp_tang = 0.0f;

    // fix to make sure characters will hit the ground softly, but in a reasonable time
    lerp_normal = 1.0 - lerp_tang;
    if ( lerp_normal > 1.0f ) lerp_normal = 1.0f;
    if ( lerp_normal < 0.2f ) lerp_normal = 0.2f;

    // slippy hills make characters slide
    if ( pchr->weight > 0 && gs->water.iswater && !enviro->inwater && INVALID_FAN != pchr->onwhichfan && mesh_has_some_bits( pmesh->Mem.tilelst, pchr->onwhichfan, MPDFX_SLIPPY ) )
    {
      paccum->acc.x -= enviro->nrm.x * gs->phys.gravity * lerp_tang * gs->phys.hillslide;
      paccum->acc.y -= enviro->nrm.y * gs->phys.gravity * lerp_tang * gs->phys.hillslide;
      paccum->acc.z += enviro->nrm.z * gs->phys.gravity * lerp_normal;
    }
    else
    {
      paccum->acc.z += gs->phys.gravity * lerp_normal;
    };
  }

  // do buoyancy
  paccum->acc.z -= enviro->buoyancy * enviro->phys.gravity;

  // Apply friction for next time
  paccum->acc.x -= ( 1.0f - enviro->horiz_friction ) * pchr->ori.vel.x;
  paccum->acc.y -= ( 1.0f - enviro->horiz_friction ) * pchr->ori.vel.y;
  paccum->acc.z -= ( 1.0f - enviro->vert_friction  ) * pchr->ori.vel.z;

  return btrue;
}

//--------------------------------------------------------------------------------------------
void move_characters( Game_t * gs, float dUpdate )
{
  // ZZ> This function handles character physics

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PMad_t madlst      = gs->MadList;
  size_t madlst_size = MADLST_COUNT;

  CHR_REF chr_ref;

  ChrEnviro_t enviro, loc_enviro;

  AI_STATE * pstate;
  Chr_t * pchr;
  Uint32 loc_rand;

  CChrEnviro_new( &enviro, &(gs->phys) );
  CChrEnviro_init( &enviro, dUpdate );

  loc_rand = gs->randie_index;

  // Move every character
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    pchr = ChrList_getPChr(gs, chr_ref);
    if(NULL == pchr) continue;

    pstate = &(pchr->aistate);

    // Character's old location
    pchr->ori_old.turn_lr = pchr->ori.turn_lr;

    if ( chr_in_pack( chrlst, chrlst_size, chr_ref ) ) continue;

    // initialize the physics data for this character
    memcpy(&loc_enviro, &enviro, sizeof(ChrEnviro_t));

    chr_do_environment(gs, pchr, &enviro);

    chr_do_latches( gs, chr_ref, &enviro, dUpdate);

    chr_do_physics(  gs, pchr, &enviro, dUpdate );

    chr_do_animation(gs, dUpdate, chr_ref, &enviro, &loc_rand);


    // Down the jump timer if we are on a valid surface
    pchr->jumptime  -= dUpdate;
    if ( pchr->jumptime < 0 ) pchr->jumptime = 0.0f;

    // Do "Be careful!" delay
    pchr->carefultime -= dUpdate;
    if ( pchr->carefultime <= 0 ) pchr->carefultime = 0;

    // Down that ol' damage timer
    pchr->damagetime -= dUpdate;
    if ( pchr->damagetime < 0 ) pchr->damagetime = 0.0f;
  }

}

//--------------------------------------------------------------------------------------------
bool_t PlaList_set_latch( Game_t * gs, Player_t * ppla )
{
  // ZZ> This function converts input readings to latch settings, so players can
  //     move around

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  float newx, newy;
  Uint16 turnsin;
  CHR_REF character;
  Uint8 device;
  float dist;
  float inputx, inputy;

  // Check to see if we need to bother
  if( !EKEY_PVALID(ppla) || !ppla->Active || INBITS_NONE == ppla->device) return bfalse;

  // Make life easier
  character = ppla->chr_ref;
  device    = ppla->device;

  if( !VALID_CHR(gs->ChrList, character) )
  {
    ppla->Active  = bfalse;
    ppla->chr_ref = INVALID_CHR;
    ppla->device  = INBITS_NONE;
    return bfalse;
  }

  // Clear the player's latch buffers
  ppla->latch.b = 0;
  ppla->latch.x *= mous.sustain;
  ppla->latch.y *= mous.sustain;

  // Mouse routines
  if ( HAS_SOME_BITS( device, INBITS_MOUS ) && mous.on )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_mouse_is_pressed( CONTROL_CAMERA ) )   // Don't allow movement in camera control mode
    {
      inputx = 0;
      inputy = 0;
      dist = mous.dlatch.x * mous.dlatch.x + mous.dlatch.y * mous.dlatch.y;

      if ( dist > 0.0 )
      {
        dist = sqrt( dist );
        inputx = ( float ) mous.dlatch.x / ( mous.sense + dist );
        inputy = ( float ) mous.dlatch.y / ( mous.sense + dist );
      }
      if ( CData.autoturncamera == 255 && control_mouse_is_pressed( CONTROL_CAMERA ) == 0 )  inputx = 0;

      turnsin = ((Uint16)GCamera.turn_lr) >> 2;
      newy = ( inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
      newx = (-inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
    }

    ppla->latch.x += newx * mous.cover * 5;
    ppla->latch.y += newy * mous.cover * 5;

    // Read buttons
    if ( control_mouse_is_pressed( CONTROL_JUMP ) )
    {
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && ACTIVE_CHR( chrlst, character ) && !chrlst[character].alive )
      {
        ppla->latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        ppla->latch.b |= LATCHBUTTON_JUMP;
      }
    };

    if ( control_mouse_is_pressed( CONTROL_LEFT_USE ) )
      ppla->latch.b |= LATCHBUTTON_LEFT;

    if ( control_mouse_is_pressed( CONTROL_LEFT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_mouse_is_pressed( CONTROL_LEFT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_USE ) )
      ppla->latch.b |= LATCHBUTTON_RIGHT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKRIGHT;
  }

  // Joystick A routines
  if ( HAS_SOME_BITS( device, INBITS_JOYA ) && joy[0].on )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_joy_is_pressed( 0, CONTROL_CAMERA ) )
    {
      inputx = joy[0].latch.x;
      inputy = joy[0].latch.y;
      dist = joy[0].latch.x * joy[0].latch.x + joy[0].latch.y * joy[0].latch.y;
      if ( dist > 1.0 )
      {
        dist = sqrt( dist );
        inputx /= dist;
        inputy /= dist;
      }
      if ( CData.autoturncamera == 255 && !control_joy_is_pressed( 0, CONTROL_CAMERA ) )  inputx = 0;

      turnsin = ((Uint16)GCamera.turn_lr) >> 2;
      newy = ( inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
      newx = (-inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
    }

    ppla->latch.x += newx * mous.cover;
    ppla->latch.y += newy * mous.cover;

    // Read buttons
    if ( control_joy_is_pressed( 0, CONTROL_JUMP ) )
    {
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && ACTIVE_CHR( chrlst, character ) && !chrlst[character].alive )
      {
        ppla->latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        ppla->latch.b |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_joy_is_pressed( 0, CONTROL_LEFT_USE ) )
      ppla->latch.b |= LATCHBUTTON_LEFT;

    if ( control_joy_is_pressed( 0, CONTROL_LEFT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_joy_is_pressed( 0, CONTROL_LEFT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_joy_is_pressed( 0, CONTROL_RIGHT_USE ) )
      ppla->latch.b |= LATCHBUTTON_RIGHT;

    if ( control_joy_is_pressed( 0, CONTROL_RIGHT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_joy_is_pressed( 0, CONTROL_RIGHT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKRIGHT;
  }

  // Joystick B routines
  if ( HAS_SOME_BITS( device, INBITS_JOYB ) && joy[1].on )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_joy_is_pressed( 1, CONTROL_CAMERA ) )
    {
      inputx = joy[1].latch.x;
      inputy = joy[1].latch.y;
      dist = joy[1].latch.x * joy[1].latch.x + joy[1].latch.y * joy[1].latch.y;
      if ( dist > 1.0 )
      {
        dist = sqrt( dist );
        inputx = joy[1].latch.x / dist;
        inputy = joy[1].latch.y / dist;
      }
      if ( CData.autoturncamera == 255 && !control_joy_is_pressed( 1, CONTROL_CAMERA ) )  inputx = 0;

      turnsin = ((Uint16)GCamera.turn_lr) >> 2;
      newy = ( inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
      newx = (-inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
    }

    ppla->latch.x += newx * mous.cover;
    ppla->latch.y += newy * mous.cover;

    // Read buttons
    if ( control_joy_is_pressed( 1, CONTROL_JUMP ) )
    {
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && ACTIVE_CHR( chrlst, character ) && !chrlst[character].alive )
      {
        ppla->latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        ppla->latch.b |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_joy_is_pressed( 1, CONTROL_LEFT_USE ) )
      ppla->latch.b |= LATCHBUTTON_LEFT;

    if ( control_joy_is_pressed( 1, CONTROL_LEFT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_joy_is_pressed( 1, CONTROL_LEFT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_joy_is_pressed( 1, CONTROL_RIGHT_USE ) )
      ppla->latch.b |= LATCHBUTTON_RIGHT;

    if ( control_joy_is_pressed( 1, CONTROL_RIGHT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_joy_is_pressed( 1, CONTROL_RIGHT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKRIGHT;
  }

  // Keyboard routines
  if ( HAS_SOME_BITS( device, INBITS_KEYB ) && keyb.on )
  {
    // Movement
    newx = 0;
    newy = 0;
    inputx = inputy = 0;
    if ( control_key_is_pressed( (CONTROL)KEY_RIGHT ) ) inputx += 1;
    if ( control_key_is_pressed( (CONTROL)KEY_LEFT  ) ) inputx -= 1;
    if ( control_key_is_pressed( (CONTROL)KEY_DOWN  ) ) inputy += 1;
    if ( control_key_is_pressed( (CONTROL)KEY_UP    ) ) inputy -= 1;

    dist = inputx * inputx + inputy * inputy;
    if ( dist > 1.0 )
    {
      dist = sqrt( dist );
      inputx /= dist;
      inputy /= dist;
    }
    if ( CData.autoturncamera == 255 && gs->cl->loc_pla_count == 1 )  inputx = 0;

    turnsin = ((Uint16)GCamera.turn_lr) >> 2;
    newy = ( inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
    newx = (-inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );

    ppla->latch.x += newx * mous.cover;
    ppla->latch.y += newy * mous.cover;

    // Read buttons
    if ( control_key_is_pressed( CONTROL_JUMP ) )
    {
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && ACTIVE_CHR( chrlst, character ) && !chrlst[character].alive )
      {
        ppla->latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        ppla->latch.b |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_key_is_pressed( CONTROL_LEFT_USE ) )
      ppla->latch.b |= LATCHBUTTON_LEFT;

    if ( control_key_is_pressed( CONTROL_LEFT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_key_is_pressed( CONTROL_LEFT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_key_is_pressed( CONTROL_RIGHT_USE ) )
      ppla->latch.b |= LATCHBUTTON_RIGHT;

    if ( control_key_is_pressed( CONTROL_RIGHT_GET ) )
      ppla->latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_key_is_pressed( CONTROL_RIGHT_PACK ) )
      ppla->latch.b |= LATCHBUTTON_PACKRIGHT;
  }

  dist = ppla->latch.x * ppla->latch.x + ppla->latch.y * ppla->latch.y;
  if ( dist > 1 )
  {
    dist = sqrt( dist );
    ppla->latch.x /= dist;
    ppla->latch.y /= dist;
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
void set_local_latches( Game_t * gs )
{
  // ZZ> This function emulates AI thinkin' by setting latches from input devices

  PLA_REF cnt;

  cnt = 0;
  while ( cnt < PLALST_COUNT )
  {
    PlaList_set_latch( gs, gs->PlaList + cnt );
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
float get_one_level( Game_t * gs, CHR_REF chr_ref )
{
  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  Mesh_t * pmesh     = Game_getMesh(gs); 

  float level;
  CHR_REF platform;

  // return a default for invalid characters
  if ( !ACTIVE_CHR( chrlst, chr_ref ) ) return bfalse;

  // return the cached value for pre-calculated characters
  if ( chrlst[chr_ref].levelvalid ) return chrlst[chr_ref].level;

  //get the base level
  chrlst[chr_ref].onwhichfan = mesh_get_fan( pmesh, chrlst[chr_ref].ori.pos );
  level = mesh_get_level( &(pmesh->Mem), chrlst[chr_ref].onwhichfan, chrlst[chr_ref].ori.pos.x, chrlst[chr_ref].ori.pos.y, chrlst[chr_ref].prop.waterwalk, &(gs->water) );

  // if there is a platform, choose whichever is higher
  platform = chr_get_onwhichplatform( chrlst, chrlst_size, chr_ref );
  if ( ACTIVE_CHR( chrlst, platform ) )
  {
    float ftmp = chrlst[platform].bmpdata.cv.z_max;
    level = MAX( level, ftmp );
  }

  chrlst[chr_ref].level      = level;
  chrlst[chr_ref].levelvalid = btrue;

  return chrlst[chr_ref].level;
};

//--------------------------------------------------------------------------------------------
void get_all_levels( Game_t * gs )
{
  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  CHR_REF chr_ref;

  // Initialize all the objects
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !ACTIVE_CHR( chrlst, chr_ref ) ) continue;

    chrlst[chr_ref].onwhichfan = INVALID_FAN;
    chrlst[chr_ref].levelvalid = bfalse;
  };

  // do the levels
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !ACTIVE_CHR( chrlst, chr_ref ) || chrlst[chr_ref].levelvalid ) continue;
    get_one_level( gs, chr_ref );
  }

}

//--------------------------------------------------------------------------------------------
void make_onwhichfan( Game_t * gs )
{
  // ZZ> This function figures out which fan characters are on and sets their level

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  Mesh_t * pmesh     = Game_getMesh(gs); 

  CHR_REF chr_ref;
  int ripand;

  float  splashstrength = 1.0f, ripplesize = 1.0f, ripplestrength = 0.0f;
  bool_t is_inwater    = bfalse;
  bool_t is_underwater = bfalse;

  // Get levels every update
  get_all_levels(gs);

  // Get levels every update
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !ACTIVE_CHR( chrlst, chr_ref ) ) continue;

    is_inwater = is_underwater = bfalse;
    splashstrength = 0.0f;
    ripplesize = 0.0f;
    if ( INVALID_FAN != chrlst[chr_ref].onwhichfan && mesh_has_some_bits( pmesh->Mem.tilelst, chrlst[chr_ref].onwhichfan, MPDFX_WATER ) )
    {
      splashstrength = chrlst[chr_ref].bmpdata.calc_size_big / 45.0f * chrlst[chr_ref].bmpdata.calc_size / 30.0f;
      if ( chrlst[chr_ref].ori.vel.z > 0.0f ) splashstrength *= 0.5;
      splashstrength *= ABS( chrlst[chr_ref].ori.vel.z ) / 10.0f;
      splashstrength *= chrlst[chr_ref].bumpstrength;
      if ( chrlst[chr_ref].ori.pos.z < gs->water.surfacelevel )
      {
        is_inwater = btrue;
      }

      ripplesize = ( chrlst[chr_ref].bmpdata.calc_size + chrlst[chr_ref].bmpdata.calc_size_big ) * 0.5f;
      if ( chrlst[chr_ref].bmpdata.cv.z_max < gs->water.surfacelevel )
      {
        is_underwater = btrue;
      }

      // scale the ripple strength
      ripplestrength = - ( chrlst[chr_ref].bmpdata.cv.z_min - gs->water.surfacelevel ) * ( chrlst[chr_ref].bmpdata.cv.z_max - gs->water.surfacelevel );
      ripplestrength /= 0.75f * chrlst[chr_ref].bmpdata.calc_height * chrlst[chr_ref].bmpdata.calc_height;
      ripplestrength *= ripplesize / 37.5f * chrlst[chr_ref].bumpstrength;
      ripplestrength = MAX( 0.0f, ripplestrength );
    };

    // splash stuff
    if ( chrlst[chr_ref].inwater != is_inwater && splashstrength > 0.1f )
    {
      PRT_SPAWN_INFO si;
      vect3 prt_pos = {chrlst[chr_ref].ori.pos.x, chrlst[chr_ref].ori.pos.y, gs->water.surfacelevel + RAISE};
      PRT_REF prt_index;

      // Splash
      prt_spawn_info_init( &si, gs, splashstrength, prt_pos, 0, INVALID_OBJ, PIP_REF(PRTPIP_SPLASH), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, 0, INVALID_CHR );
      prt_index = req_spawn_one_particle( si );

      if( RESERVED_PRT(gs->PrtList, prt_index) )
      {
        Prt_t * pprt = gs->PrtList + prt_index;

        // scale the size of the particle
        pprt->size_fp8 *= splashstrength;

        // scale the animation speed so that velocity appears the same
        if ( 0 != pprt->imageadd_fp8 && 0 != pprt->sizeadd_fp8 )
        {
          splashstrength = sqrt( splashstrength );
          pprt->imageadd_fp8 /= splashstrength;
          pprt->sizeadd_fp8  /= splashstrength;
        }
        else
        {
          pprt->imageadd_fp8 /= splashstrength;
          pprt->sizeadd_fp8  /= splashstrength;
        }

        chrlst[chr_ref].inwater = is_inwater;
        if ( gs->water.iswater && is_inwater )
        {
          chrlst[chr_ref].aistate.alert |= ALERT_INWATER;
        }
      }
    }
    else if ( is_inwater && ripplestrength > 0.0f )
    {
      // Ripples
      PRT_SPAWN_INFO si;

      ripand = ((( int ) chrlst[chr_ref].ori.vel.x ) != 0 ) | ((( int ) chrlst[chr_ref].ori.vel.y ) != 0 );
      ripand = RIPPLEAND >> ripand;
      if ( 0 == ( gs->wld_frame&ripand ) )
      {
        vect3  prt_pos = {chrlst[chr_ref].ori.pos.x, chrlst[chr_ref].ori.pos.y, gs->water.surfacelevel};
        PRT_REF prt_index;

        prt_spawn_info_init( &si, gs, ripplestrength, prt_pos, 0, INVALID_OBJ, PIP_REF(PRTPIP_RIPPLE), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, 0, INVALID_CHR );
        prt_index = req_spawn_one_particle( si );

        if( RESERVED_PRT(gs->PrtList, prt_index) )
        {
          Prt_t * pprt = gs->PrtList + prt_index;

          // scale the size of the particle
          pprt->size_fp8 *= ripplesize;

          // scale the animation speed so that velocity appears the same
          if ( 0 != pprt->imageadd_fp8 && 0 != pprt->sizeadd_fp8 )
          {
            ripplesize = sqrt( ripplesize );
            pprt->imageadd_fp8 /= ripplesize;
            pprt->sizeadd_fp8  /= ripplesize;
          }
          else
          {
            pprt->imageadd_fp8 /= ripplesize;
            pprt->sizeadd_fp8  /= ripplesize;
          }
        }
      }
    }

    // damage tile stuff
    if ( mesh_has_some_bits( pmesh->Mem.tilelst, chrlst[chr_ref].onwhichfan, MPDFX_DAMAGE ) && chrlst[chr_ref].ori.pos.z <= gs->water.surfacelevel + DAMAGERAISE )
    {
      Uint8 loc_damagemodifier;
      CHR_REF imount;

      // augment the rider's damage immunity with the mount's
      loc_damagemodifier = chrlst[chr_ref].skin.damagemodifier_fp8[GTile_Dam.type];
      imount = chr_get_attachedto(chrlst, chrlst_size, chr_ref);
      if ( ACTIVE_CHR(chrlst, imount) )
      {
        Uint8 modbits1, modbits2, modshift1, modshift2;
        Uint8 tmp_damagemodifier;

        tmp_damagemodifier = chrlst[imount].skin.damagemodifier_fp8[GTile_Dam.type];

        modbits1  = loc_damagemodifier & (~DAMAGE_SHIFT);
        modshift1 = loc_damagemodifier & DAMAGE_SHIFT;

        modbits2  = tmp_damagemodifier & (~DAMAGE_SHIFT);
        modshift2 = tmp_damagemodifier & DAMAGE_SHIFT;

        loc_damagemodifier = (modbits1 | modbits2) | MAX(modshift1, modshift2);
      }

      // DAMAGE_SHIFT means they're pretty well immune
      if ( !HAS_ALL_BITS(loc_damagemodifier, DAMAGE_SHIFT ) && !chrlst[chr_ref].prop.invictus )
      {
        PRT_SPAWN_INFO si;

        if ( chrlst[chr_ref].damagetime == 0 )
        {
          PAIR ptemp = {GTile_Dam.amount, 1};
          damage_character( gs, chr_ref, 32768, &ptemp, GTile_Dam.type, TEAM_REF(TEAM_DAMAGE), chr_get_aibumplast( chrlst, chrlst_size, chrlst + chr_ref ), DAMFX_BLOC | DAMFX_ARMO );
          chrlst[chr_ref].damagetime = DELAY_DAMAGETILE;
        }

        if ( INVALID_PIP != GTile_Dam.parttype && ( gs->wld_frame&GTile_Dam.partand ) == 0 )
        {
          prt_spawn_info_init( &si, gs, 1.0f, chrlst[chr_ref].ori.pos,
                              0, INVALID_OBJ, GTile_Dam.parttype, INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, 0, INVALID_CHR );
          req_spawn_one_particle( si );
        }

      }

      if ( chrlst[chr_ref].reaffirmdamagetype == GTile_Dam.type )
      {
        if (( gs->wld_frame&TILEREAFFIRMAND ) == 0 )
          reaffirm_attached_particles( gs, chr_ref );
      }
    }

  }

}

//--------------------------------------------------------------------------------------------
bool_t remove_from_platform( Game_t * gs, CHR_REF object_ref )
{
  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  CHR_REF platform;
  if ( !ACTIVE_CHR( chrlst, object_ref ) ) return bfalse;

  platform  = chr_get_onwhichplatform( chrlst, chrlst_size, object_ref );
  if ( !ACTIVE_CHR( chrlst, platform ) ) return bfalse;

  if ( chrlst[object_ref].weight > 0.0f )
    chrlst[platform].weight -= chrlst[object_ref].weight;

  chrlst[object_ref].onwhichplatform = INVALID_CHR;
  chrlst[object_ref].level           = chrlst[platform].level;

  if ( chr_is_player(gs, object_ref) && CData.DevMode )
  {
    debug_message( 1, "removed %s(%s) from platform", chrlst[object_ref].name, ChrList_getPCap(gs, object_ref)->classname );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_to_platform( Game_t * gs, CHR_REF object_ref, CHR_REF platform )
{
  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  remove_from_platform( gs, object_ref );

  if ( !ACTIVE_CHR( chrlst, object_ref ) || !ACTIVE_CHR( chrlst, platform ) ) return
      bfalse;

  if ( !chrlst[platform].bmpdata.calc_is_platform )
    return bfalse;

  chrlst[object_ref].onwhichplatform  = platform;
  if ( chrlst[object_ref].weight > 0.0f )
  {
    chrlst[platform].holdingweight += chrlst[object_ref].weight;
  }

  chrlst[object_ref].jumpready  = btrue;
  chrlst[object_ref].jumpnumber = chrlst[object_ref].jumpnumberreset;

  chrlst[object_ref].level = chrlst[platform].bmpdata.cv.z_max;

  if ( chr_is_player(gs, object_ref) )
  {
    debug_message( 1, "attached %s(%s) to platform", chrlst[object_ref].name, ChrList_getPCap(gs, object_ref)->classname );
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
void fill_bumplists(Game_t * gs)
{
  Mesh_t     * pmesh = Game_getMesh(gs);
  MeshInfo_t * mi    = &(pmesh->Info);
  BUMPLIST   * pbump = &(mi->bumplist);

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  CHR_REF chr_ref;
  PRT_REF prt_ref;
  Uint32  fanblock;
  Uint8   hidestate;

  // Clear the lists
  reset_bumplist(mi);

  // Fill 'em back up
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    Chr_t * pchr;
    Cap_t * pcap;
    int ix, ix_min, ix_max;
    int iy, iy_min, iy_max;

    // ignore invalid characters and objects that are packed away
    if ( !ACTIVE_CHR( chrlst, chr_ref ) || chr_in_pack( chrlst, chrlst_size, chr_ref ) || chrlst[chr_ref].gopoof ) continue;
    pchr = chrlst + chr_ref;
    pcap = ChrList_getPCap( gs, chr_ref );

    // do not include hidden objects
    hidestate = NOHIDE;
    if(NULL != pcap) hidestate = pcap->hidestate;
    if ( hidestate != NOHIDE && hidestate == pchr->aistate.state ) continue;

    ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( mi, pchr->bmpdata.cv.x_min ) );
    ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( mi, pchr->bmpdata.cv.x_max ) );
    iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( mi, pchr->bmpdata.cv.y_min ) );
    iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( mi, pchr->bmpdata.cv.y_max ) );

    for ( ix = ix_min; ix <= ix_max; ix++ )
    {
      for ( iy = iy_min; iy <= iy_max; iy++ )
      {
        fanblock = mesh_convert_block( mi, ix, iy );
        if ( INVALID_FAN == fanblock ) continue;

        // Insert before any other characters on the block
        bumplist_insert_chr(pbump, fanblock, chr_ref);
      }
    }
  };

  for ( prt_ref = 0; prt_ref < prtlst_size; prt_ref++ )
  {
    // ignore invalid particles
    if ( !ACTIVE_PRT( prtlst, prt_ref ) || prtlst[prt_ref].gopoof ) continue;

    fanblock = mesh_get_block( mi, prtlst[prt_ref].ori.pos );
    if ( INVALID_FAN == fanblock ) continue;

    // Insert before any other particles on the block
    bumplist_insert_prt(pbump, fanblock, prt_ref);
  }

  pbump->filled = btrue;
};

//--------------------------------------------------------------------------------------------
bool_t find_collision_volume( vect3 * ppa, CVolume_t * pva, vect3 * ppb, CVolume_t * pvb, bool_t exclude_vert, CVolume_t * pcv)
{
  bool_t retval = bfalse, bfound;
  CVolume_t cv, tmp_cv;
  float ftmp;

  if( NULL == ppa || NULL == pva || NULL == ppb || NULL == pvb ) return bfalse;

  //---- do the preliminary collision test ----

  // do diagonal
  cv.xy_min = MAX(pva->xy_min + ppa->x + ppa->y, pvb->xy_min + ppb->x + ppb->y);
  cv.xy_max = MIN(pva->xy_max + ppa->x + ppa->y, pvb->xy_max + ppb->x + ppb->y);
  if(cv.xy_min >= cv.xy_max) return bfalse;

  cv.yx_min = MAX(pva->yx_min - ppa->x + ppa->y, pvb->yx_min - ppb->x + ppb->y);
  cv.yx_max = MIN(pva->yx_max - ppa->x + ppa->y, pvb->yx_max - ppb->x + ppb->y);
  if(cv.yx_min >= cv.yx_max) return bfalse;

  // do square
  cv.x_min = MAX(pva->x_min + ppa->x, pvb->x_min + ppb->x);
  cv.x_max = MIN(pva->x_max + ppa->x, pvb->x_max + ppb->x);
  if(cv.x_min >= cv.x_max) return bfalse;

  cv.y_min = MAX(pva->y_min + ppa->y, pvb->y_min + ppb->y);
  cv.y_max = MIN(pva->y_max + ppa->y, pvb->y_max + ppb->y);
  if(cv.y_min >= cv.y_max) return bfalse;

  // do vert
  cv.z_min = MAX(pva->z_min + ppa->z, pvb->z_min + ppb->z);
  cv.z_max = MIN(pva->z_max + ppa->z, pvb->z_max + ppb->z);
  if(!exclude_vert && cv.z_min >= cv.z_max) return bfalse;

  //---- limit the collision volume ----

  tmp_cv = cv;

  //=================================================================
  // treat the edges of the square bbox as line segments and clip them using the
  // diagonal bbox
  //=================================================================
  // do the y segments
  bfound = bfalse;
  {
    // do the y = x_min segment

    float y_min, y_max;
    bool_t bexcluded = bfalse;

    bexcluded = bfalse;
    y_min = cv.y_min;
    y_max = cv.y_max;

    if(!bexcluded)
    {
      ftmp = -cv.x_min + cv.xy_min;
      y_min = MAX(y_min, ftmp);

      ftmp = -cv.x_min + cv.xy_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    }

    if(!bexcluded)
    {
      ftmp = cv.yx_min + cv.x_min;
      y_min = MAX(y_min, ftmp);

      ftmp = cv.x_min + cv.yx_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    };

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {
      if(!bfound)
      {
        tmp_cv.y_min = y_min;
        tmp_cv.y_max = y_max;
      }
      else
      {
        tmp_cv.y_min = MIN(tmp_cv.y_min, y_min);
        tmp_cv.y_max = MAX(tmp_cv.y_max, y_max);
      }
      assert(tmp_cv.y_min <= tmp_cv.y_max);
      bfound = btrue;
    }
  }

  //=================================================================
  {
    // do the y = x_max segment

    float y_min, y_max;
    bool_t bexcluded = bfalse;

    //---------------------

    bexcluded = bfalse;
    y_min = cv.y_min;
    y_max = cv.y_max;

    if(!bexcluded)
    {
      ftmp = -cv.x_max + cv.xy_min;
      y_min = MAX(y_min, ftmp);

      ftmp = -cv.x_max + cv.xy_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    }

    if(!bexcluded)
    {
      ftmp = cv.yx_min + cv.x_max;
      y_min = MAX(y_min, ftmp);

      ftmp = cv.x_max + cv.yx_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    };

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {

      if(!bfound)
      {
        tmp_cv.y_min = y_min;
        tmp_cv.y_max = y_max;
      }
      else
      {
        tmp_cv.y_min = MIN(tmp_cv.y_min, y_min);
        tmp_cv.y_max = MAX(tmp_cv.y_max, y_max);
      };
      assert(tmp_cv.y_min <= tmp_cv.y_max);

      bfound = btrue;
    }
  }

  //=================================================================
  // do the x segments
  bfound = bfalse;
  {
    // do the x = y_min segment

    float x_min, x_max;
    bool_t bexcluded = bfalse;

    bexcluded = bfalse;
    x_min = cv.x_min;
    x_max = cv.x_max;

    if(!bexcluded)
    {
      ftmp = cv.xy_min - cv.y_min;
      x_min = MAX(x_min, ftmp);

      ftmp = -cv.yx_min + cv.y_min;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }

    if(!bexcluded)
    {
      ftmp = -cv.yx_max + cv.y_min;
      x_min = MAX(x_min, ftmp);

      ftmp = cv.xy_max - cv.y_min;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {
      if(!bfound)
      {
        tmp_cv.x_min = x_min;
        tmp_cv.x_max = x_max;
      }
      else
      {
        tmp_cv.x_min = MIN(tmp_cv.x_min, x_min);
        tmp_cv.x_max = MAX(tmp_cv.x_max, x_max);
      }
      assert(tmp_cv.x_min <= tmp_cv.x_max);
      bfound = btrue;
    }

  }

  //=================================================================
  {
    // do the x = y_max segment

    float x_min, x_max;
    bool_t bexcluded = bfalse;

    bexcluded = bfalse;
    x_min = cv.x_min;
    x_max = cv.x_max;

    if(!bexcluded)
    {
      ftmp = cv.xy_min - cv.y_max;
      x_min = MAX(x_min, ftmp);

      ftmp = -cv.yx_min + cv.y_max;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }

    if(!bexcluded)
    {
      ftmp = -cv.yx_max + cv.y_max;
      x_min = MAX(x_min, ftmp);

      ftmp = cv.xy_max - cv.y_max;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {
      if(!bfound)
      {
        tmp_cv.x_min = x_min;
        tmp_cv.x_max = x_max;
      }
      else
      {
        tmp_cv.x_min = MIN(tmp_cv.x_min, x_min);
        tmp_cv.x_max = MAX(tmp_cv.x_max, x_max);
      };
      assert(tmp_cv.x_min <= tmp_cv.x_max);
      bfound = btrue;
    }
  }

  if(NULL != pcv)
  {
    *pcv = tmp_cv;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_inside( Game_t * gs, CHR_REF chra_ref, float lerp, CHR_REF chrb_ref, bool_t exclude_vert )
{
  // BB > Find whether an active point of chra_ref is "inside" chrb_ref's bounding volume.
  //      Abstraction of the old algorithm to see whether a character cpold be "above" another

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  float ftmp;
  BData_t * ba, * bb;

  if( !ACTIVE_CHR(chrlst, chra_ref) || !ACTIVE_CHR(chrlst, chrb_ref) ) return bfalse;

  ba = &(chrlst[chra_ref].bmpdata);
  bb = &(chrlst[chrb_ref].bmpdata);

  //---- vertical ----
  if( !exclude_vert )
  {
    ftmp = ba->mids_lo.z + (ba->mids_hi.z - ba->mids_lo.z) * lerp;
    if( ftmp < bb->cv.z_min || ftmp > bb->cv.z_max ) return bfalse;
  }

  //---- diamond ----
  if( bb->cv.lod > 1 )
  {
    ftmp = ba->mids_lo.x + ba->mids_lo.y;
    if( ftmp < bb->cv.xy_min || ftmp > bb->cv.xy_max  ) return bfalse;

    ftmp = -ba->mids_lo.x + ba->mids_lo.y;
    if( ftmp < bb->cv.yx_min || ftmp > bb->cv.yx_max ) return bfalse;
  };

  //---- square ----
  ftmp = ba->mids_lo.x;
  if( ftmp < bb->cv.x_min || ftmp > bb->cv.x_max ) return bfalse;

  ftmp = ba->mids_lo.y;
  if( ftmp < bb->cv.y_min || ftmp > bb->cv.y_max ) return bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_detect_collision( Game_t * gs, CHR_REF chra_ref, CHR_REF chrb_ref, bool_t exclude_height, CVolume_t * cv)
{
  // BB > use the bounding boxes to estimate whether a collision has occurred.

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  bool_t retval = bfalse;

  // set the minimum bumper level for object a
  if(chrlst[chra_ref].bmpdata.cv.lod < 0)
  {
    chr_calculate_bumpers( gs, chrlst + chra_ref, 0);
  }

  // set the minimum bumper level for object b
  if(chrlst[chrb_ref].bmpdata.cv.lod < 0)
  {
    chr_calculate_bumpers( gs, chrlst + chrb_ref, 0);
  }

  // find the simplest collision volume
  return find_collision_volume( &(chrlst[chra_ref].ori.pos), &(chrlst[chra_ref].bmpdata.cv), &(chrlst[chrb_ref].ori.pos), &(chrlst[chrb_ref].bmpdata.cv), exclude_height, cv);
}



//--------------------------------------------------------------------------------------------
bool_t chr_do_collision( Game_t * gs, CHR_REF chra_ref, CHR_REF chrb_ref, bool_t exclude_height, CVolume_t * cv)
{
  // BB > use the bounding boxes to determine whether a collision has occurred.
  //      there are currently 3 levels of collision detection.
  //      level 1 - the basic square axis-aligned bounding box
  //      level 2 - the octagon bounding box calculated from the actual vertex positions
  //      level 3 - an "octree" of bounding bounding boxes calculated from the actual triangle positions
  //  the level is chosen by the global variable chrcollisionlevel

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  Chr_t * pchra, * pchrb;

  bool_t retval = bfalse;

  pchra = ChrList_getPChr(gs, chra_ref);
  if(NULL == pchra) return bfalse;

  pchrb = ChrList_getPChr(gs, chrb_ref);
  if(NULL == pchrb) return bfalse;

  // set the minimum bumper level for object a
  if(pchra->bmpdata.cv.lod < 1)
  {
    chr_calculate_bumpers( gs, chrlst + chra_ref, 1);
  }

  // set the minimum bumper level for object b
  if(pchrb->bmpdata.cv.lod < 1)
  {
    chr_calculate_bumpers( gs, chrlst + chrb_ref, 1);
  }

  // find the simplest collision volume
  retval = find_collision_volume( &(pchra->ori.pos), &(pchra->bmpdata.cv), &(pchrb->ori.pos), &(pchrb->bmpdata.cv), exclude_height, cv);

  if ( chrcollisionlevel>1 && retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(pchra->bmpdata.cv.lod < 2)
    {
      chr_calculate_bumpers( gs, chrlst + chra_ref, 2);
      was_refined = btrue;
    }

    // refine the bumper
    if(pchrb->bmpdata.cv.lod < 2)
    {
      chr_calculate_bumpers( gs, chrlst + chrb_ref, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(pchra->ori.pos), &(pchra->bmpdata.cv), &(pchrb->ori.pos), &(pchrb->bmpdata.cv), exclude_height, cv);
    };

    if(chrcollisionlevel>2 && retval)
    {
      was_refined = bfalse;

      // refine the bumper
      if(pchra->bmpdata.cv.lod < 3)
      {
        chr_calculate_bumpers( gs, chrlst + chra_ref, 3);
        was_refined = btrue;
      }

      // refine the bumper
      if(pchrb->bmpdata.cv.lod < 3)
      {
        chr_calculate_bumpers( gs, chrlst + chrb_ref, 3);
        was_refined = btrue;
      }

      assert(NULL != pchra->bmpdata.cv_tree);
      assert(NULL != pchrb->bmpdata.cv_tree);

      if(was_refined)
      {
        int cnt, tnc;
        CVolume_t cv3, tmp_cv;
        bool_t loc_retval;

        retval = bfalse;
        cv3.lod = -1;
        for(cnt=0; cnt<8; cnt++)
        {
          if(-1 == pchra->bmpdata.cv_tree->leaf[cnt].lod) continue;

          for(tnc=0; tnc<8; tnc++)
          {
            if(-1 == pchrb->bmpdata.cv_tree->leaf[cnt].lod) continue;

            loc_retval = find_collision_volume( &(pchra->ori.pos), pchra->bmpdata.cv_tree->leaf + cnt, &(pchrb->ori.pos), pchrb->bmpdata.cv_tree->leaf + cnt, exclude_height, &tmp_cv);

            if(loc_retval)
            {
              retval = btrue;
              cv3 = CVolume_merge(&cv3, &tmp_cv);

#if defined(DEBUG_CVOLUME) && defined(_DEBUG)
              if(CData.DevMode)
              {
                cv_list_add( &tmp_cv );
              }
#endif
            }
          };
        };

        if(retval)
        {
          *cv = cv3;
        };
      };
    };
  }

#if defined(DEBUG_CVOLUME) && defined(_DEBUG)
  if(CData.DevMode && retval)
  {
    cv_list_add( cv );
  }
#endif

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_detect_collision( Game_t * gs, CHR_REF chra_ref, PRT_REF prtb, bool_t exclude_height )
{
  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PPrt_t prtlst = gs->PrtList;

  return find_collision_volume( &(chrlst[chra_ref].ori.pos), &(chrlst[chra_ref].bmpdata.cv), &(prtlst[prtb].ori.pos), &(prtlst[prtb].bmpdata.cv), bfalse, NULL );
}

//--------------------------------------------------------------------------------------------
bool_t prt_do_collision( Game_t * gs, CHR_REF chra_ref, PRT_REF prtb_ref, bool_t exclude_height )
{
  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PPrt_t prtlst = gs->PrtList;

  bool_t retval = bfalse;

  Chr_t * pchra;
  Prt_t * pprtb;

  pchra = ChrList_getPChr(gs, chra_ref);
  if(NULL == pchra) return bfalse;

  pprtb = PrtList_getPPrt(gs, prtb_ref);
  if(NULL == pprtb) return bfalse;

  retval = find_collision_volume( &(pchra->ori.pos), &(pchra->bmpdata.cv), &(pprtb->ori.pos), &(pprtb->bmpdata.cv), bfalse, NULL );

  if ( retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(pchra->bmpdata.cv.lod < 2)
    {
      chr_calculate_bumpers( gs, chrlst + chra_ref, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(pchra->ori.pos), &(pchra->bmpdata.cv), &(pprtb->ori.pos), &(pprtb->bmpdata.cv), bfalse, NULL );
    };
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void do_bumping( Game_t * gs, float dUpdate )
{
  // ZZ> This function sets handles characters hitting other characters or particles

  Uint32 fanblock;
  int cnt, tnc, chrinblock, prtinblock;
  vect3 apos, bpos;
  CoData_t   * d;

  // collision data
  int    cdata_count = 0;
  CoData_t cdata[CHR_MAX_COLLISIONS];

  // collision data hash nodes
  int    hn_count = 0;
  HashNode_t hnlst[CHR_MAX_COLLISIONS];

  Uint32  blnode_a, blnode_b;
  CHR_REF ichra, ichrb;
  Chr_t *  pchra, * pchrb;
  PRT_REF iprtb;
  Prt_t *  pprtb;

  float loc_platkeep, loc_platascend, loc_platstick;

  Mesh_t * pmesh     = Game_getMesh(gs); 

  MeshInfo_t * mi     = &(pmesh->Info);
  BUMPLIST   * pbump  = &(mi->bumplist);

  PChr_t chrlst        = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PPrt_t prtlst        = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  PPip_t piplst      = gs->PipList;
  size_t piplst_size = PIPLST_COUNT;

  PEnc enclst        = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;


  loc_platascend = pow( PLATASCEND, 1.0 / dUpdate );
  loc_platkeep   = 1.0 - loc_platascend;
  loc_platstick  = pow( gs->phys.platstick, 1.0 / dUpdate );

  // create a collision hash table that can keep track of 512
  // binary collisions per frame
  if(NULL == CoList)
  {
    CoList = HashList_create(-1);
    assert(NULL != CoList);
  }

  fill_bumplists(gs);
  cv_list_clear();

  // renew the CoList. Since we are filling this list with pre-allocated HashNode_t's,
  // there is no need to delete any of the existing CoList->sublist elements
  for(cnt=0; cnt<256; cnt++)
  {
    CoList->subcount[cnt] = 0;
    CoList->sublist[cnt]  = NULL;
  }

  // remove bad platforms
  for ( ichra = 0; ichra < CHRLST_COUNT; ichra++ )
  {
    pchra = ChrList_getPChr(gs, ichra);
    if(NULL == pchra) continue;

    // detach character from invalid platforms
    ichrb  = chr_get_onwhichplatform( chrlst, chrlst_size, ichra );
    pchrb = ChrList_getPChr(gs, ichrb);
    if ( NULL != pchrb )
    {
      if ( !chr_is_inside( gs, ichra, 0.0f, ichrb, btrue ) ||
            pchrb->bmpdata.cv.z_min  > pchrb->bmpdata.cv.z_max - PLATTOLERANCE )
      {
        remove_from_platform( gs, ichra );
      }
    }
  };

  // Check collisions with other characters and bump particles
  // Only check each pair once
  for ( fanblock = 0; fanblock < pbump->num_blocks; fanblock++ )
  {
    chrinblock = bumplist_get_chr_count(pbump, fanblock);
    prtinblock = bumplist_get_prt_count(pbump, fanblock);

    // detect and save character-character collisions
    for ( cnt = 0, blnode_a = bumplist_get_chr_head(pbump, fanblock);
      cnt < chrinblock && INVALID_BUMPLIST_NODE != blnode_a;
      cnt++, blnode_a = bumplist_get_next_chr( gs, pbump, blnode_a ) )
    {
      ichra = bumplist_get_ref(pbump, blnode_a);
      pchra = ChrList_getPChr(gs, ichra);
      if(NULL == pchra) continue;

      // don't do object-object collisions if they won't feel each other
      if ( pchra->bumpstrength == 0.0f ) continue;

      // Do collisions (but not with attached items/characers)
      for ( blnode_b = bumplist_get_next_chr(gs, pbump, blnode_a), tnc = cnt + 1;
        tnc < chrinblock && INVALID_BUMPLIST_NODE != blnode_b;
        tnc++, blnode_b = bumplist_get_next_chr( gs, pbump, blnode_b ) )
      {
        float bumpstrength;

        ichrb = bumplist_get_ref(pbump, blnode_b);
        pchrb = ChrList_getPChr(gs, ichrb);
        if(NULL == pchrb) continue;

        bumpstrength = pchra->bumpstrength * pchrb->bumpstrength;

        // don't do object-object collisions if they won't feel eachother
        if ( bumpstrength == 0.0f ) continue;

        // do not collide with something you are already holding
        if ( ichrb == pchra->attachedto || ichra == pchrb->attachedto ) continue;

        // do not collide with a your platform
        if ( ichrb == pchra->onwhichplatform || ichra == pchrb->onwhichplatform ) continue;

        if ( chr_detect_collision( gs, ichra, ichrb, bfalse, NULL) )
        {
          HashNode_t * n;
          bool_t found;
          int    count;
          Uint32 hashval = 0;

          // create a hash that is order-independent
          hashval = (REF_TO_INT(ichra)* 0x0111 + 0x006E) + (REF_TO_INT(ichrb)* 0x0111 + 0x006E);
          hashval &= 0xFF;

          found = bfalse;
          count = CoList->subcount[hashval];
          if( count > 0)
          {
            int i ;

            // this hash already exists. check to see if the binary collision exists, too
            n = CoList->sublist[hashval];
            for(i = 0; i<count; i++)
            {
              d = (CoData_t *)(n->data);
              if(d->chra == ichra && d->chrb == ichrb)
              {
                found = btrue;
                break;
              }
            }
          }


          // insert this collision
          if(!found)
          {
            // pick a free collision data
            d = cdata + cdata_count;
            cdata_count++;

            // fill it in
            d->chra = ichra;
            d->chrb = ichrb;
            d->prtb = INVALID_PRT;

            // generate a new hash node
            n = hnlst + hn_count;
            hn_count++;
            HashNode_new(n, (void*)d);

            // insert the node
            CoList->subcount[hashval]++;
            CoList->sublist[hashval] = HashNode_insert_before(CoList->sublist[hashval], n);
          }
        }
      }
    }

    // detect and save character-particle collisions
    for ( cnt = 0, blnode_a = bumplist_get_chr_head( pbump, fanblock );
      cnt < chrinblock && INVALID_BUMPLIST_NODE != blnode_a;
      cnt++, blnode_a = bumplist_get_next_chr( gs, pbump, blnode_a ) )
    {
      IDSZ chridvulnerability;
      float chrbump = 1.0f;
      PRT_REF iprtb;

      ichra = bumplist_get_ref( pbump, blnode_a );
      pchra = ChrList_getPChr(gs, ichra);
      if(NULL == pchra) continue;

      chridvulnerability = ChrList_getPCap(gs, ichra)->idsz[IDSZ_VULNERABILITY];
      chrbump = pchra->bumpstrength;

      // Check for object-particle interaction
      for ( tnc = 0, blnode_b = bumplist_get_prt_head(pbump, fanblock);
        tnc < prtinblock && INVALID_BUMPLIST_NODE != blnode_b;
        tnc++ , blnode_b = bumplist_get_next_prt( gs, pbump, blnode_b ) )
      {
        float bumpstrength, prtbump;
        bool_t chr_is_vulnerable;

        iprtb = bumplist_get_ref( pbump, blnode_b);
        pprtb = PrtList_getPPrt(gs, iprtb);
        if(NULL == pprtb) continue;

        chr_is_vulnerable = !pchra->prop.invictus && ( IDSZ_NONE != chridvulnerability ) && CAP_INHERIT_IDSZ( gs,  prtlst[iprtb].model, chridvulnerability );

        prtbump = prtlst[iprtb].bumpstrength;
        bumpstrength = chr_is_vulnerable ? 1.0f : chrbump * prtbump;

        if ( 0.0f == bumpstrength ) continue;

        if ( prt_detect_collision( gs, ichra, iprtb, bfalse ) )
        {
          HashNode_t * n;
          bool_t found;
          int    count;
          Uint32 hashval = 0;

          // create a hash that is order-independent
          hashval = (REF_TO_INT(ichra)* 0x0111 + 0x006E) + (REF_TO_INT(iprtb)* 0x0111 + 0x006E);
          hashval &= 0xFF;

          found = bfalse;
          count = CoList->subcount[hashval];
          if( count > 0)
          {
            int i ;

            // this hash already exists. check to see if the binary collision exists, too
            n = CoList->sublist[hashval];
            for(i = 0; i<count; i++)
            {
              d = (CoData_t *)(n->data);
              if(d->chra == ichra && d->prtb == iprtb)
              {
                found = btrue;
                break;
              }
            }
          }


          // insert this collision
          if(!found)
          {
            // pick a free collision data
            d = cdata + cdata_count;
            cdata_count++;

            // fill it in
            d->chra = ichra;
            d->chrb = INVALID_CHR;
            d->prtb = iprtb;

            // generate a new hash node
            n = hnlst + hn_count;
            hn_count++;
            HashNode_new(n, (void*)d);

            // insert the node
            CoList->subcount[hashval]++;
            CoList->sublist[hashval] = HashNode_insert_before(CoList->sublist[hashval], n);
          }
        }
      }
    }
  }

  // Do platforms
  for ( cnt = 0; cnt < CoList->allocated; cnt++ )
  {
    HashNode_t * n;
    int count = CoList->subcount[cnt];

    n = CoList->sublist[cnt];
    for( tnc = 0; tnc<count && NULL != n; tnc++, n = n->next )
    {
      d = (CoData_t *)(n->data);
      if(INVALID_PRT != d->prtb) continue;

      ichra = d->chra;
      pchra = ChrList_getPChr(gs, ichra);
      if(NULL == pchra) continue;

      // Do platforms (no interaction with held or mounted items)
      if ( chr_attached( chrlst, chrlst_size, ichra ) ) continue;

      ichrb = d->chrb;
      pchrb = ChrList_getPChr(gs, ichrb);
      if(NULL == pchrb) continue;

      // do not put something on a platform that is being carried by someone
      if ( chr_attached( chrlst, chrlst_size, ichrb ) ) continue;

      // do not consider anything that is already a item/platform combo
      if ( pchra->onwhichplatform == ichrb || pchrb->onwhichplatform == ichra ) continue;

      // make sure the platform combo is valid
      if( !(pchra->bmpdata.calc_is_platform && pchrb->prop.canuseplatforms) &&
          !(pchrb->bmpdata.calc_is_platform && pchra->prop.canuseplatforms)) continue;

      if ( chr_is_inside( gs, ichra, 0.0f, ichrb, btrue) )
      {
        // check for compatibility
        if ( pchrb->bmpdata.calc_is_platform )
        {
          // check for overlap in the z direction
          if ( pchra->ori.pos.z > MAX( pchrb->bmpdata.cv.z_min, pchrb->bmpdata.cv.z_max - PLATTOLERANCE ) && pchra->level < pchrb->bmpdata.cv.z_max )
          {
            // A is inside, coming from above
            attach_to_platform( gs, ichra, ichrb );
          }
        }
      }

      if( chr_is_inside( gs, ichrb, 0.0f, ichra, btrue) )
      {
        if ( pchra->bmpdata.calc_is_platform )
        {
          // check for overlap in the z direction
          if ( pchrb->ori.pos.z > MAX( pchra->bmpdata.cv.z_min, pchra->bmpdata.cv.z_max - PLATTOLERANCE ) && pchrb->level < pchra->bmpdata.cv.z_max )
          {
            // A is inside, coming from above
            attach_to_platform( gs, ichrb, ichra );
          }
        }
      }

    }
  }


  // Do mounting
  for ( cnt = 0; cnt < CoList->allocated; cnt++ )
  {
    HashNode_t * n;
    int count = CoList->subcount[cnt];

    n = CoList->sublist[cnt];
    for( tnc = 0; tnc<count && NULL != n; tnc++, n = n->next )
    {
      d = (CoData_t *)(n->data);
      if(INVALID_PRT != d->prtb) continue;

      ichra = d->chra;
      pchra = ChrList_getPChr(gs, ichra);
      if(NULL == pchra) continue;

      if ( chr_attached( chrlst, chrlst_size, ichra ) ) continue;

      ichrb = d->chrb;
      pchrb = ChrList_getPChr(gs, ichrb);
      if(NULL == pchrb) continue;

      // do not mount something that is being carried by someone
      if ( chr_attached( chrlst, chrlst_size, ichrb ) ) continue;

      if ( chr_is_inside( gs, ichra, 0.0f, ichrb, btrue)   )
      {

        // Now see if either is on top the other like a platform
        if ( pchra->ori.pos.z > pchrb->bmpdata.cv.z_max - PLATTOLERANCE && pchra->ori.pos.z < pchrb->bmpdata.cv.z_max + PLATTOLERANCE / 5 )
        {
          // Is A falling on B?
          if ( pchra->ori.vel.z < pchrb->ori.vel.z )
          {
            if ( pchra->flyheight == 0 && pchra->alive && ChrList_getPMad(gs, ichra)->actionvalid[ACTION_MI] && !pchra->prop.isitem )
            {
              if ( pchrb->alive && pchrb->prop.ismount && !chr_using_slot( chrlst, chrlst_size, ichrb, SLOT_SADDLE ) )
              {
                remove_from_platform( gs, ichra );
                if ( !attach_character_to_mount( gs, ichra, ichrb, SLOT_SADDLE ) )
                {
                  // failed mount is a bump
                  pchra->aistate.alert |= ALERT_BUMPED;
                  pchrb->aistate.alert |= ALERT_BUMPED;
                  pchra->aistate.bumplast = ichrb;
                  pchrb->aistate.bumplast = ichra;
                };
              }
            }
          }
        }

      }

      if( chr_is_inside( gs, ichrb, 0.0f, ichra, btrue)   )
      {
        if ( pchrb->ori.pos.z > pchra->bmpdata.cv.z_max - PLATTOLERANCE && pchrb->ori.pos.z < pchra->bmpdata.cv.z_max + PLATTOLERANCE / 5 )
        {
          // Is B falling on A?
          if ( pchrb->ori.vel.z < pchra->ori.vel.z )
          {
            if ( pchrb->flyheight == 0 && pchrb->alive && ChrList_getPMad(gs, ichrb)->actionvalid[ACTION_MI] && !pchrb->prop.isitem )
            {
              if ( pchra->alive && pchra->prop.ismount && !chr_using_slot( chrlst, chrlst_size, ichra, SLOT_SADDLE ) )
              {
                remove_from_platform( gs, ichrb );
                if ( !attach_character_to_mount( gs, ichrb, ichra, SLOT_SADDLE ) )
                {
                  // failed mount is a bump
                  pchra->aistate.alert |= ALERT_BUMPED;
                  pchrb->aistate.alert |= ALERT_BUMPED;
                  pchra->aistate.bumplast = ichrb;
                  pchrb->aistate.bumplast = ichra;
                };
              };
            }
          }
        }
      }
    }
  }

  // Do platform physics
  for ( ichra = 0; ichra < CHRLST_COUNT; ichra++ )
  {
    if( !ACTIVE_CHR(chrlst, ichra) ) continue;
    pchra = chrlst + ichra;

    ichrb = chr_get_onwhichplatform(chrlst, chrlst_size, ichra);
    if( !ACTIVE_CHR(chrlst, ichrb) ) continue;
    pchrb = chrlst + ichrb;

    if ( pchra->ori.pos.z < pchrb->bmpdata.cv.z_max + RAISE )
    {
      pchra->ori.pos.z = pchrb->bmpdata.cv.z_max + RAISE;
      if ( pchra->ori.vel.z < pchrb->ori.vel.z )
      {
        pchra->ori.vel.z = - ( pchra->ori.vel.z - pchrb->ori.vel.z ) * pchra->bumpdampen * pchrb->bumpdampen + pchrb->ori.vel.z;
      };
    }

    pchra->ori.vel.x    = ( pchra->ori.vel.x   - pchrb->ori.vel.x ) * ( 1.0 - loc_platstick ) + pchrb->ori.vel.x;
    pchra->ori.vel.y    = ( pchra->ori.vel.y   - pchrb->ori.vel.y ) * ( 1.0 - loc_platstick ) + pchrb->ori.vel.y;
    pchra->ori.turn_lr += (( pchra->ori.turn_lr - pchra->ori_old.turn_lr) - (pchrb->ori.turn_lr - pchrb->ori_old.turn_lr )) * ( 1.0 - loc_platstick ) + (pchrb->ori.turn_lr - pchrb->ori_old.turn_lr );
  }

  // Do collisions
  chr_collisions = 0;
  if(cdata_count > 0)
  {
    vect3 diff, acc, nrm;
    Uint16 direction;
    float dist, scale;
    ENC_REF enchant;
    PIP_REF pip;

    //process the saved interactions
    for ( cnt = 0; cnt < CoList->allocated; cnt++ )
    {
      HashNode_t * n;
      int count = CoList->subcount[cnt];

      chr_collisions += count;
      n = CoList->sublist[cnt];
      for( tnc = 0; tnc<count && NULL != n; tnc++, n = n->next )
      {
        float lerpa;

        d = (CoData_t *)(n->data);

        ichra = d->chra;
        pchra = ChrList_getPChr(gs, ichra);
        if(NULL == pchra) continue;

        apos = pchra->ori.pos;

        lerpa = (pchra->ori.pos.z - pchra->level) / PLATTOLERANCE;
        lerpa = CLIP(lerpa, 0.0f, 1.0f);

        if(INVALID_PRT == d->prtb)
        {
          CVolume_t cv;
          float lerpb, bumpstrength;

          // do the character-character interactions
          ichrb = d->chrb;
          pchrb = ChrList_getPChr(gs, ichrb);
          if(NULL == pchrb) continue;

          bumpstrength = pchra->bumpstrength * pchrb->bumpstrength;

          // don't do object-object collisions if they won't feel eachother
          if ( bumpstrength == 0.0f ) continue;

          // do not collide with something you are already holding
          if ( ichrb == pchra->attachedto || ichra == pchrb->attachedto ) continue;

          // do not collide with a your platform
          if ( ichrb == pchra->onwhichplatform || ichra == pchrb->onwhichplatform ) continue;

          bpos = pchrb->ori.pos;

          lerpb = (pchrb->ori.pos.z - pchrb->level) / PLATTOLERANCE;
          lerpb = CLIP(lerpb, 0, 1);

          if ( chr_do_collision( gs, ichra, ichrb, bfalse, &cv) )
          {
            vect3 depth, ovlap, nrm, diffa, diffb;
            float dotprod, pressure;
            float cr, m0, m1, psum, msum, udif, u0, u1, ln_cr;
            bool_t bfound;

            depth.x = (cv.x_max - cv.x_min);
            ovlap.x = depth.x / MIN(pchra->bmpdata.cv.x_max - pchra->bmpdata.cv.x_min, pchrb->bmpdata.cv.x_max - pchrb->bmpdata.cv.x_min);
            ovlap.x = CLIP(ovlap.x,-1,1);
            nrm.x = 1.0f;
            if(ovlap.x > 0.0f) nrm.x /= ovlap.x;

            depth.y = (cv.y_max - cv.y_min);
            ovlap.y = depth.y / MIN(pchra->bmpdata.cv.y_max - pchra->bmpdata.cv.y_min, pchrb->bmpdata.cv.y_max - pchrb->bmpdata.cv.y_min);
            ovlap.y = CLIP(ovlap.y,-1,1);
            nrm.y = 1.0f;
            if(ovlap.y > 0.0f) nrm.y /= ovlap.y;

            depth.z = (cv.z_max - cv.z_min);
            ovlap.z = depth.z / MIN(pchra->bmpdata.cv.z_max - pchra->bmpdata.cv.z_min, pchrb->bmpdata.cv.z_max - pchrb->bmpdata.cv.z_min);
            ovlap.z = CLIP(ovlap.z,-1,1);
            nrm.z = 1.0f;
            if(ovlap.z > 0.0f) nrm.z /= ovlap.z;

            nrm = Normalize(nrm);

            pressure = (depth.x / 30.0f) * (depth.y / 30.0f) * (depth.z / 30.0f);

            if(ovlap.x != 1.0)
            {
              diffa.x = pchra->bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
              diffb.x = pchrb->bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
            }
            else
            {
              diffa.x = pchra->bmpdata.mids_lo.x - pchrb->bmpdata.mids_lo.x;
              diffb.x =-diffa.x;
            }

            if(ovlap.y != 1.0)
            {
              diffa.y = pchra->bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
              diffb.y = pchrb->bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
            }
            else
            {
              diffa.y = pchra->bmpdata.mids_lo.y - pchrb->bmpdata.mids_lo.y;
              diffb.y =-diffa.y;
            }

            if(ovlap.y != 1.0)
            {
              diffa.z = pchra->bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
              diffa.z += (pchra->bmpdata.mids_hi.z - pchra->bmpdata.mids_lo.z) * lerpa;

              diffb.z = pchrb->bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
              diffb.z += (pchrb->bmpdata.mids_hi.z - pchrb->bmpdata.mids_lo.z) * lerpb;
            }
            else
            {
              diffa.z  = pchra->bmpdata.mids_lo.z - pchrb->bmpdata.mids_lo.z;
              diffa.z += (pchra->bmpdata.mids_hi.z - pchra->bmpdata.mids_lo.z) * lerpa;
              diffa.z -= (pchrb->bmpdata.mids_hi.z - pchrb->bmpdata.mids_lo.z) * lerpb;

              diffb.z =-diffa.z;
            }

            diffa = Normalize(diffa);
            diffb = Normalize(diffb);

            if(diffa.x < 0) nrm.x *= -1.0f;
            if(diffa.y < 0) nrm.y *= -1.0f;
            if(diffa.z < 0) nrm.z *= -1.0f;

            dotprod = DotProduct(diffa, nrm);
            if(dotprod != 0.0f)
            {
              diffa.x = pressure * dotprod * nrm.x;
              diffa.y = pressure * dotprod * nrm.y;
              diffa.z = pressure * dotprod * nrm.z;
            }
            else
            {
              diffa.x = pressure * nrm.x;
              diffa.y = pressure * nrm.y;
              diffa.z = pressure * nrm.z;
            };

            dotprod = DotProduct(diffb, nrm);
            if(dotprod != 0.0f)
            {
              diffb.x = pressure * dotprod * nrm.x;
              diffb.y = pressure * dotprod * nrm.y;
              diffb.z = pressure * dotprod * nrm.z;
            }
            else
            {
              diffb.x = - pressure * nrm.x;
              diffb.y = - pressure * nrm.y;
              diffb.z = - pressure * nrm.z;
            };

            // calculate a coefficient of restitution
            //ftmp = nrm.x * nrm.x + nrm.y * nrm.y;
            //cr = pchrb->bumpdampen * pchra->bumpdampen * bumpstrength * ovlap.z * ( nrm.x * nrm.x * ovlap.x + nrm.y * nrm.y * ovlap.y ) / ftmp;

            // determine a usable mass
            m0 = -1;
            m1 = -1;
            if ( pchra->weight < 0 && pchrb->weight < 0 )
            {
              m0 = m1 = 110.0f;
            }
            else if (pchra->weight == 0 && pchrb->weight == 0)
            {
              m0 = m1 = 1.0f;
            }
            else
            {
              m0 = (pchra->weight == 0.0f) ? 1.0 : pchra->weight;
              m1 = (pchrb->weight == 0.0f) ? 1.0 : pchrb->weight;
            }

            bfound = btrue;
            cr = pchrb->bumpdampen * pchra->bumpdampen;
            //ln_cr = log(cr);

            if( m0 > 0.0f && bumpstrength > 0.0f )
            {
              float k = 250.0f / m0;
              float gamma = 0.5f * (1.0f - cr) * (1.0f - cr);

              //if(cr != 0.0f)
              //{
              //  gamma = 2.0f * ABS(ln_cr) * sqrt( k / (ln_cr*ln_cr + PI*PI) );
              //}
              //else
              //{
              //  gamma = 2.0f * sqrt(k);
              //}

              pchra->accum.acc.x += (diffa.x * k  - pchra->ori.vel.x * gamma) * bumpstrength;
              pchra->accum.acc.y += (diffa.y * k  - pchra->ori.vel.y * gamma) * bumpstrength;
              pchra->accum.acc.z += (diffa.z * k  - pchra->ori.vel.z * gamma) * bumpstrength;
            }

            if( m1 > 0.0f && bumpstrength > 0.0f )
            {
              float k = 250.0f / m1;
              float gamma = 0.5f * (1.0f - cr) * (1.0f - cr);

              //if(cr != 0.0f)
              //{
              //  gamma = 2.0f * ABS(ln_cr) * sqrt( k / (ln_cr*ln_cr + PI*PI) );
              //}
              //else
              //{
              //  gamma = 2.0f * sqrt(k);
              //}

              pchrb->accum.acc.x += (diffb.x * k  - pchrb->ori.vel.x * gamma) * bumpstrength;
              pchrb->accum.acc.y += (diffb.y * k  - pchrb->ori.vel.y * gamma) * bumpstrength;
              pchrb->accum.acc.z += (diffb.z * k  - pchrb->ori.vel.z * gamma) * bumpstrength;
            }

            //bfound = bfalse;
            //if (( pchra->bmpdata.mids_lo.x - pchrb->bmpdata.mids_lo.x ) * ( pchra->ori.vel.x - pchrb->ori.vel.x ) < 0.0f )
            //{
            //  u0 = pchra->ori.vel.x;
            //  u1 = pchrb->ori.vel.x;

            //  psum = m0 * u0 + m1 * u1;
            //  udif = u1 - u0;

            //  pchra->ori.vel.x = ( psum - m1 * udif * cr ) / msum;
            //  pchrb->ori.vel.x = ( psum + m0 * udif * cr ) / msum;

            //  //pchra->bmpdata.mids_lo.x -= pchra->ori.vel.x*dUpdate;
            //  //pchrb->bmpdata.mids_lo.x -= pchrb->ori.vel.x*dUpdate;

            //  bfound = btrue;
            //}

            //if (( pchra->bmpdata.mids_lo.y - pchrb->bmpdata.mids_lo.y ) * ( pchra->ori.vel.y - pchrb->ori.vel.y ) < 0.0f )
            //{
            //  u0 = pchra->ori.vel.y;
            //  u1 = pchrb->ori.vel.y;

            //  psum = m0 * u0 + m1 * u1;
            //  udif = u1 - u0;

            //  pchra->ori.vel.y = ( psum - m1 * udif * cr ) / msum;
            //  pchrb->ori.vel.y = ( psum + m0 * udif * cr ) / msum;

            //  //pchra->bmpdata.mids_lo.y -= pchra->ori.vel.y*dUpdate;
            //  //pchrb->bmpdata.mids_lo.y -= pchrb->ori.vel.y*dUpdate;

            //  bfound = btrue;
            //}

            //if ( ovlap.x > 0 && ovlap.z > 0 )
            //{
            //  pchra->bmpdata.mids_lo.x += m1 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
            //  pchrb->bmpdata.mids_lo.x -= m0 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
            //  bfound = btrue;
            //}

            //if ( ovlap.y > 0 && ovlap.z > 0 )
            //{
            //  pchra->bmpdata.mids_lo.y += m1 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
            //  pchrb->bmpdata.mids_lo.y -= m0 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
            //  bfound = btrue;
            //}

            if ( bfound )
            {
              //apos = pchra->ori.pos;
              pchra->aistate.alert |= ALERT_BUMPED;
              pchrb->aistate.alert |= ALERT_BUMPED;
              pchra->aistate.bumplast = ichrb;
              pchrb->aistate.bumplast = ichra;
            };
          }

        }
        else
        {
          IDSZ chridvulnerability, eveidremove;
          float chrbump = 1.0f;

          float bumpstrength, prtbump;
          bool_t chr_is_vulnerable;

          CHR_REF prt_owner;
          CHR_REF prt_attached;


          chridvulnerability = ChrList_getPCap(gs, ichra)->idsz[IDSZ_VULNERABILITY];
          chrbump = pchra->bumpstrength;

          // do the character-particle interactions
          iprtb = d->prtb;
          pprtb = PrtList_getPPrt(gs, iprtb);
          if(NULL == pprtb) continue;

          prt_owner = prt_get_owner( gs, iprtb );
          prt_attached = prt_get_attachedtochr( gs, iprtb );

          pip = pprtb->pip;
          bpos = pprtb->ori.pos;

          chr_is_vulnerable = !pchra->prop.invictus && ( IDSZ_NONE != chridvulnerability ) && CAP_INHERIT_IDSZ( gs,  pprtb->model, chridvulnerability );

          prtbump = pprtb->bumpstrength;
          bumpstrength = chr_is_vulnerable ? 1.0f : chrbump * prtbump;

          if ( 0.0f == bumpstrength ) continue;

          // First check absolute value diamond
          diff.x = ABS( apos.x - bpos.x );
          diff.y = ABS( apos.y - bpos.y );
          dist = diff.x + diff.y;
          if ( prt_do_collision( gs, ichra, iprtb, bfalse ) )
          {
            vect3 pvel;

            if ( INVALID_CHR != prt_get_attachedtochr( gs, iprtb ) )
            {
              pvel.x = ( pprtb->ori.pos.x - pprtb->ori_old.pos.x ) / dUpdate;
              pvel.y = ( pprtb->ori.pos.y - pprtb->ori_old.pos.y ) / dUpdate;
              pvel.z = ( pprtb->ori.pos.z - pprtb->ori_old.pos.z ) / dUpdate;
            }
            else
            {
              pvel = pprtb->ori.vel;
            }

            if ( bpos.z > pchra->bmpdata.cv.z_max + pvel.z && pvel.z < 0 && pchra->bmpdata.calc_is_platform && !ACTIVE_CHR( chrlst, prt_attached ) )
            {
              // Particle is falling on A
              pprtb->accum.pos.z += pchra->bmpdata.cv.z_max - pprtb->ori.pos.z;

              pprtb->accum.vel.z = - (1.0f - piplst[pip].dampen * pchra->bumpdampen) * pprtb->ori.vel.z;

              pprtb->accum.acc.x += ( pvel.x - pchra->ori.vel.x ) * ( 1.0 - loc_platstick ) + pchra->ori.vel.x;
              pprtb->accum.acc.y += ( pvel.y - pchra->ori.vel.y ) * ( 1.0 - loc_platstick ) + pchra->ori.vel.y;
            }

            // Check reaffirmation of particles
            if ( prt_attached != ichra )
            {
              if ( pchra->reloadtime == 0 )
              {
                if ( pchra->reaffirmdamagetype == pprtb->damagetype && pchra->damagetime == 0 )
                {
                  reaffirm_attached_particles( gs, ichra );
                }
              }
            }

            // Check for missile treatment
            if (( pchra->skin.damagemodifier_fp8[pprtb->damagetype]&DAMAGE_SHIFT ) != DAMAGE_SHIFT ||
              MIS_NORMAL == pchra->missiletreatment  ||
              ACTIVE_CHR( chrlst, prt_attached ) ||
              ( prt_owner == ichra && !piplst[pip].friendlyfire ) ||
              ( chrlst[pchra->missilehandler].stats.mana_fp8 < ( pchra->missilecost << 4 ) && !chrlst[pchra->missilehandler].prop.canchannel ) )
            {
              if (( gs->TeamList[pprtb->team].hatesteam[pchra->team] || ( piplst[pip].friendlyfire && (( ichra != prt_owner && ichra != chrlst[prt_owner].attachedto ) || piplst[pip].onlydamagefriendly ) ) ) && !pchra->prop.invictus )
              {
                spawn_bump_particles( gs, ichra, iprtb );  // Catch on fire

                if (( pprtb->damage.ibase > 0 ) && ( pprtb->damage.irand > 0 ) )
                {
                  if ( pchra->damagetime == 0 && prt_attached != ichra && HAS_NO_BITS( piplst[pip].damfx, DAMFX_ARRO ) )
                  {

                    // Normal iprtb damage
                    if ( piplst[pip].allowpush )
                    {
                      float ftmp = 0.2;

                      if ( pchra->weight < 0 )
                      {
                        ftmp = 0;
                      }
                      else if ( pchra->weight != 0 )
                      {
                        ftmp *= ( 1.0f + pchra->bumpdampen ) * pprtb->weight / pchra->weight;
                      }

                      pchra->accum.vel.x += pvel.x * ftmp;
                      pchra->accum.vel.y += pvel.y * ftmp;
                      pchra->accum.vel.z += pvel.z * ftmp;

                      pprtb->accum.vel.x += -pchra->bumpdampen * pvel.x - pprtb->ori.vel.x;
                      pprtb->accum.vel.y += -pchra->bumpdampen * pvel.y - pprtb->ori.vel.y;
                      pprtb->accum.vel.z += -pchra->bumpdampen * pvel.z - pprtb->ori.vel.z;
                    }

                    direction = RAD_TO_TURN( atan2( pvel.y, pvel.x ) );
                    direction = 32768 + pchra->ori.turn_lr - direction;

                    // Check all enchants to see if they are removed
                    enchant = pchra->firstenchant;
                    while ( INVALID_ENC != enchant )
                    {
                      ENC_REF temp;
                      eveidremove = gs->EveList[enclst[enchant].eve].removedbyidsz;
                      temp = enclst[enchant].nextenchant;
                      if ( eveidremove != IDSZ_NONE && CAP_INHERIT_IDSZ( gs,  pprtb->model, eveidremove ) )
                      {
                        remove_enchant( gs, enchant );
                      }
                      enchant = temp;
                    }

                    //Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                    //+1 (256) bonus for every 4 points of intelligence and/or wisdom above 14. Below 14 gives -1 instead!
                    //Enemy IDAM spells damage is reduced by 1% per defender's wisdom, opposite for WDAM spells
                    if ( piplst[pip].intdamagebonus )
                    {
                      pprtb->damage.ibase += (( chrlst[prt_owner].stats.intelligence_fp8 - 3584 ) * 0.25 );    //First increase damage by the attacker
                      if(!pchra->skin.damagemodifier_fp8[pprtb->damagetype]&DAMAGE_INVERT || !pchra->skin.damagemodifier_fp8[pprtb->damagetype]&DAMAGE_CHARGE)
                        pprtb->damage.ibase -= (pprtb->damage.ibase * ( pchra->stats.wisdom_fp8 > 8 ));    //Then reduce it by defender
                    }
                    if ( piplst[pip].wisdamagebonus )  //Same with divine spells
                    {
                      pprtb->damage.ibase += (( chrlst[prt_owner].stats.wisdom_fp8 - 3584 ) * 0.25 );
                      if(!pchra->skin.damagemodifier_fp8[pprtb->damagetype]&DAMAGE_INVERT || !pchra->skin.damagemodifier_fp8[pprtb->damagetype]&DAMAGE_CHARGE)
                        pprtb->damage.ibase -= (pprtb->damage.ibase * ( pchra->stats.intelligence_fp8 > 8 ));
                    }

                    //Force Pancake animation?
                    if ( piplst[pip].causepancake )
                    {
                      vect3 panc;
                      Uint16 rotate_sin;
                      float cv, sv;

                      // just a guess
                      panc.x = 0.25 * ABS( pvel.x ) * 2.0f / ( float )( 1 + pchra->bmpdata.cv.x_max - pchra->bmpdata.cv.x_min  );
                      panc.y = 0.25 * ABS( pvel.y ) * 2.0f / ( float )( 1 + pchra->bmpdata.cv.y_max - pchra->bmpdata.cv.y_min );
                      panc.z = 0.25 * ABS( pvel.z ) * 2.0f / ( float )( 1 + pchra->bmpdata.cv.z_max - pchra->bmpdata.cv.z_min );

                      rotate_sin = pchra->ori.turn_lr >> 2;

                      cv = turntocos[rotate_sin];
                      sv = turntosin[rotate_sin];

                      pchra->pancakevel.x = - ( panc.x * cv - panc.y * sv );
                      pchra->pancakevel.y = - ( panc.x * sv + panc.y * cv );
                      pchra->pancakevel.z = -panc.z;
                    }

                    // Damage the character
                    if ( chr_is_vulnerable )
                    {
                      PAIR ptemp;
                      ptemp.ibase = pprtb->damage.ibase * 2.0f * bumpstrength;
                      ptemp.irand = pprtb->damage.irand * 2.0f * bumpstrength;
                      damage_character( gs, ichra, direction, &ptemp, pprtb->damagetype, pprtb->team, prt_owner, piplst[pip].damfx );
                      pchra->aistate.alert |= ALERT_HITVULNERABLE;
                      cost_mana( gs, ichra, piplst[pip].manadrain*2, prt_owner );  //Do mana drain too
                    }
                    else
                    {
                      PAIR ptemp;
                      ptemp.ibase = pprtb->damage.ibase * bumpstrength;
                      ptemp.irand = pprtb->damage.irand * bumpstrength;

                      damage_character( gs, ichra, direction, &pprtb->damage, pprtb->damagetype, pprtb->team, prt_owner, piplst[pip].damfx );
                      cost_mana( gs, ichra, piplst[pip].manadrain, prt_owner );  //Do mana drain too
                    }

                    // Do confuse effects
                    if ( HAS_NO_BITS( ChrList_getPMad(gs, ichra)->framefx[pchra->anim.next], MADFX_INVICTUS ) || HAS_SOME_BITS( piplst[pip].damfx, DAMFX_BLOC ) )
                    {

                      if ( piplst[pip].grogtime != 0 && chrlst[ichra].prop.canbegrogged )
                      {
                        pchra->grogtime += piplst[pip].grogtime * bumpstrength;
                        if ( pchra->grogtime < 0 )
                        {
                          pchra->grogtime = -1;
                          debug_message( 1, "placing infinite grog on %s (%s)", pchra->name, ChrList_getPCap(gs, ichra)->classname );
                        }
                        pchra->aistate.alert |= ALERT_GROGGED;
                      }

                      if ( piplst[pip].dazetime != 0 && chrlst[ichra].prop.canbedazed )
                      {
                        pchra->dazetime += piplst[pip].dazetime * bumpstrength;
                        if ( pchra->dazetime < 0 )
                        {
                          pchra->dazetime = -1;
                          debug_message( 1, "placing infinite daze on %s (%s)", pchra->name, ChrList_getPCap(gs, ichra)->classname );
                        };
                        pchra->aistate.alert |= ALERT_DAZED;
                      }
                    }

                    // Notify the attacker of a scored hit
                    if ( ACTIVE_CHR( chrlst, prt_owner ) )
                    {
                      chrlst[prt_owner].aistate.alert |= ALERT_SCOREDAHIT;
                      chrlst[prt_owner].aistate.hitlast = ichra;
                    }
                  }

                  if (( gs->wld_frame&31 ) == 0 && prt_attached == ichra )
                  {
                    // Attached iprtb damage ( Burning )
                    if ( piplst[pip].xyvel.ibase == 0 )
                    {
                      // Make character limp
                      pchra->ori.vel.x = 0;
                      pchra->ori.vel.y = 0;
                    }
                    damage_character( gs, ichra, 32768, &pprtb->damage, pprtb->damagetype, pprtb->team, prt_owner, piplst[pip].damfx );
                    cost_mana( gs, ichra, piplst[pip].manadrain, prt_owner );  //Do mana drain too

                  }
                }

                if ( piplst[pip].endbump )
                {
                  if ( piplst[pip].bumpmoney )
                  {
                    if ( pchra->prop.cangrabmoney && pchra->alive && pchra->damagetime == 0 && pchra->money != MAXMONEY )
                    {
                      if ( pchra->prop.ismount )
                      {
                        CHR_REF irider = chr_get_holdingwhich( chrlst, chrlst_size, ichra, SLOT_SADDLE );

                        // Let mounts collect money for their riders
                        if ( ACTIVE_CHR( chrlst, irider ) )
                        {
                          chrlst[irider].money += piplst[pip].bumpmoney;
                          if ( chrlst[irider].money > MAXMONEY ) chrlst[irider].money = MAXMONEY;
                          if ( chrlst[irider].money <        0 ) chrlst[irider].money = 0;
                          pprtb->gopoof = btrue;
                        }
                      }
                      else
                      {
                        // Normal money collection
                        pchra->money += piplst[pip].bumpmoney;
                        if ( pchra->money > MAXMONEY ) pchra->money = MAXMONEY;
                        if ( pchra->money < 0 ) pchra->money = 0;
                        pprtb->gopoof = btrue;
                      }
                    }
                  }
                  else
                  {
                    // Only hit one character, not several
                    pprtb->damage.ibase *= 1.0f - bumpstrength;
                    pprtb->damage.irand *= 1.0f - bumpstrength;

                    if ( pprtb->damage.ibase == 0 && pprtb->damage.irand <= 1 )
                    {
                      pprtb->gopoof = btrue;
                    };
                  }
                }
              }
            }
            else if ( prt_owner != ichra )
            {
              cost_mana( gs, pchra->missilehandler, ( pchra->missilecost << 4 ), prt_owner );

              // Treat the missile
              switch ( pchra->missiletreatment )
              {
              case MIS_DEFLECT:
                {
                  // Use old position to find normal
                  acc.x = pprtb->ori.pos.x - pvel.x * dUpdate;
                  acc.y = pprtb->ori.pos.y - pvel.y * dUpdate;
                  acc.x = pchra->ori.pos.x - acc.x;
                  acc.y = pchra->ori.pos.y - acc.y;
                  // Find size of normal
                  scale = acc.x * acc.x + acc.y * acc.y;
                  if ( scale > 0 )
                  {
                    // Make the normal a unit normal
                    scale = sqrt( scale );
                    nrm.x = acc.x / scale;
                    nrm.y = acc.y / scale;

                    // Deflect the incoming ray off the normal
                    scale = ( pvel.x * nrm.x + pvel.y * nrm.y ) * 2;
                    acc.x = scale * nrm.x;
                    acc.y = scale * nrm.y;
                    pprtb->accum.vel.x += -acc.x;
                    pprtb->accum.vel.y += -acc.y;
                  }
                }
                break;

              case MIS_REFLECT:
                {
                  // Reflect it back in the direction it came
                  pprtb->accum.vel.x += -2.0f * pprtb->ori.vel.x;
                  pprtb->accum.vel.y += -2.0f * pprtb->ori.vel.y;
                };
                break;
              };

              // Change the owner of the missile
              if ( !piplst[pip].homing )
              {
                pprtb->team = pchra->team;
                prt_owner = ichra;
              }
            }
          }

        }
      }
    }
  }




}

//--------------------------------------------------------------------------------------------
void stat_return( Game_t * gs, float dUpdate )
{
  // ZZ> This function brings mana and life back

  Client_t * cs = gs->cl;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PEnc enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  CHR_REF chr_cnt, owner, target;
  ENC_REF enc_cnt;
  EVE_REF eve;
  static int stat_return_counter = 0;

  // Do reload time

  for ( chr_cnt = 0; chr_cnt < chrlst_size; chr_cnt++ )
  {
    chrlst[chr_cnt].reloadtime -= dUpdate;
    if ( chrlst[chr_cnt].reloadtime < 0 ) chrlst[chr_cnt].reloadtime = 0;
  }

  // Do stats
  if ( cs->stat_clock == ONESECOND )
  {
    // Reset the clock
    cs->stat_clock = 0;
    stat_return_counter++;

    // Do all the characters
    for ( chr_cnt = 0; chr_cnt < chrlst_size; chr_cnt++ )
    {
      if ( !ACTIVE_CHR( chrlst, chr_cnt ) ) continue;

      if ( chrlst[chr_cnt].alive )
      {
        chrlst[chr_cnt].stats.mana_fp8 += chrlst[chr_cnt].stats.manareturn_fp8 >> MANARETURNSHIFT;
        if ( chrlst[chr_cnt].stats.mana_fp8 < 0 ) chrlst[chr_cnt].stats.mana_fp8 = 0;
        if ( chrlst[chr_cnt].stats.mana_fp8 > chrlst[chr_cnt].stats.manamax_fp8 ) chrlst[chr_cnt].stats.mana_fp8 = chrlst[chr_cnt].stats.manamax_fp8;

        chrlst[chr_cnt].stats.life_fp8 += chrlst[chr_cnt].stats.lifereturn_fp8;
        if ( chrlst[chr_cnt].stats.life_fp8 < 1 ) chrlst[chr_cnt].stats.life_fp8 = 1;
        if ( chrlst[chr_cnt].stats.life_fp8 > chrlst[chr_cnt].stats.lifemax_fp8 ) chrlst[chr_cnt].stats.life_fp8 = chrlst[chr_cnt].stats.lifemax_fp8;
      };

      if ( chrlst[chr_cnt].grogtime > 0 )
      {
        chrlst[chr_cnt].grogtime--;
        if ( chrlst[chr_cnt].grogtime < 0 ) chrlst[chr_cnt].grogtime = 0;

        if ( chrlst[chr_cnt].grogtime == 0 )
        {
          debug_message( 1, "stat_return() - removing grog on %s (%s)", chrlst[chr_cnt].name, ChrList_getPCap(gs, chr_cnt)->classname );
        };
      }

      if ( chrlst[chr_cnt].dazetime > 0 )
      {
        chrlst[chr_cnt].dazetime--;
        if ( chrlst[chr_cnt].dazetime < 0 ) chrlst[chr_cnt].dazetime = 0;
        if ( chrlst[chr_cnt].grogtime == 0 )
        {
          debug_message( 1, "stat_return() - removing daze on %s (%s)", chrlst[chr_cnt].name, ChrList_getPCap(gs, chr_cnt)->classname );
        };
      }

    }

    // Run through all the enchants as well
    for ( enc_cnt = 0; enc_cnt < enclst_size; enc_cnt++ )
    {
      bool_t kill_enchant = bfalse;
      if ( !ACTIVE_ENC(enclst, enc_cnt) ) continue;

      if ( enclst[enc_cnt].time == 0 )
      {
        kill_enchant = btrue;
      };

      if ( enclst[enc_cnt].time > 0 ) enclst[enc_cnt].time--;

      owner = enclst[enc_cnt].owner;
      target = enclst[enc_cnt].target;
      eve = enclst[enc_cnt].eve;

      // Do drains
      if ( !kill_enchant && chrlst[owner].alive )
      {
        // Change life
        chrlst[owner].stats.life_fp8 += enclst[enc_cnt].ownerlife_fp8;
        if ( chrlst[owner].stats.life_fp8 < 1 )
        {
          chrlst[owner].stats.life_fp8 = 1;
          kill_character( gs, owner, target );
        }
        if ( chrlst[owner].stats.life_fp8 > chrlst[owner].stats.lifemax_fp8 )
        {
          chrlst[owner].stats.life_fp8 = chrlst[owner].stats.lifemax_fp8;
        }
        // Change mana
        if ( !cost_mana( gs, owner, -enclst[enc_cnt].ownermana_fp8, target ) && gs->EveList[eve].endifcantpay )
        {
          kill_enchant = btrue;
        }
      }
      else if ( !gs->EveList[eve].stayifnoowner )
      {
        kill_enchant = btrue;
      }

      if ( !kill_enchant && ACTIVE_ENC(enclst, enc_cnt) )
      {
        if ( chrlst[target].alive )
        {
          // Change life
          chrlst[target].stats.life_fp8 += enclst[enc_cnt].targetlife_fp8;
          if ( chrlst[target].stats.life_fp8 < 1 )
          {
            chrlst[target].stats.life_fp8 = 1;
            kill_character( gs, target, owner );
          }
          if ( chrlst[target].stats.life_fp8 > chrlst[target].stats.lifemax_fp8 )
          {
            chrlst[target].stats.life_fp8 = chrlst[target].stats.lifemax_fp8;
          }

          // Change mana
          if ( !cost_mana( gs, target, -enclst[enc_cnt].targetmana_fp8, owner ) && gs->EveList[eve].endifcantpay )
          {
            kill_enchant = btrue;
          }
        }
        else
        {
          kill_enchant = btrue;
        }
      }

      if ( kill_enchant )
      {
        remove_enchant( gs, enc_cnt );
      };
    }
  }
}

//--------------------------------------------------------------------------------------------
void pit_kill( Game_t * gs, float dUpdate )
{
  // ZZ> This function kills any character in a deep pit...

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  PPip_t piplst      = gs->PipList;
  size_t piplst_size = PIPLST_COUNT;


  PRT_REF prt_cnt;
  CHR_REF chr_cnt;

  if ( gs->pitskill )
  {
    if ( gs->pit_clock > 19 )
    {
      gs->pit_clock = 0;

      // Kill any particles that fell in a pit, if they die in water...

      for ( prt_cnt = 0; prt_cnt < prtlst_size; prt_cnt++ )
      {
        if ( !ACTIVE_PRT( prtlst, prt_cnt ) ) continue;

        if ( prtlst[prt_cnt].ori.pos.z < PITDEPTH && PrtList_getPPip(gs, prt_cnt)->endwater )
        {
          prtlst[prt_cnt].gopoof = btrue;
        }
      }

      // Kill any characters that fell in a pit...

      for ( chr_cnt = 0; chr_cnt < chrlst_size; chr_cnt++ )
      {
        if( !ACTIVE_CHR(chrlst, chr_cnt) ) continue;

        if ( chrlst[chr_cnt].alive && !chr_in_pack( chrlst, chrlst_size, chr_cnt ) )
        {
          if ( !chrlst[chr_cnt].prop.invictus && chrlst[chr_cnt].ori.pos.z < PITDEPTH && !chr_attached( chrlst, chrlst_size, chr_cnt ) )
          {
            // Got one!
            kill_character( gs, chr_cnt, INVALID_CHR );
            chrlst[chr_cnt].ori.vel.x = 0;
            chrlst[chr_cnt].ori.vel.y = 0;
          }
        }
      }
    }
    else
    {
      gs->pit_clock += dUpdate;
    }
  }
}

//--------------------------------------------------------------------------------------------
void reset_players( Game_t * gs )
{
  // ZZ> This function clears the player list data

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;


  // Reset the local data stuff
  if(NULL != gs->cl)
  {
    gs->cl->seekurse    = bfalse;
    gs->cl->seeinvisible = bfalse;
  }
  gs->somepladead  = bfalse;
  gs->allpladead   = bfalse;

  // Reset the initial player data and latches
  PlaList_renew( gs );

  CClient_reset_latches( gs->cl );
  CServer_reset_latches( gs->sv );
}

//--------------------------------------------------------------------------------------------
void resize_characters( Game_t * gs, float dUpdate )
{
  // ZZ> This function makes the characters get bigger or smaller, depending
  //     on their sizegoto and sizegototime

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  CHR_REF chr_ref;

  bool_t willgetcaught;
  float newsize, fkeep;

  fkeep = pow( 0.9995, dUpdate );

  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !ACTIVE_CHR( chrlst, chr_ref ) || chrlst[chr_ref].sizegototime <= 0 ) continue;

    // Make sure it won't get caught in a wall
    willgetcaught = bfalse;
    if ( chrlst[chr_ref].sizegoto > chrlst[chr_ref].fat )
    {
      float x_min_save, x_max_save;
      float y_min_save, y_max_save;

      x_min_save = chrlst[chr_ref].bmpdata.cv.x_min;
      x_max_save = chrlst[chr_ref].bmpdata.cv.x_max;

      y_min_save = chrlst[chr_ref].bmpdata.cv.y_min;
      y_max_save = chrlst[chr_ref].bmpdata.cv.y_max;

      chrlst[chr_ref].bmpdata.cv.x_min -= 5;
      chrlst[chr_ref].bmpdata.cv.y_min -= 5;

      chrlst[chr_ref].bmpdata.cv.x_max += 5;
      chrlst[chr_ref].bmpdata.cv.y_max += 5;

      if ( 0 != chr_hitawall( gs, chrlst + chr_ref, NULL ) )
      {
        willgetcaught = btrue;
      }

      chrlst[chr_ref].bmpdata.cv.x_min = x_min_save;
      chrlst[chr_ref].bmpdata.cv.x_max = x_max_save;

      chrlst[chr_ref].bmpdata.cv.y_min = y_min_save;
      chrlst[chr_ref].bmpdata.cv.y_max = y_max_save;
    }

    // If it is getting caught, simply halt growth until later
    if ( willgetcaught ) continue;

    // Figure out how big it is
    chrlst[chr_ref].sizegototime -= dUpdate;
    if ( chrlst[chr_ref].sizegototime < 0 )
    {
      chrlst[chr_ref].sizegototime = 0;
    }

    if ( chrlst[chr_ref].sizegototime > 0 )
    {
      newsize = chrlst[chr_ref].fat * fkeep + chrlst[chr_ref].sizegoto * ( 1.0f - fkeep );
    }
    else if ( chrlst[chr_ref].sizegototime <= 0 )
    {
      newsize = chrlst[chr_ref].fat;
    }

    // Make it that big...
    chrlst[chr_ref].fat             = newsize;
    chrlst[chr_ref].bmpdata.shadow  = chrlst[chr_ref].bmpdata_save.shadow * newsize;
    chrlst[chr_ref].bmpdata.size    = chrlst[chr_ref].bmpdata_save.size * newsize;
    chrlst[chr_ref].bmpdata.sizebig = chrlst[chr_ref].bmpdata_save.sizebig * newsize;
    chrlst[chr_ref].bmpdata.height  = chrlst[chr_ref].bmpdata_save.height * newsize;
    chrlst[chr_ref].weight          = ChrList_getPCap(gs, chr_ref)->weight * newsize * newsize * newsize;  // preserve density

    // Now come up with the magic number
    chrlst[chr_ref].scale = newsize;

    // calculate the bumpers
    make_one_character_matrix( chrlst, chrlst_size, chrlst + chr_ref );
  }

  // recalc the bounding boxes of any character that was resized
  recalc_character_bumpers(gs);
}

//--------------------------------------------------------------------------------------------
void export_one_character_name( Game_t * gs, char *szSaveName, CHR_REF chr_ref )
{
  // ZZ> This function makes the "NAMING.TXT" file for the character

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  FILE* filewrite;
  OBJ_REF profile;

  // Can it export?
  profile = chrlst[chr_ref].model;
  filewrite = fs_fileOpen( PRI_FAIL, "export_one_character_name()", szSaveName, "w" );
  if ( NULL == filewrite )
  {
      log_error( "Error writing file (%s)\n", szSaveName );
      return;
  }

  str_convert_spaces( chrlst[chr_ref].name, sizeof( chrlst[chr_ref].name ), chrlst[chr_ref].name );
  fprintf( filewrite, ":%s\n", chrlst[chr_ref].name );
  fprintf( filewrite, ":STOP\n\n" );
  fs_fileClose( filewrite );
}

//--------------------------------------------------------------------------------------------
void export_one_character_profile( Game_t * gs, char *szSaveName, CHR_REF ichr )
{
  // ZZ> This function creates a "DATA.TXT" file for the given character.
  //     it is assumed that all enchantments have been done away with

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PObj_t   objlst = gs->ObjList;
  size_t objlst_size = OBJLST_COUNT;

  FILE* filewrite;
  int damagetype, iskin;
  char types[10] = "SCPHEFIZ";
  char codes[4];

  Chr_t  * pchr;

  OBJ_REF iobj;
  Obj_t  * pobj;

  Cap_t  * pcap;

  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return;

  disenchant_character( gs, ichr );

  // General stuff
  iobj = ChrList_getRObj(gs, ichr);
  if( INVALID_OBJ == iobj ) return;
  pobj = ChrList_getPObj(gs, ichr);

  pcap = ChrList_getPCap(gs, ichr);
  if(NULL == pcap) return;

  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( NULL == filewrite ) return;

  // Real general data
  fprintf( filewrite, "Slot number    : -1\n" );   // -1 signals a flexible load thing
  funderf( filewrite, "Class name     : ", pcap->classname );
  ftruthf( filewrite, "Uniform light  : ", pcap->uniformlit );
  fprintf( filewrite, "Maximum ammo   : %d\n", pcap->ammomax );
  fprintf( filewrite, "Current ammo   : %d\n", pcap->ammo );
  fgendef( filewrite, "Gender         : ", pcap->gender );
  fprintf( filewrite, "\n" );

  // Object stats
  fprintf( filewrite, "Life color     : %d\n", pcap->lifecolor );
  fprintf( filewrite, "Mana color     : %d\n", pcap->manacolor );
  fprintf( filewrite, "Life           : %4.2f\n", FP8_TO_FLOAT( pchr->stats.lifemax_fp8 ) );
  fpairof( filewrite, "Life up        : ", &pcap->statdata.lifeperlevel_pair );
  fprintf( filewrite, "Mana           : %4.2f\n", FP8_TO_FLOAT( pchr->stats.manamax_fp8 ) );
  fpairof( filewrite, "Mana up        : ", &pcap->statdata.manaperlevel_pair );
  fprintf( filewrite, "Mana return    : %4.2f\n", FP8_TO_FLOAT( pchr->stats.manareturn_fp8 ) );
  fpairof( filewrite, "Mana return up : ", &pcap->statdata.manareturnperlevel_pair );
  fprintf( filewrite, "Mana flow      : %4.2f\n", FP8_TO_FLOAT( pchr->stats.manaflow_fp8 ) );
  fpairof( filewrite, "Mana flow up   : ", &pcap->statdata.manaflowperlevel_pair );
  fprintf( filewrite, "STR            : %4.2f\n", FP8_TO_FLOAT( pchr->stats.strength_fp8 ) );
  fpairof( filewrite, "STR up         : ", &pcap->statdata.strengthperlevel_pair );
  fprintf( filewrite, "WIS            : %4.2f\n", FP8_TO_FLOAT( pchr->stats.wisdom_fp8 ) );
  fpairof( filewrite, "WIS up         : ", &pcap->statdata.wisdomperlevel_pair );
  fprintf( filewrite, "INT            : %4.2f\n", FP8_TO_FLOAT( pchr->stats.intelligence_fp8 ) );
  fpairof( filewrite, "INT up         : ", &pcap->statdata.intelligenceperlevel_pair );
  fprintf( filewrite, "DEX            : %4.2f\n", FP8_TO_FLOAT( pchr->stats.dexterity_fp8 ) );
  fpairof( filewrite, "DEX up         : ", &pcap->statdata.dexterityperlevel_pair );
  fprintf( filewrite, "\n" );

  // More physical attributes
  fprintf( filewrite, "Size           : %4.2f\n", pchr->sizegoto );
  fprintf( filewrite, "Size up        : %4.2f\n", pcap->sizeperlevel );
  fprintf( filewrite, "Shadow size    : %d\n", pcap->shadowsize );
  fprintf( filewrite, "Bump size      : %d\n", pcap->bumpsize );
  fprintf( filewrite, "Bump height    : %d\n", pcap->bumpheight );
  fprintf( filewrite, "Bump dampen    : %4.2f\n", pcap->bumpdampen );
  fprintf( filewrite, "Weight         : %d\n", pcap->weight < 0.0f ? 0xFF : ( Uint8 ) pcap->weight );
  fprintf( filewrite, "Jump power     : %4.2f\n", pcap->jump );
  fprintf( filewrite, "Jump number    : %d\n", pcap->jumpnumber );
  fprintf( filewrite, "Sneak speed    : %d\n", pcap->spd_sneak );
  fprintf( filewrite, "Walk speed     : %d\n", pcap->spd_walk );
  fprintf( filewrite, "Run speed      : %d\n", pcap->spd_run );
  fprintf( filewrite, "Fly to height  : %d\n", pcap->flyheight );
  fprintf( filewrite, "Flashing AND   : %d\n", pcap->flashand );
  fprintf( filewrite, "Alpha blending : %d\n", pcap->alpha_fp8 );
  fprintf( filewrite, "Light blending : %d\n", pcap->light_fp8 );
  ftruthf( filewrite, "Transfer blend : ", pcap->transferblend );
  fprintf( filewrite, "Sheen          : %d\n", pcap->sheen_fp8 );
  ftruthf( filewrite, "Phong mapping  : ", pcap->enviro );
  fprintf( filewrite, "Texture X add  : %4.2f\n", pcap->uoffvel / (float)UINT16_SIZE );
  fprintf( filewrite, "Texture Y add  : %4.2f\n", pcap->voffvel / (float)UINT16_SIZE );
  ftruthf( filewrite, "Sticky butt    : ", pcap->prop.stickybutt );
  fprintf( filewrite, "\n" );

  // Invulnerability data
  ftruthf( filewrite, "Invictus       : ", pcap->prop.invictus );
  fprintf( filewrite, "NonI facing    : %d\n", pcap->nframefacing );
  fprintf( filewrite, "NonI angle     : %d\n", pcap->nframeangle );
  fprintf( filewrite, "I facing       : %d\n", pcap->iframefacing );
  fprintf( filewrite, "I angle        : %d\n", pcap->iframeangle );
  fprintf( filewrite, "\n" );

  // Skin defenses
  fprintf( filewrite, "Base defense   : " );
  for ( iskin = 0; iskin < MAXSKIN; iskin++ ) { fprintf( filewrite, "%3d ", 255 - pcap->skin[iskin].defense_fp8 ); }
  fprintf( filewrite, "\n" );

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fprintf( filewrite, "%c damage shift :", types[damagetype] );
    for ( iskin = 0; iskin < MAXSKIN; iskin++ ) { fprintf( filewrite, "%3d ", pcap->skin[iskin].damagemodifier_fp8[damagetype]&DAMAGE_SHIFT ); };
    fprintf( filewrite, "\n" );
  }

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fprintf( filewrite, "%c damage code  : ", types[damagetype] );
    for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    {
      codes[iskin] = 'F';
      if ( pcap->skin[iskin].damagemodifier_fp8[damagetype] & DAMAGE_CHARGE ) codes[iskin] = 'C';
      if ( pcap->skin[iskin].damagemodifier_fp8[damagetype] & DAMAGE_INVERT ) codes[iskin] = 'T';
      if ( pcap->skin[iskin].damagemodifier_fp8[damagetype] & DAMAGE_MANA   ) codes[iskin] = 'M';
      fprintf( filewrite, "%3c ", codes[iskin] );
    }
    fprintf( filewrite, "\n" );
  }

  fprintf( filewrite, "Acceleration   : " );
  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
  {
    fprintf( filewrite, "%3.0f ", pcap->skin[iskin].maxaccel*80 );
  }
  fprintf( filewrite, "\n" );

  // Experience and level data
  fprintf( filewrite, "EXP for 2nd    : %d\n", pcap->experienceforlevel[1] );
  fprintf( filewrite, "EXP for 3rd    : %d\n", pcap->experienceforlevel[2] );
  fprintf( filewrite, "EXP for 4th    : %d\n", pcap->experienceforlevel[3] );
  fprintf( filewrite, "EXP for 5th    : %d\n", pcap->experienceforlevel[4] );
  fprintf( filewrite, "EXP for 6th    : %d\n", pcap->experienceforlevel[5] );
  fprintf( filewrite, "Starting EXP   : %d\n", pcap->experience );
  fprintf( filewrite, "EXP worth      : %d\n", pcap->experienceworth );
  fprintf( filewrite, "EXP exchange   : %5.3f\n", pcap->experienceexchange );
  fprintf( filewrite, "EXPSECRET      : %4.2f\n", pcap->experiencerate[0] );
  fprintf( filewrite, "EXPQUEST       : %4.2f\n", pcap->experiencerate[1] );
  fprintf( filewrite, "EXPDARE        : %4.2f\n", pcap->experiencerate[2] );
  fprintf( filewrite, "EXPKILL        : %4.2f\n", pcap->experiencerate[3] );
  fprintf( filewrite, "EXPMURDER      : %4.2f\n", pcap->experiencerate[4] );
  fprintf( filewrite, "EXPREVENGE     : %4.2f\n", pcap->experiencerate[5] );
  fprintf( filewrite, "EXPTEAMWORK    : %4.2f\n", pcap->experiencerate[6] );
  fprintf( filewrite, "EXPROLEPLAY    : %4.2f\n", pcap->experiencerate[7] );
  fprintf( filewrite, "\n" );

  // IDSZ identification tags
  fprintf( filewrite, "IDSZ Parent    : [%s]\n", undo_idsz( pcap->idsz[0] ) );
  fprintf( filewrite, "IDSZ Type      : [%s]\n", undo_idsz( pcap->idsz[1] ) );
  fprintf( filewrite, "IDSZ Skill     : [%s]\n", undo_idsz( pcap->idsz[2] ) );
  fprintf( filewrite, "IDSZ Special   : [%s]\n", undo_idsz( pcap->idsz[3] ) );
  fprintf( filewrite, "IDSZ Hate      : [%s]\n", undo_idsz( pcap->idsz[4] ) );
  fprintf( filewrite, "IDSZ Vulnie    : [%s]\n", undo_idsz( pcap->idsz[5] ) );
  fprintf( filewrite, "\n" );

  // Item and damage flags
  ftruthf( filewrite, "Is an item     : ", pcap->prop.isitem );
  ftruthf( filewrite, "Is a mount     : ", pcap->prop.ismount );
  ftruthf( filewrite, "Is stackable   : ", pcap->prop.isstackable );
  ftruthf( filewrite, "Name known     : ", pcap->prop.nameknown );
  ftruthf( filewrite, "Usage known    : ", pcap->prop.usageknown );
  ftruthf( filewrite, "Is exportable  : ", pcap->cancarrytonextmodule );
  ftruthf( filewrite, "Requires skill : ", pcap->needskillidtouse );
  ftruthf( filewrite, "Is platform    : ", pcap->prop.isplatform );
  ftruthf( filewrite, "Collects money : ", pcap->prop.cangrabmoney );
  ftruthf( filewrite, "Can open stuff : ", pcap->prop.canopenstuff );
  fprintf( filewrite, "\n" );

  // Other item and damage stuff
  fdamagf( filewrite, "Damage type    : ", pcap->damagetargettype );
  factiof( filewrite, "Attack type    : ", pcap->weaponaction );
  fprintf( filewrite, "\n" );

  // Particle attachments
  fprintf( filewrite, "Attached parts : %d\n", pcap->attachedprtamount );
  fdamagf( filewrite, "Reaffirm type  : ", pcap->attachedprtreaffirmdamagetype );
  fprintf( filewrite, "Particle type  : %d\n", pcap->attachedprttype );
  fprintf( filewrite, "\n" );

  // Character hands
  ftruthf( filewrite, "Left valid     : ", pcap->slotvalid[SLOT_LEFT] );
  ftruthf( filewrite, "Right valid    : ", pcap->slotvalid[SLOT_RIGHT] );
  fprintf( filewrite, "\n" );

  // Particle spawning on attack
  ftruthf( filewrite, "Part on weapon : ", pcap->attackattached );
  fprintf( filewrite, "Part type      : %d\n", pcap->attackprttype );
  fprintf( filewrite, "\n" );

  // Particle spawning for GoPoof
  fprintf( filewrite, "Poof amount    : %d\n", pcap->gopoofprtamount );
  fprintf( filewrite, "Facing add     : %d\n", pcap->gopoofprtfacingadd );
  fprintf( filewrite, "Part type      : %d\n", pcap->gopoofprttype );
  fprintf( filewrite, "\n" );

  // Particle spawning for blud
  ftruthf( filewrite, "Blud valid    : ", BLUD_NONE != pcap->bludlevel );
  fprintf( filewrite, "Part type      : %d\n", pcap->bludprttype );
  fprintf( filewrite, "\n" );

  // Extra stuff
  ftruthf( filewrite, "Waterwalking   : ", pcap->prop.waterwalk );
  fprintf( filewrite, "Bounce dampen  : %5.3f\n", pcap->dampen );
  fprintf( filewrite, "\n" );

  // More stuff
  fprintf( filewrite, "Life healing   : %5.3f\n", FP8_TO_FLOAT( pcap->statdata.lifeheal_fp8 ) );
  fprintf( filewrite, "Mana cost      : %5.3f\n", FP8_TO_FLOAT( pcap->manacost_fp8 ) );
  fprintf( filewrite, "Life return    : %d\n", pcap->statdata.lifereturn_fp8 );
  fprintf( filewrite, "Stopped by     : %d\n", pcap->stoppedby );

  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
  {
    STRING stmp;
    snprintf( stmp, sizeof( stmp ), "Skin %d name    : ", iskin );
    funderf( filewrite, stmp, pcap->skin[iskin].name );
  };

  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
  {
    fprintf( filewrite, "Skin %d cost    : %d\n", iskin, pcap->skin[iskin].cost );
  };

  fprintf( filewrite, "STR dampen     : %5.3f\n", pcap->strengthdampen );
  fprintf( filewrite, "\n" );

  // Another memory lapse
  ftruthf( filewrite, "No rider attak : ", !pcap->prop.ridercanattack );
  ftruthf( filewrite, "Can be dazed   : ", pcap->prop.canbedazed );
  ftruthf( filewrite, "Can be grogged : ", pcap->prop.canbegrogged );
  fprintf( filewrite, "NOT USED       : 0\n" );
  fprintf( filewrite, "NOT USED       : 0\n" );
  ftruthf( filewrite, "Can see invisi : ", pcap->prop.canseeinvisible );
  fprintf( filewrite, "Kursed chance  : %d\n", pchr->prop.iskursed*100 );
  fprintf( filewrite, "Footfall sound : %d\n", pcap->footfallsound );
  fprintf( filewrite, "Jump sound     : %d\n", pcap->jumpsound );
  fprintf( filewrite, "\n" );

  // Expansions
  fprintf( filewrite, ":[GOLD] %d\n", pcap->money );

  if ( pcap->skindressy&1 ) fprintf( filewrite, ":[DRES] 0\n" );
  if ( pcap->skindressy&2 ) fprintf( filewrite, ":[DRES] 1\n" );
  if ( pcap->skindressy&4 ) fprintf( filewrite, ":[DRES] 2\n" );
  if ( pcap->skindressy&8 ) fprintf( filewrite, ":[DRES] 3\n" );
  if ( pcap->prop.resistbumpspawn ) fprintf( filewrite, ":[STUK] 0\n" );
  if ( pcap->prop.istoobig ) fprintf( filewrite, ":[PACK] 0\n" );
  if ( !pcap->prop.reflect ) fprintf( filewrite, ":[VAMP] 1\n" );
  if ( pcap->prop.alwaysdraw ) fprintf( filewrite, ":[DRAW] 1\n" );
  if ( pcap->prop.isranged ) fprintf( filewrite, ":[RANG] 1\n" );
  if ( pcap->hidestate != NOHIDE ) fprintf( filewrite, ":[HIDE] %d\n", pcap->hidestate );
  if ( pcap->prop.isequipment ) fprintf( filewrite, ":[EQUI] 1\n" );
  if ( pcap->bumpsizebig >= pcap->bumpsize ) fprintf( filewrite, ":[SQUA] 1\n" );
  if ( pcap->prop.icon != pcap->prop.usageknown ) fprintf( filewrite, ":[ICON] %d\n", pcap->prop.icon );
  if ( pcap->prop.forceshadow ) fprintf( filewrite, ":[SHAD] 1\n" );

  //Skill expansions
  if ( pcap->prop.canseekurse )  fprintf( filewrite, ":[CKUR] 1\n" );
  if ( pcap->prop.canusearcane ) fprintf( filewrite, ":[WMAG] 1\n" );
  if ( pcap->prop.canjoust )     fprintf( filewrite, ":[JOUS] 1\n" );
  if ( pcap->prop.canusedivine ) fprintf( filewrite, ":[HMAG] 1\n" );
  if ( pcap->prop.candisarm )    fprintf( filewrite, ":[DISA] 1\n" );
  if ( pcap->prop.canusetech )   fprintf( filewrite, ":[TECH] 1\n" );
  if ( pcap->prop.canbackstab )  fprintf( filewrite, ":[STAB] 1\n" );
  if ( pcap->prop.canuseadvancedweapons ) fprintf( filewrite, ":[AWEP] 1\n" );
  if ( pcap->prop.canusepoison ) fprintf( filewrite, ":[POIS] 1\n" );
  if ( pcap->prop.canread )  fprintf( filewrite, ":[READ] 1\n" );

  //General exported ichr information
  fprintf( filewrite, ":[PLAT] %d\n", pcap->prop.canuseplatforms );
  fprintf( filewrite, ":[SKIN] %d\n", pchr->skin_ref % MAXSKIN );
  fprintf( filewrite, ":[CONT] %d\n", pchr->aistate.content );
  fprintf( filewrite, ":[STAT] %d\n", pchr->aistate.state );
  fprintf( filewrite, ":[LEVL] %d\n", pchr->experiencelevel );
  fs_fileClose( filewrite );

}

//--------------------------------------------------------------------------------------------
void export_one_character_skin( Game_t * gs, char *szSaveName, CHR_REF ichr )
{
  // ZZ> This function creates a "SKIN.TXT" file for the given character.

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  FILE* filewrite;
  OBJ_REF profile;

  // General stuff
  profile = chrlst[ichr].model;

  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( NULL != filewrite )
  {
    fprintf( filewrite, "This file is used only by the import menu\n" );
    fprintf( filewrite, ": %d\n", chrlst[ichr].skin_ref % MAXSKIN );
    fs_fileClose( filewrite );
  }
}

//--------------------------------------------------------------------------------------------
void calc_cap_experience( Game_t * gs, CAP_REF icap )
{
  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  float statdebt, statperlevel;

  statdebt  = caplst[icap].statdata.life_pair.ibase + caplst[icap].statdata.mana_pair.ibase + caplst[icap].statdata.manareturn_pair.ibase + caplst[icap].statdata.manaflow_pair.ibase;
  statdebt += caplst[icap].statdata.strength_pair.ibase + caplst[icap].statdata.wisdom_pair.ibase + caplst[icap].statdata.intelligence_pair.ibase + caplst[icap].statdata.dexterity_pair.ibase;
  statdebt += ( caplst[icap].statdata.life_pair.irand + caplst[icap].statdata.mana_pair.irand + caplst[icap].statdata.manareturn_pair.irand + caplst[icap].statdata.manaflow_pair.irand ) * 0.5f;
  statdebt += ( caplst[icap].statdata.strength_pair.irand + caplst[icap].statdata.wisdom_pair.irand + caplst[icap].statdata.intelligence_pair.irand + caplst[icap].statdata.dexterity_pair.irand ) * 0.5f;

  statperlevel  = caplst[icap].statdata.lifeperlevel_pair.ibase + caplst[icap].statdata.manaperlevel_pair.ibase + caplst[icap].statdata.manareturnperlevel_pair.ibase + caplst[icap].statdata.manaflowperlevel_pair.ibase;
  statperlevel += caplst[icap].statdata.strengthperlevel_pair.ibase + caplst[icap].statdata.wisdomperlevel_pair.ibase + caplst[icap].statdata.intelligenceperlevel_pair.ibase + caplst[icap].statdata.dexterityperlevel_pair.ibase;
  statperlevel += ( caplst[icap].statdata.lifeperlevel_pair.irand + caplst[icap].statdata.manaperlevel_pair.irand + caplst[icap].statdata.manareturnperlevel_pair.irand + caplst[icap].statdata.manaflowperlevel_pair.irand ) * 0.5f;
  statperlevel += ( caplst[icap].statdata.strengthperlevel_pair.irand + caplst[icap].statdata.wisdomperlevel_pair.irand + caplst[icap].statdata.intelligenceperlevel_pair.irand + caplst[icap].statdata.dexterityperlevel_pair.irand ) * 0.5f;

  caplst[icap].experienceconst = 50.6f * ( FP8_TO_FLOAT( statdebt ) - 51.5 );
  caplst[icap].experiencecoeff = 26.3f * MAX( 1, FP8_TO_FLOAT( statperlevel ) );
};

//--------------------------------------------------------------------------------------------
int calc_chr_experience( Game_t * gs, CHR_REF ichr, float level )
{
  Cap_t * pcap = ChrList_getPCap(gs, ichr);
  if( NULL == pcap ) return 0;

  return level*level*pcap->experiencecoeff + pcap->experienceconst + 1;
};

//--------------------------------------------------------------------------------------------
float calc_chr_level( Game_t * gs, CHR_REF ichr )
{
  Chr_t * pchr;
  Cap_t  * pcap;

  float  level;

  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return 0.0f;

  pcap = ChrList_getPCap(gs, ichr);
  if(NULL == pcap) return 0.0f;

  level = ( pchr->experience - pcap->experienceconst ) / pcap->experiencecoeff;
  if ( level <= 0.0f )
  {
    level = 0.0f;
  }
  else
  {
    level = sqrt( level );
  }

  return level;
};

//--------------------------------------------------------------------------------------------
OBJ_REF object_generate_index( Game_t * gs, char *szLoadName )
{
  // ZZ > This reads the object slot in "DATA.TXT" that the profile
  //      is assigned to.  Errors in this number may cause the program to abort

  FILE* fileread;
  OBJ_REF retval = INVALID_OBJ;
  int iobj;

  // Open the file
  fileread = fs_fileOpen( PRI_NONE, "object_generate_index()", szLoadName, "r" );
  if ( NULL == fileread  )
  {
    return retval;
  }

  globalname = szLoadName;

  // Read in the iobj slot
  iobj = fget_next_int( fileread );
  if ( iobj < 0 )
  {
    retval = gs->modstate.import.object;
    if ( INVALID_OBJ == retval )
    {
      log_error( "Object slot number %i is invalid. (%s) \n", iobj, szLoadName );
    }
  }
  else
  {
    retval = OBJ_REF(iobj);
  }

  fs_fileClose(fileread);

  return retval;
}

//--------------------------------------------------------------------------------------------
CAP_REF CapList_load_one( Game_t * gs, const char * szObjectpath, const char *szObjectname, CAP_REF irequest )
{
  // ZZ> This function fills a character profile with data from "DATA.TXT"

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  FILE* fileread;
  int iskin, cnt;
  int iTmp;
  char cTmp;
  int damagetype, level, xptype;
  IDSZ idsz;
  Cap_t * pcap;
  STRING szLoadname;

  if( !VALID_CAP_RANGE(irequest) ) return INVALID_CAP;

  // Open the file
  fileread = fs_fileOpen( PRI_NONE, "CapList_load_one()", inherit_fname(szObjectpath, szObjectname, CData.data_file), "r");
  if ( NULL == fileread  )
  {
    // The data file wasn't found
    log_error( "Data.txt could not be correctly read! (%s) \n", szLoadname );
    return INVALID_CAP;
  }

  // skip over the slot information
  fget_next_int( fileread );

  // Make sure we don't load over an existing model
  if( LOADED_CAP(gs->CapList, irequest ))
  {
    log_error( "Character Template (cap) slot %i is already used. (%s%s)\n", irequest, szObjectpath, szObjectname );
  }

  // make the notation "easier"
  pcap = gs->CapList + irequest;

  // initialize the template
  Cap_new(pcap);

  // Read in the real general data
  fget_next_name( fileread, pcap->classname, sizeof( pcap->classname ) );

  // Light cheat
  pcap->uniformlit = fget_next_bool( fileread );

  // Ammo
  pcap->ammomax = fget_next_int( fileread );
  pcap->ammo = fget_next_int( fileread );

  // Gender
  pcap->gender = fget_next_gender( fileread );

  // Read in the irequest stats
  pcap->lifecolor = fget_next_int( fileread );
  pcap->manacolor = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &pcap->statdata.life_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.lifeperlevel_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.mana_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.manaperlevel_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.manareturn_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.manareturnperlevel_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.manaflow_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.manaflowperlevel_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.strength_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.strengthperlevel_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.wisdom_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.wisdomperlevel_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.intelligence_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.intelligenceperlevel_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.dexterity_pair );
  fget_next_pair_fp8( fileread, &pcap->statdata.dexterityperlevel_pair );

  // More physical attributes
  pcap->size = fget_next_float( fileread );
  pcap->sizeperlevel = fget_next_float( fileread );
  pcap->shadowsize = fget_next_int( fileread );
  pcap->bumpsize = fget_next_int( fileread );
  pcap->bumpheight = fget_next_int( fileread );
  pcap->bumpdampen = fget_next_float( fileread );
  pcap->weight = fget_next_int( fileread );
  if ( pcap->weight == 255.0f ) pcap->weight = -1.0f;
  if ( pcap->weight ==   0.0f ) pcap->weight = 1.0f;

  pcap->bumpstrength = ( pcap->bumpsize > 0.0f ) ? 1.0f : 0.0f;
  if ( pcap->bumpsize   == 0.0f ) pcap->bumpsize   = 1.0f;
  if ( pcap->bumpheight == 0.0f ) pcap->bumpheight = 1.0f;
  if ( pcap->weight     == 0.0f ) pcap->weight     = 1.0f;

  pcap->jump = fget_next_float( fileread );
  pcap->jumpnumber = fget_next_int( fileread );
  pcap->spd_sneak = fget_next_int( fileread );
  pcap->spd_walk = fget_next_int( fileread );
  pcap->spd_run = fget_next_int( fileread );
  pcap->flyheight = fget_next_int( fileread );
  pcap->flashand = fget_next_int( fileread );
  pcap->alpha_fp8 = fget_next_int( fileread );
  pcap->light_fp8 = fget_next_int( fileread );
  if ( pcap->light_fp8 < 0xff )
  {
    pcap->alpha_fp8 = MIN( pcap->alpha_fp8, 0xff - pcap->light_fp8 );
  };

  pcap->transferblend = fget_next_bool( fileread );
  pcap->sheen_fp8 = fget_next_int( fileread );
  pcap->enviro = fget_next_bool( fileread );
  pcap->uoffvel = fget_next_float( fileread ) * (float)UINT16_MAX;
  pcap->voffvel = fget_next_float( fileread ) * (float)UINT16_MAX;
  pcap->prop.stickybutt = fget_next_bool( fileread );

  // Invulnerability data
  pcap->prop.invictus = fget_next_bool( fileread );
  pcap->nframefacing = fget_next_int( fileread );
  pcap->nframeangle = fget_next_int( fileread );
  pcap->iframefacing = fget_next_int( fileread );
  pcap->iframeangle = fget_next_int( fileread );
  // Resist burning and stuck arrows with nframe angle of 1 or more
  if ( pcap->nframeangle > 0 )
  {
    if ( pcap->nframeangle == 1 )
    {
      pcap->nframeangle = 0;
    }
  }

  // Skin defenses ( 4 skins )
  fgoto_colon( fileread );
  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    { pcap->skin[iskin].defense_fp8 = 255 - fget_int( fileread ); };

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fgoto_colon( fileread );
    for ( iskin = 0; iskin < MAXSKIN; iskin++ )
      { pcap->skin[iskin].damagemodifier_fp8[damagetype] = fget_int( fileread ); };
  }

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fgoto_colon( fileread );
    for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    {
      cTmp = fget_first_letter( fileread );
      switch ( toupper( cTmp ) )
      {
        case 'T': pcap->skin[iskin].damagemodifier_fp8[damagetype] |= DAMAGE_INVERT; break;
        case 'C': pcap->skin[iskin].damagemodifier_fp8[damagetype] |= DAMAGE_CHARGE; break;
        case 'M': pcap->skin[iskin].damagemodifier_fp8[damagetype] |= DAMAGE_MANA;   break;
      };
    }
  }

  fgoto_colon( fileread );
  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    { pcap->skin[iskin].maxaccel = fget_float( fileread ) / 80.0; };

  // Experience and level data
  pcap->experienceforlevel[0] = 0;
  for ( level = 1; level < BASELEVELS; level++ )
    { pcap->experienceforlevel[level] = fget_next_int( fileread ); }

  fget_next_pair( fileread, &pcap->experience );
  pcap->experienceworth = fget_next_int( fileread );
  pcap->experienceexchange = fget_next_float( fileread );

  for ( xptype = 0; xptype < XP_COUNT; xptype++ )
    { pcap->experiencerate[xptype] = fget_next_float( fileread ) + 0.001f; }

  // IDSZ tags
  for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
    { pcap->idsz[cnt] = fget_next_idsz( fileread ); }

  // Item and damage flags
  pcap->prop.isitem = fget_next_bool( fileread );
  pcap->prop.ismount = fget_next_bool( fileread );
  pcap->prop.isstackable = fget_next_bool( fileread );
  pcap->prop.nameknown = fget_next_bool( fileread );
  pcap->prop.usageknown = fget_next_bool( fileread );
  pcap->cancarrytonextmodule = fget_next_bool( fileread );
  pcap->needskillidtouse = fget_next_bool( fileread );
  pcap->prop.isplatform = fget_next_bool( fileread );
  pcap->prop.cangrabmoney = fget_next_bool( fileread );
  pcap->prop.canopenstuff = fget_next_bool( fileread );

  // More item and damage stuff
  pcap->damagetargettype = fget_next_damage( fileread );
  pcap->weaponaction = fget_next_action( fileread );

  // Particle attachments
  pcap->attachedprtamount = fget_next_int( fileread );
  pcap->attachedprtreaffirmdamagetype = fget_next_damage( fileread );
  pcap->attachedprttype = fget_next_int( fileread );

  // Character hands
  pcap->slotvalid[SLOT_LEFT] = fget_next_bool( fileread );
  pcap->slotvalid[SLOT_RIGHT] = fget_next_bool( fileread );
  if ( pcap->prop.ismount )
  {
    pcap->slotvalid[SLOT_SADDLE] = pcap->slotvalid[SLOT_LEFT];
    pcap->slotvalid[SLOT_LEFT]   = bfalse;
    //pcap->slotvalid[SLOT_RIGHT]  = bfalse;
  };

  // Attack order ( weapon )
  pcap->attackattached = fget_next_bool( fileread );
  iTmp = fget_next_int( fileread );
  pcap->attackprttype = (iTmp >= 0) ? PIP_REF(iTmp) : INVALID_PIP;

  // GoPoof
  pcap->gopoofprtamount = fget_next_int( fileread );
  pcap->gopoofprtfacingadd = fget_next_int( fileread );
  pcap->gopoofprttype = fget_next_int( fileread );

  // Blud
  pcap->bludlevel = fget_next_blud( fileread );
  pcap->bludprttype = fget_next_int( fileread );

  // Stuff I forgot
  pcap->prop.waterwalk = fget_next_bool( fileread );
  pcap->dampen = fget_next_float( fileread );

  // More stuff I forgot
  pcap->statdata.lifeheal_fp8 = fget_next_fixed( fileread );
  pcap->manacost_fp8 = fget_next_fixed( fileread );
  pcap->statdata.lifereturn_fp8 = fget_next_int( fileread );
  pcap->stoppedby = fget_next_int( fileread ) | MPDFX_IMPASS;

  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    { fget_next_name( fileread, pcap->skin[iskin].name, sizeof( pcap->skin[iskin].name ) ); };

  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    { pcap->skin[iskin].cost = fget_next_int( fileread ); };

  pcap->strengthdampen = fget_next_float( fileread );

  // Another memory lapse
  pcap->prop.ridercanattack = !fget_next_bool( fileread );
  pcap->prop.canbedazed = fget_next_bool( fileread );
  pcap->prop.canbegrogged = fget_next_bool( fileread );
  fget_next_int( fileread );   // !!!BAD!!! Life add
  fget_next_int( fileread );   // !!!BAD!!! Mana add
  pcap->prop.canseeinvisible = fget_next_bool( fileread );
  pcap->kursechance = fget_next_int( fileread );

  iTmp = fget_next_int( fileread ); pcap->footfallsound = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); pcap->jumpsound     = FIX_SOUND( iTmp );

  // Set the default properties
  CProperties_init( &(pcap->prop) );

  // Clear expansions...
  pcap->bumpsizebig = pcap->bumpsize * SQRT_TWO;

  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );
    if ( MAKE_IDSZ( "GOLD" ) == idsz )  pcap->money = iTmp;
    else if ( MAKE_IDSZ( "STUK" ) == idsz )  pcap->prop.resistbumpspawn = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "PACK" ) == idsz )  pcap->prop.istoobig = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "VAMP" ) == idsz )  pcap->prop.reflect = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DRAW" ) == idsz )  pcap->prop.alwaysdraw = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RANG" ) == idsz )  pcap->prop.isranged = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "HIDE" ) == idsz )  pcap->hidestate = iTmp;
    else if ( MAKE_IDSZ( "EQUI" ) == idsz )  pcap->prop.isequipment = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SQUA" ) == idsz )  pcap->bumpsizebig = pcap->bumpsize * 2.0f;
    else if ( MAKE_IDSZ( "ICON" ) == idsz )  pcap->prop.icon = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SHAD" ) == idsz )  pcap->prop.forceshadow = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SKIN" ) == idsz )  pcap->skinoverride = iTmp % MAXSKIN;
    else if ( MAKE_IDSZ( "CONT" ) == idsz )  pcap->contentoverride = iTmp;
    else if ( MAKE_IDSZ( "STAT" ) == idsz )  pcap->stateoverride = iTmp;
    else if ( MAKE_IDSZ( "LEVL" ) == idsz )  pcap->leveloverride = iTmp;
    else if ( MAKE_IDSZ( "PLAT" ) == idsz )  pcap->prop.canuseplatforms = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RIPP" ) == idsz )  pcap->prop.ripple = INT_TO_BOOL( iTmp );

    //Skill Expansions
    // [CKUR] Can it see kurses?
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  pcap->prop.canseekurse  = INT_TO_BOOL( iTmp );
    // [WMAG] Can the character use arcane spellbooks?
    else if ( MAKE_IDSZ( "WMAG" ) == idsz )  pcap->prop.canusearcane = INT_TO_BOOL( iTmp );
    // [JOUS] Can the character joust with a lance?
    else if ( MAKE_IDSZ( "JOUS" ) == idsz )  pcap->prop.canjoust     = INT_TO_BOOL( iTmp );
    // [HMAG] Can the character use divine spells?
    else if ( MAKE_IDSZ( "HMAG" ) == idsz )  pcap->prop.canusedivine = INT_TO_BOOL( iTmp );
    // [TECH] Able to use items technological items?
    else if ( MAKE_IDSZ( "TECH" ) == idsz )  pcap->prop.canusetech   = INT_TO_BOOL( iTmp );
    // [DISA] Find and disarm traps?
    else if ( MAKE_IDSZ( "DISA" ) == idsz )  pcap->prop.candisarm    = INT_TO_BOOL( iTmp );
    // [STAB] Backstab and murder?
    else if ( idsz == MAKE_IDSZ( "STAB" ) )  pcap->prop.canbackstab  = INT_TO_BOOL( iTmp );
    // [AWEP] Profiency with advanced weapons?
    else if ( idsz == MAKE_IDSZ( "AWEP" ) )  pcap->prop.canuseadvancedweapons = INT_TO_BOOL( iTmp );
    // [POIS] Use poison without err?
    else if ( idsz == MAKE_IDSZ( "POIS" ) )  pcap->prop.canusepoison = INT_TO_BOOL( iTmp );
  }

  fs_fileClose( fileread );

  calc_cap_experience( gs, irequest );

  // tell everyone that we loaded correctly
  pcap->Loaded = btrue;

  return irequest;
}

//--------------------------------------------------------------------------------------------
int fget_skin( char * szObjectpath, const char * szObjectname )
{
  // ZZ> This function reads the "SKIN.TXT" file...

  FILE* fileread;
  Uint8 skin;

  skin = 0;
  fileread = fs_fileOpen( PRI_NONE, "fget_skin()", inherit_fname(szObjectpath, szObjectname, CData.skin_file), "r" );
  if ( NULL != fileread )
  {
    skin = fget_next_int( fileread );
    skin %= MAXSKIN;
    fs_fileClose( fileread );
  }
  return skin;
}

//--------------------------------------------------------------------------------------------
void check_player_import(Game_t * gs)
{
  // ZZ> This function figures out which players may be imported, and loads basic
  //     data for each

  STRING searchname, filename, filepath;
  int skin;
  bool_t keeplooking;
  const char *foundfile;
  FS_FIND_INFO fs_finfo;

  Obj_t otmp;

  OBJ_REF iobj;
  Obj_t  * pobj;
  PObj_t objlst = gs->ObjList;

  LOAD_PLAYER_INFO * ploadplayer;

  // Set up...
  loadplayer_count = 0;

  // Search for all objects
  fs_find_info_new( &fs_finfo );
  snprintf( searchname, sizeof( searchname ), "%s" SLASH_STRING "*.obj", CData.players_dir );
  foundfile = fs_findFirstFile( &fs_finfo, CData.players_dir, NULL, "*.obj" );
  keeplooking = 1;
  if ( NULL != foundfile  )
  {
    snprintf( filepath, sizeof( filename ), "%s" SLASH_STRING "%s" SLASH_STRING, CData.players_dir, foundfile );

    while (loadplayer_count < MAXLOADPLAYER )
    {
      // grab the requested profile
      {
        iobj = ObjList_get_free(gs, OBJ_REF(loadplayer_count));
        pobj = ObjList_getPObj(gs, iobj);

        // Make up a name for the profile...  IMPORT\TEMP0000.OBJ
        strncpy( pobj->name, filepath, sizeof( pobj->name ) );
      }
      if(NULL == pobj)
      {
        assert(bfalse);
        break;
      }

      // get the loadplayer data
      ploadplayer = loadplayer + loadplayer_count;

      naming_prime( gs );

      strncpy( ploadplayer->dir, foundfile, sizeof( ploadplayer->dir ) );

      skin = fget_skin( filename, NULL );

      // Load the AI script for this object
      pobj->ai = load_ai_script( Game_getScriptInfo(gs), filepath, NULL );
      if ( AILST_COUNT == pobj->ai )
      {
        // use the default script
        pobj->ai = 0;
      }

      pobj->mad = MadList_load_one( gs, filepath, NULL, MAD_REF(loadplayer_count) );

      snprintf( filename, sizeof( filename ), "icon%d.bmp", skin );
      load_one_icon( filepath, NULL, filename);

      naming_read( gs, filename, NULL, &otmp);
      strncpy( ploadplayer->name, naming_generate( gs, &otmp ), sizeof( ploadplayer->name ) );

      loadplayer_count++;

      foundfile = fs_findNextFile(&fs_finfo);
      if (NULL == foundfile) break;
    }
  }
  fs_findClose(&fs_finfo);
}

//--------------------------------------------------------------------------------------------
bool_t check_skills( Game_t * gs, CHR_REF who, Uint32 whichskill )
{
  // ZF> This checks if the specified character has the required skill. Returns btrue if true
  // and bfalse if not. Also checks Skill expansions.

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  bool_t result = bfalse;

  // First check the character Skill ID matches
  // Then check for expansion skills too.
  if ( ChrList_getPCap(gs, who)->idsz[IDSZ_SKILL] == whichskill ) result = btrue;
  else if ( MAKE_IDSZ( "CKUR" ) == whichskill ) result = chrlst[who].prop.canseekurse;
  else if ( MAKE_IDSZ( "WMAG" ) == whichskill ) result = chrlst[who].prop.canusearcane;
  else if ( MAKE_IDSZ( "JOUS" ) == whichskill ) result = chrlst[who].prop.canjoust;
  else if ( MAKE_IDSZ( "HMAG" ) == whichskill ) result = chrlst[who].prop.canusedivine;
  else if ( MAKE_IDSZ( "DISA" ) == whichskill ) result = chrlst[who].prop.candisarm;
  else if ( MAKE_IDSZ( "TECH" ) == whichskill ) result = chrlst[who].prop.canusetech;
  else if ( MAKE_IDSZ( "AWEP" ) == whichskill ) result = chrlst[who].prop.canuseadvancedweapons;
  else if ( MAKE_IDSZ( "STAB" ) == whichskill ) result = chrlst[who].prop.canbackstab;
  else if ( MAKE_IDSZ( "POIS" ) == whichskill ) result = chrlst[who].prop.canusepoison;
  else if ( MAKE_IDSZ( "READ" ) == whichskill ) result = chrlst[who].prop.canread;

  return result;
}

//--------------------------------------------------------------------------------------------
int check_player_quest( char *whichplayer, IDSZ idsz )
{
  // ZF> This function checks if the specified player has the IDSZ in his or her quest.txt
  // and returns the quest level of that specific quest (Or NOQUEST if it is not found, QUESTBEATEN if it is finished)

  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz = bfalse;
  Sint8 result = NOQUEST;
  Sint8 iTmp;

  //Always return "true" for [NONE] IDSZ checks
  if (idsz == IDSZ_NONE) result = -1;

  snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.players_dir, whichplayer, CData.quest_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return result;

  // Check each expansion
  while ( fgoto_colon_yesno( fileread ) && !foundidsz )
  {
    newidsz = fget_idsz( fileread );
    if ( newidsz == idsz )
    {
      foundidsz = btrue;
      iTmp = fget_int( fileread );  //Read value behind colon and IDSZ
      result = iTmp;
    }
  }

  fs_fileClose( fileread );

  return result;
}

//--------------------------------------------------------------------------------------------
bool_t add_quest_idsz( char *whichplayer, IDSZ idsz )
{
  // ZF> This function writes a IDSZ (With quest level 0) into a player quest.txt file, returns btrue if succeeded

  FILE *filewrite;
  STRING newloadname;

  // Only add quest IDSZ if it doesnt have it already
  if (check_player_quest(whichplayer, idsz) >= QUESTBEATEN) return bfalse;

  // Try to open the file in read and append mode
  snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.players_dir, whichplayer, CData.quest_file );
  filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "a" );
  if ( NULL == filewrite ) return bfalse;

  fprintf( filewrite, "\n:[%4s]: 0", undo_idsz( idsz ) );
  fs_fileClose( filewrite );

  return btrue;
}

//--------------------------------------------------------------------------------------------
//TODO: This should be moved to file_common.c
bool_t fcopy_line(FILE * fileread, FILE * filewrite)
{
  // BB > copy a line of arbitrary length, in chunks of length
  //      sizeof(linebuffer)

  char linebuffer[64];

  if(NULL == fileread || NULL == filewrite) return bfalse;
  if( feof(fileread) || feof(filewrite) ) return bfalse;

  fgets(linebuffer, sizeof(linebuffer), fileread);
  fputs(linebuffer, filewrite);
  while( strlen(linebuffer) == sizeof(linebuffer) )
  {
    fgets(linebuffer, sizeof(linebuffer), fileread);
    fputs(linebuffer, filewrite);
  }

  return btrue;
};


//--------------------------------------------------------------------------------------------
int modify_quest_idsz( char *whichplayer, IDSZ idsz, int adjustment )
{
  // ZF> This function increases or decreases a Quest IDSZ quest level by the amount determined in
  //     adjustment. It then returns the current quest level it now has.
  //     It returns NOQUEST if failed and if the adjustment is 0, the quest is marked as beaten...

  FILE *filewrite, *fileread;
  STRING newloadname, copybuffer;
  bool_t foundidsz = bfalse;
  IDSZ newidsz;
  Sint8 NewQuestLevel = NOQUEST, QuestLevel;

  // Try to open the file in read/write mode
  snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.players_dir, whichplayer, CData.quest_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return NewQuestLevel;

  //Now check each expansion until we find correct IDSZ
  while ( fgoto_colon_yesno( fileread ) )
  {
    newidsz = fget_idsz( fileread );

    if ( newidsz == idsz )
    {
      foundidsz = btrue;
      QuestLevel = fget_int( fileread );
      if ( QuestLevel == QUESTBEATEN )		//Quest is already finished, do not modify
      {
        fs_fileClose( fileread );
        return NewQuestLevel;
      }

      // break out of the while loop
      break;
    }
  }

  if(foundidsz)
  {
    // modify the CData.quest_file

    char ctmp;

    //First close the file, rename it and reopen it for reading
    fs_fileClose( fileread );

    // create a "tmp_*" copy of the file
    snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.players_dir, whichplayer, CData.quest_file );
    snprintf( copybuffer, sizeof( copybuffer ), "%s" SLASH_STRING "%s" SLASH_STRING "tmp_%s", CData.players_dir, whichplayer, CData.quest_file);
    fs_copyFile( newloadname, copybuffer );

    // open the tmp file for reading and overwrite the original file
    fileread  = fs_fileOpen( PRI_NONE, NULL, copybuffer, "r" );
    filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "w" );

    // read the tmp file line-by line
    while( !feof(fileread) )
    {
      ctmp = fgetc(fileread);
      ungetc(ctmp, fileread);

      if( ctmp == '/' )
      {
        // copy comments exactly

        fcopy_line(fileread, filewrite);
      }
      else if( fgoto_colon_yesno( fileread ) )
      {
        // scan the line for quest info
        newidsz = fget_idsz( fileread );
        QuestLevel = fget_int( fileread );

        // modify it
        if ( newidsz == idsz )
        {
          if(adjustment == 0)
          {
            QuestLevel = -1;
          }
          else
          {
            QuestLevel += adjustment;
            if(QuestLevel < 0) QuestLevel = 0;
          }
          NewQuestLevel = QuestLevel;
        }

        // re-emit it
        fprintf(filewrite, "\n:[%s] %i", undo_idsz(idsz), QuestLevel);
      }
    }

    // get rid of the tmp file
    fs_fileClose( filewrite );
    fs_deleteFile( copybuffer );
  }

  fs_fileClose( fileread );

  return NewQuestLevel;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t chr_cvolume_reinit(Chr_t * pchr, CVolume_t * pcv)
{
  if(!EKEY_PVALID(pchr) || NULL == pcv) return bfalse;

  pcv->x_min = pchr->ori.pos.x - pchr->bmpdata.size * pchr->scale * pchr->pancakepos.x;
  pcv->y_min = pchr->ori.pos.y - pchr->bmpdata.size * pchr->scale * pchr->pancakepos.y;
  pcv->z_min = pchr->ori.pos.z;

  pcv->x_max = pchr->ori.pos.x + pchr->bmpdata.size   * pchr->scale * pchr->pancakepos.x;
  pcv->y_max = pchr->ori.pos.y + pchr->bmpdata.size   * pchr->scale * pchr->pancakepos.y;
  pcv->z_max = pchr->ori.pos.z + pchr->bmpdata.height * pchr->scale * pchr->pancakepos.z;

  pcv->xy_min = -(pchr->ori.pos.x + pchr->ori.pos.y) - pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pcv->xy_max = -(pchr->ori.pos.x + pchr->ori.pos.y) + pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;

  pcv->yx_min = -(-pchr->ori.pos.x + pchr->ori.pos.y) - pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pcv->yx_max = -(-pchr->ori.pos.x + pchr->ori.pos.y) + pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;

  pcv->lod = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bdata_reinit(Chr_t * pchr, BData_t * pbd)
{
  if(!EKEY_PVALID(pchr) || NULL == pbd) return bfalse;

  pbd->calc_is_platform   = pchr->prop.isplatform;
  pbd->calc_is_mount      = pchr->prop.ismount;

  pbd->mids_lo = pbd->mids_hi = pchr->ori.pos;
  pbd->mids_hi.z += pbd->height * 0.5f;

  pbd->calc_size     = pbd->size    * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pbd->calc_size_big = pbd->sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pbd->calc_height   = pbd->height  * pchr->scale *  pchr->pancakepos.z;

  chr_cvolume_reinit(pchr, &pbd->cv);

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers_0( Chr_t * pchr )
{
  int i;
  bool_t rv = bfalse;

  if( !EKEY_PVALID(pchr) ) return bfalse;

  // remove any level 3 info
  if(NULL != pchr->bmpdata.cv_tree)
  {
    for(i=0; i<8; i++)
    {
      pchr->bmpdata.cv_tree->leaf[i].lod = -1;
    }
  }

  rv = chr_bdata_reinit( pchr, &(pchr->bmpdata) );

  return btrue;
}

//--------------------------------------------------------------------------------------------
//bool_t chr_calculate_bumpers_1(Game_t * gs, CHR_REF chr_ref)
//{
//  BData_t * bd;
//  Uint16 imdl;
//  MD2_Model_t * pmdl;
//  const MD2_Frame_t * fl, * fc;
//  float lerp;
//  vect3 xdir, ydir, zdir;
//  vect3 points[8], bbmax, bbmin;
//  int cnt;
//
//  CVolume_t cv;
//
//  if( !ACTIVE_CHR(chrlst, chr_ref) ) return bfalse;
//  bd = &(chrlst[chr_ref].bmpdata);
//
//  imdl = chrlst[chr_ref].model;
//  if(!VALID_MDL(imdl) || !chrlst[chr_ref].matrix_valid )
//  {
//    set_default_bump_data( chr_ref );
//    return bfalse;
//  };
//
//  pmdl = madlst[imdl].md2_ptr;
//  if(NULL == pmdl)
//  {
//    set_default_bump_data( chr_ref );
//    return bfalse;
//  }
//
//  fl = md2_get_Frame(pmdl, chrlst[chr_ref].anim.last);
//  fc = md2_get_Frame(pmdl, chrlst[chr_ref].anim.next );
//  lerp = chrlst[chr_ref].anim.flip;
//
//  if(NULL ==fl && NULL ==fc)
//  {
//    set_default_bump_data( chr_ref );
//    return bfalse;
//  };
//
//  xdir.x = (chrlst[chr_ref].matrix).CNV(0,0);
//  xdir.y = (chrlst[chr_ref].matrix).CNV(0,1);
//  xdir.z = (chrlst[chr_ref].matrix).CNV(0,2);
//
//  ydir.x = (chrlst[chr_ref].matrix).CNV(1,0);
//  ydir.y = (chrlst[chr_ref].matrix).CNV(1,1);
//  ydir.z = (chrlst[chr_ref].matrix).CNV(1,2);
//
//  zdir.x = (chrlst[chr_ref].matrix).CNV(2,0);
//  zdir.y = (chrlst[chr_ref].matrix).CNV(2,1);
//  zdir.z = (chrlst[chr_ref].matrix).CNV(2,2);
//
//  if(NULL ==fl || lerp >= 1.0f)
//  {
//    bbmin.x = MIN(fc->bbmin[0], fc->bbmax[0]);
//    bbmin.y = MIN(fc->bbmin[1], fc->bbmax[1]);
//    bbmin.z = MIN(fc->bbmin[2], fc->bbmax[2]);
//
//    bbmax.x = MAX(fc->bbmin[0], fc->bbmax[0]);
//    bbmax.y = MAX(fc->bbmin[1], fc->bbmax[1]);
//    bbmax.z = MAX(fc->bbmin[2], fc->bbmax[2]);
//  }
//  else if(NULL ==fc || lerp <= 0.0f)
//  {
//    bbmin.x = MIN(fl->bbmin[0], fl->bbmax[0]);
//    bbmin.y = MIN(fl->bbmin[1], fl->bbmax[1]);
//    bbmin.z = MIN(fl->bbmin[2], fl->bbmax[2]);
//
//    bbmax.x = MAX(fl->bbmin[0], fl->bbmax[0]);
//    bbmax.y = MAX(fl->bbmin[1], fl->bbmax[1]);
//    bbmax.z = MAX(fl->bbmin[2], fl->bbmax[2]);
//  }
//  else
//  {
//    vect3 tmpmin, tmpmax;
//
//    tmpmin.x = fl->bbmin[0] + (fc->bbmin[0]-fl->bbmin[0])*lerp;
//    tmpmin.y = fl->bbmin[1] + (fc->bbmin[1]-fl->bbmin[1])*lerp;
//    tmpmin.z = fl->bbmin[2] + (fc->bbmin[2]-fl->bbmin[2])*lerp;
//
//    tmpmax.x = fl->bbmax[0] + (fc->bbmax[0]-fl->bbmax[0])*lerp;
//    tmpmax.y = fl->bbmax[1] + (fc->bbmax[1]-fl->bbmax[1])*lerp;
//    tmpmax.z = fl->bbmax[2] + (fc->bbmax[2]-fl->bbmax[2])*lerp;
//
//    bbmin.x = MIN(tmpmin.x, tmpmax.x);
//    bbmin.y = MIN(tmpmin.y, tmpmax.y);
//    bbmin.z = MIN(tmpmin.z, tmpmax.z);
//
//    bbmax.x = MAX(tmpmin.x, tmpmax.x);
//    bbmax.y = MAX(tmpmin.y, tmpmax.y);
//    bbmax.z = MAX(tmpmin.z, tmpmax.z);
//  };
//
//  cnt = 0;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmin.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmin.z;
//
//  cnt++;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmin.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmin.z;
//
//  cv.x_min  = cv.x_max  = points[0].x*xdir.x + points[0].y*ydir.x + points[0].z*zdir.x;
//  cv.y_min  = cv.y_max  = points[0].x*xdir.y + points[0].y*ydir.y + points[0].z*zdir.y;
//  cv.z_min  = cv.z_max  = points[0].x*xdir.z + points[0].y*ydir.z + points[0].z*zdir.z;
//  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
//  cv.yx_min = cv.yx_max = cv.x_min - cv.y_min;
//
//  for(cnt=1; cnt<8; cnt++)
//  {
//    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;
//
//    tmp_x = points[cnt].x*xdir.x + points[cnt].y*ydir.x + points[cnt].z*zdir.x;
//    if(tmp_x < cv.x_min) cv.x_min = tmp_x - 0.001f;
//    else if(tmp_x > cv.x_max) cv.x_max = tmp_x + 0.001f;
//
//    tmp_y = points[cnt].x*xdir.y + points[cnt].y*ydir.y + points[cnt].z*zdir.y;
//    if(tmp_y < cv.y_min) cv.y_min = tmp_y - 0.001f;
//    else if(tmp_y > cv.y_max) cv.y_max = tmp_y + 0.001f;
//
//    tmp_z = points[cnt].x*xdir.z + points[cnt].y*ydir.z + points[cnt].z*zdir.z;
//    if(tmp_z < cv.z_min) cv.z_min = tmp_z - 0.001f;
//    else if(tmp_z > cv.z_max) cv.z_max = tmp_z + 0.001f;
//
//    tmp_xy = tmp_x + tmp_y;
//    if(tmp_xy < cv.xy_min) cv.xy_min = tmp_xy - 0.001f;
//    else if(tmp_xy > cv.xy_max) cv.xy_max = tmp_xy + 0.001f;
//
//    tmp_yx = -tmp_x + tmp_y;
//    if(tmp_yx < cv.yx_min) cv.yx_min = tmp_yx - 0.001f;
//    else if(tmp_yx > cv.yx_max) cv.yx_max = tmp_yx + 0.001f;
//  };
//
//  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
//  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);
//
//  bd->cv.x_min  = cv.x_min  + chrlst[chr_ref].ori.pos.x;
//  bd->cv.y_min  = cv.y_min  + chrlst[chr_ref].ori.pos.y;
//  bd->cv.z_min  = cv.z_min  + chrlst[chr_ref].ori.pos.z;
//  bd->cv.xy_min = cv.xy_min + ( chrlst[chr_ref].ori.pos.x + chrlst[chr_ref].ori.pos.y);
//  bd->cv.yx_min = cv.yx_min + (-chrlst[chr_ref].ori.pos.x + chrlst[chr_ref].ori.pos.y);
//
//
//  bd->cv.x_max  = cv.x_max  + chrlst[chr_ref].ori.pos.x;
//  bd->cv.y_max  = cv.y_max  + chrlst[chr_ref].ori.pos.y;
//  bd->cv.z_max  = cv.z_max  + chrlst[chr_ref].ori.pos.z;
//  bd->cv.xy_max = cv.xy_max + ( chrlst[chr_ref].ori.pos.x + chrlst[chr_ref].ori.pos.y);
//  bd->cv.yx_max = cv.yx_max + (-chrlst[chr_ref].ori.pos.x + chrlst[chr_ref].ori.pos.y);
//
//  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + chrlst[chr_ref].ori.pos.x;
//  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + chrlst[chr_ref].ori.pos.y;
//  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + chrlst[chr_ref].ori.pos.z;
//
//  bd->mids_lo   = bd->mids_hi;
//  bd->mids_lo.z = cv.z_min  + chrlst[chr_ref].ori.pos.z;
//
//  bd->calc_height   = bd->cv.z_max;
//  bd->calc_size     = MAX( bd->cv.x_max, bd->cv.y_max ) - MIN( bd->cv.x_min, bd->cv.y_min );
//  bd->calc_size_big = 0.5f * ( MAX( bd->cv.xy_max, bd->cv.yx_max) - MIN( bd->cv.xy_min, bd->cv.yx_min) );
//
//  if(bd->calc_size_big < bd->calc_size*1.1)
//  {
//    bd->calc_size     *= -1;
//  }
//  else if (bd->calc_size*2 < bd->calc_size_big*1.1)
//  {
//    bd->calc_size_big *= -1;
//  }
//
//  return btrue;
//};
//
//

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers_1( Game_t * gs, Chr_t * pchr )
{
  BData_t * bd;
  MD2_Model_t * pmdl;
  const MD2_Frame_t * fl, * fc;
  float lerp;
  vect3 xdir, ydir, zdir, pos;
  vect3 points[8], bbmax, bbmin;
  int cnt;

  CVolume_t cv;

  PObj_t objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  OBJ_REF iobj;
  Obj_t  * pobj;

  Mad_t  * pmad;


  if(  !EKEY_PVALID(pchr) ) return bfalse;
  bd = &(pchr->bmpdata);

  iobj = pchr->model = VALIDATE_OBJ( objlst, pchr->model );
  pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  pmad = ObjList_getPMad(gs, iobj);
  if( NULL == pmad || !pchr->matrix_valid )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }


  pmdl = pmad->md2_ptr;
  if( NULL == pmdl )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }


  xdir.x = (pchr->matrix).CNV(0,0);
  xdir.y = (pchr->matrix).CNV(0,1);
  xdir.z = (pchr->matrix).CNV(0,2);

  ydir.x = (pchr->matrix).CNV(1,0);
  ydir.y = (pchr->matrix).CNV(1,1);
  ydir.z = (pchr->matrix).CNV(1,2);

  zdir.x = (pchr->matrix).CNV(2,0);
  zdir.y = (pchr->matrix).CNV(2,1);
  zdir.z = (pchr->matrix).CNV(2,2);

  pos.x = (pchr->matrix).CNV(3,0);
  pos.y = (pchr->matrix).CNV(3,1);
  pos.z = (pchr->matrix).CNV(3,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  fl = md2_get_Frame(pmdl, pchr->anim.last);
  fc = md2_get_Frame(pmdl, pchr->anim.next );
  lerp = pchr->anim.flip;

  if(NULL ==fl && NULL ==fc)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  };

  if(NULL ==fl || lerp >= 1.0f)
  {
    bbmin.x = fc->bbmin[0];
    bbmin.y = fc->bbmin[1];
    bbmin.z = fc->bbmin[2];

    bbmax.x = fc->bbmax[0];
    bbmax.y = fc->bbmax[1];
    bbmax.z = fc->bbmax[2];
  }
  else if(NULL ==fc || lerp <= 0.0f)
  {
    bbmin.x = fl->bbmin[0];
    bbmin.y = fl->bbmin[1];
    bbmin.z = fl->bbmin[2];

    bbmax.x = fl->bbmax[0];
    bbmax.y = fl->bbmax[1];
    bbmax.z = fl->bbmax[2];
  }
  else
  {
    bbmin.x = MIN(fl->bbmin[0], fc->bbmin[0]);
    bbmin.y = MIN(fl->bbmin[1], fc->bbmin[1]);
    bbmin.z = MIN(fl->bbmin[2], fc->bbmin[2]);

    bbmax.x = MAX(fl->bbmax[0], fc->bbmax[0]);
    bbmax.y = MAX(fl->bbmax[1], fc->bbmax[1]);
    bbmax.z = MAX(fl->bbmax[2], fc->bbmax[2]);
  };

  cnt = 0;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmin.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmin.z;

  cnt++;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmin.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmin.z;

  cv.x_min  = cv.x_max  = points[0].x*xdir.x + points[0].y*ydir.x + points[0].z*zdir.x + pos.x;
  cv.y_min  = cv.y_max  = points[0].x*xdir.y + points[0].y*ydir.y + points[0].z*zdir.y + pos.y;
  cv.z_min  = cv.z_max  = points[0].x*xdir.z + points[0].y*ydir.z + points[0].z*zdir.z + pos.z;
  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
  cv.yx_min = cv.yx_max =-cv.x_min + cv.y_min;

  for(cnt=1; cnt<8; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

    tmp_x = points[cnt].x*xdir.x + points[cnt].y*ydir.x + points[cnt].z*zdir.x + pos.x;
    if(tmp_x < cv.x_min) cv.x_min = tmp_x - 0.001f;
    else if(tmp_x > cv.x_max) cv.x_max = tmp_x + 0.001f;

    tmp_y = points[cnt].x*xdir.y + points[cnt].y*ydir.y + points[cnt].z*zdir.y + pos.y;
    if(tmp_y < cv.y_min) cv.y_min = tmp_y - 0.001f;
    else if(tmp_y > cv.y_max) cv.y_max = tmp_y + 0.001f;

    tmp_z = points[cnt].x*xdir.z + points[cnt].y*ydir.z + points[cnt].z*zdir.z + pos.z;
    if(tmp_z < cv.z_min) cv.z_min = tmp_z - 0.001f;
    else if(tmp_z > cv.z_max) cv.z_max = tmp_z + 0.001f;

    tmp_xy = tmp_x + tmp_y;
    if(tmp_xy < cv.xy_min) cv.xy_min = tmp_xy - 0.001f;
    else if(tmp_xy > cv.xy_max) cv.xy_max = tmp_xy + 0.001f;

    tmp_yx = -tmp_x + tmp_y;
    if(tmp_yx < cv.yx_min) cv.yx_min = tmp_yx - 0.001f;
    else if(tmp_yx > cv.yx_max) cv.yx_max = tmp_yx + 0.001f;
  };

  bd->cv.x_min  = cv.x_min;
  bd->cv.y_min  = cv.y_min;
  bd->cv.z_min  = cv.z_min;
  bd->cv.xy_min = cv.xy_min;
  bd->cv.yx_min = cv.yx_min;

  bd->cv.x_max  = cv.x_max;
  bd->cv.y_max  = cv.y_max;
  bd->cv.z_max  = cv.z_max;
  bd->cv.xy_max = cv.xy_max;
  bd->cv.yx_max = cv.yx_max;

  bd->mids_hi.x = (bd->cv.x_min + bd->cv.x_max) * 0.5f + pchr->ori.pos.x;
  bd->mids_hi.y = (bd->cv.y_min + bd->cv.y_max) * 0.5f + pchr->ori.pos.y;
  bd->mids_hi.z = (bd->cv.z_min + bd->cv.z_max) * 0.5f + pchr->ori.pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = bd->cv.z_min + pchr->ori.pos.z;

  bd->calc_height   = bd->cv.z_max;
  bd->calc_size     = MAX( MAX( bd->cv.x_max,  bd->cv.y_max ), - MIN( bd->cv.x_min,  bd->cv.y_min ) );
  bd->calc_size_big = MAX( MAX( bd->cv.xy_max, bd->cv.yx_max), - MIN( bd->cv.xy_min, bd->cv.yx_min) );

  if(bd->calc_size_big < bd->calc_size*1.1)
  {
    bd->calc_size     *= -1;
  }
  else if (bd->calc_size*2 < bd->calc_size_big*1.1)
  {
    bd->calc_size_big *= -1;
  }

  bd->cv.lod = 1;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers_2( Game_t * gs, Chr_t * pchr, vect3 * vrt_ary)
{
  BData_t * bd;
  MD2_Model_t * pmdl;
  vect3 xdir, ydir, zdir, pos;
  Uint32 cnt;
  Uint32  vrt_count;
  bool_t  free_array = bfalse;

  CVolume_t cv;

  PObj_t objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  OBJ_REF iobj;
  Obj_t  * pobj;

  Mad_t  * pmad;


  if(  !EKEY_PVALID(pchr) ) return bfalse;
  bd = &(pchr->bmpdata);

  iobj = pchr->model = VALIDATE_OBJ( objlst, pchr->model );
  pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  pmad = ObjList_getPMad(gs, iobj);
  if( NULL == pmad || !pchr->matrix_valid )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }


  pmdl = pmad->md2_ptr;
  if( NULL == pmdl )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  xdir.x = (pchr->matrix).CNV(0,0);
  xdir.y = (pchr->matrix).CNV(0,1);
  xdir.z = (pchr->matrix).CNV(0,2);

  ydir.x = (pchr->matrix).CNV(1,0);
  ydir.y = (pchr->matrix).CNV(1,1);
  ydir.z = (pchr->matrix).CNV(1,2);

  zdir.x = (pchr->matrix).CNV(2,0);
  zdir.y = (pchr->matrix).CNV(2,1);
  zdir.z = (pchr->matrix).CNV(2,2);

  pos.x = (pchr->matrix).CNV(3,0);
  pos.y = (pchr->matrix).CNV(3,1);
  pos.z = (pchr->matrix).CNV(3,2);


  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  md2_blend_vertices(pchr, -1, -1);

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  if(NULL == vrt_ary)
  {
    vrt_ary = (vect3*)calloc(vrt_count, sizeof(vect3));
    if(NULL ==vrt_ary)
    {
      return chr_calculate_bumpers_1( gs, pchr );
    }
    free_array = btrue;
  }

  // transform the verts all at once, to reduce function calling overhead
  Transform3_Full( 1.0f, 1.0f, &(pchr->matrix), pchr->vdata.Vertices, vrt_ary, vrt_count);

  cv.x_min  = cv.x_max  = vrt_ary[0].x;
  cv.y_min  = cv.y_max  = vrt_ary[0].y;
  cv.z_min  = cv.z_max  = vrt_ary[0].z;
  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
  cv.yx_min = cv.yx_max =-cv.x_min + cv.y_min;

  vrt_count = pmad->transvertices;
  for(cnt=1; cnt<vrt_count; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

    tmp_x = vrt_ary[cnt].x;
    if(tmp_x < cv.x_min) cv.x_min = tmp_x - 0.001f;
    else if(tmp_x > cv.x_max) cv.x_max = tmp_x + 0.001f;

    tmp_y = vrt_ary[cnt].y;
    if(tmp_y < cv.y_min) cv.y_min = tmp_y - 0.001f;
    else if(tmp_y > cv.y_max) cv.y_max = tmp_y + 0.001f;

    tmp_z = vrt_ary[cnt].z;
    if(tmp_z < cv.z_min) cv.z_min = tmp_z - 0.001f;
    else if(tmp_z > cv.z_max) cv.z_max = tmp_z + 0.001f;

    tmp_xy = tmp_x + tmp_y;
    if(tmp_xy < cv.xy_min) cv.xy_min = tmp_xy - 0.001f;
    else if(tmp_xy > cv.xy_max) cv.xy_max = tmp_xy + 0.001f;

    tmp_yx =-tmp_x + tmp_y;
    if(tmp_yx < cv.yx_min) cv.yx_min = tmp_yx - 0.001f;
    else if(tmp_yx > cv.yx_max) cv.yx_max = tmp_yx + 0.001f;
  };

  bd->cv = cv;

  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + pchr->ori.pos.x;
  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + pchr->ori.pos.y;
  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + pchr->ori.pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = cv.z_min + pchr->ori.pos.z;

  bd->calc_height   = bd->cv.z_max - pos.z;
  bd->calc_size     = 0.5f * ( MAX( bd->cv.x_max, bd->cv.y_max ) - MIN( bd->cv.x_min, bd->cv.y_min ) );
  bd->calc_size_big = 0.5f * ( MAX( bd->cv.xy_max, bd->cv.yx_max) - MIN( bd->cv.xy_min, bd->cv.yx_min) );

  if(bd->calc_size_big < bd->calc_size*1.1)
  {
    bd->calc_size     *= -1;
  }
  else if (bd->calc_size*2 < bd->calc_size_big*1.1)
  {
    bd->calc_size_big *= -1;
  }

  bd->cv.lod = 2;

  if(free_array)
  {
    FREE( vrt_ary );
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers_3( Game_t * gs, Chr_t * pchr, CVolume_Tree_t * cv_tree)
{
  BData_t * bd;
  MD2_Model_t * pmdl;
  Uint32 cnt, tnc;
  Uint32  tri_count, vrt_count;
  vect3 * vrt_ary;
  CVolume_t *pcv, cv_node[8];

  PObj_t objlst     = gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  OBJ_REF iobj;
  Obj_t  * pobj;

  Mad_t  * pmad;


  if(  !EKEY_PVALID(pchr) ) return bfalse;
  bd = &(pchr->bmpdata);

  iobj = pchr->model = VALIDATE_OBJ( objlst, pchr->model );
  pobj = ObjList_getPObj(gs, iobj);
  if(NULL == pobj)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  pmad = ObjList_getPMad(gs, iobj);
  if( NULL == pmad || !pchr->matrix_valid )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }


  pmdl = pmad->md2_ptr;
  if( NULL == pmdl )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  vrt_ary   = (vect3*)calloc(vrt_count, sizeof(vect3));
  if(NULL ==vrt_ary) return chr_calculate_bumpers_1(  gs, pchr );

  // make sure that we have the correct bounds
  if(bd->cv.lod < 2)
  {
    chr_calculate_bumpers_2( gs, pchr, vrt_ary);
  }

  if(NULL == cv_tree) return bfalse;

  // transform the verts all at once, to reduce function calling overhead
  Transform3_Full( 1.0f, 1.0f, &(pchr->matrix), pchr->vdata.Vertices, vrt_ary, vrt_count);

  pcv = &(pchr->bmpdata.cv);

  // initialize the octree
  for(tnc=0; tnc<8; tnc++)
  {
    cv_tree->leaf[tnc].lod = -1;
  }

  // calculate the raw CVolumes for the octree nodes
  for(tnc=0; tnc<8; tnc++)
  {

    if(0 == ((tnc >> 0) & 1))
    {
      cv_node[tnc].x_min = pcv->x_min;
      cv_node[tnc].x_max = (pcv->x_min + pcv->x_max)*0.5f;
    }
    else
    {
      cv_node[tnc].x_min = (pcv->x_min + pcv->x_max)*0.5f;
      cv_node[tnc].x_max = pcv->x_max;
    };

    if(0 == ((tnc >> 1) & 1))
    {
      cv_node[tnc].y_min = pcv->y_min;
      cv_node[tnc].y_max = (pcv->y_min + pcv->y_max)*0.5f;
    }
    else
    {
      cv_node[tnc].y_min = (pcv->y_min + pcv->y_max)*0.5f;
      cv_node[tnc].y_max = pcv->y_max;
    };

    if(0 == ((tnc >> 2) & 1))
    {
      cv_node[tnc].z_min = pcv->z_min;
      cv_node[tnc].z_max = (pcv->z_min + pcv->z_max)*0.5f;
    }
    else
    {
      cv_node[tnc].z_min = (pcv->z_min + pcv->z_max)*0.5f;
      cv_node[tnc].z_max = pcv->z_max;
    };

    cv_node[tnc].lod = 0;
  }

  tri_count = pmdl->m_numTriangles;
  for(cnt=0; cnt < tri_count; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;
    CVolume_t cv_tri;
    const MD2_Triangle_t * pmd2_tri = md2_get_Triangle(pmdl, cnt);
    short * tri = (short *)pmd2_tri->vertexIndices;
    int ivrt = tri[0];

    // find the collision volume for this triangle
    tmp_x = vrt_ary[ivrt].x;
    cv_tri.x_min = cv_tri.x_max = tmp_x;

    tmp_y = vrt_ary[ivrt].y;
    cv_tri.y_min = cv_tri.y_max = tmp_y;

    tmp_z = vrt_ary[ivrt].z;
    cv_tri.z_min = cv_tri.z_max = tmp_z;

    tmp_xy = tmp_x + tmp_y;
    cv_tri.xy_min = cv_tri.xy_max = tmp_xy;

    tmp_yx = -tmp_x + tmp_y;
    cv_tri.yx_min = cv_tri.yx_max = tmp_yx;

    for(tnc=1; tnc<3; tnc++)
    {
      ivrt = tri[tnc];

      tmp_x = vrt_ary[ivrt].x;
      if(tmp_x < cv_tri.x_min) cv_tri.x_min = tmp_x - 0.001f;
      else if(tmp_x > cv_tri.x_max) cv_tri.x_max = tmp_x + 0.001f;

      tmp_y = vrt_ary[ivrt].y;
      if(tmp_y < cv_tri.y_min) cv_tri.y_min = tmp_y - 0.001f;
      else if(tmp_y > cv_tri.y_max) cv_tri.y_max = tmp_y + 0.001f;

      tmp_z = vrt_ary[ivrt].z;
      if(tmp_z < cv_tri.z_min) cv_tri.z_min = tmp_z - 0.001f;
      else if(tmp_z > cv_tri.z_max) cv_tri.z_max = tmp_z + 0.001f;

      tmp_xy = tmp_x + tmp_y;
      if(tmp_xy < cv_tri.xy_min) cv_tri.xy_min = tmp_xy - 0.001f;
      else if(tmp_xy > cv_tri.xy_max) cv_tri.xy_max = tmp_xy + 0.001f;

      tmp_yx = -tmp_x + tmp_y;
      if(tmp_yx < cv_tri.yx_min) cv_tri.yx_min = tmp_yx - 0.001f;
      else if(tmp_yx > cv_tri.yx_max) cv_tri.yx_max = tmp_yx + 0.001f;
    };

    cv_tri.lod = 0;

    // add the triangle to the octree
    for(tnc=0; tnc<8; tnc++)
    {
      if(cv_tri.x_min >= cv_node[tnc].x_max || cv_tri.x_max <= cv_node[tnc].x_min) continue;
      if(cv_tri.y_min >= cv_node[tnc].y_max || cv_tri.y_max <= cv_node[tnc].y_min) continue;
      if(cv_tri.z_min >= cv_node[tnc].z_max || cv_tri.z_max <= cv_node[tnc].z_min) continue;

      //there is an overlap with the default otree cv
      cv_tree->leaf[tnc] = CVolume_merge(cv_tree->leaf + tnc, &cv_tri);

      // make sure that the "octree" node does not go outside the maximum bounds
      cv_tree->leaf[tnc].x_min = MAX(cv_tree->leaf[tnc].x_min, cv_node[tnc].x_min);
      cv_tree->leaf[tnc].x_max = MIN(cv_tree->leaf[tnc].x_max, cv_node[tnc].x_max);

      cv_tree->leaf[tnc].y_min = MAX(cv_tree->leaf[tnc].y_min, cv_node[tnc].y_min);
      cv_tree->leaf[tnc].y_max = MIN(cv_tree->leaf[tnc].y_max, cv_node[tnc].y_max);

      cv_tree->leaf[tnc].z_min = MAX(cv_tree->leaf[tnc].z_min, cv_node[tnc].z_min);
      cv_tree->leaf[tnc].z_max = MIN(cv_tree->leaf[tnc].z_max, cv_node[tnc].z_max);
    }

  };

  bd->cv.lod = 3;

  FREE( vrt_ary );

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers( Game_t * gs, Chr_t * pchr, int level)
{
  bool_t retval = bfalse;

  if(pchr->bmpdata.cv.lod >= level) return btrue;

  switch(level)
  {
    case 2:
      // the collision volume is an octagon, the ranges are calculated using the model's vertices
      retval = chr_calculate_bumpers_2( gs, pchr, NULL);
      break;

    case 3:
      {
        // calculate the octree collision volume
        if(NULL == pchr->bmpdata.cv_tree)
        {
          pchr->bmpdata.cv_tree = (CVolume_Tree_t*)calloc(1, sizeof(CVolume_Tree_t));
        };
        retval = chr_calculate_bumpers_3( gs, pchr, pchr->bmpdata.cv_tree);
      };
      break;

    case 1:
      // the collision volume is a simple axis-aligned bounding box, the range is calculated from the
      // md2's bounding box
      retval = chr_calculate_bumpers_1( gs, pchr);
      break;

    default:
    case 0:
      // make the simplest estimation of the bounding box using the data in data.txt
      retval = chr_calculate_bumpers_0(pchr);
      break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
void damage_character( Game_t * gs, CHR_REF chr_ref, Uint16 direction,
                       PAIR * pdam, DAMAGE damagetype, TEAM_REF team,
                       CHR_REF attacker, Uint16 effects )
{
  // ZZ> This function calculates and applies damage to a character.  It also
  //     sets alerts and begins actions.  Blocking and frame invincibility
  //     are done here too.  Direction is FRONT if the attack is coming head on,
  //     RIGHT if from the right, BEHIND if from the back, LEFT if from the
  //     left.

  Uint32 loc_rand;

  PObj_t objlst      = gs->ObjList;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PMad_t madlst      = gs->MadList;
  size_t madlst_size = MADLST_COUNT;

  CHR_REF chr_tnc;
  Uint16 tnc;

  ACTION action;
  int damage, basedamage;
  Uint16 experience, left, right;
  AI_STATE * pstate;
  Chr_t * pchr;
  Mad_t * pmad;
  Cap_t * pcap;
  Obj_t * pobj;

  if( !ACTIVE_CHR(chrlst, chr_ref) ) return;

  loc_rand = gs->randie_index;

  pchr   = ChrList_getPChr(gs, chr_ref);
  pstate = &(pchr->aistate);

  if ( NULL == pdam ) return;
  if ( chr_is_player(gs, chr_ref) && CData.DevMode ) return;

  if ( pchr->alive && pdam->ibase >= 0 && pdam->irand >= 1 )
  {
    // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    damage = generate_unsigned( &loc_rand, pdam );
    basedamage = damage;
    damage >>= ( pchr->skin.damagemodifier_fp8[damagetype] & DAMAGE_SHIFT );

    // Allow charging (Invert damage to mana)
    if ( pchr->skin.damagemodifier_fp8[damagetype]&DAMAGE_CHARGE )
    {
      pchr->stats.mana_fp8 += damage;
      if ( pchr->stats.mana_fp8 > pchr->stats.manamax_fp8 )
      {
        pchr->stats.mana_fp8 = pchr->stats.manamax_fp8;
      }
      return;
    }

    // Mana damage (Deal damage to mana)
    if ( pchr->skin.damagemodifier_fp8[damagetype]&DAMAGE_MANA )
    {
      pchr->stats.mana_fp8 -= damage;
      if ( pchr->stats.mana_fp8 < 0 )
      {
        pchr->stats.mana_fp8 = 0;
      }
      return;
    }

    // Invert damage to heal
    if ( pchr->skin.damagemodifier_fp8[damagetype]&DAMAGE_INVERT )
      damage = -damage;

    // Remember the damage type
    pstate->damagetypelast = damagetype;
    pstate->directionlast = direction;

    // Do it already
    if ( damage > 0 )
    {
      // Only damage if not invincible
      if ( pchr->damagetime == 0 && !pchr->prop.invictus )
      {
        pmad = ChrList_getPMad(gs, chr_ref);
        pcap = ChrList_getPCap(gs, chr_ref);
        pobj = ChrList_getPObj(gs, chr_ref);
        if ( HAS_SOME_BITS( effects, DAMFX_BLOC ) )
        {
          // Only damage if hitting from proper direction
          if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_INVICTUS ) )
          {
            // I Frame...
            direction -= pcap->iframefacing;
            left = ( ~pcap->iframeangle );
            right = pcap->iframeangle;

            // Check for shield
            if ( pchr->action.now >= ACTION_PA && pchr->action.now <= ACTION_PD )
            {
              // Using a shield?
              if ( pchr->action.now < ACTION_PC )
              {
                // Check left hand
                CHR_REF iholder = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_LEFT );
                if ( ACTIVE_CHR( chrlst, iholder ) )
                {
                  left  = ~ChrList_getPCap(gs, iholder)->iframeangle;
                  right = ChrList_getPCap(gs, iholder)->iframeangle;
                }
              }
              else
              {
                // Check right hand
                CHR_REF iholder = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_RIGHT );
                if ( ACTIVE_CHR( chrlst, iholder ) )
                {
                  left  = ~ChrList_getPCap(gs, iholder)->iframeangle;
                  right = ChrList_getPCap(gs, iholder)->iframeangle;
                }
              }
            }
          }
          else
          {
            // N Frame
            direction -= pcap->nframefacing;
            left = ( ~pcap->nframeangle );
            right = pcap->nframeangle;
          }
          // Check that direction
          if ( direction > left || direction < right )
          {
            damage = 0;
          }
        }

        if ( damage != 0 )
        {
          if ( HAS_SOME_BITS( effects, DAMFX_ARMO ) )
          {
            pchr->stats.life_fp8 -= damage;
          }
          else
          {
            pchr->stats.life_fp8 -= FP8_MUL( damage, pchr->skin.defense_fp8 );
          }

          if ( basedamage > DAMAGE_MIN )
          {
            PRT_SPAWN_INFO si;

            // Call for help if below 1/2 life
            if ( pchr->stats.life_fp8 < ( pchr->stats.lifemax_fp8 >> 1 ) ) //Zefz: Removed, because it caused guards to attack
              call_for_help( gs, chr_ref );                    //when dispelling overlay spells (Faerie Light)

            // Spawn blud particles
            if ( pcap->bludlevel > BLUD_NONE && ( damagetype < DAMAGE_HOLY || pcap->bludlevel == BLUD_ULTRA ) )
            {
              prt_spawn_info_init( &si, gs, 1.0f, pchr->ori.pos,
                                  pchr->ori.turn_lr + direction, pchr->model, pcap->bludprttype,
                                  INVALID_CHR, GRIP_LAST, pchr->team, chr_ref, 0, INVALID_CHR );
              req_spawn_one_particle( si );
            }
            // Set attack alert if it wasn't an accident
            if ( team == TEAM_DAMAGE )
            {
              pstate->attacklast = INVALID_CHR;
            }
            else
            {
              // Don't alert the chr_ref too much if under constant fire
              if ( pchr->carefultime == 0 )
              {
                // Don't let ichrs chase themselves...  That would be silly
                if ( attacker != chr_ref )
                {
                  pstate->alert |= ALERT_ATTACKED;
                  pstate->attacklast = attacker;
                  pchr->carefultime = DELAY_CAREFUL;
                }
              }
            }
          }

          // Taking damage action
          action = ACTION_HA;
          if ( pchr->stats.life_fp8 < 0 )
          {
            // Character has died
            pchr->alive = bfalse;
            disenchant_character( gs, chr_ref );
            pchr->action.keep = btrue;
            pchr->stats.life_fp8 = -1;
            pchr->prop.isplatform = btrue;
            pchr->bumpdampen /= 2.0;
            action = ACTION_KA;
            snd_stop_sound(pchr->loopingchannel);    //Stop sound loops
            pchr->loopingchannel = -1;
            // Give kill experience
            experience = pcap->experienceworth + ( pchr->experience * pcap->experienceexchange );
            if ( ACTIVE_CHR( chrlst, attacker ) )
            {
              // Set target
              pstate->target = attacker;
              if ( team == TEAM_DAMAGE )  pstate->target = chr_ref;
              if ( team == TEAM_NULL )  pstate->target = chr_ref;
              // Award direct kill experience
              if ( gs->TeamList[chrlst[attacker].team].hatesteam[REF_TO_INT(pchr->team)] )
              {
                give_experience( gs, attacker, experience, XP_KILLENEMY );
              }

              // Check for hated
              if ( CAP_INHERIT_IDSZ( gs,  pobj->cap, ChrList_getPCap(gs, attacker)->idsz[IDSZ_HATE] ) )
              {
                give_experience( gs, attacker, experience, XP_KILLHATED );
              }
            }

            // Clear all shop passages that it owned...
            tnc = 0;
            while ( tnc < gs->ShopList_count )
            {
              if ( gs->ShopList[tnc].owner == chr_ref )
              {
                gs->ShopList[tnc].owner = NOOWNER;
              }
              tnc++;
            }

            // Let the other characters know it died

            for ( chr_tnc = 0; chr_tnc < chrlst_size; chr_tnc++ )
            {
              if(!ACTIVE_CHR(chrlst, chr_tnc)) continue;

              if ( chrlst[chr_tnc].alive )
              {
                if ( chrlst[chr_tnc].aistate.target == chr_ref )
                {
                  chrlst[chr_tnc].aistate.alert |= ALERT_TARGETKILLED;
                }
                if ( !gs->TeamList[chrlst[chr_tnc].team].hatesteam[REF_TO_INT(team)] && gs->TeamList[chrlst[chr_tnc].team].hatesteam[REF_TO_INT(pchr->team)] )
                {
                  // All allies get team experience, but only if they also hate the dead guy's team
                  give_experience( gs, chr_tnc, experience, XP_TEAMKILL );
                }
              }
            }

            // Check if it was a leader
            if ( team_get_leader( gs, pchr->team ) == chr_ref )
            {
              // It was a leader, so set more alerts

              for ( chr_tnc = 0; chr_tnc < chrlst_size; chr_tnc++ )
              {
                if( !ACTIVE_CHR(chrlst, chr_tnc) ) continue;

                if ( chrlst[chr_tnc].team == pchr->team )
                {
                  // All folks on the leaders team get the alert
                  chrlst[chr_tnc].aistate.alert |= ALERT_LEADERKILLED;
                }

              }

              // The team now has no leader
              gs->TeamList[pchr->team].leader = search_best_leader( gs, pchr->team, chr_ref );
            }

            detach_character_from_mount( gs, chr_ref, btrue, bfalse );
            action = (ACTION)(action + IRAND(&loc_rand, 2));
            play_action( gs,  chr_ref, action, bfalse );

            // Turn off all sounds if it's a player
            for ( tnc = 0; tnc < MAXWAVE; tnc++ )
            {
              //TODO Zefz: Do we need this? This makes all sounds a chr_ref makes stop when it dies...
              //           This may stop death sounds
              //snd_stop_sound(pchr->model);
            }

            // Afford it one last thought if it's an AI
            gs->TeamList[pchr->team_base].morale--;
            pchr->team = pchr->team_base;
            pstate->alert = ALERT_KILLED;
            pchr->sparkle = NOSPARKLE;
            pstate->time = 1;  // No timeout...
            run_script( gs, chr_ref, 1.0f );
          }
          else
          {
            if ( basedamage > DAMAGE_MIN )
            {
              action = (ACTION)(action + IRAND(&loc_rand, 2));
              play_action( gs,  chr_ref, action, bfalse );

              // Make the chr_ref invincible for a limited time only
              if ( HAS_NO_BITS( effects, DAMFX_TIME ) )
                pchr->damagetime = DELAY_DAMAGE;
            }
          }
        }
        else
        {
          // Spawn a defend particle
          PRT_SPAWN_INFO si;

          prt_spawn_info_init( &si, gs, pchr->bumpstrength, pchr->ori.pos, pchr->ori.turn_lr, INVALID_OBJ, PIP_REF(PRTPIP_DEFEND), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, 0, INVALID_CHR );
          req_spawn_one_particle( si );
          pchr->damagetime = DELAY_DEFEND;
          pstate->alert |= ALERT_BLOCKED;
        }
      }
    }
    else if ( damage < 0 )
    {
      pchr->stats.life_fp8 -= damage;
      if ( pchr->stats.life_fp8 > pchr->stats.lifemax_fp8 )  pchr->stats.life_fp8 = pchr->stats.lifemax_fp8;

      // Isssue an alert
      pstate->alert |= ALERT_HEALED;
      pstate->attacklast = attacker;
      if ( team != TEAM_DAMAGE )
      {
        pstate->attacklast = INVALID_CHR;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void kill_character( Game_t * gs, CHR_REF chr_ref, CHR_REF killer )
{
  // ZZ> This function kills a character...  CHRLST_COUNT killer for accidental death

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  Uint8 modifier;
  Chr_t * pchr;

  pchr = ChrList_getPChr(gs, chr_ref);
  if(NULL == pchr) return;

  if ( !pchr->alive ) return;

  pchr->damagetime = 0;
  pchr->stats.life_fp8 = 1;
  modifier = pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH];
  pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH] = 1;
  if ( ACTIVE_CHR( chrlst, killer ) )
  {
    PAIR ptemp = {512, 1};
    damage_character( gs, chr_ref, 0, &ptemp, DAMAGE_CRUSH, chrlst[killer].team, killer, DAMFX_ARMO | DAMFX_BLOC );
  }
  else
  {
    PAIR ptemp = {512, 1};
    damage_character( gs, chr_ref, 0, &ptemp, DAMAGE_CRUSH, TEAM_REF(TEAM_DAMAGE), chr_get_aibumplast( chrlst, chrlst_size, chrlst + chr_ref ), DAMFX_ARMO | DAMFX_BLOC );
  }
  pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH] = modifier;

  // try something here.
  pchr->prop.isplatform = btrue;
  pchr->prop.ismount    = bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t import_info_clear(IMPORT_INFO * ii)
{
  int cnt;

  if(NULL == ii) return bfalse;

  ii->player = -1;

  for ( cnt = 0; cnt < OBJLST_COUNT; cnt++ )
    ii->slot_lst[cnt] = INVALID_OBJ;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t import_info_add(IMPORT_INFO * ii, OBJ_REF obj)
{
  if(NULL == ii || !VALID_OBJ_RANGE(obj)) return bfalse;

  ii->object = obj;
  ii->slot_lst[REF_TO_INT(obj)] = obj;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void cv_list_add( CVolume_t * cv)
{
  if(NULL == cv || cv_list_count > 1000) return;

  cv_list[cv_list_count++] = *cv;
};

//--------------------------------------------------------------------------------------------
INLINE void cv_list_clear()
{
  cv_list_count = 0;
};

//--------------------------------------------------------------------------------------------
INLINE void cv_list_draw()
{
  Uint32 cnt;

  for(cnt=0; cnt<cv_list_count; cnt++)
  {
    CVolume_draw( &(cv_list[cnt]), btrue, bfalse );
  };
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void play_action( Game_t * gs, CHR_REF ichr, ACTION action, bool_t ready )
{
  // ZZ> This function starts a generic action for a ichr

  Chr_t * pchr;
  Mad_t * pmad;

  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return;

  pmad = ChrList_getPMad(gs, ichr);
  if(NULL == pmad) return;

  if ( pmad->actionvalid[action] )
  {
    pchr->action.next  = ACTION_DA;
    pchr->action.now   = action;
    pchr->action.ready = ready;

    pchr->anim.ilip = 0;
    pchr->anim.flip = 0.0f;
    pchr->anim.last = pchr->anim.next;
    pchr->anim.next = pmad->actionstart[pchr->action.now];
  }

}

//--------------------------------------------------------------------------------------------
void set_frame( Game_t * gs, CHR_REF ichr, Uint16 frame, Uint8 lip )
{
  // ZZ> This function sets the frame for a ichr explicitly...  This is used to
  //     rotate Tank turrets

  Uint16 start, end;
  Chr_t * pchr;
  Mad_t * pmad;

  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return;

  pmad = ChrList_getPMad(gs, ichr);
  if(NULL == pmad) return;

  pchr->action.next  = ACTION_DA;
  pchr->action.now   = ACTION_DA;
  pchr->action.ready = btrue;

  pchr->anim.ilip = ( lip << 6 );
  pchr->anim.flip    = lip * 0.25;

  start = pmad->actionstart[pchr->action.now];
  end   = pmad->actionstart[pchr->action.now];

  if(start == end)
  {
    pchr->anim.last =
    pchr->anim.next = start;
  }
  else
  {
    pchr->anim.last    = start + frame;
    if(pchr->anim.last > end)
    {
      pchr->anim.last = (pchr->anim.last - end) % (end - start) + end;
    };

    pchr->anim.next    = pchr->anim.last + 1;
    if(pchr->anim.next > end)
    {
      pchr->anim.next = (pchr->anim.next - end) % (end - start) + end;
    };
  }

}

//--------------------------------------------------------------------------------------------
void drop_money( Game_t * gs, CHR_REF ichr, Uint16 money )
{
  // ZZ> This function drops some of a ichr's money

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  Uint16 huns, tfives, fives, ones, cnt;

  if ( money > chrlst[ichr].money )  money = chrlst[ichr].money;

  if ( money > 0 && chrlst[ichr].ori.pos.z > -2 )
  {
    PRT_SPAWN_INFO si;

    chrlst[ichr].money -= money;
    huns   = money / 100;  money -= huns   * 100;
    tfives = money /  25;  money -= tfives *  25;
    fives  = money /   5;  money -= fives  *   5;
    ones   = money;

    for ( cnt = 0; cnt < ones; cnt++ )
    {
      prt_spawn_info_init( &si, gs, 1.0f, chrlst[ichr].ori.pos, 0, INVALID_OBJ, PIP_REF(PRTPIP_COIN_001), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, cnt, INVALID_CHR );
      req_spawn_one_particle( si );
    }

    for ( cnt = 0; cnt < fives; cnt++ )
    {
      prt_spawn_info_init( &si, gs, 1.0f, chrlst[ichr].ori.pos, 0, INVALID_OBJ, PIP_REF(PRTPIP_COIN_005), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, cnt, INVALID_CHR );
      req_spawn_one_particle( si );
    }

    for ( cnt = 0; cnt < tfives; cnt++ )
    {
      prt_spawn_info_init( &si, gs, 1.0f, chrlst[ichr].ori.pos, 0, INVALID_OBJ, PIP_REF(PRTPIP_COIN_025), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, cnt, INVALID_CHR );
      req_spawn_one_particle( si );
    }

    for ( cnt = 0; cnt < huns; cnt++ )
    {
      prt_spawn_info_init( &si, gs, 1.0f, chrlst[ichr].ori.pos, 0, INVALID_OBJ, PIP_REF(PRTPIP_COIN_100), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, cnt, INVALID_CHR );
      req_spawn_one_particle( si );
    }

    chrlst[ichr].damagetime = DELAY_DAMAGE;  // So it doesn't grab it again
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_spawn( Game_t * gs,  vect3 pos, vect3 vel, OBJ_REF iobj, TEAM_REF team,
                   Uint8 skin, Uint16 facing, const char *name, CHR_REF override )
{
  CHR_REF retval;
  CHR_SPAWN_INFO chr_si;

  bool_t client_running = bfalse, server_running = bfalse, local_running = bfalse;
  bool_t local_control = bfalse;

  if(!EKEY_PVALID(gs)) return INVALID_CHR;

  client_running = CClient_Running(gs->cl);
  server_running = sv_Running(gs->sv);
  local_running  = !client_running && !server_running;
  local_control = server_running || local_running;

  // initialize the spawning data
  if(NULL == chr_spawn_info_new( &chr_si, gs )) return INVALID_CHR;

  retval = chr_spawn_info_init( &chr_si, pos, vel, iobj, team, skin, facing, name, override );
  if(INVALID_CHR == retval) return INVALID_CHR;

  // Initialize the character data.
  // It will automatically activate if this computer is in control of the spawn
  retval = _chr_spawn( chr_si, local_control );
  if(INVALID_CHR == retval) return INVALID_CHR;

  if ( !local_control )
  {
    // This computer does not control the spawn.
    // The character will only activate it the server to OKs this spawn.
    snd_chr_spawn( chr_si );
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_spawn_info_init( CHR_SPAWN_INFO * psi, vect3 pos, vect3 vel, OBJ_REF iobj, TEAM_REF team,
                            Uint8 skin, Uint16 facing, const char *name, CHR_REF override )
{
  if( !EKEY_PVALID(psi) ) return bfalse;

  // fill in all the information
  psi->ichr      = INVALID_CHR;
  psi->pos       = pos;
  psi->vel       = vel;
  psi->iobj      = iobj;
  psi->iteam     = team;
  psi->iskin     = skin;
  psi->facing    = facing;
  psi->ioverride = override;

  if(NULL != name)
  {
    strncpy(psi->name, name, sizeof(psi->name));
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF ChrList_reserve(CHR_SPAWN_INFO * psi)
{
  PChr_t chrlst;
  PObj_t objlst;
  PCap_t caplst;
  PMad_t madlst;

  Game_t * gs;
  Chr_t  * pchr;
  Obj_t  * pobj;
  Cap_t  * pcap;
  Mad_t  * pmad;

  if( !EKEY_PVALID( psi ) ) return INVALID_CHR;

  gs     = psi->gs;
  chrlst = gs->ChrList;
  objlst = gs->ObjList;
  caplst = gs->CapList;
  madlst = gs->MadList;

  pobj = ObjList_getPObj(gs, psi->iobj);
  if( NULL == pobj )
  {
    log_debug( "WARNING: chr_spawn_info_init() - invalid request : profile %d doesn't exist\n", REF_TO_INT(psi->iobj) );
    return INVALID_CHR;
  }

  pcap = ObjList_getPCap(gs, psi->iobj);
  if( NULL == pcap )
  {
    log_debug( "WARNING: chr_spawn_info_init() - invalid request : character profile (cap) doesn't exist\n" );
    return INVALID_CHR;
  }

  pmad = ObjList_getPMad(gs, psi->iobj);
  if( NULL == pmad )
  {
    log_debug( "WARNING: chr_spawn_info_init() - invalid request : character profile (mad) doesn't exist\n" );
    return INVALID_CHR;
  }

  // Get a new character
  psi->ichr = ChrList_get_free(gs, psi->ioverride);
  if(INVALID_CHR == psi->ichr || chrlst[psi->ichr].reserved) return INVALID_CHR;
  pchr = chrlst + psi->ichr;

  // Set the status of the data to "reserved"
  pchr->reserved   = btrue;

  return psi->ichr;
}

//--------------------------------------------------------------------------------------------
bool_t chr_create_stats( Uint32 * pseed, Stats_t * pstats, StatData_t * pdata)
{
  if(NULL == pstats || NULL == pdata) return bfalse;

  // LIFE
  pstats->lifeheal_fp8   = pdata->lifeheal_fp8;
  pstats->lifereturn_fp8 = pdata->lifereturn_fp8;
  pstats->lifemax_fp8    = generate_unsigned( pseed, &pdata->life_pair );
  pstats->life_fp8       = pstats->lifemax_fp8;

  // MANA
  pstats->manaflow_fp8   = generate_unsigned( pseed, &pdata->manaflow_pair );
  pstats->manareturn_fp8 = generate_unsigned( pseed, &pdata->manareturn_pair );  // >> MANARETURNSHIFT;
  pstats->manamax_fp8    = generate_unsigned( pseed, &pdata->mana_pair );
  pstats->mana_fp8       = pstats->manamax_fp8;

  // SWID
  pstats->strength_fp8     = generate_unsigned( pseed, &pdata->strength_pair );
  pstats->wisdom_fp8       = generate_unsigned( pseed, &pdata->wisdom_pair );
  pstats->intelligence_fp8 = generate_unsigned( pseed, &pdata->intelligence_pair );
  pstats->dexterity_fp8    = generate_unsigned( pseed, &pdata->dexterity_pair );

  return btrue;
}

////--------------------------------------------------------------------------------------------
//CHR_REF req_chr_spawn_one( CHR_SPAWN_INFO si)
//{
//  bool_t client_running = bfalse, server_running = bfalse, local_running = bfalse;
//  CHR_REF retval;
//  Game_t * gs = si.gs;
//
//  if( !EKEY_PVALID(gs) ) return INVALID_CHR;
//
//  client_running = CClient_Running(gs->cl);
//  server_running = sv_Running(gs->sv);
//  local_running  = !client_running && !server_running;
//
//  if(server_running || local_running)
//  {
//    retval = _chr_spawn( si, btrue );
//  }
//  else if ( client_running )
//  {
//    retval = si.ichr;
//    snd_chr_spawn( si  );
//  }
//
//}

//--------------------------------------------------------------------------------------------
CHR_REF _chr_spawn( CHR_SPAWN_INFO si, bool_t activate )
{
  // ZZ> This function initializes a Chr_t data structure in the "reserved" state,
  //     if the CHR_SPAWN_INFO is valid.
  //     On success, it returns the index, otherwise INVALID_CHR.

  PChr_t chrlst      = si.gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PObj_t objlst     = si.gs->ObjList;
  size_t  objlst_size = OBJLST_COUNT;

  PCap_t caplst      = si.gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PMad_t madlst      = si.gs->MadList;
  size_t madlst_size = MADLST_COUNT;

  Mesh_t     * pmesh = Game_getMesh(si.gs);

  int tnc;
  Uint32 loc_rand;

  AI_STATE * pstate;
  Chr_t  * pchr;
  Obj_t  * pobj;
  Cap_t  * pcap;
  Mad_t  * pmad;

  if( !RESERVED_CHR(chrlst, si.ichr) ) return INVALID_CHR;
  pchr   = chrlst + si.ichr;
  pchr->reserved = bfalse;     // remove the reserved tag immediately

  pobj = ObjList_getPObj(si.gs, si.iobj);
  if( NULL == pobj )
  {
    log_debug( "WARNING: req_chr_spawn_one() - failed to spawn : profile %d doesn't exist\n", REF_TO_INT(si.iobj) );
    return INVALID_CHR;
  }

  pcap = ObjList_getPCap(si.gs, si.iobj);
  if( NULL == pcap )
  {
    log_debug( "WARNING: req_chr_spawn_one() - failed to spawn : character profile (cap) doesn't exist\n" );
    return INVALID_CHR;
  }

  pmad = ObjList_getPMad(si.gs, si.iobj);
  if( NULL == pmad )
  {
    log_debug( "WARNING: req_chr_spawn_one() - failed to spawn : character profile (mad) doesn't exist\n" );
    return INVALID_CHR;
  }

  log_debug( "req_chr_spawn_one() - profile == %d, classname == \"%s\", index == %d\n", si.iobj, pcap->classname, si.ichr );

  // use Chr_new() to do a lot of the raw initialization
  // any values that are bfalse, 0, or NULL can be skipped
  pchr = Chr_new(pchr);
  if(NULL == pchr) return INVALID_CHR;

  // simplify access to the aistate info
  pstate = &(pchr->aistate);

  // initialize the ai state
  ai_state_init( si.gs, pstate, si.ichr);

  // copy the spawn info for any "respawn" command
  pchr->spinfo = si;

  // copy the seed value
  loc_rand = si.seed;

  // IMPORTANT!!!
  pchr->missilehandler = si.ichr;

  // Set up model stuff
  pchr->model = si.iobj;
  pchr->model_base = si.iobj;
  VData_Blended_Allocate( &(pchr->vdata), md2_get_numVertices(pmad->md2_ptr) );

  // copy over all of the properties (flags, abilities, etc.)
  pchr->prop = pcap->prop;

  pchr->stoppedby = pcap->stoppedby;
  pchr->manacost_fp8 = pcap->manacost_fp8;
  pchr->ammoknown = pchr->prop.nameknown;
  pchr->hitready = btrue;

  // generate the character's stats
  chr_create_stats( &loc_rand, &(pchr->stats), &(pcap->statdata) );

  // Kurse state
  pchr->prop.iskursed = ( RAND(&loc_rand, 0, 100) <= pcap->kursechance );
  if ( !pchr->prop.isitem )  pchr->prop.iskursed = bfalse;

  // Ammo
  pchr->ammomax = pcap->ammomax;
  pchr->ammo    = pcap->ammo;

  // Gender
  pchr->gender = pcap->gender;
  if ( pchr->gender == GEN_RANDOM )  pchr->gender = (GENDER)(GEN_FEMALE + IRAND(&loc_rand, 1));

  // Team stuff
  if ( si.iteam >= TEAM_COUNT ) si.iteam %= TEAM_COUNT;     // Make sure the si.iteam is valid
  pchr->team      = si.iteam;
  pchr->team_base = si.iteam;
  pchr->team_rank = si.gs->TeamList[si.iteam].morale++;

  // Firstborn becomes the leader
  if ( !PENDING_CHR( chrlst, team_get_leader( si.gs, si.iteam ) ) )
  {
    si.gs->TeamList[si.iteam].leader = si.ichr;
  }

  // Skin
  if ( pcap->skinoverride != NOSKINOVERRIDE )
  {
    si.iskin = pcap->skinoverride % MAXSKIN;
  }

  if ( si.iskin >= pobj->skins )
  {
    si.iskin = 0;
    if ( pobj->skins > 1 )
    {
      si.iskin = RAND(&loc_rand, 0, pobj->skins);
    }
  }
  pchr->skin_ref = si.iskin;

  // Life and Mana
  pchr->alive = btrue;
  pchr->lifecolor = pcap->lifecolor;
  pchr->manacolor = pcap->manacolor;

  // Damage
  pchr->skin.defense_fp8 = pcap->skin[si.iskin].defense_fp8;
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
  pchr->damagetargettype = pcap->damagetargettype;
  tnc = 0;
  while ( tnc < MAXDAMAGETYPE )
  {
    pchr->skin.damagemodifier_fp8[tnc] = pcap->skin[si.iskin].damagemodifier_fp8[tnc];
    tnc++;
  }

  // Jumping
  pchr->jump = pcap->jump;
  pchr->jumpnumberreset = pcap->jumpnumber;

  // Other junk
  pchr->flyheight     = pcap->flyheight;
  pchr->skin.maxaccel = pcap->skin[si.iskin].maxaccel;
  pchr->alpha_fp8     = pcap->alpha_fp8;
  pchr->light_fp8     = pcap->light_fp8;
  pchr->flashand      = pcap->flashand;
  pchr->sheen_fp8     = pcap->sheen_fp8;
  pchr->dampen        = pcap->dampen;

  // Character size and bumping
  pchr->fat      = pcap->size;
  pchr->sizegoto = pchr->fat;

  pchr->bmpdata_save.shadow  = pcap->shadowsize;
  pchr->bmpdata_save.size    = pcap->bumpsize;
  pchr->bmpdata_save.sizebig = pcap->bumpsizebig;
  pchr->bmpdata_save.height  = pcap->bumpheight;

  pchr->bmpdata.shadow   = pcap->shadowsize  * pchr->fat;
  pchr->bmpdata.size     = pcap->bumpsize    * pchr->fat;
  pchr->bmpdata.sizebig  = pcap->bumpsizebig * pchr->fat;
  pchr->bmpdata.height   = pcap->bumpheight  * pchr->fat;
  pchr->bumpstrength     = pcap->bumpstrength * FP8_TO_FLOAT( pcap->alpha_fp8 );

  pchr->bumpdampen = pcap->bumpdampen;
  pchr->weight     = pcap->weight * pchr->fat * pchr->fat * pchr->fat;   // preserve density

  // Image rendering
  pchr->uoffvel = pcap->uoffvel;
  pchr->voffvel = pcap->voffvel;

  // Movement
  pchr->spd_sneak = pcap->spd_sneak;
  pchr->spd_walk  = pcap->spd_walk;
  pchr->spd_run   = pcap->spd_run;

  // Set up position
  pchr->spinfo.pos  = si.pos;
  pchr->ori.pos.x   = si.pos.x;
  pchr->ori.pos.y   = si.pos.y;
  pchr->ori.turn_lr = si.facing;
  pchr->onwhichfan = mesh_get_fan( pmesh, pchr->ori.pos );
  pchr->level = mesh_get_level( &(pmesh->Mem), pchr->onwhichfan, pchr->ori.pos.x, pchr->ori.pos.y, pchr->prop.waterwalk, &(si.gs->water) ) + RAISE;
  if ( si.pos.z < pchr->level ) si.pos.z = pchr->level;
  pchr->ori.pos.z = si.pos.z;

  pchr->spinfo.pos  = pchr->ori.pos;
  pchr->ori_old.pos = pchr->ori.pos;
  pchr->ori_old.turn_lr = pchr->ori.turn_lr;

  pchr->scale = pchr->fat;

  // AI stuff
  ai_state_init(si.gs, pstate, si.ichr);

  // Money is added later
  pchr->money = pcap->money;

  // Name the character
  if ( EMPTY_CSTR(si.name) )
  {
    // Generate a random name
    strncpy( pchr->name, naming_generate( si.gs, pobj ), sizeof( MAXCAPNAMESIZE ) );
  }
  else
  {
    // A name has been given
    strncpy( pchr->name, si.name, sizeof( MAXCAPNAMESIZE ) );
  }

  // Particle attachments
  for ( tnc = 0; tnc < pcap->attachedprtamount; tnc++ )
  {
    PRT_SPAWN_INFO tmp_si;

    prt_spawn_info_init( &tmp_si, si.gs, 1.0f, pchr->ori.pos, 0, pchr->model, pcap->attachedprttype,
                        si.ichr, (GRIP)(GRIP_LAST + tnc), pchr->team, si.ichr, tnc, INVALID_CHR );

    req_spawn_one_particle( tmp_si );
  }
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;

  // Experience
  if ( pcap->leveloverride != 0 )
  {
    while ( pchr->experiencelevel < pcap->leveloverride )
    {
      give_experience( si.gs, si.ichr, 100, XP_DIRECT );
    }
  }
  else
  {
    pchr->experience = generate_unsigned( &loc_rand, &pcap->experience );
    pchr->experiencelevel = calc_chr_level( si.gs, si.ichr );
  }

  // calculate the matrix
  make_one_character_matrix( chrlst, chrlst_size, pchr );

  // calculate a basic bounding box
  chr_calculate_bumpers(si.gs, pchr, 0);

  // if someone requested us to activate, set the character for automatic activation
  pchr->req_active = activate;

  return si.ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( Game_t * gs, CHR_REF ichr )
{
  // ZZ> This function respawns a character

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  CHR_REF item;
  OBJ_REF profile;
  Chr_t * pchr;
  Cap_t * pcap;
  AI_STATE * pstate;
  Uint32 loc_rand;

  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return;

  pstate = &(pchr->aistate);

  if( pchr->alive ) return;

  loc_rand = pchr->spinfo.seed;

  profile = pchr->model;
  pcap = ChrList_getPCap(gs, ichr);

  spawn_poof( gs, ichr, profile );
  disaffirm_attached_particles( gs, ichr );
  pchr->alive = btrue;
  pchr->boretime = DELAY_BORE(&loc_rand);
  pchr->carefultime = DELAY_CAREFUL;
  pchr->stats.life_fp8 = pchr->stats.lifemax_fp8;
  pchr->stats.mana_fp8 = pchr->stats.manamax_fp8;
  pchr->ori.pos = pchr->spinfo.pos;
  VectorClear(pchr->ori.vel.v);
  pchr->team = pchr->team_base;
  pchr->prop.canbecrushed = bfalse;
  pchr->ori.mapturn_lr = 32768;  // These two mean on level surface
  pchr->ori.mapturn_ud = 32768;
  if ( !ACTIVE_CHR( chrlst, team_get_leader( gs, pchr->team ) ) )  gs->TeamList[pchr->team].leader = ichr;
  if ( !pchr->prop.invictus )  gs->TeamList[pchr->team_base].morale++;

  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  pchr->prop.isplatform = pcap->prop.isplatform;
  pchr->flyheight  = pcap->flyheight;
  pchr->bumpdampen = pcap->bumpdampen;

  pchr->bmpdata_save.size    = pcap->bumpsize;
  pchr->bmpdata_save.sizebig = pcap->bumpsizebig;
  pchr->bmpdata_save.height  = pcap->bumpheight;

  pchr->bmpdata.size     = pcap->bumpsize * pchr->fat;
  pchr->bmpdata.sizebig  = pcap->bumpsizebig * pchr->fat;
  pchr->bmpdata.height   = pcap->bumpheight * pchr->fat;
  pchr->bumpstrength     = pcap->bumpstrength * FP8_TO_FLOAT( pcap->alpha_fp8 );

  // clear the alert and leave the state alone
  ai_state_reinit(pstate, ichr);

  pchr->grogtime = 0.0f;
  pchr->dazetime = 0.0f;

  reaffirm_attached_particles( gs, ichr );

  // Let worn items come back
  item  = chr_get_nextinpack( chrlst, chrlst_size, ichr );
  while ( ACTIVE_CHR( chrlst, item ) )
  {
    if ( chrlst[item].isequipped )
    {
      chrlst[item].isequipped = bfalse;
      chrlst[item].aistate.alert |= ALERT_ATLASTWAYPOINT;  // doubles as PutAway
    }
    item  = chr_get_nextinpack( chrlst, chrlst_size, item );
  }

}

//--------------------------------------------------------------------------------------------
void signal_target( Game_t * gs, Uint32 priority, CHR_REF target_ref, Uint16 upper, Uint16 lower )
{
  PChr_t chrlst      = gs->ChrList;
  Chr_t * ptarget;

  if ( !ACTIVE_CHR( chrlst,  target_ref ) ) return;
  ptarget = chrlst + target_ref;

  // do a little kludge so that unimportant messages will not overwrite existing messages
  if(!HAS_SOME_BITS(ptarget->aistate.alert, ALERT_SIGNALED) || priority < ptarget->message.type)
  {
    ptarget->message.data = ( upper << 16 ) | lower;
    ptarget->message.type = priority;
    ptarget->aistate.alert |= ALERT_SIGNALED;
  }
};


//--------------------------------------------------------------------------------------------
void signal_team( Game_t * gs, CHR_REF chr_ref, Uint32 message )
{
  // ZZ> This function issues an message for help to all teammates

  PChr_t chrlst        = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  TEAM_REF team;
  Uint8 counter;
  CHR_REF chr_cnt;
  Chr_t *pchr, * ptarget;

  if(!ACTIVE_CHR(gs->ChrList, chr_ref)) return;
  pchr = chrlst + chr_ref;

  team    = pchr->team;
  counter = pchr->team_rank;
  for ( chr_cnt = 0; chr_cnt < chrlst_size; chr_cnt++ )
  {
    if ( !ACTIVE_CHR( chrlst,  chr_cnt ) || chrlst[chr_cnt].team != team ) continue;
    ptarget = chrlst + chr_cnt;

    // do a little kludge so that unimportant messages will not overwrite existing messages
    if(!HAS_SOME_BITS(ptarget->aistate.alert, ALERT_SIGNALED) || pchr->team_rank < ptarget->team_rank)
    {
      ptarget->message.data   = message;
      ptarget->message.type   = pchr->team_rank;
      ptarget->team_rank      = counter;
      ptarget->aistate.alert |= ALERT_SIGNALED;
      counter++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void signal_idsz_index( Game_t * gs, Uint32 priority, Uint32 data, IDSZ idsz, IDSZ_INDEX index )
{
  // ZZ> This function issues an data to all characters with the a matching special IDSZ

  PCap_t caplst      = gs->CapList;
  size_t caplst_size = CAPLST_COUNT;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  Uint8 counter;
  CHR_REF chr_cnt;
  Cap_t * pcap;
  Chr_t * ptarget;

  counter = priority;
  for ( chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++ )
  {
    if ( !ACTIVE_CHR( chrlst,  chr_cnt ) ) continue;
    ptarget = chrlst + chr_cnt;

    pcap = ChrList_getPCap(gs, chr_cnt);
    if(NULL == pcap) continue;

    if ( index>=IDSZ_COUNT || pcap->idsz[index] != idsz ) continue;

    // do a little kludge so that unimportant messages will not overwrite existing messages
    if(!HAS_SOME_BITS(ptarget->aistate.alert, ALERT_SIGNALED) || priority < ptarget->team_rank)
    {
      ptarget->message.data   = data;
      ptarget->message.type   = priority;
      ptarget->team_rank      = counter;
      ptarget->aistate.alert |= ALERT_SIGNALED;
      counter++;
    }
  }
}


//--------------------------------------------------------------------------------------------
bool_t ai_state_advance_wp(AI_STATE * a, bool_t do_atlastwaypoint)
{
  if(NULL == a || wp_list_empty(&(a->wp)) ) return bfalse;

  a->alert |= ALERT_ATWAYPOINT;

  if( wp_list_advance( &(a->wp) ) )
  {
    // waypoint list is at or past its end
    if ( do_atlastwaypoint )
    {
      a->alert |= ALERT_ATLASTWAYPOINT;
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Cap_t * Cap_new(Cap_t *pcap)
{
  //fprintf( stdout, "Cap_new()\n");

  if(NULL == pcap) return pcap;

  Cap_delete( pcap );

  memset(pcap, 0, sizeof(Cap_t));

  EKEY_PNEW( pcap, Cap_t );

  CProperties_new( &(pcap->prop) );

  pcap->hidestate = NOHIDE;
  pcap->skinoverride = NOSKINOVERRIDE;

  //pcap->skindressy = 0;
  //pcap->money = 0;
  //pcap->contentoverride = 0;
  //pcap->stateoverride = 0;
  //pcap->leveloverride = 0;

  return pcap;
};

//--------------------------------------------------------------------------------------------
bool_t Cap_delete( Cap_t * pcap )
{
  if(NULL == pcap) return bfalse;
  if(!EKEY_PVALID(pcap))  return btrue;

  EKEY_PINVALIDATE( pcap );

  return btrue;
}

//--------------------------------------------------------------------------------------------
Cap_t * Cap_renew(Cap_t *pcap)
{
  Cap_delete( pcap );
  return Cap_new(pcap);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ChrEnviro_t * CChrEnviro_new( ChrEnviro_t * enviro, PhysicsData_t * gphys)
{
  if(NULL == enviro) return enviro;

  CChrEnviro_delete(enviro);

  memset(enviro, 0, sizeof(ChrEnviro_t));

  EKEY_PNEW( enviro, ChrEnviro_t );

  if(NULL == gphys)
  {
    CPhysicsData_new( &(enviro->phys) );
  }
  else
  {
    memcpy( &(enviro->phys), gphys, sizeof(PhysicsData_t));
  };

  enviro->flydampen = FLYDAMPEN;

  enviro->twist = 127;
  enviro->nrm.z = 1.0f;

  enviro->air_traction = 0;

  return enviro;

};

//--------------------------------------------------------------------------------------------
bool_t CChrEnviro_delete( ChrEnviro_t * enviro )
{
  if(NULL == enviro) return bfalse;

  if( !EKEY_PVALID(enviro) ) return btrue;

  EKEY_PINVALIDATE(enviro);

  return btrue;
}

//--------------------------------------------------------------------------------------------
ChrEnviro_t * CChrEnviro_renew( ChrEnviro_t * enviro, PhysicsData_t * gphys)
{
  CChrEnviro_delete(enviro);
  return CChrEnviro_new( enviro, gphys);
}

//--------------------------------------------------------------------------------------------
bool_t CChrEnviro_init( ChrEnviro_t * enviro, float dUpdate)
{
  if(NULL == enviro) return bfalse;

  enviro->flydampen    = pow( FLYDAMPEN, dUpdate );

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t CChrEnviro_synch( ChrEnviro_t * enviro )
{
  if(NULL == enviro) return bfalse;

  enviro->air_traction = enviro->flying ? ( 1.0 - enviro->phys.airfriction ) : enviro->phys.airfriction;

  return btrue;
}


////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//bool_t triangle_clip_x(vect3 vrt_ary[], MD2_Triangle_t * ptri, CVolume_t * cv, float xpos, float xdir)
//{
//  // find the AA_BBox of the part of the triangle that is inside the half-plane
//  // returns bfalse if the triangle is completely outside the half-plane
//
//  int i,j;
//  vect3 * verts[3];
//  bool_t  flag[3] = {bfalse, bfalse, bfalse};
//  if(NULL == ptri || NULL == cv) return bfalse;
//
//  verts[0] = vrt_ary + ptri->vertexIndices[0];
//  verts[1] = vrt_ary + ptri->vertexIndices[1];
//  verts[2] = vrt_ary + ptri->vertexIndices[2];
//
//  for(i=0, j=0; i<3; i++)
//  {
//    if( (xdir>0 && verts[i]->x < xpos) || (xdir<0 && verts[i]->x > xpos) )
//    {
//      flag[i] = btrue;
//      j++;
//    }
//  }
//
//  // are any inside?
//  if(0 == j) return bfalse;
//
//  if(3 == j)
//  {
//    // all are inside
//    cv->x_min = MIN(MIN(verts[0]->x, verts[1]->x),verts[2]->x);
//    cv->x_max = MAX(MAX(verts[0]->x, verts[1]->x),verts[2]->x);
//
//    cv->y_min = MIN(MIN(verts[0]->y, verts[1]->y),verts[2]->y);
//    cv->y_max = MAX(MAX(verts[0]->y, verts[1]->y),verts[2]->y);
//
//    cv->z_min = MIN(MIN(verts[0]->z, verts[1]->z),verts[2]->z);
//    cv->z_max = MAX(MAX(verts[0]->z, verts[1]->z),verts[2]->z);
//  }
//  else
//  {
//    int ind[3];
//    float ftmp;
//    vect3 pt;
//
//    if(1 == j)
//    {
//      // one inside. find intersection of lines with the given plane
//
//      for(i=0;i<3;i++)
//      {
//        if(flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
//      }
//
//      cv->x_min = cv->x_max = verts[ind[0]]->x;
//      cv->y_min = cv->y_max = verts[ind[0]]->y;
//      cv->z_min = cv->z_max = verts[ind[0]]->z;
//    }
//    else if (2 == j)
//    {
//      // two inside. find intersection of lines with the given plane
//
//      for(i=0;i<3;i++)
//      {
//        if(!flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
//      }
//
//      cv->x_min = MIN(verts[ind[1]]->x, verts[ind[2]]->x);
//      cv->x_max = MAX(verts[ind[1]]->x, verts[ind[2]]->x);
//
//      cv->y_min = MIN(verts[ind[1]]->y, verts[ind[2]]->y);
//      cv->y_max = MAX(verts[ind[1]]->y, verts[ind[2]]->y);
//
//      cv->x_min = MIN(verts[ind[1]]->z, verts[ind[2]]->z);
//      cv->x_max = MAX(verts[ind[1]]->z, verts[ind[2]]->z);
//    }
//
//    ftmp = (xpos - verts[ind[0]]->x) / (verts[ind[1]]->x - verts[ind[0]]->x);
//    pt.x = verts[ind[0]]->x + (verts[ind[1]]->x - verts[ind[0]]->x) * ftmp;
//    pt.y = verts[ind[0]]->y + (verts[ind[1]]->y - verts[ind[0]]->y) * ftmp;
//    pt.z = verts[ind[0]]->z + (verts[ind[1]]->z - verts[ind[0]]->z) * ftmp;
//
//    cv->x_min = MIN(cv->x_min, pt.x);
//    cv->x_max = MAX(cv->x_max, pt.x);
//
//    cv->y_min = MIN(cv->y_min, pt.y);
//    cv->y_max = MAX(cv->y_max, pt.y);
//
//    cv->x_min = MIN(cv->z_min, pt.z);
//    cv->x_max = MAX(cv->z_max, pt.z);
//
//    ftmp = (xpos - verts[ind[0]]->x) / (verts[ind[2]]->x - verts[ind[0]]->x);
//    pt.x = verts[ind[0]]->x + (verts[ind[2]]->x - verts[ind[0]]->x) * ftmp;
//    pt.y = verts[ind[0]]->y + (verts[ind[2]]->y - verts[ind[0]]->y) * ftmp;
//    pt.z = verts[ind[0]]->z + (verts[ind[2]]->z - verts[ind[0]]->z) * ftmp;
//
//    cv->x_min = MIN(cv->x_min, pt.x);
//    cv->x_max = MAX(cv->x_max, pt.x);
//
//    cv->y_min = MIN(cv->y_min, pt.y);
//    cv->y_max = MAX(cv->y_max, pt.y);
//
//    cv->x_min = MIN(cv->z_min, pt.z);
//    cv->x_max = MAX(cv->z_max, pt.z);
//  }
//
//
//  return btrue;
//}
////--------------------------------------------------------------------------------------------
//bool_t triangle_clip_y(vect3 vrt_ary[], MD2_Triangle_t * ptri, CVolume_t * cv, float ypos, float ydir)
//{
//  // find the AA_BBox of the part of the triangle that is inside the half-plane
//  // returns bfalse if the triangle is completely outside the half-plane
//
//  int i,j;
//  vect3 * verts[3];
//  bool_t  flag[3] = {bfalse, bfalse, bfalse};
//  if(NULL == ptri || NULL == cv) return bfalse;
//
//  verts[0] = vrt_ary + ptri->vertexIndices[0];
//  verts[1] = vrt_ary + ptri->vertexIndices[1];
//  verts[2] = vrt_ary + ptri->vertexIndices[2];
//
//  for(i=0, j=0; i<3; i++)
//  {
//    if( (ydir>0 && verts[i]->y < ypos) || (ydir<0 && verts[i]->y > ypos) )
//    {
//      flag[i] = btrue;
//      j++;
//    }
//  }
//
//  // are any inside?
//  if(0 == j) return bfalse;
//
//  if(3 == j)
//  {
//    // all are inside
//    cv->x_min = MIN(MIN(verts[0]->x, verts[1]->x),verts[2]->x);
//    cv->x_max = MAX(MAX(verts[0]->x, verts[1]->x),verts[2]->x);
//
//    cv->y_min = MIN(MIN(verts[0]->y, verts[1]->y),verts[2]->y);
//    cv->y_max = MAX(MAX(verts[0]->y, verts[1]->y),verts[2]->y);
//
//    cv->z_min = MIN(MIN(verts[0]->z, verts[1]->z),verts[2]->z);
//    cv->z_max = MAX(MAX(verts[0]->z, verts[1]->z),verts[2]->z);
//  }
//  else
//  {
//    int ind[3];
//    float ftmp;
//    vect3 pt;
//
//    if(1 == j)
//    {
//      // one inside. find intersection of lines with the given plane
//
//      for(i=0;i<3;i++)
//      {
//        if(flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
//      }
//
//      cv->x_min = cv->x_max = verts[ind[0]]->x;
//      cv->y_min = cv->y_max = verts[ind[0]]->y;
//      cv->z_min = cv->z_max = verts[ind[0]]->z;
//    }
//    else if (2 == j)
//    {
//      // two inside. find intersection of lines with the given plane
//
//      for(i=0;i<3;i++)
//      {
//        if(!flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
//      }
//
//      cv->x_min = MIN(verts[ind[1]]->x, verts[ind[2]]->x);
//      cv->x_max = MAX(verts[ind[1]]->x, verts[ind[2]]->x);
//
//      cv->y_min = MIN(verts[ind[1]]->y, verts[ind[2]]->y);
//      cv->y_max = MAX(verts[ind[1]]->y, verts[ind[2]]->y);
//
//      cv->x_min = MIN(verts[ind[1]]->z, verts[ind[2]]->z);
//      cv->x_max = MAX(verts[ind[1]]->z, verts[ind[2]]->z);
//    }
//
//    ftmp = (ypos - verts[ind[0]]->y) / (verts[ind[1]]->y - verts[ind[0]]->y);
//    pt.x = verts[ind[0]]->x + (verts[ind[1]]->x - verts[ind[0]]->x) * ftmp;
//    pt.y = verts[ind[0]]->y + (verts[ind[1]]->y - verts[ind[0]]->y) * ftmp;
//    pt.z = verts[ind[0]]->z + (verts[ind[1]]->z - verts[ind[0]]->z) * ftmp;
//
//    cv->x_min = MIN(cv->x_min, pt.x);
//    cv->x_max = MAX(cv->x_max, pt.x);
//
//    cv->y_min = MIN(cv->y_min, pt.y);
//    cv->y_max = MAX(cv->y_max, pt.y);
//
//    cv->x_min = MIN(cv->z_min, pt.z);
//    cv->x_max = MAX(cv->z_max, pt.z);
//
//    ftmp = (ypos - verts[ind[0]]->y) / (verts[ind[2]]->y - verts[ind[0]]->y);
//    pt.x = verts[ind[0]]->x + (verts[ind[2]]->x - verts[ind[0]]->x) * ftmp;
//    pt.y = verts[ind[0]]->y + (verts[ind[2]]->y - verts[ind[0]]->y) * ftmp;
//    pt.z = verts[ind[0]]->z + (verts[ind[2]]->z - verts[ind[0]]->z) * ftmp;
//
//    cv->x_min = MIN(cv->x_min, pt.x);
//    cv->x_max = MAX(cv->x_max, pt.x);
//
//    cv->y_min = MIN(cv->y_min, pt.y);
//    cv->y_max = MAX(cv->y_max, pt.y);
//
//    cv->x_min = MIN(cv->z_min, pt.z);
//    cv->x_max = MAX(cv->z_max, pt.z);
//  }
//
//
//  return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t triangle_clip_z(vect3 vrt_ary[], MD2_Triangle_t * ptri, CVolume_t * cv, float zpos, float zdir)
//{
//  // find the AA_BBox of the part of the triangle that is inside the half-plane
//  // returns bfalse if the triangle is completely outside the half-plane
//
//  int i,j;
//  vect3 * verts[3];
//  bool_t  flag[3] = {bfalse, bfalse, bfalse};
//  if(NULL == ptri || NULL == cv) return bfalse;
//
//  verts[0] = vrt_ary + ptri->vertexIndices[0];
//  verts[1] = vrt_ary + ptri->vertexIndices[1];
//  verts[2] = vrt_ary + ptri->vertexIndices[2];
//
//  for(i=0, j=0; i<3; i++)
//  {
//    if( (zdir>0 && verts[i]->z < zpos) || (zdir<0 && verts[i]->z > zpos) )
//    {
//      flag[i] = btrue;
//      j++;
//    }
//  }
//
//  // are any inside?
//  if(0 == j) return bfalse;
//
//  if(3 == j)
//  {
//    // all are inside
//    cv->x_min = MIN(MIN(verts[0]->x, verts[1]->x),verts[2]->x);
//    cv->x_max = MAX(MAX(verts[0]->x, verts[1]->x),verts[2]->x);
//
//    cv->y_min = MIN(MIN(verts[0]->y, verts[1]->y),verts[2]->y);
//    cv->y_max = MAX(MAX(verts[0]->y, verts[1]->y),verts[2]->y);
//
//    cv->z_min = MIN(MIN(verts[0]->z, verts[1]->z),verts[2]->z);
//    cv->z_max = MAX(MAX(verts[0]->z, verts[1]->z),verts[2]->z);
//  }
//  else
//  {
//    int ind[3];
//    float ftmp;
//    vect3 pt;
//
//    if(1 == j)
//    {
//      // one inside. find intersection of lines with the given plane
//
//      for(i=0;i<3;i++)
//      {
//        if(flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
//      }
//
//      cv->x_min = cv->x_max = verts[ind[0]]->x;
//      cv->y_min = cv->y_max = verts[ind[0]]->y;
//      cv->z_min = cv->z_max = verts[ind[0]]->z;
//    }
//    else if (2 == j)
//    {
//      // two inside. find intersection of lines with the given plane
//
//      for(i=0;i<3;i++)
//      {
//        if(!flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
//      }
//
//      cv->x_min = MIN(verts[ind[1]]->x, verts[ind[2]]->x);
//      cv->x_max = MAX(verts[ind[1]]->x, verts[ind[2]]->x);
//
//      cv->y_min = MIN(verts[ind[1]]->y, verts[ind[2]]->y);
//      cv->y_max = MAX(verts[ind[1]]->y, verts[ind[2]]->y);
//
//      cv->x_min = MIN(verts[ind[1]]->z, verts[ind[2]]->z);
//      cv->x_max = MAX(verts[ind[1]]->z, verts[ind[2]]->z);
//    }
//
//    ftmp = (zpos - verts[ind[0]]->z) / (verts[ind[1]]->z - verts[ind[0]]->z);
//    pt.x = verts[ind[0]]->x + (verts[ind[1]]->x - verts[ind[0]]->x) * ftmp;
//    pt.y = verts[ind[0]]->y + (verts[ind[1]]->y - verts[ind[0]]->y) * ftmp;
//    pt.z = verts[ind[0]]->z + (verts[ind[1]]->z - verts[ind[0]]->z) * ftmp;
//
//    cv->x_min = MIN(cv->x_min, pt.x);
//    cv->x_max = MAX(cv->x_max, pt.x);
//
//    cv->y_min = MIN(cv->y_min, pt.y);
//    cv->y_max = MAX(cv->y_max, pt.y);
//
//    cv->x_min = MIN(cv->z_min, pt.z);
//    cv->x_max = MAX(cv->z_max, pt.z);
//
//    ftmp = (zpos - verts[ind[0]]->z) / (verts[ind[2]]->z - verts[ind[0]]->z);
//    pt.x = verts[ind[0]]->x + (verts[ind[2]]->x - verts[ind[0]]->x) * ftmp;
//    pt.y = verts[ind[0]]->y + (verts[ind[2]]->y - verts[ind[0]]->y) * ftmp;
//    pt.z = verts[ind[0]]->z + (verts[ind[2]]->z - verts[ind[0]]->z) * ftmp;
//
//    cv->x_min = MIN(cv->x_min, pt.x);
//    cv->x_max = MAX(cv->x_max, pt.x);
//
//    cv->y_min = MIN(cv->y_min, pt.y);
//    cv->y_max = MAX(cv->y_max, pt.y);
//
//    cv->x_min = MIN(cv->z_min, pt.z);
//    cv->x_max = MAX(cv->z_max, pt.z);
//  }
//
//
//  return btrue;
//}
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//OBJ_REF ChrList_getRObj(Game_t * gs, CHR_REF ichr)
//{
//  Chr_t * pchr;
//
//  pchr = ChrList_getPChr(gs, ichr);
//  if(NULL == pchr) return INVALID_OBJ;
//
//  pchr->model = VALIDATE_OBJ(gs->ObjList, pchr->model);
//  return pchr->model;
//}
//
////--------------------------------------------------------------------------------------------
//CAP_REF ChrList_getRCap(Game_t * gs, CHR_REF ichr)
//{
//  Obj_t * pobj = ChrList_getPObj(gs, ichr);
//  if(NULL == pobj) return INVALID_CAP;
//
//  pobj->cap = VALIDATE_CAP(gs->CapList, pobj->cap);
//  return pobj->cap;
//}
//
////--------------------------------------------------------------------------------------------
//MAD_REF ChrList_getRMad(Game_t * gs, CHR_REF ichr)
//{
//  Obj_t * pobj = ChrList_getPObj(gs, ichr);
//  if(NULL == pobj) return INVALID_MAD;
//
//  pobj->mad = VALIDATE_MAD(gs->MadList, pobj->mad);
//  return pobj->mad;
//}
//
////--------------------------------------------------------------------------------------------
//PIP_REF ChrList_getRPip(Game_t * gs, CHR_REF ichr, int i)
//{
//  Obj_t * pobj = ChrList_getPObj(gs, ichr);
//  if(NULL == pobj) return INVALID_PIP;
//
//  pobj->prtpip[i] = VALIDATE_PIP(gs->PipList, pobj->prtpip[i]);
//  return pobj->prtpip[i];
//}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//Chr_t * ChrList_getPChr(Game_t * gs, CHR_REF ichr)
//{
//  if(!ACTIVE_CHR(gs->ChrList, ichr)) return NULL;
//
//  return gs->ChrList + ichr;
//}
//
////--------------------------------------------------------------------------------------------
//Obj_t * ChrList_getPObj(Game_t * gs, CHR_REF ichr)
//{
//  Chr_t * pchr;
//
//  pchr = ChrList_getPChr(gs, ichr);
//  if(NULL == pchr) return NULL;
//
//  pchr->model = VALIDATE_OBJ(gs->ObjList, pchr->model);
//  if(!VALID_OBJ(gs->ObjList, pchr->model)) return NULL;
//
//  return gs->ObjList + pchr->model;
//}
//
////--------------------------------------------------------------------------------------------
//Cap_t * ChrList_getPCap(Game_t * gs, CHR_REF ichr)
//{
//  OBJ_REF iobj;
//
//  iobj = ChrList_getRObj(gs, ichr);
//  if(INVALID_OBJ == iobj) return NULL;
//
//  return ObjList_getPCap(gs, iobj);
//}
//
////--------------------------------------------------------------------------------------------
//Mad_t * ChrList_getPMad(Game_t * gs, CHR_REF ichr)
//{
//  OBJ_REF iobj;
//
//  iobj = ChrList_getRObj(gs, ichr);
//  if(INVALID_OBJ == iobj) return NULL;
//
//  return ObjList_getPMad(gs, iobj);
//}
//
////--------------------------------------------------------------------------------------------
//Pip_t * ChrList_getPPip(Game_t * gs, CHR_REF ichr, int i)
//{
//  OBJ_REF iobj;
//
//  iobj = ChrList_getRObj(gs, ichr);
//  if(INVALID_OBJ == iobj) return NULL;
//
//  return ObjList_getPPip(gs, iobj, i);
//}

//--------------------------------------------------------------------------------------------
bool_t wp_list_prune(WP_LIST * wl)
{
  int i, i1, i2, loops;
  bool_t eliminate[MAXWAY];
  bool_t done;

  if(NULL == wl) return bfalse;

  i = (wl->tail + 1) % MAXWAY;
  if(i == wl->head)
  {
    // only one waypoint
    return bfalse;
  }

  loops = 0;
  done = bfalse;
  while(!done)
  {
    // determine which waypoints may be eliminated
    for(i=0; i<MAXWAY; i++)
    {
      eliminate[i] = bfalse;
    }

    i = i1 = i2 = wl->tail;
    while(i != wl->head)
    {
      if(i != i1 && i1 != i2)
      {
        vect2 midpoint;
        float diff;
        midpoint.x = 0.5f * (wl->pos[i].x + wl->pos[i2].x);
        midpoint.y = 0.5f * (wl->pos[i].y + wl->pos[i2].y);

        // use metropolis metric
        diff = ABS( midpoint.x - wl->pos[i1].x ) + ABS( midpoint.y - wl->pos[i1].y );

        eliminate[i1] = (diff < 1.0f);
      }

      i2 = i1;
      i1 = i;
      i = (i + 1) % MAXWAY;
    };

    // make sure long runs of waypoints aren't eliminated willy-nilly
    i = i1 = wl->tail;
    while(i != wl->head)
    {

      if(i != i1 && eliminate[i] && eliminate[i1])
      {
        eliminate[i1] = bfalse;
      }

      i1 = i;
      i = (i + 1) % MAXWAY;
    };

    // keep going if one waypoint is marked for elimination
    done = btrue;
    for(i=0; i<MAXWAY; i++)
    {
      if(eliminate[i1]) { done = bfalse; break; }
    }

    if(done) continue;

    // do the actual trimming of the waypoints
    i = wl->tail;
    while(i != wl->head)
    {
      i1 = ( i + 1 ) % MAXWAY;

      if(eliminate[i])
      {
        i2 = i;
        while(i1 != wl->head)
        {
          memcpy(wl->pos + i2, wl->pos + i1, sizeof(WAYPOINT));
          i2 = i1;
          i1 = (i1 + 1) % MAXWAY;
        }

        // tricky-ness
        wl->head = i2;
        if(i == wl->head) break;
      }

      i = (i + 1) % MAXWAY;
    };

    loops++;
  }

  return loops > 0;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//ChrEnviro_t * CChrEnviro_new( ChrEnviro_t * enviro, PhysicsData_t * gphys)
//{
//  if(NULL == enviro) return enviro;
//  if(EKEY_PVALID(enviro)) CChrEnviro_delete(enviro);
//
//  memset(enviro, 0, sizeof(ChrEnviro_t));
//
//  if(NULL == gphys)
//  {
//    CPhysicsData_new( &(enviro->phys) );
//  }
//  else
//  {
//    memcpy( &(enviro->phys), gphys, sizeof(PhysicsData_t));
//  };
//
//  enviro->flydampen = FLYDAMPEN;
//
//  enviro->twist = 127;
//  enviro->nrm.z = 1.0f;
//
//  enviro->air_traction = 0;
//
//  return enviro;
//
//};
//
////--------------------------------------------------------------------------------------------
//bool_t CChrEnviro_delete( ChrEnviro_t * enviro )
//{
//  if(NULL == enviro) return bfalse;
//
//  if(!EKEY_PVALID(enviro)) return btrue;
//
//  EKEY_PVALID(enviro) = bfalse;
//
//  return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//ChrEnviro_t * CChrEnviro_renew( ChrEnviro_t * enviro, PhysicsData_t * gphys)
//{
//  CChrEnviro_delete(enviro);
//  return CChrEnviro_new( enviro, gphys);
//}
//
////--------------------------------------------------------------------------------------------
//bool_t CChrEnviro_init( ChrEnviro_t * enviro, float dUpdate)
//{
//  if(NULL == enviro) return bfalse;
//
//  enviro->flydampen    = pow( FLYDAMPEN, dUpdate );
//
//  return btrue;
//};
//
////--------------------------------------------------------------------------------------------
//bool_t CChrEnviro_synch( ChrEnviro_t * enviro )
//{
//  if(NULL == enviro) return bfalse;
//
//  enviro->air_traction = enviro->flying ? ( 1.0 - enviro->phys.airfriction ) : enviro->phys.airfriction;
//
//  return btrue;
//}
//

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t triangle_clip_x(vect3 vrt_ary[], MD2_Triangle_t * ptri, CVolume_t * cv, float xpos, float xdir)
{
  // find the AA_BBox of the part of the triangle that is inside the half-plane
  // returns bfalse if the triangle is completely outside the half-plane

  int i,j;
  vect3 * verts[3];
  bool_t  flag[3] = {bfalse, bfalse, bfalse};
  if(NULL == ptri || NULL == cv) return bfalse;

  verts[0] = vrt_ary + ptri->vertexIndices[0];
  verts[1] = vrt_ary + ptri->vertexIndices[1];
  verts[2] = vrt_ary + ptri->vertexIndices[2];

  for(i=0, j=0; i<3; i++)
  {
    if( (xdir>0 && verts[i]->x < xpos) || (xdir<0 && verts[i]->x > xpos) )
    {
      flag[i] = btrue;
      j++;
    }
  }

  // are any inside?
  if(0 == j) return bfalse;

  if(3 == j)
  {
    // all are inside
    cv->x_min = MIN(MIN(verts[0]->x, verts[1]->x),verts[2]->x);
    cv->x_max = MAX(MAX(verts[0]->x, verts[1]->x),verts[2]->x);

    cv->y_min = MIN(MIN(verts[0]->y, verts[1]->y),verts[2]->y);
    cv->y_max = MAX(MAX(verts[0]->y, verts[1]->y),verts[2]->y);

    cv->z_min = MIN(MIN(verts[0]->z, verts[1]->z),verts[2]->z);
    cv->z_max = MAX(MAX(verts[0]->z, verts[1]->z),verts[2]->z);
  }
  else
  {
    int ind[3];
    float ftmp;
    vect3 pt;

    if(1 == j)
    {
      // one inside. find intersection of lines with the given plane

      for(i=0;i<3;i++)
      {
        if(flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
      }

      cv->x_min = cv->x_max = verts[ind[0]]->x;
      cv->y_min = cv->y_max = verts[ind[0]]->y;
      cv->z_min = cv->z_max = verts[ind[0]]->z;
    }
    else if (2 == j)
    {
      // two inside. find intersection of lines with the given plane

      for(i=0;i<3;i++)
      {
        if(!flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
      }

      cv->x_min = MIN(verts[ind[1]]->x, verts[ind[2]]->x);
      cv->x_max = MAX(verts[ind[1]]->x, verts[ind[2]]->x);

      cv->y_min = MIN(verts[ind[1]]->y, verts[ind[2]]->y);
      cv->y_max = MAX(verts[ind[1]]->y, verts[ind[2]]->y);

      cv->x_min = MIN(verts[ind[1]]->z, verts[ind[2]]->z);
      cv->x_max = MAX(verts[ind[1]]->z, verts[ind[2]]->z);
    }

    ftmp = (xpos - verts[ind[0]]->x) / (verts[ind[1]]->x - verts[ind[0]]->x);
    pt.x = verts[ind[0]]->x + (verts[ind[1]]->x - verts[ind[0]]->x) * ftmp;
    pt.y = verts[ind[0]]->y + (verts[ind[1]]->y - verts[ind[0]]->y) * ftmp;
    pt.z = verts[ind[0]]->z + (verts[ind[1]]->z - verts[ind[0]]->z) * ftmp;

    cv->x_min = MIN(cv->x_min, pt.x);
    cv->x_max = MAX(cv->x_max, pt.x);

    cv->y_min = MIN(cv->y_min, pt.y);
    cv->y_max = MAX(cv->y_max, pt.y);

    cv->x_min = MIN(cv->z_min, pt.z);
    cv->x_max = MAX(cv->z_max, pt.z);

    ftmp = (xpos - verts[ind[0]]->x) / (verts[ind[2]]->x - verts[ind[0]]->x);
    pt.x = verts[ind[0]]->x + (verts[ind[2]]->x - verts[ind[0]]->x) * ftmp;
    pt.y = verts[ind[0]]->y + (verts[ind[2]]->y - verts[ind[0]]->y) * ftmp;
    pt.z = verts[ind[0]]->z + (verts[ind[2]]->z - verts[ind[0]]->z) * ftmp;

    cv->x_min = MIN(cv->x_min, pt.x);
    cv->x_max = MAX(cv->x_max, pt.x);

    cv->y_min = MIN(cv->y_min, pt.y);
    cv->y_max = MAX(cv->y_max, pt.y);

    cv->x_min = MIN(cv->z_min, pt.z);
    cv->x_max = MAX(cv->z_max, pt.z);
  }


  return btrue;
}
//--------------------------------------------------------------------------------------------
bool_t triangle_clip_y(vect3 vrt_ary[], MD2_Triangle_t * ptri, CVolume_t * cv, float ypos, float ydir)
{
  // find the AA_BBox of the part of the triangle that is inside the half-plane
  // returns bfalse if the triangle is completely outside the half-plane

  int i,j;
  vect3 * verts[3];
  bool_t  flag[3] = {bfalse, bfalse, bfalse};
  if(NULL == ptri || NULL == cv) return bfalse;

  verts[0] = vrt_ary + ptri->vertexIndices[0];
  verts[1] = vrt_ary + ptri->vertexIndices[1];
  verts[2] = vrt_ary + ptri->vertexIndices[2];

  for(i=0, j=0; i<3; i++)
  {
    if( (ydir>0 && verts[i]->y < ypos) || (ydir<0 && verts[i]->y > ypos) )
    {
      flag[i] = btrue;
      j++;
    }
  }

  // are any inside?
  if(0 == j) return bfalse;

  if(3 == j)
  {
    // all are inside
    cv->x_min = MIN(MIN(verts[0]->x, verts[1]->x),verts[2]->x);
    cv->x_max = MAX(MAX(verts[0]->x, verts[1]->x),verts[2]->x);

    cv->y_min = MIN(MIN(verts[0]->y, verts[1]->y),verts[2]->y);
    cv->y_max = MAX(MAX(verts[0]->y, verts[1]->y),verts[2]->y);

    cv->z_min = MIN(MIN(verts[0]->z, verts[1]->z),verts[2]->z);
    cv->z_max = MAX(MAX(verts[0]->z, verts[1]->z),verts[2]->z);
  }
  else
  {
    int ind[3];
    float ftmp;
    vect3 pt;

    if(1 == j)
    {
      // one inside. find intersection of lines with the given plane

      for(i=0;i<3;i++)
      {
        if(flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
      }

      cv->x_min = cv->x_max = verts[ind[0]]->x;
      cv->y_min = cv->y_max = verts[ind[0]]->y;
      cv->z_min = cv->z_max = verts[ind[0]]->z;
    }
    else if (2 == j)
    {
      // two inside. find intersection of lines with the given plane

      for(i=0;i<3;i++)
      {
        if(!flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
      }

      cv->x_min = MIN(verts[ind[1]]->x, verts[ind[2]]->x);
      cv->x_max = MAX(verts[ind[1]]->x, verts[ind[2]]->x);

      cv->y_min = MIN(verts[ind[1]]->y, verts[ind[2]]->y);
      cv->y_max = MAX(verts[ind[1]]->y, verts[ind[2]]->y);

      cv->x_min = MIN(verts[ind[1]]->z, verts[ind[2]]->z);
      cv->x_max = MAX(verts[ind[1]]->z, verts[ind[2]]->z);
    }

    ftmp = (ypos - verts[ind[0]]->y) / (verts[ind[1]]->y - verts[ind[0]]->y);
    pt.x = verts[ind[0]]->x + (verts[ind[1]]->x - verts[ind[0]]->x) * ftmp;
    pt.y = verts[ind[0]]->y + (verts[ind[1]]->y - verts[ind[0]]->y) * ftmp;
    pt.z = verts[ind[0]]->z + (verts[ind[1]]->z - verts[ind[0]]->z) * ftmp;

    cv->x_min = MIN(cv->x_min, pt.x);
    cv->x_max = MAX(cv->x_max, pt.x);

    cv->y_min = MIN(cv->y_min, pt.y);
    cv->y_max = MAX(cv->y_max, pt.y);

    cv->x_min = MIN(cv->z_min, pt.z);
    cv->x_max = MAX(cv->z_max, pt.z);

    ftmp = (ypos - verts[ind[0]]->y) / (verts[ind[2]]->y - verts[ind[0]]->y);
    pt.x = verts[ind[0]]->x + (verts[ind[2]]->x - verts[ind[0]]->x) * ftmp;
    pt.y = verts[ind[0]]->y + (verts[ind[2]]->y - verts[ind[0]]->y) * ftmp;
    pt.z = verts[ind[0]]->z + (verts[ind[2]]->z - verts[ind[0]]->z) * ftmp;

    cv->x_min = MIN(cv->x_min, pt.x);
    cv->x_max = MAX(cv->x_max, pt.x);

    cv->y_min = MIN(cv->y_min, pt.y);
    cv->y_max = MAX(cv->y_max, pt.y);

    cv->x_min = MIN(cv->z_min, pt.z);
    cv->x_max = MAX(cv->z_max, pt.z);
  }


  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t triangle_clip_z(vect3 vrt_ary[], MD2_Triangle_t * ptri, CVolume_t * cv, float zpos, float zdir)
{
  // find the AA_BBox of the part of the triangle that is inside the half-plane
  // returns bfalse if the triangle is completely outside the half-plane

  int i,j;
  vect3 * verts[3];
  bool_t  flag[3] = {bfalse, bfalse, bfalse};
  if(NULL == ptri || NULL == cv) return bfalse;

  verts[0] = vrt_ary + ptri->vertexIndices[0];
  verts[1] = vrt_ary + ptri->vertexIndices[1];
  verts[2] = vrt_ary + ptri->vertexIndices[2];

  for(i=0, j=0; i<3; i++)
  {
    if( (zdir>0 && verts[i]->z < zpos) || (zdir<0 && verts[i]->z > zpos) )
    {
      flag[i] = btrue;
      j++;
    }
  }

  // are any inside?
  if(0 == j) return bfalse;

  if(3 == j)
  {
    // all are inside
    cv->x_min = MIN(MIN(verts[0]->x, verts[1]->x),verts[2]->x);
    cv->x_max = MAX(MAX(verts[0]->x, verts[1]->x),verts[2]->x);

    cv->y_min = MIN(MIN(verts[0]->y, verts[1]->y),verts[2]->y);
    cv->y_max = MAX(MAX(verts[0]->y, verts[1]->y),verts[2]->y);

    cv->z_min = MIN(MIN(verts[0]->z, verts[1]->z),verts[2]->z);
    cv->z_max = MAX(MAX(verts[0]->z, verts[1]->z),verts[2]->z);
  }
  else
  {
    int ind[3];
    float ftmp;
    vect3 pt;

    if(1 == j)
    {
      // one inside. find intersection of lines with the given plane

      for(i=0;i<3;i++)
      {
        if(flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
      }

      cv->x_min = cv->x_max = verts[ind[0]]->x;
      cv->y_min = cv->y_max = verts[ind[0]]->y;
      cv->z_min = cv->z_max = verts[ind[0]]->z;
    }
    else if (2 == j)
    {
      // two inside. find intersection of lines with the given plane

      for(i=0;i<3;i++)
      {
        if(!flag[i]) {ind[0] = i; ind[1] = (ind[0] + 1)%3; ind[2] = (ind[1] + 1)%3; break;}
      }

      cv->x_min = MIN(verts[ind[1]]->x, verts[ind[2]]->x);
      cv->x_max = MAX(verts[ind[1]]->x, verts[ind[2]]->x);

      cv->y_min = MIN(verts[ind[1]]->y, verts[ind[2]]->y);
      cv->y_max = MAX(verts[ind[1]]->y, verts[ind[2]]->y);

      cv->x_min = MIN(verts[ind[1]]->z, verts[ind[2]]->z);
      cv->x_max = MAX(verts[ind[1]]->z, verts[ind[2]]->z);
    }

    ftmp = (zpos - verts[ind[0]]->z) / (verts[ind[1]]->z - verts[ind[0]]->z);
    pt.x = verts[ind[0]]->x + (verts[ind[1]]->x - verts[ind[0]]->x) * ftmp;
    pt.y = verts[ind[0]]->y + (verts[ind[1]]->y - verts[ind[0]]->y) * ftmp;
    pt.z = verts[ind[0]]->z + (verts[ind[1]]->z - verts[ind[0]]->z) * ftmp;

    cv->x_min = MIN(cv->x_min, pt.x);
    cv->x_max = MAX(cv->x_max, pt.x);

    cv->y_min = MIN(cv->y_min, pt.y);
    cv->y_max = MAX(cv->y_max, pt.y);

    cv->x_min = MIN(cv->z_min, pt.z);
    cv->x_max = MAX(cv->z_max, pt.z);

    ftmp = (zpos - verts[ind[0]]->z) / (verts[ind[2]]->z - verts[ind[0]]->z);
    pt.x = verts[ind[0]]->x + (verts[ind[2]]->x - verts[ind[0]]->x) * ftmp;
    pt.y = verts[ind[0]]->y + (verts[ind[2]]->y - verts[ind[0]]->y) * ftmp;
    pt.z = verts[ind[0]]->z + (verts[ind[2]]->z - verts[ind[0]]->z) * ftmp;

    cv->x_min = MIN(cv->x_min, pt.x);
    cv->x_max = MAX(cv->x_max, pt.x);

    cv->y_min = MIN(cv->y_min, pt.y);
    cv->y_max = MAX(cv->y_max, pt.y);

    cv->x_min = MIN(cv->z_min, pt.z);
    cv->x_max = MAX(cv->z_max, pt.z);
  }


  return btrue;
}
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
OBJ_REF ChrList_getRObj(Game_t * gs, CHR_REF ichr)
{
  Chr_t * pchr;

  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return INVALID_OBJ;

  pchr->model = VALIDATE_OBJ(gs->ObjList, pchr->model);
  return pchr->model;
}

//--------------------------------------------------------------------------------------------
CAP_REF ChrList_getRCap(Game_t * gs, CHR_REF ichr)
{
  Obj_t * pobj = ChrList_getPObj(gs, ichr);
  if(NULL == pobj) return INVALID_CAP;

  pobj->cap = VALIDATE_CAP(gs->CapList, pobj->cap);
  return pobj->cap;
}

//--------------------------------------------------------------------------------------------
MAD_REF ChrList_getRMad(Game_t * gs, CHR_REF ichr)
{
  Obj_t * pobj = ChrList_getPObj(gs, ichr);
  if(NULL == pobj) return INVALID_MAD;

  pobj->mad = VALIDATE_MAD(gs->MadList, pobj->mad);
  return pobj->mad;
}

//--------------------------------------------------------------------------------------------
PIP_REF ChrList_getRPip(Game_t * gs, CHR_REF ichr, int i)
{
  Obj_t * pobj = ChrList_getPObj(gs, ichr);
  if(NULL == pobj) return INVALID_PIP;

  pobj->prtpip[i] = VALIDATE_PIP(gs->PipList, pobj->prtpip[i]);
  return pobj->prtpip[i];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Chr_t * ChrList_getPChr(Game_t * gs, CHR_REF ichr)
{
  if(!VALID_CHR(gs->ChrList, ichr)) return NULL;

  return gs->ChrList + ichr;
}

//--------------------------------------------------------------------------------------------
Obj_t * ChrList_getPObj(Game_t * gs, CHR_REF ichr)
{
  Chr_t * pchr;

  pchr = ChrList_getPChr(gs, ichr);
  if(NULL == pchr) return NULL;

  pchr->model = VALIDATE_OBJ(gs->ObjList, pchr->model);
  if(!VALID_OBJ(gs->ObjList, pchr->model)) return NULL;

  return gs->ObjList + pchr->model;
}

//--------------------------------------------------------------------------------------------
Cap_t * ChrList_getPCap(Game_t * gs, CHR_REF ichr)
{
  OBJ_REF iobj;

  iobj = ChrList_getRObj(gs, ichr);
  if(INVALID_OBJ == iobj) return NULL;

  return ObjList_getPCap(gs, iobj);
}

//--------------------------------------------------------------------------------------------
Mad_t * ChrList_getPMad(Game_t * gs, CHR_REF ichr)
{
  OBJ_REF iobj;

  iobj = ChrList_getRObj(gs, ichr);
  if(INVALID_OBJ == iobj) return NULL;

  return ObjList_getPMad(gs, iobj);
}

//--------------------------------------------------------------------------------------------
Pip_t * ChrList_getPPip(Game_t * gs, CHR_REF ichr, int i)
{
  OBJ_REF iobj;

  iobj = ChrList_getRObj(gs, ichr);
  if(INVALID_OBJ == iobj) return NULL;

  return ObjList_getPPip(gs, iobj, i);
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_spawn_info_delete(CHR_SPAWN_INFO * psi)
{
  if(NULL == psi) return bfalse;
  if(!EKEY_PVALID(psi))  return btrue;

  EKEY_PINVALIDATE( psi );

  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_SPAWN_INFO * chr_spawn_info_new(CHR_SPAWN_INFO * psi, Game_t * gs )
{
  if(NULL == psi) return psi;

  // get rid of old info
  chr_spawn_info_delete(psi);

  // clear all data
  memset(psi, 0, sizeof(CHR_SPAWN_INFO));

  // get a new key
  EKEY_PNEW( psi, CHR_SPAWN_INFO );

  // make sure we know which Game_t we're connecting to
  if( !EKEY_PVALID(gs) ) { gs = gfxState.gs; };

  // set default values
  psi->gs   = gs;
  psi->seed = !EKEY_PVALID(gs) ? -time(NULL) : gs->randie_index;
  psi->ichr = INVALID_CHR;

  psi->iobj      = INVALID_OBJ;
  psi->iteam     = TEAM_REF(TEAM_NULL);
  psi->ioverride = INVALID_CHR;
  psi->passage   = INVALID_PASS;
  psi->slot      = SLOT_NONE;
  psi->facing    = NORTH;

  strcpy(psi->name, "*NONE*");

  return psi;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t CProperties_delete( Properties_t * p)
{
  if(NULL == p) return bfalse;
  if(!EKEY_PVALID(p)) return btrue;

  EKEY_PINVALIDATE( p );

  return btrue;
}

//--------------------------------------------------------------------------------------------
Properties_t * CProperties_new( Properties_t * p)
{
  if(NULL == p) return p;

  CProperties_delete( p );

  memset(p, 0, sizeof(Properties_t));

  EKEY_PNEW( p, Properties_t );

  CProperties_init(p);

  return p;
}

//--------------------------------------------------------------------------------------------
bool_t CProperties_init( Properties_t * p )
{
  if(NULL == p) return bfalse;

  // basic properties
  p->resistbumpspawn = bfalse;
  p->istoobig = bfalse;
  p->reflect = btrue;
  p->alwaysdraw = bfalse;
  p->isranged = bfalse;
  p->isequipment = bfalse;
  p->forceshadow = bfalse;
  p->canseeinvisible = bfalse;
  p->canchannel = bfalse;

  // Skill Expansions
  p->canseekurse = bfalse;
  p->canusearcane = bfalse;
  p->canjoust = bfalse;
  p->canusedivine = bfalse;
  p->candisarm = bfalse;
  p->canusetech = bfalse;
  p->canbackstab = bfalse;
  p->canusepoison = bfalse;
  p->canuseadvancedweapons = bfalse;

  // dependent properties
  p->canuseplatforms = !p->isplatform;
  p->icon = p->usageknown;

  return btrue;
}


//--------------------------------------------------------------------------------------------
CHR_REF force_chr_spawn( CHR_SPAWN_INFO si )
{
  return _chr_spawn( si, btrue );
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_SPAWN_QUEUE * chr_spawn_queue_new(CHR_SPAWN_QUEUE * q, int size)
{
  if(NULL == q || size < 0) return q;

  chr_spawn_queue_delete( q );

  EKEY_PNEW( q, CHR_SPAWN_QUEUE );

  // deallocate any previous data
  if(q->data_size>0) { FREE(q->data); q->data_size = 0; }

  // initialize the data
  memset(q, 0, sizeof(CHR_SPAWN_QUEUE));

  q->data = calloc(size, sizeof(CHR_SPAWN_INFO));
  if(NULL != q->data) q->data_size = size;

  return q;
}

//--------------------------------------------------------------------------------------------
bool_t chr_spawn_queue_delete(CHR_SPAWN_QUEUE * q)
{
  if(NULL == q) return bfalse;

  if(!EKEY_PVALID(q))  return btrue;

  EKEY_PINVALIDATE( q );

  // deallocate any previous data
  if(q->data_size>0) { FREE(q->data); q->data_size = 0; }

  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_SPAWN_INFO * chr_spawn_queue_pop(CHR_SPAWN_QUEUE * q)
{
  CHR_SPAWN_INFO * psi = NULL;

  if(NULL == q || q->data_size == 0 ) return psi;

  if(q->tail == q->head) return psi;

  psi = q->data + q->tail;
  q->tail = (q->tail + 1) % q->data_size;

  return psi;
}

//--------------------------------------------------------------------------------------------
bool_t chr_spawn_queue_push(CHR_SPAWN_QUEUE * q, CHR_SPAWN_INFO * psi )
{
  int tmp_head;

  if(NULL == q || q->data_size == 0 ) return bfalse;

  // check to make sure that there is room
  tmp_head = (q->head + 1) % q->data_size;
  if(tmp_head == q->tail) return bfalse;

  memcpy(q->data + q->head, psi, sizeof(CHR_SPAWN_INFO));
  q->head = tmp_head;

  return btrue;
}
