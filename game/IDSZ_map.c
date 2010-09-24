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

/// @file IDSZ_map.c
/// @brief

#include "IDSZ_map.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
IDSZ_node_t * idsz_map_init( IDSZ_node_t * idsz_map )
{
    if( NULL == idsz_map ) return idsz_map;

    idsz_map[0].id = IDSZ_NONE;
    idsz_map[0].level = IDSZ_NOT_FOUND;

    return idsz_map;
}

//--------------------------------------------------------------------------------------------
egoboo_rv idsz_map_add( IDSZ_node_t idsz_map[], size_t idsz_map_len, IDSZ idsz, int level )
{
    /// @details ZF> Adds a single IDSZ with the specified level to the map. If it already exists
    ///              in the map, the higher of the two level values will be used.

    egoboo_rv rv = rv_error;
    size_t    i;

    if( NULL == idsz_map ) return rv_error;

    // we must allow the function to add any level of IDSZ, otherwize we cannot
    // add beaten quests, skills with negative levels, etc.
    if( IDSZ_NONE == idsz ) return rv_fail;

    for( i = 0; i < idsz_map_len; i++ )
    {
        // the end of the list?
        if( IDSZ_NONE == idsz_map[i].id ) break;

        // found a matching idsz?
        if( idsz == idsz_map[i].id )
        {
            // But only if the new idsz level is "better" than the previous one
            if( (level > 0 && idsz_map[i].level >= level) ||
                (level < 0 && idsz_map[i].level <= level) )
            {
                rv = rv_fail;
            }
            else
            {
                rv = rv_success;

                idsz_map[i].level = level;
            }

            break;
        }
    }

    //Trying to add a idsz to a full idsz list?
    if( idsz_map_len == i )
    {
        log_warning("idsz_map_add() - Failed to add [%s] to an IDSZ_map. Consider increasing idsz_map_len (currently %i)\n", idsz_map_len, undo_idsz(idsz) );
        return rv_fail;
    }
    else if( IDSZ_NONE == idsz_map[i].id )
    {
        // Reached the end of the list. Simply append the new idsz to idsz_map

        int tail = i+1;

        // Set the termination down one step, if that step exists
        if( tail < idsz_map_len )
        {
            idsz_map[tail].id    = IDSZ_NONE;
            idsz_map[tail].level = IDSZ_NOT_FOUND;
        }

        //Add the new idsz
        idsz_map[i].id    = idsz;
        idsz_map[i].level = level;

        rv = rv_success;
    }

    return rv_fail;
}

//--------------------------------------------------------------------------------------------
IDSZ_node_t* idsz_map_get( IDSZ_node_t idsz_map[], size_t idsz_map_len, IDSZ idsz )
{
    /// @details ZF> This function returns a pointer to the IDSZ_node_t from the IDSZ specified
    ///              or NULL if it wasn't found in the map.
    int iterator;
    IDSZ_node_t* pidsz;

    if( NULL == idsz_map || IDSZ_NONE == idsz ) return NULL;

    // initialize the loop
    iterator = 0;
    pidsz = idsz_map_iterate( idsz_map, idsz_map_len, &iterator );

    // iterate the map
    while( pidsz != NULL )
    {
        // Did we find the idsz?
        if( pidsz->id == idsz ) return pidsz;

        // Get the next element
        pidsz = idsz_map_iterate( idsz_map, idsz_map_len, &iterator );
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------
IDSZ_node_t* idsz_map_iterate( IDSZ_node_t idsz_map[], size_t idsz_map_len, int *iterator_ptr )
{
    /// @details ZF> This function iterates through a map containing any number of IDSZ_node_t
    ///              Returns NULL if there are no more elements to iterate.
    int step;

    if( NULL == idsz_map || NULL == iterator_ptr ) return NULL;

    step = *iterator_ptr;

    // Reached the end of the list without finding a matching idsz
    if ( step >= idsz_map_len || step < 0 ) return NULL;

    // Found the end of the list
    if( IDSZ_NONE == idsz_map[step].id ) return NULL;

    // Return the next IDSZ found
    ( *iterator_ptr )++;

    return idsz_map + step;
}

//--------------------------------------------------------------------------------------------
egoboo_rv idsz_map_copy( IDSZ_node_t *pcopy_from, IDSZ_node_t *pcopy_to )
{
    ///@details ZF@> This function copies one set of IDSZ map to another IDSZ map (exact)
    if( pcopy_from == NULL || pcopy_to == NULL ) return rv_error;

    // memcpy() is probably a lot more efficient than copying each element individually
    memcpy( pcopy_to, pcopy_from, sizeof(pcopy_to) );

    return rv_success;

    /*
    int iterator = 0;
    IDSZ_node_t *pidsz;

    //First clear the array we are copying to
    idsz_map_init( pcopy_to );

    //Iterate and copy each element exact
    pidsz = idsz_map_iterate( pcopy_from, &iterator );
    while( pidsz != NULL )
    {
        idsz_map_add( pcopy_to, pidsz->id, pidsz->level );
        pidsz = idsz_map_iterate( pcopy_from, &iterator );
    }
    */
}
