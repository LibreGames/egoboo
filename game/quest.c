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
/// @details

#include "quest.h"
#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//"private" functions
quest_t* _quest_find( quest_t *pquest_log, IDSZ idsz );


//--------------------------------------------------------------------------------------------
void quest_log_download( quest_t *pquest_log, const char* player_directory )
{
	//ZF> Reads a quest.txt for a player and turns it into a data structure
	//	  we can use
	int quest_cnt = 0;
    vfs_FILE *fileread;
    STRING newloadname;

	// Figure out the file path
	snprintf( newloadname, SDL_arraysize( newloadname ), "%s/quest.txt", player_directory );
    fileread = vfs_openRead( newloadname );

    if ( NULL != fileread )
	{
		// Load each IDSZ
		while ( goto_colon( NULL, fileread, btrue ) )
		{
			if( quest_cnt + 1 == MAX_QUESTS )
			{
				log_warning("Could not load quest. Consider increasing MAX_QUESTS (currently %i)", MAX_QUESTS);
				break;
			}

			//Load a single quest
			pquest_log[quest_cnt].quest_id = fget_idsz( fileread );
			pquest_log[quest_cnt].quest_level = fget_int( fileread );

			quest_cnt++;
		}

		//Close up after we are done with it
		vfs_close( fileread );
	}

	//Terminate the end of the list with a IDSZ_NONE quest
	pquest_log[quest_cnt].quest_id		= IDSZ_NONE;
	pquest_log[quest_cnt].quest_level	= QUEST_NONE;
}

//--------------------------------------------------------------------------------------------
bool_t quest_log_upload( quest_t *pquest_log, const char *player_directory )
{
    vfs_FILE *filewrite;
	int i;

    // Try to open the file in read and append mode
	filewrite = vfs_openWrite( player_directory );

	if ( !filewrite )
    {
        log_warning( "Cannot create quest file! (%s)\n", player_directory );
        return bfalse;
    }

    vfs_printf( filewrite, "// This file keeps order of all the quests for this player\n" );
    vfs_printf( filewrite, "// The number after the IDSZ shows the quest level. %i means it is completed.", QUEST_BEATEN );

	for( i = 0; i < MAX_QUESTS; i++ )
	{
		//Reached the end of the quest list?
		if( pquest_log[i].quest_id == IDSZ_NONE ) break;

		//Write every single quest to the quest log
		vfs_printf( filewrite, "\n:[%4s] %i", undo_idsz( pquest_log[i].quest_id ), pquest_log[i].quest_level );
	}

    vfs_close( filewrite );
    return btrue;
}

//--------------------------------------------------------------------------------------------
int quest_set_level( quest_t *pquest_log, IDSZ idsz, int adjustment )
{
	//ZF> This function will modify the quest level for the specified quest with adjustment
	//     and return the new quest_level total. It will return QUEST_NONE if the quest was 
	//     not found or if it was already beaten.
	quest_t *quest = _quest_find( pquest_log, idsz );

	if( quest == NULL ) return QUEST_NONE;

	//Don't modify quests that are already beaten
	if( quest->quest_level == QUEST_BEATEN ) return QUEST_NONE;
	
	//Modify the quest level for that specific quest
	if( adjustment == QUEST_BEATEN ) quest->quest_level = QUEST_BEATEN;
	else							 quest->quest_level = CLIP(quest->quest_level + adjustment, 0, QUEST_BEATEN);

	return quest->quest_level;
}

//--------------------------------------------------------------------------------------------
int quest_get_level( quest_t *pquest_log, IDSZ idsz )
{
	//ZF> Returns the quest level for the specified quest IDSZ.
	//	  It will return QUEST_NONE if the quest was not found or if the quest was beaten.
	quest_t *pquest = _quest_find( pquest_log, idsz );

	if( pquest == NULL || pquest->quest_level == QUEST_BEATEN ) return QUEST_NONE;
	
	return pquest->quest_level;
}

//--------------------------------------------------------------------------------------------
bool_t quest_add( quest_t *pquest_log, IDSZ idsz, int level )
{
	//ZF> This adds a new quest to the quest log. If the quest is already in there, the higher quest
	//    level of either the old and new one will be kept.
	int i;

	if( idsz == IDSZ_NONE || level <= QUEST_NONE || level >= QUEST_BEATEN ) return bfalse;

	for( i = 0; i < MAX_QUESTS; i++ )
	{
		//Trying to add a quest to a full quest list?
		if( i + 1 == MAX_QUESTS )
		{
			log_warning("quest_add() - Could not add quest. Consider increasing MAX_QUESTS (currently %i)", MAX_QUESTS);
			return bfalse;
		}

		// Overwrite existing quest
		if( pquest_log[i].quest_id == idsz )
		{
			// But only if the new quest level is higher than the previous one
			if( pquest_log[i].quest_level >= level ) return bfalse;

			pquest_log[i].quest_level = level;
			return btrue;
		}

		// Simply append the quest if we reached the end of the list
		if( pquest_log[i].quest_id == IDSZ_NONE )
		{
			// Move the list termination down one step
			pquest_log[i+1].quest_id    = pquest_log[i].quest_id;
			pquest_log[i+1].quest_level = pquest_log[i].quest_level;

			//Add the new quest
			pquest_log[i].quest_id = idsz;
			pquest_log[i].quest_level = level;
			return btrue;
		}
	}

	return bfalse;
}

//--------------------------------------------------------------------------------------------
quest_t* _quest_find( quest_t *pquest_log, IDSZ idsz )
{
	//ZF> Returns a pointer to the quest_t if it was found in the pquest_log or NULL if it wasn't found
	int i;

	if( IDSZ_NONE == idsz || pquest_log == NULL )	return NULL;

	for( i = 0; i < MAX_QUESTS; i++)
	{
		//Reached the end of the list without finding the quest
		if ( pquest_log[i].quest_id == IDSZ_NONE )	return NULL;
		
		//Did we find the quest?
		if ( pquest_log[i].quest_id == idsz ) return &pquest_log[i];
	}

	return NULL;
}