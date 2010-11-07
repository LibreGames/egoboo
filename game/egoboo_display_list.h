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

/// \file egoboo_display_list.h
/// \brief Definitions for our simplistic implementation of a display list
/// \todo  add an interface for OpenGL display lists?

#include <SDL_ttf.h>

#include "egoboo_typedef.h"
#include "extensions/ogl_include.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------

/// forward declaration for that encapsulates OpenGL data for the display list

    struct display_item;
    typedef struct display_item display_item_t;

    display_item_t * display_item_new();
    display_item_t * display_item_ctor( display_item_t * );
    display_item_t * display_item_free( display_item_t * ptr, const GLboolean owner );
    display_item_t * display_item_validate_texture( display_item_t * item_ptr );
    display_item_t * display_item_validate_list( display_item_t * item_ptr );
    frect_t        * display_item_prect( display_item_t * ptr );
    GLfloat        * display_item_texCoords( display_item_t * );
    GLuint           display_item_texture_name( display_item_t * );
    GLuint           display_item_list_name( display_item_t * );
    GLboolean        display_item_set_pos( display_item_t *, const GLint x, const GLint y );
    GLboolean        display_item_set_bound( display_item_t *, const GLint x, const GLint y, const GLint w, const GLint h );
    GLboolean        display_item_adjust_bound( display_item_t *, const GLfloat dx, const GLfloat dy );

/// Tell the display_item that it does not own its resources
    GLboolean       display_item_release_ownership( display_item_t * );

/// Set the texture that the display_item is using
    GLuint display_item_set_texture( display_item_t *, const GLuint tex_name, const GLboolean owner );

/// Set the OGL display list that the display_item is using
    GLuint display_item_set_list( display_item_t *, const GLuint list_name, const GLboolean owner );

/// Draw a display_item_t directly to the screen
    GLboolean       display_item_draw( display_item_t * );

//--------------------------------------------------------------------------------------------

/// forward declaration for an opaque struct implementing an OpenGL-like display list
    struct display_list;
    typedef struct display_list display_list_t;

    display_list_t * display_list_ctor( display_list_t *, const GLsizei count );
    display_list_t * display_list_dtor( display_list_t *, const GLboolean owner );
    display_list_t * display_list_dealloc( display_list_t * );

    GLsizei          display_list_size( display_list_t * );
    GLsizei          display_list_used( display_list_t * );
    display_item_t * display_list_get( display_list_t *, const GLsizei index );
    GLboolean        display_list_pbound( display_list_t *, frect_t * ptmp );
    GLboolean        display_list_adjust_bound( display_list_t *, const GLfloat dx, const GLfloat dy );
    display_item_t * display_list_append( display_list_t *, display_item_t * pitem );

/// Draw the elements of a display_list_t directly to the screen
    GLint            display_list_draw( display_list_t * tx_lst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#define _egoboo_display_list_h

