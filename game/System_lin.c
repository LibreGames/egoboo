//********************************************************************************************
//* Egoboo - System_lin.c
//*
//* Unix/GNU/Linux/*nix - specific code
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

#include "System.h"

#include "egoboo_types.inl"

#include <stdio.h> /* for NULL */
#include <sys/time.h>

static double startuptime;

void sys_initialize()
{
  struct timeval now;
  gettimeofday( &now, NULL );
  startuptime = now.tv_sec + now.tv_usec / 1000000.0;
}

void sys_shutdown()
{
}

double sys_getTime( void )
{
  struct timeval now;
  gettimeofday( &now, NULL );
  return now.tv_sec + now.tv_usec / 1000000.0 - startuptime;
}

int sys_frameStep()
{
  return 0;
}

int main( int argc, char* argv[] )
{
  return SDL_main( argc, argv );
}
