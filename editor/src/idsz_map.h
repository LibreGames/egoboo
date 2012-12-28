/*******************************************************************************
*  IDSZ_MAP.H                                                                  *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - IDSZ-Map for Game                                                                   *
*      (c)2012 Paul Mueller <muellerp61@bluewin.ch>                            *
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*   GNU Library General Public License for more details.                       *
*                                                                              *
*   You should have received a copy of the GNU General Public License          *
*   along with this program; if not, write to the Free Software                *
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*******************************************************************************/

#ifndef _IDSZ_MAP_H_
#define _IDSZ_MAP_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define IDSZ_NOT_FOUND      -1
#define MAX_IDSZ_MAP_SIZE   64

/*******************************************************************************
* TYPEDEFS 								                                       *
*******************************************************************************/

/// The definition of a single IDSZ element in a IDSZ map
typedef struct
{
    unsigned int id;
    int  level;
} IDSZ_NODE_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

// Public functions
/*
    IDSZ_node_t* idsz_map_init( IDSZ_node_t pidsz_map[] );
    egoboo_rv    idsz_map_add( IDSZ_node_t idsz_map[], const size_t idsz_map_len, const IDSZ idsz, const int level );

    IDSZ_node_t* idsz_map_get( const IDSZ_node_t pidsz_map[], const size_t idsz_map_len, const IDSZ idsz );
    IDSZ_node_t* idsz_map_iterate( const IDSZ_node_t pidsz_map[], const size_t idsz_map_len, int *iterator );
    egoboo_rv    idsz_map_copy( const IDSZ_node_t pcopy_from[], const size_t idsz_map_len, IDSZ_node_t pcopy_to[] );
 */

#endif  /* #define _IDSZ_MAP_H_ */