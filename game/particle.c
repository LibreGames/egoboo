//********************************************************************************************
//* Egoboo - particle.c
//*
//* Manages the particle system.
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

#include "particle.inl"

#include "Log.h"
#include "camera.h"
#include "sound.h"
#include "enchant.h"
#include "game.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>

#include "char.inl"
#include "Md2.inl"
#include "mesh.inl"
#include "egoboo_math.inl"

DYNALIGHT_LIST GDynaLight[MAXDYNA];

DYNALIGHT_INFO GDyna;

Uint16 particletexture;                            // All in one bitmap

//--------------------------------------------------------------------------------------------
void make_prtlist(GameState * gs)
{
  // ZZ> This function figures out which particles are visible, and it sets up dynamic
  //     lighting

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  int cnt, tnc, disx, disy, distance, slot;

  // Don't really make a list, just set to visible or not
  GDyna.count = 0;
  GDyna.distancetobeat = MAXDYNADIST;
  for ( cnt = 0; cnt < prtlst_size; cnt++ )
  {
    prtlst[cnt].inview = bfalse;
    if ( !VALID_PRT( prtlst,  cnt ) ) continue;

    prtlst[cnt].inview = mesh_fan_is_in_renderlist( gs->Mesh_Mem.fanlst, prtlst[cnt].onwhichfan );
    // Set up the lights we need
    if ( prtlst[cnt].dyna.on )
    {
      disx = ABS( prtlst[cnt].pos.x - GCamera.trackpos.x );
      disy = ABS( prtlst[cnt].pos.y - GCamera.trackpos.y );
      distance = disx + disy;
      if ( distance < GDyna.distancetobeat )
      {
        if ( GDyna.count < MAXDYNA )
        {
          // Just add the light
          slot = GDyna.count;
          GDynaLight[slot].distance = distance;
          GDyna.count++;
        }
        else
        {
          // Overwrite the worst one
          slot = 0;
          GDyna.distancetobeat = GDynaLight[0].distance;
          for ( tnc = 1; tnc < MAXDYNA; tnc++ )
          {
            if ( GDynaLight[tnc].distance > GDyna.distancetobeat )
            {
              slot = tnc;
            }
          }
          GDynaLight[slot].distance = distance;

          // Find the new distance to beat
          GDyna.distancetobeat = GDynaLight[0].distance;
          for ( tnc = 1; tnc < MAXDYNA; tnc++ )
          {
            if ( GDynaLight[tnc].distance > GDyna.distancetobeat )
            {
              GDyna.distancetobeat = GDynaLight[tnc].distance;
            }
          }
        }
        GDynaLight[slot].pos.x = prtlst[cnt].pos.x;
        GDynaLight[slot].pos.y = prtlst[cnt].pos.y;
        GDynaLight[slot].pos.z = prtlst[cnt].pos.z;
        GDynaLight[slot].level = prtlst[cnt].dyna.level;
        GDynaLight[slot].falloff = prtlst[cnt].dyna.falloff;
      }
    }

  }
}

//--------------------------------------------------------------------------------------------
void PrtList_free_one_no_sound( GameState * gs, PRT_REF particle )
{
  // ZZ> This function sticks a particle back on the free particle stack

  gs->PrtFreeList[gs->PrtFreeList_count] = particle;
  gs->PrtFreeList_count++;
  gs->PrtList[particle].on = bfalse;
}

//--------------------------------------------------------------------------------------------
void PrtList_free_one( GameState * gs, PRT_REF particle )
{
  // ZZ> This function sticks a particle back on the free particle stack and
  //     plays the sound associated with the particle

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  int child;
  if ( prtlst[particle].spawncharacterstate != SPAWN_NOCHARACTER )
  {
    child = spawn_one_character( gs, prtlst[particle].pos, prtlst[particle].model, prtlst[particle].team, 0, prtlst[particle].facing, NULL, MAXCHR );
    if ( VALID_CHR( chrlst,  child ) )
    {
      chrlst[child].aistate.state = prtlst[particle].spawncharacterstate;
      chrlst[child].aistate.owner = prt_get_owner( gs, particle );
    }
  }
  snd_play_particle_sound( gs, 1.0f, particle, piplst[prtlst[particle].pip].soundend );

  PrtList_free_one_no_sound( gs, particle );
}

//--------------------------------------------------------------------------------------------
int PrtList_get_free( GameState * gs, int force )
{
  // ZZ> This function gets an unused particle.  If all particles are in use
  //     and force is set, it grabs the first unimportant one.  The particle
  //     index is the return value

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  PRT_REF particle;

  // Return MAXPRT if we can't find one
  particle = MAXPRT;
  if ( gs->PrtFreeList_count == 0 )
  {
    if ( force )
    {
      // Gotta find one, so go through the list
      particle = 0;
      while ( particle < MAXPRT )
      {
        if ( prtlst[particle].bumpsize == 0 )
        {
          // Found one
          return particle;
        }
        particle++;
      }
    }
  }
  else
  {
    if ( force || gs->PrtFreeList_count > ( MAXPRT / 4 ) )
    {
      // Just grab the next one
      gs->PrtFreeList_count--;
      particle = gs->PrtFreeList[gs->PrtFreeList_count];
    }
  }
  return particle;
}

