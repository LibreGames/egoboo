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
#include "IDSZ_map.h"

/// Quest system
#define QUEST_BEATEN         0x7FFFFFFF	//Same as MAX_INT
#define QUEST_NONE           -1

// Public functions
void quest_log_download_vfs( IDSZ_node_t *pquest_log, const char* player_directory );
egoboo_rv quest_log_upload_vfs( IDSZ_node_t *pquest_log, const char *player_directory );
int quest_set_level( IDSZ_node_t *pquest_log, IDSZ idsz, int adjustment );
int quest_get_level( IDSZ_node_t *pquest_log, IDSZ idsz );
egoboo_rv quest_add( IDSZ_node_t *pquest_log, IDSZ idsz, int level );
