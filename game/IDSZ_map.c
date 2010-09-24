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
void idsz_map_init( IDSZ_node_t *pidsz_map )
{
	pidsz_map[0].id = IDSZ_NONE;
	pidsz_map[0].level = IDSZ_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------
egoboo_rv idsz_map_add( IDSZ_node_t *pidsz_map, IDSZ idsz, int level )
{
	/// @details ZF> Adds a single IDSZ with the specified level to the map. If it already exists
	///              in the map, the higher of the two level values will be used.
	int i;

	if( pidsz_map == NULL || level < 0 ) return rv_error;

	if( idsz == IDSZ_NONE ) return rv_fail;

	for( i = 0; i < MAX_IDSZ_MAP_SIZE; i++ )
	{
		//Trying to add a quest to a full quest list?
		if( i + 1 == MAX_IDSZ_MAP_SIZE )
		{
			log_warning("idsz_map_add() - Failed to add [%s] to an IDSZ_map. Consider increasing MAX_IDSZ_MAP_SIZE (currently %i)\n", MAX_IDSZ_MAP_SIZE, undo_idsz(idsz) );
			return rv_error;
		}

		// Overwrite existing quest
		if( pidsz_map[i].id == idsz )
		{
			// But only if the new quest level is higher than the previous one
			if( pidsz_map[i].level >= level ) return rv_fail;

			pidsz_map[i].level = level;
			return rv_success;
		}

		// Simply append the new quest to pquest_log if we reached the end of the list
		if( pidsz_map[i].id == IDSZ_NONE )
		{
			// Move the list termination down one step
			pidsz_map[i+1].id    = pidsz_map[i].id;
			pidsz_map[i+1].level = pidsz_map[i].level;

			//Add the new quest
			pidsz_map[i].id = idsz;
			pidsz_map[i].level = level;
			return rv_success;
		}
	}

	return rv_fail;
}

//--------------------------------------------------------------------------------------------
IDSZ_node_t* idsz_map_get( IDSZ_node_t *pidsz_map, IDSZ idsz )
{
	/// @details ZF> This function returns a pointer to the IDSZ_node_t from the IDSZ specified
	///              or NULL if it wasn't found in the map.
	int iterator;
	IDSZ_node_t* pidsz;

	if( IDSZ_NONE == idsz || pidsz_map == NULL ) return NULL;

	iterator = 0;
	pidsz = idsz_map_iterate( pidsz_map, &iterator );
	while( pidsz != NULL )
	{
		//Did we find the quest?
		if( pidsz->id == idsz ) return pidsz;

		//Get the next element
		pidsz = idsz_map_iterate( pidsz_map, &iterator );
	}

	return NULL;
}

//--------------------------------------------------------------------------------------------
IDSZ_node_t* idsz_map_iterate( IDSZ_node_t *pidsz_map, int *iterator )
{
	/// @details ZF> This function iterates through a map containing any number of IDSZ_node_t
	///              Returns NULL if there are no more elements to iterate.
	int step = *iterator;

	//Reached the end of the list without finding the quest
	if ( step >= MAX_IDSZ_MAP_SIZE || step < 0 ) return NULL;

	if( pidsz_map == NULL || pidsz_map[step].id == IDSZ_NONE ) return NULL;

	//Return the next IDSZ found
	( *iterator )++;
	return &pidsz_map[step];
}

//--------------------------------------------------------------------------------------------
void idsz_map_copy( IDSZ_node_t *pcopy_from, IDSZ_node_t *pcopy_to )
{
	///@details ZF@> This function copies one set of IDSZ map to another IDSZ map (exact)
	if( pcopy_from == NULL || pcopy_to == NULL ) return;

	//memcpy() is probably a lot more efficient than copying each element individually
	memcpy( pcopy_to, pcopy_from, sizeof(pcopy_to) ); 

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