//--------------------------------------------------------------------------------------------
PRT_REF spawn_one_particle( GameState * gs, float intensity, vect3 pos,
                           Uint16 facing, Uint16 model, Uint16 local_pip,
                           CHR_REF characterattach, GRIP grip, TEAM team,
                           CHR_REF characterorigin, Uint16 multispawn, CHR_REF oldtarget )
{
  // ZZ> This function spawns a new particle, and returns the number of that particle

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  int iprt, velocity;
  float xvel, yvel, zvel, tvel;
  int offsetfacing, newrand;
  Uint16 glob_pip = MAXPRTPIP;
  float weight;
  Uint16 prt_target;
  Prt * pprt;
  Pip * ppip;
  SearchInfo loc_search;

  if ( local_pip >= MAXPRTPIP )
  {
    log_debug( "WARN: spawn_one_particle() - failed to spawn : local_pip == %d is an invalid value\n", local_pip );
    return MAXPRT;
  }

  // Convert from local local_pip to global local_pip
  if ( model < MAXMODEL && local_pip < PRTPIP_PEROBJECT_COUNT )
  {
    glob_pip = madlst[model].prtpip[local_pip];
  }

  // assume we were given a global local_pip
  if ( MAXPRTPIP == glob_pip )
  {
    glob_pip = local_pip;
  }
  ppip = piplst + glob_pip;



  iprt = PrtList_get_free( gs, ppip->force );
  if ( MAXPRT == iprt )
  {
    log_debug( "WARN: spawn_one_particle() - failed to spawn : PrtList_get_free() returned invalid value %d\n", iprt );
    return MAXPRT;
  }

  weight = 1.0f;
  if ( VALID_CHR( chrlst,  characterorigin ) ) weight = MAX( weight, chrlst[characterorigin].weight );
  if ( VALID_CHR( chrlst,  characterattach ) ) weight = MAX( weight, chrlst[characterattach].weight );
  prtlst[iprt].weight = weight;

  log_debug( "spawn_one_particle() - local pip == %d, global pip == %d, part == %d\n", local_pip, glob_pip, iprt);

  // "simplify" the notation
  pprt = prtlst + iprt;

  // clear any old data
  memset(pprt, 0, sizeof(Prt));

  // Necessary data for any part
  pprt->on = btrue;
  pprt->gopoof = bfalse;
  pprt->pip = glob_pip;
  pprt->model = model;
  pprt->inview = bfalse;
  pprt->level = 0;
  pprt->team = team;
  pprt->owner = characterorigin;
  pprt->damagetype = ppip->damagetype;
  pprt->spawncharacterstate = SPAWN_NOCHARACTER;


  // Lighting and sound
  pprt->dyna.on = bfalse;
  if ( multispawn == 0 )
  {
    pprt->dyna.on = ( DYNA_OFF != ppip->dyna.mode );
    if ( ppip->dyna.mode == DYNA_LOCAL )
    {
      pprt->dyna.on = bfalse;
    }
  }
  pprt->dyna.level   = ppip->dyna.level * intensity;
  pprt->dyna.falloff = ppip->dyna.falloff;



  // Set character attachments ( characterattach==MAXCHR means none )
  pprt->attachedtochr = characterattach;
  pprt->vertoffset = grip;



  // Targeting...
  offsetfacing = 0;
  zvel = 0;
  pos.z += generate_signed( &ppip->zspacing );
  velocity = generate_unsigned( &ppip->xyvel );
  pprt->target = oldtarget;
  prt_target = MAXCHR;
  if ( ppip->newtargetonspawn )
  {
    if ( ppip->targetcaster )
    {
      // Set the target to the caster
      pprt->target = characterorigin;
    }
    else
    {
      // Correct facing for dexterity...
      if ( chrlst[characterorigin].dexterity_fp8 < PERFECTSTAT )
      {
        // Correct facing for randomness
        newrand = FP8_DIV( PERFECTSTAT - chrlst[characterorigin].dexterity_fp8,  PERFECTSTAT );
        offsetfacing += generate_dither( &ppip->facing, newrand );
      }

      // Find a target
      if( prt_search_wide( gs, SearchInfo_new(&loc_search), iprt, facing, ppip->targetangle, ppip->onlydamagefriendly, bfalse, team, characterorigin, oldtarget ) )
      {
        pprt->target = loc_search.besttarget;
      };

      prt_target = prt_get_target( gs, iprt );
      if ( VALID_CHR( chrlst,  prt_target ) )
      {
        offsetfacing -= loc_search.useangle;

        if ( ppip->zaimspd != 0 )
        {
          // These aren't velocities...  This is to do aiming on the Z axis
          if ( velocity > 0 )
          {
            xvel = chrlst[prt_target].pos.x - pos.x;
            yvel = chrlst[prt_target].pos.y - pos.y;
            tvel = sqrt( xvel * xvel + yvel * yvel ) / velocity;   // This is the number of steps...
            if ( tvel > 0 )
            {
              zvel = ( chrlst[prt_target].pos.z + ( chrlst[prt_target].bmpdata.calc_size * 0.5f ) - pos.z ) / tvel;  // This is the zvel alteration
              if ( zvel < - ( ppip->zaimspd >> 1 ) ) zvel = - ( ppip->zaimspd >> 1 );
              if ( zvel > ppip->zaimspd ) zvel = ppip->zaimspd;
            }
          }
        }
      }
    }

    // Does it go away?
    if ( !VALID_CHR( chrlst,  prt_target ) && ppip->needtarget )
    {
      log_debug( "WARN: spawn_one_particle() - failed to spawn : pip requires target and no target specified\n", iprt );
      PrtList_free_one( gs, iprt );
      return MAXPRT;
    }

    // Start on top of target
    if ( VALID_CHR( chrlst,  prt_target ) && ppip->startontarget )
    {
      pos.x = chrlst[prt_target].pos.x;
      pos.y = chrlst[prt_target].pos.y;
    }
  }
  else
  {
    // Correct facing for randomness
    offsetfacing += generate_dither( &ppip->facing, INT_TO_FP8( 1 ) );
  }
  facing += ppip->facing.ibase + offsetfacing;
  pprt->facing = facing;
  facing >>= 2;

  // Location data from arguments
  newrand = generate_unsigned( &ppip->xyspacing );
  pos.x += turntocos[( facing+8192 ) & TRIGTABLE_MASK] * newrand;
  pos.y += turntosin[( facing+8192 ) & TRIGTABLE_MASK] * newrand;

  pos.x = mesh_clip_x( &(gs->mesh), pos.x );
  pos.y = mesh_clip_x( &(gs->mesh), pos.y );

  pprt->pos.x = pos.x;
  pprt->pos.y = pos.y;
  pprt->pos.z = pos.z;


  // Velocity data
  xvel = turntocos[( facing+8192 ) & TRIGTABLE_MASK] * velocity;
  yvel = turntosin[( facing+8192 ) & TRIGTABLE_MASK] * velocity;
  zvel += generate_signed( &ppip->zvel );
  pprt->vel.x = xvel;
  pprt->vel.y = yvel;
  pprt->vel.z = zvel;

  pprt->pos_old.x = pprt->pos.x - pprt->vel.x;
  pprt->pos_old.y = pprt->pos.y - pprt->vel.y;
  pprt->pos_old.z = pprt->pos.z - pprt->vel.z;

  // Template values
  pprt->bumpsize = ppip->bumpsize;
  pprt->bumpsizebig = pprt->bumpsize + ( pprt->bumpsize >> 1 );
  pprt->bumpheight = ppip->bumpheight;
  pprt->bumpstrength = ppip->bumpstrength * intensity;

  // figure out the particle type and transparency
  pprt->type = ppip->type;
  pprt->alpha_fp8 = 255;
  switch ( ppip->type )
  {
    case PRTTYPE_SOLID:
      if ( intensity < 1.0f )
      {
        pprt->type  = PRTTYPE_ALPHA;
        pprt->alpha_fp8 = FLOAT_TO_FP8(intensity);
      }
      break;

    case PRTTYPE_LIGHT:
      pprt->alpha_fp8 = FLOAT_TO_FP8(intensity);
      break;

    case PRTTYPE_ALPHA:
      pprt->alpha_fp8 = particletrans * intensity;
      break;
  };



  // Image data
  pprt->rotate = generate_unsigned( &ppip->rotate );
  pprt->rotateadd = ppip->rotateadd;
  pprt->size_fp8 = ppip->sizebase_fp8;
  pprt->sizeadd_fp8 = ppip->sizeadd;
  pprt->image_fp8 = 0;
  pprt->imageadd_fp8 = generate_unsigned( &ppip->imageadd );
  pprt->imagestt_fp8 = INT_TO_FP8( ppip->imagebase );
  pprt->imagemax_fp8 = INT_TO_FP8( ppip->numframes );
  pprt->time = ppip->time;
  if ( ppip->endlastframe )
  {
    if ( pprt->imageadd_fp8 != 0 ) pprt->time = 0.0;
  }


  // Set onwhichfan...
  pprt->onwhichfan = mesh_get_fan( gs, pprt->pos );


  // Damage stuff
  pprt->damage.ibase = ppip->damage_fp8.ibase * intensity;
  pprt->damage.irand = ppip->damage_fp8.irand * intensity;


  // Spawning data
  pprt->spawntime = ppip->contspawntime;
  if ( pprt->spawntime != 0 )
  {
    CHR_REF prt_attachedto = prt_get_attachedtochr( gs, iprt );

    pprt->spawntime = 1;
    if ( VALID_CHR( chrlst,  prt_attachedto ) )
    {
      pprt->spawntime++; // Because attachment takes an update before it happens
    }
  }


  // Sound effect
  snd_play_particle_sound( gs, intensity, iprt, ppip->soundspawn );

  return iprt;
}

