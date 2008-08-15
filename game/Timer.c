//********************************************************************************************
//* Egoboo - Timer.c
//*
//* Implements a kind of alarm clock.
//* This code is not currently in use.
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

#include "Timer.h"

#include "Clock.h"
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

struct sTimerNode
{
  Timer_t *timer;
  struct sTimerNode *previous, *next;
};

typedef struct sTimerNode TimerNode_t;

static ClockState_t *timerClock = NULL;
static TimerNode_t  *timerList = NULL;
static TimerNode_t  *timerListEnd = NULL;
static int timersInUse = 0;

static TimerNode_t *findTimer( Timer_t *t )
{
  TimerNode_t *node = timerList;

  while ( NULL != node  )
  {
    if ( node->timer == t )
    {
      return node;
    }
    node = node->next;
  }

  return NULL;
}

void timer_init( ClockState_t * cs )
{
  // Just set some basic stuff
  timerClock  = cs;
  timerList   = NULL;
  timersInUse = 0;
}

void timer_shutdown()
{
  TimerNode_t *node, *next;

  // Clear out the list o' timers
  node = timerList;
  while ( NULL != node  )
  {
    next = node->next;
    if( NULL != node ) { free(node); node = NULL; };
    node = next;
  }

  timerList = NULL;
  timersInUse = 0;
}

void timer_update()
{
  TimerNode_t *node;

  node = timerList;
  while ( NULL != node  )
  {
    assert( NULL != node->timer );
    if ( !node->timer->isPaused )
    {
      node->timer->frameTime = ClockState_getFrameDuration( timerClock ) * node->timer->timeScale;
      node->timer->currentTime += node->timer->frameTime;
    }

    node = node->next;
  }
}

void timer_addTimer( Timer_t *t )
{
  TimerNode_t *node;

  if ( NULL == t ) return;

  node = EGOBOO_NEW( TimerNode_t );
  node->timer = t;

  if ( timerList )
  {
    timerListEnd->next = node;
    node->previous = timerListEnd;
    node->next = NULL;
  }
  else
  {
    timerList = node;
    node->previous = NULL;
    node->next = NULL;
    timerListEnd = node;
  }
  timersInUse++;
}

void timer_removeTimer( Timer_t *t )
{
  TimerNode_t *node;

  if ( NULL == t ) return;

  node = findTimer( t );
  if ( node )
  {
    if ( node == timerList )
    {
      // Is this the only node in the list?
      if ( NULL == timerList->next )
      {
        // yup
        timerList = NULL;
        timerListEnd = NULL;
      }
      else
      {
        timerList = node->next;
        timerList->previous = NULL;
      }
    }
    else if ( node == timerListEnd )
    {
      timerListEnd = node->previous;
      timerListEnd->next = NULL;
    }
    else
    {
      node->previous->next = node->next;
      node->next->previous = node->previous;
    }
    if( NULL != node ) { free(node); node = NULL; };
    timersInUse--;
  }
}
