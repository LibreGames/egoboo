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

/// @file egoboo_process.h

#include "egoboo_typedef.h"
#include "egoboo_state_machine.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// grab a pointer to the ego_process of any object that "inherits" this type
#define PROC_PBASE(PTR) static_cast<ego_process *>(PTR)

//--------------------------------------------------------------------------------------------
// The various states that a process can occupy
enum e_process_states
{
    proc_invalid     = ego_action_invalid,
    proc_beginning   = ego_action_beginning,
    proc_entering    = ego_action_entering,
    proc_running     = ego_action_running,
    proc_leaving     = ego_action_leaving,
    proc_finishing   = ego_action_finishing
};
typedef enum e_process_states process_state_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A rudimentary implementation of "non-preemptive multitasking" in Egoboo.
/// @details All other process types "inherit" from this one

struct ego_process
{
    bool_t          valid;
    bool_t          paused;
    bool_t          killme;
    bool_t          terminated;
    process_state_t state;
    double          dtime;

    ego_process() { ego_process::ctor( this ); }

    static ego_process * ctor( ego_process * proc );
    static ego_process * clear( ego_process * ptr );

    static bool_t        start( ego_process * proc );
    static bool_t        kill( ego_process * proc );
    static bool_t        validate( ego_process * proc );
    static bool_t        terminate( ego_process * proc );
    static bool_t        pause( ego_process * proc );
    static bool_t        resume( ego_process * proc );
    static bool_t        running( ego_process * proc );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
