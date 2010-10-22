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

/// @file egoboo_object.h
/// @details Definitions of data that all Egoboo objects should "inherit"

#include "egoboo_typedef.h"
#include "egoboo_state_machine.h"

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

//--------------------------------------------------------------------------------------------

/// The state of an "egoboo object process".
/// An implementation of the same type of state machine that controls
/// the "egoboo process"
struct ego_obj_proc
{
    friend struct ego_obj_proc_data;

    // basic flags for where the object is in the creation/desctuction process
    bool_t               valid;       ///< The object is a valid object
    bool_t               constructed; ///< The object has been initialized
    bool_t               initialized; ///< The object has been initialized
    bool_t               active;      ///< The object is fully ready to go
    bool_t               killed;      ///< The object is fully "deleted"

    // other state flags
    bool_t               spawning;    ///< The object is being created
    bool_t               on;          ///< The object is being used
    bool_t               paused;      ///< The object's action will be overridden to be ego_obj_nothing

    ego_obj_actions_t    action;      ///< What action is it performing?

    static ego_obj_proc * ctor_this( ego_obj_proc * );
    static ego_obj_proc * dtor_this( ego_obj_proc * );

    static ego_obj_proc * set_valid( ego_obj_proc *, bool_t val );

    static ego_obj_proc * end_constructing( ego_obj_proc * );
    static ego_obj_proc * end_initialization( ego_obj_proc * );
    static ego_obj_proc * end_processing( ego_obj_proc * );
    static ego_obj_proc * end_deinitializing( ego_obj_proc * );
    static ego_obj_proc * end_destructing( ego_obj_proc * );
    static ego_obj_proc * end_killing( ego_obj_proc * );
    static ego_obj_proc * invalidate( ego_obj_proc * );

    static ego_obj_proc * begin_waiting( ego_obj_proc * );

private:

    static ego_obj_proc * clear( ego_obj_proc * ptr );
};

//--------------------------------------------------------------------------------------------

/// Structure for deferring communication with the ego_obj_proc.
/// Users can make requests for changes in the machine state by setting these values.
struct ego_obj_req
{
    friend struct ego_obj_proc_data;

    bool_t  unpause_me;  ///< request to un-pause the object
    bool_t  pause_me;    ///< request to pause the object
    bool_t  turn_me_on;  ///< request to turn on the object
    bool_t  turn_me_off; ///< request to turn on the object
    bool_t  kill_me;     ///< request to destroy the object

    static ego_obj_req * ctor_this( ego_obj_req * ptr );
    static ego_obj_req * dtor_this( ego_obj_req * ptr );

private:

    static ego_obj_req * clear( ego_obj_req* ptr );
};

//--------------------------------------------------------------------------------------------
/// A container for the "egoboo object process".
/// Automatically handles creation and destruction of egoboo objects.
/// Inheriting this struct will give the child a "has a" relationship to
/// The egoboo object process data.
struct ego_obj_proc_data
{
    ego_obj_proc_data * get_process_client() { return this; }

    ego_obj_proc * get_pproc() { return &_proc_data; }
    ego_obj_req     * get_preq()  { return &_req_data;   }

    ego_obj_proc & get_proc() { return _proc_data; }
    ego_obj_req     & get_req()  { return _req_data;  }

    const ego_obj_proc * cget_pproc() const { return &_proc_data; }
    const ego_obj_req     * cget_preq()  const { return &_req_data;   }

    const ego_obj_proc & cget_proc() const { return _proc_data; }
    const ego_obj_req     & cget_req()  const { return _req_data;  }

    egoboo_rv proc_validate( bool_t valid = btrue );

    egoboo_rv proc_set_wait();
    egoboo_rv proc_set_process();

    egoboo_rv proc_set_killed( bool_t val );
    egoboo_rv proc_set_on( bool_t val );
    egoboo_rv proc_set_spawning( bool_t val );

    egoboo_rv proc_req_kill();
    egoboo_rv proc_req_on( bool_t val );
    egoboo_rv proc_req_pause( bool_t val );

    egoboo_rv proc_do_on();

private:
    // "process" control control
    ego_obj_proc _proc_data;   ///< The state of the object_base "process"
    ego_obj_req  _req_data;    ///< place for making requests to change the state
};

//--------------------------------------------------------------------------------------------
/// Additional data that all egoboo object use
struct ego_obj_data
{
    // basic object definitions
    STRING         base_name;     ///< what is its name at creation. Probably related to the filename that generated it.
    Uint32         guid;          ///< a globally unique identifier

