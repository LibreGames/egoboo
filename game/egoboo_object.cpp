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
Uint32 ego_object::guid_counter   = 0;
Uint32 ego_object::spawn_depth    = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_object * ego_object::ctor( ego_object * pbase, size_t index )
{
    list_object_state * lst_obj_ptr = NULL;
    ego_object_state  * ego_obj_ptr = NULL;
    ego_object_req    * ego_req_ptr = NULL;

    if ( NULL == pbase ) return pbase;
    lst_obj_ptr = &( pbase->lst_state );
    ego_obj_ptr = &( pbase->state );
    ego_req_ptr = &( pbase->req );

    memset( pbase, 0, sizeof( *pbase ) );

    // construct the
    list_object_state_ctor( lst_obj_ptr, index );

    ego_object_state::ctor( ego_obj_ptr );

    ego_object_req::ctor( ego_req_ptr );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::dtor( ego_object * pbase )
{
    list_object_state * lst_obj_ptr = NULL;
    ego_object_state  * ego_obj_ptr = NULL;
    ego_object_req    * ego_req_ptr = NULL;

    if ( NULL == pbase ) return pbase;
    lst_obj_ptr = &( pbase->lst_state );
    ego_obj_ptr = &( pbase->state );
    ego_req_ptr = &( pbase->req );

    list_object_state_dtor( lst_obj_ptr );

    ego_object_state::dtor( ego_obj_ptr );

    ego_object_req::dtor( ego_req_ptr );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::allocate( ego_object * pbase, size_t index )
{
    list_object_state tmp_lst_state;

    if ( NULL == pbase ) return pbase;

    // we don't want to overwrite the list_object_state values since some of them belong to the
    // ChrList/PrtList/EncList rather than to the list_object_state
    tmp_lst_state = pbase->lst_state;

    // construct the a new state (this destroys the data in pbase->lst_state)
    pbase = ego_object::ctor( pbase, index );
    if ( NULL == pbase ) return pbase;

    // restore the old pbase->lst_state
    pbase->lst_state = tmp_lst_state;

    // set it to allocated
    list_object_set_allocated( &( pbase->lst_state ), btrue );

    // validate the object
    return ego_object::validate( pbase );
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::deallocate( ego_object * pbase )
{
    if ( NULL == pbase ) return pbase;

    // set it to not allocated
    list_object_set_allocated( &( pbase->lst_state ), bfalse );

    // invalidate the object
    return ego_object::invalidate( pbase );
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::invalidate( ego_object * pbase )
{
    ego_object_state * loc_pstate = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pstate = &( pbase->state );

    ego_object_state::invalidate( loc_pstate );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::end_constructing( ego_object * pbase )
{
    ego_object_state * loc_pstate = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pstate = &( pbase->state );

    ego_object_state::end_constructing( loc_pstate );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::end_initializing( ego_object * pbase )
{
    ego_object_state * loc_pstate = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pstate = &( pbase->state );

    ego_object_state::end_initialization( loc_pstate );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::end_processing( ego_object * pbase )
{
    ego_object_state * loc_pstate = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pstate = &( pbase->state );

    ego_object_state::end_processing( loc_pstate );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::end_deinitializing( ego_object * pbase )
{
    ego_object_state * loc_pstate = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pstate = &( pbase->state );

    ego_object_state::end_deinitializing( loc_pstate );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::end_destructing( ego_object * pbase )
{
    ego_object_state * loc_pstate = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pstate = &( pbase->state );

    ego_object_state::end_destructing( loc_pstate );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::end_killing( ego_object * pbase )
{
    ego_object_state * loc_pstate = NULL;

    if ( NULL == pbase ) return pbase;
    loc_pstate = &( pbase->state );

    ego_object_state::end_killing( loc_pstate );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::validate( ego_object * pbase )
{
    if ( NULL == pbase ) return pbase;

    // clear out the state
    ego_object_state::clear( &( pbase->state ) );

    // clear out any requests
    ego_object_req::clear( &( pbase->req ) );

    // set the "valid" variable and initialize the action
    ego_object_state::set_valid( &( pbase->state ), pbase->lst_state.allocated );
    if ( !pbase->state.valid ) return pbase;

    // if this succeeds, assign a new guid
    pbase->guid = ego_object::guid_counter++;

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::begin_waiting( ego_object * pbase )
{
    if ( NULL == pbase ) return pbase;

    ego_object_state::begin_waiting( &( pbase->state ) );

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::begin_processing( ego_object * pbase, const char * name )
{
    if ( NULL == pbase ) return pbase;

    if ( !pbase->state.valid || pbase->state.killed ) return pbase;

    if ( NULL == name ) name = "UNKNOWN";

    // don't bother if there is already a request to kill it
    if ( pbase->req.kill_me ) return pbase;

    // check the requirements for activating the object
    if ( !pbase->state.constructed || !pbase->state.initialized )
        return pbase;

    strncpy( pbase->base_name, name, SDL_arraysize( pbase->base_name ) );
    pbase->state.action  = ego_object_processing;

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::req_terminate( ego_object * pbase )
{
    if ( NULL == pbase ) return pbase;

    // don't bother if it's already dead
    if ( !pbase->state.valid || pbase->state.killed ) return pbase;

    // request the kill
    pbase->req.kill_me = btrue;

    // turn it off
    pbase->state.on = bfalse;

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::grant_terminate( ego_object * pbase )
{
    /// Completely turn off an ego_object object and mark it as no longer allocated

    if ( NULL == pbase ) return pbase;

    if ( !pbase->state.valid || pbase->state.killed ) return pbase;

    // turn it off
    pbase->state.on = bfalse;

    // if it was constructed, you want to destruct it
    if ( pbase->state.initialized )
    {
        // jump to deinitializing
        ego_object::end_processing( pbase );
    }
    else if ( pbase->state.constructed )
    {
        // jump to deconstructing
        ego_object::end_deinitializing( pbase );
    }
    else
    {
        ego_object::end_killing( pbase );
    }

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::grant_on( ego_object * pbase )
{
    if ( NULL == pbase ) return pbase;

    if ( !pbase->state.valid || pbase->state.killed ) return pbase;

    // anything to do?
    if ( !pbase->req.turn_me_on && !pbase->req.turn_me_off ) return pbase;

    // let turn_me_off override turn_me_on
    if ( pbase->req.turn_me_off )
    {
        pbase->state.on = bfalse;
    }
    else if ( pbase->req.turn_me_on )
    {
        pbase->state.on = btrue;
    }

    // reset the requests
    pbase->req.turn_me_on  = bfalse;
    pbase->req.turn_me_off = bfalse;

    return pbase;
}

//--------------------------------------------------------------------------------------------
ego_object * ego_object::set_spawning( ego_object * pbase, bool_t val )
{
    if ( NULL == pbase ) return pbase;

    if ( !pbase->state.valid || pbase->state.killed ) return pbase;

    pbase->state.spawning = val;

    return pbase;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::ctor( ego_object_state * ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->action = ego_object_nothing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::dtor( ego_object_state * ptr )
{
    if ( NULL == ptr ) return ptr;

    ego_object_state::clear( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::clear( ego_object_state * ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    memset( ptr, 0, sizeof( *ptr ) );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::end_constructing( ego_object_state * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->valid       = btrue;
    ptr->constructed = btrue;

    ptr->action = ego_object_initializing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::end_initialization( ego_object_state * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = btrue;
    ptr->initialized = btrue;

    ptr->action = ego_object_processing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::end_processing( ego_object_state * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = btrue;
    ptr->initialized = btrue;
    ptr->active      = bfalse;

    ptr->action = ego_object_deinitializing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::end_deinitializing( ego_object_state * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = btrue;
    ptr->initialized = bfalse;
    ptr->active      = bfalse;

    ptr->action = ego_object_destructing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::end_destructing( ego_object_state * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    ptr->constructed = bfalse;
    ptr->initialized = bfalse;
    ptr->active      = bfalse;

    ptr->action = ego_object_destructing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::end_killing( ego_object_state * ptr )
{
    if ( NULL == ptr || !ptr->valid ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->valid  = btrue;
    ptr->killed = btrue;

    ptr->action = ego_object_nothing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::invalidate( ego_object_state * ptr )
{
    if ( NULL == ptr ) return ptr;

    ego_object_state::dtor( ptr );

    return ptr;
}
//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::set_valid( ego_object_state * ptr, bool_t val )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->valid  = val;
    ptr->action = val ? ego_object_constructing : ego_object_nothing;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_object_state * ego_object_state::begin_waiting( ego_object_state * ptr )
{
    // put the object in the "waiting to be killed" mode. currently used only by particles

    if ( NULL == ptr || !ptr->valid || ptr->killed ) return ptr;

    ptr->action    = ego_object_waiting;

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_object_req * ego_object_req::ctor( ego_object_req * ptr )
{
    return ego_object_req::clear( ptr );
}

//--------------------------------------------------------------------------------------------
ego_object_req * ego_object_req::dtor( ego_object_req * ptr )
{
    return ego_object_req::clear( ptr );
}

//--------------------------------------------------------------------------------------------
ego_object_req * ego_object_req::clear( ego_object_req* ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    return ptr;
}