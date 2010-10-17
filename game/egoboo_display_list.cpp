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

/// @file egoboo_display_list.c
/// @brief A simplistic implementation of a display list
/// @details

#include "egoboo_display_list.h"
#include "egoboo_typedef.h"

#include "ogl_debug.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define DEFAULT_DISPLAY_ITEMS 20

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct display_item
{
    // SDL stuff
    frect_t      bound;
    frect_t      clip;

    // OpenGL stuff

    // a simple texture with a bound
    GLboolean   own_texture;
    GLuint      texture_name;
    GLXvector4f texCoords;

    // the index of a display list
    GLboolean   own_list;
    GLuint      list_name;

    display_item();
    ~display_item();

    static display_item * clear( display_item * ptr )
    {
        if ( NULL == ptr ) return NULL;

        memset( ptr, 0, sizeof( *ptr ) );

        ptr->texture_name = INVALID_GL_ID;
        ptr->list_name    = INVALID_GL_ID;

        return ptr;
    }
};

static GLboolean _display_item_invalidate_texture( display_item_t * item_ptr );
static GLboolean _display_item_invalidate_list( display_item_t * item_ptr );

//--------------------------------------------------------------------------------------------
display_item::display_item()
{
    display_item::clear( this );
}

//--------------------------------------------------------------------------------------------
display_item::~display_item()
{
    display_item_free( this, GL_FALSE );

    display_item::clear( this );
}

