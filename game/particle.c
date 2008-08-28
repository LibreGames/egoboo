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
/// @brief Egoboo particle system
/// @details Manages the particle system

#include "particle.inl"

#include "Log.h"
#include "camera.h"
#include "sound.h"
#include "enchant.h"
#include "game.h"
#include "Clock.h"
#include "file_common.h"

#include "egoboo_utility.h"
#include "egoboo_rpc.h"
#include "egoboo.h"

#include <assert.h>

#include "char.inl"
#include "Md2.inl"
#include "mesh.inl"
#include "object.inl"
#include "input.inl"
#include "Physics.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"


static PRT_REF _prt_spawn( PRT_SPAWN_INFO si, bool_t activate );
static bool_t  _prt_is_over_water( struct sGame * gs, PRT_REF cnt );

//--------------------------------------------------------------------------------------------
size_t DLightList_clear( Graphics_Data_t * gfx )
{
  gfx->DLightList_count = 0;
  return gfx->DLightList_count;
}


//--------------------------------------------------------------------------------------------
size_t DLightList_prune( Graphics_Data_t * gfx )
{
  /// @details BB@> remove all dynalights that are not permanent

  int i,j;
  PDLight_t dynalst = gfx->DLightList;

  // put all permanent lights at the beginning of the list
  for(i = 0, j=0; i<gfx->DLightList_count; i++)
  {
    if( dynalst[i].permanent )
    {
      if(i != j)
      {
        memcpy(dynalst + j, dynalst + i, sizeof(DYNALIGHT_INFO));
      };
      j++;
    }
  };

  // set the size of the list
  gfx->DLightList_count = j;

  return gfx->DLightList_count;
};

//--------------------------------------------------------------------------------------------
size_t DLightList_add( Graphics_Data_t * gfx, DYNALIGHT_INFO * di )
{
  int slot, tnc;
  float disx, disy, disz, absdis, max_dist;
  PDLight_t dynalst = gfx->DLightList;

  disx = ABS( di->pos.x - GCamera.trackpos.x );
  disy = ABS( di->pos.y - GCamera.trackpos.y );
  disz = ABS( di->pos.z - GCamera.trackpos.z );
  absdis = disx + disy + disz;
  di->distance = absdis;

  slot = -1;
  if(gfx->DLightList_count < MAXDYNA)
  {
    slot = gfx->DLightList_count;
    gfx->DLightList_count++;

    if( !di->permanent )
    {
      gfx->DLightList_distancetobeat = MAX(gfx->DLightList_distancetobeat, di->distance);
    };
  }
  else if(absdis < gfx->DLightList_distancetobeat || di->permanent)
  {
    // Overwrite the worst non-permanent one

    // find the farthest entry
    slot = 0;
    max_dist = dynalst[0].distance;
    for ( tnc = 1; tnc < MAXDYNA; tnc++ )
    {
      if ( !dynalst[tnc].permanent && dynalst[tnc].distance > max_dist )
      {
        slot = tnc;
        max_dist = dynalst[tnc].distance;
      }
    }

    // Find the new maximum distance
    max_dist = dynalst[0].distance;
    for ( tnc = 1; tnc < MAXDYNA; tnc++ )
    {
      if(tnc == slot) continue;

      if ( !dynalst[tnc].permanent && dynalst[tnc].distance > max_dist )
      {
        slot = tnc;
        max_dist = dynalst[tnc].distance;
      }
    }

  };

  // actually do the insertion
  if( slot >= 0)
  {
    memcpy(dynalst + slot, di, sizeof(DYNALIGHT_INFO));
  };

  return gfx->DLightList_count;

};

//--------------------------------------------------------------------------------------------
void make_prtlist(Game_t * gs)
{
  /// @details ZZ@> This function figures out which particles are visible, and it sets up dynamic
  ///     lighting

  PRT_REF        prt_cnt;

  PPrt_t prtlst      = NULL;
  size_t prtlst_size = PRTLST_COUNT;

  Mesh_t    * pmesh = NULL;
  MeshMem_t * mm    = NULL;
  Graphics_Data_t * gfx = NULL;

  if(NULL == gs) gs = Graphics_requireGame( &gfxState );

  gfx = Game_getGfx( gs );
  prtlst = gs->PrtList;
  pmesh  = Game_getMesh(gs);
  mm     = &(pmesh->Mem);

  // remove all non-permanent lights from the previous iteration
  DLightList_prune( gfx );

  // go through the particle list
  for ( prt_cnt = 0; prt_cnt < prtlst_size; prt_cnt++ )
  {
    prtlst[prt_cnt].inview = bfalse;
    if ( !ACTIVE_PRT( prtlst,  prt_cnt ) ) continue;

    // set to visible or not
    prtlst[prt_cnt].inview = mesh_fan_is_in_renderlist( mm->tilelst, prtlst[prt_cnt].onwhichfan );

    // Set up the dynamic lights we need
    if(prtlst[prt_cnt].dyna.falloff > 0 && prtlst[prt_cnt].dyna.level > 0 )
    {
      DYNALIGHT_INFO di;

      di.permanent = bfalse;
      di.pos       = prtlst[prt_cnt].ori.pos;
      di.level     = prtlst[prt_cnt].dyna.level;
      di.falloff   = prtlst[prt_cnt].dyna.falloff;

      DLightList_add( gfx, &di );
    };
  }

}

//--------------------------------------------------------------------------------------------
void PrtList_free_one( Game_t * gs, PRT_REF particle )
{
  /// @details ZZ@> This function sticks a particle back on the free particle stack

  Prt_t * pprt;

  // already deleted
  if(!VALID_PRT(gs->PrtList, particle)) return;

  PrtHeap_addFree( &(gs->PrtHeap),  particle );

  pprt = gs->PrtList + particle;
  pprt->active = bfalse;
  Prt_delete(pprt);

  //log_debug("PrtList_free_one() - \n\tPrtFreeList_count == %d\n", gs->PrtHeap.free_count);
}

//--------------------------------------------------------------------------------------------
void end_one_particle( Game_t * gs, PRT_REF particle )
{
  /// @details ZZ@> This function sticks a particle back on the free particle stack and
  ///     plays the sound associated with the particle

  PChr_t chrlst      = gs->ChrList;
  PPip_t piplst      = gs->PipList;

  CHR_REF child;

  Prt_t * pprt = PrtList_getPPrt(gs, particle);
  if(NULL == pprt) return;

  if ( pprt->spawncharacterstate != SPAWN_NOCHARACTER )
  {
    child = chr_spawn( gs, pprt->ori.pos,  pprt->ori.vel, pprt->model, pprt->team, 0, pprt->facing, NULL, INVALID_CHR );
    if ( VALID_CHR( chrlst,  child ) )
    {
      chrlst[child].aistate.state = pprt->spawncharacterstate;
      chrlst[child].aistate.owner = prt_get_owner( gs, particle );
    }
  }
  snd_play_particle_sound( gs, 1.0f, particle, piplst[pprt->pip].soundend );

  PrtList_free_one( gs, particle );
}

//--------------------------------------------------------------------------------------------
PRT_REF PrtList_get_free( Game_t * gs, bool_t is_critical )
{
  /// @details ZZ@> This function gets an unused particle.  If all particles are in use
  ///     and is_critical is set, it grabs the first unimportant one.  The particle
  ///     index is the return value
  ///
  ///     Reserve PRTLST_COUNT / 4 particles for critical particles

  PPrt_t prtlst      = gs->PrtList;

  PRT_REF iprt;

  // Return INALID_PRT if we can't find one
  iprt = INVALID_PRT;

  if ( gs->PrtHeap.free_count > PRTLST_COUNT / 4 )
  {
    // Just grab the next one
    iprt = PrtHeap_getFree( &(gs->PrtHeap), INVALID_PRT );
  }

  if(INVALID_PRT == iprt && is_critical)
  {
    iprt = PrtHeap_getFree( &(gs->PrtHeap), INVALID_PRT );

    if( INVALID_PRT == iprt )
    {
      // Gotta find one, so go through the list
      for( iprt = 0; iprt < PRTLST_COUNT; iprt++ )
      {
        if ( prtlst[iprt].bumpsize == 0 || prtlst[iprt].bumpstrength == 0.0f )
        {
          PrtList_free_one(gs, iprt);
          iprt = PrtHeap_getFree( &(gs->PrtHeap), INVALID_PRT );
          break;
        }
      }
    }
  }


  //if ( retval != irequest )
  //{
  //  log_debug( "WARNING: PrtList_get_free() - \n\tcannot find irequest index %d\n", irequest );
  //  return INVALID_PRT;
  //}

  // initialize the particle
  if(INVALID_PRT != iprt)
  {
    Prt_new(prtlst + iprt);
    PrtHeap_addUsed( &(gs->PrtHeap), iprt );
  }
  else
  {
    log_debug( "WARNING: PrtList_get_free() - \n\tcould not get valid particle\n");
  }

  //log_debug("PrtList_get_free() - \n\tPrtFreeList_count == %d\n", gs->PrtFreeList_count);

  return iprt;
}

