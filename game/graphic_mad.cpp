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

/// \file graphic_mad.c
/// \brief Character model drawing code.
/// \details

#include "graphic_mad.h"
#include "mad.h"
#include "file_formats/id_md2.h"

#include "log.h"
#include "camera.h"
#include "game.h"
#include "input.h"
#include "texture.h"
#include "lighting.h"
#include "network.h"
#include "collision.h"

#include "egoboo_setup.h"
#include "egoboo.h"

#include "profile.inl"
#include "char.inl"
#include "md2.inl"

#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void draw_points( ego_chr * pchr, const int vrt_offset, const size_t verts );
static void _draw_one_grip_raw( gfx_mad_instance * pinst, ego_mad * pmad, const int slot );
static void draw_one_grip( gfx_mad_instance * pinst, ego_mad * pmad, const int slot );
static void chr_draw_grips( ego_chr * pchr );
static void chr_draw_attached_grip( ego_chr * pchr );
static void render_chr_bbox( ego_chr * pchr );
static void render_chr_grips( ego_chr * pchr );
static void render_chr_points( ego_chr * pchr );
static bool_t render_chr_mount_cv( ego_chr * pchr );
static bool_t render_chr_grip_cv( ego_chr * pchr, const int grip_offset );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t render_one_mad_enviro( const CHR_REF & character, const GLXvector4f tint, const BIT_FIELD bits )
{
    /// \author ZZ
    /// \details  This function draws an environment mapped model

    Uint16 cnt;
    Uint16 vertex;
    float  uoffset, voffset;

    ego_chr          * pchr;
    gfx_mad_instance * pinst;
    oglx_texture_t   * ptex;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr  = ChrObjList.get_data_ptr( character );
    pinst = &( pchr->gfx_inst );

    ptex = NULL;
    if ( 0 != ( bits & CHR_PHONG ) )
    {
        ptex = TxTexture_get_ptr( TX_REF( TX_PHONG ) );
    }

    if ( NULL == ptex )
    {
        ptex = TxTexture_get_ptr( pinst->texture );
    }

    uoffset = pinst->uoffset - PCamera->turn_z_one;
    voffset = pinst->voffset;

    if ( 0 != ( bits & CHR_REFLECT ) )
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->ref.matrix.v );
    }
    else
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->matrix.v );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( "render_one_mad_enviro", GL_CURRENT_BIT );
    {
        int cmd_count;
        ego_MD2_GLCommand * glcommand;

        GLXvector4f curr_color;

        GL_DEBUG( glGetFloatv )( GL_CURRENT_COLOR, curr_color );

        // Render each command
        cmd_count   = md2_get_numCommands( pinst->md2_ptr );
        glcommand   = ( ego_MD2_GLCommand * )md2_get_Commands( pinst->md2_ptr );

        for ( cnt = 0; cnt < cmd_count && NULL != glcommand; cnt++ )
        {
            int count = glcommand->command_count;

            GL_DEBUG_BEGIN( glcommand->gl_mode );
            {
                int tnc;

                for ( tnc = 0; tnc < count; tnc++ )
                {
                    GLfloat     cmax;
                    GLXvector4f col;
                    GLfloat     tex[2];
                    ego_GLvertex   *pvrt;

                    vertex = glcommand->data[tnc].index;
                    if ( vertex >= pinst->vrt_count ) continue;

                    pvrt   = pinst->vrt_lst + vertex;

                    // normalize the color so it can be modulated by the Phong/environment map
                    col[RR] = pvrt->color_dir * INV_FF;
                    col[GG] = pvrt->color_dir * INV_FF;
                    col[BB] = pvrt->color_dir * INV_FF;
                    col[AA] = 1.0f;

                    cmax = SDL_max( SDL_max( col[RR], col[GG] ), col[BB] );

                    if ( cmax != 0.0f )
                    {
                        col[RR] /= cmax;
                        col[GG] /= cmax;
                        col[BB] /= cmax;
                    }

                    // apply the tint
                    col[RR] *= tint[RR];
                    col[GG] *= tint[GG];
                    col[BB] *= tint[BB];
                    col[AA] *= tint[AA];

                    tex[0] = pvrt->env[XX] + uoffset;
                    tex[1] = CLIP( cmax, 0.0f, 1.0f );

                    if ( 0 != ( bits & CHR_PHONG ) )
                    {
                        // determine the Phong texture coordinates
                        // the default Phong is bright in both the forward and back directions...
                        tex[1] = tex[1] * 0.5f + 0.5f;
                    }

                    GL_DEBUG( glColor4fv )( col );
                    GL_DEBUG( glNormal3fv )( pvrt->nrm );
                    GL_DEBUG( glTexCoord2fv )( tex );
                    GL_DEBUG( glVertex3fv )( pvrt->pos );
                }

            }
            GL_DEBUG_END();

            glcommand = glcommand->next;
        }
    }

    ATTRIB_POP( "render_one_mad_enviro" );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    return btrue;
}

// Do fog...
/*
if(fogon && pinst->light==255)
{
    // The full fog value
    alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

    for (cnt = 0; cnt < pmad->transvertices; cnt++)
    {
        // Figure out the z position of the vertex...  Not totally accurate
        z = (pinst->vrt_lst[cnt].pos[ZZ]) + pchr->matrix(3,2);

        // Figure out the fog coloring
        if(z < fogtop)
        {
            if(z < fogbottom)
            {
                pinst->vrt_lst[cnt].specular = alpha;
            }
            else
            {
                z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f...  Amount of fog to keep
                red = fogred * z;
                grn = foggrn * z;
                blu = fogblu * z;
                fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                pinst->vrt_lst[cnt].specular = fogspec;
            }
        }
        else
        {
            pinst->vrt_lst[cnt].specular = 0;
        }
    }
}

else
{
    for (cnt = 0; cnt < pmad->transvertices; cnt++)
        pinst->vrt_lst[cnt].specular = 0;
}

*/

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_tex( const CHR_REF & character, const GLXvector4f tint, const BIT_FIELD bits )
{
    /// \author ZZ
    /// \details  This function draws a model

    int    cmd_count;
    int    cnt;
    Uint16 vertex;
    float  uoffset, voffset;

    ego_chr          * pchr;
    gfx_mad_instance * pinst;
    oglx_texture_t   * ptex;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr  = ChrObjList.get_data_ptr( character );
    pinst = &( pchr->gfx_inst );

    if ( NULL == pinst->md2_ptr ) return bfalse;

    // To make life easier
    ptex = TxTexture_get_ptr( pinst->texture );

    uoffset = pinst->uoffset * INV_FFFF;
    voffset = pinst->voffset * INV_FFFF;

    if ( 0 != ( bits & CHR_REFLECT ) )
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->ref.matrix.v );
    }
    else
    {
        GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
        GL_DEBUG( glPushMatrix )();
        GL_DEBUG( glMultMatrixf )( pinst->matrix.v );
    }

    // Choose texture and matrix
    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( "render_one_mad_tex", GL_CURRENT_BIT );
    {
        float             base_amb;
        ego_MD2_GLCommand * glcommand;

        // set the basic tint. if the object is marked with CHR_LIGHT
        // the color will not be set again inside the loop
        GL_DEBUG( glColor4fv )( tint );

        base_amb = 0.0f;
        if ( 0 == ( bits & CHR_LIGHT ) )
        {
            // convert the "light" parameter to self-lighting for
            // every object that is not being rendered using CHR_LIGHT
            base_amb   = ( 255 == pinst->light ) ? 0 : ( pinst->light * INV_FF );
        }

        // Render each command
        cmd_count   = md2_get_numCommands( pinst->md2_ptr );
        glcommand   = ( ego_MD2_GLCommand * )md2_get_Commands( pinst->md2_ptr );

        for ( cnt = 0; cnt < cmd_count && NULL != glcommand; cnt++ )
        {
            int count = glcommand->command_count;

            GL_DEBUG_BEGIN( glcommand->gl_mode );
            {
                int tnc;

                for ( tnc = 0; tnc < count; tnc++ )
                {
                    GLXvector2f tex;
                    GLXvector4f col;
                    ego_GLvertex * pvrt;

                    vertex = glcommand->data[tnc].index;
                    if ( vertex >= pinst->vrt_count ) continue;

                    pvrt = pinst->vrt_lst + vertex;

                    // determine the texture coordinates
                    tex[0] = glcommand->data[tnc].s + uoffset;
                    tex[1] = glcommand->data[tnc].t + voffset;

                    // determine the vertex color for objects that have
                    // per vertex lighting
                    if ( 0 == ( bits & CHR_LIGHT ) )
                    {
                        float fcol;

                        // convert the "light" parameter to self-lighting for
                        // every object that is not being rendered using CHR_LIGHT
                        fcol   = pvrt->color_dir * INV_FF;

                        col[0] = fcol * tint[0];
                        col[1] = fcol * tint[1];
                        col[2] = fcol * tint[2];
                        col[3] = tint[0];

                        if ( 0 != ( bits & CHR_PHONG ) )
                        {
                            fcol = base_amb + pinst->color_amb * INV_FF;

                            col[0] += fcol * tint[0];
                            col[1] += fcol * tint[1];
                            col[2] += fcol * tint[2];
                        }

                        col[0] = CLIP( col[0], 0.0f, 1.0f );
                        col[1] = CLIP( col[1], 0.0f, 1.0f );
                        col[2] = CLIP( col[2], 0.0f, 1.0f );

                        GL_DEBUG( glColor4fv )( col );
                    }

                    GL_DEBUG( glNormal3fv )( pvrt->nrm );
                    GL_DEBUG( glTexCoord2fv )( tex );
                    GL_DEBUG( glVertex3fv )( pvrt->pos );
                }
            }
            GL_DEBUG_END();

            glcommand = glcommand->next;
        }
    }
    ATTRIB_POP( "render_one_mad_tex" );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    return btrue;
}

