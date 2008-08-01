//********************************************************************************************
//* Egoboo - Client.c
//*
//* Clock & timer functionality
//* This implementation was adapted from Noel Lopis' article in
//* Game Programming Gems 4.
//*
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

#include "Clock.h"

#include "System.h"
#include "Log.h"
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>

static clock_source_ptr _clock_timeSource = NULL;

static clock_source_ptr clock_getTimeSource()
{
  if(NULL == _clock_timeSource)
  {
    _clock_timeSource = sys_getTime;
  }

  return _clock_timeSource;
};

void clock_init()
{
  log_info( "Initializing clock services...\n" );

  clock_getTimeSource();
}

void clock_shutdown()
{
  _clock_timeSource = NULL;
}

// Clock data
struct ClockState_t
{
  // Clock data
  char * name;

  double sourceStartTime;  // The first value the clock receives from above function
  double sourceLastTime;  // The last value the clock received from above function
  double currentTime;   // The current time, not necessarily in sync w/ the source time
  double frameTime;   // The time this frame takes
  Uint32 frameNumber; // Which frame the clock is on

  double maximumFrameTime; // The maximum time delta the clock accepts (default .2 seconds)

  // Circular buffer to hold frame histories
  double *frameHistory;
  int frameHistorySize;
  int frameHistoryWindow;
  int frameHistoryHead;
};


static ClockState * ClockState_new( ClockState * cs, const char * name, int size  );
static bool_t ClockState_delete( ClockState * cs );

static void   ClockState_initTime( ClockState * cs );
static void   ClockState_setFrameHistoryWindow( ClockState * cs, int size );
static void   ClockState_addToFrameHistory( ClockState * cs, double frame );
static double ClockState_getExactLastFrameDuration( ClockState * cs );
static double ClockState_guessFrameDuration( ClockState * cs );


ClockState * ClockState_create(const char * name, int size)
{
  ClockState * cs;

  cs = ( ClockState * ) calloc( 1, sizeof( ClockState ) );
  
  return ClockState_new( cs, name, size );
};

bool_t ClockState_destroy( ClockState ** pcs )
{
  bool_t retval;

  if(NULL == pcs || NULL == *pcs) return bfalse;

  retval = ClockState_delete( *pcs );
  FREE( *pcs );

  return retval;
};


ClockState * ClockState_new( ClockState * cs, const char * name, int size )
{
  clock_source_ptr psrc;
  if(NULL == cs) return cs;

  if(size<0) size = 1;
  log_info("ClockState_new() - \"%s\"\t%d buffer(s)\n", name, size);

  memset( cs, 0, sizeof( ClockState ) );

  cs->maximumFrameTime = 0.2;
  cs->name = name;
  ClockState_setFrameHistoryWindow( cs, size );

  psrc = clock_getTimeSource();
  cs->sourceStartTime = psrc();
  cs->sourceLastTime  = cs->sourceStartTime;

  return cs;
};

bool_t ClockState_delete( ClockState * cs )
{
  if(NULL == cs) return bfalse;

  FREE ( cs->frameHistory );

  return btrue;
};

ClockState * ClockState_renew( ClockState * cs )
{
  const char * name;
  int size;

  name = cs->name;
  size = cs->frameHistorySize;

  ClockState_delete(cs);
  return ClockState_new(cs, name, size);
};

void ClockState_setFrameHistoryWindow( ClockState * cs, int size )
{
  double *history;
  int oldSize = cs->frameHistoryWindow;
  int less;

  // The frame history has to be at least 1
  cs->frameHistoryWindow = ( size > 1 ) ? size : 1;
  history = (double *)calloc( cs->frameHistoryWindow, sizeof( double ) );

  if (NULL == cs->frameHistory)
  {
    memset( history, 0, sizeof( double ) * cs->frameHistoryWindow );
  }
  else
  {
    // Copy over the older history.  Make sure that only the size of the
    // smaller buffer is copied
    less = ( cs->frameHistoryWindow < oldSize ) ? cs->frameHistoryWindow : oldSize;
    memcpy( history, cs->frameHistory, less );

    FREE( cs->frameHistory );
  }

  cs->frameHistoryHead = 0;
  cs->frameHistory = history;
}

double ClockState_guessFrameDuration( ClockState * cs )
{
  int c;
  double totalTime = 0;

  for ( c = 0; c < cs->frameHistorySize; c++ )
  {
    totalTime += cs->frameHistory[c];
  }

  return totalTime / cs->frameHistorySize;
}

void ClockState_addToFrameHistory( ClockState * cs, double frame )
{
  cs->frameHistory[cs->frameHistoryHead] = frame;

  cs->frameHistoryHead++;
  if ( cs->frameHistoryHead >= cs->frameHistoryWindow )
  {
    cs->frameHistoryHead = 0;
  }

  cs->frameHistorySize++;
  if ( cs->frameHistorySize > cs->frameHistoryWindow )
  {
    cs->frameHistorySize = cs->frameHistoryWindow;
  }
}

double ClockState_getExactLastFrameDuration( ClockState * cs )
{
  clock_source_ptr psrc;
  double sourceTime;
  double timeElapsed;

  psrc = clock_getTimeSource();
  if ( NULL != psrc )
  {
    sourceTime = psrc();
  }
  else
  {
    sourceTime = 0;
  }

  timeElapsed = sourceTime - cs->sourceLastTime;
  // If more time elapsed than the maximum we allow, say that only the maximum occurred
  if ( timeElapsed > cs->maximumFrameTime )
  {
    timeElapsed = cs->maximumFrameTime;
  }

  cs->sourceLastTime = sourceTime;
  return timeElapsed;
}

void ClockState_frameStep( ClockState * cs )
{
  double lastFrame = ClockState_getExactLastFrameDuration( cs );
  ClockState_addToFrameHistory( cs, lastFrame );

  // This feels wrong to me; we're guessing at how long this
  // frame is going to be and accepting that as our time value.
  // I'll trust Mr. Lopis for now, but it may change.
  cs->frameTime = ClockState_guessFrameDuration( cs );
  cs->currentTime += cs->frameTime;

  cs->frameNumber++;
}

double ClockState_getTime( ClockState * cs )
{
  return cs->currentTime;
}

double ClockState_getFrameDuration( ClockState * cs )
{
  return cs->frameTime;
}

Uint32 ClockState_getFrameNumber( ClockState * cs )
{
  return cs->frameNumber;
}

float ClockState_getFrameRate( ClockState * cs )
{
  return ( float )( 1.0 / cs->frameTime );
}
