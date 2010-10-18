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

/// @file egoboo_process.c
/// @brief Implementation of Egoboo "process" control routines
/// @details

#include "egoboo_process.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_process * ego_process::clear( ego_process * ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_process * ego_process::ctor( ego_process * proc )
{
    if ( NULL == proc ) return proc;

    proc = ego_process::clear( proc );
    if ( NULL == proc ) return proc;

    proc->terminated = btrue;

    return proc;
}

//--------------------------------------------------------------------------------------------
bool_t ego_process::start( ego_process * proc )
{
    if ( NULL == proc ) return bfalse;

    // choose the correct proc->state
    if ( proc->terminated || proc->state > proc_leaving )
    {
        // must re-initialize the process
        proc->state = proc_beginning;
    }
    if ( proc->state > proc_entering )
    {
        // the process is already initialized, just put it back in
        // proc_entering mode
        proc->state = proc_entering;
    }

    // tell it to run
    proc->terminated = bfalse;
    proc->valid      = btrue;
    proc->paused     = bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_process::kill( ego_process * proc )
{
    if ( NULL == proc ) return bfalse;
    if ( !ego_process::validate( proc ) ) return btrue;

    // turn the process back on with an order to commit suicide
    proc->paused = bfalse;
    proc->killme = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_process::validate( ego_process * proc )
{
    if ( NULL == proc ) return bfalse;

    if ( !proc->valid || proc->terminated )
    {
        ego_process::terminate( proc );
    }

    return proc->valid;
}

//--------------------------------------------------------------------------------------------
bool_t ego_process::terminate( ego_process * proc )
{
    if ( NULL == proc ) return bfalse;

    proc->valid      = bfalse;
    proc->terminated = btrue;
    proc->state      = proc_beginning;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_process::pause( ego_process * proc )
{
    bool_t old_value;

    if ( !ego_process::validate( proc ) ) return bfalse;

    old_value    = proc->paused;
    proc->paused = btrue;

    return old_value != proc->paused;
}

//--------------------------------------------------------------------------------------------
bool_t ego_process::resume( ego_process * proc )
{
    bool_t old_value;

    if ( !ego_process::validate( proc ) ) return bfalse;

    old_value    = proc->paused;
    proc->paused = bfalse;

    return old_value != proc->paused;
}

//--------------------------------------------------------------------------------------------
bool_t ego_process::running( ego_process * proc )
{
    if ( !ego_process::validate( proc ) ) return bfalse;

    return !proc->paused;
}