//--------------------------------------------------------------------------------------------
PRT_REF prt_spawn_info_init( PRT_SPAWN_INFO * psi, Game_t * gs, float intensity, vect3 pos, vect3 vel,
                           Uint16 facing, OBJ_REF iobj, PIP_REF local_pip,
                           CHR_REF characterattach, Uint32 offset, TEAM_REF team,
                           CHR_REF characterorigin, Uint16 multispawn, CHR_REF oldtarget )
{
  /// @details ZZ@> This function spawns a new particle, and returns the number of that particle

  PPrt_t prtlst      = gs->PrtList;
  PPip_t piplst      = gs->PipList;

  Prt_t * pprt;
  Pip_t * ppip;

  prt_spawn_info_new(psi, gs);

  // fill in the basic spawn info
  psi->gs              = gs;
  psi->seed            = gs->randie_index;
  psi->iobj            = iobj;
  psi->intensity       = intensity;
  psi->pos             = pos;
  psi->vel             = vel;
  psi->facing          = facing;
  psi->characterattach = characterattach;
  psi->offset          = offset;
  psi->team            = team;
  psi->characterorigin = characterorigin;
  psi->multispawn      = multispawn;
  psi->oldtarget       = oldtarget;

  // assume the worst
  psi->ipip = INVALID_PIP;

  // Convert from local pip to global pip
  if ( INVALID_CHR != iobj && local_pip < PRTPIP_PEROBJECT_COUNT )
  {
    psi->ipip = ObjList_getRPip(gs, iobj, REF_TO_INT(local_pip));
  }

  // assume we were given a global pip
  if ( INVALID_PIP == psi->ipip )
  {
    psi->ipip = local_pip;
  }

  if ( !LOADED_PIP(piplst, psi->ipip) )
  {
    log_debug( "WARN: prt_spawn_info_init() - \n\tfailed to spawn : local_pip == %d is an invalid value\n", local_pip );
    return INVALID_PRT;
  }
  ppip = piplst + psi->ipip;

  psi->iobj = iobj;

  psi->iprt = PrtList_get_free( gs, ppip->force );
  if ( INVALID_PRT == psi->iprt )
  {
    if(ppip->force)
    {
      log_debug( "WARN: prt_spawn_info_init() failed - PrtList_get_free() returned %d, PrtFreeList_count == %d\n", REF_TO_INT(psi->iprt), gs->PrtHeap.free_count );
    }
    else
    {
      log_debug( "WARN: prt_spawn_info_init() failed - possible particle overflow. PrtFreeList_count == %d \n", gs->PrtHeap.free_count  );
    }

    return INVALID_PRT;
  }

  // reserve the particle
  pprt = prtlst + psi->iprt;
  pprt->reserved = btrue;

  return psi->iprt;
}

//--------------------------------------------------------------------------------------------
PRT_REF _prt_spawn( PRT_SPAWN_INFO si, bool_t activate )
{
  /// @details ZZ@> This function requests a particle to spawn on the local machine
  ///     it will not actually spawn until a confirmation is received from the server

  Uint32 loc_rand;

  PObj_t objlst;
  size_t  objlst_size;

  PPrt_t prtlst;
  size_t prtlst_size;

  PPip_t piplst;
  size_t piplst_size;

  PChr_t chrlst;
  size_t chrlst_size;

  PMad_t madlst;
  size_t madlst_size;

  float velocity;
  vect3 pos, vel;
  int offsetfacing, newrand;
  float weight;
  CHR_REF prt_target;
  Prt_t * pprt;
  Pip_t * ppip;
  SearchInfo_t loc_search;
  Uint16 facing;

  assert( EKEY_PVALID(si.gs) );

  loc_rand = si.seed;

  objlst      = si.gs->ObjList;
  objlst_size = OBJLST_COUNT;

  prtlst      = si.gs->PrtList;
  prtlst_size = PRTLST_COUNT;

  piplst      = si.gs->PipList;
  piplst_size = PIPLST_COUNT;

  chrlst      = si.gs->ChrList;
  chrlst_size = CHRLST_COUNT;

  madlst      = si.gs->MadList;
  madlst_size = MADLST_COUNT;

  // make sure we have a valid particle
  if ( !RESERVED_PRT(prtlst, si.iprt) )
  {
    log_debug( "WARN: req_spawn_one_particle() - \n\tfailed to spawn : particle index == %d is an invalid value\n", REF_TO_INT(si.iprt) );
    return INVALID_PRT;
  }
  pprt = prtlst + si.iprt;

  // make sure we have a valid ipip
  if ( !LOADED_PIP(piplst, si.ipip) )
  {
    log_debug( "WARN: req_spawn_one_particle() - \n\tfailed to spawn : local_pip == %d is an invalid value\n", REF_TO_INT(si.ipip) );
    return INVALID_PRT;
  }
  ppip = piplst + si.ipip;

  weight = 1.0f;
  if ( ACTIVE_CHR( chrlst,  si.characterorigin ) ) weight = MAX( weight, chrlst[si.characterorigin].weight );
  if ( ACTIVE_CHR( chrlst,  si.characterattach ) ) weight = MAX( weight, chrlst[si.characterattach].weight );
  pprt->weight = weight;

  //log_debug( "req_spawn_one_particle() - \n\tlocal pip == %d, global pip == %d, part == %d\n", local_pip, si.ipip, si.iprt);
  //log_debug( "req_spawn_one_particle() - \n\tpart == %d, free ==  %d\n", REF_TO_INT(si.iprt), si.gs->PrtFreeList_count);

  // !!!! basic initialization should have been done by prt_spawn_info_init() !!!!
  // Prt_new(pprt);

  // copy the spawn info for any respawn command
  pprt->spinfo = si;

  // Necessary data for any part
  pprt->pip = si.ipip;
  pprt->model = si.iobj;
  pprt->team = si.team;
  pprt->owner = si.characterorigin;
  pprt->damagetype = ppip->damagetype;

  // Lighting and sound
  pprt->dyna.on = bfalse;
  if ( si.multispawn == 0 )
  {
    pprt->dyna.on = ( DYNA_OFF != ppip->dyna.mode );
    if ( ppip->dyna.mode == DYNA_LOCAL )
    {
      pprt->dyna.on = bfalse;
    }
  }
  pprt->dyna.level   = ppip->dyna.level * si.intensity;
  pprt->dyna.falloff = ppip->dyna.falloff;


  // Set character attachments ( INVALID_CHR == si.characterattach means none )
  pprt->attachedtochr = si.characterattach;
  pprt->vertoffset = si.offset;

  // Targeting...
  offsetfacing = 0;
  vel = si.vel;
  pos = si.pos;
  facing = si.facing;
  pos.z += generate_signed( &loc_rand, &ppip->zspacing );
  velocity = generate_unsigned( &loc_rand, &ppip->xyvel );
  pprt->target = si.oldtarget;
  prt_target = INVALID_CHR;
  if ( ppip->newtargetonspawn )
  {
    if ( ppip->targetcaster )
    {
      // Set the target to the caster
      pprt->target = si.characterorigin;
    }
    else
    {
      // Correct facing for dexterity...
      if ( chrlst[si.characterorigin].stats.dexterity_fp8 < PERFECTSTAT )
      {
        // Correct facing for randomness
        newrand = FP8_DIV( PERFECTSTAT - chrlst[si.characterorigin].stats.dexterity_fp8,  PERFECTSTAT );
        offsetfacing += generate_dither( &loc_rand, &ppip->facing, newrand );
      }

      // Find a target
      if( prt_search_wide( si.gs, SearchInfo_new(&loc_search), si.iprt, facing, ppip->targetangle, ppip->onlydamagefriendly, bfalse, si.team, si.characterorigin, si.oldtarget ) )
      {
        pprt->target = loc_search.besttarget;
      };

      prt_target = prt_get_target( si.gs, si.iprt );
      if ( ACTIVE_CHR( chrlst,  prt_target ) )
      {
        offsetfacing -= loc_search.useangle;

        if ( ppip->zaimspd != 0 )
        {
          // These aren't velocities...  This is to do aiming on the Z axis
          if ( velocity > 0 )
          {
            float dx,dy, dt,vz;
            dx = chrlst[prt_target].ori.pos.x - pos.x;
            dy = chrlst[prt_target].ori.pos.y - pos.y;
            dt = sqrt( dx * dx + dy * dy ) / velocity;   // This is the time
            if ( dt > 0 )
            {
              vz = ( chrlst[prt_target].ori.pos.z + ( chrlst[prt_target].bmpdata.calc_size * 0.5f ) - pos.z ) / dt;  // This is the vel.z alteration
              vel.z += CLIP(vz, - ( ppip->zaimspd >> 1 ), ( ppip->zaimspd >> 1 ));
            }
          }
        }
      }
    }

    // Does it go away?
    if ( !ACTIVE_CHR( chrlst,  prt_target ) && ppip->needtarget )
    {
      log_debug( "WARN: prt_spawn_info_init() - \n\tfailed to spawn : pip requires target and no target specified\n", si.iprt );
      end_one_particle( si.gs, si.iprt );
      return INVALID_PRT;
    }

    // Start on top of target
    if ( ACTIVE_CHR( chrlst,  prt_target ) && ppip->startontarget )
    {
      pos.x = chrlst[prt_target].ori.pos.x;
      pos.y = chrlst[prt_target].ori.pos.y;
    }
  }
  else
  {
    // Correct facing for randomness
    offsetfacing += generate_dither( &loc_rand, &ppip->facing, INT_TO_FP8( 1 ) );
  }
  facing += ppip->facing.ibase + offsetfacing;
  pprt->facing = facing;
  facing >>= 2;

  // Location data from arguments
  newrand = generate_unsigned( &loc_rand, &ppip->xyspacing );
  pos.x += turntocos[( si.facing+8192 ) & TRIGTABLE_MASK] * newrand;
  pos.y += turntosin[( si.facing+8192 ) & TRIGTABLE_MASK] * newrand;

  pos.x = mesh_clip_x( &(si.gs->Mesh.Info), si.pos.x );
  pos.y = mesh_clip_y( &(si.gs->Mesh.Info), si.pos.y );

  pprt->ori.pos = pos;

  // Velocity data
  vel.x += turntocos[( si.facing+8192 ) & TRIGTABLE_MASK] * velocity;
  vel.y += turntosin[( si.facing+8192 ) & TRIGTABLE_MASK] * velocity;
  vel.z += generate_signed( &loc_rand, &ppip->zvel );
  pprt->ori.vel = vel;

  pprt->ori_old.pos.x = pprt->ori.pos.x - pprt->ori.vel.x;
  pprt->ori_old.pos.y = pprt->ori.pos.y - pprt->ori.vel.y;
  pprt->ori_old.pos.z = pprt->ori.pos.z - pprt->ori.vel.z;

  // Template values
  pprt->bumpsize     = ppip->bumpsize;
  pprt->bumpsizebig  = pprt->bumpsize + ( pprt->bumpsize >> 1 );
  pprt->bumpheight   = ppip->bumpheight;
  pprt->bumpstrength = ppip->bumpstrength * si.intensity;

  // figure out the particle type and transparency
  pprt->type = ppip->type;
  pprt->alpha_fp8 = 255;
  switch ( ppip->type )
  {
    case PRTTYPE_SOLID:
      if ( si.intensity < 1.0f )
      {
        pprt->type      = PRTTYPE_ALPHA;
        pprt->alpha_fp8 = FLOAT_TO_FP8(si.intensity);
      }
      break;

    case PRTTYPE_LIGHT:
      pprt->alpha_fp8 = FLOAT_TO_FP8(si.intensity);
      break;

    case PRTTYPE_ALPHA:
      pprt->alpha_fp8 = particletrans_fp8 * si.intensity;
      break;
  };

  // Image data
  pprt->rotate = generate_unsigned( &loc_rand, &ppip->rotate );
  pprt->rotateadd = ppip->rotateadd;
  pprt->size_fp8 = ppip->sizebase_fp8;
  pprt->sizeadd_fp8 = ppip->sizeadd;
  pprt->imageadd_fp8 = generate_unsigned( &loc_rand, &ppip->imageadd );
  pprt->imagestt_fp8 = INT_TO_FP8( ppip->imagebase );
  pprt->imagemax_fp8 = INT_TO_FP8( ppip->numframes );
  pprt->time = ppip->time;
  if ( ppip->endlastframe )
  {
    if ( pprt->imageadd_fp8 != 0 ) pprt->time = 0.0;
  }

  // Set onwhichfan...
  pprt->onwhichfan = mesh_get_fan( Game_getMesh(si.gs), pprt->ori.pos );

  // Damage stuff
  pprt->damage.ibase = ppip->damage_fp8.ibase * si.intensity;
  pprt->damage.irand = ppip->damage_fp8.irand * si.intensity;

  // Spawning data
  pprt->spawntime = ppip->contspawntime;
  if ( pprt->spawntime != 0 )
  {
    CHR_REF prt_attachedto = prt_get_attachedtochr( si.gs, si.iprt );

    pprt->spawntime = 1;
    if ( ACTIVE_CHR( chrlst,  prt_attachedto ) )
    {
      pprt->spawntime++; // Because attachment takes an update before it happens
    }
  }

  // if someone requested us to activate, set the particle for automatic activation
  pprt->req_active = activate;
  if(pprt->req_active)
  {
    pprt->reserved = bfalse;
  }

  return si.iprt;
}





