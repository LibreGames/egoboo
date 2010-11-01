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
ego_process_engine ProcessEngine;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_process_data * ego_process_data::clear( ego_process_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    SDL_memset( pdata, 0, sizeof( *pdata ) );

    pdata->terminated = btrue;
    pdata->result     = -1;

    return pdata;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_process * ego_process::ctor_this( ego_process * proc )
{
    proc = clear( proc );
    if ( NULL == proc ) return proc;

    /* add something here */

    return proc;
}

//--------------------------------------------------------------------------------------------
ego_process * ego_process::clear( ego_process * proc )
{
    if ( NULL == proc ) return proc;

    ego_process_data * pdata = ego_process_data::clear( proc->get_data_ptr() );
    if ( NULL == pdata ) return proc;

    /* add something here */

    return proc;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process::start()
{
    if ( NULL == this ) return rv_error;

    // choose the correct state
    if ( terminated || state > proc_leaving )
    {
        // must re-initialize the process
        state = proc_beginning;
    }
    if ( state > proc_entering )
    {
        // the process is already initialized, just put it back in
        // proc_entering mode
        state = proc_entering;
    }

    // tell it to run
    terminated = bfalse;
    valid      = btrue;
    paused     = bfalse;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process::kill()
{
    egoboo_rv val_state = validate();
    if ( rv_error == val_state ) return rv_error;
    else if ( rv_fail == val_state ) return rv_success;

    // turn the process back on with an order to commit suicide
    paused = bfalse;
    killme = btrue;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process::validate()
{
    if ( NULL == this ) return rv_error;

    if ( !valid || terminated )
    {
        terminate();
    }

    return valid ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process::terminate()
{
    if ( NULL == this ) return rv_error;

    valid      = bfalse;
    terminated = btrue;
    state      = proc_beginning;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process::pause()
{
    bool_t old_value;

    egoboo_rv val_state = validate();
    if ( rv_success != val_state ) return val_state;

    old_value = paused;
    paused    = btrue;

    return old_value != paused ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process::resume()
{
    bool_t old_value;

    egoboo_rv val_state = validate();
    if ( rv_success != val_state ) return val_state;

    old_value = paused;
    paused    = bfalse;

    return old_value != paused ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process::running()
{
    egoboo_rv val_state = validate();
    if ( rv_success != val_state ) return val_state;

    return !paused  ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int ego_process_engine::run( ego_process * proc, double dt )
{
    bool_t valid;
    int    result;

    // in an invalid pointer, return an error
    if ( NULL == proc || !proc->validate() ) return rv_error;

    // update the timimg information
    proc->dtime = dt;

    // exit unless the pause is turned off
    if ( proc->paused ) return rv_success;

    // allow a process to go through several states
    valid = btrue;
    result = -1;
    while ( valid && -1 == result )
    {
        egoboo_rv retval = do_run( proc );

        result = proc->result;
        valid  = proc->validate() && ( rv_error != retval );
    };

    return proc->result;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_process_engine::do_run( ego_process * proc )
{
    // assume that the state can't be handled
    egoboo_rv handled = rv_error;

    // in an invalid pointer, return an error
    if ( NULL == proc ) return rv_error;

    // set the state to leaving on an error or on "kill me"
    if ( rv_fail != proc->killme )
    {
        proc->state = proc_leaving;
    }

    // handle various states
    handled = rv_error;
    switch ( proc->state )
    {
        case proc_beginning:
            handled = proc->do_beginning( );
            break;

        case proc_entering:
            handled = proc->do_entering( );
            break;

        case proc_running:
            handled = proc->do_running( );
            break;

        case proc_leaving:
            handled = proc->do_leaving( );
            break;

        case proc_finishing:
            handled = proc->do_finishing( );
            break;

        default:
            handled = rv_error;
    }

    return handled;
}
