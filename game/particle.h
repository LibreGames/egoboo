#pragma once

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

#include "egoboo_object.h"

#include "pip_file.h"
#include "graphic_prt.h"
#include "physics.h"
#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_obj_prt;
struct ego_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Particles

#define MAXPARTICLEIMAGE                256         ///< Number of particle images ( frames )

// Physics
#define STOPBOUNCINGPART                5.0f         ///< To make particles stop bouncing

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_pip_data : public s_pip_data {};

struct ego_pip : public ego_pip_data {};

DECLARE_STACK_EXTERN( ego_pip, PipStack, MAX_PIP );

#define VALID_PIP_RANGE( IPIP ) ( ((IPIP) >= 0) && ((IPIP) < MAX_PIP) )
#define LOADED_PIP( IPIP )       ( VALID_PIP_RANGE( IPIP ) && PipStack.lst[IPIP].loaded )

//--------------------------------------------------------------------------------------------

/// Everything that is necessary to compute the character's interaction with the environment
struct ego_prt_environment
{
    // floor stuff
    float  grid_level;           ///< Height the current grid
    float  grid_lerp;
    Uint8  grid_twist;
    float  grid_adj;              ///< The level for the particle to sit on the floor or a platform

    float  floor_level;           ///< Height of tile, platform
    float  floor_lerp;
    float  floor_adj;             ///< The level for the particle to sit on the floor or a platform

    // friction stuff
    bool_t is_slipping;
    bool_t is_slippy,    is_watery;
    float  air_friction, ice_friction;
    float  fluid_friction_hrz, fluid_friction_vrt;
    float  friction_hrz;
    float  traction;

    // misc states
    bool_t   inwater;
    fvec3_t  acc;
};

//--------------------------------------------------------------------------------------------
struct ego_prt_spawn_data
{
    fvec3_t  pos;
    FACING_T facing;
    PRO_REF  iprofile;
    PIP_REF  ipip;

    CHR_REF  chr_attach;
    Uint16   vrt_offset;
    TEAM_REF team;

    CHR_REF  chr_origin;
    PRT_REF  prt_origin;
    int      multispawn;
    CHR_REF  oldtarget;
};

//--------------------------------------------------------------------------------------------
// Particle variables
//--------------------------------------------------------------------------------------------

/// The definition of the particle data
struct ego_prt_data
{
    // profiles
    PIP_REF pip_ref;                         ///< The part template
    PRO_REF profile_ref;                     ///< the profile related to the spawned particle

    // links
    CHR_REF attachedto_ref;                  ///< For torch flame
    CHR_REF owner_ref;                       ///< The character that is attacking
    CHR_REF target_ref;                      ///< Who it's chasing
    PRT_REF parent_ref;                      ///< Did a another particle spawn this one?
    Uint32  parent_guid;                     ///< Just in case, the parent particle was despawned and a different particle now has the parent_ref

    Uint16   attachedto_vrt_off;              ///< It's vertex offset
    Uint8    type;                            ///< Transparency mode, 0-2
    FACING_T facing;                          ///< Direction of the part
    TEAM_REF team;                            ///< Team

    fvec3_t pos, pos_old, pos_stt;           ///< Position
    fvec3_t vel, vel_old, vel_stt;           ///< Velocity
    fvec3_t offset;                          ///< The initial offset when spawning the particle

    Uint32  onwhichgrid;                      ///< Where the part is
    Uint32  onwhichblock;                    ///< The particle's collision block
    bool_t  is_hidden;                       ///< Is the particle related to a hidden character?

    // platforms
    float   targetplatform_overlap;             ///< What is the height of the target platform?
    CHR_REF targetplatform_ref;               ///< Am I trying to attach to a platform?
    CHR_REF onwhichplatform_ref;              ///< Is the particle on a platform?
    Uint32  onwhichplatform_update;           ///< When was the last platform attachment made?

    FACING_T rotate;                          ///< Rotation direction
    Sint16   rotate_add;                      ///< Rotation rate

    SFP8_T  size_stt;                        ///< The initial size of particle (8.8-bit fixed point)
    SFP8_T  size;                            ///< Size of particle (8.8-bit fixed point)
    SFP8_T  size_add;                        ///< Change in size

    bool_t  inview;                          ///< Render this one?
    UFP8_T  image;                           ///< Which image (8.8-bit fixed point)
    Uint16  image_add;                       ///< Animation rate
    Uint16  image_max;                       ///< End of image loop
    Uint16  image_stt;                       ///< Start of image loop

