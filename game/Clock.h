/* Egoboo - Clock.c
 * Clock & timer functionality
 * This implementation was adapted from Noel Lopis' article in
 * Game Programming Gems 4.
 */

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

#pragma once

#include <SDL/SDL_types.h>
#include "egoboo_types.h"

struct sClockState;

typedef double( *clock_source_ptr_t )(void);
typedef struct sClockState ClockState_t;

void clock_init();      // Init the clock module
void clock_shutdown();  // Shut down the clock module
void clock_setTimeSource( clock_source_ptr_t tsrc );     // Specify where the clock gets its time values from

ClockState_t * ClockState_create( const char * name, int size );
bool_t       ClockState_destroy( ClockState_t ** cs );
ClockState_t * ClockState_renew( ClockState_t * cs );

void   ClockState_frameStep( ClockState_t * cs );          // Update the clock.
double ClockState_getTime( ClockState_t * cs );            // Returns the current time.  The clock's time only
                                                         // updates when ClockState_frameStep() is called
double ClockState_getFrameDuration( ClockState_t * cs );   // Return the length of the current frame. (Sort of.)
Uint32 ClockState_getFrameNumber( ClockState_t * cs );     // Return which frame we're on
float  ClockState_getFrameRate( ClockState_t * cs );       // Return the current instantaneous FPS

//-----------------------------------------------------------------
// macros to use the high resolution timer for profiling

#ifdef DEBUG_PROFILE

#    define PROFILE_DECLARE(XX) ClockState_t * clkstate_##XX = NULL; double clkcount_##XX = 0.0; double clktime_##XX = 0.0;
#    define PROFILE_INIT(XX)    { clkstate_##XX  = ClockState_create(#XX, -1); }
#    define PROFILE_FREE(XX)    { ClockState_destroy(&(clkstate_##XX)); }
#    define PROFILE_QUERY(XX)   ( (double)clktime_##XX / (double)clkcount_##XX )

#    define PROFILE_BEGIN(XX)  ClockState_frameStep(clkstate_##XX);
#    define PROFILE_END(XX)    ClockState_frameStep(clkstate_##XX);   clkcount_##XX = clkcount_##XX*0.9 + 0.1*1.0; clktime_##XX = clktime_##XX*0.9 + 0.1*ClockState_getFrameDuration(clkstate_##XX);
#    define PROFILE_END2(XX)   ClockState_frameStep(clkstate_##XX);   clkcount_##XX += 1.0;  clktime_##XX += ClockState_getFrameDuration(clkstate_##XX);

#else

#    define PROFILE_DECLARE(XX) ClockState_t * clkstate_##XX = NULL; double clkcount_##XX = 0.0; double clktime_##XX = 0.0;
#    define PROFILE_INIT(XX)    { clkstate_##XX  = ClockState_create(#XX, -1); }
#    define PROFILE_FREE(XX)    { ClockState_destroy(&(clkstate_##XX)); }
#    define PROFILE_QUERY(XX)   1.0

#    define PROFILE_BEGIN(XX)
#    define PROFILE_END(XX)    clkcount_##XX  = 1.0;
#    define PROFILE_END2(XX)   clkcount_##XX += 1.0;

#endif
