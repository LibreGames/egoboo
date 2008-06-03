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
#include "Mesh.h"
#include "graphic.h"

#include "egoboo_strutil.h"
#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>

#include "object.inl"
#include "input.inl"
#include "particle.inl"
#include "Md2.inl"
#include "mesh.inl"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SLOT    _slot;

Uint16  numdolist = 0;
Uint16  dolist[MAXCHR];

Uint16  chrcollisionlevel = 2;

TILE_DAMAGE GTile_Dam;

//--------------------------------------------------------------------------------------------
Uint32  cv_list_count = 0;
CVolume cv_list[1000];

INLINE void cv_list_add( CVolume * cv);
INLINE void cv_list_clear();
INLINE void cv_list_draw();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void flash_character_height( CGame * gs, CHR_REF chr_ref, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
  // ZZ> This function sets a chr_ref's lighting depending on vertex height...
  //     Can make feet dark and head light...

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  int cnt;
  Uint16 model;
  float z, flip;

  Uint32 ilast, inext;
  MD2_Model * pmdl;
  MD2_Frame * plast, * pnext;

  model = chrlst[chr_ref].model;
  inext = chrlst[chr_ref].anim.next;
  ilast = chrlst[chr_ref].anim.last;
  flip = chrlst[chr_ref].anim.flip;

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  pmdl  = madlst[model].md2_ptr;
  plast = md2_get_Frame(pmdl, ilast);
  pnext = md2_get_Frame(pmdl, inext);

  for ( cnt = 0; cnt < madlst[model].transvertices; cnt++ )
  {
    z = pnext->vertices[cnt].z + (pnext->vertices[cnt].z - plast->vertices[cnt].z) * flip;

    if ( z < low )
    {
      chrlst[chr_ref].vrta_fp8[cnt].r =
      chrlst[chr_ref].vrta_fp8[cnt].g =
      chrlst[chr_ref].vrta_fp8[cnt].b = valuelow;
    }
    else if ( z > high )
    {
      chrlst[chr_ref].vrta_fp8[cnt].r =
      chrlst[chr_ref].vrta_fp8[cnt].g =
      chrlst[chr_ref].vrta_fp8[cnt].b = valuehigh;
    }
    else
    {
      float ftmp = (float)( z - low ) / (float)( high - low );
      chrlst[chr_ref].vrta_fp8[cnt].r =
      chrlst[chr_ref].vrta_fp8[cnt].g =
      chrlst[chr_ref].vrta_fp8[cnt].b = valuelow + (valuehigh - valuelow) * ftmp;
    }
  }
}

//--------------------------------------------------------------------------------------------
void flash_character( CGame * gs, CHR_REF chr_ref, Uint8 value )
{
  // ZZ> This function sets a chr_ref's lighting

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  int cnt;
  Uint16 model = chrlst[chr_ref].model;

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  cnt = 0;
  while ( cnt < madlst[model].transvertices )
  {
    chrlst[chr_ref].vrta_fp8[cnt].r =
    chrlst[chr_ref].vrta_fp8[cnt].g =
    chrlst[chr_ref].vrta_fp8[cnt].b = value;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders(CGame * gs)
{
  // ZZ> This function keeps weapons near their holders

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  int cnt;
  CHR_REF chr_ref;

  // !!!BAD!!!  May need to do 3 levels of attachment...

  for ( cnt = 0; cnt < chrlst_size; cnt++ )
  {
    if ( !VALID_CHR( chrlst, cnt ) ) continue;

    chr_ref = chr_get_attachedto( chrlst, MAXCHR, cnt );
    if ( !VALID_CHR( chrlst, chr_ref ) )
    {
      // Keep inventory with character
      if ( !chr_in_pack( chrlst, chrlst_size, cnt ) )
      {
        chr_ref = chr_get_nextinpack( chrlst, chrlst_size, cnt );
        while ( VALID_CHR( chrlst, chr_ref ) )
        {
          chrlst[chr_ref].pos = chrlst[cnt].pos;
          chrlst[chr_ref].pos_old = chrlst[cnt].pos_old;  // Copy olds to make SendMessageNear work
          chr_ref  = chr_get_nextinpack( chrlst, chrlst_size, chr_ref );
        }
      }
    }
    else
    {
      // Keep in hand weapons with character
      if ( chrlst[chr_ref].matrixvalid && chrlst[cnt].matrixvalid )
      {
        chrlst[cnt].pos.x = chrlst[cnt].matrix.CNV( 3, 0 );
        chrlst[cnt].pos.y = chrlst[cnt].matrix.CNV( 3, 1 );
        chrlst[cnt].pos.z = chrlst[cnt].matrix.CNV( 3, 2 );
      }
      else
      {
        chrlst[cnt].pos.x = chrlst[chr_ref].pos.x;
        chrlst[cnt].pos.y = chrlst[chr_ref].pos.y;
        chrlst[cnt].pos.z = chrlst[chr_ref].pos.z;
      }
      chrlst[cnt].turn_lr = chrlst[chr_ref].turn_lr;

      // Copy this stuff ONLY if it's a weapon, not for mounts
      if ( chrlst[chr_ref].transferblend && chrlst[cnt].isitem )
      {
        if ( chrlst[chr_ref].alpha_fp8 != 255 )
        {
          Uint16 model = chrlst[cnt].model;
          assert( MAXMODEL != VALIDATE_MDL( model ) );
          chrlst[cnt].alpha_fp8 = chrlst[chr_ref].alpha_fp8;
          chrlst[cnt].bumpstrength = caplst[model].bumpstrength * FP8_TO_FLOAT( chrlst[cnt].alpha_fp8 );
        }
        if ( chrlst[chr_ref].light_fp8 != 255 )
        {
          chrlst[cnt].light_fp8 = chrlst[chr_ref].light_fp8;
        }
      }
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t make_one_character_matrix( Chr chrlst[], size_t chrlst_size, Chr * pchr )
{
  // ZZ> This function sets one character's matrix

  Chr * povl;

  Uint16 tnc;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  if ( NULL == pchr || !pchr->on ) return bfalse;

  mat_old = pchr->matrix;
  pchr->matrixvalid = bfalse;

  if ( pchr->overlay )
  {
    // Overlays are kept with their target...
    tnc = chr_get_aitarget( chrlst, chrlst_size, pchr );

    if ( VALID_CHR( chrlst, tnc ) )
    {
      povl = chrlst + tnc;

      pchr->pos.x = povl->matrix.CNV( 3, 0 );
      pchr->pos.y = povl->matrix.CNV( 3, 1 );
      pchr->pos.z = povl->matrix.CNV( 3, 2 );

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

      pchr->matrixvalid = btrue;

      recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
    }
  }
  else
  {
    pchr->matrix = ScaleXYZRotateXYZTranslate( pchr->scale * pchr->pancakepos.x, pchr->scale * pchr->pancakepos.y, pchr->scale * pchr->pancakepos.z,
                     pchr->turn_lr,
                     ( Uint16 )( pchr->mapturn_ud + 32768 ),
                     ( Uint16 )( pchr->mapturn_lr + 32768 ),
                     pchr->pos.x, pchr->pos.y, pchr->pos.z );

    pchr->matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
  }

  //if(pchr->matrixvalid && recalc_bumper)
  //{
  //  // invalidate the cached bumper data
  //  pchr->bmpdata.cv.level = -1;
  //};

  if(pchr->matrixvalid && recalc_bumper)
  {
    // invalidate the cached bumper data
    pchr->bmpdata.cv.level = -1;
  };

  return pchr->matrixvalid;
}

//--------------------------------------------------------------------------------------------
void ChrList_free_one( CGame * gs, CHR_REF chr_ref )
{
  // ZZ> This function sticks a character back on the free character stack

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  int cnt;
  Chr * pchr;

  if ( !VALID_CHR( chrlst, chr_ref ) ) return;
  pchr = chrlst + chr_ref;

  log_debug( "ChrList_free_one() - \n\tprofile == %d, caplst[profile].classname == \"%s\", index == %d\n", chrlst[chr_ref].model, caplst[chrlst[chr_ref].model].classname, chr_ref );

  // Make sure everyone knows it died
  cnt = 0;
  while ( cnt < chrlst_size )
  {
    if ( chrlst[cnt].on )
    {
      if ( chrlst[cnt].aistate.target == chr_ref )
      {
        chrlst[cnt].aistate.alert |= ALERT_TARGETKILLED;
        chrlst[cnt].aistate.target = cnt;
      }
      if ( team_get_leader( gs, chrlst[cnt].team ) == chr_ref )
      {
        chrlst[cnt].aistate.alert |= ALERT_LEADERKILLED;
      }
    }
    cnt++;
  }

  // fix the team
  if ( pchr->alive && !caplst[pchr->model].invictus )
  {
    gs->TeamList[pchr->baseteam].morale--;
  }
  if ( team_get_leader( gs, pchr->team ) == chr_ref )
  {
    gs->TeamList[pchr->team].leader = MAXCHR;
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
void chr_free_inventory( Chr chrlst[], size_t chrlst_size, CHR_REF chr_ref )
{
  // ZZ> This function frees every item in the character's inventory

  int cnt;

  cnt  = chr_get_nextinpack( chrlst, chrlst_size, chr_ref );
  while ( cnt < chrlst_size )
  {
    chrlst[cnt].freeme = btrue;
    cnt = chr_get_nextinpack( chrlst, chrlst_size, cnt );
  }
}

//--------------------------------------------------------------------------------------------
bool_t make_one_weapon_matrix( Chr chrlst[], size_t chrlst_size, CHR_REF chr_ref )
{
  // ZZ> This function sets one weapon's matrix, based on who it's attached to

  int cnt;
  CHR_REF mount_ref;
  Uint16 vertex;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  // check this character
  if ( !VALID_CHR( chrlst, chr_ref ) )  return bfalse;

  // invalidate the matrix
  chrlst[chr_ref].matrixvalid = bfalse;

  // check that the mount is valid
  mount_ref = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
  if ( !VALID_CHR( chrlst, mount_ref ) )
  {
    chrlst[chr_ref].matrix = ZeroMatrix();
    return bfalse;
  }

  mat_old = chrlst[chr_ref].matrix;

  if(0xFFFF == chrlst[chr_ref].attachedgrip[0])
  {
    // Calculate weapon's matrix
    chrlst[chr_ref].matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, chrlst[chr_ref].turn_lr + chrlst[mount_ref].turn_lr, chrlst[mount_ref].pos.x, chrlst[mount_ref].pos.y, chrlst[mount_ref].pos.z);
    chrlst[chr_ref].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(chrlst[chr_ref].matrix));
  }
  else if(0xFFFF == chrlst[chr_ref].attachedgrip[1])
  {
    // do the linear interpolation
    vertex = chrlst[chr_ref].attachedgrip[0];
    md2_blend_vertices(chrlst + mount_ref, vertex, vertex);

    // Calculate weapon's matrix
    chrlst[chr_ref].matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, chrlst[chr_ref].turn_lr + chrlst[mount_ref].turn_lr, chrlst[mount_ref].vdata.Vertices[vertex].x, chrlst[mount_ref].vdata.Vertices[vertex].y, chrlst[mount_ref].vdata.Vertices[vertex].z);
    chrlst[chr_ref].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(chrlst[chr_ref].matrix));
  }
  else
  {
    GLvector point[GRIP_SIZE], nupoint[GRIP_SIZE];

    // do the linear interpolation
    vertex = chrlst[chr_ref].attachedgrip[0];
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
    chrlst[chr_ref].matrix = FourPoints( nupoint[0], nupoint[1], nupoint[2], nupoint[3], 1.0 );
    chrlst[chr_ref].pos.x = (chrlst[chr_ref].matrix).CNV(3,0);
    chrlst[chr_ref].pos.y = (chrlst[chr_ref].matrix).CNV(3,1);
    chrlst[chr_ref].pos.z = (chrlst[chr_ref].matrix).CNV(3,2);
    chrlst[chr_ref].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(chrlst[chr_ref].matrix));
  };

  if(chrlst[chr_ref].matrixvalid && recalc_bumper)
  {
    // invalidate the cached bumper data
    chrlst[chr_ref].bmpdata.cv.level = -1;
  };

  return chrlst[chr_ref].matrixvalid;
}

//--------------------------------------------------------------------------------------------
void make_character_matrices(Chr chrlst[], size_t chrlst_size)
{
  // ZZ> This function makes all of the character's matrices

  CHR_REF chr_ref;
  bool_t  bfinished;

  // Forget about old matrices
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    chrlst[chr_ref].matrixvalid = bfalse;
  }

  // Do base characters
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    CHR_REF attached_ref;
    if ( !VALID_CHR( chrlst, chr_ref ) ) continue;

    attached_ref = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
    if ( VALID_CHR( chrlst, attached_ref ) ) continue;  // Skip weapons for now

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
      if ( chrlst[chr_ref].matrixvalid || !VALID_CHR( chrlst, chr_ref ) ) continue;

      attached_ref = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
      if ( !VALID_CHR( chrlst, attached_ref ) ) continue;

      if ( !chrlst[attached_ref].matrixvalid )
      {
        bfinished = bfalse;
        continue;
      }

      make_one_weapon_matrix( chrlst, chrlst_size, chr_ref );
    }
  };

}

//--------------------------------------------------------------------------------------------
int ChrList_get_free( CGame * gs )
{
  // ZZ> This function gets an unused character and returns its index

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  CHR_REF chr_ref;

  if ( gs->ChrFreeList_count == 0 )
  {
    // Return MAXCHR if we can't find one
    return MAXCHR;
  }
  else
  {
    // Just grab the next one
    gs->ChrFreeList_count--;
    chr_ref = gs->ChrFreeList[gs->ChrFreeList_count];
  }
  return chr_ref;
}

//--------------------------------------------------------------------------------------------
Chr * Chr_new(Chr * pchr)
{
  AI_STATE * pstate;

  //fprintf( stdout, "Chr_new()\n");

  if(NULL == pchr || pchr->on) return pchr;

  // initialize the data
  memset(pchr, 0, sizeof(Chr));

  pstate = &(pchr->aistate);

  // IMPORTANT!!!
  pchr->sparkle = NOSPARKLE;
  pchr->missilehandler = MAXCHR;

  // Set up model stuff
  pchr->inwhichslot = SLOT_NONE;
  pchr->inwhichpack = MAXCHR;
  pchr->nextinpack = MAXCHR;
  VData_Blended_construct( &(pchr->vdata) );

  pchr->basemodel = MAXPROFILE;
  pchr->stoppedby = MPDFX_WALL | MPDFX_IMPASS;
  pchr->boretime = DELAY_BORE;
  pchr->carefultime = DELAY_CAREFUL;

  //Ready for loop sound
  pchr->loopingchannel = INVALID_CHANNEL;

  // Enchant stuff
  pchr->firstenchant = MAXENCHANT;
  pchr->undoenchant = MAXENCHANT;
  pchr->canseeinvisible = bfalse;
  pchr->canchannel = bfalse;

  //Skill Expansions

  // Kurse state

  // Ammo

  // Gender
  pchr->gender = GEN_OTHER;

  // Team stuff
  pchr->team = TEAM_NULL;
  pchr->baseteam = TEAM_NULL;

  // Skin

  // Life and Mana
  pchr->lifecolor = 0;
  pchr->manacolor = 1;

  // SWID

  // Damage

  // Flags

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
  pchr->attachedto  = MAXCHR;
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    pchr->holdingwhich[_slot] = MAXCHR;
  }

  // Image rendering

  // Movement
  pchr->spd_sneak = 1;
  pchr->spd_walk  = 1;
  pchr->spd_run   = 1;

  // Set up position

  pchr->mapturn_lr = 32768;  // These two mean on level surface
  pchr->mapturn_ud = 32768;
  pchr->scale = pchr->fat;

  // AI stuff

  // action stuff
  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  pchr->onwhichplatform = MAXCHR;

  // Timers set to 0

  // Money is added later

  // Name the character
  strncpy( pchr->name, "NONE", sizeof( pchr->name ) );

  pchr->pancakepos.x = pchr->pancakepos.y = pchr->pancakepos.z = 1.0;
  pchr->pancakevel.x = pchr->pancakevel.y = pchr->pancakevel.z = 0.0f;

  // calculate the bumpers
  assert(NULL == pchr->bmpdata.cv_tree);
  chr_bdata_reinit( pchr, &(pchr->bmpdata) );
  pchr->matrix = IdentityMatrix();
  pchr->matrixvalid = bfalse;

  // initialize a non-existant collision volume octree
  bdata_new( &(pchr->bmpdata) );

  return pchr;
}

