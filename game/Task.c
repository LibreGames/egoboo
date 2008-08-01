//********************************************************************************************
//* Egoboo - Task.c
//*
//* Implements a pseudo-multi-threading environment.
//* Could be used as the master control for the menu system
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

#include "Task.h"

#include "Clock.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

struct task_t
{
  char *name;
  TaskCallback func;

  double interval;
  double timeLastCalled;
  char paused;

  struct task_t *previous, *next;
};

typedef struct task_t Task;

Task *task_list = NULL;
ClockState * taskClock = NULL;

void task_register( const char *taskName, float timeInterval, TaskCallback f )
{
  Task *aTask;
  size_t len;

  if ( NULL == taskName  || NULL == f  )
  {
    return;
  }

  if ( NULL == taskClock )
  {
    taskClock = ClockState_create("task", -1);
  };

  // If the time interval is negative, treat it as 0
  if ( timeInterval < 0 ) timeInterval = 0;

  len = strlen( taskName ) + 1;
  aTask = (Task*)calloc( 1, sizeof( Task ) );

  aTask->name = (char*)calloc( len, sizeof(char) );
  strncpy( aTask->name, taskName, len );
  aTask->func = f;

  aTask->interval = timeInterval;
  aTask->timeLastCalled = ( float ) ClockState_getTime( taskClock );

  if ( NULL == task_list  )
  {
    task_list = aTask;
  }
  else
  {
    // Order on the list doesn't really matter, so just stick this
    // new task on the front so that I don't have to maintain a tail variable
    aTask->next = task_list;
    task_list->previous = aTask;
    task_list = aTask;
  }
}

void task_remove( const char *taskName )
{
  Task *aTask;

  aTask = task_list;
  while ( NULL != aTask  )
  {
    if ( strcmp( taskName, aTask->name ) == 0 )
    {
      if ( aTask->previous ) aTask->previous->next = aTask->next;
      if ( aTask->next ) aTask->next->previous = aTask->previous;

      if ( aTask == task_list ) task_list = aTask->next;

      if( NULL != aTask->name ) { free(aTask->name); aTask->name = NULL; };
      if( NULL != aTask )       { free(aTask); aTask = NULL; };
      return;
    }
    aTask = aTask->next;
  }

  if ( NULL == task_list )
  {
    ClockState_destroy( &taskClock );
  }
}

void task_pause( const char *taskName )
{
  Task *aTask;

  aTask = task_list;
  while ( NULL != aTask  )
  {
    if ( strcmp( taskName, aTask->name ) == 0 )
    {
      aTask->paused = 1;
      return;
    }
    aTask = aTask->next;
  }
}

void task_play( const char *taskName )
{
  Task *aTask;

  aTask = task_list;
  while ( NULL != aTask  )
  {
    if ( strcmp( taskName, aTask->name ) == 0 )
    {
      aTask->paused = 0;
      return;
    }
    aTask = aTask->next;
  }
}

void task_updateAllTasks()
{
  double currentTime, deltaTime;
  Task *aTask;

  currentTime = ClockState_getTime( taskClock );

  aTask = task_list;
  while ( NULL != aTask  )
  {
    if ( aTask->paused )
    {
      // don't call the task, but update it's last called time
      // anyway so that things don't get weird when it is unpaused
      aTask->timeLastCalled = currentTime;
    }
    else
    {
      deltaTime = currentTime - aTask->timeLastCalled;
      if ( deltaTime >= aTask->interval )
      {
        aTask->func(( float ) deltaTime );
        aTask->timeLastCalled = currentTime;
      }
    }

    aTask = aTask->next;
  }
}