//--------------------------------------------------------------------------------------------
PRT_REF prt_spawn( Game_t * gs, float intensity, vect3 pos, vect3 vel,
                   Uint16 facing, OBJ_REF model, PIP_REF pip,
                   CHR_REF chr_attach, Uint32 offset, TEAM_REF team,
                   CHR_REF chr_origin, Uint16 multispawn, CHR_REF oldtarget )
{
  PRT_REF retval;
  PRT_SPAWN_INFO prt_si;
  bool_t local_control;

  if(!EKEY_PVALID(gs)) return INVALID_PRT;

  // initialize the spawning data
  if(NULL == prt_spawn_info_new( &prt_si, gs )) return INVALID_PRT;

  retval = prt_spawn_info_init( &prt_si, gs, intensity, pos, vel, facing, model, pip,
                                 chr_attach, offset, team, chr_origin, multispawn, oldtarget );
  if(INVALID_PRT == retval) return INVALID_PRT;

  // determine if this computer is in control of the spawn
  local_control = Game_hasServer( gs );

  // Initialize the particle data.
  // It will automatically activate if this computer is in control of the spawn
  retval = _prt_spawn( prt_si, local_control );
  if(INVALID_PRT == retval) return INVALID_PRT;

  if ( !local_control )
  {
    // This computer does not control the spawn.
    // The character will only activate it the server to OKs this spawn.
    snd_prt_spawn( prt_si );
  }

  return retval;
}

////--------------------------------------------------------------------------------------------
//PRT_REF req_spawn_one_particle( PRT_SPAWN_INFO si )
//{
//  /// @details ZZ@> This function requests a particle to spawn on the local machine
//  ///     it will not actually spawn until a confirmation is received from the server
//
//  Uint32 loc_rand;
//
//  PObj_t objlst;
//  size_t  objlst_size;
//
//  PPrt_t prtlst;
//  size_t prtlst_size;
//
//  PPip_t piplst;
//  size_t piplst_size;
//
//  PChr_t chrlst;
//  size_t chrlst_size;
//
//  PMad_t madlst;
//  size_t madlst_size;
//
//  float velocity;
//  vect3 pos, vel;
//  int offsetfacing, newrand;
//  float weight;
//  CHR_REF prt_target;
//  Prt_t * pprt;
//  Pip_t * ppip;
//  SearchInfo_t loc_search;
//  Uint16 facing;
//
//  assert( EKEY_PVALID(si.gs) );
//
//  loc_rand = si.seed;
//
//  objlst      = si.gs->ObjList;
//  objlst_size = OBJLST_COUNT;
//
//  prtlst      = si.gs->PrtList;
//  prtlst_size = PRTLST_COUNT;
//
//  piplst      = si.gs->PipList;
//  piplst_size = PIPLST_COUNT;
//
//  chrlst      = si.gs->ChrList;
//  chrlst_size = CHRLST_COUNT;
//
//  madlst      = si.gs->MadList;
//  madlst_size = MADLST_COUNT;
//
//  // make sure we have a valid particle
//  if ( !RESERVED_PRT(prtlst, si.iprt) )
//  {
//    log_debug( "WARN: req_spawn_one_particle() - \n\tfailed to spawn : particle index == %d is an invalid value\n", REF_TO_INT(si.iprt) );
//    return INVALID_PRT;
//  }
//  pprt = prtlst + si.iprt;
//
//  // make sure we have a valid ipip
//  if ( !LOADED_PIP(piplst, si.ipip) )
//  {
//    log_debug( "WARN: req_spawn_one_particle() - \n\tfailed to spawn : local_pip == %d is an invalid value\n", REF_TO_INT(si.ipip) );
//    return INVALID_PRT;
//  }
//  ppip = piplst + si.ipip;
//
//
//  weight = 1.0f;
//  if ( ACTIVE_CHR( chrlst,  si.characterorigin ) ) weight = MAX( weight, chrlst[si.characterorigin].weight );
//  if ( ACTIVE_CHR( chrlst,  si.characterattach ) ) weight = MAX( weight, chrlst[si.characterattach].weight );
//  prtlst[si.iprt].weight = weight;
//
//  //log_debug( "req_spawn_one_particle() - \n\tlocal pip == %d, global pip == %d, part == %d\n", local_pip, si.ipip, si.iprt);
//  //log_debug( "req_spawn_one_particle() - \n\tpart == %d, free ==  %d\n", REF_TO_INT(si.iprt), si.gs->PrtFreeList_count);
//
//  // do basic initialization
//  Prt_new(pprt);
//
//  // copy the spawn info for any respawn command
//  pprt->spinfo = si;
//
//  // Necessary data for any part
//  pprt->pip = si.ipip;
//  pprt->model = si.iobj;
//  pprt->team = si.team;
//  pprt->owner = si.characterorigin;
//  pprt->damagetype = ppip->damagetype;
//
//  // Lighting and sound
//  pprt->dyna.on = bfalse;
//  if ( si.multispawn == 0 )
//  {
//    pprt->dyna.on = ( DYNA_OFF != ppip->dyna.mode );
//    if ( ppip->dyna.mode == DYNA_LOCAL )
//    {
//      pprt->dyna.on = bfalse;
//    }
//  }
//  pprt->dyna.level   = ppip->dyna.level * si.intensity;
//  pprt->dyna.falloff = ppip->dyna.falloff;
//
//
//  // Set character attachments ( INVALID_CHR == si.characterattach means none )
//  pprt->attachedtochr = si.characterattach;
//  pprt->vertoffset = si.offset;
//
//  // Targeting...
//  offsetfacing = 0;
//  vel = si.vel;
//  pos = si.pos;
//  facing = si.facing;
//  pos.z += generate_signed( &loc_rand, &ppip->zspacing );
//  velocity = generate_unsigned( &loc_rand, &ppip->xyvel );
//  pprt->target = si.oldtarget;
//  prt_target = INVALID_CHR;
//  if ( ppip->newtargetonspawn )
//  {
//    if ( ppip->targetcaster )
//    {
//      // Set the target to the caster
//      pprt->target = si.characterorigin;
//    }
//    else
//    {
//      // Correct facing for dexterity...
//      if ( chrlst[si.characterorigin].stats.dexterity_fp8 < PERFECTSTAT )
//      {
//        // Correct facing for randomness
//        newrand = FP8_DIV( PERFECTSTAT - chrlst[si.characterorigin].stats.dexterity_fp8,  PERFECTSTAT );
//        offsetfacing += generate_dither( &loc_rand, &ppip->facing, newrand );
//      }
//
//      // Find a target
//      if( prt_search_wide( si.gs, SearchInfo_new(&loc_search), si.iprt, facing, ppip->targetangle, ppip->onlydamagefriendly, bfalse, si.team, si.characterorigin, si.oldtarget ) )
//      {
//        pprt->target = loc_search.besttarget;
//      };
//
//      prt_target = prt_get_target( si.gs, si.iprt );
//      if ( ACTIVE_CHR( chrlst,  prt_target ) )
//      {
//        offsetfacing -= loc_search.useangle;
//
//        if ( ppip->zaimspd != 0 )
//        {
//          // These aren't velocities...  This is to do aiming on the Z axis
//          if ( velocity > 0 )
//          {
//            float dx,dy, dt,vz;
//            dx = chrlst[prt_target].ori.pos.x - pos.x;
//            dy = chrlst[prt_target].ori.pos.y - pos.y;
//            dt = sqrt( dx * dx + dy * dy ) / velocity;   // This is the time
//            if ( dt > 0 )
//            {
//              vz = ( chrlst[prt_target].ori.pos.z + ( chrlst[prt_target].bmpdata.calc_size * 0.5f ) - pos.z ) / dt;  // This is the vel.z alteration
//              vel.z += CLIP(vz, - ( ppip->zaimspd >> 1 ), ( ppip->zaimspd >> 1 ));
//            }
//          }
//        }
//      }
//    }
//
//    // Does it go away?
//    if ( !ACTIVE_CHR( chrlst,  prt_target ) && ppip->needtarget )
//    {
//      log_debug( "WARN: prt_spawn_info_init() - \n\tfailed to spawn : pip requires target and no target specified\n", si.iprt );
//      end_one_particle( si.gs, si.iprt );
//      return INVALID_PRT;
//    }
//
//    // Start on top of target
//    if ( ACTIVE_CHR( chrlst,  prt_target ) && ppip->startontarget )
//    {
//      pos.x = chrlst[prt_target].ori.pos.x;
//      pos.y = chrlst[prt_target].ori.pos.y;
//    }
//  }
//  else
//  {
//    // Correct facing for randomness
//    offsetfacing += generate_dither( &loc_rand, &ppip->facing, INT_TO_FP8( 1 ) );
//  }
//  facing += ppip->facing.ibase + offsetfacing;
//  pprt->facing = facing;
//  facing >>= 2;
//
//  // Location data from arguments
//  newrand = generate_unsigned( &loc_rand, &ppip->xyspacing );
//  pos.x += turntocos[( si.facing+8192 ) & TRIGTABLE_MASK] * newrand;
//  pos.y += turntosin[( si.facing+8192 ) & TRIGTABLE_MASK] * newrand;
//
//  pos.x = mesh_clip_x( &(si.gs->Mesh.Info), si.pos.x );
//  pos.y = mesh_clip_y( &(si.gs->Mesh.Info), si.pos.y );
//
//  pprt->ori.pos = pos;
//
//  // Velocity data
//  vel.x += turntocos[( si.facing+8192 ) & TRIGTABLE_MASK] * velocity;
//  vel.y += turntosin[( si.facing+8192 ) & TRIGTABLE_MASK] * velocity;
//  vel.z += generate_signed( &loc_rand, &ppip->zvel );
//  pprt->ori.vel = vel;
//
//  pprt->ori_old.pos.x = pprt->ori.pos.x - pprt->ori.vel.x;
//  pprt->ori_old.pos.y = pprt->ori.pos.y - pprt->ori.vel.y;
//  pprt->ori_old.pos.z = pprt->ori.pos.z - pprt->ori.vel.z;
//
//  // Template values
//  pprt->bumpsize     = ppip->bumpsize;
//  pprt->bumpsizebig  = pprt->bumpsize + ( pprt->bumpsize >> 1 );
//  pprt->bumpheight   = ppip->bumpheight;
//  pprt->bumpstrength = ppip->bumpstrength * si.intensity;
//
//  // figure out the particle type and transparency
//  pprt->type = ppip->type;
//  pprt->alpha_fp8 = 255;
//  switch ( ppip->type )
//  {
//    case PRTTYPE_SOLID:
//      if ( si.intensity < 1.0f )
//      {
//        pprt->type      = PRTTYPE_ALPHA;
//        pprt->alpha_fp8 = FLOAT_TO_FP8(si.intensity);
//      }
//      break;
//
//    case PRTTYPE_LIGHT:
//      pprt->alpha_fp8 = FLOAT_TO_FP8(si.intensity);
//      break;
//
//    case PRTTYPE_ALPHA:
//      pprt->alpha_fp8 = particletrans_fp8 * si.intensity;
//      break;
//  };
//
//
//
//  // Image data
//  pprt->rotate = generate_unsigned( &loc_rand, &ppip->rotate );
//  pprt->rotateadd = ppip->rotateadd;
//  pprt->size_fp8 = ppip->sizebase_fp8;
//  pprt->sizeadd_fp8 = ppip->sizeadd;
//  pprt->imageadd_fp8 = generate_unsigned( &loc_rand, &ppip->imageadd );
//  pprt->imagestt_fp8 = INT_TO_FP8( ppip->imagebase );
//  pprt->imagemax_fp8 = INT_TO_FP8( ppip->numframes );
//  pprt->time = ppip->time;
//  if ( ppip->endlastframe )
//  {
//    if ( pprt->imageadd_fp8 != 0 ) pprt->time = 0.0;
//  }
//
//
//  // Set onwhichfan...
//  pprt->onwhichfan = mesh_get_fan( Game_getMesh(si.gs), pprt->ori.pos );
//
//
//  // Damage stuff
//  pprt->damage.ibase = ppip->damage_fp8.ibase * si.intensity;
//  pprt->damage.irand = ppip->damage_fp8.irand * si.intensity;
//
//
//  // Spawning data
//  pprt->spawntime = ppip->contspawntime;
//  if ( pprt->spawntime != 0 )
//  {
//    CHR_REF prt_attachedto = prt_get_attachedtochr( si.gs, si.iprt );
//
//    pprt->spawntime = 1;
//    if ( ACTIVE_CHR( chrlst,  prt_attachedto ) )
//    {
//      pprt->spawntime++; // Because attachment takes an update before it happens
//    }
//  }
//
//  // Sound effect
//  snd_play_particle_sound( si.gs, si.intensity, si.iprt, ppip->soundspawn );
//
//  return si.iprt;
//}

