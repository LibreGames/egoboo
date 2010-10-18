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
/// @file md2.c
/// @brief Raw MD2 loader
/// @details Raw model loader for ID Software's MD2 file format

#include "md2.inl"

#include "log.h"

#include "egoboo_typedef.h"
#include "egoboo_math.inl"

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float ego_MD2_Model::kNormals[EGO_NORMAL_COUNT][3] =
{
#include "id_normals.inl"
    , {0, 0, 0}                     // the "equal light" normal
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_MD2_Frame::ego_MD2_Frame()              { ego_MD2_Frame::clear( this ); }
//--------------------------------------------------------------------------------------------
ego_MD2_Frame::ego_MD2_Frame( size_t size ) { ego_MD2_Frame::ctor( this, size ); }
//--------------------------------------------------------------------------------------------
ego_MD2_Frame::~ego_MD2_Frame()             { ego_MD2_Frame::dtor( this ); }

//--------------------------------------------------------------------------------------------
ego_MD2_Frame * ego_MD2_Frame::clear( ego_MD2_Frame * pframe )
{
    if ( NULL == pframe ) return pframe;

    memset( pframe, 0, sizeof( *pframe ) );

    return pframe;
}

//--------------------------------------------------------------------------------------------
ego_MD2_Frame * ego_MD2_Frame::ctor( ego_MD2_Frame * pframe, size_t size )
{
    if ( NULL == pframe ) return pframe;

    pframe = ego_MD2_Frame::clear( pframe );
    if ( NULL == pframe ) return pframe;

    pframe->vertex_lst = EGOBOO_NEW_ARY( ego_MD2_Vertex, size );
    if ( NULL != pframe->vertex_lst ) pframe->vertex_count = size;

    return pframe;
}

//--------------------------------------------------------------------------------------------
ego_MD2_Frame * ego_MD2_Frame::dtor( ego_MD2_Frame * pframe )
{
    if ( NULL == pframe ) return pframe;

    EGOBOO_DELETE_ARY( pframe->vertex_lst );
    pframe->vertex_count = 0;

    pframe = ego_MD2_Frame::clear( pframe );
    if ( NULL == pframe ) return pframe;

    return pframe;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand::ego_MD2_GLCommand() { ego_MD2_GLCommand::ctor( this ); }

//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand::~ego_MD2_GLCommand() { ego_MD2_GLCommand::dtor( this ); }

//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand * ego_MD2_GLCommand::clear( ego_MD2_GLCommand * m )
{
    if ( NULL == m ) return m;

    memset( m, 0, sizeof( *m ) );

    return m;
}

//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand * ego_MD2_GLCommand::ctor( ego_MD2_GLCommand * m )
{
    if ( NULL == m ) return m;

    m = ego_MD2_GLCommand::clear( m );
    if ( NULL == m ) return m;

    m->next = NULL;
    m->data = NULL;

    return m;
}

//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand * ego_MD2_GLCommand::dtor( ego_MD2_GLCommand * m )
{
    if ( NULL == m ) return m;

    EGOBOO_DELETE_ARY( m->data );

    m = ego_MD2_GLCommand::clear( m );
    if ( NULL == m ) return m;

    return m;
}

//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand * ego_MD2_GLCommand::create()
{
    ego_MD2_GLCommand * m;

    m = EGOBOO_NEW( ego_MD2_GLCommand );

    ego_MD2_GLCommand::ctor( m );

    return m;
}

//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand * ego_MD2_GLCommand::new_vector( int n )
{
    int i;
    ego_MD2_GLCommand * v = EGOBOO_NEW_ARY( ego_MD2_GLCommand, n );
    for ( i = 0; i < n; i++ ) ego_MD2_GLCommand::ctor( v + i );
    return v;
}

//--------------------------------------------------------------------------------------------
void ego_MD2_GLCommand::destroy( ego_MD2_GLCommand ** m )
{
    if ( NULL == m || NULL == * m ) return;

    ego_MD2_GLCommand::dtor( *m );

    EGOBOO_DELETE( *m );
}

//--------------------------------------------------------------------------------------------
ego_MD2_GLCommand * ego_MD2_GLCommand::delete_list( ego_MD2_GLCommand * command_ptr, int command_count )
{
    int cnt;

    if ( NULL == command_ptr ) return command_ptr;

    for ( cnt = 0; cnt < command_count && NULL != command_ptr; cnt++ )
    {
        ego_MD2_GLCommand * tmp = command_ptr;
        command_ptr = command_ptr->next;

        ego_MD2_GLCommand::destroy( &tmp );
    }

    return command_ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_MD2_Model * ego_MD2_Model::clear( ego_MD2_Model * ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_MD2_Model * ego_MD2_Model::ctor( ego_MD2_Model * m )
{
    if ( NULL == m ) return m;

    m = ego_MD2_Model::clear( m );
    if ( NULL == m ) return m;

    return m;
}

//--------------------------------------------------------------------------------------------
void ego_MD2_Model::dealloc( ego_MD2_Model * m )
{
    EGOBOO_DELETE_ARY( m->m_skins );
    m->m_numSkins = 0;

    EGOBOO_DELETE_ARY( m->m_texCoords );
    m->m_numTexCoords = 0;

    EGOBOO_DELETE_ARY( m->m_triangles );
    m->m_numTriangles = 0;

    if ( NULL != m->m_frames )
    {
        int i;
        for ( i = 0; i < m->m_numFrames; i++ )
        {
            ego_MD2_Frame::dtor( m->m_frames + i );
        }

        EGOBOO_DELETE_ARY( m->m_frames );
        m->m_numFrames = 0;
    }

    ego_MD2_GLCommand::delete_list( m->m_commands, m->m_numCommands );
    m->m_commands = NULL;
    m->m_numCommands = 0;
}

//--------------------------------------------------------------------------------------------
ego_MD2_Model * ego_MD2_Model::dtor( ego_MD2_Model * m )
{
    if ( NULL == m ) return NULL;

    ego_MD2_Model::dealloc( m );

    return m;
}

//--------------------------------------------------------------------------------------------
ego_MD2_Model * ego_MD2_Model::create()
{
    ego_MD2_Model * m = EGOBOO_NEW( ego_MD2_Model );

    ego_MD2_Model::ctor( m );

    return m;
}

//--------------------------------------------------------------------------------------------
ego_MD2_Model * ego_MD2_Model::new_vector( int n )
{
    int i;
    ego_MD2_Model * v = EGOBOO_NEW_ARY( ego_MD2_Model, n );
    for ( i = 0; i < n; i++ ) ego_MD2_Model::ctor( v + i );
    return v;
}

//--------------------------------------------------------------------------------------------
void ego_MD2_Model::destroy( ego_MD2_Model ** m )
{
    if ( NULL == m || NULL == *m ) return;

    ego_MD2_Model::dtor( *m );

    EGOBOO_DELETE( *m );
}

//--------------------------------------------------------------------------------------------
void ego_MD2_Model::delete_vector( ego_MD2_Model * v, int n )
{
    int i;
    if ( NULL == v || 0 == n ) return;
    for ( i = 0; i < n; i++ ) ego_MD2_Model::dtor( v + i );
    EGOBOO_DELETE( v );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ego_MD2_Model::scale( ego_MD2_Model * pmd2, float scale_x, float scale_y, float scale_z )
{
    /// @details BB@> scale every vertex in the md2 by the given amount

    int cnt, tnc;
    int num_frames, num_verts;
    ego_MD2_Frame * pframe;

    num_frames = pmd2->m_numFrames;
    num_verts  = pmd2->m_numVertices;

    for ( cnt = 0; cnt < num_frames; cnt++ )
    {
        bool_t bfound;

        pframe = pmd2->m_frames + cnt;

        bfound = bfalse;
        for ( tnc = 0; tnc  < num_verts; tnc++ )
        {
            int       cnt;
            oct_vec_t opos;

            pframe->vertex_lst[tnc].pos.x *= scale_x;
            pframe->vertex_lst[tnc].pos.y *= scale_y;
            pframe->vertex_lst[tnc].pos.z *= scale_z;

            pframe->vertex_lst[tnc].nrm.x *= SGN( scale_x );
            pframe->vertex_lst[tnc].nrm.y *= SGN( scale_y );
            pframe->vertex_lst[tnc].nrm.z *= SGN( scale_z );

            fvec3_self_normalize( pframe->vertex_lst[tnc].nrm.v );

            oct_vec_ctor( opos, pframe->vertex_lst[tnc].pos );

            // Re-calculate the bounding box for this frame
            if ( !bfound )
            {
                for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
                {
                    pframe->bb.mins[cnt] = pframe->bb.maxs[cnt] = opos[cnt];
                }

                bfound = btrue;
            }
            else
            {
                for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
                {
                    pframe->bb.mins[cnt] = MIN( pframe->bb.mins[cnt], opos[cnt] );
                    pframe->bb.maxs[cnt] = MAX( pframe->bb.maxs[cnt], opos[cnt] );
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_MD2_Model* ego_MD2_Model::load( const char * szFilename, ego_MD2_Model* mdl )
{
    FILE * f;
    int i, v;
    bool_t bfound;

    id_md2_header_t md2_header;
    ego_MD2_Model    *model;

    // Open up the file, and make sure it's a MD2 model
    f = fopen( szFilename, "rb" );
    if ( NULL == f )
    {
        log_warning( "ego_MD2_Model::load() - could not open model (%s)\n", szFilename );
        return NULL;
    }

    fread( &md2_header, sizeof( md2_header ), 1, f );

    // Convert the byte ordering in the md2_header, if we need to
    md2_header.ident            = ENDIAN_INT32( md2_header.ident );
    md2_header.version          = ENDIAN_INT32( md2_header.version );
    md2_header.skinwidth        = ENDIAN_INT32( md2_header.skinwidth );
    md2_header.skinheight       = ENDIAN_INT32( md2_header.skinheight );
    md2_header.framesize        = ENDIAN_INT32( md2_header.framesize );
    md2_header.num_skins        = ENDIAN_INT32( md2_header.num_skins );
    md2_header.num_vertices     = ENDIAN_INT32( md2_header.num_vertices );
    md2_header.num_st           = ENDIAN_INT32( md2_header.num_st );
    md2_header.num_tris         = ENDIAN_INT32( md2_header.num_tris );
    md2_header.size_glcmds      = ENDIAN_INT32( md2_header.size_glcmds );
    md2_header.num_frames       = ENDIAN_INT32( md2_header.num_frames );
    md2_header.offset_skins     = ENDIAN_INT32( md2_header.offset_skins );
    md2_header.offset_st        = ENDIAN_INT32( md2_header.offset_st );
    md2_header.offset_tris      = ENDIAN_INT32( md2_header.offset_tris );
    md2_header.offset_frames    = ENDIAN_INT32( md2_header.offset_frames );
    md2_header.offset_glcmds    = ENDIAN_INT32( md2_header.offset_glcmds );
    md2_header.offset_end       = ENDIAN_INT32( md2_header.offset_end );

    if ( md2_header.ident != MD2_MAGIC_NUMBER || md2_header.version != MD2_VERSION )
    {
        fclose( f );
        log_warning( "ego_MD2_Model::load() - model does not have valid header or identifier (%s)\n", szFilename );
        return NULL;
    }

    // Allocate a ego_MD2_Model to hold all this stuff
    model = ( NULL == mdl ) ? ego_MD2_Model::create() : mdl;
    if ( NULL == model )
    {
        log_error( "ego_MD2_Model::load() - could create ego_MD2_Model\n" );
        return NULL;
    }

    model->m_numVertices  = md2_header.num_vertices;
    model->m_numTexCoords = md2_header.num_st;
    model->m_numTriangles = md2_header.num_tris;
    model->m_numSkins     = md2_header.num_skins;
    model->m_numFrames    = md2_header.num_frames;

    model->m_texCoords = EGOBOO_NEW_ARY( ego_MD2_TexCoord, md2_header.num_st );
    model->m_triangles = EGOBOO_NEW_ARY( ego_MD2_Triangle, md2_header.num_tris );
    model->m_skins     = EGOBOO_NEW_ARY( ego_MD2_SkinName, md2_header.num_skins );
    model->m_frames    = EGOBOO_NEW_ARY( ego_MD2_Frame, md2_header.num_frames );

    for ( i = 0; i < md2_header.num_frames; i++ )
    {
        ego_MD2_Frame::ctor( model->m_frames + i, md2_header.num_vertices );
    }

    // Load the texture coordinates from the file, normalizing them as we go
    fseek( f, md2_header.offset_st, SEEK_SET );
    for ( i = 0; i < md2_header.num_st; i++ )
    {
        id_md2_texcoord_t tc;
        fread( &tc, sizeof( tc ), 1, f );

        // auto-convert the byte ordering of the texture coordinates
        tc.s = ENDIAN_INT16( tc.s );
        tc.t = ENDIAN_INT16( tc.t );

        model->m_texCoords[i].tex.s = tc.s / ( float )md2_header.skinwidth;
        model->m_texCoords[i].tex.t = tc.t / ( float )md2_header.skinheight;
    }

    // Load triangles from the file.  I use the same memory layout as the file
    // on a little endian machine, so they can just be read directly
    fseek( f, md2_header.offset_tris, SEEK_SET );
    fread( model->m_triangles, sizeof( id_md2_triangle_t ), md2_header.num_tris, f );

    // auto-convert the byte ordering on the triangles
    for ( i = 0; i < md2_header.num_tris; i++ )
    {
        for ( v = 0; v < 3; v++ )
        {
            model->m_triangles[i].vertex[v] = ENDIAN_INT16( model->m_triangles[i].vertex[v] );
            model->m_triangles[i].st[v]     = ENDIAN_INT16( model->m_triangles[i].st[v] );
        }
    }

    // Load the skin names.  Again, I can load them directly
    fseek( f, md2_header.offset_skins, SEEK_SET );
    fread( model->m_skins, sizeof( id_md2_skin_t ), md2_header.num_skins, f );

    // Load the frames of animation
    fseek( f, md2_header.offset_frames, SEEK_SET );
    for ( i = 0; i < md2_header.num_frames; i++ )
    {
        id_md2_frame_header_t frame_header;

        // read the current frame
        fread( &frame_header, sizeof( frame_header ), 1, f );

        // Convert the byte ordering on the scale & translate vectors, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
        frame_header.scale[0]     = ENDIAN_FLOAT( frame_header.scale[0] );
        frame_header.scale[1]     = ENDIAN_FLOAT( frame_header.scale[1] );
        frame_header.scale[2]     = ENDIAN_FLOAT( frame_header.scale[2] );

        frame_header.translate[0] = ENDIAN_FLOAT( frame_header.translate[0] );
        frame_header.translate[1] = ENDIAN_FLOAT( frame_header.translate[1] );
        frame_header.translate[2] = ENDIAN_FLOAT( frame_header.translate[2] );
#endif

        // unpack the md2 vertex_lst from this frame
        bfound = bfalse;
        for ( v = 0; v < md2_header.num_vertices; v++ )
        {
            ego_MD2_Frame   * pframe;
            id_md2_vertex_t frame_vert;

            // read vertex_lst one-by-one. I hope this is not endian dependent, but I have no way to check it.
            fread( &frame_vert, sizeof( id_md2_vertex_t ), 1, f );

            pframe = model->m_frames + i;

            // grab the vertex position
            pframe->vertex_lst[v].pos.x = frame_vert.v[0] * frame_header.scale[0] + frame_header.translate[0];
            pframe->vertex_lst[v].pos.y = frame_vert.v[1] * frame_header.scale[1] + frame_header.translate[1];
            pframe->vertex_lst[v].pos.z = frame_vert.v[2] * frame_header.scale[2] + frame_header.translate[2];

            // grab the normal index
            pframe->vertex_lst[v].normal = frame_vert.normalIndex;
            if ( pframe->vertex_lst[v].normal > EGO_AMBIENT_INDEX ) pframe->vertex_lst[v].normal = EGO_AMBIENT_INDEX;

            // expand the normal index into an actual normal
            pframe->vertex_lst[v].nrm.x = ego_MD2_Model::kNormals[frame_vert.normalIndex][0];
            pframe->vertex_lst[v].nrm.y = ego_MD2_Model::kNormals[frame_vert.normalIndex][1];
            pframe->vertex_lst[v].nrm.z = ego_MD2_Model::kNormals[frame_vert.normalIndex][2];

            // Calculate the bounding box for this frame
            if ( !bfound )
            {
                pframe->bb.mins[OCT_X ] = pframe->bb.maxs[OCT_X ] =  pframe->vertex_lst[v].pos.x;
                pframe->bb.mins[OCT_Y ] = pframe->bb.maxs[OCT_Y ] =  pframe->vertex_lst[v].pos.y;
                pframe->bb.mins[OCT_XY] = pframe->bb.maxs[OCT_XY] =  pframe->vertex_lst[v].pos.x + pframe->vertex_lst[v].pos.y;
                pframe->bb.mins[OCT_YX] = pframe->bb.maxs[OCT_YX] = -pframe->vertex_lst[v].pos.x + pframe->vertex_lst[v].pos.y;
                pframe->bb.mins[OCT_Z ] = pframe->bb.maxs[OCT_Z ] =  pframe->vertex_lst[v].pos.z;

                bfound = btrue;
            }
            else
            {
                pframe->bb.mins[OCT_X ] = MIN( pframe->bb.mins[OCT_X ], pframe->vertex_lst[v].pos.x );
                pframe->bb.mins[OCT_Y ] = MIN( pframe->bb.mins[OCT_Y ], pframe->vertex_lst[v].pos.y );
                pframe->bb.mins[OCT_XY] = MIN( pframe->bb.mins[OCT_XY], pframe->vertex_lst[v].pos.x + pframe->vertex_lst[v].pos.y );
                pframe->bb.mins[OCT_YX] = MIN( pframe->bb.mins[OCT_YX], -pframe->vertex_lst[v].pos.x + pframe->vertex_lst[v].pos.y );
                pframe->bb.mins[OCT_Z ] = MIN( pframe->bb.mins[OCT_Z ], pframe->vertex_lst[v].pos.z );

                pframe->bb.maxs[OCT_X ] = MAX( pframe->bb.maxs[OCT_X ], pframe->vertex_lst[v].pos.x );
                pframe->bb.maxs[OCT_Y ] = MAX( pframe->bb.maxs[OCT_Y ], pframe->vertex_lst[v].pos.y );
                pframe->bb.maxs[OCT_XY] = MAX( pframe->bb.maxs[OCT_XY], pframe->vertex_lst[v].pos.x + pframe->vertex_lst[v].pos.y );
                pframe->bb.maxs[OCT_YX] = MAX( pframe->bb.maxs[OCT_YX], -pframe->vertex_lst[v].pos.x + pframe->vertex_lst[v].pos.y );
                pframe->bb.maxs[OCT_Z ] = MAX( pframe->bb.maxs[OCT_Z ], pframe->vertex_lst[v].pos.z );
            }
        }

        // make sure to copy the frame name!
        strncpy( model->m_frames[i].name, frame_header.name, 16 );
    }

    // Load up the pre-computed OpenGL optimizations
    if ( md2_header.size_glcmds > 0 )
    {
        Uint32            cmd_cnt = 0;
        int               cmd_size;
        ego_MD2_GLCommand * cmd     = NULL;
        fseek( f, md2_header.offset_glcmds, SEEK_SET );

        // count the commands
        cmd_size = 0;
        while ( cmd_size < md2_header.size_glcmds )
        {
            Sint32 commands;

            fread( &commands, sizeof( Sint32 ), 1, f );
            cmd_size += sizeof( Sint32 ) / sizeof( Uint32 );

            // auto-convert the byte ordering
            commands = ENDIAN_INT32( commands );

            if ( 0 == commands || cmd_size == md2_header.size_glcmds ) break;

            cmd = ego_MD2_GLCommand::create();
            cmd->command_count = commands;

            // set the GL drawing mode
            if ( cmd->command_count > 0 )
            {
                cmd->gl_mode = GL_TRIANGLE_STRIP;
            }
            else
            {
                cmd->command_count = -cmd->command_count;
                cmd->gl_mode = GL_TRIANGLE_FAN;
            }

            // allocate the data
            cmd->data = EGOBOO_NEW_ARY( id_glcmd_packed_t, cmd->command_count );

            // read in the data
            fread( cmd->data, sizeof( id_glcmd_packed_t ), cmd->command_count, f );
            cmd_size += ( sizeof( id_glcmd_packed_t ) * cmd->command_count ) / sizeof( Uint32 );

            // translate the data, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
            {
                int i;
                for ( i = 0; i < cmd->command_count; i++ )
                {
                    cmd->data[i].index = SDL_swap32( cmd->data[i].s );
                    cmd->data[i].s     = ENDIAN_FLOAT( cmd->data[i].s );
                    cmd->data[i].t     = ENDIAN_FLOAT( cmd->data[i].t );
                };
            }
#endif

            // attach it to the command list
            cmd->next         = model->m_commands;
            model->m_commands = cmd;

            cmd_cnt += cmd->command_count;
        };

        model->m_numCommands = cmd_cnt;
    }

    // Close the file, we're done with it
    fclose( f );

    return model;
}

