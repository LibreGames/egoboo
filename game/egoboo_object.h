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

/// \file egoboo_object.h
/// \details Definitions of data that all Egoboo objects should "inherit"

#include "egoboo_typedef_cpp.h"
#include "egoboo_state_machine.h"

#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_obj;

struct ego_chr;
struct ego_enc;
struct ego_prt;

struct ego_obj_chr;
struct ego_obj_enc;
struct ego_obj_prt;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some basic data that all Egoboo objects should have

/// The possible actions that an ego_obj object can perform
enum e_ego_obj_actions
{
    ego_obj_nothing        = ego_action_invalid,
    ego_obj_constructing   = ego_action_beginning,   ///< The object has been allocated and had its critical variables filled with safe values
    ego_obj_initializing   = ego_action_entering,    ///< The object is being initialized/re-initialized
    ego_obj_processing     = ego_action_running,     ///< The object is fully activated
    ego_obj_deinitializing = ego_action_leaving,     ///< The object is being de-initialized
    ego_obj_destructing    = ego_action_finishing,   ///< The object is being destructed

    // the states that are specific to objects
    ego_obj_waiting                                  ///< The object has been fully destructed and is awaiting final "deletion"
};

typedef enum e_ego_obj_actions ego_obj_actions_t;

/// the abstract interface to the "egoboo object process"
struct i_ego_object_process
{
    /// constructing method
    virtual int do_constructing( void ) = 0;
    /// initializing method
    virtual int do_initializing( void ) = 0;
    /// deinitializing method
    virtual int do_deinitializing( void ) = 0;
    /// processing method
    virtual int do_processing( void ) = 0;
    /// destructing method
    virtual int do_destructing( void ) = 0;

    //---- accessor methods
    virtual ego_obj * get_obj( void ) = 0;
    virtual ego_chr * get_chr( void ) = 0;
    virtual ego_enc * get_enc( void ) = 0;
    virtual ego_prt * get_prt( void ) = 0;
};

//--------------------------------------------------------------------------------------------

/// an abstract interface to the state of an "egoboo object process".
struct i_ego_object_process_state
{
    virtual int end_constructing() = 0;
    virtual int end_initializing() = 0;
    virtual int end_processing() = 0;
    virtual int end_deinitializing() = 0;
    virtual int end_destructing() = 0;
    virtual int end_invalidating() = 0;

    virtual int begin_waiting() = 0;
};

//--------------------------------------------------------------------------------------------
/// a helper struct so that the data values of ego_object_process_state can be simply cleared
/// without destroying the virtual function table
struct ego_object_process_state_data
{
    friend struct ego_object_process_engine;

    typedef ego_object_process_state_data its_type;

    // implement the flags as a bitmask
    enum
    {
        valid_bit       = 1 << 0,           ///< The object is a valid object
        constructed_bit = 1 << 1,           ///< The object has been initialized
        initialized_bit = 1 << 2,           ///< The object has been initialized
        killed_bit      = 1 << 3,           ///< The object is fully "deleted"

        // other state flags
        active_bit     = 1 << 4,          ///< The object is fully ready to go
        spawning_bit   = 1 << 5,          ///< The object is being created
        on_bit         = 1 << 6,          ///< The object is being used
        paused_bit     = 1 << 7,          ///< The object's action will be overridden to be ego_obj_nothing

        // aliases

        full_constructed = valid_bit | constructed_bit,
        full_initialized = valid_bit | constructed_bit | initialized_bit,
        full_on          = valid_bit | constructed_bit | initialized_bit | on_bit,
        full_active      = valid_bit | constructed_bit | initialized_bit | active_bit,
        full_spawning    = valid_bit | spawning_bit
    };

    //---- static accessors
    // use static functions as accessors so the program won't crash on an accidental NULL pointer

    static bool_t set_valid( its_type * ptr, const bool_t val );
    static bool_t set_action( its_type * ptr, const ego_obj_actions_t action ) { if ( NULL == ptr ) return bfalse; bool_t rv = ( action != ptr->action ); ptr->action = action; return rv; }

    static bool_t get_valid( const its_type * ptr )       { return ( NULL == ptr ) ? bfalse : has_all_bits( ptr, valid_bit )        && has_no_bits( ptr, killed_bit ); }
    static bool_t get_initialized( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : has_all_bits( ptr, full_initialized ) && has_no_bits( ptr, killed_bit ); }
    static bool_t get_constructed( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : has_all_bits( ptr, full_constructed ) && has_no_bits( ptr, killed_bit ); }
    static bool_t get_active( const its_type * ptr )      { return ( NULL == ptr ) ? bfalse : has_all_bits( ptr, full_active )      && has_no_bits( ptr, killed_bit ); }
    static bool_t get_killed( const its_type * ptr )      { return ( NULL == ptr ) ? btrue : has_all_bits( ptr, killed_bit ); }

