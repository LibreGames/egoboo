/*******************************************************************************
*  WALLMAKE.C                                                                  *
*	- Functions for setting of 'auto-walls'              	                   *
*    Based on the EgoMap Wallmaker code by Fabian Lemke                        *    
*									                                           *
*   Copyright (C) 2010  Paul Mueller <pmtech@swissonline.ch>                   *
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

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include "editmain.h"


#include "wallmake.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int x, y;

} WALLMAKE_XY;

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

/* ------ Data for checking of adjacent tiles -------- */
static WALLMAKE_XY AdjacentXY[8] = {

    {  0, -1 }, { +1, -1 }, { +1,  0 }, { +1, +1 },
    {  0, +1 }, { -1, +1 }, { -1,  0 }, { -1, -1 }

};

static WALLMAKE_XY adj_xy[] = { 

    { +0, -128 } , { +128, -128 }, { +128, +0 }, { +128, +128 }, 
    { +0, +128 } , { -128, +128 }, { -128 + 0 }, { -128, -128 }, 
    { +0, -128 } , { +128, -128 } 
    
};

/*******************************************************************************
* CODE							                                               *
*******************************************************************************/

/*
 * Name:
 *     wallmakeGetAdjacent
 * Description:
 *     Creates a list of fans which are adjacent to given 'fan'. 
 *     If a fan is of map, the field is filled by a value of -1.
 *     The list starts from North', clockwise.  
 * Input:
 *     mesh*:      Pointer on mesh with info about map size
 *     fan:        To find the adjacent tiles for  
 *     adjacent *: Where to return the list of fan-positions 
 * Output: 
 *     Number of valid adjacent tiles 
 */
static int wallmakeGetAdjacent(MESH_T *mesh, int fan, int adjacent[8])
{

    int dir, adj_pos;
    int num_adj;
    WALLMAKE_XY src_xy, dest_xy;
    
    
    num_adj = 0;            /* Count them       */
    for (dir = 0; dir < 8; dir++) {
    
        adj_pos = -1;       /* Assume invalid   */

        src_xy.x = fan % mesh -> tiles_x;
        src_xy.y = fan / mesh -> tiles_x;
        
        dest_xy.x = src_xy.x + AdjacentXY[dir].x;
        dest_xy.y = src_xy.y + AdjacentXY[dir].y;
        
        if ((dest_xy.x >= 0) && (dest_xy.x < mesh -> tiles_x)
            && (dest_xy.y >= 0) && (dest_xy.y < mesh -> tiles_y)) {

            adj_pos = (dest_xy.y * mesh -> tiles_x) + dest_xy.x;
            num_adj++;

        }

        adjacent[dir] = adj_pos;       /* Starting north */

    }

    return num_adj;

}

/* ========================================================================== */
/* ============================= THE PUBLIC ROUTINES ======================== */
/* ========================================================================== */

/*
 * Name:
 *     wallmakeMakeTile
 * Description:
 *     Set a tile wall/floor, depending on the given argument 
 * Input:
 *     mesh*:      Pointer on mesh with info about map size
 *     fan:        To find the adjacent tiles for  
 *     is_floor:   True: Set a floor, otherwie create a wall    
 *     fan_list *: List with fan numbers and walltypes to create
 * Output:
 *     Number of fans to create in fan-list
 */
int wallmakeMakeTile(MESH_T *mesh, int fan, int is_floor, WALLMAKER_INFO_T *wi)
{

    int adjacent[8];
    char shape_no, rotdir;

    wallmakeGetAdjacent(mesh, fan, adjacent);

    return 0;

}