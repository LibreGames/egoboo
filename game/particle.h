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

#include "egoboo_object_list.h"

#include "file_formats/pip_file.h"
#include "graphic_prt.h"
#include "physics.h"
#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_obj_prt;
struct ego_prt;

typedef t_ego_obj_container< ego_obj_prt, MAX_PRT >  ego_prt_container;

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

extern t_cpp_stack< ego_pip, MAX_PIP > PipStack;

#define LOADED_PIP( IPIP )       ( PipStack.in_range_ref( IPIP ) && PipStack[IPIP].loaded )

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
/// This has been separated from ego_prt so that it can be initialized easily without upsetting anuthing else

struct ego_prt_data
{
    //---- constructors and destructors

    /// default constructor
    explicit ego_prt_data()  { ego_prt_data::ctor_this( this ); };

    /// default destructor
    ~ego_prt_data() { ego_prt_data::dtor_this( this ); };

    //---- construction and destruction

    /// construct this struct, ONLY
    static ego_prt_data * ctor_this( ego_prt_data * );
    /// destruct this struct, ONLY
    static ego_prt_data * dtor_this( ego_prt_data * );
    /// set critical values to safe values
    static ego_prt_data * init( ego_prt_data * );

    //---- generic enchant data

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

protected:

    //---- construction and destruction

    /// construct this struct, and ALL dependent structs. use placement new
    static ego_prt_data * ctor_all( ego_prt_data * ptr ) { if ( NULL != ptr ) { /* puts( "\t" __FUNCTION__ ); */ new( ptr ) ego_prt_data(); } return ptr; }
    /// denstruct this struct, and ALL dependent structs. call the destructor
    static ego_prt_data * dtor_all( ego_prt_data * ptr )  { if ( NULL != ptr ) { ptr->~ego_prt_data(); /* puts( "\t" __FUNCTION__ ); */ } return ptr; }

    //---- memory management

    /// allocate data for this struct, ONLY
    static ego_prt_data * alloc( ego_prt_data * pprt );
    /// deallocate data for this struct, ONLY
    static ego_prt_data * dealloc( ego_prt_data * pprt );

};

//--------------------------------------------------------------------------------------------
/// The definition of the particle object
struct ego_prt : public ego_prt_data
{
    friend struct t_ego_obj_container< ego_prt, MAX_PRT >;

    typedef ego_obj_prt object_type;

    //---- extra data

    // counters for debugging wall collisions
    static int stoppedby_tests;
    static int pressure_tests;

    ego_prt_spawn_data  spawn_data;
    ego_BSP_leaf        bsp_leaf;

    //---- constructors and destructors

    /// non-default constructor. We MUST know who our parent is
    explicit ego_prt( object_type * pobj ) : _obj_ptr( pobj ) { ego_prt::ctor_this( this ); };

    /// default destructor
    ~ego_prt() { ego_prt::dtor_this( this ); };

    //---- implementation of required accessors

    // This ego_prt is contained by ego_che_obj. We need some way of accessing it
    // These have to have generic names to that all objects that are contained in
    // an ego_object can be interfaced with in the same way

    static       object_type &  get_obj_ref( ego_prt & ref ) { return *(( object_type * )ref._obj_ptr ); }
    static       object_type *  get_obj_ptr( ego_prt * ptr ) { return ( NULL == ptr ) ? NULL : ( object_type * )ptr->_obj_ptr; }
    static const object_type * cget_obj_ptr( const ego_prt * ptr ) { return ( NULL == ptr ) ? NULL : ptr->_obj_ptr; }

    //---- construction and destruction

    /// construct this struct, ONLY
    static ego_prt * ctor_this( ego_prt * pprt );
    /// construct this struct, ONLY
    static ego_prt * dtor_this( ego_prt * pprt );

    /// do some initialization
    static ego_prt * init( ego_prt * pprt );

    //---- generic particle functions