    static bool_t get_spawning( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : has_all_bits( ptr, full_spawning ) && has_no_bits( ptr, killed_bit ); }
    static bool_t get_on( const its_type * ptr )       { return ( NULL == ptr ) ? bfalse : has_all_bits( ptr, full_on ) && has_no_bits( ptr, killed_bit ); }
    static bool_t get_paused( const its_type * ptr )   { return ( NULL == ptr ) ? bfalse : has_all_bits( ptr, paused_bit ) && has_no_bits( ptr, killed_bit ); }

    static ego_obj_actions_t get_action( const its_type * ptr ) { return ( NULL == ptr ) ? ego_obj_nothing : ptr->action;   }

    static its_type * invalidate( its_type * ptr ) { return dtor_this( ptr ); }

    //---- non-static accessors

    bool_t set_valid( const bool_t val );
    bool_t set_action( ego_obj_actions_t act ) { bool_t rv = ( action != act ); action = act; return rv; }

    bool_t get_valid()       const { return has_all_bits( this, valid_bit )        && has_no_bits( this, killed_bit ); }
    bool_t get_initialized() const { return has_all_bits( this, full_initialized ) && has_no_bits( this, killed_bit ); }
    bool_t get_constructed() const { return has_all_bits( this, full_constructed ) && has_no_bits( this, killed_bit ); }
    bool_t get_active()      const { return has_all_bits( this, full_active )      && has_no_bits( this, killed_bit ); }
    bool_t get_killed()      const { return has_all_bits( this, killed_bit ); }

    bool_t get_spawning()    const { return has_all_bits( this, full_spawning ) && has_no_bits( this, killed_bit ); }
    bool_t get_on()          const { return has_all_bits( this, full_on ) && has_no_bits( this, killed_bit ); }
    bool_t get_paused()      const { return has_all_bits( this, paused_bit ) && has_no_bits( this, killed_bit ); }

    ego_obj_actions_t get_action() const { return action; }

    its_type * invalidate() { return dtor_this( this ); }

    static bool_t has_all_bits( const its_type* ptr, const ego_uint flags )
    {
        if ( NULL == ptr ) return bfalse;

        return flags == ( flags & ptr->state_flags );
    }

    static bool_t has_no_bits( const its_type* ptr, const ego_uint flags )
    {
        if ( NULL == ptr ) return bfalse;

        return 0 == ( flags & ptr->state_flags );
    }

    bool_t require_action( ego_obj_actions_t req_action ) const
    {
        if ( NULL == this ) return bfalse;

        return req_action == action;
    }

    bool_t require_bits( const ego_uint flags ) const
    {
        if ( NULL == this ) return bfalse;

        return flags == ( flags & state_flags );
    }

    bool_t reject_bits( const ego_uint flags ) const
    {
        if ( NULL == this ) return bfalse;

        return 0 == ( flags & state_flags );
    }

protected:

    explicit ego_object_process_state_data( ego_obj_actions_t state = ego_obj_nothing ) { ctor_this( this, state ); }
    ~ego_object_process_state_data() { dtor_this( this ); }

    static ego_object_process_state_data * ctor_this( ego_object_process_state_data * ptr, const ego_obj_actions_t state = ego_obj_nothing )
    {
        if ( NULL == ptr ) return ptr;

        // remove all the flags
        ptr->state_flags = 0;

        // set the state to construct the object
        ptr->action = state;

        return ptr;
    }

    static ego_object_process_state_data * dtor_this( ego_object_process_state_data * ptr )
    {
        if ( NULL == ptr ) return ptr;

        // remove all the flags
        ptr->state_flags = 0;

        // set the state to construct the object
        ptr->state_flags = ego_obj_nothing;

        return ptr;
    }

    static bool_t add_bits( its_type* ptr, const ego_uint flags )
    {
        if ( NULL == ptr ) return bfalse;

        ego_uint old = ptr->state_flags;

        ptr->state_flags |= flags;

        return old != ptr->state_flags;
    }