/*
    // Do fog...
    if(fogon && pinst->light==255)
    {
        // The full fog value
        alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

        for (cnt = 0; cnt < pmad->transvertices; cnt++)
        {
            // Figure out the z position of the vertex...  Not totally accurate
            z = (pinst->vrt_lst[cnt].pos[ZZ]) + pchr->matrix(3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    pinst->vrt_lst[cnt].specular = alpha;
                }
                else
                {
                    spek = pinst->vrt_lst[cnt].specular & 255;
                    z = (z - fogbottom)/fogdistance;  // 0.0f to 1.0f...  Amount of old to keep
                    fogtokeep = 1.0f-z;  // 0.0f to 1.0f...  Amount of fog to keep
                    spek = spek * z;
                    red = (fogred * fogtokeep) + spek;
                    grn = (foggrn * fogtokeep) + spek;
                    blu = (fogblu * fogtokeep) + spek;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    pinst->vrt_lst[cnt].specular = fogspec;
                }
            }
        }
    }
*/

//--------------------------------------------------------------------------------------------
bool_t render_one_mad( const CHR_REF & character, const GLXvector4f tint, const BIT_FIELD bits )
{
    /// \author ZZ
    /// \details  This function picks the actual function to use

    ego_chr * pchr;
    bool_t retval;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    if ( pchr->is_hidden ) return bfalse;

    if ( pchr->gfx_inst.enviro || HAS_SOME_BITS( bits, CHR_PHONG ) )
    {
        retval = render_one_mad_enviro( character, tint, bits );
    }
    else
    {
        retval = render_one_mad_tex( character, tint, bits );
    }

#if EGO_DEBUG && defined(DEBUG_CHR_BBOX)

    // don't draw the debug stuff for reflections
    if ( 0 == ( bits & CHR_REFLECT ) )
    {
        render_chr_bbox( pchr );
        //render_chr_grips( pchr );
        //render_chr_mount_cv( pchr );
        //render_chr_points( pchr );
    }
#endif

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t render_one_mad_ref( const CHR_REF & ichr )
{
    /// \author ZZ
    /// \details  This function draws characters reflected in the floor

    ego_chr * pchr;
    gfx_mad_instance * pinst;
    GLXvector4f tint;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;
    pinst = &( pchr->gfx_inst );

    if ( pchr->is_hidden ) return bfalse;

    if ( !pinst->ref.matrix_valid )
    {
        if ( !apply_reflection_matrix( &( pchr->gfx_inst ), pchr->enviro.grid_level ) )
        {
            return bfalse;
        }
    }

    ATTRIB_PUSH( "render_one_mad_ref", GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT );
    {
        GL_DEBUG( glEnable )( GL_CULL_FACE );  // GL_ENABLE_BIT

        // cull face CCW because we are rendering a reflected object
        GL_DEBUG( glFrontFace )( GL_CCW );    // GL_POLYGON_BIT

        if ( pinst->ref.alpha != 255 && pinst->ref.light == 255 )
        {
            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA );                        // GL_COLOR_BUFFER_BIT

            gfx_mad_instance::get_tint( pinst, tint, CHR_ALPHA | CHR_REFLECT );

            // the previous call to gfx_mad_instance::update_lighting_ref() has actually set the
            // alpha and light for all vertices
            render_one_mad( ichr, tint, CHR_ALPHA | CHR_REFLECT );
        }

        if ( pinst->ref.light != 255 )
        {
            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );                        // GL_COLOR_BUFFER_BIT

            gfx_mad_instance::get_tint( pinst, tint, CHR_LIGHT | CHR_REFLECT );

            // the previous call to gfx_mad_instance::update_lighting_ref() has actually set the
            // alpha and light for all vertices
            render_one_mad( ichr, tint, CHR_LIGHT | CHR_REFLECT );
        }

        if ( gfx.phongon && pinst->sheen > 0 )
        {
            GL_DEBUG( glEnable )( GL_BLEND );
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );

            gfx_mad_instance::get_tint( pinst, tint, CHR_PHONG | CHR_REFLECT );

            render_one_mad( ichr, tint, CHR_PHONG | CHR_REFLECT );
        }
    }
    ATTRIB_POP( "render_one_mad_ref" );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_chr_bbox( ego_chr * pchr )
{
    //PLA_REF ipla;
    bool_t render_player_platforms;

    if ( !PROCESSING_PCHR( pchr ) ) return;

    render_player_platforms = bfalse; // pchr->platform;
    //for ( player_deque::iterator ipla = PlaDeque.begin(); ipla != PlaDeque.end(); ipla++ )
    //{
    //    ego_chr * pplayer_chr = pla_get_pchr(*ipla);
    //    if( NULL == pplayer_chr ) continue;

    //    if( pchr->get_ego_obj().index == pplayer_chr->onwhichplatform_ref )
    //    {
    //        render_player_platforms = btrue;
    //        break;
    //    }
    //}

    // draw the object bounding box as a part of the graphics debug mode F7
    if ( cfg.dev_mode && ( SDLKEYDOWN( SDLK_F7 ) || render_player_platforms ) )
    {
        GL_DEBUG( glDisable )( GL_TEXTURE_2D );
        {
            ego_oct_bb   bb;

            ego_oct_bb::add_vector( pchr->chr_min_cv, pchr->pos.v, bb );

            GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );
            render_oct_bb( &bb, btrue, btrue );
        }
        GL_DEBUG( glEnable )( GL_TEXTURE_2D );
    }

    // the grips and vertices of all objects
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F6 ) )
    {
        chr_draw_attached_grip( pchr );

        // draw all the vertices of an object
        //GL_DEBUG( glPointSize( 5 ) );
        //draw_points( pchr, 0, pchr->gfx_inst.vrt_count );
    }
}

//--------------------------------------------------------------------------------------------
void render_chr_grips( ego_chr * pchr )
{
    bool_t render_mount_grip;

    if ( !PROCESSING_PCHR( pchr ) ) return;

    render_mount_grip = pchr->ismount;

    // draw the object's "saddle"  as a part of the graphics debug mode F7
    if ( cfg.dev_mode && render_mount_grip )
    {
        draw_one_grip( &( pchr->gfx_inst ), ego_chr::get_pmad( GET_REF_PCHR( pchr ) ), SLOT_LEFT );
    }

    // the grips and vertices of all objects
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F6 ) )
    {
        chr_draw_attached_grip( pchr );
    }
}

//--------------------------------------------------------------------------------------------
bool_t render_chr_mount_cv( ego_chr * pchr )
{
    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( !pchr->ismount ) return bfalse;

    return render_chr_grip_cv( pchr, GRIP_LEFT );
}

//--------------------------------------------------------------------------------------------
bool_t render_chr_grip_cv( ego_chr * pchr, const int grip_offset )
{
    bool_t retval;

    fvec3_t grip_up, grip_origin;
    ego_oct_bb    grip_cv;

    retval = calc_grip_cv( pchr, grip_offset, &grip_cv, grip_origin.v, grip_up.v );
    if ( !retval ) return bfalse;

    GL_DEBUG( glDisable )( GL_TEXTURE_2D );
    {
        GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );
        render_oct_bb( &grip_cv, btrue, btrue );
    }
    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_chr_points( ego_chr * pchr )
{

    if ( !PROCESSING_PCHR( pchr ) ) return;

    // the grips and vertices of all objects
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F6 ) )
    {
        // draw all the vertices of an object
        GL_DEBUG( glPointSize( 5 ) );
        draw_points( pchr, 0, pchr->gfx_inst.vrt_count );
    }
}

