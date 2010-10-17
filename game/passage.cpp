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

/// @file passage.c
/// @brief Passages and doors and whatnot.  Things that impede your progress!

#include "passage.h"

#include "char.inl"
#include "script.h"
#include "sound.h"
#include "mesh.inl"
#include "game.h"
#include "quest.h"
#include "network.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"

#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INSTANTIATE_STACK( ACCESS_TYPE_NONE, ego_passage, PassageStack, MAX_PASS );
INSTANTIATE_STACK( ACCESS_TYPE_NONE, ego_shop,    ShopStack, MAX_SHOP );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void PassageStack_free_all();
static void ShopStack_free_all();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void activate_passages_file_vfs()
{
    /// @details ZZ@> This function reads the passage file
    ego_passage  tmp_passage;
    vfs_FILE  *fileread;

    // Reset all of the old passages
    passage_system_clear();

    // Load the file
    fileread = vfs_openRead( "mp_data/passage.txt" );
    if ( NULL == fileread ) return;

    while ( scan_passage_data_file( fileread, &tmp_passage ) )
    {
        PassageStack_add_one( &tmp_passage );
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
void passage_system_clear()
{
    /// @details ZZ@> This function clears the passage list ( for doors )

    PassageStack_free_all();
    ShopStack_free_all();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void PassageStack_free_all()
{
    PassageStack.count = 0;
}

//--------------------------------------------------------------------------------------------
int PassageStack_get_free()
{
    int ipass = ( PASS_REF ) MAX_PASS;

    if ( PassageStack.count < MAX_PASS )
    {
        ipass = PassageStack.count;
        PassageStack.count++;
    };

    return ipass;
}

//--------------------------------------------------------------------------------------------
bool_t PassageStack_open( const PASS_REF by_reference passage )
{
    /// @details ZZ@> This function makes a passage passable

    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    return ego_passage::do_open( PassageStack.lst + passage );
}

//--------------------------------------------------------------------------------------------
void PassageStack_flash( const PASS_REF by_reference passage, Uint8 color )
{
    /// @details ZZ@> This function makes a passage flash white

    if ( !INVALID_PASSAGE( passage ) )
    {
        ego_passage::flash( PassageStack.lst + passage, color );
    }
}

//--------------------------------------------------------------------------------------------
bool_t PassageStack_point_is_inside( const PASS_REF by_reference passage, float xpos, float ypos )
{
    /// @details ZZ@> This function makes a passage passable
    bool_t useful = bfalse;

    if ( INVALID_PASSAGE( passage ) ) return useful;

    return ego_passage::point_is_in( PassageStack.lst + passage, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
bool_t PassageStack_object_is_inside( const PASS_REF by_reference passage, float xpos, float ypos, float radius )
{
    /// @details ZZ@> This function makes a passage passable
    bool_t useful = bfalse;

    if ( INVALID_PASSAGE( passage ) ) return useful;

    return ego_passage::object_is_in( PassageStack.lst + passage, xpos, ypos, radius );
}
//--------------------------------------------------------------------------------------------
CHR_REF PassageStack_who_is_blocking( const PASS_REF by_reference passage, const CHR_REF by_reference isrc, IDSZ idsz, BIT_FIELD targeting_bits, IDSZ require_item )
{
    /// @details ZZ@> This function makes a passage passable

    // Skip invalid passages
    if ( INVALID_PASSAGE( passage ) ) return ( CHR_REF )MAX_CHR;

    return ego_passage::who_is_blocking( PassageStack.lst + passage, isrc, idsz, targeting_bits, require_item );
}

//--------------------------------------------------------------------------------------------
void PassageStack_check_music()
{
    /// @details ZF@> This function checks all passages if there is a player in it, if it is, it plays a specified
    /// song set in by the AI script functions

    PASS_REF passage;

    // Check every music passage
    for ( passage = 0; passage < PassageStack.count; passage++ )
    {
        if ( ego_passage::check_music( PassageStack.lst + passage ) )
        {
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t PassageStack_close_one( const PASS_REF by_reference passage )
{
    // Skip invalid passages
    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    return ego_passage::close( PassageStack.lst + passage );
}

//--------------------------------------------------------------------------------------------
void PassageStack_add_one( ego_passage * pdata )
{
    /// @details ZZ@> This function creates a passage area

    PASS_REF    ipass;

    if ( NULL == pdata ) return;

    ipass = PassageStack_get_free();
    if ( ipass >= MAX_PASS ) return;

    ego_passage::init( PassageStack.lst + ipass, pdata );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ShopStack_free_all()
{
    SHOP_REF cnt;

    for ( cnt = 0; cnt < MAX_PASS; cnt++ )
    {
        ShopStack.lst[cnt].owner   = SHOP_NOOWNER;
        ShopStack.lst[cnt].passage = 0;
    }
    ShopStack.count = 0;
}

//--------------------------------------------------------------------------------------------
int ShopStack_get_free()
{
    int ishop = ( PASS_REF ) MAX_PASS;

    if ( ShopStack.count < MAX_PASS )
    {
        ishop = ShopStack.count;
        ShopStack.count++;
    };

    return ishop;
}

//--------------------------------------------------------------------------------------------
void ShopStack_add_one( const CHR_REF by_reference owner, const PASS_REF by_reference passage )
{
    /// @details ZZ@> This function creates a shop passage

    SHOP_REF ishop;
    CHR_REF  ichr;

    if ( !VALID_PASSAGE( passage ) ) return;

    if ( !INGAME_CHR( owner ) || !ChrList.lst[owner].alive ) return;

    ishop = ShopStack_get_free();
    if ( !VALID_SHOP( ishop ) ) return;

    // The passage exists...
    ShopStack.lst[ishop].passage = passage;
    ShopStack.lst[ishop].owner   = owner;

    // flag every item in the shop as a shop item
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ego_chr * pchr;

        if ( !INGAME_CHR( ichr ) ) continue;
        pchr = ChrList.lst + ichr;

        if ( pchr->isitem )
        {
            if ( PassageStack_object_is_inside( ShopStack.lst[ishop].passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
            {
                pchr->isshopitem = btrue;               // Full value
                pchr->iskursed   = bfalse;              // Shop items are never kursed

                // Identify only cheap items in a shop
                /// @note BB@> not all shop items should be identified.
                /// I guess there could be a minor exploit with casters identifying a book, then
                /// leaving the module and re-identifying the book.
                /// An answer would be to reduce the XP based on the some quality of the book and the character's level?
                /// Maybe give the spell books levels, and if your character's level is above the spellbook level, no XP for identifying it?

                if ( ego_chr::get_price( ichr ) <= SHOP_IDENTIFY )
                {
                    pchr->nameknown  = btrue;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
CHR_REF ShopStack_find_owner( int ix, int iy )
{
    /// ZZ@> This function returns the owner of a item in a shop

    SHOP_REF cnt;
    CHR_REF  owner = ( CHR_REF )SHOP_NOOWNER;

    for ( cnt = 0; cnt < ShopStack.count; cnt++ )
    {
        PASS_REF    passage;
        ego_passage * ppass;
        ego_shop    * pshop;

        pshop = ShopStack.lst + cnt;

        passage = pshop->passage;

        if ( INVALID_PASSAGE( passage ) ) continue;
        ppass = PassageStack.lst + passage;

        if ( irect_point_inside( &( ppass->area ), ix, iy ) )
        {
            // if there is SHOP_NOOWNER, someone has been murdered!
            owner = pshop->owner;
            break;
        }
    }

    return owner;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_passage * ego_passage::init( ego_passage * ppass, passage_data_t * pdata )
{
    if ( NULL == ppass ) return NULL;

    if ( NULL != pdata )
    {
        ppass->area.xmin  = CLIP( pdata->area.xmin, 0, PMesh->info.tiles_x - 1 );
        ppass->area.ymin  = CLIP( pdata->area.ymin, 0, PMesh->info.tiles_y - 1 );

        ppass->area.xmax  = CLIP( pdata->area.xmax, 0, PMesh->info.tiles_x - 1 );
        ppass->area.ymax  = CLIP( pdata->area.ymax, 0, PMesh->info.tiles_y - 1 );

        ppass->mask       = pdata->mask;
        ppass->music      = pdata->music;

        // Is it open or closed?
        ppass->open       = btrue;
        if ( !pdata->open )
        {
            ego_passage::close( ppass );
        }
    }

    return ppass;
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::do_open( ego_passage * ppass )
{
    int x, y;
    Uint32 fan;
    bool_t useful = bfalse;

    if ( NULL == ppass ) return useful;

    useful = !ppass->open;
    ppass->open = btrue;

    if ( ppass->area.ymin <= ppass->area.ymax )
    {
        useful = ( !ppass->open );
        ppass->open = btrue;

        for ( y = ppass->area.ymin; y <= ppass->area.ymax; y++ )
        {
            for ( x = ppass->area.xmin; x <= ppass->area.xmax; x++ )
            {
                fan = mesh_get_tile_int( PMesh, x, y );
                mesh_clear_fx( PMesh, fan, MPDFX_WALL | MPDFX_IMPASS );
            }
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
void ego_passage::flash( ego_passage * ppass, Uint8 color )
{
    int x, y, cnt;
    Uint32 fan;

    if ( NULL == ppass ) return;

    for ( y = ppass->area.ymin; y <= ppass->area.ymax; y++ )
    {
        for ( x = ppass->area.xmin; x <= ppass->area.xmax; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );

            if ( !mesh_grid_is_valid( PMesh, fan ) ) continue;

            for ( cnt = 0; cnt < 4; cnt++ )
            {
                PMesh->tmem.tile_list[fan].lcache[cnt] = color;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::point_is_in( ego_passage * ppass, float xpos, float ypos )
{
    /// @details ZF@> This return btrue if the specified X and Y coordinates are within the passage

    ego_frect_t tmp_rect;

    // Passage area
    tmp_rect.xmin = ppass->area.xmin * GRID_SIZE;
    tmp_rect.ymin = ppass->area.ymin * GRID_SIZE;
    tmp_rect.xmax = ( ppass->area.xmax + 1 ) * GRID_SIZE;
    tmp_rect.ymax = ( ppass->area.ymax + 1 ) * GRID_SIZE;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::object_is_in( ego_passage * ppass, float xpos, float ypos, float radius )
{
    /// @details ZF@> This return btrue if the specified X and Y coordinates are within the passage
    ///     radius is how much offset we allow outside the passage

    ego_frect_t tmp_rect;

    if ( NULL == ppass ) return bfalse;

    // Passage area
    radius += CLOSETOLERANCE;
    tmp_rect.xmin = ( ppass->area.xmin        * GRID_SIZE ) - radius;
    tmp_rect.ymin = ( ppass->area.ymin        * GRID_SIZE ) - radius;
    tmp_rect.xmax = (( ppass->area.xmax + 1 ) * GRID_SIZE ) + radius;
    tmp_rect.ymax = (( ppass->area.ymax + 1 ) * GRID_SIZE ) + radius;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
CHR_REF ego_passage::who_is_blocking( ego_passage * ppass, const CHR_REF by_reference isrc, IDSZ idsz, BIT_FIELD targeting_bits, IDSZ require_item )
{
    /// @details ZZ@> This function returns MAX_CHR if there is no character in the passage,
    ///    otherwise the index of the first character found is returned...
    ///    Can also look for characters with a specific quest or item in his or her inventory
    ///    Finds living ones, then items and corpses

    CHR_REF character, foundother;
    ego_chr *psrc;

    if ( NULL == ppass ) return ( CHR_REF )MAX_CHR;

    // Skip if the one who is looking doesn't exist
    if ( !INGAME_CHR( isrc ) ) return ( CHR_REF )MAX_CHR;
    psrc = ChrList.lst + isrc;

    // Look at each character
    foundother = ( CHR_REF )MAX_CHR;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        ego_chr * pchr;

        if ( !INGAME_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

        // don't do scenery objects unless we allow items
        if ( !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) && pchr->phys.weight == INFINITE_WEIGHT ) continue;

        // Check if the object has the requirements
        if ( !check_target( psrc, character, idsz, targeting_bits ) ) continue;

        // Now check if it actually is inside the passage area
        if ( ego_passage::object_is_in( ppass, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
        {
            // Found a live one, do we need to check for required items as well?
            if ( IDSZ_NONE == require_item )
            {
                return character;
            }

            // It needs to have a specific item as well
            else
            {
                CHR_REF item;

                // I: Check left hand
                if ( ego_chr::is_type_idsz( pchr->holdingwhich[SLOT_LEFT], require_item ) )
                {
                    // It has the item...
                    return character;
                }

                // II: Check right hand
                if ( ego_chr::is_type_idsz( pchr->holdingwhich[SLOT_RIGHT], require_item ) )
                {
                    // It has the item...
                    return character;
                }

                // III: Check the pack
                PACK_BEGIN_LOOP( item, pchr->pack.next )
                {
                    if ( ego_chr::is_type_idsz( item, require_item ) )
                    {
                        // It has the item in inventory...
                        return character;
                    }
                }
                PACK_END_LOOP( item );
            }
        }
    }

    // No characters found
    return foundother;
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::check_music( ego_passage * ppass )
{
    bool_t retval = bfalse;
    CHR_REF character = ( CHR_REF )MAX_CHR;
    PASS_REF passage;

    PLA_REF ipla;

    if ( NULL == ppass ) return bfalse;

    if ( ppass->music == NO_MUSIC || ppass->music == get_current_song_playing() )
    {
        return btrue;
    }

    // Look at each player
    retval = bfalse;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_chr * pchr;
        ego_player * ppla = PlaStack.lst + ipla;

        character = ppla->index;

        if ( !INGAME_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

        if ( !VALID_PLA( pchr->is_which_player ) || !pchr->alive || pchr->pack.is_packed ) continue;

        // Is it in the passage?
        if ( PassageStack_object_is_inside( passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
        {
            // Found a player, start music track
            sound_play_song( ppass->music, 0, -1 );
            retval = btrue;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::close( ego_passage * ppass )
{
    /// @details ZZ@> This function makes a passage impassable, and returns btrue if it isn't blocked

    int x, y;
    Uint32 fan, cnt;
    CHR_REF character;

    if ( NULL == ppass ) return bfalse;

    // don't compute all of this for nothing
    if ( 0 == ppass->mask ) return btrue;

    // check to see if a wall can close
    if ( 0 != HAS_SOME_BITS( ppass->mask, MPDFX_IMPASS | MPDFX_WALL ) )
    {
        size_t  numcrushed = 0;
        CHR_REF crushedcharacters[MAX_CHR];

        // Make sure it isn't blocked
        for ( character = 0; character < MAX_CHR; character++ )
        {
            ego_chr *pchr;

            if ( !INGAME_CHR( character ) ) continue;
            pchr = ChrList.lst + character;

            // Don't do held items
            if ( IS_ATTACHED_PCHR( pchr ) ) continue;

            if ( 0.0f != pchr->bump_stt.size )
            {
                if ( ego_passage::object_is_in( ppass, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
                {
                    if ( !pchr->canbecrushed )
                    {
                        // Someone is blocking, stop here
                        return bfalse;
                    }
                    else
                    {
                        crushedcharacters[numcrushed] = character;
                        numcrushed++;
                    }
                }
            }
        }

        // Crush any unfortunate characters
        for ( cnt = 0; cnt < numcrushed; cnt++ )
        {
            character = crushedcharacters[cnt];
            ADD_BITS( ego_chr::get_pai( character )->alert, ALERTIF_CRUSHED );
        }
    }

    // Close it off
    ppass->open = bfalse;
    for ( y = ppass->area.ymin; y <= ppass->area.ymax; y++ )
    {
        for ( x = ppass->area.xmin; x <= ppass->area.xmax; x++ )
        {
            fan = mesh_get_tile_int( PMesh, x, y );
            mesh_add_fx( PMesh, fan, ppass->mask );
        }
    }

    return btrue;
}

