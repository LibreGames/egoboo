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

/// \file passage.c
/// \brief Passages and doors and whatnot.  Things that impede your progress!

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
passage_stack PassageStack;
shop_stack    ShopStack;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void activate_passages_file_vfs()
{
    /// \author ZZ
    /// \details  This function reads the passage file

    ego_passage  tmp_passage;
    vfs_FILE  *fileread;

    // Reset all of the old passages
    passage_system_clear();

    // Load the file
    fileread = vfs_openRead( "mp_data/passage.txt" );
    if ( NULL == fileread ) return;

    while ( scan_passage_data_file( fileread, &tmp_passage ) )
    {
        PassageStack.add_one( &tmp_passage );
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
void passage_system_clear()
{
    /// \author ZZ
    /// \details  This function clears the passage list ( for doors )

    PassageStack.free_all();
    ShopStack.free_all();
}

//--------------------------------------------------------------------------------------------
// struct passage_stack
//--------------------------------------------------------------------------------------------
void passage_stack::free_all()
{
    count = 0;
}

//--------------------------------------------------------------------------------------------
int passage_stack::get_free()
{
    int ipass = MAX_PASS;

    if ( count < MAX_PASS )
    {
        ipass = count;
        count++;
    };

    return ipass;
}

//--------------------------------------------------------------------------------------------
bool_t passage_stack::open( const PASS_REF & passage )
{
    /// \author ZZ
    /// \details  This function makes a passage passable

    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    return ego_passage::do_open(( *this ) + passage );
}

//--------------------------------------------------------------------------------------------
void passage_stack::flash( const PASS_REF & passage, const Uint8 color )
{
    /// \author ZZ
    /// \details  This function makes a passage flash white

    if ( !INVALID_PASSAGE( passage ) )
    {
        ego_passage::flash(( *this ) + passage, color );
    }
}

//--------------------------------------------------------------------------------------------
bool_t passage_stack::point_is_inside( const PASS_REF & passage, const float xpos, const float ypos )
{
    bool_t useful = bfalse;

    if ( INVALID_PASSAGE( passage ) ) return useful;

    return ego_passage::point_is_in(( *this ) + passage, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
bool_t passage_stack::object_is_inside( const PASS_REF & passage, const float xpos, const float ypos, const float radius )
{
    bool_t useful = bfalse;

    if ( INVALID_PASSAGE( passage ) ) return useful;

    return ego_passage::object_is_in(( *this ) + passage, xpos, ypos, radius );
}

//--------------------------------------------------------------------------------------------
CHR_REF passage_stack::who_is_blocking( const PASS_REF & passage, const CHR_REF & isrc, IDSZ idsz, BIT_FIELD targeting_bits, IDSZ require_item )
{
    // Skip invalid passages
    if ( INVALID_PASSAGE( passage ) ) return CHR_REF( MAX_CHR );

    return ego_passage::who_is_blocking(( *this ) + passage, isrc, idsz, targeting_bits, require_item );
}

//--------------------------------------------------------------------------------------------
void passage_stack::check_music()
{
    /// \author ZF
    /// \details  This function checks all passages if there is a player in it, if it is, it plays a specified
    /// song set in by the AI script functions

    PASS_REF passage;

    // Check every music passage
    for ( passage = 0; passage < this->count; passage++ )
    {
        if ( ego_passage::check_music(( *this ) + passage ) )
        {
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t passage_stack::close_one( const PASS_REF & passage )
{
    // Skip invalid passages
    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    return ego_passage::close(( *this ) + passage );
}

//--------------------------------------------------------------------------------------------
void passage_stack::add_one( ego_passage * pdata )
{
    /// \author ZZ
    /// \details  This function creates a passage area

    PASS_REF    ipass;

    if ( NULL == pdata ) return;

    ipass = this->get_free();
    if ( ipass >= MAX_PASS ) return;

    ego_passage::init(( *this ) + ipass, pdata );
}

//--------------------------------------------------------------------------------------------
// struct shop_stack
//--------------------------------------------------------------------------------------------
void shop_stack::free_all()
{
    SHOP_REF cnt;

    for ( cnt = 0; cnt < MAX_PASS; cnt++ )
    {
        ego_shop * pshop = ( *this ) + cnt;
        if ( NULL == pshop ) continue;

        pshop->owner   = SHOP_NOOWNER;
        pshop->passage = 0;
    }

    count = 0;
}

//--------------------------------------------------------------------------------------------
int shop_stack::get_free()
{
    int ishop = MAX_PASS;

    if ( count < MAX_PASS )
    {
        ishop = count;
        count++;
    };

    return ishop;
}

//--------------------------------------------------------------------------------------------
void shop_stack::add_one( const CHR_REF & owner, const PASS_REF & passage )
{
    /// \author ZZ
    /// \details  This function creates a shop passage

    SHOP_REF ishop;
    CHR_REF  ichr;

    if ( !VALID_PASSAGE( passage ) ) return;

    if ( !INGAME_CHR( owner ) || !ChrObjList.get_data_ref( owner ).alive ) return;

    ishop = get_free();
    if ( !in_range_ref( ishop ) ) return;

    ego_shop * pshop = ( *this ) + ishop;

    // The passage exists...
    pshop->passage = passage;
    pshop->owner   = owner;

    // flag every item in the shop as a shop item
    CHR_BEGIN_LOOP_PROCESSING( ichr, pchr )
    {
        if ( !pchr->isitem ) continue;

        if ( PassageStack.object_is_inside( pshop->passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
        {
            pchr->isshopitem = btrue;               // Full value
            pchr->iskursed   = bfalse;              // Shop items are never kursed

            // Identify only cheap items in a shop
            /// \note BB@> not all shop items should be identified.
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
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
CHR_REF shop_stack::find_owner( const int ix, const int iy )
{
    /// \author ZZ
    /// \details This function returns the owner of a item in a shop

    SHOP_REF cnt;
    CHR_REF  owner = CHR_REF( SHOP_NOOWNER );

    for ( cnt = 0; cnt < count; cnt++ )
    {
        PASS_REF    passage;
        ego_passage * ppass;

        ego_shop * pshop = ( *this ) + cnt;

        passage = pshop->passage;

        if ( INVALID_PASSAGE( passage ) ) continue;
        ppass = PassageStack + passage;

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
// struct ego_passage
//--------------------------------------------------------------------------------------------
ego_passage * ego_passage::init( ego_passage * ppass, passage_data_t * pdata )
{
    if ( NULL == ppass ) return NULL;

    if ( NULL != pdata )
    {
        ppass->area.xmin  = CLIP( pdata->area.xmin, 0, ( ego_sint )PMesh->info.tiles_x - 1 );
        ppass->area.ymin  = CLIP( pdata->area.ymin, 0, ( ego_sint )PMesh->info.tiles_y - 1 );

        ppass->area.xmax  = CLIP( pdata->area.xmax, 0, ( ego_sint )PMesh->info.tiles_x - 1 );
        ppass->area.ymax  = CLIP( pdata->area.ymax, 0, ( ego_sint )PMesh->info.tiles_y - 1 );

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
                fan = ego_mpd::get_tile_int( PMesh, x, y );
                ego_mpd::clear_fx( PMesh, fan, MPDFX_WALL | MPDFX_IMPASS );
            }
        }
    }

    return useful;
}

//--------------------------------------------------------------------------------------------
void ego_passage::flash( ego_passage * ppass, const Uint8 color )
{
    int x, y, cnt;
    Uint32 fan;

    if ( NULL == ppass ) return;

    for ( y = ppass->area.ymin; y <= ppass->area.ymax; y++ )
    {
        for ( x = ppass->area.xmin; x <= ppass->area.xmax; x++ )
        {
            fan = ego_mpd::get_tile_int( PMesh, x, y );

            if ( !ego_mpd::grid_is_valid( PMesh, fan ) ) continue;

            for ( cnt = 0; cnt < 4; cnt++ )
            {
                PMesh->tmem.tile_list[fan].lcache[cnt] = color;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::point_is_in( ego_passage * ppass, const float xpos, const float ypos )
{
    /// \author ZF
    /// \details  This return btrue if the specified X and Y coordinates are within the passage

    ego_frect_t tmp_rect;

    // Passage area
    tmp_rect.xmin = ppass->area.xmin * GRID_SIZE;
    tmp_rect.ymin = ppass->area.ymin * GRID_SIZE;
    tmp_rect.xmax = ( ppass->area.xmax + 1 ) * GRID_SIZE;
    tmp_rect.ymax = ( ppass->area.ymax + 1 ) * GRID_SIZE;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::object_is_in( ego_passage * ppass, const float xpos, const float ypos, const float radius )
{
    /// \author ZF
    /// \details  This return btrue if the specified X and Y coordinates are within the passage
    ///     radius is how much offset we allow outside the passage

    ego_frect_t tmp_rect;

    if ( NULL == ppass ) return bfalse;

    // Passage area
    float tmp_radius = radius + CLOSETOLERANCE;
    tmp_rect.xmin = ( ppass->area.xmin        * GRID_SIZE ) - tmp_radius;
    tmp_rect.ymin = ( ppass->area.ymin        * GRID_SIZE ) - tmp_radius;
    tmp_rect.xmax = (( ppass->area.xmax + 1 ) * GRID_SIZE ) + tmp_radius;
    tmp_rect.ymax = (( ppass->area.ymax + 1 ) * GRID_SIZE ) + tmp_radius;

    return frect_point_inside( &tmp_rect, xpos, ypos );
}

//--------------------------------------------------------------------------------------------
CHR_REF ego_passage::who_is_blocking( ego_passage * ppass, const CHR_REF & isrc, IDSZ idsz, BIT_FIELD targeting_bits, IDSZ require_item )
{
    /// \author ZZ
    /// \details  This function returns MAX_CHR if there is no character in the passage,
    ///    otherwise the index of the first character found is returned...
    ///    Can also look for characters with a specific quest or item in his or her inventory
    ///    Finds living ones, then items and corpses

    CHR_REF foundother, found = CHR_REF( MAX_CHR );

    ego_chr *psrc;

    if ( NULL == ppass ) return CHR_REF( MAX_CHR );

    // Skip if the one who is looking doesn't exist
    psrc = ChrObjList.get_allocated_data_ptr( isrc );
    if ( !INGAME_PCHR( psrc ) ) return CHR_REF( MAX_CHR );

    // Look at each character
    foundother = CHR_REF( MAX_CHR );
    CHR_BEGIN_LOOP_PROCESSING( character, pchr )
    {
        if ( foundother != CHR_REF( MAX_CHR ) ) break;

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
                foundother = character;
                break;
            }

            // It needs to have a specific item as well
            else
            {
                CHR_REF item;

                // I: Check left hand
                if ( ego_chr::is_type_idsz( pchr->holdingwhich[SLOT_LEFT], require_item ) )
                {
                    // It has the item...
                    foundother = character;
                    break;
                }

                // II: Check right hand
                if ( ego_chr::is_type_idsz( pchr->holdingwhich[SLOT_RIGHT], require_item ) )
                {
                    // It has the item...
                    foundother = character;
                    break;
                }

                // III: Check the pack
                PACK_BEGIN_LOOP( item, pchr->pack.next )
                {
                    if ( ego_chr::is_type_idsz( item, require_item ) )
                    {
                        // It has the item in inventory...
                        foundother = character;
                        break;
                    }
                }
                PACK_END_LOOP( item );
            }
        }
    }
    CHR_END_LOOP();

    // No characters found
    return foundother;
}

//--------------------------------------------------------------------------------------------
bool_t ego_passage::check_music( ego_passage * ppass )
{
    bool_t retval = bfalse;
    CHR_REF character = CHR_REF( MAX_CHR );
    PASS_REF passage;

    if ( NULL == ppass ) return bfalse;

    if ( ppass->music == NO_MUSIC || ppass->music == get_current_song_playing() )
    {
        return btrue;
    }

    // Look at each player
    retval = bfalse;
    for ( player_deque::iterator ipla = PlaDeque.begin(); ipla != PlaDeque.end(); ipla++ )
    {
        ego_chr * pchr;

        character = ipla->index;

        pchr = ChrObjList.get_allocated_data_ptr( character );
        if ( !INGAME_PCHR( pchr ) ) continue;

        if ( !IS_PLAYER_PCHR( pchr ) || !pchr->alive || pchr->pack.is_packed ) continue;

        // Is it in the passage?
        if ( PassageStack.object_is_inside( passage, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
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
    /// \author ZZ
    /// \details  This function makes a passage impassable, and returns btrue if it isn't blocked

    int x, y;
    Uint32 fan, cnt;
    CHR_REF character;
    bool_t is_closed = btrue;

    if ( NULL == ppass ) return bfalse;

    // don't compute all of this for nothing
    if ( 0 == ppass->mask ) return btrue;

    is_closed = btrue;

    // check to see if a wall can close
    if ( 0 != HAS_SOME_BITS( ppass->mask, MPDFX_IMPASS | MPDFX_WALL ) )
    {
        size_t  numcrushed = 0;
        CHR_REF crushedcharacters[MAX_CHR];

        // Make sure it isn't blocked
        CHR_BEGIN_LOOP_PROCESSING( character, pchr )
        {
            // Don't do held items
            if ( IS_ATTACHED_PCHR( pchr ) ) continue;

            if ( 0.0f != pchr->bump_stt.size )
            {
                if ( ego_passage::object_is_in( ppass, pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
                {
                    if ( !pchr->canbecrushed )
                    {
                        // Someone is blocking, stop here
                        is_closed  = bfalse;
                        numcrushed = 0;
                        break;
                    }
                    else
                    {
                        crushedcharacters[numcrushed] = character;
                        numcrushed++;
                    }
                }
            }
        }
        CHR_END_LOOP();

        // Crush any unfortunate characters
        for ( cnt = 0; cnt < numcrushed; cnt++ )
        {
            character = crushedcharacters[cnt];
            ADD_BITS( ego_chr::get_pai( character )->alert, ALERTIF_CRUSHED );
        }
    }

    if ( is_closed )
    {
        // Close it off
        ppass->open = bfalse;
        for ( y = ppass->area.ymin; y <= ppass->area.ymax; y++ )
        {
            for ( x = ppass->area.xmin; x <= ppass->area.xmax; x++ )
            {
                fan = ego_mpd::get_tile_int( PMesh, x, y );
                ego_mpd::add_fx( PMesh, fan, ppass->mask );
            }
        }
    }

    return is_closed;
}

