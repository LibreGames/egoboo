//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
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

/// @file extensions/ogl_texture.c
/// @ingroup _ogl_extensions_
/// @brief Implements OpenGL texture loading using SDL_image
/// @details Basic loading and managing OpenGL textures.
///   Uses SDL_image to load .tif, .png, .bmp, .dib, .xpm, and other formats into OpenGL texures

#include "ogl_texture.h"
#include "ogl_debug.h"

#include "SDL_GL_extensions.h"

#include "../graphic_defs.h"

#include "../egoboo_setup.h"
#include "../egoboo_strutil.h"

#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define VALID_VALUE        0x32B04E67
#define ErrorImage_width   2
#define ErrorImage_height  2

#define VALID_TEXTURE( PTEX ) ( (NULL != (PTEX)) && (VALID_VALUE == (PTEX)->valid) )

static GLuint ErrorImage_binding = INVALID_GL_ID;

oglx_texture_parameters_t tex_params = {TX_UNFILTERED, 0};

static GLboolean ErrorImage_defined = GL_FALSE;

typedef GLubyte image_row_t[ErrorImage_width][4];

static GLubyte ErrorImage[ErrorImage_height][ErrorImage_width][4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ErrorImage_create( void )
{
    /// @details BB@> define a default "error texture"

    int i, j;

    if ( INVALID_GL_ID != ErrorImage_binding ) return;

    GL_DEBUG( glGenTextures )( 1, &ErrorImage_binding );

    for ( i = 0; i < ErrorImage_height; i++ )
    {
        for ( j = 0; j < ErrorImage_width; j++ )
        {
            if ( 0 == (( i&0x1 ) ^( j&0x1 ) ) )
            {
                ErrorImage[i][j][0] = ( GLubyte ) 255;
                ErrorImage[i][j][1] = ( GLubyte ) 0;
                ErrorImage[i][j][2] = ( GLubyte ) 0;
            }
            else
            {
                ErrorImage[i][j][0] = ( GLubyte ) 0;
                ErrorImage[i][j][1] = ( GLubyte ) 255;
                ErrorImage[i][j][2] = ( GLubyte ) 255;
            }

            ErrorImage[i][j][3] = ( GLubyte ) 255;
        }
    }

    ErrorImage_bind( GL_TEXTURE_2D, ErrorImage_binding );

    ErrorImage_defined = GL_TRUE;
}

//--------------------------------------------------------------------------------------------
void ErrorImage_bind( GLenum target, GLuint id )
{
    // make sure the error texture exists
    if ( !ErrorImage_defined ) ErrorImage_create();

    // only do these commands if they will not cause an error
    if ( !oglx_NewList_active )
    {
        // force ogl to use a 1 byte alignment for the pixels
        GL_DEBUG( glPushClientAttrib )( GL_CLIENT_PIXEL_STORE_BIT ) ;
        GL_DEBUG( glPixelStorei )( GL_UNPACK_ALIGNMENT, 1 );

        GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_WRAP_S, GL_REPEAT );
        GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_WRAP_T, GL_REPEAT );
        GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        GL_DEBUG( glTexParameteri )( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    }

    // do the normal thing
    {
        GL_DEBUG( glBindTexture )( target, id );

        if ( target == GL_TEXTURE_1D )
        {
            GL_DEBUG( glTexImage1D )( GL_TEXTURE_1D, 0, GL_RGBA, ErrorImage_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage );
        }
        else
        {
            GL_DEBUG( glTexImage2D )( GL_TEXTURE_2D, 0, GL_RGBA, ErrorImage_width, ErrorImage_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage );
        }
    }

    if ( !oglx_NewList_active )
    {
        // restore the state
        GL_DEBUG( glPopClientAttrib )();
    }
}