//--------------------------------------------------------------------------------------------
GLboolean _display_item_invalidate_texture( display_item_t * item_ptr )
{
    /// BB@> if we own a texture, get rid of it

    GLboolean retval = GL_FALSE;

    if ( NULL == item_ptr ) return GL_FALSE;

    if ( item_ptr->own_texture )
    {
        if ( INVALID_GL_ID != item_ptr->texture_name )
        {
            GL_DEBUG( glDeleteTextures )( 1, &( item_ptr->texture_name ) );
            retval = GL_TRUE;
        }

        item_ptr->texture_name     = INVALID_GL_ID;
        item_ptr->own_texture = GL_FALSE;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
GLboolean _display_item_invalidate_list( display_item_t * item_ptr )
{
    /// BB@> if we own a display list, get rid of it

    GLboolean retval = GL_FALSE;

    if ( NULL == item_ptr ) return GL_FALSE;

    if ( item_ptr->own_texture )
    {
        if ( INVALID_GL_ID != item_ptr->texture_name )
        {
            GL_DEBUG( glDeleteLists )( item_ptr->list_name, 1 );
            retval = GL_TRUE;
        }

        item_ptr->texture_name     = INVALID_GL_ID;
        item_ptr->own_texture = GL_FALSE;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
display_item_t *display_item_new()
{
    return EGOBOO_NEW( display_item_t );
}

//--------------------------------------------------------------------------------------------
display_item_t * display_item_free( display_item_t * item_ptr, GLboolean owner )
{
    if ( NULL == item_ptr ) return NULL;

    // get rid of any texture
    _display_item_invalidate_texture( item_ptr );

    // get rid of any list
    _display_item_invalidate_list( item_ptr );

    // is this owned by the caller?
    if ( !owner )
    {
        // NO. re-initialize the structure's values
        memset( item_ptr, 0, sizeof( *item_ptr ) );
        item_ptr->texture_name = INVALID_GL_ID;
        item_ptr->list_name    = INVALID_GL_ID;
    }
    else
    {
        // YES. then just delete the pointer, entirely
        EGOBOO_DELETE( item_ptr );
    }

    return item_ptr;
}

//--------------------------------------------------------------------------------------------
GLboolean display_item_release_ownership( display_item_t * item_ptr )
{
    GLboolean retval = GL_FALSE;

    if ( NULL == item_ptr ) return GL_FALSE;

    // will it accomplish anything?
    if ( item_ptr->own_texture && INVALID_GL_ID != item_ptr->texture_name )
    {
        retval = GL_TRUE;
    }

    // release the texture
    item_ptr->own_texture = GL_FALSE;

    // will it accomplish anything?
    if ( item_ptr->own_list && INVALID_GL_ID != item_ptr->list_name )
    {
        retval = GL_TRUE;
    }

    // release the list
    item_ptr->own_list = GL_FALSE;

    return retval;
}

//--------------------------------------------------------------------------------------------
display_item_t * display_item_validate_texture( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return item_ptr;

    _display_item_invalidate_list( item_ptr );

    if ( INVALID_GL_ID == item_ptr->texture_name )
    {
        GL_DEBUG( glGenTextures )( 1, &( item_ptr->texture_name ) );
        item_ptr->own_texture = GL_TRUE;
    }

    return item_ptr;
}

//--------------------------------------------------------------------------------------------
display_item_t * display_item_validate_list( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return item_ptr;

    _display_item_invalidate_texture( item_ptr );

    if ( INVALID_GL_ID == item_ptr->list_name )
    {
        item_ptr->list_name = GL_DEBUG( glGenLists )( 1 );
        item_ptr->own_list = GL_TRUE;
    }

    return item_ptr;
}

//--------------------------------------------------------------------------------------------
frect_t * display_item_prect( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return NULL;

    return &( item_ptr->bound );
}

//--------------------------------------------------------------------------------------------
GLfloat * display_item_texCoords( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return NULL;

    return item_ptr->texCoords;
}

//--------------------------------------------------------------------------------------------
GLuint display_item_texture_name( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return INVALID_GL_ID;

    return item_ptr->texture_name;
}

//--------------------------------------------------------------------------------------------
GLuint display_item_list_name( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return INVALID_GL_ID;

    return item_ptr->list_name;
}

//--------------------------------------------------------------------------------------------
GLboolean display_item_set_pos( display_item_t * item_ptr, GLint x, GLint y )
{
    if ( NULL == item_ptr ) return GL_FALSE;

    item_ptr->bound.x = x;
    item_ptr->bound.y = y;

    return GL_TRUE;
}

//--------------------------------------------------------------------------------------------
GLboolean display_item_set_bound( display_item_t * item_ptr, GLint x, GLint y, GLint w, GLint h )
{
    if ( NULL == item_ptr ) return GL_FALSE;

    item_ptr->bound.x = x;
    item_ptr->bound.y = y;
    item_ptr->bound.w = w;
    item_ptr->bound.h = h;

    return GL_TRUE;
}

//--------------------------------------------------------------------------------------------
GLuint display_item_set_texture( display_item_t * item_ptr, GLuint tex_name, GLboolean owner )
{
    if ( NULL == item_ptr ) return GL_FALSE;

    // if we own a display list, get rid of it
    _display_item_invalidate_list( item_ptr );

    // set the texture id
    item_ptr->texture_name = tex_name;

    // set the texture ownership
    if ( owner )
    {
        // move the ownership to this display item
        item_ptr->own_texture = GL_TRUE;
        tex_name = INVALID_GL_ID;
    }
    else
    {
        // let the caller retain ownership
        item_ptr->own_texture = GL_FALSE;
    }

    return tex_name;
}

//--------------------------------------------------------------------------------------------
GLuint display_item_set_list( display_item_t * item_ptr, GLuint list_name, GLboolean owner )
{
    if ( NULL == item_ptr ) return GL_FALSE;

    // if we own a texture, get rid of it
    _display_item_invalidate_texture( item_ptr );

    // set the texture id
    item_ptr->list_name = list_name;

    // set the list ownership
    if ( owner )
    {
        // move the ownership to this display item
        item_ptr->own_list = GL_TRUE;
        list_name          = INVALID_GL_ID;
    }
    else
    {
        // let the caller retain ownership
        item_ptr->own_list = GL_FALSE;
    }

    return list_name;
}

//--------------------------------------------------------------------------------------------
GLboolean display_item_adjust_bound( display_item_t * item_ptr, GLfloat dx, GLfloat dy )
{
    if ( NULL == item_ptr ) return GL_FALSE;

    if ( 0.0f == dx && 0.0f == dy ) return GL_TRUE;

    item_ptr->bound.x += dx;
    item_ptr->bound.y += dy;

    return GL_TRUE;
}

//--------------------------------------------------------------------------------------------
GLboolean display_item_draw_texture( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return GL_FALSE;

    // If there is no valid texture return false.
    // @note !!!! This is not egoboo's normal behavior !!!!
    // Things like buttons and other "blank" textures are implemented
    // by binding INVALID_GL_ID as a texture
    if ( INVALID_GL_ID == item_ptr->texture_name ) return GL_FALSE;

    // bind the texture
    GL_DEBUG( glBindTexture )( GL_TEXTURE_2D, item_ptr->texture_name );

    // And draw the darn thing
    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( item_ptr->texCoords[0], item_ptr->texCoords[1] );
        GL_DEBUG( glVertex2f )( item_ptr->bound.x, item_ptr->bound.y );

        GL_DEBUG( glTexCoord2f )( item_ptr->texCoords[2], item_ptr->texCoords[1] );
        GL_DEBUG( glVertex2f )( item_ptr->bound.x + item_ptr->bound.w, item_ptr->bound.y );

        GL_DEBUG( glTexCoord2f )( item_ptr->texCoords[2], item_ptr->texCoords[3] );
        GL_DEBUG( glVertex2f )( item_ptr->bound.x + item_ptr->bound.w, item_ptr->bound.y + item_ptr->bound.h );

        GL_DEBUG( glTexCoord2f )( item_ptr->texCoords[0], item_ptr->texCoords[3] );
        GL_DEBUG( glVertex2f )( item_ptr->bound.x, item_ptr->bound.y + item_ptr->bound.h );
    }
    GL_DEBUG_END();

    return GL_TRUE;
}

//--------------------------------------------------------------------------------------------
GLboolean display_item_draw_list( display_item_t * item_ptr )
{
    if ( NULL == item_ptr ) return GL_FALSE;

    // If there is no valid display list return false.
    if ( INVALID_GL_ID == item_ptr->list_name ) return GL_FALSE;

    GL_DEBUG( glCallList )( item_ptr->list_name );

    return GL_TRUE;
}

//--------------------------------------------------------------------------------------------
GLboolean display_item_draw( display_item_t * item_ptr )
{
    GLboolean retval = GL_FALSE;

    if ( NULL == item_ptr ) return GL_FALSE;

    if ( INVALID_GL_ID == item_ptr->texture_name && INVALID_GL_ID != item_ptr->list_name )
    {
        // clearly, a display list item
        retval = display_item_draw_list( item_ptr );
    }
    else if ( INVALID_GL_ID != item_ptr->texture_name && INVALID_GL_ID == item_ptr->list_name )
    {
        // clearly, a texture item
        retval = display_item_draw_texture( item_ptr );
    }
    else
    {
        // no idea what this is
        retval = GL_FALSE;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct display_list
{
    frect_t          bound;
    frect_t          clip;

    GLsizei           size;
    GLsizei           used;
    display_item_t ** ary;

    display_list( GLsizei count = 0 );
    ~display_list();

    static display_list * clear( display_list * ptr )
    {
        if ( NULL == ptr ) return NULL;

        memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }

};

//--------------------------------------------------------------------------------------------
display_list::display_list( GLsizei count )
{
    display_list::clear( this );

    if ( 0 != count )
    {
        display_list_ctor( this, count );
    }
}

//--------------------------------------------------------------------------------------------
display_list::~display_list()
{
    display_list_dtor( this, GL_FALSE );
}

//--------------------------------------------------------------------------------------------
display_list_t * display_list_ctor( display_list_t * list_ptr, GLsizei count )
{
    GLsizei cnt;
    display_list_t * ret = list_ptr;

    // if there is no list_ptr, create one
    if ( NULL == ret )
    {
        ret = EGOBOO_NEW( display_list_t );
    }

    // clear the data
    memset( ret, 0, sizeof( display_list_t ) );

    // if there is no count, return the empty struct
    if ( 0 == count ) return ret;

    // allocate the ary
    ret->ary = EGOBOO_NEW_ARY( display_item_t * , count );

    // initialize the elements of the array
    if ( NULL != ret->ary )
    {
        for ( cnt = 0; cnt < count; cnt++ )
        {
            ret->ary[cnt] = NULL;
        }
        ret->size = count;
        ret->used = 0;
    }

    return ret;
}

//--------------------------------------------------------------------------------------------
display_list_t * display_list_dtor( display_list_t * list_ptr, GLboolean owner )
{
    // short circuit a bad list_ptr pointer
    if ( NULL == list_ptr ) return list_ptr;

    // clear out any data in tha list_ptr
    list_ptr = display_list_dealloc( list_ptr );

    // remove the ary
    EGOBOO_DELETE_ARY( list_ptr->ary );

    // clear all the variables
    display_list::clear( list_ptr );

    // if the caller this list_ptr, delete the struct completely
    if ( owner )
    {
        EGOBOO_DELETE( list_ptr );
    }

    return list_ptr;
}

//--------------------------------------------------------------------------------------------
display_list_t * display_list_dealloc( display_list_t * list_ptr )
{
    GLsizei cnt;

    if ( NULL == list_ptr ) return list_ptr;

    // if the list_ptr is not empty, delete the elements
    if ( NULL != list_ptr->ary )
    {
        // remove the "used" elements of the ary
        for ( cnt = 0; cnt < list_ptr->used; cnt++ )
        {
            list_ptr->ary[cnt] = display_item_free( list_ptr->ary[cnt], GL_TRUE );
        }

        // remove any extra elements of the array
        for ( /* nothing */; cnt < list_ptr->size; cnt++ )
        {
            list_ptr->ary[cnt] = display_item_free( list_ptr->ary[cnt], GL_TRUE );
        }
    }

    // there are now no used elements in the ary
    // the ary itself and the size are not affected
    list_ptr->used = 0;

    return list_ptr;
}

//--------------------------------------------------------------------------------------------
GLsizei display_list_size( display_list_t * list_ptr )
{
    if ( NULL == list_ptr ) return 0;

    return list_ptr->size;
}

//--------------------------------------------------------------------------------------------
GLsizei display_list_used( display_list_t * list_ptr )
{
    if ( NULL == list_ptr ) return 0;

    return list_ptr->used;
}

//--------------------------------------------------------------------------------------------
display_item_t * display_list_get( display_list_t * list_ptr, GLsizei index )
{
    if ( NULL == list_ptr ) return NULL;

    if ( index >= list_ptr->size || index >= list_ptr->used ) return NULL;

    return list_ptr->ary[index];
}

//--------------------------------------------------------------------------------------------
GLboolean display_list_pbound( display_list_t * list_ptr, frect_t * ptmp )
{
    GLsizei          index;
    display_item_t * item_ptr;
    frect_t        * prect;
    GLboolean        found = GL_FALSE;

    if ( NULL == list_ptr || NULL == ptmp ) return GL_FALSE;

    if ( 0 == list_ptr->used ) return GL_FALSE;

    // find the first non-trivial rect
    found = GL_FALSE;
    for ( index = 0; index < list_ptr->used && !found; index++ )
    {
        item_ptr = display_list_get( list_ptr, index );
        if ( NULL == item_ptr ) continue;

        prect = display_item_prect( item_ptr );
        if ( NULL == prect ) continue;

        list_ptr->bound = *prect;
        found = GL_TRUE;
    }

    // find the union of all rects
    for ( /* nothing */; index < list_ptr->used; index++ )
    {
        item_ptr = display_list_get( list_ptr, index );
        if ( NULL == item_ptr ) continue;

        prect = display_item_prect( item_ptr );
        if ( NULL == prect ) continue;

        frect_union( &( list_ptr->bound ), prect, &( list_ptr->bound ) );
    }

    if ( NULL != ptmp )
    {
        *ptmp = list_ptr->bound;
    }

    return found;
}

//--------------------------------------------------------------------------------------------
GLboolean display_list_adjust_bound( display_list_t * list_ptr, GLfloat dx, GLfloat dy )
{
    GLboolean          rv = GL_FALSE;
    GLsizei          index;
    display_item_t * item_ptr;

    if ( NULL == list_ptr || NULL == list_ptr->ary || 0 == list_ptr->used ) return GL_FALSE;

    if ( 0.0f == dx && 0.0f == dy ) return GL_TRUE;

    rv = GL_FALSE;
    for ( index = 0; index < list_ptr->used; index++ )
    {
        item_ptr = display_list_get( list_ptr, index );

        if ( display_item_adjust_bound( item_ptr, dx, dy ) )
        {
            rv = GL_TRUE;
        }
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
display_item_t * display_list_append( display_list_t * list_ptr, display_item_t * pitem )
{
    if ( NULL == pitem ) return pitem;

    // if we are passed an empty list, generate a new one
    if ( NULL == list_ptr || NULL == list_ptr->ary || 0 == list_ptr->size )
    {
        list_ptr = display_list_ctor( list_ptr, DEFAULT_DISPLAY_ITEMS );
    }

    // if we get a list, then fail
    if ( NULL == list_ptr || NULL == list_ptr->ary )
    {
        return pitem;
    }

    if ( list_ptr->used < list_ptr->size )
    {
        // copy the data into the array
        list_ptr->ary[ list_ptr->used ] = pitem;
        list_ptr->used++;

        // make the pointer null to indicate that the array owns the data now
        pitem = NULL;

        // make sure we have a proper bounding rect
        display_list_pbound( list_ptr, NULL );
    }

    return pitem;
}

//--------------------------------------------------------------------------------------------
GLint display_list_draw( display_list_t *list_ptr )
{
    GLsizei loc_used, index, rendered;

    if ( NULL == list_ptr ) return GL_FALSE;

    loc_used = display_list_used( list_ptr );
    if ( 0 == loc_used ) return GL_TRUE;

    for ( rendered = 0, index = 0; index < loc_used; index++ )
    {
        display_item_t * item_ptr = display_list_get( list_ptr, index );

        if ( display_item_draw( item_ptr ) )
        {
            rendered++;
        }
    }

    return rendered;
}
