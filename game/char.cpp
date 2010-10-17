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

/// @file char.c
/// @brief Implementation of character functions
/// @details

#include "char.inl"
#include "ChrList.h"

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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_STACK( ACCESS_TYPE_NONE, ego_cap, CapStack, MAX_PROFILE );
INSTANTIATE_STACK( ACCESS_TYPE_NONE, ego_team, TeamStack, TEAM_MAX );

int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static ego_chr_instance * chr_instance_ctor( ego_chr_instance * pinst );
static ego_chr_instance * chr_instance_dtor( ego_chr_instance * pinst );
static bool_t           chr_instance_alloc( ego_chr_instance * pinst, size_t vlst_size );
static bool_t           chr_instance_free( ego_chr_instance * pinst );
static bool_t           chr_spawn_instance( ego_chr_instance * pinst, const PRO_REF by_reference profile, Uint8 skin );
static bool_t           chr_instance_set_mad( ego_chr_instance * pinst, const MAD_REF by_reference imad );

static CHR_REF chr_pack_has_a_stack( const CHR_REF by_reference item, const CHR_REF by_reference character );
static bool_t  chr_pack_add_item( const CHR_REF by_reference item, const CHR_REF by_reference character );
static bool_t  chr_pack_remove_item( CHR_REF ichr, CHR_REF iparent, CHR_REF iitem );
static CHR_REF chr_pack_get_item( const CHR_REF by_reference chr_ref, grip_offset_t grip_off, bool_t ignorekurse );

static bool_t set_weapongrip( const CHR_REF by_reference iitem, const CHR_REF by_reference iholder, Uint16 vrt_off );

static BBOARD_REF chr_add_billboard( const CHR_REF by_reference ichr, Uint32 lifetime_secs );

static ego_chr * resize_one_character( ego_chr * pchr );

static int get_grip_verts( Uint16 grip_verts[], const CHR_REF by_reference imount, int vrt_offset );

bool_t apply_one_character_matrix( ego_chr * pchr, ego_matrix_cache * mcache );
bool_t apply_one_weapon_matrix( ego_chr * pweap, ego_matrix_cache * mcache );

// definition that is consistent with using it as a callback in qsort() or some similar function
static int  cmp_matrix_cache( const void * vlhs, const void * vrhs );

static bool_t chr_upload_cap( ego_chr * pchr, ego_cap * pcap );

void cleanup_one_character( ego_chr * pchr );

static bool_t chr_instance_update_ref( ego_chr_instance * pinst, float grid_level, bool_t need_matrix );

static void chr_log_script_time( const CHR_REF by_reference ichr );

static bool_t update_chr_darkvision( const CHR_REF by_reference character );

static const float traction_min = 0.2f;

