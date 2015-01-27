/* Egoboo - gltexture.c
 * Loads BMP files into OpenGL textures.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h" // GAC - Needed for Win32 stuff
#include "gltexture.h"
#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
// The next two functions are borrowed from the gl_font.c test program from SDL_ttf
int powerOfTwo(int input)
{
  int value = 1;

  while (value < input)
  {
    value <<= 1;
  }
  return value;
}

//--------------------------------------------------------------------------------------------
Uint32 GLTexture_Load(GLenum tx_target, GLTexture *texture, const char *filename, Uint32 key)
{
  SDL_Surface * image, * screen;
  SDL_PixelFormat * pformat;
  SDL_PixelFormat   tmpformat;

  if(NULL==texture || NULL==filename || 0x00==filename[0]) return INVALID_TEXTURE;

  // make sure the old texture has been freed
  if(INVALID_TEXTURE!=texture->textureID) GLTexture_Release(texture);


  image = IMG_Load(filename);
  if (NULL == image) return INVALID_TEXTURE;

  /* set the color key, if valid */
  if (NULL != image->format && NULL != image->format->palette && INVALID_KEY != key)
  {
    SDL_SetColorKey(image, SDL_SRCCOLORKEY, key);
  };

  /* Set the texture's alpha */
  texture->alpha = image->format->alpha / 255.0f;

  /* Set the original image's size (incase it's not an exact square of a power of two) */
  texture->imgH = image->h;
  texture->imgW = image->w;

  /* Determine the correct power of two greater than or equal to the original image's size */
  texture->txH = powerOfTwo(image->h);
  texture->txW = powerOfTwo(image->w);

  screen  = SDL_GetVideoSurface();
  pformat = screen->format;
  memcpy(&tmpformat, screen->format, sizeof(SDL_PixelFormat)); // make a copy of the format

  if (0 != (image->flags & (SDL_SRCALPHA | SDL_SRCCOLORKEY)))
  {
    // the source image has an alpha channel
    // TO DO : need to take into account the possible SDL_PixelFormat::Rloss, SDL_PixelFormat::Gloss, ...
    //         parameters

    int i;

    // create the mask
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
    tmpformat.Amask = (Uint32)(0xFF << 24);
    tmpformat.Bmask = (Uint32)(0xFF << 16);
    tmpformat.Gmask = (Uint32)(0xFF <<  8);
    tmpformat.Rmask = (Uint32)(0xFF <<  0);
#else
    tmpformat.Amask = (Uint32)(0xFF <<  0);
    tmpformat.Bmask = (Uint32)(0xFF <<  8);
    tmpformat.Gmask = (Uint32)(0xFF << 16);
    tmpformat.Rmask = (Uint32)(0xFF << 24);
#endif

    tmpformat.BitsPerPixel  = 32;
    tmpformat.BytesPerPixel =  4;

    for (i = 0; i < 32 && (tmpformat.Amask&(1 << i)) == 0; i++);
    tmpformat.Ashift = i;
    tmpformat.Aloss  = 0;

    pformat = &tmpformat;
  }
  else
  {
    // the source image has no alpha channel
    // convert it to the screen format

    // correct the bits and bytes per pixel
    tmpformat.BitsPerPixel  = 32 - (tmpformat.Rloss + tmpformat.Gloss + tmpformat.Bloss + tmpformat.Aloss);
    tmpformat.BytesPerPixel = tmpformat.BitsPerPixel / 8 + ((tmpformat.BitsPerPixel % 8) > 0 ? 1 : 0);

    pformat = &tmpformat;
  }

  {
    //convert the image format to the correct format
    SDL_Surface * tmp;
    Uint32 convert_flags = SDL_SWSURFACE;
    //if(image->flags & SDL_SRCALPHA) convert_flags |= SDL_SRCALPHA;
    //if(image->flags & SDL_SRCCOLORKEY) convert_flags |= SDL_SRCCOLORKEY;
    tmp = SDL_ConvertSurface(image, pformat, convert_flags);
    SDL_FreeSurface(image);
    image = tmp;

    // fix the alpha channel on the new SDL_Surface.  For some reason, SDL wants to create
    // surface with surface->format->alpha==0x00 which causes a problem if we have to
    // use the SDL_BlitSurface() function below.  With the surface alpha set to zero, the
    // image will be converted to BLACK!
    //
    // The following statement tells SDL
    //   1) to set the image to opaque
    //   2) not to alpha blend the surface on blit
    SDL_SetAlpha(image, 0, SDL_ALPHA_OPAQUE);
    SDL_SetColorKey(image, 0, 0);
  }

  // create a texture that is acceptable to OpenGL (height and width are powers of 2)
  if (texture->imgH != texture->txH || texture->imgW != texture->txW)
  {
    SDL_Surface * tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, pformat->BitsPerPixel, pformat->Rmask, pformat->Gmask, pformat->Bmask, pformat->Amask);

    SDL_BlitSurface(image, &image->clip_rect, tmp, &image->clip_rect);
    SDL_FreeSurface(image);
    image = tmp;
  };

  /* Generate an OpenGL texture ID */
  glGenTextures(1, &texture->textureID);
  texture->texture_target =  tx_target;

  /* Set up some parameters for the format of the OpenGL texture */
  GLTexture_Bind(texture, CData.texturefilter);

  /* actually create the OpenGL textures */
  if (image->format->Aloss == 8 && tx_target == GL_TEXTURE_2D)
  {
    if (CData.texturefilter >= TX_MIPMAP && tx_target == GL_TEXTURE_2D)
      gluBuild2DMipmaps(tx_target, GL_RGB, image->w, image->h, GL_BGR_EXT, GL_UNSIGNED_BYTE, image->pixels);   //With mipmapping
    else
      glTexImage2D(tx_target, 0, GL_RGB, image->w, image->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image->pixels);
  }
  else
  {
    if (CData.texturefilter >= TX_MIPMAP && tx_target == GL_TEXTURE_2D)
      gluBuild2DMipmaps(GL_TEXTURE_2D, 4, image->w, image->h, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);   //With mipmapping
    else
      glTexImage2D(tx_target, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
  }

  SDL_FreeSurface(image);

  texture->texture_target = tx_target;

  return texture->textureID;
}