//--------------------------------------------------------------------------------------------
Uint32 prt_hitawall( Game_t * gs, PRT_REF particle, vect3 * norm )
{
  /// @details ZZ@> This function returns nonzero if the particle hit a wall

  PPrt_t prtlst      = gs->PrtList;
  PPip_t piplst      = gs->PipList;

  Mesh_t * pmesh = Game_getMesh(gs);

  Uint32 retval, collision_bits;

  if ( !ACTIVE_PRT( prtlst,  particle ) ) return 0;

  collision_bits = MPDFX_IMPASS;
  if ( 0 != piplst[prtlst[particle].pip].bumpmoney )
  {
    collision_bits |= MPDFX_WALL;
  }

  retval = mesh_hitawall( pmesh, prtlst[particle].ori.pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL );


  if( 0!=retval && NULL !=norm )
  {
    vect3 pos;

    VectorClear( norm->v );

    pos.x = prtlst[particle].ori.pos.x;
    pos.y = prtlst[particle].ori_old.pos.y;
    pos.z = prtlst[particle].ori_old.pos.z;

    if( 0!=mesh_hitawall( pmesh, pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL ) )
    {
      norm->x = SGN(prtlst[particle].ori.pos.x - prtlst[particle].ori_old.pos.x);
    }

    pos.x = prtlst[particle].ori_old.pos.x;
    pos.y = prtlst[particle].ori.pos.y;
    pos.z = prtlst[particle].ori_old.pos.z;

    if( 0!=mesh_hitawall( pmesh, pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL ) )
    {
      norm->y = SGN(prtlst[particle].ori.pos.y - prtlst[particle].ori_old.pos.y);
    }

    //pos.x = prtlst[particle].ori_old.pos.x;
    //pos.y = prtlst[particle].ori_old.pos.y;
    //pos.z = prtlst[particle].ori.pos.z;

    //if( 0!=mesh_hitawall( pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL ) )
    //{
    //  norm->z = SGN(prtlst[particle].ori.pos.z - prtlst[particle].ori_old.pos.z);
    //}

    *norm = Normalize( *norm );
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( Game_t * gs, CHR_REF character )
{
  /// @details ZZ@> This function makes sure a character has no attached particles

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  PChr_t chrlst      = gs->ChrList;

  PRT_REF particle;
  bool_t useful = bfalse;

  if ( !ACTIVE_CHR( chrlst,  character ) ) return;

  for ( particle = 0; particle < prtlst_size; particle++ )
  {
    if ( !ACTIVE_PRT( prtlst,  particle ) ) continue;

    if ( prt_get_attachedtochr( gs, particle ) == character )
    {
      prtlst[particle].gopoof        = btrue;
      prtlst[particle].attachedtochr = INVALID_CHR;
      useful = btrue;
    }
  }

  // Set the alert for disaffirmation ( wet torch )
  if ( useful )
  {
    chrlst[character].aistate.alert |= ALERT_DISAFFIRMED;
  };
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( Game_t * gs, CHR_REF character )
{
  /// @details ZZ@> This function returns the number of particles attached to the given character

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  int     cnt;
  PRT_REF particle;

  cnt = 0;
  for ( particle = 0; particle < prtlst_size; particle++ )
  {
    if ( ACTIVE_PRT( prtlst,  particle ) && prt_get_attachedtochr( gs, particle ) == character )
    {
      cnt++;
    }
  }

  return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles( Game_t * gs, CHR_REF character )
{
  /// @details ZZ@> This function makes sure a character has all of it's particles

  PPrt_t prtlst      = gs->PrtList;
  Chr_t  * pchr      = gs->ChrList + character;

  Uint16 numberattached;
  PRT_REF particle;

  numberattached = number_of_attached_particles( gs, character );
  while ( numberattached < ChrList_getPCap(gs, character)->attachedprtamount )
  {
    particle = prt_spawn( gs, 1.0f, pchr->ori.pos, pchr->ori.vel, 0, pchr->model, ChrList_getPCap(gs, character)->attachedprttype, character, (GRIP)(GRIP_LAST + numberattached), pchr->team, character, numberattached, INVALID_CHR );
    if ( RESERVED_PRT(prtlst, particle) )
    {
      attach_particle_to_character( gs, particle, character, prtlst[particle].vertoffset );
    }
    numberattached++;
  }

  // Set the alert for reaffirmation ( for exploding barrels with fire )
  pchr->aistate.alert |= ALERT_REAFFIRMED;
}

struct sPrtEnvironment
{
  egoboo_key_t ekey;

  bool_t  inwater;
  bool_t  attached;
  bool_t  flying;

  float   level;
  vect3   nrm;
  float   buoyancy;
  float   homingfriction;
  float   homingaccel;
  float   lerp_z;

  PhysicsData_t phys;
};

typedef struct sPrtEnvironment PrtEnviro_t;

PrtEnviro_t * PrtEnviro_new( PrtEnviro_t * penviro, PhysicsData_t * gphys);
bool_t        PrtEnviro_delete( PrtEnviro_t * penviro );
PrtEnviro_t * PrtEnviro_renew( PrtEnviro_t * penviro, PhysicsData_t * gphys);
bool_t        PrtEnviro_init( PrtEnviro_t * penviro, float dUpdate);
bool_t        PrtEnviro_synchronize( PrtEnviro_t * penviro, Pip_t * ppip, float dUpdate );

//--------------------------------------------------------------------------------------------
bool_t prt_do_animation(Game_t * gs, float dUpdate, PRT_REF iprt, PrtEnviro_t * penviro, Uint32 * prand)
{ 
  // Animate particle

  PPrt_t  prtlst;
  Prt_t * pprt;

  PPip_t  piplst;
  PIP_REF ipip;
  Pip_t * ppip;

  if(NULL == gs || INVALID_PRT == iprt) return bfalse;

  prtlst      = gs->PrtList;
  piplst      = gs->PipList;

  pprt = prtlst + iprt;
  ipip = pprt->pip;
  ppip = piplst + ipip;


  pprt->image_fp8 += pprt->imageadd_fp8 * dUpdate;
  if (pprt->image_fp8 >= pprt->imagemax_fp8)
  {
    if ( ppip->endlastframe )
    {
      pprt->image_fp8 =pprt->imagemax_fp8 - INT_TO_FP8( 1 );
      pprt->gopoof    = btrue;
    }
    else if (pprt->imagemax_fp8 > 0 )
    {
      // if the prtlst[].image_fp8 is a fraction of an image over prtlst[].imagemax_fp8,
      // keep the fraction
      pprt->image_fp8 %= pprt->imagemax_fp8;
    }
    else
    {
      // a strange case
      pprt->image_fp8 = 0;
    }
  };

  pprt->facing += ppip->facingadd * dUpdate;
  pprt->rotate += pprt->rotateadd * dUpdate;
  pprt->size_fp8 = (pprt->size_fp8 + pprt->sizeadd_fp8 < 0 ) ? 0 : pprt->size_fp8 + pprt->sizeadd_fp8;

  // Change dyna light values
  pprt->dyna.level   += ppip->dyna.leveladd   * dUpdate;
  pprt->dyna.falloff += ppip->dyna.falloffadd * dUpdate;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_do_environment( Game_t * gs, Prt_t * pprt, PrtEnviro_t * penviro)
{
  Graphics_Data_t * gfx = Game_getGfx(gs);
  Mesh_t * pmesh = &(gs->Mesh);
  Uint32 fan = INVALID_FAN;
  Pip_t * ppip;
  float level, radius, lerp;

  ppip = gs->PipList + pprt->pip;

  fan = pprt->onwhichfan;
  level = ( INVALID_FAN == fan ) ? 0 : mesh_get_level( &(pmesh->Mem), fan, pprt->ori.pos.x, pprt->ori.pos.y, bfalse, &(gfx->Water) );

  // calculate the radius based off the values used in graphic_prt.c
  switch( pprt->type )
  {
    case PRTTYPE_LIGHT:
      radius = FP8_TO_FLOAT(pprt->size_fp8) * 0.5f;
      break;

    default:
    case PRTTYPE_SOLID:
    case PRTTYPE_ALPHA:
      radius = FP8_TO_FLOAT(pprt->size_fp8) * 0.25f;
      break;
  }

  // adjust the image of the particle so that it is centered when in flight, but
  // will rest rest on the floor
  lerp = (pprt->ori.pos.z - level) / radius;
  lerp = 1.0f - CLIP(lerp, 0.0f, 1.0f);
  pprt->shift.x = pprt->shift.y = 0.0f;
  pprt->shift.z = radius * lerp;

  // find the distance above the surface
  penviro->lerp_z = (pprt->ori.pos.z - (level + PLATTOLERANCE + radius)) / ( float ) (PLATTOLERANCE + radius);
  penviro->lerp_z = CLIP(penviro->lerp_z, -1.0f, 1.0f);
  
  // Check underwater
  penviro->inwater = bfalse;
  if (pprt->ori.pos.z < gfx->Water.douselevel && mesh_has_some_bits( pmesh->Mem.tilelst,pprt->onwhichfan, MPDFX_WATER ) && ppip->endwater )
  {
    penviro->inwater = btrue;
  }

  // get attachment
  penviro->attached = VALID_CHR(gs->ChrList, pprt->attachedtochr);

  // is it flying on it's own?
  penviro->flying = !penviro->attached && ppip->homing && VALID_CHR(gs->ChrList, pprt->target);

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t prt_do_movement(Game_t * gs, PRT_REF iprt, PrtEnviro_t * penviro, float dUpdate)
{
  // Do volontary movement

  CHR_REF prt_target;

  Prt_t * pprt;
  Pip_t * ppip;

  PPrt_t  prtlst = gs->PrtList;
  PChr_t  chrlst = gs->ChrList;

  Chr_t * ptarget = NULL;

  pprt = PrtList_getPPrt(gs, iprt);
  if(NULL == pprt) return bfalse;

  ppip = PrtList_getPPip(gs, iprt);
  if(NULL == ppip) return bfalse;

  // Do homing
  prt_target = prt_get_target(gs, iprt);
  if ( ppip->homing && ACTIVE_CHR( chrlst,  prt_target ) )
  {
    ptarget = chrlst + prt_target;

    if ( !ptarget->alive )
    {
      pprt->gopoof = btrue;
    }
    else if ( !penviro->attached )
    {
      pprt->accum.acc.x += -(1.0f-penviro->homingfriction) * pprt->ori.vel.x;
      pprt->accum.acc.y += -(1.0f-penviro->homingfriction) * pprt->ori.vel.y;
      pprt->accum.acc.z += -(1.0f-penviro->homingfriction) * pprt->ori.vel.z;

      pprt->accum.acc.x += ( ptarget->ori.pos.x - pprt->ori.pos.x ) * penviro->homingaccel * 4.0f;
      pprt->accum.acc.y += ( ptarget->ori.pos.y - pprt->ori.pos.y ) * penviro->homingaccel * 4.0f;
      pprt->accum.acc.z += ( ptarget->ori.pos.z + ( ptarget->bmpdata.calc_height * 0.5f ) - pprt->ori.pos.z ) * penviro->homingaccel * 4.0f;
    }
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t prt_do_physics(Game_t * gs, Prt_t * pprt, Pip_t * ppip, PrtEnviro_t * penviro, float dUpdate)
{
  PhysAccum_t * paccum = &(pprt->accum);
  Mesh_t      * pmesh  = Game_getMesh(gs);

  if(!EKEY_PVALID(gs) || !EKEY_PVALID(pprt)  || !EKEY_PVALID(ppip) || !EKEY_PVALID(penviro)) return bfalse;

  if(penviro->attached)
  {
    Chr_t * pchr = ChrList_getPChr(gs, pprt->attachedtochr);

    // attached particles apply their corrections to whatever they are attached to
    if(NULL != pchr)
    {
      paccum = &(pchr->accum);
    }
  }


  if ( penviro->flying )
  {
    // we are flying,swo our target must be valid
    Chr_t * ptarget = gs->ChrList + pprt->target;
    float flyheight = ptarget->ori.pos.z - ptarget->level;

    if ( penviro->level < 0 ) paccum->pos.z += penviro->level - pprt->ori.pos.z; // Don't fall in pits...
    paccum->acc.z += ( penviro->level + flyheight - pprt->ori.pos.z ) * FLYDAMPEN;
  }
  else
  {
    /// @todo separate physics for various particle types.
    if(PRTTYPE_SOLID == pprt->type)
    {
      // Integrate the z direction
      if ( pprt->ori.pos.z > penviro->level + PLATTOLERANCE )
      {
        paccum->acc.z += gs->phys.gravity;
      }
      else
      {
        /// @todo is fome kind of smoke follows the ground it needs to slide downhill, too

        float lerp_normal, lerp_tang;

        // set the the lerp for the friction effects
        lerp_tang = 1.0f - penviro->lerp_z;
        lerp_tang = CLIP(lerp_tang, 0.0f, 1.0f);

        // set the lerp for vertical effects
        //particles will hit the ground softly, but in a reasonable time
        lerp_normal = 1.0f - lerp_tang;
        lerp_normal = CLIP(lerp_normal, 0.2f, 1.0f);

        // slippy hills make particles slide
        if ( pprt->weight > 0 && gs->GfxData.Water.iswater && !penviro->inwater && INVALID_FAN != pprt->onwhichfan && mesh_has_some_bits( pmesh->Mem.tilelst, pprt->onwhichfan, MPDFX_SLIPPY ) )
        {
          paccum->acc.x -= penviro->nrm.x * gs->phys.gravity * lerp_tang * gs->phys.hillslide;
          paccum->acc.y -= penviro->nrm.y * gs->phys.gravity * lerp_tang * gs->phys.hillslide;
          paccum->acc.z += penviro->nrm.z * gs->phys.gravity * lerp_normal;
        }
        else
        {
          paccum->acc.z += gs->phys.gravity * lerp_normal;
        };
      }
    }
  }

  // Do speed limit on Z
  // this is related to buoyancy and air resistance. 
  // Maybe we could pull these two effects apart?
  if ( !penviro->flying && pprt->ori.vel.z < -ppip->spdlimit ) 
  {
    paccum->vel.z += -ppip->spdlimit - pprt->ori.vel.z;
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
void prt_do_spawning(Game_t * gs, float dUpdate, Prt_t * pprt, Pip_t * ppip, PrtEnviro_t * penviro, Uint32 * prand )
{
  int tnc;
  Uint16 facing;
  PRT_REF iprt;

  PPrt_t prtlst = gs->PrtList;
  PChr_t chrlst = gs->ChrList;
  Graphics_Data_t * gfx = Game_getGfx( gs );


  if(penviro->inwater)
  {
    vect3 prt_pos = VECT3(pprt->ori.pos.x, pprt->ori.pos.y, gfx->Water.surfacelevel);
    vect3 prt_vel = ZERO_VECT3;

    // Splash for particles is just a ripple
    iprt = prt_spawn( gs, 1.0f, pprt->ori.pos, pprt->ori.vel, 0, INVALID_OBJ, PIP_REF(PRTPIP_RIPPLE), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, 0, INVALID_CHR );

    // Check for disaffirming character
    if ( ACTIVE_CHR( chrlst,  pprt->attachedtochr ) && prt_get_owner( gs, iprt ) == pprt->attachedtochr )
    {
      // Disaffirm the whole character
      disaffirm_attached_particles( gs, pprt->attachedtochr );
    }
    else
    {
      pprt->gopoof = btrue;
    }
  }


  // Spawn new particles if continually spawning
  if ( ppip->contspawnamount > 0.0f )
  {
    pprt->spawntime -= dUpdate;
    if (pprt->spawntime <= 0.0f )
    {
      pprt->spawntime = ppip->contspawntime;
      facing = pprt->facing;

      for ( tnc = 0; tnc < ppip->contspawnamount; tnc++ )
      {
        iprt = prt_spawn( gs, 1.0f, pprt->ori.pos, pprt->ori.vel,
          facing, pprt->model, ppip->contspawnpip,
          INVALID_CHR, GRIP_LAST, pprt->team, prt_get_owner( gs, iprt ),
          tnc, pprt->target );

        facing += ppip->contspawnfacingadd;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void prt_do_timers(Prt_t * pprt, float dUpdate)
{

  // Down the particle timer
  if (pprt->time > 0.0f )
  {
    pprt->time -= dUpdate;
    if (pprt->time <= 0.0f ) pprt->gopoof = btrue;
  };

}

//--------------------------------------------------------------------------------------------
bool_t move_particle( Game_t * gs, PRT_REF iprt, float dUpdate, PrtEnviro_t * penviro, Uint32 * prand )
{
  Prt_t * pprt;
  Pip_t * ppip;

  pprt = PrtList_getPPrt(gs, iprt);
  if(NULL == pprt) return bfalse;

  ppip = PrtList_getPPip(gs, iprt);
  if(NULL == ppip) return bfalse;

  // update the pip-dependent environment values
  PrtEnviro_synchronize( penviro, ppip, dUpdate );

  // detect the environment of this particle
  prt_do_environment(gs, pprt, penviro);
  pprt->level = penviro->level;

  // do volountary particle movement (homing, etc.)
  prt_do_movement(gs, iprt, penviro, dUpdate);

  // do the particle physics
  prt_do_physics(gs, pprt, ppip, penviro, dUpdate);

  // do the animation
  prt_do_animation(gs, dUpdate, iprt, penviro, prand);

  // spawn all new particles
  prt_do_spawning(gs, dUpdate, pprt, ppip, penviro, prand);

  // update the particle timers
  prt_do_timers(pprt, dUpdate); 

  return btrue;
};

//--------------------------------------------------------------------------------------------
void move_all_particles( Game_t * gs, float dUpdate )
{
  /// @details ZZ@> This is the particle physics function

  Uint32 loc_rand = gs->randie_index;
  PRT_REF iprt;
  Prt_t * pprt;
  PrtEnviro_t enviro, enviro_save;

  PPrt_t prtlst = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  PrtEnviro_new(&enviro_save, &(gs->phys));
  PrtEnviro_init(&enviro_save, dUpdate);

  for ( iprt = 0; iprt < prtlst_size; iprt++ )
  {
    if ( !ACTIVE_PRT( prtlst,  iprt ) ) continue;
    pprt = prtlst + iprt;

    memcpy(&enviro, &enviro_save, sizeof(PrtEnviro_t));

    move_particle( gs, iprt, dUpdate, &enviro, &loc_rand );
  }

}

//--------------------------------------------------------------------------------------------
void attach_particles(Game_t * gs)
{
  /// @details ZZ@> This function attaches particles to their characters so everything gets
  ///     drawn right

  PRT_REF prt_cnt;

  PPrt_t prtlst      = gs->PrtList;
  size_t prtlst_size = PRTLST_COUNT;

  PPip_t piplst      = gs->PipList;
  PChr_t chrlst      = gs->ChrList;

  for ( prt_cnt = 0; prt_cnt < prtlst_size; prt_cnt++ )
  {
    CHR_REF prt_attachedto;

    if ( !ACTIVE_PRT( prtlst,  prt_cnt ) ) continue;

    prt_attachedto = prt_get_attachedtochr( gs, prt_cnt );

    if ( !ACTIVE_CHR( chrlst,  prt_attachedto ) ) continue;

    attach_particle_to_character( gs, prt_cnt, prt_attachedto, prtlst[prt_cnt].vertoffset );

    // Correct facing so swords knock characters in the right direction...
    if ( HAS_SOME_BITS( piplst[prtlst[prt_cnt].pip].damfx, DAMFX_TURN ) )
    {
      prtlst[prt_cnt].facing = chrlst[prt_attachedto].ori.turn_lr;
    }
  }
}


//--------------------------------------------------------------------------------------------
bool_t _prt_is_over_water( Game_t * gs, PRT_REF prt_cnt )
{
  // This function returns btrue if the particle is over a water tile

  Uint32 fan;
  Mesh_t * pmesh = Game_getMesh(gs);

  if ( ACTIVE_PRT(gs->PrtList, prt_cnt) )
  {
    fan = mesh_get_fan( pmesh, gs->PrtList[prt_cnt].ori.pos );
    if ( mesh_has_some_bits( pmesh->Mem.tilelst, fan, MPDFX_WATER ) )  return ( INVALID_FAN != fan );
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn( Game_t * gs, float dUpdate )
{
  /// @details ZZ@> This function drops snowflakes or rain or whatever, also swings the camera

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PRT_REF particle;
  PRT_REF cnt;
  bool_t foundone = bfalse;

  // Camera swingin' in the wind
  GCamera.swing = ( GCamera.swing + GCamera.swingrate ) & TRIGTABLE_MASK;

  if( !gs->Weather.active ) return;

  gs->Weather.time -= dUpdate / 10.0f;

  if ( gs->Weather.time <= 0 )
  {
    gs->Weather.time = gs->Weather.timereset;

    // Find a valid player
    foundone = bfalse;

    for ( cnt = 0; cnt < PLALST_COUNT; cnt++ )
    {
      gs->Weather.player = PLA_REF( ( REF_TO_INT(gs->Weather.player) + 1 ) % PLALST_COUNT );
      if ( VALID_PLA( gs->PlaList, gs->Weather.player ) )
      {
        foundone = btrue;
        break;
      }
    }
  }

  // Did we find one?
  if ( foundone )
  {
    vect3 prt_vel = ZERO_VECT3;

    // Yes, but is the character valid?
    CHR_REF chr_cnt = PlaList_getRChr( gs, gs->Weather.player );
    if ( ACTIVE_CHR( chrlst, chr_cnt ) && !chr_in_pack( chrlst, chrlst_size, chr_cnt ) )
    {
      // are the water requirements met?
      if( !gs->Weather.require_water || chr_is_over_water( gs, chr_cnt ) )
      {
        // Yes, so spawn over that character
        particle = prt_spawn( gs, 1.0f, chrlst[chr_cnt].ori.pos, prt_vel, 0, INVALID_OBJ, PIP_REF(PRTPIP_WEATHER_1), INVALID_CHR, GRIP_LAST, TEAM_REF(TEAM_NULL), INVALID_CHR, 0, INVALID_CHR );
      }
    }
  }

}


//--------------------------------------------------------------------------------------------
PIP_REF PipList_load_one( Game_t * gs, const char * szObjectpath, const char * szObjectname, const char * szFname, PIP_REF override)
{
  /// @details ZZ@> This function loads a particle template, returning PIPLST_COUNT if the file wasn't
  ///     found

  PPip_t piplst      = gs->PipList;

  EGO_CONST char * fname;
  FILE* fileread;
  IDSZ idsz;
  int iTmp;
  Pip_t * ppip;
  PIP_REF ipip = INVALID_PIP;

  fname = inherit_fname(szObjectpath, szObjectname, szFname);
  fileread = fs_fileOpen( PRI_NONE, NULL, fname, "r" );
  if ( NULL == fileread ) return ipip;

  if(INVALID_PIP != override)
  {
    ipip = override;
    gs->PipList_count = MAX(gs->PipList_count, REF_TO_INT(override)) + 1;
  }
  else
  {
    ipip = gs->PipList_count;
    gs->PipList_count++;
  };

  // Make sure we don't load over an existing model
  if( LOADED_PIP(piplst, ipip) )
  {
    log_error( "Particle template (pip) %i is already used. (%s%s)\n", ipip, szObjectpath, szObjectname );
  }

  // for "simpler" notation
  ppip = piplst + ipip;

  // initialize the model template
  Pip_new(ppip);

  // store some info for debugging
  strncpy( ppip->fname, fname,    sizeof( ppip->fname ) );
  globalname = ppip->fname;

  fgets( ppip->comment, sizeof( ppip->comment ), fileread );
  if ( ppip->comment[0] != '/' )  ppip->comment[0] = EOS;

  rewind( fileread );

  // General data
  ppip->force          = fget_next_bool( fileread );
  ppip->type           = fget_next_prttype( fileread );
  ppip->imagebase      = fget_next_int( fileread );
  ppip->numframes      = fget_next_int( fileread );
  ppip->imageadd.ibase = fget_next_int( fileread );
  ppip->imageadd.irand = fget_next_int( fileread );
  ppip->rotate.ibase   = fget_next_int( fileread );
  ppip->rotate.irand   = fget_next_int( fileread );
  ppip->rotateadd      = fget_next_int( fileread );
  ppip->sizebase_fp8   = fget_next_int( fileread );
  ppip->sizeadd        = fget_next_int( fileread );
  ppip->spdlimit       = fget_next_float( fileread );
  ppip->facingadd      = fget_next_int( fileread );

  // Ending conditions
  ppip->endwater     = fget_next_bool( fileread );
  ppip->endbump      = fget_next_bool( fileread );
  ppip->endground    = fget_next_bool( fileread );
  ppip->endlastframe = fget_next_bool( fileread );
  ppip->time         = fget_next_int ( fileread );

  // Collision data
  ppip->dampen       = fget_next_float( fileread );
  ppip->bumpmoney    = fget_next_int( fileread );
  ppip->bumpsize     = fget_next_int( fileread );
  ppip->bumpheight   = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &ppip->damage_fp8 );
  ppip->damagetype   = fget_next_damage( fileread );
  ppip->bumpstrength = 1.0f;
  if ( ppip->bumpsize == 0 )
  {
    ppip->bumpstrength = 0.0f;
    ppip->bumpsize     = 0.5f * FP8_TO_FLOAT( ppip->sizebase_fp8 );
    ppip->bumpheight   = 0.5f * FP8_TO_FLOAT( ppip->sizebase_fp8 );
  };

  // Lighting data
  ppip->dyna.mode    = fget_next_dynamode( fileread );
  ppip->dyna.level   = fget_next_float( fileread );
  ppip->dyna.falloff = fget_next_int( fileread );
  if ( ppip->dyna.falloff > MAXFALLOFF )  ppip->dyna.falloff = MAXFALLOFF;

  // Initial spawning of this particle
  ppip->facing.ibase    = fget_next_int( fileread );
  ppip->facing.irand    = fget_next_int( fileread );
  ppip->xyspacing.ibase = fget_next_int( fileread );
  ppip->xyspacing.irand = fget_next_int( fileread );
  ppip->zspacing.ibase  = fget_next_int( fileread );
  ppip->zspacing.irand  = fget_next_int( fileread );
  ppip->xyvel.ibase     = fget_next_int( fileread );
  ppip->xyvel.irand     = fget_next_int( fileread );
  ppip->zvel.ibase      = fget_next_int( fileread );
  ppip->zvel.irand      = fget_next_int( fileread );


  // Continuous spawning of other particles
  ppip->contspawntime = fget_next_int( fileread );
  ppip->contspawnamount = fget_next_int( fileread );
  ppip->contspawnfacingadd = fget_next_int( fileread );
  ppip->contspawnpip = fget_next_int( fileread );


  // End spawning of other particles
  ppip->endspawnamount = fget_next_int( fileread );
  ppip->endspawnfacingadd = fget_next_int( fileread );
  ppip->endspawnpip = fget_next_int( fileread );

  // Bump spawning of attached particles
  ppip->bumpspawnamount = fget_next_int( fileread );
  ppip->bumpspawnpip = fget_next_int( fileread );


  // Random stuff  !!!BAD!!! Not complete
  ppip->dazetime = fget_next_int( fileread );
  ppip->grogtime = fget_next_int( fileread );
  ppip->spawnenchant = fget_next_bool( fileread );
  ppip->causeknockback = fget_next_bool( fileread );
  ppip->causepancake = fget_next_bool( fileread );
  ppip->needtarget = fget_next_bool( fileread );
  ppip->targetcaster = fget_next_bool( fileread );
  ppip->startontarget = fget_next_bool( fileread );
  ppip->onlydamagefriendly = fget_next_bool( fileread );

  iTmp = fget_next_int( fileread ); ppip->soundspawn = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); ppip->soundend   = FIX_SOUND( iTmp );

  ppip->friendlyfire = fget_next_bool( fileread );
  ppip->hateonly = fget_next_bool( fileread );
  ppip->newtargetonspawn = fget_next_bool( fileread );
  ppip->targetangle = fget_next_int( fileread ) >> 1;
  ppip->homing = fget_next_bool( fileread );
  ppip->homingfriction = fget_next_float( fileread );
  ppip->homingaccel = fget_next_float( fileread );
  ppip->rotatetoface = fget_next_bool( fileread );
  fgoto_colon( fileread );   //BAD! Not used
  ppip->manadrain = fget_next_fixed( fileread );   //Mana drain (Mana damage)
  ppip->lifedrain = fget_next_fixed( fileread );   //Life drain (Life steal)

  // Clear expansions...
  ppip->zaimspd     = 0;
  ppip->soundfloor = INVALID_SOUND;
  ppip->soundwall  = INVALID_SOUND;
  ppip->endwall    = ppip->endground;
  ppip->damfx      = DAMFX_TURN;
  if ( ppip->homing )  ppip->damfx = DAMFX_NONE;
  ppip->allowpush = btrue;
  ppip->dyna.falloffadd = 0;
  ppip->dyna.leveladd = 0;
  ppip->intdamagebonus = bfalse;
  ppip->wisdamagebonus = bfalse;
  ppip->rotatewithattached = btrue;
  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );

    if ( MAKE_IDSZ( "TURN" ) == idsz ) ppip->rotatewithattached = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "ZSPD" ) == idsz )  ppip->zaimspd = iTmp;
    else if ( MAKE_IDSZ( "FSND" ) == idsz )  ppip->soundfloor = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WSND" ) == idsz )  ppip->soundwall = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WEND" ) == idsz )  ppip->endwall = INT_TO_BOOL(iTmp);
    else if ( MAKE_IDSZ( "ARMO" ) == idsz )  ppip->damfx |= DAMFX_ARMO;
    else if ( MAKE_IDSZ( "BLOC" ) == idsz )  ppip->damfx |= DAMFX_BLOC;
    else if ( MAKE_IDSZ( "ARRO" ) == idsz )  ppip->damfx |= DAMFX_ARRO;
    else if ( MAKE_IDSZ( "TIME" ) == idsz )  ppip->damfx |= DAMFX_TIME;
    else if ( MAKE_IDSZ( "PUSH" ) == idsz )  ppip->allowpush = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DLEV" ) == idsz )  ppip->dyna.leveladd = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "DRAD" ) == idsz )  ppip->dyna.falloffadd = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "IDAM" ) == idsz )  ppip->intdamagebonus = INT_TO_BOOL(iTmp);
    else if ( MAKE_IDSZ( "WDAM" ) == idsz )  ppip->wisdamagebonus = INT_TO_BOOL(iTmp);
  }

  fs_fileClose( fileread );

  ppip->Loaded = btrue;

  return ipip;
}

//--------------------------------------------------------------------------------------------
void PipList_load_global(Game_t * gs)
{
  /// @details ZF@> Load in the standard global particles ( the coins for example )
  ///     This should only be needed done once at the start of the game

  log_info("PipList_load_global() - \n\tLoading global particles into memory... ");
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING, CData.basicdat_dir, CData.globalparticles_dir );

  //Defend particle
  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.defend_file, PIP_REF(PRTPIP_DEFEND) ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 1
  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money1_file, PIP_REF(PRTPIP_COIN_001) ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 5
  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money5_file, PIP_REF(PRTPIP_COIN_005) ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 25
  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money25_file, PIP_REF(PRTPIP_COIN_025) ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 100
  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money100_file, PIP_REF(PRTPIP_COIN_100) ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Everything went fine
  log_message("Succeeded!\n");
}

//--------------------------------------------------------------------------------------------
void reset_particles( Game_t * gs, char* modname )
{
  /// @details ZZ@> This resets all particle data and reads in the coin and water particles

  // Load in the standard global particles ( the coins for example )
  //BAD! This should only be needed once at the start of the game, using PipList_load_global

  PipList_renew(gs);

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING, modname, CData.gamedat_dir);

  //Load module specific information
  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.weather1_file, PIP_REF(PRTPIP_WEATHER_1) ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.weather2_file, PIP_REF(PRTPIP_WEATHER_2) ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.splash_file, PIP_REF(PRTPIP_SPLASH) ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  if ( INVALID_PIP == PipList_load_one( gs, CStringTmp1, NULL, CData.ripple_file, PIP_REF(PRTPIP_RIPPLE) ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

}

//--------------------------------------------------------------------------------------------
bool_t prt_calculate_bumpers(Game_t * gs, PRT_REF iprt)
{
  PPrt_t prtlst      = gs->PrtList;

  float ftmp;

  if( !ACTIVE_PRT( prtlst, iprt) ) return bfalse;

  prtlst[iprt].bmpdata.mids_lo = prtlst[iprt].ori.pos;

  // calculate the particle radius
  ftmp = FP8_TO_FLOAT(prtlst[iprt].size_fp8) * 0.5f;

  // calculate the "perfect" bbox for a sphere
  prtlst[iprt].bmpdata.cv.x_min = prtlst[iprt].ori.pos.x - ftmp - 0.001f;
  prtlst[iprt].bmpdata.cv.x_max = prtlst[iprt].ori.pos.x + ftmp + 0.001f;

  prtlst[iprt].bmpdata.cv.y_min = prtlst[iprt].ori.pos.y - ftmp - 0.001f;
  prtlst[iprt].bmpdata.cv.y_max = prtlst[iprt].ori.pos.y + ftmp + 0.001f;

  prtlst[iprt].bmpdata.cv.z_min = prtlst[iprt].ori.pos.z - ftmp - 0.001f;
  prtlst[iprt].bmpdata.cv.z_max = prtlst[iprt].ori.pos.z + ftmp + 0.001f;

  prtlst[iprt].bmpdata.cv.xy_min = prtlst[iprt].ori.pos.x - ftmp * SQRT_TWO - 0.001f;
  prtlst[iprt].bmpdata.cv.xy_max = prtlst[iprt].ori.pos.x + ftmp * SQRT_TWO + 0.001f;

  prtlst[iprt].bmpdata.cv.yx_min = prtlst[iprt].ori.pos.y - ftmp * SQRT_TWO - 0.001f;
  prtlst[iprt].bmpdata.cv.yx_max = prtlst[iprt].ori.pos.y + ftmp * SQRT_TWO + 0.001f;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Prt_t * Prt_new(Prt_t *pprt)
{
  //fprintf( stdout, "Prt_new()\n");

  if(NULL ==pprt) return pprt;

  Prt_delete( pprt );

  memset(pprt, 0, sizeof(Prt_t));

  EKEY_PNEW( pprt, Prt_t );

  BData_new(&pprt->bmpdata);

  pprt->pip           = INVALID_PIP;
  pprt->model         = INVALID_OBJ;
  pprt->team          = TEAM_REF(TEAM_NULL);
  pprt->owner         = INVALID_CHR;
  pprt->target        = INVALID_CHR;
  pprt->damagetype    = DAMAGE_NULL;
  pprt->attachedtochr = INVALID_CHR;
  pprt->onwhichfan    = INVALID_FAN;

  pprt->spawncharacterstate = SPAWN_NOCHARACTER;

  pprt->vertoffset = 0;


  // dummy values
  pprt->bumpsize     = 64;
  pprt->bumpsizebig  = 90;
  pprt->bumpheight   = 128;
  pprt->bumpstrength = 1.0f;
  pprt->weight       = 1.0f;

  // assume solid particle
  pprt->type = PRTTYPE_SOLID;
  pprt->alpha_fp8 = 255;

  return pprt;
}

//--------------------------------------------------------------------------------------------
bool_t Prt_delete( Prt_t * pprt )
{
  if(NULL == pprt) return bfalse;

  if( !EKEY_PVALID(pprt) ) return btrue;

  EKEY_PINVALIDATE(pprt);

  return btrue;
}

//--------------------------------------------------------------------------------------------
Prt_t * Prt_renew(Prt_t *pprt)
{
  Prt_delete(pprt);
  return Prt_new(pprt);
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Pip_t * Pip_new(Pip_t * ppip)
{
  //fprintf( stdout, "Pip_new()\n");

  if(NULL == ppip) return ppip;

  Pip_delete(ppip);

  memset(ppip, 0, sizeof(Pip_t));

  EKEY_PNEW( ppip, Pip_t );

  ppip->soundspawn = INVALID_SOUND;                   // Beginning sound
  ppip->soundend   = INVALID_SOUND;                     // Ending sound
  ppip->soundfloor = INVALID_SOUND;                   // Floor sound
  ppip->soundwall  = INVALID_SOUND;                    // Ricochet sound

  return ppip;
}

//--------------------------------------------------------------------------------------------
bool_t Pip_delete(Pip_t * ppip)
{
  if(NULL == ppip) return bfalse;
  if(!EKEY_PVALID(ppip)) return btrue;

  ppip->Loaded = bfalse;
  ppip->fname[0]   = EOS;
  ppip->comment[0] = EOS;

  EKEY_PINVALIDATE(ppip);

  return btrue;
}
//--------------------------------------------------------------------------------------------
Pip_t * Pip_renew(Pip_t * ppip)
{
  Pip_delete(ppip);
  return Pip_new(ppip);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

OBJ_REF PrtList_getRObj(Game_t * gs, PRT_REF iprt)
{
  Prt_t * pprt;

  pprt = PrtList_getPPrt(gs, iprt);
  if(NULL == pprt) return INVALID_OBJ;

  pprt->model = VALIDATE_OBJ(gs->ObjList, pprt->model);
  return pprt->model;
}

//--------------------------------------------------------------------------------------------
PIP_REF PrtList_getRPip(Game_t * gs, PRT_REF iprt)
{
  Prt_t * pprt;

  pprt = PrtList_getPPrt(gs, iprt);
  if(NULL == pprt) return INVALID_PIP;

  pprt->pip = VALIDATE_PIP(gs->ObjList, pprt->pip);
  return pprt->pip;
}

//--------------------------------------------------------------------------------------------
CAP_REF PrtList_getRCap(Game_t * gs, PRT_REF iprt)
{
  OBJ_REF iobj;

  iobj = PrtList_getRObj(gs, iprt);
  if(INVALID_OBJ == iobj) return INVALID_CAP;

  return ObjList_getRCap(gs, iobj);
}

//--------------------------------------------------------------------------------------------
MAD_REF PrtList_getRMad(Game_t * gs, PRT_REF iprt)
{
  OBJ_REF iobj;

  iobj = PrtList_getRObj(gs, iprt);
  if(INVALID_OBJ == iobj) return INVALID_MAD;

  return ObjList_getRMad(gs, iobj);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Prt_t * PrtList_getPPrt(Game_t * gs, PRT_REF iprt)
{
  if(!VALID_PRT(gs->PrtList, iprt)) return NULL;

  return gs->PrtList + iprt;
}

//--------------------------------------------------------------------------------------------
Obj_t * PrtList_getPObj(Game_t * gs, PRT_REF iprt)
{
  OBJ_REF iobj;

  iobj = PrtList_getRObj(gs, iprt);
  if(INVALID_OBJ == iobj) return NULL;

  return ObjList_getPObj(gs, iobj);
}

//--------------------------------------------------------------------------------------------
Cap_t * PrtList_getPCap(Game_t * gs, PRT_REF iprt)
{
  OBJ_REF iobj;

  iobj = PrtList_getRObj(gs, iprt);
  if(INVALID_OBJ == iobj) return NULL;

  return ObjList_getPCap(gs, iobj);
}

//--------------------------------------------------------------------------------------------
Mad_t * PrtList_getPMad(Game_t * gs, PRT_REF iprt)
{
  OBJ_REF iobj;

  iobj = PrtList_getRObj(gs, iprt);
  if(INVALID_OBJ == iobj) return NULL;

  return ObjList_getPMad(gs, iobj);
}

//--------------------------------------------------------------------------------------------
Pip_t * PrtList_getPPip(Game_t * gs, PRT_REF iprt)
{
  Prt_t * pprt;

  pprt = PrtList_getPPrt(gs, iprt);
  if(NULL == pprt) return NULL;

  pprt->pip = VALIDATE_PIP(gs->PipList, pprt->pip);
  if(INVALID_PIP == pprt->pip) return NULL;

  return gs->PipList + pprt->pip;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PRT_SPAWN_INFO * prt_spawn_info_new(PRT_SPAWN_INFO * psi, Game_t * gs)
{
  if( NULL == psi ) return psi;

  prt_spawn_info_delete( psi );

  memset(psi, 0, sizeof(PRT_SPAWN_INFO));

  EKEY_PNEW(psi, PRT_SPAWN_INFO);

  psi->gs = gs;
  psi->seed = (NULL==gs) ? time(NULL) : gs->randie_index;

  psi->iobj = INVALID_OBJ;
  psi->ipip = INVALID_PIP;
  psi->iprt = INVALID_PRT;

  psi->intensity = 1.0f;
  psi->characterattach = INVALID_CHR;
  psi->team = TEAM_REF(TEAM_NULL);
  psi->characterorigin = INVALID_CHR;
  psi->oldtarget = INVALID_CHR;

  return psi;
}

//--------------------------------------------------------------------------------------------
bool_t prt_spawn_info_delete(PRT_SPAWN_INFO * psi)
{
  if(NULL == psi) return bfalse;
  if(!EKEY_PVALID(psi))  return btrue;

  EKEY_PINVALIDATE(psi);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PrtHeap_t * PrtHeap_new   ( PrtHeap_t * pheap )
{
  if( EKEY_PVALID(pheap) )
  {
    PrtHeap_delete( pheap );
  }

  memset(pheap, 0, sizeof(PrtHeap_t));

  EKEY_PNEW( pheap, PrtHeap_t );

  PrtHeap_reset( pheap );

  return pheap;
}

//--------------------------------------------------------------------------------------------
bool_t PrtHeap_delete( PrtHeap_t * pheap )
{
  if( !EKEY_PVALID(pheap) ) return btrue;

  EKEY_PINVALIDATE( pheap );

  pheap->free_count = 0;
  pheap->used_count = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
PrtHeap_t * PrtHeap_renew ( PrtHeap_t * pheap )
{
  if( !PrtHeap_delete( pheap ) ) return pheap;

  return PrtHeap_new( pheap );
}

//--------------------------------------------------------------------------------------------
bool_t PrtHeap_reset ( PrtHeap_t * pheap )
{
  int i;

  if( !EKEY_PVALID( pheap ) ) return bfalse;

  PROFILE_BEGIN( PrtHeap );

  for( i=0; i<PRTLST_COUNT; i++)
  {
    pheap->free_list[i] = i;
    pheap->used_list[i] = INVALID_PRT;
  };
  pheap->free_count = PRTLST_COUNT;
  pheap->used_count = 0;

  PROFILE_END2( PrtHeap );

  return btrue;
}

//--------------------------------------------------------------------------------------------
PRT_REF PrtHeap_getFree( PrtHeap_t * pheap, PRT_REF request )
{
  int i;
  PRT_REF ret = INVALID_PRT;

  if( !EKEY_PVALID(pheap) ) return ret;
  if( pheap->free_count <= 0 ) return ret;

  PROFILE_BEGIN( PrtHeap );

  if(INVALID_PRT == request)
  {
    pheap->free_count--;
    ret = pheap->free_list[pheap->free_count];
    pheap->free_list[pheap->free_count] = INVALID_PRT;
  }
  else
  {
    for(i = 0; i<pheap->free_count; i++)
    {
      if( pheap->free_list[i] == request ) break;
    };

    if(i != pheap->free_count)
    {
      ret = i;
      pheap->free_count--;
      pheap->free_list[i] = pheap->free_list[pheap->free_count];
      pheap->free_list[pheap->free_count] = INVALID_PRT;
    };
  };

  { PROFILE_END2( PrtHeap ); return ret; }
}

//--------------------------------------------------------------------------------------------
PRT_REF PrtHeap_iterateUsed( PrtHeap_t * pheap, int * index )
{
  PRT_REF ret = INVALID_PRT;

  if( !EKEY_PVALID(pheap) ) return ret;
  if( NULL == index || *index >= pheap->used_count ) return ret;

  PROFILE_BEGIN( PrtHeap );

  (*index)++;
  ret = pheap->used_list[*index];

  PROFILE_END2( PrtHeap );

  { PROFILE_END2( PrtHeap ); return ret; }
}

//--------------------------------------------------------------------------------------------
bool_t  PrtHeap_addUsed( PrtHeap_t * pheap, PRT_REF ref )
{
  int i;

  if( !EKEY_PVALID(pheap) ) return bfalse;
  if( pheap->used_count >= PRTLST_COUNT) return bfalse;

  PROFILE_BEGIN( PrtHeap );

  for(i=0; i<pheap->used_count; i++)
  {
    if(pheap->used_list[i] == ref) { PROFILE_END2( PrtHeap ); return btrue; }
  };

  pheap->used_list[pheap->used_count] = ref;
  pheap->used_count++;

  PROFILE_END2( PrtHeap );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t  PrtHeap_addFree( PrtHeap_t * pheap, PRT_REF ref )
{
  int i;

  if( !EKEY_PVALID(pheap) ) return bfalse;
  if( pheap->free_count >= PRTLST_COUNT) return bfalse;

  PROFILE_BEGIN( PrtHeap );

  for(i=0; i<pheap->free_count; i++)
  {
    if(pheap->free_list[i] == ref) { PROFILE_END2( PrtHeap ); return btrue; }
  };

  pheap->free_list[pheap->free_count] = ref;
  pheap->free_count++;

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PrtEnviro_t * PrtEnviro_new( PrtEnviro_t * penviro, PhysicsData_t * gphys)
{
  if(NULL == penviro) return penviro;

  PrtEnviro_delete(penviro);

  memset(penviro, 0, sizeof(PrtEnviro_t));

  EKEY_PNEW( penviro, PrtEnviro_t );

  if(NULL == gphys)
  {
    CPhysicsData_new( &(penviro->phys) );
  }
  else
  {
    memcpy( &(penviro->phys), gphys, sizeof(PhysicsData_t));
  };

  return penviro;
}

//--------------------------------------------------------------------------------------------
bool_t PrtEnviro_delete( PrtEnviro_t * enviro )
{
  if(NULL == enviro) return bfalse;

  if( !EKEY_PVALID(enviro) ) return btrue;

  EKEY_PINVALIDATE(enviro);

  return btrue;
}

//--------------------------------------------------------------------------------------------
PrtEnviro_t * PrtEnviro_renew( PrtEnviro_t * enviro, PhysicsData_t * gphys)
{
  PrtEnviro_delete(enviro);
  return PrtEnviro_new( enviro, gphys);
}

//--------------------------------------------------------------------------------------------
bool_t PrtEnviro_init( PrtEnviro_t * penviro, float dUpdate)
{
  if(NULL == penviro) return bfalse;


  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PrtEnviro_synchronize( PrtEnviro_t * penviro, Pip_t * ppip, float dUpdate )
{
  if(NULL == penviro || NULL == ppip) return bfalse;

  penviro->homingaccel    = ppip->homingaccel;
  penviro->homingfriction = ppip->homingfriction;
  if(penviro->homingfriction != 0.0f)
  {
    penviro->homingfriction = pow( ABS(penviro->homingfriction), dUpdate );
  }

  return btrue;
}

