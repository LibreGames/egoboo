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

///
/// \file clock.h
/// \brief Clock & timer functionality.
/// \details This implementation was adapted from Noel Lopis' article in
///   Game Programming Gems 4.

#include <SDL_types.h>
#include <time.h>

#include "egoboo_typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_ClockState;

    typedef double( *clock_source_ptr_t )( void );
    typedef struct s_ClockState ClockState_t;
    typedef struct tm* EGO_TIME;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    void clk_init( void );                                 ///< Init the clock module
    void clk_shutdown( void );                             ///< Shut down the clock module
//void clk_setTimeSource( clock_source_ptr_t tsrc );   ///< Specify where the clock gets its time values from

    ClockState_t * clk_create( const char * name, const int size );
    bool_t         clk_destroy( ClockState_t ** cs );
    ClockState_t * clk_renew( ClockState_t * cs );

    void   clk_frameStep( ClockState_t * cs );          ///< Update the clock.
    double clk_getTime( ClockState_t * cs );            ///< Returns the current time.  The clock's time only updates when clk_frameStep() is called
    double clk_getFrameDuration( ClockState_t * cs );   ///< Return the length of the current frame. (Sort of.)
    Uint32 clk_getFrameNumber( ClockState_t * cs );     ///< Return which frame we're on
    float  clk_getFrameRate( ClockState_t * cs );       ///< Return the current instantaneous FPS
    EGO_TIME getCurrentTime();                          ///< Returns a structure containing current time and date

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#define _clock_h
