//********************************************************************************************
//* Egoboo - ogl_texture.c
//*
//* Implements OpenGL texture loading using SDL_image.
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
#include "ogl_texture.h"

#include "game.h"

#include "graphic.inl"
#include "egoboo_types.inl"

#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
// The next two functions are borrowed from the gl_font.c test program from SDL_ttf
int powerOfTwo( int input )
{
  int value = 1;

  while ( value < input )
  {
    value <<= 1;
  }
  return value;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GLtexture * GLtexture_new( GLtexture * ptx )
{
  if( EKEY_PVALID(ptx) )
  {
    GLtexture_delete( ptx );
  }

  memset(ptx, 0, sizeof(GLtexture));

  EKEY_PNEW(ptx, GLtexture);

  ptx->textureID = INVALID_TEXTURE;

  return ptx;
}

//--------------------------------------------------------------------------------------------
bool_t GLtexture_delete( GLtexture * ptx )
{
  if( !EKEY_PVALID(ptx) ) return btrue;

  GLtexture_Release( ptx );

  EKEY_PINVALIDATE( ptx );

  ptx->textureID = INVALID_TEXTURE;

  return btrue;
}

//--------------------------------------------------------------------------------------------
GLtexture * GLtexture_create()
{
  GLtexture * ptx = EGOBOO_NEW( GLtexture );
  ptx = GLtexture_new( ptx );

  if(NULL != ptx)
  {
    ptx->ekey.dynamic = btrue;
  };

  return ptx;
}

//--------------------------------------------------------------------------------------------
bool_t GLtexture_destory( GLtexture ** pptx )
{
  bool_t dynamic = bfalse;

  if( NULL == pptx || !EKEY_PVALID( (*pptx) ) ) return bfalse;

  dynamic = (*pptx)->ekey.dynamic;

  GLtexture_delete( *pptx );

  if( dynamic ) free( *pptx );

  *pptx = NULL;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Uint32 GLtexture_Convert( GLenum tx_target, GLtexture *texture, SDL_Surface * image, Uint32 key )
{
  SDL_Surface     * screen;
  SDL_PixelFormat * pformat;
  SDL_PixelFormat   tmpformat;

  if ( NULL == texture || NULL == image) return INVALID_TEXTURE;

  // make sure the old texture has been freed
  GLtexture_Release( texture );

  if ( NULL == image ) return INVALID_TEXTURE;

  /* set the color key, if valid */
  if ( NULL != image->format && NULL != image->format->palette && INVALID_KEY != key )
  {
    SDL_SetColorKey( image, SDL_SRCCOLORKEY, key );
  };

  /* Set the texture's alpha */
  texture->alpha = FP8_TO_FLOAT( image->format->alpha );

  /* Set the original image's size (incase it's not an exact square of a power of two) */
  texture->imgH = image->h;
  texture->imgW = image->w;

  /* Determine the correct power of two greater than or equal to the original image's size */
  texture->txH = powerOfTwo( image->h );
  texture->txW = powerOfTwo( image->w );

  screen  = SDL_GetVideoSurface();
  pformat = screen->format;
  memcpy( &tmpformat, screen->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format

  if ( 0 != ( image->flags & ( SDL_SRCALPHA | SDL_SRCCOLORKEY ) ) )
  {
    // the source image has an alpha channel
    /// @todo need to take into account the possible SDL_PixelFormat::Rloss, SDL_PixelFormat::Gloss, ...
    /// parameters

    int i;

    // create the mask
    // this will work if both endian systems think they have "RGBA" graphics
    // if you need a different pixel format (ARGB or BGRA or whatever) this section
    // will have to be changed to reflect that
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
    tmpformat.Amask = ( Uint32 )( 0xFF << 24 );
    tmpformat.Bmask = ( Uint32 )( 0xFF << 16 );
    tmpformat.Gmask = ( Uint32 )( 0xFF <<  8 );
    tmpformat.Rmask = ( Uint32 )( 0xFF <<  0 );
#else
    tmpformat.Amask = ( Uint32 )( 0xFF <<  0 );
    tmpformat.Bmask = ( Uint32 )( 0xFF <<  8 );
    tmpformat.Gmask = ( Uint32 )( 0xFF << 16 );
    tmpformat.Rmask = ( Uint32 )( 0xFF << 24 );
#endif


    tmpformat.BitsPerPixel  = screen->format->BitsPerPixel;
    tmpformat.BytesPerPixel = screen->format->BytesPerPixel;

    for ( i = 0; i < gfxState.scrz && ( tmpformat.Amask & ( 1 << i ) ) == 0; i++ );
    if( 0 == (tmpformat.Amask & ( 1 << i )) )
    {
      // no alpha bits available
      tmpformat.Ashift = 0;
      tmpformat.Aloss  = 8;
    }
    else
    {
      // normal alpha channel
      tmpformat.Ashift = i;
      tmpformat.Aloss  = 0;
    }


    pformat = &tmpformat;
  }
  else
  {
    // the source image has no alpha channel
    // convert it to the screen format

    // correct the bits and bytes per pixel
    tmpformat.BitsPerPixel  = 32 - ( tmpformat.Rloss + tmpformat.Gloss + tmpformat.Bloss + tmpformat.Aloss );
    tmpformat.BytesPerPixel = tmpformat.BitsPerPixel / 8 + (( tmpformat.BitsPerPixel % 8 ) > 0 ? 1 : 0 );

    pformat = &tmpformat;
  }

  {
    //convert the image format to the correct format
    SDL_Surface * tmp;
    Uint32 convert_flags = SDL_SWSURFACE;
    tmp = SDL_ConvertSurface( image, pformat, convert_flags );
    SDL_FreeSurface( image );
    image = tmp;

    // fix the alpha channel on the new SDL_Surface.  For some reason, SDL wants to create
    // surface with surface->format->alpha==0x00 which causes a problem if we have to
    // use the SDL_BlitSurface() function below.  With the surface alpha set to zero, the
    // image will be converted to BLACK!
    //
    // The following statement tells SDL
    //   1) to set the image to opaque
    //   2) not to alpha blend the surface on blit
    SDL_SetAlpha( image, 0, SDL_ALPHA_OPAQUE );
    SDL_SetColorKey( image, 0, 0 );
  }

  // create a texture that is acceptable to OpenGL (height and width are powers of 2)
  if ( texture->imgH != texture->txH || texture->imgW != texture->txW )
  {
    SDL_Surface * tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txW, texture->txH, pformat->BitsPerPixel, pformat->Rmask, pformat->Gmask, pformat->Bmask, pformat->Amask );

    SDL_BlitSurface( image, &image->clip_rect, tmp, &image->clip_rect );
    SDL_FreeSurface( image );
    image = tmp;
  };

  /* Generate an OpenGL texture ID */
  glGenTextures( 1, &texture->textureID );
  texture->texture_target =  tx_target;

  /* Set up some parameters for the format of the OpenGL texture */
  GLtexture_Bind( texture, &gfxState );

  /* actually create the OpenGL textures */
  if ( image->format->Aloss == 8 && tx_target == GL_TEXTURE_2D )
  {
    if ( CData.texturefilter >= TX_MIPMAP && tx_target == GL_TEXTURE_2D )
      gluBuild2DMipmaps( tx_target, GL_RGB, image->w, image->h, GL_BGR_EXT, GL_UNSIGNED_BYTE, image->pixels );    //With mipmapping
    else
      glTexImage2D( tx_target, 0, GL_RGB, image->w, image->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image->pixels );
  }
  else
  {
    if ( CData.texturefilter >= TX_MIPMAP && tx_target == GL_TEXTURE_2D )
      gluBuild2DMipmaps( GL_TEXTURE_2D, 4, image->w, image->h, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );    //With mipmapping
    else
      glTexImage2D( tx_target, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );
  }

  SDL_FreeSurface( image );

  texture->texture_target = tx_target;

  return texture->textureID;
}



//--------------------------------------------------------------------------------------------
Uint32 GLtexture_Load( GLenum tx_target, GLtexture *texture, EGO_CONST char *filename, Uint32 key )
{
  Uint32 retval;
  SDL_Surface * image;

  // get rid of any old data
  GLtexture_delete(texture);

  if ( !VALID_CSTR(filename)  ) return INVALID_TEXTURE;

  // initialize the texture
  if ( NULL == GLtexture_new( texture ) ) return INVALID_TEXTURE;

  image = IMG_Load( filename );
  if ( NULL == image ) return INVALID_TEXTURE;

  retval = GLtexture_Convert( tx_target, texture, image, key );
  strncpy(texture->name, filename, sizeof(texture->name));

  if(INVALID_TEXTURE == retval)
  {
    //printf("****GLtexture_Load() - failed to load texture \"%s\".\n", filename );
    GLtexture_delete(texture);
  }
  else
  {
    //printf("****GLtexture_Load() - loaded texture \"%s\". ID == %d.\n", filename, texture->textureID );
  }

  return retval;
}

/********************> GLtexture_GetTextureID() <*****/
GLuint  GLtexture_GetTextureID( GLtexture *texture )
{
  return texture->textureID;
}

/********************> GLtexture_GetImageHeight() <*****/
GLsizei  GLtexture_GetImageHeight( GLtexture *texture )
{
  return texture->imgH;
}

/********************> GLtexture_GetImageWidth() <*****/
GLsizei  GLtexture_GetImageWidth( GLtexture *texture )
{
  return texture->imgW;
}

/********************> GLtexture_GetTextureWidth() <*****/
GLsizei  GLtexture_GetTextureWidth( GLtexture *texture )
{
  return texture->txW;
}

/********************> GLtexture_GetTextureHeight() <*****/
GLsizei  GLtexture_GetTextureHeight( GLtexture *texture )
{
  return texture->txH;
}


/********************> GLtexture_SetAlpha() <*****/
void  GLtexture_SetAlpha( GLtexture *texture, GLfloat alpha )
{
  texture->alpha = alpha;
}

/********************> GLtexture_GetAlpha() <*****/
GLfloat  GLtexture_GetAlpha( GLtexture *texture )
{
  return texture->alpha;
}

/********************> GLtexture_Release() <*****/
void  GLtexture_Release( GLtexture *texture )
{
  if ( !EKEY_PVALID(texture) ) return;

  if( INVALID_TEXTURE != texture->textureID )
  {
    /* Delete the OpenGL texture */
    glDeleteTextures( 1, &texture->textureID );
    texture->textureID = INVALID_TEXTURE;
    //printf("****GLtexture_Release() - releasing texture %s.\n", texture->name );
  }


  /* Reset the other data */
  texture->imgH = texture->imgW = texture->txW = texture->txH  = 0;

  texture->name[0] = EOS;
}

/********************> GLtexture_Release() <*****/
void GLtexture_Bind( GLtexture *texture, Graphics_t * g )
{
  int    filt_type, anisotropy;
  GLenum target;
  GLuint id;

  target = GL_TEXTURE_2D;
  id     = INVALID_TEXTURE;
  if ( NULL != texture )
  {
    target = texture->texture_target;
    id     = texture->textureID;
  }

  filt_type  = 0;
  anisotropy = 0;
  if(NULL != g)
  {
    filt_type  = g->texturefilter;
    anisotropy = g->userAnisotropy;
  }

  if ( !glIsEnabled( target ) )
  {
    glEnable( target );
  };

  glBindTexture( target, id );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

  if ( filt_type >= TX_ANISOTROPIC )
  {
    //Anisotropic filtered!
    glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy );
  }
  else
  {
    switch ( filt_type )
    {
        // Unfiltered
      case TX_UNFILTERED:
        glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        break;

        // Linear filtered
      case TX_LINEAR:
        glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        break;

        // Bilinear interpolation
      case TX_MIPMAP:
        glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
        glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        break;

        // Bilinear interpolation
      case TX_BILINEAR:
        glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
        glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        break;

        // Trilinear filtered (quality 1)
      case TX_TRILINEAR_1:
        glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
        glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        break;

        // Trilinear filtered (quality 2)
      case TX_TRILINEAR_2:
        glTexParameterf( target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameterf( target, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        break;

    };
  }

};