static ego_chr_bundle * move_one_character_get_environment( ego_chr_bundle * pbdl, ego_chr_environment * penviro );
static ego_chr_bundle * move_one_character_do_fluid_friction( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_voluntary_flying( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_voluntary( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_involuntary( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_orientation( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_z_motion( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_animation( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_limit_flying( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_jump( ego_chr_bundle * pbdl );
static ego_chr_bundle * move_one_character_do_floor( ego_chr_bundle * pbdl );

static bool_t pack_validate( ego_pack * ppack );

static fvec2_t chr_get_diff( ego_chr * pchr, float test_pos[], float center_pressure );
static float   get_mesh_pressure( ego_chr * pchr, float test_pos[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void character_system_begin()
{
    ChrList_init();
    init_all_cap();
}

//--------------------------------------------------------------------------------------------
void character_system_end()
{
    release_all_cap();
    ChrList_dtor();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ego_chr::dealloc( ego_chr * pchr )
{
    if ( !ego_chr_data::dealloc( pchr ) ) return bfalse;

    // do some list clean-up
    remove_all_character_enchants( GET_REF_PCHR( pchr ) );

    ego_BSP_leaf::dtor( &( pchr->bsp_leaf ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr_data::dealloc( ego_chr_data * pdata )
{
    /// Free all allocated memory

    if ( NULL == pdata ) return bfalse;

    // deallocate
    BillboardList_free_one( REF_TO_INT( pdata->ibillboard ) );

    LoopedList_remove( pdata->loopedsound_channel );

    chr_instance_dtor( &( pdata->inst ) );
    ego_ai_state::dtor( &( pdata->ai ) );

    EGOBOO_ASSERT( NULL == pdata->inst.vrt_lst );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_chr_data * ego_chr_data::ctor( ego_chr_data * pdata )
{
    /// @details BB@> initialize the character data to safe values
    ///     since we use memset(..., 0, ...), everything == 0 == false == 0.0f
    ///     statements are redundant

    int cnt;

    if ( NULL == pdata ) return pdata;

    // deallocate any existing data
    if ( NULL == ego_chr_data::dealloc( pdata ) ) return NULL;

    // clear out all data
    memset( pdata, 0, sizeof( *pdata ) );

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
    pdata->firstenchant = ( ENC_REF ) MAX_ENC;
    pdata->undoenchant = ( ENC_REF ) MAX_ENC;
    pdata->missiletreatment = MISSILE_NORMAL;

    // Character stuff
    pdata->turnmode = TURNMODE_VELOCITY;
    pdata->alive = btrue;

    // Jumping
    pdata->jump_time = JUMP_DELAY;

    // Grip info
    pdata->attachedto = ( CHR_REF )MAX_CHR;
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pdata->holdingwhich[cnt] = ( CHR_REF )MAX_CHR;
    }

    // pack/inventory info
    pdata->pack.next = ( CHR_REF )MAX_CHR;
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        pdata->inventory[cnt] = ( CHR_REF )MAX_CHR;
    }

    // Set up position
    pdata->ori.map_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    pdata->ori.map_facing_x = MAP_TURN_OFFSET;

    // I think we have to set the dismount timer, otherwise objects that
    // are spawned by chests will behave strangely...
    // nope this did not fix it
    // ZF@> If this is != 0 then scorpion claws and riders are dropped at spawn (non-item objects)
    pdata->dismount_timer  = 0;
    pdata->dismount_object = ( CHR_REF )MAX_CHR;

    // set all of the integer references to invalid values
    pdata->firstenchant = ( ENC_REF ) MAX_ENC;
    pdata->undoenchant  = ( ENC_REF ) MAX_ENC;
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        pdata->holdingwhich[cnt] = ( CHR_REF )MAX_CHR;
    }

    pdata->pack.next = ( CHR_REF )MAX_CHR;
    for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
    {
        pdata->inventory[cnt] = ( CHR_REF )MAX_CHR;
    }

    pdata->onwhichplatform_ref    = ( CHR_REF )MAX_CHR;
    pdata->onwhichplatform_update = 0;
    pdata->targetplatform_ref     = ( CHR_REF )MAX_CHR;
    pdata->attachedto             = ( CHR_REF )MAX_CHR;

    // all movements valid
    pdata->movement_bits   = ( unsigned )( ~0 );

    // not a player
    pdata->is_which_player = MAX_PLAYER;


    //---- call the constructors of the "has a" classes

    // set the instance values to safe values
    chr_instance_ctor( &( pdata->inst ) );

    // initialize the ai_state
    ego_ai_state::ctor( &( pdata->ai ) );

    return pdata;
}

//--------------------------------------------------------------------------------------------
ego_chr_data * ego_chr_data::dtor( ego_chr_data * pdata )
{
    if ( NULL == pdata ) return pdata;

    // destruct/free any allocated data
    ego_chr_data::dealloc( pdata );

    return pdata;
}


//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::ctor( ego_chr * pchr )
{
    /// @details BB@> initialize the character data to safe values
    ///     since we use memset(..., 0, ...), everything == 0 == false == 0.0f
    ///     statements are redundant

    if ( NULL == pchr ) return pchr;

    // deallocate any existing data
    if ( VALID_PCHR( pchr ) )
    {
        // call the dtor
        if ( NULL == ego_chr::dtor( pchr ) ) return NULL;
    }

    // call the data ctor
    if ( NULL == ego_chr_data::ctor( pchr ) ) return NULL;

    // start the character out in the "dance" animation
    ego_chr::start_anim( pchr, ACTION_DA, btrue, btrue );

    // initialize the bsp node for this character
    ego_BSP_leaf::ctor( &( pchr->bsp_leaf ), 3, pchr, 1 );

    pchr->bsp_leaf.data      = pchr;
    pchr->bsp_leaf.data_type = LEAF_CHR;
    pchr->bsp_leaf.index     = GET_INDEX_PCHR( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::dtor( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    // destruct/free any allocated data
    ego_chr::dealloc( pchr );

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    POBJ_TERMINATE( pchr );

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr::ego_chr() { ego_chr::ctor( this ); };
ego_chr::~ego_chr() { ego_chr::dtor( this ); };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_count_free()
{
    return ChrList.free_count;
}

//--------------------------------------------------------------------------------------------
void flash_character_height( const CHR_REF by_reference character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
    /// @details ZZ@> This function sets a character's lighting depending on vertex height...
    ///    Can make feet dark and head light...

    Uint32 cnt;
    Sint16 z;

    ego_mad * pmad;
    ego_chr_instance * pinst;

    pinst = ego_chr::get_pinstance( character );
    if ( NULL == pinst ) return;

    pmad = ego_chr::get_pmad( character );
    if ( NULL == pmad ) return;

    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        z = pinst->vrt_lst[cnt].pos[ZZ];

        if ( z < low )
        {
            pinst->color_amb = valuelow;
        }
        else
        {
            if ( z > high )
            {
                pinst->color_amb = valuehigh;
            }
            else
            {
                pinst->color_amb = ( valuehigh * ( z - low ) / ( high - low ) ) +
                                   ( valuelow * ( high - z ) / ( high - low ) );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
    /// @details ZZ@> This function keeps weapons near their holders

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        CHR_REF iattached = pchr->attachedto;

        if ( INGAME_CHR( iattached ) )
        {
            ego_chr * pattached = ChrList.lst + iattached;

            // Keep in hand weapons with iattached
            if ( ego_chr::matrix_valid( pchr ) )
            {
                ego_chr::set_pos( pchr, mat_getTranslate_v( pchr->inst.matrix ) );
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
                if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.alpha )
                {
                    ego_chr::set_alpha( pchr, SEEINVISIBLE );
                }
                else
                {
                    // Only if not naturally transparent
                    if ( 255 == pchr->alpha_base )
                    {
                        ego_chr::set_alpha( pchr, pattached->inst.alpha );
                    }
                    else
                    {
                        ego_chr::set_alpha( pchr, pchr->alpha_base );
                    }
                }

                // Do light too
                if ( VALID_PLA( pattached->is_which_player ) && 255 != pattached->inst.light )
                {
                    ego_chr::set_light( pchr, SEEINVISIBLE );
                }
                else
                {
                    // Only if not naturally transparent
                    if ( 255 == ego_chr::get_pcap( cnt )->light )
                    {
                        ego_chr::set_light( pchr, pattached->inst.light );
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
            pchr->attachedto = ( CHR_REF )MAX_CHR;

            // Keep inventory with iattached
            if ( !pchr->pack.is_packed )
            {
                PACK_BEGIN_LOOP( iattached, pchr->pack.next )
                {
                    ego_chr::set_pos( ChrList.lst + iattached, ego_chr::get_pos_v( pchr ) );

                    // Copy olds to make SendMessageNear work
                    ChrList.lst[iattached].pos_old = pchr->pos_old;
                }
                PACK_END_LOOP( iattached );
            }
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( const CHR_REF by_reference ichr )
{
    /// @details ZZ@> This function sets one character's matrix

    ego_chr * pchr;
    ego_chr_instance * pinst;

    if ( !INGAME_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;
    pinst = &( pchr->inst );

    // invalidate this matrix
    pinst->matrix_cache.matrix_valid = bfalse;

    if ( pchr->is_overlay )
    {
        // This character is an overlay and its ai.target points to the object it is overlaying
        // Overlays are kept with their target...
        if ( INGAME_CHR( pchr->ai.target ) )
        {
            ego_chr * ptarget = ChrList.lst + pchr->ai.target;

            ego_chr::set_pos( pchr, ego_chr::get_pos_v( ptarget ) );

            // copy the matrix
            CopyMatrix( &( pinst->matrix ), &( ptarget->inst.matrix ) );

            // copy the matrix data
            pinst->matrix_cache = ptarget->inst.matrix_cache;
        }
    }
    else
    {
        if ( pchr->stickybutt )
        {
            pinst->matrix = ScaleXYZRotateXYZTranslate_SpaceFixed( pchr->fat, pchr->fat, pchr->fat,
                            TO_TURN( pchr->ori.facing_z ),
                            TO_TURN( pchr->ori.map_facing_x - MAP_TURN_OFFSET ),
                            TO_TURN( pchr->ori.map_facing_y - MAP_TURN_OFFSET ),
                            pchr->pos.x, pchr->pos.y, pchr->pos.z );
        }
        else
        {
            pinst->matrix = ScaleXYZRotateXYZTranslate_BodyFixed( pchr->fat, pchr->fat, pchr->fat,
                            TO_TURN( pchr->ori.facing_z ),
                            TO_TURN( pchr->ori.map_facing_x - MAP_TURN_OFFSET ),
                            TO_TURN( pchr->ori.map_facing_y - MAP_TURN_OFFSET ),
                            pchr->pos.x, pchr->pos.y, pchr->pos.z );
        }

        pinst->matrix_cache.valid        = btrue;
        pinst->matrix_cache.matrix_valid = btrue;
        pinst->matrix_cache.type_bits    = MAT_CHARACTER;

        pinst->matrix_cache.self_scale.x = pchr->fat;
        pinst->matrix_cache.self_scale.y = pchr->fat;
        pinst->matrix_cache.self_scale.z = pchr->fat;

        pinst->matrix_cache.rotate.x = CLIP_TO_16BITS( pchr->ori.map_facing_x - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate.y = CLIP_TO_16BITS( pchr->ori.map_facing_y - MAP_TURN_OFFSET );
        pinst->matrix_cache.rotate.z = pchr->ori.facing_z;

        pinst->matrix_cache.pos = ego_chr::get_pos( pchr );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_log_script_time( const CHR_REF by_reference ichr )
{
    // log the amount of script time that this object used up

    ego_chr * pchr;
    ego_cap * pcap;
    FILE * ftmp;

    if ( !DEFINED_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

    if ( pchr->ai._clkcount <= 0 ) return;

    pcap = ego_chr::get_pcap( ichr );
    if ( NULL == pcap ) return;

    ftmp = fopen( vfs_resolveWriteFilename( "/debug/script_timing.txt" ), "a+" );
    if ( NULL != ftmp )
    {
        fprintf( ftmp, "update == %d\tindex == %d\tname == \"%s\"\tclassname == \"%s\"\ttotal_time == %e\ttotal_calls == %f\n",
                 update_wld, REF_TO_INT( ichr ), pchr->name, pcap->classname,
                 pchr->ai._clktime, pchr->ai._clkcount );
        fflush( ftmp );
        fclose( ftmp );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character_in_game( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function sticks a character back on the free character stack
    ///
    /// @note This should only be called by cleanup_all_characters() or free_inventory_in_game()

    int     cnt;
    ego_cap * pcap;
    ego_chr * pchr;

    if ( !DEFINED_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return;

    // Remove from stat list
    if ( pchr->StatusList_on )
    {
        bool_t stat_found;

        pchr->StatusList_on = bfalse;

        stat_found = bfalse;
        for ( cnt = 0; cnt < StatusList_count; cnt++ )
        {
            if ( StatusList[cnt] == character )
            {
                stat_found = btrue;
                break;
            }
        }

        if ( stat_found )
        {
            for ( cnt++; cnt < StatusList_count; cnt++ )
            {
                SWAP( CHR_REF, StatusList[cnt-1], StatusList[cnt] );
            }
            StatusList_count--;
        }
    }

    // Make sure everyone knows it died
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        ego_ai_state * pai;

        if ( !INGAME_CHR( cnt ) || cnt == character ) continue;
        pai = ego_chr::get_pai( cnt );

        if ( pai->target == character )
        {
            ADD_BITS( pai->alert, ALERTIF_TARGETKILLED );
            pai->target = MAX_CHR;
        }

        if ( ego_chr::get_pteam( cnt )->leader == character )
        {
            ADD_BITS( pai->alert, ALERTIF_LEADERKILLED );
        }
    }
    CHR_END_LOOP();

    // Handle the team
    if ( pchr->alive && !pcap->invictus && TeamStack.lst[pchr->baseteam].morale > 0 )
    {
        TeamStack.lst[pchr->baseteam].morale--;
    }

    if ( TeamStack.lst[pchr->team].leader == character )
    {
        TeamStack.lst[pchr->team].leader = NOLEADER;
    }

    // remove any attached particles
    disaffirm_attached_particles( character );

    // actually get rid of the character
    ChrList_free_one( character );
}

//--------------------------------------------------------------------------------------------
void free_inventory_in_game( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function frees every item in the character's inventory
    ///
    /// @note this should only be called by cleanup_all_characters()

    CHR_REF cnt;

    if ( !DEFINED_CHR( character ) ) return;

    PACK_BEGIN_LOOP( cnt, ChrList.lst[character].pack.next )
    {
        free_one_character_in_game( cnt );
    }
    PACK_END_LOOP( cnt );

    // set the inventory to the "empty" state
    ChrList.lst[character].pack.count = 0;
    ChrList.lst[character].pack.next  = ( CHR_REF )MAX_CHR;
}

//--------------------------------------------------------------------------------------------
ego_prt * place_particle_at_vertex( ego_prt * pprt, const CHR_REF by_reference character, int vertex_offset )
{
    /// @details ZZ@> This function sets one particle's position to be attached to a character.
    ///    It will kill the particle if the character is no longer around

    int     vertex;
    fvec4_t point[1], nupoint[1];

    ego_chr * pchr;

    if ( !DEFINED_PPRT( pprt ) ) return pprt;

    if ( !INGAME_CHR( character ) )
    {
        goto place_particle_at_vertex_fail;
    }
    pchr = ChrList.lst + character;

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
            fvec3_t tmp_pos = VECT3( pchr->inst.matrix.CNV( 3, 0 ), pchr->inst.matrix.CNV( 3, 1 ), pchr->inst.matrix.CNV( 3, 2 ) );
            ego_prt::set_pos( pprt, tmp_pos.v );

            return pprt;
        }

        vertex = 0;
        if ( NULL != pmad )
        {
            vertex = (( signed )pchr->inst.vrt_count ) - vertex_offset;

            // do the automatic update
            chr_instance_update_vertices( &( pchr->inst ), vertex, vertex, bfalse );

            // Calculate vertex_offset point locations with linear interpolation and other silly things
            point[0].x = pchr->inst.vrt_lst[vertex].pos[XX];
            point[0].y = pchr->inst.vrt_lst[vertex].pos[YY];
            point[0].z = pchr->inst.vrt_lst[vertex].pos[ZZ];
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
        TransformVertices( &( pchr->inst.matrix ), point, nupoint, 1 );

        ego_prt::set_pos( pprt, nupoint[0].v );
    }
    else
    {
        // No matrix, so just wing it...
        ego_prt::set_pos( pprt, ego_chr::get_pos_v( pchr ) );
    }

    return pprt;

place_particle_at_vertex_fail:

    prt_request_free_ref( GET_REF_PPRT( pprt ) );

    return NULL;
}

//--------------------------------------------------------------------------------------------
void make_all_character_matrices( bool_t do_physics )
{
    /// @details ZZ@> This function makes all of the character's matrices

    //int cnt;
    //bool_t done;

    //// blank the accumulators
    //for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    //{
    //    ChrList.lst[ichr].phys.apos_plat.x = 0.0f;
    //    ChrList.lst[ichr].phys.apos_plat.y = 0.0f;
    //    ChrList.lst[ichr].phys.apos_plat.z = 0.0f;

    //    ChrList.lst[ichr].phys.apos_coll.x = 0.0f;
    //    ChrList.lst[ichr].phys.apos_coll.y = 0.0f;
    //    ChrList.lst[ichr].phys.apos_coll.z = 0.0f;

    //    ChrList.lst[ichr].phys.avel.x = 0.0f;
    //    ChrList.lst[ichr].phys.avel.y = 0.0f;
    //    ChrList.lst[ichr].phys.avel.z = 0.0f;
    //}

    // just call ego_chr::update_matrix on every character
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
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
    ////        pchr = ChrList.lst + ichr;

    ////        tmp_pos = ego_chr::get_pos( pchr );

    ////        // do the "integration" of the accumulated accelerations
    ////        pchr->vel.x += pchr->phys.avel.x;
    ////        pchr->vel.y += pchr->phys.avel.y;
    ////        pchr->vel.z += pchr->phys.avel.z;

    ////        // do the "integration" on the position
    ////        if ( ABS(pchr->phys.apos_coll.x) > 0 )
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

    ////        if ( ABS(pchr->phys.apos_coll.y) > 0 )
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

    ////        if ( ABS(pchr->phys.apos_coll.z) > 0 )
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
    ////        pchr = ChrList.lst + ichr;

    ////        if( !pchr->inst.matrix_cache.valid ) continue;

    ////        pchr->inst.matrix.CNV( 3, 0 ) = pchr->pos.x;
    ////        pchr->inst.matrix.CNV( 3, 1 ) = pchr->pos.y;
    ////        pchr->inst.matrix.CNV( 3, 2 ) = pchr->pos.z;
    ////    }
    ////}
}

//--------------------------------------------------------------------------------------------
void free_all_chraracters()
{
    /// @details ZZ@> This function resets the character allocation list

    // free all the characters
    ChrList_free_all();

    // free_all_players
    PlaStack.count = 0;
    local_numlpla = 0;
    local_stats.noplayers = btrue;

    // free_all_stats
    StatusList_count = 0;
}

//--------------------------------------------------------------------------------------------
float chr_get_mesh_pressure( ego_chr * pchr, float test_pos[] )
{
    float retval = 0.0f;
    float radius = 0.0f;

    if ( !DEFINED_PCHR( pchr ) ) return retval;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    /// @note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_get_pressure( PMesh, test_pos, radius, pchr->stoppedby );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
fvec2_t chr_get_diff( ego_chr * pchr, float test_pos[], float center_pressure )
{
    fvec2_t retval = ZERO_VECT2;
    float   radius;

    if ( !DEFINED_PCHR( pchr ) ) return retval;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return retval;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    /// @note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_get_diff( PMesh, test_pos, radius, center_pressure, pchr->stoppedby );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD chr_hit_wall( ego_chr * pchr, float test_pos[], float nrm[], float * pressure )
{
    /// @details ZZ@> This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    BIT_FIELD    retval;
    float        radius;

    if ( !DEFINED_PCHR( pchr ) ) return 0;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return 0;

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // calculate the radius based on whether the character is on camera
    /// @note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_hit_wall( PMesh, test_pos, radius, pchr->stoppedby, nrm, pressure );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests  += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_test_wall( ego_chr * pchr, float test_pos[] )
{
    /// @details ZZ@> This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    bool_t retval;
    float  radius;

    if ( !ACTIVE_PCHR( pchr ) ) return 0;

    if ( 0.0f == pchr->bump_stt.size || INFINITE_WEIGHT == pchr->phys.weight ) return bfalse;

    // calculate the radius based on whether the character is on camera
    /// @note ZF@> this may be the cause of the bug allowing AI to move through walls when the camera is not looking at them?
    radius = 0.0f;
    if ( mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pchr->onwhichgrid ].inrenderlist )
        {
            radius = pchr->bump_1.size;
        }
    }

    if ( NULL == test_pos ) test_pos = pchr->pos.v;

    // do the wall test
    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = mesh_test_wall( PMesh, test_pos, radius, pchr->stoppedby, NULL );
    }
    chr_stoppedby_tests += mesh_mpdfx_tests;
    chr_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function fixes a character's max acceleration

    ENC_REF enchant;
    ego_chr * pchr;
    ego_cap * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Okay, remove all acceleration enchants
    enchant = pchr->firstenchant;
    while ( enchant != MAX_ENC )
    {
        ego_enc::remove_add( enchant, ADDACCEL );
        enchant = EncList.lst[enchant].nextenchant_ref;
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
        enchant = EncList.lst[enchant].nextenchant_ref;
    }
}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_mount( const CHR_REF by_reference character, Uint8 ignorekurse, Uint8 doshop )
{
    /// @details ZZ@> This function drops an item

    CHR_REF mount;
    Uint16  hand;
    ENC_REF enchant;
    bool_t  inshop;
    ego_chr * pchr, * pmount;

    // Make sure the character is valid
    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    // Make sure the character is mounted
    mount = ChrList.lst[character].attachedto;
    if ( !INGAME_CHR( mount ) ) return bfalse;
    pmount = ChrList.lst + mount;

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
    pchr->attachedto = ( CHR_REF )MAX_CHR;
    if ( pmount->holdingwhich[SLOT_LEFT] == character )
        pmount->holdingwhich[SLOT_LEFT] = ( CHR_REF )MAX_CHR;

    if ( pmount->holdingwhich[SLOT_RIGHT] == character )
        pmount->holdingwhich[SLOT_RIGHT] = ( CHR_REF )MAX_CHR;

    if ( pchr->alive )
    {
        // play the falling animation...
        ego_chr::play_action( pchr, ACTION_JB + hand, bfalse );
    }
    else if ( pchr->inst.action_which < ACTION_KA || pchr->inst.action_which > ACTION_KD )
    {
        // play the "killed" animation...
        ego_chr::play_action( pchr, ACTION_KA + generate_randmask( 0, 3 ), bfalse );
        pchr->inst.action_keep = btrue;
    }

    // Set the positions
    if ( ego_chr::matrix_valid( pchr ) )
    {
        ego_chr::set_pos( pchr, mat_getTranslate_v( pchr->inst.matrix ) );
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
    pchr->inst.action_loop = bfalse;

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

            enchant = EncList.lst[enchant].nextenchant_ref;
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

            enchant = EncList.lst[enchant].nextenchant_ref;
        }
    }

    // Set twist
    pchr->ori.map_facing_y = MAP_TURN_OFFSET;
    pchr->ori.map_facing_x = MAP_TURN_OFFSET;

    ego_chr::update_matrix( pchr, btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function fixes an item's transparency

    CHR_REF mount;
    ENC_REF enchant;
    ego_chr * pchr, * pmount;

    // Make sure the character is valid
    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // Make sure the character is mounted
    mount = ChrList.lst[character].attachedto;
    if ( !INGAME_CHR( mount ) ) return;
    pmount = ChrList.lst + mount;

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

            enchant = EncList.lst[enchant].nextenchant_ref;
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

            enchant = EncList.lst[enchant].nextenchant_ref;
        }
    }
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount( const CHR_REF by_reference iitem, const CHR_REF by_reference iholder, grip_offset_t grip_off )
{
    /// @details ZZ@> This function attaches one character/item to another ( the holder/mount )
    ///    at a certain vertex offset ( grip_off )

    slot_t slot;

    ego_chr * pitem, * pholder;

    // Make sure the character/item is valid
    // this could be called before the item is fully instantiated
    if ( !DEFINED_CHR( iitem ) ) return;
    pitem = ChrList.lst + iitem;

    // cannot be mounted if you are packed
    if ( pitem->pack.is_packed ) return;

    // make a reasonable time for the character to remount something
    // for characters jumping out of pots, etc
    if ( !pitem->isitem && pitem->dismount_timer > 0 ) return;

    // Make sure the holder/mount is valid
    if ( !INGAME_CHR( iholder ) ) return;
    pholder = ChrList.lst + iholder;

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

    ego_chr::set_pos( pitem, mat_getTranslate_v( pitem->inst.matrix ) );

    pitem->enviro.inwater  = bfalse;
    pitem->jump_time = 4 * JUMP_DELAY;

    // Run the held animation
    if ( pholder->ismount && grip_off == GRIP_ONLY )
    {
        // Riding iholder
        ego_chr::play_action( pitem, ACTION_MI, btrue );
        pitem->inst.action_loop = btrue;
    }
    else if ( pitem->alive )
    {
        /// @note ZF@> hmm, here is the torch holding bug. Removing
        /// the interpolation seems to fix it...
        ego_chr::play_action( pitem, ACTION_MM + slot, bfalse );
        pitem->inst.frame_lst = pitem->inst.frame_nxt;

        if ( pitem->isitem )
        {
            // Item grab
            pitem->inst.action_keep = btrue;
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
void drop_all_idsz( const CHR_REF by_reference character, IDSZ idsz_min, IDSZ idsz_max )
{
    /// @details ZZ@> This function drops all items ( idsz_min to idsz_max ) that are in a character's
    ///    inventory ( Not hands ).

    ego_chr  * pchr;
    CHR_REF  item, lastitem;
    FACING_T direction;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->pos.z <= ( PITDEPTH >> 1 ) )
    {
        // Don't lose items in pits...
        return;
    }

    lastitem = character;
    PACK_BEGIN_LOOP( item, pchr->pack.next )
    {
        if ( INGAME_CHR( item ) && item != character )
        {
            ego_chr * pitem = ChrList.lst + item;

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
    }
    PACK_END_LOOP( item );

}

//--------------------------------------------------------------------------------------------
bool_t drop_all_items( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function drops all of a character's items

    CHR_REF  item;
    FACING_T direction;
    Sint16   diradd;
    ego_chr  * pchr;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], btrue, bfalse );
    detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], btrue, bfalse );
    if ( pchr->pack.count > 0 )
    {
        direction = pchr->ori.facing_z + ATK_BEHIND;
        diradd    = 0x00010000 / pchr->pack.count;

        while ( pchr->pack.count > 0 )
        {
            item = chr_inventory_remove_item( character, GRIP_LEFT, bfalse );

            if ( INGAME_CHR( item ) )
            {
                ego_chr * pitem = ChrList.lst + item;

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
bool_t character_grab_stuff( const CHR_REF by_reference ichr_a, grip_offset_t grip_off, bool_t grab_people )
{
    /// @details ZZ@> This function makes the character pick up an item if there's one around

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

    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

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
        frame_nxt = pchr_a->inst.frame_nxt;
        vertex    = pchr_a->inst.vrt_count - grip_off;

        // do the automatic update
        chr_instance_update_vertices( &( pchr_a->inst ), vertex, vertex, bfalse );

        // Calculate grip_off point locations with linear interpolation and other silly things
        point[0].x = pchr_a->inst.vrt_lst[vertex].pos[XX];
        point[0].y = pchr_a->inst.vrt_lst[vertex].pos[YY];
        point[0].z = pchr_a->inst.vrt_lst[vertex].pos[ZZ];
        point[0].w = 1.0f;

        // Do the transform
        TransformVertices( &( pchr_a->inst.matrix ), point, nupoint, 1 );
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
    CHR_BEGIN_LOOP_ACTIVE( ichr_b, pchr_b )
    {
        fvec3_t   pos_b;
        float     dx, dy, dz, dxy;
        bool_t    can_grab = btrue;

        // do nothing to yourself
        if ( ichr_a == ichr_b ) continue;

        // Don't do hidden objects
        if ( pchr_b->is_hidden ) continue;

        if ( pchr_b->pack.is_packed ) continue;        // pickpocket not allowed yet
        if ( INGAME_CHR( pchr_b->attachedto ) ) continue; // disarm not allowed yet

        // do not pick up your mount
        if ( pchr_b->holdingwhich[SLOT_LEFT] == ichr_a ||
             pchr_b->holdingwhich[SLOT_RIGHT] == ichr_a ) continue;

        pos_b = pchr_b->pos;

        // First check absolute value diamond
        dx = ABS( nupoint[0].x - pos_b.x );
        dy = ABS( nupoint[0].y - pos_b.y );
        dz = ABS( nupoint[0].z - pos_b.z );
        dxy = dx + dy;

        if ( dxy > GRID_SIZE * 2 || dz > MAX( pchr_b->bump.height, GRABSIZE ) ) continue;

        // reasonable carrying capacity
        if ( pchr_b->phys.weight > pchr_a->phys.weight + pchr_a->strength * INV_FF )
        {
            can_grab = bfalse;
        }

        // grab_people == btrue allows you to pick up living non-items
        // grab_people == false allows you to pick up living (functioning) items
        if ( pchr_b->alive && ( grab_people == pchr_b->isitem ) )
        {
            can_grab = bfalse;
        }

        if ( can_grab )
        {
            grab_list[grab_count].ichr = ichr_b;
            grab_list[grab_count].dist = dxy;
            grab_count++;
        }
        else
        {
            ungrab_list[grab_count].ichr = ichr_b;
            ungrab_list[grab_count].dist = dxy;
            ungrab_count++;
        }
    }
    CHR_END_LOOP();

    // sort the grab list
    if ( grab_count > 1 )
    {
        qsort( grab_list, grab_count, sizeof( ego_grab_data ), grab_data_cmp );
    }

    // try to grab something
    retval = bfalse;
    for ( cnt = 0; cnt < grab_count; cnt++ )
    {
        bool_t can_grab;

        ego_chr * pchr_b;

        ichr_b = grab_list[cnt].ichr;
        pchr_b = ChrList.lst + ichr_b;

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
        if ( cfg.feedback != FEEDBACK_OFF && VALID_PLA( pchr_a->is_which_player ) )
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
        if ( VALID_PLA( pchr_a->is_which_player ) && ungrab_count > 0 )
        {
            ego_chr::get_MatForward( pchr_a, &vforward );

            // sort the ungrab list
            if ( ungrab_count > 1 )
            {
                qsort( ungrab_list, ungrab_count, sizeof( ego_grab_data ), grab_data_cmp );
            }

            for ( cnt = 0; cnt < ungrab_count; cnt++ )
            {
                float       ftmp;
                fvec3_t     diff;
                ego_chr     * pchr_b;

                if ( ungrab_list[cnt].dist > GRABSIZE ) continue;

                ichr_b = ungrab_list[cnt].ichr;
                if ( !INGAME_CHR( ichr_b ) ) continue;

                pchr_b = ChrList.lst + ichr_b;

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
void character_swipe( const CHR_REF by_reference ichr, slot_t slot )
{
    /// @details ZZ@> This function spawns an attack particle

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

    if ( !INGAME_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

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
        action = pchr->inst.action_which;
    }

    if ( !INGAME_CHR( iweapon ) ) return;
    pweapon = ChrList.lst + iweapon;

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
    if ( !unarmed_attack && (( pweapon_cap->isstackable && pweapon->ammo > 1 ) || ACTION_IS_TYPE( pweapon->inst.action_which, F ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        ithrown = spawn_one_character( pchr->pos, pweapon->profile_ref, ego_chr::get_iteam( iholder ), 0, pchr->ori.facing_z, pweapon->name, ( CHR_REF )MAX_CHR );
        if ( INGAME_CHR( ithrown ) )
        {
            ego_chr * pthrown = ChrList.lst + ithrown;

            pthrown->iskursed = bfalse;
            pthrown->ammo = 1;
            ADD_BITS( pthrown->ai.alert, ALERTIF_THROWN );
            velocity = pchr->strength / ( pthrown->phys.weight * THROWFIX );
            velocity += MINTHROWVELOCITY;
            velocity = MIN( velocity, MAXTHROWVELOCITY );

            turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            {
                fvec3_t _tmp_vec = VECT3( turntocos[ turn ] * velocity, turntocos[ turn ] * velocity, DROPZVEL );
                phys_data_accumulate_avel( &( pthrown->phys ), _tmp_vec.v );
            }

            if ( pweapon->ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( iweapon, btrue, bfalse );
                ego_chr::request_terminate( GET_REF_PCHR( pweapon ) );
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
        if ( pweapon->ammomax == 0 || pweapon->ammo != 0 )
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
                iparticle = spawn_one_particle( pweapon->pos, pchr->ori.facing_z, pweapon->profile_ref, pweapon_cap->attack_pip, iweapon, spawn_vrt_offset, ego_chr::get_iteam( iholder ), iholder, ( PRT_REF )TOTAL_MAX_PRT, 0, ( CHR_REF )MAX_CHR );

                if ( VALID_PRT( iparticle ) )
                {
                    fvec3_t tmp_pos;
                    ego_prt * pprt = PrtList.lst + iparticle;

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
                        pprt->attachedto_ref = ( CHR_REF )MAX_CHR;

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
void drop_money( const CHR_REF by_reference character, int money )
{
    /// @details ZZ@> This function drops some of a character's money

    int huns, tfives, fives, ones, cnt;

    if ( !INGAME_CHR( character ) ) return;

    if ( money > ChrList.lst[character].money )  money = ChrList.lst[character].money;
    if ( money > 0 && ChrList.lst[character].pos.z > -2 )
    {
        ChrList.lst[character].money = ChrList.lst[character].money - money;
        huns = money / 100;  money -= ( huns << 7 ) - ( huns << 5 ) + ( huns << 2 );
        tfives = money / 25;  money -= ( tfives << 5 ) - ( tfives << 3 ) + tfives;
        fives = money / 5;  money -= ( fives << 2 ) + fives;
        ones = money;

        for ( cnt = 0; cnt < ones; cnt++ )
        {
            spawn_one_particle_global( ChrList.lst[character].pos, ATK_FRONT, PIP_COIN1, cnt );
        }

        for ( cnt = 0; cnt < fives; cnt++ )
        {
            spawn_one_particle_global( ChrList.lst[character].pos, ATK_FRONT, PIP_COIN5, cnt );
        }

        for ( cnt = 0; cnt < tfives; cnt++ )
        {
            spawn_one_particle_global( ChrList.lst[character].pos, ATK_FRONT, PIP_COIN25, cnt );
        }

        for ( cnt = 0; cnt < huns; cnt++ )
        {
            spawn_one_particle_global( ChrList.lst[character].pos, ATK_FRONT, PIP_COIN100, cnt );
        }

        ChrList.lst[character].damagetime = DAMAGETIME;  // So it doesn't grab it again
    }
}

//--------------------------------------------------------------------------------------------
void call_for_help( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function issues a call for help to all allies

    TEAM_REF team;

    if ( !INGAME_CHR( character ) ) return;

    team = ego_chr::get_iteam( character );
    TeamStack.lst[team].sissy = character;

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        if ( cnt != character && !team_hates_team( pchr->team, team ) )
        {
            ADD_BITS( pchr->ai.alert, ALERTIF_CALLEDFORHELP );
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool_t setup_xp_table( const CAP_REF by_reference icap )
{
    /// @details ZF@> This calculates the xp needed to reach next level and stores it in an array for later use

    Uint8 level;
    ego_cap * pcap;

    if ( !LOADED_CAP( icap ) ) return bfalse;
    pcap = CapStack.lst + icap;

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
void do_level_up( const CHR_REF by_reference character )
{
    /// @details BB@> level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    ego_chr * pchr;
    ego_cap * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return;

    // Do level ups and stat changes
    curlevel = pchr->experiencelevel + 1;
    if ( curlevel < MAXLEVEL )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = pchr->experience;
        xpneeded  = pcap->experience_forlevel[curlevel];
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            pchr->experiencelevel++;
            xpneeded = pcap->experience_forlevel[curlevel];
            ADD_BITS( pchr->ai.alert, ALERTIF_LEVELUP );

            // The character is ready to advance...
            if ( VALID_PLA( pchr->is_which_player ) )
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
            number += pchr->lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->life += ( number - pchr->lifemax );
            pchr->lifemax = number;

            // Mana
            number = generate_irand_range( pcap->mana_stat.perlevel );
            number += pchr->manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            pchr->mana += ( number - pchr->manamax );
            pchr->manamax = number;

            // Mana Return
            number = generate_irand_range( pcap->manareturn_stat.perlevel );
            number += pchr->manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->manareturn = number;

            // Mana Flow
            number = generate_irand_range( pcap->manaflow_stat.perlevel );
            number += pchr->manaflow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchr->manaflow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( const CHR_REF by_reference character, int amount, xp_type xptype, bool_t override_invictus )
{
    /// @details ZZ@> This function gives a character experience

    float newamount;

    ego_chr * pchr;
    ego_cap * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

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
void give_team_experience( const TEAM_REF by_reference team, int amount, xp_type xptype )
{
    /// @details ZZ@> This function gives every character on a team experience

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
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
    /// @details ZZ@> This function makes the characters get bigger or smaller, depending
    ///    on their fat_goto and fat_goto_time. Spellbooks do not resize
    ///    BB@> assume that this will only be called from inside ego_chr::do_processing(),
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
                pchr->phys.weight = MIN( itmp, MAX_WEIGHT );
            }
        }
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
egoboo_rv export_one_character_quest_vfs( const char *szSaveName, const CHR_REF by_reference character )
{
    /// @details ZZ@> This function makes the naming.txt file for the character
    ego_player *ppla;

    if ( !INGAME_CHR( character ) ) return rv_fail;

    ppla = net_get_ppla( character );
    if ( ppla == NULL ) return rv_fail;

    return quest_log_upload_vfs( ppla->quest_log, SDL_arraysize( ppla->quest_log ), szSaveName );
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_name_vfs( const char *szSaveName, const CHR_REF by_reference character )
{
    /// @details ZZ@> This function makes the naming.txt file for the character

    if ( !INGAME_CHR( character ) ) return bfalse;

    return chop_export_vfs( szSaveName, ChrList.lst[character].name );
}

//--------------------------------------------------------------------------------------------
bool_t chr_upload_cap( ego_chr * pchr, ego_cap * pcap )
{
    /// @details BB@> prepare a character profile for exporting, by uploading some special values into the
    ///     cap. Just so that there is no confusion when you export multiple items of the same type,
    ///     DO NOT pass the pointer returned by ego_chr::get_pcap(). Instead, use a custom ego_cap declared on the stack,
    ///     or something similar
    ///
    /// @note This has been modified to basically reverse the actions of chr_download_cap().
    ///       If all enchants have been removed, this should export all permanent changes to the
    ///       base character profile.

    int tnc;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pcap || !pcap->loaded ) return bfalse;

    // export values that override spawn.txt values
    pcap->content_override   = pchr->ai.content;
    pcap->state_override     = pchr->ai.state;
    pcap->money              = pchr->money;
    pcap->skin_override      = pchr->skin;
    pcap->level_override     = pchr->experiencelevel;

    // export the current experience
    ints_to_range( pchr->experience, 0, &( pcap->experience ) );

    // export the current mana and life
    pcap->life_spawn         = CLIP( pchr->life, 0, pchr->lifemax );
    pcap->mana_spawn         = CLIP( pchr->mana, 0, pchr->manamax );

    // Movement
    pcap->anim_speed_sneak = pchr->anim_speed_sneak;
    pcap->anim_speed_walk = pchr->anim_speed_walk;
    pcap->anim_speed_run = pchr->anim_speed_run;

    // weight and size
    pcap->size       = pchr->fat_goto;
    pcap->bumpdampen = pchr->phys.bumpdampen;
    if ( pchr->phys.weight == INFINITE_WEIGHT )
    {
        pcap->weight = CAP_INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pchr->phys.weight / pchr->fat / pchr->fat / pchr->fat;
        pcap->weight = MIN( itmp, CAP_MAX_WEIGHT );
    }

    // Other junk
    pcap->fly_height  = pchr->fly_height;
    pcap->alpha       = pchr->alpha_base;
    pcap->light       = pchr->light_base;
    pcap->flashand    = pchr->flashand;
    pcap->dampen      = pchr->phys.dampen;

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
    pcap->lifecolor = pchr->lifecolor;
    pcap->manacolor = pchr->manacolor;
    ints_to_range( pchr->lifemax     , 0, &( pcap->life_stat.val ) );
    ints_to_range( pchr->manamax     , 0, &( pcap->mana_stat.val ) );
    ints_to_range( pchr->manareturn  , 0, &( pcap->manareturn_stat.val ) );
    ints_to_range( pchr->manaflow    , 0, &( pcap->manaflow_stat.val ) );

    // Gender
    pcap->gender  = pchr->gender;

    // Ammo
    pcap->ammomax = pchr->ammomax;
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
bool_t chr_download_cap( ego_chr * pchr, ego_cap * pcap )
{
    /// @details BB@> grab all of the data from the data.txt file

    int iTmp, tnc;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

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
    pchr->darkvision_level = ego_chr::get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
    pchr->see_invisible_level = pcap->see_invisible_level;

    // Ammo
    pchr->ammomax = pcap->ammomax;
    pchr->ammo = pcap->ammo;

    // Gender
    pchr->gender = pcap->gender;
    if ( pchr->gender == GENDER_RANDOM )  pchr->gender = generate_randmask( GENDER_FEMALE, GENDER_MALE );

    // Life and Mana
    pchr->lifecolor = pcap->lifecolor;
    pchr->manacolor = pcap->manacolor;
    pchr->lifemax = generate_irand_range( pcap->life_stat.val );
    pchr->life_return = pcap->life_return;
    pchr->manamax = generate_irand_range( pcap->mana_stat.val );
    pchr->manaflow = generate_irand_range( pcap->manaflow_stat.val );
    pchr->manareturn = generate_irand_range( pcap->manareturn_stat.val );

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
    pchr->jump_power = pcap->jump;
    ego_chr::set_jump_number_reset( pchr, pcap->jump_number );

    // Other junk
    ego_chr::set_fly_height( pchr, pcap->fly_height );
    pchr->maxaccel    = pchr->maxaccel_reset = pcap->maxaccel[pchr->skin];
    pchr->alpha_base  = pcap->alpha;
    pchr->light_base  = pcap->light;
    pchr->flashand    = pcap->flashand;
    pchr->phys.dampen = pcap->dampen;

    // Load current life and mana. this may be overridden later
    pchr->life = CLIP( pcap->life_spawn, LOWSTAT, ( UFP8_T )MAX( 0, pchr->lifemax ) );
    pchr->mana = CLIP( pcap->mana_spawn,       0, ( UFP8_T )MAX( 0, pchr->manamax ) );

    pchr->phys.bumpdampen = pcap->bumpdampen;
    if ( CAP_INFINITE_WEIGHT == pcap->weight )
    {
        pchr->phys.weight = INFINITE_WEIGHT;
    }
    else
    {
        Uint32 itmp = pcap->weight * pcap->size * pcap->size * pcap->size;
        pchr->phys.weight = MIN( itmp, MAX_WEIGHT );
    }

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
    pchr->experience      = MIN( iTmp, MAXXP );
    pchr->experiencelevel = pcap->level_override;

    // Particle attachments
    pchr->reaffirmdamagetype = pcap->attachedprt_reaffirmdamagetype;

    // Character size and bumping
    ego_chr::init_size( pchr, pcap );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_profile_vfs( const char *szSaveName, const CHR_REF by_reference character )
{
    /// @details ZZ@> This function creates a data.txt file for the given character.
    ///    it is assumed that all enchantments have been done away with

    ego_chr * pchr;
    ego_cap * pcap;

    // a local version of the cap, so that the CapStack data won't be corrupted
    ego_cap cap_tmp;

    if ( INVALID_CSTR( szSaveName ) && !DEFINED_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return bfalse;

    // load up the temporary cap
    memcpy( &cap_tmp, pcap, sizeof( ego_cap ) );

    // fill in the cap values with the ones we want to export from the character profile
    chr_upload_cap( pchr, &cap_tmp );

    return save_one_cap_data_file_vfs( szSaveName, NULL, &cap_tmp );
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_skin_vfs( const char *szSaveName, const CHR_REF by_reference character )
{
    /// @details ZZ@> This function creates a skin.txt file for the given character.

    vfs_FILE* filewrite;

    if ( !INGAME_CHR( character ) ) return bfalse;

    // Open the file
    filewrite = vfs_openWrite( szSaveName );
    if ( NULL == filewrite ) return bfalse;

    vfs_printf( filewrite, "// This file is used only by the import menu\n" );
    vfs_printf( filewrite, ": %d\n", ChrList.lst[character].skin );
    vfs_close( filewrite );
    return btrue;
}

//--------------------------------------------------------------------------------------------
CAP_REF load_one_character_profile_vfs( const char * tmploadname, int slot_override, bool_t required )
{
    /// @details ZZ@> This function fills a character profile with data from data.txt, returning
    /// the icap slot that the profile was stuck into.  It may cause the program
    /// to abort if bad things happen.

    CAP_REF  icap = ( CAP_REF )MAX_CAP;
    ego_cap * pcap;
    STRING  szLoadName;

    if ( VALID_PRO_RANGE( slot_override ) )
    {
        icap = ( CAP_REF )slot_override;
    }
    else
    {
        icap = pro_get_slot_vfs( tmploadname, MAX_PROFILE );
    }

    if ( !VALID_CAP_RANGE( icap ) )
    {
        // The data file wasn't found
        if ( required )
        {
            log_debug( "load_one_character_profile_vfs() - \"%s\" was not found. Overriding a global object?\n", szLoadName );
        }
        else if ( VALID_CAP_RANGE( slot_override ) && slot_override > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "load_one_character_profile_vfs() - Not able to open file \"%s\"\n", szLoadName );
        }

        return ( CAP_REF )MAX_CAP;
    }

    pcap = CapStack.lst + icap;

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
            log_error( "Object slot %i used twice (%s, %s)\n", REF_TO_INT( icap ), pcap->name, szLoadName );
        }
        else
        {
            // Stop, we don't want to override it
            return ( CAP_REF )MAX_CAP;
        }

        // If loading over an existing model is allowed (?how?) then make sure to release the old one
        release_one_cap( icap );
    }

    if ( NULL == load_one_cap_data_file_vfs( tmploadname, pcap ) )
    {
        return ( CAP_REF )MAX_CAP;
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
    pcap->bumpdampen = MAX( INV_FF, pcap->bumpdampen );

    return icap;
}

//--------------------------------------------------------------------------------------------
bool_t heal_character( const CHR_REF by_reference character, const CHR_REF by_reference healer, int amount, bool_t ignore_invictus )
{
    /// @details ZF@> This function gives some pure life points to the target, ignoring any resistances and so forth
    ego_chr * pchr, *pchr_h;

    // Setup the healed character
    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    // Setup the healer
    if ( !INGAME_CHR( healer ) ) return bfalse;
    pchr_h = ChrList.lst + healer;

    // Don't heal dead and invincible stuff
    if ( !pchr->alive || ( IS_INVICTUS_PCHR_RAW( pchr ) && !ignore_invictus ) ) return bfalse;

    // This actually heals the character
    pchr->life = CLIP( pchr->life, pchr->life + ABS( amount ), pchr->lifemax );

    // Set alerts, but don't alert that we healed ourselves
    if ( healer != character && pchr_h->attachedto != character && ABS( amount ) > HURTDAMAGE )
    {
        ADD_BITS( pchr->ai.alert, ALERTIF_HEALED );
        pchr->ai.attacklast = healer;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void cleanup_one_character( ego_chr * pchr )
{
    /// @details BB@> Everything necessary to disconnect one character from the game

    CHR_REF  ichr, itmp;
    SHOP_REF ishop;

    if ( !VALID_PCHR( pchr ) ) return;
    ichr = GET_REF_PCHR( pchr );

    pchr->sparkle = NOSPARKLE;

    // Remove it from the team
    pchr->team = pchr->baseteam;
    if ( TeamStack.lst[pchr->team].morale > 0 ) TeamStack.lst[pchr->team].morale--;

    if ( TeamStack.lst[pchr->team].leader == ichr )
    {
        // The team now has no leader if the character is the leader
        TeamStack.lst[pchr->team].leader = NOLEADER;
    }

    // Clear all shop passages that it owned...
    for ( ishop = 0; ishop < ShopStack.count; ishop++ )
    {
        if ( ShopStack.lst[ishop].owner != ichr ) continue;
        ShopStack.lst[ishop].owner = SHOP_NOOWNER;
    }

    // detach from any mount
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        detach_character_from_mount( ichr, btrue, bfalse );
    }

    // drop your left item
    itmp = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( itmp ) && ChrList.lst[itmp].isitem )
    {
        detach_character_from_mount( itmp, btrue, bfalse );
    }

    // drop your right item
    itmp = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( itmp ) && ChrList.lst[itmp].isitem )
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
            ego_enc_next = EncList.lst[ego_enc_now].nextenchant_ref;

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
void kill_character( const CHR_REF by_reference ichr, const CHR_REF by_reference killer, bool_t ignore_invictus )
{
    /// @details BB@> Handle a character death. Set various states, disconnect it from the world, etc.

    ego_chr * pchr;
    ego_cap * pcap;
    int action;
    Uint16 experience;
    TEAM_REF killer_team;
    ego_ai_bundle tmp_bdl_ai;

    if ( !DEFINED_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

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
    pchr->inst.action_keep = btrue;

    // Give kill experience
    experience = pcap->experience_worth + ( pchr->experience * pcap->experience_exchange );

    // Set target
    pchr->ai.target = INGAME_CHR( killer ) ? killer : MAX_CHR;
    if ( killer_team == TEAM_DAMAGE ) pchr->ai.target = MAX_CHR;
    if ( killer_team == TEAM_NULL ) pchr->ai.target = ichr;

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

    CHR_BEGIN_LOOP_ACTIVE( tnc, plistener )
    {
        if ( !plistener->alive ) continue;

        // All allies get team experience, but only if they also hate the dead guy's team
        if ( tnc != killer && !team_hates_team( plistener->team, killer_team ) && team_hates_team( plistener->team, pchr->team ) )
        {
            give_experience( tnc, experience, XP_TEAMKILL, bfalse );
        }

        // Check if it was a leader
        if ( TeamStack.lst[pchr->team].leader == ichr && ego_chr::get_iteam( tnc ) == pchr->team )
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
    if ( VALID_PLA( pchr->is_which_player ) ) revivetimer = ONESECOND; // 1 second

    // Let its AI script run one last time
    pchr->ai.timer = update_wld + 1;            // Prevent IfTimeOut in scr_run_chr_script()
    scr_run_chr_script( ego_ai_bundle::set( &tmp_bdl_ai, pchr ) );

    // Stop any looped sounds
    if ( pchr->loopedsound_channel != INVALID_SOUND ) sound_stop_channel( pchr->loopedsound_channel );
    looped_stop_object_sounds( ichr );
    pchr->loopedsound_channel = INVALID_SOUND;
}

//--------------------------------------------------------------------------------------------
int damage_character_hurt( ego_chr_bundle * pbdl, int base_damage, int actual_damage, CHR_REF attacker, bool_t ignore_invictus )
{
    CHR_REF      loc_ichr;
    ego_chr      * loc_pchr;

    if ( 0 == actual_damage ) return 0;

    if ( NULL == pbdl || NULL == pbdl->chr_ptr ) return 0;

    // alias some variables
    loc_ichr = pbdl->chr_ref;
    loc_pchr = pbdl->chr_ptr;

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
        ego_chr::play_action( loc_pchr, randomize_action( ACTION_HA, 0 ), bfalse );
    }

    return actual_damage;
}

//--------------------------------------------------------------------------------------------
int damage_character_heal( ego_chr_bundle * pbdl, int heal_amount, CHR_REF attacker, bool_t ignore_invictus )
{
    CHR_REF      loc_ichr;
    ego_chr      * loc_pchr;
    ego_ai_state * loc_pai;

    if ( NULL == pbdl || NULL == pbdl->chr_ptr ) return 0;

    // alias some variables
    loc_ichr = pbdl->chr_ref;
    loc_pchr = pbdl->chr_ptr;
    loc_pai  = &( loc_pchr->ai );

    heal_character( loc_ichr, attacker, heal_amount, ignore_invictus );

    return heal_amount;
}

//--------------------------------------------------------------------------------------------
int damage_character( const CHR_REF by_reference character, FACING_T direction,
                      IPair  damage, Uint8 damagetype, TEAM_REF team,
                      CHR_REF attacker, BIT_FIELD effects, bool_t ignore_invictus )
{
    /// @details ZZ@> This function calculates and applies damage to a character.  It also
    ///    sets alerts and begins actions.  Blocking and frame invincibility
    ///    are done here too.  Direction is ATK_FRONT if the attack is coming head on,
    ///    ATK_RIGHT if from the right, ATK_BEHIND if from the back, ATK_LEFT if from the
    ///    left.

    ego_chr_bundle bdl;
    ego_chr      * loc_pchr;
    ego_cap      * loc_pcap;
    ego_ai_state * loc_pai;
    CHR_REF      loc_ichr;

    int     actual_damage, base_damage, max_damage;
    int     mana_damage;
    bool_t friendly_fire = bfalse, immune_to_damage = bfalse;
    bool_t do_feedback = ( FEEDBACK_OFF != cfg.feedback );

    if ( !INGAME_CHR( character ) ) return 0;
    loc_pchr = ChrList.lst + character;

    if ( NULL == ego_chr_bundle::set( &bdl, loc_pchr ) ) return 0;
    loc_pchr = bdl.chr_ptr;
    loc_pcap = bdl.cap_ptr;
    loc_ichr = bdl.chr_ref;
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
        if ( !ChrList.lst[attacker].StatusList_on )
        {
            do_feedback = bfalse;
        }

        // don't show damage to players since they get feedback from the status bars
        if ( loc_pchr->StatusList_on || VALID_PLA( loc_pchr->is_which_player ) )
        {
            do_feedback = bfalse;
        }
    }

    actual_damage = 0;
    mana_damage   = 0;
    max_damage    = ABS( damage.base ) + ABS( damage.rand );

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
            /// @note ZF@> Is double damage too much?
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
        tmp_final_mana = CLIP( tmp_final_mana, 0, loc_pchr->manamax );
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
        tmp_final_mana = CLIP( tmp_final_mana, 0, loc_pchr->manamax );
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
        if ( cfg.difficulty >= GAME_HARD && VALID_PLA( loc_pchr->is_which_player ) && !VALID_PLA( ChrList.lst[attacker].is_which_player ) )
        {
            actual_damage *= 1.25f;
        }

        // Easy mode deals 25% extra actual damage by players and 25% less to players
        if ( cfg.difficulty <= GAME_EASY )
        {
            if ( VALID_PLA( ChrList.lst[attacker].is_which_player ) && !VALID_PLA( loc_pchr->is_which_player ) )
            {
                actual_damage *= 1.25f;
            }

            if ( !VALID_PLA( ChrList.lst[attacker].is_which_player ) &&  VALID_PLA( loc_pchr->is_which_player ) )
            {
                actual_damage *= 0.75f;
            }
        }

        if ( HAS_NO_BITS( DAMFX_ARMO, effects ) )
        {
            actual_damage *= loc_pchr->defense * INV_FF;
        }

        // hurt the character
        actual_damage = damage_character_hurt( &bdl, base_damage, actual_damage, attacker, ignore_invictus );

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
                                    ( CHR_REF )MAX_CHR, GRIP_LAST, loc_pchr->team, loc_ichr, ( PRT_REF )TOTAL_MAX_PRT, 0, ( CHR_REF )MAX_CHR );
            }
        }

        // Set attack alert if it wasn't an accident
        if ( base_damage > HURTDAMAGE )
        {
            if ( team == TEAM_DAMAGE )
            {
                loc_pai->attacklast = ( CHR_REF )MAX_CHR;
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

        /// @test spawn a fly-away damage indicator?
        if ( do_feedback )
        {
            const char * tmpstr;
            int rank;

            tmpstr = describe_wounds( loc_pchr->lifemax, loc_pchr->life );

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
                    tmpstr = describe_damage( actual_damage, loc_pchr->lifemax, &rank );
                    if ( rank >= -1 && rank <= 1 )
                    {
                        tmpstr = describe_wounds( loc_pchr->lifemax, loc_pchr->life );
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
                snprintf( text_buffer, SDL_arraysize( text_buffer ), "%s", tmpstr );

                chr_make_text_billboard( loc_ichr, text_buffer, text_color, friendly_fire ? tint_friend : tint_enemy, lifetime, ( BIT_FIELD )bb_opt_all );
            }
        }
    }
    else if ( actual_damage < 0 )
    {
        int actual_heal = 0;

        // heal the character
        actual_heal = damage_character_heal( &bdl, -actual_damage, attacker, ignore_invictus );

        // Isssue an alert
        if ( team == TEAM_DAMAGE )
        {
            loc_pai->attacklast = ( CHR_REF )MAX_CHR;
        }

        /// @test spawn a fly-away heal indicator?
        if ( do_feedback )
        {
            const float lifetime = 3;
            STRING text_buffer = EMPTY_CSTR;

            // "white" text
            SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};

            // heal == yellow, right ;)
            GLXvector4f tint = { 1.00f, 1.00f, 0.75f, 1.00f };

            // write the string into the buffer
            snprintf( text_buffer, SDL_arraysize( text_buffer ), "%s", describe_value( actual_heal, damage.base + damage.rand, NULL ) );

            chr_make_text_billboard( loc_ichr, text_buffer, text_color, tint, lifetime, ( BIT_FIELD )bb_opt_all );
        }
    }

    return actual_damage;
}

//--------------------------------------------------------------------------------------------
void spawn_defense_ping( ego_chr *pchr, const CHR_REF by_reference attacker )
{
    /// @details ZF@> Spawn a defend particle
    if ( pchr->damagetime != 0 ) return;

    spawn_one_particle_global( pchr->pos, pchr->ori.facing_z, PIP_DEFEND, 0 );

    pchr->damagetime    = DEFENDTIME;
    ADD_BITS( pchr->ai.alert, ALERTIF_BLOCKED );
    pchr->ai.attacklast = attacker;                 // For the ones attacking a shield
}

//--------------------------------------------------------------------------------------------
void spawn_poof( const CHR_REF by_reference character, const PRO_REF by_reference profile )
{
    /// @details ZZ@> This function spawns a character poof

    FACING_T facing_z;
    CHR_REF  origin;
    int      cnt;

    ego_chr * pchr;
    ego_cap * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( profile );
    if ( NULL == pcap ) return;

    origin = pchr->ai.owner;
    facing_z   = pchr->ori.facing_z;
    for ( cnt = 0; cnt < pcap->gopoofprt_amount; cnt++ )
    {
        spawn_one_particle( pchr->pos_old, facing_z, profile, pcap->gopoofprt_pip,
                            ( CHR_REF )MAX_CHR, GRIP_LAST, pchr->team, origin, ( PRT_REF )TOTAL_MAX_PRT, cnt, ( CHR_REF )MAX_CHR );

        facing_z += pcap->gopoofprt_facingadd;
    }
}

//--------------------------------------------------------------------------------------------
void ego_ai_state::spawn( ego_ai_state * pself, const CHR_REF by_reference index, const PRO_REF by_reference iobj, Uint16 rank )
{
    ego_chr * pchr;
    ego_pro * ppro;
    ego_cap * pcap;

    pself = ego_ai_state::ctor( pself );

    if ( NULL == pself || !DEFINED_CHR( index ) ) return;
    pchr = ChrList.lst + index;

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
ego_chr * ego_chr::do_init( ego_chr * pchr )
{
    CHR_REF  ichr;
    CAP_REF  icap;
    TEAM_REF loc_team;
    int      tnc, iteam, kursechance;

    ego_cap * pcap;
    fvec3_t pos_tmp;

    if ( NULL == pchr ) return NULL;
    ichr = GET_INDEX_PCHR( pchr );

    // get the character profile pointer
    pcap = pro_get_pcap( pchr->spawn_data.profile );
    if ( NULL == pcap )
    {
        log_debug( "ego_chr::do_init() - cannot initialize character.\n" );

        return NULL;
    }

    // get the character profile index
    icap = pro_get_icap( pchr->spawn_data.profile );

    // make a copy of the data in pchr->spawn_data.pos
    pos_tmp = pchr->spawn_data.pos;

    // download all the values from the character pchr->spawn_data.profile
    chr_download_cap( pchr, pcap );

    // Make sure the pchr->spawn_data.team is valid
    loc_team = pchr->spawn_data.team;
    iteam = REF_TO_INT( loc_team );
    iteam = CLIP( iteam, 0, TEAM_MAX );
    loc_team = ( TEAM_REF )iteam;

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
    ego_ai_state::spawn( &( pchr->ai ), ichr, pchr->profile_ref, TeamStack.lst[loc_team].morale );

    // Team stuff
    pchr->team     = loc_team;
    pchr->baseteam = loc_team;
    if ( !IS_INVICTUS_PCHR_RAW( pchr ) )  TeamStack.lst[loc_team].morale++;

    // Firstborn becomes the leader
    if ( TeamStack.lst[loc_team].leader == NOLEADER )
    {
        TeamStack.lst[loc_team].leader = ichr;
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
        pchr->life = pchr->lifemax;
        pchr->mana = pchr->manamax;
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
        snprintf( pchr->name, SDL_arraysize( pchr->name ), "%s", pro_create_chop( pchr->spawn_data.profile ) );
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
                            ichr, GRIP_LAST + tnc, pchr->team, ichr, ( PRT_REF )TOTAL_MAX_PRT, tnc, ( CHR_REF )MAX_CHR );
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
            if ( SHOP_NOOWNER == ShopStack.lst[ishop].owner ) continue;

            if ( PassageStack_object_is_inside( ShopStack.lst[ishop].passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
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
    chr_spawn_instance( &( pchr->inst ), pchr->spawn_data.profile, pchr->spawn_data.skin );

    // force the calculation of the matrix
    ego_chr::update_matrix( pchr, btrue );

    // force the instance to update
    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, btrue );

    // determine whether the object is hidden
    ego_chr::update_hide( pchr );

#if EGO_DEBUG && defined(DEBUG_WAYPOINTS)
    if ( !IS_ATTACHED_PCHR( pchr ) && INFINITE_WEIGHT != pchr->phys.weight && !pchr->safe_valid )
    {
        log_warning( "spawn_one_character() - \n\tinitial spawn position <%f,%f> is \"inside\" a wall. Wall normal is <%f,%f>\n",
                     pchr->pos.x, pchr->pos.y, nrm.x, nrm.y );
    }
#endif

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
    if ( !pchr->is_hidden && pchr->pos.z < water.surface_level && ( 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_WATER ) ) )
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
                ripple_suppression -= (( int )pchr->vel.x != 0 ) | (( int )pchr->vel.y != 0 );

                if ( ripple_suppression > 0 )
                {
                    ripand = ~(( ~RIPPLEAND ) << ripple_suppression );
                }
                else
                {
                    ripand = RIPPLEAND >> ( -ripple_suppression );
                }

                if ( 0 == (( update_wld + pchr->obj_base.guid ) & ripand ) && pchr->pos.z < water.surface_level && pchr->alive )
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
            pchr->dismount_object = ( CHR_REF )MAX_CHR;
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
    pchr->inst.uoffset += pchr->uoffvel;
    pchr->inst.voffset += pchr->voffvel;

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
            pchr->mana = MAX( 0, MIN( pchr->mana, pchr->manamax ) );

            pchr->life += liferegen;
            pchr->life = MAX( 1, MIN( pchr->life, pchr->lifemax ) );
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
ego_chr * ego_chr::do_deinit( ego_chr * pchr )
{
    if ( NULL == pchr ) return pchr;

    /* nothing to do yet */

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::run_object_construct( ego_chr * pchr, int max_iterations )
{
    int                 iterations;
    ego_object * pbase;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( pbase->state.action > ( int )( ego_object_constructing + 1 ) )
    {
        ego_chr * tmp_chr = ego_chr::run_object_deconstruct( pchr, max_iterations );
        if ( tmp_chr == pchr ) return NULL;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state.action <= ego_object_constructing && iterations < max_iterations )
    {
        ego_chr * ptmp = ego_chr::run_object( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::run_object_initialize( ego_chr * pchr, int max_iterations )
{
    int                 iterations;
    ego_object * pbase;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( pbase->state.action > ( int )( ego_object_initializing + 1 ) )
    {
        ego_chr * tmp_chr = ego_chr::run_object_deconstruct( pchr, max_iterations );
        if ( tmp_chr == pchr ) return NULL;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state.action <= ego_object_initializing && iterations < max_iterations )
    {
        ego_chr * ptmp = ego_chr::run_object( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::run_object_activate( ego_chr * pchr, int max_iterations )
{
    int                 iterations;
    ego_object * pbase;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( pbase->state.action > ( int )( ego_object_processing + 1 ) )
    {
        ego_chr * tmp_chr = ego_chr::run_object_deconstruct( pchr, max_iterations );
        if ( tmp_chr == pchr ) return NULL;
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state.action < ego_object_processing && iterations < max_iterations )
    {
        ego_chr * ptmp = ego_chr::run_object( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    EGOBOO_ASSERT( pbase->state.action == ego_object_processing );
    if ( pbase->state.action == ego_object_processing )
    {
        ChrList_add_used( GET_INDEX_PCHR( pchr ) );
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::run_object_deinitialize( ego_chr * pchr, int max_iterations )
{
    int                 iterations;
    ego_object * pbase;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the character is already beyond this stage, deinitialize it
    if ( pbase->state.action > ( int )( ego_object_deinitializing + 1 ) )
    {
        return pchr;
    }
    else if ( pbase->state.action < ego_object_deinitializing )
    {
        ego_object::end_processing( pbase );
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state.action <= ego_object_deinitializing && iterations < max_iterations )
    {
        ego_chr * ptmp = ego_chr::run_object( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::run_object_deconstruct( ego_chr * pchr, int max_iterations )
{
    int                 iterations;
    ego_object * pbase;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if the character is already beyond this stage, do nothing
    if ( pbase->state.action > ( int )( ego_object_destructing + 1 ) )
    {
        return pchr;
    }
    else if ( pbase->state.action < ego_object_deinitializing )
    {
        // make sure that you deinitialize before destructing
        ego_object::end_processing( pbase );
    }

    iterations = 0;
    while ( NULL != pchr && pbase->state.action <= ego_object_destructing && iterations < max_iterations )
    {
        ego_chr * ptmp = ego_chr::run_object( pchr );
        if ( ptmp != pchr ) return NULL;
        iterations++;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::run_object( ego_chr * pchr )
{
    ego_object * pbase;

    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( FLAG_REQ_TERMINATION_PBASE( pbase ) )
    {
        pbase = ego_object::grant_terminate( pbase );
    }

    switch ( pbase->state.action )
    {
        default:
        case ego_object_nothing:
            /* no operation */
            break;

        case ego_object_constructing:
            pchr = ego_chr::do_object_constructing( pchr );
            break;

        case ego_object_initializing:
            pchr = ego_chr::do_object_initializing( pchr );
            break;

        case ego_object_processing:
            pchr = ego_chr::do_object_processing( pchr );
            break;

        case ego_object_deinitializing:
            pchr = ego_chr::do_object_deinitializing( pchr );
            break;

        case ego_object_destructing:
            pchr = ego_chr::do_object_destructing( pchr );
            break;

        case ego_object_waiting:
            /* do nothing */
            break;
    }

    if ( NULL == pchr )
    {
        pbase->update_guid = INVALID_UPDATE_GUID;
    }
    else if ( ego_object_processing == pbase->state.action )
    {
        pbase->update_guid = ChrList.update_guid;
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_object_constructing( ego_chr * pchr )
{
    /// @details BB@> initialize the character data to safe values
    ///     since we use memset(..., 0, ...), all = 0, = false, and = 0.0f
    ///     statements are redundant

    ego_object * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( pbase ) ) return pchr;

    // run the constructor
    pchr = ego_chr::ctor( pchr );
    if ( NULL == pchr ) return pchr;

    // move on to the next action
    ego_object::end_constructing( pbase );

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_object_initializing( ego_chr * pchr )
{
    ego_object * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_INITIALIZING_PBASE( pbase ) ) return pchr;

    // tell the game that we're spawning something
    POBJ_BEGIN_SPAWN( pchr );

    // run the initialization routine
    pchr = ego_chr::do_init( pchr );
    if ( NULL == pchr ) return NULL;

    // request that we be turned on
    pbase->req.turn_me_on = btrue;

    // do something about being turned on
    if ( 0 == chr_loop_depth )
    {
        ego_object::grant_on( pbase );
    }
    else
    {
        ChrList_add_activation( GET_INDEX_PCHR( pchr ) );
    }

    // move on to the next action
    ego_object::end_initializing( pbase );

    // this will only work after the object has been fully initialized
    if ( !LOADED_CAP( pchr->spawn_data.profile ) )
    {
        POBJ_ACTIVATE( pchr, "*UNKNOWN*" );
    }
    else
    {
        ego_cap * pcap = pro_get_pcap( pchr->profile_ref );
        if ( NULL != pcap )
        {
            POBJ_ACTIVATE( pchr, pcap->name );
        }
    }

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_object_processing( ego_chr * pchr )
{
    // there's nothing to configure if the object is active...

    ego_object * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_PROCESSING_PBASE( pbase ) ) return pchr;

    // do this here (instead of at the end of *_do_object_initializing()) so that
    // we are sure that the object is actually "on"
    POBJ_END_SPAWN( pchr );

    // run the main loop
    pchr = ego_chr::do_processing( pchr );
    if ( NULL == pchr ) return NULL;

    /* add stuff here */

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_object_deinitializing( ego_chr * pchr )
{
    /// @details BB@> deinitialize the character data

    ego_object * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return pchr;

    // make sure that the spawn is terminated
    POBJ_END_SPAWN( pchr );

    // run a deinitialization routine
    pchr = ego_chr::do_deinit( pchr );
    if ( NULL == pchr ) return NULL;

    // move on to the next action
    ego_object::end_deinitializing( pbase );

    // make sure the object is off
    pbase->state.on = bfalse;

    return pchr;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::do_object_destructing( ego_chr * pchr )
{
    /// @details BB@> deinitialize the character data

    ego_object * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pchr );
    if ( !VALID_PBASE( pbase ) ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_DESTRUCTING_PBASE( pbase ) ) return pchr;

    // make sure that the spawn is terminated
    POBJ_END_SPAWN( pchr );

    // run the destructor
    pchr = ego_chr::dtor( pchr );
    if ( NULL == pchr ) return NULL;

    // move on to the next action (dead)
    ego_object::end_destructing( pbase );

    return pchr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF spawn_one_character( fvec3_t pos, const PRO_REF by_reference profile, const TEAM_REF by_reference team,
                             Uint8 skin, FACING_T facing, const char *name, const CHR_REF by_reference override )
{
    /// @details ZZ@> This function spawns a character and returns the character's index number
    ///               if it worked, MAX_CHR otherwise

    CHR_REF   ichr;
    ego_chr   * pchr;

    // fix a "bad" name
    if ( NULL == name ) name = "";

    if ( profile >= MAX_PROFILE )
    {
        log_warning( "spawn_one_character() - profile value too large %d out of %d\n", REF_TO_INT( profile ), MAX_PROFILE );
        return ( CHR_REF )MAX_CHR;
    }

    if ( !LOADED_PRO( profile ) )
    {
        if ( profile > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", REF_TO_INT( profile ) );
        }
        return ( CHR_REF )MAX_CHR;
    }

    // allocate a new character
    ichr = ChrList_allocate( override );
    if ( !DEFINED_CHR( ichr ) )
    {
        log_warning( "spawn_one_character() - failed to spawn character (invalid index number %d?)\n", REF_TO_INT( ichr ) );
        return ( CHR_REF )MAX_CHR;
    }

    pchr = ChrList.lst + ichr;

    // just set the spawn info
    pchr->spawn_data.pos      = pos;
    pchr->spawn_data.profile  = profile;
    pchr->spawn_data.team     = team;
    pchr->spawn_data.skin     = skin;
    pchr->spawn_data.facing   = facing;
    strncpy( pchr->spawn_data.name, name, SDL_arraysize( pchr->spawn_data.name ) );
    pchr->spawn_data.override = override;

    // actually force the character to spawn
    ego_chr::run_object_activate( pchr, 100 );

#if defined(DEBUG_OBJECT_SPAWN) && EGO_DEBUG
    {
        CAP_REF icap = pro_get_icap( profile );
        log_debug( "spawn_one_character() - slot: %i, index: %i, name: %s, class: %s\n", REF_TO_INT( profile ), REF_TO_INT( ichr ), name, CapStack.lst[icap].classname );
    }
#endif

    return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function respawns a character

    CHR_REF item;
    int old_attached_prt_count, new_attached_prt_count;

    ego_chr * pchr;
    ego_cap * pcap;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->alive ) return;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return;

    old_attached_prt_count = number_of_attached_particles( character );

    spawn_poof( character, pchr->profile_ref );
    disaffirm_attached_particles( character );

    pchr->alive = btrue;
    pchr->boretime = BORETIME;
    pchr->carefultime = CAREFULTIME;
    pchr->life = pchr->lifemax;
    pchr->mana = pchr->manamax;
    ego_chr::set_pos( pchr, pchr->pos_stt.v );
    pchr->vel.x = 0;
    pchr->vel.y = 0;
    pchr->vel.z = 0;
    pchr->team = pchr->baseteam;
    pchr->canbecrushed = bfalse;
    pchr->ori.map_facing_y = MAP_TURN_OFFSET;  // These two mean on level surface
    pchr->ori.map_facing_x = MAP_TURN_OFFSET;
    if ( NOLEADER == TeamStack.lst[pchr->team].leader )  TeamStack.lst[pchr->team].leader = character;
    if ( !IS_INVICTUS_PCHR_RAW( pchr ) )  TeamStack.lst[pchr->baseteam].morale++;

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
    ego_chr::set_fly_height( pchr, pcap->fly_height );
    pchr->phys.bumpdampen = pcap->bumpdampen;

    pchr->ai.alert = ALERTIF_CLEANEDUP;
    pchr->ai.target = character;
    pchr->ai.timer  = 0;

    pchr->grogtime = 0;
    pchr->dazetime = 0;

    // Let worn items come back
    PACK_BEGIN_LOOP( item, pchr->pack.next )
    {
        if ( INGAME_CHR( item ) && ChrList.lst[item].isequipped )
        {
            ChrList.lst[item].isequipped = bfalse;
            ADD_BITS( ego_chr::get_pai( item )->alert, ALERTIF_PUTAWAY ); // same as ALERTIF_ATLASTWAYPOINT
        }
    }
    PACK_END_LOOP( item );

    // re-initialize the instance
    chr_spawn_instance( &( pchr->inst ), pchr->profile_ref, pchr->skin );
    ego_chr::update_matrix( pchr, btrue );

    // determine whether the object is hidden
    ego_chr::update_hide( pchr );

    if ( !pchr->is_hidden )
    {
        reaffirm_attached_particles( character );
        new_attached_prt_count = number_of_attached_particles( character );
    }

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, btrue );
}

//--------------------------------------------------------------------------------------------
int chr_change_skin( const CHR_REF by_reference character, Uint32 skin )
{
    ego_chr * pchr;
    ego_pro * ppro;
    ego_mad * pmad;
    ego_chr_instance * pinst;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr  = ChrList.lst + character;
    pinst = &( pchr->inst );

    ppro = ego_chr::get_ppro( character );

    pmad = pro_get_pmad( pchr->profile_ref );
    if ( NULL == pmad )
    {
        // make sure that the instance has a valid imad
        if ( NULL != ppro && !LOADED_MAD( pinst->imad ) )
        {
            if ( chr_instance_set_mad( pinst, ppro->imad ) )
            {
                ego_chr::update_collision_size( pchr, btrue );
            }
            pmad = ego_chr::get_pmad( character );
        }
    }

    if ( NULL == pmad )
    {
        pchr->skin     = 0;
        pinst->texture = TX_WATER_TOP;
    }
    else
    {
        TX_REF txref = ( TX_REF )TX_WATER_TOP;

        // do the best we can to change the skin
        if ( NULL == ppro || 0 == ppro->skins )
        {
            ppro->skins = 1;
            ppro->tex_ref[0] = TX_WATER_TOP;

            skin  = 0;
            txref = TX_WATER_TOP;
        }
        else
        {
            if ( skin > ppro->skins )
            {
                skin = 0;
            }

            txref = ppro->tex_ref[skin];
        }

        pchr->skin     = skin;
        pinst->texture = txref;
    }

    // If the we are respawning a player, then the camera needs to be reset
    if ( VALID_PLA( pchr->is_which_player ) )
    {
        ego_camera::reset_target( PCamera, PMesh );
    }

    return pchr->skin;
}

//--------------------------------------------------------------------------------------------
int change_armor( const CHR_REF by_reference character, int skin )
{
    /// @details ZZ@> This function changes the armor of the character

    ENC_REF enchant;
    int     iTmp;
    ego_cap * pcap;
    ego_chr * pchr;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

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

        enchant = EncList.lst[enchant].nextenchant_ref;
    }

    // Change the skin
    pcap = ego_chr::get_pcap( character );
    skin = chr_change_skin( character, skin );

    // Change stats associated with skin
    pchr->defense = pcap->defense[skin];

    for ( iTmp = 0; iTmp < DAMAGE_COUNT; iTmp++ )
    {
        pchr->damagemodifier[iTmp] = pcap->damagemodifier[iTmp][skin];
    }

    // set teh character's maximum acceleration
    ego_chr::set_maxaccel( pchr, pcap->maxaccel[skin] );

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // Reset armor enchantments
    /// @todo These should really be done in reverse order ( Start with last enchant ), but
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

        enchant = EncList.lst[enchant].nextenchant_ref;
    }

    return skin;
}

//--------------------------------------------------------------------------------------------
void change_character_full( const CHR_REF by_reference ichr, const PRO_REF by_reference profile, Uint8 skin, Uint8 leavewhich )
{
    /// @details ZF@> This function polymorphs a character permanently so that it can be exported properly
    /// A character turned into a frog with this function will also export as a frog!

    MAD_REF imad_old, imad_new;

    if ( !LOADED_PRO( profile ) ) return;

    imad_new = pro_get_imad( profile );
    if ( !LOADED_MAD( imad_new ) ) return;

    imad_old = ego_chr::get_imad( ichr );
    if ( !LOADED_MAD( imad_old ) ) return;

    // copy the new name
    strncpy( MadStack.lst[imad_old].name, MadStack.lst[imad_new].name, SDL_arraysize( MadStack.lst[imad_old].name ) );

    // change their model
    change_character( ichr, profile, skin, leavewhich );

    // set the base model to the new model, too
    ChrList.lst[ichr].basemodel_ref = profile;
}

//--------------------------------------------------------------------------------------------
bool_t set_weapongrip( const CHR_REF by_reference iitem, const CHR_REF by_reference iholder, Uint16 vrt_off )
{
    int i;

    bool_t needs_update;
    Uint16 grip_verts[GRIP_VERTS];

    ego_matrix_cache * mcache;
    ego_chr * pitem;

    needs_update = bfalse;

    if ( !INGAME_CHR( iitem ) ) return bfalse;
    pitem = ChrList.lst + iitem;
    mcache = &( pitem->inst.matrix_cache );

    // is the item attached to this valid holder?
    if ( pitem->attachedto != iholder ) return bfalse;

    needs_update  = btrue;

    if ( GRIP_VERTS == get_grip_verts( grip_verts, iholder, vrt_off ) )
    {
        //---- detect any changes in the matrix_cache data

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
void change_character( const CHR_REF by_reference ichr, const PRO_REF by_reference profile_new, Uint8 skin, Uint8 leavewhich )
{
    /// @details ZZ@> This function polymorphs a character, changing stats, dropping weapons

    int tnc;
    ENC_REF enchant;
    CHR_REF item_ref, item;
    ego_chr * pchr;

    ego_pro * pobj_new;
    ego_cap * pcap_new;
    ego_mad * pmad_new;

    int old_attached_prt_count, new_attached_prt_count;

    if ( !LOADED_PRO( profile_new ) || !INGAME_CHR( ichr ) ) return;
    pchr = ChrList.lst + ichr;

    old_attached_prt_count = number_of_attached_particles( ichr );

    if ( !LOADED_PRO( profile_new ) ) return;
    pobj_new = ProList.lst + profile_new;

    pcap_new = pro_get_pcap( profile_new );
    pmad_new = pro_get_pmad( profile_new );

    // Drop left weapon
    item_ref = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( item_ref ) && ( !pcap_new->slotvalid[SLOT_LEFT] || pcap_new->ismount ) )
    {
        detach_character_from_mount( item_ref, btrue, btrue );
        detach_character_from_platform( ChrList.lst + item_ref );

        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            ChrList.lst[item_ref].vel.z    = DISMOUNTZVEL;
            ChrList.lst[item_ref].jump_time = JUMP_DELAY;

            tmp_pos = ego_chr::get_pos( ChrList.lst + item_ref );
            tmp_pos.z += DISMOUNTZVEL;
            ego_chr::set_pos( ChrList.lst + item_ref, tmp_pos.v );
        }
    }

    // Drop right weapon
    item_ref = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( item_ref ) && !pcap_new->slotvalid[SLOT_RIGHT] )
    {
        detach_character_from_mount( item_ref, btrue, btrue );
        detach_character_from_platform( ChrList.lst + item_ref );

        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            ChrList.lst[item_ref].vel.z    = DISMOUNTZVEL;
            ChrList.lst[item_ref].jump_time = JUMP_DELAY;

            tmp_pos = ego_chr::get_pos( ChrList.lst + item_ref );
            tmp_pos.z += DISMOUNTZVEL;
            ego_chr::set_pos( ChrList.lst + item_ref, tmp_pos.v );
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
            enchant = EncList.lst[enchant].nextenchant_ref;
            while ( MAX_ENC != enchant )
            {
                remove_enchant( enchant, NULL );

                enchant = EncList.lst[enchant].nextenchant_ref;
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
    pchr->ammomax = pcap_new->ammomax;
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
    pchr->darkvision_level = ego_chr::get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
    pchr->see_invisible_level = pcap_new->see_invisible_level;

    /// @note BB@> changing this could be disastrous, in case you can't un-morph yourself???
    /// pchr->canusearcane          = pcap_new->canusearcane;
    /// @note ZF@> No, we want this, I have specifically scripted morph books to handle unmorphing
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
        pchr->phys.weight = MIN( itmp, MAX_WEIGHT );
    }

    // Image rendering
    pchr->uoffvel = pcap_new->uoffvel;
    pchr->voffvel = pcap_new->voffvel;

    // Movement
    pchr->anim_speed_sneak = pcap_new->anim_speed_sneak;
    pchr->anim_speed_walk  = pcap_new->anim_speed_walk;
    pchr->anim_speed_run   = pcap_new->anim_speed_run;

    // initialize the instance
    chr_spawn_instance( &( pchr->inst ), profile_new, skin );
    ego_chr::update_matrix( pchr, btrue );

    // Action stuff that must be down after chr_spawn_instance()
    pchr->inst.action_ready = bfalse;
    pchr->inst.action_keep  = bfalse;
    pchr->inst.action_loop  = bfalse;
    if ( pchr->alive )
    {
        ego_chr::play_action( pchr, ACTION_DA, bfalse );
    }
    else
    {
        ego_chr::play_action( pchr, ACTION_KA + generate_randmask( 0, 3 ), bfalse );
        pchr->inst.action_keep = btrue;
    }

    // Set the skin after changing the model in chr_spawn_instance()
    change_armor( ichr, skin );

    // Must set the weapon grip AFTER the model is changed in chr_spawn_instance()
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        set_weapongrip( ichr, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );
    }

    item = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( item ) )
    {
        EGOBOO_ASSERT( ChrList.lst[item].attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_LEFT );
    }

    item = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( item ) )
    {
        EGOBOO_ASSERT( ChrList.lst[item].attachedto == ichr );
        set_weapongrip( item, ichr, GRIP_RIGHT );
    }

    // determine whether the object is hidden
    ego_chr::update_hide( pchr );

    // Reaffirm them particles...
    pchr->reaffirmdamagetype = pcap_new->attachedprt_reaffirmdamagetype;

    /// @note ZF@> remove this line so that books don't burn when dropped
    //reaffirm_attached_particles( ichr );

    new_attached_prt_count = number_of_attached_particles( ichr );

    ego_ai_state::set_changed( &( pchr->ai ) );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, btrue );
}

//--------------------------------------------------------------------------------------------
bool_t cost_mana( const CHR_REF by_reference character, int amount, const CHR_REF by_reference killer )
{
    /// @details ZZ@> This function takes mana from a character ( or gives mana ),
    ///    and returns btrue if the character had enough to pay, or bfalse
    ///    otherwise. This can kill a character in hard mode.

    int mana_final;
    bool_t mana_paid;

    ego_chr * pchr;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

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

        if ( mana_final > pchr->manamax )
        {
            mana_surplus = mana_final - pchr->manamax;
            pchr->mana   = pchr->manamax;
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
void switch_team( const CHR_REF by_reference character, const TEAM_REF by_reference team )
{
    /// @details ZZ@> This function makes a character join another team...

    if ( !INGAME_CHR( character ) || team >= TEAM_MAX ) return;

    if ( !IS_INVICTUS_PCHR_RAW( ChrList.lst + character ) )
    {
        if ( ego_chr::get_pteam_base( character )->morale > 0 ) ego_chr::get_pteam_base( character )->morale--;
        TeamStack.lst[team].morale++;
    }
    if (( !ChrList.lst[character].ismount || !INGAME_CHR( ChrList.lst[character].holdingwhich[SLOT_LEFT] ) ) &&
        ( !ChrList.lst[character].isitem  || !INGAME_CHR( ChrList.lst[character].attachedto ) ) )
    {
        ChrList.lst[character].team = team;
    }

    ChrList.lst[character].baseteam = team;
    if ( TeamStack.lst[team].leader == NOLEADER )
    {
        TeamStack.lst[team].leader = character;
    }
}

//--------------------------------------------------------------------------------------------
void issue_clean( const CHR_REF by_reference character )
{
    /// @details ZZ@> This function issues a clean up order to all teammates

    TEAM_REF team;

    if ( !INGAME_CHR( character ) ) return;

    team = ego_chr::get_iteam( character );

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
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
int restock_ammo( const CHR_REF by_reference character, IDSZ idsz )
{
    /// @details ZZ@> This function restocks the characters ammo, if it needs ammo and if
    ///    either its parent or type idsz match the given idsz.  This
    ///    function returns the amount of ammo given.

    int amount;

    ego_chr * pchr;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

    amount = 0;
    if ( ego_chr::is_type_idsz( character, idsz ) )
    {
        if ( ChrList.lst[character].ammo < ChrList.lst[character].ammomax )
        {
            amount = ChrList.lst[character].ammomax - ChrList.lst[character].ammo;
            ChrList.lst[character].ammo = ChrList.lst[character].ammomax;
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
int ego_chr::get_skill( ego_chr *pchr, IDSZ whichskill )
{
    /// @details ZF@> This returns the skill level for the specified skill or 0 if the character doesn't
    ///                  have the skill. Also checks the skill IDSZ.
    IDSZ_node_t *pskill;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

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
bool_t update_chr_darkvision( const CHR_REF by_reference character )
{
    /// @details BB@> as an offset to negative status effects like things like poisoning, a
    ///               character gains darkvision ability the more they are "poisoned".
    ///               True poisoning can be removed by [HEAL] and tints the character green
    ego_eve * peve;
    ENC_REF ego_enc_now, ego_enc_next;
    int life_regen = 0;

    ego_chr * pchr;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

    // cleanup the enchant list
    cleanup_character_enchants( pchr );

    // grab the life loss due poison to determine how much darkvision a character has earned, he he he!
    // clean up the enchant list before doing anything
    ego_enc_now = pchr->firstenchant;
    while ( ego_enc_now != MAX_ENC )
    {
        ego_enc_next = EncList.lst[ego_enc_now].nextenchant_ref;
        peve = ego_enc::get_peve( ego_enc_now );

        // Is it true poison?
        if ( NULL != peve && MAKE_IDSZ( 'H', 'E', 'A', 'L' ) == peve->removedbyidsz )
        {
            life_regen += EncList.lst[ego_enc_now].target_life;
            if ( EncList.lst[ego_enc_now].owner_ref == pchr->ai.index ) life_regen += EncList.lst[ego_enc_now].owner_life;
        }

        ego_enc_now = ego_enc_next;
    }

    if ( life_regen < 0 )
    {
        int tmp_level  = -( 10 * life_regen ) / pchr->lifemax;                        // Darkvision gained by poison
        int base_level = ego_chr::get_skill( pchr, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );    // Natural darkvision

        // Use the better of the two darkvision abilities
        pchr->darkvision_level = MAX( base_level, tmp_level );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void update_all_characters()
{
    /// @details ZZ@> This function updates stats and such for every character

    CHR_REF ichr;

    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ego_chr::run_object( ChrList.lst + ichr );
    }

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

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
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
    pweapon     = ChrList.lst + iweapon;
    pweapon_cap = ego_chr::get_pcap( iweapon );

    // grab the iweapon's action
    base_action = pweapon_cap->weaponaction;
    hand_action = randomize_action( base_action, which_slot );

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
            if ( !ego_chr::get_skill( pchr, ego_chr::get_idsz( iweapon, IDSZ_SKILL ) ) )
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
            weapon     = ChrList.lst + test_weapon;
            if ( weapon->iskursed ) allowedtoattack = bfalse;
        }
    }

    if ( !allowedtoattack )
    {
        if ( 0 == pweapon->reloadtime )
        {
            // This character can't use this iweapon
            pweapon->reloadtime = 50;
            if ( pchr->StatusList_on || cfg.dev_mode )
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
        CHR_REF mount = pchr->attachedto;

        if ( INGAME_CHR( mount ) )
        {
            ego_chr * pmount = ChrList.lst + mount;
            ego_cap * pmount_cap = ego_chr::get_pcap( mount );

            // let the mount steal the rider's attack
            if ( !pmount_cap->ridercanattack ) allowedtoattack = bfalse;

            // can the mount do anything?
            if ( pmount->ismount && pmount->alive )
            {
                // can the mount be told what to do?
                if ( !VALID_PLA( pmount->is_which_player ) && pmount->inst.action_ready )
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
        if ( pchr->inst.action_ready && action_valid )
        {
            // Check mana cost
            bool_t mana_paid = cost_mana( ichr, pweapon->manacost, iweapon );

            if ( mana_paid )
            {
                Uint32 action_madfx = 0;
                bool_t action_uses_MADFX_ACT = bfalse;

                // Check life healing
                pchr->life += pweapon->life_heal;
                if ( pchr->life > pchr->lifemax )  pchr->life = pchr->lifemax;

                // randomize the action
                action = randomize_action( action, which_slot );

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
                    pchr->inst.rate = 0.125f;                       // base attack speed
                    pchr->inst.rate += chr_dex / 40;                // +0.25f for every 10 dexterity

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
ego_chr_bundle * chr_do_latch_button( ego_chr_bundle * pbdl )
{
    /// @details BB@> Character latches for generalized buttons

    CHR_REF       loc_ichr;
    ego_chr       * loc_pchr;
    ego_phys_data * loc_pphys;
    ego_ai_state  * loc_pai;

    CHR_REF item;
    bool_t attack_handled;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_ichr    = pbdl->chr_ref;
    loc_pchr    = pbdl->chr_ptr;
    loc_pphys   = &( loc_pchr->phys );
    loc_pai     = &( loc_pchr->ai );

    if ( !loc_pchr->alive || 0 == loc_pchr->latch.trans.b ) return pbdl;

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

        if ( INGAME_CHR( loc_pchr->attachedto ) )
        {
            // Jump from our mount

            ego_chr * pmount = ChrList.lst + loc_pchr->attachedto;

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
            if ( loc_pchr->inst.action_ready )
            {
                ego_chr::play_action( loc_pchr, ACTION_JA, btrue );
            }

            // down the jump counter
            if ( JUMP_NUMBER_INFINITE != loc_pchr->jump_number_reset && loc_pchr->jump_number >= 0 )
            {
                loc_pchr->jump_number--;
            }

            // Play the jump sound (Boing!)
            if ( NULL != pbdl->cap_ptr )
            {
                ijump = pbdl->cap_ptr->sound_index[SOUND_JUMP];
                if ( VALID_SND( ijump ) )
                {
                    sound_play_chunk( loc_pchr->pos, ego_chr::get_chunk_ptr( loc_pchr, ijump ) );
                }
            }
        }
    }

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_ALTLEFT ) && loc_pchr->inst.action_ready && 0 == loc_pchr->reloadtime )
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

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_ALTRIGHT ) && loc_pchr->inst.action_ready && 0 == loc_pchr->reloadtime )
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

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_PACKLEFT ) && loc_pchr->inst.action_ready && 0 == loc_pchr->reloadtime )
    {
        loc_pchr->reloadtime = PACKDELAY;
        item = loc_pchr->holdingwhich[SLOT_LEFT];

        if ( INGAME_CHR( item ) )
        {
            ego_chr * pitem = ChrList.lst + item;

            if (( pitem->iskursed || pro_get_pcap( pitem->profile_ref )->istoobig ) && !pro_get_pcap( pitem->profile_ref )->isequipment )
            {
                // The item couldn't be put away
                ADD_BITS( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
                if ( VALID_PLA( loc_pchr->is_which_player ) )
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

    if ( HAS_SOME_BITS( loc_pchr->latch.trans.b, LATCHBUTTON_PACKRIGHT ) && loc_pchr->inst.action_ready && 0 == loc_pchr->reloadtime )
    {
        loc_pchr->reloadtime = PACKDELAY;
        item = loc_pchr->holdingwhich[SLOT_RIGHT];
        if ( INGAME_CHR( item ) )
        {
            ego_chr * pitem     = ChrList.lst + item;
            ego_cap * pitem_cap = ego_chr::get_pcap( item );

            if (( pitem->iskursed || pitem_cap->istoobig ) && !pitem_cap->isequipment )
            {
                // The item couldn't be put away
                ADD_BITS( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
                if ( VALID_PLA( loc_pchr->is_which_player ) )
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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_handle_madfx( ego_chr * pchr )
{
    ///@details This handles special commands an animation frame might execute, for example
    ///         grabbing stuff or spawning attack particles.
    CHR_REF ichr;
    Uint32 framefx;

    if ( NULL == pchr ) return bfalse;

    ichr    = GET_REF_PCHR( pchr );
    framefx = ego_chr::get_framefx( pchr );

    // Check frame effects
    if ( HAS_SOME_BITS( framefx, MADFX_ACTLEFT ) )
    {
        character_swipe( ichr, SLOT_LEFT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_ACTRIGHT ) )
    {
        character_swipe( ichr, SLOT_RIGHT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, bfalse );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, bfalse );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARLEFT ) )
    {
        character_grab_stuff( ichr, GRIP_LEFT, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARRIGHT ) )
    {
        character_grab_stuff( ichr, GRIP_RIGHT, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPLEFT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_LEFT], bfalse, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPRIGHT ) )
    {
        detach_character_from_mount( pchr->holdingwhich[SLOT_RIGHT], bfalse, btrue );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_POOF ) && !VALID_PLA( pchr->is_which_player ) )
    {
        pchr->ai.poof_time = update_wld;
    }

    if ( HAS_SOME_BITS( framefx, MADFX_FOOTFALL ) )
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
    /// @details BB@> Sort MOD REF values based on the rank of the module that they point to.
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

    retval = ( int )prhs->allowed - ( int )plhs->allowed;
    if ( 0 != retval ) return retval;

    retval = SGN( plhs->speed - prhs->speed );

    return retval;
}

//--------------------------------------------------------------------------------------------
float set_character_animation_rate( ego_chr * pchr )
{
    /// @details ZZ@> Get running, walking, sneaking, or dancing, from speed
    ///
    /// @note BB@> added automatic calculation of variable animation rates for movement animations

    float  speed;
    int    action, lip;
    bool_t can_be_interrupted;
    bool_t is_walk_type;
    bool_t found;

    int           frame_count;
    ego_MD2_Frame * frame_list, * pframe_nxt;

    ego_chr_instance * pinst;
    ego_mad          * pmad;
    CHR_REF          ichr;

    // set the character speed to zero
    speed = 0;

    if ( NULL == pchr ) return 1.0f;
    pinst = &( pchr->inst );
    ichr  = GET_REF_PCHR( pchr );

    // get the model
    pmad = ego_chr::get_pmad( ichr );
    if ( NULL == pmad ) return pinst->rate;

    // if the action is set to keep then do nothing
    can_be_interrupted = !pinst->action_keep;
    if ( !can_be_interrupted ) return pinst->rate = 1.0f;

    // don't change the rate if it is an attack animation
    if ( chr_is_attacking( pchr ) )    return pinst->rate;

    // if the animation is not a walking-type animation, ignore the variable animation rates
    // and the automatic determination of the walk animation
    // "dance" is walking with zero speed
    is_walk_type = ACTION_IS_TYPE( pinst->action_which, D ) || ACTION_IS_TYPE( pinst->action_which, W );
    if ( !is_walk_type ) return pinst->rate = 1.0f;

    // if the action cannot be changed on the at this time, there's nothing to do.
    // keep the same animation rate
    if ( !pinst->action_ready )
    {
        if ( 0.0f == pinst->rate ) pinst->rate = 1.0f;
        return pinst->rate;
    }

    // go back to a base animation rate, in case the next frame is not a
    // "variable speed frame"
    pinst->rate = 1.0f;

    // for non-flying objects, you have to be touching the ground
    if ( !pchr->enviro.grounded && !pchr->is_flying_platform ) return pinst->rate;

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
    if ( ABS( speed ) > 1.0f )
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

            /// @note ZF@> small fix here, if there is no sneak animation, try to default to normal walk with reduced animation speed
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
            return pinst->rate;
        }

        // sort the allowed movement(s) data
        qsort( anim_info, CHR_MOVEMENT_COUNT, sizeof( ego_chr_anim_data ), cmp_chr_anim_data );

        // handle a special case
        if ( 1 == anim_count )
        {
            if ( 0.0f != anim_info[0].speed )
            {
                pinst->rate = speed / anim_info[0].speed ;
            }

            return pinst->rate;
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
                    pinst->rate = speed / anim_info[cnt].speed;
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
                pinst->rate = speed / anim_info[cnt].speed;
            }
            found = btrue;
        }

        if ( !found )
        {
            return pinst->rate;
        }
    }

    frame_count = md2_get_numFrames( pmad->md2_ptr );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmad->md2_ptr );
    pframe_nxt  = frame_list + pinst->frame_nxt;

    EGOBOO_ASSERT( pinst->frame_nxt < frame_count );

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
            tmp_action = mad_get_action( pinst->imad, ACTION_DB + ( rand_val % 3 ) );
            ego_chr::start_anim( pchr, tmp_action, btrue, btrue );
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if ( !ACTION_IS_TYPE( pinst->action_which, D ) )
            {
                // get an appropriate version of the boredom action
                int tmp_action = mad_get_action( pinst->imad, ACTION_DA );

                // start the animation
                ego_chr::start_anim( pchr, tmp_action, btrue, btrue );
            }
        }
    }
    else
    {
        int tmp_action = mad_get_action( pinst->imad, action );
        if ( ACTION_COUNT != tmp_action )
        {
            if ( pinst->action_which != tmp_action )
            {
                ego_chr::set_anim( pchr, tmp_action, pmad->frameliptowalkframe[lip][pframe_nxt->framelip], btrue, btrue );
            }

            // "loop" the action
            pinst->action_next = tmp_action;
        }
    }

    pinst->rate = CLIP( pinst->rate, 0.1f, 10.0f );

    return pinst->rate;
}

//--------------------------------------------------------------------------------------------
bool_t chr_is_attacking( ego_chr *pchr )
{
    return pchr->inst.action_which >= ACTION_UA && pchr->inst.action_which <= ACTION_FD;
}

//--------------------------------------------------------------------------------------------
bool_t chr_calc_environment( ego_chr * pchr )
{
    ego_chr_bundle  bdl, *retval;

    if ( NULL == pchr ) return bfalse;

    retval = move_one_character_get_environment( ego_chr_bundle::set( &bdl, pchr ), NULL );

    return NULL != retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_get_environment( ego_chr_bundle * pbdl, ego_chr_environment * penviro )
{
    Uint32 itile = INVALID_TILE;

    ego_chr             * loc_pchr;
    ego_phys_data       * loc_pphys;
    ego_chr_environment * loc_penviro;
    ego_ai_state        * loc_pai;

    float   grid_level, water_level;
    ego_chr * pplatform = NULL;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr  = pbdl->chr_ptr;
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
        pplatform = ChrList.lst + loc_pchr->onwhichplatform_ref;
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
    if ( 0 != mesh_test_fx( PMesh, loc_pchr->onwhichgrid, MPDFX_WATER ) )
    {
        loc_penviro->fly_level = MAX( loc_penviro->walk_level, water.surface_level );
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
    if ( mesh_grid_is_valid( PMesh, itile ) )
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
        loc_penviro->is_slippy = !loc_penviro->is_watery && ( 0 != mesh_test_fx( PMesh, loc_pchr->onwhichgrid, MPDFX_SLIPPY ) );
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
    if ( mesh_grid_is_valid( PMesh, loc_pchr->onwhichgrid ) && loc_penviro->is_slippy )
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
            loc_pchr->jump_number = MIN( 1, loc_pchr->jump_number_reset );
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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_fluid_friction( ego_chr_bundle * pbdl )
{
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    // no fluid friction for "mario platforms"
    if ( loc_pchr->platform && loc_pchr->is_flying_platform && INFINITE_WEIGHT == loc_pphys->weight ) return pbdl;

    // no fluid friction if you are riding or being carried
    if ( IS_ATTACHED_PCHR( loc_pchr ) ) return pbdl;

    // Apply fluid friction from last time
    {
        fvec3_t _tmp_vec = VECT3(
                               -loc_pchr->vel.x * ( 1.0f - loc_penviro->fluid_friction_hrz ),
                               -loc_pchr->vel.y * ( 1.0f - loc_penviro->fluid_friction_hrz ),
                               -loc_pchr->vel.z * ( 1.0f - loc_penviro->fluid_friction_vrt ) );

        phys_data_accumulate_avel( loc_pphys, _tmp_vec.v );
    }

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_voluntary_flying( ego_chr_bundle * pbdl )
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

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_penviro = &( loc_pchr->enviro );
    loc_pori    = &( loc_pchr->ori );

    // auto-control a variety of factors based on the transition between running and flying
    jump_lerp = ( float )loc_pchr->jump_time / ( float )JUMP_DELAY;
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
    mdl_up = mat_getChrUp( loc_pchr->inst.matrix );
    mdl_rt = mat_getChrRight( loc_pchr->inst.matrix );
    mdl_fw = mat_getChrForward( loc_pchr->inst.matrix );

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
        speed_lerp = CLIP( ABS( speed_lerp ), 0.1f, 1.0f );
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
    yaw_factor = ABS( tmp_flt ) / maxspeed;
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
    loc_pori->map_facing_y += loc_latch.y * 0.125f * ( float )TRIG_TABLE_SIZE / TARGET_UPS * ( 1.0f - jump_lerp );

    // roll
    loc_pori->map_facing_x += loc_latch.x * 0.5f * ( float )TRIG_TABLE_SIZE / TARGET_UPS * ( 1.0f - jump_lerp );

    // set the desired acceleration
    loc_penviro->chr_acc = acc;

    // fake the desired velocity
    loc_penviro->chr_vel = fvec3_add( loc_pchr->vel.v, acc.v );

    return pbdl;
    //return move_one_character_limit_flying( pbdl );
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_voluntary_riding( ego_chr_bundle * pbdl )
{
    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_voluntary( ego_chr_bundle * pbdl )
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

    if ( NULL == pbdl || NULL == pbdl->chr_ptr ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    // clear the outputs of this routine every time through
    fvec3_self_clear( loc_penviro->chr_acc.v );
    fvec3_self_clear( loc_penviro->chr_vel.v );

    // non-active characters do not move themselves
    if ( !ACTIVE_PCHR( loc_pchr ) ) return pbdl;

    // if it is attached to another character, there is no voluntary motion
    if ( IS_ATTACHED_PCHR( loc_pchr ) )
    {
        return move_one_character_do_voluntary_riding( pbdl );
    }

    // if we are flying, interpret the controls differently
    if ( loc_pchr->is_flying_jump )
    {
        return move_one_character_do_voluntary_flying( pbdl );
    }

    // should we use the character's latch info?
    use_latch = loc_pchr->alive && !loc_pchr->isitem && loc_pchr->latch.trans_valid;
    if ( !use_latch ) return pbdl;

    // is this character a player?
    is_player = VALID_PLA( loc_pchr->is_which_player );

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
    if ( is_player )
    {
        // determine whether the user is hitting the "sneak button"
        ego_player * ppla = PlaStack.lst + loc_pchr->is_which_player;

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
        loc_pchr->movement_bits = ( unsigned )( ~CHR_MOVEMENT_BITS_SNEAK );
        loc_pchr->maxaccel      = loc_pchr->maxaccel_reset;
    }

    // determine the character's desired velocity from the latch
    if ( 0.0f != fvec3_length_abs( loc_latch.v ) )
    {
        float scale;

        dv2 = fvec3_length_2( loc_latch.v );

        // determine how response of the character to the latch
        scale = 1.0f;
        if ( is_player )
        {
            float dv;
            ego_player * ppla = PlaStack.lst + loc_pchr->is_which_player;

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
    if ( 0 != ( ego_chr::get_framefx( loc_pchr ) & MADFX_STOP ) )
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
        if ( 1.0f == ABS( loc_penviro->walk_nrm.z ) )
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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_involuntary( ego_chr_bundle * pbdl )
{
    /// Do the "non-physics" motion that the character has no control over

    bool_t use_latch;

    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
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
    //        vfront = mat_getChrRight( loc_pchr->inst.matrix );
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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_orientation( ego_chr_bundle * pbdl )
{
    // do voluntary motion

    CHR_REF             loc_ichr;
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_ai_state        * loc_pai;

    bool_t can_control_direction;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_ichr    = pbdl->chr_ref;
    loc_penviro = &( loc_pchr->enviro );
    loc_pai     = &( loc_pchr->ai );

    // set the old orientation
    loc_pchr->ori_old = loc_pchr->ori;

    // handle the special case of a mounted character.
    // the actual matrix is generated by the attachment points, but the scripts still use
    // the orientation values
    if ( INGAME_CHR( loc_pchr->attachedto ) )
    {
        ego_chr * pmount = ChrList.lst + loc_pchr->attachedto;

        loc_pchr->ori = pmount->ori;

        return pbdl;
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
                            if ( VALID_PLA( loc_pchr->is_which_player ) )
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
                        if ( loc_ichr != loc_pai->target )
                        {
                            fvec2_t loc_diff;

                            loc_diff.x = ChrList.lst[loc_pai->target].pos.x - loc_pchr->pos.x;
                            loc_diff.y = ChrList.lst[loc_pai->target].pos.y - loc_pchr->pos.y;

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
        FACING_T new_facing_x = MAP_TURN_OFFSET, new_facing_y = MAP_TURN_OFFSET;

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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_z_motion( ego_chr_bundle * pbdl )
{
    ego_chr             * loc_pchr;
    ego_phys_data       * loc_pphys;
    ego_chr_environment * loc_penviro;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // aliases for easier notiation
    loc_pchr    = pbdl->chr_ptr;
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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_animation( ego_chr_bundle * pbdl )
{
    float dflip, flip_diff;

    ego_chr          * loc_pchr;
    ego_chr_instance * loc_pinst;
    CHR_REF          loc_ichr;

    if ( NULL == pbdl || !INGAME_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr  = pbdl->chr_ptr;
    loc_ichr  = pbdl->chr_ref;
    loc_pinst = &( loc_pchr->inst );

    // Animate the character.
    // Right now there are 50/4 = 12.5 animation frames per second
    dflip       = 0.25f * loc_pinst->rate;
    flip_diff = fmod( loc_pinst->flip, 0.25f ) + dflip;

    while ( flip_diff >= 0.25f )
    {
        flip_diff -= 0.25f;

        // update the lips
        loc_pinst->ilip   = ( loc_pinst->ilip + 1 ) % 4;
        loc_pinst->flip  += 0.25f;

        // handle frame FX for the new frame
        if ( 3 == loc_pinst->ilip )
        {
            chr_handle_madfx( loc_pchr );
        }

        if ( 0 == loc_pinst->ilip )
        {
            if ( rv_success == ego_chr::increment_frame( loc_pchr ) )
            {
                loc_pinst->flip = fmod( loc_pinst->flip, 1.0f );
            }
            else
            {
                log_warning( "ego_chr::increment_frame() did not succeed" );
            }
        }
    }

    if ( flip_diff > 0.0f )
    {
        int ilip_new;

        // update the lips
        loc_pinst->flip  += flip_diff;
        ilip_new      = (( int )floor( loc_pinst->flip * 4 ) ) % 4;

        if ( ilip_new != loc_pinst->ilip )
        {
            loc_pinst->ilip = ilip_new;

            // handle frame FX for the new frame
            if ( 3 == loc_pinst->ilip )
            {
                chr_handle_madfx( loc_pchr );
            }

            if ( 0 == loc_pinst->ilip )
            {
                if ( rv_success == ego_chr::increment_frame( loc_pchr ) )
                {
                    loc_pinst->flip = fmod( loc_pinst->flip, 1.0f );
                }
                else
                {
                    log_warning( "ego_chr::increment_frame() did not succeed" );
                }
            }
        }
    }

    set_character_animation_rate( loc_pchr );

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_limit_flying( ego_chr_bundle * pbdl )
{
    // this should only be called by move_one_character_do_floor() after the
    // normal acceleration has been killed

    fvec3_t total_acc;

    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    return pbdl;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    if ( !IS_FLYING_PCHR( loc_pchr ) ) return pbdl;

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

                if ( ABS( total_acc_para_len ) > 0.0f )
                {
                    // limit the tangential acceleration
                    fvec3_self_normalize_to( total_acc.v, ABS( total_acc_para_len ) );
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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_jump( ego_chr_bundle * pbdl )
{
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    fvec3_t total_acc, total_acc_para, total_acc_perp;
    float coeff_para, coeff_perp;

    float jump_lerp, fric_lerp;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    if ( IS_FLYING_PCHR( loc_pchr ) ) return pbdl;

    // determine whether a character has recently jumped
    jump_lerp = ( float )loc_pchr->jump_time / ( float )JUMP_DELAY;
    jump_lerp = CLIP( jump_lerp, 0.0f, 1.0f );

    // if we have not jumped, there's nothing to do
    if ( 0.0f == jump_lerp ) return pbdl;

    // determine the amount of acceleration necessary to get the desired acceleration
    total_acc = fvec3_sub( loc_penviro->chr_acc.v, loc_pphys->avel.v );

    if ( 0.0f == fvec3_length_abs( total_acc.v ) ) return pbdl;

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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_flying( ego_chr_bundle * pbdl )
{
    ego_chr             * loc_pchr;
    ego_chr_environment * loc_penviro;
    ego_phys_data       * loc_pphys;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    if ( !IS_FLYING_PCHR( loc_pchr ) || IS_ATTACHED_PCHR( loc_pchr ) ) return pbdl;

    // apply the flying forces
    phys_data_accumulate_avel( loc_pphys, loc_penviro->chr_acc.v );

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * move_one_character_do_floor( ego_chr_bundle * pbdl )
{
    /// @details BB@> Friction is complicated when you want to have sliding characters :P
    ///
    /// @note really, for this to work properly, all the friction interaction should be stored in a list
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

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr    = pbdl->chr_ptr;
    loc_penviro = &( loc_pchr->enviro );
    loc_pphys   = &( loc_pchr->phys );

    // no floor friction if you are riding or being carried
    if ( IS_ATTACHED_PCHR( loc_pchr ) ) return pbdl;

    // exempt "mario platforms" from interaction with the floor
    if ( loc_pchr->is_flying_platform && INFINITE_WEIGHT == loc_pphys->weight ) return pbdl;

    fric_lerp = 1.0f;
    pplatform = NULL;
    if ( ACTIVE_CHR( loc_pchr->onwhichplatform_ref ) )
    {
        pplatform = ChrList.lst + loc_pchr->onwhichplatform_ref;

        // if the character has just dismounted from this platform, then there is less friction
        fric_lerp *= calc_dismount_lerp( loc_pchr, pplatform );
    }

    // determine whether the object is part of the scenery
    is_scenery_object = ( INFINITE_WEIGHT == loc_pphys->weight );
    scenery_grip = is_scenery_object ? 1.0f : ( float )loc_pphys->weight / ( float )MAX_WEIGHT;
    scenery_grip = CLIP( scenery_grip, 0.0f, 1.0f );

    // apply the acceleration from the "normal force"
    phys_data_apply_normal_acceleration( loc_pphys, loc_penviro->walk_nrm, 1.0f - scenery_grip, loc_penviro->walk_lerp, &normal_acc );

    // determine how much influence the floor has on the object
    fric_lerp *= 1.0f - loc_penviro->walk_lerp;

    // if we're not in touch with the ground, there is no walking
    if ( 0.0f == fric_lerp ) return pbdl;

    // if there is no friction with the ground, nothing to do
    if ( loc_penviro->ground_fric >= 1.0f ) return pbdl;

    // determine the amount of acceleration necessary to get the desired acceleration
    total_acc = fvec3_sub( loc_penviro->chr_acc.v, loc_pphys->avel.v );

    // if there is no total acceleration, there's nothing to do
    if ( 0.0f == fvec3_length_abs( total_acc.v ) ) return pbdl;

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
        coefficient = fric_lerp * loc_penviro->traction * est_max_friction_acc / ABS( gravity );

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
            coefficient           = fric_lerp * loc_penviro->traction * est_max_friction_acc  / ABS( gravity );
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

    return pbdl;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle *  move_one_character( ego_chr_bundle * pbdl )
{
    ego_chr * loc_pchr;

    if ( NULL == pbdl || !ACTIVE_PCHR( pbdl->chr_ptr ) ) return pbdl;

    // alias some variables
    loc_pchr = pbdl->chr_ptr;

    // calculate the acceleration from the last time-step
    loc_pchr->enviro.acc = fvec3_sub( loc_pchr->vel.v, loc_pchr->vel_old.v );

    // Character's old location
    loc_pchr->pos_old = ego_chr::get_pos( loc_pchr );
    loc_pchr->vel_old = loc_pchr->vel;
    loc_pchr->ori_old = loc_pchr->ori;

    // determine the character's environment
    // if this is not done, reflections will not update properly
    move_one_character_get_environment( pbdl, NULL );

    //---- none of the remaining functions are valid for attached characters
    if ( IS_ATTACHED_PCHR( loc_pchr ) ) return pbdl;

    // make the object want to continue its current motion, unless this is overridden
    if ( loc_pchr->latch.trans_valid )
    {
        loc_pchr->enviro.chr_vel = loc_pchr->vel;
    }

    // apply the fluid friction for the object
    move_one_character_do_fluid_friction( pbdl );

    // determine how the character would *like* to move
    move_one_character_do_voluntary( pbdl );

    // apply gravitational an buoyancy effects
    move_one_character_do_z_motion( pbdl );

    // determine how the character is being *forced* to move
    move_one_character_do_involuntary( pbdl );

    // read and apply any latch buttons
    chr_do_latch_button( pbdl );

    // allow the character to control its flight
    move_one_character_do_flying( pbdl );

    // allow the character to have some additional control over jumping
    move_one_character_do_jump( pbdl );

    // determine how the character can *actually* move
    move_one_character_do_floor( pbdl );

    // do the character animation and apply any MADFX found in the animation frames
    move_one_character_do_animation( pbdl );

    // set the rotation angles for the character
    move_one_character_do_orientation( pbdl );

    return pbdl;
}

//--------------------------------------------------------------------------------------------
void move_all_characters( void )
{
    /// @details ZZ@> This function handles character physics

    chr_stoppedby_tests = 0;

    // Move every character
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        ego_chr_bundle bdl;

        // prime the environment
        pchr->enviro.air_friction = air_friction;
        pchr->enviro.ice_friction = ice_friction;

        move_one_character( ego_chr_bundle::set( &bdl, pchr ) );
    }
    CHR_END_LOOP();

    // The following functions need to be called any time you actually change a charcter's position
    keep_weapons_with_holders();
    attach_all_particles();
    make_all_character_matrices( update_wld != 0 );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void cleanup_all_characters()
{
    CHR_REF cnt;

    // Do poofing
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ego_chr * pchr;
        bool_t time_out;

        if ( !VALID_CHR( cnt ) ) continue;
        pchr = ChrList.lst + cnt;

        time_out = ( pchr->ai.poof_time >= 0 ) && ( pchr->ai.poof_time <= ( Sint32 )update_wld );
        if ( !WAITING_PBASE( POBJ_GET_PBASE( pchr ) ) && !time_out ) continue;

        // detach the character from the game
        cleanup_one_character( pchr );

        // free the character's inventory
        free_inventory_in_game( cnt );

        // free the character
        free_one_character_in_game( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void increment_all_character_update_counters()
{
    CHR_REF cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ego_object * pbase;

        pbase = POBJ_GET_PBASE( ChrList.lst + cnt );
        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
bool_t is_invictus_direction( FACING_T direction, const CHR_REF by_reference character, Uint16 effects )
{
    FACING_T left, right;

    ego_chr * pchr;
    ego_cap * pcap;
    ego_mad * pmad;

    bool_t  is_invictus;

    if ( !INGAME_CHR( character ) ) return btrue;
    pchr = ChrList.lst + character;

    pmad = ego_chr::get_pmad( character );
    if ( NULL == pmad ) return btrue;

    pcap = ego_chr::get_pcap( character );
    if ( NULL == pcap ) return btrue;

    // if the invictus flag is set, we are invictus
    if ( IS_INVICTUS_PCHR_RAW( pchr ) ) return btrue;

    // if the effect is shield piercing, ignore shielding
    if ( HAS_SOME_BITS( effects, DAMFX_NBLOC ) ) return bfalse;

    // if the character's frame is invictus, then check the angles
    if ( HAS_SOME_BITS( ego_chr::get_framefx( pchr ), MADFX_INVICTUS ) )
    {
        // I Frame
        direction -= pcap->iframefacing;
        left       = ( FACING_T )(( int )0x00010000 - ( int )pcap->iframeangle );
        right      = pcap->iframeangle;

        // If using shield, use the shield invictus instead
        if ( ACTION_IS_TYPE( pchr->inst.action_which, P ) )
        {
            bool_t parry_left = ( pchr->inst.action_which < ACTION_PC );

            // Using a shield?
            if ( parry_left )
            {
                // Check left hand
                ego_cap * pcap_tmp = ego_chr::get_pcap( pchr->holdingwhich[SLOT_LEFT] );
                if ( NULL != pcap )
                {
                    left  = ( FACING_T )(( int )0x00010000 - ( int )pcap_tmp->iframeangle );
                    right = pcap_tmp->iframeangle;
                }
            }
            else
            {
                // Check right hand
                ego_cap * pcap_tmp = ego_chr::get_pcap( pchr->holdingwhich[SLOT_RIGHT] );
                if ( NULL != pcap )
                {
                    left  = ( FACING_T )(( int )0x00010000 - ( int )pcap_tmp->iframeangle );
                    right = pcap_tmp->iframeangle;
                }
            }
        }
    }
    else
    {
        // N Frame
        direction -= pcap->nframefacing;
        left       = ( FACING_T )(( int )0x00010000 - ( int )pcap->nframeangle );
        right      = pcap->nframeangle;
    }

    // Check that direction
    is_invictus = bfalse;
    if ( direction <= left && direction <= right )
    {
        is_invictus = btrue;
    }

    return is_invictus;
}

//--------------------------------------------------------------------------------------------
ego_chr_reflection_cache * chr_reflection_cache_init( ego_chr_reflection_cache * pcache )
{
    if ( NULL == pcache ) return pcache;

    memset( pcache, 0, sizeof( *pcache ) );

    pcache->alpha = 127;
    pcache->light = 255;

    return pcache;
}

//--------------------------------------------------------------------------------------------
void chr_instance_clear_cache( ego_chr_instance * pinst )
{
    /// @details BB@> force chr_instance_update_vertices() recalculate the vertices the next time
    ///     the function is called

    vlst_cache_init( &( pinst->save ) );

    matrix_cache_init( &( pinst->matrix_cache ) );

    chr_reflection_cache_init( &( pinst->ref ) );

    pinst->lighting_update_wld = 0;
    pinst->lighting_frame_all  = 0;
}

//--------------------------------------------------------------------------------------------
ego_chr_instance * chr_instance_dtor( ego_chr_instance * pinst )
{
    if ( NULL == pinst ) return pinst;

    chr_instance_free( pinst );

    EGOBOO_ASSERT( NULL == pinst->vrt_lst );

    memset( pinst, 0, sizeof( *pinst ) );

    return pinst;
}

//--------------------------------------------------------------------------------------------
ego_chr_instance * chr_instance_ctor( ego_chr_instance * pinst )
{
    Uint32 cnt;

    if ( NULL == pinst ) return pinst;

    memset( pinst, 0, sizeof( *pinst ) );

    // model parameters
    pinst->imad = MAX_MAD;
    pinst->vrt_count = 0;

    // set the initial cache parameters
    chr_instance_clear_cache( pinst );

    // Set up initial fade in lighting
    pinst->color_amb = 0;
    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        pinst->vrt_lst[cnt].color_dir = 0;
    }

    // clear out the matrix cache
    matrix_cache_init( &( pinst->matrix_cache ) );

    // the matrix should never be referenced if the cache is not valid,
    // but it never pays to have a 0 matrix...
    pinst->matrix = IdentityMatrix();

    // set the animation state
    pinst->rate         = 1.0f;
    pinst->action_next  = ACTION_DA;
    pinst->action_ready = btrue;                     // argh! this must be set at the beginning, script's spawn animations do not work!
    pinst->frame_nxt    = pinst->frame_lst = 0;

    // the vlst_cache parameters are not valid
    pinst->save.valid = bfalse;

    return pinst;
}

//--------------------------------------------------------------------------------------------
bool_t chr_instance_free( ego_chr_instance * pinst )
{
    if ( NULL == pinst ) return bfalse;

    EGOBOO_DELETE_ARY( pinst->vrt_lst );
    pinst->vrt_count = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_instance_alloc( ego_chr_instance * pinst, size_t vlst_size )
{
    if ( NULL == pinst ) return bfalse;

    chr_instance_free( pinst );

    if ( 0 == vlst_size ) return btrue;

    pinst->vrt_lst = EGOBOO_NEW_ARY( ego_GLvertex, vlst_size );
    if ( NULL != pinst->vrt_lst )
    {
        pinst->vrt_count = vlst_size;
    }

    return ( NULL != pinst->vrt_lst );
}

//--------------------------------------------------------------------------------------------
bool_t chr_instance_set_mad( ego_chr_instance * pinst, const MAD_REF by_reference imad )
{
    /// @details BB@> try to set the model used by the character instance.
    ///     If this fails, it leaves the old data. Just to be safe it
    ///     would be best to check whether the old modes is valid, and
    ///     if not, the data should be set to safe values...

    ego_mad * pmad;
    bool_t updated = bfalse;
    size_t vlst_size;

    if ( !LOADED_MAD( imad ) ) return bfalse;
    pmad = MadStack.lst + imad;

    if ( NULL == pmad || pmad->md2_ptr == NULL )
    {
        log_error( "Invalid pmad instance spawn. (Slot number %i)\n", imad );
        return bfalse;
    }

    if ( pinst->imad != imad )
    {
        updated = btrue;
        pinst->imad = imad;
    }

    // set the vertex size
    vlst_size = md2_get_numVertices( pmad->md2_ptr );
    if ( pinst->vrt_count != vlst_size )
    {
        updated = btrue;
        chr_instance_alloc( pinst, vlst_size );
    }

    // set the frames to frame 0 of this object's data
    if ( pinst->frame_nxt != 0 || pinst->frame_lst != 0 )
    {
        updated = btrue;
        pinst->frame_nxt = pinst->frame_lst = 0;

        // the vlst_cache parameters are not valid
        pinst->save.valid = bfalse;
    }

    if ( updated )
    {
        // update the vertex and lighting cache
        chr_instance_clear_cache( pinst );
        chr_instance_update_vertices( pinst, -1, -1, btrue );
    }

    return updated;
}

//--------------------------------------------------------------------------------------------
bool_t chr_instance_update_ref( ego_chr_instance * pinst, float grid_level, bool_t need_matrix )
{
    int trans_temp;

    if ( NULL == pinst ) return bfalse;

    if ( need_matrix )
    {
        // reflect the ordinary matrix
        apply_reflection_matrix( pinst, grid_level );
    }

    trans_temp = 255;
    if ( pinst->ref.matrix_valid )
    {
        float pos_z;

        // determine the reflection alpha
        pos_z = grid_level - pinst->ref.matrix.CNV( 3, 2 );
        if ( pos_z < 0 ) pos_z = 0;

        trans_temp -= (( int )pos_z ) >> 1;
        if ( trans_temp < 0 ) trans_temp = 0;

        trans_temp |= gfx.reffadeor;  // Fix for Riva owners
        trans_temp = CLIP( trans_temp, 0, 255 );
    }

    pinst->ref.alpha = ( pinst->alpha * trans_temp * INV_FF ) * 0.5f;
    pinst->ref.light = ( 255 == pinst->light ) ? 255 : ( pinst->light * trans_temp * INV_FF ) * 0.5f;

    pinst->ref.redshift = pinst->redshift + 1;
    pinst->ref.grnshift = pinst->grnshift + 1;
    pinst->ref.blushift = pinst->blushift + 1;

    pinst->ref.sheen    = pinst->sheen >> 1;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_spawn_instance( ego_chr_instance * pinst, const PRO_REF by_reference profile, Uint8 skin )
{
    Sint8 greensave = 0, redsave = 0, bluesave = 0;

    ego_pro * pobj;
    ego_cap * pcap;

    if ( NULL == pinst ) return bfalse;

    // Remember any previous color shifts in case of lasting enchantments
    greensave = pinst->grnshift;
    redsave   = pinst->redshift;
    bluesave  = pinst->blushift;

    // clear the instance
    chr_instance_ctor( pinst );

    if ( !LOADED_PRO( profile ) ) return bfalse;
    pobj = ProList.lst + profile;

    pcap = pro_get_pcap( profile );

    // lighting parameters
    pinst->texture   = pobj->tex_ref[skin];
    pinst->enviro    = pcap->enviro;
    pinst->alpha     = pcap->alpha;
    pinst->light     = pcap->light;
    pinst->sheen     = pcap->sheen;
    pinst->grnshift  = greensave;
    pinst->redshift  = redsave;
    pinst->blushift  = bluesave;

    // model parameters
    chr_instance_set_mad( pinst, pro_get_imad( profile ) );

    // set the initial action, all actions override it
    chr_instance_play_action( pinst, ACTION_DA, btrue );

    // upload these parameters to the reflection cache, but don't compute the matrix
    chr_instance_update_ref( pinst, 0, bfalse );

    return btrue;
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
        int islot = (( int )grip_off / GRIP_VERTS ) - 1;

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
    inventory_idsz[INVEN_NECK]  = MAKE_IDSZ( 'N', 'E', 'C', 'K' );
    inventory_idsz[INVEN_WRIS]  = MAKE_IDSZ( 'W', 'R', 'I', 'S' );
    inventory_idsz[INVEN_FOOT]  = MAKE_IDSZ( 'F', 'O', 'O', 'T' );
}

//--------------------------------------------------------------------------------------------
bool_t ego_ai_state::add_order( ego_ai_state * pai, Uint32 value, Uint16 counter )
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
BBOARD_REF chr_add_billboard( const CHR_REF by_reference ichr, Uint32 lifetime_secs )
{
    /// @details BB@> Attach a basic billboard to a character. You set the billboard texture
    ///     at any time after this. Returns the index of the billboard or INVALID_BILLBOARD
    ///     if the allocation fails.
    ///
    ///    must be called with a valid character, so be careful if you call this function from within
    ///    spawn_one_character()

    ego_chr * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( BBOARD_REF )INVALID_BILLBOARD;
    pchr = ChrList.lst + ichr;

    if ( INVALID_BILLBOARD != pchr->ibillboard )
    {
        BillboardList_free_one( REF_TO_INT( pchr->ibillboard ) );
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
ego_billboard_data * chr_make_text_billboard( const CHR_REF by_reference ichr, const char * txt, SDL_Color text_color, GLXvector4f tint, int lifetime_secs, BIT_FIELD opt_bits )
{
    ego_chr            * pchr;
    ego_billboard_data * pbb;
    int                rv;

    BBOARD_REF ibb = ( BBOARD_REF )INVALID_BILLBOARD;

    if ( !INGAME_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    // create a new billboard or override the old billboard
    ibb = chr_add_billboard( ichr, lifetime_secs );
    if ( INVALID_BILLBOARD == ibb ) return NULL;

    pbb = BillboardList_get_ptr( pchr->ibillboard );
    if ( NULL == pbb ) return pbb;

    rv = ego_billboard_data::printf_ttf( pbb, ui_getFont(), text_color, "%s", txt );

    if ( rv < 0 )
    {
        pchr->ibillboard = INVALID_BILLBOARD;
        BillboardList_free_one( REF_TO_INT( ibb ) );
        pbb = NULL;
    }
    else
    {
        // copy the tint over
        memmove( pbb->tint, tint, sizeof( GLXvector4f ) );

        if ( HAS_SOME_BITS( opt_bits, bb_opt_randomize_pos ) )
        {
            // make a random offset from the character
            pbb->offset[XX] = ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * GRID_SIZE / 5.0f;
            pbb->offset[YY] = ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * GRID_SIZE / 5.0f;
            pbb->offset[ZZ] = ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * GRID_SIZE / 5.0f;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_randomize_vel ) )
        {
            // make the text fly away in a random direction
            pbb->offset_add[XX] += ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * 2.0f * GRID_SIZE / lifetime_secs / TARGET_UPS;
            pbb->offset_add[YY] += ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * 2.0f * GRID_SIZE / lifetime_secs / TARGET_UPS;
            pbb->offset_add[ZZ] += ((( rand() << 1 ) - RAND_MAX ) / ( float )RAND_MAX ) * 2.0f * GRID_SIZE / lifetime_secs / TARGET_UPS;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_fade ) )
        {
            // make the billboard fade to transparency
            pbb->tint_add[AA] = -1.0f / lifetime_secs / TARGET_UPS;
        }

        if ( HAS_SOME_BITS( opt_bits, bb_opt_burn ) )
        {
            float minval, maxval;

            minval = MIN( MIN( pbb->tint[RR], pbb->tint[GG] ), pbb->tint[BB] );
            maxval = MAX( MAX( pbb->tint[RR], pbb->tint[GG] ), pbb->tint[BB] );

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
const char * ego_chr::get_name( const CHR_REF by_reference ichr, Uint32 bits )
{
    static STRING szName;

    if ( !DEFINED_CHR( ichr ) )
    {
        // the default name
        strncpy( szName, "Unknown", SDL_arraysize( szName ) );
    }
    else
    {
        ego_chr * pchr = ChrList.lst + ichr;
        ego_cap * pcap = pro_get_pcap( pchr->profile_ref );

        if ( pchr->nameknown )
        {
            snprintf( szName, SDL_arraysize( szName ), "%s", pchr->name );
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
                    lTmp = toupper( pcap->classname[0] );

                    if ( 'A' == lTmp || 'E' == lTmp || 'I' == lTmp || 'O' == lTmp || 'U' == lTmp )
                    {
                        article = "an";
                    }
                    else
                    {
                        article = "a";
                    }
                }

                snprintf( szName, SDL_arraysize( szName ), "%s %s", article, pcap->classname );
            }
            else
            {
                snprintf( szName, SDL_arraysize( szName ), "%s", pcap->classname );
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
        szName[0] = toupper( szName[0] );
    }

    return szName;
}

//--------------------------------------------------------------------------------------------
const char * ego_chr::get_gender_possessive( const CHR_REF by_reference ichr, char buffer[], size_t buffer_len )
{
    static STRING szTmp = EMPTY_CSTR;
    ego_chr * pchr;

    if ( NULL == buffer )
    {
        buffer = szTmp;
        buffer_len = SDL_arraysize( szTmp );
    }
    if ( 0 == buffer_len ) return buffer;

    if ( !DEFINED_CHR( ichr ) )
    {
        buffer[0] = CSTR_END;
        return buffer;
    }
    pchr = ChrList.lst + ichr;

    if ( GENDER_FEMALE == pchr->gender )
    {
        snprintf( buffer, buffer_len, "her" );
    }
    else if ( GENDER_MALE == pchr->gender )
    {
        snprintf( buffer, buffer_len, "his" );
    }
    else
    {
        snprintf( buffer, buffer_len, "its" );
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
const char * ego_chr::get_gender_name( const CHR_REF by_reference ichr, char buffer[], size_t buffer_len )
{
    static STRING szTmp = EMPTY_CSTR;
    ego_chr * pchr;

    if ( NULL == buffer )
    {
        buffer = szTmp;
        buffer_len = SDL_arraysize( szTmp );
    }
    if ( 0 == buffer_len ) return buffer;

    if ( !DEFINED_CHR( ichr ) )
    {
        buffer[0] = CSTR_END;
        return buffer;
    }
    pchr = ChrList.lst + ichr;

    if ( GENDER_FEMALE == pchr->gender )
    {
        snprintf( buffer, buffer_len, "female " );
    }
    else if ( GENDER_MALE == pchr->gender )
    {
        snprintf( buffer, buffer_len, "male " );
    }
    else
    {
        snprintf( buffer, buffer_len, " " );
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------
const char * ego_chr::get_dir_name( const CHR_REF by_reference ichr )
{
    static STRING buffer = EMPTY_CSTR;
    ego_chr * pchr;

    strncpy( buffer, "/debug", SDL_arraysize( buffer ) );

    if ( !DEFINED_CHR( ichr ) ) return buffer;
    pchr = ChrList.lst + ichr;

    if ( !LOADED_PRO( pchr->profile_ref ) )
    {
        char * sztmp;

        EGOBOO_ASSERT( bfalse );

        // copy the character's data.txt path
        strncpy( buffer, pchr->obj_base.base_name, SDL_arraysize( buffer ) );

        // the name should be "...some path.../data.txt"
        // grab the path

        sztmp = strstr( buffer, "/\\" );
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
egoboo_rv ego_chr::update_collision_size( ego_chr * pchr, bool_t update_matrix )
{
    ///< @details BB@> use this function to update the pchr->prt_cv and  pchr->chr_min_cv with
    ///<       values that reflect the best possible collision volume
    ///<
    ///< @note This function takes quite a bit of time, so it must only be called when the
    ///< vertices change because of an animation or because the matrix changes.
    ///<
    ///< @todo it might be possible to cache the src_vrts[] array used in this function.
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
    if ( rv_error == chr_instance_update_bbox( &( pchr->inst ) ) )
    {
        return rv_error;
    }

    // convert the point cloud in the ego_GLvertex array (pchr->inst.vrt_lst) to
    // a level 1 bounding box. Subtract off the position of the character
    memcpy( &bsrc, &( pchr->inst.bbox ), sizeof( bsrc ) );

    // convert the corners of the level 1 bounding box to a point cloud
    vrt_count = oct_bb_to_points( &bsrc, src_vrts, 16 );

    // transform the new point cloud
    TransformVertices( &( pchr->inst.matrix ), src_vrts, dst_vrts, vrt_count );

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

        pchr->chr_min_cv.mins[OCT_X ] = MAX( pchr->chr_min_cv.mins[OCT_X ], -pchr->bump.size );
        pchr->chr_min_cv.mins[OCT_Y ] = MAX( pchr->chr_min_cv.mins[OCT_Y ], -pchr->bump.size );
        pchr->chr_min_cv.maxs[OCT_X ] = MIN( pchr->chr_min_cv.maxs[OCT_X ],  pchr->bump.size );
        pchr->chr_min_cv.maxs[OCT_Y ] = MIN( pchr->chr_min_cv.maxs[OCT_Y ],  pchr->bump.size );

        pchr->chr_max_cv.mins[OCT_X ] = MIN( pchr->chr_max_cv.mins[OCT_X ], -pchr->bump.size );
        pchr->chr_max_cv.mins[OCT_Y ] = MIN( pchr->chr_max_cv.mins[OCT_Y ], -pchr->bump.size );
        pchr->chr_max_cv.maxs[OCT_X ] = MAX( pchr->chr_max_cv.maxs[OCT_X ],  pchr->bump.size );
        pchr->chr_max_cv.maxs[OCT_Y ] = MAX( pchr->chr_max_cv.maxs[OCT_Y ],  pchr->bump.size );
    }

    // only use pchr->bump.size_big if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->bump_stt.size_big >= 0.0f )
    {
        pchr->chr_cv.mins[OCT_YX] = -pchr->bump.size_big;
        pchr->chr_cv.mins[OCT_XY] = -pchr->bump.size_big;
        pchr->chr_cv.maxs[OCT_YX] =  pchr->bump.size_big;
        pchr->chr_cv.maxs[OCT_XY] =  pchr->bump.size_big;

        pchr->chr_min_cv.mins[OCT_YX] = MAX( pchr->chr_min_cv.mins[OCT_YX], -pchr->bump.size_big );
        pchr->chr_min_cv.mins[OCT_XY] = MAX( pchr->chr_min_cv.mins[OCT_XY], -pchr->bump.size_big );
        pchr->chr_min_cv.maxs[OCT_YX] = MIN( pchr->chr_min_cv.maxs[OCT_YX], pchr->bump.size_big );
        pchr->chr_min_cv.maxs[OCT_XY] = MIN( pchr->chr_min_cv.maxs[OCT_XY], pchr->bump.size_big );

        pchr->chr_max_cv.mins[OCT_YX] = MIN( pchr->chr_max_cv.mins[OCT_YX], -pchr->bump.size_big );
        pchr->chr_max_cv.mins[OCT_XY] = MIN( pchr->chr_max_cv.mins[OCT_XY], -pchr->bump.size_big );
        pchr->chr_max_cv.maxs[OCT_YX] = MAX( pchr->chr_max_cv.maxs[OCT_YX], pchr->bump.size_big );
        pchr->chr_max_cv.maxs[OCT_XY] = MAX( pchr->chr_max_cv.maxs[OCT_XY], pchr->bump.size_big );
    }

    // only use pchr->bump.height if it was overridden in data.txt through the [MODL] expansion
    if ( pchr->bump_stt.height >= 0.0f )
    {
        pchr->chr_cv.mins[OCT_Z ] = 0.0f;
        pchr->chr_cv.maxs[OCT_Z ] = pchr->bump.height * pchr->fat;

        pchr->chr_min_cv.mins[OCT_Z ] = MAX( pchr->chr_min_cv.mins[OCT_Z ], 0.0f );
        pchr->chr_min_cv.maxs[OCT_Z ] = MIN( pchr->chr_min_cv.maxs[OCT_Z ], pchr->bump.height   * pchr->fat );

        pchr->chr_max_cv.mins[OCT_Z ] = MIN( pchr->chr_max_cv.mins[OCT_Z ], 0.0f );
        pchr->chr_max_cv.maxs[OCT_Z ] = MAX( pchr->chr_max_cv.maxs[OCT_Z ], pchr->bump.height   * pchr->fat );
    }

    // raise the upper bound for platforms
    if ( pchr->platform )
    {
        pchr->chr_max_cv.maxs[OCT_Z] += PLATTOLERANCE;
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
    ego_oct_bb::downgrade( &bdst, pchr->bump_stt, pchr->bump, &( pchr->bump_1 ), NULL );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
const char* describe_value( float value, float maxval, int * rank_ptr )
{
    /// @details ZF@> This converts a stat number into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if ( cfg.feedback == FEEDBACK_NUMBER )
    {
        snprintf( retval, SDL_arraysize( retval ), "%2.1f", SFP8_TO_FLOAT(( int )value ) );
        return retval;
    }

    fval = ( 0 == maxval ) ? 1.0f : value / maxval;

    *rank_ptr = -5;
    strcpy( retval, "Unknown" );

    if ( fval >= .83 )        { strcpy( retval, "Godlike!" );   *rank_ptr =  8; }
    else if ( fval >= .66 ) { strcpy( retval, "Ultimate" );   *rank_ptr =  7; }
    else if ( fval >= .56 ) { strcpy( retval, "Epic" );       *rank_ptr =  6; }
    else if ( fval >= .50 ) { strcpy( retval, "Powerful" );   *rank_ptr =  5; }
    else if ( fval >= .43 ) { strcpy( retval, "Heroic" );     *rank_ptr =  4; }
    else if ( fval >= .36 ) { strcpy( retval, "Very High" );  *rank_ptr =  3; }
    else if ( fval >= .30 ) { strcpy( retval, "High" );       *rank_ptr =  2; }
    else if ( fval >= .23 ) { strcpy( retval, "Good" );       *rank_ptr =  1; }
    else if ( fval >= .17 ) { strcpy( retval, "Average" );    *rank_ptr =  0; }
    else if ( fval >= .11 ) { strcpy( retval, "Pretty Low" ); *rank_ptr = -1; }
    else if ( fval >= .07 ) { strcpy( retval, "Bad" );        *rank_ptr = -2; }
    else if ( fval >  .00 ) { strcpy( retval, "Terrible" );   *rank_ptr = -3; }
    else                    { strcpy( retval, "None" );       *rank_ptr = -4; }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char* describe_damage( float value, float maxval, int * rank_ptr )
{
    /// @details ZF@> This converts a damage value into a more descriptive word

    static STRING retval;

    float fval;
    int local_rank;

    if ( NULL == rank_ptr ) rank_ptr = &local_rank;

    if ( cfg.feedback == FEEDBACK_NUMBER )
    {
        snprintf( retval, SDL_arraysize( retval ), "%2.1f", SFP8_TO_FLOAT(( int )value ) );
        return retval;
    }

    fval = ( 0 == maxval ) ? 1.0f : value / maxval;

    *rank_ptr = -5;
    strcpy( retval, "Unknown" );

    if ( fval >= 1.50 )         { strcpy( retval, "Annihilation!" ); *rank_ptr =  5; }
    else if ( fval >= 1.00 ) { strcpy( retval, "Overkill!" );     *rank_ptr =  4; }
    else if ( fval >= 0.80 ) { strcpy( retval, "Deadly" );        *rank_ptr =  3; }
    else if ( fval >= 0.70 ) { strcpy( retval, "Crippling" );     *rank_ptr =  2; }
    else if ( fval >= 0.50 ) { strcpy( retval, "Devastating" );   *rank_ptr =  1; }
    else if ( fval >= 0.25 ) { strcpy( retval, "Hurtful" );       *rank_ptr =  0; }
    else if ( fval >= 0.10 ) { strcpy( retval, "A Scratch" );     *rank_ptr = -1; }
    else if ( fval >= 0.05 ) { strcpy( retval, "Ticklish" );      *rank_ptr = -2; }
    else if ( fval >= 0.00 ) { strcpy( retval, "Meh..." );        *rank_ptr = -3; }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char* describe_wounds( float max, float current )
{
    /// @details ZF@> This tells us how badly someone is injured

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
        snprintf( retval, SDL_arraysize( retval ), "%2.1f", SFP8_TO_FLOAT(( int )current ) );
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
TX_REF ego_chr::get_icon_ref( const CHR_REF by_reference item )
{
    /// @details BB@> Get the index to the icon texture (in TxTexture) that is supposed to be used with this object.
    ///               If none can be found, return the index to the texture of the null icon.

    size_t iskin;
    TX_REF icon_ref = ( TX_REF )ICON_NULL;
    bool_t is_spell_fx, is_book, draw_book;

    ego_cap * pitem_cap;
    ego_chr * pitem;
    ego_pro * pitem_pro;

    if ( !DEFINED_CHR( item ) ) return icon_ref;
    pitem = ChrList.lst + item;

    if ( !LOADED_PRO( pitem->profile_ref ) ) return icon_ref;
    pitem_pro = ProList.lst + pitem->profile_ref;

    pitem_cap = pro_get_pcap( pitem->profile_ref );
    if ( NULL == pitem_cap ) return icon_ref;

    // what do we need to draw?
    is_spell_fx = ( NO_SKIN_OVERRIDE != pitem_cap->spelleffect_type );     // the value of spelleffect_type == the skin of the book or -1 for not a spell effect
    is_book     = ( SPELLBOOK == pitem->profile_ref );

    /// @note ZF@> uncommented a part because this caused a icon bug when you were morphed and mounted
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
    /// @details BB@> initialize every character profile in the game

    CAP_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        ego_cap::init( CapStack.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void release_all_cap()
{
    /// @details BB@> release every character profile in the game

    CAP_REF cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        release_one_cap( cnt );
    };
}

//--------------------------------------------------------------------------------------------
bool_t release_one_cap( const CAP_REF by_reference icap )
{
    /// @details BB@> release any allocated data and return all values to safe values

    ego_cap * pcap;

    if ( !VALID_CAP_RANGE( icap ) ) return bfalse;
    pcap = CapStack.lst + icap;

    if ( !pcap->loaded ) return btrue;

    ego_cap::init( pcap );

    pcap->loaded  = bfalse;
    pcap->name[0] = CSTR_END;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_teams()
{
    /// @details ZZ@> This function makes everyone hate everyone else

    TEAM_REF teama, teamb;

    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        // Make the team hate everyone
        for ( teamb = 0; teamb < TEAM_MAX; teamb++ )
        {
            TeamStack.lst[teama].hatesteam[REF_TO_INT( teamb )] = btrue;
        }

        // Make the team like itself
        TeamStack.lst[teama].hatesteam[REF_TO_INT( teama )] = bfalse;

        // Set defaults
        TeamStack.lst[teama].leader = NOLEADER;
        TeamStack.lst[teama].sissy = 0;
        TeamStack.lst[teama].morale = 0;
    }

    // Keep the null team neutral
    for ( teama = 0; teama < TEAM_MAX; teama++ )
    {
        TeamStack.lst[teama].hatesteam[TEAM_NULL] = bfalse;
        TeamStack.lst[( TEAM_REF )TEAM_NULL].hatesteam[REF_TO_INT( teama )] = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
bool_t chr_teleport( const CHR_REF by_reference ichr, float x, float y, float z, FACING_T facing_z )
{
    /// @details BB@> Determine whether the character can be teleported to the specified location
    ///               and do it, if possible. Success returns btrue, failure returns bfalse;

    ego_chr  * pchr;
    FACING_T facing_old, facing_new;
    fvec3_t  pos_old, pos_new;
    bool_t   retval;

    if ( !INGAME_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;

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
bool_t ego_chr::request_terminate( const CHR_REF by_reference ichr )
{
    /// @details BB@> Mark this character for deletion

    if ( !DEFINED_CHR( ichr ) ) return bfalse;

    POBJ_REQUEST_TERMINATE( ChrList.lst + ichr );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_chr * ego_chr::update_hide( ego_chr * pchr )
{
    /// @details BB@> Update the hide state of the character. Should be called every time that
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
    /// @details BB@> do something tricky here

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
ego_matrix_cache * matrix_cache_init( ego_matrix_cache * mcache )
{
    /// @details BB@> clear out the matrix cache data

    int cnt;

    if ( NULL == mcache ) return mcache;

    memset( mcache, 0, sizeof( *mcache ) );

    mcache->type_bits = MAT_UNKNOWN;
    mcache->grip_chr  = ( CHR_REF )MAX_CHR;
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
bool_t ego_chr::matrix_valid( ego_chr * pchr )
{
    /// @details BB@> Determine whether the character has a valid matrix

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    // both the cache and the matrix need to be valid
    return pchr->inst.matrix_cache.valid && pchr->inst.matrix_cache.matrix_valid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int get_grip_verts( Uint16 grip_verts[], const CHR_REF by_reference imount, int vrt_offset )
{
    /// @details BB@> Fill the grip_verts[] array from the mount's data.
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

    if ( !INGAME_CHR( imount ) ) return 0;
    pmount = ChrList.lst + imount;

    pmount_mad = ego_chr::get_pmad( imount );
    if ( NULL == pmount_mad ) return 0;

    if ( 0 == pmount->inst.vrt_count ) return 0;

    //---- set the proper weapongrip vertices
    tnc = ( signed )pmount->inst.vrt_count - ( signed )vrt_offset;

    // if the starting vertex is less than 0, just take the first vertex
    if ( tnc < 0 )
    {
        grip_verts[0] = 0;
        return 1;
    }

    vrt_count = 0;
    for ( i = 0; i < GRIP_VERTS; i++ )
    {
        if ( tnc + i < pmount->inst.vrt_count )
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
bool_t ego_chr::get_matrix_cache( ego_chr * pchr, ego_matrix_cache * mc_tmp )
{
    /// @details BB@> grab the matrix cache data for a given character and put it into mc_tmp.

    bool_t handled;
    CHR_REF itarget, ichr;

    if ( NULL == mc_tmp ) return bfalse;
    if ( !DEFINED_PCHR( pchr ) ) return bfalse;
    ichr = GET_REF_PCHR( pchr );

    handled = bfalse;
    itarget = ( CHR_REF )MAX_CHR;

    // initialize xome parameters in case we fail
    mc_tmp->valid     = bfalse;
    mc_tmp->type_bits = MAT_UNKNOWN;

    mc_tmp->self_scale.x = mc_tmp->self_scale.y = mc_tmp->self_scale.z = pchr->fat;

    // handle the overlay first of all
    if ( !handled && pchr->is_overlay && ichr != pchr->ai.target && INGAME_CHR( pchr->ai.target ) )
    {
        // this will pretty much fail the cmp_matrix_cache() every time...

        ego_chr * ptarget = ChrList.lst + pchr->ai.target;

        // make sure we have the latst info from the target
        ego_chr::update_matrix( ptarget, btrue );

        // grab the matrix cache into from the character we are overlaying
        memcpy( mc_tmp, &( ptarget->inst.matrix_cache ), sizeof( ego_matrix_cache ) );

        // just in case the overlay's matrix cannot be corrected
        // then treat it as if it is not an overlay
        handled = mc_tmp->valid;
    }

    // this will happen if the overlay "failed" or for any non-overlay character
    if ( !handled )
    {
        // assume that the "target" of the MAT_CHARACTER data will be the character itself
        itarget = GET_REF_PCHR( pchr );

        //---- update the MAT_WEAPON data
        if ( DEFINED_CHR( pchr->attachedto ) )
        {
            ego_chr * pmount = ChrList.lst + pchr->attachedto;

            // make sure we have the latst info from the target
            ego_chr::update_matrix( pmount, btrue );

            // just in case the mount's matrix cannot be corrected
            // then treat it as if it is not mounted... yuck
            if ( pmount->inst.matrix_cache.matrix_valid )
            {
                mc_tmp->valid     = btrue;
                ADD_BITS( mc_tmp->type_bits, MAT_WEAPON );        // add in the weapon data

                mc_tmp->grip_chr  = pchr->attachedto;
                mc_tmp->grip_slot = pchr->inwhich_slot;
                get_grip_verts( mc_tmp->grip_verts, pchr->attachedto, slot_to_grip_offset( pchr->inwhich_slot ) );

                itarget = pchr->attachedto;
            }
        }

        //---- update the MAT_CHARACTER data
        if ( DEFINED_CHR( itarget ) )
        {
            ego_chr * ptarget = ChrList.lst + itarget;

            mc_tmp->valid   = btrue;
            ADD_BITS( mc_tmp->type_bits, MAT_CHARACTER );  // add in the MAT_CHARACTER-type data for the object we are "connected to"

            mc_tmp->rotate.x = CLIP_TO_16BITS( ptarget->ori.map_facing_x - MAP_TURN_OFFSET );
            mc_tmp->rotate.y = CLIP_TO_16BITS( ptarget->ori.map_facing_y - MAP_TURN_OFFSET );
            mc_tmp->rotate.z = ptarget->ori.facing_z;

            mc_tmp->pos = ego_chr::get_pos( ptarget );

            mc_tmp->grip_scale.x = mc_tmp->grip_scale.y = mc_tmp->grip_scale.z = ptarget->fat;
        }
    }

    return mc_tmp->valid;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_local_points( ego_chr * pholder, Uint16 grip_verts[], fvec4_t dst_point[] )
{
    /// @details ZZ@> a helper function for apply_one_weapon_matrix()

    int cnt, point_count;

    if ( NULL == grip_verts || NULL == dst_point ) return 0;

    if ( !ACTIVE_PCHR( pholder ) ) return 0;

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
        chr_instance_update_grip_verts( &( pholder->inst ), grip_verts, GRIP_VERTS );

        // copy the vertices into dst_point[]
        for ( point_count = 0, cnt = 0; cnt < GRIP_VERTS; cnt++, point_count++ )
        {
            Uint16 vertex = grip_verts[cnt];

            if ( 0xFFFF == vertex ) continue;

            dst_point[point_count].x = pholder->inst.vrt_lst[vertex].pos[XX];
            dst_point[point_count].y = pholder->inst.vrt_lst[vertex].pos[YY];
            dst_point[point_count].z = pholder->inst.vrt_lst[vertex].pos[ZZ];
            dst_point[point_count].w = 1.0f;
        }
    }

    return point_count;
}

//--------------------------------------------------------------------------------------------
int convert_grip_to_global_points( const CHR_REF by_reference iholder, Uint16 grip_verts[], fvec4_t   dst_point[] )
{
    /// @details ZZ@> a helper function for apply_one_weapon_matrix()

    ego_chr *   pholder;
    int       point_count;
    fvec4_t   src_point[GRIP_VERTS];

    if ( !INGAME_CHR( iholder ) ) return 0;
    pholder = ChrList.lst + iholder;

    // find the grip points in the character's "local" or "body-fixed" coordinates
    point_count = convert_grip_to_local_points( pholder, grip_verts, src_point );

    if ( 0 == point_count ) return 0;

    // use the math function instead of rolling out own
    TransformVertices( &( pholder->inst.matrix ), src_point, dst_point, point_count );

    return point_count;
}

//--------------------------------------------------------------------------------------------
bool_t apply_one_weapon_matrix( ego_chr * pweap, ego_matrix_cache * mc_tmp )
{
    /// @details ZZ@> Request that the data in the matrix cache be used to create a "character matrix".
    ///               i.e. a matrix that is not being held by anything.

    fvec4_t   nupoint[GRIP_VERTS];
    int       iweap_points;

    ego_chr * pholder;
    ego_matrix_cache * pweap_mcache;

    if ( NULL == mc_tmp || !mc_tmp->valid || 0 == ( MAT_WEAPON & mc_tmp->type_bits ) ) return bfalse;

    if ( !DEFINED_PCHR( pweap ) ) return bfalse;
    pweap_mcache = &( pweap->inst.matrix_cache );

    if ( !INGAME_CHR( mc_tmp->grip_chr ) ) return bfalse;
    pholder = ChrList.lst + mc_tmp->grip_chr;

    // make sure that the matrix is invalid in case of an error
    pweap_mcache->matrix_valid = bfalse;

    // grab the grip points in world coordinates
    iweap_points = convert_grip_to_global_points( mc_tmp->grip_chr, mc_tmp->grip_verts, nupoint );

    if ( 4 == iweap_points )
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        pweap->inst.matrix = FourPoints( nupoint[0].v, nupoint[1].v, nupoint[2].v, nupoint[3].v, mc_tmp->self_scale.z );

        // update the weapon position
        ego_chr::set_pos( pweap, nupoint[3].v );

        memcpy( &( pweap->inst.matrix_cache ), mc_tmp, sizeof( ego_matrix_cache ) );

        pweap_mcache->matrix_valid = btrue;
    }
    else if ( iweap_points > 0 )
    {
        // cannot find enough vertices. punt.
        // ignore the shape of the grip and just stick the character to the single mount point

        // update the character position
        ego_chr::set_pos( pweap, nupoint[0].v );

        // make sure we have the right data
        ego_chr::get_matrix_cache( pweap, mc_tmp );

        // add in the appropriate mods
        // this is a hybrid character and weapon matrix
        ADD_BITS( mc_tmp->type_bits, MAT_CHARACTER );

        // treat it like a normal character matrix
        apply_one_character_matrix( pweap, mc_tmp );
    }

    return pweap_mcache->matrix_valid;
}

//--------------------------------------------------------------------------------------------
bool_t apply_one_character_matrix( ego_chr * pchr, ego_matrix_cache * mc_tmp )
{
    /// @details ZZ@> Request that the matrix cache data be used to create a "weapon matrix".
    ///               i.e. a matrix that is attached to a specific grip.

    if ( NULL == mc_tmp ) return bfalse;

    // only apply character matrices using this function
    if ( 0 == ( MAT_CHARACTER & mc_tmp->type_bits ) ) return bfalse;

    pchr->inst.matrix_cache.matrix_valid = bfalse;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( pchr->stickybutt )
    {
        pchr->inst.matrix = ScaleXYZRotateXYZTranslate_SpaceFixed( mc_tmp->self_scale.x, mc_tmp->self_scale.y, mc_tmp->self_scale.z,
                            TO_TURN( mc_tmp->rotate.z ), TO_TURN( mc_tmp->rotate.x ), TO_TURN( mc_tmp->rotate.y ),
                            mc_tmp->pos.x, mc_tmp->pos.y, mc_tmp->pos.z );
    }
    else
    {
        pchr->inst.matrix = ScaleXYZRotateXYZTranslate_BodyFixed( mc_tmp->self_scale.x, mc_tmp->self_scale.y, mc_tmp->self_scale.z,
                            TO_TURN( mc_tmp->rotate.z ), TO_TURN( mc_tmp->rotate.x ), TO_TURN( mc_tmp->rotate.y ),
                            mc_tmp->pos.x, mc_tmp->pos.y, mc_tmp->pos.z );
    }

    memcpy( &( pchr->inst.matrix_cache ), mc_tmp, sizeof( ego_matrix_cache ) );

    pchr->inst.matrix_cache.matrix_valid = btrue;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t apply_reflection_matrix( ego_chr_instance * pinst, float grid_level )
{
    /// @details BB@> Generate the extra data needed to display a reflection for this character

    if ( NULL == pinst ) return bfalse;

    // invalidate the current matrix
    pinst->ref.matrix_valid = bfalse;

    // actually flip the matrix
    if ( pinst->matrix_cache.valid )
    {
        pinst->ref.matrix = pinst->matrix;

        pinst->ref.matrix.CNV( 0, 2 ) = -pinst->ref.matrix.CNV( 0, 2 );
        pinst->ref.matrix.CNV( 1, 2 ) = -pinst->ref.matrix.CNV( 1, 2 );
        pinst->ref.matrix.CNV( 2, 2 ) = -pinst->ref.matrix.CNV( 2, 2 );
        pinst->ref.matrix.CNV( 3, 2 ) = 2 * grid_level - pinst->ref.matrix.CNV( 3, 2 );

        pinst->ref.matrix_valid = btrue;
    }

    return pinst->ref.matrix_valid;
}

//--------------------------------------------------------------------------------------------
bool_t apply_matrix_cache( ego_chr * pchr, ego_matrix_cache * mc_tmp )
{
    /// @details BB@> request that the info in the matrix cache mc_tmp, be used to
    ///               make a matrix for the character pchr.

    bool_t applied = bfalse;

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;
    if ( NULL == mc_tmp || !mc_tmp->valid ) return bfalse;

    if ( 0 != ( MAT_WEAPON & mc_tmp->type_bits ) )
    {
        if ( DEFINED_CHR( mc_tmp->grip_chr ) )
        {
            applied = apply_one_weapon_matrix( pchr, mc_tmp );
        }
        else
        {
            ego_matrix_cache * mcache = &( pchr->inst.matrix_cache );

            // !!!the mc_tmp was mis-labeled as a MAT_WEAPON!!!
            make_one_character_matrix( GET_REF_PCHR( pchr ) );

            // recover the matrix_cache values from the character
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
    else if ( 0 != ( MAT_CHARACTER & mc_tmp->type_bits ) )
    {
        applied = apply_one_character_matrix( pchr, mc_tmp );
    }

    if ( applied )
    {
        apply_reflection_matrix( &( pchr->inst ), pchr->enviro.grid_level );
    }

    return applied;
}

//--------------------------------------------------------------------------------------------
int cmp_matrix_cache( const void * vlhs, const void * vrhs )
{
    /// @details BB@> check for differences between the data pointed to
    ///     by vlhs and vrhs, assuming that they point to ego_matrix_cache data.
    ///
    ///    The function is implemented this way so that in principle
    ///    if could be used in a function like qsort().
    ///
    ///    We could almost certainly make something easier and quicker by
    ///    using the function memcmp()

    int   itmp, cnt;
    float ftmp;

    ego_matrix_cache * plhs = ( ego_matrix_cache * )vlhs;
    ego_matrix_cache * prhs = ( ego_matrix_cache * )vrhs;

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
    if ( 0 != itmp ) goto cmp_matrix_cache_end;

    //---- check for differences in the MAT_WEAPON data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) )
    {
        itmp = ( signed )REF_TO_INT( plhs->grip_chr ) - ( signed )REF_TO_INT( prhs->grip_chr );
        if ( 0 != itmp ) goto cmp_matrix_cache_end;

        itmp = ( signed )plhs->grip_slot - ( signed )prhs->grip_slot;
        if ( 0 != itmp ) goto cmp_matrix_cache_end;

        for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            itmp = ( signed )plhs->grip_verts[cnt] - ( signed )prhs->grip_verts[cnt];
            if ( 0 != itmp ) goto cmp_matrix_cache_end;
        }

        // handle differences in the scale of our mount
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->grip_scale.v[cnt] - prhs->grip_scale.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the MAT_CHARACTER data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in the "Euler" rotation angles in 16-bit form
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->rotate.v[cnt] - prhs->rotate.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }

        // handle differences in the translate vector
        for ( cnt = 0; cnt < 3; cnt++ )
        {
            ftmp = plhs->pos.v[cnt] - prhs->pos.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the shared data
    if ( HAS_SOME_BITS( plhs->type_bits, MAT_WEAPON ) || HAS_SOME_BITS( plhs->type_bits, MAT_CHARACTER ) )
    {
        // handle differences in our own scale
        for ( cnt = 0; cnt < 3; cnt ++ )
        {
            ftmp = plhs->self_scale.v[cnt] - prhs->self_scale.v[cnt];
            if ( 0.0f != ftmp ) { itmp = SGN( ftmp ); goto cmp_matrix_cache_end; }
        }
    }

    // if it got here, the data is all the same
    itmp = 0;

cmp_matrix_cache_end:

    return SGN( itmp );
}

//--------------------------------------------------------------------------------------------
egoboo_rv matrix_cache_needs_update( ego_chr * pchr, ego_matrix_cache * pmc )
{
    /// @details BB@> determine whether a matrix cache has become invalid and needs to be updated

    ego_matrix_cache local_mc;
    bool_t needs_cache_update;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;

    if ( NULL == pmc ) pmc = &local_mc;

    // get the matrix data that is supposed to be used to make the matrix
    ego_chr::get_matrix_cache( pchr, pmc );

    // compare that data to the actual data used to make the matrix
    needs_cache_update = ( 0 != cmp_matrix_cache( pmc, &( pchr->inst.matrix_cache ) ) );

    return needs_cache_update ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::update_matrix( ego_chr * pchr, bool_t update_size )
{
    /// @details BB@> Do everything necessary to set the current matrix for this character.
    ///     This might include recursively going down the list of this character's mounts, etc.
    ///
    ///     Return btrue if a new matrix is applied to the character, bfalse otherwise.

    egoboo_rv      retval;
    bool_t         needs_update = bfalse;
    bool_t         applied      = bfalse;
    ego_matrix_cache mc_tmp;
    ego_matrix_cache *pchr_mc = NULL;

    if ( !DEFINED_PCHR( pchr ) ) return rv_error;
    pchr_mc = &( pchr->inst.matrix_cache );

    // recursively make sure that any mount matrices are updated
    if ( DEFINED_CHR( pchr->attachedto ) )
    {
        egoboo_rv attached_update = rv_error;

        attached_update = ego_chr::update_matrix( ChrList.lst + pchr->attachedto, btrue );

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
    retval = matrix_cache_needs_update( pchr, &mc_tmp );
    if ( rv_error == retval ) return rv_error;
    needs_update = ( rv_success == retval );

    // Update the grip vertices no matter what (if they are used)
    if ( HAS_SOME_BITS( mc_tmp.type_bits, MAT_WEAPON ) && INGAME_CHR( mc_tmp.grip_chr ) )
    {
        egoboo_rv grip_retval;
        ego_chr   * ptarget = ChrList.lst + mc_tmp.grip_chr;

        // has that character changes its animation?
        grip_retval = chr_instance_update_grip_verts( &( ptarget->inst ), mc_tmp.grip_verts, GRIP_VERTS );

        if ( rv_error   == grip_retval ) return rv_error;
        if ( rv_success == grip_retval ) needs_update = btrue;
    }

    // if it is not the same, make a new matrix with the new data
    applied = bfalse;
    if ( needs_update )
    {
        // we know the matrix is not valid
        pchr_mc->matrix_valid = bfalse;

        applied = apply_matrix_cache( pchr, &mc_tmp );
    }

    if ( applied && update_size )
    {
        // call ego_chr::update_collision_size() but pass in a false value to prevent a recursize call
        ego_chr::update_collision_size( pchr, bfalse );
    }

    return applied ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
bool_t ego_ai_state::set_bumplast( ego_ai_state * pself, const CHR_REF by_reference ichr )
{
    /// @details BB@> bumping into a chest can initiate whole loads of update messages.
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
CHR_REF chr_has_inventory_idsz( const CHR_REF by_reference ichr, IDSZ idsz, bool_t equipped, CHR_REF * pack_last )
{
    /// @details BB@> check the pack a matching item

    bool_t matches_equipped;
    CHR_REF item, tmp_item, tmp_var;
    ego_chr * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    // make sure that pack_last points to something
    if ( NULL == pack_last ) pack_last = &tmp_var;

    item = ( CHR_REF )MAX_CHR;

    *pack_last = GET_REF_PCHR( pchr );

    PACK_BEGIN_LOOP( tmp_item, pchr->pack.next )
    {
        matches_equipped = ( !equipped || ( INGAME_CHR( tmp_item ) && ChrList.lst[tmp_item].isequipped ) );

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
CHR_REF chr_holding_idsz( const CHR_REF by_reference ichr, IDSZ idsz )
{
    /// @details BB@> check the character's hands for a matching item

    bool_t found;
    CHR_REF item, tmp_item;
    ego_chr * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    item = ( CHR_REF )MAX_CHR;
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
CHR_REF chr_has_item_idsz( const CHR_REF by_reference ichr, IDSZ idsz, bool_t equipped, CHR_REF * pack_last )
{
    /// @details BB@> is ichr holding an item matching idsz, or is such an item in his pack?
    ///               return the index of the found item, or MAX_CHR if not found. Also return
    ///               the previous pack item in *pack_last, or MAX_CHR if it was not in a pack.

    bool_t found;
    CHR_REF item, tmp_var;
    ego_chr * pchr;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    // make sure that pack_last points to something
    if ( NULL == pack_last ) pack_last = &tmp_var;

    // Check the pack
    *pack_last = ( CHR_REF )MAX_CHR;
    item       = ( CHR_REF )MAX_CHR;
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
bool_t chr_can_see_object( const CHR_REF by_reference ichr, const CHR_REF by_reference iobj )
{
    /// @details BB@> can ichr see iobj?

    ego_chr * pchr, * pobj;
    int     light, self_light, enviro_light;
    int     alpha;

    if ( !INGAME_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;

    if ( !INGAME_CHR( iobj ) ) return bfalse;
    pobj = ChrList.lst + iobj;

    alpha = pobj->inst.alpha;
    if ( pchr->see_invisible_level > 0 )
    {
        alpha *= pchr->see_invisible_level + 1;
    }
    alpha = CLIP( alpha, 0, 255 );

    /// @note ZF@> Invictus characters can always see through darkness (spells, items, quest handlers, etc.)
    if ( IS_INVICTUS_PCHR_RAW( pchr ) && alpha >= INVISIBLE ) return btrue;

    enviro_light = ( alpha * pobj->inst.max_light ) * INV_FF;
    self_light   = ( pobj->inst.light == 255 ) ? 0 : pobj->inst.light;
    light        = MAX( enviro_light, self_light );

    if ( pchr->darkvision_level > 0 )
    {
        light *= pchr->darkvision_level + 1;
    }

    // Scenery, spells and quest objects can always see through darkness
    if ( IS_INVICTUS_PCHR_RAW( pchr ) ) light *= 20;

    return light >= INVISIBLE;
}

//--------------------------------------------------------------------------------------------
int ego_chr::get_price( const CHR_REF by_reference ichr )
{
    /// @details BB@> determine the correct price for an item

    CAP_REF icap;
    Uint16  iskin;
    float   price;

    ego_chr * pchr;
    ego_cap * pcap;

    if ( !INGAME_CHR( ichr ) ) return 0;
    pchr = ChrList.lst + ichr;

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
    pcap = CapStack.lst + icap;

    price = ( float ) pcap->skincost[iskin];

    // Items spawned in shops are more valuable
    if ( !pchr->isshopitem ) price *= 0.5f;

    // base the cost on the number of items/charges
    if ( pcap->isstackable )
    {
        price *= pchr->ammo;
    }
    else if ( pcap->isranged && pchr->ammo < pchr->ammomax )
    {
        if ( 0 != pchr->ammo )
        {
            price *= ( float ) pchr->ammo / ( float ) pchr->ammomax;
        }
    }

    return ( int )price;
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_enviro_grid_level( ego_chr * pchr, float level )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    if ( level != pchr->enviro.grid_level )
    {
        pchr->enviro.grid_level = level;

        apply_reflection_matrix( &( pchr->inst ), level );
    }
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_redshift( ego_chr * pchr, int rs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.redshift = rs;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_grnshift( ego_chr * pchr, int gs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.grnshift = gs;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_blushift( ego_chr * pchr, int bs )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.blushift = bs;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_sheen( ego_chr * pchr, int sheen )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.sheen = sheen;

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_alpha( ego_chr * pchr, int alpha )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.alpha = CLIP( alpha, 0, 255 );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_light( ego_chr * pchr, int light )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->inst.light = CLIP( light, 0, 255 );

    chr_instance_update_ref( &( pchr->inst ), pchr->enviro.grid_level, bfalse );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_fly_height( ego_chr * pchr, float height )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->fly_height = height;

    pchr->is_flying_platform = ( 0.0f != pchr->fly_height );
}

//--------------------------------------------------------------------------------------------
void ego_chr::set_jump_number_reset( ego_chr * pchr, int number )
{
    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->jump_number_reset = number;

    pchr->can_fly_jump = ( JUMP_NUMBER_INFINITE == pchr->jump_number_reset );
}

//--------------------------------------------------------------------------------------------
void chr_instance_get_tint( ego_chr_instance * pinst, GLfloat * tint, BIT_FIELD bits )
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
        local_alpha    = pinst->ref.alpha;
        local_light    = pinst->ref.light;
        local_sheen    = pinst->ref.sheen;
        local_redshift = pinst->ref.redshift;
        local_grnshift = pinst->ref.grnshift;
        local_blushift = pinst->ref.blushift;
    }
    else
    {
        // this is NOT a reflection, use the normal parameters
        local_alpha    = pinst->alpha;
        local_light    = pinst->light;
        local_sheen    = pinst->sheen;
        local_redshift = pinst->redshift;
        local_grnshift = pinst->grnshift;
        local_blushift = pinst->blushift;
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
        weight_sum += 1.0;

        tint[0] += 1.0f / ( 1 << local_redshift );
        tint[1] += 1.0f / ( 1 << local_grnshift );
        tint[2] += 1.0f / ( 1 << local_blushift );
        tint[3] += 1.0f;
    }

    if ( HAS_SOME_BITS( bits, CHR_ALPHA ) )
    {
        // alpha characters are blended onto the canvas using the alpha channel
        // the alpha channel is not important
        weight_sum += 1.0;

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

        weight_sum += 1.0;

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

        weight_sum += 1.0;

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
CHR_REF ego_chr::get_lowest_attachment( const CHR_REF by_reference ichr, bool_t non_item )
{
    /// @details BB@> Find the lowest attachment for a given object.
    ///               This was basically taken from the script function scr_set_TargetToLowestTarget()
    ///
    ///               You should be able to find the holder of a weapon by specifying non_item == btrue
    ///
    ///               To prevent possible loops in the data structures, use a counter to limit
    ///               the depth of the search, and make sure that ichr != ChrList.lst[object].attachedto

    int cnt;
    CHR_REF original_object, object, object_next;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;

    original_object = object = ichr;
    for ( cnt = 0, object = ichr; cnt < MAX_CHR && INGAME_CHR( object ); cnt++ )
    {
        object_next = ChrList.lst[object].attachedto;

        if ( non_item && !ChrList.lst[object].isitem )
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
    /// @details BB@> calculate a "mass" for each object, taking into account possible infinite masses.

    float loc_wta, loc_wtb;
    bool_t infinite_weight;

    if ( !ACTIVE_PCHR( pchr_a ) || !ACTIVE_PCHR( pchr_b ) ) return bfalse;

    if ( NULL == wta ) wta = &loc_wta;
    if ( NULL == wtb ) wtb = &loc_wtb;

    infinite_weight = ( pchr_a->platform && pchr_a->is_flying_platform ) || ( INFINITE_WEIGHT == pchr_a->phys.weight );
    *wta = infinite_weight ? -( float )INFINITE_WEIGHT : pchr_a->phys.weight;

    infinite_weight = ( pchr_b->platform && pchr_b->is_flying_platform ) || ( INFINITE_WEIGHT == pchr_b->phys.weight );
    *wtb = infinite_weight ? -( float )INFINITE_WEIGHT : pchr_b->phys.weight;

    if ( 0.0f == *wta && 0.0f == *wtb )
    {
        *wta = *wtb = 1;
    }
    else if ( 0.0f == *wta )
    {
        *wta = 1;
        *wtb = -( float )INFINITE_WEIGHT;
    }
    else if ( 0.0f == *wtb )
    {
        *wtb = 1;
        *wta = -( float )INFINITE_WEIGHT;
    }

    if ( 0.0f == pchr_a->phys.bumpdampen && 0.0f == pchr_b->phys.bumpdampen )
    {
        /* do nothing */
    }
    else if ( 0.0f == pchr_a->phys.bumpdampen )
    {
        // make the weight infinite
        *wta = -( float )INFINITE_WEIGHT;
    }
    else if ( 0.0f == pchr_b->phys.bumpdampen )
    {
        // make the weight infinite
        *wtb = -( float )INFINITE_WEIGHT;
    }
    else
    {
        // adjust the weights to respect bumpdampen
        if ( -( float )INFINITE_WEIGHT != *wta ) *wta /= pchr_a->phys.bumpdampen;
        if ( -( float )INFINITE_WEIGHT != *wtb ) *wtb /= pchr_b->phys.bumpdampen;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_can_mount( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
{
    bool_t is_valid_rider_a, is_valid_mount_b, has_ride_anim;
    int action_mi;

    ego_chr * pchr_a, * pchr_b;
    ego_cap * pcap_a, * pcap_b;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = ego_chr::get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

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
Uint32 ego_chr::get_framefx( ego_chr * pchr )
{
    int           frame_count;
    ego_MD2_Frame * frame_list, * pframe_nxt;
    ego_mad       * pmad;
    ego_MD2_Model * pmd2;

    if ( !DEFINED_PCHR( pchr ) ) return 0;

    pmad = ego_chr::get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return 0;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return 0;

    frame_count = md2_get_numFrames( pmd2 );
    EGOBOO_ASSERT( pchr->inst.frame_nxt < frame_count );

    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmd2 );
    pframe_nxt  = frame_list + pchr->inst.frame_nxt;

    return pframe_nxt->framefx;
}

//--------------------------------------------------------------------------------------------
egoboo_rv chr_invalidate_child_instances( ego_chr * pchr )
{
    int cnt;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    // invalidate vlst_cache of everything in this character's holdingwhich array
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
        CHR_REF iitem = pchr->holdingwhich[SLOT_LEFT];
        if ( !INGAME_CHR( iitem ) ) continue;

        // invalidate the matrix_cache
        ChrList.lst[iitem].inst.matrix_cache.valid = bfalse;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::set_action( ego_chr * pchr, int action, bool_t action_ready, bool_t override_action )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = chr_instance_set_action( &( pchr->inst ), action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    chr_invalidate_child_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::start_anim( ego_chr * pchr, int action, bool_t action_ready, bool_t override_action )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = chr_instance_start_anim( &( pchr->inst ), action, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    chr_invalidate_child_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::set_anim( ego_chr * pchr, int action, int frame, bool_t action_ready, bool_t override_action )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = chr_instance_set_anim( &( pchr->inst ), action, frame, action_ready, override_action );
    if ( rv_success != retval ) return retval;

    chr_invalidate_child_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::increment_action( ego_chr * pchr )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = chr_instance_increment_action( &( pchr->inst ) );
    if ( rv_success != retval ) return retval;

    chr_invalidate_child_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::increment_frame( ego_chr * pchr )
{
    egoboo_rv retval;
    ego_mad * pmad;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    pmad = ego_chr::get_pmad( GET_REF_PCHR( pchr ) );
    if ( NULL == pmad ) return rv_error;

    retval = chr_instance_increment_frame( &( pchr->inst ), pmad, pchr->attachedto );
    if ( rv_success != retval ) return retval;

    chr_invalidate_child_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_chr::play_action( ego_chr * pchr, int action, bool_t action_ready )
{
    egoboo_rv retval;

    if ( !ACTIVE_PCHR( pchr ) ) return rv_error;

    retval = chr_instance_play_action( &( pchr->inst ), action, action_ready );
    if ( rv_success != retval ) return retval;

    chr_invalidate_child_instances( pchr );

    return retval;
}

//--------------------------------------------------------------------------------------------
MAD_REF ego_chr::get_imad( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return ( MAD_REF )MAX_MAD;
    pchr = ChrList.lst + ichr;

    // try to repair a bad model if it exists
    if ( !LOADED_MAD( pchr->inst.imad ) )
    {
        MAD_REF imad_tmp = pro_get_imad( pchr->profile_ref );
        if ( LOADED_MAD( imad_tmp ) )
        {
            if ( chr_instance_set_mad( &( pchr->inst ), imad_tmp ) )
            {
                ego_chr::update_collision_size( pchr, btrue );
            }
        }
        if ( !LOADED_MAD( pchr->inst.imad ) ) return ( MAD_REF )MAX_MAD;
    }

    return pchr->inst.imad;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_chr::get_pmad( const CHR_REF by_reference ichr )
{
    ego_chr * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    // try to repair a bad model if it exists
    if ( !LOADED_MAD( pchr->inst.imad ) )
    {
        MAD_REF imad_tmp = pro_get_imad( pchr->profile_ref );
        if ( LOADED_MAD( imad_tmp ) )
        {
            chr_instance_set_mad( &( pchr->inst ), imad_tmp );
        }
    }

    if ( !LOADED_MAD( pchr->inst.imad ) ) return NULL;

    return MadStack.lst + pchr->inst.imad;
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

    pchr->onwhichgrid   = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );
    pchr->onwhichblock  = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );

    // update whether the current character position is safe
    ego_chr::update_safe( pchr, bfalse );

    // update the breadcrumb list
    ego_chr::update_breadcrumb( pchr, bfalse );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::set_pos( ego_chr * pchr, fvec3_base_t pos )
{
    bool_t retval = bfalse;

    if ( !VALID_PCHR( pchr ) ) return retval;

    retval = btrue;

    if (( pos[kX] != pchr->pos.v[kX] ) || ( pos[kY] != pchr->pos.v[kY] ) || ( pos[kZ] != pchr->pos.v[kZ] ) )
    {
        memmove( pchr->pos.v, pos, sizeof( fvec3_base_t ) );

        retval = chr_update_pos( pchr );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::set_maxaccel( ego_chr * pchr, float new_val )
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
ego_chr_bundle * ego_chr_bundle::ctor( ego_chr_bundle * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    pbundle->chr_ref = ( CHR_REF ) MAX_CHR;
    pbundle->chr_ptr = NULL;

    pbundle->cap_ref = ( CAP_REF ) MAX_CAP;
    pbundle->cap_ptr = NULL;

    pbundle->pro_ref = ( PRO_REF ) MAX_PROFILE;
    pbundle->pro_ptr = NULL;

    return pbundle;
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * ego_chr_bundle::validate( ego_chr_bundle * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    // get the character info from the reference or the pointer
    if ( VALID_CHR( pbundle->chr_ref ) )
    {
        pbundle->chr_ptr = ChrList.lst + pbundle->chr_ref;
    }
    else if ( NULL != pbundle->chr_ptr )
    {
        pbundle->chr_ref = GET_REF_PCHR( pbundle->chr_ptr );
    }
    else
    {
        pbundle->chr_ref = MAX_CHR;
        pbundle->chr_ptr = NULL;
    }

    if ( NULL == pbundle->chr_ptr ) goto ego_chr_bundle__validate_fail;

    // get the profile info
    pbundle->pro_ref = pbundle->chr_ptr->profile_ref;
    if ( !LOADED_PRO( pbundle->pro_ref ) ) goto ego_chr_bundle__validate_fail;

    pbundle->pro_ptr = ProList.lst + pbundle->pro_ref;

    // get the cap info
    pbundle->cap_ref = pbundle->pro_ptr->icap;

    if ( !LOADED_CAP( pbundle->cap_ref ) ) goto ego_chr_bundle__validate_fail;
    pbundle->cap_ptr = CapStack.lst + pbundle->cap_ref;

    return pbundle;

ego_chr_bundle__validate_fail:

    return ego_chr_bundle::ctor( pbundle );
}

//--------------------------------------------------------------------------------------------
ego_chr_bundle * ego_chr_bundle::set( ego_chr_bundle * pbundle, ego_chr * pchr )
{
    if ( NULL == pbundle ) return NULL;

    // blank out old data
    pbundle = ego_chr_bundle::ctor( pbundle );

    if ( NULL == pbundle || NULL == pchr ) return pbundle;

    // set the particle pointer
    pbundle->chr_ptr = pchr;

    // validate the particle data
    pbundle = ego_chr_bundle::validate( pbundle );

    return pbundle;
}

//--------------------------------------------------------------------------------------------
void character_physics_initialize()
{
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        phys_data_blank_accumulators( &( pchr->phys ) );
    }
    CHR_END_LOOP();
}
//--------------------------------------------------------------------------------------------
bool_t chr_bump_mesh_attached( ego_chr_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt )
{
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bump_mesh( ego_chr_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt )
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

    // does the bundle exist?
    if ( NULL == pbdl ) return bfalse;

    // some aliases to make the notation easier
    loc_pchr    = pbdl->chr_ptr;
    if ( NULL == loc_pchr ) return bfalse;

    loc_pcap    = pbdl->cap_ptr;
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
        if ( ABS( final_vel_z ) < STOPBOUNCING )
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
bool_t chr_bump_grid_attached( ego_chr_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt )
{
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bump_grid( ego_chr_bundle * pbdl, fvec3_t test_pos, fvec3_t test_vel, float dt )
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

    // does the bundle exist?
    if ( NULL == pbdl ) return bfalse;

    // some aliases to make the notation easier
    loc_pchr = pbdl->chr_ptr;
    if ( NULL == loc_pchr ) return bfalse;

    loc_pcap = pbdl->cap_ptr;
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

    // try to get a normal from the mesh_get_diff() function
    if ( !found_nrm )
    {
        fvec2_t diff;

        diff = chr_get_diff( loc_pchr, test_pos.v, pressure );
        diff_function_called = btrue;

        nrm.x = diff.x;
        nrm.y = diff.y;

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
        ftmp = MAX( ABS( diff_perp.x ), ABS( diff_perp.y ) );
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
void character_physics_finalize_one( ego_chr_bundle * pbdl, float dt )
{
    bool_t bumped_mesh = bfalse, bumped_grid  = bfalse, needs_update = bfalse;

    fvec3_t test_pos, test_vel;

    // aliases for easier notation
    ego_chr * loc_pchr;
    ego_phys_data * loc_pphys;

    if ( NULL == pbdl ) return;

    // alias these parameter for easier notation
    loc_pchr   = pbdl->chr_ptr;
    loc_pphys  = &( loc_pchr->phys );

    // do the "integration" of the accumulators
    // work on test_pos and test velocity instead of the actual character position and velocity
    test_pos = ego_chr::get_pos( loc_pchr );
    test_vel = loc_pchr->vel;
    phys_data_integrate_accumulators( &test_pos, &test_vel, loc_pphys, dt );

    // bump the character with the mesh
    bumped_mesh = bfalse;
    if ( ACTIVE_CHR( loc_pchr->attachedto ) )
    {
        bumped_mesh = chr_bump_mesh_attached( pbdl, test_pos, test_vel, dt );
    }
    else
    {
        bumped_mesh = chr_bump_mesh( pbdl, test_pos, test_vel, dt );
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
    if ( ACTIVE_CHR( loc_pchr->attachedto ) )
    {
        bumped_grid = chr_bump_grid_attached( pbdl, test_pos, test_vel, dt );
    }
    else
    {
        bumped_grid = chr_bump_grid( pbdl, test_pos, test_vel, dt );
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
        Uint32 chr_update = loc_pchr->obj_base.guid + update_wld;

        needs_update = ( 0 == ( chr_update & 7 ) );
    }

    if ( needs_update )
    {
        ego_chr::update_safe( loc_pchr, needs_update );
    }
}

//--------------------------------------------------------------------------------------------
void character_physics_finalize_all( float dt )
{
    // accumulate the accumulators
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        ego_chr_bundle bdl;

        // create a bundle for the character data
        if ( NULL == ego_chr_bundle::set( &bdl, pchr ) ) continue;

        character_physics_finalize_one( &bdl, dt );
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
bool_t ego_chr::update_breadcrumb( ego_chr * pchr, bool_t force )
{
    Uint32 new_grid;
    bool_t retval = bfalse;
    bool_t needs_update = bfalse;
    ego_breadcrumb * bc_ptr, bc;

    if ( !VALID_PCHR( pchr ) ) return bfalse;

    bc_ptr = breadcrumb_list_last_valid( &( pchr->crumbs ) );
    if ( NULL == bc_ptr )
    {
        force  = btrue;
        bc_ptr = &bc;
        breadcrumb_init_chr( bc_ptr, pchr );
    }

    if ( force )
    {
        needs_update = btrue;
    }
    else
    {
        new_grid = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pchr->pos.x - bc_ptr->pos.x ) > GRID_SIZE ||
                 ABS( pchr->pos.y - bc_ptr->pos.y ) > GRID_SIZE )
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
        pchr->safe_grid  = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_chr::update_safe( ego_chr * pchr, bool_t force )
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
        new_grid = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pchr->pos.x - pchr->safe_pos.x ) > GRID_SIZE ||
                 ABS( pchr->pos.y - pchr->safe_pos.y ) > GRID_SIZE )
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
            memmove( pos_v, pchr->safe_pos.v, sizeof( fvec3_base_t ) );
        }
    }

    if ( !found )
    {
        ego_breadcrumb * bc;

        bc = breadcrumb_list_last_valid( &( pchr->crumbs ) );

        if ( NULL != bc )
        {
            found = btrue;
            memmove( pos_v, bc->pos.v, sizeof( fvec3_base_t ) );
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

    memset( platch, 0, sizeof( *platch ) );
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
    ppack->count = MIN( ppack->count, MAX_CHR - 1 );

    parent_pack_ptr = ppack;
    item_ref        = ppack->next;
    for ( cnt = 0; cnt < ppack->count && DEFINED_CHR( item_ref ); cnt++ )
    {
        ego_chr  * item_ptr      = ChrList.lst + item_ref;
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
                    log_warning( "pack_validate() - The item %s is in a pack, even though it is not tagged as an item.\n", item_ptr->obj_base.base_name );

                    // remove the item from the pack
                    parent_pack_ptr->next     = item_pack_ptr->next;

                    // re-initialize the item's "pack"
                    item_pack_ptr->next       = ( CHR_REF )MAX_CHR;
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
        log_warning( "pack_validate() - Found a bad pack and am fixing it.\n" );

        if ( parent_pack_ptr == ppack )
        {
            ppack->count = 0;
        }
        else
        {
            parent_pack_ptr->next = ( CHR_REF )MAX_CHR;
        }
    }

    // update the pack count to the actual number of objects in the pack
    ppack->count = cnt;

    // make sure that the ppack->next matches ppack->count
    if ( 0 == ppack->count )
    {
        ppack->next = ( CHR_REF )MAX_CHR;
    }

    // make sure that the ppack->count matches ppack->next
    if ( !DEFINED_CHR( ppack->next ) )
    {
        ppack->count = 0;
        ppack->next  = ( CHR_REF )MAX_CHR;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t pack_add_item( ego_pack * ppack, CHR_REF item )
{
    ego_chr  * pitem;
    ego_pack * pitem_pack;

    // make sure the pack is valid
    if ( !pack_validate( ppack ) ) return bfalse;

    // make sure that the item is valid
    if ( !DEFINED_CHR( item ) ) return bfalse;
    pitem      = ChrList.lst + item;
    pitem_pack = &( pitem->pack );

    // is this item packed in another pack?
    if ( pitem_pack->is_packed )
    {
        log_warning( "pack_add_item() - Trying to add a packed item (%s) to a pack.\n", pitem->obj_base.base_name );

        return bfalse;
    }

    // does this item have packed objects of its own?
    if ( 0 != pitem_pack->count || MAX_CHR != pitem_pack->next )
    {
        log_warning( "pack_add_item() - Trying to add an item (%s) to a pack that has a sub-pack.\n", pitem->obj_base.base_name );

        return bfalse;
    }

    // is the item even an item?
    if ( !pitem->isitem )
    {
        log_debug( "pack_add_item() - Trying to add a non-item %s to a pack.\n", pitem->obj_base.base_name );
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
bool_t pack_remove_item( ego_pack * ppack, CHR_REF iparent, CHR_REF iitem )
{
    CHR_REF old_next;
    ego_chr * pitem, * pparent;

    // make sure the pack is valid
    if ( !pack_validate( ppack ) ) return bfalse;

    // convert the iitem it to a pointer
    old_next = ( CHR_REF )MAX_CHR;
    pitem    = NULL;
    if ( DEFINED_CHR( iitem ) )
    {
        pitem    = ChrList.lst + iitem;
        old_next = pitem->pack.next;
    }

    // convert the pparent it to a pointer
    pparent = NULL;
    if ( DEFINED_CHR( iparent ) )
    {
        pparent = ChrList.lst + iparent;
    }

    // Remove the item from the pack
    if ( NULL != pitem )
    {
        pitem->pack.was_packed = pitem->pack.is_packed;
        pitem->pack.is_packed  = bfalse;
        pitem->pack.next       = ( CHR_REF )MAX_CHR;
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
bool_t chr_inventory_add_item( const CHR_REF by_reference item, const CHR_REF by_reference character )
{
    ego_chr * pchr, * pitem;
    ego_cap * pitem_cap;
    bool_t  slot_found, pack_added;
    int     slot_index;
    int     cnt;

    if ( !INGAME_CHR( item ) ) return bfalse;
    pitem = ChrList.lst + item;

    // don't allow sub-inventories
    if ( pitem->pack.is_packed || pitem->isequipped ) return bfalse;

    pitem_cap = pro_get_pcap( pitem->profile_ref );
    if ( NULL == pitem_cap ) return bfalse;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr = ChrList.lst + character;

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
            pchr->inventory[slot_index] = ( CHR_REF )MAX_CHR;
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
CHR_REF chr_inventory_remove_item( const CHR_REF by_reference ichr, grip_offset_t grip_off, bool_t ignorekurse )
{
    ego_chr * pchr;
    CHR_REF iitem;
    int     cnt;

    if ( !INGAME_CHR( ichr ) ) return ( CHR_REF )MAX_CHR;
    pchr = ChrList.lst + ichr;

    // make sure the pack is not empty
    if ( 0 == pchr->pack.count || MAX_CHR == pchr->pack.next ) return ( CHR_REF )MAX_CHR;

    // do not allow sub inventories
    if ( pchr->pack.is_packed || pchr->isitem ) return ( CHR_REF )MAX_CHR;

    // grab the first item from the pack
    iitem = chr_pack_get_item( ichr, grip_off, ignorekurse );
    if ( !INGAME_CHR( iitem ) ) return ( CHR_REF )MAX_CHR;

    // remove it from the "equipped" slots
    if ( DEFINED_CHR( iitem ) )
    {
        for ( cnt = 0; cnt < INVEN_COUNT; cnt++ )
        {
            if ( iitem == pchr->inventory[cnt] )
            {
                pchr->inventory[cnt]          = ( CHR_REF )MAX_CHR;
                ChrList.lst[iitem].isequipped = bfalse;
                break;
            }
        }
    }

    return iitem;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF chr_pack_has_a_stack( const CHR_REF by_reference item, const CHR_REF by_reference character )
{
    /// @details ZZ@> This function looks in the character's pack for an item similar
    ///    to the one given.  If it finds one, it returns the similar item's
    ///    index number, otherwise it returns MAX_CHR.

    CHR_REF istack;
    Uint16  id;
    bool_t  found;

    ego_chr * pitem;
    ego_cap * pitem_cap;

    found  = bfalse;
    istack = ( CHR_REF )MAX_CHR;

    if ( !INGAME_CHR( item ) ) return istack;
    pitem = ChrList.lst + item;
    pitem_cap = ego_chr::get_pcap( item );

    if ( pitem_cap->isstackable )
    {
        PACK_BEGIN_LOOP( istack, ChrList.lst[character].pack.next )
        {
            if ( INGAME_CHR( istack ) )
            {
                ego_chr * pstack     = ChrList.lst + istack;
                ego_cap * pstack_cap = ego_chr::get_pcap( istack );

                found = pstack_cap->isstackable;

                if ( pstack->ammo >= pstack->ammomax )
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
            istack = ( CHR_REF )MAX_CHR;
        }
    }

    return istack;
}

//--------------------------------------------------------------------------------------------
bool_t chr_pack_add_item( const CHR_REF by_reference item, const CHR_REF by_reference character )
{
    /// @details ZZ@> This function puts an item inside a character's pack

    CHR_REF stack;
    int     newammo;

    ego_chr  * pchr, * pitem;
    ego_cap  * pchr_cap,  * pitem_cap;
    ego_pack * pchr_pack, * pitem_pack;

    if ( !INGAME_CHR( character ) ) return bfalse;
    pchr      = ChrList.lst + character;
    pchr_pack = &( pchr->pack );
    pchr_cap  = ego_chr::get_pcap( character );

    if ( !INGAME_CHR( item ) ) return bfalse;
    pitem      = ChrList.lst + item;
    pitem_pack = &( pitem->pack );
    pitem_cap  = ego_chr::get_pcap( item );

    // Make sure everything is hunkydori
    if ( pitem_pack->is_packed || pchr_pack->is_packed || pchr->isitem )
        return bfalse;

    stack = chr_pack_has_a_stack( item, character );
    if ( INGAME_CHR( stack ) )
    {
        // We found a similar, stackable item in the pack

        ego_chr  * pstack      = ChrList.lst + stack;
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
        if ( newammo <= pstack->ammomax )
        {
            // All transferred, so kill the in hand item
            pstack->ammo = newammo;
            if ( INGAME_CHR( pitem->attachedto ) )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            ego_chr::request_terminate( item );
        }
        else
        {
            // Only some were transferred,
            pitem->ammo     = pitem->ammo + pstack->ammo - pstack->ammomax;
            pstack->ammo    = pstack->ammomax;
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
bool_t chr_pack_remove_item( CHR_REF ichr, CHR_REF iparent, CHR_REF iitem )
{
    ego_chr  * pchr;
    ego_pack * pchr_pack;

    bool_t removed;

    if ( !DEFINED_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;
    pchr_pack = &( pchr->pack );

    // remove it from the pack
    removed = pack_remove_item( pchr_pack, iparent, iitem );

    // unequip the item
    if ( removed && DEFINED_CHR( iitem ) )
    {
        ChrList.lst[iitem].isequipped = bfalse;
    }

    return removed;
}
//--------------------------------------------------------------------------------------------
CHR_REF chr_pack_get_item( const CHR_REF by_reference chr_ref, grip_offset_t grip_off, bool_t ignorekurse )
{
    /// @details ZZ@> This function takes the last item in the chrcharacter's pack and puts
    ///    it into the designated hand.  It returns the item_ref or MAX_CHR.

    CHR_REF tmp_ref, item_ref, parent_ref;
    ego_chr  * pchr, * item_ptr, *parent_ptr;
    ego_pack * chr_pack_ptr, * item_pack_ptr, *parent_pack_ptr;

    // does the chr_ref exist?
    if ( !DEFINED_CHR( chr_ref ) ) return bfalse;
    pchr         = ChrList.lst + chr_ref;
    chr_pack_ptr = &( pchr->pack );

    // Can the chr_ref have a pack?
    if ( chr_pack_ptr->is_packed || pchr->isitem ) return ( CHR_REF )MAX_CHR;

    // is the pack empty?
    if ( MAX_CHR == chr_pack_ptr->next || 0 == chr_pack_ptr->count )
    {
        return ( CHR_REF )MAX_CHR;
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
    if ( chr_ref == item_ref || MAX_CHR == item_ref ) return bfalse;

    // convert the item_ref it to a pointer
    item_ptr      = NULL;
    item_pack_ptr = NULL;
    if ( DEFINED_CHR( item_ref ) )
    {
        item_ptr = ChrList.lst + item_ref;
        item_pack_ptr = &( item_ptr->pack );
    }

    // did we find a valid item?
    if ( NULL == item_ptr )
    {
        chr_pack_remove_item( chr_ref, parent_ref, item_ref );

        return bfalse;
    }

    // convert the parent_ptr it to a pointer
    parent_ptr      = NULL;
    parent_pack_ptr = NULL;
    if ( DEFINED_CHR( parent_ref ) )
    {
        parent_ptr      = ChrList.lst + parent_ref;
        parent_pack_ptr = &( parent_ptr->pack );
    }

    // Figure out what to do with it
    if ( item_ptr->iskursed && item_ptr->isequipped && !ignorekurse )
    {
        // The equipped item cannot be taken off, move the item to the front
        // of the pack

        // remove the item from the list
        parent_pack_ptr->next = item_pack_ptr->next;
        item_pack_ptr->next   = ( CHR_REF )MAX_CHR;

        // Add the item to the front of the list
        item_pack_ptr->next = chr_pack_ptr->next;
        chr_pack_ptr->next  = item_ref;

        // Flag the last item_ref as not removed
        ADD_BITS( item_ptr->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY

        // let the calling function know that we didn't find anything
        item_ref = ( CHR_REF )MAX_CHR;
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
    /// @details ZF@> This function returns true if the character is over a water tile

    if ( !DEFINED_PCHR( pchr ) ) return bfalse;

    if ( !water.is_water || !mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) return bfalse;

    return 0 != mesh_test_fx( PMesh, pchr->onwhichgrid, MPDFX_WATER );
}

//--------------------------------------------------------------------------------------------
float calc_dismount_lerp( const ego_chr * pchr_a, const ego_chr * pchr_b )
{
    /// @details BB@> generate a "lerp" for characters that have dismounted

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
        dismount_lerp_a = ( float )pchr_a->dismount_timer / ( float )PHYS_DISMOUNT_TIME;
        dismount_lerp_a = 1.0f - CLIP( dismount_lerp_a, 0.0f, 1.0f );
        found = ( 1.0f != dismount_lerp_a );
    }

    dismount_lerp_b = 1.0f;
    if ( pchr_b->dismount_timer > 0 && pchr_b->dismount_object == ichr_a )
    {
        dismount_lerp_b = ( float )pchr_b->dismount_timer / ( float )PHYS_DISMOUNT_TIME;
        dismount_lerp_b = 1.0f - CLIP( dismount_lerp_b, 0.0f, 1.0f );
        found = ( 1.0f != dismount_lerp_b );
    }

    return !found ? 1.0f : dismount_lerp_a * dismount_lerp_b;
}

//--------------------------------------------------------------------------------------------
bool_t chr_copy_enviro( ego_chr * chr_psrc, ego_chr * chr_pdst )
{
    /// BB@> do a deep copy on the character's enviro data

    ego_chr_environment * psrc, * pdst;

    if ( NULL == chr_psrc || NULL == chr_pdst ) return bfalse;

    if ( chr_psrc == chr_pdst ) return btrue;

    psrc = &( chr_psrc->enviro );
    pdst = &( chr_pdst->enviro );

    // use the special function to set the grid level
    // this must done first so that the character's reflection data is set properly
    ego_chr::set_enviro_grid_level( chr_pdst, psrc->grid_level );

    // now just copy the other data.
    // use memmove() in the odd case the regions overlap
    memmove( psrc, pdst, sizeof( *psrc ) );

    return btrue;
}
