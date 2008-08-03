#include "System.h"

#include <SDL.h>

static double startuptime;

void sys_initialize()
{
	startuptime = SDL_GetTicks() * 1000000.0;
}

void sys_shutdown()
{
}

double sys_getTime()
{
	startuptime = SDL_GetTicks() * 1000000.0 - startuptime;
}

int sys_frameStep()
{
  return 0;
}

int main( int argc, char* argv[] )
{
  return SDL_main( argc, argv );
}