//--------------------------------------------------------------------------------------------
bool_t Chr_delete(Chr * pchr)
{
  int i;

  if(NULL==pchr) return bfalse;
  if(!pchr->on)  return btrue;

  // reset some values
  pchr->on = bfalse;
  pchr->alive = bfalse;
  pchr->staton = bfalse;
  pchr->matrixvalid = bfalse;
  pchr->model = MAXMODEL;
  VData_Blended_Deallocate(&(pchr->vdata));
  pchr->name[0] = '\0';

  // invalidate pack
  pchr->numinpack = 0;
  pchr->inwhichpack = MAXCHR;
  pchr->nextinpack = MAXCHR;

  // invalidate attachmants
  pchr->inwhichslot = SLOT_NONE;
  pchr->attachedto = MAXCHR;
  for ( i = 0; i < SLOT_COUNT; i++ )
  {
    pchr->holdingwhich[i] = MAXCHR;
  };

  // remove existing collision volume octree
  bdata_delete( &(pchr->bmpdata) );

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
Chr * Chr_renew(Chr * pchr)
{
  if(NULL == pchr) return pchr;

  Chr_delete(pchr);
  return Chr_new(pchr);
}

//--------------------------------------------------------------------------------------------
Uint32 chr_hitawall( CGame * gs, Chr * pchr, vect3 * norm )
{
  // ZZ> This function returns nonzero if the character hit a wall that the
  //     chr_ref is not allowed to cross

  Uint32 retval;
  vect3  pos, size, test_norm, tmp_norm;

  if ( NULL == pchr || !pchr->on || 0.0f == pchr->bumpstrength ) return 0;

  VectorClear( tmp_norm.v );

  pos.x = ( pchr->bmpdata.cv.x_max + pchr->bmpdata.cv.x_min ) * 0.5f;
  pos.y = ( pchr->bmpdata.cv.y_max + pchr->bmpdata.cv.y_min ) * 0.5f;
  pos.z =   pchr->bmpdata.cv.z_min;

  size.x = ( pchr->bmpdata.cv.x_max - pchr->bmpdata.cv.x_min ) * 0.5f;
  size.y = ( pchr->bmpdata.cv.y_max - pchr->bmpdata.cv.y_min ) * 0.5f;
  size.z = ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min ) * 0.5f;

  retval = mesh_hitawall( gs, pos, size.x, size.y, pchr->stoppedby, NULL );
  test_norm.z = 0;

  if(0 != retval)
  {
    vect3 diff, pos2;

    diff.x = pchr->pos.x - pchr->pos_old.x;
    diff.y = pchr->pos.y - pchr->pos_old.y;
    diff.z = pchr->pos.z - pchr->pos_old.z;

    pos2.x = pos.x - diff.x;
    pos2.y = pos.y - diff.y;
    pos2.z = pos.z - diff.z;

    if( 0 != mesh_hitawall( gs, pos2, size.x, size.y, pchr->stoppedby, NULL ) )
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
    if( 0 == mesh_hitawall( gs, pos2, size.x, size.y, pchr->stoppedby, NULL ) )
    {
      tmp_norm.x += -diff.x;
    }

    // check for a wall in the x-z plane
    pos2.x = pos.x;
    pos2.y = pos.y - diff.y;
    pos2.z = pos.z;
    if( 0 == mesh_hitawall( gs, pos2, size.x, size.y, pchr->stoppedby, NULL ) )
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
      tmp_norm.x = -pchr->vel.x;
      tmp_norm.y = -pchr->vel.y;
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
void chr_reset_accel( CGame * gs, CHR_REF chr_ref )
{
  // ZZ> This function fixes a character's MAX acceleration

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Uint16 enchant;

  if ( !VALID_CHR( chrlst, chr_ref ) ) return;

  // Okay, remove all acceleration enchants
  enchant = chrlst[chr_ref].firstenchant;
  while ( enchant < enclst_size )
  {
    remove_enchant_value( gs, enchant, ADDACCEL );
    enchant = enclst[enchant].nextenchant;
  }

  // Set the starting value
  assert( MAXMODEL != VALIDATE_MDL( chrlst[chr_ref].model ) );
  chrlst[chr_ref].skin.maxaccel = caplst[chrlst[chr_ref].model].skin[chrlst[chr_ref].skin_ref % MAXSKIN].maxaccel;

  // Put the acceleration enchants back on
  enchant = chrlst[chr_ref].firstenchant;
  while ( enchant < enclst_size )
  {
    add_enchant_value( gs, enchant, ADDACCEL, enclst[enchant].eve );
    enchant = enclst[enchant].nextenchant;
  }

}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_mount( CGame * gs, CHR_REF chr_ref, bool_t ignorekurse, bool_t doshop )
{
  // ZZ> This function drops an item

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  Uint16 imount, iowner = MAXCHR;
  Uint16 enchant, passage;
  Uint16 cnt, price;
  bool_t inshop;

  // Make sure the chr_ref is valid
  if ( !VALID_CHR( chrlst, chr_ref ) )
    return bfalse;

  // Make sure the chr_ref is mounted
  imount = chr_get_attachedto( chrlst, chrlst_size, chr_ref );
  if ( !VALID_CHR( chrlst, imount ) )
    return bfalse;

  // Don't allow living characters to drop kursed weapons
  if ( !ignorekurse && chrlst[chr_ref].iskursed && chrlst[imount].alive )
  {
    chrlst[chr_ref].aistate.alert |= ALERT_NOTDROPPED;
    return bfalse;
  }

  // Rip 'em apart
  _slot = chrlst[chr_ref].inwhichslot;
  if(_slot == SLOT_INVENTORY)
  {
    chrlst[chr_ref].attachedto = MAXCHR;
    chrlst[chr_ref].inwhichslot = SLOT_NONE;
  }
  else
  {
    assert(_slot != SLOT_NONE);
    assert(chr_ref == chrlst[imount].holdingwhich[_slot]);
    chrlst[chr_ref].attachedto = MAXCHR;
    chrlst[chr_ref].inwhichslot = SLOT_NONE;
    chrlst[imount].holdingwhich[_slot] = MAXCHR;
  }

  chrlst[chr_ref].scale = chrlst[chr_ref].fat; // * madlst[chrlst[chr_ref].model].scale * 4;

  // Run the falling animation...
  play_action( gs, chr_ref, (ACTION)(ACTION_JB + ( chrlst[chr_ref].inwhichslot % 2 )), bfalse );

  // Set the positions
  if ( chrlst[chr_ref].matrixvalid )
  {
    chrlst[chr_ref].pos.x = chrlst[chr_ref].matrix.CNV( 3, 0 );
    chrlst[chr_ref].pos.y = chrlst[chr_ref].matrix.CNV( 3, 1 );
    chrlst[chr_ref].pos.z = chrlst[chr_ref].matrix.CNV( 3, 2 );
  }
  else
  {
    chrlst[chr_ref].pos.x = chrlst[imount].pos.x;
    chrlst[chr_ref].pos.y = chrlst[imount].pos.y;
    chrlst[chr_ref].pos.z = chrlst[imount].pos.z;
  }

  // Make sure it's not dropped in a wall...
  if ( 0 != chr_hitawall( gs, chrlst + chr_ref, NULL ) )
  {
    chrlst[chr_ref].pos.x = chrlst[imount].pos.x;
    chrlst[chr_ref].pos.y = chrlst[imount].pos.y;
  }

  // Check for shop passages
  inshop = bfalse;
  if ( chrlst[chr_ref].isitem && gs->ShopList_count != 0 && doshop )
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
      Uint16 model = chrlst[chr_ref].model;

      assert( MAXMODEL != VALIDATE_MDL( model ) );

      // Give the imount its money back, alert the shop iowner
      price = caplst[model].skin[chrlst[chr_ref].skin_ref % MAXSKIN].cost;
      if ( caplst[model].isstackable )
      {
        price *= chrlst[chr_ref].ammo;
      }
      chrlst[imount].money += price;
      chrlst[iowner].money -= price;
      if ( chrlst[iowner].money < 0 )  chrlst[iowner].money = 0;
      if ( chrlst[imount].money > MAXMONEY )  chrlst[imount].money = MAXMONEY;

      chrlst[iowner].aistate.alert |= ALERT_SIGNALED;
      chrlst[iowner].message = price;  // Tell iowner how much...
      chrlst[iowner].messagedata = 0;  // 0 for buying an item
    }
  }

  // Make sure it works right
  chrlst[chr_ref].hitready = btrue;
  chrlst[chr_ref].aistate.alert   |= ALERT_DROPPED;
  if ( inshop )
  {
    // Drop straight down to avoid theft
    chrlst[chr_ref].vel.x = 0;
    chrlst[chr_ref].vel.y = 0;
  }
  else
  {
    Uint16 sin_dir = RANDIE(gs);
    chrlst[chr_ref].accum_vel.x += chrlst[imount].vel.x + 0.5 * DROPXYVEL * turntocos[sin_dir>>2];
    chrlst[chr_ref].accum_vel.y += chrlst[imount].vel.y + 0.5 * DROPXYVEL * turntosin[sin_dir>>2];
  }
  chrlst[chr_ref].accum_vel.z += DROPZVEL;

  // Turn looping off
  chrlst[chr_ref].action.loop = bfalse;

  // Reset the team if it is a imount
  if ( chrlst[imount].ismount )
  {
    chrlst[imount].team = chrlst[imount].baseteam;
    chrlst[imount].aistate.alert |= ALERT_DROPPED;
  }
  chrlst[chr_ref].team = chrlst[chr_ref].baseteam;
  chrlst[chr_ref].aistate.alert |= ALERT_DROPPED;

  // Reset transparency
  if ( chrlst[chr_ref].isitem && chrlst[imount].transferblend )
  {
    Uint16 model = chrlst[chr_ref].model;

    assert( MAXMODEL != VALIDATE_MDL( model ) );

    // Okay, reset transparency
    enchant = chrlst[chr_ref].firstenchant;
    while ( enchant < enclst_size )
    {
      unset_enchant_value( gs, enchant, SETALPHABLEND );
      unset_enchant_value( gs, enchant, SETLIGHTBLEND );
      enchant = enclst[enchant].nextenchant;
    }

    chrlst[chr_ref].alpha_fp8 = caplst[model].alpha_fp8;
    chrlst[chr_ref].bumpstrength = caplst[model].bumpstrength * FP8_TO_FLOAT( chrlst[chr_ref].alpha_fp8 );
    chrlst[chr_ref].light_fp8 = caplst[model].light_fp8;
    enchant = chrlst[chr_ref].firstenchant;
    while ( enchant < enclst_size )
    {
      set_enchant_value( gs, enchant, SETALPHABLEND, enclst[enchant].eve );
      set_enchant_value( gs, enchant, SETLIGHTBLEND, enclst[enchant].eve );
      enchant = enclst[enchant].nextenchant;
    }
  }

  // Set twist
  chrlst[chr_ref].mapturn_lr = 32768;
  chrlst[chr_ref].mapturn_ud = 32768;

  if ( chr_is_player(gs, chr_ref) )
    debug_message( 1, "dismounted %s(%s) from (%s)", chrlst[chr_ref].name, caplst[chrlst[chr_ref].model].classname, caplst[chrlst[imount].model].classname );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_character_to_mount( CGame * gs, CHR_REF chr_ref, CHR_REF mount_ref, SLOT slot )
{
  // ZZ> This function attaches one character to another ( the mount )
  //     at either the left or right grip

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  int tnc;

  // Make sure both are still around
  if ( !VALID_CHR( chrlst, chr_ref ) || !VALID_CHR( chrlst, mount_ref ) )
    return bfalse;

  // the item may hit the floor if this fails
  chrlst[chr_ref].hitready = bfalse;

  //make sure you're not trying to mount yourself!
  if ( chr_ref == mount_ref )
    return bfalse;

  // make sure that neither is in someone's pack
  if ( chr_in_pack( chrlst, chrlst_size, chr_ref ) || chr_in_pack( chrlst, chrlst_size, mount_ref ) )
    return bfalse;

  // Make sure the the slot is valid
  assert( MAXMODEL != VALIDATE_MDL( chrlst[mount_ref].model ) );
  if ( SLOT_NONE == slot || !caplst[chrlst[mount_ref].model].slotvalid[slot] )
    return bfalse;

  // Put 'em together
  assert(slot != SLOT_NONE);
  chrlst[chr_ref].inwhichslot = slot;
  chrlst[chr_ref].attachedto  = mount_ref;
  chrlst[mount_ref].holdingwhich[slot] = chr_ref;

  // handle the vertices
  {
    Uint16 model = chrlst[mount_ref].model;
    Uint16 vrtoffset = slot_to_offset( slot );

    assert( MAXMODEL != VALIDATE_MDL( model ) );
    if ( madlst[model].vertices > vrtoffset && vrtoffset > 0 )
    {
      tnc = madlst[model].vertices - vrtoffset;
      chrlst[chr_ref].attachedgrip[0] = tnc;
      chrlst[chr_ref].attachedgrip[1] = tnc + 1;
      chrlst[chr_ref].attachedgrip[2] = tnc + 2;
      chrlst[chr_ref].attachedgrip[3] = tnc + 3;
    }
    else
    {
      chrlst[chr_ref].attachedgrip[0] = madlst[model].vertices - 1;
      chrlst[chr_ref].attachedgrip[1] = 0xFFFF;
      chrlst[chr_ref].attachedgrip[2] = 0xFFFF;
      chrlst[chr_ref].attachedgrip[3] = 0xFFFF;
    }
  }

  chrlst[chr_ref].jumptime = DELAY_JUMP * 4;

  // Run the held animation
  if ( chrlst[mount_ref].bmpdata.calc_is_mount && slot == SLOT_SADDLE )
  {
    // Riding mount_ref
    play_action( gs, chr_ref, ACTION_MI, btrue );
    chrlst[chr_ref].action.loop = btrue;
  }
  else
  {
    play_action( gs, chr_ref, (ACTION)(ACTION_MM + slot), bfalse );
    if ( chrlst[chr_ref].isitem )
    {
      // Item grab
      chrlst[chr_ref].action.keep = btrue;
    }
  }

  // Set the team
  if ( chrlst[chr_ref].isitem )
  {
    chrlst[chr_ref].team = chrlst[mount_ref].team;
    // Set the alert
    chrlst[chr_ref].aistate.alert |= ALERT_GRABBED;
  }
  else if ( chrlst[mount_ref].bmpdata.calc_is_mount )
  {
    chrlst[mount_ref].team = chrlst[chr_ref].team;
    // Set the alert
    if ( !chrlst[mount_ref].isitem )
    {
      chrlst[mount_ref].aistate.alert |= ALERT_GRABBED;
    }
  }

  // It's not gonna hit the floor
  chrlst[chr_ref].hitready = bfalse;

  if ( chr_is_player(gs, chr_ref) )
  {
    debug_message( 1, "mounted %s(%s) to (%s)", chrlst[chr_ref].name, caplst[chrlst[chr_ref].model].classname, caplst[chrlst[mount_ref].model].classname );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF stack_in_pack( CGame * gs, CHR_REF item_ref, CHR_REF chr_ref )
{
  // ZZ> This function looks in the chraracter's pack for an item similar
  //     to the one given.  If it finds one, it returns the similar item's
  //     index number, otherwise it returns MAXCHR.

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Uint16 inpack, id;
  bool_t allok;

  Uint16 item_mdl = chrlst[item_ref].model;

  assert( MAXMODEL != VALIDATE_MDL( item_mdl ) );

  if ( caplst[item_mdl].isstackable )
  {
    Uint16 inpack_mdl;

    inpack = chr_get_nextinpack( chrlst, MAXCHR, chr_ref );
    inpack_mdl = chrlst[inpack].model;

    allok = bfalse;
    while ( VALID_CHR( chrlst, inpack ) && !allok )
    {
      assert( MAXMODEL != VALIDATE_MDL( inpack_mdl ) );

      allok = btrue;
      if ( inpack_mdl != item_mdl )
      {
        if ( !caplst[inpack_mdl].isstackable )
        {
          allok = bfalse;
        }

        if ( chrlst[inpack].ammomax != chrlst[item_ref].ammomax )
        {
          allok = bfalse;
        }

        id = 0;
        while ( id < IDSZ_COUNT && allok )
        {
          if ( caplst[inpack_mdl].idsz[id] != caplst[item_mdl].idsz[id] )
          {
            allok = bfalse;
          }
          id++;
        }
      }
      if ( !allok )
      {
        inpack = chr_get_nextinpack( chrlst, MAXCHR, inpack );
      }
    }

    if ( allok )
    {
      return inpack;
    }
  }

  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
static bool_t pack_push_front( Chr chrlst[], size_t chrlst_size, CHR_REF item_ref, CHR_REF chr_ref )
{
  // make sure the item and character are valid
  if ( !VALID_CHR( chrlst, item_ref ) || !VALID_CHR( chrlst, chr_ref ) ) return bfalse;

  // make sure the item is free to add
  if ( chr_attached( chrlst, chrlst_size, item_ref ) || chr_in_pack( chrlst, chrlst_size, item_ref ) ) return bfalse;

  // we cannot do packs within packs, so
  if ( chr_in_pack( chrlst, chrlst_size, chr_ref ) ) return bfalse;

  // make sure there is space for the item
  if ( chrlst[chr_ref].numinpack >= MAXNUMINPACK ) return bfalse;

  // insert at the front of the list
  chrlst[item_ref].nextinpack  = chr_get_nextinpack( chrlst, chrlst_size, chr_ref );
  chrlst[chr_ref].nextinpack = item_ref;
  chrlst[item_ref].inwhichpack = chr_ref;
  chrlst[chr_ref].numinpack++;

  return btrue;
};

//--------------------------------------------------------------------------------------------
static Uint16 pack_pop_back( Chr chrlst[], size_t chrlst_size, CHR_REF chr_ref )
{
  Uint16 iitem = MAXCHR, itail = MAXCHR;

  // make sure the character is valid
  if ( !VALID_CHR( chrlst, chr_ref ) ) return MAXCHR;

  // if character is in a pack, it has no inventory of it's own
  if ( chr_in_pack( chrlst, chrlst_size, iitem ) ) return MAXCHR;

  // make sure there is something in the pack
  if ( !chr_has_inventory( chrlst, chrlst_size, chr_ref ) ) return MAXCHR;

  // remove from the back of the list
  itail = chr_ref;
  iitem = chr_get_nextinpack( chrlst, chrlst_size, chr_ref );
  while ( VALID_CHR( chrlst, chrlst[iitem].nextinpack ) )
  {
    // do some error checking
    assert( 0 == chrlst[iitem].numinpack );

    // go to the next element
    itail = iitem;
    iitem = chr_get_nextinpack( chrlst, chrlst_size, iitem );
  };

  // disconnect the item from the list
  chrlst[itail].nextinpack = MAXCHR;
  chrlst[chr_ref].numinpack--;

  // do some error checking
  assert( VALID_CHR( chrlst, iitem ) );

  // fix the removed item
  chrlst[iitem].numinpack   = 0;
  chrlst[iitem].nextinpack  = MAXCHR;
  chrlst[iitem].inwhichpack = MAXCHR;
  chrlst[iitem].isequipped = bfalse;

  return iitem;
};

//--------------------------------------------------------------------------------------------
bool_t pack_add_item( CGame * gs, CHR_REF item_ref, CHR_REF chr_ref )
{
  // ZZ> This function puts one chr_ref inside the other's pack

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Uint16 newammo, istack;

  // Make sure both objects exist
  if ( !VALID_CHR( chrlst, chr_ref ) || !VALID_CHR( chrlst, item_ref ) ) return bfalse;

  // Make sure neither object is in a pack
  if ( chr_in_pack( chrlst, chrlst_size, chr_ref ) || chr_in_pack( chrlst, chrlst_size, item_ref ) ) return bfalse;

  // make sure we the character IS NOT an item and the item IS an item
  if ( chrlst[chr_ref].isitem || !chrlst[item_ref].isitem ) return bfalse;

  // make sure the item does not have an inventory of its own
  if ( chr_has_inventory( chrlst, chrlst_size, item_ref ) ) return bfalse;

  istack = stack_in_pack( gs, item_ref, chr_ref );
  if ( VALID_CHR( chrlst, istack ) )
  {
    // put out torches, etc.
    disaffirm_attached_particles( gs, item_ref );

    // We found a similar, stackable item_ref in the pack
    if ( chrlst[item_ref].nameknown || chrlst[istack].nameknown )
    {
      chrlst[item_ref].nameknown = btrue;
      chrlst[istack].nameknown = btrue;
    }
    if ( caplst[chrlst[item_ref].model].usageknown || caplst[chrlst[istack].model].usageknown )
    {
      caplst[chrlst[item_ref].model].usageknown = btrue;
      caplst[chrlst[istack].model].usageknown = btrue;
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
      chrlst[chr_ref].aistate.alert |= ALERT_TOOMUCHBAGGAGE;
    }
  }
  else
  {
    // Make sure we have room for another item_ref
    if ( chrlst[chr_ref].numinpack >= MAXNUMINPACK )
    {
      chrlst[chr_ref].aistate.alert |= ALERT_TOOMUCHBAGGAGE;
      return bfalse;
    }

    // Take the item out of hand
    if ( detach_character_from_mount( gs, item_ref, btrue, bfalse ) )
    {
      chrlst[item_ref].aistate.alert &= ~ALERT_DROPPED;
    }

    if ( pack_push_front( chrlst, chrlst_size, item_ref, chr_ref ) )
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
Uint16 pack_get_item( CGame * gs, CHR_REF chr_ref, SLOT slot, bool_t ignorekurse )
{
  // ZZ> This function takes the last item in the character's pack and puts
  //     it into the designated hand.  It returns the item number or MAXCHR.

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 item;

  // dose the chr_ref exist?
  if ( !VALID_CHR( chrlst, chr_ref ) )
    return MAXCHR;

  // make sure a valid inventory exists
  if ( !chr_has_inventory( chrlst, chrlst_size, chr_ref ) )
    return MAXCHR;

  item = pack_pop_back( chrlst, chrlst_size, chr_ref );

  // Figure out what to do with it
  if ( chrlst[item].iskursed && chrlst[item].isequipped && !ignorekurse )
  {
    // Flag the last item as not removed
    chrlst[item].aistate.alert |= ALERT_NOTPUTAWAY;  // Doubles as IfNotTakenOut

    // push it back on the front of the list
    pack_push_front( chrlst, chrlst_size, item, chr_ref );

    // return the "fail" value
    item = MAXCHR;
  }
  else
  {
    // Attach the item to the chr_ref's hand
    attach_character_to_mount( gs, item, chr_ref, slot );

    // fix some item values
    chrlst[item].aistate.alert &= ( ~ALERT_GRABBED );
    chrlst[item].aistate.alert |= ALERT_TAKENOUT;
    //chrlst[item].team   = chrlst[chr_ref].team;
  }

  return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( CGame * gs, CHR_REF chr_ref )
{
  // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a chr_ref's
  //     inventory ( Not hands ).

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 item, lastitem, nextitem, direction, cosdir;
  IDSZ testa, testz;

  if ( !VALID_CHR( chrlst, chr_ref ) ) return;

  if ( chrlst[chr_ref].pos.z > -2 ) // Don't lose keys in pits...
  {
    // The IDSZs to find
    testa = MAKE_IDSZ( "KEYA" );   // [KEYA]
    testz = MAKE_IDSZ( "KEYZ" );   // [KEYZ]

    lastitem = chr_ref;
    item = chr_get_nextinpack( chrlst, MAXCHR, chr_ref );
    while ( VALID_CHR( chrlst, item ) )
    {
      nextitem = chr_get_nextinpack( chrlst, MAXCHR, item );
      if ( item != chr_ref ) // Should never happen...
      {
        if ( CAP_INHERIT_IDSZ_RANGE( gs, chrlst[item].model, testa, testz ) )
        {
          // We found a key...
          chrlst[item].inwhichpack = MAXCHR;
          chrlst[item].isequipped = bfalse;

          chrlst[lastitem].nextinpack = nextitem;
          chrlst[item].nextinpack = MAXCHR;
          chrlst[chr_ref].numinpack--;

          chrlst[item].hitready = btrue;
          chrlst[item].aistate.alert |= ALERT_DROPPED;

          direction = RANDIE(gs);
          chrlst[item].turn_lr = direction + 32768;
          cosdir = direction + 16384;
          chrlst[item].level = chrlst[chr_ref].level;
          chrlst[item].pos.x = chrlst[chr_ref].pos.x;
          chrlst[item].pos.y = chrlst[chr_ref].pos.y;
          chrlst[item].pos.z = chrlst[chr_ref].pos.z;
          chrlst[item].accum_vel.x += turntocos[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          chrlst[item].accum_vel.y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          chrlst[item].accum_vel.z += DROPZVEL;
          chrlst[item].team = chrlst[item].baseteam;
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
void drop_all_items( CGame * gs, CHR_REF chr_ref )
{
  // ZZ> This function drops all of a character's items

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 item, direction, cosdir, diradd;

  if ( !VALID_CHR( chrlst, chr_ref ) ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    detach_character_from_mount( gs, chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, _slot ), !chrlst[chr_ref].alive, bfalse );
  };

  if ( chr_has_inventory( chrlst, chrlst_size, chr_ref ) )
  {
    direction = chrlst[chr_ref].turn_lr + 32768;
    diradd = (float)UINT16_SIZE / chrlst[chr_ref].numinpack;
    while ( chrlst[chr_ref].numinpack > 0 )
    {
      item = pack_get_item( gs, chr_ref, SLOT_NONE, !chrlst[chr_ref].alive );
      if ( detach_character_from_mount( gs, item, btrue, btrue ) )
      {
        chrlst[item].hitready = btrue;
        chrlst[item].aistate.alert |= ALERT_DROPPED;
        chrlst[item].pos.x = chrlst[chr_ref].pos.x;
        chrlst[item].pos.y = chrlst[chr_ref].pos.y;
        chrlst[item].pos.z = chrlst[chr_ref].pos.z;
        chrlst[item].level = chrlst[chr_ref].level;
        chrlst[item].turn_lr = direction + 32768;

        cosdir = direction + 16384;
        chrlst[item].accum_vel.x += turntocos[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        chrlst[item].accum_vel.y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        chrlst[item].accum_vel.z += DROPZVEL;
        chrlst[item].team = chrlst[item].baseteam;
      }

      direction += diradd;
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t chr_grab_stuff( CGame * gs, CHR_REF chr_ref, SLOT slot, bool_t people )
{
  // ZZ> This function makes the character pick up an item if there's one around

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  vect4 posa, point;
  vect3 posb, posc;
  float dist, mindist;
  CHR_REF object_ref, minchr_ref, holder_ref, packer_ref, owner_ref = MAXCHR;
  Uint16 vertex, model, passage, cnt, price;
  bool_t inshop, can_disarm, can_pickpocket, bfound, ballowed;
  GRIP grip;
  float grab_width, grab_height;

  CHR_REF trg_chr = MAXCHR;
  Sint16 trg_strength_fp8, trg_intelligence_fp8;
  TEAM trg_team;

  Chr * pchr;

  if ( !VALID_CHR( chrlst, chr_ref ) ) return bfalse;
  pchr = chrlst + chr_ref;

  model = chrlst[chr_ref].model;
  if ( !VALID_MDL( model ) ) return bfalse;

  // Make sure the character doesn't have something already, and that it has hands

  if ( chr_using_slot( chrlst, chrlst_size, chr_ref, slot ) || !caplst[model].slotvalid[slot] )
    return bfalse;

  // Make life easier
  grip  = slot_to_grip( slot );

  // !!!!base the grab distance off of the character size!!!!
  if( !pchr->bmpdata.valid )
  {
    chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chr_ref, 0);
  }

  grab_width  = ( pchr->bmpdata.calc_size_big + pchr->bmpdata.calc_size ) / 2.0f * 1.5f;
  grab_height = pchr->bmpdata.calc_height / 2.0f * 1.5f;

  // Do we have a matrix???
  if ( pchr->matrixvalid )
  {
    // Transform the weapon grip from model to world space
    vertex = pchr->attachedgrip[0];

    if(0xFFFF == vertex)
    {
      point.x = pchr->pos.x;
      point.y = pchr->pos.y;
      point.z = pchr->pos.z;
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
    posa.x = pchr->pos.x;
    posa.y = pchr->pos.y;
    posa.z = pchr->pos.z;
  }

  // Go through all characters to find the best match
  can_disarm     = check_skills( gs, chr_ref, MAKE_IDSZ( "DISA" ) );
  can_pickpocket = check_skills( gs, chr_ref, MAKE_IDSZ( "PICK" ) );
  bfound = bfalse;
  for ( object_ref = 0; object_ref < chrlst_size; object_ref++ )
  {
    // Don't mess with stuff that doesn't exist
    if ( !VALID_CHR( chrlst, object_ref ) ) continue;

    holder_ref = chr_get_attachedto(chrlst, chrlst_size, object_ref);
    packer_ref = chr_get_inwhichpack(chrlst, chrlst_size, object_ref);

    // don't mess with yourself or anything you're already holding
    if ( object_ref == chr_ref || packer_ref == chr_ref || holder_ref == chr_ref ) continue;

    // don't mess with stuff you can't see
    if ( !pchr->canseeinvisible && chr_is_invisible( chrlst, chrlst_size, object_ref ) ) continue;

    // if we can't pickpocket, don't mess with inventory items
    if ( !can_pickpocket && VALID_CHR( chrlst, packer_ref ) ) continue;

    // if we can't disarm, don't mess with held items
    if ( !can_disarm && VALID_CHR( chrlst, holder_ref ) ) continue;

    // if we can't grab people, don't mess with them
    if ( !people && !chrlst[object_ref].isitem ) continue;

    // get the target object position
    if ( !VALID_CHR( chrlst, packer_ref ) && !VALID_CHR(chrlst, holder_ref) )
    {
      trg_strength_fp8     = chrlst[object_ref].strength_fp8;
      trg_intelligence_fp8 = chrlst[object_ref].intelligence_fp8;
      trg_team             = chrlst[object_ref].team;

      posb = chrlst[object_ref].pos;
    }
    else if ( VALID_CHR(chrlst, holder_ref) )
    {
      trg_chr              = holder_ref;
      trg_strength_fp8     = chrlst[holder_ref].strength_fp8;
      trg_intelligence_fp8 = chrlst[holder_ref].intelligence_fp8;

      trg_team = chrlst[holder_ref].team;
      posb     = chrlst[object_ref].pos;
    }
    else // must be in a pack
    {
      trg_chr              = packer_ref;
      trg_strength_fp8     = chrlst[packer_ref].strength_fp8;
      trg_intelligence_fp8 = chrlst[packer_ref].intelligence_fp8;

      trg_team = chrlst[packer_ref].team;
      posb     = chrlst[packer_ref].pos;
      posb.z  += chrlst[packer_ref].bmpdata.calc_height / 2;
    };

    // First check absolute value diamond
    posc.x = ABS( posa.x - posb.x );
    posc.y = ABS( posa.y - posb.y );
    posc.z = ABS( posa.z - posb.z );
    dist = posc.x + posc.y;

    // close enough to grab ?
    if ( dist > grab_width || posc.z > grab_height ) continue;

    if ( VALID_CHR(chrlst, packer_ref) )
    {
      // check for pickpocket
      ballowed = pchr->dexterity_fp8 >= trg_intelligence_fp8 && gs->TeamList[pchr->team].hatesteam[trg_team];

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
    else if ( VALID_CHR( chrlst, holder_ref ) )
    {
      // check for stealing item from hand
      ballowed = !chrlst[object_ref].iskursed && pchr->strength_fp8 > trg_strength_fp8 && gs->TeamList[pchr->team].hatesteam[trg_team];

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
  if ( mesh_check( (&gs->mesh), chrlst[minchr_ref].pos.x, chrlst[minchr_ref].pos.y ) )
  {
    if ( gs->ShopList_count == 0 )
    {
      ballowed = btrue;
    }
    else if ( chrlst[minchr_ref].isitem )
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
    if ( pchr->isitem )
    {
      ballowed = btrue; // As in NetHack, Pets can shop for free =]
    }
    else
    {
      // Pay the shop owner_ref, or don't allow grab...
      chrlst[owner_ref].aistate.alert |= ALERT_SIGNALED;
      price = caplst[chrlst[minchr_ref].model].skin[chrlst[minchr_ref].skin_ref % MAXSKIN].cost;
      if ( caplst[chrlst[minchr_ref].model].isstackable )
      {
        price *= chrlst[minchr_ref].ammo;
      }
      chrlst[owner_ref].message = price;  // Tell owner_ref how much...
      if ( pchr->money >= price )
      {
        // Okay to buy
        pchr->money  -= price;  // Skin 0 cost is price
        chrlst[owner_ref].money += price;
        if ( chrlst[owner_ref].money > MAXMONEY )  chrlst[owner_ref].money = MAXMONEY;

        ballowed = btrue;
        chrlst[owner_ref].messagedata = 1;  // 1 for selling an item
      }
      else
      {
        // Don't allow purchase
        chrlst[owner_ref].messagedata = 2;  // 2 for "you can't afford that"
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
    chrlst[minchr_ref].accum_vel.z += DROPZVEL;
    chrlst[minchr_ref].hitready = btrue;
    chrlst[minchr_ref].aistate.alert |= ALERT_DROPPED;
  };

  return ballowed;
}

//--------------------------------------------------------------------------------------------
void chr_swipe( CGame * gs, CHR_REF chr_ref, SLOT slot )
{
  // ZZ> This function spawns an attack particle

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  Uint16 weapon, particle, thrown;
  ACTION action;
  Uint16 tTmp;
  float dampen;
  vect3 pos;
  float velocity;
  GRIP spawngrip;

  weapon = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, slot );
  spawngrip = GRIP_LAST;
  action = chrlst[chr_ref].action.now;
  // See if it's an unarmed attack...
  if ( !VALID_CHR( chrlst, weapon ) )
  {
    weapon = chr_ref;
    spawngrip = slot_to_grip( slot );
  }

  if ( weapon != chr_ref && (( caplst[chrlst[weapon].model].isstackable && chrlst[weapon].ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
  {
    // Throw the weapon if it's stacked or a hurl animation
    pos.x = chrlst[chr_ref].pos.x;
    pos.y = chrlst[chr_ref].pos.y;
    pos.z = chrlst[chr_ref].pos.z;
    thrown = spawn_one_character( gs, chrlst[chr_ref].pos, chrlst[weapon].model, chrlst[chr_ref].team, 0, chrlst[chr_ref].turn_lr, chrlst[weapon].name, MAXCHR );
    if ( VALID_CHR( chrlst, thrown ) )
    {
      chrlst[thrown].iskursed = bfalse;
      chrlst[thrown].ammo = 1;
      chrlst[thrown].aistate.alert |= ALERT_THROWN;

      velocity = 0.0f;
      if ( chrlst[chr_ref].weight >= 0.0f )
      {
        velocity = chrlst[chr_ref].strength_fp8 / ( chrlst[thrown].weight * THROWFIX );
      };

      velocity += MINTHROWVELOCITY;
      if ( velocity > MAXTHROWVELOCITY )
      {
        velocity = MAXTHROWVELOCITY;
      }
      tTmp = ( 0x7FFF + chrlst[chr_ref].turn_lr ) >> 2;
      chrlst[thrown].accum_vel.x += turntocos[( tTmp+8192 ) & TRIGTABLE_MASK] * velocity;
      chrlst[thrown].accum_vel.y += turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * velocity;
      chrlst[thrown].accum_vel.z += DROPZVEL;
      if ( chrlst[weapon].ammo <= 1 )
      {
        // Poof the item
        detach_character_from_mount( gs, weapon, btrue, bfalse );
        chrlst[weapon].freeme = btrue;
      }
      else
      {
        chrlst[weapon].ammo--;
      }
    }
  }
  else
  {
    // Spawn an attack particle
    if ( chrlst[weapon].ammomax == 0 || chrlst[weapon].ammo != 0 )
    {
      if ( chrlst[weapon].ammo > 0 && !caplst[chrlst[weapon].model].isstackable )
      {
        chrlst[weapon].ammo--;  // Ammo usage
      }

      //HERE
      if ( caplst[chrlst[weapon].model].attackprttype != -1 )
      {
        particle = spawn_one_particle( gs, 1.0f, chrlst[weapon].pos, chrlst[chr_ref].turn_lr, chrlst[weapon].model, caplst[chrlst[weapon].model].attackprttype, weapon, spawngrip, chrlst[chr_ref].team, chr_ref, 0, MAXCHR );
        if ( VALID_PRT(prtlst, particle) )
        {
          CHR_REF prt_target = prt_get_target( gs, particle );

          if ( !caplst[chrlst[weapon].model].attackattached )
          {
            // Detach the particle
            if ( !piplst[prtlst[particle].pip].startontarget || !VALID_CHR( chrlst, prt_target ) )
            {
              attach_particle_to_character( gs, particle, weapon, spawngrip );

              // Correct Z spacing base, but nothing else...
              prtlst[particle].pos.z += piplst[prtlst[particle].pip].zspacing.ibase;
            }
            prtlst[particle].attachedtochr = MAXCHR;

            // Don't spawn in walls
            if ( 0 != prt_hitawall( gs, particle, NULL ) )
            {
              prtlst[particle].pos.x = chrlst[weapon].pos.x;
              prtlst[particle].pos.y = chrlst[weapon].pos.y;
              if ( 0 != prt_hitawall( gs, particle, NULL ) )
              {
                prtlst[particle].pos.x = chrlst[chr_ref].pos.x;
                prtlst[particle].pos.y = chrlst[chr_ref].pos.y;
              }
            }
          }
          else
          {
            // Attached particles get a strength bonus for reeling...
            if ( piplst[prtlst[particle].pip].causeknockback ) dampen = ( REELBASE + ( chrlst[chr_ref].strength_fp8 / REEL ) ) * 4; //Extra knockback?
            else dampen = REELBASE + ( chrlst[chr_ref].strength_fp8 / REEL );      // No, do normal

            prtlst[particle].accum_vel.x += -(1.0f - dampen) * prtlst[particle].vel.x;
            prtlst[particle].accum_vel.y += -(1.0f - dampen) * prtlst[particle].vel.y;
            prtlst[particle].accum_vel.z += -(1.0f - dampen) * prtlst[particle].vel.z;
          }

          // Initial particles get a strength bonus, which may be 0.00
          prtlst[particle].damage.ibase += ( chrlst[chr_ref].strength_fp8 * caplst[chrlst[weapon].model].strengthdampen );

          // Initial particles get an enchantment bonus
          prtlst[particle].damage.ibase += chrlst[weapon].damageboost;

          // Initial particles inherit damage type of weapon
          prtlst[particle].damagetype = chrlst[weapon].damagetargettype;
        }
      }
    }
    else
    {
      chrlst[weapon].ammoknown = btrue;
    }
  }
}

//--------------------------------------------------------------------------------------------
void move_characters( CGame * gs, float dUpdate )
{
  // ZZ> This function handles character physics

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  CHR_REF chr_ref, weapon, imount, item;
  Uint16 imdl;
  Uint8 twist;
  Uint8 speed, framelip;
  float maxvel, dvx, dvy, dvmax;
  ACTION action;
  bool_t   ready, allowedtoattack, watchtarget, grounded, dojumptimer;
  TURNMODE loc_turnmode;
  float level;

  float horiz_friction, vert_friction;
  float loc_slippyfriction, loc_airfriction, loc_waterfriction, loc_noslipfriction;
  float loc_flydampen, loc_traction;
  float turnfactor;
  vect3 nrm = {0.0f, 0.0f, 0.0f};

  AI_STATE * pstate;
  Chr * pchr;
  Mad * pmad;

  loc_airfriction    = airfriction;
  loc_waterfriction  = waterfriction;
  loc_slippyfriction = slippyfriction;
  loc_noslipfriction = noslipfriction;
  loc_flydampen      = pow( FLYDAMPEN     , dUpdate );

  // Move every character
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !VALID_CHR( chrlst, chr_ref ) ) continue;

    pchr = chrlst + chr_ref;
    pstate = &(pchr->aistate);

    // Character's old location
    pchr->turn_lr_old = pchr->turn_lr;

    if ( chr_in_pack( chrlst, chrlst_size, chr_ref ) ) continue;

    // get the model
    imdl = VALIDATE_MDL( pchr->model );
    assert( MAXMODEL != imdl );
    pmad = madlst + pchr->model;

    // get the imount
    imount = chr_get_attachedto(chrlst, chrlst_size, chr_ref);

    // get the level
    level = pchr->level;

    // TURNMODE_VELOCITY acts strange when someone is mounted on a "bucking" imount, like the gelfeet
    loc_turnmode = pstate->turnmode;
    if ( VALID_CHR( chrlst, imount ) ) loc_turnmode = TURNMODE_NONE;

    // make characters slide downhill
    twist = mesh_get_twist( gs->Mesh_Mem.fanlst, pchr->onwhichfan );

    // calculate the normal GDyna.mically from the mesh coordinates
    if ( !mesh_calc_normal( gs, pchr->pos, &nrm ) )
    {
      nrm = twist_table[twist].nrm;
    };

    // TODO : replace with line(s) below
    turnfactor = 2.0f;
    // scale the turn rate by the dexterity.
    // For zero dexterity, rate is half speed
    // For maximum dexterity, rate is 1.5 normal rate
    //turnfactor = (3.0f * (float)pchr->dexterity_fp8 / (float)PERFECTSTAT + 1.0f) / 2.0f;

    grounded    = bfalse;

    // Down that ol' damage timer
    pchr->damagetime -= dUpdate;
    if ( pchr->damagetime < 0 ) pchr->damagetime = 0.0f;

    // Texture movement
    pchr->uoffset_fp8 += pchr->uoffvel * dUpdate;
    pchr->voffset_fp8 += pchr->voffvel * dUpdate;

    // calculate the Character's environment
    {
      float wt;
      float air_traction = ( pchr->flyheight == 0.0f ) ? ( 1.0 - airfriction ) : airfriction;

      wt = 0.0f;
      loc_traction   = 0;
      horiz_friction = 0;
      vert_friction  = 0;

      if ( pchr->inwater )
      {
        // we are partialy under water
        float buoy, lerp;

        if ( pchr->weight < 0.0f || pchr->holdingweight < 0.0f )
        {
          buoy = 0.0f;
        }
        else
        {
          float volume, weight;

          weight = pchr->weight + pchr->holdingweight;
          volume = ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min ) * ( pchr->bmpdata.cv.x_max - pchr->bmpdata.cv.x_min ) * ( pchr->bmpdata.cv.y_max - pchr->bmpdata.cv.y_min );

          // this adjusts the buoyancy so that the default adventurer gets a buoyancy of 0.3
          buoy = 0.3f * ( weight / volume ) * 1196.0f;
          if ( buoy < 0.0f ) buoy = 0.0f;
          if ( buoy > 1.0f ) buoy = 1.0f;
        };

        lerp = ( float )( gs->water.surfacelevel - pchr->pos.z ) / ( float ) ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min );
        if ( lerp > 1.0f ) lerp = 1.0f;
        if ( lerp < 0.0f ) lerp = 0.0f;

        loc_traction   += waterfriction * lerp;
        horiz_friction += waterfriction * lerp;
        vert_friction  += waterfriction * lerp;
        pchr->accum_acc.z             -= buoy * gravity  * lerp;

        wt += lerp;
      }

      if ( pchr->pos.z < level + PLATTOLERANCE )
      {
        // we are close to something
        bool_t is_slippy;
        float lerp = ( level + PLATTOLERANCE - pchr->pos.z ) / ( float ) PLATTOLERANCE;
        if ( lerp > 1.0f ) lerp = 1.0f;
        if ( lerp < 0.0f ) lerp = 0.0f;

        if ( VALID_CHR( chrlst, pchr->onwhichplatform ) )
        {
          is_slippy = bfalse;
        }
        else
        {
          is_slippy = ( INVALID_FAN != pchr->onwhichfan ) && mesh_has_some_bits( gs->Mesh_Mem.fanlst, pchr->onwhichfan, MPDFX_SLIPPY );
        }

        if ( is_slippy )
        {
          loc_traction   += ( 1.0 - slippyfriction ) * lerp;
          horiz_friction += slippyfriction * lerp;
        }
        else
        {
          loc_traction   += ( 1.0 - noslipfriction ) * lerp;
          horiz_friction += noslipfriction * lerp;
        };
        vert_friction += loc_airfriction * lerp;

        wt += lerp;
      };

      if ( wt < 1.0f )
      {
        // we are in clear air
        loc_traction   += ( 1.0f - wt ) * air_traction;
        horiz_friction += ( 1.0f - wt ) * loc_airfriction;
        vert_friction  += ( 1.0f - wt ) * loc_airfriction;
      }
      else
      {
        loc_traction   /= wt;
        horiz_friction /= wt;
        vert_friction  /= wt;
      };

      grounded = ( pchr->pos.z < level + PLATTOLERANCE / 20.0f );
    }

    // do volontary movement
    if ( pchr->alive )
    {
      // Apply the latches
      if ( !VALID_CHR( chrlst, imount ) )
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
            pchr->turn_lr = terp_dir( pchr->turn_lr, dvx, dvy, dUpdate * turnfactor );
          }
        }

        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_STOP ) )
        {
          dvx = 0;
          dvy = 0;
        }

        // TODO : change to line(s) below
        maxvel = pchr->skin.maxaccel / ( 1.0 - noslipfriction );
        // set a minimum speed of 6 to fix some stupid slow speeds
        //maxvel = 1.5f * MAX(MAX(3,pchr->spd_run), MAX(pchr->spd_walk,pchr->spd_sneak));
        pstate->trgvel.x = dvx * maxvel;
        pstate->trgvel.y = dvy * maxvel;
        pstate->trgvel.z = 0;

        if ( pchr->skin.maxaccel > 0.0f )
        {
          dvx = ( pstate->trgvel.x - pchr->vel.x );
          dvy = ( pstate->trgvel.y - pchr->vel.y );

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
          //  chrvel2 = pchr->vel.x*pchr->vel.x + pchr->vel.y*pchr->vel.y;
          //  ftmp = MIN(1.0 , chrvel2/maxvel/maxvel);
          //  dvmax   = 2.0f * pchr->maxaccel * (1.0-ftmp);
          //};

          if ( dvx < -dvmax ) dvx = -dvmax;
          if ( dvx >  dvmax ) dvx =  dvmax;
          if ( dvy < -dvmax ) dvy = -dvmax;
          if ( dvy >  dvmax ) dvy =  dvmax;

          loc_traction *= 11.0f;                    // 11.0f corrects traction so that it gives full traction for non-slip floors in advent.mod
          loc_traction = MIN( 1.0, loc_traction );

          pchr->accum_acc.x += dvx * loc_traction * nrm.z;
          pchr->accum_acc.y += dvy * loc_traction * nrm.z;
        };
      }

      // Apply chrlst[].latch.x and chrlst[].latch.y
      if ( !VALID_CHR( chrlst, imount ) )
      {
        // Face the target
        watchtarget = ( loc_turnmode == TURNMODE_WATCHTARGET );
        if ( watchtarget )
        {
          CHR_REF ai_target = chr_get_aitarget( chrlst, MAXCHR, chrlst + chr_ref );
          if ( VALID_CHR( chrlst, ai_target ) && chr_ref != ai_target )
          {
            pchr->turn_lr = terp_dir( pchr->turn_lr, chrlst[ai_target].pos.x - pchr->pos.x, chrlst[ai_target].pos.y - pchr->pos.y, dUpdate * turnfactor );
          };
        }

        // Get direction from ACTUAL change in velocity
        if ( loc_turnmode == TURNMODE_VELOCITY )
        {
          if ( chr_is_player(gs, chr_ref) )
            pchr->turn_lr = terp_dir( pchr->turn_lr, pstate->trgvel.x, pstate->trgvel.y, dUpdate * turnfactor );
          else
            pchr->turn_lr = terp_dir( pchr->turn_lr, pstate->trgvel.x, pstate->trgvel.y, dUpdate * turnfactor / 4.0f );
        }

        // Otherwise make it spin
        else if ( loc_turnmode == TURNMODE_SPIN )
        {
          pchr->turn_lr += SPINRATE * dUpdate * turnfactor;
        }
      };

      // Character latches for generalized buttons
      if ( LATCHBUTTON_NONE != pstate->latch.b )
      {
        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_JUMP ) && pchr->jumptime == 0.0f )
        {
          if ( detach_character_from_mount( gs, chr_ref, btrue, btrue ) )
          {
            pchr->jumptime = DELAY_JUMP;
            pchr->accum_vel.z += ( pchr->flyheight == 0 ) ? DISMOUNTZVEL : DISMOUNTZVELFLY;
            if ( pchr->jumpnumberreset != JUMPINFINITE && pchr->jumpnumber > 0 )
              pchr->jumpnumber -= dUpdate;

            // Play the jump sound
            if ( INVALID_SOUND != caplst[imdl].jumpsound )
            {
              snd_play_sound( gs, 1.0f, pchr->pos, caplst[imdl].wavelist[caplst[imdl].jumpsound], 0, imdl, caplst[imdl].jumpsound );
            };
          }
          else if ( pchr->jumpnumber > 0 && ( pchr->jumpready || pchr->jumpnumberreset > 1 ) )
          {
            // Make the character jump
            if ( pchr->inwater && !grounded )
            {
              pchr->accum_vel.z += WATERJUMP / 3.0f;
              pchr->jumptime = DELAY_JUMP / 3.0f;
            }
            else
            {
              pchr->accum_vel.z += pchr->jump * 2.0f;
              pchr->jumptime = DELAY_JUMP;

              // Set to jump animation if not doing anything better
              if ( pchr->action.ready )    play_action( gs, chr_ref, ACTION_JA, btrue );

              // Play the jump sound (Boing!)
              if ( INVALID_SOUND != caplst[imdl].jumpsound )
              {
                snd_play_sound( gs, MIN( 1.0f, pchr->jump / 50.0f ), pchr->pos, caplst[imdl].wavelist[caplst[imdl].jumpsound], 0, imdl, caplst[imdl].jumpsound );
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
          if ( !chr_using_slot( chrlst, chrlst_size, chr_ref, SLOT_LEFT ) )
          {
            // Grab left
            play_action( gs, chr_ref, ACTION_ME, bfalse );
          }
          else
          {
            // Drop left
            play_action( gs, chr_ref, ACTION_MA, bfalse );
          }
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_ALTRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_GRAB;
          if ( !chr_using_slot( chrlst, chrlst_size, chr_ref, SLOT_RIGHT ) )
          {
            // Grab right
            play_action( gs, chr_ref, ACTION_MF, bfalse );
          }
          else
          {
            // Drop right
            play_action( gs, chr_ref, ACTION_MB, bfalse );
          }
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_PACKLEFT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_PACK;
          item = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_LEFT );
          if ( VALID_CHR( chrlst, item ) )
          {
            if (( chrlst[item].iskursed || caplst[chrlst[item].model].istoobig ) && !caplst[chrlst[item].model].isequipment )
            {
              // The item couldn't be put away
              chrlst[item].aistate.alert |= ALERT_NOTPUTAWAY;
            }
            else
            {
              // Put the item into the pack
              pack_add_item( gs, item, chr_ref );
            }
          }
          else
          {
            // Get a new one out and put it in hand
            pack_get_item( gs, chr_ref, SLOT_LEFT, bfalse );
          }

          // Make it take a little time
          play_action( gs, chr_ref, ACTION_MG, bfalse );
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_PACKRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_PACK;
          item = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_RIGHT );
          if ( VALID_CHR( chrlst, item ) )
          {
            if (( chrlst[item].iskursed || caplst[chrlst[item].model].istoobig ) && !caplst[chrlst[item].model].isequipment )
            {
              // The item couldn't be put away
              chrlst[item].aistate.alert |= ALERT_NOTPUTAWAY;
            }
            else
            {
              // Put the item into the pack
              pack_add_item( gs, item, chr_ref );
            }
          }
          else
          {
            // Get a new one out and put it in hand
            pack_get_item( gs, chr_ref, SLOT_RIGHT, bfalse );
          }

          // Make it take a little time
          play_action( gs,  chr_ref, ACTION_MG, bfalse );
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_LEFT ) && pchr->reloadtime == 0 )
        {
          // Which weapon?
          weapon = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_LEFT );
          if ( !VALID_CHR( chrlst, weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = chr_ref;
          }
          action = caplst[chrlst[weapon].model].weaponaction;

          // Can it do it?
          allowedtoattack = btrue;
          if ( !pmad->actionvalid[action] || chrlst[weapon].reloadtime > 0 ||
               ( caplst[chrlst[weapon].model].needskillidtouse && !check_skills( gs, chr_ref, caplst[chrlst[weapon].model].idsz[IDSZ_SKILL] ) ) )
          {
            allowedtoattack = bfalse;
            if ( chrlst[weapon].reloadtime == 0 )
            {
              // This character can't use this weapon
              chrlst[weapon].reloadtime = 50;
              if ( pchr->staton )
              {
                // Tell the player that they can't use this weapon
                debug_message( 1, "%s can't use this item...", pchr->name );
              }
            }
          }

          if ( action == ACTION_DA )
          {
            allowedtoattack = bfalse;
            if ( chrlst[weapon].reloadtime == 0 )
            {
              chrlst[weapon].aistate.alert |= ALERT_USED;
            }
          }

          if ( allowedtoattack )
          {
            // Rearing imount
            if ( VALID_CHR( chrlst, imount ) )
            {
              allowedtoattack = caplst[chrlst[imount].model].ridercanattack;
              if ( chrlst[imount].ismount && chrlst[imount].alive && !chr_is_player(gs, imount) && chrlst[imount].action.ready )
              {
                if (( action != ACTION_PA || !allowedtoattack ) && pchr->action.ready )
                {
                  play_action( gs,  imount, ( ACTION )( ACTION_UA + ( rand() &1 ) ), bfalse );
                  chrlst[imount].aistate.alert |= ALERT_USED;
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
                if ( pchr->mana_fp8 >= chrlst[weapon].manacost || pchr->canchannel )
                {
                  cost_mana( gs, chr_ref, chrlst[weapon].manacost, weapon );
                  // Check life healing
                  pchr->life_fp8 += chrlst[weapon].lifeheal;
                  if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;
                  ready = ( action == ACTION_PA );
                  action += rand() & 1;
                  play_action( gs,  chr_ref, action, ready );
                  if ( weapon != chr_ref )
                  {
                    // Make the weapon attack too
                    play_action( gs,  weapon, ACTION_MJ, bfalse );
                    chrlst[weapon].aistate.alert |= ALERT_USED;
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
          // Which weapon?
          weapon = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_RIGHT );
          if ( !VALID_CHR( chrlst, weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = chr_ref;
          }
          action = caplst[chrlst[weapon].model].weaponaction + 2;

          // Can it do it?
          allowedtoattack = btrue;
          if ( !pmad->actionvalid[action] || chrlst[weapon].reloadtime > 0 ||
               ( caplst[chrlst[weapon].model].needskillidtouse && !check_skills( gs, chr_ref, caplst[chrlst[weapon].model].idsz[IDSZ_SKILL] ) ) )
          {
            allowedtoattack = bfalse;
            if ( chrlst[weapon].reloadtime == 0 )
            {
              // This character can't use this weapon
              chrlst[weapon].reloadtime = 50;
              if ( pchr->staton )
              {
                // Tell the player that they can't use this weapon
                debug_message( 1, "%s can't use this item...", pchr->name );
              }
            }
          }
          if ( action == ACTION_DC )
          {
            allowedtoattack = bfalse;
            if ( chrlst[weapon].reloadtime == 0 )
            {
              chrlst[weapon].aistate.alert |= ALERT_USED;
            }
          }

          if ( allowedtoattack )
          {
            // Rearing imount
            if ( VALID_CHR( chrlst, imount ) )
            {
              allowedtoattack = caplst[chrlst[imount].model].ridercanattack;
              if ( chrlst[imount].ismount && chrlst[imount].alive && !chr_is_player(gs, imount) && chrlst[imount].action.ready )
              {
                if (( action != ACTION_PC || !allowedtoattack ) && pchr->action.ready )
                {
                  play_action( gs,  imount, ( ACTION )( ACTION_UC + ( rand() &1 ) ), bfalse );
                  chrlst[imount].aistate.alert |= ALERT_USED;
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
                if ( pchr->mana_fp8 >= chrlst[weapon].manacost || pchr->canchannel )
                {
                  cost_mana( gs, chr_ref, chrlst[weapon].manacost, weapon );
                  // Check life healing
                  pchr->life_fp8 += chrlst[weapon].lifeheal;
                  if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;
                  ready = ( action == ACTION_PC );
                  action += rand() & 1;
                  play_action( gs,  chr_ref, action, ready );
                  if ( weapon != chr_ref )
                  {
                    // Make the weapon attack too
                    play_action( gs,  weapon, ACTION_MJ, bfalse );
                    chrlst[weapon].aistate.alert |= ALERT_USED;
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
    }

    // Integrate the z direction
    if ( 0.0f != pchr->flyheight )
    {
      if ( level < 0 ) pchr->accum_pos.z += level - pchr->pos.z; // Don't fall in pits...
      pchr->accum_acc.z += ( level + pchr->flyheight - pchr->pos.z ) * FLYDAMPEN;

      vert_friction = 1.0;
    }
    else if ( pchr->pos.z > level + PLATTOLERANCE )
    {
      pchr->accum_acc.z += gravity;
    }
    else
    {
      float lerp_normal, lerp_tang;
      lerp_tang = ( level + PLATTOLERANCE - pchr->pos.z ) / ( float ) PLATTOLERANCE;
      if ( lerp_tang > 1.0f ) lerp_tang = 1.0f;
      if ( lerp_tang < 0.0f ) lerp_tang = 0.0f;

      // fix to make sure characters will hit the ground softly, but in a reasonable time
      lerp_normal = 1.0 - lerp_tang;
      if ( lerp_normal > 1.0f ) lerp_normal = 1.0f;
      if ( lerp_normal < 0.2f ) lerp_normal = 0.2f;

      // slippy hills make characters slide
      if ( pchr->weight > 0 && gs->water.iswater && !pchr->inwater && INVALID_FAN != pchr->onwhichfan && mesh_has_some_bits( gs->Mesh_Mem.fanlst, pchr->onwhichfan, MPDFX_SLIPPY ) )
      {
        pchr->accum_acc.x -= nrm.x * gravity * lerp_tang * hillslide;
        pchr->accum_acc.y -= nrm.y * gravity * lerp_tang * hillslide;
        pchr->accum_acc.z += nrm.z * gravity * lerp_normal;
      }
      else
      {
        pchr->accum_acc.z += gravity * lerp_normal;
      };
    }

    // Apply friction for next time
    pchr->accum_acc.x -= ( 1.0f - horiz_friction ) * pchr->vel.x;
    pchr->accum_acc.y -= ( 1.0f - horiz_friction ) * pchr->vel.y;
    pchr->accum_acc.z -= ( 1.0f - vert_friction ) * pchr->vel.z;

    // reset the jump
    pchr->jumpready  = grounded || pchr->inwater;
    if ( pchr->jumptime == 0.0f )
    {
      if ( grounded && pchr->jumpnumber < pchr->jumpnumberreset )
      {
        pchr->jumpnumber = pchr->jumpnumberreset;
        pchr->jumptime   = DELAY_JUMP;
      }
      else if ( pchr->inwater && pchr->jumpnumber < 1 )
      {
        // "Swimming"
        pchr->jumpready  = btrue;
        pchr->jumptime   = DELAY_JUMP / 3.0f;
        pchr->jumpnumber += 1;
      }
    };

    // check to see if it can jump
    dojumptimer = btrue;
    if ( grounded )
    {
      // only slippy, non-flat surfaces don't allow jumps
      if ( INVALID_FAN != pchr->onwhichfan && mesh_has_some_bits( gs->Mesh_Mem.fanlst, pchr->onwhichfan, MPDFX_SLIPPY ) )
      {
        if ( !twist_table[twist].flat )
        {
          pchr->jumpready = bfalse;
          dojumptimer     = bfalse;
        };
      }
    }

    if ( dojumptimer )
    {
      pchr->jumptime  -= dUpdate;
      if ( pchr->jumptime < 0 ) pchr->jumptime = 0.0f;
    }

    // Characters with sticky butts lie on the surface of the mesh
    if ( grounded && ( pchr->stickybutt || !pchr->alive ) )
    {
      pchr->mapturn_lr = pchr->mapturn_lr * 0.9 + twist_table[twist].lr * 0.1;
      pchr->mapturn_ud = pchr->mapturn_ud * 0.9 + twist_table[twist].ud * 0.1;
    }

    // Animate the character

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
      pchr->anim.lip_fp8 += 64;

      // handle the mad fx
      if ( pchr->anim.lip_fp8 == 192 )
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
          if ( INVALID_SOUND != caplst[imdl].footfallsound )
          {
            float volume = ( ABS( pchr->vel.x ) +  ABS( pchr->vel.y ) ) / caplst[imdl].spd_sneak;
            snd_play_sound( gs, MIN( 1.0f, volume ), pchr->pos, caplst[imdl].wavelist[caplst[imdl].footfallsound], 0, imdl, caplst[imdl].footfallsound );
          }
        }
      }

      // change frames
      if ( pchr->anim.lip_fp8 == 0 )
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
          else if ( VALID_CHR(chrlst, imount) )
          {
            // See if the character is mounted...
            pchr->action.now = ACTION_MI;

            pchr->anim.next = pmad->actionstart[pchr->action.now];
          }

          pchr->action.ready = btrue;
        }
      }

    };

    // Do "Be careful!" delay
    pchr->carefultime -= dUpdate;
    if ( pchr->carefultime <= 0 ) pchr->carefultime = 0;

    // Get running, walking, sneaking, or dancing, from speed
    if ( !pchr->action.keep && !pchr->action.loop )
    {
      framelip = pmad->framelip[pchr->anim.next];  // 0 - 15...  Way through animation
      if ( pchr->action.ready && pchr->anim.lip_fp8 == 0 && grounded && pchr->flyheight == 0 && ( framelip&7 ) < 2 )
      {
        // Do the motion stuff
        speed = ABS( pchr->vel.x ) + ABS( pchr->vel.y );
        if ( speed < pchr->spd_sneak )
        {
          //                        pchr->action.next = ACTION_DA;
          // Do boredom
          pchr->boretime -= dUpdate;
          if ( pchr->boretime <= 0 ) pchr->boretime = 0;

          if ( pchr->boretime <= 0 )
          {
            pstate->alert |= ALERT_BORED;
            pchr->boretime = DELAY_BORE;
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
          pchr->boretime = DELAY_BORE;
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
  }

}

//--------------------------------------------------------------------------------------------
bool_t PlaList_set_latch( CGame * gs, Player * ppla )
{
  // ZZ> This function converts input readings to latch settings, so players can
  //     move around

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  float newx, newy;
  Uint16 turnsin, character;
  Uint8 device;
  float dist;
  float inputx, inputy;

  // Check to see if we need to bother
  if(NULL == ppla || !ppla->used || INBITS_NONE == ppla->device) return bfalse;

  // Make life easier
  character = ppla->chr_ref;
  device    = ppla->device;

  if( !VALID_CHR(gs->ChrList, character) )
  {
    ppla->used    = bfalse;
    ppla->chr_ref = MAXCHR;
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
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && VALID_CHR( chrlst, character ) && !chrlst[character].alive )
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
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && VALID_CHR( chrlst, character ) && !chrlst[character].alive )
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
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && VALID_CHR( chrlst, character ) && !chrlst[character].alive )
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
    if ( control_key_is_pressed( KEY_RIGHT ) ) inputx += 1;
    if ( control_key_is_pressed( KEY_LEFT  ) ) inputx -= 1;
    if ( control_key_is_pressed( KEY_DOWN  ) ) inputy += 1;
    if ( control_key_is_pressed( KEY_UP    ) ) inputy -= 1;

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
      if ((( gs->modstate.respawnanytime && gs->somepladead ) || ( gs->allpladead && gs->modstate.respawnvalid )) && VALID_CHR( chrlst, character ) && !chrlst[character].alive )
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
void set_local_latches( CGame * gs )
{
  // ZZ> This function emulates AI thinkin' by setting latches from input devices

  int cnt;

  cnt = 0;
  while ( cnt < MAXPLAYER )
  {
    PlaList_set_latch( gs, gs->PlaList + cnt );
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
float get_one_level( CGame * gs, CHR_REF chr_ref )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  float level;
  Uint16 platform;

  // return a default for invalid characters
  if ( !VALID_CHR( chrlst, chr_ref ) ) return bfalse;

  // return the cached value for pre-calculated characters
  if ( chrlst[chr_ref].levelvalid ) return chrlst[chr_ref].level;

  //get the base level
  chrlst[chr_ref].onwhichfan = mesh_get_fan( gs, chrlst[chr_ref].pos );
  level = mesh_get_level( gs, chrlst[chr_ref].onwhichfan, chrlst[chr_ref].pos.x, chrlst[chr_ref].pos.y, chrlst[chr_ref].waterwalk );

  // if there is a platform, choose whichever is higher
  platform = chr_get_onwhichplatform( chrlst, chrlst_size, chr_ref );
  if ( VALID_CHR( chrlst, platform ) )
  {
    float ftmp = chrlst[platform].bmpdata.cv.z_max;
    level = MAX( level, ftmp );
  }

  chrlst[chr_ref].level      = level;
  chrlst[chr_ref].levelvalid = btrue;

  return chrlst[chr_ref].level;
};

//--------------------------------------------------------------------------------------------
void get_all_levels( CGame * gs )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  CHR_REF chr_ref;

  // Initialize all the objects
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !VALID_CHR( chrlst, chr_ref ) ) continue;

    chrlst[chr_ref].onwhichfan = INVALID_FAN;
    chrlst[chr_ref].levelvalid = bfalse;
  };

  // do the levels
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !VALID_CHR( chrlst, chr_ref ) || chrlst[chr_ref].levelvalid ) continue;
    get_one_level( gs, chr_ref );
  }

}

//--------------------------------------------------------------------------------------------
void make_onwhichfan( CGame * gs )
{
  // ZZ> This function figures out which fan characters are on and sets their level

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

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
    if ( !VALID_CHR( chrlst, chr_ref ) ) continue;

    is_inwater = is_underwater = bfalse;
    splashstrength = 0.0f;
    ripplesize = 0.0f;
    if ( INVALID_FAN != chrlst[chr_ref].onwhichfan && mesh_has_some_bits( gs->Mesh_Mem.fanlst, chrlst[chr_ref].onwhichfan, MPDFX_WATER ) )
    {
      splashstrength = chrlst[chr_ref].bmpdata.calc_size_big / 45.0f * chrlst[chr_ref].bmpdata.calc_size / 30.0f;
      if ( chrlst[chr_ref].vel.z > 0.0f ) splashstrength *= 0.5;
      splashstrength *= ABS( chrlst[chr_ref].vel.z ) / 10.0f;
      splashstrength *= chrlst[chr_ref].bumpstrength;
      if ( chrlst[chr_ref].pos.z < gs->water.surfacelevel )
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
      vect3 prt_pos = {chrlst[chr_ref].pos.x, chrlst[chr_ref].pos.y, gs->water.surfacelevel + RAISE};
      Uint16 prt_index;

      // Splash
      prt_index = spawn_one_particle( gs, splashstrength, prt_pos, 0, MAXMODEL, PRTPIP_SPLASH, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );

      // scale the size of the particle
      prtlst[prt_index].size_fp8 *= splashstrength;

      // scale the animation speed so that velocity appears the same
      if ( 0 != prtlst[prt_index].imageadd_fp8 && 0 != prtlst[prt_index].sizeadd_fp8 )
      {
        splashstrength = sqrt( splashstrength );
        prtlst[prt_index].imageadd_fp8 /= splashstrength;
        prtlst[prt_index].sizeadd_fp8  /= splashstrength;
      }
      else
      {
        prtlst[prt_index].imageadd_fp8 /= splashstrength;
        prtlst[prt_index].sizeadd_fp8  /= splashstrength;
      }

      chrlst[chr_ref].inwater = is_inwater;
      if ( gs->water.iswater && is_inwater )
      {
        chrlst[chr_ref].aistate.alert |= ALERT_INWATER;
      }
    }
    else if ( is_inwater && ripplestrength > 0.0f )
    {
      // Ripples
      ripand = ((( int ) chrlst[chr_ref].vel.x ) != 0 ) | ((( int ) chrlst[chr_ref].vel.y ) != 0 );
      ripand = RIPPLEAND >> ripand;
      if ( 0 == ( gs->wld_frame&ripand ) )
      {
        vect3  prt_pos = {chrlst[chr_ref].pos.x, chrlst[chr_ref].pos.y, gs->water.surfacelevel};
        Uint16 prt_index;

        prt_index = spawn_one_particle( gs, ripplestrength, prt_pos, 0, MAXMODEL, PRTPIP_RIPPLE, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );

        // scale the size of the particle
        prtlst[prt_index].size_fp8 *= ripplesize;

        // scale the animation speed so that velocity appears the same
        if ( 0 != prtlst[prt_index].imageadd_fp8 && 0 != prtlst[prt_index].sizeadd_fp8 )
        {
          ripplesize = sqrt( ripplesize );
          prtlst[prt_index].imageadd_fp8 /= ripplesize;
          prtlst[prt_index].sizeadd_fp8  /= ripplesize;
        }
        else
        {
          prtlst[prt_index].imageadd_fp8 /= ripplesize;
          prtlst[prt_index].sizeadd_fp8  /= ripplesize;
        }
      }
    }

    // damage tile stuff
    if ( mesh_has_some_bits( gs->Mesh_Mem.fanlst, chrlst[chr_ref].onwhichfan, MPDFX_DAMAGE ) && chrlst[chr_ref].pos.z <= gs->water.surfacelevel + DAMAGERAISE )
    {
      Uint8 loc_damagemodifier;
      CHR_REF imount;

      // augment the rider's damage immunity with the mount's
      loc_damagemodifier = chrlst[chr_ref].skin.damagemodifier_fp8[GTile_Dam.type];
      imount = chr_get_attachedto(chrlst, chrlst_size, chr_ref);
      if ( VALID_CHR(chrlst, imount) )
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
      if ( !HAS_ALL_BITS(loc_damagemodifier, DAMAGE_SHIFT ) && !chrlst[chr_ref].invictus )  
      {
        if ( chrlst[chr_ref].damagetime == 0 )
        {
          PAIR ptemp = {GTile_Dam.amount, 1};
          damage_character( gs, chr_ref, 32768, &ptemp, GTile_Dam.type, TEAM_DAMAGE, chr_get_aibumplast( chrlst, chrlst_size, chrlst + chr_ref ), DAMFX_BLOC | DAMFX_ARMO );
          chrlst[chr_ref].damagetime = DELAY_DAMAGETILE;
        }

        if ( GTile_Dam.parttype != MAXPRTPIP && ( gs->wld_frame&GTile_Dam.partand ) == 0 )
        {
          spawn_one_particle( gs, 1.0f, chrlst[chr_ref].pos,
                              0, MAXMODEL, GTile_Dam.parttype, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
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
bool_t remove_from_platform( CGame * gs, CHR_REF object_ref )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Uint16 platform;
  if ( !VALID_CHR( chrlst, object_ref ) ) return bfalse;

  platform  = chr_get_onwhichplatform( chrlst, chrlst_size, object_ref );
  if ( !VALID_CHR( chrlst, platform ) ) return bfalse;

  if ( chrlst[object_ref].weight > 0.0f )
    chrlst[platform].weight -= chrlst[object_ref].weight;

  chrlst[object_ref].onwhichplatform = MAXCHR;
  chrlst[object_ref].level           = chrlst[platform].level;

  if ( chr_is_player(gs, object_ref) && CData.DevMode )
  {
    debug_message( 1, "removed %s(%s) from platform", chrlst[object_ref].name, caplst[chrlst[object_ref].model].classname );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_to_platform( CGame * gs, CHR_REF object_ref, Uint16 platform )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  remove_from_platform( gs, object_ref );

  if ( !VALID_CHR( chrlst, object_ref ) || !VALID_CHR( chrlst, platform ) ) return
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
    debug_message( 1, "attached %s(%s) to platform", chrlst[object_ref].name, caplst[chrlst[object_ref].model].classname );
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
void create_bumplists(CGame * gs)
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  CHR_REF chr_ref;
  PRT_REF prt_ref;
  Uint32  fanblock;
  Uint8   hidestate;

  // Clear the lists
  reset_bumplist();

  // Fill 'em back up
  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    int ix, ix_min, ix_max;
    int iy, iy_min, iy_max;

    // ignore invalid characters and objects that are packed away
    if ( !VALID_CHR( chrlst, chr_ref ) || chr_in_pack( chrlst, chrlst_size, chr_ref ) || chrlst[chr_ref].gopoof ) continue;

    // do not include hidden objects
    hidestate = caplst[chrlst[chr_ref].model].hidestate;
    if ( hidestate != NOHIDE && hidestate == chrlst[chr_ref].aistate.state ) continue;

    ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( &(gs->mesh), chrlst[chr_ref].bmpdata.cv.x_min ) );
    ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( &(gs->mesh), chrlst[chr_ref].bmpdata.cv.x_max ) );
    iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( &(gs->mesh), chrlst[chr_ref].bmpdata.cv.y_min ) );
    iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( &(gs->mesh), chrlst[chr_ref].bmpdata.cv.y_max ) );

    for ( ix = ix_min; ix <= ix_max; ix++ )
    {
      for ( iy = iy_min; iy <= iy_max; iy++ )
      {
        fanblock = mesh_convert_block( &(gs->mesh), ix, iy );
        if ( INVALID_FAN == fanblock ) continue;

        // Insert before any other characters on the block
        bumplist_insert_chr(&bumplist, fanblock, chr_ref);
      }
    }
  };

  for ( prt_ref = 0; prt_ref < prtlst_size; prt_ref++ )
  {
    // ignore invalid particles
    if ( !VALID_PRT( prtlst, prt_ref ) || prtlst[prt_ref].gopoof ) continue;

    fanblock = mesh_get_block( &(gs->mesh), prtlst[prt_ref].pos );
    if ( INVALID_FAN == fanblock ) continue;

    // Insert before any other particles on the block
    bumplist_insert_prt(&bumplist, fanblock, prt_ref);
  }

  bumplist.filled = btrue;
};

//--------------------------------------------------------------------------------------------
bool_t find_collision_volume( vect3 * ppa, CVolume * pva, vect3 * ppb, CVolume * pvb, bool_t exclude_vert, CVolume * pcv)
{
  bool_t retval = bfalse, bfound;
  CVolume cv, tmp_cv;
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
bool_t chr_is_inside( CGame * gs, CHR_REF chra_ref, float lerp, CHR_REF chrb_ref, bool_t exclude_vert )
{
  // BB > Find whether an active point of chra_ref is "inside" chrb_ref's bounding volume.
  //      Abstraction of the old algorithm to see whether a character cpold be "above" another

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  float ftmp;
  BData * ba, * bb;

  if( !VALID_CHR(chrlst, chra_ref) || !VALID_CHR(chrlst, chrb_ref) ) return bfalse;

  ba = &(chrlst[chra_ref].bmpdata);
  bb = &(chrlst[chrb_ref].bmpdata);

  //---- vertical ----
  if( !exclude_vert )
  {
    ftmp = ba->mids_lo.z + (ba->mids_hi.z - ba->mids_lo.z) * lerp;
    if( ftmp < bb->cv.z_min + bb->mids_lo.z || ftmp > bb->cv.z_max + bb->mids_lo.z ) return bfalse;
  }

  //---- diamond ----
  if( bb->cv.level > 1 )
  {
    ftmp = ba->mids_lo.x + ba->mids_lo.y;
    if( ftmp < bb->cv.xy_min + bb->mids_lo.x + bb->mids_lo.y || ftmp > bb->cv.xy_max + bb->mids_lo.x + bb->mids_lo.y ) return bfalse;

    ftmp = -ba->mids_lo.x + ba->mids_lo.y;
    if( ftmp < bb->cv.yx_min - bb->mids_lo.x + bb->mids_lo.y || ftmp > bb->cv.yx_max - bb->mids_lo.x + bb->mids_lo.y ) return bfalse;
  };

  //---- square ----
  ftmp = ba->mids_lo.x;
  if( ftmp < bb->cv.x_min + bb->mids_lo.x || ftmp > bb->cv.x_max + bb->mids_lo.x ) return bfalse;

  ftmp = ba->mids_lo.y;
  if( ftmp < bb->cv.y_min + bb->mids_lo.y || ftmp > bb->cv.y_max + bb->mids_lo.y ) return bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_do_collision( CGame * gs, CHR_REF chra_ref, CHR_REF chrb_ref, bool_t exclude_height, CVolume * cv)
{
  // BB > use the bounding boxes to determine whether a collision has occurred.
  //      there are currently 3 levels of collision detection.
  //      level 1 - the basic square axis-aligned bounding box
  //      level 2 - the octagon bounding box calculated from the actual vertex positions
  //      level 3 - an "octree" of bounding bounding boxes calculated from the actual trianglr positions
  //  the level is chosen by the global variable chrcollisionlevel

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  bool_t retval = bfalse;

  // set the minimum bumper level for object a
  if(chrlst[chra_ref].bmpdata.cv.level < 1)
  {
    chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chra_ref, 1);
  }

  // set the minimum bumper level for object b
  if(chrlst[chrb_ref].bmpdata.cv.level < 1)
  {
    chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chrb_ref, 1);
  }

  // find the simplest collision volume
  find_collision_volume( &(chrlst[chra_ref].pos), &(chrlst[chra_ref].bmpdata.cv), &(chrlst[chrb_ref].pos), &(chrlst[chrb_ref].bmpdata.cv), exclude_height, cv);

  if ( chrcollisionlevel>1 && retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(chrlst[chra_ref].bmpdata.cv.level < 2)
    {
      chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chra_ref, 2);
      was_refined = btrue;
    }

    // refine the bumper
    if(chrlst[chrb_ref].bmpdata.cv.level < 2)
    {
      chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chrb_ref, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(chrlst[chra_ref].pos), &(chrlst[chra_ref].bmpdata.cv), &(chrlst[chrb_ref].pos), &(chrlst[chrb_ref].bmpdata.cv), exclude_height, cv);
    };

    if(chrcollisionlevel>2 && retval)
    {
      was_refined = bfalse;

      // refine the bumper
      if(chrlst[chra_ref].bmpdata.cv.level < 3)
      {
        chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chra_ref, 3);
        was_refined = btrue;
      }

      // refine the bumper
      if(chrlst[chrb_ref].bmpdata.cv.level < 3)
      {
        chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chrb_ref, 3);
        was_refined = btrue;
      }

      assert(NULL != chrlst[chra_ref].bmpdata.cv_tree);
      assert(NULL != chrlst[chrb_ref].bmpdata.cv_tree);

      if(was_refined)
      {
        int cnt, tnc;
        CVolume cv3, tmp_cv;
        bool_t loc_retval;

        retval = bfalse;
        cv3.level = -1;
        for(cnt=0; cnt<8; cnt++)
        {
          if(-1 == (*chrlst[chra_ref].bmpdata.cv_tree)[cnt].level) continue;

          for(tnc=0; tnc<8; tnc++)
          {
            if(-1 == (*chrlst[chrb_ref].bmpdata.cv_tree)[cnt].level) continue;

            loc_retval = find_collision_volume( &(chrlst[chra_ref].pos), &((*chrlst[chra_ref].bmpdata.cv_tree)[cnt]), &(chrlst[chrb_ref].pos), &((*chrlst[chrb_ref].bmpdata.cv_tree)[cnt]), exclude_height, &tmp_cv);

            if(loc_retval)
            {
              retval = btrue;
              cv3 = cvolume_merge(&cv3, &tmp_cv);

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
bool_t prt_do_collision( CGame * gs, CHR_REF chra_ref, PRT_REF prtb, bool_t exclude_height )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  bool_t retval = find_collision_volume( &(chrlst[chra_ref].pos), &(chrlst[chra_ref].bmpdata.cv), &(chrlst[prtb].pos), &(chrlst[prtb].bmpdata.cv), bfalse, NULL );

  if ( retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(chrlst[chra_ref].bmpdata.cv.level < 2)
    {
      chr_calculate_bumpers( gs->MadList, MAXMODEL, chrlst + chra_ref, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(chrlst[chra_ref].pos), &(chrlst[chra_ref].bmpdata.cv), &(chrlst[prtb].pos), &(chrlst[prtb].bmpdata.cv), bfalse, NULL );
    };
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void do_bumping( CGame * gs, float dUpdate )
{
  // ZZ> This function sets handles characters hitting other characters or particles

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint32  blnode_a, blnode_b;
  CHR_REF chra_ref, chrb_ref;
  Uint32 fanblock;
  int cnt, tnc, chrinblock, prtinblock;
  vect3 apos, bpos;

  float loc_platkeep, loc_platascend, loc_platstick;

  loc_platascend = pow( PLATASCEND, 1.0 / dUpdate );
  loc_platkeep   = 1.0 - loc_platascend;
  loc_platstick  = pow( platstick, 1.0 / dUpdate );

  create_bumplists(gs);
  cv_list_clear();

  // Check collisions with other characters and bump particles
  // Only check each pair once
  for ( fanblock = 0; fanblock < bumplist.num_blocks; fanblock++ )
  {
    chrinblock = bumplist_get_chr_count(&bumplist, fanblock);
    prtinblock = bumplist_get_prt_count(&bumplist, fanblock);

    //// remove bad platforms
    //for ( cnt = 0, blnode_a = bumplist_get_chr_head(&bumplist, fanblock);
    //      cnt < chrinblock && INVALID_BUNPLIST_NODE != blnode_a;
    //      cnt++, blnode_a = bumplist_get_next_chr( &bumplist, blnode_a ) )
    //{
    //  chra_ref = bumplist_get_ref(&bumplist, blnode_a);
    //  VALID_CHR( chrlst, chra_ref );
    //
    //  // detach character from invalid platforms
    //  chrb_ref  = chr_get_onwhichplatform( chra_ref );
    //  if ( VALID_CHR( chrlst, chrb_ref ) )
    //  {
    //    if ( !chr_is_inside( chra_ref, 0.0f, chrb_ref, btrue ) ||
    //         chrlst[chrb_ref].bmpdata.cv.z_min  > chrlst[chrb_ref].bmpdata.cv.z_max - PLATTOLERANCE )
    //    {
    //      remove_from_platform( chra_ref );
    //    }
    //  }
    //};

    //// do attachments
    //for ( cnt = 0, blnode_a = bumplist_get_chr_head(&bumplist, fanblock);
    //      cnt < chrinblock && INVALID_BUNPLIST_NODE != blnode_a;
    //      cnt++, blnode_a = bumplist_get_next_chr( &bumplist, blnode_a ) )
    //{
    //  chra_ref = bumplist_get_ref(&bumplist, blnode_a);
    //  VALID_CHR( chrlst, chra_ref );
    //
    //  // Do platforms (no interaction with held or mounted items)
    //  if ( chr_attached( chra_ref ) ) continue;

    //  for ( blnode_b = bumplist_get_next_chr( &bumplist, blnode_a ), tnc = cnt + 1;
    //        tnc < chrinblock && VALID_CHR_RANGE( chrb_ref );
    //        tnc++, blnode_b = bumplist_get_next_chr( &bumplist, blnode_b ) )
    //  {
    //    chrb_ref = bumplist_get_ref(&bumplist, blnode_b);
    //    VALID_CHR( chrlst, chrb_ref );
    //
    //    // do not put something on a platform that is being carried by someone
    //    if ( chr_attached( chrb_ref ) ) continue;

    //    // do not consider anything that is already a item/platform combo
    //    if ( chrlst[chra_ref].onwhichplatform == chrb_ref || chrlst[chrb_ref].onwhichplatform == chra_ref ) continue;

    //    if ( chr_is_inside( chra_ref, 0.0f, chrb_ref, btrue) )
    //    {
    //      // check for compatibility
    //      if ( chrlst[chrb_ref].bmpdata.calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( chrlst[chra_ref].pos.z > MAX( chrlst[chrb_ref].bmpdata.cv.z_min, chrlst[chrb_ref].bmpdata.cv.z_max - PLATTOLERANCE ) && chrlst[chra_ref].level < chrlst[chrb_ref].bmpdata.cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chra_ref, chrb_ref );
    //        }
    //      }
    //    }
    //
    //    if( chr_is_inside( chrb_ref, 0.0f, chra_ref, btrue) )
    //    {
    //      if ( chrlst[chra_ref].bmpdata.calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( chrlst[chrb_ref].pos.z > MAX( chrlst[chra_ref].bmpdata.cv.z_min, chrlst[chra_ref].bmpdata.cv.z_max - PLATTOLERANCE ) && chrlst[chrb_ref].level < chrlst[chra_ref].bmpdata.cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chrb_ref, chra_ref );
    //        }
    //      }
    //    }

    //  }
    //}

    //// Do mounting
    //for ( cnt = 0, blnode_a = bumplist_get_chr_head(&bumplist, fanblock);
    //      cnt < chrinblock && INVALID_BUNPLIST_NODE != blnode_a;
    //      cnt++, blnode_a = bumplist_get_next_chr( &bumplist, blnode_a ) )
    //{
    //  chra_ref = bumplist_get_ref(&bumplist, blnode_a);
    //  VALID_CHR( chrlst, chra_ref );
    //
    //  if ( chr_attached( chra_ref ) ) continue;

    //  for ( blnode_b = bumplist_get_next_chr( &bumplist, blnode_a ), tnc = cnt + 1;
    //        tnc < chrinblock && VALID_CHR_RANGE( chrb_ref );
    //        tnc++, blnode_b = bumplist_get_next_chr( &bumplist, blnode_b ) )
    //  {
    //    chrb_ref = bumplist_get_ref(&bumplist, blnode_b);
    //    VALID_CHR( chrlst, chrb_ref );
    //
    //    // do not mount something that is being carried by someone
    //    if ( chr_attached( chrb_ref ) ) continue;

    //    if ( chr_is_inside( chra_ref, 0.0f, chrb_ref, btrue)   )
    //    {

    //      // Now see if either is on top the other like a platform
    //      if ( chrlst[chra_ref].pos.z > chrlst[chrb_ref].bmpdata.cv.z_max - PLATTOLERANCE && chrlst[chra_ref].pos.z < chrlst[chrb_ref].bmpdata.cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is A falling on B?
    //        if ( chrlst[chra_ref].vel.z < chrlst[chrb_ref].vel.z )
    //        {
    //          if ( chrlst[chra_ref].flyheight == 0 && chrlst[chra_ref].alive && madlst[chrlst[chra_ref].model].actionvalid[ACTION_MI] && !chrlst[chra_ref].isitem )
    //          {
    //            if ( chrlst[chrb_ref].alive && chrlst[chrb_ref].ismount && !chr_using_slot( chrb_ref, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chra_ref );
    //              if ( !attach_character_to_mount( chra_ref, chrb_ref, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                chrlst[chra_ref].aistate.alert |= ALERT_BUMPED;
    //                chrlst[chrb_ref].aistate.alert |= ALERT_BUMPED;
    //                chrlst[chra_ref].aistate.bumplast = chrb_ref;
    //                chrlst[chrb_ref].aistate.bumplast = chra_ref;
    //              };
    //            }
    //          }
    //        }
    //      }

    //    }

    //    if( chr_is_inside( chrb_ref, 0.0f, chra_ref, btrue)   )
    //    {
    //      if ( chrlst[chrb_ref].pos.z > chrlst[chra_ref].bmpdata.cv.z_max - PLATTOLERANCE && chrlst[chrb_ref].pos.z < chrlst[chra_ref].bmpdata.cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is B falling on A?
    //        if ( chrlst[chrb_ref].vel.z < chrlst[chra_ref].vel.z )
    //        {
    //          if ( chrlst[chrb_ref].flyheight == 0 && chrlst[chrb_ref].alive && madlst[chrlst[chrb_ref].model].actionvalid[ACTION_MI] && !chrlst[chrb_ref].isitem )
    //          {
    //            if ( chrlst[chra_ref].alive && chrlst[chra_ref].ismount && !chr_using_slot( chra_ref, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chrb_ref );
    //              if ( !attach_character_to_mount( chrb_ref, chra_ref, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                chrlst[chra_ref].aistate.alert |= ALERT_BUMPED;
    //                chrlst[chrb_ref].aistate.alert |= ALERT_BUMPED;
    //                chrlst[chra_ref].aistate.bumplast = chrb_ref;
    //                chrlst[chrb_ref].aistate.bumplast = chra_ref;
    //              };
    //            };
    //          }
    //        }
    //      }
    //    }
    //  }
    //}

    // do collisions
    for ( cnt = 0, blnode_a = bumplist_get_chr_head(&bumplist, fanblock);
          cnt < chrinblock && INVALID_BUMPLIST_NODE != blnode_a;
          cnt++, blnode_a = bumplist_get_next_chr( gs, &bumplist, blnode_a ) )
    {
      float lerpa;

      chra_ref = bumplist_get_ref(&bumplist, blnode_a);
      VALID_CHR( chrlst, chra_ref );
    
      lerpa = (chrlst[chra_ref].pos.z - chrlst[chra_ref].level) / PLATTOLERANCE;
      lerpa = CLIP(lerpa, 0, 1);

      apos = chrlst[chra_ref].pos;

      // don't do object-object collisions if they won't feel each other
      if ( chrlst[chra_ref].bumpstrength == 0.0f ) continue;

      // Do collisions (but not with attached items/characers)
      for ( blnode_b = bumplist_get_next_chr(gs, &bumplist, blnode_a), tnc = cnt + 1;
            tnc < chrinblock && INVALID_BUMPLIST_NODE != blnode_b;
            tnc++, blnode_b = bumplist_get_next_chr( gs, &bumplist, blnode_b ) )
      {
        CVolume cv;
        float lerpb, bumpstrength;

        chrb_ref = bumplist_get_ref(&bumplist, blnode_b);
        VALID_CHR( chrlst, chrb_ref );

        bumpstrength = chrlst[chra_ref].bumpstrength * chrlst[chrb_ref].bumpstrength;

        // don't do object-object collisions if they won't feel eachother
        if ( bumpstrength == 0.0f ) continue;

        // do not collide with something you are already holding
        if ( chrb_ref == chrlst[chra_ref].attachedto || chra_ref == chrlst[chrb_ref].attachedto ) continue;

        // do not collide with a your platform
        if ( chrb_ref == chrlst[chra_ref].onwhichplatform || chra_ref == chrlst[chrb_ref].onwhichplatform ) continue;

        bpos = chrlst[chrb_ref].pos;

        lerpb = (chrlst[chrb_ref].pos.z - chrlst[chrb_ref].level) / PLATTOLERANCE;
        lerpb = CLIP(lerpb, 0, 1);

        if ( chr_do_collision( gs, chra_ref, chrb_ref, bfalse, &cv) )
        {
          vect3 depth, ovlap, nrm, diffa, diffb;
          float dotprod, pressure;
          float cr, m0, m1, psum, msum, udif, u0, u1, ln_cr;
          bool_t bfound;

          depth.x = (cv.x_max - cv.x_min);
          ovlap.x = depth.x / MIN(chrlst[chra_ref].bmpdata.cv.x_max - chrlst[chra_ref].bmpdata.cv.x_min, chrlst[chrb_ref].bmpdata.cv.x_max - chrlst[chrb_ref].bmpdata.cv.x_min);
          ovlap.x = CLIP(ovlap.x,-1,1);
          nrm.x = 1.0f / ovlap.x;

          depth.y = (cv.y_max - cv.y_min);
          ovlap.y = depth.y / MIN(chrlst[chra_ref].bmpdata.cv.y_max - chrlst[chra_ref].bmpdata.cv.y_min, chrlst[chrb_ref].bmpdata.cv.y_max - chrlst[chrb_ref].bmpdata.cv.y_min);
          ovlap.y = CLIP(ovlap.y,-1,1);
          nrm.y = 1.0f / ovlap.y;

          depth.z = (cv.z_max - cv.z_min);
          ovlap.z = depth.z / MIN(chrlst[chra_ref].bmpdata.cv.z_max - chrlst[chra_ref].bmpdata.cv.z_min, chrlst[chrb_ref].bmpdata.cv.z_max - chrlst[chrb_ref].bmpdata.cv.z_min);
          ovlap.z = CLIP(ovlap.z,-1,1);
          nrm.z = 1.0f / ovlap.z;

          nrm = Normalize(nrm);

          pressure = (depth.x / 30.0f) * (depth.y / 30.0f) * (depth.z / 30.0f);

          if(ovlap.x != 1.0)
          {
            diffa.x = chrlst[chra_ref].bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
            diffb.x = chrlst[chrb_ref].bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
          }
          else
          {
            diffa.x = chrlst[chra_ref].bmpdata.mids_lo.x - chrlst[chrb_ref].bmpdata.mids_lo.x;
            diffb.x =-diffa.x;
          }

          if(ovlap.y != 1.0)
          {
            diffa.y = chrlst[chra_ref].bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
            diffb.y = chrlst[chrb_ref].bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
          }
          else
          {
            diffa.y = chrlst[chra_ref].bmpdata.mids_lo.y - chrlst[chrb_ref].bmpdata.mids_lo.y;
            diffb.y =-diffa.y;
          }

          if(ovlap.y != 1.0)
          {
            diffa.z = chrlst[chra_ref].bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffa.z += (chrlst[chra_ref].bmpdata.mids_hi.z - chrlst[chra_ref].bmpdata.mids_lo.z) * lerpa;

            diffb.z = chrlst[chrb_ref].bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffb.z += (chrlst[chrb_ref].bmpdata.mids_hi.z - chrlst[chrb_ref].bmpdata.mids_lo.z) * lerpb;
          }
          else
          {
            diffa.z  = chrlst[chra_ref].bmpdata.mids_lo.z - chrlst[chrb_ref].bmpdata.mids_lo.z;
            diffa.z += (chrlst[chra_ref].bmpdata.mids_hi.z - chrlst[chra_ref].bmpdata.mids_lo.z) * lerpa;
            diffa.z -= (chrlst[chrb_ref].bmpdata.mids_hi.z - chrlst[chrb_ref].bmpdata.mids_lo.z) * lerpb;

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
          //cr = chrlst[chrb_ref].bumpdampen * chrlst[chra_ref].bumpdampen * bumpstrength * ovlap.z * ( nrm.x * nrm.x * ovlap.x + nrm.y * nrm.y * ovlap.y ) / ftmp;

          // determine a usable mass
          m0 = -1;
          m1 = -1;
          if ( chrlst[chra_ref].weight < 0 && chrlst[chrb_ref].weight < 0 )
          {
            m0 = m1 = 110.0f;
          }
          else if (chrlst[chra_ref].weight == 0 && chrlst[chrb_ref].weight == 0)
          {
            m0 = m1 = 1.0f;
          }
          else
          {
            m0 = (chrlst[chra_ref].weight == 0.0f) ? 1.0 : chrlst[chra_ref].weight;
            m1 = (chrlst[chrb_ref].weight == 0.0f) ? 1.0 : chrlst[chrb_ref].weight;
          }

          bfound = btrue;
          cr = chrlst[chrb_ref].bumpdampen * chrlst[chra_ref].bumpdampen;
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

            chrlst[chra_ref].accum_acc.x += (diffa.x * k  - chrlst[chra_ref].vel.x * gamma) * bumpstrength;
            chrlst[chra_ref].accum_acc.y += (diffa.y * k  - chrlst[chra_ref].vel.y * gamma) * bumpstrength;
            chrlst[chra_ref].accum_acc.z += (diffa.z * k  - chrlst[chra_ref].vel.z * gamma) * bumpstrength;
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

            chrlst[chrb_ref].accum_acc.x += (diffb.x * k  - chrlst[chrb_ref].vel.x * gamma) * bumpstrength;
            chrlst[chrb_ref].accum_acc.y += (diffb.y * k  - chrlst[chrb_ref].vel.y * gamma) * bumpstrength;
            chrlst[chrb_ref].accum_acc.z += (diffb.z * k  - chrlst[chrb_ref].vel.z * gamma) * bumpstrength;
          }

          //bfound = bfalse;
          //if (( chrlst[chra_ref].bmpdata.mids_lo.x - chrlst[chrb_ref].bmpdata.mids_lo.x ) * ( chrlst[chra_ref].vel.x - chrlst[chrb_ref].vel.x ) < 0.0f )
          //{
          //  u0 = chrlst[chra_ref].vel.x;
          //  u1 = chrlst[chrb_ref].vel.x;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  chrlst[chra_ref].vel.x = ( psum - m1 * udif * cr ) / msum;
          //  chrlst[chrb_ref].vel.x = ( psum + m0 * udif * cr ) / msum;

          //  //chrlst[chra_ref].bmpdata.mids_lo.x -= chrlst[chra_ref].vel.x*dUpdate;
          //  //chrlst[chrb_ref].bmpdata.mids_lo.x -= chrlst[chrb_ref].vel.x*dUpdate;

          //  bfound = btrue;
          //}

          //if (( chrlst[chra_ref].bmpdata.mids_lo.y - chrlst[chrb_ref].bmpdata.mids_lo.y ) * ( chrlst[chra_ref].vel.y - chrlst[chrb_ref].vel.y ) < 0.0f )
          //{
          //  u0 = chrlst[chra_ref].vel.y;
          //  u1 = chrlst[chrb_ref].vel.y;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  chrlst[chra_ref].vel.y = ( psum - m1 * udif * cr ) / msum;
          //  chrlst[chrb_ref].vel.y = ( psum + m0 * udif * cr ) / msum;

          //  //chrlst[chra_ref].bmpdata.mids_lo.y -= chrlst[chra_ref].vel.y*dUpdate;
          //  //chrlst[chrb_ref].bmpdata.mids_lo.y -= chrlst[chrb_ref].vel.y*dUpdate;

          //  bfound = btrue;
          //}

          //if ( ovlap.x > 0 && ovlap.z > 0 )
          //{
          //  chrlst[chra_ref].bmpdata.mids_lo.x += m1 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  chrlst[chrb_ref].bmpdata.mids_lo.x -= m0 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  bfound = btrue;
          //}

          //if ( ovlap.y > 0 && ovlap.z > 0 )
          //{
          //  chrlst[chra_ref].bmpdata.mids_lo.y += m1 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  chrlst[chrb_ref].bmpdata.mids_lo.y -= m0 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  bfound = btrue;
          //}

          if ( bfound )
          {
            //apos = chrlst[chra_ref].pos;
            chrlst[chra_ref].aistate.alert |= ALERT_BUMPED;
            chrlst[chrb_ref].aistate.alert |= ALERT_BUMPED;
            chrlst[chra_ref].aistate.bumplast = chrb_ref;
            chrlst[chrb_ref].aistate.bumplast = chra_ref;
          };
        }
      }
    };

    // Now check collisions with every bump particle in same area
    //for ( cnt = 0, blnode_a = bumplist_get_chr_head( &bumplist, fanblock );
    //      cnt < chrinblock && INVALID_BUNPLIST_NODE != blnode_a;
    //      cnt++, blnode_a = bumplist_get_next_chr( &bumplist, blnode_a ) )
    //{
    //  IDSZ chridvulnerability, eveidremove;
    //  float chrbump = 1.0f;

    //  chra_ref = bumplist_get_ref( &bumplist, blnode_a );
    //  assert(VALID_CHR( chrlst, chra_ref ));
    //

    //  apos = chrlst[chra_ref].pos;
    //  chridvulnerability = caplst[chrlst[chra_ref].model].idsz[IDSZ_VULNERABILITY];
    //  chrbump = chrlst[chra_ref].bumpstrength;

    //  // Check for object-particle interaction
    //  for ( tnc = 0, blnode_b = bumplist_get_prt_head(&bumplist, fanblock);
    //        tnc < prtinblock && INVALID_BUMPLIST_NODE != blnode_b;
    //        tnc++ , blnode_b = bumplist_get_next_prt( &bumplist, blnode_b ) )
    //  {
    //    float bumpstrength, prtbump;
    //    bool_t chr_is_vulnerable;

    //    prtb = bumplist_get_ref( &bumplist, blnode_b);

    //    CHR_REF prt_owner = prt_get_owner( prtb );
    //    CHR_REF prt_attached = prt_get_attachedtochr( prtb );

    //    pip = prtlst[prtb].pip;
    //    bpos = prtlst[prtb].pos;

    //    chr_is_vulnerable = !chrlst[chra_ref].invictus && ( IDSZ_NONE != chridvulnerability ) && CAP_INHERIT_IDSZ( gs,  prtlst[prtb].model, chridvulnerability );

    //    prtbump = prtlst[prtb].bumpstrength;
    //    bumpstrength = chr_is_vulnerable ? 1.0f : chrbump * prtbump;

    //    if ( 0.0f == bumpstrength ) continue;

    //    // First check absolute value diamond
    //    diff.x = ABS( apos.x - bpos.x );
    //    diff.y = ABS( apos.y - bpos.y );
    //    dist = diff.x + diff.y;
    //    if ( prt_do_collision( chra_ref, prtb, bfalse ) )
    //    {
    //      vect3 pvel;

    //      if ( MAXCHR != prt_get_attachedtochr( prtb ) )
    //      {
    //        pvel.x = ( prtlst[prtb].pos.x - prtlst[prtb].pos_old.x ) / dUpdate;
    //        pvel.y = ( prtlst[prtb].pos.y - prtlst[prtb].pos_old.y ) / dUpdate;
    //        pvel.z = ( prtlst[prtb].pos.z - prtlst[prtb].pos_old.z ) / dUpdate;
    //      }
    //      else
    //      {
    //        pvel = prtlst[prtb].vel;
    //      }

    //      if ( bpos.z > chrlst[chra_ref].bmpdata.cv.z_max + pvel.z && pvel.z < 0 && chrlst[chra_ref].bmpdata.calc_is_platform && !VALID_CHR( chrlst, prt_attached ) )
    //      {
    //        // Particle is falling on A
    //        prtlst[prtb].accum_pos.z += chrlst[chra_ref].bmpdata.cv.z_max - prtlst[prtb].pos.z;

    //        prtlst[prtb].accum_vel.z = - (1.0f - piplst[pip].dampen * chrlst[chra_ref].bumpdampen) * prtlst[prtb].vel.z;

    //        prtlst[prtb].accum_acc.x += ( pvel.x - chrlst[chra_ref].vel.x ) * ( 1.0 - loc_platstick ) + chrlst[chra_ref].vel.x;
    //        prtlst[prtb].accum_acc.y += ( pvel.y - chrlst[chra_ref].vel.y ) * ( 1.0 - loc_platstick ) + chrlst[chra_ref].vel.y;
    //      }

    //      // Check reaffirmation of particles
    //      if ( prt_attached != chra_ref )
    //      {
    //        if ( chrlst[chra_ref].reloadtime == 0 )
    //        {
    //          if ( chrlst[chra_ref].reaffirmdamagetype == prtlst[prtb].damagetype && chrlst[chra_ref].damagetime == 0 )
    //          {
    //            reaffirm_attached_particles( chra_ref );
    //          }
    //        }
    //      }

    //      // Check for missile treatment
    //      if (( chrlst[chra_ref].damagemodifier_fp8[prtlst[prtb].damagetype]&DAMAGE_SHIFT ) != DAMAGE_SHIFT ||
    //            MIS_NORMAL == chrlst[chra_ref].missiletreatment  ||
    //            VALID_CHR( chrlst, prt_attached ) ||
    //            ( prt_owner == chra_ref && !piplst[pip].friendlyfire ) ||
    //            ( chrlst[chrmissilehandler[chra_ref]].mana_fp8 < ( chrlst[chra_ref].missilecost << 4 ) && !chrlst[chrmissilehandler[chra_ref]].canchannel ) )
    //      {
    //        if (( gs->TeamList[prtlst[prtb].team].hatesteam[chrlst[chra_ref].team] || ( piplst[pip].friendlyfire && (( chra_ref != prt_owner && chra_ref != chrlst[prt_owner].attachedto ) || piplst[pip].onlydamagefriendly ) ) ) && !chrlst[chra_ref].invictus )
    //        {
    //          spawn_bump_particles( chra_ref, prtb );  // Catch on fire

    //          if (( prtlst[prtb].damage.ibase > 0 ) && ( prtlst[prtb].damage.irand > 0 ) )
    //          {
    //            if ( chrlst[chra_ref].damagetime == 0 && prt_attached != chra_ref && HAS_NO_BITS( piplst[pip].damfx, DAMFX_ARRO ) )
    //            {

    //              // Normal prtb damage
    //              if ( piplst[pip].allowpush )
    //              {
    //                float ftmp = 0.2;

    //                if ( chrlst[chra_ref].weight < 0 )
    //                {
    //                  ftmp = 0;
    //                }
    //                else if ( chrlst[chra_ref].weight != 0 )
    //                {
    //                  ftmp *= ( 1.0f + chrlst[chra_ref].bumpdampen ) * prtlst[prtb].weight / chrlst[chra_ref].weight;
    //                }

    //                chrlst[chra_ref].accum_vel.x += pvel.x * ftmp;
    //                chrlst[chra_ref].accum_vel.y += pvel.y * ftmp;
    //                chrlst[chra_ref].accum_vel.z += pvel.z * ftmp;

    //                prtlst[prtb].accum_vel.x += -chrlst[chra_ref].bumpdampen * pvel.x - prtlst[prtb].vel.x;
    //                prtlst[prtb].accum_vel.y += -chrlst[chra_ref].bumpdampen * pvel.y - prtlst[prtb].vel.y;
    //                prtlst[prtb].accum_vel.z += -chrlst[chra_ref].bumpdampen * pvel.z - prtlst[prtb].vel.z;
    //              }

    //              direction = RAD_TO_TURN( atan2( pvel.y, pvel.x ) );
    //              direction = 32768 + chrlst[chra_ref].turn_lr - direction;

    //              // Check all enchants to see if they are removed
    //              enchant = chrlst[chra_ref].firstenchant;
    //              while ( enchant != MAXENCHANT )
    //              {
    //                eveidremove = gs->EveList[enclst[enchant].eve].removedbyidsz;
    //                temp = enclst[enchant].nextenchant;
    //                if ( eveidremove != IDSZ_NONE && CAP_INHERIT_IDSZ( gs,  prtlst[prtb].model, eveidremove ) )
    //                {
    //                  remove_enchant( enchant );
    //                }
    //                enchant = temp;
    //              }

    //              //Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
    //              //+1 (256) bonus for every 4 points of intelligence and/or wisdom above 14. Below 14 gives -1 instead!
    //              //Enemy IDAM spells damage is reduced by 1% per defender's wisdom, opposite for WDAM spells
    //              if ( piplst[pip].intdamagebonus )
    //              {
    //                prtlst[prtb].damage.ibase += (( chrlst[prt_owner].intelligence_fp8 - 3584 ) * 0.25 );    //First increase damage by the attacker
    //                if(!chrlst[chra_ref].damagemodifier_fp8[prtlst[prtb].damagetype]&DAMAGE_INVERT || !chrlst[chra_ref].damagemodifier_fp8[prtlst[prtb].damagetype]&DAMAGE_CHARGE)
    //                prtlst[prtb].damage.ibase -= (prtlst[prtb].damage.ibase * ( chrlst[chra_ref].wisdom_fp8 > 8 ));    //Then reduce it by defender
    //              }
    //              if ( piplst[pip].wisdamagebonus )  //Same with divine spells
    //              {
    //                prtlst[prtb].damage.ibase += (( chrlst[prt_owner].wisdom_fp8 - 3584 ) * 0.25 );
    //                if(!chrlst[chra_ref].damagemodifier_fp8[prtlst[prtb].damagetype]&DAMAGE_INVERT || !chrlst[chra_ref].damagemodifier_fp8[prtlst[prtb].damagetype]&DAMAGE_CHARGE)
    //                prtlst[prtb].damage.ibase -= (prtlst[prtb].damage.ibase * ( chrlst[chra_ref].intelligence_fp8 > 8 ));
    //              }

    //              //Force Pancake animation?
    //              if ( piplst[pip].causepancake )
    //              {
    //                vect3 panc;
    //                Uint16 rotate_cos, rotate_sin;
    //                float cv, sv;

    //                // just a guess
    //                panc.x = 0.25 * ABS( pvel.x ) * 2.0f / ( float )( 1 + chrlst[chra_ref].bmpdata.cv.x_max - chrlst[chra_ref].bmpdata.cv.x_min  );
    //                panc.y = 0.25 * ABS( pvel.y ) * 2.0f / ( float )( 1 + chrlst[chra_ref].bmpdata.cv.y_max - chrlst[chra_ref].bmpdata.cv.y_min );
    //                panc.z = 0.25 * ABS( pvel.z ) * 2.0f / ( float )( 1 + chrlst[chra_ref].bmpdata.cv.z_max - chrlst[chra_ref].bmpdata.cv.z_min );

    //                rotate_sin = chrlst[chra_ref].turn_lr >> 2;

    //                cv = turntocos[rotate_sin];
    //                sv = turntosin[rotate_sin];

    //                chrlst[chra_ref].pancakevel.x = - ( panc.x * cv - panc.y * sv );
    //                chrlst[chra_ref].pancakevel.y = - ( panc.x * sv + panc.y * cv );
    //                chrlst[chra_ref].pancakevel.z = -panc.z;
    //              }

    //              // Damage the character
    //              if ( chr_is_vulnerable )
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = prtlst[prtb].damage.ibase * 2.0f * bumpstrength;
    //                ptemp.irand = prtlst[prtb].damage.irand * 2.0f * bumpstrength;
    //                damage_character( chra_ref, direction, &ptemp, prtlst[prtb].damagetype, prtlst[prtb].team, prt_owner, piplst[pip].damfx );
    //                chrlst[chra_ref].aistate.alert |= ALERT_HITVULNERABLE;
    //                cost_mana( gs, chra_ref, piplst[pip].manadrain*2, prt_owner );  //Do mana drain too
    //              }
    //              else
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = prtlst[prtb].damage.ibase * bumpstrength;
    //                ptemp.irand = prtlst[prtb].damage.irand * bumpstrength;

    //                damage_character( chra_ref, direction, &prtlst[prtb].damage, prtlst[prtb].damagetype, prtlst[prtb].team, prt_owner, piplst[pip].damfx );
    //                cost_mana( gs, chra_ref, piplst[pip].manadrain, prt_owner );  //Do mana drain too
    //              }

    //              // Do confuse effects
    //              if ( HAS_NO_BITS( madlst[chrlst[chra_ref].model].framefx[chrlst[chra_ref].anim.next], MADFX_INVICTUS ) || HAS_SOME_BITS( piplst[pip].damfx, DAMFX_BLOC ) )
    //              {

    //                if ( piplst[pip].grogtime != 0 && caplst[chrlst[chra_ref].model].canbegrogged )
    //                {
    //                  chrlst[chra_ref].grogtime += piplst[pip].grogtime * bumpstrength;
    //                  if ( chrlst[chra_ref].grogtime < 0 )
    //                  {
    //                    chrlst[chra_ref].grogtime = -1;
    //                    debug_message( 1, "placing infinite grog on %s (%s)", chrlst[chra_ref].name, caplst[chrlst[chra_ref].model].classname );
    //                  }
    //                  chrlst[chra_ref].aistate.alert |= ALERT_GROGGED;
    //                }

    //                if ( piplst[pip].dazetime != 0 && caplst[chrlst[chra_ref].model].canbedazed )
    //                {
    //                  chrlst[chra_ref].dazetime += piplst[pip].dazetime * bumpstrength;
    //                  if ( chrlst[chra_ref].dazetime < 0 )
    //                  {
    //                    chrlst[chra_ref].dazetime = -1;
    //                    debug_message( 1, "placing infinite daze on %s (%s)", chrlst[chra_ref].name, caplst[chrlst[chra_ref].model].classname );
    //                  };
    //                  chrlst[chra_ref].aistate.alert |= ALERT_DAZED;
    //                }
    //              }

    //              // Notify the attacker of a scored hit
    //              if ( VALID_CHR( chrlst, prt_owner ) )
    //              {
    //                chrlst[prt_owner].aistate.alert |= ALERT_SCOREDAHIT;
    //                chrlst[prt_owner].aistate.hitlast = chra_ref;
    //              }
    //            }

    //            if (( gs->wld_frame&31 ) == 0 && prt_attached == chra_ref )
    //            {
    //              // Attached prtb damage ( Burning )
    //              if ( piplst[pip].xyvel.ibase == 0 )
    //              {
    //                // Make character limp
    //                chrlst[chra_ref].vel.x = 0;
    //                chrlst[chra_ref].vel.y = 0;
    //              }
    //              damage_character( chra_ref, 32768, &prtlst[prtb].damage, prtlst[prtb].damagetype, prtlst[prtb].team, prt_owner, piplst[pip].damfx );
    //              cost_mana( gs, chra_ref, piplst[pip].manadrain, prt_owner );  //Do mana drain too

    //            }
    //          }

    //          if ( piplst[pip].endbump )
    //          {
    //            if ( piplst[pip].bumpmoney )
    //            {
    //              if ( chrlst[chra_ref].cangrabmoney && chrlst[chra_ref].alive && chrlst[chra_ref].damagetime == 0 && chrlst[chra_ref].money != MAXMONEY )
    //              {
    //                if ( chrlst[chra_ref].ismount )
    //                {
    //                  CHR_REF irider = chr_get_holdingwhich( chrlst, chrlst_size, chra_ref, SLOT_SADDLE );

    //                  // Let mounts collect money for their riders
    //                  if ( VALID_CHR( chrlst, irider ) )
    //                  {
    //                    chrlst[irider].money += piplst[pip].bumpmoney;
    //                    if ( chrlst[irider].money > MAXMONEY ) chrlst[irider].money = MAXMONEY;
    //                    if ( chrlst[irider].money <        0 ) chrlst[irider].money = 0;
    //                    prtlst[prtb].gopoof = btrue;
    //                  }
    //                }
    //                else
    //                {
    //                  // Normal money collection
    //                  chrlst[chra_ref].money += piplst[pip].bumpmoney;
    //                  if ( chrlst[chra_ref].money > MAXMONEY ) chrlst[chra_ref].money = MAXMONEY;
    //                  if ( chrlst[chra_ref].money < 0 ) chrlst[chra_ref].money = 0;
    //                  prtlst[prtb].gopoof = btrue;
    //                }
    //              }
    //            }
    //            else
    //            {
    //              // Only hit one character, not several
    //              prtlst[prtb].damage.ibase *= 1.0f - bumpstrength;
    //              prtlst[prtb].damage.irand *= 1.0f - bumpstrength;

    //              if ( prtlst[prtb].damage.ibase == 0 && prtlst[prtb].damage.irand <= 1 )
    //              {
    //                prtlst[prtb].gopoof = btrue;
    //              };
    //            }
    //          }
    //        }
    //      }
    //      else if ( prt_owner != chra_ref )
    //      {
    //        cost_mana( gs, chrlst[chra_ref].missilehandler, ( chrlst[chra_ref].missilecost << 4 ), prt_owner );

    //        // Treat the missile
    //        switch ( chrlst[chra_ref].missiletreatment )
    //        {
    //          case MIS_DEFLECT:
    //            {
    //              // Use old position to find normal
    //              acc.x = prtlst[prtb].pos.x - pvel.x * dUpdate;
    //              acc.y = prtlst[prtb].pos.y - pvel.y * dUpdate;
    //              acc.x = chrlst[chra_ref].pos.x - acc.x;
    //              acc.y = chrlst[chra_ref].pos.y - acc.y;
    //              // Find size of normal
    //              scale = acc.x * acc.x + acc.y * acc.y;
    //              if ( scale > 0 )
    //              {
    //                // Make the normal a unit normal
    //                scale = sqrt( scale );
    //                nrm.x = acc.x / scale;
    //                nrm.y = acc.y / scale;

    //                // Deflect the incoming ray off the normal
    //                scale = ( pvel.x * nrm.x + pvel.y * nrm.y ) * 2;
    //                acc.x = scale * nrm.x;
    //                acc.y = scale * nrm.y;
    //                prtlst[prtb].accum_vel.x += -acc.x;
    //                prtlst[prtb].accum_vel.y += -acc.y;
    //              }
    //            }
    //            break;

    //          case MIS_REFLECT:
    //            {
    //              // Reflect it back in the direction it came
    //              prtlst[prtb].accum_vel.x += -2.0f * prtlst[prtb].vel.x;
    //              prtlst[prtb].accum_vel.y += -2.0f * prtlst[prtb].vel.y;
    //            };
    //            break;
    //        };

    //        // Change the owner of the missile
    //        if ( !piplst[pip].homing )
    //        {
    //          prtlst[prtb].team = chrlst[chra_ref].team;
    //          prt_owner = chra_ref;
    //        }
    //      }
    //    }
    //  }
    //}

    // do platform physics
    //for ( cnt = 0, blnode_a = bumplist_get_chr_head(&bumplist, fanblock);
    //      cnt < chrinblock && INVALID_BUNPLIST_NODE != blnode_a;
    //      cnt++, blnode_a = bumplist_get_next_chr( blnode_a ) )
    //{
    //  // detach character from invalid platforms

    //  chra_ref = bumplist_get_ref( &bumplist, blnode_a );
    //  assert(VALID_CHR( chrlst, chra_ref ));
    //
    //  chrb_ref  = chr_get_onwhichplatform( chra_ref );
    //  if ( !VALID_CHR( chrlst, chrb_ref ) ) continue;

    //  if ( chrlst[chra_ref].pos.z < chrlst[chrb_ref].bmpdata.cv.z_max + RAISE )
    //  {
    //    chrlst[chra_ref].pos.z = chrlst[chrb_ref].bmpdata.cv.z_max + RAISE;
    //    if ( chrlst[chra_ref].vel.z < chrlst[chrb_ref].vel.z )
    //    {
    //      chrlst[chra_ref].vel.z = - ( chrlst[chra_ref].vel.z - chrlst[chrb_ref].vel.z ) * chrlst[chra_ref].bumpdampen * chrlst[chrb_ref].bumpdampen + chrlst[chrb_ref].vel.z;
    //    };
    //  }

    //  chrlst[chra_ref].vel.x = ( chrlst[chra_ref].vel.x - chrlst[chrb_ref].vel.x ) * ( 1.0 - loc_platstick ) + chrlst[chrb_ref].vel.x;
    //  chrlst[chra_ref].vel.y = ( chrlst[chra_ref].vel.y - chrlst[chrb_ref].vel.y ) * ( 1.0 - loc_platstick ) + chrlst[chrb_ref].vel.y;
    //  chrlst[chra_ref].turn_lr += ( chrlst[chrb_ref].turn_lr - chrlst[chrb_ref].turn_lr_old ) * ( 1.0 - loc_platstick );
    //}

  };
}

//--------------------------------------------------------------------------------------------
void stat_return( CGame * gs, float dUpdate )
{
  // ZZ> This function brings mana and life back

  CClient * cs = gs->cl;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Enc  * enclst      = gs->EncList;
  size_t enclst_size = MAXENCHANT;

  int cnt, owner, target, eve;
  static int stat_return_counter = 0;

  // Do reload time
  cnt = 0;
  while ( cnt < chrlst_size )
  {
    chrlst[cnt].reloadtime -= dUpdate;
    if ( chrlst[cnt].reloadtime < 0 ) chrlst[cnt].reloadtime = 0;
    cnt++;
  }

  // Do stats
  if ( cs->stat_clock == ONESECOND )
  {
    // Reset the clock
    cs->stat_clock = 0;
    stat_return_counter++;

    // Do all the characters
    for ( cnt = 0; cnt < chrlst_size; cnt++ )
    {
      if ( !VALID_CHR( chrlst, cnt ) ) continue;

      if ( chrlst[cnt].alive )
      {
        chrlst[cnt].mana_fp8 += chrlst[cnt].manareturn_fp8 >> MANARETURNSHIFT;
        if ( chrlst[cnt].mana_fp8 < 0 ) chrlst[cnt].mana_fp8 = 0;
        if ( chrlst[cnt].mana_fp8 > chrlst[cnt].manamax_fp8 ) chrlst[cnt].mana_fp8 = chrlst[cnt].manamax_fp8;

        chrlst[cnt].life_fp8 += chrlst[cnt].lifereturn;
        if ( chrlst[cnt].life_fp8 < 1 ) chrlst[cnt].life_fp8 = 1;
        if ( chrlst[cnt].life_fp8 > chrlst[cnt].lifemax_fp8 ) chrlst[cnt].life_fp8 = chrlst[cnt].lifemax_fp8;
      };

      if ( chrlst[cnt].grogtime > 0 )
      {
        chrlst[cnt].grogtime--;
        if ( chrlst[cnt].grogtime < 0 ) chrlst[cnt].grogtime = 0;

        if ( chrlst[cnt].grogtime == 0 )
        {
          debug_message( 1, "stat_return() - removing grog on %s (%s)", chrlst[cnt].name, caplst[chrlst[cnt].model].classname );
        };
      }

      if ( chrlst[cnt].dazetime > 0 )
      {
        chrlst[cnt].dazetime--;
        if ( chrlst[cnt].dazetime < 0 ) chrlst[cnt].dazetime = 0;
        if ( chrlst[cnt].grogtime == 0 )
        {
          debug_message( 1, "stat_return() - removing daze on %s (%s)", chrlst[cnt].name, caplst[chrlst[cnt].model].classname );
        };
      }

    }

    // Run through all the enchants as well
    for ( cnt = 0; cnt < enclst_size; cnt++ )
    {
      bool_t kill_enchant = bfalse;
      if ( !enclst[cnt].on ) continue;

      if ( enclst[cnt].time == 0 )
      {
        kill_enchant = btrue;
      };

      if ( enclst[cnt].time > 0 ) enclst[cnt].time--;

      owner = enclst[cnt].owner;
      target = enclst[cnt].target;
      eve = enclst[cnt].eve;

      // Do drains
      if ( !kill_enchant && chrlst[owner].alive )
      {
        // Change life
        chrlst[owner].life_fp8 += enclst[cnt].ownerlife_fp8;
        if ( chrlst[owner].life_fp8 < 1 )
        {
          chrlst[owner].life_fp8 = 1;
          kill_character( gs, owner, target );
        }
        if ( chrlst[owner].life_fp8 > chrlst[owner].lifemax_fp8 )
        {
          chrlst[owner].life_fp8 = chrlst[owner].lifemax_fp8;
        }
        // Change mana
        if ( !cost_mana( gs, owner, -enclst[cnt].ownermana_fp8, target ) && gs->EveList[eve].endifcantpay )
        {
          kill_enchant = btrue;
        }
      }
      else if ( !gs->EveList[eve].stayifnoowner )
      {
        kill_enchant = btrue;
      }

      if ( !kill_enchant && enclst[cnt].on )
      {
        if ( chrlst[target].alive )
        {
          // Change life
          chrlst[target].life_fp8 += enclst[cnt].targetlife_fp8;
          if ( chrlst[target].life_fp8 < 1 )
          {
            chrlst[target].life_fp8 = 1;
            kill_character( gs, target, owner );
          }
          if ( chrlst[target].life_fp8 > chrlst[target].lifemax_fp8 )
          {
            chrlst[target].life_fp8 = chrlst[target].lifemax_fp8;
          }

          // Change mana
          if ( !cost_mana( gs, target, -enclst[cnt].targetmana_fp8, owner ) && gs->EveList[eve].endifcantpay )
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
        remove_enchant( gs, cnt );
      };
    }
  }
}

//--------------------------------------------------------------------------------------------
void pit_kill( CGame * gs, float dUpdate )
{
  // ZZ> This function kills any character in a deep pit...

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;


  int cnt;

  if ( gs->pitskill )
  {
    if ( gs->pit_clock > 19 )
    {
      gs->pit_clock = 0;

      // Kill any particles that fell in a pit, if they die in water...

      for ( cnt = 0; cnt < prtlst_size; cnt++ )
      {
        if ( !VALID_PRT( prtlst, cnt ) ) continue;

        if ( prtlst[cnt].pos.z < PITDEPTH && piplst[prtlst[cnt].pip].endwater )
        {
          prtlst[cnt].gopoof = btrue;
        }
      }

      // Kill any characters that fell in a pit...
      cnt = 0;
      while ( cnt < chrlst_size )
      {
        if ( chrlst[cnt].on && chrlst[cnt].alive && !chr_in_pack( chrlst, chrlst_size, cnt ) )
        {
          if ( !chrlst[cnt].invictus && chrlst[cnt].pos.z < PITDEPTH && !chr_attached( chrlst, chrlst_size, cnt ) )
          {
            // Got one!
            kill_character( gs, cnt, MAXCHR );
            chrlst[cnt].vel.x = 0;
            chrlst[cnt].vel.y = 0;
          }
        }
        cnt++;
      }
    }
    else
    {
      gs->pit_clock += dUpdate;
    }
  }
}

//--------------------------------------------------------------------------------------------
void reset_players( CGame * gs )
{
  // ZZ> This function clears the player list data

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;



  // Reset the local data stuff
  gs->cl->seekurse    = bfalse;
  gs->cl->seeinvisible = bfalse;
  gs->somepladead  = bfalse;
  gs->allpladead   = bfalse;

  // Reset the initial player data and latches
  PlaList_renew( gs );

  CClient_reset_latches( gs->cl );
  CServer_reset_latches( gs->sv );
}

//--------------------------------------------------------------------------------------------
void resize_characters( CGame * gs, float dUpdate )
{
  // ZZ> This function makes the characters get bigger or smaller, depending
  //     on their sizegoto and sizegototime

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  CHR_REF chr_ref;

  bool_t willgetcaught;
  float newsize, fkeep;

  fkeep = pow( 0.9995, dUpdate );

  for ( chr_ref = 0; chr_ref < chrlst_size; chr_ref++ )
  {
    if ( !VALID_CHR( chrlst, chr_ref ) || chrlst[chr_ref].sizegototime <= 0 ) continue;

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
    chrlst[chr_ref].weight          = caplst[chrlst[chr_ref].model].weight * newsize * newsize * newsize;  // preserve density

    // Now come up with the magic number
    chrlst[chr_ref].scale = newsize;

    // calculate the bumpers
    make_one_character_matrix( chrlst, chrlst_size, chrlst + chr_ref );
  }
}

//--------------------------------------------------------------------------------------------
void export_one_character_name( CGame * gs, char *szSaveName, CHR_REF chr_ref )
{
  // ZZ> This function makes the "NAMING.TXT" file for the character

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  FILE* filewrite;
  int profile;

  // Can it export?
  profile = chrlst[chr_ref].model;
  filewrite = fs_fileOpen( PRI_FAIL, "export_one_character_name()", szSaveName, "w" );
  if ( NULL== filewrite )
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
void export_one_character_profile( CGame * gs, char *szSaveName, CHR_REF chr_ref )
{
  // ZZ> This function creates a "DATA.TXT" file for the given character.
  //     it is assumed that all enchantments have been done away with

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  FILE* filewrite;
  int profile;
  int damagetype, iskin;
  char types[10] = "SCPHEFIZ";
  char codes[4];
  Chr * pchr;
  Cap * pcap;

  if( !VALID_CHR(chrlst, chr_ref) ) return;
  pchr = chrlst + chr_ref;

  disenchant_character( gs, chr_ref );

  // General stuff
  profile = pchr->model;
  if( !VALID_MDL(profile) ) return;
  pcap = caplst + profile;

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
  fprintf( filewrite, "Life           : %4.2f\n", FP8_TO_FLOAT( pchr->lifemax_fp8 ) );
  fpairof( filewrite, "Life up        : ", &pcap->lifeperlevel_fp8 );
  fprintf( filewrite, "Mana           : %4.2f\n", FP8_TO_FLOAT( pchr->manamax_fp8 ) );
  fpairof( filewrite, "Mana up        : ", &pcap->manaperlevel_fp8 );
  fprintf( filewrite, "Mana return    : %4.2f\n", FP8_TO_FLOAT( pchr->manareturn_fp8 ) );
  fpairof( filewrite, "Mana return up : ", &pcap->manareturnperlevel_fp8 );
  fprintf( filewrite, "Mana flow      : %4.2f\n", FP8_TO_FLOAT( pchr->manaflow_fp8 ) );
  fpairof( filewrite, "Mana flow up   : ", &pcap->manaflowperlevel_fp8 );
  fprintf( filewrite, "STR            : %4.2f\n", FP8_TO_FLOAT( pchr->strength_fp8 ) );
  fpairof( filewrite, "STR up         : ", &pcap->strengthperlevel_fp8 );
  fprintf( filewrite, "WIS            : %4.2f\n", FP8_TO_FLOAT( pchr->wisdom_fp8 ) );
  fpairof( filewrite, "WIS up         : ", &pcap->wisdomperlevel_fp8 );
  fprintf( filewrite, "INT            : %4.2f\n", FP8_TO_FLOAT( pchr->intelligence_fp8 ) );
  fpairof( filewrite, "INT up         : ", &pcap->intelligenceperlevel_fp8 );
  fprintf( filewrite, "DEX            : %4.2f\n", FP8_TO_FLOAT( pchr->dexterity_fp8 ) );
  fpairof( filewrite, "DEX up         : ", &pcap->dexterityperlevel_fp8 );
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
  ftruthf( filewrite, "Sticky butt    : ", pcap->stickybutt );
  fprintf( filewrite, "\n" );

  // Invulnerability data
  ftruthf( filewrite, "Invictus       : ", pcap->invictus );
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
  ftruthf( filewrite, "Is an item     : ", pcap->isitem );
  ftruthf( filewrite, "Is a mount     : ", pcap->ismount );
  ftruthf( filewrite, "Is stackable   : ", pcap->isstackable );
  ftruthf( filewrite, "Name known     : ", pcap->nameknown );
  ftruthf( filewrite, "Usage known    : ", pcap->usageknown );
  ftruthf( filewrite, "Is exportable  : ", pcap->cancarrytonextmodule );
  ftruthf( filewrite, "Requires skill : ", pcap->needskillidtouse );
  ftruthf( filewrite, "Is platform    : ", pcap->isplatform );
  ftruthf( filewrite, "Collects money : ", pcap->cangrabmoney );
  ftruthf( filewrite, "Can open stuff : ", pcap->canopenstuff );
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
  ftruthf( filewrite, "Waterwalking   : ", pcap->waterwalk );
  fprintf( filewrite, "Bounce dampen  : %5.3f\n", pcap->dampen );
  fprintf( filewrite, "\n" );

  // More stuff
  fprintf( filewrite, "Life healing   : %5.3f\n", FP8_TO_FLOAT( pcap->lifeheal_fp8 ) );
  fprintf( filewrite, "Mana cost      : %5.3f\n", FP8_TO_FLOAT( pcap->manacost_fp8 ) );
  fprintf( filewrite, "Life return    : %d\n", pcap->lifereturn_fp8 );
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
  ftruthf( filewrite, "No rider attak : ", !pcap->ridercanattack );
  ftruthf( filewrite, "Can be dazed   : ", pcap->canbedazed );
  ftruthf( filewrite, "Can be grogged : ", pcap->canbegrogged );
  fprintf( filewrite, "NOT USED       : 0\n" );
  fprintf( filewrite, "NOT USED       : 0\n" );
  ftruthf( filewrite, "Can see invisi : ", pcap->canseeinvisible );
  fprintf( filewrite, "Kursed chance  : %d\n", pchr->iskursed*100 );
  fprintf( filewrite, "Footfall sound : %d\n", pcap->footfallsound );
  fprintf( filewrite, "Jump sound     : %d\n", pcap->jumpsound );
  fprintf( filewrite, "\n" );

  // Expansions
  fprintf( filewrite, ":[GOLD] %d\n", pcap->money );

  if ( pcap->skindressy&1 ) fprintf( filewrite, ":[DRES] 0\n" );
  if ( pcap->skindressy&2 ) fprintf( filewrite, ":[DRES] 1\n" );
  if ( pcap->skindressy&4 ) fprintf( filewrite, ":[DRES] 2\n" );
  if ( pcap->skindressy&8 ) fprintf( filewrite, ":[DRES] 3\n" );
  if ( pcap->resistbumpspawn ) fprintf( filewrite, ":[STUK] 0\n" );
  if ( pcap->istoobig ) fprintf( filewrite, ":[PACK] 0\n" );
  if ( !pcap->reflect ) fprintf( filewrite, ":[VAMP] 1\n" );
  if ( pcap->alwaysdraw ) fprintf( filewrite, ":[DRAW] 1\n" );
  if ( pcap->isranged ) fprintf( filewrite, ":[RANG] 1\n" );
  if ( pcap->hidestate != NOHIDE ) fprintf( filewrite, ":[HIDE] %d\n", pcap->hidestate );
  if ( pcap->isequipment ) fprintf( filewrite, ":[EQUI] 1\n" );
  if ( pcap->bumpsizebig >= pcap->bumpsize ) fprintf( filewrite, ":[SQUA] 1\n" );
  if ( pcap->icon != pcap->usageknown ) fprintf( filewrite, ":[ICON] %d\n", pcap->icon );
  if ( pcap->forceshadow ) fprintf( filewrite, ":[SHAD] 1\n" );

  //Skill expansions
  if ( pcap->canseekurse )  fprintf( filewrite, ":[CKUR] 1\n" );
  if ( pcap->canusearcane ) fprintf( filewrite, ":[WMAG] 1\n" );
  if ( pcap->canjoust )     fprintf( filewrite, ":[JOUS] 1\n" );
  if ( pcap->canusedivine ) fprintf( filewrite, ":[HMAG] 1\n" );
  if ( pcap->candisarm )    fprintf( filewrite, ":[DISA] 1\n" );
  if ( pcap->canusetech )   fprintf( filewrite, ":[TECH] 1\n" );
  if ( pcap->canbackstab )  fprintf( filewrite, ":[STAB] 1\n" );
  if ( pcap->canuseadvancedweapons ) fprintf( filewrite, ":[AWEP] 1\n" );
  if ( pcap->canusepoison ) fprintf( filewrite, ":[POIS] 1\n" );
  if ( pcap->canread )  fprintf( filewrite, ":[READ] 1\n" );

  //General exported chr_ref information
  fprintf( filewrite, ":[PLAT] %d\n", pcap->canuseplatforms );
  fprintf( filewrite, ":[SKIN] %d\n", pchr->skin_ref % MAXSKIN );
  fprintf( filewrite, ":[CONT] %d\n", pchr->aistate.content );
  fprintf( filewrite, ":[STAT] %d\n", pchr->aistate.state );
  fprintf( filewrite, ":[LEVL] %d\n", pchr->experiencelevel );
  fs_fileClose( filewrite );

}

//--------------------------------------------------------------------------------------------
void export_one_character_skin( CGame * gs, char *szSaveName, CHR_REF chr_ref )
{
  // ZZ> This function creates a "SKIN.TXT" file for the given character.

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  FILE* filewrite;
  int profile;

  // General stuff
  profile = chrlst[chr_ref].model;

  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( NULL != filewrite )
  {
    fprintf( filewrite, "This file is used only by the import menu\n" );
    fprintf( filewrite, ": %d\n", chrlst[chr_ref].skin_ref % MAXSKIN );
    fs_fileClose( filewrite );
  }
}

//--------------------------------------------------------------------------------------------
void calc_cap_experience( CGame * gs, Uint16 profile )
{
  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  float statdebt, statperlevel;

  statdebt  = caplst[profile].life_fp8.ibase + caplst[profile].mana_fp8.ibase + caplst[profile].manareturn_fp8.ibase + caplst[profile].manaflow_fp8.ibase;
  statdebt += caplst[profile].strength_fp8.ibase + caplst[profile].wisdom_fp8.ibase + caplst[profile].intelligence_fp8.ibase + caplst[profile].dexterity_fp8.ibase;
  statdebt += ( caplst[profile].life_fp8.irand + caplst[profile].mana_fp8.irand + caplst[profile].manareturn_fp8.irand + caplst[profile].manaflow_fp8.irand ) * 0.5f;
  statdebt += ( caplst[profile].strength_fp8.irand + caplst[profile].wisdom_fp8.irand + caplst[profile].intelligence_fp8.irand + caplst[profile].dexterity_fp8.irand ) * 0.5f;

  statperlevel  = caplst[profile].lifeperlevel_fp8.ibase + caplst[profile].manaperlevel_fp8.ibase + caplst[profile].manareturnperlevel_fp8.ibase + caplst[profile].manaflowperlevel_fp8.ibase;
  statperlevel += caplst[profile].strengthperlevel_fp8.ibase + caplst[profile].wisdomperlevel_fp8.ibase + caplst[profile].intelligenceperlevel_fp8.ibase + caplst[profile].dexterityperlevel_fp8.ibase;
  statperlevel += ( caplst[profile].lifeperlevel_fp8.irand + caplst[profile].manaperlevel_fp8.irand + caplst[profile].manareturnperlevel_fp8.irand + caplst[profile].manaflowperlevel_fp8.irand ) * 0.5f;
  statperlevel += ( caplst[profile].strengthperlevel_fp8.irand + caplst[profile].wisdomperlevel_fp8.irand + caplst[profile].intelligenceperlevel_fp8.irand + caplst[profile].dexterityperlevel_fp8.irand ) * 0.5f;

  caplst[profile].experienceconst = 50.6f * ( FP8_TO_FLOAT( statdebt ) - 51.5 );
  caplst[profile].experiencecoeff = 26.3f * MAX( 1, FP8_TO_FLOAT( statperlevel ) );
};

//--------------------------------------------------------------------------------------------
int calc_chr_experience( CGame * gs, Uint16 object, float level )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Uint16 profile;

  if ( !VALID_CHR( chrlst, object ) ) return 0;

  profile = chrlst[object].model;

  return level*level*caplst[profile].experiencecoeff + caplst[profile].experienceconst + 1;
};

//--------------------------------------------------------------------------------------------
float calc_chr_level( CGame * gs, Uint16 object )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Uint16 profile;
  float  level;

  if ( !VALID_CHR( chrlst, object ) ) return 0.0f;

  profile = chrlst[object].model;

  level = ( chrlst[object].experience - caplst[profile].experienceconst ) / caplst[profile].experiencecoeff;
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
Uint16 object_generate_index( CGame * gs, char *szLoadName )
{
  // ZZ > This reads the object slot in "DATA.TXT" that the profile
  //      is assigned to.  Errors in this number may cause the program to abort

  FILE* fileread;
  int iobj = MAXMODEL;

  // Open the file
  fileread = fs_fileOpen( PRI_NONE, "object_generate_index()", szLoadName, "r" );
  if ( fileread == NULL )
  {
    return iobj;
  }

  globalname = szLoadName;

  // Read in the iobj slot
  iobj = fget_next_int( fileread );
  if ( iobj < 0 )
  {
    if ( gs->modstate.import.object < 0 )
    {
      log_error( "Object slot number %i is invalid. (%s) \n", iobj, szLoadName );
    }
    else
    {
      iobj = gs->modstate.import.object;
    }
  }

  fs_fileClose(fileread);

  return iobj;
}

//--------------------------------------------------------------------------------------------
Uint16 CapList_load_one( CGame * gs, char * szObjectpath, char *szObjectname, Uint16 icap )
{
  // ZZ> This function fills a character profile with data from "DATA.TXT"

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  FILE* fileread;
  int iskin, cnt;
  int iTmp;
  char cTmp;
  int damagetype, level, xptype;
  IDSZ idsz;
  Cap * pcap;
  STRING szLoadname;

  // Open the file
  fileread = fs_fileOpen( PRI_NONE, "CapList_load_one()", inherit_fname(szObjectpath, szObjectname, CData.data_file), "r");
  if ( fileread == NULL )
  {
    // The data file wasn't found
    log_error( "Data.txt could not be correctly read! (%s) \n", szLoadname );
    return icap;
  }

  // make the notation "easier"
  pcap = caplst + icap;

  // skip over the slot information
  fget_next_int( fileread );

  // Read in the real general data
  fget_next_name( fileread, pcap->classname, sizeof( pcap->classname ) );

  // Make sure we don't load over an existing model
  if ( pcap->used )
  {
    log_error( "Character profile slot %i is already used. (%s)\n", icap, szLoadname );
  }

  // Light cheat
  pcap->uniformlit = fget_next_bool( fileread );

  // Ammo
  pcap->ammomax = fget_next_int( fileread );
  pcap->ammo = fget_next_int( fileread );

  // Gender
  pcap->gender = fget_next_gender( fileread );

  // Read in the icap stats
  pcap->lifecolor = fget_next_int( fileread );
  pcap->manacolor = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &pcap->life_fp8 );
  fget_next_pair_fp8( fileread, &pcap->lifeperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->mana_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manaperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manareturn_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manareturnperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manaflow_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manaflowperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->strength_fp8 );
  fget_next_pair_fp8( fileread, &pcap->strengthperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->wisdom_fp8 );
  fget_next_pair_fp8( fileread, &pcap->wisdomperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->intelligence_fp8 );
  fget_next_pair_fp8( fileread, &pcap->intelligenceperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->dexterity_fp8 );
  fget_next_pair_fp8( fileread, &pcap->dexterityperlevel_fp8 );

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
  pcap->stickybutt = fget_next_bool( fileread );

  // Invulnerability data
  pcap->invictus = fget_next_bool( fileread );
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
  pcap->isitem = fget_next_bool( fileread );
  pcap->ismount = fget_next_bool( fileread );
  pcap->isstackable = fget_next_bool( fileread );
  pcap->nameknown = fget_next_bool( fileread );
  pcap->usageknown = fget_next_bool( fileread );
  pcap->cancarrytonextmodule = fget_next_bool( fileread );
  pcap->needskillidtouse = fget_next_bool( fileread );
  pcap->isplatform = fget_next_bool( fileread );
  pcap->cangrabmoney = fget_next_bool( fileread );
  pcap->canopenstuff = fget_next_bool( fileread );

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
  if ( pcap->ismount )
  {
    pcap->slotvalid[SLOT_SADDLE] = pcap->slotvalid[SLOT_LEFT];
    pcap->slotvalid[SLOT_LEFT]   = bfalse;
    //pcap->slotvalid[SLOT_RIGHT]  = bfalse;
  };

  // Attack order ( weapon )
  pcap->attackattached = fget_next_bool( fileread );
  pcap->attackprttype = fget_next_int( fileread );

  // GoPoof
  pcap->gopoofprtamount = fget_next_int( fileread );
  pcap->gopoofprtfacingadd = fget_next_int( fileread );
  pcap->gopoofprttype = fget_next_int( fileread );

  // Blud
  pcap->bludlevel = fget_next_blud( fileread );
  pcap->bludprttype = fget_next_int( fileread );

  // Stuff I forgot
  pcap->waterwalk = fget_next_bool( fileread );
  pcap->dampen = fget_next_float( fileread );

  // More stuff I forgot
  pcap->lifeheal_fp8 = fget_next_fixed( fileread );
  pcap->manacost_fp8 = fget_next_fixed( fileread );
  pcap->lifereturn_fp8 = fget_next_int( fileread );
  pcap->stoppedby = fget_next_int( fileread ) | MPDFX_IMPASS;

  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    { fget_next_name( fileread, pcap->skin[iskin].name, sizeof( pcap->skin[iskin].name ) ); };

  for ( iskin = 0; iskin < MAXSKIN; iskin++ )
    { pcap->skin[iskin].cost = fget_next_int( fileread ); };

  pcap->strengthdampen = fget_next_float( fileread );

  // Another memory lapse
  pcap->ridercanattack = !fget_next_bool( fileread );
  pcap->canbedazed = fget_next_bool( fileread );
  pcap->canbegrogged = fget_next_bool( fileread );
  fget_next_int( fileread );   // !!!BAD!!! Life add
  fget_next_int( fileread );   // !!!BAD!!! Mana add
  pcap->canseeinvisible = fget_next_bool( fileread );
  pcap->kursechance = fget_next_int( fileread );

  iTmp = fget_next_int( fileread ); pcap->footfallsound = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); pcap->jumpsound     = FIX_SOUND( iTmp );

  // Clear expansions...
  pcap->skindressy = bfalse;
  pcap->resistbumpspawn = bfalse;
  pcap->istoobig = bfalse;
  pcap->reflect = btrue;
  pcap->alwaysdraw = bfalse;
  pcap->isranged = bfalse;
  pcap->hidestate = NOHIDE;
  pcap->isequipment = bfalse;
  pcap->bumpsizebig = pcap->bumpsize * SQRT_TWO;
  pcap->money = 0;
  pcap->icon = pcap->usageknown;
  pcap->forceshadow = bfalse;
  pcap->skinoverride = NOSKINOVERRIDE;
  pcap->contentoverride = 0;
  pcap->stateoverride = 0;
  pcap->leveloverride = 0;
  pcap->canuseplatforms = !pcap->isplatform;

  //Reset Skill Expansions
  pcap->canseekurse = bfalse;
  pcap->canusearcane = bfalse;
  pcap->canjoust = bfalse;
  pcap->canusedivine = bfalse;
  pcap->candisarm = bfalse;
  pcap->canusetech = bfalse;
  pcap->canbackstab = bfalse;
  pcap->canusepoison = bfalse;
  pcap->canuseadvancedweapons = bfalse;

  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );
    if ( MAKE_IDSZ( "GOLD" ) == idsz )  pcap->money = iTmp;
    else if ( MAKE_IDSZ( "STUK" ) == idsz )  pcap->resistbumpspawn = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "PACK" ) == idsz )  pcap->istoobig = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "VAMP" ) == idsz )  pcap->reflect = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DRAW" ) == idsz )  pcap->alwaysdraw = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RANG" ) == idsz )  pcap->isranged = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "HIDE" ) == idsz )  pcap->hidestate = iTmp;
    else if ( MAKE_IDSZ( "EQUI" ) == idsz )  pcap->isequipment = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SQUA" ) == idsz )  pcap->bumpsizebig = pcap->bumpsize * 2.0f;
    else if ( MAKE_IDSZ( "ICON" ) == idsz )  pcap->icon = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SHAD" ) == idsz )  pcap->forceshadow = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SKIN" ) == idsz )  pcap->skinoverride = iTmp % MAXSKIN;
    else if ( MAKE_IDSZ( "CONT" ) == idsz )  pcap->contentoverride = iTmp;
    else if ( MAKE_IDSZ( "STAT" ) == idsz )  pcap->stateoverride = iTmp;
    else if ( MAKE_IDSZ( "LEVL" ) == idsz )  pcap->leveloverride = iTmp;
    else if ( MAKE_IDSZ( "PLAT" ) == idsz )  pcap->canuseplatforms = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RIPP" ) == idsz )  pcap->ripple = INT_TO_BOOL( iTmp );

    //Skill Expansions
    // [CKUR] Can it see kurses?
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  pcap->canseekurse  = INT_TO_BOOL( iTmp );
    // [WMAG] Can the character use arcane spellbooks?
    else if ( MAKE_IDSZ( "WMAG" ) == idsz )  pcap->canusearcane = INT_TO_BOOL( iTmp );
    // [JOUS] Can the character joust with a lance?
    else if ( MAKE_IDSZ( "JOUS" ) == idsz )  pcap->canjoust     = INT_TO_BOOL( iTmp );
    // [HMAG] Can the character use divine spells?
    else if ( MAKE_IDSZ( "HMAG" ) == idsz )  pcap->canusedivine = INT_TO_BOOL( iTmp );
    // [TECH] Able to use items technological items?
    else if ( MAKE_IDSZ( "TECH" ) == idsz )  pcap->canusetech   = INT_TO_BOOL( iTmp );
    // [DISA] Find and disarm traps?
    else if ( MAKE_IDSZ( "DISA" ) == idsz )  pcap->candisarm    = INT_TO_BOOL( iTmp );
    // [STAB] Backstab and murder?
    else if ( idsz == MAKE_IDSZ( "STAB" ) )  pcap->canbackstab  = INT_TO_BOOL( iTmp );
    // [AWEP] Profiency with advanced weapons?
    else if ( idsz == MAKE_IDSZ( "AWEP" ) )  pcap->canuseadvancedweapons = INT_TO_BOOL( iTmp );
    // [POIS] Use poison without err?
    else if ( idsz == MAKE_IDSZ( "POIS" ) )  pcap->canusepoison = INT_TO_BOOL( iTmp );
  }

  fs_fileClose( fileread );

  calc_cap_experience( gs, icap );

  // tell everyone that we loaded correctly
  pcap->used = btrue;

  return icap;
}

//--------------------------------------------------------------------------------------------
int fget_skin( char * szObjectpath, char * szObjectname )
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
void check_player_import(CGame * gs)
{
  // ZZ> This function figures out which players may be imported, and loads basic
  //     data for each

  STRING searchname, filename, filepath;
  int skin;
  bool_t keeplooking;
  const char *foundfile;
  FS_FIND_INFO fs_finfo;

  // Set up...
  loadplayer_count = 0;

  // Search for all objects
  fs_find_info_new( &fs_finfo );
  snprintf( searchname, sizeof( searchname ), "%s" SLASH_STRING "*.obj", CData.players_dir );
  foundfile = fs_findFirstFile( &fs_finfo, CData.players_dir, NULL, "*.obj" );
  keeplooking = 1;
  if ( foundfile != NULL )
  {
    snprintf( filepath, sizeof( filename ), "%s" SLASH_STRING "%s" SLASH_STRING, CData.players_dir, foundfile );

    while (loadplayer_count < MAXLOADPLAYER )
    {
      prime_names( gs );

      strncpy( loadplayer[loadplayer_count].dir, foundfile, sizeof( loadplayer[loadplayer_count].dir ) );

      skin = fget_skin( filename, NULL );

      MadList_load_one( gs, filepath, NULL, loadplayer_count );

      snprintf( filename, sizeof( filename ), "icon%d.bmp", skin );
      load_one_icon( filepath, NULL, filename);

      read_naming( gs, filename, NULL, 0);
      naming_names( gs, 0 );
      strncpy( loadplayer[loadplayer_count].name, namingnames, sizeof( loadplayer[loadplayer_count].name ) );

      loadplayer_count++;

      foundfile = fs_findNextFile(&fs_finfo);
      if (NULL == foundfile) break;
    }
  }
  fs_findClose(&fs_finfo);
}

//--------------------------------------------------------------------------------------------
bool_t check_skills( CGame * gs, int who, Uint32 whichskill )
{
  // ZF> This checks if the specified character has the required skill. Returns btrue if true
  // and bfalse if not. Also checks Skill expansions.

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  bool_t result = bfalse;

  // First check the character Skill ID matches
  // Then check for expansion skills too.
  if ( caplst[chrlst[who].model].idsz[IDSZ_SKILL] == whichskill ) result = btrue;
  else if ( MAKE_IDSZ( "CKUR" ) == whichskill ) result = chrlst[who].canseekurse;
  else if ( MAKE_IDSZ( "WMAG" ) == whichskill ) result = chrlst[who].canusearcane;
  else if ( MAKE_IDSZ( "JOUS" ) == whichskill ) result = chrlst[who].canjoust;
  else if ( MAKE_IDSZ( "HMAG" ) == whichskill ) result = chrlst[who].canusedivine;
  else if ( MAKE_IDSZ( "DISA" ) == whichskill ) result = chrlst[who].candisarm;
  else if ( MAKE_IDSZ( "TECH" ) == whichskill ) result = chrlst[who].canusetech;
  else if ( MAKE_IDSZ( "AWEP" ) == whichskill ) result = chrlst[who].canuseadvancedweapons;
  else if ( MAKE_IDSZ( "STAB" ) == whichskill ) result = chrlst[who].canbackstab;
  else if ( MAKE_IDSZ( "POIS" ) == whichskill ) result = chrlst[who].canusepoison;
  else if ( MAKE_IDSZ( "READ" ) == whichskill ) result = chrlst[who].canread;

  return result;
}

//--------------------------------------------------------------------------------------------
int check_player_quest( char *whichplayer, IDSZ idsz )
{
  // ZF> This function checks if the specified player has the IDSZ in his or her quest.txt
  // and returns the quest level of that specific quest (Or -2 if it is not found, -1 if it is finished)

  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz = bfalse;
  Sint8 result = -2;
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
  if (check_player_quest(whichplayer, idsz) >= -1) return bfalse;

  // Try to open the file in read and append mode
  snprintf( newloadname, sizeof( newloadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.players_dir, whichplayer, CData.quest_file );
  filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "a" );
  if ( NULL == filewrite ) return bfalse;

  fprintf( filewrite, "\n:[%4s]: 0", undo_idsz( idsz ) );
  fs_fileClose( filewrite );

  return btrue;
}

//--------------------------------------------------------------------------------------------
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
  //     It returns -2 if failed and if the adjustment is 0, the quest is marked as beaten...

  FILE *filewrite, *fileread;
  STRING newloadname, copybuffer;
  bool_t foundidsz = bfalse;
  IDSZ newidsz;
  Sint8 NewQuestLevel = -2, QuestLevel;

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

      if ( QuestLevel == -1 )					//Quest is already finished, do not modify
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

bool_t chr_cvolume_reinit(Chr * pchr, CVolume * pcv)
{
  if(NULL == pchr || !pchr->on || NULL == pcv) return bfalse;

  pcv->x_min = pchr->pos.x - pchr->bmpdata.size * pchr->scale * pchr->pancakepos.x;
  pcv->y_min = pchr->pos.y - pchr->bmpdata.size * pchr->scale * pchr->pancakepos.y;
  pcv->z_min = pchr->pos.z;

  pcv->x_max = pchr->pos.x + pchr->bmpdata.size * pchr->scale * pchr->pancakepos.x;
  pcv->y_max = pchr->pos.y + pchr->bmpdata.size * pchr->scale * pchr->pancakepos.y;
  pcv->z_max = pchr->pos.z + pchr->bmpdata.height * pchr->scale * pchr->pancakepos.z;

  pcv->xy_min = -(pchr->pos.x + pchr->pos.y) - pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pcv->xy_max = -(pchr->pos.x + pchr->pos.y) + pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;

  pcv->yx_min = -(-pchr->pos.x + pchr->pos.y) - pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pcv->yx_max = -(-pchr->pos.x + pchr->pos.y) + pchr->bmpdata.sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;

  pcv->level = -1;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bdata_reinit(Chr * pchr, BData * pbd)
{
  if(NULL == pchr || !pchr->on || NULL == pbd) return bfalse;

  pbd->calc_is_platform   = pchr->isplatform;
  pbd->calc_is_mount      = pchr->ismount;

  pbd->mids_lo = pbd->mids_hi = pchr->pos;
  pbd->mids_hi.z += pbd->height * 0.5f;

  pbd->calc_size     = pbd->size    * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pbd->calc_size_big = pbd->sizebig * pchr->scale * (pchr->pancakepos.x + pchr->pancakepos.y) * 0.5f;
  pbd->calc_height   = pbd->height  * pchr->scale *  pchr->pancakepos.z;

  chr_cvolume_reinit(pchr, &pbd->cv);

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers_0( Chr * pchr )
{
  int i;
  bool_t rv = bfalse;

  if( NULL==pchr || !pchr->on ) return bfalse;

  // remove any level 3 info
  if(NULL != pchr->bmpdata.cv_tree)
  {
    for(i=0; i<8; i++)
    {
      (*pchr->bmpdata.cv_tree)[i].level = -1;
    }
  }

  rv = chr_bdata_reinit( pchr, &(pchr->bmpdata) );

  pchr->bmpdata.cv.level = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//bool_t chr_calculate_bumpers_1(CGame * gs, CHR_REF chr_ref)
//{
//  BData * bd;
//  Uint16 imdl;
//  MD2_Model * pmdl;
//  const MD2_Frame * fl, * fc;
//  float lerp;
//  vect3 xdir, ydir, zdir;
//  vect3 points[8], bbmax, bbmin;
//  int cnt;
//
//  CVolume cv;
//
//  if( !VALID_CHR(chrlst, chr_ref) ) return bfalse;
//  bd = &(chrlst[chr_ref].bmpdata);
//
//  imdl = chrlst[chr_ref].model;
//  if(!VALID_MDL(imdl) || !chrlst[chr_ref].matrixvalid )
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
//  if(NULL==fl && NULL==fc)
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
//  if(NULL==fl || lerp >= 1.0f)
//  {
//    bbmin.x = MIN(fc->bbmin[0], fc->bbmax[0]);
//    bbmin.y = MIN(fc->bbmin[1], fc->bbmax[1]);
//    bbmin.z = MIN(fc->bbmin[2], fc->bbmax[2]);
//
//    bbmax.x = MAX(fc->bbmin[0], fc->bbmax[0]);
//    bbmax.y = MAX(fc->bbmin[1], fc->bbmax[1]);
//    bbmax.z = MAX(fc->bbmin[2], fc->bbmax[2]);
//  }
//  else if(NULL==fc || lerp <= 0.0f)
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
//  bd->cv.x_min  = cv.x_min  + chrlst[chr_ref].pos.x;
//  bd->cv.y_min  = cv.y_min  + chrlst[chr_ref].pos.y;
//  bd->cv.z_min  = cv.z_min  + chrlst[chr_ref].pos.z;
//  bd->cv.xy_min = cv.xy_min + ( chrlst[chr_ref].pos.x + chrlst[chr_ref].pos.y);
//  bd->cv.yx_min = cv.yx_min + (-chrlst[chr_ref].pos.x + chrlst[chr_ref].pos.y);
//
//
//  bd->cv.x_max  = cv.x_max  + chrlst[chr_ref].pos.x;
//  bd->cv.y_max  = cv.y_max  + chrlst[chr_ref].pos.y;
//  bd->cv.z_max  = cv.z_max  + chrlst[chr_ref].pos.z;
//  bd->cv.xy_max = cv.xy_max + ( chrlst[chr_ref].pos.x + chrlst[chr_ref].pos.y);
//  bd->cv.yx_max = cv.yx_max + (-chrlst[chr_ref].pos.x + chrlst[chr_ref].pos.y);
//
//  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + chrlst[chr_ref].pos.x;
//  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + chrlst[chr_ref].pos.y;
//  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + chrlst[chr_ref].pos.z;
//
//  bd->mids_lo   = bd->mids_hi;
//  bd->mids_lo.z = cv.z_min  + chrlst[chr_ref].pos.z;
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
bool_t chr_calculate_bumpers_1( Mad madlst[], size_t madlst_size, Chr * pchr )
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  const MD2_Frame * fl, * fc;
  float lerp;
  vect3 xdir, ydir, zdir;
  vect3 points[8], bbmax, bbmin;
  int cnt;

  CVolume cv;

  if(  NULL==pchr || !pchr->on ) return bfalse;
  bd = &(pchr->bmpdata);

  imdl = pchr->model;
  if(!VALID_MDL(imdl) || !pchr->matrixvalid )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  };

  xdir.x = (pchr->matrix).CNV(0,0);
  xdir.y = (pchr->matrix).CNV(0,1);
  xdir.z = (pchr->matrix).CNV(0,2);

  ydir.x = (pchr->matrix).CNV(1,0);
  ydir.y = (pchr->matrix).CNV(1,1);
  ydir.z = (pchr->matrix).CNV(1,2);

  zdir.x = (pchr->matrix).CNV(2,0);
  zdir.y = (pchr->matrix).CNV(2,1);
  zdir.z = (pchr->matrix).CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = madlst[imdl].md2_ptr;
  if(NULL == pmdl)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  fl = md2_get_Frame(pmdl, pchr->anim.last);
  fc = md2_get_Frame(pmdl, pchr->anim.next );
  lerp = pchr->anim.flip;

  if(NULL==fl && NULL==fc)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  };

  if(NULL==fl || lerp >= 1.0f)
  {
    bbmin.x = fc->bbmin[0];
    bbmin.y = fc->bbmin[1];
    bbmin.z = fc->bbmin[2];

    bbmax.x = fc->bbmax[0];
    bbmax.y = fc->bbmax[1];
    bbmax.z = fc->bbmax[2];
  }
  else if(NULL==fc || lerp <= 0.0f)
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

  cv.x_min  = cv.x_max  = points[0].x*xdir.x + points[0].y*ydir.x + points[0].z*zdir.x;
  cv.y_min  = cv.y_max  = points[0].x*xdir.y + points[0].y*ydir.y + points[0].z*zdir.y;
  cv.z_min  = cv.z_max  = points[0].x*xdir.z + points[0].y*ydir.z + points[0].z*zdir.z;
  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
  cv.yx_min = cv.yx_max = cv.x_min - cv.y_min;

  for(cnt=1; cnt<8; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

    tmp_x = points[cnt].x*xdir.x + points[cnt].y*ydir.x + points[cnt].z*zdir.x;
    if(tmp_x < cv.x_min) cv.x_min = tmp_x - 0.001f;
    else if(tmp_x > cv.x_max) cv.x_max = tmp_x + 0.001f;

    tmp_y = points[cnt].x*xdir.y + points[cnt].y*ydir.y + points[cnt].z*zdir.y;
    if(tmp_y < cv.y_min) cv.y_min = tmp_y - 0.001f;
    else if(tmp_y > cv.y_max) cv.y_max = tmp_y + 0.001f;

    tmp_z = points[cnt].x*xdir.z + points[cnt].y*ydir.z + points[cnt].z*zdir.z;
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

  bd->mids_hi.x = (bd->cv.x_min + bd->cv.x_max) * 0.5f + pchr->pos.x;
  bd->mids_hi.y = (bd->cv.y_min + bd->cv.y_max) * 0.5f + pchr->pos.y;
  bd->mids_hi.z = (bd->cv.z_min + bd->cv.z_max) * 0.5f + pchr->pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = bd->cv.z_min + pchr->pos.z;

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

  bd->cv.level = 1;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers_2( Mad madlst[], size_t madlst_size, Chr * pchr, vect3 * vrt_ary)
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  vect3 xdir, ydir, zdir;
  Uint32 cnt;
  Uint32  vrt_count;
  bool_t  free_array = bfalse;

  CVolume cv;

  if(  NULL==pchr || !pchr->on ) return bfalse;
  bd = &(pchr->bmpdata);

  imdl = pchr->model;
  if(!VALID_MDL(imdl) || !pchr->matrixvalid )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  };

  xdir.x = (pchr->matrix).CNV(0,0);
  xdir.y = (pchr->matrix).CNV(0,1);
  xdir.z = (pchr->matrix).CNV(0,2);

  ydir.x = (pchr->matrix).CNV(1,0);
  ydir.y = (pchr->matrix).CNV(1,1);
  ydir.z = (pchr->matrix).CNV(1,2);

  zdir.x = (pchr->matrix).CNV(2,0);
  zdir.y = (pchr->matrix).CNV(2,1);
  zdir.z = (pchr->matrix).CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = madlst[imdl].md2_ptr;
  if(NULL == pmdl)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  md2_blend_vertices(pchr, -1, -1);

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  if(NULL == vrt_ary)
  {
    vrt_ary = (vect3*)calloc(vrt_count, sizeof(vect3));
    if(NULL==vrt_ary)
    {
      return chr_calculate_bumpers_1( madlst, madlst_size, pchr );
    }
    free_array = btrue;
  }

  // transform the verts all at once, to reduce function calling overhead
  Transform3( 1.0f, 1.0f, &(pchr->matrix), pchr->vdata.Vertices, vrt_ary, vrt_count);

  cv.x_min  = cv.x_max  = vrt_ary[0].x;
  cv.y_min  = cv.y_max  = vrt_ary[0].y;
  cv.z_min  = cv.z_max  = vrt_ary[0].z;
  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
  cv.yx_min = cv.yx_max = cv.x_min - cv.y_min;

  vrt_count = madlst[imdl].transvertices;
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

    tmp_yx = -tmp_x + tmp_y;
    if(tmp_yx < cv.yx_min) cv.yx_min = tmp_yx - 0.001f;
    else if(tmp_yx > cv.yx_max) cv.yx_max = tmp_yx + 0.001f;
  };

  bd->cv = cv;

  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + pchr->pos.x;
  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + pchr->pos.y;
  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + pchr->pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = cv.z_min + pchr->pos.z;

  bd->calc_height   = bd->cv.z_max;
  bd->calc_size     = MAX( bd->cv.x_max, bd->cv.y_max ) - MIN( bd->cv.x_min, bd->cv.y_min );
  bd->calc_size_big = 0.5f * ( MAX( bd->cv.xy_max, bd->cv.yx_max) - MIN( bd->cv.xy_min, bd->cv.yx_min) );

  if(bd->calc_size_big < bd->calc_size*1.1)
  {
    bd->calc_size     *= -1;
  }
  else if (bd->calc_size*2 < bd->calc_size_big*1.1)
  {
    bd->calc_size_big *= -1;
  }

  bd->cv.level = 2;

  if(free_array)
  {
    FREE( vrt_ary );
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers_3( Mad madlst[], size_t madlst_size, Chr * pchr, CVolume_Tree * cv_tree)
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  Uint32 cnt, tnc;
  Uint32  tri_count, vrt_count;
  vect3 * vrt_ary;
  CVolume *pcv, cv_node[8];

  if(  NULL==pchr || !pchr->on ) return bfalse;
  bd = &(pchr->bmpdata);

  imdl = pchr->model;
  if(!VALID_MDL(imdl) || !pchr->matrixvalid )
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  };

  pmdl = madlst[imdl].md2_ptr;
  if(NULL == pmdl)
  {
    chr_calculate_bumpers_0( pchr );
    return bfalse;
  }

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  vrt_ary   = (vect3*)calloc(vrt_count, sizeof(vect3));
  if(NULL==vrt_ary) return chr_calculate_bumpers_1(  madlst, madlst_size, pchr );

  // make sure that we have the correct bounds
  if(bd->cv.level < 2)
  {
    chr_calculate_bumpers_2(  madlst, madlst_size, pchr, vrt_ary);
  }

  if(NULL == cv_tree) return bfalse;

  // transform the verts all at once, to reduce function calling overhead
  Transform3( 1.0f, 1.0f, &(pchr->matrix), pchr->vdata.Vertices, vrt_ary, vrt_count);

  pcv = &(pchr->bmpdata.cv);

  // initialize the octree
  for(tnc=0; tnc<8; tnc++)
  {
    (*cv_tree)[tnc].level = -1;
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

    cv_node[tnc].level = 0;
  }

  tri_count = pmdl->m_numTriangles;
  for(cnt=0; cnt < tri_count; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;
    CVolume cv_tri;
    MD2_Triangle * pmd2_tri = md2_get_Triangle(pmdl, cnt);
    short * tri = pmd2_tri->vertexIndices;
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

    cv_tri.level = 0;

    // add the triangle to the octree
    for(tnc=0; tnc<8; tnc++)
    {
      if(cv_tri.x_min >= cv_node[tnc].x_max || cv_tri.x_max <= cv_node[tnc].x_min) continue;
      if(cv_tri.y_min >= cv_node[tnc].y_max || cv_tri.y_max <= cv_node[tnc].y_min) continue;
      if(cv_tri.z_min >= cv_node[tnc].z_max || cv_tri.z_max <= cv_node[tnc].z_min) continue;

      //there is an overlap with the default otree cv
      (*cv_tree)[tnc] = cvolume_merge(&(*cv_tree)[tnc], &cv_tri);
    }

  };

  bd->cv.level = 3;

  FREE( vrt_ary );

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t chr_calculate_bumpers( Mad madlst[], size_t madlst_size, Chr * pchr, int level)
{
  bool_t retval = bfalse;

  if(pchr->bmpdata.cv.level >= level) return btrue;

  switch(level)
  {
    case 2:
      // the collision volume is an octagon, the ranges are calculated using the model's vertices
      retval = chr_calculate_bumpers_2( madlst, madlst_size, pchr, NULL);
      break;

    case 3:
      {
        // calculate the octree collision volume
        if(NULL == pchr->bmpdata.cv_tree)
        {
          pchr->bmpdata.cv_tree = (CVolume_Tree*)calloc(1, sizeof(CVolume_Tree));
        };
        retval = chr_calculate_bumpers_3( madlst, madlst_size, pchr, pchr->bmpdata.cv_tree);
      };
      break;

    case 1:
      // the collision volume is a simple axis-aligned bounding box, the range is calculated from the
      // md2's bounding box
      retval = chr_calculate_bumpers_1(madlst, madlst_size, pchr);

    default:
    case 0:
      // make the simplest estimation of the bounding box using the data in data.txt
      retval = chr_calculate_bumpers_0(pchr);
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
void damage_character( CGame * gs, CHR_REF chr_ref, Uint16 direction,
                       PAIR * pdam, DAMAGE damagetype, TEAM team,
                       Uint16 attacker, Uint16 effects )
{
  // ZZ> This function calculates and applies damage to a character.  It also
  //     sets alerts and begins actions.  Blocking and frame invincibility
  //     are done here too.  Direction is FRONT if the attack is coming head on,
  //     RIGHT if from the right, BEHIND if from the back, LEFT if from the
  //     left.

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  int tnc;
  ACTION action;
  int damage, basedamage;
  Uint16 experience, model, left, right;
  AI_STATE * pstate;
  Chr * pchr;
  Mad * pmad;
  Cap * pcap;

  if( !VALID_CHR(chrlst, chr_ref) ) return;

  pchr   = chrlst + chr_ref;
  pstate = &(pchr->aistate);

  if ( NULL == pdam ) return;
  if ( chr_is_player(gs, chr_ref) && CData.DevMode ) return;

  if ( pchr->alive && pdam->ibase >= 0 && pdam->irand >= 1 )
  {
    // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    damage = generate_unsigned( pdam );
    basedamage = damage;
    damage >>= ( pchr->skin.damagemodifier_fp8[damagetype] & DAMAGE_SHIFT );

    // Allow charging (Invert damage to mana)
    if ( pchr->skin.damagemodifier_fp8[damagetype]&DAMAGE_CHARGE )
    {
      pchr->mana_fp8 += damage;
      if ( pchr->mana_fp8 > pchr->manamax_fp8 )
      {
        pchr->mana_fp8 = pchr->manamax_fp8;
      }
      return;
    }

    // Mana damage (Deal damage to mana)
    if ( pchr->skin.damagemodifier_fp8[damagetype]&DAMAGE_MANA )
    {
      pchr->mana_fp8 -= damage;
      if ( pchr->mana_fp8 < 0 )
      {
        pchr->mana_fp8 = 0;
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
      if ( pchr->damagetime == 0 && !pchr->invictus )
      {
        model = pchr->model;
        pmad = madlst + model;
        pcap = caplst + model;
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
                if ( VALID_CHR( chrlst, iholder ) )
                {
                  left  = ~caplst[iholder].iframeangle;
                  right = caplst[iholder].iframeangle;
                }
              }
              else
              {
                // Check right hand
                CHR_REF iholder = chr_get_holdingwhich( chrlst, chrlst_size, chr_ref, SLOT_RIGHT );
                if ( VALID_CHR( chrlst, iholder ) )
                {
                  left  = ~caplst[iholder].iframeangle;
                  right = caplst[iholder].iframeangle;
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
            pchr->life_fp8 -= damage;
          }
          else
          {
            pchr->life_fp8 -= FP8_MUL( damage, pchr->skin.defense_fp8 );
          }

          if ( basedamage > DAMAGE_MIN )
          {
            // Call for help if below 1/2 life
            if ( pchr->life_fp8 < ( pchr->lifemax_fp8 >> 1 ) ) //Zefz: Removed, because it caused guards to attack
              call_for_help( gs, chr_ref );                    //when dispelling overlay spells (Faerie Light)

            // Spawn blud particles
            if ( pcap->bludlevel > BLUD_NONE && ( damagetype < DAMAGE_HOLY || pcap->bludlevel == BLUD_ULTRA ) )
            {
              spawn_one_particle( gs, 1.0f, pchr->pos,
                                  pchr->turn_lr + direction, pchr->model, pcap->bludprttype,
                                  MAXCHR, GRIP_LAST, pchr->team, chr_ref, 0, MAXCHR );
            }
            // Set attack alert if it wasn't an accident
            if ( team == TEAM_DAMAGE )
            {
              pstate->attacklast = MAXCHR;
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
          if ( pchr->life_fp8 < 0 )
          {
            // Character has died
            pchr->alive = bfalse;
            disenchant_character( gs, chr_ref );
            pchr->action.keep = btrue;
            pchr->life_fp8 = -1;
            pchr->isplatform = btrue;
            pchr->bumpdampen /= 2.0;
            action = ACTION_KA;
            snd_stop_sound(pchr->loopingchannel);    //Stop sound loops
            pchr->loopingchannel = -1;
            // Give kill experience
            experience = pcap->experienceworth + ( pchr->experience * pcap->experienceexchange );
            if ( VALID_CHR( chrlst, attacker ) )
            {
              // Set target
              pstate->target = attacker;
              if ( team == TEAM_DAMAGE )  pstate->target = chr_ref;
              if ( team == TEAM_NULL )  pstate->target = chr_ref;
              // Award direct kill experience
              if ( gs->TeamList[chrlst[attacker].team].hatesteam[pchr->team] )
              {
                give_experience( gs, attacker, experience, XP_KILLENEMY );
              }

              // Check for hated
              if ( CAP_INHERIT_IDSZ( gs,  model, caplst[chrlst[attacker].model].idsz[IDSZ_HATE] ) )
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
            tnc = 0;
            while ( tnc < chrlst_size )
            {
              if ( chrlst[tnc].on && chrlst[tnc].alive )
              {
                if ( chrlst[tnc].aistate.target == chr_ref )
                {
                  chrlst[tnc].aistate.alert |= ALERT_TARGETKILLED;
                }
                if ( !gs->TeamList[chrlst[tnc].team].hatesteam[team] && gs->TeamList[chrlst[tnc].team].hatesteam[pchr->team] )
                {
                  // All allies get team experience, but only if they also hate the dead guy's team
                  give_experience( gs, tnc, experience, XP_TEAMKILL );
                }
              }
              tnc++;
            }

            // Check if it was a leader
            if ( team_get_leader( gs, pchr->team ) == chr_ref )
            {
              // It was a leader, so set more alerts
              tnc = 0;
              while ( tnc < chrlst_size )
              {
                if ( chrlst[tnc].on && chrlst[tnc].team == pchr->team )
                {
                  // All folks on the leaders team get the alert
                  chrlst[tnc].aistate.alert |= ALERT_LEADERKILLED;
                }
                tnc++;
              }

              // The team now has no leader
              gs->TeamList[pchr->team].leader = search_best_leader( gs, pchr->team, chr_ref );
            }

            detach_character_from_mount( gs, chr_ref, btrue, bfalse );
            action += ( rand() & 3 );
            play_action( gs,  chr_ref, action, bfalse );

            // Turn off all sounds if it's a player
            for ( tnc = 0; tnc < MAXWAVE; tnc++ )
            {
              //TODO Zefz: Do we need this? This makes all sounds a chr_ref makes stop when it dies...
              //           This may stop death sounds
              //snd_stop_sound(pchr->model);
            }

            // Afford it one last thought if it's an AI
            gs->TeamList[pchr->baseteam].morale--;
            pchr->team = pchr->baseteam;
            pstate->alert = ALERT_KILLED;
            pchr->sparkle = NOSPARKLE;
            pstate->time = 1;  // No timeout...
            let_character_think( gs, chr_ref, 1.0f );
          }
          else
          {
            if ( basedamage > DAMAGE_MIN )
            {
              action += ( rand() & 3 );
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
          spawn_one_particle( gs, pchr->bumpstrength, pchr->pos, pchr->turn_lr, MAXMODEL, PRTPIP_DEFEND, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
          pchr->damagetime = DELAY_DEFEND;
          pstate->alert |= ALERT_BLOCKED;
        }
      }
    }
    else if ( damage < 0 )
    {
      pchr->life_fp8 -= damage;
      if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;

      // Isssue an alert
      pstate->alert |= ALERT_HEALED;
      pstate->attacklast = attacker;
      if ( team != TEAM_DAMAGE )
      {
        pstate->attacklast = MAXCHR;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void kill_character( CGame * gs, CHR_REF chr_ref, Uint16 killer )
{
  // ZZ> This function kills a character...  MAXCHR killer for accidental death

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint8 modifier;
  Chr * pchr;

  if( !VALID_CHR(chrlst, chr_ref) ) return;

  pchr = chrlst + chr_ref;

  if ( !pchr->alive ) return;

  pchr->damagetime = 0;
  pchr->life_fp8 = 1;
  modifier = pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH];
  pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH] = 1;
  if ( VALID_CHR( chrlst, killer ) )
  {
    PAIR ptemp = {512, 1};
    damage_character( gs, chr_ref, 0, &ptemp, DAMAGE_CRUSH, chrlst[killer].team, killer, DAMFX_ARMO | DAMFX_BLOC );
  }
  else
  {
    PAIR ptemp = {512, 1};
    damage_character( gs, chr_ref, 0, &ptemp, DAMAGE_CRUSH, TEAM_DAMAGE, chr_get_aibumplast( chrlst, chrlst_size, chrlst + chr_ref ), DAMFX_ARMO | DAMFX_BLOC );
  }
  pchr->skin.damagemodifier_fp8[DAMAGE_CRUSH] = modifier;

  // try something here.
  pchr->isplatform = btrue;
  pchr->ismount    = bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t import_info_clear(IMPORT_INFO * ii)
{
  int cnt;

  if(NULL == ii) return bfalse;

  ii->player = -1;

  for ( cnt = 0; cnt < MAXPROFILE; cnt++ )
    ii->slot_lst[cnt] = 10000;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t import_info_add(IMPORT_INFO * ii, int obj)
{
  if(NULL == ii || obj < 0 || obj>=MAXPROFILE) return bfalse;
  
  ii->object = obj;
  ii->slot_lst[obj] = obj;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void cv_list_add( CVolume * cv)
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
    draw_CVolume( &(cv_list[cnt]) );
  };
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CVolume cvolume_merge(CVolume * pv1, CVolume * pv2)
{
  CVolume rv;

  rv.level = -1;

  if(NULL==pv1 && NULL==pv2)
  {
    return rv;
  }
  else if(NULL==pv2)
  {
    return *pv1;
  }
  else if(NULL==pv1)
  {
    return *pv2;
  }
  else
  {
    bool_t binvalid;

    // check for uninitialized volumes
    if(-1==pv1->level && -1==pv2->level)
    {
      return rv;
    }
    else if(-1==pv1->level)
    {
      return *pv2;
    }
    else if(-1==pv2->level)
    {
      return *pv1;
    };

    // merge the volumes

    rv.x_min = MIN(pv1->x_min, pv2->x_min);
    rv.x_max = MAX(pv1->x_max, pv2->x_max);

    rv.y_min = MIN(pv1->y_min, pv2->y_min);
    rv.y_max = MAX(pv1->y_max, pv2->y_max);

    rv.z_min = MIN(pv1->z_min, pv2->z_min);
    rv.z_max = MAX(pv1->z_max, pv2->z_max);

    rv.xy_min = MIN(pv1->xy_min, pv2->xy_min);
    rv.xy_max = MAX(pv1->xy_max, pv2->xy_max);

    rv.yx_min = MIN(pv1->yx_min, pv2->yx_min);
    rv.yx_max = MAX(pv1->yx_max, pv2->yx_max);

    // check for an invalid volume
    binvalid = (rv.x_min >= rv.x_max) || (rv.y_min >= rv.y_max) || (rv.z_min >= rv.z_max);
    binvalid = binvalid ||(rv.xy_min >= rv.xy_max) || (rv.yx_min >= rv.yx_max);

    rv.level = binvalid ? -1 : 1;
  }

  return rv;
}

//--------------------------------------------------------------------------------------------
void play_action( CGame * gs, CHR_REF character, ACTION action, bool_t ready )
{
  // ZZ> This function starts a generic action for a character

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  Chr * pchr;
  Mad * pmad;

  if(!VALID_CHR(chrlst, character)) return;

  pchr = chrlst + character;

  if(!VALID_MDL(pchr->model) || !madlst[pchr->model].used) return;

  pmad = madlst + pchr->model;

  if ( pmad->actionvalid[action] )
  {
    pchr->action.next  = ACTION_DA;
    pchr->action.now   = action;
    pchr->action.ready = ready;

    pchr->anim.lip_fp8 = 0;
    pchr->anim.flip    = 0.0f;
    pchr->anim.last    = pchr->anim.next;
    pchr->anim.next    = pmad->actionstart[pchr->action.now];
  }

}

//--------------------------------------------------------------------------------------------
void set_frame( CGame * gs, CHR_REF character, Uint16 frame, Uint8 lip )
{
  // ZZ> This function sets the frame for a character explicitly...  This is used to
  //     rotate Tank turrets

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  Uint16 start, end;
  Chr * pchr;
  Mad * pmad;

  if(!VALID_CHR(chrlst, character)) return;

  pchr = chrlst + character;

  if(!VALID_MDL(pchr->model) || !madlst[pchr->model].used) return;

  pmad = madlst + pchr->model;

  pchr->action.next  = ACTION_DA;
  pchr->action.now   = ACTION_DA;
  pchr->action.ready = btrue;

  pchr->anim.lip_fp8 = ( lip << 6 );
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
void drop_money( CGame * gs, CHR_REF character, Uint16 money )
{
  // ZZ> This function drops some of a character's money

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint16 huns, tfives, fives, ones, cnt;

  if ( money > chrlst[character].money )  money = chrlst[character].money;

  if ( money > 0 && chrlst[character].pos.z > -2 )
  {
    chrlst[character].money -= money;
    huns   = money / 100;  money -= huns   * 100;
    tfives = money /  25;  money -= tfives *  25;
    fives  = money /   5;  money -= fives  *   5;
    ones   = money;

    for ( cnt = 0; cnt < ones; cnt++ )
      spawn_one_particle( gs, 1.0f, chrlst[character].pos, 0, MAXMODEL, PRTPIP_COIN_001, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, cnt, MAXCHR );

    for ( cnt = 0; cnt < fives; cnt++ )
      spawn_one_particle( gs, 1.0f, chrlst[character].pos, 0, MAXMODEL, PRTPIP_COIN_005, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, cnt, MAXCHR );

    for ( cnt = 0; cnt < tfives; cnt++ )
      spawn_one_particle( gs, 1.0f, chrlst[character].pos, 0, MAXMODEL, PRTPIP_COIN_025, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, cnt, MAXCHR );

    for ( cnt = 0; cnt < huns; cnt++ )
      spawn_one_particle( gs, 1.0f, chrlst[character].pos, 0, MAXMODEL, PRTPIP_COIN_100, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, cnt, MAXCHR );

    chrlst[character].damagetime = DELAY_DAMAGE;  // So it doesn't grab it again
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF spawn_one_character( CGame * gs, vect3 pos, int profile, TEAM team,
                            Uint8 iskin, Uint16 facing, char *name, Uint16 override )
{
  // ZZ> This function spawns a character and returns the character's index number
  //     if it worked, MAXCHR otherwise

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  int ichr, tnc;

  AI_STATE * pstate;
  Chr  * pchr;
  Cap  * pcap;
  Mad  * pmad;

  // Make sure the team is valid
  if ( team >= TEAM_COUNT ) team %= TEAM_COUNT;

  pcap = caplst + profile;
  pmad = madlst + profile;

  // Get a new character
  if ( !pmad->used )
  {
    log_debug( "WARNING: spawn_one_character() - failed to spawn : model %d doesn't exist\n", profile );
    return MAXCHR;
  }

  ichr = MAXCHR;
  if ( VALID_CHR( chrlst, override ) )
  {
    ichr = ChrList_get_free( gs );
    if ( ichr != override )
    {
      // Picked the wrong one, so put this one back and find the right one
      tnc = 0;
      while ( tnc < chrlst_size )
      {
        if ( gs->ChrFreeList[tnc] == override )
        {
          gs->ChrFreeList[tnc] = ichr;
          tnc = MAXCHR;
        }
        tnc++;
      }
      ichr = override;
    }

    if ( MAXCHR == ichr )
    {
      log_debug( "WARNING: spawn_one_character() - failed to spawn : cannot find override index %d\n", override );
      return MAXCHR;
    }
  }
  else
  {
    ichr = ChrList_get_free( gs );

    if ( MAXCHR == ichr )
    {
      log_debug( "WARNING: spawn_one_character() - failed to spawn : ChrList_get_free() returned invalid value %d\n", ichr );
      return MAXCHR;
    }
  }

  log_debug( "spawn_one_character() - profile == %d, classname == \"%s\", index == %d\n", profile, pcap->classname, ichr );

  // "simplify" the notation
  pchr   = chrlst + ichr;
  pstate = &(pchr->aistate);

  // clear any old data
  memset(pchr, 0, sizeof(Chr));

  // IMPORTANT!!!
  pchr->indolist = bfalse;
  pchr->isequipped = bfalse;
  pchr->sparkle = NOSPARKLE;
  pchr->overlay = bfalse;
  pchr->missilehandler = ichr;

  // Set up model stuff
  pchr->on = btrue;
  pchr->freeme = bfalse;
  pchr->gopoof = bfalse;
  pchr->reloadtime = 0;
  pchr->inwhichslot = SLOT_NONE;
  pchr->inwhichpack = MAXCHR;
  pchr->nextinpack = MAXCHR;
  pchr->numinpack = 0;
  pchr->model = profile;
  VData_Blended_construct( &(pchr->vdata) );
  VData_Blended_Allocate( &(pchr->vdata), md2_get_numVertices(pmad->md2_ptr) );

  pchr->basemodel = profile;
  pchr->stoppedby = pcap->stoppedby;
  pchr->lifeheal = pcap->lifeheal_fp8;
  pchr->manacost = pcap->manacost_fp8;
  pchr->inwater = bfalse;
  pchr->nameknown = pcap->nameknown;
  pchr->ammoknown = pcap->nameknown;
  pchr->hitready = btrue;
  pchr->boretime = DELAY_BORE;
  pchr->carefultime = DELAY_CAREFUL;
  pchr->canbecrushed = bfalse;
  pchr->damageboost = 0;
  pchr->icon = pcap->icon;

  //Ready for loop sound
  pchr->loopingchannel = -1;

  // Enchant stuff
  pchr->firstenchant = MAXENCHANT;
  pchr->undoenchant = MAXENCHANT;
  pchr->canseeinvisible = pcap->canseeinvisible;
  pchr->canchannel = bfalse;
  pchr->missiletreatment = MIS_NORMAL;
  pchr->missilecost = 0;

  //Skill Expansions
  pchr->canseekurse = pcap->canseekurse;
  pchr->canusedivine = pcap->canusedivine;
  pchr->canusearcane = pcap->canusearcane;
  pchr->candisarm = pcap->candisarm;
  pchr->canjoust = pcap->canjoust;
  pchr->canusetech = pcap->canusetech;
  pchr->canusepoison = pcap->canusepoison;
  pchr->canuseadvancedweapons = pcap->canuseadvancedweapons;
  pchr->canbackstab = pcap->canbackstab;
  pchr->canread = pcap->canread;

  // Kurse state
  pchr->iskursed = (( rand() % 100 ) < pcap->kursechance );
  if ( !pcap->isitem )  pchr->iskursed = bfalse;

  // Ammo
  pchr->ammomax = pcap->ammomax;
  pchr->ammo = pcap->ammo;

  // Gender
  pchr->gender = pcap->gender;
  if ( pchr->gender == GEN_RANDOM )  pchr->gender = GEN_FEMALE + ( rand() & 1 );

  // Team stuff
  pchr->team = team;
  pchr->baseteam = team;
  pchr->messagedata = gs->TeamList[team].morale;
  if ( !pcap->invictus )  gs->TeamList[team].morale++;
  pchr->message = 0;
  // Firstborn becomes the leader
  if ( !VALID_CHR( chrlst, team_get_leader( gs, team ) ) )
  {
    gs->TeamList[team].leader = ichr;
  }

  // Skin
  if ( pcap->skinoverride != NOSKINOVERRIDE )
  {
    iskin = pcap->skinoverride % MAXSKIN;
  }
  if ( iskin >= pmad->skins )
  {
    iskin = 0;
    if ( pmad->skins > 1 )
    {
      iskin = rand() % pmad->skins;
    }
  }
  pchr->skin_ref = iskin;

  // Life and Mana
  pchr->alive = btrue;
  pchr->lifecolor = pcap->lifecolor;
  pchr->manacolor = pcap->manacolor;
  pchr->lifemax_fp8 = generate_unsigned( &pcap->life_fp8 );
  pchr->life_fp8 = pchr->lifemax_fp8;
  pchr->lifereturn = pcap->lifereturn_fp8;
  pchr->manamax_fp8 = generate_unsigned( &pcap->mana_fp8 );
  pchr->manaflow_fp8 = generate_unsigned( &pcap->manaflow_fp8 );
  pchr->manareturn_fp8 = generate_unsigned( &pcap->manareturn_fp8 );  //>> MANARETURNSHIFT;
  pchr->mana_fp8 = pchr->manamax_fp8;

  // SWID
  pchr->strength_fp8 = generate_unsigned( &pcap->strength_fp8 );
  pchr->wisdom_fp8 = generate_unsigned( &pcap->wisdom_fp8 );
  pchr->intelligence_fp8 = generate_unsigned( &pcap->intelligence_fp8 );
  pchr->dexterity_fp8 = generate_unsigned( &pcap->dexterity_fp8 );

  // Damage
  pchr->skin.defense_fp8 = pcap->skin[iskin].defense_fp8;
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;
  pchr->damagetargettype = pcap->damagetargettype;
  tnc = 0;
  while ( tnc < MAXDAMAGETYPE )
  {
    pchr->skin.damagemodifier_fp8[tnc] = pcap->skin[iskin].damagemodifier_fp8[tnc];
    tnc++;
  }

  pchr->whichplayer   = MAXPLAYER;

  // Flags
  pchr->stickybutt = pcap->stickybutt;
  pchr->openstuff = pcap->canopenstuff;
  pchr->transferblend = pcap->transferblend;
  pchr->enviro = pcap->enviro;
  pchr->waterwalk = pcap->waterwalk;
  pchr->isplatform = pcap->isplatform;
  pchr->isitem = pcap->isitem;
  pchr->invictus = pcap->invictus;
  pchr->ismount = pcap->ismount;
  pchr->cangrabmoney = pcap->cangrabmoney;

  // Jumping
  pchr->jump = pcap->jump;
  pchr->jumpready = btrue;
  pchr->jumpnumber = 1;
  pchr->jumpnumberreset = pcap->jumpnumber;
  pchr->jumptime = DELAY_JUMP;

  // Other junk
  pchr->flyheight = pcap->flyheight;
  pchr->skin.maxaccel = pcap->skin[iskin].maxaccel;
  pchr->alpha_fp8 = pcap->alpha_fp8;
  pchr->light_fp8 = pcap->light_fp8;
  pchr->flashand = pcap->flashand;
  pchr->sheen_fp8 = pcap->sheen_fp8;
  pchr->dampen = pcap->dampen;

  // Character size and bumping
  pchr->fat = pcap->size;
  pchr->sizegoto = pchr->fat;
  pchr->sizegototime = 0;

  pchr->bmpdata_save.shadow  = pcap->shadowsize;
  pchr->bmpdata_save.size    = pcap->bumpsize;
  pchr->bmpdata_save.sizebig = pcap->bumpsizebig;
  pchr->bmpdata_save.height  = pcap->bumpheight;

  pchr->bmpdata.shadow   = pcap->shadowsize  * pchr->fat;
  pchr->bmpdata.size     = pcap->bumpsize    * pchr->fat;
  pchr->bmpdata.sizebig  = pcap->bumpsizebig * pchr->fat;
  pchr->bmpdata.height   = pcap->bumpheight  * pchr->fat;
  pchr->bumpstrength   = pcap->bumpstrength * FP8_TO_FLOAT( pcap->alpha_fp8 );

  pchr->bumpdampen = pcap->bumpdampen;
  pchr->weight = pcap->weight * pchr->fat * pchr->fat * pchr->fat;   // preserve density

  // Grip info
  pchr->inwhichslot = SLOT_NONE;
  pchr->attachedto = MAXCHR;
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    pchr->holdingwhich[_slot] = MAXCHR;
  }

  // Image rendering
  pchr->uoffset_fp8 = 0;
  pchr->voffset_fp8 = 0;
  pchr->uoffvel = pcap->uoffvel;
  pchr->voffvel = pcap->voffvel;
  pchr->redshift = 0;
  pchr->grnshift = 0;
  pchr->blushift = 0;

  // Movement
  pchr->spd_sneak = pcap->spd_sneak;
  pchr->spd_walk = pcap->spd_walk;
  pchr->spd_run = pcap->spd_run;

  // Set up position
  pchr->pos.x = pos.x;
  pchr->pos.y = pos.y;
  pchr->turn_lr = facing;
  pchr->onwhichfan = mesh_get_fan( gs, pchr->pos );
  pchr->level = mesh_get_level( gs, pchr->onwhichfan, pchr->pos.x, pchr->pos.y, pchr->waterwalk ) + RAISE;
  if ( pos.z < pchr->level ) pos.z = pchr->level;
  pchr->pos.z = pos.z;

  pchr->stt         = pchr->pos;
  pchr->pos_old     = pchr->pos;
  pchr->turn_lr_old = pchr->turn_lr;

  pchr->tlight.turn_lr.r = 0;
  pchr->tlight.turn_lr.g = 0;
  pchr->tlight.turn_lr.b = 0;

  pchr->vel.x = 0;
  pchr->vel.y = 0;
  pchr->vel.z = 0;
  pchr->mapturn_lr = 32768;  // These two mean on level surface
  pchr->mapturn_ud = 32768;
  pchr->scale = pchr->fat;

  // AI stuff
  ai_state_new(gs, pstate, ichr);

  // action stuff
  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  pchr->passage = 0;
  pchr->holdingweight = 0;
  pchr->onwhichplatform = MAXCHR;

  // Timers set to 0
  pchr->grogtime = 0.0f;
  pchr->dazetime = 0.0f;

  // Money is added later
  pchr->money = pcap->money;

  // Name the character
  if ( name == NULL )
  {
    // Generate a random name
    naming_names( gs, profile );
    strncpy( pchr->name, namingnames, sizeof( pchr->name ) );
  }
  else
  {
    // A name has been given
    tnc = 0;
    while ( tnc < MAXCAPNAMESIZE - 1 )
    {
      pchr->name[tnc] = name[tnc];
      tnc++;
    }
    pchr->name[tnc] = 0;
  }

  // Set up initial fade in lighting
  tnc = 0;
  while ( tnc < madlst[pchr->model].transvertices )
  {
    pchr->vrta_fp8[tnc].r = 0;
    pchr->vrta_fp8[tnc].g = 0;
    pchr->vrta_fp8[tnc].b = 0;
    tnc++;
  }

  // Particle attachments
  for ( tnc = 0; tnc < pcap->attachedprtamount; tnc++ )
  {
    spawn_one_particle( gs, 1.0f, pchr->pos, 0, pchr->model, pcap->attachedprttype,
                        ichr, GRIP_LAST + tnc, pchr->team, ichr, tnc, MAXCHR );
  }
  pchr->reaffirmdamagetype = pcap->attachedprtreaffirmdamagetype;

  // Experience
  if ( pcap->leveloverride != 0 )
  {
    while ( pchr->experiencelevel < pcap->leveloverride )
    {
      give_experience( gs, ichr, 100, XP_DIRECT );
    }
  }
  else
  {
    pchr->experience = generate_unsigned( &pcap->experience );
    pchr->experiencelevel = calc_chr_level( gs, ichr );
  }

  pchr->pancakepos.x = pchr->pancakepos.y = pchr->pancakepos.z = 1.0;
  pchr->pancakevel.x = pchr->pancakevel.y = pchr->pancakevel.z = 0.0f;

  pchr->loopingchannel = INVALID_CHANNEL;

  // calculate the bumpers
  assert(NULL == pchr->bmpdata.cv_tree);
  chr_bdata_reinit( pchr, &(pchr->bmpdata) );
  make_one_character_matrix( chrlst, chrlst_size, pchr );

  return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( CGame * gs, CHR_REF ichr )
{
  // ZZ> This function respawns a character

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Uint16 item, profile;
  Chr * pchr;
  Cap * pcap;
  AI_STATE * pstate;

  if ( !VALID_CHR( chrlst, ichr )  ) return;

  pchr = chrlst + ichr;
  pstate = &(pchr->aistate);

  if( pchr->alive ) return;

  profile = pchr->model;
  pcap = caplst + profile;

  spawn_poof( gs, ichr, profile );
  disaffirm_attached_particles( gs, ichr );
  pchr->alive = btrue;
  pchr->boretime = DELAY_BORE;
  pchr->carefultime = DELAY_CAREFUL;
  pchr->life_fp8 = pchr->lifemax_fp8;
  pchr->mana_fp8 = pchr->manamax_fp8;
  pchr->pos.x = pchr->stt.x;
  pchr->pos.y = pchr->stt.y;
  pchr->pos.z = pchr->stt.z;
  pchr->vel.x = 0;
  pchr->vel.y = 0;
  pchr->vel.z = 0;
  pchr->team = pchr->baseteam;
  pchr->canbecrushed = bfalse;
  pchr->mapturn_lr = 32768;  // These two mean on level surface
  pchr->mapturn_ud = 32768;
  if ( !VALID_CHR( chrlst, team_get_leader( gs, pchr->team ) ) )  gs->TeamList[pchr->team].leader = ichr;
  if ( !pchr->invictus )  gs->TeamList[pchr->baseteam].morale++;

  action_info_new( &(pchr->action) );
  anim_info_new( &(pchr->anim) );

  pchr->isplatform = pcap->isplatform;
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
  ai_state_renew(pstate, ichr);

  pchr->grogtime = 0.0f;
  pchr->dazetime = 0.0f;

  reaffirm_attached_particles( gs, ichr );

  // Let worn items come back
  item  = chr_get_nextinpack( chrlst, chrlst_size, ichr );
  while ( VALID_CHR( chrlst, item ) )
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
void signal_target( CGame * gs, CHR_REF target_ref, Uint16 upper, Uint16 lower )
{
  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  if ( !VALID_CHR( chrlst,  target_ref ) ) return;

  chrlst[target_ref].message = ( upper << 16 ) | lower;
  chrlst[target_ref].messagedata = 0;
  chrlst[target_ref].aistate.alert |= ALERT_SIGNALED;
};


//--------------------------------------------------------------------------------------------
void signal_team( CGame * gs, CHR_REF chr_ref, Uint32 message )
{
  // ZZ> This function issues an message for help to all teammates

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  TEAM team;
  Uint8 counter;
  Uint16 cnt;

  team = chrlst[chr_ref].team;
  counter = 0;
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( chrlst,  cnt ) || chrlst[cnt].team != team ) continue;

    chrlst[cnt].message = message;
    chrlst[cnt].messagedata = counter;
    chrlst[cnt].aistate.alert |= ALERT_SIGNALED;
    counter++;
  }
}

//--------------------------------------------------------------------------------------------
void signal_idsz_index( CGame * gs, Uint32 order, IDSZ idsz, IDSZ_INDEX index )
{
  // ZZ> This function issues an order to all characters with the a matching special IDSZ

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Uint8 counter;
  Uint16 cnt, model;

  counter = 0;
  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( chrlst,  cnt ) ) continue;

    model = chrlst[cnt].model;

    if ( caplst[model].idsz[index] != idsz ) continue;

    chrlst[cnt].message        = order;
    chrlst[cnt].messagedata    = counter;
    chrlst[cnt].aistate.alert |= ALERT_SIGNALED;
    counter++;
  }
}


//--------------------------------------------------------------------------------------------
bool_t ai_state_advance_wp(AI_STATE * a, bool_t do_atlastwaypoint)
{
  if(NULL == a) return bfalse;

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
Cap * Cap_new(Cap *pcap) 
{ 
  //fprintf( stdout, "Cap_new()\n");

  if(NULL==pcap) return pcap; 
  
  memset(pcap, 0, sizeof(Cap));

  pcap->used         = bfalse;
  pcap->classname[0] = '\0';
  
  return pcap; 
};

//--------------------------------------------------------------------------------------------
bool_t Cap_delete( Cap * pcap )
{
  if(NULL == pcap) return bfalse;

  pcap->used         = bfalse;
  pcap->classname[0] = '\0';

  return btrue;
}

//--------------------------------------------------------------------------------------------
Cap * Cap_renew(Cap *pcap)
{
  Cap_delete( pcap );
  return Cap_new(pcap);
}