//--------------------------------------------------------------------------------------------
GLuint ErrorImage_get_binding()
{
    // make sure the error texture exists
    if ( !ErrorImage_defined ) ErrorImage_create();

    return ErrorImage_binding;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oglx_texture_t * oglx_texture_ctor( oglx_texture_t * ptex )
{
    if ( NULL == ptex ) return ptex;

    SDL_memset( ptex, 0, sizeof( *ptex ) );

    // only need one base.binding per texture
    // do not need to ask for a new id, even if we change the texture data
    GL_DEBUG( glGenTextures )( 1, &( ptex->base.binding ) );

    // set the flag validity flag
    if ( VALID_BINDING( ptex->base.binding ) && !ERROR_IMAGE_BINDING( ptex->base.binding ) )
    {
        ptex->valid = VALID_VALUE;
    }
    else
    {
        ptex->valid = ( GLuint )( ~VALID_VALUE );
    }

    // set to 2d texture by default
    ptex->base.target = GL_TEXTURE_2D;

    // set the image to be clamped in s and t
    ptex->base.wrap_s = GL_CLAMP;
    ptex->base.wrap_t = GL_CLAMP;

    return ptex;
}

//--------------------------------------------------------------------------------------------
void oglx_texture_dtor( oglx_texture_t * ptex )
{
    if ( !VALID_TEXTURE( ptex ) )  return;

    // set a bad value for ptex->valid
    ptex->valid = ( GLuint )( ~VALID_VALUE );

    // actually delete the OpenGL texture data
    if ( VALID_BINDING( ptex->base.binding ) )
    {
        GL_DEBUG( glDeleteTextures )( 1, &ptex->base.binding );
        ptex->base.binding = INVALID_GL_ID;
    }

    // set the image to be clamped in s and t
    ptex->base.wrap_s = GL_CLAMP;
    ptex->base.wrap_t = GL_CLAMP;

    // Reset the other data
    ptex->imgH = ptex->imgW = ptex->base.width = ptex->base.height  = 0;
    ptex->name[0] = '\0';

    if ( NULL != ptex->surface )
    {
        SDL_FreeSurface( ptex->surface );
        ptex->surface = NULL;
    }
}

//--------------------------------------------------------------------------------------------
GLuint oglx_texture_Convert( oglx_texture_t *ptex, SDL_Surface * image, Uint32 key )
{
    /// @details BB@> an oglx_texture_t wrapper for the SDL_GL_convert_surface() function

    if ( NULL == ptex ) return INVALID_GL_ID;

    // make sure the old texture has been freed
    oglx_texture_Release( ptex );

    if ( NULL == image ) return INVALID_GL_ID;

    /* set the color key, if valid */
    if ( NULL != image->format && NULL != image->format->palette && INVALID_KEY != key )
    {
        SDL_SetColorKey( image, SDL_SRCCOLORKEY, key );
    };

    // Determine the correct power of two greater than or equal to the original image's size
    ptex->base.binding = SDL_GL_convert_surface( ptex->base.binding, image, ptex->base.wrap_s, ptex->base.wrap_t );

    ptex->base.target = (( 1 == image->h ) && ( image->w > 1 ) ) ? GL_TEXTURE_1D : GL_TEXTURE_2D;
    ptex->base.height = powerOfTwo( image->h );
    ptex->base.width  = powerOfTwo( image->w );

    // Set up some parameters for the format of the oglx_texture_t
    ptex->base_valid = btrue;
    ptex->surface    = image;
    ptex->imgW       = image->w;
    ptex->imgH       = image->h;
    ptex->alpha      = image->format->alpha / 255.0f;
    strncpy( ptex->name, "SDL_Surface()", SDL_arraysize( ptex->name ) );

    //// use the following command to grab every possible texture attribute in OpenGL v1.4 for
    //// this texture. Useful for debugging
    // ptex->base_valid = bfalse;
    //oglx_grab_texture_state( tx_target, 0, ptex );

    return ptex->base.binding;
}

//--------------------------------------------------------------------------------------------
GLuint oglx_texture_Load( oglx_texture_t *ptex, const char *filename, Uint32 key )
{
    GLuint retval;
    SDL_Surface * image;

    if ( VALID_TEXTURE( ptex ) )
    {
        // release any old texture
        oglx_texture_Release( ptex );
    }
    else
    {
        // clean out any uninitialied data
        ptex = oglx_texture_ctor( ptex );
        if ( NULL == ptex ) return INVALID_GL_ID;
    }

    image = IMG_Load( filename );
    if ( NULL == image ) return INVALID_GL_ID;

    retval = oglx_texture_Convert( ptex, image, key );

    if ( !VALID_BINDING( retval ) )
    {
        oglx_texture_dtor( ptex );
    }
    else
    {
        strncpy( ptex->name, filename, SDL_arraysize( ptex->name ) );

        ptex->base.wrap_s = GL_REPEAT;
        ptex->base.wrap_t = GL_REPEAT;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
GLuint  oglx_texture_GetTextureID( oglx_texture_t *texture )
{
    return ( NULL == texture ) ? INVALID_GL_ID : texture->base.binding;
}

//--------------------------------------------------------------------------------------------
GLsizei  oglx_texture_GetImageHeight( oglx_texture_t *texture )
{
    return ( NULL == texture ) ? 0 : texture->imgH;
}

//--------------------------------------------------------------------------------------------
GLsizei  oglx_texture_GetImageWidth( oglx_texture_t *texture )
{
    return ( NULL == texture ) ? 0 : texture->imgW;
}

//--------------------------------------------------------------------------------------------
GLfloat  oglx_texture_GetTextureWidth( oglx_texture_t *texture )
{
    return ( NULL == texture ) ? 0.0f : texture->base.width;
}

//--------------------------------------------------------------------------------------------
GLfloat  oglx_texture_GetTextureHeight( oglx_texture_t *texture )
{
    return ( NULL == texture ) ? 0.0f : texture->base.height;
}

//--------------------------------------------------------------------------------------------
void  oglx_texture_SetAlpha( oglx_texture_t *texture, GLfloat alpha )
{
    if ( NULL != texture )
    {
        texture->alpha = alpha;
    }
}

//--------------------------------------------------------------------------------------------
GLfloat  oglx_texture_GetAlpha( oglx_texture_t *texture )
{
    return ( NULL == texture ) ? 0 : texture->alpha;
}

//--------------------------------------------------------------------------------------------
void  oglx_texture_Release( oglx_texture_t *texture )
{
    if ( !VALID_TEXTURE( texture ) ) return;

    // delete any existing SDL surface
    if ( NULL != texture->surface )
    {
        SDL_FreeSurface( texture->surface );
        texture->surface = NULL;
    }

    // try to get rid of any stored texture data for this texture
    GL_DEBUG( glDeleteTextures )( 1, &( texture->base.binding ) );

    if ( !ErrorImage_defined ) ErrorImage_create();

    // generate a new texture binding
    GL_DEBUG( glGenTextures )( 1, &( texture->base.binding ) );

    // Bind the error texture instead of the old texture
    ErrorImage_bind( texture->base.target, texture->base.binding );

    // Reset the other data
    texture->imgW = texture->base.width = ErrorImage_width;
    texture->imgH = texture->base.height = ErrorImage_height;
    strncpy( texture->name, "ErrorImage", sizeof( texture->name ) );

    // set the image to be repeat in s and t
    texture->base.wrap_s = GL_REPEAT;
    texture->base.wrap_t = GL_REPEAT;

    //// use the following command to grab every possible texture attribute in OpenGL v1.4 for
    //// this texture. Useful for debugging
    //oglx_grab_texture_state( GL_TEXTURE_2D, 0, texture );
}

//--------------------------------------------------------------------------------------------
void oglx_texture_Bind( oglx_texture_t *texture )
{
    /// @details BB@> a oglx_texture_t wrapper for oglx_bind_default() function

    GLenum target;
    GLuint id;
    GLint wrap_s, wrap_t;

    // assume the texture is going to be the error texture
    target = GL_TEXTURE_2D;
    wrap_s = wrap_t = GL_REPEAT;
    id     = ErrorImage_binding;

    if ( NULL == texture )
    {
        // NULL texture means white blob
        id = INVALID_GL_ID;
    }
    else if ( VALID_TEXTURE( texture ) && VALID_BINDING( texture->base.binding ) )
    {
        // grab the info from the texture
        target = texture->base.target;
        id     = texture->base.binding;
        wrap_s = texture->base.wrap_s;
        wrap_t = texture->base.wrap_t;
    }

    // upload the texture
    id = oglx_bind_default( id, target, wrap_s, wrap_t );

    // if the texture binding changed, upload the change.
    if ( VALID_TEXTURE( texture ) )
    {
        texture->base.binding = id;
    }

    // use the following command to grab every possible texture attribute in OpenGL v1.4 for
    // this texture. Useful for debugging
    //if ( NULL != texture && !texture->base_valid )
    //{
    //    oglx_grab_texture_state( target, 0, texture );
    //}
}

//--------------------------------------------------------------------------------------------
GLboolean oglx_texture_valid( oglx_texture_t *ptex )
{
    return VALID_TEXTURE( ptex );
}

//--------------------------------------------------------------------------------------------
void oglx_grab_texture_state( GLenum target, GLint level, oglx_texture_t * texture )
{
    if ( NULL == texture ) return;

    gl_grab_texture_state( target, level, &( texture->base ) );

    texture->base_valid = GL_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GLuint oglx_bind_default( GLuint binding, GLenum target, GLint wrap_s, GLint wrap_t )
{
    int    filt_type, anisotropy;

    GLuint local_binding;

    // make sure that the texture mapping is turned on
    GL_DEBUG( glEnable ) ( target );

    // make sure the error texture exists
    if ( !ErrorImage_defined ) ErrorImage_create();

    local_binding = VALID_BINDING( binding ) ? binding : ErrorImage_binding;

    // handle default parameters
    if ( wrap_s < 0 ) wrap_s = GL_REPEAT;
    if ( wrap_t < 0 ) wrap_t = GL_REPEAT;


    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT, GL_TEXTURE_BIT )
    {
        // enable textre mapping
        GL_DEBUG( glEnable )( target );

        // bind the texture and set the wrapping
        GL_DEBUG( glBindTexture )( target, local_binding );

        // set some other parameters using the settings in tex_params
        oglx_default_filtering( target );

        oglx_set_wrapping( target, wrap_s, wrap_t );

    }
    ATTRIB_POP( __FUNCTION__ );

    return local_binding;
}

//--------------------------------------------------------------------------------------------
void oglx_default_filtering( GLenum target )
{
    int    filt_type, anisotropy;

    // make sure that the texture mapping is turned on
    GL_DEBUG( glEnable ) ( target );

    // grab the filter parameters from tex_params
    filt_type  = tex_params.texturefilter;
    anisotropy = tex_params.userAnisotropy;

    // map these values to ones that opengl can use
    if ( filt_type >= TX_ANISOTROPIC )
    {
        // Anisotropic filtered!
        oglx_set_filtering( target, GL_LINEAR, GL_LINEAR, anisotropy );
    }
    else
    {
        switch ( filt_type )
        {
            // Unfiltered
        case TX_UNFILTERED:
            oglx_set_filtering( target, GL_NEAREST, GL_NEAREST, 0 );
            break;

            // Linear filtered
        case TX_LINEAR:
            oglx_set_filtering( target, GL_LINEAR, GL_LINEAR, 0 );
            break;

            // Bilinear interpolation
        case TX_MIPMAP:
            oglx_set_filtering( target, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR, 0 );
            break;

            // Bilinear interpolation
        case TX_BILINEAR:
            oglx_set_filtering( target, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0 );
            break;

            // Trilinear filtered (quality 1)
        case TX_TRILINEAR_1:
            oglx_set_filtering( target, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR, 0 );
            break;

            // Trilinear filtered (quality 2)
        case TX_TRILINEAR_2:
            oglx_set_filtering( target, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0 );
            break;
        };
    }
}