    // things related to the updating of objects
    size_t         update_count;  ///< How many updates have been made to this object?
    size_t         frame_count;   ///< How many frames have been rendered?

    unsigned       update_guid;   ///< a value that lets you know if an reference in synch with its object list
};

//--------------------------------------------------------------------------------------------

/// The full definition of an "egoboo object"
/// It has a "has a" relationship to the "egoboo object process."
/// It inherits cpp_list_client to provide some helper functions and data up to be a proper "client" for the t_cpp_list<> "server" that stores it.

struct ego_obj : public ego_obj_data, public cpp_list_client, public ego_obj_proc_data
{
    /// How many objects have been created?
    static Uint32 guid_counter;

    /// How many objects being spawned at the moment
    static Uint32 spawn_depth;

    // generic accessors so that objects which inherit this struct can access it at will
    ego_obj & get_ego_obj() { return *this; }
    ego_obj * get_pego_obj() { return this; }

    const ego_obj & cget_ego_obj() const { return *this; }
    const ego_obj * cget_pego_obj() const { return this; }

    explicit ego_obj( size_t index = size_t( -1 ) ) : cpp_list_client( index ) { ctor_this( this ); }
    ~ego_obj() { dtor_this( this ); }

    // generic construction/destruction functions
    static ego_obj * ctor_this( ego_obj * ptr ) { return ptr; }
    static ego_obj * dtor_this( ego_obj * ptr ) { return ptr; }

    // use placement new and a destructor call
    static ego_obj * ctor_all( ego_obj * ptr, size_t index )
    {
        if ( NULL != ptr )  ptr = new( ptr ) ego_obj( index ); return ptr;
    }
    static ego_obj * dtor_all( ego_obj * ptr )                { if ( NULL != ptr ) ptr->~ego_obj(); return ptr; }

    // memory management functions
    static ego_obj * allocate( ego_obj * pobj, size_t index );
    static ego_obj * deallocate( ego_obj * pobj );

    // process initialization
    static ego_obj * validate( ego_obj * );
    static ego_obj * invalidate( ego_obj * );

    // functions for changing the state of the "egoboo object process"
    static ego_obj * end_constructing( ego_obj * );
    static ego_obj * end_initializing( ego_obj * );
    static ego_obj * end_processing( ego_obj * );
    static ego_obj * end_deinitializing( ego_obj * );
    static ego_obj * end_destructing( ego_obj * );
    static ego_obj * end_killing( ego_obj * );
    static ego_obj * begin_processing( ego_obj *, const char * name );
    static ego_obj * begin_waiting( ego_obj * );

    // functions for requesting a change in the "egoboo object process"
    static ego_obj * req_terminate( ego_obj * );