    bool_t  is_eternal;                      ///< Does the particle ever time-out?
    size_t  lifetime;                        ///< Total particle lifetime in updates
    size_t  lifetime_remaining;              ///< How many updates does the particle have left?
    size_t  frames_remaining;                ///< How many frames does the particle have left?
    int     contspawn_delay;                 ///< Time until spawn

    Uint32            bump_size_stt;                   ///< the starting size of the particle (8.8-bit fixed point)
    ego_bumper          bump_real;                       ///< Actual size of the particle
    ego_bumper          bump_padded;                     ///< The maximum of the "real" bumper and the "bump size" bumper
    ego_bumper          bump_min;                        ///< The minimum of the "real" bumper and the "bump size" bumper
    ego_oct_bb            prt_cv;                          ///< Collision volume for chr-prt interactions
    ego_oct_bb            prt_min_cv;                      ///< Collision volume for chr-platform interactions
    PRT_REF           bumplist_next;                   ///< Next particle on fanblock
    IPair             damage;                          ///< For strength
    Uint8             damagetype;                      ///< Damage type
    Uint16            lifedrain;
    Uint16            manadrain;

    bool_t            is_bumpspawn;                      ///< this particle is like a flame, burning something
    bool_t            inwater;

    int               spawncharacterstate;              ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    bool_t            is_homing;                 ///< Is the particle in control of its motion?

    // some data that needs to be copied from the particle profile
    Uint8             end_spawn_amount;        ///< The number of particles to be spawned at the end
    Uint16            end_spawn_facingadd;     ///< The angular spacing for the end spawn
    int               end_spawn_pip;           ///< The actual pip that will be spawned at the end

    dynalight_info_t  dynalight;              ///< Dynamic lighting...
    ego_prt_instance    inst;                   ///< Everything needed for rendering
    ego_prt_environment enviro;                 ///< the particle's environment
    ego_phys_data       phys;                   ///< the particle's physics data

    bool_t         safe_valid;                ///< is the last "safe" position valid?
    fvec3_t        safe_pos;                  ///< the last "safe" position
    Uint32         safe_time;                 ///< the last "safe" time
    Uint32         safe_grid;                 ///< the last "safe" grid

    float          buoyancy;                  ///< an estimate of the particle bouyancy in air
    float          air_resistance;            ///< an estimate of the particle's extra resistance to air motion

    ego_prt_data() { ego_prt_data::ctor( this ); }
    ~ego_prt_data() { ego_prt_data::dtor( this ); }

    static ego_prt_data * ctor( ego_prt_data * );
    static ego_prt_data * dtor( ego_prt_data * );
    static bool_t         dealloc( ego_prt_data * pdata );
};

//--------------------------------------------------------------------------------------------
/// The definition of the particle object
struct ego_prt : public ego_prt_data
{
    friend struct ego_obj_prt;

private:
    const ego_obj_prt * _parent_obj_ptr;

public:
    ego_prt_spawn_data  spawn_data;
    ego_BSP_leaf        bsp_leaf;

    explicit ego_prt( ego_obj_prt * _parent ) : _parent_obj_ptr( _parent ) { ego_prt::ctor( this ); }
    ~ego_prt() { ego_prt::dtor( this ); }

    const ego_obj_prt * cget_pparent() const { return _parent_obj_ptr; }
    ego_obj_prt       * get_pparent()        { return ( ego_obj_prt * )_parent_obj_ptr; }

    static ego_prt * ctor( ego_prt * pprt );
    static ego_prt * dtor( ego_prt * pprt );

    static bool_t    set_pos( ego_prt * pprt, fvec3_base_t pos );
    static float *   get_pos_v( ego_prt * pprt );
    static fvec3_t   get_pos( ego_prt * pprt );
    static void      set_level( ego_prt * pprt, float level );

    // INLINE functions
    static INLINE PIP_REF    get_ipip( const PRT_REF by_reference particle );
    static INLINE ego_pip  * get_ppip( const PRT_REF by_reference particle );
    static INLINE CHR_REF    get_iowner( const PRT_REF by_reference iprt, int depth );
    static INLINE bool_t     set_size( ego_prt *, int size );
    static INLINE float      get_scale( ego_prt * pprt );

private:
    static ego_prt * alloc( ego_prt * pprt );
    static ego_prt * dealloc( ego_prt * pprt );

    static ego_prt * do_init( ego_prt * pprt );
    static ego_prt * do_active( ego_prt * pprt );
    static ego_prt * do_deinit( ego_prt * pprt );
};

//--------------------------------------------------------------------------------------------
// the container object for the particle list
struct ego_obj_prt : public ego_obj
{
    typedef ego_prt data_type;

    /// the flag for limbo particles
    bool_t  obj_base_display;

    ego_obj_prt() : _prt_data( this ) { ctor( this, bfalse ); };
    ~ego_obj_prt() { dtor( this, bfalse ); };