//--------------------------------------------------------------------------------------------
void draw_points( ego_chr * pchr, const int vrt_offset, const size_t verts )
{
    /// \author BB
    /// \details  a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.

    ego_mad * pmad;

    int vmin, vmax, cnt;
    GLboolean texture_1d_enabled, texture_2d_enabled;

    if ( !PROCESSING_PCHR( pchr ) ) return;

    pmad = ego_chr::get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return;

    vmin = vrt_offset;
    vmax = vmin + verts;

    if ( vmin < 0 || vmax < 0 ) return;
    if (( const size_t )vmin > pchr->gfx_inst.vrt_count || ( const size_t )vmax > pchr->gfx_inst.vrt_count ) return;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glMultMatrixf )( pchr->gfx_inst.matrix.v );

    GL_DEBUG_BEGIN( GL_POINTS );
    {
        for ( cnt = vmin; cnt < vmax; cnt++ )
        {
            GL_DEBUG( glVertex3fv )( pchr->gfx_inst.vrt_lst[cnt].pos );
        }
    }
    GL_DEBUG_END();

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    if ( texture_1d_enabled ) GL_DEBUG( glEnable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glEnable )( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void draw_one_grip( gfx_mad_instance * pinst, ego_mad * pmad, const int slot )
{
    GLboolean texture_1d_enabled, texture_2d_enabled;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glMultMatrixf )( pinst->matrix.v );

    _draw_one_grip_raw( pinst, pmad, slot );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    if ( texture_1d_enabled ) GL_DEBUG( glEnable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glEnable )( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void _draw_one_grip_raw( gfx_mad_instance * pinst, ego_mad * pmad, const int slot )
{
    int vmin, vmax, cnt;

    GLXvector4f red = {1, 0, 0, 1};
    GLXvector4f grn = {0, 1, 0, 1};
    GLXvector4f blu = {0, 0, 1, 1};
    GLfloat  * col_ary[3];

    col_ary[0] = red;
    col_ary[1] = grn;
    col_ary[2] = blu;

    if ( NULL == pinst || NULL == pmad ) return;

    vmin = ego_sint( pinst->vrt_count ) - ego_sint( slot_to_grip_offset(( slot_t )slot ) );
    vmax = vmin + GRIP_VERTS;

    if ( vmin >= 0 && vmax >= 0 && ( const size_t )vmax <= pinst->vrt_count )
    {
        fvec3_t   src, dst, diff;

        src.x = pinst->vrt_lst[vmin].pos[XX];
        src.y = pinst->vrt_lst[vmin].pos[YY];
        src.z = pinst->vrt_lst[vmin].pos[ZZ];

        GL_DEBUG_BEGIN( GL_LINES );
        {
            for ( cnt = 1; cnt < GRIP_VERTS; cnt++ )
            {
                diff.x = pinst->vrt_lst[vmin+cnt].pos[XX] - src.x;
                diff.y = pinst->vrt_lst[vmin+cnt].pos[YY] - src.y;
                diff.z = pinst->vrt_lst[vmin+cnt].pos[ZZ] - src.z;

                dst.x = src.x + 3 * diff.x;
                dst.y = src.y + 3 * diff.y;
                dst.z = src.z + 3 * diff.z;

                GL_DEBUG( glColor4fv )( col_ary[cnt-1] );

                GL_DEBUG( glVertex3fv )( src.v );
                GL_DEBUG( glVertex3fv )( dst.v );
            }
        }
        GL_DEBUG_END();
    }

    GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f );
}

//--------------------------------------------------------------------------------------------
void chr_draw_attached_grip( ego_chr * pchr )
{
    ego_mad * pholder_mad;
    ego_cap * pholder_cap;
    ego_chr * pholder;

    if ( !PROCESSING_PCHR( pchr ) ) return;

    if ( !DEFINED_CHR( pchr->attachedto ) ) return;
    pholder = ChrObjList.get_data_ptr( pchr->attachedto );

    pholder_cap = pro_get_pcap( pholder->profile_ref );
    if ( NULL == pholder_cap ) return;

    pholder_mad = ego_chr::get_pmad( GET_REF_PCHR( pholder ) );
    if ( NULL == pholder_mad ) return;

    draw_one_grip( &( pholder->gfx_inst ), pholder_mad, pchr->inwhich_slot );
}

//--------------------------------------------------------------------------------------------
void chr_draw_grips( ego_chr * pchr )
{
    ego_mad * pmad;
    ego_cap * pcap;

    int slot;
    GLboolean texture_1d_enabled, texture_2d_enabled;

    if ( !PROCESSING_PCHR( pchr ) ) return;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return;

    pmad = ego_chr::get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glMultMatrixf )( pchr->gfx_inst.matrix.v );

    slot = SLOT_LEFT;
    if ( pcap->slotvalid[slot] )
    {
        _draw_one_grip_raw( &( pchr->gfx_inst ), pmad, slot );
    }

    slot = SLOT_RIGHT;
    if ( pcap->slotvalid[slot] )
    {
        _draw_one_grip_raw( &( pchr->gfx_inst ), pmad, slot );
    }

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    if ( texture_1d_enabled ) GL_DEBUG( glEnable )( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) GL_DEBUG( glEnable )( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void gfx_mad_instance::update_lighting_base( gfx_mad_instance * pinst, ego_chr * pchr, const bool_t force )
{
    /// \author BB
    /// \details  determine the basic per-vertex lighting

    Uint16 cnt;

    ego_lighting_cache global_light, loc_light;

    ego_GLvertex * vrt_lst;

    bool_t loc_force = force;

    if ( NULL == pinst || NULL == pchr ) return;
    vrt_lst = pinst->vrt_lst;

    // loc_force this function to be evaluated the 1st time through
    if ( 0 == update_wld && 0 == frame_all ) loc_force = btrue;

    // has this already been calculated this update?
    if ( !loc_force && pinst->lighting_update_wld >= update_wld ) return;
    pinst->lighting_update_wld = update_wld;

    // make sure the matrix is valid
    ego_chr::update_matrix( pchr, btrue );

    // has this already been calculated this frame?
    if ( !loc_force && pinst->lighting_frame_all >= frame_all ) return;

    // reduce the amount of updates to an average of about 1 every 2 frames, but dither
    // the updating so that not all objects update on the same frame
    pinst->lighting_frame_all = frame_all + (( frame_all + ego_chr::cget_obj_ref( *pchr ).get_id() ) & 0x03 );

    // interpolate the lighting for the origin of the object
    grid_lighting_interpolate( PMesh, &global_light, pchr->pos.x, pchr->pos.y );

    // rotate the lighting data to body_centered coordinates
    lighting_project_cache( &loc_light, &global_light, pinst->matrix );

    pinst->color_amb = 0.9f * pinst->color_amb + 0.1f * ( loc_light.hgh.lighting[LVEC_AMB] + loc_light.low.lighting[LVEC_AMB] ) * 0.5f;

    pinst->max_light = -255;
    pinst->min_light =  255;
    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        Sint16 lite;

        ego_GLvertex * pvert = pinst->vrt_lst + cnt;

        // a simple "height" measurement
        float hgt = pvert->pos[ZZ] * pinst->matrix.CNV( 3, 3 ) + pinst->matrix.CNV( 3, 3 );

        if ( pvert->nrm[0] == 0.0f && pvert->nrm[1] == 0.0f && pvert->nrm[2] == 0.0f )
        {
            // this is the "ambient only" index, but it really means to sum up all the light
            GLfloat tnrm[3];

            tnrm[0] = tnrm[1] = tnrm[2] = 1.0f;
            lite  = lighting_evaluate_cache( &loc_light, tnrm, hgt, PMesh->tmem.bbox, NULL, NULL );

            tnrm[0] = tnrm[1] = tnrm[2] = -1.0f;
            lite += lighting_evaluate_cache( &loc_light, tnrm, hgt, PMesh->tmem.bbox, NULL, NULL );

            // average all the directions
            lite /= 6;
        }
        else
        {
            lite  = lighting_evaluate_cache( &loc_light, pvert->nrm, hgt, PMesh->tmem.bbox, NULL, NULL );
        }

        pvert->color_dir = 0.9f * pvert->color_dir + 0.1f * lite;

        pinst->max_light = SDL_max( pinst->max_light, pvert->color_dir );
        pinst->min_light = SDL_min( pinst->min_light, pvert->color_dir );
    }

    // ??coerce this to reasonable values in the presence of negative light??
    if ( pinst->max_light < 0 ) pinst->max_light = 0;
    if ( pinst->min_light < 0 ) pinst->min_light = 0;
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::update_bbox( gfx_mad_instance * pinst )
{
    int           i, frame_count;

    ego_MD2_Frame * frame_list, * pframe_nxt, * pframe_lst;

    if ( NULL == pinst || NULL == pinst->md2_ptr ) return rv_error;

    frame_count = md2_get_numFrames( pinst->md2_ptr );
    if ( pinst->pose.frame_nxt >= frame_count ||  pinst->pose.frame_lst >= frame_count ) return rv_error;

    frame_list = ( ego_MD2_Frame * )md2_get_Frames( pinst->md2_ptr );
    pframe_lst = frame_list + pinst->pose.frame_lst;
    pframe_nxt = frame_list + pinst->pose.frame_nxt;

    if ( pinst->pose.frame_nxt == pinst->pose.frame_lst || pinst->pose.flip == 0.0f )
    {
        pinst->bbox = pframe_lst->bb;
    }
    else if ( pinst->pose.flip == 1.0f )
    {
        pinst->bbox = pframe_nxt->bb;
    }
    else
    {
        for ( i = 0; i < OCT_COUNT; i++ )
        {
            pinst->bbox.mins[i] = pframe_lst->bb.mins[i] + ( pframe_nxt->bb.mins[i] - pframe_lst->bb.mins[i] ) * pinst->pose.flip;
            pinst->bbox.maxs[i] = pframe_lst->bb.maxs[i] + ( pframe_nxt->bb.maxs[i] - pframe_lst->bb.maxs[i] ) * pinst->pose.flip;
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::test_pose( const pose_data & p_old, const pose_data & p_new )
{
    if ( ego_uint( ~0L ) == p_old.id ) return rv_fail;
    if ( ego_uint( ~0L ) == p_new.id ) return rv_fail;

    return ( p_old == p_new ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
int gfx_mad_instance::validate_pose( gfx_mad_instance * pgfx_inst, const mad_instance * pmad_inst )
{
    /// \author BB
    /// \details  determine whether the gfx_vlst_range has valid animation data
    //                returns the number of items that were invalidated

    bool_t matches = bfalse;

    if ( NULL == pgfx_inst ) return rv_error;

    matches = bfalse;
    if ( NULL != pmad_inst )
    {
        egoboo_rv test_rv = test_pose( pgfx_inst->pose, pmad_inst->state );
        if ( rv_error == test_rv ) return test_rv;

        matches = ( rv_success == test_rv );
    }

    if ( !matches )
    {
        // copy the pose info over
        pgfx_inst->pose = pmad_inst->state;

        // invalidate the vertex range
        pgfx_inst->vrange.init();

        // invalidate the matrix info, too?
        pgfx_inst->mcache.valid = bfalse;
    }

    return matches;
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::test_vertices( const gfx_range & r_old, const gfx_range & r_new )
{
    /// \author BB
    /// \details  determine whether the vr_old matches r_new
    //                rv_error   means that the function was passed invalid values
    //                rv_fail    means that the gfx_vlst_range is NOT valid
    //                rv_success means that the gfx_vlst_range is valid

    // check to see if the stored range is valid
    if ( r_old.is_null() || r_new.is_null() ) return rv_fail;

    // is r_new completely inside vr_old?
    bool_t matches = gfx_range::rhs_inside_lhs( r_old, r_new );

    return matches ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::validate_vertices( gfx_mad_instance * pinst, const gfx_range & r_new )
{
    /// \author BB
    /// \details  determine whether the gfx_vlst_range has valid frame data in the given range
    //                rv_error   means that the function was passed invalid values
    //                rv_fail    means that the gfx_vlst_range is NOT valid
    //                rv_success means that the gfx_vlst_range is valid

    // do we have a valid instance?
    if ( NULL == pinst ) return rv_error;

    // alias the fange data
    gfx_vlst_range & vr_old = pinst->vrange;

    // make a copy in case we need to expand a default range
    gfx_range rtmp_new = r_new;

    // get the last valid vertex from the chr_instance
    int maxvert = ego_sint( pinst->vrt_count ) - 1;

    // a null range on rtmp_new means testing the full range
    if ( rtmp_new.is_null() )
    {
        rtmp_new.vmin = 0;
        rtmp_new.vmax = maxvert;
    }

    // a null range on rtmp_new means testing the full range
    if ( rtmp_new.is_null() )
    {
        rtmp_new.vmin = 0;
        rtmp_new.vmax = maxvert;
    }

    egoboo_rv test_rv = test_vertices( vr_old.cget_range(), rtmp_new );
    if ( rv_error == test_rv ) return test_rv;

    // if not, invalidate vr_old
    if ( rv_success != test_rv )
    {
        // the new range is over different vertices than the old range.
        vr_old.init();
    }

    return test_rv;
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::needs_update( gfx_mad_instance * pgfx_inst, const pose_data & p_new, const gfx_range & r_new, bool_t *verts_match, bool_t *frames_match )
{
    /// \author BB
    /// \details  determine whether some specific vertices of an instance need to be updated
    //                rv_error   means that the function was passed invalid values
    //                rv_fail    means that the instance does not need to be updated
    //                rv_success means that the instance should be updated

    bool_t local_verts_match, local_frames_match;
    egoboo_rv verts_rv, frames_rv;

    if ( NULL == pgfx_inst ) return rv_error;

    // ensure that the pointers point to something
    if ( NULL == verts_match ) verts_match  = &local_verts_match;
    if ( NULL == frames_match ) frames_match = &local_frames_match;

    // initialize the booleans
    *verts_match  = bfalse;
    *frames_match = bfalse;

    // test the vertex range
    verts_rv = gfx_mad_instance::test_vertices( pgfx_inst->vrange, r_new );
    if ( rv_error == verts_rv ) return verts_rv;
    *verts_match  = ( rv_success == verts_rv );

    // test the pose
    frames_rv = gfx_mad_instance::test_pose( pgfx_inst->pose, p_new );
    if ( rv_error == frames_rv ) return frames_rv;
    *frames_match = ( rv_success == frames_rv );

    return ( !( *verts_match ) || !( *frames_match ) ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::update_vertices( gfx_mad_instance * pgfx_inst, const pose_data & p_new, const gfx_range & r_new, const bool_t force )
{
    int    i, maxvert, frame_count, md2_vertices;
    bool_t vertices_match, frames_match;

    egoboo_rv retval;

    gfx_vlst_range * psave;

    ego_MD2_Frame * frame_list, * pframe_nxt, * pframe_lst;

    if ( NULL == pgfx_inst ) return rv_error;
    psave = &( pgfx_inst->vrange );

    // make a copy in case we have to expand a default range
    gfx_range rtmp_new = r_new;

    if ( rv_error == gfx_mad_instance::update_bbox( pgfx_inst ) )
    {
        return rv_error;
    }

    if ( NULL == pgfx_inst->md2_ptr ) return rv_error;

    // make sure we have valid data
    md2_vertices = md2_get_numVertices( pgfx_inst->md2_ptr );
    if ( md2_vertices < 0 )
    {
        log_error( "%s - md2 model has negative number of vertices.... is it corrupted?\n", __FUNCTION__ );
    }

    if ( pgfx_inst->vrt_count != ( const size_t )md2_vertices )
    {
        log_error( "%s - character instance vertex data does not match its md2\n", __FUNCTION__ );
    }

    // get the vertex list size from the chr_instance
    maxvert = ego_sint( pgfx_inst->vrt_count ) - 1;

    // handle the default parameters
    if ( rtmp_new.vmin < 0 ) rtmp_new.vmin = 0;
    if ( rtmp_new.vmax < 0 ) rtmp_new.vmax = maxvert;

    // are they in the right order?
    if ( rtmp_new.vmax < rtmp_new.vmin ) SWAP( int, rtmp_new.vmax, rtmp_new.vmin );

    if ( force )
    {
        // force an update of vertices

        // select a range that encompasses the requested vertices and the saved vertices
        // if this is the 1st update, the saved vertices may be set to invalid values, as well
        rtmp_new.vmin = ( psave->vmin < 0 ) ? rtmp_new.vmin : SDL_min( rtmp_new.vmin, psave->vmin );
        rtmp_new.vmax = ( psave->vmax < 0 ) ? rtmp_new.vmax : SDL_max( rtmp_new.vmax, psave->vmax );

        // force the routine to update
        vertices_match = bfalse;
        frames_match   = bfalse;
    }
    else
    {
        // make sure that the vertices are within the max range
        rtmp_new.vmin = CLIP( rtmp_new.vmin, 0, maxvert );
        rtmp_new.vmax = CLIP( rtmp_new.vmax, 0, maxvert );

        // do we need to update?
        retval = gfx_mad_instance::needs_update( pgfx_inst, p_new, rtmp_new, &vertices_match, &frames_match );
        if ( rv_error == retval ) return rv_error;            // rv_error == retval means some pointer or reference is messed up
        if ( rv_fail  == retval ) return rv_success;          // rv_fail  == retval means we do not need to update this round
    }

    // make sure the frames are in the valid range
    frame_count = md2_get_numFrames( pgfx_inst->md2_ptr );
    if ( p_new.frame_nxt >= frame_count || p_new.frame_lst >= frame_count )
    {
        log_error( "%s - character instance frame is outside the range of its md2\n", __FUNCTION__ );
    }

    // grab the frame data from the correct model
    frame_list = ( ego_MD2_Frame * )md2_get_Frames( pgfx_inst->md2_ptr );
    pframe_nxt = frame_list + p_new.frame_nxt;
    pframe_lst = frame_list + p_new.frame_lst;

    if ( p_new.frame_nxt == p_new.frame_lst || p_new.flip == 0.0f )
    {
        for ( i = rtmp_new.vmin; i <= rtmp_new.vmax; i++ )
        {
            Uint16 vrta_lst;

            pgfx_inst->vrt_lst[i].pos[XX] = pframe_lst->vertex_lst[i].pos.x;
            pgfx_inst->vrt_lst[i].pos[YY] = pframe_lst->vertex_lst[i].pos.y;
            pgfx_inst->vrt_lst[i].pos[ZZ] = pframe_lst->vertex_lst[i].pos.z;
            pgfx_inst->vrt_lst[i].pos[WW] = 1.0f;

            pgfx_inst->vrt_lst[i].nrm[XX] = pframe_lst->vertex_lst[i].nrm.x;
            pgfx_inst->vrt_lst[i].nrm[YY] = pframe_lst->vertex_lst[i].nrm.y;
            pgfx_inst->vrt_lst[i].nrm[ZZ] = pframe_lst->vertex_lst[i].nrm.z;

            vrta_lst = pframe_lst->vertex_lst[i].normal;

            pgfx_inst->vrt_lst[i].env[XX] = indextoenvirox[vrta_lst];
            pgfx_inst->vrt_lst[i].env[YY] = 0.5f * ( 1.0f + pgfx_inst->vrt_lst[i].nrm[ZZ] );
        }
    }
    else if ( p_new.flip == 1.0f )
    {
        for ( i = rtmp_new.vmin; i <= rtmp_new.vmax; i++ )
        {
            Uint16 vrta_nxt;

            pgfx_inst->vrt_lst[i].pos[XX] = pframe_nxt->vertex_lst[i].pos.x;
            pgfx_inst->vrt_lst[i].pos[YY] = pframe_nxt->vertex_lst[i].pos.y;
            pgfx_inst->vrt_lst[i].pos[ZZ] = pframe_nxt->vertex_lst[i].pos.z;
            pgfx_inst->vrt_lst[i].pos[WW] = 1.0f;

            pgfx_inst->vrt_lst[i].nrm[XX] = pframe_nxt->vertex_lst[i].nrm.x;
            pgfx_inst->vrt_lst[i].nrm[YY] = pframe_nxt->vertex_lst[i].nrm.y;
            pgfx_inst->vrt_lst[i].nrm[ZZ] = pframe_nxt->vertex_lst[i].nrm.z;

            vrta_nxt = pframe_nxt->vertex_lst[i].normal;

            pgfx_inst->vrt_lst[i].env[XX] = indextoenvirox[vrta_nxt];
            pgfx_inst->vrt_lst[i].env[YY] = 0.5f * ( 1.0f + pgfx_inst->vrt_lst[i].nrm[ZZ] );
        }
    }
    else
    {
        for ( i = rtmp_new.vmin; i <= rtmp_new.vmax; i++ )
        {
            Uint16 vrta_lst, vrta_nxt;

            pgfx_inst->vrt_lst[i].pos[XX] = pframe_lst->vertex_lst[i].pos.x + ( pframe_nxt->vertex_lst[i].pos.x - pframe_lst->vertex_lst[i].pos.x ) * p_new.flip;
            pgfx_inst->vrt_lst[i].pos[YY] = pframe_lst->vertex_lst[i].pos.y + ( pframe_nxt->vertex_lst[i].pos.y - pframe_lst->vertex_lst[i].pos.y ) * p_new.flip;
            pgfx_inst->vrt_lst[i].pos[ZZ] = pframe_lst->vertex_lst[i].pos.z + ( pframe_nxt->vertex_lst[i].pos.z - pframe_lst->vertex_lst[i].pos.z ) * p_new.flip;
            pgfx_inst->vrt_lst[i].pos[WW] = 1.0f;

            pgfx_inst->vrt_lst[i].nrm[XX] = pframe_lst->vertex_lst[i].nrm.x + ( pframe_nxt->vertex_lst[i].nrm.x - pframe_lst->vertex_lst[i].nrm.x ) * p_new.flip;
            pgfx_inst->vrt_lst[i].nrm[YY] = pframe_lst->vertex_lst[i].nrm.y + ( pframe_nxt->vertex_lst[i].nrm.y - pframe_lst->vertex_lst[i].nrm.y ) * p_new.flip;
            pgfx_inst->vrt_lst[i].nrm[ZZ] = pframe_lst->vertex_lst[i].nrm.z + ( pframe_nxt->vertex_lst[i].nrm.z - pframe_lst->vertex_lst[i].nrm.z ) * p_new.flip;

            vrta_lst = pframe_lst->vertex_lst[i].normal;
            vrta_nxt = pframe_nxt->vertex_lst[i].normal;

            pgfx_inst->vrt_lst[i].env[XX] = indextoenvirox[vrta_lst] + ( indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst] ) * p_new.flip;
            pgfx_inst->vrt_lst[i].env[YY] = 0.5f * ( 1.0f + pgfx_inst->vrt_lst[i].nrm[ZZ] );
        }
    }

    // update the saved parameters
    return gfx_mad_instance::update_vlst_cache( pgfx_inst, p_new, rtmp_new, force, vertices_match, frames_match );
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::update_vlst_cache( gfx_mad_instance * pgfx_inst, const pose_data & p_new, const gfx_range & r_new, const bool_t force, const bool_t vertices_match, const bool_t frames_match )
{
    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // pgfx_inst->vrange.* values be tested and stored properly

    bool_t verts_updated, frames_updated;
    int    maxvert;

    if ( NULL == pgfx_inst ) return rv_error;
    maxvert = ego_sint( pgfx_inst->vrt_count ) - 1;
    gfx_vlst_range & r_old = pgfx_inst->vrange;
    pose_data      & p_old = pgfx_inst->pose;

    // the save_vmin and save_vmax is the most complex
    verts_updated = bfalse;
    if ( force )
    {
        // to get here, either the specified range was outside the clean range or
        // the animation was updated. In any case, the only vertices that are
        // clean are in the range [vmin, r_new.vmax]

        r_old.vmin   = r_new.vmin;
        r_old.vmax   = r_new.vmax;
        verts_updated = btrue;
    }
    else if ( vertices_match )
    {
        // The only way to get here is to fail the frames_match test, and pass vertices_match

        // This means that all of the vertices were SUPPOSED TO BE updated,
        // but only the ones in the range [vmin, r_new.vmax] actually were.
        r_old.vmin = r_new.vmin;
        r_old.vmax = r_new.vmax;
        verts_updated = btrue;
    }
    else if ( frames_match )
    {
        // The only way to get here is to fail the vertices_match test, and pass frames_match test

        // There was no update to the animation,  but there was an update to some of the vertices
        // The clean vertices should be the union of the sets of the vertices updated this time
        // and the ones updated last time.
        //
        // If these ranges are disjoint, then only one of them can be saved. Choose the larger set

        if ( r_new.vmax >= r_old.vmin && r_new.vmin <= r_old.vmax )
        {
            // the old list [save_vmin, save_vmax] and the new list [vmin, r_new.vmax]
            // overlap, so we can merge them
            r_old.vmin = SDL_min( r_old.vmin, r_new.vmin );
            r_old.vmax = SDL_max( r_old.vmax, r_new.vmax );
            verts_updated = btrue;
        }
        else
        {
            // the old list and the new list are disjoint sets, so we are out of luck
            // save the set with the largest number of members
            if (( r_old.vmax - r_old.vmin ) >= ( r_new.vmax - r_new.vmin ) )
            {
                // obviously no change...
                r_old.vmin = r_old.vmin;
                r_old.vmax = r_old.vmax;
                verts_updated = btrue;
            }
            else
            {
                r_old.vmin = r_new.vmin;
                r_old.vmax = r_new.vmax;
                verts_updated = btrue;
            }
        }
    }
    else
    {
        // The only way to get here is to fail the vertices_match test, and fail the frames_match test

        // everything was dirty, so just save the new vertex list
        r_old.vmin = r_new.vmin;
        r_old.vmax = r_new.vmax;
        verts_updated = btrue;
    }

    // copy the pos info over, including the id value
    p_old = p_new;

    // store the last time there was an update to the animation
    frames_updated = bfalse;
    if ( !frames_match )
    {
        r_old.frame_wld = update_wld;
        frames_updated   = btrue;
    }

    // store the time of the last full update
    if ( 0 == r_new.vmin && maxvert == r_new.vmax )
    {
        r_old.update_wld  = update_wld;
    }

    // mark the saved vlst_cache data as valid
    r_old.valid = btrue;

    return ( verts_updated || frames_updated ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv gfx_mad_instance::update_grip_verts( gfx_mad_instance * pinst, const Uint16 vrt_lst[], const size_t vrt_count )
{
    gfx_range r_new( 0xFFFF, 0 );
    Uint32 cnt;
    size_t count;
    egoboo_rv retval;

    if ( NULL == pinst ) return rv_error;

    if ( NULL == vrt_lst || 0 == vrt_count ) return rv_fail;

    // count the valid attachment points
    count = 0;
    for ( cnt = 0; cnt < vrt_count; cnt++ )
    {
        if ( 0xFFFF == vrt_lst[cnt] ) continue;

        r_new.vmin = SDL_min( r_new.vmin, vrt_lst[cnt] );
        r_new.vmax = SDL_max( r_new.vmax, vrt_lst[cnt] );
        count++;
    }

    // if there are no valid points, there is nothing to do
    if ( 0 == count ) return rv_fail;

    // force the vertices to update
    retval = gfx_mad_instance::update_vertices( pinst, pinst->pose, r_new, btrue );

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_mad_instance * gfx_mad_instance::clear( gfx_mad_instance * ptr )
{
    if ( NULL == ptr ) return ptr;

    // position info
    ptr->matrix = IdentityMatrix();           ///< Character's matrix

    ptr->facing_z = 0;

    // render mode info
    ptr->alpha = 255;
    ptr->light = 0;
    ptr->sheen = 0;
    ptr->enviro = bfalse;

    // color info
    ptr->redshift = 0;        ///< Color channel shifting
    ptr->grnshift = 0;
    ptr->blushift = 0;

    // texture info
    ptr->texture = INVALID_TX_TEXTURE;         ///< The texture id of the character's skin
    ptr->uoffset = 0;         ///< For moving textures
    ptr->voffset = 0;

    // lighting info
    ptr->color_amb           = ~0;
    ptr->max_light           = 0;
    ptr->min_light           = 0;
    ptr->lighting_update_wld = ego_uint( ~0L );
    ptr->lighting_frame_all  = ego_uint( ~0L );

    ptr->vrt_count = 0;
    ptr->vrt_lst   = NULL;

    ptr->indolist = bfalse;

    return ptr;
}

//--------------------------------------------------------------------------------------------
void gfx_mad_instance::clear_cache( gfx_mad_instance * pgfx_inst )
{
    /// \author BB
    /// \details  force gfx_mad_instance::update_vertices() recalculate the vertices the next time
    ///     the function is called

    pgfx_inst->vrange.init();

    gfx_mad_matrix_data::init( &( pgfx_inst->mcache ) );

    gfx_chr_reflection_info::init( &( pgfx_inst->ref ) );

    pgfx_inst->lighting_update_wld = 0;
    pgfx_inst->lighting_frame_all  = 0;
}

//--------------------------------------------------------------------------------------------
gfx_mad_instance * gfx_mad_instance::dtor_this( gfx_mad_instance * pgfx_inst )
{
    if ( NULL == pgfx_inst ) return pgfx_inst;

    gfx_mad_instance::dealloc( pgfx_inst );

    EGOBOO_ASSERT( NULL == pgfx_inst->vrt_lst );

    SDL_memset( pgfx_inst, 0, sizeof( *pgfx_inst ) );

    return clear( pgfx_inst );
}

//--------------------------------------------------------------------------------------------
gfx_mad_instance * gfx_mad_instance::ctor_this( gfx_mad_instance * pgfx_inst )
{
    Uint32 cnt;

    if ( NULL == pgfx_inst ) return pgfx_inst;

    SDL_memset( pgfx_inst, 0, sizeof( *pgfx_inst ) );

    // model parameters
    pgfx_inst->vrt_count = 0;

    // set the initial cache parameters
    gfx_mad_instance::clear_cache( pgfx_inst );

    // Set up initial fade in lighting
    pgfx_inst->color_amb = 0;
    for ( cnt = 0; cnt < pgfx_inst->vrt_count; cnt++ )
    {
        pgfx_inst->vrt_lst[cnt].color_dir = 0;
    }

    // clear out the matrix cache
    gfx_mad_matrix_data::init( &( pgfx_inst->mcache ) );

    // the matrix should never be referenced if the cache is not valid,
    // but it never pays to have a 0 matrix...
    pgfx_inst->matrix = IdentityMatrix();

    // the vlst_cache parameters are not valid
    pgfx_inst->vrange.valid = bfalse;

    return pgfx_inst;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_instance::dealloc( gfx_mad_instance * pgfx_inst )
{
    if ( NULL == pgfx_inst ) return bfalse;

    EGOBOO_DELETE_ARY( pgfx_inst->vrt_lst );
    pgfx_inst->vrt_count = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_instance::alloc( gfx_mad_instance * pgfx_inst, const size_t vlst_size )
{
    if ( NULL == pgfx_inst ) return bfalse;

    gfx_mad_instance::dealloc( pgfx_inst );

    if ( 0 == vlst_size ) return btrue;

    pgfx_inst->vrt_lst = EGOBOO_NEW_ARY( ego_GLvertex, vlst_size );
    if ( NULL != pgfx_inst->vrt_lst )
    {
        pgfx_inst->vrt_count = vlst_size;
    }

    return ( NULL != pgfx_inst->vrt_lst );
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_instance::update_ref( gfx_mad_instance * pgfx_inst, const float grid_level, const bool_t need_matrix )
{
    int trans_temp;

    if ( NULL == pgfx_inst ) return bfalse;

    if ( need_matrix )
    {
        // reflect the ordinary matrix
        apply_reflection_matrix( pgfx_inst, grid_level );
    }

    trans_temp = 255;
    if ( pgfx_inst->ref.matrix_valid )
    {
        float pos_z;

        // determine the reflection alpha
        pos_z = grid_level - pgfx_inst->ref.matrix.CNV( 3, 2 );
        if ( pos_z < 0 ) pos_z = 0;

        trans_temp -= ( int( pos_z ) ) >> 1;
        if ( trans_temp < 0 ) trans_temp = 0;

        trans_temp |= gfx.reffadeor;  // Fix for Riva owners
        trans_temp = CLIP( trans_temp, 0, 255 );
    }

    pgfx_inst->ref.alpha = ( pgfx_inst->alpha * trans_temp * INV_FF ) * 0.5f;
    pgfx_inst->ref.light = ( 255 == pgfx_inst->light ) ? 255 : ( pgfx_inst->light * trans_temp * INV_FF ) * 0.5f;

    pgfx_inst->ref.redshift = pgfx_inst->redshift + 1;
    pgfx_inst->ref.grnshift = pgfx_inst->grnshift + 1;
    pgfx_inst->ref.blushift = pgfx_inst->blushift + 1;

    pgfx_inst->ref.sheen    = pgfx_inst->sheen >> 1;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_instance::spawn( gfx_mad_instance * pgfx_inst, const PRO_REF & profile, const Uint8 skin )
{
    Sint8 greensave = 0, redsave = 0, bluesave = 0;

    ego_pro * pobj;
    ego_cap * pcap;

    if ( NULL == pgfx_inst ) return bfalse;

    // Remember any previous color shifts in case of lasting enchantments
    greensave = pgfx_inst->grnshift;
    redsave   = pgfx_inst->redshift;
    bluesave  = pgfx_inst->blushift;

    // clear the instance
    gfx_mad_instance::ctor_this( pgfx_inst );

    if ( !LOADED_PRO( profile ) ) return bfalse;
    pobj = ProList.lst + profile;

    pcap = pro_get_pcap( profile );

    // lighting parameters
    pgfx_inst->texture   = pobj->tex_ref[skin];
    pgfx_inst->enviro    = pcap->enviro;
    pgfx_inst->alpha     = pcap->alpha;
    pgfx_inst->light     = pcap->light;
    pgfx_inst->sheen     = pcap->sheen;
    pgfx_inst->grnshift  = greensave;
    pgfx_inst->redshift  = redsave;
    pgfx_inst->blushift  = bluesave;

    // model parameters
    gfx_mad_instance::set_mad( pgfx_inst, pro_get_imad( profile ) );

    // upload these parameters to the reflection cache, but don't compute the matrix
    gfx_mad_instance::update_ref( pgfx_inst, 0, bfalse );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void gfx_mad_instance::get_tint( const gfx_mad_instance * pgfx_inst, GLfloat * tint, const BIT_FIELD bits )
{
    int i;
    float weight_sum;
    GLXvector4f local_tint;

    int local_alpha;
    int local_light;
    int local_sheen;
    int local_redshift;
    int local_grnshift;
    int local_blushift;

    if ( NULL == tint ) tint = local_tint;

    if ( HAS_SOME_BITS( bits, CHR_REFLECT ) )
    {
        // this is a reflection, use the reflected parameters
        local_alpha    = pgfx_inst->ref.alpha;
        local_light    = pgfx_inst->ref.light;
        local_sheen    = pgfx_inst->ref.sheen;
        local_redshift = pgfx_inst->ref.redshift;
        local_grnshift = pgfx_inst->ref.grnshift;
        local_blushift = pgfx_inst->ref.blushift;
    }
    else
    {
        // this is NOT a reflection, use the normal parameters
        local_alpha    = pgfx_inst->alpha;
        local_light    = pgfx_inst->light;
        local_sheen    = pgfx_inst->sheen;
        local_redshift = pgfx_inst->redshift;
        local_grnshift = pgfx_inst->grnshift;
        local_blushift = pgfx_inst->blushift;
    }

    // modify these values based on local character abilities
    local_alpha = get_local_alpha( local_alpha );
    local_light = get_local_light( local_light );

    // clear out the tint
    weight_sum = 0;
    for ( i = 0; i < 4; i++ ) tint[i] = 0;

    if ( HAS_SOME_BITS( bits, CHR_SOLID ) )
    {
        // solid characters are not blended onto the canvas
        // the alpha channel is not important
        weight_sum += 1.0f;

        tint[0] += 1.0f / ( 1 << local_redshift );
        tint[1] += 1.0f / ( 1 << local_grnshift );
        tint[2] += 1.0f / ( 1 << local_blushift );
        tint[3] += 1.0f;
    }

    if ( HAS_SOME_BITS( bits, CHR_ALPHA ) )
    {
        // alpha characters are blended onto the canvas using the alpha channel
        // the alpha channel is not important
        weight_sum += 1.0f;

        tint[0] += 1.0f / ( 1 << local_redshift );
        tint[1] += 1.0f / ( 1 << local_grnshift );
        tint[2] += 1.0f / ( 1 << local_blushift );
        tint[3] += local_alpha * INV_FF;
    }

    if ( HAS_SOME_BITS( bits, CHR_LIGHT ) )
    {
        // alpha characters are blended onto the canvas by adding their color
        // the more black the colors, the less visible the character
        // the alpha channel is not important

        weight_sum += 1.0f;

        if ( local_light < 255 )
        {
            tint[0] += local_light * INV_FF / ( 1 << local_redshift );
            tint[1] += local_light * INV_FF / ( 1 << local_grnshift );
            tint[2] += local_light * INV_FF / ( 1 << local_blushift );
        }

        tint[3] += 1.0f;
    }

    if ( HAS_SOME_BITS( bits, CHR_PHONG ) )
    {
        // Phong is essentially the same as light, but it is the
        // sheen that sets the effect

        float amount;

        weight_sum += 1.0f;

        amount = ( CLIP( local_sheen, 0, 15 ) << 4 ) / 240.0f;

        tint[0] += amount;
        tint[1] += amount;
        tint[2] += amount;
        tint[3] += 1.0f;
    }

    // average the tint
    if ( weight_sum != 0.0f && weight_sum != 1.0f )
    {
        for ( i = 0; i < 4; i++ )
        {
            tint[i] /= weight_sum;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_instance::set_mad( gfx_mad_instance * pgfx_inst, const MAD_REF & imad )
{
    /// \author BB
    /// \details  try to set the model used by the character instance.
    ///     If this fails, it leaves the old data. Just to be safe it
    ///     would be best to check whether the old modes is valid, and
    ///     if not, the data should be set to safe values...

    ego_mad * pmad;
    bool_t updated = bfalse;
    size_t vlst_size;

    if ( !LOADED_MAD( imad ) ) return bfalse;
    pmad = MadStack + imad;

    if ( NULL == pmad )
    {
        log_error( "Invalid pmad instance spawn. (Slot number %i)\n", imad.get_value() );
        return bfalse;
    }

    if ( NULL == pmad->md2_ptr )
    {
        log_error( "Invalid md2 pointer in MadStack[%i].md2_ptr\n", imad.get_value() );
        return bfalse;
    }

    // copy the md2 pointer
    pgfx_inst->md2_ptr = pmad->md2_ptr;

    // set the vertex size
    vlst_size = md2_get_numVertices( pgfx_inst->md2_ptr );
    if ( pgfx_inst->vrt_count != vlst_size )
    {
        updated = btrue;

        gfx_mad_instance::dealloc( pgfx_inst );
        gfx_mad_instance::alloc( pgfx_inst, vlst_size );
    }

    if ( updated )
    {
        // completely invalidate the pgfx_inst
        pgfx_inst->vrange.valid     = bfalse;
        pgfx_inst->mcache.valid     = bfalse;
        pgfx_inst->ref.matrix_valid = bfalse;
        pgfx_inst->ref.update_wld   = Uint32( -1 );

        // update the vertex and lighting cache
        gfx_mad_instance::clear_cache( pgfx_inst );
        gfx_mad_instance::update_vertices( pgfx_inst, pgfx_inst->pose, gfx_range( -1, -1 ), btrue );
    }

    return updated;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_chr_reflection_info * gfx_chr_reflection_info::init( gfx_chr_reflection_info * pcache )
{
    if ( NULL == pcache ) return pcache;

    pcache = clear( pcache );
    if ( NULL == pcache ) return pcache;

    /* add something here */

    return pcache;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_mad_matrix_data * gfx_mad_matrix_data::init( gfx_mad_matrix_data * mcache )
{
    /// \author BB
    /// \details  clear out the matrix cache data

    int cnt;

    if ( NULL == mcache ) return mcache;

    SDL_memset( mcache, 0, sizeof( *mcache ) );

    mcache->type_bits = MAT_UNKNOWN;
    mcache->grip_chr  = CHR_REF( MAX_CHR );
    for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
    {
        mcache->grip_verts[cnt] = 0xFFFF;
    }

    mcache->rotate.x = 0;
    mcache->rotate.y = 0;
    mcache->rotate.z = 0;

    return mcache;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_matrix_data::download( gfx_mad_matrix_data & mc_tmp, ego_chr * pchr )
{
    /// \author BB
    /// \details  grab the matrix cache data for a given character and put it into mc_tmp.

    bool_t handled;
    CHR_REF itarget, ichr;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;
    ichr = GET_REF_PCHR( pchr );

    handled = bfalse;
    itarget = CHR_REF( MAX_CHR );

    // initialize some parameters in case we fail
    mc_tmp.valid     = bfalse;
    mc_tmp.type_bits = MAT_UNKNOWN;

    mc_tmp.self_scale.x = mc_tmp.self_scale.y = mc_tmp.self_scale.z = pchr->fat;

    // handle the overlay first of all
    if ( !handled && pchr->is_overlay && ichr != pchr->ai.target && INGAME_CHR( pchr->ai.target ) )
    {
        // this will pretty much fail the cmp_matrix_data() every time...

        ego_chr * ptarget = ChrObjList.get_data_ptr( pchr->ai.target );

        // make sure we have the latst info from the target
        ego_chr::update_matrix( ptarget, btrue );

        // grab the matrix cache into from the character we are overlaying
        SDL_memcpy( &mc_tmp, &( ptarget->gfx_inst.mcache ), sizeof( gfx_mad_matrix_data ) );

        // just in case the overlay's matrix cannot be corrected
        // then treat it as if it is not an overlay
        handled = mc_tmp.valid;
    }

    // this will happen if the overlay "failed" or for any non-overlay character
    if ( !handled )
    {
        // assume that the "target" of the MAT_CHARACTER data will be the character itself
        itarget = GET_REF_PCHR( pchr );

        //---- update the MAT_WEAPON data
        if ( DEFINED_CHR( pchr->attachedto ) )
        {
            ego_chr * pmount = ChrObjList.get_data_ptr( pchr->attachedto );

            // make sure we have the latst info from the target
            ego_chr::update_matrix( pmount, btrue );

            // just in case the mount's matrix cannot be corrected
            // then treat it as if it is not mounted... yuck
            if ( pmount->gfx_inst.mcache.matrix_valid )
            {
                mc_tmp.valid     = btrue;
                ADD_BITS( mc_tmp.type_bits, MAT_WEAPON );        // add in the weapon data

                mc_tmp.grip_chr  = pchr->attachedto;
                mc_tmp.grip_slot = pchr->inwhich_slot;
                get_grip_verts( mc_tmp.grip_verts, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );

                itarget = pchr->attachedto;
            }
        }

        //---- update the MAT_CHARACTER data
        if ( DEFINED_CHR( itarget ) )
        {
            ego_chr * ptarget = ChrObjList.get_data_ptr( itarget );

            mc_tmp.valid   = btrue;
            ADD_BITS( mc_tmp.type_bits, MAT_CHARACTER );  // add in the MAT_CHARACTER-type data for the object we are "connected to"

            mc_tmp.rotate.x = CLIP_TO_16BITS( ptarget->ori.map_facing_x - MAP_TURN_OFFSET );
            mc_tmp.rotate.y = CLIP_TO_16BITS( ptarget->ori.map_facing_y - MAP_TURN_OFFSET );
            mc_tmp.rotate.z = ptarget->ori.facing_z;

            mc_tmp.pos = ego_chr::get_pos( ptarget );

            mc_tmp.grip_scale.x = mc_tmp.grip_scale.y = mc_tmp.grip_scale.z = ptarget->fat;
        }
    }

    return mc_tmp.valid;
}

bool_t gfx_mad_matrix_data::generate_weapon_matrix( gfx_mad_matrix_data & mc_tmp, ego_chr * pweap )
{
    /// \author ZZ
    /// \details  Request that the data in the matrix cache be used to create a "character matrix".
    ///               i.e. a matrix that is not being held by anything.

    fvec4_t   nupoint[GRIP_VERTS];
    int       iweap_points;

    ego_chr * pholder;
    gfx_mad_matrix_data * pweap_mcache;

    if ( !mc_tmp.valid || 0 == ( MAT_WEAPON & mc_tmp.type_bits ) ) return bfalse;

    if ( !DEFINED_PCHR( pweap ) ) return bfalse;
    pweap_mcache = &( pweap->gfx_inst.mcache );

    if ( !INGAME_CHR( mc_tmp.grip_chr ) ) return bfalse;
    pholder = ChrObjList.get_data_ptr( mc_tmp.grip_chr );

    // make sure that the matrix is invalid in case of an error
    pweap_mcache->matrix_valid = bfalse;

    // grab the grip points in world coordinates
    iweap_points = convert_grip_to_global_points( mc_tmp.grip_chr, mc_tmp.grip_verts, nupoint );

    if ( 4 == iweap_points )
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        pweap->gfx_inst.matrix = FourPoints( nupoint[0].v, nupoint[1].v, nupoint[2].v, nupoint[3].v, mc_tmp.self_scale.z );

        // update the weapon position
        ego_chr::set_pos( pweap, nupoint[3].v );

        SDL_memcpy( &( pweap->gfx_inst.mcache ), &mc_tmp, sizeof( gfx_mad_matrix_data ) );

        pweap_mcache->matrix_valid = btrue;
    }
    else if ( iweap_points > 0 )
    {
        // cannot find enough vertices. punt.
        // ignore the shape of the grip and just stick the character to the single mount point

        // update the character position
        ego_chr::set_pos( pweap, nupoint[0].v );

        // make sure we have the right data
        gfx_mad_matrix_data::download( mc_tmp, pweap );

        // add in the appropriate mods
        // this is a hybrid character and weapon matrix
        ADD_BITS( mc_tmp.type_bits, MAT_CHARACTER );

        // treat it like a normal character matrix
        gfx_mad_matrix_data::generate_character_matrix( mc_tmp, pweap );
    }

    return pweap_mcache->matrix_valid;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_matrix_data::generate_character_matrix( gfx_mad_matrix_data & mc_tmp, ego_chr * pchr )
{
    /// \author ZZ
    /// \details  Request that the matrix cache data be used to create a "weapon matrix".
    ///               i.e. a matrix that is attached to a specific grip.

    // only apply character matrices using this function
    if ( 0 == ( MAT_CHARACTER & mc_tmp.type_bits ) ) return bfalse;

    pchr->gfx_inst.mcache.matrix_valid = bfalse;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( pchr->stickybutt )
    {
        pchr->gfx_inst.matrix = ScaleXYZRotateXYZTranslate_SpaceFixed( mc_tmp.self_scale.x, mc_tmp.self_scale.y, mc_tmp.self_scale.z,
                                TO_TURN( mc_tmp.rotate.z ), TO_TURN( mc_tmp.rotate.x ), TO_TURN( mc_tmp.rotate.y ),
                                mc_tmp.pos.x, mc_tmp.pos.y, mc_tmp.pos.z );
    }
    else
    {
        pchr->gfx_inst.matrix = ScaleXYZRotateXYZTranslate_BodyFixed( mc_tmp.self_scale.x, mc_tmp.self_scale.y, mc_tmp.self_scale.z,
                                TO_TURN( mc_tmp.rotate.z ), TO_TURN( mc_tmp.rotate.x ), TO_TURN( mc_tmp.rotate.y ),
                                mc_tmp.pos.x, mc_tmp.pos.y, mc_tmp.pos.z );
    }

    SDL_memcpy( &( pchr->gfx_inst.mcache ), &mc_tmp, sizeof( gfx_mad_matrix_data ) );

    pchr->gfx_inst.mcache.matrix_valid = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t gfx_mad_matrix_data::generate_matrix( gfx_mad_matrix_data & mc_tmp, ego_chr * pchr )
{
    /// \author BB
    /// \details  request that the info in the matrix cache mc_tmp, be used to
    ///               make a matrix for the character pchr.

    bool_t applied = bfalse;

    if ( !DEFINED_PCHR( pchr ) || !mc_tmp.valid ) return bfalse;

    if ( 0 != ( MAT_WEAPON & mc_tmp.type_bits ) )
    {
        if ( DEFINED_CHR( mc_tmp.grip_chr ) )
        {
            applied = gfx_mad_matrix_data::generate_weapon_matrix( mc_tmp, pchr );
        }
        else
        {
            gfx_mad_matrix_data * mcache = &( pchr->gfx_inst.mcache );

            // !!!the mc_tmp was mis-labeled as a MAT_WEAPON!!!
            make_one_character_matrix( GET_REF_PCHR( pchr ) );

            // recover the mcache values from the character
            ADD_BITS( mcache->type_bits, MAT_CHARACTER );
            if ( mcache->matrix_valid )
            {
                mcache->valid     = btrue;
                mcache->type_bits = MAT_CHARACTER;

                mcache->self_scale.x =
                    mcache->self_scale.y =
                        mcache->self_scale.z = pchr->fat;

                mcache->grip_scale = mcache->self_scale;

                mcache->rotate.x = CLIP_TO_16BITS( pchr->ori.map_facing_x - MAP_TURN_OFFSET );
                mcache->rotate.y = CLIP_TO_16BITS( pchr->ori.map_facing_y - MAP_TURN_OFFSET );
                mcache->rotate.z = pchr->ori.facing_z;

                mcache->pos = ego_chr::get_pos( pchr );

                applied = btrue;
            }
        }
    }
    else if ( 0 != ( MAT_CHARACTER & mc_tmp.type_bits ) )
    {
        applied = gfx_mad_matrix_data::generate_character_matrix( mc_tmp, pchr );
    }

    if ( applied )
    {
        apply_reflection_matrix( &( pchr->gfx_inst ), pchr->enviro.grid_level );
    }

    return applied;
}

//--------------------------------------------------------------------------------------------
int cmp_matrix_data( const void * vlhs, const void * vrhs )
{
    /// \author BB
    /// \details  check for differences between the data pointed to
    ///     by vlhs and vrhs, assuming that they point to gfx_mad_matrix_data data.
    ///
    ///    The function is implemented this way so that in principle
    ///    if could be used in a function like SDL_qsort().
    ///
    ///    We could almost certainly make something easier and quicker by
    ///    using the function SDL_memcmp()

    int   itmp, cnt;
    float ftmp;

    gfx_mad_matrix_data * plhs = ( gfx_mad_matrix_data * )vlhs;
    gfx_mad_matrix_data * prhs = ( gfx_mad_matrix_data * )vrhs;

    // handle problems with pointers
    if ( plhs == prhs )
    {
        return 0;
    }
    else if ( NULL == plhs )
    {
        return 1;
    }
    else if ( NULL == prhs )
    {
        return -1;
    }

    // handle one of both if the matrix caches being invalid
    if ( !plhs->valid && !prhs->valid )
    {
        return 0;
    }
    else if ( !plhs->valid )
    {
        return 1;
    }
    else if ( !prhs->valid )
    {
        return -1;
    }

    // handle differences in the type
    itmp = plhs->type_bits - prhs->type_bits;
    if ( 0 != itmp ) goto cmp_matrix_data_end;

    //---- check for differences in the MAT_WEAPON data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) )
    {
        itmp = ego_sint( plhs->grip_chr.get_value() ) - ego_sint( prhs->grip_chr.get_value() );
        if ( 0 != itmp ) goto cmp_matrix_data_end;

        itmp = ego_sint( plhs->grip_slot ) - ego_sint( prhs->grip_slot );
        if ( 0 != itmp ) goto cmp_matrix_data_end;

        for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            itmp = ego_sint( plhs->grip_verts[cnt] ) - ego_sint( prhs->grip_verts[cnt] );
            if ( 0 != itmp ) goto cmp_matrix_data_end;
        }

        // handle differences in the scale of our mount
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->grip_scale.v[cnt] - prhs->grip_scale.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_data_end; }
        }
    }

    //---- check for differences in the MAT_CHARACTER data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in the "Euler" rotation angles in 16-bit form
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->rotate.v[cnt] - prhs->rotate.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_data_end; }
        }

        // handle differences in the translate vector
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->pos.v[cnt] - prhs->pos.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_data_end; }
        }
    }

    //---- check for differences in the shared data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) || HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in our own scale
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->self_scale.v[cnt] - prhs->self_scale.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_data_end; }
        }
    }

    // if it got here, the data is all the same
    itmp = 0;

cmp_matrix_data_end:

    return SGN( itmp );
}

