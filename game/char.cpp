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

/// \file char.c
/// \brief Implementation of character functions
/// \details

#include "char.inl"
#include "ChrList.inl"

#include "mad.h"

#include "log.h"
#include "script.h"
#include "menu.h"
#include "sound.h"
#include "camera.h"
#include "input.h"
#include "passage.h"
#include "graphic.h"
#include "game.h"
#include "texture.h"
#include "ui.h"
#include "collision.h"                    // Only for detach_character_from_platform()
#include "quest.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include "md2.inl"
#include "mesh.inl"
#include "physics.inl"
#include "egoboo_math.inl"

#include <float.h>
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_chr_anim_data
{
    bool_t allowed;
    int    action;
    int    lip;
    float  speed;
};

int cmp_chr_anim_data( void const * vp_lhs, void const * vp_rhs );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static IDSZ    inventory_idsz[INVEN_COUNT];

PROFILE_DECLARE( move_one_character );
PROFILE_DECLARE( keep_weapons_with_holders );
PROFILE_DECLARE( attach_all_particles );
PROFILE_DECLARE( make_all_character_matrices );

PROFILE_DECLARE( move_one_character_get_environment );
PROFILE_DECLARE( move_one_character_do_fluid_friction );
PROFILE_DECLARE( move_one_character_do_voluntary_flying );
PROFILE_DECLARE( move_one_character_do_voluntary );
PROFILE_DECLARE( move_one_character_do_involuntary );
PROFILE_DECLARE( move_one_character_do_orientation );
PROFILE_DECLARE( move_one_character_do_z_motion );
PROFILE_DECLARE( move_one_character_do_animation );
PROFILE_DECLARE( move_one_character_limit_flying );
PROFILE_DECLARE( move_one_character_do_jump );
PROFILE_DECLARE( move_one_character_do_floor );

PROFILE_DECLARE( chr_do_latch_button );
PROFILE_DECLARE( move_one_character_do_flying );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
t_stack< ego_cap, MAX_PROFILE  > CapStack;
t_stack< ego_team, TEAM_MAX  > TeamStack;

int ego_chr::stoppedby_tests = 0;
int ego_chr::pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static CHR_REF chr_pack_has_a_stack( const CHR_REF & item, const CHR_REF & character );
static bool_t  chr_pack_add_item( const CHR_REF & item, const CHR_REF & character );
static bool_t  chr_pack_remove_item( const CHR_REF & ichr, const CHR_REF & iparent, const CHR_REF & iitem );
static CHR_REF chr_pack_get_item( const CHR_REF & chr_ref, grip_offset_t grip_off, const bool_t ignorekurse );

static bool_t set_weapongrip( const CHR_REF & iitem, const CHR_REF & iholder, const Uint16 vrt_off );

static BBOARD_REF chr_add_billboard( const CHR_REF & ichr, const Uint32 lifetime_secs );

static ego_chr * resize_one_character( ego_chr * pchr );

void cleanup_one_character( ego_chr * pchr );

static void chr_log_script_time( const CHR_REF & ichr );

static bool_t update_chr_darkvision( const CHR_REF & character );

static const float traction_min = 0.2f;

static ego_bundle_chr & move_one_character_get_environment( ego_bundle_chr & bdl, ego_chr_environment * penviro );
static ego_bundle_chr & move_one_character_do_fluid_friction( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_voluntary_flying( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_voluntary( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_involuntary( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_orientation( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_z_motion( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_animation( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_limit_flying( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_jump( ego_bundle_chr & bdl );
static ego_bundle_chr & move_one_character_do_floor( ego_bundle_chr & bdl );

static bool_t pack_validate( ego_pack * ppack );

static fvec2_t chr_get_diff( ego_chr * pchr, const float test_pos[], const float center_pressure );
//static float   get_mesh_pressure( ego_chr * pchr, const float test_pos[] );

static egoboo_rv chr_invalidate_instances( ego_chr * pchr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void character_system_begin()
{
    ChrObjList.init();
    init_all_cap();
}

//--------------------------------------------------------------------------------------------
void character_system_end()
{
    release_all_cap();
    ChrObjList.deinit();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_count_free()
{
    return ChrObjList.free_count();
}

//--------------------------------------------------------------------------------------------
void flash_character_height( const CHR_REF & character, const Uint8 valuelow, const Sint16 low,
                             const Uint8 valuehigh, const Sint16 high )
{
    /// \author ZZ
    /// \details  This function sets a character's lighting depending on vertex height...
    ///    Can make feet dark and head light...

    Uint32 cnt;
    Sint16 z;

    ego_mad * pmad;
    gfx_mad_instance * pgfx_inst;

    pgfx_inst = ego_chr::get_pinstance( character );
    if ( NULL == pgfx_inst ) return;

    pmad = ego_chr::get_pmad( character );
    if ( NULL == pmad ) return;

    for ( cnt = 0; cnt < pgfx_inst->vrt_count; cnt++ )
    {
        z = pgfx_inst->vrt_lst[cnt].pos[ZZ];

        if ( z < low )
        {
            pgfx_inst->color_amb = valuelow;
        }
        else
        {
            if ( z > high )
            {
                pgfx_inst->color_amb = valuehigh;
            }
            else
            {
                pgfx_inst->color_amb = ( valuehigh * ( z - low ) / ( high - low ) ) +
                                       ( valuelow * ( high - z ) / ( high - low ) );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
    /// \author ZZ
    /// \details  This function keeps weapons near their holders

    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        CHR_REF iattached = pchr->attachedto;
        ego_chr * pattached = ChrObjList.get_allocated_data_ptr( iattached );

        if ( INGAME_PCHR( pattached ) )
        {
            ego_chr * pattached = ChrObjList.get_data_ptr( iattached );

            // Keep in hand weapons with iattached
            if ( ego_chr::matrix_valid( pchr ) )
            {
                ego_chr::set_pos( pchr, mat_getTranslate_v( pchr->gfx_inst.matrix ) );
            }
            else
            {
                ego_chr::set_pos( pchr, ego_chr::get_pos_v( pattached ) );
            }

            pchr->ori.facing_z = pattached->ori.facing_z;

            // Copy this stuff ONLY if it's a weapon, not for mounts
            if ( pattached->transferblend && pchr->isitem )
            {

                // Items become partially invisible in hands of players
                if ( IS_PLAYER_PCHR( pattached ) && 255 != pattached->gfx_inst.alpha )
                {
                    ego_chr::set_alpha( pchr, SEEINVISIBLE );
                }
                else
                {
                    // Only if not naturally transparent
                    if ( 255 == pchr->alpha_base )
                    {
                        ego_chr::set_alpha( pchr, pattached->gfx_inst.alpha );
                    }
                    else
                    {
                        ego_chr::set_alpha( pchr, pchr->alpha_base );
                    }
                }

                // Do light too
                if ( IS_PLAYER_PCHR( pattached ) && 255 != pattached->gfx_inst.light )
                {
                    ego_chr::set_light( pchr, SEEINVISIBLE );
                }
                else
                {
                    // Only if not naturally transparent
                    if ( 255 == ego_chr::get_pcap( cnt )->light )
                    {
                        ego_chr::set_light( pchr, pattached->gfx_inst.light );
                    }
                    else
                    {
                        ego_chr::set_light( pchr, pchr->light_base );
                    }
                }
            }
        }
        else
        {
            pchr->attachedto = CHR_REF( MAX_CHR );

            // Keep inventory with iattached
            if ( !pchr->pack.is_packed )
            {
                CHR_REF ipacked = iattached;
                PACK_BEGIN_LOOP( ipacked, pchr->pack.next )
                {
                    ego_chr * tmp_chr = ChrObjList.get_allocated_data_ptr( ipacked );
                    if ( NULL != tmp_chr )
                    {
                        ego_chr::set_pos( tmp_chr, ego_chr::get_pos_v( pchr ) );

                        // copy olds to make SendMessageNear work
                        tmp_chr->pos_old = pchr->pos_old;
                    }
                }
                PACK_END_LOOP( ipacked );
            }
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( const CHR_REF & ichr )
{
    /// \author ZZ
    /// \details  This function sets one character's matrix

    ego_chr * pchr;
    gfx_mad_instance * pgfx_inst;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return;

    pgfx_inst = &( pchr->gfx_inst );

    // invalidate this matrix
    pgfx_inst->mcache.matrix_valid = bfalse;

    if ( pchr->is_overlay )
    {
        // This character is an overlay and its ai.target points to the object it is overlaying
        // Overlays are kept with their target...
        if ( INGAME_CHR( pchr->ai.target ) )
        {
            ego_chr * ptarget = ChrObjList.get_data_ptr( pchr->ai.target );

            ego_chr::set_pos( pchr, ego_chr::get_pos_v( ptarget ) );

            // copy the matrix
            CopyMatrix( &( pgfx_inst->matrix ), &( ptarget->gfx_inst.matrix ) );

            // copy the matrix data
            pgfx_inst->mcache = ptarget->gfx_inst.mcache;
        }
    }
    else
    {
        if ( pchr->stickybutt )
        {
            pgfx_inst->matrix = ScaleXYZRotateXYZTranslate_SpaceFixed( pchr->fat, pchr->fat, pchr->fat,
                                TO_TURN( pchr->ori.facing_z ),
                                TO_TURN( pchr->ori.map_facing_x - MAP_TURN_OFFSET ),
                                TO_TURN( pchr->ori.map_facing_y - MAP_TURN_OFFSET ),
                                pchr->pos.x, pchr->pos.y, pchr->pos.z );
        }
        else
        {
            pgfx_inst->matrix = ScaleXYZRotateXYZTranslate_BodyFixed( pchr->fat, pchr->fat, pchr->fat,
                                TO_TURN( pchr->ori.facing_z ),
                                TO_TURN( pchr->ori.map_facing_x - MAP_TURN_OFFSET ),
                                TO_TURN( pchr->ori.map_facing_y - MAP_TURN_OFFSET ),
                                pchr->pos.x, pchr->pos.y, pchr->pos.z );
        }

        pgfx_inst->mcache.valid        = btrue;
        pgfx_inst->mcache.matrix_valid = btrue;
        pgfx_inst->mcache.type_bits    = MAT_CHARACTER;

        pgfx_inst->mcache.self_scale.x = pchr->fat;
        pgfx_inst->mcache.self_scale.y = pchr->fat;
        pgfx_inst->mcache.self_scale.z = pchr->fat;

        pgfx_inst->mcache.rotate.x = CLIP_TO_16BITS( pchr->ori.map_facing_x - MAP_TURN_OFFSET );
        pgfx_inst->mcache.rotate.y = CLIP_TO_16BITS( pchr->ori.map_facing_y - MAP_TURN_OFFSET );
        pgfx_inst->mcache.rotate.z = pchr->ori.facing_z;

        pgfx_inst->mcache.pos = ego_chr::get_pos( pchr );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_log_script_time( const CHR_REF & ichr )
{
    // log the amount of script time that this object used up

    ego_chr * pchr;
    ego_cap * pcap;
    FILE * ftmp;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !DEFINED_PCHR( pchr ) ) return;

    if ( pchr->ai.dbg_pro.count <= 0 ) return;

    pcap = ego_chr::get_pcap( ichr );
    if ( NULL == pcap ) return;

    ftmp = fopen( vfs_resolveWriteFilename( "/debug/script_timing.txt" ), "a+" );
    if ( NULL != ftmp )
    {
        fprintf( ftmp, "update == %d\tindex == %d\tname == \"%s\"\tclassname == \"%s\"\ttotal_time == %e\ttotal_calls == %f\n",
                 update_wld, ichr.get_value(), pchr->name, pcap->classname,
                 pchr->ai.dbg_pro.time, pchr->ai.dbg_pro.count );
        fflush( ftmp );
        fclose( ftmp );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character_in_game( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function sticks a character back on the free character stack
    ///
    /// \note This should only be called by cleanup_all_characters() or free_inventory_in_game()

    ego_cap * pcap;
    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !DEFINED_PCHR( pchr ) ) return;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return;

    // Remove from stat list
    if ( pchr->draw_stats )
    {
        bool_t stat_found;

        pchr->draw_stats = bfalse;

        StatList.remove( character );
    }

    // Make sure everyone knows it died
    CHR_BEGIN_LOOP_INGAME( ichr_listener, pchr_listener )
    {
        ego_ai_state * pai;

        if ( ichr_listener == character ) continue;
        pai = ego_chr::get_pai( ichr_listener );

        if ( pai->target == character )
        {
            ADD_BITS( pai->alert, ALERTIF_TARGETKILLED );
            pai->target = MAX_CHR;
        }

        if ( ego_chr::get_pteam( ichr_listener )->leader == character )
        {
            ADD_BITS( pai->alert, ALERTIF_LEADERKILLED );
        }
    }
    CHR_END_LOOP();

    // Handle the team
    if ( pchr->alive && !pcap->invictus && TeamStack[pchr->baseteam].morale > 0 )
    {
        TeamStack[pchr->baseteam].morale--;
    }

    if ( TeamStack[pchr->team].leader == character )
    {
        TeamStack[pchr->team].leader = NOLEADER;
    }

    // remove any attached particles
    disaffirm_attached_particles( character );

    // actually get rid of the character
    ChrObjList.free_one( character );
}

//--------------------------------------------------------------------------------------------
void free_inventory_in_game( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function frees every item in the character's inventory
    ///
    /// \note this should only be called by cleanup_all_characters()

    CHR_REF cnt;

    ego_obj_chr * pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !DEFINED_PCHR( pchr ) ) return;

    PACK_BEGIN_LOOP( cnt, pchr->pack.next )
    {
        free_one_character_in_game( cnt );
    }
    PACK_END_LOOP( cnt );

    // set the inventory to the "empty" state
    pchr->pack.count = 0;
    pchr->pack.next  = CHR_REF( MAX_CHR );
}

//--------------------------------------------------------------------------------------------
ego_prt * place_particle_at_vertex( ego_prt * pprt, const CHR_REF & character, const int vertex_offset )
{
    /// \author ZZ
    /// \details  This function sets one particle's position to be attached to a character.
    ///    It will kill the particle if the character is no longer around

    int     vertex;
    fvec4_t point[1], nupoint[1];

    ego_chr * pchr;

    if ( !DEFINED_PPRT( pprt ) ) return pprt;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) )
    {
        goto place_particle_at_vertex_fail;
    }

    // Check validity of attachment
    if ( pchr->pack.is_packed )
    {
        goto place_particle_at_vertex_fail;
    }

    // Do we have a matrix???
    if ( !ego_chr::matrix_valid( pchr ) )
    {
        ego_chr::update_matrix( pchr, btrue );
    }

    // Do we have a matrix???
    if ( ego_chr::matrix_valid( pchr ) )
    {
        // Transform the weapon vertex_offset from model to world space
        ego_mad * pmad = ego_chr::get_pmad( character );

        if ( vertex_offset == GRIP_ORIGIN )
        {
            fvec3_t tmp_pos = VECT3( pchr->gfx_inst.matrix.CNV( 3, 0 ), pchr->gfx_inst.matrix.CNV( 3, 1 ), pchr->gfx_inst.matrix.CNV( 3, 2 ) );
            ego_prt::set_pos( pprt, tmp_pos.v );

            return pprt;
        }

        vertex = 0;
        if ( NULL != pmad )
        {
            vertex = ego_sint( pchr->gfx_inst.vrt_count ) - vertex_offset;

            // do the automatic update
            gfx_mad_instance::update_vertices( &( pchr->gfx_inst ), pchr->mad_inst.state, gfx_range( vertex, vertex ), bfalse );

            // Calculate vertex_offset point locations with linear interpolation and other silly things
            point[0].x = pchr->gfx_inst.vrt_lst[vertex].pos[XX];
            point[0].y = pchr->gfx_inst.vrt_lst[vertex].pos[YY];
            point[0].z = pchr->gfx_inst.vrt_lst[vertex].pos[ZZ];
            point[0].w = 1.0f;
        }
        else
        {
            point[0].x =
                point[0].y =
                    point[0].z = 0.0f;
            point[0].w = 1.0f;
        }

        // Do the transform
        TransformVertices( &( pchr->gfx_inst.matrix ), point, nupoint, 1 );

        ego_prt::set_pos( pprt, nupoint[0].v );
    }
    else
    {
        // No matrix, so just wing it...
        ego_prt::set_pos( pprt, ego_chr::get_pos_v( pchr ) );
    }

    return pprt;

place_particle_at_vertex_fail:

    ego_obj_prt::request_terminate( GET_REF_PPRT( pprt ) );

    return NULL;
}

//--------------------------------------------------------------------------------------------
void make_all_character_matrices( const bool_t do_physics )
{
    /// \author ZZ
    /// \details  This function makes all of the character's matrices

    //int cnt;
    //bool_t done;

    //// blank the accumulators
    //for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    //{
    //    ChrObjList.get_data_ref(ichr).phys.apos_plat.x = 0.0f;
    //    ChrObjList.get_data_ref(ichr).phys.apos_plat.y = 0.0f;
    //    ChrObjList.get_data_ref(ichr).phys.apos_plat.z = 0.0f;

    //    ChrObjList.get_data_ref(ichr).phys.apos_coll.x = 0.0f;
    //    ChrObjList.get_data_ref(ichr).phys.apos_coll.y = 0.0f;
    //    ChrObjList.get_data_ref(ichr).phys.apos_coll.z = 0.0f;

    //    ChrObjList.get_data_ref(ichr).phys.avel.x = 0.0f;
    //    ChrObjList.get_data_ref(ichr).phys.avel.y = 0.0f;
    //    ChrObjList.get_data_ref(ichr).phys.avel.z = 0.0f;
    //}

    // just call ego_chr::update_matrix on every character
    CHR_BEGIN_LOOP_PROCESSING( ichr, pchr )
    {
        ego_chr::update_matrix( pchr, btrue );
    }
    CHR_END_LOOP();

    ////if ( do_physics )
    ////{
    ////    // accumulate the accumulators
    ////    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    ////    {
    ////        float nrm[2];
    ////        float tmpx, tmpy, tmpz;
    ////        fvec3_t tmp_pos;
    ////        ego_chr * pchr;

    ////        if ( !INGAME_CHR(ichr) ) continue;
    ////        pchr = ChrObjList.get_data_ptr(ichr);

    ////        tmp_pos = ego_chr::get_pos( pchr );

    ////        // do the "integration" of the accumulated accelerations
    ////        pchr->vel.x += pchr->phys.avel.x;
    ////        pchr->vel.y += pchr->phys.avel.y;
    ////        pchr->vel.z += pchr->phys.avel.z;

    ////        // do the "integration" on the position
    ////        if ( SDL_abs(pchr->phys.apos_coll.x) > 0 )
    ////        {
    ////            tmpx = tmp_pos.x;
    ////            tmp_pos.x += pchr->phys.apos_coll.x;
    ////            if ( chr_hit_wall(ichr, nrm, NULL) )
    ////            {
    ////                // restore the old values
    ////                tmp_pos.x = tmpx;
    ////            }
    ////            else
    ////            {
    ////                // pchr->vel.x += pchr->phys.apos_coll.x;
    ////                pchr->safe_pos.x = tmpx;
    ////            }
    ////        }

    ////        if ( SDL_abs(pchr->phys.apos_coll.y) > 0 )
    ////        {
    ////            tmpy = tmp_pos.y;
    ////            tmp_pos.y += pchr->phys.apos_coll.y;
    ////            if ( chr_hit_wall(ichr, nrm, NULL) )
    ////            {
    ////                // restore the old values
    ////                tmp_pos.y = tmpy;
    ////            }
    ////            else
    ////            {
    ////                // pchr->vel.y += pchr->phys.apos_coll.y;
    ////                pchr->safe_pos.y = tmpy;
    ////            }
    ////        }

    ////        if ( SDL_abs(pchr->phys.apos_coll.z) > 0 )
    ////        {
    ////            tmpz = tmp_pos.z;
    ////            tmp_pos.z += pchr->phys.apos_coll.z;
    ////            if ( tmp_pos.z < pchr->enviro.level )
    ////            {
    ////                // restore the old values
    ////                tmp_pos.z = tmpz;
    ////            }
    ////            else
    ////            {
    ////                // pchr->vel.z += pchr->phys.apos_coll.z;
    ////                pchr->safe_pos.z = tmpz;
    ////            }
    ////        }

    ////        if ( 0 == chr_hit_wall(ichr, nrm, NULL) )
    ////        {
    ////            pchr->safe_valid = btrue;
    ////        }
    ////        ego_chr::set_pos( pchr, tmp_pos.v );
    ////    }

    ////    // fix the matrix positions
    ////    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    ////    {
    ////        ego_chr * pchr;

    ////        if ( !INGAME_CHR(ichr) ) continue;
    ////        pchr = ChrObjList.get_data_ptr(ichr);

    ////        if( !pchr->gfx_inst.mcache.valid ) continue;

    ////        pchr->gfx_inst.matrix.CNV( 3, 0 ) = pchr->pos.x;
    ////        pchr->gfx_inst.matrix.CNV( 3, 1 ) = pchr->pos.y;
    ////        pchr->gfx_inst.matrix.CNV( 3, 2 ) = pchr->pos.z;
    ////    }
    ////}
}

//--------------------------------------------------------------------------------------------
void free_all_chraracters()
{
    /// \author ZZ
    /// \details  This function resets the character allocation list

    // free all the characters
    ChrObjList.free_all();

    // free_all_players
    PlaDeque.reinit();
    net_stats.pla_count_local = 0;

    // free_all_stats
    StatList.count = 0;
}

//--------------------------------------------------------------------------------------------
float chr_get_mesh_pressure( ego_chr * pchr, const float test_pos[] )
{
    float retval = 0.0f;
    float radius = 0.0f;

    if ( !DEFINED_PCHR( pchr ) ) return retval;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    /// \note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( ego_mpd::grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::get_pressure( PMesh, test_pos, radius, pchr->stoppedby );
    }
    ego_chr::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_chr::pressure_tests += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
fvec2_t chr_get_diff( ego_chr * pchr, const float test_pos[], const float center_pressure )
{
    fvec2_t retval = ZERO_VECT2;
    float   radius;

    if ( !DEFINED_PCHR( pchr ) ) return retval;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    /// \note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( ego_mpd::grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::get_diff( PMesh, test_pos, radius, center_pressure, pchr->stoppedby );
    }
    ego_chr::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_chr::pressure_tests += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD chr_hit_wall( ego_chr * pchr, const float test_pos[], float nrm[], float * pressure )
{
    /// \author ZZ
    /// \details  This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    BIT_FIELD    retval;
    float        radius;

    if ( !DEFINED_PCHR( pchr ) ) return 0;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return 0;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    /// \note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( ego_mpd::grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::hit_wall( PMesh, test_pos, radius, pchr->stoppedby, nrm, pressure );
    }
    ego_chr::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_chr::pressure_tests  += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_test_wall( ego_chr * pchr, const float test_pos[] )
{
    /// \author ZZ
    /// \details  This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    bool_t retval;
    float  radius;

    if ( !PROCESSING_PCHR( pchr ) ) return 0;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return bfalse;

    // calculate the radius based on whether the character is on camera
    /// \note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( ego_mpd::grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // do the wall test
    ego_mpd::mpdfx_tests = 0;
    ego_mpd::bound_tests = 0;
    ego_mpd::pressure_tests = 0;
    {
        retval = ego_mpd::test_wall( PMesh, test_pos, radius, pchr->stoppedby, NULL );
    }
    ego_chr::stoppedby_tests += ego_mpd::mpdfx_tests;
    ego_chr::pressure_tests += ego_mpd::pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function fixes a character's max acceleration

    ENC_REF enchant;
    ego_chr * pchr;
    ego_cap * pcap;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Okay, remove all acceleration enchants
    enchant = pchr->firstenchant;
    while ( enchant != MAX_ENC )
    {
        ego_enc::remove_add( enchant, ADDACCEL );
        enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
    }

    // Set the starting value
    pchr->maxaccel_reset = 0;
    pcap = ego_chr::get_pcap( character );
    if ( NULL != pcap )
    {
        pchr->maxaccel = pchr->maxaccel_reset = pcap->maxaccel[pchr->skin];
    }

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Put the acceleration enchants back on
    enchant = pchr->firstenchant;
    while ( enchant != MAX_ENC )
    {
        ego_enc::apply_add( enchant, ADDACCEL, ego_enc::get_ieve( enchant ) );
        enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
    }
}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_mount( const CHR_REF & character, const Uint8 ignorekurse, const Uint8 doshop )
{
    /// \author ZZ
    /// \details  This function drops an item

    CHR_REF mount;
    Uint16  hand;
    ENC_REF enchant;
    bool_t  inshop;
    ego_chr * pchr, * pmount;

    // Make sure the character is valid
    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    // Make sure the character is mounted
    mount = pchr->attachedto;
    pmount = ChrObjList.get_allocated_data_ptr( mount );
    if ( !INGAME_PCHR( pmount ) ) return bfalse;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && pchr->iskursed && pmount->alive && pchr->isitem )
    {
        ADD_BITS( pchr->ai.alert, ALERTIF_NOTDROPPED );
        return bfalse;
    }

    // set the dismount timer
    if ( !pchr->isitem ) pchr->dismount_timer  = PHYS_DISMOUNT_TIME;
    pchr->dismount_object = mount;

    // Figure out which hand it's in
    hand = pchr->inwhich_slot;

    // Rip 'em apart
    pchr->attachedto = CHR_REF( MAX_CHR );
    if ( pmount->holdingwhich[SLOT_LEFT] == character )
        pmount->holdingwhich[SLOT_LEFT] = CHR_REF( MAX_CHR );

    if ( pmount->holdingwhich[SLOT_RIGHT] == character )
        pmount->holdingwhich[SLOT_RIGHT] = CHR_REF( MAX_CHR );

    if ( pchr->alive )
    {
        // play the falling animation...
        ego_chr::play_action( pchr, ACTION_JB + hand, bfalse );
    }
    else if ( pchr->mad_inst.action.which < ACTION_KA || pchr->mad_inst.action.which > ACTION_KD )
    {
        // play the "killed" animation...
        ego_chr::play_action( pchr, ACTION_KA + generate_randmask( 0, 3 ), bfalse );
        pchr->mad_inst.action.keep = btrue;
    }

    // Set the positions
    if ( ego_chr::matrix_valid( pchr ) )
    {
        ego_chr::set_pos( pchr, mat_getTranslate_v( pchr->gfx_inst.matrix ) );
    }
    else
    {
        ego_chr::set_pos( pchr, ego_chr::get_pos_v( pmount ) );
    }

    // Make sure it's not dropped in a wall...
    if ( chr_test_wall( pchr, NULL ) )
    {
        fvec3_t pos_tmp;

        pos_tmp.x = pmount->pos.x;
        pos_tmp.y = pmount->pos.y;
        pos_tmp.z = pchr->pos.z;

        ego_chr::set_pos( pchr, pos_tmp.v );

        ego_chr::update_breadcrumb( pchr, btrue );
    }

    // Check for shop passages
    inshop = bfalse;
    if ( doshop )
    {
        inshop = do_shop_drop( mount, character );
    }

    // Make sure it works right
    pchr->hitready = btrue;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        pchr->vel.x = 0;
        pchr->vel.y = 0;
    }
    else
    {
        pchr->vel.x = pmount->vel.x;
        pchr->vel.y = pmount->vel.y;
    }

    pchr->vel.z = DROPZVEL;

    // Turn looping off
    pchr->mad_inst.action.loop = bfalse;

    // Reset the team if it is a mount
    if ( pmount->ismount )
    {
        pmount->team = pmount->baseteam;
        ADD_BITS( pmount->ai.alert, ALERTIF_DROPPED );
    }

    pchr->team = pchr->baseteam;
    ADD_BITS( pchr->ai.alert, ALERTIF_DROPPED );

    // Reset transparency
    if ( pchr->isitem && pmount->transferblend )
    {
        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // Okay, reset transparency
        enchant = pchr->firstenchant;
        while ( enchant != MAX_ENC )
        {
            ego_enc::remove_set( enchant, SETALPHABLEND );
            ego_enc::remove_set( enchant, SETLIGHTBLEND );

            enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
        }

        ego_chr::set_alpha( pchr, pchr->alpha_base );
        ego_chr::set_light( pchr, pchr->light_base );

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // apply the blend enchants
        enchant = pchr->firstenchant;
        while ( enchant != MAX_ENC )
        {
            PRO_REF ipro = ego_enc::get_ipro( enchant );

            if ( LOADED_PRO( ipro ) )
            {
                ego_enc::apply_set( enchant, SETALPHABLEND, ipro );
                ego_enc::apply_set( enchant, SETLIGHTBLEND, ipro );
            }

            enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
        }
    }

    // Set twist
    pchr->ori.map_facing_y = MAP_TURN_OFFSET;
    pchr->ori.map_facing_x = MAP_TURN_OFFSET;

    ego_chr::update_matrix( pchr, btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function fixes an item's transparency

    CHR_REF mount;
    ENC_REF enchant;
    ego_chr * pchr, * pmount;

    // Make sure the character is valid
    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    // Make sure the character is mounted
    mount = pchr->attachedto;
    pmount = ChrObjList.get_allocated_data_ptr( mount );
    if ( !INGAME_PCHR( pmount ) ) return;

    if ( pchr->isitem && pmount->transferblend )
    {
        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // Okay, reset transparency
        enchant = pchr->firstenchant;
        while ( enchant != MAX_ENC )
        {
            ego_enc::remove_set( enchant, SETALPHABLEND );
            ego_enc::remove_set( enchant, SETLIGHTBLEND );

            enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
        }

        ego_chr::set_alpha( pchr, pchr->alpha_base );
        ego_chr::set_light( pchr, pchr->light_base );

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        enchant = pchr->firstenchant;
        while ( enchant != MAX_ENC )
        {
            PRO_REF ipro = ego_enc::get_ipro( enchant );

            if ( LOADED_PRO( ipro ) )
            {
                ego_enc::apply_set( enchant, SETALPHABLEND, ipro );
                ego_enc::apply_set( enchant, SETLIGHTBLEND, ipro );
            }

            enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
        }
    }
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount( const CHR_REF & iitem, const CHR_REF & iholder, grip_offset_t grip_off )
{
    /// \author ZZ
    /// \details  This function attaches one character/item to another ( the holder/mount )
    ///    at a certain vertex offset ( grip_off )

    slot_t slot;

    ego_chr * pitem, * pholder;

    // Make sure the character/item is valid
    // this could be called before the item is fully instantiated
    pitem = ChrObjList.get_allocated_data_ptr( iitem );
    if ( !DEFINED_PCHR( pitem ) ) return;

    // cannot be mounted if you are packed
    if ( pitem->pack.is_packed ) return;

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if ( !pitem->isitem && pitem->dismount_timer > 0 ) return;

    // Make sure the holder/mount is valid
    pholder = ChrObjList.get_allocated_data_ptr( iholder );
    if ( !INGAME_PCHR( pholder ) ) return;

    // cannot be a holder if you are packed
    if ( pholder->pack.is_packed ) return;

#if !defined(ENABLE_BODY_GRAB)
    if ( !pitem->alive ) return;
#endif

    // Figure out which slot this grip_off relates to
    slot = grip_offset_to_slot( grip_off );

    // Make sure the the slot is valid
    if ( !ego_chr::get_pcap( iholder )->slotvalid[slot] ) return;

    // This is a small fix that allows special grabbable mounts not to be mountable while
    // held by another character (such as the magic carpet for example)
    if ( !pitem->isitem && pholder->ismount && INGAME_CHR( pholder->attachedto ) ) return;

    // Put 'em together
    pitem->inwhich_slot   = slot;
    pitem->attachedto     = iholder;
    pholder->holdingwhich[slot] = iitem;

    // set the grip vertices for the iitem
    set_weapongrip( iitem, iholder, grip_off );

    ego_chr::update_matrix( pitem, btrue );

    ego_chr::set_pos( pitem, mat_getTranslate_v( pitem->gfx_inst.matrix ) );

    pitem->enviro.inwater  = bfalse;
    pitem->jump_time = 4 * JUMP_DELAY;

    // Run the held animation
    if ( pholder->ismount && grip_off == GRIP_ONLY )
    {
        // Riding iholder
        ego_chr::play_action( pitem, ACTION_MI, btrue );
        pitem->mad_inst.action.loop = btrue;
    }
    else if ( pitem->alive )
    {
        /// \note ZF@> hmm, here is the torch holding bug. Removing
        /// the interpolation seems to fix it...
        ego_chr::play_action( pitem, ACTION_MM + slot, bfalse );
        pitem->mad_inst.state.frame_lst = pitem->mad_inst.state.frame_nxt;

        if ( pitem->isitem )
        {
            // Item grab
            pitem->mad_inst.action.keep = btrue;
        }
    }

    // Set the team
    if ( pitem->isitem )
    {
        pitem->team = pholder->team;

        // Set the alert
        if ( pitem->alive )
        {
            ADD_BITS( pitem->ai.alert, ALERTIF_GRABBED );
        }
    }

    if ( pholder->ismount )
    {
        pholder->team = pitem->team;

        // Set the alert
        if ( !pholder->isitem && pholder->alive )
        {
            ADD_BITS( pholder->ai.alert, ALERTIF_GRABBED );
        }
    }

    // It's not gonna hit the floor
    pitem->hitready = bfalse;
}

//--------------------------------------------------------------------------------------------
void drop_all_idsz( const CHR_REF & character, IDSZ idsz_min, IDSZ idsz_max )
{
    /// \author ZZ
    /// \details  This function drops all items ( idsz_min to idsz_max ) that are in a character's
    ///    inventory ( Not hands ).

    ego_chr  * pchr;
    CHR_REF  item, lastitem;
    FACING_T direction;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    if ( pchr->pos.z <= ( PITDEPTH >> 1 ) )
    {
        // Don't lose items in pits...
        return;
    }

    lastitem = character;
    PACK_BEGIN_LOOP( item, pchr->pack.next )
    {
        if ( item != character ) continue;

        ego_chr * pitem = ChrObjList.get_allocated_data_ptr( item );
        if ( !INGAME_PCHR( pitem ) ) continue;

        if (( ego_chr::get_idsz( item, IDSZ_PARENT ) >= idsz_min && ego_chr::get_idsz( item, IDSZ_PARENT ) <= idsz_max ) ||
            ( ego_chr::get_idsz( item, IDSZ_TYPE ) >= idsz_min && ego_chr::get_idsz( item, IDSZ_TYPE ) <= idsz_max ) )
        {
            // We found a valid item...
            TURN_T turn;

            direction = RANDIE;
            turn      = TO_TURN( direction );

            // unpack the item
            if ( chr_pack_remove_item( character, lastitem, item ) )
            {
                ADD_BITS( pitem->ai.alert, ALERTIF_DROPPED );
                pitem->hitready       = btrue;

                ego_chr::set_pos( pitem, ego_chr::get_pos_v( pchr ) );

                pitem->ori.facing_z           = direction + ATK_BEHIND;
                pitem->onwhichplatform_ref    = pchr->onwhichplatform_ref;
                pitem->onwhichplatform_update = pchr->onwhichplatform_update;
                pitem->vel.x                  = turntocos[ turn ] * DROPXYVEL;
                pitem->vel.y                  = turntosin[ turn ] * DROPXYVEL;
                pitem->vel.z                  = DROPZVEL;
                pitem->dismount_timer         = PHYS_DISMOUNT_TIME;

                chr_copy_enviro( pitem, pchr );
            }
        }
        else
        {
            lastitem = item;
        }

    }
    PACK_END_LOOP( item );

}

//--------------------------------------------------------------------------------------------
bool_t drop_all_items( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function drops all of a character's items

    CHR_REF  item;
    FACING_T direction;
    Sint16   diradd;
    ego_chr  * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], btrue, bfalse );
    detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], btrue, bfalse );
    if ( pchr->pack.count > 0 )
    {
        direction = pchr->ori.facing_z + ATK_BEHIND;
        diradd    = 0x00010000 / pchr->pack.count;

        for ( int cnt = pchr->pack.count; cnt >= 0; cnt-- )
        {
            item = chr_inventory_remove_item( character, GRIP_LEFT, bfalse );

            ego_chr * pitem = ChrObjList.get_allocated_data_ptr( item );
            if ( !INGAME_PCHR( pitem ) )
            {
                /* DO SOMETHING HERE */
            }
            else
            {
                detach_character_from_mount( item, btrue, btrue );

                ego_chr::set_pos( pitem, ego_chr::get_pos_v( pchr ) );
                pitem->hitready           = btrue;
                ADD_BITS( pitem->ai.alert, ALERTIF_DROPPED );
                pitem->dismount_timer          = PHYS_DISMOUNT_TIME;
                pitem->onwhichplatform_ref    = pchr->onwhichplatform_ref;
                pitem->onwhichplatform_update = pchr->onwhichplatform_update;
                pitem->ori.facing_z           = direction + ATK_BEHIND;
                pitem->vel.x                  = turntocos[( direction>>2 ) & TRIG_TABLE_MASK ] * DROPXYVEL;
                pitem->vel.y                  = turntosin[( direction>>2 ) & TRIG_TABLE_MASK ] * DROPXYVEL;
                pitem->vel.z                  = DROPZVEL;
                pitem->team                   = pitem->baseteam;

                pitem->dismount_timer         = PHYS_DISMOUNT_TIME;
                pitem->dismount_object        = character;

                chr_copy_enviro( pitem, pchr );
            }

            direction += diradd;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_grab_data
{
    CHR_REF ichr;
    float  dist;
};

//--------------------------------------------------------------------------------------------
int grab_data_cmp( const void * pleft, const void * pright )
{
    int rv;
    float diff;

    ego_grab_data * dleft  = ( ego_grab_data * )pleft;
    ego_grab_data * dright = ( ego_grab_data * )pright;

    diff = dleft->dist - dright->dist;

    if ( diff < 0.0f )
    {
        rv = -1;
    }
    else if ( diff > 0.0f )
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t character_grab_stuff( const CHR_REF & ichr_a, grip_offset_t grip_off, const bool_t grab_people )
{
    /// \author ZZ
    /// \details  This function makes the character pick up an item if there's one around

    int       cnt;
    size_t    vertex;
    CHR_REF   ichr_b;
    int       frame_nxt;
    slot_t    slot;
    fvec4_t   point[1], nupoint[1];
    SDL_Color color_red = {0xFF, 0x7F, 0x7F, 0xFF};
    SDL_Color color_grn = {0x7F, 0xFF, 0x7F, 0xFF};
    //   SDL_Color color_blu = {0x7F, 0x7F, 0xFF, 0xFF};

    ego_chr * pchr_a;

    int ticks;

    bool_t retval;

    // valid objects that can be grabbed
    int         grab_count = 0;
    ego_grab_data grab_list[MAX_CHR];

    // valid objects that cannot be grabbed
    int         ungrab_count = 0;
    ego_grab_data ungrab_list[MAX_CHR];

    pchr_a = ChrObjList.get_allocated_data_ptr( ichr_a );
    if ( !INGAME_PCHR( pchr_a ) ) return bfalse;

    ticks = egoboo_get_ticks();

    // Make life easier
    slot = grip_offset_to_slot( grip_off );  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( INGAME_CHR( pchr_a->holdingwhich[slot] ) || !ego_chr::get_pcap( ichr_a )->slotvalid[slot] )
        return bfalse;

    // Do we have a matrix???
    if ( ego_chr::matrix_valid( pchr_a ) )
    {
        // Transform the weapon grip_off from pchr_a->profile_ref to world space
        frame_nxt = pchr_a->mad_inst.state.frame_nxt;
        vertex    = pchr_a->gfx_inst.vrt_count - grip_off;

        // do the automatic update
        gfx_mad_instance::update_vertices( &( pchr_a->gfx_inst ), pchr_a->mad_inst.state, gfx_range( vertex, vertex ), bfalse );

        // Calculate grip_off point locations with linear interpolation and other silly things
        point[0].x = pchr_a->gfx_inst.vrt_lst[vertex].pos[XX];
        point[0].y = pchr_a->gfx_inst.vrt_lst[vertex].pos[YY];
        point[0].z = pchr_a->gfx_inst.vrt_lst[vertex].pos[ZZ];
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &( pchr_a->gfx_inst.matrix ), point, nupoint, 1 );
    }
    else
    {
        // Just wing it
        nupoint[0].x = pchr_a->pos.x;
        nupoint[0].y = pchr_a->pos.y;
        nupoint[0].z = pchr_a->pos.z;
        nupoint[0].w = 1.0f;
    }

    // Go through all characters to find the best match
    CHR_BEGIN_LOOP_PROCESSING( ichr_grab, pchr_grab )
    {
        fvec3_t   pos_b;
        float     dx, dy, dz, dxy;
        bool_t    can_grab = btrue;

        // do nothing to yourself
        if ( ichr_a == ichr_grab ) continue;

        // Don't do hidden objects
        if ( pchr_grab->is_hidden ) continue;

        if ( pchr_grab->pack.is_packed ) continue;        // pickpocket not allowed yet
        if ( INGAME_CHR( pchr_grab->attachedto ) ) continue; // disarm not allowed yet

        // do not pick up your mount
        if ( pchr_grab->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_grab->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        pos_b = pchr_grab->pos;

        // First check absolute value diamond
        dx = SDL_abs( nupoint[0].x - pos_b.x );
        dy = SDL_abs( nupoint[0].y - pos_b.y );
        dz = SDL_abs( nupoint[0].z - pos_b.z );
        dxy = dx + dy;

        if ( dxy > GRID_SIZE * 2 || dz > SDL_max( pchr_grab->bump.height, GRABSIZE ) ) continue;

        // reasonable carrying capacity
        if ( pchr_grab->phys.weight > pchr_a->phys.weight + pchr_a->strength * INV_FF )
        {
            can_grab = bfalse;
        }

        // grab_people == btrue allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( pchr_grab->alive && ( grab_people == pchr_grab->isitem ) )
        {
            can_grab = bfalse;
        }

        if ( can_grab )
        {
            grab_list[grab_count].ichr = ichr_grab;
            grab_list[grab_count].dist = dxy;
            grab_count++;
        }
        else
        {
            ungrab_list[grab_count].ichr = ichr_grab;
            ungrab_list[grab_count].dist = dxy;
            ungrab_count++;
        }
    }
    CHR_END_LOOP();

    // sort the grab list
    if ( grab_count > 1 )
    {
        SDL_qsort( grab_list, grab_count, sizeof( ego_grab_data ), grab_data_cmp );
    }

    // try to grab something
    retval = bfalse;
    for ( cnt = 0; cnt < grab_count; cnt++ )
    {
        bool_t can_grab;

        ego_chr * pchr_b;

        ichr_b = grab_list[cnt].ichr;
        pchr_b = ChrObjList.get_data_ptr( ichr_b );

        if ( grab_list[cnt].dist > GRABSIZE ) continue;

        can_grab = do_item_pickup( ichr_a, ichr_b );

        if ( can_grab )
        {
            // Stick 'em together and quit
            attach_character_to_mount( ichr_b, ichr_a, grip_off );
            if ( grab_people )
            {
                // Do a slam animation...  ( Be sure to drop!!! )
                ego_chr::play_action( pchr_a, ACTION_MC + slot, bfalse );
            }
            retval = btrue;
            break;
        }
        else
        {
            // Lift the item a little and quit...
            pchr_b->vel.z = DROPZVEL;
            pchr_b->hitready = btrue;
            ADD_BITS( pchr_b->ai.alert, ALERTIF_DROPPED );
            break;
        }

    }

    if ( !retval )
    {
        fvec3_t   vforward;

        //---- generate billboards for things that players can interact with
        if ( cfg.feedback != FEEDBACK_OFF && IS_PLAYER_PCHR( pchr_a ) )
        {
            GLXvector4f default_tint = { 1.00f, 1.00f, 1.00f, 1.00f };

            // things that can be grabbed (5 secs and green)
            for ( cnt = 0; cnt < grab_count; cnt++ )
            {
                ichr_b = grab_list[cnt].ichr;
                chr_make_text_billboard( ichr_b, ego_chr::get_name( ichr_b, CHRNAME_ARTICLE | CHRNAME_CAPITAL ), color_grn, default_tint, 5, bb_opt_none );
            }

            // things that can't be grabbed (5 secs and red)
            for ( cnt = 0; cnt < ungrab_count; cnt++ )
            {
                ichr_b = ungrab_list[cnt].ichr;
                chr_make_text_billboard( ichr_b, ego_chr::get_name( ichr_b, CHRNAME_ARTICLE | CHRNAME_CAPITAL ), color_red, default_tint, 5, bb_opt_none );
            }
        }

        //---- if you can't grab anything, activate something using ALERTIF_BUMPED
        if ( IS_PLAYER_PCHR( pchr_a ) && ungrab_count > 0 )
        {
            ego_chr::get_MatForward( pchr_a, &vforward );

            // sort the ungrab list
            if ( ungrab_count > 1 )
            {
                SDL_qsort( ungrab_list, ungrab_count, sizeof( ego_grab_data ), grab_data_cmp );
            }

            for ( cnt = 0; cnt < ungrab_count; cnt++ )
            {
                float       ftmp;
                fvec3_t     diff;
                ego_chr     * pchr_b;

                if ( ungrab_list[cnt].dist > GRABSIZE ) continue;

                ichr_b = ungrab_list[cnt].ichr;
                pchr_b = ChrObjList.get_allocated_data_ptr( ichr_b );
                if ( !INGAME_PCHR( pchr_b ) ) continue;

                diff = fvec3_sub( pchr_a->pos.v, pchr_b->pos.v );

                // ignore vertical displacement in the dot product
                ftmp = vforward.x * diff.x + vforward.y * diff.y;
                if ( ftmp > 0.0f )
                {
                    ego_ai_state::set_bumplast( &( pchr_b->ai ), ichr_a );
                }
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void character_swipe( const CHR_REF & ichr, slot_t slot )
{
    /// \author ZZ
    /// \details  This function spawns an attack particle

    CHR_REF iweapon, ithrown, iholder;
    ego_chr * pchr, * pweapon;
    ego_cap * pweapon_cap;

    PRT_REF iparticle;

    int   spawn_vrt_offset;
    Uint8 action;
    Uint16 turn;
    float dampen;
    float velocity;

    bool_t unarmed_attack;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return;

    iweapon = pchr->holdingwhich[slot];

    // See if it's an unarmed attack...
    if ( MAX_CHR == iweapon )
    {
        unarmed_attack   = btrue;
        iweapon          = ichr;
        spawn_vrt_offset = slot_to_grip_offset( slot );  // SLOT_LEFT -> GRIP_LEFT, SLOT_RIGHT -> GRIP_RIGHT
    }
    else
    {
        unarmed_attack   = bfalse;
        spawn_vrt_offset = GRIP_LAST;
        action = pchr->mad_inst.action.which;
    }

    pweapon = ChrObjList.get_allocated_data_ptr( iweapon );
    if ( !INGAME_PCHR( pweapon ) ) return;

    pweapon_cap = ego_chr::get_pcap( iweapon );
    if ( NULL == pweapon_cap ) return;

    // find the 1st non-item that is holding the weapon
    iholder = ego_chr::get_lowest_attachment( iweapon, btrue );

    if ( iweapon != iholder && iweapon != ichr )
    {
        // This seems to be the "proper" place to activate the held object.

        // Make the iweapon attack too
        ego_chr::play_action( pweapon, ACTION_MJ, bfalse );

        ADD_BITS( pweapon->ai.alert, ALERTIF_USED );
    }

    // What kind of attack are we going to do?
    if ( !unarmed_attack && (( pweapon_cap->isstackable && pweapon->ammo > 1 ) || ACTION_IS_TYPE( pweapon->mad_inst.action.which, F ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        ithrown = spawn_one_character( pchr->pos, pweapon->profile_ref, ego_chr::get_iteam( iholder ), 0, pchr->ori.facing_z, pweapon->name, CHR_REF( MAX_CHR ) );

        ego_chr * pthrown = ChrObjList.get_allocated_data_ptr( ithrown );
        if ( INGAME_PCHR( pthrown ) )
        {
            pthrown->iskursed = bfalse;
            pthrown->ammo = 1;
            ADD_BITS( pthrown->ai.alert, ALERTIF_THROWN );
            velocity = pchr->strength / ( pthrown->phys.weight * THROWFIX );
            velocity += MINTHROWVELOCITY;
            velocity = SDL_min( velocity, MAXTHROWVELOCITY );

            turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            {
                fvec3_t _tmp_vec = VECT3( turntocos[ turn ] * velocity, turntocos[ turn ] * velocity, DROPZVEL );
                phys_data_accumulate_avel( &( pthrown->phys ), _tmp_vec.v );
            }

            if ( pweapon->ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( iweapon, btrue, bfalse );
                POBJ_REQUEST_TERMINATE( ego_chr::get_obj_ptr( pweapon ) );
            }
            else
            {
                pweapon->ammo--;
            }
        }
    }
    else
    {
        // A generic attack. Spawn the damage particle.
        if ( pweapon->ammo_max == 0 || pweapon->ammo != 0 )
        {
            if ( pweapon->ammo > 0 && !pweapon_cap->isstackable )
            {
                pweapon->ammo--;  // Ammo usage
            }

            // Spawn an attack particle
            if ( pweapon_cap->attack_pip != -1 )
            {
                // make the weapon's holder the owner of the attack particle?
                // will this mess up wands?
                iparticle = spawn_one_particle( pweapon->pos, pchr->ori.facing_z, pweapon->profile_ref, pweapon_cap->attack_pip, iweapon, spawn_vrt_offset, ego_chr::get_iteam( iholder ), iholder, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );

                ego_prt * pprt = PrtObjList.get_allocated_data_ptr( iparticle );
                if ( NULL != pprt )
                {
                    fvec3_t tmp_pos;

                    tmp_pos = ego_prt::get_pos( pprt );

                    if ( pweapon_cap->attack_attached )
                    {
                        // attached particles get a strength bonus for reeling...
                        dampen = REELBASE + ( pchr->strength / REEL );

                        pprt->vel_stt.x *= dampen;
                        pprt->vel_stt.y *= dampen;
                        pprt->vel_stt.z *= dampen;

                        pprt = place_particle_at_vertex( pprt, iweapon, spawn_vrt_offset );
                        if ( NULL == pprt ) return;
                    }
                    else
                    {
                        // NOT ATTACHED
                        pprt->attachedto_ref = CHR_REF( MAX_CHR );

                        // Detach the particle
                        if ( !ego_prt::get_ppip( iparticle )->startontarget || !INGAME_CHR( pprt->target_ref ) )
                        {
                            pprt = place_particle_at_vertex( pprt, iweapon, spawn_vrt_offset );
                            if ( NULL == pprt ) return;

                            // Correct Z spacing base, but nothing else...
                            tmp_pos.z += ego_prt::get_ppip( iparticle )->spacing_vrt_pair.base;
                        }

                        // Don't spawn in walls
                        if ( prt_test_wall( pprt, tmp_pos.v ) )
                        {
                            tmp_pos.x = pweapon->pos.x;
                            tmp_pos.y = pweapon->pos.y;
                            if ( prt_test_wall( pprt, tmp_pos.v ) )
                            {
                                tmp_pos.x = pchr->pos.x;
                                tmp_pos.y = pchr->pos.y;
                            }
                        }
                    }

                    // Initial particles get a bonus, which may be 0.00f
                    pprt->damage.base += ( pchr->strength     * pweapon_cap->str_bonus );
                    pprt->damage.base += ( pchr->wisdom       * pweapon_cap->wis_bonus );
                    pprt->damage.base += ( pchr->intelligence * pweapon_cap->int_bonus );
                    pprt->damage.base += ( pchr->dexterity    * pweapon_cap->dex_bonus );

                    // Initial particles get an enchantment bonus
                    pprt->damage.base += pweapon->damageboost;

                    ego_prt::set_pos( pprt, tmp_pos.v );
                }
            }
        }
        else
        {
            pweapon->ammoknown = btrue;
        }
    }
}

//--------------------------------------------------------------------------------------------
int drop_money( const CHR_REF & character, const int money )
{
    /// \author ZZ
    /// \details  This function drops some of a character's money

    int huns, tfives, fives, ones, cnt;

    ego_obj_chr * pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return 0;

    int loc_money = money;
    if ( loc_money > pchr->money )  loc_money = pchr->money;

    if ( loc_money > 0 && pchr->pos.z > -2 )
    {
        pchr->money -= loc_money;
        huns = loc_money / 100;  loc_money -= ( huns << 7 ) - ( huns << 5 ) + ( huns << 2 );
        tfives = loc_money / 25;  loc_money -= ( tfives << 5 ) - ( tfives << 3 ) + tfives;
        fives = loc_money / 5;  loc_money -= ( fives << 2 ) + fives;
        ones = loc_money;

        for ( cnt = 0; cnt < ones; cnt++ )
        {
            spawn_one_particle_global( pchr->pos, ATK_FRONT, PIP_COIN1, cnt );
        }

        for ( cnt = 0; cnt < fives; cnt++ )
        {
            spawn_one_particle_global( pchr->pos, ATK_FRONT, PIP_COIN5, cnt );
        }

        for ( cnt = 0; cnt < tfives; cnt++ )
        {
            spawn_one_particle_global( pchr->pos, ATK_FRONT, PIP_COIN25, cnt );
        }

        for ( cnt = 0; cnt < huns; cnt++ )
        {
            spawn_one_particle_global( pchr->pos, ATK_FRONT, PIP_COIN100, cnt );
        }

        pchr->damagetime = DAMAGETIME;  // So it doesn't grab it again
    }

    return loc_money;
}

//--------------------------------------------------------------------------------------------
void call_for_help( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function issues a call for help to all allies

    TEAM_REF team;

    if ( !INGAME_CHR( character ) ) return;

    team = ego_chr::get_iteam( character );
    TeamStack[team].sissy = character;

    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        if ( cnt != character && !team_hates_team( pchr->team, team ) )
        {
            ADD_BITS( pchr->ai.alert, ALERTIF_CALLEDFORHELP );
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t setup_xp_table( const CAP_REF & icap )
{
    /// \author ZF
    /// \details  This calculates the xp needed to reach next level and stores it in an array for later use

    Uint8 level;
    ego_cap * pcap;

    if ( !LOADED_CAP( icap ) ) return bfalse;
    pcap = CapStack + icap;

    // Calculate xp needed
    for ( level = MAXBASELEVEL; level < MAXLEVEL; level++ )
    {
        Uint32 xpneeded = pcap->experience_forlevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= (( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        pcap->experience_forlevel[level] = xpneeded;
    }
    return btrue;
}

//--------------------------------------------------------------------------------------------
void do_level_up( const CHR_REF & character )
{
    /// \author BB
    /// \details  level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    ego_chr * pchr;
    ego_cap * pcap;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return;

    // Do level ups and stat changes
    curlevel = pchr->experience_level + 1;
    if ( curlevel < MAXLEVEL )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = pchr->experience;
        xpneeded  = pcap->experience_forlevel[curlevel];
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            pchr->experience_level++;
            xpneeded = pcap->experience_forlevel[curlevel];
            ADD_BITS( pchr->ai.alert, ALERTIF_LEVELUP );

            // The character is ready to advance...
            if ( IS_PLAYER_PCHR( pchr ) )
            {
                debug_printf( "%s gained a level!!!", ego_chr::get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                sound_play_chunk_full( g_wavelist[GSND_LEVELUP] );
            }

            // Size
            pchr->fat_goto += pcap->size_perlevel * 0.25f;  // Limit this?
            pchr->fat_goto_time += SIZETIME;

            // Strength
            number = generate_irand_range( pcap->strength_stat.perlevel );
            number += pchr->strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->strength = number;

            // Wisdom
            number = generate_irand_range( pcap->wisdom_stat.perlevel );
            number += pchr->wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->wisdom = number;

            // Intelligence
            number = generate_irand_range( pcap->intelligence_stat.perlevel );
            number += pchr->intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->intelligence = number;

            // Dexterity
            number = generate_irand_range( pcap->dexterity_stat.perlevel );
            number += pchr->dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->dexterity = number;

            // Life
            number = generate_irand_range( pcap->life_stat.perlevel );
            number += pchr->life_max;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->life += ( number - pchr->life_max );
            pchr->life_max = number;

            // Mana
            number = generate_irand_range( pcap->mana_stat.perlevel );
            number += pchr->mana_max;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->mana += ( number - pchr->mana_max );
            pchr->mana_max = number;

            // Mana Return
            number = generate_irand_range( pcap->mana_return_stat.perlevel );
            number += pchr->mana_return;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->mana_return = number;

            // Mana Flow
            number = generate_irand_range( pcap->mana_flow_stat.perlevel );
            number += pchr->mana_flow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->mana_flow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( const CHR_REF & character, const int amount, xp_type xptype, const bool_t override_invictus )
{
    /// \author ZZ
    /// \details  This function gives a character experience

    float newamount;

    ego_chr * pchr;
    ego_cap * pcap;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return;

    // No xp to give
    if ( 0 == amount ) return;

    if ( !IS_INVICTUS_PCHR_RAW( pchr ) || override_invictus )
    {
        float intadd = ( SFP8_TO_SINT( pchr->intelligence ) - 10.0f ) / 200.0f;
        float wisadd = ( SFP8_TO_SINT( pchr->wisdom )       - 10.0f ) / 400.0f;

        // Figure out how much experience to give
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * pcap->experience_rate[xptype];
        }

        // Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount *= 1.00f + intadd + wisadd;

        // Apply XP bonus/penalty depending on game difficulty
        if ( cfg.difficulty >= GAME_HARD ) newamount *= 1.20f;                // 20% extra on hard
        else if ( cfg.difficulty >= GAME_NORMAL ) newamount *= 1.10f;       // 10% extra on normal

        pchr->experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( const TEAM_REF & team, const int amount, xp_type xptype )
{
    /// \author ZZ
    /// \details  This function gives every character on a team experience

    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        if ( pchr->team == team )
        {
            give_experience( cnt, amount, xptype, bfalse );
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
ego_chr * resize_one_character( ego_chr * pchr )
{
    /// \author ZZ
    /// \details  This function makes the characters get bigger or smaller, depending
    ///    on their fat_goto and fat_goto_time. Spellbooks do not resize
    /// \note  BB@> assume that this will only be called from inside ego_chr::do_processing(),
    ///         so pchr is just right to be used here

    CHR_REF ichr;
    ego_cap * pcap;
    bool_t  willgetcaught;
    float   newsize;

    if ( NULL == pchr ) return pchr;

    ichr = GET_REF_PCHR( pchr );
    pcap = ego_chr::get_pcap( ichr );

    if ( pchr->fat_goto_time < 0 ) return pchr;

    if ( pchr->fat_goto != pchr->fat )
    {
        int bump_increase;

        bump_increase = ( pchr->fat_goto - pchr->fat ) * 0.10f * pchr->bump.size;

        // Make sure it won't get caught in a wall
        willgetcaught = bfalse;
        if ( pchr->fat_goto > pchr->fat )
        {
            pchr->bump_1.size += bump_increase;

            if ( chr_test_wall( pchr, NULL ) )
            {
                willgetcaught = btrue;
            }

            pchr->bump_1.size -= bump_increase;
        }

        // If it is getting caught, simply halt growth until later
        if ( !willgetcaught )
        {
            // Figure out how big it is
            pchr->fat_goto_time--;

            newsize = pchr->fat_goto;
            if ( pchr->fat_goto_time > 0 )
            {
                newsize = ( pchr->fat * 0.90f ) + ( newsize * 0.10f );
            }

            // Make it that big...
            ego_chr::set_fat( pchr, newsize );

            if ( CAP_INFINITE_WEIGHT == pcap->weight )
            {
                pchr->phys.weight = INFINITE_WEIGHT;
            }
            else
            {
                Uint32 itmp = pcap->weight * pchr->fat * pchr->fat * pchr->fat;
                pchr->phys.weight = SDL_min( itmp, MAX_WEIGHT );
            }
        }
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
egoboo_rv export_one_character_quest_vfs( const char *szSaveName, const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function makes the naming.txt file for the character
    ego_player *ppla;

    if ( !INGAME_CHR( character ) ) return rv_fail;

    ppla = net_get_ppla( character );
    if ( ppla == NULL ) return rv_fail;

    return quest_log_upload_vfs( ppla->quest_log, SDL_arraysize( ppla->quest_log ), szSaveName );
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_name_vfs( const char *szSaveName, const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function makes the naming.txt file for the character

    ego_obj_chr * pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    return chop_export_vfs( szSaveName, pchr->name );
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_profile_vfs( const char *szSaveName, const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function creates a data.txt file for the given character.
    ///    it is assumed that all enchantments have been done away with

    ego_chr * pchr;
    ego_cap * pcap;

    // a local version of the cap, so that the CapStack data won't be corrupted
    ego_cap cap_tmp;

    if ( INVALID_CSTR( szSaveName ) && !DEFINED_CHR( character ) ) return bfalse;
    pchr = ChrObjList.get_data_ptr( character );

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return bfalse;

    // load up the temporary cap
    SDL_memcpy( &cap_tmp, pcap, sizeof( ego_cap ) );

    // fill in the cap values with the ones we want to export from the character profile
    ego_chr::upload_cap( pchr, &cap_tmp );

    return save_one_cap_data_file_vfs( szSaveName, NULL, &cap_tmp );
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_skin_vfs( const char *szSaveName, const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function creates a skin.txt file for the given character.

    vfs_FILE* filewrite;

    ego_obj_chr * pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    // Open the file
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    vfs_printf( filewrite, "// This file is used only by the import menu\n" );
    vfs_printf( filewrite, ": %d\n", pchr->skin );
    vfs_close( filewrite );
    return btrue;
}

//--------------------------------------------------------------------------------------------
CAP_REF load_one_character_profile_vfs( const char * tmploadname, const int slot_override, const bool_t required )
{
    /// \author ZZ
    /// \details  This function fills a character profile with data from data.txt, returning
    /// the icap slot that the profile was stuck into.  It may cause the program
    /// to abort if bad things happen.

    CAP_REF  icap = CAP_REF( MAX_CAP );
    ego_cap * pcap;
    STRING  szLoadName;

    if ( VALID_PRO_RANGE( slot_override ) )
    {
        icap = CAP_REF( slot_override );
    }
    else
    {
        icap = pro_get_slot_vfs( tmploadname, MAX_PROFILE );
    }

    if ( !CapStack.in_range_ref( icap ) )
    {
        // The data file wasn't found
        if ( required )
        {
            log_debug( "%s - \"%s\" was not found. Overriding a global object?\n", __FUNCTION__, szLoadName );
        }
        else if ( CapStack.valid_idx( slot_override ) && slot_override > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "%s - Not able to open file \"%s\"\n", __FUNCTION__, szLoadName );
        }

        return CAP_REF( MAX_CAP );
    }

    pcap = CapStack + icap;

    // if there is data in this profile, release it
    if ( pcap->loaded )
    {
        // Make sure global objects don't load over existing models
        if ( required && SPELLBOOK == icap )
        {
            log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
        }
        else if ( required && overrideslots )
        {
            log_error( "Object slot %i used twice (%s, %s)\n", icap.get_value(), pcap->name, szLoadName );
        }
        else
        {
            // Stop, we don't want to override it
            return CAP_REF( MAX_CAP );
        }

        // If loading over an existing model is allowed (?how?) then make sure to release the old one
        release_one_cap( icap );
    }

    if ( NULL == load_one_cap_data_file_vfs( tmploadname, pcap ) )
    {
        return CAP_REF( MAX_CAP );
    }

    // do the rest of the levels not listed in data.txt
    setup_xp_table( icap );

    if ( cfg.gourard_req )
    {
        pcap->uniformlit = bfalse;
    }

    // limit the wave indices to rational values
    pcap->sound_index[SOUND_FOOTFALL] = CLIP( pcap->sound_index[SOUND_FOOTFALL], INVALID_SOUND, MAX_WAVE );
    pcap->sound_index[SOUND_JUMP]     = CLIP( pcap->sound_index[SOUND_JUMP], INVALID_SOUND, MAX_WAVE );

    // bumpdampen == 0 means infinite mass, and causes some problems
    pcap->bumpdampen = SDL_max( INV_FF, pcap->bumpdampen );

    return icap;
}

//--------------------------------------------------------------------------------------------
bool_t heal_character( const CHR_REF & character, const CHR_REF & healer, const int amount, const bool_t ignore_invictus )
{
    /// \author ZF
    /// \details  This function gives some pure life points to the target, ignoring any resistances and so forth
    ego_chr * pchr, *pchr_h;

    // Setup the healed character
    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    // Setup the healer
    pchr_h = ChrObjList.get_allocated_data_ptr( healer );
    if ( !INGAME_PCHR( pchr_h ) ) return bfalse;

    // Don't heal dead and invincible stuff
    if ( !pchr->alive || ( IS_INVICTUS_PCHR_RAW( pchr ) && !ignore_invictus ) ) return bfalse;

    // This actually heals the character
    pchr->life = CLIP( pchr->life, pchr->life + SDL_abs( amount ), pchr->life_max );

    // Set alerts, but don't alert that we healed ourselves
    if ( healer != character && pchr_h->attachedto != character && SDL_abs( amount ) > HURTDAMAGE )
    {
        ADD_BITS( pchr->ai.alert, ALERTIF_HEALED );
        pchr->ai.attacklast = healer;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void cleanup_one_character( ego_chr * pchr )
{
    /// \author BB
    /// \details  Everything necessary to disconnect one character from the game

    CHR_REF  ichr, itmp;
    SHOP_REF ishop;

    if ( !VALID_PCHR( pchr ) ) return;
    ichr = GET_REF_PCHR( pchr );

    pchr->sparkle = NOSPARKLE;

    // Remove it from the team
    pchr->team = pchr->baseteam;
    if ( TeamStack[pchr->team].morale > 0 ) TeamStack[pchr->team].morale--;

    if ( TeamStack[pchr->team].leader == ichr )
    {
        // The team now has no leader if the character is the leader
        TeamStack[pchr->team].leader = NOLEADER;
    }

    // Clear all shop passages that it owned...
    for ( ishop = 0; ishop < ShopStack.count; ishop++ )
    {
        if ( ShopStack[ishop].owner != ichr ) continue;
        ShopStack[ishop].owner = SHOP_NOOWNER;
    }

    // detach from any mount
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        detach_character_from_mount( ichr, btrue, bfalse );
    }

    // drop your left item
    itmp = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( itmp ) && ChrObjList.get_data_ref( itmp ).isitem )
    {
        detach_character_from_mount( itmp, btrue, bfalse );
    }

    // drop your right item
    itmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( itmp ) && ChrObjList.get_data_ref( itmp ).isitem )
    {
        detach_character_from_mount( itmp, btrue, bfalse );
    }

    // start with a clean list
    cleanup_character_enchants( pchr );

    // remove enchants from the character
    if ( pchr->life >= 0 )
    {
        remove_all_character_enchants( ichr );
    }
    else
    {
        ego_eve * peve;
        ENC_REF ego_enc_now, ego_enc_next;

        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // remove all invalid enchants
        ego_enc_now = pchr->firstenchant;
        while ( ego_enc_now != MAX_ENC )
        {
            ego_enc_next = EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref;

            peve = ego_enc::get_peve( ego_enc_now );
            if ( NULL != peve && !peve->stayiftargetdead )
            {
                remove_enchant( ego_enc_now, NULL );
            }

            ego_enc_now = ego_enc_next;
        }
    }

    // Stop all sound loops for this object
    looped_stop_object_sounds( ichr );
}

//--------------------------------------------------------------------------------------------
void kill_character( const CHR_REF & ichr, const CHR_REF & killer, const bool_t ignore_invictus )
{
    /// \author BB
    /// \details  Handle a character death. Set various states, disconnect it from the world, etc.

    ego_chr * pchr;
    ego_cap * pcap;
    int action;
    Uint16 experience;
    TEAM_REF killer_team;
    ego_ai_bundle tmp_bdl_ai;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !DEFINED_PCHR( pchr ) ) return;

    // No need to continue is there?
    if ( !pchr->alive || ( IS_INVICTUS_PCHR_RAW( pchr ) && !ignore_invictus ) ) return;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return;

    killer_team = ego_chr::get_iteam( killer );

    pchr->alive = bfalse;
    pchr->waskilled = btrue;

    pchr->life            = -1;
    pchr->platform        = btrue;
    pchr->canuseplatforms = btrue;
    pchr->phys.bumpdampen = pchr->phys.bumpdampen / 2.0f;

    // Play the death animation
    action = ACTION_KA + generate_randmask( 0, 3 );
    ego_chr::play_action( pchr, action, bfalse );
    pchr->mad_inst.action.keep = btrue;

    // Give kill experience
    experience = pcap->experience_worth + ( pchr->experience * pcap->experience_exchange );

    // Set target
    pchr->ai.target = INGAME_CHR( killer ) ? killer : CHR_REF( MAX_CHR );
    if ( killer_team == TEAM_DAMAGE ) pchr->ai.target = CHR_REF( MAX_CHR );
    if ( killer_team == TEAM_NULL ) pchr->ai.target   = ichr;

    // distribute experience to the attacker
    if ( MAX_CHR != pchr->ai.target )
    {
        // Award experience for kill?
        if ( team_hates_team( killer_team, pchr->team ) )
        {
            // Check for special hatred
            if ( ego_chr::get_idsz( killer, IDSZ_HATE ) == ego_chr::get_idsz( ichr, IDSZ_PARENT ) ||
                 ego_chr::get_idsz( killer, IDSZ_HATE ) == ego_chr::get_idsz( ichr, IDSZ_TYPE ) )
            {
                give_experience( killer, experience, XP_KILLHATED, bfalse );
            }

            // Nope, award direct kill experience instead
            else give_experience( killer, experience, XP_KILLENEMY, bfalse );
        }
    }

    // Set various alerts to let others know it has died
    // and distribute experience to whoever needs it
    ADD_BITS( pchr->ai.alert, ALERTIF_KILLED );

    CHR_BEGIN_LOOP_PROCESSING( tnc, plistener )
    {
        if ( !plistener->alive ) continue;

        // All allies get team experience, but only if they also hate the dead guy's team
        if ( tnc != killer && !team_hates_team( plistener->team, killer_team ) && team_hates_team( plistener->team, pchr->team ) )
        {
            give_experience( tnc, experience, XP_TEAMKILL, bfalse );
        }

        // Check if it was a leader
        if ( TeamStack[pchr->team].leader == ichr && ego_chr::get_iteam( tnc ) == pchr->team )
        {
            // All folks on the leaders team get the alert
            ADD_BITS( plistener->ai.alert, ALERTIF_LEADERKILLED );
        }

        // Let the other characters know it died
        if ( plistener->ai.target == ichr )
        {
            ADD_BITS( plistener->ai.alert, ALERTIF_TARGETKILLED );
        }
    }
    CHR_END_LOOP();

    // Detach the character from the game
    cleanup_one_character( pchr );

    // If it's a player, let it die properly before enabling respawn
    if ( IS_PLAYER_PCHR( pchr ) ) timer_revive = ONESECOND; // 1 second

    // Let its AI script run one last time
    pchr->ai.timer = update_wld + 1;            // Prevent IfTimeOut in scr_run_chr_script()
    scr_run_chr_script( ego_ai_bundle::set( &tmp_bdl_ai, pchr ) );

    // Stop any looped sounds
    if ( pchr->loopedsound_channel != INVALID_SOUND ) sound_stop_channel( pchr->loopedsound_channel );
    looped_stop_object_sounds( ichr );
    pchr->loopedsound_channel = INVALID_SOUND;
}

//--------------------------------------------------------------------------------------------
int damage_character_hurt( ego_bundle_chr & bdl, const int base_damage, const int actual_damage, const CHR_REF & attacker, const bool_t ignore_invictus )
{
    CHR_REF      loc_ichr;
    ego_chr      * loc_pchr;

    if ( 0 == actual_damage ) return 0;

    if ( NULL == bdl.chr_ptr() ) return 0;

    // alias some variables
    loc_ichr = bdl.chr_ref();
    loc_pchr = bdl.chr_ptr();

    // Only actual_damage if not invincible
    if ( loc_pchr->damagetime > 0 && !ignore_invictus ) return 0;

    loc_pchr->life -= actual_damage;

    // Taking actual_damage action
    if ( loc_pchr->life <= 0 )
    {
        kill_character( loc_ichr, attacker, ignore_invictus );
    }
    else if ( base_damage > HURTDAMAGE )
    {
        ego_chr::play_action( loc_pchr, mad_randomize_action( ACTION_HA, 0 ), bfalse );
    }

    return actual_damage;
}

//--------------------------------------------------------------------------------------------
int damage_character_heal( ego_bundle_chr & bdl, const int heal_amount, const CHR_REF & attacker, const bool_t ignore_invictus )
{
    CHR_REF      loc_ichr;
    ego_chr      * loc_pchr;
    ego_ai_state * loc_pai;

    if ( NULL == bdl.chr_ptr() ) return 0;

    // alias some variables
    loc_ichr = bdl.chr_ref();
    loc_pchr = bdl.chr_ptr();
    loc_pai  = &( loc_pchr->ai );

    heal_character( loc_ichr, attacker, heal_amount, ignore_invictus );

    return heal_amount;
}

//--------------------------------------------------------------------------------------------
int damage_character( const CHR_REF & character, FACING_T direction,
                      IPair  damage, const Uint8 damagetype, TEAM_REF team,
                      CHR_REF attacker, BIT_FIELD effects, const bool_t ignore_invictus )
{
    /// \author ZZ
    /// \details  This function calculates and applies damage to a character.  It also
    ///    sets alerts and begins actions.  Blocking and frame invincibility
    ///    are done here too.  Direction is ATK_FRONT if the attack is coming head on,
    ///    ATK_RIGHT if from the right, ATK_BEHIND if from the back, ATK_LEFT if from the
    ///    left.

    ego_chr      * loc_pchr;
    ego_cap      * loc_pcap;
    ego_ai_state * loc_pai;
    CHR_REF      loc_ichr;

    int     actual_damage, base_damage, max_damage;
    int     mana_damage;
    bool_t friendly_fire = bfalse, immune_to_damage = bfalse;
    bool_t do_feedback = ( FEEDBACK_OFF != cfg.feedback );

    loc_pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( loc_pchr ) ) return 0;
    ego_bundle_chr bdl( loc_pchr );

    bdl.set( loc_pchr );
    if ( NULL == bdl.chr_ptr() ) return 0;

    loc_pchr = bdl.chr_ptr();
    loc_pcap = bdl.cap_ptr();
    loc_ichr = bdl.chr_ref();
    loc_pai  = &( loc_pchr->ai );

    // determine some optional behavior
    if ( !INGAME_CHR( attacker ) )
    {
        do_feedback = bfalse;
    }
    else
    {
        // do not show feedback for damaging yourself
        if ( attacker == loc_ichr )
        {
            do_feedback = bfalse;
        }

        // identify friendly fire for color selection :)
        if ( ego_chr::get_iteam( loc_ichr ) == ego_chr::get_iteam( attacker ) )
        {
            friendly_fire = btrue;
        }

        // don't show feedback from random objects hitting each other
        if ( !ChrObjList.get_data_ref( attacker ).draw_stats )
        {
            do_feedback = bfalse;
        }

        // don't show damage to players since they get feedback from the status bars
        if ( loc_pchr->draw_stats || IS_PLAYER_PCHR( loc_pchr ) )
        {
            do_feedback = bfalse;
        }
    }

    actual_damage = 0;
    mana_damage   = 0;
    max_damage    = SDL_abs( damage.base ) + SDL_abs( damage.rand );

    // Lessen actual_damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    actual_damage = generate_irand_pair( damage );
    base_damage   = actual_damage;
    actual_damage = actual_damage >> GET_DAMAGE_RESIST( loc_pchr->damagemodifier[damagetype] );

    // Increase electric damage when in water
    if ( damagetype == DAMAGE_ZAP && chr_is_over_water( loc_pchr ) )
    {
        // Only if actually in the water
        if ( loc_pchr->pos.z <= water.surface_level )
        {
            /// \note ZF@> Is double damage too much?
            actual_damage = actual_damage << 1;
        }
    }

    // Allow actual_damage to be dealt to mana (mana shield spell)
    if ( HAS_SOME_BITS( loc_pchr->damagemodifier[damagetype], DAMAGEMANA ) )
    {
        // determine the change to the mana
        int tmp_final_mana = loc_pchr->mana - actual_damage;
        int delta_mana;

        // clip the final mana so it is within the range
        tmp_final_mana = CLIP( tmp_final_mana, 0, loc_pchr->mana_max );
        delta_mana     = tmp_final_mana - loc_pchr->mana;

        // modify the actual damage amounts
        mana_damage   += delta_mana;
        actual_damage -= delta_mana;
    }

    // Allow charging (Invert actual_damage to mana_damage)
    if ( HAS_SOME_BITS( loc_pchr->damagemodifier[damagetype], DAMAGECHARGE ) )
    {
        // determine the change to the mana
        int tmp_final_mana = loc_pchr->mana + actual_damage;
        int delta_mana;

        // clip the final mana so it is within the range
        tmp_final_mana = CLIP( tmp_final_mana, 0, loc_pchr->mana_max );
        delta_mana     = tmp_final_mana - loc_pchr->mana;

        // modify the actual damage amounts
        mana_damage   += delta_mana;
        actual_damage -= delta_mana;
    }

    // Invert actual_damage to heal
    if ( HAS_SOME_BITS( loc_pchr->damagemodifier[damagetype], DAMAGEINVERT ) )
    {
        actual_damage = -actual_damage;
    }

    // Remember the actual_damage type
    loc_pai->damagetypelast = damagetype;
    loc_pai->directionlast  = direction;

    // Check for characters who are immune to this damage, no need to continue if they have
    immune_to_damage = ( actual_damage > 0 && actual_damage <= loc_pchr->damagethreshold ) || HAS_SOME_BITS( loc_pchr->damagemodifier[damagetype], DAMAGEINVICTUS );
    if ( immune_to_damage )
    {
        // Dark green text
        const float lifetime = 3;
        SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
        GLXvector4f tint  = { 0.0f, 0.5f, 0.00f, 1.00f };

        actual_damage = 0;
        spawn_defense_ping( loc_pchr, attacker );

        // Character is simply immune to the damage
        chr_make_text_billboard( loc_ichr, "Immune!", text_color, tint, lifetime, ( BIT_FIELD )bb_opt_all );
    }

    if ( actual_damage > 0 )
    {
        // Hard mode deals 25% extra actual damage to players!
        if ( cfg.difficulty >= GAME_HARD && IS_PLAYER_PCHR( loc_pchr ) && !IS_PLAYER_PCHR( ChrObjList.get_data_ptr( attacker ) ) )
        {
            actual_damage *= 1.25f;
        }

        // Easy mode deals 25% extra actual damage by players and 25% less to players
        if ( cfg.difficulty <= GAME_EASY )
        {
            if ( IS_PLAYER_PCHR( ChrObjList.get_data_ptr( attacker ) ) && !IS_PLAYER_PCHR( loc_pchr ) )
            {
                actual_damage *= 1.25f;
            }

            if ( !IS_PLAYER_PCHR( ChrObjList.get_data_ptr( attacker ) ) &&  IS_PLAYER_PCHR( loc_pchr ) )
            {
                actual_damage *= 0.75f;
            }
        }

        if ( HAS_NO_BITS( DAMFX_ARMO, effects ) )
        {
            actual_damage *= loc_pchr->defense * INV_FF;
        }

        // hurt the character
        actual_damage = damage_character_hurt( bdl, base_damage, actual_damage, attacker, ignore_invictus );

        // Make the character invincible for a limited time only
        if ( HAS_NO_BITS( effects, DAMFX_TIME ) )
        {
            loc_pchr->damagetime = DAMAGETIME;
        }

        // Spawn blud particles
        if ( loc_pcap->blud_valid )
        {
            if ( loc_pcap->blud_valid == ULTRABLUDY || ( base_damage > HURTDAMAGE && damagetype < DAMAGE_HOLY ) )
            {
                spawn_one_particle( loc_pchr->pos, loc_pchr->ori.facing_z + direction, loc_pchr->profile_ref, loc_pcap->blud_pip,
                                    CHR_REF( MAX_CHR ), GRIP_LAST, loc_pchr->team, loc_ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );
            }
        }

        // Set attack alert if it wasn't an accident
        if ( base_damage > HURTDAMAGE )
        {
            if ( team == TEAM_DAMAGE )
            {
                loc_pai->attacklast = CHR_REF( MAX_CHR );
            }
            else
            {
                // Don't alert the character too much if under constant fire
                if ( loc_pchr->carefultime == 0 )
                {
                    // Don't let characters chase themselves...  That would be silly
                    if ( attacker != loc_ichr )
                    {
                        ADD_BITS( loc_pai->alert, ALERTIF_ATTACKED );
                        loc_pai->attacklast = attacker;
                        loc_pchr->carefultime = CAREFULTIME;
                    }
                }
            }
        }

        /// \test spawn a fly-away damage indicator?
        if ( do_feedback )
        {
            const char * tmpstr;
            int rank;

            tmpstr = describe_wounds( loc_pchr->life_max, loc_pchr->life );

            tmpstr = describe_value( actual_damage, UINT_TO_UFP8( 10 ), &rank );
            if ( rank < 4 )
            {
                tmpstr = describe_value( actual_damage, max_damage, &rank );
                if ( rank < 0 )
                {
                    tmpstr = "Fumble!";
                }
                else
                {
                    tmpstr = describe_damage( actual_damage, loc_pchr->life_max, &rank );
                    if ( rank >= -1 && rank <= 1 )
                    {
                        tmpstr = describe_wounds( loc_pchr->life_max, loc_pchr->life );
                    }
                }
            }

            if ( NULL != tmpstr )
            {
                const float lifetime = 3;
                STRING text_buffer = EMPTY_CSTR;

                // "white" text
                SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};

                // friendly fire damage = "purple"
                GLXvector4f tint_friend = { 0.88f, 0.75f, 1.00f, 1.00f };

                // enemy damage = "red"
                GLXvector4f tint_enemy  = { 1.00f, 0.75f, 0.75f, 1.00f };

                // write the string into the buffer
                SDL_snprintf( text_buffer, SDL_arraysize( text_buffer ), "%s", tmpstr );

                chr_make_text_billboard( loc_ichr, text_buffer, text_color, friendly_fire ? tint_friend : tint_enemy, lifetime, ( BIT_FIELD )bb_opt_all );
            }
        }
    }
    else if ( actual_damage < 0 )
    {
        int actual_heal = 0;

        // heal the character
        actual_heal = damage_character_heal( bdl, -actual_damage, attacker, ignore_invictus );

        // Isssue an alert
        if ( team == TEAM_DAMAGE )
        {
            loc_pai->attacklast = CHR_REF( MAX_CHR );
        }

        /// \test spawn a fly-away heal indicator?
        if ( do_feedback )
        {
            const float lifetime = 3;
            STRING text_buffer = EMPTY_CSTR;

            // "white" text
            SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};

            // heal == yellow, right ;)
            GLXvector4f tint = { 1.00f, 1.00f, 0.75f, 1.00f };

            // write the string into the buffer
            SDL_snprintf( text_buffer, SDL_arraysize( text_buffer ), "%s", describe_value( actual_heal, damage.base + damage.rand, NULL ) );

            chr_make_text_billboard( loc_ichr, text_buffer, text_color, tint, lifetime, ( BIT_FIELD )bb_opt_all );
        }
    }

    return actual_damage;
}

//--------------------------------------------------------------------------------------------
void spawn_defense_ping( ego_chr *pchr, const CHR_REF & attacker )
{
    /// \author ZF
    /// \details  Spawn a defend particle
    if ( pchr->damagetime != 0 ) return;

    spawn_one_particle_global( pchr->pos, pchr->ori.facing_z, PIP_DEFEND, 0 );

    pchr->damagetime    = DEFENDTIME;
    ADD_BITS( pchr->ai.alert, ALERTIF_BLOCKED );
    pchr->ai.attacklast = attacker;                 // For the ones attacking a shield
}

//--------------------------------------------------------------------------------------------
void spawn_poof( const CHR_REF & character, const PRO_REF & profile )
{
    /// \author ZZ
    /// \details  This function spawns a character poof

    FACING_T facing_z;
    CHR_REF  origin;
    int      cnt;

    ego_chr * pchr;
    ego_cap * pcap;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    pcap = pro_get_pcap( profile );
    if ( NULL == pcap ) return;

    origin = pchr->ai.owner;
    facing_z   = pchr->ori.facing_z;
    for ( cnt = 0; cnt < pcap->gopoofprt_amount; cnt++ )
    {
        spawn_one_particle( pchr->pos_old, facing_z, profile, pcap->gopoofprt_pip,
                            CHR_REF( MAX_CHR ), GRIP_LAST, pchr->team, origin, PRT_REF( MAX_PRT ), cnt, CHR_REF( MAX_CHR ) );

        facing_z += pcap->gopoofprt_facingadd;
    }
}

//--------------------------------------------------------------------------------------------
void ego_ai_state::spawn( ego_ai_state * pself, const CHR_REF & index, const PRO_REF & iobj, const Uint16 rank )
{
    ego_chr * pchr;
    ego_pro * ppro;
    ego_cap * pcap;

    pself = ego_ai_state::ctor_this( pself );

    if ( NULL == pself || !DEFINED_CHR( index ) ) return;
    pchr = ChrObjList.get_data_ptr( index );

    // a character cannot be spawned without a valid profile
    if ( !LOADED_PRO( iobj ) ) return;
    ppro = ProList.lst + iobj;

    // a character cannot be spawned without a valid cap
    pcap = pro_get_pcap( iobj );
    if ( NULL == pcap ) return;

    pself->index      = index;
    pself->type       = ppro->iai;
    pself->alert      = ALERTIF_SPAWNED;
    pself->state      = pcap->state_override;
    pself->content    = pcap->content_override;
    pself->passage    = 0;
    pself->target     = index;
    pself->owner      = index;
    pself->child      = index;
    pself->target_old = index;

    pself->bumplast   = index;
    pself->hitlast    = index;

    pself->order_counter = rank;
    pself->order_value   = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF spawn_one_character( fvec3_t pos, const PRO_REF & profile, const TEAM_REF & team,
                             const Uint8 skin, const FACING_T facing, const char *name, const CHR_REF & override )
{
    /// \author ZZ
    /// \details  This function spawns a character and returns the character's index number
    ///               if it worked, MAX_CHR otherwise

    CHR_REF       ichr_obj;
    ego_obj_chr * pchr_obj;

    CAP_REF       icap;
    ego_cap     * pcap;

    // fix a "bad" name
    if ( NULL == name ) name = "";

    if ( profile >= MAX_PROFILE )
    {
        log_warning( "%s - profile value too large %d out of %d\n", __FUNCTION__, profile.get_value(), MAX_PROFILE );
        return CHR_REF( MAX_CHR );
    }

    if ( !LOADED_PRO( profile ) )
    {
        if ( profile > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "%s - trying to spawn using invalid profile %d\n", __FUNCTION__, profile.get_value() );
        }
        return CHR_REF( MAX_CHR );
    }

    icap = pro_get_icap( profile );
    if ( !LOADED_CAP( icap ) )
    {
        log_warning( "%s - trying to spawn using unknown character profile (data.txt) %d\n", __FUNCTION__, icap.get_value() );

        return CHR_REF( MAX_CHR );
    }
    pcap = CapStack + icap;

    // count all the requests for this character type
    pcap->request_count++;

    // allocate a new character
    ichr_obj = ChrObjList.allocate( override );
    if ( !DEFINED_CHR( ichr_obj ) )
    {
        log_warning( "%s - failed to spawn character (invalid index number %d?)\n", __FUNCTION__, ichr_obj.get_value() );
        return CHR_REF( MAX_CHR );
    }
    pchr_obj = ChrObjList.get_data_ptr( ichr_obj );

    // just set the spawn info
    pchr_obj->spawn_data.pos      = pos;
    pchr_obj->spawn_data.profile  = profile;
    pchr_obj->spawn_data.team     = team;
    pchr_obj->spawn_data.skin     = skin;
    pchr_obj->spawn_data.facing   = facing;
    strncpy( pchr_obj->spawn_data.name, name, SDL_arraysize( pchr_obj->spawn_data.name ) );
    pchr_obj->spawn_data.override = override;

    POBJ_BEGIN_SPAWN( pchr_obj );

    // actually force the character to spawn
    if ( NULL != ego_object_engine::run_activate( pchr_obj, 100 ) )
    {
        pcap->create_count++;
    }

#if defined(DEBUG_OBJECT_SPAWN) && EGO_DEBUG
    {
        log_debug( "%s - slot: %i, index: %i, name: %s, class: %s\n", __FUNCTION__, profile.get_value(), ichr_obj.get_value(), name, pcap->classname );
    }
#endif

    return ichr_obj;
}

//--------------------------------------------------------------------------------------------
void ego_chr::respawn( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function respawns a character

    CHR_REF item;
    int old_attached_prt_count, new_attached_prt_count;

    ego_chr * pchr;
    ego_cap * pcap;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return;

    if ( pchr->alive ) return;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return;

    old_attached_prt_count = number_of_attached_particles( character );

    spawn_poof( character, pchr->profile_ref );
    disaffirm_attached_particles( character );

    pchr->alive = btrue;
    pchr->boretime = BORETIME;
    pchr->carefultime = CAREFULTIME;
    pchr->life = pchr->life_max;
    pchr->mana = pchr->mana_max;
    ego_chr::set_pos( pchr, pchr->pos_stt.v );
    pchr->vel.x = 0;
    pchr->vel.y = 0;
    pchr->vel.z = 0;
    pchr->team = pchr->baseteam;
    pchr->canbecrushed = bfalse;
    pchr->ori.map_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    pchr->ori.map_facing_x = MAP_TURN_OFFSET;
    if ( NOLEADER == TeamStack[pchr->team].leader )  TeamStack[pchr->team].leader = character;
    if ( !IS_INVICTUS_PCHR_RAW( pchr ) )  TeamStack[pchr->baseteam].morale++;

    // start the character out in the "dance" animation
    ego_chr::start_anim( pchr, ACTION_DA, btrue, btrue );

    // reset all of the bump size information
    {
        float old_fat = pchr->fat;
        ego_chr::init_size( pchr, pcap );
        ego_chr::set_fat( pchr, old_fat );
    }

    pchr->platform        = pcap->platform;
    pchr->canuseplatforms = pcap->canuseplatforms;
    ego_chr_data::set_fly_height( pchr, pcap->fly_height );
    pchr->phys.bumpdampen = pcap->bumpdampen;

    pchr->ai.alert = ALERTIF_CLEANEDUP;
    pchr->ai.target = character;
    pchr->ai.timer  = 0;

    pchr->grogtime = 0;
    pchr->dazetime = 0;

    // Let worn items come back
    PACK_BEGIN_LOOP( item, pchr->pack.next )
    {
        if ( INGAME_CHR( item ) && ChrObjList.get_data_ref( item ).isequipped )
        {
            ChrObjList.get_data_ref( item ).isequipped = bfalse;
            ADD_BITS( ego_chr::get_pai( item )->alert, ALERTIF_PUTAWAY ); // same as ALERTIF_ATLASTWAYPOINT
        }
    }
    PACK_END_LOOP( item );

    // re-initialize the instance
    gfx_mad_instance::spawn( &( pchr->gfx_inst ), pchr->profile_ref, pchr->skin );
    ego_chr::update_matrix( pchr, btrue );

    // determine whether the object is hidden
    ego_chr::update_hide( pchr );

    if ( !pchr->is_hidden )
    {
        reaffirm_attached_particles( character );
        new_attached_prt_count = number_of_attached_particles( character );
    }

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, btrue );
}

//--------------------------------------------------------------------------------------------
int ego_chr::change_skin( const CHR_REF & character, const Uint32 skin )
{
    ego_chr * pchr;
    ego_pro * ppro;
    ego_mad * pmad;
    gfx_mad_instance * pgfx_inst;
    mad_instance     * pmad_inst;

    Uint32 loc_skin = skin;

    pchr  = ChrObjList.get_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return 0;

    pgfx_inst = &( pchr->gfx_inst );
    pmad_inst = &( pchr->mad_inst );

    ppro = ego_chr::get_ppro( character );

    pmad = pro_get_pmad( pchr->profile_ref );
    if ( NULL == pmad )
    {
        // make sure that the instance has a valid imad
        if ( NULL != ppro && !LOADED_MAD( pmad_inst->imad ) )
        {
            if ( ego_chr::set_mad( pchr, ppro->imad ) )
            {
                ego_chr::update_collision_size( pchr, btrue );
            }
            pmad = ego_chr::get_pmad( character );
        }
    }

    if ( NULL == pmad )
    {
        pchr->skin     = 0;
        pgfx_inst->texture = TX_WATER_TOP;
    }
    else
    {
        TX_REF txref = TX_REF( TX_WATER_TOP );

        // do the best we can to change the loc_skin
        if ( NULL == ppro || 0 == ppro->skins )
        {
            ppro->skins = 1;
            ppro->tex_ref[0] = TX_WATER_TOP;

            loc_skin  = 0;
            txref = TX_WATER_TOP;
        }
        else
        {
            if ( loc_skin > ppro->skins )
            {
                loc_skin = 0;
            }

            txref = ppro->tex_ref[loc_skin];
        }

        pchr->skin     = loc_skin;
        pgfx_inst->texture = txref;
    }

    // If the we are respawning a player, then the camera needs to be reset
    if ( IS_PLAYER_PCHR( pchr ) )
    {
        ego_camera::reset_target( PCamera, PMesh );
    }

    return pchr->skin;
}

//--------------------------------------------------------------------------------------------
int ego_chr::change_armor( const CHR_REF & character, const int skin )
{
    /// \author ZZ
    /// \details  This function changes the armor of the character

    ENC_REF enchant;
    int     iTmp;
    ego_cap * pcap;
    ego_chr * pchr;

    int loc_skin = skin;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return 0;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Remove armor enchantments
    enchant = pchr->firstenchant;
    while ( enchant < MAX_ENC )
    {
        ego_enc::remove_set( enchant, SETSLASHMODIFIER );
        ego_enc::remove_set( enchant, SETCRUSHMODIFIER );
        ego_enc::remove_set( enchant, SETPOKEMODIFIER );
        ego_enc::remove_set( enchant, SETHOLYMODIFIER );
        ego_enc::remove_set( enchant, SETEVILMODIFIER );
        ego_enc::remove_set( enchant, SETFIREMODIFIER );
        ego_enc::remove_set( enchant, SETICEMODIFIER );
        ego_enc::remove_set( enchant, SETZAPMODIFIER );

        enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
    }

    // Change the loc_skin
    pcap = ego_chr::get_pcap( character );
    loc_skin = ego_chr::change_skin( character, loc_skin );

    // Change stats associated with loc_skin
    pchr->defense = pcap->defense[loc_skin];

    for ( iTmp = 0; iTmp < DAMAGE_COUNT; iTmp++ )
    {
        pchr->damagemodifier[iTmp] = pcap->damagemodifier[iTmp][loc_skin];
    }

    // set the character's maximum acceleration
    ego_chr::set_maxaccel( pchr, pcap->maxaccel[loc_skin] );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Reset armor enchantments
    /// \todo These should really be done in reverse order ( Start with last enchant ), but
    /// I don't care at this point !!!BAD!!!
    enchant = pchr->firstenchant;
    while ( enchant < MAX_ENC )
    {
        PRO_REF ipro = ego_enc::get_ipro( enchant );

        if ( LOADED_PRO( ipro ) )
        {
            EVE_REF ieve = pro_get_ieve( ipro );

            ego_enc::apply_set( enchant, SETSLASHMODIFIER, ipro );
            ego_enc::apply_set( enchant, SETCRUSHMODIFIER, ipro );
            ego_enc::apply_set( enchant, SETPOKEMODIFIER,  ipro );
            ego_enc::apply_set( enchant, SETHOLYMODIFIER,  ipro );
            ego_enc::apply_set( enchant, SETEVILMODIFIER,  ipro );
            ego_enc::apply_set( enchant, SETFIREMODIFIER,  ipro );
            ego_enc::apply_set( enchant, SETICEMODIFIER,   ipro );
            ego_enc::apply_set( enchant, SETZAPMODIFIER,   ipro );

            ego_enc::apply_add( enchant, ADDACCEL,         ieve );
            ego_enc::apply_add( enchant, ADDDEFENSE,       ieve );
        }

        enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
    }

    return loc_skin;
}

//--------------------------------------------------------------------------------------------
void change_character_full( const CHR_REF & ichr, const PRO_REF & profile, const Uint8 skin, const Uint8 leavewhich )
{
    /// \author ZF
    /// \details  This function polymorphs a character permanently so that it can be exported properly
    /// A character turned into a frog with this function will also export as a frog!

    MAD_REF imad_old, imad_new;

    if ( !LOADED_PRO( profile ) ) return;

    imad_new = pro_get_imad( profile );
    if ( !LOADED_MAD( imad_new ) ) return;

    imad_old = ego_chr::get_imad( ichr );
    if ( !LOADED_MAD( imad_old ) ) return;

    // copy the new name
    strncpy( MadStack[imad_old].name, MadStack[imad_new].name, SDL_arraysize( MadStack[imad_old].name ) );

    // change their model
    ego_chr::change_profile( ichr, profile, skin, leavewhich );

    // set the base model to the new model, too
    ChrObjList.get_data_ref( ichr ).basemodel_ref = profile;
}

//--------------------------------------------------------------------------------------------
bool_t set_weapongrip( const CHR_REF & iitem, const CHR_REF & iholder, const Uint16 vrt_off )
{
    int i;

    bool_t needs_update;
    Uint16 grip_verts[GRIP_VERTS];

    gfx_mad_matrix_data * mcache;
    ego_chr * pitem;

    needs_update = bfalse;

    pitem = ChrObjList.get_allocated_data_ptr( iitem );
    if ( !INGAME_PCHR( pitem ) ) return bfalse;
    mcache = &( pitem->gfx_inst.mcache );

    // is the item attached to this valid holder?
    if ( pitem->attachedto != iholder ) return bfalse;

    needs_update  = btrue;

    if ( GRIP_VERTS == get_grip_verts( grip_verts, iholder, vrt_off ) )
    {
        //---- detect any changes in the mcache data

        needs_update  = bfalse;

        if ( iholder != mcache->grip_chr || pitem->attachedto != iholder )
        {
            needs_update  = btrue;
        }

        if ( pitem->inwhich_slot != mcache->grip_slot )
        {
            needs_update  = btrue;
        }

        // check to see if any of the
        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            if ( grip_verts[i] != mcache->grip_verts[i] )
            {
                needs_update = btrue;
                break;
            }
        }
    }

    if ( needs_update )
    {
        // cannot create the matrix, therefore the current matrix must be invalid
        mcache->matrix_valid = bfalse;

        mcache->grip_chr  = iholder;
        mcache->grip_slot = pitem->inwhich_slot;

        for ( i = 0; i < GRIP_VERTS; i++ )
        {
            mcache->grip_verts[i] = grip_verts[i];
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void ego_chr::change_profile( const CHR_REF & ichr, const PRO_REF & profile_new, const Uint8 skin, const Uint8 leavewhich )
{
    /// \author ZZ
    /// \details  This function polymorphs a character, changing stats, dropping weapons

    int tnc;
    ENC_REF enchant;
    CHR_REF item_ref, item;
    ego_chr * pchr;

    ego_pro * pobj_new;
    ego_cap * pcap_new;
    ego_mad * pmad_new;

    int old_attached_prt_count, new_attached_prt_count;

    if ( !LOADED_PRO( profile_new ) ) return;

    pchr = ChrObjList.get_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return;

    old_attached_prt_count = number_of_attached_particles( ichr );

    if ( !LOADED_PRO( profile_new ) ) return;
    pobj_new = ProList.lst + profile_new;

    pcap_new = pro_get_pcap( profile_new );
    pmad_new = pro_get_pmad( profile_new );

    // Drop left weapon
    item_ref = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( item_ref ) && ( !pcap_new->slotvalid[SLOT_LEFT] || pcap_new->ismount ) )
    {
        ego_chr * pitem = ChrObjList.get_data_ptr( item_ref );

        detach_character_from_mount( item_ref, btrue, btrue );
        detach_character_from_platform( pitem );

        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            pitem->vel.z    = DISMOUNTZVEL;
            pitem->jump_time = JUMP_DELAY;

            tmp_pos = ego_chr::get_pos( pitem );
            tmp_pos.z += DISMOUNTZVEL;
            ego_chr::set_pos( pitem, tmp_pos.v );
        }
    }

    // Drop right weapon
    item_ref = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( item_ref ) && !pcap_new->slotvalid[SLOT_RIGHT] )
    {
        ego_chr * pitem = ChrObjList.get_data_ptr( item_ref );

        detach_character_from_mount( item_ref, btrue, btrue );
        detach_character_from_platform( pitem );

        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            pitem->vel.z    = DISMOUNTZVEL;
            pitem->jump_time = JUMP_DELAY;

            tmp_pos = ego_chr::get_pos( pitem );
            tmp_pos.z += DISMOUNTZVEL;
            ego_chr::set_pos( pitem, tmp_pos.v );
        }
    }

    // Remove particles
    disaffirm_attached_particles( ichr );

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Remove enchantments
    if ( leavewhich == ENC_LEAVE_FIRST )
    {
        // cleanup the enchant list
        cleanup_character_enchants( pchr );

        // Remove all enchantments except top one
        enchant = pchr->firstenchant;
        if ( enchant != MAX_ENC )
        {
            enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
            while ( MAX_ENC != enchant )
            {
                remove_enchant( enchant, NULL );

                enchant = EncObjList.get_data_ref( enchant ).nextenchant_ref;
            }
        }
    }
    else if ( ENC_LEAVE_NONE == leavewhich )
    {
        // Remove all enchantments
        remove_all_character_enchants( ichr );
    }

    // Stuff that must be set
    pchr->profile_ref  = profile_new;
    pchr->stoppedby = pcap_new->stoppedby;
    pchr->life_heal  = pcap_new->life_heal;
    pchr->manacost  = pcap_new->manacost;

    // Ammo
    pchr->ammo_max = pcap_new->ammo_max;
    pchr->ammo    = pcap_new->ammo;

    // Gender
    if ( pcap_new->gender != GENDER_RANDOM )  // GENDER_RANDOM means keep old gender
    {
        pchr->gender = pcap_new->gender;
    }

    // Sound effects
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->sound_index[tnc] = pcap_new->sound_index[tnc];
    }

    // AI stuff
    pchr->ai.type           = pobj_new->iai;
    pchr->ai.state          = 0;
    pchr->ai.timer          = 0;
    pchr->turnmode          = TURNMODE_VELOCITY;

    latch_game_init( &( pchr->latch ) );

    // Flags
    pchr->stickybutt      = pcap_new->stickybutt;
    pchr->openstuff       = pcap_new->canopenstuff;
    pchr->transferblend   = pcap_new->transferblend;
    pchr->platform        = pcap_new->platform;
    pchr->canuseplatforms = pcap_new->canuseplatforms;
    pchr->isitem          = pcap_new->isitem;
    pchr->invictus        = pcap_new->invictus;
    pchr->ismount         = pcap_new->ismount;
    pchr->cangrabmoney    = pcap_new->cangrabmoney;
    pchr->jump_time        = JUMP_DELAY;
    pchr->alpha_base       = pcap_new->alpha;
    pchr->light_base       = pcap_new->light;

    // change the skillz, too, jack!
    idsz_map_copy( pcap_new->skills, SDL_arraysize( pcap_new->skills ), pchr->skills );
    pchr->darkvision_level = ego_chr_data::get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
    pchr->see_invisible_level = pcap_new->see_invisible_level;

    /// \note BB@> changing this could be disastrous, in case you can't un-morph yourself???
    /// pchr->canusearcane          = pcap_new->canusearcane;
    /// \note ZF@> No, we want this, I have specifically scripted morph books to handle unmorphing
    /// even if you cannot cast arcane spells. Some morph spells specifically morph the player
    /// into a fighter or a tech user, but as a balancing factor prevents other spellcasting.

    // Character size and bumping
    // set the character size so that the new model is the same size as the old model
    // the model will then morph its size to the correct size over time
    {
        float old_fat = pchr->fat;
        float new_fat;

        if ( 0.0f == pchr->bump_stt.size )
        {
            new_fat = pcap_new->size;
        }
        else
        {
            new_fat = ( pcap_new->bump_size * pcap_new->size ) / pchr->bump.size;
        }

        // Spellbooks should stay the same size, even if their spell effect cause changes in size
        if ( pchr->profile_ref == SPELLBOOK ) new_fat = old_fat = 1.00f;

        // copy all the cap size info over, as normal
        ego_chr::init_size( pchr, pcap_new );

        // make the model's size congruent
        if ( 0.0f != new_fat && new_fat != old_fat )
        {
            ego_chr::set_fat( pchr, new_fat );
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = SIZETIME;
        }
        else
        {
            ego_chr::set_fat( pchr, old_fat );
            pchr->fat_goto      = old_fat;
            pchr->fat_goto_time = 0;
        }
    }

    // Physics
    pchr->phys.bumpdampen     = pcap_new->bumpdampen;

    if ( CAP_INFINITE_WEIGHT == pcap_new->weight )
    {
        pchr->phys.weight = INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pcap_new->weight * pchr->fat * pchr->fat * pchr->fat;
        pchr->phys.weight = SDL_min( itmp, MAX_WEIGHT );
    }

    // Image rendering
    pchr->uoffvel = pcap_new->uoffvel;
    pchr->voffvel = pcap_new->voffvel;

    // Movement
    pchr->anim_speed_sneak = pcap_new->anim_speed_sneak;
    pchr->anim_speed_walk  = pcap_new->anim_speed_walk;
    pchr->anim_speed_run   = pcap_new->anim_speed_run;

    // initialize the instance
    gfx_mad_instance::spawn( &( pchr->gfx_inst ), profile_new, skin );
    ego_chr::update_matrix( pchr, btrue );

    // Action stuff that must be down after gfx_mad_instance::spawn()
    pchr->mad_inst.action.ready = bfalse;
    pchr->mad_inst.action.keep  = bfalse;
    pchr->mad_inst.action.loop  = bfalse;
    if ( pchr->alive )
    {
        ego_chr::play_action( pchr, ACTION_DA, bfalse );
    }
    else
    {
        ego_chr::play_action( pchr, ACTION_KA + generate_randmask( 0, 3 ), bfalse );
        pchr->mad_inst.action.keep = btrue;
    }

    // Set the skin after changing the model in gfx_mad_instance::spawn()
    ego_chr::change_armor( ichr, skin );

    // Must set the weapon grip AFTER the model is changed in gfx_mad_instance::spawn()
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        set_weapongrip( ichr, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );
    }

    item = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( item ) )
    {
        EGOBOO_ASSERT( ChrObjList.get_data_ref( item ).attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_LEFT );
    }

    item = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( item ) )
    {
        EGOBOO_ASSERT( ChrObjList.get_data_ref( item ).attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_RIGHT );
    }

    // determine whether the object is hidden
    ego_chr::update_hide( pchr );

    // Reaffirm them particles...
    pchr->reaffirmdamagetype = pcap_new->attachedprt_reaffirmdamagetype;

    /// \note ZF@> remove this line so that books don't burn when dropped
    //reaffirm_attached_particles( ichr );

    new_attached_prt_count = number_of_attached_particles( ichr );

    ego_ai_state::set_changed( &( pchr->ai ) );

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, btrue );
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( const CHR_REF & character, const int amount, const CHR_REF & killer )
{
    /// \author ZZ
    /// \details  This function takes mana from a character ( or gives mana ),
    ///    and returns btrue if the character had enough to pay, or bfalse
    ///    otherwise. This can kill a character in hard mode.

    int mana_final;
    bool_t mana_paid;

    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    mana_paid  = bfalse;
    mana_final = pchr->mana - amount;

    if ( mana_final < 0 )
    {
        int mana_debt = -mana_final;

        pchr->mana = 0;

        if ( pchr->canchannel )
        {
            pchr->life -= mana_debt;

            if ( pchr->life <= 0 && cfg.difficulty >= GAME_HARD )
            {
                kill_character( character, !INGAME_CHR( killer ) ? character : killer, bfalse );
            }

            mana_paid = btrue;
        }
    }
    else
    {
        int mana_surplus = 0;

        pchr->mana = mana_final;

        if ( mana_final > pchr->mana_max )
        {
            mana_surplus = mana_final - pchr->mana_max;
            pchr->mana   = pchr->mana_max;
        }

        // allow surplus mana to go to health if you can channel?
        if ( pchr->canchannel && mana_surplus > 0 )
        {
            // use some factor, divide by 2
            heal_character( GET_REF_PCHR( pchr ), killer, mana_surplus << 1, btrue );
        }

        mana_paid = btrue;

    }

    return mana_paid;
}

//--------------------------------------------------------------------------------------------
void switch_team( const CHR_REF & character, const TEAM_REF & team )
{
    /// \author ZZ
    /// \details  This function makes a character join another team...

    if ( team >= TEAM_MAX ) return;

    ego_chr * pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_CHR( character ) ) return;

    if ( !IS_INVICTUS_PCHR_RAW( pchr ) )
    {
        if ( ego_chr::get_pteam_base( character )->morale > 0 ) ego_chr::get_pteam_base( character )->morale--;
        TeamStack[team].morale++;
    }

    if (( !pchr->ismount || !INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) ) &&
        ( !pchr->isitem  || !INGAME_CHR( pchr->attachedto ) ) )
    {
        pchr->team = team;
    }

    pchr->baseteam = team;
    if ( TeamStack[team].leader == NOLEADER )
    {
        TeamStack[team].leader = character;
    }
}

//--------------------------------------------------------------------------------------------
void issue_clean( const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function issues a clean up order to all teammates

    TEAM_REF team;

    if ( !INGAME_CHR( character ) ) return;

    team = ego_chr::get_iteam( character );

    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        if ( team != ego_chr::get_iteam( cnt ) ) continue;

        if ( !pchr->alive )
        {
            pchr->ai.timer  = update_wld + 2;  // Don't let it think too much...
        }

        ADD_BITS( pchr->ai.alert, ALERTIF_CLEANEDUP );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
int restock_ammo( const CHR_REF & character, const IDSZ idsz )
{
    /// \author ZZ
    /// \details  This function restocks the characters ammo, if it needs ammo and if
    ///    either its parent or type idsz match the given idsz.  This
    ///    function returns the amount of ammo given.

    int amount;

    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return 0;

    amount = 0;
    if ( ego_chr::is_type_idsz( character, idsz ) )
    {
        if ( pchr->ammo < pchr->ammo_max )
        {
            amount     = pchr->ammo_max - pchr->ammo;
            pchr->ammo = pchr->ammo_max;
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
/// implementation of the constructor
/// \note all *_REF members will be automatically constructed to an invalid value of ego_uint( ~0L )
/// The team and baseteam references shouold be initialized to some default team
ego_chr_data::ego_chr_data() :
        // character stats
        life( LOWSTAT ),
        mana( 0 ),

        experience( 0 ),
        experience_level( 0 ),

        // team stuff
        team( TEAM_NULL ),
        baseteam( TEAM_NULL ),

        fat( 1.0f ),
        fat_goto( 1.0f ),
        fat_goto_time( 0 ),

        // jump stuff
        jump_time( JUMP_DELAY ),
        jump_number( 1 ),
        jump_ready( btrue ),

        // attachments
        inwhich_slot( SLOT_LEFT ),

        // platform stuff
        holdingweight( 0 ),

        // combat stuff
        damageboost( 0 ),
        damagethreshold( 0 ),

        // sound stuff
        loopedsound_channel( INVALID_SOUND_CHANNEL ),

        // missile handling
        missiletreatment( MISSILE_NORMAL ),
        missilecost( 0 ),

        // "variable" properties
        is_hidden( bfalse ),
        alive( btrue ),
        waskilled( bfalse ),
        islocalplayer( bfalse ),
        hitready( btrue ),
        isequipped( bfalse ),

        // "constant" properties
        isshopitem( bfalse ),
        canbecrushed( bfalse ),
        canchannel( bfalse ),

        // misc timers
        grogtime( 0 ),
        dazetime( 0 ),
        boretime( BORETIME ),
        carefultime( CAREFULTIME ),
        reloadtime( 0 ),
        damagetime( 0 ),

        // graphics info
        sparkle( NOSPARKLE ),
        draw_stats( bfalse ),
        shadow_size( 30 ),
        shadow_size_save( 30 ),

        // model info
        is_overlay( bfalse ),

        // Skills
        darkvision_level( -1 ),
        see_kurse_level( -1 ),

        onwhichgrid( INVALID_TILE ),
        onwhichblock( INVALID_BLOCK ),

        // movement properties
        turnmode( TURNMODE_VELOCITY ),

        movement_bits( BIT_FIELD( ~0 ) ),
        maxaccel( 50 ),

        is_flying_platform( bfalse ),
        is_flying_jump( bfalse ),
        fly_height( 0.0f ),

        safe_valid( bfalse ),
        safe_time( 0 ),
        safe_grid( INVALID_TILE )
{
    ego_chr_data::ctor_this( this );
}

//--------------------------------------------------------------------------------------------
int ego_chr_data::get_skill( ego_chr_data *pchr, IDSZ whichskill )
{
    /// \author ZF
    /// \details  This returns the skill level for the specified skill or 0 if the character doesn't
    ///                  have the skill. Also checks the skill IDSZ.
    IDSZ_node_t *pskill;

    if ( NULL == pchr ) return bfalse;

    // Any [NONE] IDSZ returns always "true"
    if ( IDSZ_NONE == whichskill ) return 1;

    // Do not allow poison or backstab skill if we are restricted by code of conduct
    if ( MAKE_IDSZ( 'P', 'O', 'I', 'S' ) == whichskill || MAKE_IDSZ( 's', 'T', 'A', 'B' ) == whichskill )
    {
        if ( NULL != idsz_map_get( pchr->skills, SDL_arraysize( pchr->skills ), MAKE_IDSZ( 'C', 'O', 'D', 'E' ) ) )
        {
            return 0;
        }
    }

    // First check the character Skill ID matches
    // Then check for expansion skills too.
    if ( ego_chr::get_idsz( pchr->ai.index, IDSZ_SKILL )  == whichskill )
    {
        return 1;
    }

    // Simply return the skill level if we have the skill
    pskill = idsz_map_get( pchr->skills, SDL_arraysize( pchr->skills ), whichskill );
    if ( pskill != NULL )
    {
        return pskill->level;
    }

    // Truesight allows reading
    if ( MAKE_IDSZ( 'R', 'E', 'A', 'D' ) == whichskill )
    {
        pskill = idsz_map_get( pchr->skills, SDL_arraysize( pchr->skills ), MAKE_IDSZ( 'C', 'K', 'U', 'R' ) );
        if ( pskill != NULL && pchr->see_invisible_level > 0 )
        {
            return pchr->see_invisible_level + pskill->level;
        }
    }

    // Skill not found
    return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t update_chr_darkvision( const CHR_REF & character )
{
    /// \author BB
    /// \details  as an offset to negative status effects like things like poisoning, a
    ///               character gains darkvision ability the more they are "poisoned".
    ///               True poisoning can be removed by [HEAL] and tints the character green
    ego_eve * peve;
    ENC_REF ego_enc_now, ego_enc_next;
    int life_regen = 0;

    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // grab the life loss due poison to determine how much darkvision a character has earned, he he he!
    // clean up the enchant list before doing anything
    ego_enc_now = pchr->firstenchant;
    while ( ego_enc_now != MAX_ENC )
    {
        ego_enc_next = EncObjList.get_data_ref( ego_enc_now ).nextenchant_ref;
        peve = ego_enc::get_peve( ego_enc_now );

        // Is it true poison?
        if ( NULL != peve && MAKE_IDSZ( 'H', 'E', 'A', 'L' ) == peve->removedbyidsz )
        {
            life_regen += EncObjList.get_data_ref( ego_enc_now ).target_life;
            if ( EncObjList.get_data_ref( ego_enc_now ).owner_ref == pchr->ai.index ) life_regen += EncObjList.get_data_ref( ego_enc_now ).owner_life;
        }

        ego_enc_now = ego_enc_next;
    }

    if ( life_regen < 0 )
    {
        int tmp_level  = -( 10 * life_regen ) / pchr->life_max;                        // Darkvision gained by poison
        int base_level = ego_chr_data::get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );    // Natural darkvision

        // Use the better of the two darkvision abilities
        pchr->darkvision_level = SDL_max( base_level, tmp_level );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void update_all_characters()
{
    /// \author ZZ
    /// \details  This function updates stats and such for every character

    CHR_BEGIN_LOOP_DEFINED( ichr, pchr )
    {
        ego_object_engine::run( ego_chr::get_obj_ptr( pchr ) );
    }
    CHR_END_LOOP();

    // fix the stat timer
    if ( clock_chr_stat >= ONESECOND )
    {
        // Reset the clock
        clock_chr_stat -= ONESECOND;
    }

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_do_latch_attack( ego_chr * pchr, slot_t which_slot )
{
    ego_chr * pweapon;
    ego_cap * pweapon_cap;
    CHR_REF ichr, iweapon;
    MAD_REF imad;

    int    base_action, hand_action, action;
    bool_t action_valid, allowedtoattack;

    bool_t retval = bfalse;

    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;
    ichr = GET_REF_PCHR( pchr );

    imad = ego_chr::get_imad( ichr );

    if ( which_slot < 0 || which_slot >= SLOT_COUNT ) return bfalse;

    // Which iweapon?
    iweapon = pchr->holdingwhich[which_slot];
    if ( !INGAME_CHR( iweapon ) )
    {
        // Unarmed means character itself is the iweapon
        iweapon = ichr;
    }
    pweapon     = ChrObjList.get_data_ptr( iweapon );
    pweapon_cap = ego_chr::get_pcap( iweapon );

    // grab the iweapon's action
    base_action = pweapon_cap->weaponaction;
    hand_action = mad_randomize_action( base_action, which_slot );

    // see if the character can play this action
    action       = mad_get_action( imad, hand_action );
    action_valid = ( ACTION_COUNT != action );

    // Can it do it?
    allowedtoattack = btrue;

    // First check if reload time and action is okay
    if ( !action_valid || pweapon->reloadtime > 0 )
    {
        allowedtoattack = bfalse;
    }
    else
    {
        // Then check if a skill is needed
        if ( pweapon_cap->needskillidtouse )
        {
            if ( !ego_chr_data::get_skill( pchr, ego_chr::get_idsz( iweapon, IDSZ_SKILL ) ) )
            {
                allowedtoattack = bfalse;
            }
        }
    }

    // Don't allow users with kursed weapon in the other hand to use longbows
    if ( allowedtoattack && ACTION_IS_TYPE( action, L ) )
    {
        CHR_REF test_weapon;
        test_weapon = pchr->holdingwhich[which_slot == SLOT_LEFT ? SLOT_RIGHT : SLOT_LEFT];
        if ( INGAME_CHR( test_weapon ) )
        {
            ego_chr * weapon;
            weapon     = ChrObjList.get_data_ptr( test_weapon );
            if ( weapon->iskursed ) allowedtoattack = bfalse;
        }
    }

    if ( !allowedtoattack )
    {
        if ( 0 == pweapon->reloadtime )
        {
            // This character can't use this iweapon
            pweapon->reloadtime = 50;
            if ( pchr->draw_stats || cfg.dev_mode )
            {
                // Tell the player that they can't use this iweapon
                debug_printf( "%s can't use this item...", ego_chr::get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_CAPITAL ) );
            }
        }

        return bfalse;
    }

    if ( ACTION_DA == action )
    {
        allowedtoattack = bfalse;
        if ( 0 == pweapon->reloadtime )
        {
            ADD_BITS( pweapon->ai.alert, ALERTIF_USED );
        }
    }

    // deal with your mount (which could steal your attack)
    if ( allowedtoattack )
    {
        // Rearing mount
        CHR_REF   mount = pchr->attachedto;
        ego_chr * pmount = ChrObjList.get_data_ptr( mount );
        if ( INGAME_PCHR( pmount ) )
        {
            ego_cap * pmount_cap = ego_chr::get_pcap( mount );

            // let the mount steal the rider's attack
            if ( !pmount_cap->ridercanattack ) allowedtoattack = bfalse;

            // can the mount do anything?
            if ( pmount->ismount && pmount->alive )
            {
                // can the mount be told what to do?
                if ( !IS_PLAYER_PCHR( pmount ) && pmount->mad_inst.action.ready )
                {
                    if ( !ACTION_IS_TYPE( action, P ) || !pmount_cap->ridercanattack )
                    {
                        ego_chr::play_action( pmount, generate_randmask( ACTION_UA, 1 ), bfalse );
                        ADD_BITS( pmount->ai.alert, ALERTIF_USED );
                        pchr->ai.lastitemused = mount;

                        retval = btrue;
                    }
                }
            }
        }
    }

    // Attack button
    if ( allowedtoattack )
    {
        if ( pchr->mad_inst.action.ready && action_valid )
        {
            // Check mana cost
            bool_t mana_paid = cost_mana( ichr, pweapon->manacost, iweapon );

            if ( mana_paid )
            {
                Uint32 action_madfx = 0;
                bool_t action_uses_MADFX_ACT = bfalse;

                // Check life healing
                pchr->life += pweapon->life_heal;
                if ( pchr->life > pchr->life_max )  pchr->life = pchr->life_max;

                // randomize the action
                action = mad_randomize_action( action, which_slot );

                // make sure it is valid
                action = mad_get_action( imad, action );

                // grab the MADFX_* flags for this action
                action_madfx = mad_get_actionfx( imad, action );

                // If the attack action of the character holding the weapon does not have
                // MADFX_ACTLEFT or MADFX_ACTRIGHT bits (and so character_swipe() is never called)
                // then the action is played and the ALERTIF_USED bit is set in the chr_do_latch_attack()
                // function.
                //
                // It would be better to move all of this to the character_swipe() function, but we cannot be assured
                // that all models have the proper bits set.
                action_uses_MADFX_ACT = HAS_SOME_BITS( action, MADFX_ACTLEFT | MADFX_ACTRIGHT );

                if ( ACTION_IS_TYPE( action, P ) )
                {
                    // we must set parry actions to be interrupted by anything
                    ego_chr::play_action( pchr, action, btrue );
                }
                else
                {
                    float chr_dex = SFP8_TO_FLOAT( pchr->dexterity );

                    ego_chr::play_action( pchr, action, bfalse );

                    // Make the weapon animate the attack as well as the character holding it
                    if ( !action_uses_MADFX_ACT )
                    {
                        if ( iweapon != ichr )
                        {
                            // the attacking character has no bits in the animation telling it
                            // to use the weapon, so we play the animation here

                            // Make the iweapon attack too
                            ego_chr::play_action( pweapon, ACTION_MJ, bfalse );
                        }
                    }

                    // Determine the attack speed (how fast we play the animation)
                    pchr->mad_inst.action.rate = 0.125f;                       // base attack speed
                    pchr->mad_inst.action.rate += chr_dex / 40;                // +0.25f for every 10 dexterity

                    // Add some reload time as a true limit to attacks per second
                    // Dexterity decreases the reload time for all weapons. We could allow other stats like intelligence
                    // reduce reload time for spells or gonnes here.
                    if ( !pweapon_cap->attack_fast )
                    {
                        int base_reload_time = -chr_dex;
                        if ( ACTION_IS_TYPE( action, U ) ) base_reload_time += 50;          // Unarmed  (Fists)
                        else if ( ACTION_IS_TYPE( action, T ) ) base_reload_time += 45;     // Thrust   (Spear)
                        else if ( ACTION_IS_TYPE( action, C ) ) base_reload_time += 75;     // Chop     (Axe)
                        else if ( ACTION_IS_TYPE( action, S ) ) base_reload_time += 55;     // Slice    (Sword)
                        else if ( ACTION_IS_TYPE( action, B ) ) base_reload_time += 60;     // Bash     (Mace)
                        else if ( ACTION_IS_TYPE( action, L ) ) base_reload_time += 50;     // Longbow  (Longbow)
                        else if ( ACTION_IS_TYPE( action, X ) ) base_reload_time += 100;    // Xbow     (Crossbow)
                        else if ( ACTION_IS_TYPE( action, F ) ) base_reload_time += 50;     // Flinged  (Unused)

                        // it is possible to have so high dex to eliminate all reload time
                        if ( base_reload_time > 0 ) pweapon->reloadtime += base_reload_time;
                    }
                }

                // let everyone know what we did
                pchr->ai.lastitemused = iweapon;
                if ( iweapon == ichr || !action_uses_MADFX_ACT )
                {
                    ADD_BITS( pweapon->ai.alert, ALERTIF_USED );
                }

                retval = btrue;
            }
        }
    }

    // Reset boredom timer if the attack succeeded
    if ( retval )
    {
        pchr->boretime = BORETIME;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & chr_do_latch_button( ego_bundle_chr & bdl )
{
    /// \author BB
    /// \details  Character latches for generalized buttons

    CHR_REF       loc_ichr;
    ego_chr       * loc_pchr;
    ego_phys_data * loc_pphys;
    ego_ai_state  * loc_pai;

    CHR_REF item;
    bool_t attack_handled;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_ichr    = bdl.chr_ref();
    loc_pchr    = bdl.chr_ptr();
    loc_pphys   = &( loc_pchr->phys );
    loc_pai     = &( loc_pchr->ai );

    if ( !loc_pchr->alive || 0 == loc_pchr->latch.trans.b ) return bdl;

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_JUMP ) && 0 == loc_pchr->jump_time )
    {
        int ijump;

        bool_t can_jump;
        float jump_vel, jump_pos;
        int   jump_delay;

        can_jump   = bfalse;
        jump_delay = JUMP_DELAY;
        jump_vel   = 0.0f;
        jump_pos   = 0.0f;

        ego_chr * pmount = ChrObjList.get_data_ptr( loc_pchr->attachedto );
        if ( INGAME_PCHR( pmount ) )
        {
            // Jump from our mount

            detach_character_from_mount( loc_ichr, btrue, btrue );
            detach_character_from_platform( loc_pchr );

            can_jump = btrue;

            if ( pmount->is_flying_platform && pmount->platform )
            {
                jump_vel = DISMOUNTZVELFLY;
            }
            else
            {
                jump_vel = DISMOUNTZVEL;
            }

            // make a slight offset so that you won't get sucked back into the mount point
            jump_pos = jump_vel;
        }
        else if ( loc_pchr->jump_number > 0 && !IS_FLYING_PCHR( loc_pchr ) )
        {
            if ( loc_pchr->can_fly_jump )
            {
                // enter jump-flying mode?
                loc_pchr->is_flying_jump = !loc_pchr->enviro.inwater;

                can_jump = !loc_pchr->is_flying_jump;
            }
            else if ( loc_pchr->jump_ready || ( loc_pchr->jump_number_reset > 1 && 0 == loc_pchr->jump_time ) )
            {
                // Normal jump
                can_jump = btrue;
            }

            // Some environmental properties
            if ( can_jump )
            {
                if ( loc_pchr->enviro.inwater || loc_pchr->enviro.is_slippy )
                {
                    jump_delay = JUMP_DELAY * 4;         // To prevent 'bunny jumping' in water
                    jump_vel = JUMP_SPEED_WATER;
                }
                else
                {
                    jump_delay = JUMP_DELAY;
                    jump_vel   = loc_pchr->jump_power * 1.5f;
                }
            }
        }

        // do the jump
        if ( can_jump )
        {
            // set some jump states
            loc_pchr->hitready   = btrue;
            loc_pchr->jump_ready = bfalse;
            loc_pchr->jump_time = jump_delay;

            // make the character jump
            phys_data_accumulate_avel_index( loc_pphys, jump_vel, kZ );
            phys_data_accumulate_apos_plat_index( loc_pphys, jump_pos, kZ );

            // Set to jump animation if not doing anything better
            if ( loc_pchr->mad_inst.action.ready )
            {
                ego_chr::play_action( loc_pchr, ACTION_JA, btrue );
            }

            // down the jump counter
            if ( JUMP_NUMBER_INFINITE != loc_pchr->jump_number_reset && loc_pchr->jump_number >= 0 )
            {
                loc_pchr->jump_number--;
            }

            // Play the jump sound (Boing!)
            if ( NULL != bdl.cap_ptr() )
            {
                ijump = bdl.cap_ptr()->sound_index[SOUND_JUMP];
                if ( VALID_SND( ijump ) )
                {
                    sound_play_chunk( loc_pchr->pos, ego_chr::get_chunk_ptr( loc_pchr, ijump ) );
                }
            }
        }
    }

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_ALTLEFT ) && loc_pchr->mad_inst.action.ready && 0 == loc_pchr->reloadtime )
    {
        loc_pchr->reloadtime = GRABDELAY;
        if ( !INGAME_CHR( loc_pchr->holdingwhich[SLOT_LEFT] ) )
        {
            // Grab left
            ego_chr::play_action( loc_pchr, ACTION_ME, bfalse );
        }
        else
        {
            // Drop left
            ego_chr::play_action( loc_pchr, ACTION_MA, bfalse );
        }
    }

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_ALTRIGHT ) && loc_pchr->mad_inst.action.ready && 0 == loc_pchr->reloadtime )
    {
        loc_pchr->reloadtime = GRABDELAY;
        if ( !INGAME_CHR( loc_pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            // Grab right
            ego_chr::play_action( loc_pchr, ACTION_MF, bfalse );
        }
        else
        {
            // Drop right
            ego_chr::play_action( loc_pchr, ACTION_MB, bfalse );
        }
    }

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_PACKLEFT ) && loc_pchr->mad_inst.action.ready && 0 == loc_pchr->reloadtime )
    {
        loc_pchr->reloadtime = PACKDELAY;
        item = loc_pchr->holdingwhich[SLOT_LEFT];

        ego_chr * pitem = ChrObjList.get_data_ptr( item );
        if ( INGAME_PCHR( pitem ) )
        {
            if (( pitem->iskursed || pro_get_pcap( pitem->profile_ref )->istoobig ) && !pro_get_pcap( pitem->profile_ref )->isequipment )
            {
                // The item couldn't be put away
                ADD_BITS( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
                if ( IS_PLAYER_PCHR( loc_pchr ) )
                {
                    if ( pro_get_pcap( pitem->profile_ref )->istoobig )
                    {
                        debug_printf( "%s is too big to be put away...", ego_chr::get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                    else if ( pitem->iskursed )
                    {
                        debug_printf( "%s is sticky...", ego_chr::get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                }
            }
            else
            {
                // Put the item into the pack
                chr_inventory_add_item( item, loc_ichr );
            }
        }
        else
        {
            // Get a new one out and put it in hand
            chr_inventory_remove_item( loc_ichr, GRIP_LEFT, bfalse );
        }

        // Make it take a little time
        ego_chr::play_action( loc_pchr, ACTION_MG, bfalse );
    }

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_PACKRIGHT ) && loc_pchr->mad_inst.action.ready && 0 == loc_pchr->reloadtime )
    {
        loc_pchr->reloadtime = PACKDELAY;
        item = loc_pchr->holdingwhich[SLOT_RIGHT];
        ego_chr * pitem = ChrObjList.get_allocated_data_ptr( item );
        if ( INGAME_PCHR( pitem ) )
        {
            ego_cap * pitem_cap = ego_chr::get_pcap( item );

            if (( pitem->iskursed || pitem_cap->istoobig ) && !pitem_cap->isequipment )
            {
                // The item couldn't be put away
                ADD_BITS( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
                if ( IS_PLAYER_PCHR( loc_pchr ) )
                {
                    if ( pitem_cap->istoobig )
                    {
                        debug_printf( "%s is too big to be put away...", ego_chr::get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                    else if ( pitem->iskursed )
                    {
                        debug_printf( "%s is sticky...", ego_chr::get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
                    }
                }
            }
            else
            {
                // Put the item into the pack
                chr_inventory_add_item( item, loc_ichr );
            }
        }
        else
        {
            // Get a new one out and put it in hand
            chr_inventory_remove_item( loc_ichr, GRIP_RIGHT, bfalse );
        }

        // Make it take a little time
        ego_chr::play_action( loc_pchr, ACTION_MG, bfalse );
    }

    // LATCHBUTTON_LEFT and LATCHBUTTON_RIGHT are mutually exclusive
    attack_handled = bfalse;
    if ( !attack_handled && HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_LEFT ) && 0 == loc_pchr->reloadtime )
    {
        attack_handled = chr_do_latch_attack( loc_pchr, SLOT_LEFT );
    }
    if ( !attack_handled && HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_RIGHT ) && 0 == loc_pchr->reloadtime )
    {
        attack_handled = chr_do_latch_attack( loc_pchr, SLOT_RIGHT );
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_handle_madfx( ego_chr * pchr, BIT_FIELD madfx )
{
    ///\details This handles special commands an animation frame might execute, for example
    ///         grabbing stuff or spawning attack particles.
    CHR_REF ichr;

    if ( NULL == pchr || EMPTY_BIT_FIELD == madfx ) return bfalse;

    ichr = GET_REF_PCHR( pchr );

    // Check frame effects
    if ( HAS_SOME_BITS( madfx, MADFX_ACTLEFT ) )
    {
        character_swipe( ichr, SLOT_LEFT );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_ACTRIGHT ) )
    {
        character_swipe( ichr, SLOT_RIGHT );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_GRABLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, bfalse );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_GRABRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, bfalse );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_CHARLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, btrue );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_CHARRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, btrue );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_DROPLEFT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], bfalse, btrue );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_DROPRIGHT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], bfalse, btrue );
    }

    if ( HAS_SOME_BITS( madfx, MADFX_POOF ) && !IS_PLAYER_PCHR( pchr ) )
    {
        pchr->ai.poof_time = update_wld;
    }

    if ( HAS_SOME_BITS( madfx, MADFX_FOOTFALL ) )
    {
        ego_cap * pcap = pro_get_pcap( pchr->profile_ref );
        if ( NULL != pcap )
        {
            int ifoot = pcap->sound_index[SOUND_FOOTFALL];
            if ( VALID_SND( ifoot ) )
            {
                sound_play_chunk( pchr->pos, ego_chr::get_chunk_ptr( pchr, ifoot ) );
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int cmp_chr_anim_data( void const * vp_lhs, void const * vp_rhs )
{
    /// \author BB
    /// \details  Sort MOD REF values based on the rank of the module that they point to.
    ///               Trap all stupid values.

    ego_chr_anim_data * plhs = ( ego_chr_anim_data * )vp_lhs;
    ego_chr_anim_data * prhs = ( ego_chr_anim_data * )vp_rhs;

    int retval = 0;

    if ( NULL == plhs && NULL == prhs )
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

    retval = int( prhs->allowed ) - int( plhs->allowed );
    if ( 0 != retval ) return retval;

    retval = SGN( plhs->speed - prhs->speed );

    return retval;
}

//--------------------------------------------------------------------------------------------
float set_character_animation_rate( ego_chr * pchr )
{
    /// \author ZZ
    /// \details  Get running, walking, sneaking, or dancing, from speed
    ///
    /// \note BB@> added automatic calculation of variable animation rates for movement animations

    float  speed;
    int    action, lip;
    bool_t can_be_interrupted;
    bool_t is_walk_type;
    bool_t found;

    int           frame_count;
    ego_MD2_Frame * frame_list, * pframe_nxt;

    gfx_mad_instance * pgfx_inst;
    mad_instance     * pmad_inst;
    ego_mad          * pmad;
    CHR_REF          ichr;

    // set the character speed to zero
    speed = 0;

    if ( NULL == pchr ) return 1.0f;
    pgfx_inst = &( pchr->gfx_inst );
    pmad_inst = &( pchr->mad_inst );
    ichr  = GET_REF_PCHR( pchr );

    // get the model
    pmad = ego_chr::get_pmad( ichr );
    if ( NULL == pmad ) return pmad_inst->action.rate;

    // if the action is set to keep then do nothing
    can_be_interrupted = !pmad_inst->action.keep;
    if ( !can_be_interrupted ) return pmad_inst->action.rate = 1.0f;

    // don't change the rate if it is an attack animation
    if ( chr_is_attacking( pchr ) )    return pmad_inst->action.rate;

    // if the animation is not a walking-type animation, ignore the variable animation rates
    // and the automatic determination of the walk animation
    // "dance" is walking with zero speed
    is_walk_type = ACTION_IS_TYPE( pmad_inst->action.which, D ) || ACTION_IS_TYPE( pmad_inst->action.which, W );
    if ( !is_walk_type ) return pmad_inst->action.rate = 1.0f;

    // if the action cannot be changed on the at this time, there's nothing to do.
    // keep the same animation rate
    if ( !pmad_inst->action.ready )
    {
        if ( 0.0f == pmad_inst->action.rate ) pmad_inst->action.rate = 1.0f;
        return pmad_inst->action.rate;
    }

    // go back to a base animation rate, in case the next frame is not a
    // "variable speed frame"
    pmad_inst->action.rate = 1.0f;

    // for non-flying objects, you have to be touching the ground
    if ( !pchr->enviro.grounded && !pchr->is_flying_platform ) return pmad_inst->action.rate;

    // estimate our speed
    if ( pchr->is_flying_platform )
    {
        // for flying objects, the speed is the actual speed
        speed = fvec3_length_abs( pchr->vel.v );
    }
    else
    {
        // for non-flying objects, we use the intended speed

        // get the character animation speed from the speed of the character's legs.
        // if it is slipping, then the leg speed can be comically fast! :)
        speed = fvec3_length( pchr->enviro.legs_vel.v ) ;
    }

    if ( 0.0f != pchr->fat )
    {
        // if pchr->fat == 0.0f we have much bigger problems
        speed /= pchr->fat;
    }

    // get the rate based on the speed data in data.txt
    action = ACTION_DA;
    lip    = LIPDA;
    if ( SDL_abs( speed ) > 1.0f )
    {
        int             anim_count;
        ego_chr_anim_data anim_info[CHR_MOVEMENT_COUNT];
        int    cnt;

        //---- set up the anim_info structure
        anim_info[CHR_MOVEMENT_STOP ].speed = 0.0f;
        anim_info[CHR_MOVEMENT_SNEAK].speed = pchr->anim_speed_sneak;
        anim_info[CHR_MOVEMENT_WALK ].speed = pchr->anim_speed_walk;
        anim_info[CHR_MOVEMENT_RUN  ].speed = pchr->anim_speed_run;

        if ( pchr->is_flying_platform )
        {
            // for flying characters, you have to flap like crazy to stand still and
            // do nothing to move quickly
            anim_info[CHR_MOVEMENT_STOP ].action = ACTION_WC;
            anim_info[CHR_MOVEMENT_SNEAK].action = ACTION_WB;
            anim_info[CHR_MOVEMENT_WALK ].action = ACTION_WA;
            anim_info[CHR_MOVEMENT_RUN  ].action = ACTION_DA;
        }
        else
        {
            anim_info[CHR_MOVEMENT_STOP ].action = ACTION_DA;
            anim_info[CHR_MOVEMENT_SNEAK].action = ACTION_WA;
            anim_info[CHR_MOVEMENT_WALK ].action = ACTION_WB;
            anim_info[CHR_MOVEMENT_RUN  ].action = ACTION_WC;
        }

        anim_info[CHR_MOVEMENT_STOP ].lip = LIPDA;
        anim_info[CHR_MOVEMENT_SNEAK].lip = LIPWA;
        anim_info[CHR_MOVEMENT_WALK ].lip = LIPWB;
        anim_info[CHR_MOVEMENT_RUN  ].lip = LIPWC;

        // set up the arrays that are going tp
        // determine whether the various movements are allowed
        for ( cnt = 0; cnt < CHR_MOVEMENT_COUNT; cnt++ )
        {
            anim_info[cnt].allowed = HAS_SOME_BITS( pchr->movement_bits, 1 << cnt );
        }

        if ( ACTION_WA != pmad->action_map[ACTION_WA] )
        {
            // no specific sneak animation exists
            anim_info[CHR_MOVEMENT_SNEAK].allowed = bfalse;

            /// \note ZF@> small fix here, if there is no sneak animation, try to default to normal walk with reduced animation speed
            if ( HAS_SOME_BITS( pchr->movement_bits, CHR_MOVEMENT_BITS_SNEAK ) )
            {
                anim_info[CHR_MOVEMENT_WALK].allowed = btrue;
                anim_info[CHR_MOVEMENT_WALK].speed *= 2;
            }
        }

        if ( ACTION_WB != pmad->action_map[ACTION_WB] )
        {
            // no specific walk animation exists
            anim_info[CHR_MOVEMENT_WALK].allowed = bfalse;
        }

        if ( ACTION_WC != pmad->action_map[ACTION_WC] )
        {
            // no specific run animation exists
            anim_info[CHR_MOVEMENT_RUN].allowed = bfalse;
        }

        // count the allowed movements
        anim_count = 0;
        for ( cnt = 0; cnt < CHR_MOVEMENT_COUNT; cnt++ )
        {
            if ( anim_info[cnt].allowed )
            {
                anim_count++;
            }
        }

        // nothing to be done
        if ( 0 == anim_count )
        {
            return pmad_inst->action.rate;
        }

        // sort the allowed movement(s) data
        SDL_qsort( anim_info, CHR_MOVEMENT_COUNT, sizeof( ego_chr_anim_data ), cmp_chr_anim_data );

        // handle a special case
        if ( 1 == anim_count )
        {
            if ( 0.0f != anim_info[0].speed )
            {
                pmad_inst->action.rate = speed / anim_info[0].speed ;
            }

            return pmad_inst->action.rate;
        }

        // search for the correct animation
        found  = bfalse;
        for ( cnt = 0; cnt < anim_count - 1; cnt++ )
        {
            float speed_mid = 0.5f * ( anim_info[cnt].speed + anim_info[cnt+1].speed );

            // make a special case for dance animation(s)
            if ( 0.0f == anim_info[cnt].speed && speed <= 1e-3 )
            {
                found = btrue;
            }
            else
            {
                found = ( speed < speed_mid );
            }

            if ( found )
            {
                action = anim_info[cnt].action;
                lip    = anim_info[cnt].lip;
                if ( 0.0f != anim_info[cnt].speed )
                {
                    pmad_inst->action.rate = speed / anim_info[cnt].speed;
                }
                break;
            }
        }

        if ( !found )
        {
            action = anim_info[cnt].action;
            lip    = anim_info[cnt].lip;
            if ( 0.0f != anim_info[cnt].speed )
            {
                pmad_inst->action.rate = speed / anim_info[cnt].speed;
            }
            found = btrue;
        }

        if ( !found )
        {
            return pmad_inst->action.rate;
        }
    }

    frame_count = md2_get_numFrames( pmad->md2_ptr );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmad->md2_ptr );
    pframe_nxt  = frame_list + pmad_inst->state.frame_nxt;

    EGOBOO_ASSERT( pmad_inst->state.frame_nxt < frame_count );

    if ( ACTION_DA == action )
    {
        // Do standstill

        // handle boredom
        pchr->boretime--;
        if ( pchr->boretime < 0 )
        {
            int tmp_action, rand_val;

            ADD_BITS( pchr->ai.alert, ALERTIF_BORED );
            pchr->boretime = BORETIME;

            // set the action to "bored", which is ACTION_DB, ACTION_DC, or ACTION_DD
            rand_val   = RANDIE;
            tmp_action = mad_get_action( pmad_inst->imad, ACTION_DB + ( rand_val % 3 ) );
            ego_chr::start_anim( pchr, tmp_action, btrue, btrue );
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if ( !ACTION_IS_TYPE( pmad_inst->action.which, D ) )
            {
                // get an appropriate version of the boredom action
                int tmp_action = mad_get_action( pmad_inst->imad, ACTION_DA );

                // start the animation
                ego_chr::start_anim( pchr, tmp_action, btrue, btrue );
            }
        }
    }
    else
    {
        int tmp_action = mad_get_action( pmad_inst->imad, action );
        if ( ACTION_COUNT != tmp_action )
        {
            if ( pmad_inst->action.which != tmp_action )
            {
                ego_chr::set_anim( pchr, tmp_action, pmad->frameliptowalkframe[lip][pframe_nxt->framelip] );
            }

            // "loop" the action
            pmad_inst->action.next = tmp_action;
        }
    }

    pmad_inst->action.rate = CLIP( pmad_inst->action.rate, 0.1f, 10.0f );

    return pmad_inst->action.rate;
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_attacking( const ego_chr *pchr )
{
    return pchr->mad_inst.action.which >= ACTION_UA && pchr->mad_inst.action.which <= ACTION_FD;
}

//--------------------------------------------------------------------------------------------
bool_t chr_calc_environment( ego_chr * pchr )
{
    ego_bundle_chr  bdl( pchr );

    if ( NULL == bdl.chr_ptr() ) return bfalse;

    bdl = move_one_character_get_environment( bdl, NULL );

    return NULL != bdl.chr_ptr();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_get_environment( ego_bundle_chr & bdl, ego_chr_environment * penviro )
{
    Uint32 itile = INVALID_TILE;

    ego_chr             * loc_pchr;
    ego_phys_data       * loc_pphys;
    ego_chr_environment * loc_penviro;
    ego_ai_state        * loc_pai;

    float   grid_level, water_level;
    ego_chr * pplatform = NULL;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr  = bdl.chr_ptr();
    loc_pphys = &( loc_pchr->phys );
    loc_pai   = &( loc_pchr->ai );

    loc_penviro = penviro;
    if ( NULL == loc_penviro )
    {
        loc_penviro = &( loc_pchr->enviro );
    }

    // determine if the character is standing on a platform
    pplatform = NULL;
    if ( INGAME_CHR( loc_pchr->onwhichplatform_ref ) )
    {
        pplatform = ChrObjList.get_data_ptr( loc_pchr->onwhichplatform_ref );
    }

    // get the current tile
    itile = INVALID_TILE;
    if ( NULL != pplatform )
    {
        // this only works for 1 level of attachment
        itile = pplatform->onwhichgrid;
    }
    else
    {
        itile = loc_pchr->onwhichgrid;
    }

    //---- set the character's various levels

    // character "floor" level
    grid_level  = get_mesh_level( PMesh, loc_pchr->pos.x, loc_pchr->pos.y, bfalse );
    water_level = get_mesh_level( PMesh, loc_pchr->pos.x, loc_pchr->pos.y, btrue );
    ego_chr::set_enviro_grid_level( loc_pchr, grid_level );

    // The actual level of the character.
    if ( NULL != pplatform )
    {
        loc_penviro->walk_level = pplatform->pos.z + pplatform->chr_min_cv.maxs[OCT_Z];
    }
    else
    {
        loc_penviro->walk_level = loc_pchr->waterwalk ? water_level : grid_level;
    }

    // The flying height of the character, the maximum of tile level, platform level and water level
    loc_penviro->fly_level = loc_penviro->walk_level;
    if ( 0 != ego_mpd::test_fx( PMesh, loc_pchr->onwhichgrid, MPDFX_WATER ) )
    {
        loc_penviro->fly_level = SDL_max( loc_penviro->walk_level, water.surface_level );
    }

    if ( loc_penviro->fly_level < 0 )
    {
        loc_penviro->fly_level = 0;  // fly above pits...
    }

    //---- determine the character's various lerps after we have done everything to the character's levels we care to

    loc_penviro->grid_lerp = ( loc_pchr->pos.z - loc_penviro->grid_level ) / PLATTOLERANCE;
    loc_penviro->grid_lerp = CLIP( loc_penviro->grid_lerp, 0.0f, 1.0f );

    loc_penviro->walk_lerp = ( loc_pchr->pos.z - loc_penviro->walk_level ) / PLATTOLERANCE;
    loc_penviro->walk_lerp = CLIP( loc_penviro->walk_lerp, 0.0f, 1.0f );

    loc_penviro->fly_lerp = ( loc_pchr->pos.z - loc_penviro->fly_level ) / PLATTOLERANCE;
    loc_penviro->fly_lerp = CLIP( loc_penviro->fly_lerp, 0.0f, 1.0f );

    loc_penviro->grounded = !IS_FLYING_PCHR( loc_pchr ) && ( loc_penviro->walk_lerp < 0.25f );

    //---- the "twist" of the floor
    loc_penviro->grid_twist = TWIST_FLAT;
    if ( ego_mpd::grid_is_valid( PMesh, itile ) )
    {
        loc_penviro->grid_twist = PMesh->gmem.grid_list[itile].twist;
    }

    // get the "normal" vector for whatever the character is standing on
    if ( NULL != pplatform )
    {
        ego_chr::get_MatUp( pplatform, &loc_penviro->walk_nrm );
        fvec3_self_normalize( loc_penviro->walk_nrm.v );
    }
    else
    {
        loc_penviro->walk_nrm = map_twist_nrm[loc_penviro->grid_twist];
    }

    //---- the "watery-ness" of whatever water might be here

    // we will be "watery" if we are under water, even if on a platform
    loc_penviro->is_watery = water.is_water && loc_penviro->inwater;

    // we should only get slippy, however, if we are not on a platform
    loc_penviro->is_slippy = bfalse;
    if ( NULL == pplatform )
    {
        loc_penviro->is_slippy = !loc_penviro->is_watery && ( 0 != ego_mpd::test_fx( PMesh, loc_pchr->onwhichgrid, MPDFX_SLIPPY ) );
    }

    //---- traction
    loc_penviro->traction = 1.0f;
    if ( loc_penviro->is_slippy )
    {
        loc_penviro->traction /= 4.00f * hillslide;
        loc_penviro->traction = CLIP( loc_penviro->traction, 0.0f, 1.0f );
    }

    //---- the friction of the fluid we are in
    if ( loc_penviro->is_watery )
    {
        loc_penviro->fluid_friction_vrt = waterfriction;
        loc_penviro->fluid_friction_hrz = waterfriction;
    }
    else
    {
        loc_penviro->fluid_friction_hrz = loc_penviro->air_friction;       // like real-life air friction
        loc_penviro->fluid_friction_vrt = loc_penviro->air_friction;
    }

    //---- friction

    // give "mario platforms" a special exemption from vertical air friction
    // otherwise they will stop bouncing
    if ( loc_pchr->is_flying_platform && loc_pchr->platform && INFINITE_WEIGHT == loc_pphys->weight )
    {
        // override the z friction for "mario platforms".
        // friction in the z-direction will make the platform's bouncing motion stop
        loc_penviro->fluid_friction_vrt = 1.0f;
    }

    // Make the characters slide on flippy surfaces
    loc_penviro->friction_hrz = noslipfriction;
    if ( ego_mpd::grid_is_valid( PMesh, loc_pchr->onwhichgrid ) && loc_penviro->is_slippy )
    {
        // It's slippy all right...
        loc_penviro->friction_hrz = slippyfriction;
    }

    //---- jump stuff
    if ( IS_FLYING_PCHR( loc_pchr ) )
    {
        // Flying
        loc_pchr->jump_ready = bfalse;
    }
    else if ( DEFINED_CHR( loc_pchr->attachedto ) )
    {
        // Mounted
        loc_pchr->jump_ready = btrue;

        if ( 0 == loc_pchr->jump_time )
        {
            // Reset jumping
            loc_pchr->jump_number = SDL_min( 1, loc_pchr->jump_number_reset );
        }
    }
    else
    {
        // Is the character in the air?
        loc_pchr->jump_ready = loc_penviro->grounded;

        // Do ground hits
        if ( loc_penviro->grounded && loc_pchr->vel.z < -STOPBOUNCING && loc_pchr->hitready )
        {
            ADD_BITS( loc_pai->alert, ALERTIF_HITGROUND );
            loc_pchr->hitready = bfalse;
        }

        // Special considerations for slippy surfaces
        if ( loc_penviro->is_slippy )
        {
            if ( NULL != pplatform || map_twist_flat[loc_penviro->grid_twist] )
            {
                // Reset jumping on flat areas of slippiness
                if ( loc_penviro->grounded && 0 == loc_pchr->jump_time )
                {
                    loc_pchr->jump_number = loc_pchr->jump_number_reset;
                }
            }
        }
        else if ( loc_penviro->grounded && 0 == loc_pchr->jump_time )
        {
            // Reset jumping
            loc_pchr->jump_number = loc_pchr->jump_number_reset;
        }
    }

    //---- properties of the "ground" we are standing on

    // assume the ground isn't moving
    fvec3_self_clear( loc_penviro->ground_vel.v );

    // assume we have the normal friction with the ground
    loc_penviro->ground_fric = loc_penviro->friction_hrz;

    // are we attached to a platform?
    if ( NULL != pplatform )
    {
        // use the platform's up vector and platstick for the friction
        loc_penviro->ground_fric = platstick;
        loc_penviro->ground_vel  = pplatform->vel;
    }
    else if ( !loc_pchr->alive || loc_pchr->isitem )
    {
        // make "dead" objects and items have lots of friction with the ground
        loc_penviro->ground_fric = 0.5f;
    }

    // the velocity difference relative to the ground causes acceleration
    loc_penviro->ground_diff = fvec3_sub( loc_penviro->ground_vel.v, loc_pchr->vel.v );

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_fluid_friction( ego_bundle_chr & bdl )
{
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    // no fluid friction for "mario platforms"
    if ( loc_pchr->platform && loc_pchr->is_flying_platform && INFINITE_WEIGHT == loc_pphys->weight ) return bdl;

    // no fluid friction if you are riding or being carried
    if ( IS_ATTACHED_PCHR( loc_pchr ) ) return bdl;

    // Apply fluid friction from last time
    {
        fvec3_t _tmp_vec = VECT3(
                               -loc_pchr->vel.x * ( 1.0f - loc_penviro->fluid_friction_hrz ),
                               -loc_pchr->vel.y * ( 1.0f - loc_penviro->fluid_friction_hrz ),
                               -loc_pchr->vel.z * ( 1.0f - loc_penviro->fluid_friction_vrt ) );

        phys_data_accumulate_avel( loc_pphys, _tmp_vec.v );
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_voluntary_flying( ego_bundle_chr & bdl )
{
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_orientation     * loc_pori;

    float lift, throttle, maxspeed, speed_lerp, jump_lerp, tmp_flt, throttle_scale;
    float max_control, max_throttle, max_lift;
    float fluid_factor_222, fluid_factor;
    float yaw_factor;

    fvec3_t fly_up, fly_rt, fly_fw;
    fvec3_t mdl_up, mdl_rt, mdl_fw;
    fvec3_t acc_up, acc_rt, acc_fw, acc;
    fvec3_t loc_latch;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );
    loc_pori    = &( loc_pchr->ori );

    // auto-control a variety of factors based on the transition between running and flying
    jump_lerp = ( const float )loc_pchr->jump_time / ( const float )JUMP_DELAY;
    jump_lerp = CLIP( jump_lerp, 0.0f, 1.0f );

    // determine the net effect of the air friction on terminal velocity using the v2.22 method
    fluid_factor_222 = airfriction / ( 1.0f - airfriction );

    // determine the factor for the fluid we are actually flying through
    fluid_factor     = loc_penviro->fluid_friction_vrt / ( 1.0f - loc_penviro->fluid_friction_vrt );

    // let the flying speed be roughly 10x the running speed...
    throttle_scale = 3.0f;

    // scale the effectiveness of the throttle so that it won't make your speed go crazy in thin atmospheres
    if ( fluid_factor > fluid_factor_222 )
    {
        throttle_scale *= fluid_factor_222 / fluid_factor;
    }

    // the control surfaces can't exert maxaccel, so reduce their power
    max_control  = loc_pchr->maxaccel * 0.2f;
    max_throttle = loc_pchr->maxaccel * throttle_scale;
    max_lift     = -gravity + max_control;

    // the model's frame of reference
    mdl_up = mat_getChrUp( loc_pchr->gfx_inst.matrix );
    mdl_rt = mat_getChrRight( loc_pchr->gfx_inst.matrix );
    mdl_fw = mat_getChrForward( loc_pchr->gfx_inst.matrix );

    // a frame of reference for flying
    fly_fw = fvec3_normalize( mdl_fw.v );
    fly_up = fvec3_normalize( mdl_up.v );
    fly_rt = fvec3_normalize( mdl_rt.v );

    // determine a "maximum speed" using the v2.22 method
    maxspeed = loc_pchr->maxaccel * fluid_factor_222;

    // calculate a speed lerp so that the controls can have more effect when the
    // object is moving faster
    speed_lerp = 0.0f;
    if ( maxspeed > 0.0f && fvec3_length_abs( loc_pchr->vel.v ) > 0.0f )
    {
        speed_lerp = fvec3_dot_product( mdl_fw.v, loc_pchr->vel.v ) / maxspeed;

        // give the character a minimum of traction in the air (0.1)
        speed_lerp = CLIP( SDL_abs( speed_lerp ), 0.1f, 1.0f );
    }

    // initialize the control vector to 0
    fvec3_self_clear( loc_latch.v );

    // interpret the latch similar to an aeroplane's controls
    if ( loc_pchr->latch.raw_valid )
    {
        throttle = ( HAS_SOME_BITS( loc_pchr->latch.raw.b, LATCHBUTTON_JUMP ) ? 1.0f : 0.0f );

        // interpolate between the "taking off" conditions (max throttle, max climb, straight ahead )
        // and the player's control of the flying
        loc_latch.x += ( 1.0f - jump_lerp ) * loc_pchr->latch.raw.dir[kX];
        loc_latch.y += ( 1.0f - jump_lerp ) * loc_pchr->latch.raw.dir[kY];
        loc_latch.z += throttle;
    }

    // clear the sum of the accelerations
    fvec3_self_clear( acc.v );

    // determine the amount of acceleration in various directions
    // the lift and yaw are limited by the speed of the object
    acc_fw = fvec3_scale( fly_fw.v, loc_latch.z * max_throttle );

    // yaw effect
    tmp_flt = fvec3_dot_product( fly_rt.v, loc_pchr->vel.v );
    yaw_factor = SDL_abs( tmp_flt ) / maxspeed;
    acc_rt = fvec3_scale( fly_rt.v, - tmp_flt * 0.2f * ( 1.0f - jump_lerp ) * CLIP( yaw_factor, 0.0f, 1.0f ) );

    // angle of attack effects
    tmp_flt = fvec3_dot_product( fly_up.v, loc_pchr->vel.v );
    lift    = SGN( tmp_flt ) * tmp_flt * tmp_flt / maxspeed / maxspeed * max_lift;
    lift    = CLIP( lift, -max_lift, max_lift );
    acc_up  = fvec3_scale( fly_up.v, lift  - tmp_flt * 0.2f * ( 1.0f - jump_lerp ) );

    // sum up the accelerations
    fvec3_self_sum( acc.v, acc_rt.v );
    fvec3_self_sum( acc.v, acc_up.v );
    fvec3_self_sum( acc.v, acc_fw.v );

    // flying attitude controls
    // (float)TRIG_TABLE_SIZE / TARGET_UPS / 3.0f means that at maximum control, you should be able
    // co complete one rotation in 3 seconds

    // pitch
    loc_pori->map_facing_y += loc_latch.y * 0.125f * ( const float )TRIG_TABLE_SIZE / TARGET_UPS * ( 1.0f - jump_lerp );

    // roll
    loc_pori->map_facing_x += loc_latch.x * 0.5f * ( const float )TRIG_TABLE_SIZE / TARGET_UPS * ( 1.0f - jump_lerp );

    // set the desired acceleration
    loc_penviro->chr_acc = acc;

    // fake the desired velocity
    loc_penviro->chr_vel = fvec3_add( loc_pchr->vel.v, acc.v );

    return bdl;
    //return move_one_character_limit_flying( bdl );
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_voluntary_riding( ego_bundle_chr & bdl )
{
    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_voluntary( ego_bundle_chr & bdl )
{
    // do voluntary motion

    ego_chr             * loc_pchr;
    ego_phys_data       * loc_pphys;
    ego_chr_environment * loc_penviro;

    bool_t is_player, use_latch;

    fvec3_t loc_latch;
    float speed_max, acc_old;
    float dv2;

    bool_t sneak_mode_active = bfalse;

    fvec3_t total_vel;

    if ( NULL == bdl.chr_ptr() ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    // clear the outputs of this routine every time through
    fvec3_self_clear( loc_penviro->chr_acc.v );
    fvec3_self_clear( loc_penviro->chr_vel.v );

    // non-active characters do not move themselves
    if ( !PROCESSING_PCHR( loc_pchr ) ) return bdl;

    // if it is attached to another character, there is no voluntary motion
    if ( IS_ATTACHED_PCHR( loc_pchr ) )
    {
        return move_one_character_do_voluntary_riding( bdl );
    }

    // if we are flying, interpret the controls differently
    if ( loc_pchr->is_flying_jump )
    {
        return move_one_character_do_voluntary_flying( bdl );
    }

    // should we use the character's latch info?
    use_latch = loc_pchr->alive && !loc_pchr->isitem && loc_pchr->latch.trans_valid;
    if ( !use_latch ) return bdl;

    // is this character a player?
    is_player = IS_PLAYER_PCHR( loc_pchr );

    // Make a copy of the character's latch
    loc_latch.x = loc_pchr->latch.trans.dir[kX];
    loc_latch.y = loc_pchr->latch.trans.dir[kY];
    loc_latch.z = loc_pchr->latch.trans.dir[kZ];

    // this is the maximum speed that a character could go under the v2.22 system
    speed_max = loc_pchr->maxaccel;
    if ( 0.0f != airfriction && 1.0f != airfriction )
    {
        speed_max = loc_pchr->maxaccel * airfriction / ( 1.0f - airfriction );
    }

    // handle sneaking
    sneak_mode_active = bfalse;
    ego_player * ppla = PlaDeque.find_by_ref( loc_pchr->is_which_player );
    if ( is_player && NULL != ppla && ppla->valid )
    {
        // determine whether the user is hitting the "sneak button"
        if ( HAS_SOME_BITS( ppla->device.bits, INPUT_BITS_KEYBOARD ) )
        {
            // use the shift keys to enter sneak mode
            sneak_mode_active = SDLKEYDOWN( SDLK_LSHIFT ) || SDLKEYDOWN( SDLK_RSHIFT );
        }
    }

    if ( sneak_mode_active )
    {
        // sneak mode
        loc_pchr->maxaccel      = loc_pchr->maxaccel_reset * 0.33f;
        loc_pchr->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;
    }
    else
    {
        // non-sneak mode
        loc_pchr->movement_bits = ego_uint( ~CHR_MOVEMENT_BITS_SNEAK );
        loc_pchr->maxaccel      = loc_pchr->maxaccel_reset;
    }

    // determine the character's desired velocity from the latch
    if ( 0.0f != fvec3_length_abs( loc_latch.v ) )
    {
        float scale;

        dv2 = fvec3_length_2( loc_latch.v );

        // determine how response of the character to the latch
        scale = 1.0f;
        ego_player * ppla = PlaDeque.find_by_ref( loc_pchr->is_which_player );
        if ( is_player && NULL != ppla && ppla->valid )
        {
            float dv;

            dv = SQRT( dv2 );
            if ( dv < 1.0f )
            {
                // this function makes the control have a kind of dead zone near the center and a
                // have less sensitivity near "full throttle"
                scale = dv2 * ( 3.0f - 2.0f * dv );
            }
            else
            {
                scale = 1.0f / dv;
            }

            // determine whether the character is sneaking
            if ( !HAS_SOME_BITS( ppla->device.bits, INPUT_BITS_KEYBOARD ) )
            {
                sneak_mode_active = ( dv * scale < 1.0f / 3.0f );
            }
        }
        else
        {
            if ( dv2 > 1.0f )
            {
                scale = 1.0f / SQRT( dv2 );
            }
        }

        // adjust the latch values
        fvec3_self_scale( loc_latch.v, scale );

        // determine the correct speed
        loc_penviro->chr_vel = fvec3_scale( loc_latch.v, speed_max );
    }

    // add the special "limp" effect
    if ( 0 != ( mad_instance::get_framefx( &( loc_pchr->mad_inst ) ) & MADFX_STOP ) )
    {
        fvec3_self_clear( loc_penviro->chr_vel.v );
    }

    // get the actual desired character velocity by adding in the speed of the ground
    total_vel = fvec3_add( loc_penviro->ground_vel.v, loc_penviro->chr_vel.v );

    // get the desired acceleration from the difference between the desired velocity and the actual velocity
    loc_penviro->chr_acc = fvec3_sub( total_vel.v, loc_pchr->vel.v );

    // Also, fight acceleration from collisions and everything else.
    // This function call must be done before gravity is added in.
    // TODO: this is making objects on platforms behave a bit strangely. They are wobbling back and forth
    // because of the acceleration of both objects.
    {
        fvec3_t vec_tmp = fvec3_scale( loc_pphys->avel.v, -0.5f );

        fvec3_self_sum( loc_penviro->chr_acc.v, vec_tmp.v );
    }

    // limit the acceleration to maxaccel
    acc_old = 0.0f;
    if ( fvec3_length_abs( loc_penviro->chr_acc.v ) > 0.0f )
    {
        acc_old = fvec3_length( loc_penviro->chr_acc.v );
    }

    // if the character is not flying, fix the desired acceleration so that it is roughly parallel to the ground
    if ( !IS_FLYING_PCHR( loc_pchr ) && acc_old > 0.0f )
    {
        // remove any voluntary motion that is in the character's "up" direction
        if ( 1.0f == SDL_abs( loc_penviro->walk_nrm.z ) )
        {
            loc_penviro->chr_acc.z = 0.0f;
        }
        else
        {
            float dot;

            dot = fvec3_dot_product( loc_penviro->chr_acc.v, loc_penviro->walk_nrm.v );

            if ( 0.0f != dot )
            {
                loc_penviro->chr_acc.x -= dot * loc_penviro->walk_nrm.x;
                loc_penviro->chr_acc.y -= dot * loc_penviro->walk_nrm.y;
                loc_penviro->chr_acc.z -= dot * loc_penviro->walk_nrm.z;
            }
        }

        // try to make the length the same as the original acceleration
        if ( fvec3_length_abs( loc_penviro->chr_acc.v ) > 0.0f )
        {
            fvec3_self_normalize_to( loc_penviro->chr_acc.v, acc_old );
        }
    }

    // limit the acceleration to "maxaccel"
    if ( acc_old > 0.0f )
    {
        // since the changes in velocity this round are not applied until next round,
        // there is an added factor of "airfriction" that is applied to the character's
        // speed before the acceleration actually shows up in a change in position
        float adj_accel = loc_pchr->maxaccel / airfriction;

        if ( acc_old > adj_accel )
        {
            loc_penviro->chr_acc.x *= adj_accel / acc_old;
            loc_penviro->chr_acc.y *= adj_accel / acc_old;
            loc_penviro->chr_acc.z *= adj_accel / acc_old;
        }
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_involuntary( ego_bundle_chr & bdl )
{
    /// Do the "non-physics" motion that the character has no control over

    bool_t use_latch;

    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );

    // should we use the character's latch info?
    use_latch = loc_pchr->alive && !loc_pchr->isitem && loc_pchr->latch.trans_valid;

    // if we are not using the latch info
    if ( !use_latch )
    {
        if ( !IS_FLYING_PCHR( loc_pchr ) )
        {
            loc_penviro->chr_acc = fvec3_sub( loc_penviro->ground_vel.v, loc_pchr->vel.v );
        }
    }

    //if ( loc_pchr->alive && !loc_pchr->isitem )
    //{
    //    // Assume that the object acts like a vehicle; acceleration from the side is
    //    // resisted by the tires

    //    float vlerp = 0.0f;

    //    if( fvec3_length_abs(local_diff.v) > 0.0f )
    //    {
    //        float maxspeed, maxspeed2, vel2;

    //        if( 1.0f != airfriction )
    //        {
    //            maxspeed = loc_pchr->maxaccel * airfriction / ( 1.0f - airfriction );
    //            maxspeed = CLIP( maxspeed, 0.0f, 1000.0f );
    //        }
    //        else
    //        {
    //            maxspeed = 1000.0f;
    //        }

    //        maxspeed2 = maxspeed * maxspeed;
    //        vel2      = fvec3_dot_product( local_diff.v, local_diff.v );

    //        vlerp = vel2 / maxspeed2;
    //        vlerp = CLIP( vlerp, 0.0f, 1.0f );
    //    }

    //    if ( vlerp > 0.0f )
    //    {
    //        float  ftmp;
    //        fvec3_t vfront, diff_side;

    //        //---- get the "bad" velocity (perpendicular to the direction of motion)
    //        vfront = mat_getChrRight( loc_pchr->gfx_inst.matrix );
    //        fvec3_self_normalize( vfront.v );
    //        ftmp   = fvec3_dot_product( local_diff.v, vfront.v );

    //        // what is the "sideways" velocity?
    //        diff_side.x = local_diff.x - ftmp * vfront.x;
    //        diff_side.y = local_diff.y - ftmp * vfront.y;
    //        diff_side.z = local_diff.z - ftmp * vfront.z;

    //        // remove the amount of velocity diference in the sideways direction
    //        //local_diff.x -= diff_side.x * vlerp;
    //        //local_diff.y -= diff_side.y * vlerp;
    //    }
    //}

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_orientation( ego_bundle_chr & bdl )
{
    // do voluntary motion

    CHR_REF             loc_ichr;
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_ai_state        * loc_pai;

    bool_t can_control_direction;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_ichr    = bdl.chr_ref();
    loc_penviro = &( loc_pchr->enviro );
    loc_pai     = &( loc_pchr->ai );

    // set the old orientation
    loc_pchr->ori_old = loc_pchr->ori;

    // handle the special case of a mounted character.
    // the actual matrix is generated by the attachment points, but the scripts still use
    // the orientation values
    if ( INGAME_CHR( loc_pchr->attachedto ) )
    {
        ego_chr * pmount = ChrObjList.get_data_ptr( loc_pchr->attachedto );

        loc_pchr->ori = pmount->ori;

        return bdl;
    }

    // can the character control its direction?
    can_control_direction = bfalse;
    if ( IS_FLYING_PCHR( loc_pchr ) || TURNMODE_SPIN == loc_pchr->turnmode )
    {
        can_control_direction = btrue;
    }
    else
    {
        can_control_direction = loc_penviro->walk_lerp < 1.0f;
    }

    if ( can_control_direction )
    {
        fvec3_t loc_vel = ZERO_VECT3, loc_chr_vel = ZERO_VECT3;
        bool_t  use_latch;

        // "dead" characters and items do not adjust their ori.facing_z
        use_latch = loc_pchr->alive && !loc_pchr->isitem && loc_pchr->latch.trans_valid;

        // make an exception for TURNMODE_SPIN == loc_pchr->turnmode
        if ( TURNMODE_SPIN == loc_pchr->turnmode ) use_latch = btrue;

        // determine the velocities relative to the character's "ground" (if it exists)
        fvec3_self_clear( loc_vel.v );

        if ( IS_FLYING_PCHR( loc_pchr ) || loc_penviro->walk_lerp > 1.0f )
        {
            // this velocity is relative to "still air"
            loc_vel = fvec3_sub( loc_pchr->vel.v, windspeed.v );
        }
        else
        {
            fvec3_t tmp_vec0, tmp_vec1;

            // As the character approaches the ground, make the velocity vector relative to the
            // ground

            // get the actial velocity relative to the air
            tmp_vec0 = fvec3_sub( loc_pchr->vel.v, windspeed.v );
            fvec3_self_scale( tmp_vec0.v, 1.0f - loc_penviro->walk_lerp );

            // get the actial velocity relative to the ground
            tmp_vec1 = fvec3_sub( loc_pchr->vel.v, loc_penviro->ground_vel.v );
            fvec3_self_scale( tmp_vec1.v, loc_penviro->walk_lerp );

            fvec3_self_sum( loc_vel.v, tmp_vec0.v );
            fvec3_self_sum( loc_vel.v, tmp_vec1.v );
        }

        // the desired velocity is already relative to whatever ground the character is interacting with
        loc_chr_vel = loc_penviro->chr_vel;

        // apply changes to ori.facing_z
        if ( use_latch )
        {
            float speed_max, speed_lerp;
            int loc_turnmode = loc_pchr->turnmode;

            // this is the maximum speed that a character could go under the v2.22 system
            speed_max  = loc_pchr->maxaccel * airfriction / ( 1.0f - airfriction );

            if ( loc_pchr->is_flying_jump )
            {
                loc_turnmode = TURNMODE_FLYING_JUMP;
            }
            else if ( loc_pchr->is_flying_platform && TURNMODE_SPIN != loc_turnmode )
            {
                loc_turnmode = TURNMODE_FLYING_PLATFORM;
            }

            speed_lerp = 0.0f;
            if ( fvec2_length_abs( loc_vel.v ) > 0.0f )
            {
                if ( 0.0f == speed_max )
                {
                    speed_lerp = 1.0f;
                }
                else
                {
                    speed_lerp = fvec2_dot_product( loc_vel.v, loc_vel.v ) / speed_max / speed_max;
                }

                speed_lerp = CLIP( speed_lerp, 0.0f, 1.0f );
            }

            // Determine the character rotation
            switch ( loc_turnmode )
            {
                    // Get direction from ACTUAL velocity
                default:
                case TURNMODE_FLYING_PLATFORM:
                case TURNMODE_VELOCITY:
                    {
                        fvec2_t tmp_vel;

                        // interpolate between the desired "actual velocity" and the backup "desired velocity"
                        if ( 0.0f == speed_lerp )
                        {
                            tmp_vel.x = loc_chr_vel.x;
                            tmp_vel.y = loc_chr_vel.y;
                        }
                        else if ( 1.0f == speed_lerp )
                        {
                            tmp_vel.x = loc_vel.x;
                            tmp_vel.y = loc_vel.y;
                        }
                        else
                        {
                            tmp_vel.x = speed_lerp * loc_vel.x + ( 1.0f - speed_lerp ) * loc_chr_vel.x;
                            tmp_vel.y = speed_lerp * loc_vel.y + ( 1.0f - speed_lerp ) * loc_chr_vel.y;
                        }

                        if ( fvec2_length_abs( tmp_vel.v ) > TURN_SPEED )
                        {
                            if ( IS_PLAYER_PCHR( loc_pchr ) )
                            {
                                // Players turn quickly
                                loc_pchr->ori.facing_z += terp_dir( loc_pchr->ori.facing_z, vec_to_facing( tmp_vel.x , tmp_vel.y ), 2 );
                            }
                            else
                            {
                                // AI turn slowly
                                loc_pchr->ori.facing_z += terp_dir( loc_pchr->ori.facing_z, vec_to_facing( tmp_vel.x , tmp_vel.y ), 8 );
                            }
                        }
                    }
                    break;

                    // Get direction from the DESIRED acceleration
                case TURNMODE_ACCELERATION:
                    {
                        fvec2_t tmp_vel;

                        // interpolate between the desired "desired acceleration" and the backup "actual acceleration"
                        if ( 0.0f == speed_lerp )
                        {
                            tmp_vel.x = loc_penviro->acc.x;
                            tmp_vel.y = loc_penviro->acc.y;
                        }
                        else if ( 1.0f == speed_lerp )
                        {
                            tmp_vel.x = loc_penviro->chr_acc.x;
                            tmp_vel.y = loc_penviro->chr_acc.y;
                        }
                        else
                        {
                            tmp_vel.x = speed_lerp * loc_penviro->chr_acc.x + ( 1.0f - speed_lerp ) * loc_penviro->acc.x;
                            tmp_vel.y = speed_lerp * loc_penviro->chr_acc.y + ( 1.0f - speed_lerp ) * loc_penviro->acc.y;
                        }

                        if ( fvec2_length_abs( tmp_vel.v ) > WATCH_SPEED )
                        {
                            loc_pchr->ori.facing_z += terp_dir( loc_pchr->ori.facing_z, vec_to_facing( tmp_vel.x , tmp_vel.y ), 8 );
                        }
                    }
                    break;

                    // Face the target
                case TURNMODE_WATCHTARGET:
                    {
                        if ( ChrObjList.valid_index_range( loc_pai->target.get_value() ) && loc_ichr != loc_pai->target )
                        {
                            fvec2_t loc_diff;

                            loc_diff.x = ChrObjList.get_data_ref( loc_pai->target ).pos.x - loc_pchr->pos.x;
                            loc_diff.y = ChrObjList.get_data_ref( loc_pai->target ).pos.y - loc_pchr->pos.y;

                            if ( fvec2_length_abs( loc_diff.v ) > WATCH_SPEED )
                            {
                                loc_pchr->ori.facing_z += terp_dir( loc_pchr->ori.facing_z, vec_to_facing( loc_diff.x, loc_diff.y ), 8 );
                            }
                        }
                    }
                    break;

                    // Otherwise make it spin
                case TURNMODE_SPIN:
                    {
                        loc_pchr->ori.facing_z += SPINRATE;
                    }
                    break;

                    // at this point, the TURNMODE_FLYING_JUMP orientation is controled elsewhere
                    //case TURNMODE_FLYING_JUMP:
                    //    {
                    //        fvec2_t loc_vel;

                    //        // interpolate between the desired "actual velocity" and the backup "desired velocity"
                    //        if( 0.0f == speed_lerp )
                    //        {
                    //            loc_vel.x = loc_chr_vel.x;
                    //            loc_vel.y = loc_chr_vel.y;
                    //        }
                    //        else if( 1.0f == speed_lerp )
                    //        {
                    //            loc_vel.x = loc_vel.x;
                    //            loc_vel.y = loc_vel.y;
                    //        }
                    //        else
                    //        {
                    //            loc_vel.x = speed_lerp * loc_vel.x + (1.0f - speed_lerp ) * loc_chr_vel.x;
                    //            loc_vel.y = speed_lerp * loc_vel.y + (1.0f - speed_lerp ) * loc_chr_vel.y;
                    //        }

                    //        if ( fvec2_length_abs( loc_vel.v ) > FLYING_SPEED )
                    //        {
                    //            loc_pchr->ori.facing_z += terp_dir( loc_pchr->ori.facing_z, vec_to_facing( loc_vel.x , loc_vel.y ), 16 );
                    //        }
                    //    }
                    //    break;
            }
        }

    }
    // Characters with sticky butts lie on the surface of the mesh
    if ( loc_penviro->walk_lerp < 1.0f && !loc_pchr->is_flying_jump && !loc_pchr->is_flying_platform )
    {
        FACING_T new_facing_x = MAP_TURN_OFFSET;
        FACING_T new_facing_y = MAP_TURN_OFFSET;

        float loc_lerp = CLIP( loc_penviro->walk_lerp, 0.0f, 1.0f );
        float fkeep = ( 7.0f + loc_lerp ) / 8.0f;
        float fnew  = ( 1.0f - loc_lerp ) / 8.0f;

        if ( loc_pchr->stickybutt || !loc_pchr->alive || loc_pchr->isitem )
        {
            new_facing_x = map_twist_x[loc_penviro->grid_twist];
            new_facing_y = map_twist_y[loc_penviro->grid_twist];
        }

        if ( new_facing_x != loc_pchr->ori.map_facing_x )
        {
            loc_pchr->ori.map_facing_x = loc_pchr->ori.map_facing_x * fkeep + new_facing_x * fnew;
        }

        if ( new_facing_y != loc_pchr->ori.map_facing_y )
        {
            loc_pchr->ori.map_facing_y = loc_pchr->ori.map_facing_y * fkeep + new_facing_y * fnew;
        }
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_z_motion( ego_bundle_chr & bdl )
{
    ego_chr             * loc_pchr;
    ego_phys_data       * loc_pphys;
    ego_chr_environment * loc_penviro;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // aliases for easier notiation
    loc_pchr    = bdl.chr_ptr();
    loc_pphys   = &( loc_pchr->phys );
    loc_penviro = &( loc_pchr->enviro );

    //---- do z acceleration
    if ( loc_pchr->is_flying_platform )
    {
        phys_data_accumulate_avel_index( loc_pphys, ( loc_penviro->fly_level + loc_pchr->fly_height - loc_pchr->pos.z ) * FLYDAMPEN, kZ );
    }
    else
    {
        phys_data_accumulate_avel_index( loc_pphys, gravity, kZ );
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_animation( ego_bundle_chr & bdl )
{
    float dflip, flip_diff;
    float test_amount;

    ego_chr          * loc_pchr;
    gfx_mad_instance * loc_pgfx_inst;
    mad_instance     * loc_pmad_inst;
    CHR_REF            loc_ichr;
    ego_uint           old_id;

    if ( !INGAME_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr      = bdl.chr_ptr();
    loc_ichr      = bdl.chr_ref();
    loc_pgfx_inst = &( loc_pchr->gfx_inst );
    loc_pmad_inst = &( loc_pchr->mad_inst );

    // save the old mad_instance id
    old_id = loc_pmad_inst->state.id;

    // Animate the character.
    // Right now there are 50/4 = 12.5 animation frames per second
    dflip       = 0.25f * loc_pmad_inst->action.rate;
    flip_diff = fmod( loc_pmad_inst->state.flip, 0.25f ) + dflip;

    // grab the amount to take you to the next ilip
    test_amount = pose_data::get_remaining_flip( &( loc_pmad_inst->state ) );
    if ( test_amount <= 0.0f || test_amount > 0.25f ) test_amount = 0.25f;

    // fun through all the animation steps until you are left with
    // a little bit past the last full ilip
    while ( flip_diff >= test_amount )
    {
        flip_diff -= test_amount;

        // update the animation to the next ilip
        BIT_FIELD madfx = mad_instance::update_animation_one( loc_pmad_inst );

        // handle frame FX for the new frame
        if ( 3 == loc_pmad_inst->state.ilip )
        {
            chr_handle_madfx( loc_pchr, madfx );
        }

        test_amount = 0.25f;
    }

    // handle any remaining flip_diff
    if ( flip_diff > 0.0f )
    {
        int ilip_old = loc_pmad_inst->state.ilip;

        // updte the animation thto the next fraction of an ilip
        BIT_FIELD madfx = mad_instance::update_animation( loc_pmad_inst, flip_diff );

        // don't do the framefx more than once
        if ( ilip_old != loc_pmad_inst->state.ilip )
        {
            // handle frame FX for the new frame
            if ( 3 == loc_pmad_inst->state.ilip )
            {
                chr_handle_madfx( loc_pchr, madfx );
            }
        }
    }

    set_character_animation_rate( loc_pchr );

    // NOW invalidate the instances, if the mad_instance was changed
    if ( old_id != loc_pmad_inst->state.id )
    {
        // invalidate the current model data
        loc_pgfx_inst->vrange.valid = bfalse;

        // invalidate all the child instances
        chr_invalidate_instances( loc_pchr );
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_limit_flying( ego_bundle_chr & bdl )
{
    // this should only be called by move_one_character_do_floor() after the
    // normal acceleration has been killed

    fvec3_t total_acc;

    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    return bdl;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    if ( !IS_FLYING_PCHR( loc_pchr ) ) return bdl;

    total_acc = loc_penviro->chr_acc;

    // at this point, flying platforms are not like flying carpets or planes, just the silly levetating platforms, "mario platforms"
    if ( loc_pchr->is_flying_jump )
    {
        float   chr_vel_len, chr_vel_len2;
        fvec3_t total_acc_para, total_acc_perp;

        // find the difference between the desired acceleration and the external accelerations
        total_acc = fvec3_sub( loc_penviro->chr_acc.v, loc_pphys->avel.v );

        chr_vel_len = chr_vel_len2 = 0.0f;
        if ( fvec3_length_abs( loc_pchr->vel.v ) > 0.0f )
        {
            fvec3_t chr_vel_nrm;

            chr_vel_len  = fvec3_length( loc_pchr->vel.v );
            chr_vel_len2 = chr_vel_len * chr_vel_len;

            chr_vel_nrm = fvec3_normalize( loc_pchr->vel.v );

            fvec3_decompose( total_acc.v, chr_vel_nrm.v, total_acc_para.v, total_acc_perp.v );
        }

        //--- limit the maximum power of the flying object
        if ( chr_vel_len > 0.0f )
        {
            float max_power = loc_pchr->maxaccel * loc_pchr->maxaccel / ( 1.0f - airfriction );

            float loc_power = - loc_pchr->vel.z * gravity + ( 1.0f - airfriction ) * chr_vel_len2 + fvec3_dot_product( loc_pchr->vel.v, total_acc.v );

            if ( loc_power > max_power )
            {
                float term, total_acc_para_len;

                // work backwards to get the fvec3_dot_product(loc_pchr->vel.v, total_acc.v) term from the previous equation
                term = loc_power - ( - loc_pchr->vel.z * gravity + ( 1.0f - airfriction ) * chr_vel_len2 );

                // since fvec3_dot_product(loc_pchr->vel.v, total_acc.v) == chr_vel_len * total_acc_para_len
                total_acc_para_len = term / chr_vel_len;

                if ( SDL_abs( total_acc_para_len ) > 0.0f )
                {
                    // limit the tangential acceleration
                    fvec3_self_normalize_to( total_acc.v, SDL_abs( total_acc_para_len ) );
                }
                else
                {
                    // elimiate the tangential acceleration
                    fvec3_self_clear( total_acc_para.v );
                }
            }
        }

        total_acc = fvec3_add( total_acc_para.v, total_acc_perp.v );

        //---- limit the maximum aerodynamic force on a flying object
        if ( fvec3_length_abs( total_acc.v ) > 0.0f )
        {
            float max_force = loc_pchr->maxaccel / ( 1.0f - airfriction );

            float total_acc_len = fvec3_length( total_acc.v );

            if ( total_acc_len > max_force )
            {
                fvec3_self_normalize_to( total_acc.v, max_force );
            }
        }
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_jump( ego_bundle_chr & bdl )
{
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    fvec3_t total_acc, total_acc_para, total_acc_perp;
    float coeff_para, coeff_perp;

    float jump_lerp, fric_lerp;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    if ( IS_FLYING_PCHR( loc_pchr ) ) return bdl;

    // determine whether a character has recently jumped
    jump_lerp = ( const float )loc_pchr->jump_time / ( const float )JUMP_DELAY;
    jump_lerp = CLIP( jump_lerp, 0.0f, 1.0f );

    // if we have not jumped, there's nothing to do
    if ( 0.0f == jump_lerp ) return bdl;

    // determine the amount of acceleration necessary to get the desired acceleration
    total_acc = fvec3_sub( loc_penviro->chr_acc.v, loc_pphys->avel.v );

    if ( 0.0f == fvec3_length_abs( total_acc.v ) ) return bdl;

    // decompose this into acceleration parallel and perpendicular to the ground
    fvec3_decompose( total_acc.v, loc_penviro->walk_nrm.v, total_acc_perp.v, total_acc_para.v );

    // the strength of the player's control over a jumping character is small and
    // diminshes over the time of the jump
    fric_lerp = traction_min * jump_lerp * ( 1.0f - loc_penviro->walk_lerp );

    // "jumping traction" is mostly only parallel to the ground
    coeff_para = 1.0f;
    coeff_perp = 1.0f - loc_penviro->walk_lerp;

    // scale the acceleration
    fvec3_self_scale( total_acc_para.v, fric_lerp * coeff_para );
    fvec3_self_scale( total_acc_perp.v, fric_lerp * coeff_perp );

    // determine the total acceleration
    total_acc = fvec3_add( total_acc_para.v, total_acc_perp.v );

    // apply the jumping control
    phys_data_accumulate_avel( loc_pphys, total_acc.v );

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_flying( ego_bundle_chr & bdl )
{
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    if ( !IS_FLYING_PCHR( loc_pchr ) || IS_ATTACHED_PCHR( loc_pchr ) ) return bdl;

    // apply the flying forces
    phys_data_accumulate_avel( loc_pphys, loc_penviro->chr_acc.v );

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character_do_floor( ego_bundle_chr & bdl )
{
    /// \author BB
    /// \details  Friction is complicated when you want to have sliding characters :P
    ///
    /// \note really, for this to work properly, all the friction interaction should be stored in a list
    /// and then acted on after the main loop in move_all_characters() has been completed for all objects.

    // a "typical" maximum speed
    const float speed_max_typical      = 8.0f;
    const float friction_magnification = 4.0f;

    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    ego_chr             * pplatform = NULL;

    fvec3_t total_acc, total_acc_para, total_acc_perp, normal_acc, traction_acc;

    bool_t is_scenery_object;
    float scenery_grip;

    float  fric_lerp;

    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    loc_pchr    = bdl.chr_ptr();
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    // no floor friction if you are riding or being carried
    if ( IS_ATTACHED_PCHR( loc_pchr ) ) return bdl;

    // exempt "mario platforms" from interaction with the floor
    if ( loc_pchr->is_flying_platform && INFINITE_WEIGHT == loc_pphys->weight ) return bdl;

    fric_lerp = 1.0f;
    pplatform = NULL;
    if ( PROCESSING_CHR( loc_pchr->onwhichplatform_ref ) )
    {
        pplatform = ChrObjList.get_data_ptr( loc_pchr->onwhichplatform_ref );

        // if the character has just dismounted from this platform, then there is less friction
        fric_lerp *= calc_dismount_lerp( loc_pchr, pplatform );
    }

    // determine whether the object is part of the scenery
    is_scenery_object = ( INFINITE_WEIGHT == loc_pphys->weight );
    scenery_grip = is_scenery_object ? 1.0f : ( const float )loc_pphys->weight / ( const float )MAX_WEIGHT;
    scenery_grip = CLIP( scenery_grip, 0.0f, 1.0f );

    // apply the acceleration from the "normal force"
    phys_data_apply_normal_acceleration( loc_pphys, loc_penviro->walk_nrm, 1.0f - scenery_grip, loc_penviro->walk_lerp, &normal_acc );

    // determine how much influence the floor has on the object
    fric_lerp *= 1.0f - loc_penviro->walk_lerp;

    // if we're not in touch with the ground, there is no walking
    if ( 0.0f == fric_lerp ) return bdl;

    // if there is no friction with the ground, nothing to do
    if ( loc_penviro->ground_fric >= 1.0f ) return bdl;

    // determine the amount of acceleration necessary to get the desired acceleration
    total_acc = fvec3_sub( loc_penviro->chr_acc.v, loc_pphys->avel.v );

    // if there is no total acceleration, there's nothing to do
    if ( 0.0f == fvec3_length_abs( total_acc.v ) ) return bdl;

    // decompose this into acceleration parallel and perpendicular to the ground
    fvec3_decompose( total_acc.v, loc_penviro->walk_nrm.v, total_acc_perp.v, total_acc_para.v );

    // determine an amount of "minimum traction"
    traction_acc = fvec3_scale( total_acc_para.v, traction_min );

    // any traction away from the ground was calculated in move_one_character_do_jump()
    fvec3_self_scale( traction_acc.v, fric_lerp );

    // if there is no normal_acc, then there is no ordinary traction
    if ( 0.0f == fvec3_length_abs( normal_acc.v ) )
    {
        fvec3_self_clear( total_acc_para.v );
    }
    else
    {
        float   est_max_friction_acc;
        float   coefficient, normal_acc_len, max_fric_acc;

        //---- use the static coefficient of friction

        // the max acceleration from static friction is proportional to the normal acceleration
        normal_acc_len = fvec3_length( normal_acc.v );

        // find a "maximum" amount of deceleration due to friction for this object
        est_max_friction_acc = - speed_max_typical * LOG( loc_penviro->ground_fric );

        // the default amount of floor friction is too weak, boost it a little
        est_max_friction_acc *= friction_magnification;

        // determine an effective coefficient of static friction
        coefficient = fric_lerp * loc_penviro->traction * est_max_friction_acc / SDL_abs( gravity );

        // determine the maximum amount of acceleration from static friction
        max_fric_acc = coefficient * normal_acc_len;

        // infinite weight objects can never slip
        if ( is_scenery_object || fvec3_length( total_acc_para.v ) <= max_fric_acc )
        {
            loc_penviro->is_slipping = bfalse;

            {
                fvec3_t tmp_vec1, tmp_vec2;

                tmp_vec1 = fvec3_scale( loc_penviro->ground_diff.v, 0.1f );
                tmp_vec2 = fvec3_scale( loc_penviro->legs_vel.v, 0.9f );

                loc_penviro->legs_vel = fvec3_add( tmp_vec1.v, tmp_vec2.v );
            }
        }
        else
        {
            //---- use a dynamic coefficient of friction

            loc_penviro->is_slipping = btrue;

            // recompute the max_fric_acc with the "slippy" coefficient of friction
            est_max_friction_acc *= 0.5f;
            coefficient           = fric_lerp * loc_penviro->traction * est_max_friction_acc  / SDL_abs( gravity );
            max_fric_acc          = coefficient * normal_acc_len;

            // we are slipping
            fvec3_self_normalize_to( total_acc_para.v, max_fric_acc );

            {
                fvec3_t tmp_vec1, tmp_vec2;

                tmp_vec1 = fvec3_scale( loc_penviro->chr_vel.v, 0.1f );
                tmp_vec2 = fvec3_scale( loc_penviro->legs_vel.v, 0.9f );

                loc_penviro->legs_vel = fvec3_add( tmp_vec1.v, tmp_vec2.v );
            }
        }

        // scale this amount to take into account the fact that traction_acc was removed from it earlier
        fvec3_self_scale( total_acc_para.v, 1.0f - traction_min );
    }

    // apply the floor friction
    phys_data_accumulate_avel( loc_pphys, total_acc_para.v );
    //phys_data_accumulate_avel( loc_pphys, traction_acc.v   );

    // we should apply the acceleration to the floor in the opposite direction
    // make ordinary "plaforms" immune
    if ( NULL != pplatform && !pplatform->is_flying_platform )
    {
        float wta, wtb;

        character_physics_get_mass_pair( loc_pchr, pplatform, &wta, &wtb );

        // take the relative "masses" into account
        if ( INFINITE_WEIGHT != wtb && 0.0f != wta )
        {
            float   factor = wta / ( wtb + pplatform->holdingweight );
            fvec3_t plat_acc_para = fvec3_scale( total_acc_para.v, - factor );

            phys_data_accumulate_avel( &( pplatform->phys ), plat_acc_para.v );
        }
    }

    return bdl;
}

//--------------------------------------------------------------------------------------------
ego_bundle_chr & move_one_character( ego_bundle_chr & bdl )
{
    if ( !PROCESSING_PCHR( bdl.chr_ptr() ) ) return bdl;

    // alias some variables
    ego_chr * loc_pchr = bdl.chr_ptr();

    // calculate the acceleration from the last time-step
    loc_pchr->enviro.acc = fvec3_sub( loc_pchr->vel.v, loc_pchr->vel_old.v );

    // Character's old location
    loc_pchr->pos_old = ego_chr::get_pos( loc_pchr );
    loc_pchr->vel_old = loc_pchr->vel;
    loc_pchr->ori_old = loc_pchr->ori;

    // determine the character's environment
    // if this is not done, reflections will not update properly
    PROFILE_BEGIN( move_one_character_get_environment );
    bdl = move_one_character_get_environment( bdl, NULL );
    PROFILE_END2( move_one_character_get_environment );

    //---- none of the remaining functions are valid for attached characters
    if ( IS_ATTACHED_PCHR( loc_pchr ) ) return bdl;

    // make the object want to continue its current motion, unless this is overridden
    if ( loc_pchr->latch.trans_valid )
    {
        loc_pchr->enviro.chr_vel = loc_pchr->vel;
    }

    // apply the fluid friction for the object
    PROFILE_BEGIN( move_one_character_do_fluid_friction );
    bdl = move_one_character_do_fluid_friction( bdl );
    PROFILE_END2( move_one_character_do_fluid_friction );

    // determine how the character would *like* to move
    PROFILE_BEGIN( move_one_character_do_voluntary );
    bdl = move_one_character_do_voluntary( bdl );
    PROFILE_END2( move_one_character_do_voluntary );

    // apply gravitational an buoyancy effects
    PROFILE_BEGIN( move_one_character_do_z_motion );
    bdl = move_one_character_do_z_motion( bdl );
    PROFILE_END2( move_one_character_do_z_motion );

    // determine how the character is being *forced* to move
    PROFILE_BEGIN( move_one_character_do_involuntary );
    bdl = move_one_character_do_involuntary( bdl );
    PROFILE_END2( move_one_character_do_involuntary );

    // read and apply any latch buttons
    PROFILE_BEGIN( chr_do_latch_button );
    bdl = chr_do_latch_button( bdl );
    PROFILE_END2( chr_do_latch_button );

    // allow the character to control its flight
    PROFILE_BEGIN( move_one_character_do_flying );
    bdl = move_one_character_do_flying( bdl );
    PROFILE_END2( move_one_character_do_flying );

    // allow the character to have some additional control over jumping
    PROFILE_BEGIN( move_one_character_do_jump );
    bdl = move_one_character_do_jump( bdl );
    PROFILE_END2( move_one_character_do_jump );

    // determine how the character can *actually* move
    PROFILE_BEGIN( move_one_character_do_floor );
    bdl = move_one_character_do_floor( bdl );
    PROFILE_END2( move_one_character_do_floor );

    // do the character animation and apply any MADFX found in the animation frames
    PROFILE_BEGIN( move_one_character_do_animation );
    bdl = move_one_character_do_animation( bdl );
    PROFILE_END2( move_one_character_do_animation );

    // set the rotation angles for the character
    PROFILE_BEGIN( move_one_character_do_orientation );
    bdl = move_one_character_do_orientation( bdl );
    PROFILE_END2( move_one_character_do_orientation );

    return bdl;
}

//--------------------------------------------------------------------------------------------
void move_all_characters( void )
{
    /// \author ZZ
    /// \details  This function handles character physics

    ego_chr::stoppedby_tests = 0;

    // Move every character
    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        PROFILE_BEGIN( move_one_character );

        ego_bundle_chr bdl( pchr );

        // prime the environment
        pchr->enviro.air_friction = air_friction;
        pchr->enviro.ice_friction = ice_friction;

        move_one_character( bdl );

        PROFILE_END2( move_one_character );
    }
    CHR_END_LOOP();

    // The following functions need to be called any time you actually change a charcter's position
    PROFILE_BEGIN( keep_weapons_with_holders );
    keep_weapons_with_holders();
    PROFILE_END2( keep_weapons_with_holders );

    PROFILE_BEGIN( attach_all_particles );
    attach_all_particles();
    PROFILE_END2( attach_all_particles );

    PROFILE_BEGIN( make_all_character_matrices );
    make_all_character_matrices( update_wld != 0 );
    PROFILE_END2( make_all_character_matrices );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void cleanup_all_characters()
{
    // Do poofing
    CHR_BEGIN_LOOP_DEFINED( ichr, pchr )
    {
        bool_t time_out;

        ego_obj_chr * pobj = ego_chr::get_obj_ptr( pchr );
        if ( NULL == pobj ) continue;

        time_out = ( pchr->ai.poof_time >= 0 ) && ( Uint32( pchr->ai.poof_time ) <= update_wld );
        if ( !WAITING_PBASE( pobj ) && !time_out ) continue;

        // detach the character from the game
        cleanup_one_character( pchr );

        // free the character's inventory
        free_inventory_in_game( ichr );

        // free the character
        free_one_character_in_game( ichr );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void increment_all_character_update_counters()
{
    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        pchr_obj->update_count++;
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t is_invictus_direction( const FACING_T direction, const CHR_REF & character, const BIT_FIELD effects )
{
    FACING_T left, right;

    ego_chr * pchr;
    ego_cap * pcap;
    ego_mad * pmad;

    bool_t  is_invictus;

    FACING_T loc_direction = direction;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return btrue;

    pmad = ego_chr::get_pmad( character );
    if ( NULL == pmad ) return btrue;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return btrue;

    // if the invictus flag is set, we are invictus
    if ( IS_INVICTUS_PCHR_RAW( pchr ) ) return btrue;

    // if the effect is shield piercing, ignore shielding
    if ( HAS_SOME_BITS( effects, DAMFX_NBLOC ) ) return bfalse;

    // if the character's frame is invictus, then check the angles
    if ( HAS_SOME_BITS( mad_instance::get_framefx( &( pchr->mad_inst ) ), MADFX_INVICTUS ) )
    {
        // I Frame
        loc_direction -= pcap->iframefacing;
        left       = FACING_T( int( 0x00010000L ) - int( pcap->iframeangle ) );
        right      = pcap->iframeangle;

        // If using shield, use the shield invictus instead
        if ( ACTION_IS_TYPE( pchr->mad_inst.action.which, P ) )
        {
            bool_t parry_left = ( pchr->mad_inst.action.which < ACTION_PC );

            // Using a shield?
            if ( parry_left )
            {
                // Check left hand
                ego_cap * pcap_tmp = ego_chr::get_pcap( pchr->holdingwhich[SLOT_LEFT] );
                if ( NULL != pcap )
                {
                    left  = FACING_T( int( 0x00010000L ) - int( pcap_tmp->iframeangle ) );
                    right = pcap_tmp->iframeangle;
                }
            }
            else
            {
                // Check right hand
                ego_cap * pcap_tmp = ego_chr::get_pcap( pchr->holdingwhich[SLOT_RIGHT] );
                if ( NULL != pcap )
                {
                    left  = FACING_T( int( 0x00010000L ) - int( pcap_tmp->iframeangle ) );
                    right = pcap_tmp->iframeangle;
                }
            }
        }
    }
    else
    {
        // N Frame
        loc_direction -= pcap->nframefacing;
        left       = FACING_T( int( 0x00010000L ) - int( pcap->nframeangle ) );
        right      = pcap->nframeangle;
    }

    // Check that loc_direction
    is_invictus = bfalse;
    if ( loc_direction <= left && loc_direction <= right )
    {
        is_invictus = btrue;
    }

    return is_invictus;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::set_mad( ego_chr * pchr, const MAD_REF & imad )
{
    bool_t gfx_set, mad_set;

    if ( NULL == pchr ) return bfalse;

    mad_set = mad_instance::set_mad( &( pchr->mad_inst ), imad );
    gfx_set = gfx_mad_instance::set_mad( &( pchr->gfx_inst ), imad );

    return gfx_set && mad_set;
}

//--------------------------------------------------------------------------------------------
bool_t mad_instance::set_mad( mad_instance * pmad_inst, const MAD_REF & imad )
{
    /// \author BB
    /// \details  try to set the model used by the character instance.
    ///     If this fails, it leaves the old data. Just to be safe it
    ///     would be best to check whether the old modes is valid, and
    ///     if not, the data should be set to safe values...

    ego_mad * pmad;
    bool_t updated = bfalse;

    if ( !LOADED_MAD( imad ) ) return bfalse;
    pmad = MadStack + imad;

    if ( NULL == pmad || pmad->md2_ptr == NULL )
    {
        log_error( "Invalid pmad instance spawn. (Slot number %i)\n", imad.get_value() );
        return bfalse;
    }

    if ( pmad_inst->imad != imad )
    {
        updated = btrue;
        pmad_inst->imad = imad;
    }

    // set the frames to frame 0 of this object's data
    if ( pmad_inst->state.frame_nxt != 0 || pmad_inst->state.frame_lst != 0 )
    {
        updated = btrue;
        pmad_inst->state.frame_nxt = pmad_inst->state.frame_lst = 0;
    }

    return updated;
}

//--------------------------------------------------------------------------------------------
grip_offset_t slot_to_grip_offset( slot_t slot )
{
    int retval = GRIP_ORIGIN;

    retval = ( slot + 1 ) * GRIP_VERTS;

    return ( grip_offset_t )retval;
}

//--------------------------------------------------------------------------------------------
slot_t grip_offset_to_slot( grip_offset_t grip_off )
{
    slot_t retval = SLOT_LEFT;

    if ( 0 != grip_off % GRIP_VERTS )
    {
        // this does not correspond to a valid slot
        // coerce it to the "default" slot
        retval = SLOT_LEFT;
    }
    else
    {
        int islot = ( int( grip_off ) / GRIP_VERTS ) - 1;

        // coerce the slot number to fit within the valid range
        islot = CLIP( islot, 0, SLOT_COUNT );

        retval = ( slot_t ) islot;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void init_slot_idsz()
{
    inventory_idsz[INVEN_PACK]  = IDSZ_NONE;
    inventory_idsz[INVEN_NECK]  = IDSZ_NECK;
    inventory_idsz[INVEN_WRIS]  = IDSZ_WRIS;
    inventory_idsz[INVEN_FOOT]  = IDSZ_FOOT;
}

//--------------------------------------------------------------------------------------------
bool_t ego_ai_state::add_order( ego_ai_state * pai, const Uint32 value, const Uint16 counter )
{
    bool_t retval;

    if ( NULL == pai ) return bfalse;

    // this function is only truly valid if there is no other order
    retval = HAS_NO_BITS( pai->alert, ALERTIF_ORDERED );

    ADD_BITS( pai->alert, ALERTIF_ORDERED );
    pai->order_value   = value;
    pai->order_counter = counter;

    return retval;
}

//--------------------------------------------------------------------------------------------
BBOARD_REF chr_add_billboard( const CHR_REF & ichr, const Uint32 lifetime_secs )
{
    /// \author BB
    /// \details  Attach a basic billboard to a character. You set the billboard texture
    ///     at any time after this. Returns the index of the billboard or INVALID_BILLBOARD
    ///     if the allocation fails.
    ///
    ///    must be called with a valid character, so be careful if you call this function from within
    ///    spawn_one_character()

    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return BBOARD_REF( INVALID_BILLBOARD );

    if ( INVALID_BILLBOARD != pchr->ibillboard )
    {
        BillboardList_free_one(( pchr->ibillboard ).get_value() );
        pchr->ibillboard = INVALID_BILLBOARD;
    }

    pchr->ibillboard = BillboardList_get_free( lifetime_secs );

    // attach the billboard to the character
    if ( INVALID_BILLBOARD != pchr->ibillboard )
    {
        ego_billboard_data * pbb = BillboardList.lst + pchr->ibillboard;

        pbb->ichr = ichr;
    }

    return pchr->ibillboard;
}

//--------------------------------------------------------------------------------------------
ego_billboard_data * chr_make_text_billboard( const CHR_REF & ichr, const char * txt, const SDL_Color text_color, const GLXvector4f tint, const int lifetime_secs, const BIT_FIELD opt_bits )
{
    ego_chr            * pchr;
    ego_billboard_data * pbb;
    int                rv;

    BBOARD_REF ibb = BBOARD_REF( INVALID_BILLBOARD );

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return NULL;

    // create a new billboard or override the old billboard
    ibb = chr_add_billboard( ichr, lifetime_secs );
    if ( INVALID_BILLBOARD == ibb ) return NULL;

    pbb = BillboardList_get_ptr( pchr->ibillboard );
    if ( NULL == pbb ) return pbb;

    rv = ego_billboard_data::printf_ttf( pbb, ui_getFont(), text_color, "%s", txt );

    if ( rv < 0 )
    {
        pchr->ibillboard = INVALID_BILLBOARD;
        BillboardList_free_one( ibb.get_value() );
        pbb = NULL;
    }
    else
    {
        // copy the tint over
        SDL_memmove( pbb->tint, tint, sizeof( GLXvector4f ) );

        if ( HAS_SOME_BITS( opt_bits, bb_opt_randomize_pos ) )
        {
            // make a random offset from the character
            pbb->offset[XX] = ((( rand() << 1 ) - RAND_MAX ) / ( const float )RAND_MAX ) * GRID_SIZE / 5.0f;
            pbb->offset[YY] = ((( rand() << 1 ) - RAND_MAX ) / ( const float )RAND_MAX ) * GRID_SIZE / 5.0f;
            pbb->offset[ZZ] = ((( rand() << 1 ) - RAND_MAX ) / ( const float )RAND_MAX ) * GRID_SIZE / 5.0f;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_randomize_vel ) )
        {
            // make the text fly away in a random direction
            pbb->offset_add[XX] += ((( rand() << 1 ) - RAND_MAX ) / ( const float )RAND_MAX ) * 2.0f * GRID_SIZE / lifetime_secs / TARGET_UPS;
            pbb->offset_add[YY] += ((( rand() << 1 ) - RAND_MAX ) / ( const float )RAND_MAX ) * 2.0f * GRID_SIZE / lifetime_secs / TARGET_UPS;
            pbb->offset_add[ZZ] += ((( rand() << 1 ) - RAND_MAX ) / ( const float )RAND_MAX ) * 2.0f * GRID_SIZE / lifetime_secs / TARGET_UPS;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_fade ) )
        {
            // make the billboard fade to transparency
            pbb->tint_add[AA] = -1.0f / lifetime_secs / TARGET_UPS;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_burn ) )
        {
            float minval, maxval;

            minval = SDL_min( SDL_min( pbb->tint[RR], pbb->tint[GG] ), pbb->tint[BB] );
            maxval = SDL_max( SDL_max( pbb->tint[RR], pbb->tint[GG] ), pbb->tint[BB] );

            if ( pbb->tint[RR] != maxval )
            {
                pbb->tint_add[RR] = -pbb->tint[RR] / lifetime_secs / TARGET_UPS;
            }

            if ( pbb->tint[GG] != maxval )
            {
                pbb->tint_add[GG] = -pbb->tint[GG] / lifetime_secs / TARGET_UPS;
            }

            if ( pbb->tint[BB] != maxval )
            {
                pbb->tint_add[BB] = -pbb->tint[BB] / lifetime_secs / TARGET_UPS;
            }
        }
    }

    return pbb;
}

//--------------------------------------------------------------------------------------------
const char * ego_chr::get_name( const CHR_REF & ichr, const Uint32 bits )
{
    static STRING szName;

    if ( !DEFINED_CHR( ichr ) )
    {
        // the default name
        strncpy( szName, "Unknown", SDL_arraysize( szName ) );
    }
    else
    {
        ego_chr * pchr = ChrObjList.get_data_ptr( ichr );
        ego_cap * pcap = pro_get_pcap( pchr->profile_ref );

        if ( pchr->nameknown )
        {
            SDL_snprintf( szName, SDL_arraysize( szName ), "%s", pchr->name );
        }
        else if ( NULL != pcap )
        {
            char lTmp;

            if ( 0 != ( bits & CHRNAME_ARTICLE ) )
            {
                const char * article;

                if ( 0 != ( bits & CHRNAME_DEFINITE ) )
                {
                    article = "the";
                }
                else
                {
                    lTmp = SDL_toupper( pcap->classname[0] );

                    if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
                    {
                        article = "an";
                    }
                    else
                    {
                        article = "a";
                    }
                }

                SDL_snprintf( szName, SDL_arraysize( szName ), "%s %s", article, pcap->classname );
            }
            else
            {
                SDL_snprintf( szName, SDL_arraysize( szName ), "%s", pcap->classname );
            }
        }
        else
        {
            strncpy( szName, "Invalid", SDL_arraysize( szName ) );
        }
    }

    if ( 0 != ( bits & CHRNAME_CAPITAL ) )
    {
        // capitalize the name ?
        szName[0] = SDL_toupper( szName[0] );
    }

    return szName;
}

//--------------------------------------------------------------------------------------------
const char * ego_chr::get_gender_possessive( const CHR_REF & ichr, char buffer[], const size_t src_buffer_len )
{
    static STRING szTmp = EMPTY_CSTR;
    ego_chr * pchr;

    size_t dst_buffer_len = src_buffer_len;

    if ( NULL == buffer )
    {
        buffer = szTmp;
        dst_buffer_len = SDL_arraysize( szTmp );
    }
    if ( 0 == dst_buffer_len ) return buffer;

    if ( !DEFINED_CHR( ichr ) )
    {
        buffer[0] = CSTR_END;
        return buffer;
    }
    pchr = ChrObjList.get_data_ptr( ichr );

    if ( GENDER_FEMALE == pchr->gender )
    {
        SDL_snprintf( buffer, dst_buffer_len, "her" );
    }
    else if ( GENDER_MALE == pchr->gender )
    {
        SDL_snprintf( buffer, dst_buffer_len, "his" );
    }
    else
    {
        SDL_snprintf( buffer, dst_buffer_len, "its" );
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
const char * ego_chr::get_gender_name( const CHR_REF & ichr, char buffer[], const size_t src_buffer_len )
{
    static STRING szTmp = EMPTY_CSTR;
    ego_chr * pchr;

    size_t dst_buffer_len = src_buffer_len;

    if ( NULL == buffer )
    {
        buffer = szTmp;
        dst_buffer_len = SDL_arraysize( szTmp );
    }
    if ( 0 == dst_buffer_len ) return buffer;

    if ( !DEFINED_CHR( ichr ) )
    {
        buffer[0] = CSTR_END;
        return buffer;
    }
    pchr = ChrObjList.get_data_ptr( ichr );

    if ( GENDER_FEMALE == pchr->gender )
    {
        SDL_snprintf( buffer, dst_buffer_len, "female " );
    }
    else if ( GENDER_MALE == pchr->gender )
    {
        SDL_snprintf( buffer, dst_buffer_len, "male " );
    }
    else
    {
        SDL_snprintf( buffer, dst_buffer_len, " " );
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
const char * ego_chr::get_dir_name( const CHR_REF & ichr )
{
    static STRING buffer = EMPTY_CSTR;
    ego_chr * pchr;

    strncpy( buffer, "/debug", SDL_arraysize( buffer ) );

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !DEFINED_PCHR( pchr ) ) return buffer;

    if ( !LOADED_PRO( pchr->profile_ref ) )
    {
        char * sztmp;

        EGOBOO_ASSERT( bfalse );

        // copy the character's data.txt path
        strncpy( buffer, ego_chr::get_obj_ref( *pchr ).obj_name, SDL_arraysize( buffer ) );

        // the name should be "...some path.../data.txt"
        // grab the path

        sztmp = SDL_strstr( buffer, "/\\" );
        if ( NULL != sztmp ) *sztmp = CSTR_END;
    }
    else
    {
        ego_pro * ppro = ProList.lst + pchr->profile_ref;

        // copy the character's data.txt path
        strncpy( buffer, ppro->name, SDL_arraysize( buffer ) );
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::update_collision_size( ego_chr * pchr, const bool_t update_matrix )
{
    ///< \details BB@> use this function to update the pchr->prt_cv and  pchr->chr_min_cv with
    ///<       values that reflect the best possible collision volume
    ///<
    ///< \note This function takes quite a bit of time, so it must only be called when the
    ///< vertices change because of an animation or because the matrix changes.
    ///<
    ///< \todo it might be possible to cache the src_vrts[] array used in this function.
    ///< if the matrix changes, then you would not need to recalculate this data...

    int       cnt;
    int       vrt_count;   // the actual number of vertices, in case the object is square
    fvec4_t   src_vrts[16];  // for the upper and lower octagon points
    fvec4_t   dst_vrts[16];  // for the upper and lower octagon points

    ego_oct_bb   bsrc, bdst;

    ego_mad * pmad;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;

    pmad = ego_chr::get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return rv_error;

    // make sure the matrix is updated properly
    if ( update_matrix )
    {
        // call ego_chr::update_matrix() but pass in a false value to prevent a recursize call
        if ( rv_error == ego_chr::update_matrix( pchr, bfalse ) )
        {
            return rv_error;
        }
    }

    // make sure the bounding box is calculated properly
    if ( rv_error == gfx_mad_instance::update_bbox( &( pchr->gfx_inst ) ) )
    {
        return rv_error;
    }

    // convert the point cloud in the ego_GLvertex array (pchr->gfx_inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    SDL_memcpy( &bsrc, &( pchr->gfx_inst.bbox ), sizeof( bsrc ) );

    // convert the corners of the level 1 bounding box to a point cloud
    vrt_count = oct_bb_to_points( &bsrc, src_vrts, 16 );

    // transform the new point cloud
    TransformVertices( &( pchr->gfx_inst.matrix ), src_vrts, dst_vrts, vrt_count );

    // convert the new point cloud into a level 1 bounding box
    points_to_oct_bb( &bdst, dst_vrts, vrt_count );

    //---- set the bounding boxes
    pchr->chr_cv     = bdst;
    pchr->chr_min_cv = bdst;
    pchr->chr_max_cv = bdst;

    // only use pchr->bump.size if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->bump_stt.size >= 0.0f )
    {
        pchr->chr_cv.mins[OCT_X ] = -pchr->bump.size;
        pchr->chr_cv.mins[OCT_Y ] = -pchr->bump.size;
        pchr->chr_cv.maxs[OCT_X ] =  pchr->bump.size;
        pchr->chr_cv.maxs[OCT_Y ] =  pchr->bump.size;

        pchr->chr_min_cv.mins[OCT_X ] = SDL_max( pchr->chr_min_cv.mins[OCT_X ], -pchr->bump.size );
        pchr->chr_min_cv.mins[OCT_Y ] = SDL_max( pchr->chr_min_cv.mins[OCT_Y ], -pchr->bump.size );
        pchr->chr_min_cv.maxs[OCT_X ] = SDL_min( pchr->chr_min_cv.maxs[OCT_X ],  pchr->bump.size );
        pchr->chr_min_cv.maxs[OCT_Y ] = SDL_min( pchr->chr_min_cv.maxs[OCT_Y ],  pchr->bump.size );

        pchr->chr_max_cv.mins[OCT_X ] = SDL_min( pchr->chr_max_cv.mins[OCT_X ], -pchr->bump.size );
        pchr->chr_max_cv.mins[OCT_Y ] = SDL_min( pchr->chr_max_cv.mins[OCT_Y ], -pchr->bump.size );
        pchr->chr_max_cv.maxs[OCT_X ] = SDL_max( pchr->chr_max_cv.maxs[OCT_X ],  pchr->bump.size );
        pchr->chr_max_cv.maxs[OCT_Y ] = SDL_max( pchr->chr_max_cv.maxs[OCT_Y ],  pchr->bump.size );
    }

    // only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->bump_stt.size_big >= 0.0f )
    {
        pchr->chr_cv.mins[OCT_YX] = -pchr->bump.size_big;
        pchr->chr_cv.mins[OCT_XY] = -pchr->bump.size_big;
        pchr->chr_cv.maxs[OCT_YX] =  pchr->bump.size_big;
        pchr->chr_cv.maxs[OCT_XY] =  pchr->bump.size_big;

        pchr->chr_min_cv.mins[OCT_YX] = SDL_max( pchr->chr_min_cv.mins[OCT_YX], -pchr->bump.size_big );
        pchr->chr_min_cv.mins[OCT_XY] = SDL_max( pchr->chr_min_cv.mins[OCT_XY], -pchr->bump.size_big );
        pchr->chr_min_cv.maxs[OCT_YX] = SDL_min( pchr->chr_min_cv.maxs[OCT_YX], pchr->bump.size_big );
        pchr->chr_min_cv.maxs[OCT_XY] = SDL_min( pchr->chr_min_cv.maxs[OCT_XY], pchr->bump.size_big );

        pchr->chr_max_cv.mins[OCT_YX] = SDL_min( pchr->chr_max_cv.mins[OCT_YX], -pchr->bump.size_big );
        pchr->chr_max_cv.mins[OCT_XY] = SDL_min( pchr->chr_max_cv.mins[OCT_XY], -pchr->bump.size_big );
        pchr->chr_max_cv.maxs[OCT_YX] = SDL_max( pchr->chr_max_cv.maxs[OCT_YX], pchr->bump.size_big );
        pchr->chr_max_cv.maxs[OCT_XY] = SDL_max( pchr->chr_max_cv.maxs[OCT_XY], pchr->bump.size_big );
    }

    // only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->bump_stt.height >= 0.0f )
    {
        pchr->chr_cv.mins[ OCT_Z ] = 0.0f;
        pchr->chr_cv.maxs[ OCT_Z ] = pchr->bump.height * pchr->fat;

        pchr->chr_min_cv.mins[ OCT_Z ] = SDL_max( pchr->chr_min_cv.mins[ OCT_Z ], 0.0f );
        pchr->chr_min_cv.maxs[ OCT_Z ] = SDL_min( pchr->chr_min_cv.maxs[ OCT_Z ], pchr->bump.height   * pchr->fat );

        pchr->chr_max_cv.mins[ OCT_Z ] = SDL_min( pchr->chr_max_cv.mins[ OCT_Z ], 0.0f );
        pchr->chr_max_cv.maxs[ OCT_Z ] = SDL_max( pchr->chr_max_cv.maxs[ OCT_Z ], pchr->bump.height   * pchr->fat );
    }

    // raise the upper bound for platforms
    if ( pchr->platform )
    {
        pchr->chr_max_cv.maxs[OCT_Z] += PLATTOLERANCE;
    }

    // make a union
    if ( pchr->ismount )
    {
        ego_oct_bb::do_union( pchr->chr_max_cv, pchr->chr_saddle_cv, pchr->chr_max_cv );
    }

    // make sure all the bounding coordinates are valid
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        if ( pchr->chr_cv.mins[cnt] >= pchr->chr_cv.maxs[cnt] )
        {
            pchr->chr_cv.mins[cnt] = pchr->chr_cv.maxs[cnt] = 0.0f;
        }

        if ( pchr->chr_min_cv.mins[cnt] >= pchr->chr_min_cv.maxs[cnt] )
        {
            pchr->chr_min_cv.mins[cnt] = pchr->chr_min_cv.maxs[cnt] = 0.0f;
        }

        if ( pchr->chr_max_cv.mins[cnt] >= pchr->chr_max_cv.maxs[cnt] )
        {
            pchr->chr_max_cv.mins[cnt] = pchr->chr_max_cv.maxs[cnt] = 0.0f;
        }
    }

    // convert the level 1 bounding box to a level 0 bounding box
    ego_oct_bb::downgrade( bdst, pchr->bump_stt, pchr->bump, &( pchr->bump_1 ), NULL );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
const char* describe_value( const float value, const float maxval, int * rank_ptr )
{
    /// \author ZF
    /// \details  This converts a stat number into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if ( cfg.feedback == FEEDBACK_NUMBER )
    {
        SDL_snprintf( retval, SDL_arraysize( retval ), "%2.1f", SFP8_TO_FLOAT(( const int )value ) );
        return retval;
    }

    fval = ( 0 == maxval ) ? 1.0f : value / maxval;

    *rank_ptr = -5;
    strcpy( retval, "Unknown" );

    if ( fval >= .83f )        { strcpy( retval, "Godlike!" );   *rank_ptr =  8; }
    else if ( fval >= .66f ) { strcpy( retval, "Ultimate" );   *rank_ptr =  7; }
    else if ( fval >= .56f ) { strcpy( retval, "Epic" );       *rank_ptr =  6; }
    else if ( fval >= .50f ) { strcpy( retval, "Powerful" );   *rank_ptr =  5; }
    else if ( fval >= .43f ) { strcpy( retval, "Heroic" );     *rank_ptr =  4; }
    else if ( fval >= .36f ) { strcpy( retval, "Very High" );  *rank_ptr =  3; }
    else if ( fval >= .30f ) { strcpy( retval, "High" );       *rank_ptr =  2; }
    else if ( fval >= .23f ) { strcpy( retval, "Good" );       *rank_ptr =  1; }
    else if ( fval >= .17f ) { strcpy( retval, "Average" );    *rank_ptr =  0; }
    else if ( fval >= .11f ) { strcpy( retval, "Pretty Low" ); *rank_ptr = -1; }
    else if ( fval >= .07f ) { strcpy( retval, "Bad" );        *rank_ptr = -2; }
    else if ( fval >  .00f ) { strcpy( retval, "Terrible" );   *rank_ptr = -3; }
    else                    { strcpy( retval, "None" );       *rank_ptr = -4; }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char* describe_damage( const float value, const float maxval, int * rank_ptr )
{
    /// \author ZF
    /// \details  This converts a damage value into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if ( cfg.feedback == FEEDBACK_NUMBER )
    {
        SDL_snprintf( retval, SDL_arraysize( retval ), "%2.1f", SFP8_TO_FLOAT( int( value ) ) );
        return retval;
    }

    fval = ( 0 == maxval ) ? 1.0f : value / maxval;

    *rank_ptr = -5;
    strcpy( retval, "Unknown" );

    if ( fval >= 1.50f )         { strcpy( retval, "Annihilation!" ); *rank_ptr =  5; }
    else if ( fval >= 1.00f ) { strcpy( retval, "Overkill!" );     *rank_ptr =  4; }
    else if ( fval >= 0.80f ) { strcpy( retval, "Deadly" );        *rank_ptr =  3; }
    else if ( fval >= 0.70f ) { strcpy( retval, "Crippling" );     *rank_ptr =  2; }
    else if ( fval >= 0.50f ) { strcpy( retval, "Devastating" );   *rank_ptr =  1; }
    else if ( fval >= 0.25f ) { strcpy( retval, "Hurtful" );       *rank_ptr =  0; }
    else if ( fval >= 0.10f ) { strcpy( retval, "A Scratch" );     *rank_ptr = -1; }
    else if ( fval >= 0.05f ) { strcpy( retval, "Ticklish" );      *rank_ptr = -2; }
    else if ( fval >= 0.00f ) { strcpy( retval, "Meh..." );        *rank_ptr = -3; }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char* describe_wounds( const float max, const float current )
{
    /// \author ZF
    /// \details  This tells us how badly someone is injured

    static STRING retval;
    float fval;

    // Is it already dead?
    if ( current <= 0 )
    {
        strcpy( retval, "Dead!" );
        return retval;
    }

    // Calculate the percentage
    if ( max == 0 ) return NULL;
    fval = ( current / max ) * 100;

    if ( cfg.feedback == FEEDBACK_NUMBER )
    {
        SDL_snprintf( retval, SDL_arraysize( retval ), "%2.1f", SFP8_TO_FLOAT( int( current ) ) );
        return retval;
    }

    strcpy( retval, "Uninjured" );
    if ( fval <= 5 )            strcpy( retval, "Almost Dead!" );
    else if ( fval <= 10 )        strcpy( retval, "Near Death" );
    else if ( fval <= 25 )        strcpy( retval, "Mortally Wounded" );
    else if ( fval <= 40 )        strcpy( retval, "Badly Wounded" );
    else if ( fval <= 60 )        strcpy( retval, "Injured" );
    else if ( fval <= 75 )        strcpy( retval, "Hurt" );
    else if ( fval <= 90 )        strcpy( retval, "Bruised" );
    else if ( fval < 100 )        strcpy( retval, "Scratched" );

    return retval;
}

//--------------------------------------------------------------------------------------------
TX_REF ego_chr::get_icon_ref( const CHR_REF & item )
{
    /// \author BB
    /// \details  Get the index to the icon texture (in TxTexture) that is supposed to be used with this object.
    ///               If none can be found, return the index to the texture of the null icon.

    size_t iskin;
    TX_REF icon_ref = TX_REF( ICON_NULL );
    bool_t is_spell_fx, is_book, draw_book;

    ego_cap * pitem_cap;
    ego_chr * pitem;
    ego_pro * pitem_pro;

    pitem = ChrObjList.get_allocated_data_ptr( item );
    if ( !DEFINED_PCHR( pitem ) ) return icon_ref;

    if ( !LOADED_PRO( pitem->profile_ref ) ) return icon_ref;
    pitem_pro = ProList.lst + pitem->profile_ref;

    pitem_cap = pro_get_pcap( pitem->profile_ref );
    if ( NULL == pitem_cap ) return icon_ref;

    // what do we need to draw?
    is_spell_fx = ( NO_SKIN_OVERRIDE != pitem_cap->spelleffect_type );     // the value of spelleffect_type == the skin of the book or -1 for not a spell effect
    is_book     = ( SPELLBOOK == pitem->profile_ref );

    /// \note ZF@> uncommented a part because this caused a icon bug when you were morphed and mounted
    draw_book   = ( is_book || ( is_spell_fx && !pitem->draw_icon ) /*|| ( is_spell_fx && MAX_CHR != pitem->attachedto )*/ ) && ( bookicon_count > 0 );

    if ( !draw_book )
    {
        iskin = pitem->skin;

        icon_ref = pitem_pro->ico_ref[iskin];
    }
    else if ( draw_book )
    {
        iskin = 0;

        if ( pitem_cap->spelleffect_type > 0 )
        {
            iskin = pitem_cap->spelleffect_type;
        }
        else if ( pitem_cap->skin_override > 0 )
        {
            iskin = pitem_cap->skin_override;
        }

        iskin = CLIP( iskin, 0, bookicon_count );

        icon_ref = bookicon_ref[ iskin ];
    }

    return icon_ref;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_cap()
{
    /// \author BB
    /// \details  initialize every character profile in the game

    CAP_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        ego_cap::init( CapStack + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_cap()
{
    /// \author BB
    /// \details  release every character profile in the game

    CAP_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_cap( cnt );
    };
}

//--------------------------------------------------------------------------------------------
bool_t release_one_cap( const CAP_REF & icap )
{
    /// \author BB
    /// \details  release any allocated data and return all values to safe values

    ego_cap * pcap;

    if ( !CapStack.in_range_ref( icap ) ) return bfalse;
    pcap = CapStack + icap;

    if ( !pcap->loaded ) return btrue;

    ego_cap::init( pcap );

    pcap->loaded  = bfalse;
    pcap->name[0] = CSTR_END;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_teams()
{
    /// \author ZZ
    /// \details  This function makes everyone hate everyone else

    TEAM_REF teama, teamb;

    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        // Make the team hate everyone
        for ( teamb = 0; teamb < TEAM_MAX; teamb++ )
        {
            TeamStack[teama].hatesteam[teamb.get_value()] = btrue;
        }

        // Make the team like itself
        TeamStack[teama].hatesteam[teama.get_value()] = bfalse;

        // Set defaults
        TeamStack[teama].leader = NOLEADER;
        TeamStack[teama].sissy = 0;
        TeamStack[teama].morale = 0;
    }

    // Keep the null team neutral
    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        TeamStack[teama].hatesteam[TEAM_NULL] = bfalse;
        TeamStack[TEAM_REF( TEAM_NULL )].hatesteam[teama.get_value()] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
bool_t chr_teleport( const CHR_REF & ichr, const float x, const float y, const float z, const FACING_T facing_z )
{
    /// \author BB
    /// \details  Determine whether the character can be teleported to the specified location
    ///               and do it, if possible. Success returns btrue, failure returns bfalse;

    ego_chr  * pchr;
    FACING_T facing_old, facing_new;
    fvec3_t  pos_old, pos_new;
    bool_t   retval;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    if ( x < 0.0f || x > PMesh->gmem.edge_x ) return bfalse;
    if ( y < 0.0f || y > PMesh->gmem.edge_y ) return bfalse;

    pos_old  = ego_chr::get_pos( pchr );
    facing_old = pchr->ori.facing_z;

    pos_new.x  = x;
    pos_new.y  = y;
    pos_new.z  = z;
    facing_new = facing_z;

    if ( chr_hit_wall( pchr, pos_new.v, NULL, NULL ) )
    {
        // No it didn't...
        ego_chr::set_pos( pchr, pos_old.v );
        pchr->ori.facing_z = facing_old;

        retval = bfalse;
    }
    else
    {
        // Yeah!  It worked!

        // update the old position
        pchr->pos_old          = pos_new;
        pchr->ori_old.facing_z = facing_new;

        // update the new position
        ego_chr::set_pos( pchr, pos_new.v );
        pchr->ori.facing_z = facing_new;

        if ( !detach_character_from_mount( ichr, btrue, bfalse ) )
        {
            // detach_character_from_mount() updates the character matrix unless it is not mounted
            ego_chr::update_matrix( pchr, btrue );
        }

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::update_hide( ego_chr * pchr )
{
    /// \author BB
    /// \details  Update the hide state of the character. Should be called every time that
    ///               the state variable in an ego_ai_state structure is updated

    Sint8 hide;
    ego_cap * pcap;

    if ( !DEFINED_PCHR( pchr ) ) return pchr;

    hide = NOHIDE;
    pcap = ego_chr::get_pcap( GET_REF_PCHR( pchr ) );
    if ( NULL != pcap )
    {
        hide = pcap->hidestate;
    }

    pchr->is_hidden = bfalse;
    if ( hide != NOHIDE && hide == pchr->ai.state )
    {
        pchr->is_hidden = btrue;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
bool_t ego_ai_state::set_changed( ego_ai_state * pai )
{
    /// \author BB
    /// \details  do something tricky here

    bool_t retval = bfalse;

    if ( NULL == pai ) return bfalse;

    if ( HAS_NO_BITS( pai->alert, ALERTIF_CHANGED ) )
    {
        ADD_BITS( pai->alert, ALERTIF_CHANGED );
        retval = btrue;
    }

    if ( !pai->changed )
    {
        pai->changed = btrue;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::matrix_valid( const ego_chr * pchr )
{
    /// \author BB
    /// \details  Determine whether the character has a valid matrix

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    // both the cache and the matrix need to be valid
    return pchr->gfx_inst.mcache.valid && pchr->gfx_inst.mcache.matrix_valid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t apply_reflection_matrix( gfx_mad_instance * pgfx_inst, const float grid_level )
{
    /// \author BB
    /// \details  Generate the extra data needed to display a reflection for this character

    if ( NULL == pgfx_inst ) return bfalse;

    // invalidate the current matrix
    pgfx_inst->ref.matrix_valid = bfalse;

    // actually flip the matrix
    if ( pgfx_inst->mcache.valid )
    {
        pgfx_inst->ref.matrix = pgfx_inst->matrix;

        pgfx_inst->ref.matrix.CNV( 0, 2 ) = -pgfx_inst->ref.matrix.CNV( 0, 2 );
        pgfx_inst->ref.matrix.CNV( 1, 2 ) = -pgfx_inst->ref.matrix.CNV( 1, 2 );
        pgfx_inst->ref.matrix.CNV( 2, 2 ) = -pgfx_inst->ref.matrix.CNV( 2, 2 );
        pgfx_inst->ref.matrix.CNV( 3, 2 ) = 2 * grid_level - pgfx_inst->ref.matrix.CNV( 3, 2 );

        pgfx_inst->ref.matrix_valid = btrue;
    }

    return pgfx_inst->ref.matrix_valid;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::matrix_data_needs_update( ego_chr * pchr, gfx_mad_matrix_data & mc )
{
    /// \author BB
    /// \details  determine whether a matrix cache has become invalid and needs to be updated

    bool_t needs_cache_update;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;

    // get the matrix data that is supposed to be used to make the matrix
    gfx_mad_matrix_data::download( mc, pchr );

    // compare that data to the actual data used to make the matrix
    needs_cache_update = ( 0 != cmp_matrix_data( &mc, &( pchr->gfx_inst.mcache ) ) );

    return needs_cache_update ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::update_matrix( ego_chr * pchr, const bool_t update_size )
{
    /// \author BB
    /// \details  Do everything necessary to set the current matrix for this character.
    ///     This might include recursively going down the list of this character's mounts, etc.
    ///
    ///     Return btrue if a new matrix is applied to the character, bfalse otherwise.

    egoboo_rv      retval;
    bool_t         needs_update = bfalse;
    bool_t         applied      = bfalse;
    gfx_mad_matrix_data mc_tmp;
    gfx_mad_matrix_data *pchr_mc = NULL;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;
    pchr_mc = &( pchr->gfx_inst.mcache );

    // recursively make sure that any mount matrices are updated
    if ( DEFINED_CHR( pchr->attachedto ) )
    {
        egoboo_rv attached_update = rv_error;

        attached_update = ego_chr::update_matrix( ChrObjList.get_data_ptr( pchr->attachedto ), btrue );

        // if this fails, we should probably do something...
        if ( rv_error == attached_update )
        {
            // there is an error so this matrix is not defined and no readon to go farther
            pchr_mc->matrix_valid = bfalse;
            return attached_update;
        }
        else if ( rv_success == attached_update )
        {
            // the holder/mount matrix has changed.
            // this matrix is no longer valid.
            pchr_mc->matrix_valid = bfalse;
        }
    }

    // does the matrix cache need an update at all?
    retval = ego_chr::matrix_data_needs_update( pchr, mc_tmp );
    if ( rv_error == retval ) return rv_error;
    needs_update = ( rv_success == retval );

    // Update the grip vertices no matter what (if they are used)
    if ( HAS_SOME_BITS( mc_tmp.type_bits, MAT_WEAPON ) && INGAME_CHR( mc_tmp.grip_chr ) )
    {
        egoboo_rv grip_retval;
        ego_chr   * ptarget = ChrObjList.get_data_ptr( mc_tmp.grip_chr );

        // has that character changes its animation?
        grip_retval = gfx_mad_instance::update_grip_verts( &( ptarget->gfx_inst ), mc_tmp.grip_verts, GRIP_VERTS );

        if ( rv_error   == grip_retval ) return rv_error;
        if ( rv_success == grip_retval ) needs_update = btrue;
    }

    // if it is not the same, make a new matrix with the new data
    applied = bfalse;
    if ( needs_update )
    {
        // we know the matrix is not valid
        pchr_mc->matrix_valid = bfalse;

        applied = gfx_mad_matrix_data::generate_matrix( mc_tmp, pchr );
    }

    if ( applied && update_size )
    {
        // call ego_chr::update_collision_size() but pass in a false value to prevent a recursize call
        ego_chr::update_collision_size( pchr, bfalse );
    }

    return applied ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
bool_t ego_ai_state::set_bumplast( ego_ai_state * pself, const CHR_REF & ichr )
{
    /// \author BB
    /// \details  bumping into a chest can initiate whole loads of update messages.
    ///     Try to throttle the rate that new "bump" messages can be passed to the ai

    if ( NULL == pself ) return bfalse;

    if ( !INGAME_CHR( ichr ) ) return bfalse;

    // 5 bumps per second?
    if ( pself->bumplast != ichr ||  update_wld > pself->bumplast_time + TARGET_UPS / 5 )
    {
        pself->bumplast_time = update_wld;
        ADD_BITS( pself->alert, ALERTIF_BUMPED );
    }
    pself->bumplast = ichr;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF chr_has_inventory_idsz( const CHR_REF & ichr, const IDSZ idsz, const bool_t equipped, CHR_REF * pack_last )
{
    /// \author BB
    /// \details  check the pack a matching item

    bool_t matches_equipped;
    CHR_REF item, tmp_item, tmp_var;
    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return CHR_REF( MAX_CHR );

    // make sure that pack_last points to something
    if ( NULL == pack_last ) pack_last = &tmp_var;

    item = CHR_REF( MAX_CHR );

    *pack_last = GET_REF_PCHR( pchr );

    PACK_BEGIN_LOOP( tmp_item, pchr->pack.next )
    {
        matches_equipped = ( !equipped || ( INGAME_CHR( tmp_item ) && ChrObjList.get_data_ref( tmp_item ).isequipped ) );

        if ( ego_chr::is_type_idsz( tmp_item, idsz ) && matches_equipped )
        {
            item = tmp_item;
            break;
        }

        *pack_last = tmp_item;
    }
    PACK_END_LOOP( tmp_item );

    return item;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_holding_idsz( const CHR_REF & ichr, const IDSZ idsz )
{
    /// \author BB
    /// \details  check the character's hands for a matching item

    bool_t found;
    CHR_REF item, tmp_item;
    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return CHR_REF( MAX_CHR );

    item = CHR_REF( MAX_CHR );
    found = bfalse;

    if ( !found )
    {
        // Check right hand. technically a held item cannot be equipped...
        tmp_item = pchr->holdingwhich[SLOT_RIGHT];

        if ( ego_chr::is_type_idsz( tmp_item, idsz ) )
        {
            found = btrue;
            item = tmp_item;
        }
    }

    if ( !found )
    {
        // Check left hand. technically a held item cannot be equipped...
        tmp_item = pchr->holdingwhich[SLOT_LEFT];

        if ( ego_chr::is_type_idsz( tmp_item, idsz ) )
        {
            found = btrue;
            item = tmp_item;
        }
    }

    return item;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_has_item_idsz( const CHR_REF & ichr, const IDSZ idsz, const bool_t equipped, CHR_REF * pack_last )
{
    /// \author BB
    /// \details  is ichr holding an item matching idsz, or is such an item in his pack?
    ///               return the index of the found item, or MAX_CHR if not found. Also return
    ///               the previous pack item in *pack_last, or MAX_CHR if it was not in a pack.

    bool_t found;
    CHR_REF item, tmp_var;
    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return CHR_REF( MAX_CHR );

    // make sure that pack_last points to something
    if ( NULL == pack_last ) pack_last = &tmp_var;

    // Check the pack
    *pack_last = CHR_REF( MAX_CHR );
    item       = CHR_REF( MAX_CHR );
    found      = bfalse;

    if ( !found )
    {
        item = chr_holding_idsz( ichr, idsz );
        found = INGAME_CHR( item );
    }

    if ( !found )
    {
        item = chr_has_inventory_idsz( ichr, idsz, equipped, pack_last );
        found = INGAME_CHR( item );
    }

    return item;
}

//--------------------------------------------------------------------------------------------
bool_t chr_can_see_object( const CHR_REF & ichr, const CHR_REF & iobj )
{
    /// \author BB
    /// \details  can ichr see iobj?

    ego_chr * pchr, * pobj;
    int     light, self_light, enviro_light;
    int     alpha;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    pobj = ChrObjList.get_allocated_data_ptr( iobj );
    if ( !INGAME_PCHR( pobj ) ) return bfalse;

    alpha = pobj->gfx_inst.alpha;
    if ( pchr->see_invisible_level > 0 )
    {
        alpha *= pchr->see_invisible_level + 1;
    }
    alpha = CLIP( alpha, 0, 255 );

    /// \note ZF@> Invictus characters can always see through darkness (spells, items, quest handlers, etc.)
    if ( IS_INVICTUS_PCHR_RAW( pchr ) && alpha >= INVISIBLE ) return btrue;

    enviro_light = ( alpha * pobj->gfx_inst.max_light ) * INV_FF;
    self_light   = ( pobj->gfx_inst.light == 255 ) ? 0 : pobj->gfx_inst.light;
    light        = SDL_max( enviro_light, self_light );

    if ( pchr->darkvision_level > 0 )
    {
        light *= pchr->darkvision_level + 1;
    }

    // Scenery, spells and quest objects can always see through darkness
    if ( IS_INVICTUS_PCHR_RAW( pchr ) ) light *= 20;

    return light >= INVISIBLE;
}

//--------------------------------------------------------------------------------------------
int ego_chr::get_price( const CHR_REF & ichr )
{
    /// \author BB
    /// \details  determine the correct price for an item

    CAP_REF icap;
    Uint16  iskin;
    float   price;

    ego_chr * pchr;
    ego_cap * pcap;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return 0;

    // Make sure spell books are priced according to their spell and not the book itself
    if ( pchr->profile_ref == SPELLBOOK )
    {
        icap = pro_get_icap( pchr->basemodel_ref );
        iskin = 0;
    }
    else
    {
        icap  = pro_get_icap( pchr->profile_ref );
        iskin = pchr->skin;
    }

    if ( !LOADED_CAP( icap ) ) return 0;
    pcap = CapStack + icap;

    price = ( const float ) pcap->skincost[iskin];

    // Items spawned in shops are more valuable
    if ( !pchr->isshopitem ) price *= 0.5f;

    // base the cost on the number of items/charges
    if ( pcap->isstackable )
    {
        price *= pchr->ammo;
    }
    else if ( pcap->isranged && pchr->ammo < pchr->ammo_max )
    {
        if ( 0 != pchr->ammo )
        {
            price *= ( const float ) pchr->ammo / ( const float ) pchr->ammo_max;
        }
    }

    return int( price );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_enviro_grid_level( ego_chr * pchr, const float level )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    if ( level != pchr->enviro.grid_level )
    {
        pchr->enviro.grid_level = level;

        apply_reflection_matrix( &( pchr->gfx_inst ), level );
    }
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_redshift( ego_chr * pchr, const int rs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->gfx_inst.redshift = rs;

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_grnshift( ego_chr * pchr, const int gs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->gfx_inst.grnshift = gs;

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_blushift( ego_chr * pchr, const int bs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->gfx_inst.blushift = bs;

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_sheen( ego_chr * pchr, const int sheen )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->gfx_inst.sheen = sheen;

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_alpha( ego_chr * pchr, const int alpha )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->gfx_inst.alpha = CLIP( alpha, 0, 255 );

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_light( ego_chr * pchr, const int light )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->gfx_inst.light = CLIP( light, 0, 255 );

    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_fly_height( ego_chr * pchr, const float height )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    ego_chr_data::set_fly_height( pchr, height );
}

//--------------------------------------------------------------------------------------------
void ego_chr_data::set_fly_height( ego_chr_data * pchr, const float height )
{
    if ( NULL == pchr ) return;

    pchr->fly_height = height;

    pchr->is_flying_platform = ( 0.0f != pchr->fly_height );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_jump_number_reset( ego_chr * pchr, const int number )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    ego_chr_data::set_jump_number_reset( pchr, number );
}

//--------------------------------------------------------------------------------------------
void ego_chr_data::set_jump_number_reset( ego_chr_data * pchr, const int number )
{
    if ( NULL == pchr ) return;

    pchr->jump_number_reset = number;

    pchr->can_fly_jump = ( JUMP_NUMBER_INFINITE == pchr->jump_number_reset );
}

//--------------------------------------------------------------------------------------------
CHR_REF ego_chr::get_lowest_attachment( const CHR_REF & ichr, const bool_t non_item )
{
    /// \author BB
    /// \details  Find the lowest attachment for a given object.
    ///               This was basically taken from the script function scr_set_TargetToLowestTarget()
    ///
    ///               You should be able to find the holder of a weapon by specifying non_item == btrue
    ///
    ///               To prevent possible loops in the data structures, use a counter to limit
    ///               the depth of the search, and make sure that ichr != ChrObjList.get_data_ref(object).attachedto

    int cnt;
    CHR_REF original_object, object, object_next;

    if ( !INGAME_CHR( ichr ) ) return CHR_REF( MAX_CHR );

    original_object = object = ichr;
    for ( cnt = 0, object = ichr; cnt < MAX_CHR && INGAME_CHR( object ); cnt++ )
    {
        object_next = ChrObjList.get_data_ref( object ).attachedto;

        if ( non_item && !ChrObjList.get_data_ref( object ).isitem )
        {
            break;
        }

        // check for a list with a loop. shouldn't happen, but...
        if ( !INGAME_CHR( object_next ) || object_next == original_object )
        {
            break;
        }

        object = object_next;
    }

    return object;
}

//--------------------------------------------------------------------------------------------
bool_t character_physics_get_mass_pair( ego_chr * pchr_a, ego_chr * pchr_b, float * wta, float * wtb )
{
    /// \author BB
    /// \details  calculate a "mass" for each object, taking into account possible infinite masses.

    float loc_wta, loc_wtb;
    bool_t infinite_weight;

    if ( !PROCESSING_PCHR( pchr_a ) || !PROCESSING_PCHR( pchr_b ) ) return bfalse;

    if ( NULL == wta ) wta = &loc_wta;
    if ( NULL == wtb ) wtb = &loc_wtb;

    infinite_weight = ( pchr_a->platform && pchr_a->is_flying_platform ) || ( INFINITE_WEIGHT == pchr_a->phys.weight );
    *wta = infinite_weight ? -( const float )INFINITE_WEIGHT : pchr_a->phys.weight;

    infinite_weight = ( pchr_b->platform && pchr_b->is_flying_platform ) || ( INFINITE_WEIGHT == pchr_b->phys.weight );
    *wtb = infinite_weight ? -( const float )INFINITE_WEIGHT : pchr_b->phys.weight;

    if ( 0.0f == *wta && 0.0f == *wtb )
    {
        *wta = *wtb = 1;
    }
    else if ( 0.0f == *wta )
    {
        *wta = 1;
        *wtb = -( const float )INFINITE_WEIGHT;
    }
    else if ( 0.0f == *wtb )
    {
        *wtb = 1;
        *wta = -( const float )INFINITE_WEIGHT;
    }

    if ( 0.0f == pchr_a->phys.bumpdampen && 0.0f == pchr_b->phys.bumpdampen )
    {
        /* do nothing */
    }
    else if ( 0.0f == pchr_a->phys.bumpdampen )
    {
        // make the weight infinite
        *wta = -( const float )INFINITE_WEIGHT;
    }
    else if ( 0.0f == pchr_b->phys.bumpdampen )
    {
        // make the weight infinite
        *wtb = -( const float )INFINITE_WEIGHT;
    }
    else
    {
        // adjust the weights to respect bumpdampen
        if ( -( const float )INFINITE_WEIGHT != *wta ) *wta /= pchr_a->phys.bumpdampen;
        if ( -( const float )INFINITE_WEIGHT != *wtb ) *wtb /= pchr_b->phys.bumpdampen;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_can_mount( const CHR_REF & ichr_a, const CHR_REF & ichr_b )
{
    bool_t is_valid_rider_a, is_valid_mount_b, has_ride_anim;
    int action_mi;

    ego_chr * pchr_a, * pchr_b;
    ego_cap * pcap_a, * pcap_b;

    // make sure that A is valid
    pchr_a = ChrObjList.get_allocated_data_ptr( ichr_a );
    if ( !INGAME_PCHR( pchr_a ) ) return bfalse;

    pcap_a = ego_chr::get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // make sure that B is valid
    pchr_b = ChrObjList.get_allocated_data_ptr( ichr_b );
    if ( !INGAME_PCHR( pchr_b ) ) return bfalse;

    pcap_b = ego_chr::get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    action_mi = mad_get_action( ego_chr::get_imad( ichr_a ), ACTION_MI );
    has_ride_anim = ( ACTION_COUNT != action_mi && !ACTION_IS_TYPE( action_mi, D ) );

    is_valid_rider_a = !pchr_a->isitem && pchr_a->alive && !IS_FLYING_PCHR( pchr_a ) &&
                       !IS_ATTACHED_PCHR( pchr_a ) && has_ride_anim;

    is_valid_mount_b = pchr_b->ismount && pchr_b->alive && !pchr_b->pack.is_packed &&
                       pcap_b->slotvalid[SLOT_LEFT] && !INGAME_CHR( pchr_b->holdingwhich[SLOT_LEFT] );

    return is_valid_rider_a && is_valid_mount_b;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_invalidate_instances( ego_chr * pchr )
{
    /// recursively invalidate all gfx_mad_instances which depend on the location of this object's vertices

    if ( !PROCESSING_PCHR( pchr ) ) return rv_error;

    // does the current mad_instance change the validation of the current gfx_mad_instance caches?
    // if there is no change, then there is nothing to do
    if ( rv_success == gfx_mad_instance::validate_pose( &( pchr->gfx_inst ), &( pchr->mad_inst ) ) )
    {
        return rv_success;
    }

    // set the flags to invalid
    pchr->gfx_inst.vrange.valid = bfalse;
    pchr->gfx_inst.mcache.valid = bfalse;

    // if it DOES change the invalidation of the gfx_mad_instance, then invalidate any child instances
    for ( int cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        CHR_REF iitem = pchr->holdingwhich[cnt];

        chr_invalidate_instances( ChrObjList.get_data_ptr( iitem ) );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::set_action( ego_chr * pchr, const int new_action, const bool_t new_ready, const bool_t override_action )
{
    egoboo_rv retval;

    if ( !PROCESSING_PCHR( pchr ) ) return rv_error;

    retval = mad_instance::set_action( &( pchr->mad_inst ), new_action, new_ready, override_action );
    if ( rv_success != retval ) return retval;

    chr_invalidate_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::start_anim( ego_chr * pchr, const int new_action, const bool_t new_ready, const bool_t override_action )
{
    egoboo_rv retval;

    if ( !PROCESSING_PCHR( pchr ) ) return rv_error;

    retval = mad_instance::start_anim( &( pchr->mad_inst ), new_action, new_ready, override_action );
    if ( rv_success != retval ) return retval;

    chr_invalidate_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::increment_action( ego_chr * pchr )
{
    egoboo_rv retval;

    if ( !PROCESSING_PCHR( pchr ) ) return rv_error;

    retval = mad_instance::increment_action( &( pchr->mad_inst ) );
    if ( rv_success != retval ) return retval;

    chr_invalidate_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::increment_frame( ego_chr * pchr )
{
    egoboo_rv retval;

    if ( !PROCESSING_PCHR( pchr ) ) return rv_error;

    retval = mad_instance::increment_frame( &( pchr->mad_inst ), DEFINED_CHR( pchr->attachedto ) );
    if ( rv_success != retval ) return retval;

    chr_invalidate_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::play_action( ego_chr * pchr, const int new_action, const bool_t new_ready )
{
    egoboo_rv retval;

    if ( !PROCESSING_PCHR( pchr ) ) return rv_error;

    retval = mad_instance::play_action( &( pchr->mad_inst ), new_action, new_ready );
    if ( rv_success != retval ) return retval;

    chr_invalidate_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
MAD_REF ego_chr::get_imad( const CHR_REF & ichr )
{
    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !DEFINED_PCHR( pchr ) ) return MAD_REF( MAX_MAD );

    // try to repair a bad model if it exists
    if ( !LOADED_MAD( pchr->mad_inst.imad ) )
    {
        MAD_REF imad_tmp = pro_get_imad( pchr->profile_ref );
        if ( LOADED_MAD( imad_tmp ) )
        {
            if ( ego_chr::set_mad( pchr, imad_tmp ) )
            {
                ego_chr::update_collision_size( pchr, btrue );
            }
        }
        if ( !LOADED_MAD( pchr->mad_inst.imad ) ) return MAD_REF( MAX_MAD );
    }

    return pchr->mad_inst.imad;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_chr::get_pmad( const CHR_REF & ichr )
{
    ego_chr * pchr;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !DEFINED_PCHR( pchr ) ) return NULL;

    // try to repair a bad model if it exists
    if ( !LOADED_MAD( pchr->mad_inst.imad ) )
    {
        MAD_REF imad_tmp = pro_get_imad( pchr->profile_ref );
        if ( LOADED_MAD( imad_tmp ) )
        {
            ego_chr::set_mad( pchr, imad_tmp );
        }
    }

    if ( !LOADED_MAD( pchr->mad_inst.imad ) ) return NULL;

    return MadStack + pchr->mad_inst.imad;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
fvec3_t ego_chr::get_pos( ego_chr * pchr )
{
    fvec3_t vtmp = ZERO_VECT3;

    if ( !VALID_PCHR( pchr ) ) return vtmp;

    return pchr->pos;
}

//--------------------------------------------------------------------------------------------
float * ego_chr::get_pos_v( ego_chr * pchr )
{
    static fvec3_t vtmp = ZERO_VECT3;

    if ( !VALID_PCHR( pchr ) ) return vtmp.v;

    return pchr->pos.v;
}

//--------------------------------------------------------------------------------------------
bool_t chr_update_pos( ego_chr * pchr )
{
    if ( !VALID_PCHR( pchr ) ) return bfalse;

    pchr->onwhichgrid   = ego_mpd::get_tile( PMesh, pchr->pos.x, pchr->pos.y );
    pchr->onwhichblock  = ego_mpd::get_block( PMesh, pchr->pos.x, pchr->pos.y );

    // update whether the current character position is safe
    ego_chr::update_safe( pchr, bfalse );

    // update the breadcrumb list
    ego_chr::update_breadcrumb( pchr, bfalse );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::set_pos( ego_chr * pchr, const fvec3_base_t pos )
{
    bool_t retval = bfalse;

    if ( !VALID_PCHR( pchr ) ) return retval;

    retval = btrue;

    if (( pos[kX] != pchr->pos.v[kX] ) || ( pos[kY] != pchr->pos.v[kY] ) || ( pos[kZ] != pchr->pos.v[kZ] ) )
    {
        SDL_memmove( pchr->pos.v, pos, sizeof( fvec3_base_t ) );

        retval = chr_update_pos( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::set_maxaccel( ego_chr * pchr, const float new_val )
{
    bool_t retval = bfalse;
    float ftmp;

    if ( !VALID_PCHR( pchr ) ) return retval;

    ftmp = pchr->maxaccel / pchr->maxaccel_reset;
    pchr->maxaccel_reset = new_val;
    pchr->maxaccel = ftmp * pchr->maxaccel_reset;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_bundle_chr * ego_bundle_chr::ctor_this( ego_bundle_chr * pbundle )
{
    if ( NULL == pbundle ) return pbundle;

    pbundle->_chr_ref = CHR_REF( MAX_CHR );
    pbundle->_chr_ptr = NULL;

    pbundle->_cap_ref = CAP_REF( MAX_CAP );
    pbundle->_cap_ptr = NULL;

    pbundle->_pro_ref = PRO_REF( MAX_PROFILE );
    pbundle->_pro_ptr = NULL;

    return pbundle;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_bundle_chr::validate()
{
    if ( NULL == this ) return rv_error;

    // get the character info from the reference or the pointer
    if ( VALID_CHR( _chr_ref ) )
    {
        _chr_ptr = ChrObjList.get_data_ptr( _chr_ref );
    }
    else if ( NULL != _chr_ptr )
    {
        _chr_ref = GET_REF_PCHR( _chr_ptr );
    }
    else
    {
        _chr_ref = MAX_CHR;
        _chr_ptr = NULL;
    }

    if ( NULL == _chr_ptr ) goto ego_chr_bundle__validate_fail;

    // get the profile info
    _pro_ref = _chr_ptr->profile_ref;
    if ( !LOADED_PRO( _pro_ref ) ) goto ego_chr_bundle__validate_fail;

    _pro_ptr = ProList.lst + _pro_ref;

    // get the cap info
    _cap_ref = _pro_ptr->icap;

    if ( !LOADED_CAP( _cap_ref ) ) goto ego_chr_bundle__validate_fail;
    _cap_ptr = CapStack + _cap_ref;

    return rv_success;

ego_chr_bundle__validate_fail:

    ctor_this( this );

    return rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_bundle_chr::set( const ego_chr * pchr )
{
    // blank out old data
    if ( NULL == ctor_this( this ) ) return rv_error;

    if ( NULL == pchr ) return rv_success;

    // set the particle pointer
    _chr_ptr = ( ego_chr * )pchr;

    return validate();
}

//--------------------------------------------------------------------------------------------
void character_physics_initialize_all()
{
    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr )
    {
        phys_data_blank_accumulators( &( pchr->phys ) );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t chr_bump_mesh_attached( ego_bundle_chr & bdl, fvec3_t test_pos, fvec3_t test_vel, const float dt )
{
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bump_mesh( ego_bundle_chr & bdl, fvec3_t test_pos, fvec3_t test_vel, const float dt )
{
    ego_chr             * loc_pchr;
    ego_cap             * loc_pcap;
    ego_phys_data       * loc_pphys;
    ego_chr_environment * loc_penviro;

    fvec3_t save_apos_plat;
    fvec3_t save_avel;
    float   dampen;
    float   diff;
    bool_t  needs_update = bfalse;

    // some aliases to make the notation easier
    loc_pchr    = bdl.chr_ptr();
    if ( NULL == loc_pchr ) return bfalse;

    loc_pcap    = bdl.cap_ptr();
    if ( NULL == loc_pcap ) return bfalse;

    loc_pphys   = &( loc_pchr->phys );
    loc_penviro = &( loc_pchr->enviro );

    // save some parameters
    save_apos_plat = loc_pphys->apos_plat;
    save_avel      = loc_pphys->avel;

    // limit the dampen to a reasonable value
    dampen = CLIP( loc_pphys->dampen, 0.0f, 1.0f );

    // interaction with the mesh
    if ( test_pos.z < loc_penviro->grid_level )
    {
        float final_pos_z = loc_penviro->walk_level;
        float final_vel_z = test_vel.z;

        // make hitting the floor be the termination condition for "jump flying"
        loc_pchr->is_flying_jump = bfalse;

        // reflect the final velocity off the surface
        final_vel_z = test_vel.z;
        if ( final_vel_z < 0.0f )
        {
            final_vel_z *= -dampen;
        }

        // determine some special cases
        if ( SDL_abs( final_vel_z ) < STOPBOUNCING )
        {
            // STICK!
            // make the object come to rest on the surface rather than bouncing forever
            final_vel_z = 0.0f;
        }
        else
        {
            // BOUNCE!
            // reflect the position off of the floor
            diff        = loc_penviro->walk_level - test_pos.z;
            final_pos_z = loc_penviro->walk_level + diff;
        }

        phys_data_accumulate_avel_index( loc_pphys, ( final_vel_z - test_vel.z ) / dt, kZ );
        phys_data_accumulate_apos_plat_index( loc_pphys, final_pos_z - test_pos.z,        kZ );
    }

    // has there been an adjustment?
    needs_update = ( fvec3_dist_abs( save_apos_plat.v, loc_pphys->apos_plat.v ) != 0.0f ) ||
                   ( fvec3_dist_abs( save_avel.v, loc_pphys->avel.v ) * dt != 0.0f ) ;

    return needs_update;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bump_grid_attached( ego_bundle_chr & bdl, fvec3_t test_pos, fvec3_t test_vel, const float dt )
{
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bump_grid( ego_bundle_chr & bdl, fvec3_t test_pos, fvec3_t test_vel, const float dt )
{
    ego_chr             * loc_pchr;
    ego_cap             * loc_pcap;
    ego_phys_data       * loc_pphys;
    ego_chr_environment * loc_penviro;

    fvec3_t save_apos_plat, save_avel;
    float   pressure;
    float   bumpdampen;
    bool_t  diff_function_called = bfalse;
    bool_t  needs_test = bfalse, updated_2d = bfalse;
    Uint32  flags = 0;

    bool_t  found_nrm  = bfalse;
    fvec2_t nrm        = ZERO_VECT2;

    bool_t  found_safe = bfalse;
    fvec3_t safe_pos   = ZERO_VECT3;

    bool_t  found_diff = bfalse;
    fvec2_t diff       = ZERO_VECT2;

    ego_breadcrumb * bc         = NULL;

    // some aliases to make the notation easier
    loc_pchr = bdl.chr_ptr();
    if ( NULL == loc_pchr ) return bfalse;

    loc_pcap = bdl.cap_ptr();
    if ( NULL == loc_pcap ) return bfalse;

    loc_pphys   = &( loc_pchr->phys );
    loc_penviro = &( loc_pchr->enviro );

    // save some parameters
    save_apos_plat = loc_pphys->apos_plat;
    save_avel      = loc_pphys->avel;

    // limit the bumpdampen to a reasonable value
    bumpdampen = CLIP( loc_pphys->bumpdampen, 0.0f, 1.0f );
    bumpdampen = ( bumpdampen + 1.0f ) / 2.0f;

    if ( !chr_test_wall( loc_pchr, test_pos.v ) )
    {
        // no interaction with the grid flags
        return bfalse;
    }

    // actually calculate the interaction with the wall
    flags = chr_hit_wall( loc_pchr, test_pos.v, nrm.v, &pressure );

    // how is the character hitting the wall?
    if ( 0.0f == pressure )
    {
        // oops the chr_test_wall() detected a false interaction with the grid flags...
        return bfalse;
    }

    // try to get the correct "outward" pressure from nrm
    if ( !found_nrm && fvec2_length_abs( nrm.v ) > 0.0f )
    {
        found_nrm = btrue;
    }

    if ( !found_diff && loc_pchr->safe_valid )
    {
        if ( !found_safe )
        {
            found_safe = btrue;
            safe_pos   = loc_pchr->safe_pos;
        }

        diff.x = loc_pchr->safe_pos.x - loc_pchr->pos.x;
        diff.y = loc_pchr->safe_pos.y - loc_pchr->pos.y;

        if ( fvec2_length_abs( diff.v ) > 0.0f )
        {
            found_diff = btrue;
        }
    }

    // try to get a diff from a breadcrumb
    if ( !found_diff )
    {
        bc = ego_chr::get_last_breadcrumb( loc_pchr );

        if ( NULL != bc && bc->valid )
        {
            if ( !found_safe )
            {
                found_safe = btrue;
                safe_pos   = loc_pchr->safe_pos;
            }

            diff.x = bc->pos.x - loc_pchr->pos.x;
            diff.y = bc->pos.y - loc_pchr->pos.y;

            if ( fvec2_length_abs( diff.v ) > 0.0f )
            {
                found_diff = btrue;
            }
        }
    }

    // try to get a normal from the ego_mpd::get_diff() function
    if ( !found_nrm )
    {
        fvec2_t tmp_diff;

        tmp_diff = chr_get_diff( loc_pchr, test_pos.v, pressure );
        diff_function_called = btrue;

        nrm.x = tmp_diff.x;
        nrm.y = tmp_diff.y;

        if ( fvec2_length_abs( nrm.v ) > 0.0f )
        {
            found_nrm = btrue;
        }
    }

    if ( !found_diff )
    {
        // try to get the diff from the character velocity
        diff.x = test_vel.x;
        diff.y = test_vel.y;

        // make sure that the diff is in the same direction as the velocity
        if ( fvec2_dot_product( diff.v, nrm.v ) < 0.0f )
        {
            diff.x *= -1.0f;
            diff.y *= -1.0f;
        }

        if ( fvec2_length_abs( diff.v ) > 0.0f )
        {
            found_diff = btrue;
        }
    }

    if ( !found_nrm )
    {
        // After all of our best efforts, we can't generate a normal to the wall.
        // This can happen if the object is completely inside a wall,
        // (like if it got pushed in there) or if a passage closed around it.
        // Just teleport the character to a "safe" position.

        if ( !found_safe && NULL == bc )
        {
            bc = ego_chr::get_last_breadcrumb( loc_pchr );

            if ( NULL != bc && bc->valid )
            {
                found_safe = btrue;
                safe_pos   = loc_pchr->safe_pos;
            }
        }

        if ( !found_safe )
        {
            // the only safe position is the spawn point???
            found_safe = btrue;
            safe_pos = loc_pchr->pos_stt;
        }

        {
            fvec3_t _tmp_vec = fvec3_sub( safe_pos.v , test_pos.v );
            phys_data_accumulate_apos_plat( loc_pphys, _tmp_vec.v );
        }
    }
    else if ( found_diff && found_nrm )
    {
        const float tile_fraction = 0.1f;
        float ftmp, dot, pressure_old, pressure_new;
        fvec3_t new_pos, save_pos;
        float nrm2;

        fvec2_t v_perp = ZERO_VECT2;
        fvec2_t diff_perp = ZERO_VECT2;

        nrm2 = fvec2_dot_product( nrm.v, nrm.v );

        save_pos = test_pos;

        // make the diff point "out"
        dot = fvec2_dot_product( diff.v, nrm.v );
        if ( dot < 0.0f )
        {
            diff.x *= -1.0f;
            diff.y *= -1.0f;
            dot    *= -1.0f;
        }

        // find the part of the diff that is parallel to the normal
        diff_perp.x = nrm.x * dot / nrm2;
        diff_perp.y = nrm.y * dot / nrm2;

        // normalize the diff_perp so that it is at most tile_fraction of a grid in any direction
        ftmp = SDL_max( SDL_abs( diff_perp.x ), SDL_abs( diff_perp.y ) );
        if ( ftmp > 0.0f )
        {
            diff_perp.x *= tile_fraction * GRID_SIZE / ftmp;
            diff_perp.y *= tile_fraction * GRID_SIZE / ftmp;
        }

        // try moving the character
        new_pos = test_pos;
        new_pos.x += diff_perp.x * pressure;
        new_pos.y += diff_perp.y * pressure;

        // determine whether the pressure is less at this location
        pressure_old = chr_get_mesh_pressure( loc_pchr, save_pos.v );
        pressure_new = chr_get_mesh_pressure( loc_pchr, new_pos.v );

        if ( pressure_new < pressure_old )
        {
            // !!success!!
            fvec3_t _tmp_vec = fvec3_sub( new_pos.v , test_pos.v );
            phys_data_accumulate_apos_plat( loc_pphys, _tmp_vec.v );
        }
        else
        {
            // !!failure!! restore the saved position
            fvec3_t _tmp_vec = fvec3_sub( save_pos.v , test_pos.v );
            phys_data_accumulate_apos_plat( loc_pphys, _tmp_vec.v );
        }

        dot = fvec2_dot_product( test_vel.v, nrm.v );
        if ( dot < 0.0f )
        {
            v_perp.x = nrm.x * dot / nrm2;
            v_perp.y = nrm.y * dot / nrm2;

            {
                fvec3_t _tmp_vec = VECT3( v_perp.x, v_perp.y, 0.0f );
                fvec3_self_scale( _tmp_vec.v, - ( 1.0f + bumpdampen ) * pressure / dt );
                phys_data_accumulate_avel( loc_pphys, _tmp_vec.v );
            }
        }
    }

    needs_test = ( fvec3_dist_abs( save_apos_plat.v, loc_pphys->apos_plat.v ) != 0.0f ) ||
                 ( fvec3_dist_abs( save_avel.v, loc_pphys->avel.v ) * dt != 0.0f ) ;

    return needs_test || updated_2d;
}

//--------------------------------------------------------------------------------------------
void character_physics_finalize_one( ego_bundle_chr & bdl, const float dt )
{
    bool_t bumped_mesh = bfalse, bumped_grid  = bfalse, needs_update = bfalse;

    fvec3_t test_pos, test_vel;

    // aliases for easier notation
    ego_chr * loc_pchr;
    ego_phys_data * loc_pphys;

    // alias these parameter for easier notation
    loc_pchr   = bdl.chr_ptr();
    loc_pphys  = &( loc_pchr->phys );

    // do the "integration" of the accumulators
    // work on test_pos and test velocity instead of the actual character position and velocity
    test_pos = ego_chr::get_pos( loc_pchr );
    test_vel = loc_pchr->vel;
    phys_data_integrate_accumulators( &test_pos, &test_vel, loc_pphys, dt );

    // bump the character with the mesh
    bumped_mesh = bfalse;
    if ( PROCESSING_CHR( loc_pchr->attachedto ) )
    {
        bumped_mesh = chr_bump_mesh_attached( bdl, test_pos, test_vel, dt );
    }
    else
    {
        bumped_mesh = chr_bump_mesh( bdl, test_pos, test_vel, dt );
    }

    // if the character hit the mesh, re-do the "integration" of the accumulators again,
    // to make sure that it does not go through the mesh
    if ( bumped_mesh )
    {
        test_pos = ego_chr::get_pos( loc_pchr );
        test_vel = loc_pchr->vel;
        phys_data_integrate_accumulators( &test_pos, &test_vel, loc_pphys, dt );
    }

    // bump the character with the grid flags
    bumped_grid = bfalse;
    if ( PROCESSING_CHR( loc_pchr->attachedto ) )
    {
        bumped_grid = chr_bump_grid_attached( bdl, test_pos, test_vel, dt );
    }
    else
    {
        bumped_grid = chr_bump_grid( bdl, test_pos, test_vel, dt );
    }

    // if the character hit the grid, re-do the "integration" of the accumulators again,
    // to make sure that it does not go through the grid
    if ( bumped_grid )
    {
        test_pos = ego_chr::get_pos( loc_pchr );
        test_vel = loc_pchr->vel;
        phys_data_integrate_accumulators( &test_pos, &test_vel, loc_pphys, dt );
    }

    // do a special "non-physics" update
    if ( loc_pchr->is_flying_platform && test_pos.z < 0.0f )
    {
        test_pos.z = 0.0f;  // Don't fall in pits...
    }

    // determine whether there is any need to update the character's safe position
    needs_update = bumped_mesh || bumped_grid;

    // update the character's position and velocity
    ego_chr::set_pos( loc_pchr, test_pos.v );
    loc_pchr->vel = test_vel;

    // we need to test the validity of the current position every 8 frames or so,
    // no matter what
    if ( !needs_update )
    {
        // make a timer that is individual for each object
        Uint32 chr_update = ego_chr::get_obj_ref( *loc_pchr ).get_id() + update_wld;

        needs_update = ( 0 == ( chr_update & 7 ) );
    }

    if ( needs_update )
    {
        ego_chr::update_safe( loc_pchr, needs_update );
    }
}

//--------------------------------------------------------------------------------------------
void character_physics_finalize_all( const float dt )
{
    // accumulate the accumulators
    CHR_BEGIN_LOOP_PROCESSING( ichr, pchr )
    {
        ego_bundle_chr bdl( pchr );
        character_physics_finalize_one( bdl, dt );
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_chr::update_breadcrumb_raw( ego_chr * pchr )
{
    ego_breadcrumb bc;
    bool_t retval = bfalse;

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    breadcrumb_init_chr( &bc, pchr );

    if ( bc.valid )
    {
        retval = breadcrumb_list_add( &( pchr->crumbs ), &bc );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::update_breadcrumb( ego_chr * pchr, const bool_t force )
{
    Uint32 new_grid;
    bool_t retval = bfalse;
    bool_t needs_update = bfalse;
    ego_breadcrumb * bc_ptr, bc;

    bool_t loc_force = force;

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    bc_ptr = breadcrumb_list_last_valid( &( pchr->crumbs ) );
    if ( NULL == bc_ptr )
    {
        loc_force  = btrue;
        bc_ptr = &bc;
        breadcrumb_init_chr( bc_ptr, pchr );
    }

    if ( loc_force )
    {
        needs_update = btrue;
    }
    else
    {
        new_grid = ego_mpd::get_tile( PMesh, pchr->pos.x, pchr->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( SDL_abs( pchr->pos.x - bc_ptr->pos.x ) > GRID_SIZE ||
                 SDL_abs( pchr->pos.y - bc_ptr->pos.y ) > GRID_SIZE )
            {
                needs_update = btrue;
            }
        }
        else if ( new_grid != bc_ptr->grid )
        {
            needs_update = btrue;
        }
    }

    if ( needs_update )
    {
        retval = ego_chr::update_breadcrumb_raw( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_breadcrumb * ego_chr::get_last_breadcrumb( ego_chr * pchr )
{
    if ( !VALID_PCHR( pchr ) ) return NULL;

    if ( 0 == pchr->crumbs.count ) return NULL;

    return breadcrumb_list_last_valid( &( pchr->crumbs ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_chr::update_safe_raw( ego_chr * pchr )
{
    bool_t retval = bfalse;

    BIT_FIELD hit_a_wall;

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    hit_a_wall = chr_hit_wall( pchr, NULL, NULL, NULL );
    if ( 0 != hit_a_wall )
    {
        pchr->safe_valid = btrue;
        pchr->safe_pos   = ego_chr::get_pos( pchr );
        pchr->safe_time  = update_wld;
        pchr->safe_grid  = ego_mpd::get_tile( PMesh, pchr->pos.x, pchr->pos.y );

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::update_safe( ego_chr * pchr, const bool_t force )
{
    Uint32 new_grid;
    bool_t retval = bfalse;
    bool_t needs_update = bfalse;

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    if ( force || !pchr->safe_valid )
    {
        needs_update = btrue;
    }
    else
    {
        new_grid = ego_mpd::get_tile( PMesh, pchr->pos.x, pchr->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( SDL_abs( pchr->pos.x - pchr->safe_pos.x ) > GRID_SIZE ||
                 SDL_abs( pchr->pos.y - pchr->safe_pos.y ) > GRID_SIZE )
            {
                needs_update = btrue;
            }
        }
        else if ( new_grid != pchr->safe_grid )
        {
            needs_update = btrue;
        }
    }

    if ( needs_update )
    {
        retval = ego_chr::update_safe_raw( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::get_safe( ego_chr * pchr, fvec3_base_t pos_v )
{
    bool_t found = bfalse;
    fvec3_t loc_pos;

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    // handle optional parameters
    if ( NULL == pos_v ) pos_v = loc_pos.v;

    if ( !found && pchr->safe_valid )
    {
        if ( !chr_hit_wall( pchr, NULL, NULL, NULL ) )
        {
            found = btrue;
            SDL_memmove( pos_v, pchr->safe_pos.v, sizeof( fvec3_base_t ) );
        }
    }

    if ( !found )
    {
        ego_breadcrumb * bc;

        bc = breadcrumb_list_last_valid( &( pchr->crumbs ) );

        if ( NULL != bc )
        {
            found = btrue;
            SDL_memmove( pos_v, bc->pos.v, sizeof( fvec3_base_t ) );
        }
    }

    // maybe there is one last fallback after this? we could check the character's current position?
    if ( !found )
    {
        log_debug( "Uh oh! We could not find a valid non-wall position for %s!\n", ego_chr::get_pcap( pchr->ai.index )->name );
    }

    return found;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void latch_game_init( ego_latch_game * platch )
{
    if ( NULL == platch ) return;

    SDL_memset( platch, 0, sizeof( *platch ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t pack_validate( ego_pack * ppack )
{
    int      cnt;
    ego_pack * parent_pack_ptr;
    CHR_REF  item_ref;

    if ( NULL == ppack ) return bfalse;
    ppack->is_packed  = bfalse;
    ppack->was_packed = bfalse;

    // limit the number of objects in the pack to less than the total number of objects...
    ppack->count = SDL_min( ppack->count, MAX_CHR - 1 );

    parent_pack_ptr = ppack;
    item_ref        = ppack->next;
    for ( cnt = 0; cnt < ppack->count && DEFINED_CHR( item_ref ); cnt++ )
    {
        ego_chr  * item_ptr      = ChrObjList.get_data_ptr( item_ref );
        ego_pack * item_pack_ptr = &( item_ptr->pack );

        // make sure that the item "pack" is working as a stored item and not a
        // sub-pack
        item_pack_ptr->count     = 0;
        item_pack_ptr->is_packed = btrue;

        // is this item allowed to be packed?
        //Everything is allowed to be packed. Some monsters hide in chests
        //and don't pop out until the chest is opened. This is true even if they
        //aren't items, so allow non-items in packs.
        /*        if ( !item_ptr->isitem )
                {
                    // how did this get in a pack?
                    log_warning( "pack_validate() - The item %s is in a pack, even though it is not tagged as an item.\n", ego_chr::get_obj_ref(*item_ptr).obj_name );

                    // remove the item from the pack
                    parent_pack_ptr->next     = item_pack_ptr->next;

                    // re-initialize the item's "pack"
                    item_pack_ptr->next       = CHR_REF(MAX_CHR);
                    item_pack_ptr->was_packed = item_pack_ptr->is_packed;
                    item_pack_ptr->is_packed  = bfalse;
                }
                else*/

        {
            parent_pack_ptr = item_pack_ptr;
        }

        // update the reference
        item_ref = parent_pack_ptr->next;
    }

    // Did the loop terminate properly?
    if ( MAX_CHR != item_ref && !DEFINED_CHR( item_ref ) )
    {
        // There was corrupt data in the pack list.
        log_warning( "%s - Found a bad pack and am fixing it.\n", __FUNCTION__ );

        if ( parent_pack_ptr == ppack )
        {
            ppack->count = 0;
        }
        else
        {
            parent_pack_ptr->next = CHR_REF( MAX_CHR );
        }
    }

    // update the pack count to the actual number of objects in the pack
    ppack->count = cnt;

    // make sure that the ppack->next matches ppack->count
    if ( 0 == ppack->count )
    {
        ppack->next = CHR_REF( MAX_CHR );
    }

    // make sure that the ppack->count matches ppack->next
    if ( !DEFINED_CHR( ppack->next ) )
    {
        ppack->count = 0;
        ppack->next  = CHR_REF( MAX_CHR );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t pack_add_item( ego_pack * ppack, const CHR_REF & item )
{
    ego_chr  * pitem;
    ego_pack * pitem_pack;

    // make sure the pack is valid
    if ( !pack_validate( ppack ) ) return bfalse;

    // make sure that the item is valid
    if ( !DEFINED_CHR( item ) ) return bfalse;
    pitem      = ChrObjList.get_data_ptr( item );
    pitem_pack = &( pitem->pack );

    // is this item packed in another pack?
    if ( pitem_pack->is_packed )
    {
        log_warning( "%s - Trying to add a packed item (%s) to a pack.\n", __FUNCTION__, ego_chr::get_obj_ref( *pitem ).obj_name );

        return bfalse;
    }

    // does this item have packed objects of its own?
    if ( 0 != pitem_pack->count || MAX_CHR != pitem_pack->next )
    {
        log_warning( "%s - Trying to add an item (%s) to a pack that has a sub-pack.\n", __FUNCTION__, ego_chr::get_obj_ref( *pitem ).obj_name );

        return bfalse;
    }

    // is the item even an item?
    if ( !pitem->isitem )
    {
        log_debug( "%s - Trying to add a non-item %s to a pack.\n", __FUNCTION__, ego_chr::get_obj_ref( *pitem ).obj_name );
    }

    // add the item to the front of the pack's linked list
    pitem_pack->next = ppack->next;
    ppack->next      = item;
    ppack->count++;

    // tag the item as packed
    pitem_pack->was_packed = pitem_pack->is_packed;
    pitem_pack->is_packed  = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t pack_remove_item( ego_pack * ppack, const CHR_REF & iparent, const CHR_REF & iitem )
{
    CHR_REF old_next;
    ego_chr * pitem, * pparent;

    // make sure the pack is valid
    if ( !pack_validate( ppack ) ) return bfalse;

    // convert the iitem it to a pointer
    old_next = CHR_REF( MAX_CHR );
    pitem    = NULL;
    if ( DEFINED_CHR( iitem ) )
    {
        pitem    = ChrObjList.get_data_ptr( iitem );
        old_next = pitem->pack.next;
    }

    // convert the pparent it to a pointer
    pparent = NULL;
    if ( DEFINED_CHR( iparent ) )
    {
        pparent = ChrObjList.get_data_ptr( iparent );
    }

    // Remove the item from the pack
    if ( NULL != pitem )
    {
        pitem->pack.was_packed = pitem->pack.is_packed;
        pitem->pack.is_packed  = bfalse;
        pitem->pack.next       = CHR_REF( MAX_CHR );
    }

    // adjust the parent's next
    if ( NULL != pparent )
    {
        pparent->pack.next = old_next;
    }

    // just re-validate the pack to get the correct count
    pack_validate( ppack );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_inventory_add_item( const CHR_REF & item, const CHR_REF & character )
{
    ego_chr * pchr, * pitem;
    ego_cap * pitem_cap;
    bool_t  slot_found, pack_added;
    int     slot_index;
    int     cnt;

    pitem = ChrObjList.get_allocated_data_ptr( item );
    if ( !INGAME_PCHR( pitem ) ) return bfalse;

    // don't allow sub-inventories
    if ( pitem->pack.is_packed || pitem->isequipped ) return bfalse;

    pitem_cap = pro_get_pcap( pitem->profile_ref );
    if ( NULL == pitem_cap ) return bfalse;

    pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return bfalse;

    // don't allow sub-inventories
    if ( pchr->pack.is_packed || pchr->isequipped ) return bfalse;

    slot_found = bfalse;
    slot_index = 0;
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        if ( IDSZ_NONE == inventory_idsz[cnt] ) continue;

        if ( inventory_idsz[cnt] == pitem_cap->idsz[IDSZ_PARENT] )
        {
            slot_index = cnt;
            slot_found = btrue;
        }
    }

    if ( slot_found )
    {
        if ( INGAME_CHR( pchr->holdingwhich[slot_index] ) )
        {
            pchr->inventory[slot_index] = CHR_REF( MAX_CHR );
        }
    }

    pack_added = chr_pack_add_item( item, character );
    if ( slot_found && pack_added )
    {
        pchr->inventory[slot_index] = item;
    }

    return pack_added;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_inventory_remove_item( const CHR_REF & ichr, const grip_offset_t grip_off, const bool_t ignorekurse )
{
    ego_chr * pchr;
    CHR_REF iitem;
    int     cnt;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !INGAME_PCHR( pchr ) ) return CHR_REF( MAX_CHR );

    // make sure the pack is not empty
    if ( 0 == pchr->pack.count || MAX_CHR == pchr->pack.next ) return CHR_REF( MAX_CHR );

    // do not allow sub inventories
    if ( pchr->pack.is_packed || pchr->isitem ) return CHR_REF( MAX_CHR );

    // grab the first item from the pack
    iitem = chr_pack_get_item( ichr, grip_off, ignorekurse );
    if ( !INGAME_CHR( iitem ) ) return CHR_REF( MAX_CHR );

    // remove it from the "equipped" slots
    if ( DEFINED_CHR( iitem ) )
    {
        for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
        {
            if ( iitem == pchr->inventory[cnt] )
            {
                pchr->inventory[cnt]          = CHR_REF( MAX_CHR );
                ChrObjList.get_data_ref( iitem ).isequipped = bfalse;
                break;
            }
        }
    }

    return iitem;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF chr_pack_has_a_stack( const CHR_REF & item, const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function looks in the character's pack for an item similar
    ///    to the one given.  If it finds one, it returns the similar item's
    ///    index number, otherwise it returns MAX_CHR.

    CHR_REF istack;
    Uint16  id;
    bool_t  found;

    ego_chr * pitem;
    ego_cap * pitem_cap;

    found  = bfalse;
    istack = CHR_REF( MAX_CHR );

    ego_obj_chr * pchr = ChrObjList.get_allocated_data_ptr( character );
    if ( !INGAME_PCHR( pchr ) ) return istack;

    pitem = ChrObjList.get_allocated_data_ptr( item );
    if ( !INGAME_PCHR( pitem ) ) return istack;
    pitem_cap = ego_chr::get_pcap( item );

    if ( pitem_cap->isstackable )
    {
        PACK_BEGIN_LOOP( istack, pchr->pack.next )
        {
            if ( INGAME_CHR( istack ) )
            {
                ego_chr * pstack     = ChrObjList.get_data_ptr( istack );
                ego_cap * pstack_cap = ego_chr::get_pcap( istack );

                found = pstack_cap->isstackable;

                if ( pstack->ammo >= pstack->ammo_max )
                {
                    found = bfalse;
                }

                // you can still stack something even if the profiles don't match exactly,
                // but they have to have all the same IDSZ properties
                if ( found && ( pstack->profile_ref != pitem->profile_ref ) )
                {
                    for ( id = 0; id < IDSZ_COUNT && found; id++ )
                    {
                        if ( ego_chr::get_idsz( istack, id ) != ego_chr::get_idsz( item, id ) )
                        {
                            found = bfalse;
                        }
                    }
                }
            }

            if ( found ) break;
        }
        PACK_END_LOOP( istack );

        if ( !found )
        {
            istack = CHR_REF( MAX_CHR );
        }
    }

    return istack;
}

//--------------------------------------------------------------------------------------------
bool_t chr_pack_add_item( const CHR_REF & item, const CHR_REF & character )
{
    /// \author ZZ
    /// \details  This function puts an item inside a character's pack

    CHR_REF stack;
    int     newammo;

    ego_chr  * pchr, * pitem;
    ego_cap  * pchr_cap,  * pitem_cap;
    ego_pack * pchr_pack, * pitem_pack;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr      = ChrObjList.get_data_ptr( character );
    pchr_pack = &( pchr->pack );
    pchr_cap  = ego_chr::get_pcap( character );

    if ( !INGAME_CHR( item ) ) return bfalse;
    pitem      = ChrObjList.get_data_ptr( item );
    pitem_pack = &( pitem->pack );
    pitem_cap  = ego_chr::get_pcap( item );

    // Make sure everything is hunkydori
    if ( pitem_pack->is_packed || pchr_pack->is_packed || pchr->isitem )
        return bfalse;

    stack = chr_pack_has_a_stack( item, character );
    if ( INGAME_CHR( stack ) )
    {
        // We found a similar, stackable item in the pack

        ego_chr  * pstack      = ChrObjList.get_data_ptr( stack );
        ego_cap  * pstack_cap  = ego_chr::get_pcap( stack );

        // reveal the name of the item or the stack
        if ( pitem->nameknown || pstack->nameknown )
        {
            pitem->nameknown  = btrue;
            pstack->nameknown = btrue;
        }

        // reveal the usage of the item or the stack
        if ( pitem_cap->usageknown || pstack_cap->usageknown )
        {
            pitem_cap->usageknown  = btrue;
            pstack_cap->usageknown = btrue;
        }

        // add the item ammo to the stack
        newammo = pitem->ammo + pstack->ammo;
        if ( newammo <= pstack->ammo_max )
        {
            // All transferred, so kill the in hand item
            pstack->ammo = newammo;
            if ( INGAME_CHR( pitem->attachedto ) )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            ego_obj_chr::request_terminate( item );
        }
        else
        {
            // Only some were transferred,
            pitem->ammo     = pitem->ammo + pstack->ammo - pstack->ammo_max;
            pstack->ammo    = pstack->ammo_max;
            ADD_BITS( pchr->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
        }
    }
    else
    {
        // Make sure we have room for another item
        if ( pchr_pack->count >= MAXNUMINPACK )
        {
            ADD_BITS( pchr->ai.alert, ALERTIF_TOOMUCHBAGGAGE );
            return bfalse;
        }

        // Take the item out of hand
        if ( INGAME_CHR( pitem->attachedto ) )
        {
            detach_character_from_mount( item, btrue, bfalse );

            // clear the dropped flag
            REMOVE_BITS( pitem->ai.alert, ALERTIF_DROPPED );
        }

        // Remove the item from play
        pitem->hitready        = bfalse;

        // Insert the item into the pack as the first one
        pack_add_item( pchr_pack, item );

        // fix the flags
        if ( pitem_cap->isequipment )
        {
            ADD_BITS( pitem->ai.alert, ALERTIF_PUTAWAY );  // same as ALERTIF_ATLASTWAYPOINT;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_pack_remove_item( const CHR_REF & ichr, const CHR_REF & iparent, const CHR_REF & iitem )
{
    ego_chr  * pchr;
    ego_pack * pchr_pack;

    bool_t removed;

    pchr = ChrObjList.get_allocated_data_ptr( ichr );
    if ( !DEFINED_PCHR( pchr ) ) return bfalse;
    pchr_pack = &( pchr->pack );

    // remove it from the pack
    removed = pack_remove_item( pchr_pack, iparent, iitem );

    // unequip the item
    if ( removed && DEFINED_CHR( iitem ) )
    {
        ChrObjList.get_data_ref( iitem ).isequipped = bfalse;
    }

    return removed;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_pack_get_item( const CHR_REF & chr_ref, const grip_offset_t grip_off, const bool_t ignorekurse )
{
    /// \author ZZ
    /// \details  This function takes the last item in the chrcharacter's pack and puts
    ///    it into the designated hand.  It returns the item_ref or MAX_CHR.

    CHR_REF tmp_ref, item_ref, parent_ref;
    ego_chr  * pchr, * item_ptr, *parent_ptr;
    ego_pack * chr_pack_ptr, * item_pack_ptr, *parent_pack_ptr;

    // does the chr_ref exist?
    if ( !DEFINED_CHR( chr_ref ) ) return CHR_REF( MAX_CHR );
    pchr         = ChrObjList.get_data_ptr( chr_ref );
    chr_pack_ptr = &( pchr->pack );

    // Can the chr_ref have a pack?
    if ( chr_pack_ptr->is_packed || pchr->isitem ) return CHR_REF( MAX_CHR );

    // is the pack empty?
    if ( MAX_CHR == chr_pack_ptr->next || 0 == chr_pack_ptr->count )
    {
        return CHR_REF( MAX_CHR );
    }

    // Find the last item_ref in the pack
    parent_ref = chr_ref;
    item_ref   = chr_ref;
    PACK_BEGIN_LOOP( tmp_ref, chr_pack_ptr->next )
    {
        parent_ref = item_ref;
        item_ref   = tmp_ref;
    }
    PACK_END_LOOP( tmp_ref );

    // did we find anything?
    if ( chr_ref == item_ref || MAX_CHR == item_ref ) return CHR_REF( MAX_CHR );

    // convert the item_ref it to a pointer
    item_ptr      = NULL;
    item_pack_ptr = NULL;
    if ( DEFINED_CHR( item_ref ) )
    {
        item_ptr = ChrObjList.get_data_ptr( item_ref );
        item_pack_ptr = &( item_ptr->pack );
    }

    // did we find a valid item?
    if ( NULL == item_ptr )
    {
        chr_pack_remove_item( chr_ref, parent_ref, item_ref );

        return CHR_REF( MAX_CHR );
    }

    // convert the parent_ptr it to a pointer
    parent_ptr      = NULL;
    parent_pack_ptr = NULL;
    if ( DEFINED_CHR( parent_ref ) )
    {
        parent_ptr      = ChrObjList.get_data_ptr( parent_ref );
        parent_pack_ptr = &( parent_ptr->pack );
    }

    // Figure out what to do with it
    if ( item_ptr->iskursed && item_ptr->isequipped && !ignorekurse )
    {
        // The equipped item cannot be taken off, move the item to the front
        // of the pack

        // remove the item from the list
        parent_pack_ptr->next = item_pack_ptr->next;
        item_pack_ptr->next   = CHR_REF( MAX_CHR );

        // Add the item to the front of the list
        item_pack_ptr->next = chr_pack_ptr->next;
        chr_pack_ptr->next  = item_ref;

        // Flag the last item_ref as not removed
        ADD_BITS( item_ptr->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY

        // let the calling function know that we didn't find anything
        item_ref = CHR_REF( MAX_CHR );
    }
    else
    {
        // Remove the last item from the pack
        chr_pack_remove_item( chr_ref, parent_ref, item_ref );
        ADD_BITS( item_ptr->ai.alert, ALERTIF_TAKENOUT );

        // Attach the item to the correct grip
        attach_character_to_mount( item_ref, chr_ref, grip_off );

        // remove the ALERTIF_GRABBED flag that is generated by attach_character_to_mount()
        REMOVE_BITS( item_ptr->ai.alert, ALERTIF_GRABBED );
    }

    // use this function to make sure the pack count is valid
    pack_validate( chr_pack_ptr );

    return item_ref;
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_over_water( ego_chr *pchr )
{
    /// \author ZF
    /// \details  This function returns true if the character is over a water tile

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( !water.is_water || !ego_mpd::grid_is_valid( PMesh, pchr->onwhichgrid ) ) return bfalse;

    return 0 != ego_mpd::test_fx( PMesh, pchr->onwhichgrid, MPDFX_WATER );
}

//--------------------------------------------------------------------------------------------
float calc_dismount_lerp( const ego_chr * pchr_a, const ego_chr * pchr_b )
{
    /// \author BB
    /// \details  generate a "lerp" for characters that have dismounted

    CHR_REF ichr_a, ichr_b;
    float dismount_lerp_a, dismount_lerp_b;
    bool_t found = bfalse;

    if ( !DEFINED_PCHR( pchr_a ) ) return 0.0f;
    ichr_a = GET_REF_PCHR( pchr_a );

    if ( !DEFINED_PCHR( pchr_b ) ) return 0.0f;
    ichr_b = GET_REF_PCHR( pchr_b );

    dismount_lerp_a = 1.0f;
    if ( pchr_a->dismount_timer > 0 && pchr_a->dismount_object == ichr_b )
    {
        dismount_lerp_a = ( const float )pchr_a->dismount_timer / ( const float )PHYS_DISMOUNT_TIME;
        dismount_lerp_a = 1.0f - CLIP( dismount_lerp_a, 0.0f, 1.0f );
        found = ( 1.0f != dismount_lerp_a );
    }

    dismount_lerp_b = 1.0f;
    if ( pchr_b->dismount_timer > 0 && pchr_b->dismount_object == ichr_a )
    {
        dismount_lerp_b = ( const float )pchr_b->dismount_timer / ( const float )PHYS_DISMOUNT_TIME;
        dismount_lerp_b = 1.0f - CLIP( dismount_lerp_b, 0.0f, 1.0f );
        found = ( 1.0f != dismount_lerp_b );
    }

    return !found ? 1.0f : dismount_lerp_a * dismount_lerp_b;
}

//--------------------------------------------------------------------------------------------
bool_t chr_copy_enviro( const ego_chr * chr_psrc, ego_chr * chr_pdst )
{
    /// \author BB
    /// \details do a deep copy on the character's enviro data

    const ego_chr_environment * psrc;
    ego_chr_environment * pdst;

    if ( NULL == chr_psrc || NULL == chr_pdst ) return bfalse;

    if ( chr_psrc == chr_pdst ) return btrue;

    psrc = &( chr_psrc->enviro );
    pdst = &( chr_pdst->enviro );

    // use the special function to set the grid level
    // this must done first so that the character's reflection data is set properly
    ego_chr::set_enviro_grid_level( chr_pdst, psrc->grid_level );

    // now just copy the other data.
    // use SDL_memmove() in the odd case the regions overlap
    SDL_memmove( pdst, psrc, sizeof( *psrc ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
// struct ego_chr_data - memory management
//--------------------------------------------------------------------------------------------
ego_chr_data * ego_chr_data::alloc( ego_chr_data * pdata )
{
    /// Free all allocated memory

    if ( NULL == pdata ) return pdata;

    // allocate
    gfx_mad_instance::ctor_this( &( pdata->gfx_inst ) );

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_chr_data * ego_chr_data::dealloc( ego_chr_data * pdata )
{
    /// Free all allocated memory

    if ( NULL == pdata ) return pdata;

    gfx_mad_instance::dtor_this( &( pdata->gfx_inst ) );

    EGOBOO_ASSERT( NULL == pdata->gfx_inst.vrt_lst );

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_chr_data * ego_chr_data::ctor_this( ego_chr_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    // set the critical values to safe ones
    pdata = ego_chr_data::init( pdata );
    if ( NULL == pdata ) return NULL;

    // construct/allocate any dynamic data
    pdata = ego_chr_data::alloc( pdata );
    if ( NULL == pdata ) return NULL;

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_chr_data * ego_chr_data::dtor_this( ego_chr_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    // destruct/free any dynamic data
    pdata = ego_chr_data::dealloc( pdata );
    if ( NULL == pdata ) return NULL;

    // remove it from the BillboardList
    BillboardList_free_one(( pdata->ibillboard ).get_value() );

    // remove it from the LoopedList
    LoopedList_remove( pdata->loopedsound_channel );

    // reset the ego_ai_state
    ego_ai_state::dtor_this( &( pdata->ai ) );

    // set the critical values to safe ones
    pdata = ego_chr_data::init( pdata );
    if ( NULL == pdata ) return NULL;

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_chr_data * ego_chr_data::init( ego_chr_data * pdata )
{
    /// \author BB
    /// \details  initialize the character data to safe values
    ///     since we use SDL_memset(..., 0, ...), everything == 0 == false == 0.0f
    ///     statements are redundant

    int cnt;

    if ( NULL == pdata ) return pdata;

    // IMPORTANT!!!
    pdata->ibillboard = INVALID_BILLBOARD;
    pdata->sparkle = NOSPARKLE;
    pdata->loopedsound_channel = INVALID_SOUND_CHANNEL;

    // Set up model stuff
    pdata->inwhich_slot = SLOT_LEFT;
    pdata->hitready = btrue;
    pdata->boretime = BORETIME;
    pdata->carefultime = CAREFULTIME;

    // Enchant stuff
    pdata->firstenchant = ENC_REF( MAX_ENC );
    pdata->undoenchant = ENC_REF( MAX_ENC );
    pdata->missiletreatment = MISSILE_NORMAL;

    // Character stuff
    pdata->turnmode = TURNMODE_VELOCITY;
    pdata->alive = btrue;

    // Jumping
    pdata->jump_time = JUMP_DELAY;

    // Grip info
    pdata->attachedto = CHR_REF( MAX_CHR );
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pdata->holdingwhich[cnt] = CHR_REF( MAX_CHR );
    }

    // pack/inventory info
    pdata->pack.next = CHR_REF( MAX_CHR );
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        pdata->inventory[cnt] = CHR_REF( MAX_CHR );
    }

    // Set up position
    pdata->ori.map_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    pdata->ori.map_facing_x = MAP_TURN_OFFSET;

    // \note BB@> I think we have to set the dismount timer, otherwise objects that
    // are spawned by chests will behave strangely...
    // nope this did not fix it
    // \note ZF@> If this is != 0 then scorpion claws and riders are dropped at spawn (non-item objects)
    pdata->dismount_timer  = 0;
    pdata->dismount_object = CHR_REF( MAX_CHR );

    // set all of the integer references to invalid values
    pdata->firstenchant = ENC_REF( MAX_ENC );
    pdata->undoenchant  = ENC_REF( MAX_ENC );
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pdata->holdingwhich[cnt] = CHR_REF( MAX_CHR );
    }

    pdata->pack.next = CHR_REF( MAX_CHR );
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        pdata->inventory[cnt] = CHR_REF( MAX_CHR );
    }

    pdata->onwhichplatform_ref    = CHR_REF( MAX_CHR );
    pdata->onwhichplatform_update = 0;
    pdata->targetplatform_ref     = CHR_REF( MAX_CHR );
    pdata->attachedto             = CHR_REF( MAX_CHR );

    // all movements valid
    pdata->movement_bits   = ego_uint( ~0 );

    // not a player
    pdata->is_which_player = MAX_PLAYER;

    return pdata;
}

//--------------------------------------------------------------------------------------------
// struct ego_chr - memory management
//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::alloc( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    dealloc( pchr );

    /* add something here */

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::dealloc( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    /* add somethign here */

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_alloc( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    ego_chr::alloc( pchr );
    ego_chr_data::alloc( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_dealloc( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    ego_chr_data::dealloc( pchr );

    ego_chr::dealloc( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::ctor_this( ego_chr * pchr )
{
    /// \author BB
    /// \details  initialize the ego_chr

    if ( NULL == pchr ) return pchr;

    // set the critical values to safe ones
    pchr = ego_chr::init( pchr );
    if ( NULL == pchr ) return pchr;

    // construct/allocate any dynamic data
    pchr = ego_chr::alloc( pchr );
    if ( NULL == pchr ) return pchr;

    // start the character out in the "dance" animation
    ego_chr::start_anim( pchr, ACTION_DA, btrue, btrue );

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::dtor_this( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    // destruct/free any dynamic data
    ego_chr::dealloc( pchr );

    // do some list clean-up
    remove_all_character_enchants( GET_REF_PCHR( pchr ) );

    // set the critical values to safe ones
    pchr = ego_chr::init( pchr );
    if ( NULL == pchr ) return pchr;

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::update_instance( ego_chr * pchr )
{
    gfx_mad_instance * pgfx_inst;
    egoboo_rv retval;

    if ( !PROCESSING_PCHR( pchr ) ) return rv_error;
    pgfx_inst = &( pchr->gfx_inst );

    // make sure that the vertices are interpolated
    retval = gfx_mad_instance::update_vertices( pgfx_inst, pchr->mad_inst.state, gfx_range( -1, -1 ), btrue );
    if ( rv_error == retval )
    {
        return rv_error;
    }

    // do the basic lighting
    gfx_mad_instance::update_lighting_base( pgfx_inst, pchr, bfalse );

    return retval;
}

ego_chr_cap_data::ego_chr_cap_data() :
        // skins
        maxaccel_reset( 5 ),

        // overrides
        skin( 0 ),
        experience_level_reset( 0 ),
        state_stt( 0 ),
        content_stt( 0 ),

        // inventory
        ammo_max( 0 ),
        ammo( 0 ),
        money( 0 ),

        // character stats
        gender( GENDER_OTHER ),

        // life
        life_max( LOWSTAT ),
        life_return( 0 ),
        life_heal( 0 ),

        //// mana
        mana_max( LOWSTAT ),
        mana_return( 0 ),
        mana_flow( 0 ),

        strength( LOWSTAT ),
        wisdom( LOWSTAT ),
        intelligence( LOWSTAT ),
        dexterity( LOWSTAT ),

        //---- physics
        weight_stt( 255 ),
        dampen_stt( 0.1f ),
        bumpdampen_stt( 0.1f ),

        fat_stt( 1.0f ),
        shadow_size_stt( 32 ),
        stoppedby( MPDFX_WALL | MPDFX_IMPASS ),

        //---- movement
        jump_power( 5 ),
        jump_number_reset( 1 ),
        anim_speed_sneak( 1 ),
        anim_speed_walk( 2 ),
        anim_speed_run( 3 ),
        fly_height_reset( 0 ),
        waterwalk( bfalse ),

        //---- status graphics
        life_color( COLOR_RED ),
        mana_color( COLOR_BLUE ),
        draw_icon( bfalse ),

        //---- graphics
        flashand( 0 ),
        alpha_base( 255 ),
        light_base( 0 ),
        transferblend( bfalse ),
        uoffvel( 0.0f ),
        voffvel( 0.0f ),

        //---- defense
        defense( 0 ),

        //---- xp
        experience_reset( 0 ),

        //---- flags
        isitem( bfalse ),
        ismount( bfalse ),
        invictus( bfalse ),
        platform( bfalse ),
        canuseplatforms( btrue ),
        cangrabmoney( bfalse ),
        openstuff( bfalse ),
        nameknown( bfalse ),
        usageknown( bfalse ),
        ammoknown( bfalse ),
        damagetargettype( DAMAGE_CRUSH ),
        iskursed( bfalse ),

        //---- item usage
        manacost( 0 ),

        //---- special particle effects
        reaffirmdamagetype( -1 ),

        //---- skill system
        see_invisible_level( 0 ),

        // random stuff
        stickybutt( bfalse )
{
    // naming
    strncpy( name, "*UNKNOWN*", SDL_arraysize( name ) );          ///< My name

    // basic bumper size
    bump_stt.height   = 70;
    bump_stt.size     = 30;
    bump_stt.size_big = bump_stt.size * SQRT_TWO;

    // no defenses
    for ( int cnt = 0; cnt < DAMAGE_COUNT; cnt++ )
    {
        damagemodifier[cnt] = 0;
    }

    // no special sounds
    for ( int cnt = 0; cnt < SOUND_COUNT; cnt++ )
    {
        sound_index[cnt] = -1;
    }

    // no special skills
    for ( int cnt = 0; cnt < SOUND_COUNT; cnt++ )
    {
        skills[cnt].id    = IDSZ_NONE;
        skills[cnt].level = -1;
    }

};

//--------------------------------------------------------------------------------------------
bool_t ego_chr::download_cap( ego_chr * pchr, const ego_cap * pcap )
{
    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    bool_t rv = ego_chr_data::download_cap( pchr, pcap );

    if ( rv )
    {
        // this must be done after ego_chr_data::download_cap() so that the
        // more complicatd collision volumes will be updated
        ego_chr::update_collision_size( pchr, btrue );
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr_data::download_cap( ego_chr_data * pchr, const ego_cap * pcap )
{
    bool_t rv = ego_chr_cap_data::download_cap( pchr, pcap );

    if ( rv )
    {
        //// sound stuff...  copy from the cap
        //for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
        //{
        //    pchr->sound_index[tnc] = pcap->sound_index[tnc];
        //}

        //// Set up model stuff
        //pchr->stoppedby = pcap->stoppedby;
        //pchr->life_heal = pcap->life_heal;
        //pchr->manacost  = pcap->manacost;
        //pchr->nameknown = pcap->nameknown;
        //pchr->ammoknown = pcap->nameknown;
        //pchr->draw_icon = pcap->draw_icon;

        //// calculate a base kurse state. this may be overridden later
        //if ( pcap->isitem )
        //{
        //    IPair loc_rand = {1, 100};
        //    pchr->iskursed = ( generate_irand_pair( loc_rand ) <= pcap->kursechance );
        //}

        //// Skillz
        //idsz_map_copy( pcap->skills, SDL_arraysize( pcap->skills ), pchr->skills );
        pchr->darkvision_level = ego_chr_data::get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
        //pchr->see_invisible_level = pcap->see_invisible_level;

        //// Ammo
        //pchr->ammo_max = pcap->ammo_max;
        //pchr->ammo = pcap->ammo;

        //// Gender
        //pchr->gender = pcap->gender;
        //if ( pchr->gender == GENDER_RANDOM )  pchr->gender = generate_randmask( GENDER_FEMALE, GENDER_MALE );

        //// Life and Mana
        //pchr->life_color = pcap->life_color;
        //pchr->mana_color = pcap->mana_color;
        //pchr->life_max = generate_irand_range( pcap->life_stat.val );
        //pchr->life_return = pcap->life_return;
        //pchr->mana_max = generate_irand_range( pcap->mana_stat.val );
        //pchr->mana_flow = generate_irand_range( pcap->mana_flow_stat.val );
        //pchr->mana_return = generate_irand_range( pcap->mana_return_stat.val );

        //// SWID
        //pchr->strength = generate_irand_range( pcap->strength_stat.val );
        //pchr->wisdom = generate_irand_range( pcap->wisdom_stat.val );
        //pchr->intelligence = generate_irand_range( pcap->intelligence_stat.val );
        //pchr->dexterity = generate_irand_range( pcap->dexterity_stat.val );

        //// Skin
        //pchr->skin = 0;
        //if ( pcap->spelleffect_type != NO_SKIN_OVERRIDE )
        //{
        //    pchr->skin = pcap->spelleffect_type % MAX_SKIN;
        //}
        //else if ( pcap->skin_override != NO_SKIN_OVERRIDE )
        //{
        //    pchr->skin = pcap->skin_override % MAX_SKIN;
        //}

        //// Damage
        //pchr->defense = pcap->defense[pchr->skin];
        //pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;
        //pchr->damagetargettype = pcap->damagetargettype;
        //for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
        //{
        //    pchr->damagemodifier[tnc] = pcap->damagemodifier[tnc][pchr->skin];
        //}

        //// Flags
        //pchr->stickybutt      = pcap->stickybutt;
        //pchr->openstuff       = pcap->canopenstuff;
        //pchr->transferblend   = pcap->transferblend;
        //pchr->waterwalk       = pcap->waterwalk;
        //pchr->platform        = pcap->platform;
        //pchr->canuseplatforms = pcap->canuseplatforms;
        //pchr->isitem          = pcap->isitem;
        //pchr->invictus        = pcap->invictus;
        //pchr->ismount         = pcap->ismount;
        //pchr->cangrabmoney    = pcap->cangrabmoney;

        //// Jumping
        //pchr->jump_power = pcap->jump;
        ego_chr_data::set_jump_number_reset( pchr, pchr->jump_number_reset );

        //// Other junk
        ego_chr_data::set_fly_height( pchr, pchr->fly_height_reset );
        pchr->maxaccel    = pchr->maxaccel_reset;
        //pchr->alpha_base  = pcap->alpha;
        //pchr->light_base  = pcap->light;
        //pchr->flashand    = pcap->flashand;
        //pchr->phys.dampen = pcap->dampen;

        // Load current life and mana. this may be overridden later
        pchr->life = pchr->life_max;
        pchr->mana = pchr->mana_max;

        pchr->phys.bumpdampen = pcap->bumpdampen;
        if ( CAP_INFINITE_WEIGHT == pcap->weight )
        {
            pchr->phys.weight = INFINITE_WEIGHT;
        }
        else
        {
            Uint32 itmp = pcap->weight * pcap->size * pcap->size * pcap->size;
            pchr->phys.weight = SDL_min( itmp, MAX_WEIGHT );
        }

        //// Image rendering
        //pchr->uoffvel = pcap->uoffvel;
        //pchr->voffvel = pcap->voffvel;

        //// Movement
        //pchr->anim_speed_sneak = pcap->anim_speed_sneak;
        //pchr->anim_speed_walk = pcap->anim_speed_walk;
        //pchr->anim_speed_run = pcap->anim_speed_run;

        //// Money is added later
        //pchr->money = pcap->money;

        //// Experience
        //iTmp = generate_irand_range( pcap->experience );
        pchr->experience       = pchr->experience_reset;
        pchr->experience_level = pchr->experience_level_reset;

        //// Particle attachments
        //pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;

        // Character size and bumping
        ego_chr_data::init_size( pchr, pcap );
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr_cap_data::download_cap( ego_chr_cap_data * pchr, const ego_cap * pcap )
{
    /// \author BB
    /// \details  grab all of the data from the data.txt file

    int iTmp, tnc;

    if ( NULL == pchr ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    // sound stuff...  copy from the cap
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pchr->sound_index[tnc] = pcap->sound_index[tnc];
    }

    // Set up model stuff
    pchr->stoppedby = pcap->stoppedby;
    pchr->life_heal = pcap->life_heal;
    pchr->manacost  = pcap->manacost;
    pchr->nameknown = pcap->nameknown;
    pchr->ammoknown = pcap->nameknown;
    pchr->draw_icon = pcap->draw_icon;

    // calculate a base kurse state. this may be overridden later
    if ( pcap->isitem )
    {
        IPair loc_rand = {1, 100};
        pchr->iskursed = ( generate_irand_pair( loc_rand ) <= pcap->kursechance );
    }

    // Skillz
    idsz_map_copy( pcap->skills, SDL_arraysize( pcap->skills ), pchr->skills );
    //pchr->darkvision_level = ego_chr_data::get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
    pchr->see_invisible_level = pcap->see_invisible_level;

    // Ammo
    pchr->ammo_max = pcap->ammo_max;
    pchr->ammo = pcap->ammo;

    // Gender
    pchr->gender = pcap->gender;
    if ( pchr->gender == GENDER_RANDOM )  pchr->gender = generate_randmask( GENDER_FEMALE, GENDER_MALE );

    // Life and Mana
    pchr->life_color = pcap->life_color;
    pchr->mana_color = pcap->mana_color;
    pchr->life_max = generate_irand_range( pcap->life_stat.val );
    pchr->life_return = pcap->life_return;
    pchr->mana_max = generate_irand_range( pcap->mana_stat.val );
    pchr->mana_flow = generate_irand_range( pcap->mana_flow_stat.val );
    pchr->mana_return = generate_irand_range( pcap->mana_return_stat.val );

    // SWID
    pchr->strength = generate_irand_range( pcap->strength_stat.val );
    pchr->wisdom = generate_irand_range( pcap->wisdom_stat.val );
    pchr->intelligence = generate_irand_range( pcap->intelligence_stat.val );
    pchr->dexterity = generate_irand_range( pcap->dexterity_stat.val );

    // Skin
    pchr->skin = 0;
    if ( pcap->spelleffect_type != NO_SKIN_OVERRIDE )
    {
        pchr->skin = pcap->spelleffect_type % MAX_SKIN;
    }
    else if ( pcap->skin_override != NO_SKIN_OVERRIDE )
    {
        pchr->skin = pcap->skin_override % MAX_SKIN;
    }

    // Damage
    pchr->defense = pcap->defense[pchr->skin];
    pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;
    pchr->damagetargettype = pcap->damagetargettype;
    for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
    {
        pchr->damagemodifier[tnc] = pcap->damagemodifier[tnc][pchr->skin];
    }

    // Flags
    pchr->stickybutt      = pcap->stickybutt;
    pchr->openstuff       = pcap->canopenstuff;
    pchr->transferblend   = pcap->transferblend;
    pchr->waterwalk       = pcap->waterwalk;
    pchr->platform        = pcap->platform;
    pchr->canuseplatforms = pcap->canuseplatforms;
    pchr->isitem          = pcap->isitem;
    pchr->invictus        = pcap->invictus;
    pchr->ismount         = pcap->ismount;
    pchr->cangrabmoney    = pcap->cangrabmoney;

    // Jumping
    pchr->jump_power        = pcap->jump;
    pchr->jump_number_reset = pcap->jump_number;

    // Other junk
    pchr->fly_height_reset = pcap->fly_height;
    pchr->maxaccel_reset   = pcap->maxaccel[pchr->skin];
    pchr->alpha_base       = pcap->alpha;
    pchr->light_base       = pcap->light;
    pchr->flashand         = pcap->flashand;
    //pchr->phys.dampen    = pcap->dampen;

    // Load current life and mana. this may be overridden later
    pchr->life_max = CLIP( pcap->life_spawn, LOWSTAT, ( UFP8_T )SDL_max( 0, pchr->life_max ) );
    pchr->mana_max = CLIP( pcap->mana_spawn,       0, ( UFP8_T )SDL_max( 0, pchr->mana_max ) );

    //pchr->phys.bumpdampen = pcap->bumpdampen;
    //if ( CAP_INFINITE_WEIGHT == pcap->weight )
    //{
    //    pchr->phys.weight = INFINITE_WEIGHT;
    //}
    //else
    //{
    //    Uint32 itmp = pcap->weight * pcap->size * pcap->size * pcap->size;
    //    pchr->phys.weight = SDL_min( itmp, MAX_WEIGHT );
    //}

    // Image rendering
    pchr->uoffvel = pcap->uoffvel;
    pchr->voffvel = pcap->voffvel;

    // Movement
    pchr->anim_speed_sneak = pcap->anim_speed_sneak;
    pchr->anim_speed_walk = pcap->anim_speed_walk;
    pchr->anim_speed_run = pcap->anim_speed_run;

    // Money is added later
    pchr->money = pcap->money;

    // Experience
    iTmp = generate_irand_range( pcap->experience );
    pchr->experience_reset = SDL_min( iTmp, MAX_XP );
    pchr->experience_level_reset = pcap->level_override;

    // Particle attachments
    pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::upload_cap( const ego_chr * pchr, ego_cap * pcap )
{
    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    /* add something here */

    bool_t rv = ego_chr_data::upload_cap( pchr, pcap );

    return rv;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr_data::upload_cap( const ego_chr_data * pchr, ego_cap * pcap )
{
    /// \author BB
    /// \details  prepare a character profile for exporting, by uploading some special values into the
    ///     cap. Just so that there is no confusion when you export multiple items of the same type,
    ///     DO NOT pass the pointer returned by ego_chr::get_pcap(). Instead, use a custom ego_cap declared on the stack,
    ///     or something similar
    ///
    /// \note This has been modified to basically reverse the actions of ego_chr::download_cap().
    ///       If all enchants have been removed, this should export all permanent changes to the
    ///       base character profile.

    if ( NULL == pchr ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    // export values that override spawn.txt values
    pcap->content_override   = pchr->ai.content;
    pcap->state_override     = pchr->ai.state;
    //pcap->money              = pchr->money;
    //pcap->skin_override      = pchr->skin;
    pcap->level_override     = pchr->experience_level;

    //// export the current experience
    ints_to_range( pchr->experience, 0, &( pcap->experience ) );

    // export the current mana and life
    pcap->life_spawn         = CLIP( pchr->life, 0, pchr->life_max );
    pcap->mana_spawn         = CLIP( pchr->mana, 0, pchr->mana_max );

    //// Movement
    //pcap->anim_speed_sneak = pchr->anim_speed_sneak;
    //pcap->anim_speed_walk = pchr->anim_speed_walk;
    //pcap->anim_speed_run = pchr->anim_speed_run;

    //// weight and size
    pcap->size       = pchr->fat_goto;
    pcap->bumpdampen = pchr->phys.bumpdampen;
    if ( pchr->phys.weight == INFINITE_WEIGHT )
    {
        pcap->weight = CAP_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pchr->phys.weight / pchr->fat / pchr->fat / pchr->fat;
        pcap->weight = SDL_min( itmp, CAP_MAX_WEIGHT );
    }

    //// Other junk
    pcap->fly_height  = pchr->fly_height;
    //pcap->alpha       = pchr->alpha_base;
    //pcap->light       = pchr->light_base;
    //pcap->flashand    = pchr->flashand;
    pcap->dampen      = pchr->phys.dampen;

    //// Jumping
    //pcap->jump       = pchr->jump_power;
    //pcap->jump_number = pchr->jump_number_reset;

    //// Flags
    //pcap->stickybutt      = pchr->stickybutt;
    //pcap->canopenstuff    = pchr->openstuff;
    //pcap->transferblend   = pchr->transferblend;
    //pcap->waterwalk       = pchr->waterwalk;
    //pcap->platform        = pchr->platform;
    //pcap->canuseplatforms = pchr->canuseplatforms;
    //pcap->isitem          = pchr->isitem;
    //pcap->invictus        = pchr->invictus;
    //pcap->ismount         = pchr->ismount;
    //pcap->cangrabmoney    = pchr->cangrabmoney;

    //// Damage
    //pcap->attachedprt_reaffirmdamagetype = pchr->reaffirmdamagetype;
    //pcap->damagetargettype               = pchr->damagetargettype;

    //// SWID
    //ints_to_range( pchr->strength    , 0, &( pcap->strength_stat.val ) );
    //ints_to_range( pchr->wisdom      , 0, &( pcap->wisdom_stat.val ) );
    //ints_to_range( pchr->intelligence, 0, &( pcap->intelligence_stat.val ) );
    //ints_to_range( pchr->dexterity   , 0, &( pcap->dexterity_stat.val ) );

    //// Life and Mana
    //pcap->life_color = pchr->life_color;
    //pcap->mana_color = pchr->mana_color;
    //ints_to_range( pchr->life_max     , 0, &( pcap->life_stat.val ) );
    //ints_to_range( pchr->mana_max     , 0, &( pcap->mana_stat.val ) );
    //ints_to_range( pchr->mana_return  , 0, &( pcap->mana_return_stat.val ) );
    //ints_to_range( pchr->mana_flow    , 0, &( pcap->mana_flow_stat.val ) );

    //// Gender
    //pcap->gender  = pchr->gender;

    //// Ammo
    //pcap->ammo_max = pchr->ammo_max;
    //pcap->ammo    = pchr->ammo;

    //// update any skills that have been learned
    //idsz_map_copy( pchr->skills, SDL_arraysize( pchr->skills ), pcap->skills );

    //// Enchant stuff
    //pcap->see_invisible_level = pchr->see_invisible_level;

    //// base kurse state
    //pcap->kursechance = pchr->iskursed ? 100 : 0;

    //// Model stuff
    //pcap->stoppedby = pchr->stoppedby;
    //pcap->life_heal = pchr->life_heal;
    //pcap->manacost  = pchr->manacost;
    //pcap->nameknown = pchr->nameknown || pchr->ammoknown;          // make sure that identified items are saved as identified
    //pcap->draw_icon = pchr->draw_icon;

    //// sound stuff...
    //for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    //{
    //    pcap->sound_index[tnc] = pchr->sound_index[tnc];
    //}

    return ego_chr_cap_data::upload_cap( pchr, pcap );
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr_cap_data::upload_cap( const ego_chr_cap_data * pchr, ego_cap * pcap )
{
    /// \author BB
    /// \details  prepare a character profile for exporting, by uploading some special values into the
    ///     cap. Just so that there is no confusion when you export multiple items of the same type,
    ///     DO NOT pass the pointer returned by ego_chr::get_pcap(). Instead, use a custom ego_cap declared on the stack,
    ///     or something similar
    ///
    /// \note This has been modified to basically reverse the actions of ego_chr::download_cap().
    ///       If all enchants have been removed, this should export all permanent changes to the
    ///       base character profile.

    int tnc;

    if ( NULL == pchr ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    // export values that override spawn.txt values
    //pcap->content_override   = pchr->ai.content;
    //pcap->state_override     = pchr->ai.state;
    pcap->money              = pchr->money;
    pcap->skin_override      = pchr->skin;
    //pcap->level_override     = pchr->experience_level;

    // export the current experience
    //ints_to_range( pchr->experience, 0, &( pcap->experience ) );

    // export the current mana and life
    //pcap->life_spawn         = CLIP( pchr->life, 0, pchr->life_max );
    //pcap->mana_spawn         = CLIP( pchr->mana, 0, pchr->mana_max );

    // Movement
    pcap->anim_speed_sneak = pchr->anim_speed_sneak;
    pcap->anim_speed_walk = pchr->anim_speed_walk;
    pcap->anim_speed_run = pchr->anim_speed_run;

    // weight and size
    //pcap->size       = pchr->fat_goto;
    //pcap->bumpdampen = pchr->phys.bumpdampen;
    //if ( pchr->phys.weight == INFINITE_WEIGHT )
    //{
    //    pcap->weight = CAP_INFINITE_WEIGHT;
    //}
    //else
    //{
    //    Uint32 itmp = pchr->phys.weight / pchr->fat / pchr->fat / pchr->fat;
    //    pcap->weight = SDL_min( itmp, CAP_MAX_WEIGHT );
    //}

    // Other junk
    //pcap->fly_height  = pchr->fly_height;
    pcap->alpha       = pchr->alpha_base;
    pcap->light       = pchr->light_base;
    pcap->flashand    = pchr->flashand;
    //pcap->dampen      = pchr->phys.dampen;

    // Jumping
    pcap->jump       = pchr->jump_power;
    pcap->jump_number = pchr->jump_number_reset;

    // Flags
    pcap->stickybutt      = pchr->stickybutt;
    pcap->canopenstuff    = pchr->openstuff;
    pcap->transferblend   = pchr->transferblend;
    pcap->waterwalk       = pchr->waterwalk;
    pcap->platform        = pchr->platform;
    pcap->canuseplatforms = pchr->canuseplatforms;
    pcap->isitem          = pchr->isitem;
    pcap->invictus        = pchr->invictus;
    pcap->ismount         = pchr->ismount;
    pcap->cangrabmoney    = pchr->cangrabmoney;

    // Damage
    pcap->attachedprt_reaffirmdamagetype = pchr->reaffirmdamagetype;
    pcap->damagetargettype               = pchr->damagetargettype;

    // SWID
    ints_to_range( pchr->strength    , 0, &( pcap->strength_stat.val ) );
    ints_to_range( pchr->wisdom      , 0, &( pcap->wisdom_stat.val ) );
    ints_to_range( pchr->intelligence, 0, &( pcap->intelligence_stat.val ) );
    ints_to_range( pchr->dexterity   , 0, &( pcap->dexterity_stat.val ) );

    // Life and Mana
    pcap->life_color = pchr->life_color;
    pcap->mana_color = pchr->mana_color;
    ints_to_range( pchr->life_max     , 0, &( pcap->life_stat.val ) );
    ints_to_range( pchr->mana_max     , 0, &( pcap->mana_stat.val ) );
    ints_to_range( pchr->mana_return  , 0, &( pcap->mana_return_stat.val ) );
    ints_to_range( pchr->mana_flow    , 0, &( pcap->mana_flow_stat.val ) );

    // Gender
    pcap->gender  = pchr->gender;

    // Ammo
    pcap->ammo_max = pchr->ammo_max;
    pcap->ammo    = pchr->ammo;

    // update any skills that have been learned
    idsz_map_copy( pchr->skills, SDL_arraysize( pchr->skills ), pcap->skills );

    // Enchant stuff
    pcap->see_invisible_level = pchr->see_invisible_level;

    // base kurse state
    pcap->kursechance = pchr->iskursed ? 100 : 0;

    // Model stuff
    pcap->stoppedby = pchr->stoppedby;
    pcap->life_heal = pchr->life_heal;
    pcap->manacost  = pchr->manacost;
    pcap->nameknown = pchr->nameknown || pchr->ammoknown;          // make sure that identified items are saved as identified
    pcap->draw_icon = pchr->draw_icon;

    // sound stuff...
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        pcap->sound_index[tnc] = pchr->sound_index[tnc];
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_chr *  ego_chr::do_constructing( ego_chr * pchr )
{
    // this object has already been constructed as a part of the
    // ego_obj_chr, so its parent is properly defined
    ego_obj_chr * old_parent = ego_chr::get_obj_ptr( pchr );

    // reconstruct the character data with the old parent
    pchr = ego_chr::ctor_all( pchr, old_parent );

    /* add something here */

    // call the parent's virtual function
    get_obj_ptr( pchr )->ego_obj::do_constructing();

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr *  ego_chr::do_initializing( ego_chr * pchr )
{
    CHR_REF  ichr;
    CAP_REF  icap;
    TEAM_REF loc_team;
    int      tnc, iteam, kursechance;

    ego_cap * pcap;
    fvec3_t pos_tmp;

    if ( NULL == pchr ) return NULL;
    ichr = GET_IDX_PCHR( pchr );

    // get the character profile pointer
    pcap = pro_get_pcap( pchr->spawn_data.profile );
    if ( NULL == pcap )
    {
        log_debug( "%s - cannot initialize character.\n", __FUNCTION__ );

        return NULL;
    }

    // get the character profile index
    icap = pro_get_icap( pchr->spawn_data.profile );

    // make a copy of the data in pchr->spawn_data.pos
    pos_tmp = pchr->spawn_data.pos;

    // download all the values from the character pchr->spawn_data.profile
    ego_chr::download_cap( pchr, pcap );

    // Make sure the pchr->spawn_data.team is valid
    loc_team = pchr->spawn_data.team;
    iteam = loc_team.get_value();
    iteam = CLIP( iteam, 0, TEAM_MAX );
    loc_team = TEAM_REF( iteam );

    // IMPORTANT!!!
    pchr->missilehandler = ichr;

    // Set up model stuff
    pchr->profile_ref   = pchr->spawn_data.profile;
    pchr->basemodel_ref = pchr->spawn_data.profile;

    // Kurse state
    if ( pcap->isitem )
    {
        IPair loc_rand = {1, 100};

        kursechance = pcap->kursechance;
        if ( cfg.difficulty >= GAME_HARD )                        kursechance *= 2.0f;  // Hard mode doubles chance for Kurses
        if ( cfg.difficulty < GAME_NORMAL && kursechance != 100 ) kursechance *= 0.5f;  // Easy mode halves chance for Kurses
        pchr->iskursed = ( generate_irand_pair( loc_rand ) <= kursechance );
    }

    // AI stuff
    ego_ai_state::spawn( &( pchr->ai ), ichr, pchr->profile_ref, TeamStack[loc_team].morale );

    // Team stuff
    pchr->team     = loc_team;
    pchr->baseteam = loc_team;
    if ( !IS_INVICTUS_PCHR_RAW( pchr ) )  TeamStack[loc_team].morale++;

    // Firstborn becomes the leader
    if ( TeamStack[loc_team].leader == NOLEADER )
    {
        TeamStack[loc_team].leader = ichr;
    }

    // Skin
    if ( pcap->skin_override != NO_SKIN_OVERRIDE )
    {
        // override the value passed into the function from spawn.txt
        // with the calue from the expansion in data.txt
        pchr->spawn_data.skin = pchr->skin;
    }
    if ( pchr->spawn_data.skin >= ProList.lst[pchr->spawn_data.profile].skins )
    {
        // place this here so that the random number generator advances
        // no matter the state of ProList.lst[pchr->spawn_data.profile].skins... Eases
        // possible synch problems with other systems?
        int irand = RANDIE;

        pchr->spawn_data.skin = 0;
        if ( 0 != ProList.lst[pchr->spawn_data.profile].skins )
        {
            pchr->spawn_data.skin = irand % ProList.lst[pchr->spawn_data.profile].skins;
        }
    }
    pchr->skin = pchr->spawn_data.skin;

    // fix the pchr->spawn_data.skin-related parameters, in case there was some funy business with overriding
    // the pchr->spawn_data.skin from the data.txt file
    if ( pchr->spawn_data.skin != pchr->skin )
    {
        pchr->skin = pchr->spawn_data.skin;

        pchr->defense = pcap->defense[pchr->skin];
        for ( tnc = 0; tnc < DAMAGE_COUNT; tnc++ )
        {
            pchr->damagemodifier[tnc] = pcap->damagemodifier[tnc][pchr->skin];
        }

        ego_chr::set_maxaccel( pchr, pcap->maxaccel[pchr->skin] );
    }

    // override the default behavior for an "easy" game
    if ( cfg.difficulty < GAME_NORMAL )
    {
        pchr->life = pchr->life_max;
        pchr->mana = pchr->mana_max;
    }

    // Character size and bumping
    pchr->fat_goto      = pchr->fat;
    pchr->fat_goto_time = 0;

    // grab all of the environment information
    chr_calc_environment( pchr );

    ego_chr::set_pos( pchr, pos_tmp.v );

    pchr->pos_stt  = pos_tmp;
    pchr->pos_old  = pos_tmp;

    pchr->ori.facing_z     = pchr->spawn_data.facing;
    pchr->ori_old.facing_z = pchr->ori.facing_z;

    // Name the character
    if ( CSTR_END == pchr->spawn_data.name[0] )
    {
        // Generate a random pchr->spawn_data.name
        SDL_snprintf( pchr->name, SDL_arraysize( pchr->name ), "%s", pro_create_chop( pchr->spawn_data.profile ) );
    }
    else
    {
        // A pchr->spawn_data.name has been given
        tnc = 0;

        while ( tnc < MAXCAPNAMESIZE - 1 )
        {
            pchr->name[tnc] = pchr->spawn_data.name[tnc];
            tnc++;
        }

        pchr->name[tnc] = CSTR_END;
    }

    // Particle attachments
    for ( tnc = 0; tnc < pcap->attachedprt_amount; tnc++ )
    {
        spawn_one_particle( pchr->pos, 0, pchr->profile_ref, pcap->attachedprt_pip,
                            ichr, GRIP_LAST + tnc, pchr->team, ichr, PRT_REF( MAX_PRT ), tnc, CHR_REF( MAX_CHR ) );
    }

    // is the object part of a shop's inventory?
    if ( pchr->isitem )
    {
        SHOP_REF ishop;

        // Items that are spawned inside shop passages are more expensive than normal
        pchr->isshopitem = bfalse;
        for ( ishop = 0; ishop < ShopStack.count; ishop++ )
        {
            // Make sure the owner is not dead
            if ( SHOP_NOOWNER == ShopStack[ishop].owner ) continue;

            if ( PassageStack.object_is_inside( ShopStack[ishop].passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
            {
                pchr->isshopitem = btrue;               // Full value
                pchr->iskursed   = bfalse;              // Shop items are never kursed
                pchr->nameknown  = btrue;
                break;
            }
        }
    }

    // override the shopitem flag if the item is known to be valuable
    if ( pcap->isvaluable )
    {
        pchr->isshopitem = btrue;
    }

    // initialize the character instance
    gfx_mad_instance::spawn( &( pchr->gfx_inst ), pchr->spawn_data.profile, pchr->spawn_data.skin );

    // force the calculation of the matrix
    ego_chr::update_matrix( pchr, btrue );

    // force the instance to update
    gfx_mad_instance::update_ref( &( pchr->gfx_inst ), pchr->enviro.grid_level, btrue );

    // determine whether the object is hidden
    ego_chr::update_hide( pchr );

#if EGO_DEBUG && defined(DEBUG_WAYPOINTS)
    if ( !IS_ATTACHED_PCHR( pchr ) && INFINITE_WEIGHT != pchr->phys.weight && !pchr->safe_valid )
    {
        log_warning( "%s - \n\tinitial spawn position <%f,%f> is \"inside\" a wall. Wall normal is <%f,%f>\n",
                     __FUNCTION__ ,
                     pchr->pos.x, pchr->pos.y, nrm.x, nrm.y );
    }
#endif

    // call the parent's virtual function
    get_obj_ptr( pchr )->ego_obj::do_initializing();

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_processing( ego_chr * pchr )
{
    ego_cap * pcap;
    int     ripand;
    CHR_REF ichr;

    if ( NULL == pchr ) return pchr;
    ichr = GET_REF_PCHR( pchr );

    // then do status updates
    ego_chr::update_hide( pchr );

    // Don't do items that are in inventory
    if ( pchr->pack.is_packed ) return pchr;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return pchr;

    // do the character interaction with water
    if ( !pchr->is_hidden && pchr->pos.z < water.surface_level && ( 0 != ego_mpd::test_fx( PMesh, pchr->onwhichgrid, MPDFX_WATER ) ) )
    {
        // do splash and ripple
        if ( !pchr->enviro.inwater )
        {
            // Splash
            fvec3_t vtmp = VECT3( pchr->pos.x, pchr->pos.y, water.surface_level + RAISE );

            spawn_one_particle_global( vtmp, ATK_FRONT, PIP_SPLASH, 0 );

            if ( water.is_water )
            {
                ADD_BITS( pchr->ai.alert, ALERTIF_INWATER );
            }
        }
        else
        {
            // Ripples
            if ( !pchr->pack.is_packed && pcap->ripple && pchr->pos.z + pchr->chr_min_cv.maxs[OCT_Z] + RIPPLETOLERANCE > water.surface_level && pchr->pos.z + pchr->chr_min_cv.mins[OCT_Z] < water.surface_level )
            {
                int ripple_suppression;

                // suppress ripples if we are far below the surface
                ripple_suppression = water.surface_level - ( pchr->pos.z + pchr->chr_min_cv.maxs[OCT_Z] );
                ripple_suppression = ( 4 * ripple_suppression ) / RIPPLETOLERANCE;
                ripple_suppression = CLIP( ripple_suppression, 0, 4 );

                // make more ripples if we are moving
                ripple_suppression -= ( int( pchr->vel.x ) != 0 ) | ( int( pchr->vel.y ) != 0 );

                if ( ripple_suppression > 0 )
                {
                    ripand = ~(( ~RIPPLEAND ) << ripple_suppression );
                }
                else
                {
                    ripand = RIPPLEAND >> ( -ripple_suppression );
                }

                if ( 0 == (( update_wld + ego_chr::get_obj_ref( *pchr ).get_id() ) & ripand ) && pchr->pos.z < water.surface_level && pchr->alive )
                {
                    fvec3_t   vtmp = VECT3( pchr->pos.x, pchr->pos.y, water.surface_level );

                    spawn_one_particle_global( vtmp, ATK_FRONT, PIP_RIPPLE, 0 );
                }
            }

            if ( water.is_water && HAS_NO_BITS( update_wld, 7 ) )
            {
                pchr->jump_ready = btrue;
                pchr->jump_number = 1;
            }
        }

        pchr->enviro.inwater  = btrue;
    }
    else
    {
        pchr->enviro.inwater = bfalse;
    }

    // the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pchr;

    //---- Do timers and such

    // decrement the dismount timer
    if ( pchr->dismount_timer > 0 )
    {
        pchr->dismount_timer--;

        if ( 0 == pchr->dismount_timer )
        {
            pchr->dismount_object = CHR_REF( MAX_CHR );
        }
    }

    // reduce attack cooldowns
    if ( pchr->reloadtime > 0 )
    {
        pchr->reloadtime--;
    }

    // Down that ol' damage timer
    if ( pchr->damagetime > 0 )
    {
        pchr->damagetime--;
    }

    // Do "Be careful!" delay
    if ( pchr->carefultime > 0 )
    {
        pchr->carefultime--;
    }

    // Down jump timer
    if ( pchr->jump_time > 0 && ( INGAME_CHR( pchr->attachedto ) || pchr->jump_ready || pchr->jump_number > 0 ) )
    {
        pchr->jump_time--;
    }

    //---- Texture movement
    pchr->gfx_inst.uoffset += pchr->uoffvel;
    pchr->gfx_inst.voffset += pchr->voffvel;

    //---- Do stats once every second
    if ( clock_chr_stat >= ONESECOND )
    {
        // check for a level up
        do_level_up( ichr );

        // do the mana and life regen for "living" characters
        if ( pchr->alive )
        {
            int manaregen = 0;
            int liferegen = 0;
            get_chr_regeneration( pchr, &liferegen, &manaregen );

            pchr->mana += manaregen;
            pchr->mana = SDL_max( 0, SDL_min( pchr->mana, pchr->mana_max ) );

            pchr->life += liferegen;
            pchr->life = SDL_max( 1, SDL_min( pchr->life, pchr->life_max ) );
        }

        // countdown confuse effects
        if ( pchr->grogtime > 0 )
        {
            pchr->grogtime--;
        }

        if ( pchr->dazetime > 0 )
        {
            pchr->dazetime--;
        }

        // possibly gain/lose darkvision
        update_chr_darkvision( ichr );
    }

    pchr = resize_one_character( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_deinitializing( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    /* nothing to do yet */

    // call the parent's virtual function
    get_obj_ptr( pchr )->ego_obj::do_deinitializing();

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_destructing( ego_chr * pchr )
{
    pchr = ego_chr::dtor_all( pchr );

    /* add something here */

    // call the parent's virtual function
    get_obj_ptr( pchr )->ego_obj::do_destructing();

    return pchr;
}

//-------------------------------------------------------------------------------------------
// ego_chr - specialization (if any) of the i_ego_obj interface
//--------------------------------------------------------------------------------------------
bool_t ego_obj_chr::object_allocated( void )
{
    if ( NULL == this || NULL == _container_ptr ) return bfalse;

    return FLAG_ALLOCATED_PCONT( container_type,  _container_ptr );
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_chr::object_update_list_id( void )
{
    if ( NULL == this ) return bfalse;

    // get a non-const version of the container pointer
    container_type * pcont = get_container_ptr( this );
    if ( NULL == pcont ) return bfalse;

    if ( !FLAG_ALLOCATED_PCONT( container_type,  pcont ) ) return bfalse;

    // deal with the return state
    if ( !get_valid( this ) )
    {
        container_type::set_list_id( pcont, INVALID_UPDATE_GUID );
    }
    else if ( ego_obj_processing == get_action( this ) )
    {
        container_type::set_list_id( pcont, ChrObjList.get_list_id() );
    }

    return INVALID_UPDATE_GUID != container_type::get_list_id( pcont );
}

//--------------------------------------------------------------------------------------------
// struct ego_obj_chr -
//--------------------------------------------------------------------------------------------

bool_t ego_obj_chr::request_terminate( const CHR_REF & ichr )
{
    /// \author BB
    /// \details  Tell the game to get rid of this object and treat it
    ///               as if it was already dead
    ///
    /// \note ego_obj_chr::request_terminate() will force the game to
    ///       (eventually) call free_one_character_in_game() on this character

    return request_terminate( ChrObjList.get_allocated_data_ptr( ichr ) );
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_chr::request_terminate( ego_obj_chr * pobj )
{
    /// \author BB
    /// \details  Tell the game to get rid of this object and treat it
    ///               as if it was already dead
    ///
    /// \note ego_obj_chr::request_terminate() will force the game to
    ///       (eventually) call free_one_character_in_game() on this character

    if ( NULL == pobj || TERMINATED_PCHR( pobj ) ) return bfalse;

    // wait for ChrObjList.cleanup() to work its magic
    pobj->begin_waiting( );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_obj_chr::request_terminate( ego_bundle_chr & bdl )
{
    bool_t retval;

    retval = ego_obj_chr::request_terminate( bdl.chr_ref() );

    if ( retval ) bdl.validate();

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::set_frame( ego_chr * pchr, const int frame, const int ilip )
{
    /// \author ZZ
    /// \details  This function sets the frame for a character explicitly.
    /// This is used to rotate Tank turrets

    if ( NULL == pchr ) return rv_error;

    if ( rv_success != mad_instance::set_frame( &( pchr->mad_inst ), frame, ilip ) )
    {
        return rv_fail;
    }

    chr_invalidate_instances( pchr );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::set_anim( ego_chr * pchr, const int src_action, const int frame, const int ilip )
{
    /// \author ZZ
    /// \details  This function sets a frame and an action explicitly

    egoboo_rv retval = rv_fail;
    ego_mad * pmad;

    mad_instance * pmad_inst;
    gfx_mad_instance * pgfx_inst;

    if ( !INGAME_PCHR( pchr ) ) return rv_error;
    pmad_inst  = &( pchr->mad_inst );
    pgfx_inst  = &( pchr->gfx_inst );

    if ( LOADED_MAD( pmad_inst->imad ) ) return rv_error;
    pmad = MadStack + pmad_inst->imad;

    // grab the correct action
    int dst_action = mad_get_action( pmad_inst->imad, src_action );

    // set the action and frame
    retval = rv_fail;
    if ( rv_success == ego_chr::set_action( pchr, dst_action, btrue, btrue ) )
    {
        retval = ego_chr::set_frame( pchr, frame, ilip );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int get_grip_verts( Uint16 grip_verts[], const CHR_REF & imount, const int vrt_offset )
{
    /// \author BB
    /// \details  Fill the grip_verts[] array from the mount's data.
    ///     Return the number of vertices found.

    Uint32  i;
    int vrt_count, tnc;

    ego_chr * pmount;
    ego_mad * pmount_mad;

    if ( NULL == grip_verts ) return 0;

    // set all the vertices to a "safe" value
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        grip_verts[i] = 0xFFFF;
    }

    pmount = ChrObjList.get_allocated_data_ptr( imount );
    if ( !INGAME_PCHR( pmount ) ) return 0;

    pmount_mad = ego_chr::get_pmad( imount );
    if ( NULL == pmount_mad ) return 0;

    if ( 0 == pmount->gfx_inst.vrt_count ) return 0;

    //---- set the proper weapongrip vertices
    tnc = ego_sint( pmount->gfx_inst.vrt_count ) - ego_sint( vrt_offset );

    // if the starting vertex is less than 0, just take the first vertex
    if ( tnc < 0 )
    {
        grip_verts[0] = 0;
        return 1;
    }

    vrt_count = 0;
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        if ( tnc + i < pmount->gfx_inst.vrt_count )
        {
            grip_verts[i] = tnc + i;
            vrt_count++;
        }
        else
        {
            grip_verts[i] = 0xFFFF;
        }
    }

    return vrt_count;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_local_points( ego_chr * pholder, const Uint16 grip_verts[], fvec4_t dst_point[] )
{
    /// \author ZZ
    /// \details  a helper function for gfx_mad_matrix_data::generate_weapon_matrix()

    int cnt, point_count;

    if ( NULL == grip_verts || NULL == dst_point ) return 0;

    if ( !PROCESSING_PCHR( pholder ) ) return 0;

    // count the valid weapon connection dst_points
    point_count = 0;
    for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
    {
        if ( 0xFFFF != grip_verts[cnt] )
        {
            point_count++;
        }
    }

    // do the best we can
    if ( 0 == point_count )
    {
        // punt! attach to origin
        dst_point[0].x = pholder->pos.x;
        dst_point[0].y = pholder->pos.y;
        dst_point[0].z = pholder->pos.z;
        dst_point[0].w = 1;

        point_count = 1;
    }
    else
    {
        // update the grip vertices
        gfx_mad_instance::update_grip_verts( &( pholder->gfx_inst ), grip_verts, GRIP_VERTS );

        // copy the vertices into dst_point[]
        for ( point_count = 0, cnt = 0; cnt < GRIP_VERTS; cnt++, point_count++ )
        {
            Uint16 vertex = grip_verts[cnt];

            if ( 0xFFFF == vertex ) continue;

            dst_point[point_count].x = pholder->gfx_inst.vrt_lst[vertex].pos[XX];
            dst_point[point_count].y = pholder->gfx_inst.vrt_lst[vertex].pos[YY];
            dst_point[point_count].z = pholder->gfx_inst.vrt_lst[vertex].pos[ZZ];
            dst_point[point_count].w = 1.0f;
        }
    }

    return point_count;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_global_points( const CHR_REF & iholder, const Uint16 grip_verts[], fvec4_t dst_point[] )
{
    /// \author ZZ
    /// \details  a helper function for gfx_mad_matrix_data::generate_weapon_matrix()

    ego_chr *   pholder;
    int       point_count;
    fvec4_t   src_point[GRIP_VERTS];

    pholder = ChrObjList.get_allocated_data_ptr( iholder );
    if ( !INGAME_PCHR( pholder ) ) return 0;

    // find the grip points in the character's "local" or "body-fixed" coordinates
    point_count = convert_grip_to_local_points( pholder, grip_verts, src_point );

    if ( 0 == point_count ) return 0;

    // use the math function instead of rolling out own
    TransformVertices( &( pholder->gfx_inst.matrix ), src_point, dst_point, point_count );

    return point_count;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_obj_chr * ego_obj_chr::alloc( ego_obj_chr * pobj )
{
    if ( NULL == pobj ) return pobj;

    dealloc( pobj );

    // initialize the bsp node for this character
    ego_BSP_leaf::init( &( pobj->bsp_leaf ), 3, pobj, 1 );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_chr * ego_obj_chr::dealloc( ego_obj_chr * pobj )
{
    if ( NULL == pobj ) return pobj;

    // initialize the bsp node for this character
    ego_BSP_leaf::dealloc( &( pobj->bsp_leaf ) );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_chr * ego_obj_chr::ctor_this( ego_obj_chr * pobj )
{
    /// \author BB
    /// \details  initialize the ego_obj_chr

    if ( NULL == pobj ) return pobj;

    // construct/allocate any dynamic data
    pobj = ego_obj_chr::alloc( pobj );
    if ( NULL == pobj ) return pobj;

    // set the ego_BSP_leaf data
    pobj->bsp_leaf.data      = pobj;
    pobj->bsp_leaf.data_type = LEAF_CHR;
    pobj->bsp_leaf.index     = GET_IDX_PCHR( pobj );

    return pobj;
}

//--------------------------------------------------------------------------------------------
ego_obj_chr * ego_obj_chr::dtor_this( ego_obj_chr * pobj )
{
    if ( NULL == pobj ) return pobj;

    // destruct/free any dynamic data
    pobj = ego_obj_chr::dealloc( pobj );
    if ( NULL == pobj ) return NULL;

    /* add something here */

    return pobj;
}

//--------------------------------------------------------------------------------------------
void ego_obj_chr::update_max_cv()
{
    /// \author BB
    ///
    /// \details use this function to update the ego_obj::max_cv with
    ///       the largest possible collision volume

    update_collision_size( this, btrue );

    ego_oct_bb::add_vector( chr_max_cv, ego_chr::get_pos_v( this ), max_cv );
}

//--------------------------------------------------------------------------------------------
void ego_obj_chr::update_bsp()
{
    ego_oct_bb   tmp_oct;

    // re-initialize the data.
    bsp_leaf.data      = static_cast<ego_chr*>( this );
    bsp_leaf.index     = GET_IDX_PCHR( this );
    bsp_leaf.data_type = LEAF_CHR;
    bsp_leaf.inserted  = bfalse;

    update_max_cv();

    // use the object velocity to figure out where the volume that the object will occupy during this
    // update
    phys_expand_chr_bb( this, 0.0f, 1.0f, tmp_oct );

    // convert the bounding box
    ego_BSP_aabb::from_oct_bb( bsp_leaf.bbox, tmp_oct );
}