    ego_prt & get_data()  { return _prt_data; }
    ego_prt * get_pdata() { return &_prt_data; }

    const ego_prt & cget_data() const { return _prt_data; }
    const ego_prt * cget_pdata()const { return &_prt_data; }

    static ego_obj_prt * set_limbo( ego_obj_prt * pbase, bool_t val );

    static ego_obj_prt * ctor( ego_obj_prt * pobj, bool_t recursive = btrue ) { if ( NULL == pobj ) return NULL; if ( recursive ) ego_prt::ctor( pobj->get_pdata() ); return pobj; };
    static ego_obj_prt * dtor( ego_obj_prt * pobj, bool_t recursive = btrue ) { if ( NULL == pobj ) return NULL; if ( recursive ) ego_prt::dtor( pobj->get_pdata() ); return pobj; };

    // global prt configuration functions
    static ego_obj_prt * run( ego_obj_prt * pprt );
    static ego_obj_prt * run_construct( ego_obj_prt * pprt, int max_iterations );
    static ego_obj_prt * run_initialize( ego_obj_prt * pprt, int max_iterations );
    static ego_obj_prt * run_activate( ego_obj_prt * pprt, int max_iterations );
    static ego_obj_prt * run_deinitialize( ego_obj_prt * pprt, int max_iterations );
    static ego_obj_prt * run_deconstruct( ego_obj_prt * pprt, int max_iterations );

protected:
    static bool_t    request_terminate( const PRT_REF & iprt );

    static ego_obj_prt * do_constructing( ego_obj_prt * pprt );
    static ego_obj_prt * do_initializing( ego_obj_prt * pprt );
    static ego_obj_prt * do_deinitializing( ego_obj_prt * pprt );
    static ego_obj_prt * do_processing( ego_obj_prt * pprt );
    static ego_obj_prt * do_destructing( ego_obj_prt * pprt );

private:
    ego_prt _prt_data;
};

// counters for debugging wall collisions
extern int prt_stoppedby_tests;
extern int prt_pressure_tests;

//--------------------------------------------------------------------------------------------
struct ego_prt_bundle
{
    PRT_REF   prt_ref;
    ego_prt   * prt_ptr;

    PIP_REF   pip_ref;
    ego_pip   * pip_ptr;

    static ego_prt_bundle * ctor( ego_prt_bundle * pbundle );
    static ego_prt_bundle * validate( ego_prt_bundle * pbundle );
    static ego_prt_bundle * set( ego_prt_bundle * pbundle, ego_prt * pprt );
};

//--------------------------------------------------------------------------------------------
// function prototypes

void particle_system_begin();
void particle_system_end();

void initialize_particle_physics();
void particle_physics_finalize_all( float dt );

void   init_all_pip();
void   release_all_pip();
bool_t release_one_pip( const PIP_REF by_reference ipip );

void   free_one_particle_in_game( const PRT_REF by_reference particle );

void update_all_particles( void );
void move_all_particles( void );
void cleanup_all_particles( void );
void increment_all_particle_update_counters( void );

void play_particle_sound( const PRT_REF by_reference particle, Sint8 sound );

PRT_REF spawn_one_particle( fvec3_t pos, FACING_T facing, const PRO_REF by_reference iprofile, int pip_index,
                            const CHR_REF by_reference chr_attach, Uint16 vrt_offset, const TEAM_REF by_reference team,
                            const CHR_REF by_reference chr_origin, const PRT_REF by_reference prt_origin, int multispawn, const CHR_REF by_reference oldtarget );

#define spawn_one_particle_global( pos, facing, ipip, multispawn ) spawn_one_particle( pos, facing, (PRO_REF)MAX_PROFILE, ipip, (CHR_REF)MAX_CHR, GRIP_LAST, (TEAM_REF)TEAM_NULL, (CHR_REF)MAX_CHR, (PRT_REF)MAX_PRT, multispawn, (CHR_REF)MAX_CHR );

BIT_FIELD prt_hit_wall( ego_prt * pprt, float test_pos[], float nrm[], float * pressure );
bool_t    prt_test_wall( ego_prt * pprt, float test_pos[] );
bool_t    prt_is_over_water( const PRT_REF by_reference particle );
bool_t    prt_request_free( ego_prt_bundle * pbdl_prt );
bool_t    prt_request_free_ref( const PRT_REF by_reference iprt );

PIP_REF load_one_particle_profile_vfs( const char *szLoadName, const PIP_REF by_reference pip_override );
void    reset_particles();

ego_prt_bundle * prt_calc_environment( ego_prt_bundle * pbdl );

#define _particle_h

