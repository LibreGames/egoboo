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

/// @file egoboo_object.c
/// @brief Implementation of Egoboo "object" control routines
/// @details

#include "egoboo_object.h"

#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 ego_obj::guid_counter   = 0;
Uint32 ego_obj::spawn_depth    = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::ctor_this( ego_obj_proc * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr = ego_obj_proc::clear( ptr );
    if ( NULL == ptr ) return ptr;

    ptr->action = ego_obj_nothing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::dtor_this( ego_obj_proc * ptr )
{
    if ( NULL == ptr ) return ptr;

    ego_obj_proc::clear( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::clear( ego_obj_proc * ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::end_constructing( ego_obj_proc * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->valid       = btrue;
    ptr->constructed = btrue;

    ptr->action = ego_obj_initializing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::end_initialization( ego_obj_proc * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = btrue;
    ptr->initialized = btrue;

    ptr->action = ego_obj_processing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::end_processing( ego_obj_proc * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = btrue;
    ptr->initialized = btrue;
    ptr->active      = bfalse;

    ptr->action = ego_obj_deinitializing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::end_deinitializing( ego_obj_proc * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = btrue;
    ptr->initialized = bfalse;
    ptr->active      = bfalse;

    ptr->action = ego_obj_destructing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::end_destructing( ego_obj_proc * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = bfalse;
    ptr->initialized = bfalse;
    ptr->active      = bfalse;

    ptr->action = ego_obj_destructing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::end_killing( ego_obj_proc * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr = ego_obj_proc::clear( ptr );
    if ( NULL == ptr ) return ptr;

    ptr->valid  = btrue;
    ptr->killed = btrue;

    ptr->action = ego_obj_nothing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::invalidate( ego_obj_proc * ptr )
{
    if ( NULL == ptr ) return ptr;

    ego_obj_proc::dtor_this( ptr );

    return ptr;
}
//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::set_valid( ego_obj_proc * ptr, bool_t val )
{
    if ( NULL == ptr ) return ptr;

    ptr = ego_obj_proc::clear( ptr );
    if ( NULL == ptr ) return ptr;

    ptr->valid  = val;
    ptr->action = val ? ego_obj_constructing : ego_obj_nothing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_proc * ego_obj_proc::begin_waiting( ego_obj_proc * ptr )
{
    // put the object in the "waiting to be killed" mode. currently used only by particles

    if ( NULL == ptr || !ptr->valid || ptr->killed ) return ptr;

    ptr->action    = ego_obj_waiting;

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_obj_req * ego_obj_req::ctor_this( ego_obj_req * ptr )
{
    return ego_obj_req::clear( ptr );
}

//--------------------------------------------------------------------------------------------
ego_obj_req * ego_obj_req::dtor_this( ego_obj_req * ptr )
{
    return ego_obj_req::clear( ptr );
}

//--------------------------------------------------------------------------------------------
ego_obj_req * ego_obj_req::clear( ego_obj_req* ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_validate( bool_t val )
{
    if ( NULL == this ) return rv_error;

    // clear out the state
    ego_obj_proc::clear( &_proc_data );

    // clear out any requests
    ego_obj_req::clear( &_req_data );

    // set the "valid" variable and initialize the action
    ego_obj_proc::set_valid( &_proc_data, val );

    return _proc_data.valid ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_req_kill()
{
    if ( NULL == this ) return rv_error;

    if ( !_proc_data.valid ) return rv_error;

    if ( _proc_data.killed ) return rv_success;

    // save the old value
    bool_t was_kill_me = _req_data.kill_me;

    // do the request
    _req_data.kill_me = btrue;

    // turn the object off
    proc_set_on( bfalse );

    return was_kill_me ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_req_on( bool_t val )
{
    if ( NULL == this ) return rv_error;

    if ( !_proc_data.valid || _proc_data.killed ) return rv_error;

    bool_t needed = ( val != _req_data.turn_me_on );

    _req_data.turn_me_on  = val;
    _req_data.turn_me_off = !val;

    return needed ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_req_pause( bool_t val )
{
    if ( NULL == this ) return rv_error;

    if ( !_proc_data.valid || _proc_data.killed ) return rv_error;

    bool_t needed = ( val != _req_data.pause_me );

    _req_data.pause_me   = val;
    _req_data.unpause_me = !val;

    return needed ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_set_wait()
{
    ego_obj_proc * rv;

    if ( NULL == this ) return rv_error;

    rv = ego_obj_proc::begin_waiting( &_proc_data );

    return ( NULL != rv ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_set_process()
{
    if ( NULL == this ) return rv_error;

    if ( !_proc_data.valid || _proc_data.killed ) return rv_error;

    // don't bother if there is already a request to kill it
    if ( _req_data.kill_me ) return rv_fail;

    // check the requirements for activating the object
    if ( !_proc_data.constructed || !_proc_data.initialized )
        return rv_fail;

    _proc_data.action  = ego_obj_processing;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_set_killed( bool_t val )
{
    if ( NULL == this ) return rv_error;

    // don't bother if it's dead
    if ( !_proc_data.valid ) return rv_error;

    bool_t was_killed = _proc_data.killed;

    // set the killed value
    _proc_data.killed = val;

    // clear all requests
    _req_data.kill_me  = bfalse;

    return was_killed ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_set_on( bool_t val )
{
    if ( NULL == this ) return rv_error;

    // don't bother if it's dead
    if ( !_proc_data.valid || _proc_data.killed ) return rv_error;

    bool_t was_on = _proc_data.on;

    // turn it off
    _proc_data.on = val;

    // clear all requests
    _req_data.turn_me_on  = bfalse;
    _req_data.turn_me_off = bfalse;

    return was_on ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_do_on()
{
    if ( NULL == this ) return rv_error;

    if ( !_proc_data.valid || _proc_data.killed ) return rv_error;

    // anything to do?
    if ( !_req_data.turn_me_on && !_req_data.turn_me_off ) return rv_error;

    // assume no change
    bool_t on_val = _proc_data.on;

    // poll the requests
    // let turn_me_off override turn_me_on
    if ( _req_data.turn_me_off )
    {
        on_val = bfalse;
    }
    else if ( _req_data.turn_me_on )
    {
        on_val = btrue;
    }

    // actually set the value
    return proc_set_on( on_val );
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_obj_proc_data::proc_set_spawning( bool_t val )
{
    bool_t old_val;

    if ( NULL == this ) return rv_error;

    if ( !_proc_data.valid || _proc_data.killed ) return rv_error;

    old_val             = _proc_data.spawning;
    _proc_data.spawning = val;

    return ( old_val == val ) ? rv_fail : rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::allocate( ego_obj * pbase, size_t index )
{
    cpp_list_state tmp_lst_state;

    if ( NULL == pbase ) return pbase;

    // we don't want to overwrite the cpp_list_state values since some of them belong to the
    // ChrObjList/PrtObjList/EncObjList rather than to the cpp_list_state
    tmp_lst_state = pbase->get_list();

    // construct the a new state (this destroys the data in pbase->lst_state)
    pbase = ego_obj::ctor_all( pbase, index );
    if ( NULL == pbase ) return pbase;

    // restore the old pbase->lst_state
    pbase->get_list() = tmp_lst_state;

    // set it to allocated
    cpp_list_state::set_allocated( pbase->get_plist(), btrue );

    // validate the object
    return ego_obj::validate( pbase );
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::deallocate( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    // set it to not allocated
    cpp_list_state::set_allocated( pbase->get_plist(), bfalse );

    // invalidate the object
    return ego_obj::invalidate( pbase );
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::invalidate( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    ego_obj_proc::invalidate( pbase->get_pproc() );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::end_constructing( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    ego_obj_proc::end_constructing( pbase->get_pproc() );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::end_initializing( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    ego_obj_proc::end_initialization( pbase->get_pproc() );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::end_processing( ego_obj * pbase )
{
    ego_obj_proc * loc_pproc = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pproc = pbase->get_pproc();

    ego_obj_proc::end_processing( loc_pproc );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::end_deinitializing( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    ego_obj_proc::end_deinitializing( pbase->get_pproc() );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::end_destructing( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    ego_obj_proc::end_destructing( pbase->get_pproc() );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::end_killing( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    ego_obj_proc::end_killing( pbase->get_pproc() );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::validate( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    if ( rv_success == pbase->proc_validate( pbase->get_allocated() ) )
    {
        // if this succeeds, assign a new guid
        pbase->guid = ego_obj::guid_counter++;
    }

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::begin_waiting( ego_obj * pbase )
{
    if ( NULL == pbase ) return pbase;

    // tell the process to wait
    pbase->proc_set_wait();

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

    pbase->proc_req_kill();

    return pbase;
}
//--------------------------------------------------------------------------------------------
ego_obj * ego_obj::grant_terminate( ego_obj * pbase )
{
    /// Completely turn off an ego_obj object and mark it as no longer allocated

    if ( NULL == pbase ) return pbase;

    ego_obj_proc * obj_prc_ptr = pbase->get_pproc();

    if ( !obj_prc_ptr->valid || obj_prc_ptr->killed ) return pbase;

    // figure out the next step in killing the object
    if ( obj_prc_ptr->initialized )
    {
        // jump to deinitializing
        ego_obj::end_processing( pbase );
    }
    else if ( obj_prc_ptr->constructed )
    {
        // jump to deconstructing
        ego_obj::end_deinitializing( pbase );
    }
    else if ( obj_prc_ptr->killed )
    {
        ego_obj::end_killing( pbase );
    }

    // turn it off
    pbase->proc_set_on( bfalse );

    return pbase;
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