    static bool_t remove_bits( its_type* ptr, const ego_uint flags )
    {
        if ( NULL == ptr ) return bfalse;

        ego_uint old = ptr->state_flags;

        ptr->state_flags &= ~flags;

        return old != ptr->state_flags;
    }

    static bool_t set_on( its_type * ptr, const bool_t val )
    {
        bool_t rv = bfalse;

        if ( val )
        {
            rv = add_bits( ptr, on_bit );
        }
        else
        {
            rv = remove_bits( ptr, on_bit );
        }

        return rv;
    }

private:
    ego_obj_actions_t    action;      ///< What action is it performing?

    BIT_FIELD            state_flags;
};

//--------------------------------------------------------------------------------------------
/// The state of an "egoboo object process".
/// An implementation of the same type of state machine that controls
/// the "egoboo process"
struct ego_object_process_state : public i_ego_object_process_state, public ego_object_process_state_data
{
    //---- implementation of the i_ego_object_process_state methods

    virtual int end_constructing();
    virtual int end_initializing();
    virtual int end_processing();
    virtual int end_deinitializing();
    virtual int end_destructing();
    virtual int end_invalidating();

    virtual int begin_waiting();
};

//--------------------------------------------------------------------------------------------

/// Structure for deferring communication with the ego_object_process_state.
/// Users can make requests for changes in the machine state by setting these values.
struct ego_object_request_data
{
    friend struct ego_object_process_engine;

    typedef ego_object_request_data its_type;

    //---- ACCESSORS
    // use static functions as accessors so the program won't crash on am accidental NULL pointer

    static bool_t  get_pause_off( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : ptr->unpause_me;  };
    static bool_t  get_pause_on( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : ptr->pause_me;    };
    static bool_t  get_turn_on( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : ptr->turn_me_on;  };
    static bool_t  get_turn_off( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : ptr->turn_me_off; };
    static bool_t  get_kill( const its_type * ptr ) { return ( NULL == ptr ) ? bfalse : ptr->kill_me;     };

protected:

    ego_object_request_data() { clear(); }
    ~ego_object_request_data() { clear(); }

    /// \note MAKE SURE there are no virtual functions or any members that are constructed
    /// before you use memset like this
    void clear() { SDL_memset( this, 0, sizeof( *this ) ); };

    bool_t  unpause_me;  ///< request to un-pause the object
    bool_t  pause_me;    ///< request to pause the object
    bool_t  turn_me_on;  ///< request to turn on the object
    bool_t  turn_me_off; ///< request to turn on the object
    bool_t  kill_me;     ///< request to destroy the object
};

//--------------------------------------------------------------------------------------------

/// a concrete "egoboo object process"
struct ego_object_process : public i_ego_object_process, public ego_object_request_data, public ego_object_process_state
{
    //---- default implementation of the virtual methods

    virtual int do_constructing()   { return end_constructing();   }
    virtual int do_initializing()   { return end_initializing();   }
    virtual int do_deinitializing() { return end_deinitializing(); }
    virtual int do_processing()     { return end_processing();     }
    virtual int do_destructing()    { return end_destructing();    }

    //---- accessor methods
    virtual ego_obj * get_obj() { return NULL; }
    virtual ego_chr * get_chr() { return NULL; }
    virtual ego_enc * get_enc() { return NULL; }
    virtual ego_prt * get_prt() { return NULL; }

    ego_object_process * get_object_process() { return this; }

    static egoboo_rv validate( ego_object_process * ptr, const bool_t val = btrue );

    egoboo_rv proc_req_on( const bool_t val );
    egoboo_rv proc_req_pause( const bool_t val );
    egoboo_rv proc_req_terminate();

    egoboo_rv proc_set_wait();
    egoboo_rv proc_set_process();
    egoboo_rv proc_set_killed( const bool_t val );
    egoboo_rv proc_set_on( const bool_t val );
    egoboo_rv proc_do_on();
    egoboo_rv proc_set_spawning( const bool_t val );
};

//--------------------------------------------------------------------------------------------
struct ego_object_process_engine
{
    static ego_object_process * run( ego_object_process * );
};

extern ego_object_process_engine obj_proc_engine;

//--------------------------------------------------------------------------------------------

/// All addtional data used by ego_obj
struct ego_obj_data
{
    //---- public data

    // all ego_obj have a name
    STRING         obj_name;                       ///< what is its name at creation. Probably related to the filename that generated it.

    // all ego_obj can count their updates
    size_t         update_count;                   ///< How many updates have been made to this object?
    size_t         frame_count;                    ///< How many frames have been rendered?