//--------------------------------------------------------------------------------------------
Uint32 prt_hitawall( GameState * gs, PRT_REF particle, vect3 * norm )
{
  // ZZ> This function returns nonzero if the particle hit a wall

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  Uint32 retval, collision_bits;

  if ( !VALID_PRT( prtlst,  particle ) ) return 0;

  collision_bits = MPDFX_IMPASS;
  if ( 0 != piplst[prtlst[particle].pip].bumpmoney )
  {
    collision_bits |= MPDFX_WALL;
  }

  retval = mesh_hitawall( gs, prtlst[particle].pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL );


  if( 0!=retval && NULL!=norm )
  {
    vect3 pos;

    VectorClear( norm->v );

    pos.x = prtlst[particle].pos.x;
    pos.y = prtlst[particle].pos_old.y;
    pos.z = prtlst[particle].pos_old.z;

    if( 0!=mesh_hitawall( gs, pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL ) )
    {
      norm->x = SGN(prtlst[particle].pos.x - prtlst[particle].pos_old.x);
    }

    pos.x = prtlst[particle].pos_old.x;
    pos.y = prtlst[particle].pos.y;
    pos.z = prtlst[particle].pos_old.z;

    if( 0!=mesh_hitawall( gs, pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL ) )
    {
      norm->y = SGN(prtlst[particle].pos.y - prtlst[particle].pos_old.y);
    }

    //pos.x = prtlst[particle].pos_old.x;
    //pos.y = prtlst[particle].pos_old.y;
    //pos.z = prtlst[particle].pos.z;

    //if( 0!=mesh_hitawall( pos, prtlst[particle].bumpsize, prtlst[particle].bumpsize, collision_bits, NULL ) )
    //{
    //  norm->z = SGN(prtlst[particle].pos.z - prtlst[particle].pos_old.z);
    //}

    *norm = Normalize( *norm );
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( GameState * gs, CHR_REF character )
{
  // ZZ> This function makes sure a character has no attached particles

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  PRT_REF particle;
  bool_t useful = bfalse;

  if ( !VALID_CHR( chrlst,  character ) ) return;

  for ( particle = 0; particle < prtlst_size; particle++ )
  {
    if ( !VALID_PRT( prtlst,  particle ) ) continue;

    if ( prt_get_attachedtochr( gs, particle ) == character )
    {
      prtlst[particle].gopoof        = btrue;
      prtlst[particle].attachedtochr = MAXCHR;
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
Uint16 number_of_attached_particles( GameState * gs, CHR_REF character )
{
  // ZZ> This function returns the number of particles attached to the given character

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Uint16 cnt, particle;

  cnt = 0;
  for ( particle = 0; particle < prtlst_size; particle++ )
  {
    if ( VALID_PRT( prtlst,  particle ) && prt_get_attachedtochr( gs, particle ) == character )
    {
      cnt++;
    }
  }

  return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles( GameState * gs, CHR_REF character )
{
  // ZZ> This function makes sure a character has all of it's particles

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Chr  * pchr        = gs->ChrList + character;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  Uint16 numberattached;
  PRT_REF particle;

  numberattached = number_of_attached_particles( gs, character );
  while ( numberattached < caplst[pchr->model].attachedprtamount )
  {
    particle = spawn_one_particle( gs, 1.0f, pchr->pos, 0, pchr->model, caplst[pchr->model].attachedprttype, character, GRIP_LAST + numberattached, pchr->team, character, numberattached, MAXCHR );
    if ( VALID_PRT(prtlst, particle) )
    {
      attach_particle_to_character( gs, particle, character, prtlst[particle].vertoffset );
    }
    numberattached++;
  }

  // Set the alert for reaffirmation ( for exploding barrels with fire )
  pchr->aistate.alert |= ALERT_REAFFIRMED;
}

//--------------------------------------------------------------------------------------------
void move_particles( GameState * gs, float dUpdate )
{
  // ZZ> This is the particle physics function

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  int iprt, tnc;
  Uint32 fan;
  Uint16 facing, pip, particle;
  float level;
  CHR_REF prt_target, prt_owner, prt_attachedto;

  float loc_noslipfriction, loc_homingfriction, loc_homingaccel;

  loc_noslipfriction = pow( noslipfriction, dUpdate );

  for ( iprt = 0; iprt < prtlst_size; iprt++ )
  {
    if ( !VALID_PRT( prtlst,  iprt ) ) continue;

    prt_target     = prt_get_target( gs, iprt );
    prt_owner      = prt_get_owner( gs, iprt );
    prt_attachedto = prt_get_attachedtochr( gs, iprt );

    prtlst[iprt].pos_old = prtlst[iprt].pos;

    prtlst[iprt].onwhichfan = INVALID_FAN;
    prtlst[iprt].level = 0;
    fan = mesh_get_fan( gs, prtlst[iprt].pos );
    prtlst[iprt].onwhichfan = fan;
    prtlst[iprt].level = ( INVALID_FAN == fan ) ? 0 : mesh_get_level( gs, fan, prtlst[iprt].pos.x, prtlst[iprt].pos.y, bfalse );

    // To make it easier
    pip = prtlst[iprt].pip;

    loc_homingfriction = pow( piplst[pip].homingfriction, dUpdate );
    loc_homingaccel    = piplst[pip].homingaccel;

    // Animate particle
    prtlst[iprt].image_fp8 += prtlst[iprt].imageadd_fp8 * dUpdate;
    if ( prtlst[iprt].image_fp8 >= prtlst[iprt].imagemax_fp8 )
    {
      if ( piplst[pip].endlastframe )
      {
        prtlst[iprt].image_fp8 = prtlst[iprt].imagemax_fp8 - INT_TO_FP8( 1 );
        prtlst[iprt].gopoof    = btrue;
      }
      else if ( prtlst[iprt].imagemax_fp8 > 0 )
      {
        // if the prtlst[].image_fp8 is a fraction of an image over prtlst[].imagemax_fp8,
        // keep the fraction
        prtlst[iprt].image_fp8 %= prtlst[iprt].imagemax_fp8;
      }
      else
      {
        // a strange case
        prtlst[iprt].image_fp8 = 0;
      }
    };

    prtlst[iprt].rotate += prtlst[iprt].rotateadd * dUpdate;
    prtlst[iprt].size_fp8 = ( prtlst[iprt].size_fp8 + prtlst[iprt].sizeadd_fp8 < 0 ) ? 0 : prtlst[iprt].size_fp8 + prtlst[iprt].sizeadd_fp8;

    // Change dyna light values
    prtlst[iprt].dyna.level   += piplst[pip].dyna.leveladd * dUpdate;
    prtlst[iprt].dyna.falloff += piplst[pip].dyna.falloffadd * dUpdate;


    // Make it sit on the floor...  Shift is there to correct for sprite size
    level = prtlst[iprt].level + FP8_TO_FLOAT( prtlst[iprt].size_fp8 ) * 0.5f;

    // do interaction with the floor
    if(  !VALID_CHR( chrlst,  prt_attachedto ) && prtlst[iprt].pos.z > level )
    {
      float lerp = ( prtlst[iprt].pos.z - ( prtlst[iprt].level + PLATTOLERANCE ) ) / ( float ) PLATTOLERANCE;
      if ( lerp < 0.2f ) lerp = 0.2f;
      if ( lerp > 1.0f ) lerp = 1.0f;

      prtlst[iprt].accum_acc.z += gravity * lerp;

      prtlst[iprt].accum_acc.x -= ( 1.0f - noslipfriction ) * lerp * prtlst[iprt].vel.x;
      prtlst[iprt].accum_acc.y -= ( 1.0f - noslipfriction ) * lerp * prtlst[iprt].vel.y;
    }

    // Do speed limit on Z
    if ( prtlst[iprt].vel.z < -piplst[pip].spdlimit )  prtlst[iprt].accum_vel.z += -piplst[pip].spdlimit - prtlst[iprt].vel.z;

    // Do homing
    if ( piplst[pip].homing && VALID_CHR( chrlst,  prt_target ) )
    {
      if ( !chrlst[prt_target].alive )
      {
        prtlst[iprt].gopoof = btrue;
      }
      else
      {
        if ( !VALID_CHR( chrlst,  prt_attachedto ) )
        {
          prtlst[iprt].accum_acc.x += -(1.0f-loc_homingfriction) * prtlst[iprt].vel.x;
          prtlst[iprt].accum_acc.y += -(1.0f-loc_homingfriction) * prtlst[iprt].vel.y;
          prtlst[iprt].accum_acc.z += -(1.0f-loc_homingfriction) * prtlst[iprt].vel.z;

          prtlst[iprt].accum_acc.x += ( chrlst[prt_target].pos.x - prtlst[iprt].pos.x ) * loc_homingaccel * 4.0f;
          prtlst[iprt].accum_acc.y += ( chrlst[prt_target].pos.y - prtlst[iprt].pos.y ) * loc_homingaccel * 4.0f;
          prtlst[iprt].accum_acc.z += ( chrlst[prt_target].pos.z + ( chrlst[prt_target].bmpdata.calc_height * 0.5f ) - prtlst[iprt].pos.z ) * loc_homingaccel * 4.0f;
        }
      }
    }


    // Spawn new particles if continually spawning
    if ( piplst[pip].contspawnamount > 0.0f )
    {
      prtlst[iprt].spawntime -= dUpdate;
      if ( prtlst[iprt].spawntime <= 0.0f )
      {
        prtlst[iprt].spawntime = piplst[pip].contspawntime;
        facing = prtlst[iprt].facing;
        
        for ( tnc = 0; tnc < piplst[pip].contspawnamount; tnc++ )
        {
          particle = spawn_one_particle( gs, 1.0f, prtlst[iprt].pos,
                                         facing, prtlst[iprt].model, piplst[pip].contspawnpip,
                                         MAXCHR, GRIP_LAST, prtlst[iprt].team, prt_get_owner( gs, iprt ), tnc, prt_target );

          if ( VALID_PRT(prtlst, particle) && 0 != piplst[prtlst[iprt].pip].facingadd)
          {
            // Hack to fix velocity
            prtlst[particle].vel.x += prtlst[iprt].vel.x;
            prtlst[particle].vel.y += prtlst[iprt].vel.y;
          }

          facing += piplst[pip].contspawnfacingadd;
        }
      }
    }


    // Check underwater
    if ( prtlst[iprt].pos.z < gs->water.douselevel && mesh_has_some_bits( gs->Mesh_Mem.fanlst, prtlst[iprt].onwhichfan, MPDFX_WATER ) && piplst[pip].endwater )
    {
      vect3 prt_pos = {prtlst[iprt].pos.x, prtlst[iprt].pos.y, gs->water.surfacelevel};

      // Splash for particles is just a ripple
      spawn_one_particle( gs, 1.0f, prt_pos, 0, MAXMODEL, PRTPIP_RIPPLE, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );


      // Check for disaffirming character
      if ( VALID_CHR( chrlst,  prt_attachedto ) && prt_get_owner( gs, iprt ) == prt_attachedto )
      {
        // Disaffirm the whole character
        disaffirm_attached_particles( gs, prt_attachedto );
      }
      else
      {
        prtlst[iprt].gopoof = btrue;
      }
    }

    // Down the particle timer
    if ( prtlst[iprt].time > 0.0f )
    {
      prtlst[iprt].time -= dUpdate;
      if ( prtlst[iprt].time <= 0.0f ) prtlst[iprt].gopoof = btrue;
    };

    prtlst[iprt].facing += piplst[pip].facingadd * dUpdate;
  }

}

//--------------------------------------------------------------------------------------------
void attach_particles(GameState * gs)
{
  // ZZ> This function attaches particles to their characters so everything gets
  //     drawn right

  int cnt;

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  for ( cnt = 0; cnt < prtlst_size; cnt++ )
  {
    CHR_REF prt_attachedto;

    if ( !VALID_PRT( prtlst,  cnt ) ) continue;

    prt_attachedto = prt_get_attachedtochr( gs, cnt );

    if ( !VALID_CHR( chrlst,  prt_attachedto ) ) continue;

    attach_particle_to_character( gs, cnt, prt_attachedto, prtlst[cnt].vertoffset );

    // Correct facing so swords knock characters in the right direction...
    if ( HAS_SOME_BITS( piplst[prtlst[cnt].pip].damfx, DAMFX_TURN ) )
    {
      prtlst[cnt].facing = chrlst[prt_attachedto].turn_lr;
    }
  }
}


//--------------------------------------------------------------------------------------------
void setup_particles( GameState * gs )
{
  // ZZ> This function sets up particle data

  particletexture = 0;

  // Reset the allocation table
  PrtList_delete(gs);
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir( Uint16 majordir, float dx, float dy, float dUpdate )
{
  // ZZ> This function returns a direction between the major and minor ones, closer
  //     to the major.

  Uint16 rotate_sin, minordir;
  Sint16 diff_dir;
  const float turnspeed = 2000.0f;

  if ( ABS( dx ) + ABS( dy ) > TURNSPD )
  {
    minordir = vec_to_turn( dx, dy );

    diff_dir = (( Sint16 ) minordir ) - (( Sint16 ) majordir );

    if (( diff_dir > -turnspeed * dUpdate ) && ( diff_dir < turnspeed * dUpdate ) )
    {
      rotate_sin = ( Uint16 ) diff_dir;
    }
    else
    {
      rotate_sin = ( Uint16 )( turnspeed * dUpdate * SGN( diff_dir ) );
    };

    return majordir + rotate_sin;
  }
  else
    return majordir;
}

//--------------------------------------------------------------------------------------------
void spawn_bump_particles( GameState * gs, CHR_REF character, PRT_REF particle )
{
  // ZZ> This function is for catching characters on fire and such

  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;

  Mad  * madlst      = gs->MadList;
  size_t madlst_size = MAXMODEL;

  Cap  * caplst      = gs->CapList;
  size_t caplst_size = MAXCAP;

  int cnt;
  Sint16 x, y, z;
  int distance, bestdistance;
  Uint16 facing, bestvertex;
  Uint16 amount;
  Uint16 pip;
  Uint16 vertices;
  Uint16 direction, left, right, model;
  float fsin, fcos;

  pip = prtlst[particle].pip;
  amount = piplst[pip].bumpspawnamount;


  if ( amount != 0 || piplst[pip].spawnenchant )
  {
    // Only damage if hitting from proper direction
    model = chrlst[character].model;
    vertices = madlst[model].vertices;
    direction = chrlst[character].turn_lr - vec_to_turn( -prtlst[particle].vel.x, -prtlst[particle].vel.y );
    if ( HAS_SOME_BITS( madlst[chrlst[character].model].framefx[chrlst[character].anim.next], MADFX_INVICTUS ) )
    {
      // I Frame
      if ( HAS_SOME_BITS( piplst[pip].damfx, DAMFX_BLOC ) )
      {
        left  = UINT16_MAX;
        right = 0;
      }
      else
      {
        direction -= caplst[model].iframefacing;
        left = ( ~caplst[model].iframeangle );
        right = caplst[model].iframeangle;
      }
    }
    else
    {
      // N Frame
      direction -= caplst[model].nframefacing;
      left = ( ~caplst[model].nframeangle );
      right = caplst[model].nframeangle;
    }
    // Check that direction
    if ( direction <= left && direction >= right )
    {
      // Spawn new enchantments
      if ( piplst[pip].spawnenchant )
      {
        spawn_enchant( gs, prt_get_owner( gs, particle ), character, MAXCHR, MAXENCHANT, prtlst[particle].model );
      }

      // Spawn particles
      if ( amount != 0 && !caplst[chrlst[character].model].resistbumpspawn && !chrlst[character].invictus &&
           vertices != 0 && ( chrlst[character].skin.damagemodifier_fp8[prtlst[particle].damagetype]&DAMAGE_SHIFT ) != DAMAGE_SHIFT )
      {
        if ( amount == 1 )
        {
          // A single particle ( arrow? ) has been stuck in the character...
          // Find best vertex to attach to

          Uint32 ilast, inext;
          MD2_Model * pmdl;
          MD2_Frame * plast, * pnext;
          float flip;

          model = chrlst[character].model;
          inext = chrlst[character].anim.next;
          ilast = chrlst[character].anim.last;
          flip = chrlst[character].anim.flip;

          assert( MAXMODEL != VALIDATE_MDL( model ) );

          pmdl  = madlst[model].md2_ptr;
          plast = md2_get_Frame(pmdl, ilast);
          pnext = md2_get_Frame(pmdl, inext);

          bestvertex = 0;
          bestdistance = 9999999;

          z =  prtlst[particle].pos.z - ( chrlst[character].pos.z + RAISE );
          facing = prtlst[particle].facing - chrlst[character].turn_lr - 16384;
          facing >>= 2;
          fsin = turntosin[facing & TRIGTABLE_MASK];
          fcos = turntocos[facing & TRIGTABLE_MASK];
          y = 8192;
          x = -y * fsin;
          y = y * fcos;

          for (cnt = 0; cnt < vertices; cnt++ )
          {
            vect3 vpos;

            vpos.x = pnext->vertices[cnt].x + (plast->vertices[cnt].x - pnext->vertices[cnt].x)*flip;
            vpos.y = pnext->vertices[cnt].y + (plast->vertices[cnt].y - pnext->vertices[cnt].y)*flip;
            vpos.z = pnext->vertices[cnt].z + (plast->vertices[cnt].z - pnext->vertices[cnt].z)*flip;

            distance = ABS( x - vpos.x ) + ABS( y - vpos.y ) + ABS( z - vpos.z );
            if ( distance < bestdistance )
            {
              bestdistance = distance;
              bestvertex = cnt;
            }

          }

          spawn_one_particle( gs, 1.0f, chrlst[character].pos, 0, prtlst[particle].model, piplst[pip].bumpspawnpip,
                              character, bestvertex + 1, prtlst[particle].team, prt_get_owner( gs, particle ), cnt, character );
        }
        else
        {
          amount = ( amount * vertices ) >> 5;  // Correct amount for size of character
          
          for ( cnt = 0; cnt < amount; cnt++ )
          {
            spawn_one_particle( gs, 1.0f, chrlst[character].pos, 0, prtlst[particle].model, piplst[pip].bumpspawnpip,
                                character, rand() % vertices, prtlst[particle].team, prt_get_owner( gs, particle ), cnt, character );
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
bool_t prt_is_over_water( GameState * gs, int cnt )
{
  // This function returns btrue if the particle is over a water tile

  Uint32 fan;

  if ( VALID_PRT(gs->PrtList, cnt) )
  {
    fan = mesh_get_fan( gs, gs->PrtList[cnt].pos );
    if ( mesh_has_some_bits( gs->Mesh_Mem.fanlst, fan, MPDFX_WATER ) )  return ( INVALID_FAN != fan );
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn( GameState * gs, float dUpdate )
{
  // ZZ> This function drops snowflakes or rain or whatever, also swings the camera

  Chr  * chrlst      = gs->ChrList;
  size_t chrlst_size = MAXCHR;


  PRT_REF particle;
  int cnt;
  bool_t foundone = bfalse;

  if ( GWeather.time > 0 )
  {
    GWeather.time -= dUpdate;
    if ( GWeather.time < 0 ) GWeather.time = 0;

    if ( GWeather.time == 0 )
    {
      GWeather.time = GWeather.timereset;

      // Find a valid player
      foundone = bfalse;
      cnt = 0;
      while ( cnt < MAXPLAYER )
      {
        GWeather.player = ( GWeather.player + 1 ) % MAXPLAYER;
        if ( VALID_PLA( gs->PlaList, GWeather.player ) )
        {
          foundone = btrue;
          cnt = MAXPLAYER;
        }
        cnt++;
      }
    }


    // Did we find one?
    if ( foundone )
    {
      // Yes, but is the character valid?
      cnt = PlaList_get_character( gs, GWeather.player );
      if ( VALID_CHR( chrlst, cnt ) && !chr_in_pack( chrlst, chrlst_size, cnt ) )
      {
        // Yes, so spawn over that character
        particle = spawn_one_particle( gs, 1.0f, chrlst[cnt].pos, 0, MAXMODEL, PRTPIP_WEATHER_1, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
        if ( VALID_PRT(gs->PrtList, particle) )
        {
          if ( GWeather.overwater && !prt_is_over_water( gs, particle ) )
          {
            PrtList_free_one_no_sound( gs, particle );
          }
        }
      }
    }
  }
  GCamera.swing = ( GCamera.swing + GCamera.swingrate ) & TRIGTABLE_MASK;
}


//--------------------------------------------------------------------------------------------
Uint32 PipList_load_one( GameState * gs, char * szObjectpath, char * szObjectname, char * szFname, int override)
{
  // ZZ> This function loads a particle template, returning MAXPRTPIP if the file wasn't
  //     found

  Pip  * piplst      = gs->PipList;
  size_t piplst_size = MAXPRTPIP;

  const char * fname;
  FILE* fileread;
  IDSZ idsz;
  int iTmp;
  Pip * pip;
  Uint32 ipip = MAXPRTPIP;

  fname = inherit_fname(szObjectpath, szObjectname, szFname);
  fileread = fs_fileOpen( PRI_NONE, NULL, fname, "r" );
  if ( NULL == fileread ) return ipip;

  if(override > 0)
  {
    ipip = override;
  }
  else
  {
    ipip = gs->PipList_count;
    gs->PipList_count++;
  };

  // for "simpler" notation
  pip = piplst + ipip;

  // store some info for debugging
  strncpy( pip->fname, fname,    sizeof( pip->fname ) );
  globalname = pip->fname;

  fgets( pip->comment, sizeof( pip->comment ), fileread );
  if ( pip->comment[0] != '/' )  pip->comment[0] = '\0';

  rewind( fileread );

  // General data
  pip->force = fget_next_bool( fileread );
  pip->type = fget_next_prttype( fileread );
  pip->imagebase = fget_next_int( fileread );
  pip->numframes = fget_next_int( fileread );
  pip->imageadd.ibase = fget_next_int( fileread );
  pip->imageadd.irand = fget_next_int( fileread );
  pip->rotate.ibase = fget_next_int( fileread );
  pip->rotate.irand = fget_next_int( fileread );
  pip->rotateadd = fget_next_int( fileread );
  pip->sizebase_fp8 = fget_next_int( fileread );
  pip->sizeadd = fget_next_int( fileread );
  pip->spdlimit = fget_next_float( fileread );
  pip->facingadd = fget_next_int( fileread );


  // Ending conditions
  pip->endwater = fget_next_bool( fileread );
  pip->endbump = fget_next_bool( fileread );
  pip->endground = fget_next_bool( fileread );
  pip->endlastframe = fget_next_bool( fileread );
  pip->time = fget_next_int( fileread );


  // Collision data
  pip->dampen = fget_next_float( fileread );
  pip->bumpmoney = fget_next_int( fileread );
  pip->bumpsize = fget_next_int( fileread );
  pip->bumpheight = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &pip->damage_fp8 );
  pip->damagetype = fget_next_damage( fileread );
  pip->bumpstrength = 1.0f;
  if ( pip->bumpsize == 0 )
  {
    pip->bumpstrength = 0.0f;
    pip->bumpsize   = 0.5f * FP8_TO_FLOAT( pip->sizebase_fp8 );
    pip->bumpheight = 0.5f * FP8_TO_FLOAT( pip->sizebase_fp8 );
  };

  // Lighting data
  pip->dyna.mode = fget_next_dynamode( fileread );
  pip->dyna.level = fget_next_float( fileread );
  pip->dyna.falloff = fget_next_int( fileread );
  if ( pip->dyna.falloff > MAXFALLOFF )  pip->dyna.falloff = MAXFALLOFF;



  // Initial spawning of this particle
  pip->facing.ibase    = fget_next_int( fileread );
  pip->facing.irand    = fget_next_int( fileread );
  pip->xyspacing.ibase = fget_next_int( fileread );
  pip->xyspacing.irand = fget_next_int( fileread );
  pip->zspacing.ibase  = fget_next_int( fileread );
  pip->zspacing.irand  = fget_next_int( fileread );
  pip->xyvel.ibase     = fget_next_int( fileread );
  pip->xyvel.irand     = fget_next_int( fileread );
  pip->zvel.ibase      = fget_next_int( fileread );
  pip->zvel.irand      = fget_next_int( fileread );


  // Continuous spawning of other particles
  pip->contspawntime = fget_next_int( fileread );
  pip->contspawnamount = fget_next_int( fileread );
  pip->contspawnfacingadd = fget_next_int( fileread );
  pip->contspawnpip = fget_next_int( fileread );


  // End spawning of other particles
  pip->endspawnamount = fget_next_int( fileread );
  pip->endspawnfacingadd = fget_next_int( fileread );
  pip->endspawnpip = fget_next_int( fileread );

  // Bump spawning of attached particles
  pip->bumpspawnamount = fget_next_int( fileread );
  pip->bumpspawnpip = fget_next_int( fileread );


  // Random stuff  !!!BAD!!! Not complete
  pip->dazetime = fget_next_int( fileread );
  pip->grogtime = fget_next_int( fileread );
  pip->spawnenchant = fget_next_bool( fileread );
  pip->causeknockback = fget_next_bool( fileread );
  pip->causepancake = fget_next_bool( fileread );
  pip->needtarget = fget_next_bool( fileread );
  pip->targetcaster = fget_next_bool( fileread );
  pip->startontarget = fget_next_bool( fileread );
  pip->onlydamagefriendly = fget_next_bool( fileread );

  iTmp = fget_next_int( fileread ); pip->soundspawn = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); pip->soundend   = FIX_SOUND( iTmp );

  pip->friendlyfire = fget_next_bool( fileread );
  pip->hateonly = fget_next_bool( fileread );
  pip->newtargetonspawn = fget_next_bool( fileread );
  pip->targetangle = fget_next_int( fileread ) >> 1;
  pip->homing = fget_next_bool( fileread );
  pip->homingfriction = fget_next_float( fileread );
  pip->homingaccel = fget_next_float( fileread );
  pip->rotatetoface = fget_next_bool( fileread );
  fgoto_colon( fileread );   //BAD! Not used
  pip->manadrain = fget_next_fixed( fileread );   //Mana drain (Mana damage)
  pip->lifedrain = fget_next_fixed( fileread );   //Life drain (Life steal)



  // Clear expansions...
  pip->zaimspd     = 0;
  pip->soundfloor = INVALID_SOUND;
  pip->soundwall  = INVALID_SOUND;
  pip->endwall    = pip->endground;
  pip->damfx      = DAMFX_TURN;
  if ( pip->homing )  pip->damfx = DAMFX_NONE;
  pip->allowpush = btrue;
  pip->dyna.falloffadd = 0;
  pip->dyna.leveladd = 0;
  pip->intdamagebonus = bfalse;
  pip->wisdamagebonus = bfalse;
  pip->rotatewithattached = btrue;
  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );

    if ( MAKE_IDSZ( "TURN" ) == idsz ) pip->rotatewithattached = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "ZSPD" ) == idsz )  pip->zaimspd = iTmp;
    else if ( MAKE_IDSZ( "FSND" ) == idsz )  pip->soundfloor = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WSND" ) == idsz )  pip->soundwall = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WEND" ) == idsz )  pip->endwall = 0 != iTmp;
    else if ( MAKE_IDSZ( "ARMO" ) == idsz )  pip->damfx |= DAMFX_ARMO;
    else if ( MAKE_IDSZ( "BLOC" ) == idsz )  pip->damfx |= DAMFX_BLOC;
    else if ( MAKE_IDSZ( "ARRO" ) == idsz )  pip->damfx |= DAMFX_ARRO;
    else if ( MAKE_IDSZ( "TIME" ) == idsz )  pip->damfx |= DAMFX_TIME;
    else if ( MAKE_IDSZ( "PUSH" ) == idsz )  pip->allowpush = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DLEV" ) == idsz )  pip->dyna.leveladd = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "DRAD" ) == idsz )  pip->dyna.falloffadd = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "IDAM" ) == idsz )  pip->intdamagebonus = 0 != iTmp;
    else if ( MAKE_IDSZ( "WDAM" ) == idsz )  pip->wisdamagebonus = 0 != iTmp;
  }

  fs_fileClose( fileread );

  return ipip;
}

//--------------------------------------------------------------------------------------------
void PipList_load_global(GameState * gs)
{
  // ZF> Load in the standard global particles ( the coins for example )
  //     This should only be needed done once at the start of the game

  log_info("PipList_load_global() - Loading global particles into memory... ");
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING, CData.basicdat_dir, CData.globalparticles_dir );

  //Defend particle
  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.defend_file, PRTPIP_DEFEND ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 1
  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money1_file, PRTPIP_COIN_001 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 5
  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money5_file, PRTPIP_COIN_005 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 25
  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money25_file, PRTPIP_COIN_025 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 100
  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.money100_file, PRTPIP_COIN_100 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Everything went fine
  log_message("Succeeded!\n");
}

//--------------------------------------------------------------------------------------------
void reset_particles( GameState * gs, char* modname )
{
  // ZZ> This resets all particle data and reads in the coin and water particles

  // Load in the standard global particles ( the coins for example )
  //BAD! This should only be needed once at the start of the game, using PipList_load_global

  PipList_renew(gs);

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s" SLASH_STRING, modname, CData.gamedat_dir);

  //Load module specific information
  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.weather1_file, PRTPIP_WEATHER_1 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.weather2_file, PRTPIP_WEATHER_2 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.splash_file, PRTPIP_SPLASH ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  if ( MAXPRTPIP == PipList_load_one( gs, CStringTmp1, NULL, CData.ripple_file, PRTPIP_RIPPLE ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

}

//--------------------------------------------------------------------------------------------
bool_t prt_calculate_bumpers(GameState * gs, PRT_REF iprt)
{
  Prt  * prtlst      = gs->PrtList;
  size_t prtlst_size = MAXPRT;

  float ftmp;

  if( !VALID_PRT( prtlst, iprt) ) return bfalse;

  prtlst[iprt].bmpdata.mids_lo = prtlst[iprt].pos;

  // calculate the particle radius
  ftmp = FP8_TO_FLOAT(prtlst[iprt].size_fp8) * 0.5f;

  // calculate the "perfect" bbox for a sphere
  prtlst[iprt].bmpdata.cv.x_min = prtlst[iprt].pos.x - ftmp - 0.001f;
  prtlst[iprt].bmpdata.cv.x_max = prtlst[iprt].pos.x + ftmp + 0.001f;

  prtlst[iprt].bmpdata.cv.y_min = prtlst[iprt].pos.y - ftmp - 0.001f;
  prtlst[iprt].bmpdata.cv.y_max = prtlst[iprt].pos.y + ftmp + 0.001f;

  prtlst[iprt].bmpdata.cv.z_min = prtlst[iprt].pos.z - ftmp - 0.001f;
  prtlst[iprt].bmpdata.cv.z_max = prtlst[iprt].pos.z + ftmp + 0.001f;

  prtlst[iprt].bmpdata.cv.xy_min = prtlst[iprt].pos.x - ftmp * SQRT_TWO - 0.001f;
  prtlst[iprt].bmpdata.cv.xy_max = prtlst[iprt].pos.x + ftmp * SQRT_TWO + 0.001f;

  prtlst[iprt].bmpdata.cv.yx_min = prtlst[iprt].pos.y - ftmp * SQRT_TWO - 0.001f;
  prtlst[iprt].bmpdata.cv.yx_max = prtlst[iprt].pos.y + ftmp * SQRT_TWO + 0.001f;

  return btrue;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Prt * Prt_new(Prt *pprt) 
{ 
  //fprintf( stdout, "Prt_new()\n");

  if(NULL==pprt) return pprt; 
  
  memset(pprt, 0, sizeof(Prt)); 
  pprt->on = bfalse;
  
  return pprt; 
};

//--------------------------------------------------------------------------------------------
bool_t Prt_delete( Prt * pprt )
{ 
  if(NULL == pprt) return bfalse;

  pprt->on = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Prt * Prt_renew(Prt *pprt)
{
  Prt_delete(pprt);
  return Prt_new(pprt);
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Pip * Pip_new(Pip * ppip)
{
  //fprintf( stdout, "Pip_new()\n");

  if(NULL == ppip) return ppip;

  memset(ppip, 0, sizeof(Pip));
  ppip->used = bfalse;

  ppip->soundspawn = INVALID_SOUND;                   // Beginning sound
  ppip->soundend   = INVALID_SOUND;                     // Ending sound
  ppip->soundfloor = INVALID_SOUND;                   // Floor sound
  ppip->soundwall  = INVALID_SOUND;                    // Ricochet sound

  return ppip;
}

//--------------------------------------------------------------------------------------------
Pip * Pip_delete(Pip * ppip)
{
  if(NULL == ppip) return ppip;

  ppip->used = bfalse;

  ppip->fname[0]   = '\0';
  ppip->comment[0] = '\0';

  return ppip;
}
//--------------------------------------------------------------------------------------------
Pip * Pip_renew(Pip * ppip)
{
  Pip_delete(ppip);
  return Pip_new(ppip);
}