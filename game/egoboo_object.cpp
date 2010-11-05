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

/// \file egoboo_object.c
/// \brief Implementation of Egoboo "object" control routines
/// \details

#include "egoboo_strutil.h"

#include "ChrList.h"
#include "EncList.h"
#include "PrtList.h"

#include "egoboo_object.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 ego_obj::guid_counter   = 0;
Uint32 ego_obj::spawn_depth    = 0;

// it should not matter whether these are instantiated since all they contain is
// static functions, but...
ego_object_process_engine obj_proc_engine;
ego_object_engine         obj_engine;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int ego_object_process_state::end_constructing()
{
    if ( !get_valid( this ) ) return -1;

    add_bits( this, full_constructed );

    set_action( this, ego_obj_initializing );

    return 1;
}

//--------------------------------------------------------------------------------------------
int ego_object_process_state::end_initializing()
{
    // the two add_bits statements are equivalent to "full_active",
    // add_bits( this, full_active ), but it is clearer to do it this way...

    if ( !get_valid( this ) ) return -1;

    // the obvious things you must do when "end_initializing"
    add_bits( this, full_initialized );
    set_action( this, ego_obj_processing );

    // the additional thing we must do since there is no seperate
    // command to place the object "in game"
    add_bits( this, on_bit | active_bit );

    return 1;
}

//--------------------------------------------------------------------------------------------
int ego_object_process_state::end_processing()
{
    if ( !get_valid( this ) ) return -1;

    remove_bits( this, active_bit | on_bit );

    set_action( this, ego_obj_deinitializing );

    return 1;
}

//--------------------------------------------------------------------------------------------
int ego_object_process_state::end_deinitializing()
{
    if ( !get_valid( this ) ) return -1;

    remove_bits( this, initialized_bit | active_bit | on_bit );

    set_action( this, ego_obj_destructing );

    return 1;
}

//--------------------------------------------------------------------------------------------
int ego_object_process_state::end_destructing()
{
    if ( !get_valid( this ) ) return -1;

    remove_bits( this, constructed_bit | initialized_bit | active_bit | on_bit );

    add_bits( this, killed_bit );

    set_action( this, ego_obj_waiting );

    return 1;
}

//--------------------------------------------------------------------------------------------
int ego_object_process_state::end_invalidating()
{
    if ( !get_valid( this ) ) return -1;

    ego_object_process_state_data::invalidate( this );

    add_bits( this, killed_bit );

    return 1;
}

