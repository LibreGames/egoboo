#pragma once

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

///
/// @file
/// @brief Basic OpenGL Wrapper
/// @details Basic definitions for loading and managing OpenGL textures in Egoboo.
///   Uses SDL_image to load .tif, .png, .bmp, .dib, .xpm, and other formats into
///   OpenGL texures

#include "ogl_include.h"

#include <SDL.h>

struct sGraphics;

#define INVALID_TX_ID (~(GLuint)0)
#define INVALID_KEY     (~(Uint32)0)

/**> DATA STRUCTURE: GLtexture <**/
struct ogl_texture_t
{
  egoboo_key_t ekey;

  char    name[256];
  GLuint  textureID;    /* The OpenGL texture number */
  GLint   internalFormat;  /* GL_RGB or GL_RGBA */
  GLsizei imgH, imgW;      /* the height & width of the original image */
  GLsizei txH,  txW;     /* the height/width of the the OpenGL texture (must be a power of two) */
  GLfloat alpha;      /* the alpha for the texture */
  GLenum  texture_target;
};
typedef struct ogl_texture_t GLtexture;

/**> FUNCTION PROTOTYPES: GLtexture <**/
GLtexture * GLtexture_new( GLtexture * ptx );
bool_t      GLtexture_delete( GLtexture * ptx );
GLtexture * GLtexture_create( void );
bool_t      GLtexture_destory( GLtexture ** ptx );

Uint32  GLtexture_Convert( GLenum tx_target, GLtexture *texture, SDL_Surface * image, Uint32 key );
Uint32  GLtexture_Load( GLenum tx_target, GLtexture *texture, EGO_CONST char *filename, Uint32 key );
GLuint  GLtexture_GetTextureID( GLtexture *texture );
GLsizei GLtexture_GetImageHeight( GLtexture *texture );
GLsizei GLtexture_GetImageWidth( GLtexture *texture );
GLsizei GLtexture_GetTextureWidth( GLtexture *texture );
GLsizei GLtexture_GetTextureHeight( GLtexture *texture );
void    GLtexture_SetAlpha( GLtexture *texture, GLfloat alpha );
GLfloat GLtexture_GetAlpha( GLtexture *texture );
void    GLtexture_Release( GLtexture *texture );

void    GLtexture_Bind( GLtexture * texture, struct sGraphics * g );
