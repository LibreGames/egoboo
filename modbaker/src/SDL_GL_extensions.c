#include "SDL_GL_extensions.h"
#include "ogl_debug.h"

//---------------------------------------------------------------------
//-   Global function stolen from Jonathan Fisher
//- who stole it from gl_font.c test program from SDL_ttf ;)
//---------------------------------------------------------------------
int powerOfTwo( int input )
{
	int value = 1;

	while ( value < input )
	{
		value <<= 1;
	}
	return value;
}


//---------------------------------------------------------------------
//-   Global function stolen from Jonathan Fisher
//- who stole it from gl_font.c test program from SDL_ttf ;)
//---------------------------------------------------------------------
int copySurfaceToTexture( SDL_Surface *surface, GLuint texture, GLfloat *texCoords )
{
	int w, h;
	SDL_Surface *image;
	SDL_Rect area;
	Uint32  saved_flags;
	Uint8  saved_alpha;

	// Use the surface width & height expanded to the next powers of two
	w = powerOfTwo( surface->w );
	h = powerOfTwo( surface->h );
	texCoords[0] = 0.0f;
	texCoords[1] = 0.0f;
	texCoords[2] = ( GLfloat )surface->w / w;
	texCoords[3] = ( GLfloat )surface->h / h;

	image = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, rmask, gmask, bmask, amask );

	if ( image == NULL )
	{
		return 0;
	}

	// Save the alpha blending attributes
	saved_flags = surface->flags & ( SDL_SRCALPHA | SDL_RLEACCELOK );
	saved_alpha = surface->format->alpha;
	if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
	{
		SDL_SetAlpha( surface, 0, 0 );
	}

	// Copy the surface into the texture image
	area.x = 0;
	area.y = 0;
	area.w = surface->w;
	area.h = surface->h;
	SDL_BlitSurface( surface, &area, image, &area );

	// Restore the blending attributes
	if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
	{
		SDL_SetAlpha( surface, saved_flags, saved_alpha );
	}

	// Send the texture to OpenGL
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D,  0, GL_RGBA,  w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,  image->pixels );

	// Don't need the extra image anymore
	SDL_FreeSurface( image );

	return 1;
}