    // functions for granting requests
    static ego_obj * grant_terminate( ego_obj * pobj );
    static ego_obj * grant_on( ego_obj * pobj );
    static ego_obj * set_spawning( ego_obj * pobj, bool_t val );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// pointer conversions

// Grab a pointer to the ego_obj of an object that "inherits" this data
#define POBJ_GET_PBASE( POBJ )    ( (NULL == (POBJ)) ? NULL : (POBJ)->get_pego_obj() )
#define POBJ_CGET_PBASE( POBJ )   ( (NULL == (POBJ)) ? NULL : (POBJ)->cget_pego_obj() )

// Grab a pointer to the parent container object for a data item like ego_chr, ego_enc, or ego_prt
#define PDATA_GET_POBJ( PDATA )    ( (NULL == (PDATA)) ? NULL : (PDATA)->get_pparent() )
#define PDATA_CGET_POBJ( PDATA )   ( (NULL == (PDATA)) ? NULL : (PDATA)->cget_pparent() )

// Grab a pointer to data item like ego_chr, ego_enc, or ego_prt from the parent container
#define POBJ_GET_PDATA( PDATA )    ( (NULL == (PDATA)) ? NULL : (PDATA)->get_pdata() )
#define POBJ_CGET_PDATA( PDATA )   ( (NULL == (PDATA)) ? NULL : (PDATA)->cget_pdata() )

// Grab a pointer to the ego_obj for a data item like ego_chr, ego_enc, or ego_prt
#define PDATA_GET_PBASE( PDATA )    POBJ_GET_PBASE( PDATA_GET_POBJ( PDATA ) )
#define PDATA_CGET_PBASE( PDATA )   POBJ_CGET_PBASE( PDATA_CGET_POBJ( PDATA ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Mark a ego_obj object as being allocated
#define POBJ_ALLOCATE( POBJ, INDEX ) ( (NULL == (POBJ)) ? NULL : ego_obj::allocate( POBJ_GET_PBASE(POBJ), INDEX ) )
#define PDATA_ALLOCATE( PDATA, INDEX ) POBJ_ALLOCATE( PDATA_GET_POBJ(PDATA, INDEX) )

/// Mark a ego_obj object as being deallocated
#define POBJ_INVALIDATE( POBJ ) ( (NULL == (POBJ)) ? NULL : ego_obj::invalidate( POBJ_GET_PBASE(POBJ) ) )
#define PDATA_INVALIDATE( PDATA ) POBJ_INVALIDATE( PDATA_GET_POBJ(PDATA) )

/// Turn on an ego_obj object
#define POBJ_ACTIVATE( POBJ, NAME ) ( (NULL == (POBJ)) ? NULL : ego_obj::begin_processing( POBJ_GET_PBASE(POBJ), NAME ) )
#define PDATA_ACTIVATE( PDATA, NAME ) POBJ_ACTIVATE( PDATA_GET_POBJ(PDATA), NAME )

/// Begin turning off an ego_obj object
#define POBJ_REQUEST_TERMINATE( POBJ ) ( (NULL == (POBJ)) ? NULL : ego_obj::req_terminate( POBJ_GET_PBASE(POBJ) ) )
#define PDATA_REQUEST_TERMINATE( PDATA ) POBJ_REQUEST_TERMINATE( PDATA_GET_POBJ(PDATA) )

/// Completely turn off an ego_obj object and mark it as no longer allocated
#define POBJ_TERMINATE( POBJ ) ( (NULL == (POBJ)) ? NULL : ego_obj::grant_terminate( POBJ_GET_PBASE(POBJ) ) )
#define PDATA_TERMINATE( PDATA ) POBJ_TERMINATE( PDATA_GET_POBJ(PDATA) )

#define POBJ_BEGIN_SPAWN( POBJ ) \
    if( (NULL != (POBJ)) && FLAG_VALID_PBASE(POBJ_CGET_PBASE(POBJ))  ) \
    {\
        if( !(POBJ)->cget_ego_obj().cget_proc().spawning )\
        {\
            ego_obj::set_spawning( POBJ_GET_PBASE(POBJ), btrue );\
            ego_obj::spawn_depth++;\
        }\
    }\
     
#define POBJ_END_SPAWN( POBJ ) \
    if( (NULL != (POBJ)) && FLAG_VALID_PBASE(POBJ_CGET_PBASE(POBJ)) ) \
    {\
        if( (POBJ)->cget_ego_obj().cget_proc().spawning )\
        {\
            ego_obj::set_spawning( POBJ_GET_PBASE(POBJ), bfalse );\
            ego_obj::spawn_depth--;\
        }\
    }\
     
/// Is the object flagged as allocated?
#define FLAG_ALLOCATED_PBASE( PBASE ) ( (PBASE)->get_allocated() )
/// Is the object allocated?
#define ALLOCATED_PBASE( PBASE )       ( (NULL != (PBASE)) && FLAG_ALLOCATED_PBASE(PBASE) )

/// Is the object flagged as valid?
#define FLAG_VALID_PBASE( PBASE ) ( (PBASE)->cget_proc().valid )
/// Is the object valid?
#define VALID_PBASE( PBASE )       ( (NULL != (PBASE))  && FLAG_ALLOCATED_PBASE( PBASE ) && FLAG_VALID_PBASE(PBASE) )

/// Is the object flagged as constructed?
#define FLAG_CONSTRUCTED_PBASE( PBASE ) ( (PBASE)->cget_proc().constructed )
/// Is the object constructed?
#define CONSTRUCTED_PBASE( PBASE )       ( VALID_PBASE( PBASE ) && FLAG_CONSTRUCTED_PBASE(PBASE) )

/// Is the object flagged as initialized?
#define FLAG_INITIALIZED_PBASE( PBASE ) ( (PBASE)->cget_proc().initialized )
/// Is the object initialized?
#define INITIALIZED_PBASE( PBASE )      ( CONSTRUCTED_PBASE( PBASE ) && FLAG_INITIALIZED_PBASE(PBASE) )

/// Is the object flagged as killed?
#define FLAG_KILLED_PBASE( PBASE ) ( (PBASE)->cget_proc().killed )
/// Is the object killed?
#define KILLED_PBASE( PBASE )       ( VALID_PBASE( PBASE ) && FLAG_KILLED_PBASE(PBASE) )

/// Is the object flagged as requesting termination?
#define FLAG_ON_PBASE( PBASE )  ( (PBASE)->cget_proc().on )
/// Is the object on?
#define ON_PBASE( PBASE )       ( VALID_PBASE( PBASE ) && FLAG_ON_PBASE(PBASE) && !FLAG_KILLED_PBASE( PBASE ) )

/// Is the object flagged as kill_me?
#define FLAG_REQ_TERMINATION_PBASE( PBASE ) ( (PBASE)->get_req().kill_me )
/// Is the object kill_me?
#define REQ_TERMINATION_PBASE( PBASE )      ( VALID_PBASE( PBASE ) && FLAG_REQ_TERMINATION_PBASE(PBASE) )

/// Has the object been created yet?
#define STATE_CONSTRUCTING_PBASE( PBASE ) ( (ego_obj_constructing == (PBASE)->cget_proc().action) )
/// Has the object been created yet?
#define CONSTRUCTING_PBASE( PBASE )       ( VALID_PBASE( PBASE ) && STATE_CONSTRUCTING_PBASE(PBASE) )

/// Is the object in the initializing state?
#define STATE_INITIALIZING_PBASE( PBASE ) ( (ego_obj_initializing == (PBASE)->cget_proc().action) )
/// Is the object being initialized right now?
#define INITIALIZING_PBASE( PBASE )       ( CONSTRUCTED_PBASE( PBASE ) && STATE_INITIALIZING_PBASE(PBASE) )

/// Is the object in the active state?
#define STATE_PROCESSING_PBASE( PBASE ) ( ego_obj_processing == (PBASE)->cget_proc().action )
/// Is the object active?
#define ACTIVE_PBASE( PBASE )           ( INITIALIZED_PBASE( PBASE ) && STATE_PROCESSING_PBASE(PBASE) && FLAG_ON_PBASE(PBASE) && !FLAG_KILLED_PBASE( PBASE ) )

/// Is the object in the deinitializing state?
#define STATE_DEINITIALIZING_PBASE( PBASE ) ( (ego_obj_deinitializing == (PBASE)->cget_proc().action) )
/// Is the object being deinitialized right now?
#define DEINITIALIZING_PBASE( PBASE )       ( CONSTRUCTED_PBASE( PBASE ) && STATE_DEINITIALIZING_PBASE(PBASE) )

/// Is the object in the destructing state?
#define STATE_DESTRUCTING_PBASE( PBASE ) ( (ego_obj_destructing == (PBASE)->cget_proc().action) )
/// Is the object being deinitialized right now?
#define DESTRUCTING_PBASE( PBASE )       ( VALID_PBASE( PBASE ) && STATE_DESTRUCTING_PBASE(PBASE) )

/// Is the object "waiting to die" state?
#define STATE_WAITING_PBASE( PBASE ) ( ego_obj_waiting == (PBASE)->cget_proc().action )
/// Is the object "waiting to die"?
#define WAITING_PBASE( PBASE )       ( VALID_PBASE( PBASE ) && STATE_WAITING_PBASE(PBASE) )

/// Has the object been marked as terminated? (alias for KILLED_PBASE)
#define STATE_TERMINATED_PBASE( PBASE ) FLAG_KILLED_PBASE(PBASE)
#define TERMINATED_PBASE( PBASE )       KILLED_PBASE( PBASE )

// Grab the index value of ego_obj pointer
#define GET_IDX_PBASE( PBASE, FAIL_VALUE ) ( (NULL == (PBASE) || !VALID_PBASE( PBASE ) ) ? FAIL_VALUE : (PBASE)->get_index() )
#define GET_REF_PBASE( PBASE, FAIL_VALUE ) ((REF_T)GET_IDX_PBASE( PBASE, FAIL_VALUE ))

// Grab the index value of object that "inherits" from ego_obj
#define GET_IDX_POBJ( POBJ, FAIL_VALUE )  GET_IDX_PBASE(POBJ_CGET_PBASE( POBJ ), FAIL_VALUE)
#define GET_REF_POBJ( POBJ, FAIL_VALUE )  ((REF_T)GET_IDX_POBJ( POBJ, FAIL_VALUE ))

/// Grab the state of object that "inherits" from ego_obj
#define GET_STATE_POBJ( POBJ )  ( (NULL == (POBJ) || !VALID_PBASE( POBJ_CGET_PBASE( POBJ ) ) ) ? ego_obj_nothing : (POBJ)->get_ego_obj().get_index() )

/// Is the object active?
#define DEFINED_PBASE( PBASE )  ( FLAG_ON_PBASE(PBASE) && !FLAG_KILLED_PBASE( PBASE ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define _egoboo_object_h

