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

/// @file egoboo.h
///
/// @details Disgusting, hairy, way too monolithic header file for the whole darn
/// project.  In severe need of cleaning up.  Venture here with extreme
/// caution, and bring one of those canaries with you to make sure you
/// don't run out of oxygen.

#include "egoboo_defs.h"

#include "egoboo_config.h"       /* configure for this platform */
#include "egoboo_math.h"         /* vector and matrix math */
#include "egoboo_setup.h"
#include "egoboo_process.h"

#include "file_common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <SDL.h>
#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// an in-game clock that tracks elapsed time, elapsed frames, and elapsed updates
struct ego_clock
{
    bool_t   initialized;

    ego_uint update_old;
    ego_uint update_new;
    ego_sint update_cnt;
    ego_sint update_dif;

    ego_uint frame_old;
    ego_uint frame_new;
    ego_sint frame_cnt;
    ego_sint frame_dif;

    Uint32 tick_old;
    Uint32 tick_new;
    Uint32 tick_cnt;
    Sint32 tick_dif;

    ego_clock() { clear(); }

    void update_counters()
    {
        update_old  = update_new;
        update_new  = update_wld;
        update_dif  = ego_sint( update_new ) - ego_sint( update_old );
        update_cnt += update_dif;

        frame_old  = frame_new;
        frame_new  = frame_all;
        frame_dif  = ego_sint( frame_new ) - ego_sint( frame_old );
        frame_cnt += frame_dif;
    }

    void update_ticks()
    {
        tick_old  = tick_new;
        tick_new  = egoboo_get_ticks();
        tick_dif  = tick_new - tick_old;
        tick_cnt += tick_dif;
    }

    void update_ticks( Sint32 dif )
    {
        tick_old  = tick_new;
        tick_new += dif;
        tick_dif  = dif;
        tick_cnt += dif;
    }

    /// try to prevent overflow in these registers
    void blank()
    {
        update_cnt = 0;
        frame_cnt  = 0;
        tick_cnt   = 0;
    }

    void init() { clear(); initialized = btrue; }

    void clear()
    {
        update_new = update_wld;
        update_old = update_wld;
        update_cnt = 0;
        update_dif = 0;

        frame_new = frame_all;
        frame_old = frame_all;
        frame_cnt = 0;
        frame_dif = 0;

        tick_new = egoboo_get_ticks();
        tick_old = tick_new;
        tick_cnt = 0;
        tick_dif = 0;
    }

};

struct picked_module_info
{
    bool_t          ready;              ///< Is there a new picked module?
    int             index;              ///< The module index number
    STRING          path;               ///< The picked module's full path name
    STRING          name;               ///< The picked module's short name
    STRING          write_path;         ///< The picked module's path name relative to the userdata directory

    picked_module_info( int idx = -1 ) { init( idx ); }

    void init( int idx = -1 );
};

extern picked_module_info pickedmodule;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a process that controls the master loop of the program

struct ego_main_process_data
{
    double frameDuration;
    int    menuResult;

    bool_t was_active;
    bool_t escape_requested, escape_latch;

    bool_t screenshot_requested, screenshot_keyready;

    int    ticks_next, ticks_now;

    char * argv0;

    ego_main_process_data() { clear( this ); }

    static ego_main_process_data * ctor_this( ego_main_process_data * ptr ) { return clear( ptr ); }

private:

    static ego_main_process_data * clear( ego_main_process_data * ptr )
    {
        if ( NULL == ptr ) return NULL;

        SDL_memset( ptr, 0, sizeof( *ptr ) );

        ptr->screenshot_keyready = btrue;

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_main_process : public ego_main_process_data, public ego_process
{
    ego_main_process() { ctor_this( this ); }

    static ego_main_process * init( ego_main_process * eproc, int argc, char **argv );

    static ego_main_process * ctor_this( ego_main_process * ptr )
    {
        if ( NULL == ptr ) return ptr;

        /* add something here */

        return ptr;
    }

    static ego_main_process * ctor_all( ego_main_process * ptr )
    {
        ego_process::ctor_this( ptr );
        ego_main_process_data::ctor_this( ptr );

        return ptr;
    }

    static int Run( ego_main_process * eproc, double frameDuration );

    virtual egoboo_rv do_beginning();
    virtual egoboo_rv do_running();
    virtual egoboo_rv do_leaving();
    virtual egoboo_rv do_finishing();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_config_data
{
    ego_config_data( config_data_t * src ) : _src( src ) {};

    config_data_t * cfg_ptr() { return _src; }

    static bool_t synch( ego_config_data * pcfg );
    static int    upload( ego_config_data * pcfg );

private:
    config_data_t * _src;
};

extern ego_config_data ego_cfg;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the main process
extern ego_main_process * EProc;

// the clock for tracking the frames-per-second
extern ego_clock fps_clk;

// the clock for tracking updates-per-second
extern ego_clock ups_clk;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define  _egoboo_h

