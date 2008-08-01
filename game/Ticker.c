//********************************************************************************************
//* Egoboo - Ticker.c
//*
//* Implements a kind of stopwatch. Given a time interval, it measures the
//* integer number of time intervals that have elapsed since the ticker was started.
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

#include "Ticker.h"

#include "Clock.h"
#include <assert.h>
#include <stddef.h>

static ClockState * tickerClock = NULL;

void ticker_initWithInterval( Ticker *ticker, double interval )
{
  assert( NULL != ticker  && "ticker_initWithInterval: NULL ticker passed!" );
  if ( NULL == ticker  || interval <= 0 ) return;

  if ( NULL == tickerClock )
  {
    tickerClock = ClockState_create("ticker", -1);
  }

  ticker->lastTime = ClockState_getTime( tickerClock );
  ticker->numTicks = 0;
  ticker->tickInterval = interval;
}

void ticker_initWithFrequency( Ticker *ticker, int freq )
{
  double interval = 1.0 / freq;
  ticker_initWithInterval( ticker, interval );
}

void ticker_update( Ticker *ticker )
{
  double deltaTime, currentTime;
  assert( NULL != ticker  && "ticker_update: NULL ticker passed!" );
  if ( NULL == ticker  ) return;

  currentTime = ClockState_getTime( tickerClock );
  deltaTime = currentTime - ticker->lastTime;

  while ( deltaTime > ticker->tickInterval )
  {
    ticker->numTicks++;
    ticker->lastTime += ticker->tickInterval;

    deltaTime -= ticker->tickInterval;
  }
}

int ticker_tick( Ticker *ticker )
{
  int numTicks;

  assert( NULL != ticker  && "ticker_tick: NULL ticker passed!" );
  if ( NULL == ticker  ) return 0;

  numTicks = ticker->numTicks;
  ticker->numTicks--;
  return numTicks;
}


