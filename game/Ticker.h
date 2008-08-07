/* Egoboo - Ticker_t.h
 * This code is not currently in use.
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


#ifndef egoboo_Ticker_h
#define egoboo_Ticker_h

struct sTicker
{
  double lastTime;
  double tickInterval;
  int numTicks;
};
typedef struct sTicker Ticker_t;

extern void ticker_initWithInterval( Ticker_t *ticker, double interval );
extern void ticker_initWithFrequency( Ticker_t *ticker, int freq );

extern void ticker_update( Ticker_t *ticker );
extern int  ticker_tick( Ticker_t *ticker );

#endif // include guard
