//********************************************************************************************
//* Egoboo - egoboo_math.c
//*
//* Implementation of math functions
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

/**> HEADER FILES <**/
#include "egoboo_math.inl"

float turntosin[TRIGTABLE_SIZE];
float turntocos[TRIGTABLE_SIZE];

//--------------------------------------------------------------------------------------------
void make_turntosin( void )
{
  // ZZ> This function makes the lookup table for chrturn...

  int cnt;

  for ( cnt = 0; cnt < TRIGTABLE_SIZE; cnt++ )
  {
    turntosin[cnt] = sin(( TWO_PI * cnt ) / ( float ) TRIGTABLE_SIZE );
    turntocos[cnt] = cos(( TWO_PI * cnt ) / ( float ) TRIGTABLE_SIZE );
  }
}

