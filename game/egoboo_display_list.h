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

/// @file egoboo_display_list.h
/// @brief Definitions for our simplistic implementation of a display list
/// @todo  add an interface for OpenGL display lists?

#include <SDL_ttf.h>

#include "egoboo_typedef.h"
#include "ogl_include.h"

//--------------------------------------------------------------------------------------------

/// forward declaration for that encapsulates OpenGL data for the display list

struct display_item_t;

extern display_item_t * display_item_new();
extern display_item_t * display_item_ctor( display_item_t * );
extern display_item_t * display_item_free( display_item_t * ptr, GLboolean owner );
extern display_item_t * display_item_validate_texture( display_item_t * item_ptr );
extern display_item_t * display_item_validate_list( display_item_t * item_ptr );
extern frect_t        * display_item_prect( display_item_t * ptr );
extern GLfloat        * display_item_texCoords( display_item_t * );
extern GLuint           display_item_texture_name( display_item_t * );
extern GLuint           display_item_list_name( display_item_t * );
extern GLboolean        display_item_set_pos( display_item_t *, GLint x, GLint y );
extern GLboolean        display_item_set_bound( display_item_t *, GLint x, GLint y, GLint w, GLint h );
extern GLboolean        display_item_adjust_bound( display_item_t *, GLfloat dx, GLfloat dy );

/// Tell the display_item that it does not own its resources
extern GLboolean       display_item_release_ownership( display_item_t * );

/// Set the texture that the display_item is using
extern GLuint display_item_set_texture( display_item_t *, GLuint tex_name, GLboolean owner );

/// Set the OGL display list that the display_item is using
extern GLuint display_item_set_list( display_item_t *, GLuint list_name, GLboolean owner );

/// Draw a display_item_t directly to the screen
extern GLboolean       display_item_draw( display_item_t * );

//--------------------------------------------------------------------------------------------
/// forward declaration for an opaque struct implementing an OpenGL-like display list

struct display_list_t;

extern display_list_t * display_list_ctor( display_list_t *, GLsizei count );
extern display_list_t * display_list_dtor( display_list_t *, GLboolean owner );
extern display_list_t * display_list_clear( display_list_t * );

extern GLsizei          display_list_size( display_list_t * );
extern GLsizei          display_list_used( display_list_t * );
extern display_item_t * display_list_get( display_list_t *, GLsizei index );
extern GLboolean        display_list_pbound( display_list_t *, frect_t * ptmp );
extern GLboolean        display_list_adjust_bound( display_list_t *, GLfloat dx, GLfloat dy );
extern display_item_t * display_list_append( display_list_t *, display_item_t * pitem );
/// Draw the elements of a display_list_t directly to the screen
extern GLint            display_list_draw( display_list_t * tx_lst );

#define EGOBOO_DISPLAY_LIST_H