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

/// @file particle.c
/// @brief Manages particle systems.

#include "particle.inl"

#include "PrtList.h"

#include "log.h"
#include "sound.h"
#include "camera.h"
#include "game.h"
#include "mesh.h"
#include "network.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include "egoboo_mem.h"

#include "enchant.inl"
#include "mad.h"
#include "profile.inl"
#include "physics.inl"
#include "mesh.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PRT_TRANS 0x80

const float buoyancy_friction = 0.2f;          // how fast does a "cloud-like" object slow down?

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int ego_prt::stoppedby_tests = 0;
int ego_prt::pressure_tests = 0;

t_cpp_stack< ego_pip, MAX_PIP  > PipStack;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int prt_do_end_spawn( const PRT_REF & iprt );
static int prt_do_contspawn( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * prt_do_bump_damage( ego_prt_bundle * pbdl_prt );

static ego_prt_bundle * prt_update_animation( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * prt_update_dynalight( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * prt_update_timers( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * prt_update_do_water( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * prt_update_ingame( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * prt_update_limbo( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * prt_update( ego_prt_bundle * pbdl_prt );

static ego_prt_bundle * move_one_particle_get_environment( ego_prt_bundle * pbdl_prt, ego_prt_environment * penviro );
static ego_prt_bundle * move_one_particle_do_fluid_friction( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * move_one_particle_do_homing( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * move_one_particle_do_z_motion( ego_prt_bundle * pbdl_prt );
static ego_prt_bundle * move_one_particle_do_floor( ego_prt_bundle * pbdl_prt );

// private functions to request the "deletion" of particles
static bool_t    _prt_request_terminate( ego_prt_bundle * pbdl_prt );
static bool_t    _prt_request_terminate_ref( const PRT_REF & iprt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_prt_data * ego_prt_data::alloc( ego_prt_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    /* add something here */

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_prt_data * ego_prt_data::dealloc( ego_prt_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    /* add something here */

    return pdata;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//ego_prt * ego_prt::dealloc( ego_prt * pprt )
//{
//    if ( !ego_prt_data::dealloc( pprt ) ) return pprt;
//
//    // do not allow this if you are inside a particle loop
//    EGOBOO_ASSERT( 0 == PrtObjList.loop_depth );
//
//    if ( TERMINATED_PPRT( pprt ) ) return pprt;
//
//    // deallocate any dynamic data
//    ego_BSP_leaf::dtor_this( &( pprt->bsp_leaf ) );
//
//    return pprt;
//}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_prt_data * ego_prt_data::ctor_this( ego_prt_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    // set the critical values to safe ones
    pdata = ego_prt_data::init( pdata );
    if ( NULL == pdata ) return NULL;

    // construct/allocate any dynamic data
    pdata = ego_prt_data::alloc( pdata );
    if ( NULL == pdata ) return NULL;

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_prt_data * ego_prt_data::dtor_this( ego_prt_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    // destruct/free any dynamic data
    pdata = ego_prt_data::dealloc( pdata );
    if ( NULL == pdata ) return NULL;

    // set the critical values to safe ones
    pdata = ego_prt_data::init( pdata );
    if ( NULL == pdata ) return NULL;

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_prt_data * ego_prt_data::init( ego_prt_data * pdata )
{
    /// BB@> Set all particle parameters to safe values.
    ///      @details The c equivalent of the particle prt::new() function.

    if ( NULL == pdata ) return pdata;

    // "no lifetime" = "eternal"
    pdata->lifetime           = ( size_t )( ~0 );
    pdata->lifetime_remaining = pdata->lifetime;
    pdata->frames_remaining   = ( size_t )( ~0 );

    pdata->pip_ref      = MAX_PIP;
    pdata->profile_ref  = MAX_PROFILE;

    pdata->attachedto_ref = CHR_REF( MAX_CHR );
    pdata->owner_ref      = CHR_REF( MAX_CHR );
    pdata->target_ref     = CHR_REF( MAX_CHR );
    pdata->parent_ref     = MAX_PRT;
    pdata->parent_guid    = 0xFFFFFFFF;

    pdata->onwhichplatform_ref    = CHR_REF( MAX_CHR );
    pdata->onwhichplatform_update = 0;
    pdata->targetplatform_ref     = CHR_REF( MAX_CHR );

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::ctor_this( ego_prt * pprt )
{
    /// BB@> Set all particle parameters to safe values.
    ///      @details The c equivalent of the particle prt::new() function.

    pprt = ego_prt::alloc( pprt );
    if ( NULL == pprt ) return pprt;

    // set the ego_BSP_leaf data
    pprt->bsp_leaf.data      = pprt;
    pprt->bsp_leaf.data_type = LEAF_PRT;
    pprt->bsp_leaf.index     = GET_IDX_PPRT( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::dtor_this( ego_prt * pprt )
{
    if ( NULL == pprt ) return pprt;

    // destruct/free any dynamic data
    pprt = ego_prt::dealloc( pprt );
    if ( NULL == pprt ) return pprt;

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::alloc( ego_prt * pprt )
{
    if ( NULL == pprt ) return pprt;

    // initialize the bsp node for this particle
    ego_BSP_leaf::ctor_this( &( pprt->bsp_leaf ), 3, pprt, LEAF_PRT );

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::dealloc( ego_prt * pprt )
{
    if ( NULL == pprt ) return pprt;

    // initialize the bsp node for this particle
    ego_BSP_leaf::dtor_this( &( pprt->bsp_leaf ) );

    return pprt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void play_particle_sound( const PRT_REF & particle, Sint8 sound )
{
    /// ZZ@> This function plays a sound effect for a particle

    ego_prt * pprt;

    if ( !DEFINED_PRT( particle ) ) return;
    pprt = PrtObjList.get_pdata( particle );

    if ( !VALID_SND( sound ) ) return;

    if ( LOADED_PRO( pprt->profile_ref ) )
    {
        sound_play_chunk( ego_prt::get_pos( pprt ), pro_get_chunk( pprt->profile_ref, sound ) );
    }
    else
    {
        sound_play_chunk( ego_prt::get_pos( pprt ), g_wavelist[sound] );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_in_game( const PRT_REF & particle )
{
    /// @details ZZ@> This function sticks a particle back on the free particle stack and
    ///    plays the sound associated with the particle
    ///
    /// @note BB@> Use prt_request_free() instead of calling this function directly.
    ///            Requesting termination will defer the actual deletion of a particle until
    ///            it is finally destroyed by cleanup_all_particles()

    CHR_REF child;

    ego_prt * pprt = PrtObjList.get_valid_pdata( particle );
    if ( NULL == pprt ) return;

    if ( DEFINED_PPRT( pprt ) )
    {
        // the particle has valid data

        if ( pprt->spawncharacterstate )
        {
            child = spawn_one_character( ego_prt::get_pos( pprt ), pprt->profile_ref, pprt->team, 0, pprt->facing, NULL, CHR_REF( MAX_CHR ) );
            if ( INGAME_CHR( child ) )
            {
                ego_chr::get_pai( child )->state = pprt->spawncharacterstate;
                ego_chr::get_pai( child )->owner = pprt->owner_ref;
            }
        }

        if ( LOADED_PIP( pprt->pip_ref ) )
        {
            play_particle_sound( particle, PipStack.lst[pprt->pip_ref].end_sound );
        }
    }

    _prt_request_terminate_ref( particle );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::do_construct( ego_prt * pprt )
{
    // this object has already been constructed as a part of the
    // ego_obj_prt, so its parent is properly defined

    ego_prt * rv = ego_prt::ctor_all( pprt, pprt->get_pparent() );

    /* add something here */

    return rv;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::do_init( ego_prt * pprt )
{
    PRT_REF            iprt;
    ego_pip            * ppip;
    ego_prt_spawn_data * pdata;

    int     velocity;
    fvec3_t vel;
    float   tvel;
    int     offsetfacing = 0, newrand;
    Uint32  prt_lifetime;
    fvec3_t tmp_pos;
    Uint16  turn;
    float   loc_spdlimit;

    FACING_T loc_facing;
    CHR_REF loc_chr_origin;

    if ( NULL == pprt ) return NULL;
    pdata = &( pprt->spawn_data );
    iprt  = GET_IDX_PPRT( pprt );

    // Convert from local pdata->ipip to global pdata->ipip
    if ( !LOADED_PIP( pdata->ipip ) )
    {
        log_debug( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                   ( pdata->ipip ).get_value(), ( pdata->chr_origin ).get_value(), DEFINED_CHR( pdata->chr_origin ) ? ChrObjList.get_data( pdata->chr_origin ).name : "INVALID",
                   ( pdata->iprofile ).get_value(), LOADED_PRO( pdata->iprofile ) ? ProList.lst[pdata->iprofile].name : "INVALID" );

        return NULL;
    }
    ppip = PipStack.lst + pdata->ipip;

    // make some local copies of the spawn data
    loc_facing     = pdata->facing;

    // Save a version of the position for local use.
    // In cpp, will be passed by reference, so we do not want to alter the
    // components of the original vector.
    tmp_pos = pdata->pos;

    // try to get an idea of who our owner is even if we are
    // given bogus info
    loc_chr_origin = pdata->chr_origin;
    if ( !DEFINED_CHR( pdata->chr_origin ) && DEFINED_PRT( pdata->prt_origin ) )
    {
        loc_chr_origin = ego_prt::get_iowner( pdata->prt_origin, 0 );
    }

    pprt->pip_ref     = pdata->ipip;
    pprt->profile_ref = pdata->iprofile;
    pprt->team        = pdata->team;
    pprt->owner_ref   = loc_chr_origin;
    pprt->parent_ref  = pdata->prt_origin;
    pprt->parent_guid = VALID_PRT( pdata->prt_origin ) ? PrtObjList.get_obj( pdata->prt_origin ).guid : (( Uint32 )( ~0 ) );
    pprt->damagetype  = ppip->damagetype;
    pprt->lifedrain   = ppip->lifedrain;
    pprt->manadrain   = ppip->manadrain;

    // Lighting and sound
    pprt->dynalight    = ppip->dynalight;
    pprt->dynalight.on = bfalse;
    if ( 0 == pdata->multispawn )
    {
        pprt->dynalight.on = ppip->dynalight.mode;
        if ( DYNA_MODE_LOCAL == ppip->dynalight.mode )
        {
            pprt->dynalight.on = DYNA_MODE_OFF;
        }
    }

    // Set character attachments ( pdata->chr_attach==MAX_CHR means none )
    pprt->attachedto_ref     = pdata->chr_attach;
    pprt->attachedto_vrt_off = pdata->vrt_offset;

    // Correct loc_facing
    loc_facing += ppip->facing_pair.base;

    // Targeting...
    pprt->is_homing = bfalse;
    vel.z = 0;
    pprt->offset.z = generate_randmask( ppip->spacing_vrt_pair.base, ppip->spacing_vrt_pair.rand ) - ( ppip->spacing_vrt_pair.rand >> 1 );
    tmp_pos.z += pprt->offset.z;
    velocity = generate_randmask( ppip->vel_hrz_pair.base, ppip->vel_hrz_pair.rand );
    pprt->target_ref = pdata->oldtarget;
    if ( ppip->newtargetonspawn )
    {
        if ( ppip->targetcaster )
        {
            // Set the target to the caster
            pprt->target_ref = loc_chr_origin;
        }
        else
        {
            // Find a target
            pprt->target_ref = prt_find_target( pdata->pos.x, pdata->pos.y, pdata->pos.z, loc_facing, pdata->ipip, pdata->team, loc_chr_origin, pdata->oldtarget );
            if ( DEFINED_CHR( pprt->target_ref ) && !ppip->homing )
            {
                loc_facing -= glo_useangle;
                /// @note ZF@> ?What does this do?!
                /// @note BB@> glo_useangle is the angle found in prt_find_target()
            }

            // Correct loc_facing for dexterity...
            offsetfacing = 0;
            if ( ChrObjList.get_data( loc_chr_origin ).dexterity < PERFECTSTAT )
            {
                // Correct loc_facing for randomness
                offsetfacing  = generate_randmask( 0, ppip->facing_pair.rand ) - ( ppip->facing_pair.rand >> 1 );
                offsetfacing  = ( offsetfacing * ( PERFECTSTAT - ChrObjList.get_data( loc_chr_origin ).dexterity ) ) / PERFECTSTAT;
            }

            if ( 0 != ppip->zaimspd && DEFINED_CHR( pprt->target_ref ) )
            {
                // These aren't velocities...  This is to do aiming on the Z axis
                if ( velocity > 0 )
                {
                    vel.x = ChrObjList.get_data( pprt->target_ref ).pos.x - pdata->pos.x;
                    vel.y = ChrObjList.get_data( pprt->target_ref ).pos.y - pdata->pos.y;
                    tvel = SQRT( vel.x * vel.x + vel.y * vel.y ) / velocity;  // This is the number of steps...
                    if ( tvel > 0 )
                    {
                        vel.z = ( ChrObjList.get_data( pprt->target_ref ).pos.z + ( ChrObjList.get_data( pprt->target_ref ).bump.height * 0.5f ) - tmp_pos.z ) / tvel;  // This is the vel.z alteration
                        if ( vel.z < -( ppip->zaimspd >> 1 ) ) vel.z = -( ppip->zaimspd >> 1 );
                        if ( vel.z > ppip->zaimspd ) vel.z = ppip->zaimspd;
                    }
                }
            }
        }

        // get an initial value for the is_homing variable
        pprt->is_homing = ppip->homing && !INGAME_CHR( pprt->attachedto_ref ) && INGAME_CHR( pprt->target_ref );

        // Does it go away?
        if ( !DEFINED_CHR( pprt->target_ref ) && ppip->needtarget )
        {
            // should this destroy the particle without ever activating it? I htink so.
            _prt_request_terminate_ref( iprt );
            return NULL;
        }

        // Start on top of target
        if ( DEFINED_CHR( pprt->target_ref ) && ppip->startontarget )
        {
            tmp_pos.x = ChrObjList.get_data( pprt->target_ref ).pos.x;
            tmp_pos.y = ChrObjList.get_data( pprt->target_ref ).pos.y;
        }
    }
    else
    {
        // Correct loc_facing for randomness
        offsetfacing = generate_randmask( 0,  ppip->facing_pair.rand ) - ( ppip->facing_pair.rand >> 1 );
    }
    loc_facing += offsetfacing;
    pprt->facing = loc_facing;

    // this is actually pointing in the opposite direction?
    turn = TO_TURN( loc_facing );

    // Location data from arguments
    newrand = generate_randmask( ppip->spacing_hrz_pair.base, ppip->spacing_hrz_pair.rand );
    pprt->offset.x = -turntocos[ turn ] * newrand;
    pprt->offset.y = -turntosin[ turn ] * newrand;

    tmp_pos.x += pprt->offset.x;
    tmp_pos.y += pprt->offset.y;

    tmp_pos.x = CLIP( tmp_pos.x, 0, PMesh->gmem.edge_x - 2 );
    tmp_pos.y = CLIP( tmp_pos.y, 0, PMesh->gmem.edge_y - 2 );

    ego_prt::set_pos( pprt, tmp_pos.v );
    pprt->pos_old  = tmp_pos;
    pprt->pos_stt  = tmp_pos;

    // Velocity data
    vel.x = -turntocos[ turn ] * velocity;
    vel.y = -turntosin[ turn ] * velocity;
    vel.z += generate_randmask( ppip->vel_vrt_pair.base, ppip->vel_vrt_pair.rand ) - ( ppip->vel_vrt_pair.rand >> 1 );
    pprt->vel = pprt->vel_old = pprt->vel_stt = vel;

    // Template values
    pprt->bump_size_stt = ppip->bump_size;
    pprt->type          = ppip->type;

    // Image data
    pprt->rotate        = generate_irand_pair( ppip->rotate_pair );
    pprt->rotate_add    = ppip->rotate_add;

    pprt->size_stt      = ppip->size_base;
    pprt->size_add      = ppip->size_add;

    pprt->image_stt     = UINT_TO_UFP8( ppip->image_base );
    pprt->image_add     = generate_irand_pair( ppip->image_add );
    pprt->image_max     = UINT_TO_UFP8( ppip->numframes );

    // figure out the actual particle lifetime
    prt_lifetime        = ppip->time;
    if ( ppip->end_lastframe && pprt->image_add != 0 )
    {
        if ( 0 == ppip->time )
        {
            // Part time is set to 1 cycle
            int frames = ( pprt->image_max / pprt->image_add ) - 1;
            prt_lifetime = frames;
        }
        else
        {
            // Part time is used to give number of cycles
            int frames = (( pprt->image_max / pprt->image_add ) - 1 );
            prt_lifetime = ppip->time * frames;
        }
    }

    // "no lifetime" = "eternal"
    if ( 0 == prt_lifetime )
    {
        pprt->lifetime           = ( size_t )( ~0 );
        pprt->lifetime_remaining = pprt->lifetime;
        pprt->is_eternal         = btrue;
    }
    else
    {
        // the lifetime is really supposed tp be in terms of frames, but
        // to keep the number of updates stable, the frames could lag.
        // sooo... we just rescale the prt_lifetime so that it will work with the
        // updates and cross our fingers
        pprt->lifetime           = ceil(( float ) prt_lifetime * ( float )TARGET_UPS / ( float )TARGET_FPS );
        pprt->lifetime_remaining = pprt->lifetime;
    }

    // make the particle display AT LEAST one frame, regardless of how many updates
    // it has or when someone requests for it to terminate
    pprt->frames_remaining = MAX( 1, prt_lifetime );

    // Damage stuff
    range_to_pair( ppip->damage, &( pprt->damage ) );

    // Spawning data
    pprt->contspawn_delay = ppip->contspawn_delay;
    if ( pprt->contspawn_delay != 0 )
    {
        pprt->contspawn_delay = 1;
        if ( DEFINED_CHR( pprt->attachedto_ref ) )
        {
            pprt->contspawn_delay++; // Because attachment takes an update before it happens
        }
    }

    // Sound effect
    play_particle_sound( iprt, ppip->soundspawn );

    // set up the particle transparency
    pprt->inst.alpha = 0xFF;
    switch ( pprt->inst.type )
    {
        case SPRITE_SOLID: break;
        case SPRITE_ALPHA: pprt->inst.alpha = PRT_TRANS; break;
        case SPRITE_LIGHT: break;
    }

    // is the spawn location safe?
    if ( 0 == prt_hit_wall( pprt, tmp_pos.v, NULL, NULL ) )
    {
        pprt->safe_pos   = tmp_pos;
        pprt->safe_valid = btrue;
        pprt->safe_grid  = pprt->onwhichgrid;
    }

    // estimate some parameters for buoyancy and air resistance
    loc_spdlimit = ppip->spdlimit;
    if ( 0.0f == loc_spdlimit ) loc_spdlimit = -STANDARD_GRAVITY / ( 1.0f - air_friction );

    {
        const float buoyancy_min       = 0.0f;
        const float buoyancy_max       = 2.0f * ABS( STANDARD_GRAVITY );
        const float air_resistance_min = 0.0f;
        const float air_resistance_max = 1.0f;

        // find the buoyancy, assuming that the air_resistance of the particle
        // is equal to air_friction at standard gravity
        pprt->buoyancy = -loc_spdlimit * ( 1.0f - air_friction ) - STANDARD_GRAVITY;
        pprt->buoyancy = CLIP( pprt->buoyancy, buoyancy_min, buoyancy_max );

        // reduce the buoyancy if the particle falls
        if ( loc_spdlimit >= 0.0f ) pprt->buoyancy *= 0.5f;

        // determine if there is any left-over air resistance
        pprt->air_resistance  = 1.0f - ( pprt->buoyancy + STANDARD_GRAVITY ) / -loc_spdlimit;
        pprt->air_resistance = CLIP( pprt->air_resistance, air_resistance_min, air_resistance_max );

        pprt->air_resistance /= air_friction;
        pprt->air_resistance = CLIP( pprt->air_resistance, 0.0f, 1.0f );
    }

    ego_prt::set_size( pprt, ppip->size_base );

#if EGO_DEBUG && defined(DEBUG_PRT_LIST)

    // some code to track all allocated particles, where they came from, how long they are going to last,
    // what they are being used for...
    log_debug( "spawn_one_particle() - spawned a particle %d\n"
               "\tupdate == %d, last update == %d, frame == %d, minimum frame == %d\n"
               "\towner == %d(\"%s\")\n"
               "\tpip == %d(\"%s\")\n"
               "\t\t%s"
               "\tprofile == %d(\"%s\")\n"
               "\n",
               iprt,
               update_wld, pprt->time_update, frame_all, pprt->time_frame,
               loc_chr_origin, DEFINED_CHR( loc_chr_origin ) ? ChrObjList.get_data( loc_chr_origin ).name : "INVALID",
               pdata->ipip, ( NULL != ppip ) ? ppip->name : "INVALID", ( NULL != ppip ) ? ppip->comment : "",
               pdata->profile_ref, LOADED_PRO( pdata->profile_ref ) ? ProList.lst[pdata->profile_ref].name : "INVALID" );
#endif

    // count out all the requests for this particle type
    ppip->prt_create_count++;

    if ( MAX_CHR != pprt->attachedto_ref )
    {
        ego_prt_bundle prt_bdl;

        ego_prt_bundle::set( &prt_bdl, pprt );

        attach_one_particle( &prt_bdl );
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::do_process( ego_prt * pprt )
{
    // is there ever a reason to change the state?

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::do_deinit( ego_prt * pprt )
{
    if ( NULL == pprt ) return pprt;

    /* nothing to do yet */

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::do_destruct( ego_prt * pprt )
{
    ego_prt * rv = ego_prt::dtor_all( pprt );

    /* add something here */

    return rv;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::run_construct( ego_obj_prt * pobj, int max_iterations )
{
    int                 iterations;
    ego_obj * pbase;

    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->get_proc().action > ( int )( ego_obj_constructing + 1 ) )
    {
        ego_obj_prt * tmp_prt = ego_obj_prt::run_deconstruct( pobj, max_iterations );
        if ( tmp_prt == pobj ) return NULL;
    }

    iterations = 0;
    while ( NULL != pobj && pbase->get_proc().action <= ego_obj_constructing && iterations < max_iterations )
    {
        ego_obj_prt * ptmp = ego_obj_prt::run( pobj );
        if ( ptmp != pobj ) return NULL;
        iterations++;
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::run_initialize( ego_obj_prt * pobj, int max_iterations )
{
    int                 iterations;
    ego_obj * pbase;

    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->get_proc().action > ( int )( ego_obj_initializing + 1 ) )
    {
        ego_obj_prt * tmp_prt = ego_obj_prt::run_deconstruct( pobj, max_iterations );
        if ( tmp_prt == pobj ) return NULL;
    }

    iterations = 0;
    while ( NULL != pobj && pbase->get_proc().action <= ego_obj_initializing && iterations < max_iterations )
    {
        ego_obj_prt * ptmp = ego_obj_prt::run( pobj );
        if ( ptmp != pobj ) return NULL;
        iterations++;
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::run_activate( ego_obj_prt * pobj, int max_iterations )
{
    int          iterations;
    ego_obj * pbase;

    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->get_proc().action > ( int )( ego_obj_processing + 1 ) )
    {
        ego_obj_prt * tmp_prt = ego_obj_prt::run_deconstruct( pobj, max_iterations );
        if ( tmp_prt == pobj ) return NULL;
    }

    iterations = 0;
    while ( NULL != pobj && pbase->get_proc().action < ego_obj_processing && iterations < max_iterations )
    {
        ego_obj_prt * ptmp = ego_obj_prt::run( pobj );
        if ( ptmp != pobj ) return NULL;
        iterations++;
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::run_deinitialize( ego_obj_prt * pobj, int max_iterations )
{
    int                 iterations;
    ego_obj * pbase;

    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the particle is already beyond this stage, deinitialize it
    if ( pbase->get_proc().action > ( int )( ego_obj_deinitializing + 1 ) )
    {
        return pobj;
    }
    else if ( pbase->get_proc().action < ego_obj_deinitializing )
    {
        ego_obj::end_processing( pbase );
    }

    iterations = 0;
    while ( NULL != pobj && pbase->get_proc().action <= ego_obj_deinitializing && iterations < max_iterations )
    {
        ego_obj_prt * ptmp = ego_obj_prt::run( pobj );
        if ( ptmp != pobj ) return NULL;
        iterations++;
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::run_deconstruct( ego_obj_prt * pobj, int max_iterations )
{
    int                 iterations;
    ego_obj * pbase;

    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the particle is already beyond this stage, deconstruct it
    if ( pbase->get_proc().action > ( int )( ego_obj_destructing + 1 ) )
    {
        return pobj;
    }
    else if ( pbase->get_proc().action < ego_obj_destructing )
    {
        // make sure that you deinitialize before destructing
        ego_obj::end_processing( pbase );
    }

    iterations = 0;
    while ( NULL != pobj && pbase->get_proc().action <= ego_obj_destructing && iterations < max_iterations )
    {
        ego_obj_prt * ptmp = ego_obj_prt::run( pobj );
        if ( ptmp != pobj ) return NULL;
        iterations++;
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::run( ego_obj_prt * pobj )
{
    ego_obj * pbase;

    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( FLAG_REQ_TERMINATION_PBASE( pbase ) )
    {
        pbase = ego_obj::grant_terminate( pbase );
    }

    switch ( pbase->get_proc().action )
    {
        default:
        case ego_obj_nothing:
            /* no operation */
            break;

        case ego_obj_constructing:
            pobj = ego_obj_prt::do_constructing( pobj );
            break;

        case ego_obj_initializing:
            pobj = ego_obj_prt::do_initializing( pobj );
            break;

        case ego_obj_processing:
            pobj = ego_obj_prt::do_processing( pobj );
            break;

        case ego_obj_deinitializing:
            pobj = ego_obj_prt::do_deinitializing( pobj );
            break;

        case ego_obj_destructing:
            pobj = ego_obj_prt::do_destructing( pobj );
            break;

        case ego_obj_waiting:
            /* do nothing */
            break;
    }

    if ( NULL == pobj )
    {
        pbase->update_guid = INVALID_UPDATE_GUID;
    }
    else if ( ego_obj_processing == pbase->get_proc().action )
    {
        pbase->update_guid = PrtObjList.update_guid();
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::do_constructing( ego_obj_prt * pobj )
{
    ego_obj * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( pbase ) ) return pobj;

    // run the constructor
    ego_prt * pprt = ego_prt::do_construct( pobj->get_pdata() );
    if ( NULL == pprt ) return NULL;

    // move on to the next action
    ego_obj::end_constructing( pbase );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::do_initializing( ego_obj_prt * pobj )
{
    ego_obj * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_INITIALIZING_PBASE( pbase ) ) return pobj;

    // tell the game that we're spawning something
    POBJ_BEGIN_SPAWN( pobj );

    // run the initialization routine
    ego_prt * pprt = ego_prt::do_init( pobj->get_pdata() );
    if ( NULL == pprt ) return pobj;

    // request that we be turned on
    pbase->proc_req_on( btrue );

    // do something about being turned on
    if ( 0 == PrtObjList.loop_depth )
    {
        ego_obj::grant_on( pbase );
    }
    else
    {
        PrtObjList.add_activation( GET_REF_PPRT_OBJ( pobj ) );
    }

    // move on to the next action
    ego_obj::end_initializing( pbase );

    if ( !LOADED_PIP( pobj->get_data().pip_ref ) )
    {
        POBJ_ACTIVATE( pobj, "*UNKNOWN*" );
    }
    else
    {
        ego_pip * ppip = PipStack.lst + pobj->get_data().pip_ref ;

        POBJ_ACTIVATE( pobj, ppip->name );
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::do_processing( ego_obj_prt * pobj )
{
    // there's nothing to configure if the object is active...

    ego_obj * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_PROCESSING_PBASE( pbase ) ) return pobj;

    // do this here (instead of at the end of *_do_object_initializing()) so that
    // we are sure that the object is actually "on"
    POBJ_END_SPAWN( pobj );

    // run the main loop
    ego_prt * pprt = ego_prt::do_process( pobj->get_pdata() );
    if ( NULL == pprt ) return pobj;

    /* add stuff here */

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::do_deinitializing( ego_obj_prt * pobj )
{
    /// @details BB@> deinitialize the character data

    ego_obj * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return pobj;

    // make sure that the spawn is terminated
    POBJ_END_SPAWN( pobj );

    // run a deinitialization routine
    ego_prt * pprt = ego_prt::do_deinit( pobj->get_pdata() );
    if ( NULL == pprt ) return pobj;

    // move on to the next action
    ego_obj::end_deinitializing( pbase );

    // make sure the object is off
    pbase->get_proc().on = bfalse;

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::do_destructing( ego_obj_prt * pobj )
{
    ego_obj * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pobj );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_DESTRUCTING_PBASE( pbase ) ) return pobj;

    // make sure that the spawn is terminated
    POBJ_END_SPAWN( pobj );

    // run the destructor
    ego_prt * pprt = ego_prt::do_destruct( pobj->get_pdata() );
    if ( NULL == pprt ) return pobj;

    // move on to the next action (dead)
    ego_obj::end_destructing( pbase );

    return pobj;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PRT_REF spawn_one_particle( fvec3_t pos, FACING_T facing, const PRO_REF & iprofile, int pip_index,
                            const CHR_REF & chr_attach, Uint16 vrt_offset, const TEAM_REF & team,
                            const CHR_REF & chr_origin, const PRT_REF & prt_origin, int multispawn, const CHR_REF & oldtarget )
{
    /// @details ZZ@> This function spawns a new particle.
    ///               Returns the index of that particle or MAX_PRT on a failure.

    PIP_REF ipip;
    PRT_REF iprt;

    ego_prt * pprt;
    ego_pip * ppip;

    // Convert from local ipip to global ipip
    ipip = pro_get_ipip( iprofile, pip_index );

    if ( !LOADED_PIP( ipip ) )
    {
        log_debug( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                   ( ipip ).get_value(), ( chr_origin ).get_value(), INGAME_CHR( chr_origin ) ? ChrObjList.get_data( chr_origin ).name : "INVALID",
                   ( iprofile ).get_value(), LOADED_PRO( iprofile ) ? ProList.lst[iprofile].name : "INVALID" );

        return PRT_REF( MAX_PRT );
    }
    ppip = PipStack.lst + ipip;

    // count all the requests for this particle type
    ppip->prt_request_count++;

    iprt = PrtObjList.allocate( ppip->force );
    if ( !DEFINED_PRT( iprt ) )
    {
#if EGO_DEBUG && defined(DEBUG_PRT_LIST)
        log_debug( "spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
                   chr_origin, INGAME_CHR( chr_origin ) ? ChrObjList.get_data( chr_origin ).name : "INVALID",
                   ipip, LOADED_PIP( ipip ) ? PipStack.lst[ipip].name : "INVALID",
                   iprofile, LOADED_PRO( iprofile ) ? ProList.lst[iprofile].name : "INVALID" );
#endif

        return PRT_REF( MAX_PRT );
    }

    pprt = PrtObjList.get_pdata( iprt );

    pprt->spawn_data.pos        = pos;
    pprt->spawn_data.facing     = facing;
    pprt->spawn_data.iprofile   = iprofile;
    pprt->spawn_data.ipip       = ipip;

    pprt->spawn_data.chr_attach = chr_attach;
    pprt->spawn_data.vrt_offset = vrt_offset;
    pprt->spawn_data.team       = team;

    pprt->spawn_data.chr_origin = chr_origin;
    pprt->spawn_data.prt_origin = prt_origin;
    pprt->spawn_data.multispawn = multispawn;
    pprt->spawn_data.oldtarget  = oldtarget;

    // actually force the character to spawn
    ego_obj_prt::run_activate( PDATA_GET_POBJ( ego_prt, pprt ), 100 );

    // count out all the requests for this particle type
    if ( NULL != pprt )
    {
        ppip->prt_create_count++;
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
float prt_calc_mesh_pressure( ego_prt * pprt, float test_pos[] )
{
    float retval = 0.0f;
    BIT_FIELD  stoppedby;
    ego_pip      * ppip;

    if ( !DEFINED_PPRT( pprt ) ) return retval;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return retval;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( 0 != ppip->bump_money ) ADD_BITS( stoppedby, MPDFX_WALL );

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = ego_prt::get_pos_v( pprt );
    if ( NULL == test_pos ) return 0;

    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::get_pressure( PMesh, test_pos, 0.0f, stoppedby );
    }
    ego_prt::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_prt::pressure_tests += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
fvec2_t prt_calc_diff( ego_prt * pprt, float test_pos[], float center_pressure )
{
    fvec2_t        retval = ZERO_VECT2;
    float        radius;
    BIT_FIELD   stoppedby;
    ego_pip      * ppip;

    if ( !DEFINED_PPRT( pprt ) ) return retval;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return retval;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( 0 != ppip->bump_money ) ADD_BITS( stoppedby, MPDFX_WALL );

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = ego_prt::get_pos_v( pprt );
    if ( NULL == test_pos ) return retval;

    // calculate the radius based on whether the particle is on camera
    /// @note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( ego_mpd::grid_is_valid( PMesh, pprt->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pprt->onwhichgrid ].inrenderlist )
        {
            radius = pprt->bump_real.size;
        }
    }

    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::get_diff( PMesh, test_pos, radius, center_pressure, stoppedby );
    }
    ego_prt::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_prt::pressure_tests += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD prt_hit_wall( ego_prt * pprt, float test_pos[], float nrm[], float * pressure )
{
    /// @details ZZ@> This function returns nonzero if the particle hit a wall that the
    ///    particle is not allowed to cross

    BIT_FIELD  retval;
    BIT_FIELD  stoppedby;
    ego_pip      * ppip;

    if ( !DEFINED_PPRT( pprt ) ) return 0;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return 0;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( 0 != ppip->bump_money ) ADD_BITS( stoppedby, MPDFX_WALL );

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = ego_prt::get_pos_v( pprt );
    if ( NULL == test_pos ) return 0;

    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::hit_wall( PMesh, test_pos, 0.0f, stoppedby, nrm, pressure );
    }
    ego_prt::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_prt::pressure_tests += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_test_wall( ego_prt * pprt, float test_pos[] )
{
    /// @details ZZ@> This function returns nonzero if the particle hit a wall that the
    ///    particle is not allowed to cross

    bool_t retval;
    ego_pip * ppip;
    BIT_FIELD  stoppedby;

    if ( !ACTIVE_PPRT( pprt ) ) return 0;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( 0 != ppip->bump_money ) ADD_BITS( stoppedby, MPDFX_WALL );

    // handle optional parameters
    if ( NULL == test_pos ) test_pos = ego_prt::get_pos_v( pprt );
    if ( NULL == test_pos ) return 0;

    // do the wall test
    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::test_wall( PMesh, test_pos, 0.0f, stoppedby, NULL );
    }
    ego_prt::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_prt::pressure_tests += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
void update_all_particles()
{
    /// @details BB@> main loop for updating particles. Do not use the
    ///               PRT_BEGIN_LOOP_* macro.
    ///               Converted all the update functions to the ego_obj_prt::run() paradigm.

    PRT_REF iprt;
    ego_prt_bundle prt_bdl;

    // activate any particles might have been generated last update in an in-active state
    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        ego_prt * pprt = PrtObjList.get_valid_pdata( iprt );
        if ( NULL == pprt ) continue;

        prt_update( ego_prt_bundle::set( &prt_bdl, pprt ) );
    }
}

//--------------------------------------------------------------------------------------------
void ego_prt::set_level( ego_prt * pprt, float level )
{
    float loc_height;

    if ( !INGAME_PPRT_BASE( pprt ) ) return;

    pprt->enviro.floor_level = level;

    loc_height = ego_prt::get_scale( pprt ) * MAX( UFP8_TO_FLOAT( pprt->size ), pprt->offset.z * 0.5 );

    pprt->enviro.grid_adj  = pprt->enviro.grid_level;
    pprt->enviro.floor_adj = pprt->enviro.floor_level;

    pprt->enviro.grid_adj  += loc_height;
    pprt->enviro.floor_adj += loc_height;

    // set the lerp after we have done everything to the particle's level we care to
    pprt->enviro.grid_lerp = ( pprt->pos.z - pprt->enviro.grid_adj ) / PLATTOLERANCE;
    pprt->enviro.grid_lerp = CLIP( pprt->enviro.grid_lerp, 0.0f, 1.0f );

    pprt->enviro.floor_lerp = ( pprt->pos.z - pprt->enviro.floor_adj ) / PLATTOLERANCE;
    pprt->enviro.floor_lerp = CLIP( pprt->enviro.floor_lerp, 0.0f, 1.0f );

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_calc_environment( ego_prt_bundle * pbdl )
{
    return move_one_particle_get_environment( pbdl, NULL );
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * move_one_particle_get_environment( ego_prt_bundle * pbdl_prt, ego_prt_environment * penviro )
{
    /// @details BB@> A helper function that gets all of the information about the particle's
    ///               environment (like friction, etc.) that will be necessary for the other
    ///               move_one_particle_*() functions to work

    Uint32 itile;
    float loc_level = 0.0f;

    ego_prt * loc_pprt;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;

    // handle the optional parameter
    if ( NULL == penviro ) penviro = &( loc_pprt->enviro );

    //---- particle "floor" level
    penviro->grid_level  = ego_mpd::get_level( PMesh, loc_pprt->pos.x, loc_pprt->pos.y );
    penviro->floor_level = penviro->grid_level;

    //---- The actual level of the character.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    loc_level = penviro->grid_level;
    if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        loc_level = MAX( penviro->grid_level, ChrObjList.get_data( loc_pprt->onwhichplatform_ref ).pos.z + ChrObjList.get_data( loc_pprt->onwhichplatform_ref ).chr_min_cv.maxs[OCT_Z] );
    }
    ego_prt::set_level( loc_pprt, loc_level );

    //---- the "twist" of the floor
    penviro->grid_twist = TWIST_FLAT;
    itile               = INVALID_TILE;
    if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        // this only works for 1 level of attachment
        itile = ChrObjList.get_data( loc_pprt->onwhichplatform_ref ).onwhichgrid;
    }
    else
    {
        itile = loc_pprt->onwhichgrid;
    }

    if ( ego_mpd::grid_is_valid( PMesh, itile ) )
    {
        penviro->grid_twist = PMesh->gmem.grid_list[itile].twist;
    }

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water.is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && ( 0 != ego_mpd::test_fx( PMesh, loc_pprt->onwhichgrid, MPDFX_SLIPPY ) );

    //---- traction
    penviro->traction = 1.0f;
    if ( loc_pprt->is_homing )
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        fvec3_t   platform_up;

        ego_chr::get_MatUp( ChrObjList.get_pdata( loc_pprt->onwhichplatform_ref ), &platform_up );
        fvec3_self_normalize( platform_up.v );

        penviro->traction = ABS( platform_up.z ) * ( 1.0f - penviro->floor_lerp ) + 0.25 * penviro->floor_lerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= hillslide * ( 1.0f - penviro->floor_lerp ) + 1.0f * penviro->floor_lerp;
        }
    }
    else if ( ego_mpd::grid_is_valid( PMesh, loc_pprt->onwhichgrid ) )
    {
        penviro->traction = ABS( map_twist_nrm[penviro->grid_twist].z ) * ( 1.0f - penviro->floor_lerp ) + 0.25 * penviro->floor_lerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= hillslide * ( 1.0f - penviro->floor_lerp ) + 1.0f * penviro->floor_lerp;
        }
    }

    //---- the friction of the fluid we are in
    if ( penviro->is_watery )
    {
        penviro->fluid_friction_vrt  = waterfriction;
        penviro->fluid_friction_hrz = waterfriction;
    }
    else
    {
        penviro->fluid_friction_hrz = penviro->air_friction;       // like real-life air friction
        penviro->fluid_friction_vrt  = penviro->air_friction;
    }

    //---- friction
    penviro->friction_hrz = 1.0f;
    if ( !loc_pprt->is_homing )
    {
        // Make the characters slide
        penviro->friction_hrz = noslipfriction;

        if ( ego_mpd::grid_is_valid( PMesh, loc_pprt->onwhichgrid ) && penviro->is_slippy )
        {
            // It's slippy all right...
            penviro->friction_hrz = slippyfriction;
        }
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * move_one_particle_do_fluid_friction( ego_prt_bundle * pbdl_prt )
{
    /// @details BB@> A helper function that computes particle friction with the floor
    ///
    /// @note this is pretty much ripped from the character version of this function and may
    ///       contain some features that are not necessary for any particles that are actually in game.
    ///       For instance, the only particles that is under their own control are the homing particles
    ///       but they do not have friction with the mesh, but that case is still treated in the code below.

    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    fvec3_t fluid_acc;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt    = pbdl_prt->prt_ptr;
    loc_ppip    = pbdl_prt->pip_ptr;
    loc_penviro = &( loc_pprt->enviro );
    loc_pphys   = &( loc_pprt->phys );

    // if the particle is homing in on something, ignore friction
    if ( loc_pprt->is_homing ) return pbdl_prt;

    // assume no acceleration
    fvec3_self_clear( fluid_acc.v );

    // Light isn't affected by fluid velocity
    if ( SPRITE_LIGHT != loc_pprt->type )
    {
        float loc_fluid_friction = loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;

        if ( loc_pprt->inwater )
        {
            fluid_acc = fvec3_sub( waterspeed.v, loc_pprt->vel.v );
            fvec3_self_scale( fluid_acc.v, 1.0f - loc_fluid_friction );
        }
        else
        {
            fluid_acc = fvec3_sub( windspeed.v, loc_pprt->vel.v );
            fvec3_self_scale( fluid_acc.v, 1.0f - loc_fluid_friction );
        }
    }

    phys_data_accumulate_avel( loc_pphys, fluid_acc.v );

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * move_one_particle_do_homing( ego_prt_bundle * pbdl_prt )
{
    ego_chr * ptarget;

    ego_prt             * loc_pprt;
    PRT_REF             loc_iprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt    = pbdl_prt->prt_ptr;
    loc_iprt    = pbdl_prt->prt_ref;
    loc_ppip    = pbdl_prt->pip_ptr;
    loc_penviro = &( loc_pprt->enviro );
    loc_pphys   = &( loc_pprt->phys );

    if ( !loc_pprt->is_homing || !INGAME_CHR( loc_pprt->target_ref ) ) return pbdl_prt;

    if ( !INGAME_CHR( loc_pprt->target_ref ) )
    {
        goto move_one_particle_do_homing_fail;
    }
    ptarget = ChrObjList.get_pdata( loc_pprt->target_ref );

    if ( !ptarget->alive )
    {
        goto move_one_particle_do_homing_fail;
    }
    else if ( !INGAME_CHR( loc_pprt->attachedto_ref ) )
    {
        int       ival;
        float     vlen, min_length, uncertainty;
        fvec3_t   vdiff, vdither;

        vdiff = fvec3_sub( ptarget->pos.v, ego_prt::get_pos_v( loc_pprt ) );
        vdiff.z += ptarget->bump.height * 0.5f;

        min_length = ( 2 * 5 * 256 * ChrObjList.get_data( loc_pprt->owner_ref ).wisdom ) / PERFECTBIG;

        // make a little uncertainty about the target
        uncertainty = 256 - ( 256 * ChrObjList.get_data( loc_pprt->owner_ref ).intelligence ) / PERFECTBIG;

        ival = RANDIE;
        vdither.x = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

        ival = RANDIE;
        vdither.y = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

        ival = RANDIE;
        vdither.z = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

        // take away any dithering along the direction of motion of the particle
        vlen = fvec3_dot_product( loc_pprt->vel.v, loc_pprt->vel.v );
        if ( vlen > 0.0f )
        {
            float vdot = fvec3_dot_product( vdither.v, loc_pprt->vel.v ) / vlen;

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
        vlen = fvec3_length_abs( vdiff.v );
        if ( vlen != 0.0f )
        {
            float factor = min_length / vlen;

            vdiff.x *= factor;
            vdiff.y *= factor;
            vdiff.z *= factor;
        }

        {
            fvec3_t _tmp_vec_1 = fvec3_scale( loc_pprt->vel.v, -( 1.0f - loc_ppip->homingfriction ) );
            fvec3_t _tmp_vec_2 = fvec3_scale( vdiff.v, loc_ppip->homingaccel * loc_ppip->homingfriction );

            fvec3_self_sum( _tmp_vec_1.v, _tmp_vec_2.v );

            phys_data_accumulate_avel( loc_pphys, _tmp_vec_1.v );
        }
    }

    return pbdl_prt;

move_one_particle_do_homing_fail:

    prt_request_free( pbdl_prt );

    return NULL;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * move_one_particle_do_z_motion( ego_prt_bundle * pbdl_prt )
{
    /// @details BB@> A helper function that does gravitational acceleration and buoyancy

    float loc_zlerp;

    ego_prt             * loc_pprt;
    PRT_REF             loc_iprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt    = pbdl_prt->prt_ptr;
    loc_iprt    = pbdl_prt->prt_ref;
    loc_ppip    = pbdl_prt->pip_ptr;
    loc_penviro = &( loc_pprt->enviro );
    loc_pphys   = &( loc_pprt->phys );

    /// @note ZF@> We really can't do gravity for Light! A lot of magical effects and attacks in the game depend on being able
    ///            to move forward in a straight line without being dragged down into the dust!
    /// @note BB@> however, the fireball particle is light, and without gravity it will never bounce on the
    ///            ground as it is supposed to
    if ( /* loc_pprt->type == SPRITE_LIGHT || */ loc_pprt->is_homing || INGAME_CHR( loc_pprt->attachedto_ref ) ) return pbdl_prt;

    loc_zlerp = CLIP( loc_penviro->floor_lerp, 0.0f, 1.0f );

    // Do particle buoyancy. This is kinda BS the way it is calculated
    if ( loc_pprt->buoyancy > 0.0f )
    {
        float loc_buoyancy = loc_pprt->buoyancy  + ( STANDARD_GRAVITY - gravity );

        if ( loc_zlerp < 1.0f )
        {
            // the particle is close to the ground
            if ( loc_pprt->buoyancy + gravity < 0.0f )
            {
                // the particle is not bouyant enough to hold itself up.
                // this means that the normal force will overcome it as it gets close to the ground
                // and the force needs to disappear close to the ground
                loc_buoyancy *= loc_zlerp;
            }
            else
            {
                // the particle floats up in the air. it does not reduce its upward
                // acceleration as we get closer to the floor.
                loc_buoyancy += loc_zlerp * gravity;
            }
        }

        phys_data_accumulate_avel_index( loc_pphys,  loc_buoyancy, kZ );
    }

    // do gravity
    phys_data_accumulate_avel_index( loc_pphys,  loc_zlerp * gravity, kZ );

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * move_one_particle_do_floor( ego_prt_bundle * pbdl_prt )
{
    /// @details BB@> A helper function that computes particle friction with the floor
    ///
    /// @note this is pretty much ripped from the character version of this function and may
    ///       contain some features that are not necessary for any particles that are actually in game.
    ///       For instance, the only particles that is under their own control are the homing particles
    ///       but they do not have friction with the mesh, but that case is still treated in the code below.

    float temp_friction_xy;
    fvec3_t vup, floor_acc, fric, fric_floor;

    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt     = pbdl_prt->prt_ptr;
    loc_ppip     = pbdl_prt->pip_ptr;
    loc_penviro  = &( loc_pprt->enviro );
    loc_pphys    = &( loc_pprt->phys );

    // no friction for attached particles
    if ( ACTIVE_CHR( loc_pprt->attachedto_ref ) ) return pbdl_prt;

    // determine the surface normal for the particle's "floor"
    if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        ego_chr::get_MatUp( ChrObjList.get_pdata( loc_pprt->onwhichplatform_ref ), &vup );
    }
    else if ( TWIST_FLAT != loc_penviro->grid_twist )
    {
        vup = map_twist_nrm[loc_penviro->grid_twist];
    }
    else
    {
        vup.x = vup.y = 0.0f;
        vup.z = -SGN( gravity );
    }

    // only solid particles that are not "homing"
    // have normal floor friction
    if ( !loc_pprt->is_homing && SPRITE_SOLID == loc_pprt->type )
    {
        // figure out the acceleration due to the current "floor"
        fvec3_self_clear( floor_acc.v );
        temp_friction_xy = 1.0f;
        if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
        {
            ego_chr * pplat = ChrObjList.get_pdata( loc_pprt->onwhichplatform_ref );

            temp_friction_xy = platstick;

            floor_acc.x = pplat->vel.x - pplat->vel_old.x;
            floor_acc.y = pplat->vel.y - pplat->vel_old.y;
            floor_acc.z = pplat->vel.z - pplat->vel_old.z;
        }
        else
        {
            temp_friction_xy = 0.5f;
            floor_acc.x = -loc_pprt->vel.x;
            floor_acc.y = -loc_pprt->vel.y;
            floor_acc.z = -loc_pprt->vel.z;
        }

        // the first guess about the floor friction
        fric_floor.x = floor_acc.x * ( 1.0f - loc_penviro->floor_lerp ) * ( 1.0f - temp_friction_xy ) * loc_penviro->traction;
        fric_floor.y = floor_acc.y * ( 1.0f - loc_penviro->floor_lerp ) * ( 1.0f - temp_friction_xy ) * loc_penviro->traction;
        fric_floor.z = floor_acc.z * ( 1.0f - loc_penviro->floor_lerp ) * ( 1.0f - temp_friction_xy ) * loc_penviro->traction;

        // the total "friction" due to the floor
        fric.x = fric_floor.x + loc_penviro->acc.x;
        fric.y = fric_floor.y + loc_penviro->acc.y;
        fric.z = fric_floor.z + loc_penviro->acc.z;

        //---- limit the friction to whatever is horizontal to the mesh
        if ( TWIST_FLAT == loc_penviro->grid_twist )
        {
            floor_acc.z = 0.0f;
            fric.z      = 0.0f;
        }
        else
        {
            float ftmp;
            fvec3_t   mesh_up = map_twist_nrm[loc_penviro->grid_twist];

            ftmp = fvec3_dot_product( floor_acc.v, mesh_up.v );

            floor_acc.x -= ftmp * mesh_up.x;
            floor_acc.y -= ftmp * mesh_up.y;
            floor_acc.z -= ftmp * mesh_up.z;

            ftmp = fvec3_dot_product( fric.v, mesh_up.v );

            fric.x -= ftmp * mesh_up.x;
            fric.y -= ftmp * mesh_up.y;
            fric.z -= ftmp * mesh_up.z;
        }

        // test to see if the player has any more friction left?
        loc_penviro->is_slipping = ( fvec3_length_abs( fric.v ) > loc_penviro->friction_hrz );

        if ( loc_penviro->is_slipping )
        {
            loc_penviro->traction *= 0.5f;
            temp_friction_xy  = SQRT( temp_friction_xy );

            fric_floor.x = floor_acc.x * ( 1.0f - loc_penviro->floor_lerp ) * ( 1.0f - temp_friction_xy ) * loc_penviro->traction;
            fric_floor.y = floor_acc.y * ( 1.0f - loc_penviro->floor_lerp ) * ( 1.0f - temp_friction_xy ) * loc_penviro->traction;
            fric_floor.z = floor_acc.z * ( 1.0f - loc_penviro->floor_lerp ) * ( 1.0f - temp_friction_xy ) * loc_penviro->traction;
        }

        // apply the floor friction
        phys_data_accumulate_avel( loc_pphys, fric_floor.v );
    }

    // all particles that are not "light" are supported by the floor
    if ( SPRITE_LIGHT != loc_pprt->type )
    {
        // apply the acceleration from the "normal force"
        phys_data_apply_normal_acceleration( loc_pphys, vup, 1.0f, loc_penviro->floor_lerp, NULL );
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
bool_t move_one_particle( ego_prt_bundle * pbdl_prt )
{
    /// @details BB@> The master function for controlling a particle's motion

    ego_prt             * loc_pprt;
    ego_prt_environment * loc_penviro;

    if ( NULL == pbdl_prt ) return bfalse;
    loc_pprt     = pbdl_prt->prt_ptr;
    loc_penviro  = &( loc_pprt->enviro );

    if ( !INGAME_PPRT_BASE( loc_pprt ) ) return bfalse;

    // if the particle is hidden it is frozen in time. do nothing.
    if ( loc_pprt->is_hidden ) return bfalse;

    // save the acceleration from the last time-step
    loc_penviro->acc = fvec3_sub( loc_pprt->vel.v, loc_pprt->vel_old.v );

    // Particle's old location
    loc_pprt->pos_old = ego_prt::get_pos( loc_pprt );
    loc_pprt->vel_old = loc_pprt->vel;

    // determine the actual velocity for attached particles
    if ( INGAME_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->vel = fvec3_sub( ego_prt::get_pos_v( loc_pprt ), loc_pprt->pos_old.v );
    }

    // what is the local environment like?
    pbdl_prt = move_one_particle_get_environment( pbdl_prt, NULL );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;

    // wind, current, and other fluid friction effects
    pbdl_prt = move_one_particle_do_fluid_friction( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;

    // particle homing effects
    pbdl_prt = move_one_particle_do_homing( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;

    // gravity effects
    pbdl_prt = move_one_particle_do_z_motion( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;

    // do friction with the floor last so we can guess about slipping
    pbdl_prt = move_one_particle_do_floor( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;

    //pbdl_prt = move_one_particle_integrate_motion( pbdl_prt );
    //if( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void move_all_particles( void )
{
    /// @details ZZ@> This is the particle physics function

    ego_prt::stoppedby_tests = 0;

    // move every particle
    PRT_BEGIN_LOOP_USED( cnt, prt_bdl )
    {
        // prime the environment
        prt_bdl.prt_ptr->enviro.air_friction = air_friction;
        prt_bdl.prt_ptr->enviro.ice_friction = ice_friction;

        move_one_particle( &prt_bdl );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void particle_system_begin()
{
    /// @details ZZ@> This function sets up particle data

    // Reset the allocation table
    PrtObjList.init();

    init_all_pip();
}

//--------------------------------------------------------------------------------------------
void particle_system_end()
{
    release_all_pip();

    PrtObjList.deinit();
}

//--------------------------------------------------------------------------------------------
int spawn_bump_particles( const CHR_REF & character, const PRT_REF & particle )
{
    /// @details ZZ@> This function is for catching characters on fire and such

    int      cnt, bs_count;
    float    x, y, z;
    FACING_T facing;
    int      amount;
    FACING_T direction;
    float    fsin, fcos;

    ego_pip * ppip;
    ego_chr * pchr;
    ego_mad * pmad;
    ego_prt * pprt;
    ego_cap * pcap;

    if ( !INGAME_PRT( particle ) ) return 0;
    pprt = PrtObjList.get_pdata( particle );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return 0;
    ppip = PipStack.lst + pprt->pip_ref;

    // no point in going on, is there?
    if ( 0 == ppip->bumpspawn_amount && !ppip->spawnenchant ) return 0;
    amount = ppip->bumpspawn_amount;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrObjList.get_pdata( character );

    pmad = ego_chr::get_pmad( character );
    if ( NULL == pmad ) return 0;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return 0;

    bs_count = 0;

    // Only damage if hitting from proper direction
    direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
    direction = ATK_BEHIND + ( pchr->ori.facing_z - direction );

    // Check that direction
    if ( !is_invictus_direction( direction, character, ppip->damfx ) )
    {
        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_one_enchant( pprt->owner_ref, character, CHR_REF( MAX_CHR ), ENC_REF( MAX_ENC ), pprt->profile_ref );
        }

        // Spawn particles - this has been modded to maximize the visual effect
        // on a given target. It is not the most optimal solution for lots of particles
        // spawning. That would probably be to make the distance calculations and then
        // to quicksort the list and choose the n closest points.
        //
        // however, it seems that the bump particles in game rarely attach more than
        // one bump particle
        if ( amount != 0 && !pcap->resistbumpspawn && !IS_INVICTUS_PCHR_RAW( pchr ) && GET_DAMAGE_RESIST( pchr->damagemodifier[pprt->damagetype] ) )
        {
            int grip_verts, vertices;
            int slot_count;

            slot_count = 0;
            if ( pcap->slotvalid[SLOT_LEFT] ) slot_count++;
            if ( pcap->slotvalid[SLOT_RIGHT] ) slot_count++;

            if ( slot_count == 0 )
            {
                grip_verts = 1;  // always at least 1?
            }
            else
            {
                grip_verts = GRIP_VERTS * slot_count;
            }

            vertices = ( int )pchr->inst.vrt_count - ( int )grip_verts;
            vertices = MAX( 0, vertices );

            if ( vertices != 0 )
            {
                PRT_REF *vertex_occupied;
                float   *vertex_distance;
                float    dist;
                TURN_T   turn;

                vertex_occupied = EGOBOO_NEW_ARY( PRT_REF, vertices );
                vertex_distance = EGOBOO_NEW_ARY( float,   vertices );

                // this could be done more easily with a quicksort....
                // but I guess it doesn't happen all the time
                dist = fvec3_dist_abs( ego_prt::get_pos_v( pprt ), ego_chr::get_pos_v( pchr ) );

                // clear the occupied list
                z = pprt->pos.z - pchr->pos.z;
                facing = pprt->facing - pchr->ori.facing_z;
                turn   = TO_TURN( facing );
                fsin = turntosin[ turn ];
                fcos = turntocos[ turn ];
                x = dist * fcos;
                y = dist * fsin;

                // prepare the array values
                for ( cnt = 0; cnt < vertices; cnt++ )
                {
                    dist = ABS( x - pchr->inst.vrt_lst[vertices-cnt-1].pos[XX] ) +
                           ABS( y - pchr->inst.vrt_lst[vertices-cnt-1].pos[YY] ) +
                           ABS( z - pchr->inst.vrt_lst[vertices-cnt-1].pos[ZZ] );

                    vertex_distance[cnt] = dist;
                    vertex_occupied[cnt] = MAX_PRT;
                }

                // determine if some of the vertex sites are already occupied
                PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
                {
                    if ( character != prt_bdl.prt_ptr->attachedto_ref ) continue;

                    if ( prt_bdl.prt_ptr->attachedto_vrt_off >= 0 && prt_bdl.prt_ptr->attachedto_vrt_off < vertices )
                    {
                        vertex_occupied[prt_bdl.prt_ptr->attachedto_vrt_off] = prt_bdl.prt_ref;
                    }
                }
                PRT_END_LOOP()

                // Find best vertices to attach the particles to
                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    PRT_REF bs_part;
                    Uint32  bestdistance;
                    int     bestvertex;

                    bestvertex   = 0;
                    bestdistance = 0xFFFFFFFF;         // Really high number

                    for ( cnt = 0; cnt < vertices; cnt++ )
                    {
                        if ( vertex_occupied[cnt] != MAX_PRT )
                            continue;

                        if ( vertex_distance[cnt] < bestdistance )
                        {
                            bestdistance = vertex_distance[cnt];
                            bestvertex   = cnt;
                        }
                    }

                    bs_part = spawn_one_particle( pchr->pos, 0, pprt->profile_ref, ppip->bumpspawn_pip,
                                                  character, bestvertex + 1, pprt->team, pprt->owner_ref, particle, cnt, character );

                    if ( VALID_PRT( bs_part ) )
                    {
                        vertex_occupied[bestvertex] = bs_part;
                        PrtObjList.get_data( bs_part ).is_bumpspawn = btrue;
                        bs_count++;
                    }
                }
                //}
                //else
                //{
                //    // Multiple particles are attached to character
                //    for ( cnt = 0; cnt < amount; cnt++ )
                //    {
                //        int irand = RANDIE;

                //        bs_part = spawn_one_particle( pchr->pos, 0, pprt->profile_ref, ppip->bumpspawn_pip,
                //                            character, irand % vertices, pprt->team, pprt->owner_ref, particle, cnt, character );

                //        if( VALID_PRT(bs_part) )
                //        {
                //            PrtObjList.get_data(bs_part).is_bumpspawn = btrue;
                //            bs_count++;
                //        }
                //    }
                //}

                EGOBOO_DELETE_ARY( vertex_occupied );
                EGOBOO_DELETE_ARY( vertex_distance );
            }
        }
    }

    return bs_count;
}

//--------------------------------------------------------------------------------------------
bool_t prt_is_over_water( const PRT_REF & iprt )
{
    /// ZZ@> This function returns btrue if the particle is over a water tile
    Uint32 fan;

    if ( !VALID_PRT( iprt ) ) return bfalse;

    fan = ego_mpd::get_tile( PMesh, PrtObjList.get_data( iprt ).pos.x, PrtObjList.get_data( iprt ).pos.y );
    if ( ego_mpd::grid_is_valid( PMesh, fan ) )
    {
        if ( 0 != ego_mpd::test_fx( PMesh, fan, MPDFX_WATER ) )  return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
PIP_REF PipStack_get_free()
{
    PIP_REF retval = PIP_REF( MAX_PIP );

    if ( PipStack.count < MAX_PIP )
    {
        retval = PipStack.count;
        PipStack.count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
PIP_REF load_one_particle_profile_vfs( const char *szLoadName, const PIP_REF & pip_override )
{
    /// @details ZZ@> This function loads a particle template, returning bfalse if the file wasn't
    ///    found

    PIP_REF ipip;
    ego_pip * ppip;

    ipip = PIP_REF( MAX_PIP );
    if ( VALID_PIP_RANGE( pip_override ) )
    {
        release_one_pip( pip_override );
        ipip = pip_override;
    }
    else
    {
        ipip = PipStack_get_free();
    }

    if ( !VALID_PIP_RANGE( ipip ) )
    {
        return PIP_REF( MAX_PIP );
    }
    ppip = PipStack.lst + ipip;

    if ( NULL == load_one_pip_file_vfs( szLoadName, ppip ) )
    {
        return PIP_REF( MAX_PIP );
    }

    ppip->end_sound = CLIP( ppip->end_sound, INVALID_SOUND, MAX_WAVE );
    ppip->soundspawn = CLIP( ppip->soundspawn, INVALID_SOUND, MAX_WAVE );

    return ipip;
}

//--------------------------------------------------------------------------------------------
void reset_particles( /* const char* modname */ )
{
    /// @details ZZ@> This resets all particle data and reads in the coin and water particles

    const char *loadpath;

    release_all_local_pips();
    release_all_pip();

    // Load in the standard global particles ( the coins for example )
    loadpath = "mp_data/1money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_COIN1 ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/5money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_COIN5 ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/25money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_COIN25 ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/100money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_COIN100 ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // Load module specific information
    loadpath = "mp_data/weather4.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_WEATHER4 ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/weather5.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_WEATHER5 ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/splash.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_SPLASH ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/ripple.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_RIPPLE ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // This is also global...
    loadpath = "mp_data/defend.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, PIP_REF( PIP_DEFEND ) ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    PipStack.count = GLOBAL_PIP_COUNT;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_pip()
{
    PIP_REF cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        pip_init( PipStack.lst + cnt );
    }

    // Reset the pip stack "pointer"
    PipStack.count = 0;
}

//--------------------------------------------------------------------------------------------
void release_all_pip()
{
    PIP_REF cnt;
    int tnc;
    int max_request;

    max_request = 0;
    for ( cnt = 0, tnc = 0; cnt < MAX_PIP; cnt++ )
    {
        if ( LOADED_PIP( cnt ) )
        {
            ego_pip * ppip = PipStack.lst + cnt;

            max_request = MAX( max_request, ppip->prt_request_count );
            tnc++;
        }
    }

    if ( tnc > 0 && max_request > 0 )
    {
        FILE * ftmp = fopen( vfs_resolveWriteFilename( "/debug/pip_usage.txt" ), "w" );
        if ( NULL != ftmp )
        {
            fprintf( ftmp, "List of used pips\n\n" );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                if ( LOADED_PIP( cnt ) )
                {
                    ego_pip * ppip = PipStack.lst + cnt;
                    fprintf( ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n", ( cnt ).get_value(), ppip->name, ppip->prt_create_count, ppip->prt_request_count );
                }
            }

            fflush( ftmp );

            fclose( ftmp );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                release_one_pip( cnt );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_pip( const PIP_REF & ipip )
{
    ego_pip * ppip;

    if ( !VALID_PIP_RANGE( ipip ) ) return bfalse;
    ppip = PipStack.lst + ipip;

    if ( !ppip->loaded ) return btrue;

    pip_init( ppip );

    ppip->loaded  = bfalse;
    ppip->name[0] = CSTR_END;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t _prt_request_terminate( ego_prt_bundle * pbdl_prt )
{
    bool_t retval;
    if ( NULL == pbdl_prt ) return bfalse;

    retval = _prt_request_terminate_ref( pbdl_prt->prt_ref );

    if ( retval )
    {
        ego_prt_bundle::validate( pbdl_prt );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t _prt_request_terminate_ref( const PRT_REF & iprt )
{
    /// @details BB@> Tell the game to get rid of this object and treat it
    ///               as if it was already dead
    ///
    /// @note _prt_request_terminate() will force the game to
    ///       (eventually) call PrtObjList.free_one() on this particle

    if ( !VALID_PRT( iprt ) || TERMINATED_PRT( iprt ) ) return bfalse;

    POBJ_REQUEST_TERMINATE( PrtObjList.get_valid_ptr( iprt ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_request_free( ego_prt_bundle * pbdl_prt )
{
    bool_t retval;
    if ( NULL == pbdl_prt ) return bfalse;

    retval = prt_request_free_ref( pbdl_prt->prt_ref );

    if ( retval )
    {
        ego_prt_bundle::validate( pbdl_prt );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_request_free_ref( const PRT_REF & iprt )
{
    /// @details BB@> Tell the game to get rid of this object and treat it
    ///               as if it was already dead
    ///
    /// @note prt_request_free() will force the game to
    ///       (eventually) call free_one_particle_in_game() on this particle

    ego_prt * pprt = NULL;

    pprt  = PrtObjList.get_valid_pdata( iprt );
    if ( NULL == pprt || TERMINATED_PPRT( pprt ) ) return bfalse;

    // wait for PrtObjList.cleanup() to work its magic
    ego_obj::begin_waiting( PDATA_GET_PBASE( ego_prt, pprt ) );

    // tell
    ego_obj_prt::set_limbo( PDATA_GET_POBJ( ego_prt, pprt ), btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
int prt_do_end_spawn( const PRT_REF & iprt )
{
    int end_spawn_count = 0;

    ego_prt * pprt = PrtObjList.get_valid_pdata( iprt );
    if ( NULL == pprt || pprt->end_spawn_pip < 0 ) return end_spawn_count;

    if ( pprt->end_spawn_pip < 0 ) return end_spawn_count;
    PIP_REF ipip = PIP_REF( pprt->end_spawn_pip );

    if ( pprt->end_spawn_amount <= 0 || !LOADED_PIP( ipip ) ) return end_spawn_count;

    // Spawn new particles if time for old one is up
    FACING_T facing;
    int      tnc;

    facing = pprt->facing;
    for ( tnc = 0; tnc < pprt->end_spawn_amount; tnc++ )
    {
        // we have been given the absolute pip reference when the particle was spawned
        // so, set the profile reference to PRO_REF(MAX_PROFILE), so that the
        // value of ipip will be used directly
        PRT_REF spawned_prt = spawn_one_particle( pprt->pos_old, facing, PRO_REF( MAX_PROFILE ), pprt->end_spawn_pip,
                              CHR_REF( MAX_CHR ), GRIP_LAST, pprt->team, ego_prt::get_iowner( iprt, 0 ), iprt, tnc, pprt->target_ref );

        if ( VALID_PRT( spawned_prt ) )
        {
            end_spawn_count++;
        }

        facing += pprt->end_spawn_facingadd;
    }

    // we have already spawned these particles, so set this amount to
    // zero in case we are not actually calling free_one_particle_in_game()
    // this time around.
    pprt->end_spawn_amount = 0;

    return end_spawn_count;
}

//--------------------------------------------------------------------------------------------
void cleanup_all_particles()
{
    // do end-of-life care for particles. Must iterate over all particles since the
    // number of particles could change inside this list
    for ( int cnt = 0; cnt < MAX_PRT; cnt++ )
    {
        bool_t time_out;

        ego_obj_prt * pobj = PrtObjList.get_ptr( PRT_REF( cnt ) );
        if ( NULL == pobj ) continue;

        ego_prt * pprt = pobj->get_pdata();
        if ( NULL == pprt ) continue;

        ego_obj * pbase = PDATA_GET_PBASE( ego_prt, pprt );
        if ( NULL == pbase ) continue;

        // don't bother with particles that are not allocated
        if ( !FLAG_ALLOCATED_PBASE( pbase ) || !FLAG_VALID_PBASE( pbase ) ) continue;

        if ( FLAG_TERMINATED_PBASE( pbase ) )
        {
            // the memory area is marked as invalid,
            // but it has not been "killed" yet
            PrtObjList.free_one( GET_REF_PPRT( pprt ) );
        }
        else if ( STATE_WAITING_PBASE( pbase ) )
        {
            // the object is waiting to be killed, so
            // do all of the end of life care for the particle
            prt_do_end_spawn( PRT_REF( cnt ) );

            free_one_particle_in_game( PRT_REF( cnt ) );

            // tell the particle to finish deallocating itself
            ego_obj::end_processing( pbase );
        }
    }
}

//--------------------------------------------------------------------------------------------
void increment_all_particle_update_counters()
{
    PRT_REF cnt;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        ego_obj * pbase = PrtObjList.get_valid_ptr( cnt );

        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_do_bump_damage( ego_prt_bundle * pbdl_prt )
{
    // apply damage from attached bump particles (about once a second)

    CHR_REF ichr, iholder;
    Uint32  update_count;
    IPair  local_damage;

    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    // wait until the right time
    update_count = update_wld + loc_pprt->get_pparent()->guid;
    if ( 0 != ( update_count & 31 ) ) return pbdl_prt;

    // do nothing if the particle is hidden
    /// @note ZF@> This is already checked in prt_update_ingame()
    //if ( loc_pprt->is_hidden ) return;

    // we must be attached to something
    ichr = loc_pprt->attachedto_ref;
    if ( !INGAME_CHR( ichr ) ) return pbdl_prt;

    // find out who is holding the owner of this object
    iholder = ego_chr::get_lowest_attachment( ichr, btrue );
    if ( MAX_CHR == iholder ) iholder = ichr;

    // do nothing if you are attached to your owner
    if (( MAX_CHR != loc_pprt->owner_ref ) && ( iholder == loc_pprt->owner_ref || ichr == loc_pprt->owner_ref ) ) return pbdl_prt;

    // Attached particle damage ( Burning )
    if ( loc_ppip->allowpush && 0 == loc_ppip->vel_hrz_pair.base )
    {
        // Make character limp
        ChrObjList.get_data( ichr ).vel.x *= 0.5f;
        ChrObjList.get_data( ichr ).vel.y *= 0.5f;
    }

    /// @note  Why is this commented out? Attached arrows need to do damage.
    local_damage = loc_pprt->damage;

    // distribute the damage over the particle's lifetime
    if ( !loc_pprt->is_eternal )
    {
        local_damage.base /= loc_pprt->lifetime;
        local_damage.rand /= loc_pprt->lifetime;
    }

    damage_character( ichr, ATK_BEHIND, local_damage, loc_pprt->damagetype, loc_pprt->team, loc_pprt->owner_ref, loc_ppip->damfx, bfalse );

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
int prt_do_contspawn( ego_prt_bundle * pbdl_prt )
{
    /// Spawn new particles if continually spawning

    int spawn_count = 0;

    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;

    if ( NULL == pbdl_prt ) return spawn_count;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    if ( loc_ppip->contspawn_amount <= 0 || -1 == loc_ppip->contspawn_pip )
    {
        return spawn_count;
    }

    if ( 0 == loc_pprt->contspawn_delay )
    {
        FACING_T facing;
        Uint8    tnc;

        // reset the spawn timer
        loc_pprt->contspawn_delay = loc_ppip->contspawn_delay;

        facing = loc_pprt->facing;
        for ( tnc = 0; tnc < loc_ppip->contspawn_amount; tnc++ )
        {
            PRT_REF prt_child = spawn_one_particle( ego_prt::get_pos( loc_pprt ), facing, loc_pprt->profile_ref, loc_ppip->contspawn_pip,
                                                    CHR_REF( MAX_CHR ), GRIP_LAST, loc_pprt->team, loc_pprt->owner_ref, pbdl_prt->prt_ref, tnc, loc_pprt->target_ref );

            if ( VALID_PRT( prt_child ) )
            {
                // Inherit velocities from the particle we were spawned from, but only if it wasn't attached to something

                /// @note ZF@> I have disabled this at the moment. This is what caused the erratic particle movement for the Adventurer Torch
                /// @note BB@> taking out the test works, though  I should have checked vs. loc_pprt->attached_ref, anyway,
                ///            since we already specified that the particle is not attached in the function call :P
                /*if( !ACTIVE_CHR( loc_pprt->attachedto_ref ) )
                {
                    PrtObjList.get_data(prt_child).vel.x += loc_pprt->vel.x;
                    PrtObjList.get_data(prt_child).vel.y += loc_pprt->vel.y;
                    PrtObjList.get_data(prt_child).vel.z += loc_pprt->vel.z;
                }*/
                /// @note ZF@> I have again disabled this. Is this really needed? It wasn't implemented before and causes
                ///     many, many, many issues with all particles around the game.

                // Keep count of how many were actually spawned
                spawn_count++;
            }

            facing += loc_ppip->contspawn_facingadd;
        }
    }

    return spawn_count;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_update_do_water( ego_prt_bundle * pbdl_prt )
{
    /// handle the particle interaction with water

    bool_t inwater;

    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * penviro;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    inwater = ( pbdl_prt->prt_ptr->pos.z < water.surface_level ) && ( 0 != ego_mpd::test_fx( PMesh, pbdl_prt->prt_ptr->onwhichgrid, MPDFX_WATER ) );

    if ( inwater && water.is_water && pbdl_prt->pip_ptr->end_water )
    {
        // Check for disaffirming character
        if ( INGAME_CHR( pbdl_prt->prt_ptr->attachedto_ref ) && pbdl_prt->prt_ptr->owner_ref == pbdl_prt->prt_ptr->attachedto_ref )
        {
            // Disaffirm the whole character
            disaffirm_attached_particles( pbdl_prt->prt_ptr->attachedto_ref );
        }
        else
        {
            // destroy the particle without ever activating it
            _prt_request_terminate( pbdl_prt );
            return NULL;
        }
    }
    else if ( inwater )
    {
        bool_t  spawn_valid     = bfalse;
        int     spawn_pip_index = -1;
        fvec3_t vtmp            = VECT3( pbdl_prt->prt_ptr->pos.x, pbdl_prt->prt_ptr->pos.y, water.surface_level );

        if ( MAX_CHR == pbdl_prt->prt_ptr->owner_ref && ( PIP_SPLASH == pbdl_prt->prt_ptr->pip_ref || PIP_RIPPLE == pbdl_prt->prt_ptr->pip_ref ) )
        {
            /* do not spawn anything for a splash or a ripple */
            spawn_valid = bfalse;
        }
        else
        {
            if ( !pbdl_prt->prt_ptr->inwater )
            {
                if ( SPRITE_SOLID == pbdl_prt->prt_ptr->type )
                {
                    spawn_pip_index = PIP_SPLASH;
                }
                else
                {
                    spawn_pip_index = PIP_RIPPLE;
                }
                spawn_valid = btrue;
            }
            else
            {
                if ( SPRITE_SOLID == pbdl_prt->prt_ptr->type && !INGAME_CHR( pbdl_prt->prt_ptr->attachedto_ref ) )
                {
                    // only spawn ripples if you are touching the water surface!
                    if ( pbdl_prt->prt_ptr->pos.z + pbdl_prt->prt_ptr->bump_real.height > water.surface_level && pbdl_prt->prt_ptr->pos.z - pbdl_prt->prt_ptr->bump_real.height < water.surface_level )
                    {
                        int ripand = ~(( ~RIPPLEAND ) << 1 );
                        if ( 0 == (( update_wld + pbdl_prt->prt_ptr->get_pparent()->guid ) & ripand ) )
                        {
                            spawn_valid = btrue;
                            spawn_pip_index = PIP_RIPPLE;
                        }
                    }
                }
            }
        }

        if ( spawn_valid )
        {
            // Splash for particles is just a ripple
            spawn_one_particle_global( vtmp, 0, spawn_pip_index, 0 );
        }

        pbdl_prt->prt_ptr->inwater  = btrue;
    }
    else
    {
        pbdl_prt->prt_ptr->inwater = bfalse;
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_update_animation( ego_prt_bundle * pbdl_prt )
{
    /// animate the particle

    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    loc_pprt->image = loc_pprt->image + loc_pprt->image_add;
    if ( loc_pprt->image >= loc_pprt->image_max ) loc_pprt->image = 0;

    // rotate the particle
    loc_pprt->rotate += loc_pprt->rotate_add;

    // update the particle size
    if ( 0 != loc_pprt->size_add )
    {
        int size_new;

        // resize the particle
        size_new = loc_pprt->size + loc_pprt->size_add;
        size_new = CLIP( size_new, 0, 0xFFFF );

        ego_prt::set_size( loc_pprt, size_new );
    }

    // spin the particle
    loc_pprt->facing += loc_ppip->facingadd;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_update_dynalight( ego_prt_bundle * pbdl_prt )
{
    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    // Change dyna light values
    if ( loc_pprt->dynalight.level > 0 )
    {
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
        if ( loc_pprt->dynalight.level < 0 ) loc_pprt->dynalight.level = 0;
    }
    else if ( loc_pprt->dynalight.level < 0 )
    {
        // try to guess what should happen for negative lighting
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
        if ( loc_pprt->dynalight.level > 0 ) loc_pprt->dynalight.level = 0;
    }
    else
    {
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
    }

    loc_pprt->dynalight.falloff += loc_ppip->dynalight.falloff_add;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_update_timers( ego_prt_bundle * pbdl_prt )
{
    ego_prt             * loc_pprt;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;

    // down the remaining lifetime of the particle
    if ( loc_pprt->lifetime_remaining > 0 ) loc_pprt->lifetime_remaining--;

    // down the continuous spawn timer
    if ( loc_pprt->contspawn_delay > 0 ) loc_pprt->contspawn_delay--;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_update_ingame( ego_prt_bundle * pbdl_prt )
{
    /// @details BB@> update everything about a particle that does not depend on collisions
    ///               or interactions with characters

    ego_obj * pbase;
    ego_prt * loc_pprt;
    ego_pip * loc_ppip;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    pbase = PDATA_GET_PBASE( ego_prt, loc_pprt );

    // if the object is not "on", it is no longer "in game" but still needs to be displayed
    if ( !INGAME_PPRT( loc_pprt ) )
    {
        return pbdl_prt;
    }

    // clear out the attachment if the attached character doesn't exist
    if ( !DEFINED_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->attachedto_ref = CHR_REF( MAX_CHR );
    }

    // update some particle states
    {
        loc_pprt->is_hidden = bfalse;
        if ( INGAME_CHR( loc_pprt->attachedto_ref ) )
        {
            loc_pprt->is_hidden = ChrObjList.get_data( loc_pprt->attachedto_ref ).is_hidden;
        }

        loc_pprt->is_homing = loc_ppip->homing && !INGAME_CHR( loc_pprt->attachedto_ref ) && INGAME_CHR( loc_pprt->target_ref );
    }

    // figure out where the particle is on the mesh and update pbdl_prt->prt_ref states
    if ( !loc_pprt->is_hidden )
    {
        pbdl_prt = prt_update_do_water( pbdl_prt );
        if ( NULL == pbdl_prt || NULL == loc_pprt ) return pbdl_prt;
    }

    //---- the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pbdl_prt;

    // hidden particles really don't do much of anything
    if ( !loc_pprt->is_hidden )
    {
        pbdl_prt = prt_update_animation( pbdl_prt );
        if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

        pbdl_prt = prt_update_dynalight( pbdl_prt );
        if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

        pbdl_prt = prt_update_timers( pbdl_prt );
        if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

        prt_do_contspawn( pbdl_prt );

        pbdl_prt = prt_do_bump_damage( pbdl_prt );
        if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    }

    if ( !loc_pprt->is_hidden )
    {
        pbase->update_count++;
    }

    // If the particle is done updating, send it to limbo
    if ( !loc_pprt->is_eternal && ( pbase->update_count > 0 && 0 == loc_pprt->lifetime_remaining ) )
    {
        ego_obj_prt::set_limbo( PDATA_GET_POBJ( ego_prt, loc_pprt ), btrue );
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_update_limbo( ego_prt_bundle * pbdl_prt )
{
    /// @details BB@> handle the case where the particle is still being diaplayed, but is no longer
    ///               in the game

    bool_t prt_display;
    bool_t request_terminate;

    ego_obj * pbase;
    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    pbase = PDATA_GET_PBASE( ego_prt, pbdl_prt->prt_ptr );
    if ( !VALID_PBASE( pbase ) ) return pbdl_prt;

    request_terminate = pbase->get_req().kill_me;
    if ( !loc_pprt->is_eternal )
    {
        if ( 0 == loc_pprt->frames_remaining ) request_terminate = btrue;
        if ( 0 == loc_pprt->lifetime_remaining ) request_terminate = btrue;
    }

    // if the object has been around for more than one frame, there is no need to keep it as a "limbo particle"
    prt_display = !pbase->get_proc().on && ( 0 == pbase->frame_count );
    if ( request_terminate && !prt_display )
    {
        prt_request_free( pbdl_prt );
        return NULL;
    }

    // clear out the attachment if the character doesn't exist at all
    if ( !DEFINED_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->attachedto_ref = CHR_REF( MAX_CHR );
    }

    // determine whether the pbdl_prt->prt_ref is hidden
    loc_pprt->is_hidden = bfalse;
    if ( INGAME_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->is_hidden = ChrObjList.get_data( loc_pprt->attachedto_ref ).is_hidden;
    }

    loc_pprt->is_homing = loc_ppip->homing && !INGAME_CHR( loc_pprt->attachedto_ref ) && INGAME_CHR( loc_pprt->target_ref );

    // the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pbdl_prt;

    pbdl_prt = prt_update_animation( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

    pbdl_prt = prt_update_dynalight( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

    if ( !loc_pprt->is_hidden )
    {
        pbase->update_count++;
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_update( ego_prt_bundle * pbdl_prt )
{
    ego_prt             * loc_pprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * penviro;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    // do the next step in the particle configuration
    ego_obj_prt * tmp_pobj = ego_obj_prt::run( PDATA_GET_POBJ( ego_prt, pbdl_prt->prt_ptr ) );
    if ( NULL == tmp_pobj ) { ego_prt_bundle::ctor_this( pbdl_prt ); return NULL; }

    if ( tmp_pobj->get_pdata() != pbdl_prt->prt_ptr )
    {
        // "new" particle, so re-validate the bundle
        ego_prt_bundle::set( pbdl_prt, pbdl_prt->prt_ptr );
    }

    // if the bundle is no longer valid, return
    if ( NULL == pbdl_prt->prt_ptr || NULL == pbdl_prt->pip_ptr ) return pbdl_prt;

    // if the particle is no longer allocated, return
    if ( !VALID_PPRT( pbdl_prt->prt_ptr ) ) return pbdl_prt;

    // handle different particle states differently
    if ( LIMBO_PPRT( pbdl_prt->prt_ptr ) )
    {
        // the particle is in limbo
        pbdl_prt = prt_update_limbo( pbdl_prt );
    }
    else
    {
        // the particle is on
        pbdl_prt = prt_update_ingame( pbdl_prt );
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t prt_update_safe_raw( ego_prt * pprt )
{
    bool_t retval = bfalse;

    BIT_FIELD hit_a_wall;
    float  pressure;

    if ( !VALID_PPRT( pprt ) ) return bfalse;

    hit_a_wall = prt_hit_wall( pprt, NULL, NULL, &pressure );
    if ( 0 != hit_a_wall && 0.0f == pressure )
    {
        pprt->safe_valid = btrue;
        pprt->safe_pos   = ego_prt::get_pos( pprt );
        pprt->safe_time  = update_wld;
        pprt->safe_grid  = ego_mpd::get_tile( PMesh, pprt->pos.x, pprt->pos.y );

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_update_safe( ego_prt * pprt, bool_t force )
{
    Uint32 new_grid;
    bool_t retval = bfalse;
    bool_t needs_update = bfalse;

    if ( !VALID_PPRT( pprt ) ) return bfalse;

    if ( force || !pprt->safe_valid )
    {
        needs_update = btrue;
    }
    else
    {
        new_grid = ego_mpd::get_tile( PMesh, pprt->pos.x, pprt->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pprt->pos.x - pprt->safe_pos.x ) > GRID_SIZE ||
                 ABS( pprt->pos.y - pprt->safe_pos.y ) > GRID_SIZE )
            {
                needs_update = btrue;
            }
        }
        else if ( new_grid != pprt->safe_grid )
        {
            needs_update = btrue;
        }
    }

    if ( needs_update )
    {
        retval = prt_update_safe_raw( pprt );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
fvec3_t ego_prt::get_pos( ego_prt * pprt )
{
    fvec3_t vtmp = ZERO_VECT3;

    if ( !VALID_PPRT( pprt ) ) return vtmp;

    return pprt->pos;
}

//--------------------------------------------------------------------------------------------
float * ego_prt::get_pos_v( ego_prt * pprt )
{
    static fvec3_t vtmp = ZERO_VECT3;

    if ( !VALID_PPRT( pprt ) ) return vtmp.v;

    return pprt->pos.v;
}

//--------------------------------------------------------------------------------------------
bool_t prt_update_pos( ego_prt * pprt )
{
    if ( !VALID_PPRT( pprt ) ) return bfalse;

    pprt->onwhichgrid  = ego_mpd::get_tile( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->onwhichblock = ego_mpd::get_block( PMesh, pprt->pos.x, pprt->pos.y );

    // update whether the current character position is safe
    prt_update_safe( pprt, bfalse );

    // update the breadcrumb list (does not exist for particles )
    // prt_update_breadcrumb( pprt, bfalse );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_prt::set_pos( ego_prt * pprt, fvec3_base_t pos )
{
    bool_t retval = bfalse;

    if ( !VALID_PPRT( pprt ) ) return retval;

    retval = btrue;

    if (( pos[kX] != pprt->pos.v[kX] ) || ( pos[kY] != pprt->pos.v[kY] ) || ( pos[kZ] != pprt->pos.v[kZ] ) )
    {
        memmove( pprt->pos.v, pos, sizeof( fvec3_base_t ) );

        retval = prt_update_pos( pprt );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::set_limbo( ego_obj_prt * pobj, bool_t val )
{
    // An analogy to the ego_obj_set_*() functions, but with the extra particle data in
    // obj_base_display

    ego_obj * pbase = POBJ_GET_PBASE( pobj );
    if ( NULL == pbase ) return pobj;

    if ( !FLAG_VALID_PBASE( pbase ) || FLAG_TERMINATED_PBASE( pbase ) ) return pobj;

    pobj->obj_base_display = val;

    return pobj;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_prt_bundle * ego_prt_bundle::ctor_this( ego_prt_bundle * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    pbundle->prt_ref = PRT_REF( MAX_PRT );
    pbundle->prt_ptr = NULL;

    pbundle->pip_ref = PIP_REF( MAX_PIP );
    pbundle->pip_ptr = NULL;

    return pbundle;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * ego_prt_bundle::validate( ego_prt_bundle * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    if ( VALID_PRT( pbundle->prt_ref ) )
    {
        pbundle->prt_ptr = PrtObjList.get_pdata( pbundle->prt_ref );
    }
    else if ( NULL != pbundle->prt_ptr )
    {
        pbundle->prt_ref = GET_REF_PPRT( pbundle->prt_ptr );
    }
    else
    {
        pbundle->prt_ref = MAX_PRT;
        pbundle->prt_ptr = NULL;
    }

    if ( !LOADED_PIP( pbundle->pip_ref ) && NULL != pbundle->prt_ptr )
    {
        pbundle->pip_ref = pbundle->prt_ptr->pip_ref;
    }

    if ( LOADED_PIP( pbundle->pip_ref ) )
    {
        pbundle->pip_ptr = PipStack.lst + pbundle->pip_ref;
    }
    else
    {
        pbundle->pip_ref = PIP_REF( MAX_PIP );
        pbundle->pip_ptr = NULL;
    }

    return pbundle;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * ego_prt_bundle::set( ego_prt_bundle * pbundle, ego_prt * pprt )
{
    if ( NULL == pbundle ) return NULL;

    // blank out old data
    pbundle = ego_prt_bundle::ctor_this( pbundle );

    if ( NULL == pbundle || NULL == pprt ) return pbundle;

    // set the particle pointer
    pbundle->prt_ptr = pprt;

    // validate the particle data
    pbundle = ego_prt_bundle::validate( pbundle );

    return pbundle;
}

//--------------------------------------------------------------------------------------------
void initialize_particle_physics()
{
    PRT_BEGIN_LOOP_USED( cnt, bdl_prt )
    {
        phys_data_blank_accumulators( &( bdl_prt.prt_ptr->phys ) );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_bump_mesh_attached( ego_prt_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt )
{
    /// @details BB@> A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    bool_t hit_a_mesh;

    ego_prt             * loc_pprt;
    PRT_REF             loc_iprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;

    if ( NULL == pbdl ) return NULL;
    loc_iprt = pbdl->prt_ref;
    loc_pprt = pbdl->prt_ptr;
    loc_ppip = pbdl->pip_ptr;
    loc_penviro  = &( loc_pprt->enviro );

    // if the particle is not still in "display mode" there is no point in going on
    if ( !INGAME_PPRT_BASE( loc_pprt ) ) return pbdl;

    // only deal with attached particles
    if ( MAX_CHR == loc_pprt->attachedto_ref ) return pbdl;

    // Move the particle
    hit_a_mesh = bfalse;
    if ( test_pos.z < loc_penviro->grid_adj )
    {
        hit_a_mesh = btrue;
    }

    if ( hit_a_mesh )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( loc_iprt, loc_ppip->end_sound_floor );
    }

    // handle the collision
    if ( hit_a_mesh && loc_ppip->end_ground )
    {
        prt_request_free( pbdl );
        return NULL;
    }

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle * prt_bump_grid_attached( ego_prt_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt )
{
    /// @details BB@> A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    float loc_level;
    bool_t hit_a_grid, needs_test, updated_2d;
    fvec3_t nrm_total;

    ego_prt             * loc_pprt;
    PRT_REF             loc_iprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;

    if ( NULL == pbdl ) return NULL;
    loc_pprt = pbdl->prt_ptr;
    loc_iprt = pbdl->prt_ref;
    loc_ppip = pbdl->pip_ptr;
    loc_penviro  = &( loc_pprt->enviro );

    // if the particle is not still in "display mode" there is no point in going on
    if ( !INGAME_PPRT_BASE( loc_pprt ) ) return pbdl;

    // only deal with attached particles
    if ( MAX_CHR == loc_pprt->attachedto_ref ) return pbdl;

    hit_a_grid  = bfalse;
    fvec3_self_clear( nrm_total.v );

    loc_level = loc_penviro->grid_adj;

    // interaction with the mesh walls
    hit_a_grid = bfalse;
    updated_2d = bfalse;
    needs_test = bfalse;
    if ( fvec2_length_abs( test_vel.v ) > 0.0f )
    {
        if ( prt_test_wall( loc_pprt, test_pos.v ) )
        {
            Uint32  hit_bits;
            fvec2_t nrm;
            float   pressure;

            // how is the character hitting the wall?
            hit_bits = prt_hit_wall( loc_pprt, test_pos.v, nrm.v, &pressure );

            if ( 0 != hit_bits )
            {
                hit_a_grid = btrue;
            }
        }
    }

    // handle the sounds
    if ( hit_a_grid )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( loc_iprt, loc_ppip->end_sound_wall );
    }

    // handle the collision
    if ( hit_a_grid && ( loc_ppip->end_wall || loc_ppip->end_bump ) )
    {
        prt_request_free( pbdl );
        return NULL;
    }

    return pbdl;
}
//--------------------------------------------------------------------------------------------
ego_prt_bundle *  prt_bump_mesh( ego_prt_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt, bool_t * pbumped_mesh )
{
    /// @details BB@> A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    float loc_level;
    bool_t hit_a_mesh;
    bool_t touch_a_mesh;
    fvec3_t nrm_total;
    bool_t loc_bumped_mesh;

    ego_prt             * loc_pprt;
    PRT_REF             loc_iprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    fvec3_t final_vel, final_pos;
    fvec3_t save_apos_plat, save_avel;
    fvec3_t old_pos;

    // handle optional parameters
    if ( NULL == pbumped_mesh ) pbumped_mesh = &loc_bumped_mesh;
    *pbumped_mesh = bfalse;

    if ( NULL == pbdl ) return NULL;
    loc_pprt = pbdl->prt_ptr;
    loc_iprt = pbdl->prt_ref;
    loc_ppip = pbdl->pip_ptr;
    loc_penviro  = &( loc_pprt->enviro );
    loc_pphys    = &( loc_pprt->phys );

    // if the particle is not still in "display mode" there is no point in going on
    if ( !INGAME_PPRT_BASE( loc_pprt ) ) return pbdl;

    // no point in doing this if the particle thinks it's attached
    if ( MAX_CHR != loc_pprt->attachedto_ref )
    {
        return prt_bump_mesh_attached( pbdl, test_pos, test_vel, dt );
    }

    // save some parameters
    save_apos_plat = loc_pphys->apos_plat;
    save_avel      = loc_pphys->avel;
    old_pos        = ego_prt::get_pos( loc_pprt );

    // assume no change
    final_vel = test_vel;
    final_pos = test_pos;
    hit_a_mesh   = bfalse;
    touch_a_mesh = bfalse;

    // initialize the normal
    fvec3_self_clear( nrm_total.v );

    loc_level = loc_penviro->grid_adj;
    if ( test_pos.z < loc_level )
    {
        fvec3_t floor_nrm = VECT3( 0, 0, 1 );
        float vel_dot;
        fvec3_t vel_perp, vel_para;
        Uint8 tmp_twist = TWIST_FLAT;

        touch_a_mesh = btrue;

        tmp_twist = cartman_get_fan_twist( PMesh, loc_pprt->onwhichgrid );

        if ( TWIST_FLAT != tmp_twist )
        {
            floor_nrm = map_twist_nrm[loc_penviro->grid_twist];
        }

        vel_dot = fvec3_dot_product( floor_nrm.v, test_vel.v );

        vel_perp.x = floor_nrm.x * vel_dot;
        vel_perp.y = floor_nrm.y * vel_dot;
        vel_perp.z = floor_nrm.z * vel_dot;

        vel_para.x = test_vel.x - vel_perp.x;
        vel_para.y = test_vel.y - vel_perp.y;
        vel_para.z = test_vel.z - vel_perp.z;

        if ( vel_dot < - STOPBOUNCINGPART )
        {
            // the particle will bounce
            nrm_total.x += floor_nrm.x;
            nrm_total.y += floor_nrm.y;
            nrm_total.z += floor_nrm.z;

            final_pos.z = old_pos.z;

            hit_a_mesh = btrue;
        }
        else if ( vel_dot > 0.0f )
        {
            // the particle is not bouncing, it is just at the wrong height
            final_pos.z = loc_level;
        }
        else
        {
            // the particle is in the "stop bouncing zone"
            final_pos.z = loc_level + 0.0001f;
            final_vel   = vel_para;
        }
    }

    // handle the sounds
    if ( hit_a_mesh )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( loc_iprt, loc_ppip->end_sound_floor );
    }

    // handle the collision
    if ( touch_a_mesh && loc_ppip->end_ground )
    {
        prt_request_free( pbdl );
        return NULL;
    }

    // do the reflections off the walls and floors
    if ( !INGAME_CHR( loc_pprt->attachedto_ref ) && hit_a_mesh )
    {
        if ( hit_a_mesh && ( test_vel.z * nrm_total.z ) < 0.0f )
        {
            float vdot;
            fvec3_t   vpara, vperp;

            fvec3_self_normalize( nrm_total.v );

            vdot  = fvec3_dot_product( nrm_total.v, test_vel.v );

            vperp.x = nrm_total.x * vdot;
            vperp.y = nrm_total.y * vdot;
            vperp.z = nrm_total.z * vdot;

            vpara.x = test_vel.x - vperp.x;
            vpara.y = test_vel.y - vperp.y;
            vpara.z = test_vel.z - vperp.z;

            // we can use the impulse to determine how much velocity to kill in the parallel direction
            //imp.x = vperp.x * (1.0f + loc_ppip->dampen);
            //imp.y = vperp.y * (1.0f + loc_ppip->dampen);
            //imp.z = vperp.z * (1.0f + loc_ppip->dampen);

            // do the reflection
            vperp.x *= -loc_ppip->dampen;
            vperp.y *= -loc_ppip->dampen;
            vperp.z *= -loc_ppip->dampen;

            // fake the friction, for now
            if ( 0.0f != nrm_total.y || 0.0f != nrm_total.z )
            {
                vpara.x *= loc_ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.z )
            {
                vpara.y *= loc_ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.y )
            {
                vpara.z *= loc_ppip->dampen;
            }

            // add the components back together
            final_vel.x = vpara.x + vperp.x;
            final_vel.y = vpara.y + vperp.y;
            final_vel.z = vpara.z + vperp.z;
        }

        if ( nrm_total.z != 0.0f && ABS( final_vel.z ) < STOPBOUNCINGPART )
        {
            // this is the very last bounce
            final_vel.z = 0.0f;
            final_pos.z = loc_level + 0.0001f;
        }
    }

    {
        fvec3_t _tmp_vec = fvec3_sub( final_vel.v, test_vel.v );
        fvec3_self_scale( _tmp_vec.v, 1.0f / dt );
        phys_data_accumulate_avel( loc_pphys,  _tmp_vec.v );
    }

    {
        fvec3_t _tmp_vec = fvec3_sub( final_pos.v, test_pos.v );
        phys_data_accumulate_apos_plat( loc_pphys,  _tmp_vec.v );
    }

    *pbumped_mesh = ( fvec3_dist_abs( save_apos_plat.v, loc_pphys->apos_plat.v ) != 0.0f ) ||
                    ( fvec3_dist_abs( save_avel.v, loc_pphys->avel.v ) * dt != 0.0f ) ;

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_prt_bundle *  prt_bump_grid( ego_prt_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt, bool_t * pbumped_grid )
{
    /// @details BB@> A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    float loc_level;
    bool_t hit_a_grid, needs_test, updated_2d;
    bool_t touch_a_grid;
    fvec3_t nrm_total;
    bool_t loc_bumped_grid;

    ego_prt             * loc_pprt;
    PRT_REF             loc_iprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    fvec3_t final_vel, final_pos;
    fvec3_t save_apos_plat, save_avel;
    fvec3_t old_pos;

    BIT_FIELD  hit_bits;
    fvec2_t nrm;
    float   pressure;

    // handle optional parameters
    if ( NULL == pbumped_grid ) pbumped_grid = &loc_bumped_grid;
    *pbumped_grid = bfalse;

    if ( NULL == pbdl ) return NULL;
    loc_pprt = pbdl->prt_ptr;
    loc_iprt = pbdl->prt_ref;
    loc_ppip = pbdl->pip_ptr;
    loc_penviro  = &( loc_pprt->enviro );
    loc_pphys    = &( loc_pprt->phys );

    // if the particle is not still in "display mode" there is no point in going on
    if ( !INGAME_PPRT_BASE( loc_pprt ) ) return pbdl;

    // no point in doing this if the particle thinks it's attached
    if ( MAX_CHR != loc_pprt->attachedto_ref )
    {
        return prt_bump_grid_attached( pbdl, test_pos, test_vel, dt );
    }

    // save some parameters
    save_apos_plat = loc_pphys->apos_plat;
    save_avel      = loc_pphys->avel;
    old_pos        = ego_prt::get_pos( loc_pprt );

    // assume no change
    final_vel = test_vel;
    final_pos = test_pos;
    hit_a_grid    = bfalse;
    touch_a_grid  = bfalse;
    updated_2d = bfalse;
    needs_test = bfalse;

    // initialize the normal
    fvec3_self_clear( nrm_total.v );

    loc_level = loc_penviro->grid_adj;

    if ( !prt_test_wall( loc_pprt, test_pos.v ) )
    {
        // no detected interaction
        return pbdl;
    }

    // how is the character hitting the wall?
    hit_bits = prt_hit_wall( loc_pprt, test_pos.v, nrm.v, &pressure );

    if ( 0 != hit_bits )
    {
        touch_a_grid = btrue;

        final_pos.x = old_pos.x;
        final_pos.y = old_pos.y;

        nrm_total.x += nrm.x;
        nrm_total.y += nrm.y;

        hit_a_grid = ( fvec2_dot_product( test_vel.v, nrm.v ) < 0.0f );
    }

    // handle the sounds
    if ( hit_a_grid )
    {
        // Play the sound for hitting the wall [WSND]
        play_particle_sound( loc_iprt, loc_ppip->end_sound_wall );
    }

    // handle the collision
    if ( touch_a_grid && ( loc_ppip->end_wall || loc_ppip->end_bump ) )
    {
        prt_request_free( pbdl );
        return NULL;
    }

    // do the reflections off the walls and floors
    if ( !INGAME_CHR( loc_pprt->attachedto_ref ) && hit_a_grid )
    {
        if ( hit_a_grid && ( test_vel.x * nrm_total.x + test_vel.y * nrm_total.y ) < 0.0f )
        {
            float vdot;
            fvec3_t   vpara, vperp;

            fvec3_self_normalize( nrm_total.v );

            vdot  = fvec3_dot_product( nrm_total.v, test_vel.v );

            vperp.x = nrm_total.x * vdot;
            vperp.y = nrm_total.y * vdot;
            vperp.z = nrm_total.z * vdot;

            vpara.x = test_vel.x - vperp.x;
            vpara.y = test_vel.y - vperp.y;
            vpara.z = test_vel.z - vperp.z;

            // we can use the impulse to determine how much velocity to kill in the parallel direction
            //imp.x = vperp.x * (1.0f + loc_ppip->dampen);
            //imp.y = vperp.y * (1.0f + loc_ppip->dampen);
            //imp.z = vperp.z * (1.0f + loc_ppip->dampen);

            // do the reflection
            vperp.x *= -loc_ppip->dampen;
            vperp.y *= -loc_ppip->dampen;
            vperp.z *= -loc_ppip->dampen;

            // fake the friction, for now
            if ( 0.0f != nrm_total.y || 0.0f != nrm_total.z )
            {
                vpara.x *= loc_ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.z )
            {
                vpara.y *= loc_ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.y )
            {
                vpara.z *= loc_ppip->dampen;
            }

            // add the components back together
            final_vel.x = vpara.x + vperp.x;
            final_vel.y = vpara.y + vperp.y;
            final_vel.z = vpara.z + vperp.z;
        }

        if ( nrm_total.z != 0.0f && ABS( test_vel.z ) < STOPBOUNCINGPART )
        {
            // this is the very last bounce
            final_vel.z = 0.0f;
            final_pos.z = loc_level + 0.0001f;
        }

        if ( hit_a_grid )
        {
            float fx, fy;

            // fix the facing
            facing_to_vec( loc_pprt->facing, &fx, &fy );

            if ( 0.0f != nrm_total.x )
            {
                fx *= -1;
            }

            if ( 0.0f != nrm_total.y )
            {
                fy *= -1;
            }

            loc_pprt->facing = vec_to_facing( fx, fy );
        }
    }

    {
        fvec3_t _tmp_vec = fvec3_sub( final_vel.v, test_vel.v );
        fvec3_self_scale( _tmp_vec.v, 1.0f / dt );
        phys_data_accumulate_avel( loc_pphys,  _tmp_vec.v );
    }

    {
        fvec3_t _tmp_vec = fvec3_sub( final_pos.v, test_pos.v );
        phys_data_accumulate_apos_plat( loc_pphys,  _tmp_vec.v );
    }

    *pbumped_grid = ( fvec3_dist_abs( save_apos_plat.v, loc_pphys->apos_plat.v ) != 0.0f ) ||
                    ( fvec3_dist_abs( save_avel.v, loc_pphys->avel.v ) * dt != 0.0f ) ;

    return pbdl;
}

//--------------------------------------------------------------------------------------------
egoboo_rv particle_physics_finalize_one( ego_prt_bundle * pbdl, float dt )
{
    ego_prt             * loc_pprt;
    PRT_REF             loc_iprt;
    ego_pip             * loc_ppip;
    ego_prt_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    bool_t bumped_mesh = bfalse, bumped_grid = bfalse, needs_update = bfalse;

    fvec3_t test_pos, test_vel;

    // aliases for easier notation
    if ( NULL == pbdl ) return rv_error;

    // alias these parameter for easier notation
    loc_iprt    = pbdl->prt_ref;
    loc_pprt    = pbdl->prt_ptr;
    loc_ppip    = pbdl->pip_ptr;
    loc_penviro = &( loc_pprt->enviro );
    loc_pphys   = &( loc_pprt->phys );

    // work on test_pos and test velocity instead of the actual particle position and velocity
    test_pos = ego_prt::get_pos( loc_pprt );
    test_vel = loc_pprt->vel;

    // do the "integration" of the accumulators
    test_pos = ego_prt::get_pos( loc_pprt );
    test_vel = loc_pprt->vel;
    phys_data_integrate_accumulators( &test_pos, &test_vel, loc_pphys, dt );

    // bump the particles with the mesh
    bumped_mesh = bfalse;
    if ( ACTIVE_CHR( loc_pprt->attachedto_ref ) )
    {
        pbdl = prt_bump_mesh_attached( pbdl, test_pos, test_vel, dt );
    }
    else
    {
        pbdl = prt_bump_mesh( pbdl, test_pos, test_vel, dt, &bumped_mesh );
    }
    if ( NULL == pbdl ) return rv_success;

    // if the particle hit the mesh, re-do the "integration" of the accumulators again,
    // to make sure that it does not go through the mesh
    if ( bumped_mesh )
    {
        test_pos = ego_prt::get_pos( loc_pprt );
        test_vel = loc_pprt->vel;
        phys_data_integrate_accumulators( &test_pos, &test_vel, loc_pphys, dt );
    }

    bumped_grid = bfalse;
    if ( ACTIVE_CHR( loc_pprt->attachedto_ref ) )
    {
        pbdl = prt_bump_grid_attached( pbdl, test_pos, test_vel, dt );
    }
    else
    {
        pbdl = prt_bump_grid( pbdl, test_pos, test_vel, dt, &bumped_grid );
    }
    if ( NULL == pbdl ) return rv_success;

    // if the particle hit the grid, re-do the "integration" of the accumulators again,
    // to make sure that it does not go through the grid
    if ( bumped_grid )
    {
        test_pos = ego_prt::get_pos( loc_pprt );
        test_vel = loc_pprt->vel;
        phys_data_integrate_accumulators( &test_pos, &test_vel, loc_pphys, dt );
    }

    // do a special "non-physics" update
    if ( loc_pprt->is_homing && test_pos.z < 0.0f )
    {
        test_pos.z = 0.0f;  // Don't fall in pits...
    }

    // fix the particle orientation
    if ( !ACTIVE_CHR( loc_pprt->attachedto_ref ) && loc_ppip->rotatetoface )
    {
        if ( fvec2_length_abs( test_vel.v ) > 1e-6 )
        {
            // use velocity to find the angle
            loc_pprt->facing = vec_to_facing( test_vel.x, test_vel.y );
        }
        else if ( INGAME_CHR( loc_pprt->target_ref ) )
        {
            ego_chr * ptarget =  ChrObjList.get_pdata( loc_pprt->target_ref );

            // face your target
            loc_pprt->facing = vec_to_facing( ptarget->pos.x - test_pos.x , ptarget->pos.y - test_pos.y );
        }
    }

    // determine whether there is any need to update the particle's safe position
    needs_update = bumped_mesh || bumped_grid;

    // update the particle's position and velocity
    ego_prt::set_pos( loc_pprt, test_pos.v );
    loc_pprt->vel = test_vel;

    // we need to test the validity of the current position every 8 frames or so,
    // no matter what
    if ( !needs_update )
    {
        // make a timer that is individual for each object
        Uint32 prt_update = loc_pprt->get_pparent()->guid + update_wld;

        needs_update = ( 0 == ( prt_update & 7 ) );
    }

    if ( needs_update )
    {
        prt_update_safe( loc_pprt, needs_update );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
void particle_physics_finalize_all( float dt )
{
    // accumulate the accumulators
    PRT_BEGIN_LOOP_ACTIVE( iprt, bdl )
    {
        particle_physics_finalize_one( &bdl, dt );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
// struct ego_obj_prt - memory management
//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::ctor_this( ego_obj_prt * pobj )
{
    // construct this struct, ONLY

    if ( NULL == pobj ) return NULL;

    pobj = ego_obj_prt::alloc( pobj );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::dtor_this( ego_obj_prt * pobj )
{
    // destruct this struct, ONLY

    if ( NULL == pobj || !FLAG_ALLOCATED_PBASE( pobj ) || FLAG_TERMINATED_PBASE( pobj ) ) return pobj;

    pobj = ego_obj_prt::dealloc( pobj );

    // Sets the state to ego_obj_terminated automatically.
    POBJ_TERMINATE( pobj );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::dealloc( ego_obj_prt * pobj )
{
    // deallocate this struct
    if ( !VALID_PBASE( pobj ) ) return pobj;

    /* add something here */

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::do_alloc( ego_prt * pprt )
{
    if ( NULL == pprt ) return pprt;

    ego_prt::alloc( pprt );
    ego_prt_data::alloc( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_prt * ego_prt::do_dealloc( ego_prt * pprt )
{
    if ( NULL == pprt ) return pprt;

    ego_prt_data::dealloc( pprt );

    ego_prt::dealloc( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::do_dealloc( ego_obj_prt * pobj )
{
    // deallocate this struct and all sub structs

    if ( !VALID_PBASE( pobj ) ) return pobj;

    ego_prt::do_dealloc( pobj->get_pdata() );

    ego_obj_prt::dealloc( pobj );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::alloc( ego_obj_prt * pobj )
{
    // allocate this struct
    if ( !ALLOCATED_PBASE( pobj ) ) return pobj;

    /* add something here */

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_prt * ego_obj_prt::do_alloc( ego_obj_prt * pobj )
{
    // allocate this struct and all sub structs

    if ( !ALLOCATED_PBASE( pobj ) ) return pobj;

    ego_obj_prt::alloc( pobj );

    ego_prt::do_alloc( pobj->get_pdata() );

    return pobj;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------
// ego_prt_bundle * move_one_particle_integrate_motion( ego_prt_bundle * pbdl_prt )
//{
//    /// @details BB@> A helper function that figures out the next valid position of the particle.
//    ///               Collisions with the mesh are included in this step.
//
//    float ftmp, loc_level;
//    bool_t hit_a_floor, hit_a_wall, needs_test, updated_2d;
//    bool_t touch_a_floor, touch_a_wall;
//    fvec3_t nrm_total;
//    fvec3_t tmp_pos;
//
//    ego_prt             * loc_pprt;
//    PRT_REF             loc_iprt;
//    ego_pip             * loc_ppip;
//    ego_prt_environment * penviro;
//
//    if( NULL == pbdl_prt ) return NULL;
//    loc_pprt = pbdl_prt->prt_ptr;
//    loc_iprt = pbdl_prt->prt_ref;
//    loc_ppip = pbdl_prt->pip_ptr;
//    penviro  = &(loc_pprt->enviro);
//
//    // if the particle is not still in "display mode" there is no point in going on
//    if ( !INGAME_PPRT_BASE( loc_pprt ) ) return pbdl_prt;
//
//    // capture the position
//    tmp_pos = ego_prt::get_pos( loc_pprt );
//
//    // no point in doing this if the particle thinks it's attached
//    if( MAX_CHR != loc_pprt->attachedto_ref )
//    {
//        return move_one_particle_integrate_motion_attached( pbdl_prt );
//    }
//
//    hit_a_floor   = bfalse;
//    hit_a_wall    = bfalse;
//    touch_a_floor = bfalse;
//    touch_a_wall  = bfalse;
//    nrm_total.x = nrm_total.y = nrm_total.z = 0.0f;
//
//    loc_level = penviro->grid_adj;
//
//    // Move the particle
//    ftmp = tmp_pos.z;
//    tmp_pos.z += loc_pprt->vel.z;
//    LOG_NAN( tmp_pos.z );
//    if ( tmp_pos.z < loc_level )
//    {
//        fvec3_t floor_nrm = VECT3(0,0,1);
//        float vel_dot;
//        fvec3_t vel_perp, vel_para;
//        Uint8 tmp_twist = TWIST_FLAT;
//
//        touch_a_floor = btrue;
//
//        tmp_twist = cartman_get_fan_twist( PMesh, loc_pprt->onwhichgrid );
//
//        if( TWIST_FLAT != tmp_twist )
//        {
//            floor_nrm = map_twist_nrm[penviro->grid_twist];
//        }
//
//        vel_dot = fvec3_dot_product(floor_nrm.v, loc_pprt->vel.v);
//
//        vel_perp.x = floor_nrm.x * vel_dot;
//        vel_perp.y = floor_nrm.y * vel_dot;
//        vel_perp.z = floor_nrm.z * vel_dot;
//
//        vel_para.x = loc_pprt->vel.x - vel_perp.x;
//        vel_para.y = loc_pprt->vel.y - vel_perp.y;
//        vel_para.z = loc_pprt->vel.z - vel_perp.z;
//
//        if ( vel_dot < - STOPBOUNCINGPART )
//        {
//            // the particle will bounce
//            nrm_total.x += floor_nrm.x;
//            nrm_total.y += floor_nrm.y;
//            nrm_total.z += floor_nrm.z;
//
//            tmp_pos.z = ftmp;
//            hit_a_floor = btrue;
//        }
//        else if ( vel_dot > 0.0f )
//        {
//            // the particle is not bouncing, it is just at the wrong height
//            tmp_pos.z = loc_level;
//        }
//        else
//        {
//            // the particle is in the "stop bouncing zone"
//            tmp_pos.z     = loc_level + 0.0001f;
//            loc_pprt->vel = vel_para;
//        }
//    }
//
//    // handle the sounds
//    if ( hit_a_floor )
//    {
//        // Play the sound for hitting the floor [FSND]
//        play_particle_sound( loc_iprt, loc_ppip->end_sound_floor );
//    }
//
//    // handle the collision
//    if ( touch_a_floor && loc_ppip->end_ground )
//    {
//        prt_request_free( pbdl_prt );
//        return NULL;
//    }
//
//    // interaction with the mesh walls
//    hit_a_wall = bfalse;
//    updated_2d = bfalse;
//    needs_test = bfalse;
//    if ( fvec2_length_abs( loc_pprt->vel.v ) > 0.0f )
//    {
//        float old_x, old_y, new_x, new_y;
//
//        old_x = tmp_pos.x; LOG_NAN( old_x );
//        old_y = tmp_pos.y; LOG_NAN( old_y );
//
//        new_x = old_x + loc_pprt->vel.x; LOG_NAN( new_x );
//        new_y = old_y + loc_pprt->vel.y; LOG_NAN( new_y );
//
//        tmp_pos.x = new_x;
//        tmp_pos.y = new_y;
//
//        if ( !prt_test_wall( loc_pprt, tmp_pos.v ) )
//        {
//            updated_2d = btrue;
//        }
//        else
//        {
//            BIT_FIELD  hit_bits;
//            fvec2_t nrm;
//            float   pressure;
//
//            // how is the character hitting the wall?
//            hit_bits = prt_hit_wall( loc_pprt, tmp_pos.v, nrm.v, &pressure );
//
//            if ( 0 != hit_bits )
//            {
//                touch_a_wall = btrue;
//
//                tmp_pos.x = old_x;
//                tmp_pos.y = old_y;
//
//                nrm_total.x += nrm.x;
//                nrm_total.y += nrm.y;
//
//                hit_a_wall = (fvec2_dot_product(loc_pprt->vel.v, nrm.v) < 0.0f);
//            }
//        }
//    }
//
//    // handle the sounds
//    if ( hit_a_wall )
//    {
//        // Play the sound for hitting the wall [WSND]
//        play_particle_sound( loc_iprt, loc_ppip->end_sound_wall );
//    }
//
//    // handle the collision
//    if ( touch_a_wall && ( loc_ppip->end_wall || loc_ppip->end_bump ) )
//    {
//        prt_request_free( pbdl_prt );
//        return NULL;
//    }
//
//    // do the reflections off the walls and floors
//    if ( !INGAME_CHR( loc_pprt->attachedto_ref ) && ( hit_a_wall || hit_a_floor ) )
//    {
//        if (( hit_a_wall && ( loc_pprt->vel.x * nrm_total.x + loc_pprt->vel.y * nrm_total.y ) < 0.0f ) ||
//            ( hit_a_floor && ( loc_pprt->vel.z * nrm_total.z ) < 0.0f ) )
//        {
//            float vdot;
//            fvec3_t   vpara, vperp;
//
//            fvec3_self_normalize( nrm_total.v );
//
//            vdot  = fvec3_dot_product( nrm_total.v, loc_pprt->vel.v );
//
//            vperp.x = nrm_total.x * vdot;
//            vperp.y = nrm_total.y * vdot;
//            vperp.z = nrm_total.z * vdot;
//
//            vpara.x = loc_pprt->vel.x - vperp.x;
//            vpara.y = loc_pprt->vel.y - vperp.y;
//            vpara.z = loc_pprt->vel.z - vperp.z;
//
//            // we can use the impulse to determine how much velocity to kill in the parallel direction
//            //imp.x = vperp.x * (1.0f + loc_ppip->dampen);
//            //imp.y = vperp.y * (1.0f + loc_ppip->dampen);
//            //imp.z = vperp.z * (1.0f + loc_ppip->dampen);
//
//            // do the reflection
//            vperp.x *= -loc_ppip->dampen;
//            vperp.y *= -loc_ppip->dampen;
//            vperp.z *= -loc_ppip->dampen;
//
//            // fake the friction, for now
//            if ( 0.0f != nrm_total.y || 0.0f != nrm_total.z )
//            {
//                vpara.x *= loc_ppip->dampen;
//            }
//
//            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.z )
//            {
//                vpara.y *= loc_ppip->dampen;
//            }
//
//            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.y )
//            {
//                vpara.z *= loc_ppip->dampen;
//            }
//
//            // add the components back together
//            loc_pprt->vel.x = vpara.x + vperp.x;
//            loc_pprt->vel.y = vpara.y + vperp.y;
//            loc_pprt->vel.z = vpara.z + vperp.z;
//        }
//
//        if ( nrm_total.z != 0.0f && loc_pprt->vel.z < STOPBOUNCINGPART )
//        {
//            // this is the very last bounce
//            loc_pprt->vel.z = 0.0f;
//            tmp_pos.z = loc_level + 0.0001f;
//        }
//
//        if ( hit_a_wall )
//        {
//            float fx, fy;
//
//            // fix the facing
//            facing_to_vec( loc_pprt->facing, &fx, &fy );
//
//            if ( 0.0f != nrm_total.x )
//            {
//                fx *= -1;
//            }
//
//            if ( 0.0f != nrm_total.y )
//            {
//                fy *= -1;
//            }
//
//            loc_pprt->facing = vec_to_facing( fx, fy );
//        }
//    }
//
//    if ( loc_pprt->is_homing && tmp_pos.z < 0 )
//    {
//        tmp_pos.z = 0;  // Don't fall in pits...
//    }
//
//    if ( loc_ppip->rotatetoface )
//    {
//        if ( fvec2_length_abs( loc_pprt->vel.v ) > 1e-6 )
//        {
//            // use velocity to find the angle
//            loc_pprt->facing = vec_to_facing( loc_pprt->vel.x, loc_pprt->vel.y );
//        }
//        else if ( INGAME_CHR( loc_pprt->target_ref ) )
//        {
//            ego_chr * ptarget =  ChrObjList.get_pdata( loc_pprt->target_ref);
//
//            // face your target
//            loc_pprt->facing = vec_to_facing( ptarget->pos.x - tmp_pos.x , ptarget->pos.y - tmp_pos.y );
//        }
//    }
//
//    ego_prt::set_pos( loc_pprt, tmp_pos.v );
//
//    return pbdl_prt;
//}

//--------------------------------------------------------------------------------------------
//ego_prt_bundle * move_one_particle_integrate_motion_attached( ego_prt_bundle * pbdl_prt )
//{
//    /// @details BB@> A helper function that figures out the next valid position of the particle.
//    ///               Collisions with the mesh are included in this step.
//
//    float loc_level;
//    bool_t hit_a_floor, hit_a_wall, needs_test, updated_2d;
//    fvec3_t nrm_total;
//    fvec3_t tmp_pos;
//
//    ego_prt             * loc_pprt;
//    PRT_REF             loc_iprt;
//    ego_pip             * loc_ppip;
//    ego_prt_environment * penviro;
//
//    if( NULL == pbdl_prt ) return NULL;
//    loc_pprt = pbdl_prt->prt_ptr;
//    loc_iprt = pbdl_prt->prt_ref;
//    loc_ppip = pbdl_prt->pip_ptr;
//    penviro  = &(loc_pprt->enviro);
//
//    // if the particle is not still in "display mode" there is no point in going on
//    if ( !INGAME_PPRT_BASE( loc_pprt ) ) return pbdl_prt;
//
//    // capture the particle position
//    tmp_pos = ego_prt::get_pos( loc_pprt );
//
//    // only deal with attached particles
//    if( MAX_CHR == loc_pprt->attachedto_ref ) return pbdl_prt;
//
//    hit_a_floor = bfalse;
//    hit_a_wall  = bfalse;
//    nrm_total.x = nrm_total.y = nrm_total.z = 0;
//
//    loc_level = penviro->grid_adj;
//
//    // Move the particle
//    if ( tmp_pos.z < loc_level )
//    {
//        hit_a_floor = btrue;
//    }
//
//    if ( hit_a_floor )
//    {
//        // Play the sound for hitting the floor [FSND]
//        play_particle_sound( loc_iprt, loc_ppip->end_sound_floor );
//    }
//
//    // handle the collision
//    if ( hit_a_floor && loc_ppip->end_ground )
//    {
//        prt_request_free( pbdl_prt );
//        return NULL;
//    }
//
//    // interaction with the mesh walls
//    hit_a_wall = bfalse;
//    updated_2d = bfalse;
//    needs_test = bfalse;
//    if ( fvec2_length_abs( loc_pprt->vel.v ) > 0.0f )
//    {
//        if ( prt_test_wall( loc_pprt, tmp_pos.v ) )
//        {
//            Uint32  hit_bits;
//            fvec2_t nrm;
//            float   pressure;
//
//            // how is the character hitting the wall?
//            hit_bits = prt_hit_wall( loc_pprt, tmp_pos.v, nrm.v, &pressure );
//
//            if ( 0 != hit_bits )
//            {
//                hit_a_wall = btrue;
//            }
//        }
//    }
//
//    // handle the sounds
//    if ( hit_a_wall )
//    {
//        // Play the sound for hitting the floor [FSND]
//        play_particle_sound( loc_iprt, loc_ppip->end_sound_wall );
//    }
//
//    // handle the collision
//    if ( hit_a_wall && ( loc_ppip->end_wall || loc_ppip->end_bump ) )
//    {
//        prt_request_free( pbdl_prt );
//        return NULL;
//    }
//
//    ego_prt::set_pos( loc_pprt, tmp_pos.v );
//
//    return pbdl_prt;
//}