    static bool_t    set_pos( ego_prt * pprt, fvec3_base_t pos );
    static float *   get_pos_v( ego_prt * pprt );
    static fvec3_t   get_pos( ego_prt * pprt );
    static void      set_level( ego_prt * pprt, float level );

    // INLINE functions
    static INLINE PIP_REF    get_ipip( const PRT_REF & particle );
    static INLINE ego_pip  * get_ppip( const PRT_REF & particle );
    static INLINE CHR_REF    get_iowner( const PRT_REF & iprt, int depth );
    static INLINE bool_t     set_size( ego_prt *, int size );
    static INLINE float      get_scale( ego_prt * pprt );

protected:

    //---- construction and destruction

    /// construct this struct, and ALL dependent structs. use placement new
    static ego_prt * ctor_all( ego_prt * ptr, object_type * pobj );
    /// denstruct this struct, and ALL dependent structs. call the destructor
    static ego_prt * dtor_all( ego_prt * ptr );

    //---- memory management

    /// allocate data for this struct, ONLY
    static ego_prt * alloc( ego_prt * pprt );
    /// deallocate data for this struct, ONLY
    static ego_prt * dealloc( ego_prt * pprt );

    /// allocate data for this struct, and ALL dependent structs
    static ego_prt * do_alloc( ego_prt * pprt );
    /// deallocate data for this struct, and ALL dependent structs
    static ego_prt * do_dealloc( ego_prt * pprt );

    //---- private implementations of the configuration functions

    static ego_prt * do_constructing( ego_prt * pprt );
    static ego_prt * do_initializing( ego_prt * pprt );
    static ego_prt * do_processing( ego_prt * pprt );
    static ego_prt * do_deinitializing( ego_prt * pprt );
    static ego_prt * do_destructing( ego_prt * pprt );

private:

    /// a hook to object_type, which is the parent container of this object
    const object_type * _obj_ptr;
};

//--------------------------------------------------------------------------------------------

/// The actual container that will be stored in the t_cpp_list<>

struct ego_obj_prt : public ego_obj, public ego_prt
{
    typedef ego_prt_container container_type;
    typedef ego_prt           data_type;
    typedef PRT_REF           reference_type;
    typedef ego_obj_prt       its_type;

    ego_obj_prt( const container_type * pcont ) : ego_prt( this ), _container_ptr( pcont ), obj_base_display( btrue ) {};

    //---- extra data

    /// the flag for limbo particles
    bool_t  obj_base_display;

    //---- accessors

    /// tell the its_type whether it is in limbo or not
    static its_type * set_limbo( its_type * pobj, bool_t val );

    // This its_type is contained by container_type. We need some way of accessing it.

    static       container_type *  get_container_ptr( its_type * ptr ) { return NULL == ptr ? NULL : ( container_type * )ptr->_container_ptr; }
    static const container_type * cget_container_ptr( const its_type * ptr ) { return NULL == ptr ? NULL : ptr->_container_ptr; }

    static       data_type *  get_data_ptr( its_type * ptr ) { return NULL == ptr ? NULL : static_cast<data_type *>( ptr ); }
    static const data_type * cget_data_ptr( const its_type * ptr ) { return NULL == ptr ? NULL : static_cast<const data_type *>( ptr ); }

    static bool_t request_terminate( const reference_type & iprt );
    static bool_t request_terminate( ego_bundle_prt * pbdl_prt );
    static bool_t request_terminate( its_type * pprt );

    //---- specialization of the ego_object_process methods
    virtual int do_constructing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_constructing( this ) ? -1 : 1; };
    virtual int do_initializing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_initializing( this ) ? -1 : 1; };
    virtual int do_deinitializing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_deinitializing( this ) ? -1 : 1; };
    virtual int do_processing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_processing( this ) ? -1 : 1; };
    virtual int do_destructing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_destructing( this ) ? -1 : 1; };

    //---- specialization (if any) of the i_ego_obj interface
    virtual bool_t    object_allocated( void );
    virtual bool_t    object_update_list_id( void );
    //virtual ego_obj     *  get_obj_ptr(void);
    //virtual ego_obj_chr *  get_obj_chr_ptr(void);
    //virtual ego_obj_enc *  get_obj_enc_ptr(void);
    virtual ego_obj_prt *  get_obj_prt_ptr( void )  { return this; }