/********************> GLTexture_GetTextureID() <*****/
GLuint  GLTexture_GetTextureID(GLTexture *texture)
{

  return texture->textureID;

}

/********************> GLTexture_GetImageHeight() <*****/
GLsizei  GLTexture_GetImageHeight(GLTexture *texture)
{

  return texture->imgH;

}

/********************> GLTexture_GetImageWidth() <*****/
GLsizei  GLTexture_GetImageWidth(GLTexture *texture)
{

  return texture->imgW;

}

/********************> GLTexture_GetTextureWidth() <*****/
GLsizei  GLTexture_GetTextureWidth(GLTexture *texture)
{
  return texture->txW;
}

/********************> GLTexture_GetTextureHeight() <*****/
GLsizei  GLTexture_GetTextureHeight(GLTexture *texture)
{
  return texture->txH;
}


/********************> GLTexture_SetAlpha() <*****/
void  GLTexture_SetAlpha(GLTexture *texture, GLfloat alpha)
{

  texture->alpha = alpha;

}

/********************> GLTexture_GetAlpha() <*****/
GLfloat  GLTexture_GetAlpha(GLTexture *texture)
{

  return texture->alpha;

}

/********************> GLTexture_Release() <*****/
void  GLTexture_Release(GLTexture *texture)
{
  /* Delete the OpenGL texture */
  glDeleteTextures(1, &texture->textureID);

  /* Reset the other data */
  texture->textureID = INVALID_TEXTURE;
  texture->imgH = texture->imgW = texture->txW = texture->txH  = 0;
}

/********************> GLTexture_Release() <*****/
void GLTexture_Bind(GLTexture *texture, int filt_type)
{
  if (NULL == texture) return;

  if (!glIsEnabled(texture->texture_target))
  {
    glEnable(texture->texture_target);
  };

  glBindTexture(texture->texture_target, texture->textureID);

  if (filt_type >= TX_ANISOTROPIC)
  {
    //Anisotropic filtered!
    glTexParameterf(texture->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(texture->texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(texture->texture_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, userAnisotropy);
  }
  else
  {
    switch (filt_type)
    {
      // Unfiltered
    case TX_UNFILTERED:
      glTexParameterf(texture->texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(texture->texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      break;

      // Linear filtered
    case TX_LINEAR:
      glTexParameterf(texture->texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(texture->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      break;

      // Bilinear interpolation
    case TX_MIPMAP:
      glTexParameterf(texture->texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
      glTexParameterf(texture->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      break;

      // Bilinear interpolation
    case TX_BILINEAR:
      glTexParameterf(texture->texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
      glTexParameterf(texture->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      break;

      // Trilinear filtered (quality 1)
    case TX_TRILINEAR_1:
      glTexParameterf(texture->texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
      glTexParameterf(texture->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      break;

      // Trilinear filtered (quality 2)
    case TX_TRILINEAR_2:
      glTexParameterf(texture->texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameterf(texture->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      break;

    };
  }

};