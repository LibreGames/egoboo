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

/// @file quest.c
/// @brief Handles functions that modify quest.txt files and the players quest log
/// @details ZF> This could be done more optimal with a proper HashMap allowing O(1) speed instead of O(n)
///              I think we should also implement a similar system for skill IDSZ.

#include "quest.h"
#include "IDSZ_map.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
void quest_log_download_vfs( IDSZ_node_t *pquest_log, const char* player_directory )
{
	/// @details ZF@> Reads a quest.txt for a player and turns it into a data structure
	///				  we can use. If the file isn't found, the quest log will be initialized as empty.
    vfs_FILE *fileread;
    STRING newloadname;

	if( pquest_log == NULL ) return;

	// Figure out the file path
	snprintf( newloadname, SDL_arraysize( newloadname ), "%s/quest.txt", player_directory );
    fileread = vfs_openRead( newloadname );

	idsz_map_init( pquest_log );

    if ( NULL != fileread )
	{
		// Load each IDSZ
		while ( goto_colon( NULL, fileread, btrue ) )
		{
			egoboo_rv success;
			IDSZ idsz = fget_idsz( fileread );
			int level = fget_int( fileread );

			//Try to add a single quest to the map
			success = idsz_map_add( pquest_log, idsz, level );
			
			//Stop here if it failed
			if( !success )
			{
				log_warning("Unable to load all quests. (%s)", newloadname);
				break;
			}
		}

		// Close up after we are done with it
		vfs_close( fileread );
	}
}

//--------------------------------------------------------------------------------------------
egoboo_rv quest_log_upload_vfs( IDSZ_node_t *pquest_log, const char *player_directory )
{
	/// @details ZF@> This exports quest_log data into a quest.txt file
    vfs_FILE *filewrite;
	int iterator;
	IDSZ_node_t *pquest;

	if( pquest_log == NULL ) return rv_error;

	//Write a new quest file with all the quests
	filewrite = vfs_openWrite( player_directory );

	if ( !filewrite )
    {
        log_warning( "Cannot create quest file! (%s)\n", player_directory );
        return rv_fail;
    }

    vfs_printf( filewrite, "// This file keeps order of all the quests for this player\n" );
    vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. %i means it is completed.", QUEST_BEATEN );

	//Iterate through every element in the IDSZ map
	iterator = 0;
	pquest = idsz_map_iterate( pquest_log, &iterator );
	while( pquest != NULL )
	{
		// Write every single quest to the quest log
		vfs_printf( filewrite, "\n:[%4s] %i", undo_idsz( pquest->id ), pquest->level );

		//Get the next element
		pquest = idsz_map_iterate( pquest_log, &iterator );
	}

	// Clean up and return
    vfs_close( filewrite );
    return rv_success;
}

//--------------------------------------------------------------------------------------------
int quest_set_level( IDSZ_node_t *pquest_log, IDSZ idsz, int adjustment )
{
	///@details	ZF@> This function will modify the quest level for the specified quest with adjustment
	///			and return the new quest_level total. It will return QUEST_NONE if the quest was 
	///			not found or if it was already beaten.
	IDSZ_node_t *pquest = idsz_map_get( pquest_log, idsz );

	// Could not find the quest
	if( pquest == NULL ) return QUEST_NONE;

	// Don't modify quests that are already beaten
	if( pquest->level == QUEST_BEATEN ) return QUEST_NONE;
	
	// Modify the quest level for that specific quest
	if( adjustment == QUEST_BEATEN ) pquest->level = QUEST_BEATEN;
	else							 pquest->level = CLIP(pquest->level + adjustment, 0, QUEST_BEATEN);

	return pquest->level;
}

//--------------------------------------------------------------------------------------------
int quest_get_level( IDSZ_node_t *pquest_log, IDSZ idsz )
{
	///@details ZF@> Returns the quest level for the specified quest IDSZ.
	///				 It will return QUEST_NONE if the quest was not found or if the quest was beaten.
	IDSZ_node_t *pquest = idsz_map_get( pquest_log, idsz );

	if( pquest == NULL || pquest->level == QUEST_BEATEN ) return QUEST_NONE;
	
	return pquest->level;
}

//--------------------------------------------------------------------------------------------
egoboo_rv quest_add( IDSZ_node_t *pquest_log, IDSZ idsz, int level )
{
	///@details ZF@> This adds a new quest to the quest log. If the quest is already in there, the higher quest
	///				 level of either the old and new one will be kept.
	if( level >= QUEST_BEATEN ) return rv_fail;

	return idsz_map_add( pquest_log, idsz, level );
}
