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

/// \file egoboo_process.h
/// \details A rudimentary implementation of "non-preemptive multitasking" in Egoboo.

#include "egoboo_typedef_cpp.h"
#include "egoboo_state_machine.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_process;

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

/// an abstract interface for the "egoboo process"
struct i_ego_process
{
    virtual i_ego_process * get_interface()   = 0;
    virtual ego_process   * get_process()     = 0;

    virtual egoboo_rv  do_beginning() = 0;
    virtual egoboo_rv  do_entering()  = 0;
    virtual egoboo_rv  do_running()   = 0;
    virtual egoboo_rv  do_leaving()   = 0;
    virtual egoboo_rv  do_finishing() = 0;
};

//--------------------------------------------------------------------------------------------

/// a container for the data (without virtual functions) to make intializing easier
struct ego_process_data
{
    friend struct ego_process;

    // the "state" of the process
    bool_t valid;
    bool_t paused;
    bool_t killme;
    bool_t terminated;
    int    state;

    // some other generic information
    double          dtime;

    // ther "return value" of the do_*() function call
    int result;

    //---- construction/destruction
    ego_process_data() { clear( this ); }
    ~ego_process_data() { clear( this ); };

    ego_process_data * get_data_ptr() { return this; }

private:

    static ego_process_data * clear( ego_process_data * );
};

//--------------------------------------------------------------------------------------------
/// a concrete base class for "egoboo process" objects
/// All other process types "inherit" this
struct ego_process : public ego_process_data, public i_ego_process
{
    friend struct ego_process_engine;

    //---- construction/destruction
    ego_process() { ctor_this( this ); }
    virtual ~ego_process() {};

    //---- externally accessible means of interacting with the process
    egoboo_rv start();
    egoboo_rv kill();
    egoboo_rv validate();
    egoboo_rv terminate();
    egoboo_rv pause();
    egoboo_rv resume();
    egoboo_rv running();

    //---- basic implementation of the interface accessors
    virtual i_ego_process * get_interface() { return static_cast<i_ego_process *>( this ); }
    virtual ego_process   * get_process()   { return static_cast< ego_process  *>( this ); }

protected:

    //---- construction/destruction
    static ego_process * ctor_this( ego_process * );

    //---- implementation of the basic process behavior
    virtual egoboo_rv do_beginning() { result = -1; state = proc_entering;  return rv_success; }
    virtual egoboo_rv do_entering()  { result = -1; state = proc_running;   return rv_success; }
    virtual egoboo_rv do_running()   { result = -1; state = proc_leaving;   return rv_success; }
    virtual egoboo_rv do_leaving()   { result = -1; state = proc_finishing; return rv_success; }
    virtual egoboo_rv do_finishing() { result = -1; state = proc_invalid;   return rv_success; }

private:

    static ego_process * clear( ego_process * );
};

//--------------------------------------------------------------------------------------------
/// the basic engine for running a process.
struct ego_process_engine
{
    /// the entry point for running a process
    static int run( ego_process *, double dt = 1.0f );

protected:

    /// A return value of rv_error indicates that the engine did not know what to do.
    static egoboo_rv do_run( ego_process * proc );
};

extern ego_process_engine ProcessEngine;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
