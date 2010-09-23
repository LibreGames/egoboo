#pragma once

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

/// @file quest.h
/// @brief read/write/modify the quest.txt file

#include "egoboo_typedef.h"


/// The definition of a quest
struct s_quest
{
	IDSZ quest_id;
	int	 quest_level;
};
typedef struct s_quest quest_t;

/// Quest system
#define QUEST_BEATEN         0x7FFFFF	//Same as MAX_INT
#define QUEST_NONE           -1
#define MAX_QUESTS			 64			//Max number of quests a player can have

void quest_log_download( quest_t *pquest_log, const char* player_directory );
bool_t quest_log_upload( quest_t *pquest_log, const char *player_directory );
int quest_set_level( quest_t *pquest_log, IDSZ idsz, int adjustment );
int quest_get_level( quest_t *pquest_log, IDSZ idsz );
bool_t quest_add( quest_t *pquest_log, IDSZ idsz, int level );
