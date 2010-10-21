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

/// @file md2.h
/// @brief Md2 Model display routines
/// @details Adapted from "Tactics - MD2_Model.h" by Jonathan Fischer
///   A class for loading/using Quake 2 and Egoboo md2 models.
///   Creating/destroying objects of this class is done in the same fashion as
///   Textures, so see Texture.h for details.
/// @note You will routinely include "md2.h" only in headers (*.h) files where you need to declare an
///       struct defined in this file. In *.inl files or *.c/*.cpp files you will routinely include "md2.inl", instead.

#include "id_md2.h"

#include "egoboo_typedef.h"
#include "physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define EGO_NORMAL_COUNT  (MD2_MAX_NORMALS + 1)
#define EGO_AMBIENT_INDEX  MD2_MAX_NORMALS

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_MD2_Vertex
{
    fvec3_t pos;
    fvec3_t nrm;
    int     normal;  ///< index to id-normal array

    ego_MD2_Vertex() { clear(this); }

private:

    static ego_MD2_Vertex * clear( ego_MD2_Vertex * ptr )
    {
        if ( NULL == ptr ) return NULL;

        memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
struct ego_MD2_TexCoord
{
    fvec2_t tex;

    ego_MD2_TexCoord() { clear(this); }

private:

    static ego_MD2_TexCoord * clear( ego_MD2_TexCoord * ptr )
    {
        if ( NULL == ptr ) return NULL;

        memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
struct ego_MD2_Frame
{
    friend struct ego_MD2_Model;

    char          name[16];

    size_t        vertex_count;
    ego_MD2_Vertex *vertex_lst;

    ego_oct_bb        bb;             ///< axis-aligned octagonal bounding box limits
    int           framelip;       ///< the position in the current animation
    BIT_FIELD     framefx;        ///< the special effects associated with this frame

    ego_MD2_Frame();
    ego_MD2_Frame( size_t size );
    ~ego_MD2_Frame();

protected:
    static ego_MD2_Frame * ctor( ego_MD2_Frame * pframe, size_t size );
    static ego_MD2_Frame * dtor( ego_MD2_Frame * pframe );

private:
    static ego_MD2_Frame * clear( ego_MD2_Frame * pframe );
};

//--------------------------------------------------------------------------------------------
struct ego_MD2_Triangle : public id_md2_triangle_t {};

//--------------------------------------------------------------------------------------------
struct ego_MD2_SkinName : public id_md2_skin_t {};

//--------------------------------------------------------------------------------------------
struct ego_MD2_GLCommand
{
    friend struct ego_MD2_Model;

    ego_MD2_GLCommand * next;

    GLenum              gl_mode;
    signed int          command_count;
    id_glcmd_packed_t * data;

    ego_MD2_GLCommand();
    ~ego_MD2_GLCommand();

    static ego_MD2_GLCommand * create( void );
    static ego_MD2_GLCommand * new_vector( int n );
    static void                destroy( ego_MD2_GLCommand ** m );
    static void                delete_vector( ego_MD2_GLCommand * v, int n );

protected:
    static ego_MD2_GLCommand * ctor( ego_MD2_GLCommand * m );
    static ego_MD2_GLCommand * dtor( ego_MD2_GLCommand * m );
    static ego_MD2_GLCommand * delete_list( ego_MD2_GLCommand * command_ptr, int command_count );

private:
    static ego_MD2_GLCommand * clear( ego_MD2_GLCommand * m );
};

//--------------------------------------------------------------------------------------------
struct ego_MD2_Model
{
    int m_numVertices;
    int m_numTexCoords;
    int m_numTriangles;
    int m_numSkins;
    int m_numFrames;
    int m_numCommands;

    ego_MD2_SkinName  *m_skins;
    ego_MD2_TexCoord  *m_texCoords;
    ego_MD2_Triangle  *m_triangles;
    ego_MD2_Frame     *m_frames;
    ego_MD2_GLCommand *m_commands;

    static float kNormals[EGO_NORMAL_COUNT][3];

    // CTORS
    static ego_MD2_Model * create( void );
    static void            destroy( ego_MD2_Model ** m );

    static ego_MD2_Model * new_vector( int n );
    static void            delete_vector( ego_MD2_Model * v, int n );

    // Other functions
    static ego_MD2_Model * load( const char * szFilename, ego_MD2_Model* m );
    static void            dealloc( ego_MD2_Model * m );
    static void            scale( ego_MD2_Model * pmd2, float scale_x, float scale_y, float scale_z );

protected:

    static ego_MD2_Model * ctor( ego_MD2_Model * m );
    static ego_MD2_Model * dtor( ego_MD2_Model * m );

private:
    // by making this private, the only way to create it is through the create and destroy methods
    ego_MD2_Model() { ctor( this ); }
    ~ego_MD2_Model() { dtor( this ); }

    static ego_MD2_Model * clear( ego_MD2_Model * ptr );
};

#define _md2_h

