/*
  pixdemo - demo of put/get pixel for SDL.
  Copyright (C) 1999, 2000 Neil McGill

  This game is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This game is distributed in the hope that it will be fun,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this game; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Neil McGill

  $Id: SDL_PutPixel.cpp,v 1.1 2002/08/21 22:14:00 arakon Exp $
*/

#include "SDL_Pixel.h"
#include <string.h>

/*
 * This function sets the specified pixel on a surface. Sanity checks are
 * performed on the co-ordinates and the surface is locked for you.
 * Safe, but slow. For more speed, try the lower level access function.
 */
int SDL_PutPixel( SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 rgb )
{
    /* Perform clipping */
    if (( x < 0 ) || ( y < 0 ) || ( x > dst->w ) || ( y > dst->h ) )
    {
        return -1;
    }

    /* Perform software fill */
    if ( SDL_LockSurface( dst ) != 0 )
    {
        return -1;
    }

    /* semi-private interface */
    SDL_LowerPutPixel( dst, x, y, rgb );

    SDL_UnlockSurface( dst );

    return 0;
}

/*
 * This function performs a 'low-level' pixel put, on a previously locked
 * surface. It is assumed that the pixel is within the bounds of the surface.
 * i.e. there are no sanity checks here to save you!
 */
void SDL_LowerPutPixel( SDL_Surface *dst, Sint16 x, Sint16 y, Uint32 rgb )
{
    Uint8  bpp = dst->format->BytesPerPixel;
    Uint16  pitch = dst->pitch / bpp;

    switch ( bpp )
    {
        default:
        case 1:
            switch ( pitch )
            {
                case 16:
                    SDL_PutPixel_16x8bpp( dst, x, y, rgb );
                    return;
                case 24:
                    SDL_PutPixel_24x8bpp( dst, x, y, rgb );
                    return;
                case 32:
                    SDL_PutPixel_32x8bpp( dst, x, y, rgb );
                    return;
                case 48:
                    SDL_PutPixel_48x8bpp( dst, x, y, rgb );
                    return;
                case 64:
                    SDL_PutPixel_64x8bpp( dst, x, y, rgb );
                    return;
                case 128:
                    SDL_PutPixel_128x8bpp( dst, x, y, rgb );
                    return;
                case 256:
                    SDL_PutPixel_256x8bpp( dst, x, y, rgb );
                    return;
                case 320:
                    SDL_PutPixel_320x8bpp( dst, x, y, rgb );
                    return;
                case 640:
                    SDL_PutPixel_640x8bpp( dst, x, y, rgb );
                    return;
                case 800:
                    SDL_PutPixel_800x8bpp( dst, x, y, rgb );
                    return;
                case 1024:
                    SDL_PutPixel_1024x8bpp( dst, x, y, rgb );
                    return;
                default:
                    SDL_PutPixel_8bpp( dst, x, y, rgb );
                    return;
            }

        case 2:
            switch ( pitch )
            {
                case 16:
                    SDL_PutPixel_16x16bpp( dst, x, y, rgb );
                    return;
                case 24:
                    SDL_PutPixel_24x16bpp( dst, x, y, rgb );
                    return;
                case 32:
                    SDL_PutPixel_32x16bpp( dst, x, y, rgb );
                    return;
                case 48:
                    SDL_PutPixel_48x16bpp( dst, x, y, rgb );
                    return;
                case 64:
                    SDL_PutPixel_64x16bpp( dst, x, y, rgb );
                    return;
                case 128:
                    SDL_PutPixel_128x16bpp( dst, x, y, rgb );
                    return;
                case 256:
                    SDL_PutPixel_256x16bpp( dst, x, y, rgb );
                    return;
                case 320:
                    SDL_PutPixel_320x16bpp( dst, x, y, rgb );
                    return;
                case 640:
                    SDL_PutPixel_640x16bpp( dst, x, y, rgb );
                    return;
                case 800:
                    SDL_PutPixel_800x16bpp( dst, x, y, rgb );
                    return;
                case 1024:
                    SDL_PutPixel_1024x16bpp( dst, x, y, rgb );
                    return;
                default:
                    SDL_PutPixel_16bpp( dst, x, y, rgb );
                    return;
            }

        case 3:
            switch ( pitch )
            {
                case 16:
                    SDL_PutPixel_16x24bpp( dst, x, y, rgb );
                    return;
                case 24:
                    SDL_PutPixel_24x24bpp( dst, x, y, rgb );
                    return;
                case 32:
                    SDL_PutPixel_32x24bpp( dst, x, y, rgb );
                    return;
                case 48:
                    SDL_PutPixel_48x24bpp( dst, x, y, rgb );
                    return;
                case 64:
                    SDL_PutPixel_64x24bpp( dst, x, y, rgb );
                    return;
                case 128:
                    SDL_PutPixel_128x24bpp( dst, x, y, rgb );
                    return;
                case 256:
                    SDL_PutPixel_256x24bpp( dst, x, y, rgb );
                    return;
                case 640:
                    SDL_PutPixel_640x24bpp( dst, x, y, rgb );
                    return;
                case 320:
                    SDL_PutPixel_320x24bpp( dst, x, y, rgb );
                    return;
                case 800:
                    SDL_PutPixel_800x24bpp( dst, x, y, rgb );
                    return;
                case 1024:
                    SDL_PutPixel_1024x24bpp( dst, x, y, rgb );
                    return;
                default:
                    SDL_PutPixel_24bpp( dst, x, y, rgb );
                    return;
            }

        case 4:
            switch ( pitch )
            {
                case 16:
                    SDL_PutPixel_16x32bpp( dst, x, y, rgb );
                    return;
                case 24:
                    SDL_PutPixel_24x32bpp( dst, x, y, rgb );
                    return;
                case 32:
                    SDL_PutPixel_32x32bpp( dst, x, y, rgb );
                    return;
                case 48:
                    SDL_PutPixel_48x32bpp( dst, x, y, rgb );
                    return;
                case 64:
                    SDL_PutPixel_64x32bpp( dst, x, y, rgb );
                    return;
                case 128:
                    SDL_PutPixel_128x32bpp( dst, x, y, rgb );
                    return;
                case 256:
                    SDL_PutPixel_256x32bpp( dst, x, y, rgb );
                    return;
                case 320:
                    SDL_PutPixel_320x32bpp( dst, x, y, rgb );
                    return;
                case 640:
                    SDL_PutPixel_640x32bpp( dst, x, y, rgb );
                    return;
                case 800:
                    SDL_PutPixel_800x32bpp( dst, x, y, rgb );
                    return;
                case 1024:
                    SDL_PutPixel_1024x32bpp( dst, x, y, rgb );
                    return;
                default:
                    SDL_PutPixel_32bpp( dst, x, y, rgb );
                    return;
            }
    }
}

