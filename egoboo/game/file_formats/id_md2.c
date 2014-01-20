//********************************************************************************************
//*
//*    This file is part of id_md2 reader library.
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

/// @file file_formats/id_md2.c
/// @brief A raw reader and writer for ID software's .md2 model type
/// @details

#include "id_md2.h"
#include "egoboo_vfs.h"

#include <stdlib.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/* Table of precalculated normals */
float kid_md2_normals[MD2_MAX_NORMALS][3] =
{
#    include "id_normals.inl"
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Load an MD2 model from file.
///
/// @note MD2 format stores model's data in little-endian ordering.  On
/// big-endian machines, you'll have to perform proper conversions.

id_md2_model_t * id_md2_load( const char *filename, id_md2_model_t * mdl )
{
    vfs_FILE *fp;
    int i;

    fp = vfs_openRead( filename );
    if ( !fp )
    {
        fprintf( stderr, "Error: couldn't open \"%s\"!\n", filename );
        return 0;
    }

    if ( NULL == mdl )
    {
        mdl = ( id_md2_model_t* )calloc( 1, sizeof( id_md2_model_t ) );
    }

    if ( NULL == mdl ) return NULL;

    /* Read header */
    vfs_read( &mdl->header, 1, sizeof( id_md2_header_t ), fp );

    if (( mdl->header.ident != MD2_MAGIC_NUMBER ) ||
        ( mdl->header.version != MD2_VERSION ) )
    {
        /* Error! */
        fprintf( stderr, "Error: bad version or identifier\n" );
        vfs_close( fp );
        return 0;
    }

    /* Memory allocations */
    mdl->skins     = ( id_md2_skin_t     * ) calloc( mdl->header.num_skins,   sizeof( id_md2_skin_t ) );
    mdl->texcoords = ( id_md2_texcoord_t * ) calloc( mdl->header.num_st,      sizeof( id_md2_texcoord_t ) );
    mdl->triangles = ( id_md2_triangle_t * ) calloc( mdl->header.num_tris,    sizeof( id_md2_triangle_t ) );
    mdl->frames    = ( id_md2_frame_t    * ) calloc( mdl->header.num_frames,  sizeof( id_md2_frame_t ) );
    mdl->glcmds    = ( int               * ) calloc( mdl->header.size_glcmds, sizeof( int ) );

    /* Read model data */
    vfs_seek( fp, mdl->header.offset_skins );
    vfs_read( mdl->skins, sizeof( id_md2_skin_t ), mdl->header.num_skins, fp );

    vfs_seek( fp, mdl->header.offset_st );
    vfs_read( mdl->texcoords, sizeof( id_md2_texcoord_t ), mdl->header.num_st, fp );

    vfs_seek( fp, mdl->header.offset_tris );
    vfs_read( mdl->triangles, sizeof( id_md2_triangle_t ), mdl->header.num_tris, fp );

    vfs_seek( fp, mdl->header.offset_glcmds );
    vfs_read( mdl->glcmds, sizeof( int ), mdl->header.size_glcmds, fp );

    /* Read frames */
    vfs_seek( fp, mdl->header.offset_frames );
    for ( i = 0; i < mdl->header.num_frames; ++i )
    {
        /* Memory allocation for vertices of this frame */
        mdl->frames[i].verts = ( id_md2_vertex_t * )calloc( mdl->header.num_vertices, sizeof( id_md2_vertex_t ) );

        /* Read frame data */
        vfs_read( mdl->frames[i].scale, sizeof( float ), 3, fp );
        vfs_read( mdl->frames[i].translate, sizeof( float ), 3, fp );
        vfs_read( mdl->frames[i].name, sizeof( char ), 16, fp );
        vfs_read( mdl->frames[i].verts, sizeof( id_md2_vertex_t ), mdl->header.num_vertices, fp );
    }

    vfs_close( fp );
    return mdl;
}

//--------------------------------------------------------------------------------------------
// Free resources allocated for the model.

void id_md2_free( id_md2_model_t * mdl )
{
    int i;

    if ( mdl->skins )
    {
        free( mdl->skins );
        mdl->skins = NULL;
    }

    if ( mdl->texcoords )
    {
        free( mdl->texcoords );
        mdl->texcoords = NULL;
    }

    if ( mdl->triangles )
    {
        free( mdl->triangles );
        mdl->triangles = NULL;
    }

    if ( mdl->glcmds )
    {
        free( mdl->glcmds );
        mdl->glcmds = NULL;
    }

    if ( mdl->frames )
    {
        for ( i = 0; i < mdl->header.num_frames; ++i )
        {
            free( mdl->frames[i].verts );
            mdl->frames[i].verts = NULL;
        }

        free( mdl->frames );
        mdl->frames = NULL;
    }
}