private:

    /// a hook to container_type, which is the parent container of this object
    const container_type * _container_ptr;
};

//--------------------------------------------------------------------------------------------
struct ego_bundle_prt
{
    PRT_REF   prt_ref;
    ego_prt   * prt_ptr;

    PIP_REF   pip_ref;
    ego_pip   * pip_ptr;

    ego_bundle_prt( ego_prt * pprt = NULL ) { ctor_this( this ); if ( NULL != pprt ) set( this, pprt ); }

    static ego_bundle_prt * ctor_this( ego_bundle_prt * pbundle );
    static ego_bundle_prt * validate( ego_bundle_prt * pbundle );
    static ego_bundle_prt * set( ego_bundle_prt * pbundle, ego_prt * pprt );

    static ego_prt & get_prt_ref( ego_bundle_prt & ref )
    {
        // handle the worst-case scenario
        if ( NULL == ref.prt_ptr ) { validate( &ref ); CPP_EGOBOO_ASSERT( NULL != ref.prt_ptr ); }

        return *( ref.prt_ptr );
    }

    static ego_prt * get_prt_ptr( ego_bundle_prt * ptr )
    {
        if ( NULL == ptr ) return NULL;

        if ( NULL == ptr->prt_ptr ) validate( ptr );

        return ptr->prt_ptr;
    }

    static const ego_prt * cget_prt_ptr( const ego_bundle_prt * ptr )
    {
        if ( NULL == ptr ) return NULL;

        // cannot do the following since the bundle is CONST... ;)
        // if( NULL == ptr->prt_ptr ) validate(ptr);

        return ptr->prt_ptr;
    }
};

//--------------------------------------------------------------------------------------------
// function prototypes

void particle_system_begin();
void particle_system_end();

void initialize_particle_physics();
void particle_physics_finalize_all( float dt );

void   init_all_pip();
void   release_all_pip();
bool_t release_one_pip( const PIP_REF & ipip );

void   free_one_particle_in_game( const PRT_REF & particle );

void update_all_particles( void );
void move_all_particles( void );
void cleanup_all_particles( void );
void increment_all_particle_update_counters( void );

void play_particle_sound( const PRT_REF & particle, Sint8 sound );

PRT_REF spawn_one_particle( fvec3_t pos, FACING_T facing, const PRO_REF & iprofile, int pip_index,
                            const CHR_REF & chr_attach, Uint16 vrt_offset, const TEAM_REF & team,
                            const CHR_REF & chr_origin, const PRT_REF & prt_origin, int multispawn, const CHR_REF & oldtarget );

#define spawn_one_particle_global( pos, facing, ipip, multispawn ) spawn_one_particle( pos, facing, PRO_REF(MAX_PROFILE), ipip, CHR_REF(MAX_CHR), GRIP_LAST, TEAM_REF(TEAM_NULL), CHR_REF(MAX_CHR), PRT_REF(MAX_PRT), multispawn, CHR_REF(MAX_CHR) );

BIT_FIELD prt_hit_wall( ego_prt * pprt, float test_pos[], float nrm[], float * pressure );
bool_t    prt_test_wall( ego_prt * pprt, float test_pos[] );
bool_t    prt_is_over_water( const PRT_REF & particle );

PIP_REF load_one_particle_profile_vfs( const char *szLoadName, const PIP_REF & pip_override );
void    reset_particles();

ego_bundle_prt * prt_calc_environment( ego_bundle_prt * pbdl );

#define _particle_h