//--------------------------------------------------------------------------------------------
bool_t ego_object_process_state_data::set_valid( ego_object_process_state_data * ptr, bool_t val )
{
    if ( NULL == ptr ) return bfalse;

    // set the state to the corect values
    if ( val )
    {
        ptr->state_flags = valid_bit;
        ptr->action      = ego_obj_constructing;
    }
    else
    {
        ptr->state_flags = 0;
        ptr->action      = ego_obj_nothing;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int ego_object_process_state::begin_waiting( )
{
    // put the object in the "waiting to be killed" mode. currently used only by particles

    if ( NULL == this || !get_valid( this ) || get_killed( this ) ) return -1;

    set_action( this, ego_obj_waiting );

    return 1;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::validate( ego_object_process * ptr, bool_t val )
{
    if ( NULL == ptr ) return rv_error;

    // clear out the state
    ego_object_process_state::invalidate( ptr );

    // clear out any requests
    ptr->ego_object_request_data::clear();

    // set the "valid" variable and initialize the action
    set_valid( ptr, val );

    return ( val == get_valid( ptr ) ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_req_terminate()
{
    if ( !get_valid( this ) ) return rv_error;

    if ( get_killed( this ) ) return rv_success;

    // save the old value
    bool_t was_kill_me = kill_me;

    // do the request
    kill_me = btrue;

    // turn the object off
    proc_set_on( bfalse );

    return was_kill_me ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_req_on( bool_t val )
{
    if ( !get_valid( this ) || get_killed( this ) ) return rv_error;

    bool_t needed = ( val != turn_me_on );

    turn_me_on  = val;
    turn_me_off = !val;

    return needed ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_req_pause( bool_t val )
{
    if ( !get_valid( this ) || get_killed( this ) ) return rv_error;

    bool_t needed = ( val != pause_me );

    pause_me   = val;
    unpause_me = !val;

    return needed ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_set_wait()
{
    if ( NULL == this ) return rv_error;

    int rv = ego_object_process::begin_waiting();

    return ( rv > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_set_process()
{
    if ( !get_valid( this ) || get_killed( this ) ) return rv_error;

    // don't bother if there is already a request to kill it
    if ( kill_me ) return rv_fail;

    // check the requirements for activating the object
    if ( !get_constructed( this ) || !get_initialized( this ) )
        return rv_fail;

    set_action( this, ego_obj_processing );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_set_killed( bool_t val )
{
    if ( NULL == this ) return rv_error;

    // don't bother if it's dead
    if ( !get_valid( this ) ) return rv_error;

    bool_t was_killed = get_killed( this );

    // set the killed value
    if ( val )
    {
        add_bits( this, killed_bit );
    }
    else
    {
        remove_bits( this, killed_bit );
    }

    // clear all requests
    kill_me  = bfalse;

    return was_killed ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_set_on( bool_t val )
{
    if ( !get_valid( this ) || get_killed( this ) ) return rv_error;

    bool_t was_on = get_on( this );

    // turn it off
    if ( val )
    {
        add_bits( this, on_bit );
    }
    else
    {
        remove_bits( this, on_bit );
    }

    // clear all requests
    turn_me_on  = bfalse;
    turn_me_off = bfalse;

    return was_on ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_do_on()
{
    if ( !get_valid( this ) || get_killed( this ) ) return rv_error;

    // anything to do?
    if ( !turn_me_on && !turn_me_off ) return rv_error;

    // assume no change
    bool_t on_val = get_on( this );

    // poll the requests
    // let turn_me_off override turn_me_on
    if ( turn_me_off )
    {
        on_val = bfalse;
    }
    else if ( turn_me_on )
    {
        on_val = btrue;
    }

    // actually set the value
    return proc_set_on( on_val );
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_object_process::proc_set_spawning( bool_t val )
{
    bool_t old_val;

    if ( !get_valid( this ) || get_killed( this ) ) return rv_error;

    old_val  = get_spawning( this );

    if ( val )
    {
        add_bits( this, spawning_bit );
    }
    else
    {
        remove_bits( this, spawning_bit );
    }

    return ( old_val == val ) ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::allocate_all( ego_obj * pobj )
{
    // re-initialize the ego_obj_lst_state
    pobj = dtor_all( pobj );

    // validate the object
    return ego_obj::validate( pobj );
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::deallocate_all( ego_obj * pobj )
{
    // do any deconstruction
    pobj = ego_obj::ctor_all( pobj );

    // invalidate the object
    return ego_obj::validate( pobj, bfalse );
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::validate( ego_obj * pbase, bool_t val )
{
    if ( NULL == pbase ) return pbase;

    if ( rv_success == ego_object_process::validate( pbase, val ) )
    {
        // if this succeeds, assign a new guid
        pbase->guid = ego_obj::guid_counter++;
    }

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::begin_processing( ego_obj * pbase, const char * name )
{
    if ( NULL == pbase ) return pbase;

    strncpy( pbase->base_name, "*INVLAID*", SDL_arraysize( pbase->base_name ) );

    // tell the process to begin processing
    if ( rv_success != pbase->proc_set_process() )
    {
        return pbase;
    }

    // create a name for the object
    if ( NULL == name ) name = "*UNKNOWN*";
    strncpy( pbase->base_name, name, SDL_arraysize( pbase->base_name ) );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::req_terminate( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    pbase->proc_req_terminate();

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::grant_terminate( ego_obj * pobj )
{
    /// Completely turn off an ego_obj object and mark it as no longer allocated

    if ( NULL == pobj ) return pobj;

    // if this object os not allocated, get out of here
    if ( !pobj->object_allocated() ) return pobj;

    // no reason to be in here unless someone is aksking to die
    if ( !pobj->kill_me ) return pobj;

    // poke it to make sure that it is at least a little bit alive
    if ( !get_valid( pobj ) || get_killed( pobj ) ) return pobj;

    // figure out the next step in killing the object
    if ( get_initialized( pobj ) )
    {
        // ready to be killed. jump to deinitializing
        pobj->end_processing();
    }
    else if ( get_constructed( pobj ) )
    {
        // already deinitialized. jump to deconstructing
        pobj->end_deinitializing();
    }
    else if ( get_valid( pobj ) )
    {
        // already deconstructed. jump past deconstructing.
        pobj->end_destructing();

        /// \note BB@> The ego_obj::end_invalidating() function is called in the
        ///            t_obj_lst_map<>::free_one() function when the object is finally
        ///            deallocated. Do not call it here.
    }

    // turn it off, no matter what state it is in
    pobj->proc_set_on( bfalse );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::grant_on( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    pbase->proc_do_on();

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::set_spawning( ego_obj * pbase, bool_t val )
{
    if ( NULL == pbase ) return pbase;

    pbase->proc_set_spawning( val );

    return pbase;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_object_process * ego_object_process_engine::run( ego_object_process * pobj )
{
    int rv = -1;

    if ( !ego_object_process::get_valid( pobj ) || ego_object_process::get_killed( pobj ) ) return NULL;

    switch ( pobj->action )
    {
        default:
        case ego_obj_nothing:
            // do nothing and DO return an "error"
            rv = -1;
            break;

        case ego_obj_constructing:
            rv = pobj->do_constructing();
            break;

        case ego_obj_initializing:
            rv = pobj->do_initializing();
            break;

        case ego_obj_processing:
            rv = pobj->do_processing();
            break;

        case ego_obj_deinitializing:
            rv = pobj->do_deinitializing();
            break;

        case ego_obj_destructing:
            rv = pobj->do_destructing();
            break;

        case ego_obj_waiting:
            // do nothing but DON'T return an "error"
            rv = 1;
            break;
    }

    return pobj;
}

//--------------------------------------------------------------------------------------------
// struct ego_object_engine - main engine
//--------------------------------------------------------------------------------------------
ego_obj * ego_object_engine::run( ego_obj * pobj )
{
    if ( NULL == pobj ) return pobj;

    if ( pobj->kill_me )
    {
        ego_obj::grant_terminate( pobj );
    }

    // make sure that the object is out of "spawn mode" if it
    // is fully initialized
    if ( ego_obj::get_action( pobj ) >= ego_obj_processing )
    {
        POBJ_END_SPAWN( pobj );
    }

    // run the object engine
    ego_object_process_engine::run( pobj );

    // update the object's update_guid
    pobj->object_update_list_id();

    return pobj;
}
