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

/// @file IDSZ_map.h
/// @brief 

#include "egoboo_typedef.h"

/// The definition of a single IDSZ element in a IDSZ map
struct s_IDSZ_node
{
	IDSZ id;
	int	 level;
};
typedef struct s_IDSZ_node IDSZ_node_t;

//Constants
#define IDSZ_NOT_FOUND           -1
#define MAX_IDSZ_MAP_SIZE		 32

// Public functions
IDSZ_node_t* idsz_map_get( IDSZ_node_t *pidsz_map, IDSZ idsz );
egoboo_rv idsz_map_add( IDSZ_node_t *pidsz_map, IDSZ idsz, int level );
void idsz_map_init( IDSZ_node_t *pidsz_map );

IDSZ_node_t* idsz_map_iterate( IDSZ_node_t *pidsz_map, int *iterator );
void idsz_map_copy( IDSZ_node_t *pcopy_from, IDSZ_node_t *pcopy_to );
