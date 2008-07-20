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

#include <SDL_types.h>
#include "egoboo_types.h"

struct ClockState_t;

typedef double( *clock_source_ptr )(void);
typedef struct ClockState_t ClockState;

void clock_init();      // Init the clock module
void clock_shutdown();  // Shut down the clock module
void clock_setTimeSource( clock_source_ptr tsrc );     // Specify where the clock gets its time values from

ClockState * ClockState_create( const char * name, int size );
bool_t       ClockState_destroy( ClockState ** cs );
ClockState * ClockState_renew( ClockState * cs );

void   ClockState_frameStep( ClockState * cs );          // Update the clock.
double ClockState_getTime( ClockState * cs );            // Returns the current time.  The clock's time only
                                                         // updates when ClockState_frameStep() is called
double ClockState_getFrameDuration( ClockState * cs );   // Return the length of the current frame. (Sort of.)
Uint32 ClockState_getFrameNumber( ClockState * cs );     // Return which frame we're on
float  ClockState_getFrameRate( ClockState * cs );       // Return the current instantaneous FPS
