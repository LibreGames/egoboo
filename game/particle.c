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

/* Egoboo - particle.c
* Manages particle systems.
*/

#include "particle.h"
#include "enchant.h"
#include "char.h"
#include "mad.h"
#include "profile.h"

#include "log.h"
#include "sound.h"
#include "camera.h"
#include "mesh.h"
#include "game.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float            sprite_list_u[MAXPARTICLEIMAGE][2];        // Texture coordinates
float            sprite_list_v[MAXPARTICLEIMAGE][2];

Uint16           maxparticles = 512;                            // max number of particles

DECLARE_STACK( ACCESS_TYPE_NONE, pip_t, PipStack );
DECLARE_LIST ( ACCESS_TYPE_NONE, prt_t, PrtList );


static const Uint32  particletrans = 0x80;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void prt_init( prt_t * pprt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_count_free()
{
    return PrtList.free_count;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_free_one( Uint16 iprt )
{
    // ZZ> This function sticks a particle back on the free particle stack

    bool_t retval;

    if ( !VALID_PRT_RANGE(iprt) ) return bfalse;

    // particle "destructor"
    // sets all boolean values to false, incluting the "on" flag
    prt_init( PrtList.lst + iprt );

#if defined(USE_DEBUG)
    {
        int cnt;
        // determine whether this particle is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < PrtList.free_count; cnt++ )
        {
            if ( iprt == PrtList.free_ref[cnt] )
            {
                return bfalse;
            }
        }
    }
#endif

    // push it on the free stack
    retval = bfalse;
    if ( PrtList.free_count < TOTAL_MAX_PRT )
    {
        PrtList.free_ref[PrtList.free_count] = iprt;
        PrtList.free_count++;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( Uint16 particle, Sint8 sound )
{
    // This function plays a sound effect for a particle

    prt_t * pprt;

    if ( !ACTIVE_PRT(particle) ) return;
    pprt = PrtList.lst + particle;

    if ( sound >= 0 && sound < MAX_WAVE )
    {
        if ( VALID_PRO( pprt->profile_ref ) )
        {
            sound_play_chunk( pprt->pos, pro_get_chunk(pprt->profile_ref, sound) );
        }
        else
        {
            sound_play_chunk( pprt->pos, g_wavelist[sound] );
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_in_game( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack and
    //    plays the sound associated with the particle

    Uint16 child;
    prt_t * pprt;
    pip_t * ppip;

    if ( !ALLOCATED_PRT( particle) ) return;

    pprt = PrtList.lst + particle;

    if ( pprt->spawncharacterstate != SPAWNNOCHARACTER )
    {
        child = spawn_one_character( pprt->pos, pprt->profile_ref, pprt->team, 0, pprt->facing, NULL, MAX_CHR );
        if ( ACTIVE_CHR(child) )
        {
            chr_get_pai(child)->state = pprt->spawncharacterstate;
            chr_get_pai(child)->owner = pprt->owner_ref;
        }
    }

    ppip = prt_get_ppip( particle );
    if ( NULL != ppip )
    {
        play_particle_sound( particle, ppip->soundend );
    }

    PrtList_free_one( particle );
}

//--------------------------------------------------------------------------------------------
Uint16 PrtList_get_free()
{
    // ZZ> This function returns the next free particle or TOTAL_MAX_PRT if there are none

    Uint16 retval = TOTAL_MAX_PRT;

    if ( PrtList.free_count > 0 )
    {
        PrtList.free_count--;
        retval = PrtList.free_ref[PrtList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int get_free_particle( int force )
{
    // ZZ> This function gets an unused particle.  If all particles are in use
    //    and force is set, it grabs the first unimportant one.  The particle
    //    index is the return value
    int particle;

    // Return maxparticles if we can't find one
    particle = TOTAL_MAX_PRT;
    if ( 0 == PrtList.free_count )
    {
        if ( force )
        {
            // Gotta find one, so go through the list and replace a unimportant one
            particle = 0;

            while ( particle < maxparticles )
            {
                if ( PrtList.lst[particle].bumpsize == 0 )
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
        if ( force || PrtList.free_count > ( maxparticles / 4 ) )
        {
            // Just grab the next one
            particle = PrtList_get_free();
        }
    }

    return (particle >= maxparticles) ? TOTAL_MAX_PRT : particle;
}

//--------------------------------------------------------------------------------------------
void prt_init( prt_t * pprt )
{
    if( NULL == pprt ) return;

    memset( pprt, 0, sizeof(prt_t) );

    /* pprt->inview     = bfalse; */

    /* pprt->floor_level = 0; */
    pprt->spawncharacterstate = SPAWNNOCHARACTER;

    // Lighting and sound
    /* pprt->dynalight_on = bfalse; */

    // Image data
    /* pprt->image = 0; */

    // "no lifetime" = "eternal"
    /* pprt->is_eternal = bfalse; */
    pprt->time      = (Uint32)~0;

    pprt->pip_ref      = MAX_PIP;
    pprt->profile_ref  = MAX_PROFILE;

    pprt->attachedto_ref = MAX_CHR;
    pprt->owner_ref      = MAX_CHR;
    pprt->target_ref     = MAX_CHR;
    pprt->parent_ref     = TOTAL_MAX_PRT;
}

//--------------------------------------------------------------------------------------------
Uint16 prt_get_iowner( Uint16 iprt )
{
    Uint16 iowner = MAX_CHR;

    prt_t * pprt;

    if( !ACTIVE_PRT(iprt) ) return MAX_CHR;
    pprt = PrtList.lst + iprt;

    if( ACTIVE_CHR(pprt->owner_ref) )
    {
        iowner = pprt->owner_ref;
    }
    else
    {
        // make a check for a stupid looping structure...
        // cannot be sure you could never get a loop, though
        if( iprt != pprt->parent_ref )
        {
            iowner = prt_get_iowner( pprt->parent_ref );
        }
    }

    return iowner;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle( GLvector3 pos, Uint16 facing, Uint16 iprofile, Uint16 ipip,
                           Uint16 chr_attach, Uint16 vrt_offset, Uint8 team,
                           Uint16 chr_origin, Uint16 prt_origin, Uint16 multispawn, Uint16 oldtarget )
{
    // ZZ> This function spawns a new particle, and returns the number of that particle
    int iprt, velocity;
    GLvector3 vel;
    float tvel;
    int offsetfacing = 0, newrand;
    prt_t * pprt;
    pip_t * ppip;
    Uint32 prt_lifetime;
    GLvector3 tmp_pos;
    Uint16 turn;

    // Convert from local ipip to global ipip
    if ( ipip < MAX_PIP_PER_PROFILE && VALID_PRO(iprofile) )
    {
        ipip = pro_get_ipip(iprofile, ipip);
    }

    if ( INVALID_PIP(ipip) )
    {
        //log_warning( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
        //    ipip,
        //    chr_origin, ACTIVE_CHR(chr_origin) ? ChrList.lst[chr_origin].name : "INVALID",
        //    iprofile, VALID_PRO(iprofile) ? ProList.lst[iprofile].name : "INVALID" );
        return TOTAL_MAX_PRT;
    }
    ppip = PipStack.lst + ipip;

    iprt = get_free_particle( ppip->force );
    if ( !VALID_PRT_RANGE(iprt) )
    {
        //log_warning( "spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
        //    chr_origin, ACTIVE_CHR(chr_origin) ? ChrList.lst[chr_origin].name : "INVALID",
        //    ipip, VALID_PIP(ipip) ? PipStack.lst[ipip].name : "INVALID",
        //    iprofile, VALID_PRO(iprofile) ? ProList.lst[iprofile].name : "INVALID" );
        return TOTAL_MAX_PRT;
    }
    pprt = PrtList.lst + iprt;

    // clear out all data
    prt_init( pprt );

    tmp_pos = pos;

    // Necessary data for any part
    EGO_OBJECT_ACTIVATE( pprt, iprt, ppip->name );

    // try to get an idea of who our owner is even if we are
    // given bogus info
    if( !ACTIVE_CHR(chr_origin) && ACTIVE_PRT( prt_origin ) )
    {
        chr_origin = prt_get_iowner( prt_origin );
    }

    pprt->pip_ref     = ipip;
    pprt->profile_ref = iprofile;
    pprt->team        = team;
    pprt->owner_ref   = chr_origin;
    pprt->parent_ref  = prt_origin;
    pprt->damagetype  = ppip->damagetype;

    // Lighting and sound
    if ( multispawn == 0 )
    {
        pprt->dynalight_on = ppip->dynalight_mode;
        if ( ppip->dynalight_mode == DYNALOCAL )
        {
            pprt->dynalight_on = bfalse;
        }
    }

    pprt->dynalight_level = ppip->dynalight_level;
    pprt->dynalight_falloff = ppip->dynalight_falloff;

    // Set character attachments ( chr_attach==MAX_CHR means none )
    pprt->attachedto_ref = chr_attach;
    pprt->vrt_off = vrt_offset;

    // Correct facing
    facing += ppip->facing_pair.base;

    // Targeting...
    vel.z = 0;
    tmp_pos.z = tmp_pos.z + generate_randmask( ppip->zspacing_pair.base, ppip->zspacing_pair.rand ) - ( ppip->zspacing_pair.rand >> 1 );
    velocity = generate_randmask( ppip->xyvel_pair.base, ppip->xyvel_pair.rand );
    pprt->target_ref = oldtarget;
    if ( ppip->newtargetonspawn )
    {
        if ( ppip->targetcaster )
        {
            // Set the target to the caster
            pprt->target_ref = chr_origin;
        }
        else
        {
            // Find a target
            pprt->target_ref = get_particle_target( pos.x, pos.y, pos.z, facing, ipip, team, chr_origin, oldtarget );
            if ( ACTIVE_CHR(pprt->target_ref) && !ppip->homing )
            {
                facing -= glouseangle;
            }

            // Correct facing for dexterity...
            offsetfacing = 0;
            if ( ChrList.lst[chr_origin].dexterity < PERFECTSTAT )
            {
                // Correct facing for randomness
                offsetfacing  = generate_randmask( 0, ppip->facing_pair.rand) - (ppip->facing_pair.rand >> 1);
                offsetfacing  = ( offsetfacing * ( PERFECTSTAT - ChrList.lst[chr_origin].dexterity ) ) / PERFECTSTAT;
            }

            if ( ACTIVE_CHR(pprt->target_ref) && ppip->zaimspd != 0 )
            {
                // These aren't velocities...  This is to do aiming on the Z axis
                if ( velocity > 0 )
                {
                    vel.x = ChrList.lst[pprt->target_ref].pos.x - pos.x;
                    vel.y = ChrList.lst[pprt->target_ref].pos.y - pos.y;
                    tvel = SQRT( vel.x * vel.x + vel.y * vel.y ) / velocity;  // This is the number of steps...
                    if ( tvel > 0 )
                    {
                        vel.z = ( ChrList.lst[pprt->target_ref].pos.z + ( ChrList.lst[pprt->target_ref].bump.height * 0.5f ) - tmp_pos.z ) / tvel;  // This is the vel.z alteration
                        if ( vel.z < -( ppip->zaimspd >> 1 ) ) vel.z = -( ppip->zaimspd >> 1 );
                        if ( vel.z > ppip->zaimspd ) vel.z = ppip->zaimspd;
                    }
                }
            }
        }

        // Does it go away?
        if ( !ACTIVE_CHR(pprt->target_ref) && ppip->needtarget )
        {
            free_one_particle_in_game( iprt );
            return maxparticles;
        }

        // Start on top of target
        if ( ACTIVE_CHR(pprt->target_ref) && ppip->startontarget )
        {
            tmp_pos.x = ChrList.lst[pprt->target_ref].pos.x;
            tmp_pos.y = ChrList.lst[pprt->target_ref].pos.y;
        }
    }
    else
    {
        // Correct facing for randomness
        offsetfacing = generate_randmask( 0,  ppip->facing_pair.rand ) - (ppip->facing_pair.rand >> 1);
    }
    facing += offsetfacing;
    pprt->facing = facing;

    // this is actually pointint in the opposite direction?
    turn = (facing + 32768) >> 2;

    // Location data from arguments
    newrand = generate_randmask( ppip->xyspacing_pair.base, ppip->xyspacing_pair.rand );
    tmp_pos.x += turntocos[turn & TRIG_TABLE_MASK] * newrand;
    tmp_pos.y += turntosin[turn & TRIG_TABLE_MASK] * newrand;

    tmp_pos.x = CLIP(tmp_pos.x, 0, PMesh->info.edge_x - 2);
    tmp_pos.y = CLIP(tmp_pos.y, 0, PMesh->info.edge_y - 2);

    pprt->pos = pprt->pos_old = pprt->pos_stt = tmp_pos;

    // Velocity data
    vel.x = turntocos[turn & TRIG_TABLE_MASK] * velocity;
    vel.y = turntosin[turn & TRIG_TABLE_MASK] * velocity;
    vel.z += generate_randmask( ppip->zvel_pair.base, ppip->zvel_pair.rand ) - ( ppip->zvel_pair.rand >> 1 );
    pprt->vel = pprt->vel_old = vel;

    // Template values
    pprt->bumpsize = ppip->bumpsize;
    pprt->bumpsizebig = pprt->bumpsize * SQRT_TWO;
    pprt->bumpheight = ppip->bumpheight;
    pprt->type = ppip->type;

    // Image data
    pprt->rotate = generate_irand_pair( ppip->rotate_pair );
    pprt->rotateadd = ppip->rotateadd;
    pprt->size_stt = pprt->size = ppip->sizebase;
    pprt->size_add = ppip->sizeadd;
    pprt->imageadd = generate_irand_pair( ppip->imageadd );
    pprt->imagestt = INT_TO_FP8( ppip->imagebase );
    pprt->imagemax = INT_TO_FP8( ppip->numframes );
    prt_lifetime = ppip->time;
    if ( ppip->endlastframe && pprt->imageadd != 0 )
    {
        if ( ppip->time == 0 )
        {
            // Part time is set to 1 cycle
            int frames = ( pprt->imagemax / pprt->imageadd ) - 1;
            prt_lifetime = frames;
        }
        else
        {
            // Part time is used to give number of cycles
            int frames = ( ( pprt->imagemax / pprt->imageadd ) - 1 );
            prt_lifetime = ppip->time * frames;
        }
    }

    // "no lifetime" = "eternal"
    if ( 0 == prt_lifetime )
    {
        pprt->is_eternal = btrue;
    }
    else
    {
        pprt->time = frame_all + prt_lifetime;
    }

    // Set onwhichfan...
    pprt->onwhichfan   = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

    // Damage stuff
    range_to_pair(ppip->damage, &(pprt->damage));

    // Spawning data
    pprt->spawntime = ppip->contspawn_time;
    if ( pprt->spawntime != 0 )
    {
        pprt->spawntime = 1;
        if ( ACTIVE_CHR(pprt->attachedto_ref) )
        {
            pprt->spawntime++; // Because attachment takes an update before it happens
        }
    }

    // Sound effect
    play_particle_sound( iprt, ppip->soundspawn );

    // set up the particle transparency
    pprt->inst.alpha = 0xFF;
    switch ( pprt->inst.type )
    {
        case SPRITE_SOLID: break;
        case SPRITE_ALPHA: pprt->inst.alpha = particletrans; break;
        case SPRITE_LIGHT: break;
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
Uint8 __prthitawall( Uint16 particle )
{
    // ZZ> This function returns nonzero if the particle hit a wall

    Uint8  retval = MPDFX_IMPASS | MPDFX_WALL;
    Uint32 fan;

    pip_t * ppip;
    prt_t * pprt;

    if( !ACTIVE_PRT(particle) ) return retval;
    pprt = PrtList.lst + particle;

    ppip = prt_get_ppip(particle);

    fan = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
    if ( VALID_TILE(PMesh, fan) )
    {
        if ( ppip->bumpmoney )
        {
            retval = mesh_test_fx(PMesh, fan, MPDFX_IMPASS | MPDFX_WALL );
        }
        else
        {
            retval = mesh_test_fx(PMesh, fan, MPDFX_IMPASS);
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void update_all_particles( void )
{
    int cnt, size_new;

    Uint16 particle;

    // figure out where the particle is on the mesh
    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;
        Uint16 ichr;

        if ( !ACTIVE_PRT(particle) ) continue;
        pprt = PrtList.lst + particle;

        pprt->onwhichfan   = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->floor_level  = mesh_get_level( PMesh, pprt->pos.x, pprt->pos.y );

        // reject particles that are hidden
        pprt->is_hidden = bfalse;

        ichr = pprt->attachedto_ref;
        if ( ACTIVE_CHR( ichr ) )
        {
            pprt->is_hidden = ChrList.lst[ichr].is_hidden;
        }
    }

    // do the particle interaction with water
    for ( particle = 0; particle < maxparticles; particle++ )
    {
        bool_t inwater;

        prt_t * pprt;
        pip_t * ppip;

        if ( !ACTIVE_PRT(particle) ) continue;
        pprt = PrtList.lst + particle;

        // do nothing if the particle is hidden
        if( pprt->is_hidden ) continue;

        ppip = prt_get_ppip( particle );
        if( NULL == ppip ) continue;

        inwater = (pprt->pos.z < water.surface_level) && (0 != mesh_test_fx( PMesh, pprt->onwhichfan, MPDFX_WATER ));

        if( inwater && water.is_water && ppip->endwater )
        {
            // Check for disaffirming character
            if ( ACTIVE_CHR( pprt->attachedto_ref ) && pprt->owner_ref == pprt->attachedto_ref )
            {
                // Disaffirm the whole character
                disaffirm_attached_particles( pprt->attachedto_ref );
            }
            else
            {
                // destroy the particle
                prt_request_terminate( particle );
            }
        }
        else if ( inwater )
        {
            bool_t spawn_valid = bfalse;
            Uint16 spawn_pip   = MAX_PIP;
            GLvector3 vtmp = VECT3( pprt->pos.x, pprt->pos.y, water.surface_level );

            if ( !pprt->inwater )
            {
                spawn_valid = btrue;

                if( SPRITE_SOLID == pprt->type )
                {
                    spawn_pip = PIP_SPLASH;
                }
                else
                {
                    spawn_pip = PIP_RIPPLE;
                }
            }
            else
            {
                if( SPRITE_SOLID == pprt->type && !ACTIVE_CHR( pprt->attachedto_ref ) )
                {
                    spawn_valid = btrue;
                    spawn_pip = PIP_RIPPLE;
                }
            }

            if( spawn_valid )
            {
                // Splash for particles is just a ripple
                spawn_one_particle( vtmp, 0, MAX_PROFILE, spawn_pip, MAX_CHR, GRIP_LAST,
                                    TEAM_NULL, MAX_CHR, TOTAL_MAX_PRT, 0, MAX_CHR );
            }


            pprt->inwater  = btrue;
        }
        else
        {
            pprt->inwater = bfalse;
        }
    }


    // apply damage from  attatched bump particles (about once a second)
    if ( 0 == ( frame_all & 31 ) )
    {
        for ( particle = 0; particle < maxparticles; particle++ )
        {
            prt_t * pprt;
            pip_t * ppip;
            Uint16 ichr;

            if ( !ACTIVE_PRT(particle) ) continue;
            pprt = PrtList.lst + particle;

            // do nothing if the particle is hidden
            if( pprt->is_hidden ) continue;

            ppip = prt_get_ppip( particle );
            if( NULL == ppip ) continue;

            ichr = pprt->attachedto_ref;
            if( !ACTIVE_CHR( ichr ) ) continue;

            // Attached iprt_b damage ( Burning )
            if ( ppip->allowpush && ppip->xyvel_pair.base == 0 )
            {
                // Make character limp
                ChrList.lst[ichr].vel.x *= 0.5f;
                ChrList.lst[ichr].vel.y *= 0.5f;
            }

            damage_character( ichr, ATK_BEHIND, pprt->damage, pprt->damagetype, pprt->team, pprt->owner_ref, ppip->damfx, bfalse );
        }
    }

    // the following functions should not be done the first time through the update loop
    if( clock_wld == 0 ) return;

    // update particle timers and such
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        pip_t * ppip;
        prt_t * pprt;

        if ( !ACTIVE_PRT(cnt) ) continue;
        pprt = PrtList.lst + cnt;

        ppip = prt_get_ppip( cnt );
        if ( NULL == ppip ) continue;

        pprt->onwhichfan   = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->floor_level  = mesh_get_level( PMesh, pprt->pos.x, pprt->pos.y );

        // Animate particle
        pprt->image = pprt->image + pprt->imageadd;
        if ( pprt->image >= pprt->imagemax ) pprt->image = 0;

        // rotate the particle
        pprt->rotate += pprt->rotateadd;

        if( 0 != pprt->size_add )
        {
            // resize the paricle
            size_new = pprt->size + pprt->size_add;
            pprt->size = CLIP(size_new, 0, 0xFFFF);

            if( SPRITE_SOLID != pprt->type && 0.0f != pprt->inst.alpha )
            {
                // adjust the particle alpha
                if( size_new > 0 )
                {
                    float ftmp = 1.0f - (float)ABS(pprt->size_add) / (float)size_new;
                    pprt->inst.alpha *= ftmp;
                }
                else
                {
                    pprt->inst.alpha = 0xFF;
                }
            }
        }

        // Change dyna light values
        pprt->dynalight_level   += ppip->dynalight_leveladd;
        if( pprt->dynalight_level < 0 ) pprt->dynalight_level = 0;

        pprt->dynalight_falloff += ppip->dynalight_falloffadd;

        pprt->facing += ppip->facingadd;
    }
}

//--------------------------------------------------------------------------------------------
void move_all_particles( void )
{
    // ZZ> This is the particle physics function

    int cnt, tnc;
    Uint16 facing, particle;
    float level;

    // do the iterative physics
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        float lerp_z, ftmp;
        pip_t * ppip;
        prt_t * pprt;

        bool_t hit_a_wall, hit_a_floor;
        GLvector3 nrm;

        if ( !ACTIVE_PRT(cnt) ) continue;
        pprt = PrtList.lst + cnt;

        pprt->pos_old = pprt->pos;
        pprt->vel_old = pprt->vel;

        ppip = prt_get_ppip( cnt );
        if ( NULL == ppip ) continue;

        // if the particle is hidden, do nothing
        if ( pprt->is_hidden ) continue;

        lerp_z = (pprt->pos.z - pprt->floor_level) / PLATTOLERANCE;
        lerp_z = CLIP(lerp_z, 0, 1);

        // Make it sit on the floor...  Shift is there to correct for sprite size
        level = pprt->floor_level + FP8_TO_FLOAT( pprt->size ) * (1.0f - lerp_z);

        hit_a_wall  = bfalse;
        hit_a_floor = bfalse;
        nrm.x = nrm.y = nrm.z = 0.0f;

        // check for a floor collision
        ftmp = pprt->pos.z;
        pprt->pos.z += pprt->vel.z;
        if ( pprt->pos.z < level )
        {
            if( pprt->vel.z < - STOPBOUNCINGPART )
            {
                // the particle will bounce
                hit_a_floor = btrue;

                nrm.z = 1.0f;
                pprt->pos.z = ftmp;
            }
            else if ( pprt->vel.z > 0.0f )
            {
                // the particle is not bouncing, it is just at the wrong height
                pprt->pos.z = level;
            }
            else
            {
                // the particle is in the "stop bouncing zone"
                pprt->pos.z = level + 1;
                pprt->vel.z = 0.0f;
            }
        }

        // check for an x wall collision
        if( ABS(pprt->vel.x) > 0.0f )
        {
            ftmp = pprt->pos.x;
            pprt->pos.x += pprt->vel.x;
            if ( __prthitawall( cnt ) )
            {
                hit_a_wall = btrue;

                nrm.x = -SGN(pprt->vel.x);
                pprt->pos.x = ftmp;
            }
        }

        // check for an y wall collision
        if( ABS(pprt->vel.y) > 0.0f )
        {
            ftmp = pprt->pos.y;
            pprt->pos.y += pprt->vel.y;
            if ( __prthitawall( cnt ) )
            {
                hit_a_wall = btrue;

                nrm.y = -SGN(pprt->vel.y);
                pprt->pos.y = ftmp;
            }
        }

        // handle the collision
        if( (hit_a_wall && ppip->endwall) || (hit_a_floor && ppip->endground) )
        {
            prt_request_terminate( cnt );
            continue;
        }

        // handle the sounds
        if( hit_a_wall )
        {
            // Play the sound for hitting the floor [FSND]
            play_particle_sound( cnt, ppip->soundwall );
        }

        if( hit_a_floor )
        {
            // Play the sound for hitting the floor [FSND]
            play_particle_sound( cnt, ppip->soundfloor );
        }

        if( !ACTIVE_CHR( pprt->attachedto_ref ) && (hit_a_wall || hit_a_floor) )
        {
            float fx, fy;

            // do the reflections off the walls
            if( (hit_a_wall && ABS(pprt->vel.x) + ABS(pprt->vel.y) > 0.0f) ||
                (hit_a_floor && pprt->vel.z < 0.0f) )
            {
                float vdot;
                GLvector3 vpara, vperp;

                nrm = VNormalize( nrm );

                vdot  = VDotProduct( nrm, pprt->vel );

                vperp.x = nrm.x * vdot;
                vperp.y = nrm.y * vdot;
                vperp.z = nrm.z * vdot;

                vpara.x = pprt->vel.x - vperp.x;
                vpara.y = pprt->vel.y - vperp.y;
                vpara.z = pprt->vel.z - vperp.z;

                // we can use the impulse to determine how much velocity to kill in the parallel direction
                //imp.x = vperp.x * (1.0f + ppip->dampen);
                //imp.y = vperp.y * (1.0f + ppip->dampen);
                //imp.z = vperp.z * (1.0f + ppip->dampen);

                // do the reflection
                vperp.x *= -ppip->dampen;
                vperp.y *= -ppip->dampen;
                vperp.z *= -ppip->dampen;

                // fake the friction, for now
                if( 0.0f != nrm.y || 0.0f != nrm.z )
                {
                    vpara.x *= ppip->dampen;
                }

                if( 0.0f != nrm.x || 0.0f != nrm.z )
                {
                    vpara.y *= ppip->dampen;
                }

                if( 0.0f != nrm.x || 0.0f != nrm.y )
                {
                    vpara.z *= ppip->dampen;
                }

                // add the components back together
                pprt->vel.x = vpara.x + vperp.x;
                pprt->vel.y = vpara.y + vperp.y;
                pprt->vel.z = vpara.z + vperp.z;
            }

            if( nrm.z != 0.0f && pprt->vel.z < STOPBOUNCINGPART )
            {
                // this is the very last bounce
                pprt->vel.z = 0.0f;
                pprt->pos.z = level + 0.0001f;
            }

            if( hit_a_wall )
            {
                // fix the facing
                facing_to_vec( pprt->facing, &fx, &fy );

                if( 0.0f != nrm.x )
                {
                    fx *= -1;
                }

                if( 0.0f != nrm.y )
                {
                    fy *= -1;
                }

                pprt->facing = vec_to_facing( fx, fy );
            }
        }

        // Do homing
        if ( ppip->homing && ACTIVE_CHR( pprt->target_ref ) )
        {
            if ( !ChrList.lst[pprt->target_ref].alive )
            {
                prt_request_terminate( cnt );
            }
            else
            {
                if ( !ACTIVE_CHR( pprt->attachedto_ref ) )
                {
                    int       ival;
                    float     vlen, min_length, uncertainty;
                    GLvector3 vdiff, vdither;

                    vdiff = VSub( ChrList.lst[pprt->target_ref].pos, pprt->pos );
                    vdiff.z += ChrList.lst[pprt->target_ref].bump.height * 0.5f;

                    min_length = ( 2 * 5 * 256 * ChrList.lst[pprt->owner_ref].wisdom ) / PERFECTBIG;

                    // make a little incertainty about the target
                    uncertainty = 256 - ( 256 * ChrList.lst[pprt->owner_ref].intelligence ) / PERFECTBIG;

                    ival = RANDIE;
                    vdither.x = ( ((float) ival / 0x8000) - 1.0f )  * uncertainty;

                    ival = RANDIE;
                    vdither.y = ( ((float) ival / 0x8000) - 1.0f )  * uncertainty;

                    ival = RANDIE;
                    vdither.z = ( ((float) ival / 0x8000) - 1.0f )  * uncertainty;

                    // take away any dithering along the direction of motion of the particle
                    vlen = VDotProduct(pprt->vel, pprt->vel);
                    if( vlen > 0.0f )
                    {
                        float vdot = VDotProduct(vdither, pprt->vel) / vlen;

                        vdither.x -= vdot * vdiff.x / vlen;
                        vdither.y -= vdot * vdiff.y / vlen;
                        vdither.z -= vdot * vdiff.z / vlen;
                    }

                    // add in the dithering
                    vdiff.x += vdither.x;
                    vdiff.y += vdither.y;
                    vdiff.z += vdither.z;

                    // Make sure that vdiff doesn't ever get too small.
                    // That just makes the particle slooooowww down when it approaches the target.
                    // Do a real kludge here. this should be a lot faster than a square root, but ...
                    vlen = ABS(vdiff.x) + ABS(vdiff.y) + ABS(vdiff.z);
                    if( vlen != 0.0f )
                    {
                        float factor = min_length / vlen;

                        vdiff.x *= factor;
                        vdiff.y *= factor;
                        vdiff.z *= factor;
                    }

                    pprt->vel.x = ( pprt->vel.x + vdiff.x * ppip->homingaccel ) * ppip->homingfriction;
                    pprt->vel.y = ( pprt->vel.y + vdiff.y * ppip->homingaccel ) * ppip->homingfriction;
                    pprt->vel.z = ( pprt->vel.z + vdiff.z * ppip->homingaccel ) * ppip->homingfriction;
                }

                if ( ppip->rotatetoface )
                {
                    // Turn to face target
                    pprt->facing =vec_to_facing( ChrList.lst[pprt->target_ref].pos.x - pprt->pos.x , ChrList.lst[pprt->target_ref].pos.y - pprt->pos.y );
                }
            }
        }

        // do gravitational acceleration
        if( !ACTIVE_CHR( pprt->attachedto_ref ) && !ppip->homing )
        {
            pprt->vel.z += gravity * lerp_z;

            // Do speed limit on Z
            if ( pprt->vel.z < -ppip->spdlimit )
            {
                pprt->vel.z = -ppip->spdlimit;
            }
        }

        // Spawn new particles if continually spawning
        if ( ppip->contspawn_amount > 0 )
        {
            pprt->spawntime--;
            if ( pprt->spawntime == 0 )
            {
                pprt->spawntime = ppip->contspawn_time;
                facing = pprt->facing;
                tnc = 0;

                while ( tnc < ppip->contspawn_amount )
                {
                    particle = spawn_one_particle( pprt->pos, facing, pprt->profile_ref, ppip->contspawn_pip,
                                                   MAX_CHR, GRIP_LAST, pprt->team, pprt->owner_ref, cnt, tnc, pprt->target_ref );

                    if ( PipStack.lst[pprt->pip_ref].facingadd != 0 && ACTIVE_PRT(particle) )
                    {
                        // Hack to fix velocity
                        PrtList.lst[particle].vel.x += pprt->vel.x;
                        PrtList.lst[particle].vel.y += pprt->vel.y;
                    }

                    facing += ppip->contspawn_facingadd;
                    tnc++;
                }
            }
        }

    }
}

//--------------------------------------------------------------------------------------------
void cleanup_all_particles()
{
    int iprt, tnc;

    // do end-of-life care for particles
    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        prt_t * pprt;
        Uint16  ipip;
        bool_t  time_out;

        if ( !PrtList.lst[iprt].allocated ) continue;
        pprt = PrtList.lst + iprt;

        time_out = !pprt->is_eternal && frame_all >= pprt->time;
        if ( !pprt->kill_me && !time_out ) continue;

        // Spawn new particles if time for old one is up
        ipip = pprt->pip_ref;
        if ( VALID_PIP( ipip ) )
        {
            pip_t * ppip;
            Uint16 facing;

            ppip = PipStack.lst + ipip;

            facing = pprt->facing;
            for ( tnc = 0; tnc < ppip->endspawn_amount; tnc++ )
            {
                spawn_one_particle( pprt->pos_old, facing, pprt->profile_ref, ppip->endspawn_pip,
                    MAX_CHR, GRIP_LAST, pprt->team, pprt->owner_ref, iprt, tnc, pprt->target_ref );

                facing += ppip->endspawn_facingadd;
            }
        }

        // free the particle
        free_one_particle_in_game( iprt );
    }
}


//--------------------------------------------------------------------------------------------
void PrtList_free_all()
{
    // ZZ> This function resets the particle allocation lists

    int cnt;

    // free all the particles
    PrtList.free_count = 0;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        // reuse this code
        PrtList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void setup_particles()
{
    // ZZ> This function sets up particle data
    int cnt;
    double x, y;

    // Image coordinates on the big particle bitmap
    for ( cnt = 0; cnt < MAXPARTICLEIMAGE; cnt++ )
    {
        x = cnt & 15;
        y = cnt >> 4;
        sprite_list_u[cnt][0] = (float)( ( 0.05f + x ) / 16.0f );
        sprite_list_u[cnt][1] = (float)( ( 0.95f + x ) / 16.0f );
        sprite_list_v[cnt][0] = (float)( ( 0.05f + y ) / 16.0f );
        sprite_list_v[cnt][1] = (float)( ( 0.95f + y ) / 16.0f );
    }

    // Reset the allocation table
    PrtList_free_all();
}

//--------------------------------------------------------------------------------------------
void spawn_bump_particles( Uint16 character, Uint16 particle )
{
    // ZZ> This function is for catching characters on fire and such

    int cnt;
    Sint16 x, y, z;
    Uint32 distance, bestdistance;
    Uint16 facing, bestvertex;
    Uint16 amount;
    Uint16 vertices;
    Uint16 direction;
    float fsin, fcos;

    pip_t * ppip;
    chr_t * pchr;
    mad_t * pmad;
    prt_t * pprt;
    cap_t * pcap;

    if ( !ACTIVE_PRT(particle) ) return;
    pprt = PrtList.lst + particle;

    if ( INVALID_PIP(pprt->pip_ref) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    // no point in going on, is there?
    if ( 0 == ppip->bumpspawn_amount && !ppip->spawnenchant ) return;
    amount = ppip->bumpspawn_amount;

    if ( !ACTIVE_CHR(character) ) return;
    pchr = ChrList.lst + character;

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    // Only damage if hitting from proper direction
    direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
    direction = pchr->turn_z - direction + 32768;

    // Check that direction
    if ( !is_invictus_direction( direction, character, ppip->damfx) )
    {
        vertices = pmad->md2_data.vertices;

        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_one_enchant( pprt->owner_ref, character, MAX_CHR, MAX_ENC, pprt->profile_ref );
        }

        // Spawn particles
        if ( amount != 0 && !pcap->resistbumpspawn && !pchr->invictus && vertices != 0 && ( pchr->damagemodifier[pprt->damagetype]&DAMAGESHIFT ) < 3 )
        {
            Uint16 bs_part;

            if ( amount == 1 )
            {
                // A single particle ( arrow? ) has been stuck in the character...
                // Find best vertex to attach to

                bestvertex = 0;
                bestdistance = 1 << 31;         //Really high number

                z = pprt->pos.z - pchr->pos.z + RAISE;
                facing = pprt->facing - pchr->turn_z - FACE_NORTH;
                facing = facing >> 2;
                fsin = turntosin[facing & TRIG_TABLE_MASK ];
                fcos = turntocos[facing & TRIG_TABLE_MASK ];
                x = -8192 * fsin;
                y =  8192 * fcos;

                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    distance = ABS( x - pchr->inst.vlst[vertices-cnt-1].pos[XX] ) +
                               ABS( y - pchr->inst.vlst[vertices-cnt-1].pos[YY] ) +
                               ABS( z - pchr->inst.vlst[vertices-cnt-1].pos[ZZ] );
                    if ( distance < bestdistance )
                    {
                        bestdistance = distance;
                        bestvertex = cnt;
                    }
                }

                bs_part = spawn_one_particle( pchr->pos, 0, pprt->profile_ref, ppip->bumpspawn_pip,
                                               character, bestvertex + 1, pprt->team, pprt->owner_ref, particle, cnt, character );

                if( ACTIVE_PRT(bs_part) )
                {
                    PrtList.lst[bs_part].is_bumpspawn = btrue;
                }
            }
            else
            {
                // Multiple particles are attached to character
                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    int irand = RANDIE;

                    bs_part = spawn_one_particle( pchr->pos, 0, pprt->profile_ref, ppip->bumpspawn_pip,
                                        character, irand % vertices, pprt->team, pprt->owner_ref, particle, cnt, character );

                    if( ACTIVE_PRT(bs_part) )
                    {
                        PrtList.lst[bs_part].is_bumpspawn = btrue;
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
int prt_is_over_water( Uint16 cnt )
{
    // This function returns btrue if the particle is over a water tile
    Uint32 fan;

    if ( !ACTIVE_PRT(cnt) ) return bfalse;

    fan = mesh_get_tile( PMesh, PrtList.lst[cnt].pos.x, PrtList.lst[cnt].pos.y );
    if ( VALID_TILE(PMesh, fan) )
    {
        if ( 0 != mesh_test_fx( PMesh, fan, MPDFX_WATER ) )  return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 PipStack_get_free()
{
    Uint16 retval = MAX_PIP;

    if( PipStack.count < MAX_PIP )
    {
        retval = PipStack.count;
        PipStack.count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int load_one_particle_profile( const char *szLoadName, Uint16 pip_override )
{
    // ZZ> This function loads a particle template, returning bfalse if the file wasn't
    //    found

    Uint16  ipip;
    pip_t * ppip;

    ipip = MAX_PIP;
    if( VALID_PIP_RANGE(pip_override) )
    {
        release_one_pip(pip_override);
        ipip = pip_override;
    }
    else
    {
        ipip = PipStack_get_free();
    }

    if ( !VALID_PIP_RANGE(ipip) )
    {
        return MAX_PIP;
    }
    ppip = PipStack.lst + ipip;

    if( NULL == load_one_pip_file( szLoadName, ppip ) )
    {
        return MAX_PIP;
    }

    ppip->soundend = CLIP(ppip->soundend, -1, MAX_WAVE);
    ppip->soundspawn = CLIP(ppip->soundspawn, -1, MAX_WAVE);
//   if ( ppip->dynalight_falloff > MAXFALLOFF && PMod->rtscontrol )  ppip->dynalight_falloff = MAXFALLOFF;

    return ipip;
}

//--------------------------------------------------------------------------------------------
void reset_particles( const char* modname )
{
    // ZZ> This resets all particle data and reads in the coin and water particles

    STRING newloadname;
    char *loadpath;

    release_all_local_pips();
    release_all_pip();

    // Load in the standard global particles ( the coins for example )
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "1money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN1 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "5money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN5 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "25money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN25 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "100money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN100 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    // Load module specific information
    make_newloadname( modname, "gamedat" SLASH_STR "weather4.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, PIP_WEATHER4 ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "weather5.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, PIP_WEATHER5 ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "splash.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, PIP_SPLASH ) )
    {
        if (cfg.dev_mode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "splash.txt";
        if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_SPLASH ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    make_newloadname( modname, "gamedat" SLASH_STR "ripple.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname, PIP_RIPPLE ) )
    {
        if (cfg.dev_mode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "ripple.txt";
        if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_RIPPLE ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    // This is also global...
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_DEFEND ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    PipStack.count = PIP_DEFEND;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16  prt_get_ipip( Uint16 iprt )
{
    prt_t * pprt;

    if( !ACTIVE_PRT(iprt) ) return MAX_PIP;
    pprt = PrtList.lst + iprt;

    if( INVALID_PIP(pprt->pip_ref) ) return MAX_PIP;

    return pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
pip_t * prt_get_ppip( Uint16 iprt )
{
    prt_t * pprt;

    if( !ACTIVE_PRT(iprt) ) return NULL;
    pprt = PrtList.lst + iprt;

    if( INVALID_PIP(pprt->pip_ref) ) return NULL;

    return PipStack.lst + pprt->pip_ref;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void init_all_pip()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        memset( PipStack.lst + cnt, 0, sizeof(pip_t) );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_pip()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        release_one_pip( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_pip( Uint16 ipip )
{
    pip_t * ppip;

    if( !VALID_PIP_RANGE(ipip) ) return bfalse;
    ppip = PipStack.lst + ipip;


    if( !ppip->loaded ) return btrue;

    memset( ppip, 0, sizeof(pip_t) );

    ppip->loaded  = bfalse;
    ppip->name[0] = '\0';

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_request_terminate( Uint16 iprt )
{
    if( !ACTIVE_PRT(iprt) ) return bfalse;

    EGO_OBJECT_TERMINATE( PrtList.lst + iprt );

    return btrue;
}