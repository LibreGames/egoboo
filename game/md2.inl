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
/// \file md2.inl
/// \brief Implementation Egoboo's Md2 loading and saving
/// \details functions that will be declared inside the base class
/// \note You will routinely include "md2.inl" in all *.inl files or *.c/*.cpp files, instead of "md2.h"

#include "egoboo_typedef_cpp.inl"

#include "md2.h"
#include "file_formats/id_md2.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE const ego_MD2_SkinName  *md2_get_Skin( ego_MD2_Model * m, int index );
static INLINE const ego_MD2_Frame     *md2_get_Frame( ego_MD2_Model * m, int index );
static INLINE const ego_MD2_Triangle  *md2_get_Triangle( ego_MD2_Model * m, int index );

static INLINE int md2_get_numVertices( ego_MD2_Model * m );
static INLINE int md2_get_numTexCoords( ego_MD2_Model * m );
static INLINE int md2_get_numTriangles( ego_MD2_Model * m );
static INLINE int md2_get_numSkins( ego_MD2_Model * m );
static INLINE int md2_get_numFrames( ego_MD2_Model * m );
static INLINE int md2_get_numCommands( ego_MD2_Model * m );

static INLINE ego_MD2_SkinName  *md2_get_SkinNames( ego_MD2_Model * m );
static INLINE ego_MD2_TexCoord  *md2_get_TexCoords( ego_MD2_Model * m );
static INLINE ego_MD2_Triangle  *md2_get_Triangles( ego_MD2_Model * m );
static INLINE ego_MD2_Frame     *md2_get_Frames( ego_MD2_Model * m );
static INLINE ego_MD2_GLCommand *md2_get_Commands( ego_MD2_Model * m );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE int md2_get_numVertices( ego_MD2_Model * m )  { return m->m_numVertices;  }
INLINE int md2_get_numTexCoords( ego_MD2_Model * m ) { return m->m_numTexCoords; }
INLINE int md2_get_numTriangles( ego_MD2_Model * m ) { return m->m_numTriangles; }
INLINE int md2_get_numSkins( ego_MD2_Model * m )     { return m->m_numSkins;     }
INLINE int md2_get_numFrames( ego_MD2_Model * m )    { return m->m_numFrames;    }
INLINE int md2_get_numCommands( ego_MD2_Model * m )  { return m->m_numCommands;  }

INLINE ego_MD2_SkinName  *md2_get_SkinNames( ego_MD2_Model * m ) { return m->m_skins;     }
INLINE ego_MD2_TexCoord  *md2_get_TexCoords( ego_MD2_Model * m ) { return m->m_texCoords; }
INLINE ego_MD2_Triangle  *md2_get_Triangles( ego_MD2_Model * m ) { return m->m_triangles; }
INLINE ego_MD2_Frame     *md2_get_Frames( ego_MD2_Model * m ) { return m->m_frames;    }
INLINE ego_MD2_GLCommand *md2_get_Commands( ego_MD2_Model * m ) { return m->m_commands;  }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const ego_MD2_SkinName *md2_get_Skin( ego_MD2_Model * m, int index )
{
    if ( index >= 0 && index < m->m_numSkins )
    {
        return m->m_skins + index;
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------
INLINE const ego_MD2_Frame *md2_get_Frame( ego_MD2_Model * m, int index )
{
    if ( index >= 0 && index < m->m_numFrames )
    {
        return m->m_frames + index;
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------
INLINE const ego_MD2_Triangle  *md2_get_Triangle( ego_MD2_Model * m, int index )
{
    if ( index >= 0 && index < m->m_numTriangles )
    {
        return m->m_triangles + index;
    }
    return NULL;
}