    // all ego_obj can be placed in a BSP
    ego_BSP_leaf      bsp_leaf;                    ///< where this object is in its BSP

    // all ego_obj can collide with characters
    ego_oct_bb        max_cv;                      ///< largest possible collision volume for this object

    //---- constructors
    ego_obj_data( const Uint32 guid ) : _id( guid ) { clear(); ctor_this( this ); }
    ~ego_obj_data()
    {
        dtor_this( this );
        clear();

        // do something stupid to set the id to a "bad" value
        *(( Uint32 * )&_id ) = Uint32( ~0 );
    }

    //---- accessors
    const Uint32 get_id() const { return _id; }

private:

    //---- construction
    static ego_obj_data * ctor_this( ego_obj_data * ptr );
    static ego_obj_data * dtor_this( ego_obj_data * ptr );

    static ego_obj_data * alloc( ego_obj_data * pobj );
    static ego_obj_data * dealloc( ego_obj_data * pobj );

    void clear();

    //---- private data
    const Uint32   _id;            ///< a globally unique identifier for each object

};

//--------------------------------------------------------------------------------------------
/// an interface for functions that can only be evaluated by an object that inherits ego_obj
struct i_ego_obj
{
    //---- functions that ego_obj cannot call on its own
    virtual bool_t object_allocated( void ) = 0;
    virtual bool_t object_update_list_id( void ) = 0;

    //---- pointers to classes that inherit this interface
    virtual ego_obj     * get_obj_ptr( void )     = 0;
    virtual ego_obj_chr * get_obj_chr_ptr( void ) = 0;
    virtual ego_obj_enc * get_obj_enc_ptr( void ) = 0;
    virtual ego_obj_prt * get_obj_prt_ptr( void ) = 0;

    virtual void          update_max_cv() = 0;
    virtual void          update_bsp() = 0;
};

//--------------------------------------------------------------------------------------------

/// The base class for an "egoboo object"
/// The purpose of this struct is to fully implement the the idea of a process for all objects

struct ego_obj : public i_ego_obj, public ego_obj_data, public ego_object_process
{
    friend struct ego_object_engine;

    /// force use the guid_counter to automatically assign a unique id to an ego_object
    /// upon creation
    explicit ego_obj() : ego_obj_data( guid_counter++ ) {}

    static ego_obj * reallocate_all( ego_obj * ptr )
    {
        ptr = deallocate_all( ptr );
        ptr = allocate_all( ptr );
        return ptr;
    }

    // process initialization
    static ego_obj * validate( ego_obj *, const bool_t val = btrue );

    static ego_obj * begin_processing( ego_obj * , const char * name = "*UNKNOWN*" );

    // functions for requesting a change in the "egoboo object process"
    static ego_obj * req_terminate( ego_obj * );

    // functions for granting requests
    static ego_obj * grant_terminate( ego_obj * pobj );
    static ego_obj * grant_on( ego_obj * pobj );
    static ego_obj * set_spawning( ego_obj * pobj, const bool_t val );

    static Uint32    get_spawn_depth()       { return spawn_depth; };
    static Uint32    increment_spawn_depth() { return ++spawn_depth; };
    static Uint32    decrement_spawn_depth() { return --spawn_depth; };

    // generic accessors so that objects which inherit this struct can access it at will
    static       ego_obj &  get_ego_obj_ref( ego_obj & ref ) { return ref; }
    static const ego_obj & cget_ego_obj_ref( const ego_obj & ref ) { return ref; }
    static       ego_obj *  get_ego_obj_ptr( ego_obj * ptr ) { return ptr; }
    static const ego_obj * cget_ego_obj_ptr( const ego_obj * ptr ) { return ptr; }

    //---- implementation of the i_ego_obj interface
    virtual bool_t        object_allocated( void )   { return bfalse; }
    virtual bool_t        object_update_list_id( void ) { return bfalse; }
    virtual ego_obj     * get_obj_ptr( void )        { return this; }
    virtual ego_obj_chr * get_obj_chr_ptr( void )    { return NULL; }
    virtual ego_obj_enc * get_obj_enc_ptr( void )    { return NULL; }
    virtual ego_obj_prt * get_obj_prt_ptr( void )    { return NULL; }

    virtual void          update_max_cv()            { max_cv.invalidate();  }
    virtual void          update_bsp()               { update_max_cv(); }

protected:

    // use placement new and a destructor call
    static ego_obj * ctor_all( ego_obj * ptr ) { if ( NULL != ptr )  ptr = new( ptr ) ego_obj; return ptr; }
    static ego_obj * dtor_all( ego_obj * ptr ) { if ( NULL != ptr ) ptr->~ego_obj(); return ptr; }

    static ego_obj * allocate_all( ego_obj * ptr );
    static ego_obj * deallocate_all( ego_obj * ptr );

    // completely reallocate this object
    static ego_obj * retor_all( ego_obj * ptr )
    {
        ptr = ego_obj::dtor_all( ptr );
        ptr = ego_obj::ctor_all( ptr );
        return ptr;
    }

    /// How many objects have been created?
    static Uint32 guid_counter;

    /// How many objects being spawned at the moment
    static Uint32 spawn_depth;

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the engine that runs an "egoboo object process"
struct ego_object_engine
{
    static ego_obj * run( ego_obj * );

    template <typename _ty> static _ty * run_construct( _ty * pobj, const int max_iterations );
    template <typename _ty> static _ty * run_initialize( _ty * pobj, const int max_iterations );
    template <typename _ty> static _ty * run_activate( _ty * pobj, const int max_iterations );
    template <typename _ty> static _ty * run_deinitialize( _ty * pobj, const int max_iterations );
    template <typename _ty> static _ty * run_deconstruct( _ty * pobj, const int max_iterations );
};

extern ego_object_engine obj_engine;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// pointer conversions

// Grab a pointer to the parent container object for a data item like ego_chr, ego_enc, or ego_prt
#define PDATA_GET_POBJ( TYPE, PDATA )    ( TYPE::get_obj_ptr(PDATA) )
#define PDATA_CGET_POBJ( TYPE, PDATA )   ( TYPE::cget_obj_ptr(PDATA) )

// Grab a pointer to data item like ego_chr, ego_enc, or ego_prt from the parent container
#define POBJ_GET_PDATA( TYPE, PDATA )    ( TYPE::get_data_ptr(PDATA) )
#define POBJ_CGET_PDATA( TYPE, PDATA )   ( TYPE::cget_data_ptr(PDATA) )

// Grab a pointer to the ego_obj for a data item like ego_chr, ego_enc, or ego_prt
#define PDATA_GET_PBASE( TYPE, PDATA )    PDATA_GET_POBJ( TYPE, PDATA )
#define PDATA_CGET_PBASE( TYPE, PDATA )   PDATA_CGET_POBJ( TYPE, PDATA )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Mark a ego_obj object as being allocated
#define POBJ_ALLOCATE( POBJ, INDEX )   ego_obj::reallocate( POBJ, INDEX )
#define PDATA_ALLOCATE( TYPE, PDATA, INDEX ) POBJ_ALLOCATE( PDATA_GET_POBJ(TYPE, PDATA, INDEX) )

/// Mark a ego_obj object as being deallocated
#define POBJ_INVALIDATE( POBJ )   ego_obj::invalidate( POBJ )
#define PDATA_INVALIDATE( TYPE, PDATA ) POBJ_INVALIDATE( PDATA_GET_POBJ(TYPE, PDATA) )

/// Turn on an ego_obj object
#define POBJ_ACTIVATE( POBJ, NAME )   ego_obj::begin_processing( POBJ, NAME )
#define PDATA_ACTIVATE( TYPE, PDATA, NAME ) POBJ_ACTIVATE( PDATA_GET_POBJ(TYPE, PDATA), NAME )

/// Begin turning off an ego_obj object
#define POBJ_REQUEST_TERMINATE( POBJ )   ego_obj::req_terminate( POBJ )
#define PDATA_REQUEST_TERMINATE( TYPE, PDATA ) POBJ_REQUEST_TERMINATE( PDATA_GET_POBJ(TYPE, PDATA) )

/// Completely turn off an ego_obj object and mark it as no longer allocated
#define POBJ_TERMINATE( POBJ )   ego_obj::grant_terminate( POBJ )
#define PDATA_TERMINATE( TYPE, PDATA ) POBJ_TERMINATE( PDATA_GET_POBJ(TYPE, PDATA) )

#define POBJ_BEGIN_SPAWN( POBJ ) \
    if( (NULL != (POBJ)) && FLAG_VALID_PBASE(POBJ)  ) \
    {\
        if( !ego_obj::get_spawning(POBJ) )\
        {\
            ego_obj::set_spawning( POBJ, btrue );\
            ego_obj::increment_spawn_depth();\
        }\
    }\
     
#define POBJ_END_SPAWN( POBJ ) \
    if( (NULL != (POBJ)) && FLAG_VALID_PBASE(POBJ) ) \
    {\
        if( ego_obj::get_spawning(POBJ) )\
        {\
            ego_obj::set_spawning( POBJ, bfalse );\
            ego_obj::decrement_spawn_depth();\
        }\
    }\
     
/// Is the object flagged as valid?
#define FLAG_VALID_PBASE( PBASE ) ego_object_process_state_data::get_valid(PBASE)

/// Is the object flagged as constructed?
#define FLAG_CONSTRUCTED_PBASE( PBASE ) ego_object_process_state_data::get_constructed(PBASE)

/// Is the object flagged as initialized?
#define FLAG_INITIALIZED_PBASE( PBASE ) ego_obj::get_initialized(PBASE)

/// Has the object been marked as terminated? (alias for FLAG_TERMINATED_PBASE)
#define FLAG_TERMINATED_PBASE( PBASE ) ego_obj::get_killed(PBASE)

/// Is the object flagged as requesting termination?
#define FLAG_ON_PBASE( PBASE )  ego_obj::get_on(PBASE)

/// Is the object flagged as kill_me?
#define FLAG_REQ_TERMINATION_PBASE( PBASE ) ego_obj::get_kill(PBASE)

/// Has the object in the constructing state?
#define STATE_CONSTRUCTING_PBASE( PBASE ) (ego_obj_constructing == ego_obj::get_action(PBASE))

/// Is the object in the initializing state?
#define STATE_INITIALIZING_PBASE( PBASE ) ( ego_obj_initializing == ego_obj::get_action(PBASE) )

/// Is the object in the processing state?
#define STATE_PROCESSING_PBASE( PBASE ) ( ego_obj_processing == ego_obj::get_action(PBASE) )

/// Is the object in the deinitializing state?
#define STATE_DEINITIALIZING_PBASE( PBASE ) ( ego_obj_deinitializing == ego_obj::get_action(PBASE) )

/// Is the object in the destructing state?
#define STATE_DESTRUCTING_PBASE( PBASE )  ( ego_obj_destructing == ego_obj::get_action(PBASE) )

/// Is the object "waiting to die" state?
#define STATE_WAITING_PBASE( PBASE ) ( ego_obj_waiting == ego_object_process_state_data::get_action(PBASE) )

/// Is the object kill_me?
#define REQ_TERMINATION_PBASE( PBASE )    ( FLAG_VALID_PBASE( PBASE ) && !FLAG_TERMINATED_PBASE(PBASE) && FLAG_REQ_TERMINATION_PBASE(PBASE) )

/// Has the object been created yet?
#define CONSTRUCTING_PBASE( PBASE )       ( FLAG_VALID_PBASE( PBASE ) && !FLAG_TERMINATED_PBASE(PBASE) && STATE_CONSTRUCTING_PBASE(PBASE) )

/// Is the object being initialized right now?
#define INITIALIZING_PBASE( PBASE )       ( FLAG_CONSTRUCTED_PBASE( PBASE ) && !FLAG_TERMINATED_PBASE(PBASE) && STATE_INITIALIZING_PBASE(PBASE) )

/// Is the object active?
#define PROCESSING_PBASE( PBASE )         ( FLAG_ON_PBASE(PBASE) && !FLAG_TERMINATED_PBASE(PBASE) && STATE_PROCESSING_PBASE(PBASE) )

/// Is the object being deinitialized right now?
#define DEINITIALIZING_PBASE( PBASE )     (  FLAG_CONSTRUCTED_PBASE( PBASE ) && !FLAG_TERMINATED_PBASE(PBASE) && STATE_DEINITIALIZING_PBASE(PBASE) )

/// Is the object being deinitialized right now?
#define DESTRUCTING_PBASE( PBASE )        ( FLAG_VALID_PBASE( PBASE ) && !FLAG_TERMINATED_PBASE(PBASE) && STATE_DESTRUCTING_PBASE(PBASE) )

/// Is the object "waiting to die" state?
#define WAITING_PBASE( PBASE )           ( FLAG_VALID_PBASE( PBASE ) && !FLAG_TERMINATED_PBASE(PBASE) && STATE_WAITING_PBASE( PBASE ) )

/// Is the object active?
#define DEFINED_PBASE( PBASE )  ( FLAG_VALID_PBASE(PBASE) && !FLAG_TERMINATED_PBASE( PBASE ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define _egoboo_object_h
