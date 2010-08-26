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

#define WALLMAKE_DEFAULT_TILE   ((char)1)
#define WALLMAKE_TOP_TILE       ((char)63)  /* Black texture    */

/* --------- Info for preset tiles ------- */
#define WALLMAKE_PRESET_FLOOR   0
#define WALLMAKE_PRESET_TOP     1
#define WALLMAKE_PRESET_WALL    2
#define WALLMAKE_PRESET_EDGEO   3
#define WALLMAKE_PRESET_EDGEI   4
#define WALLMAKE_PRESET_MAX     5

/* Now the tile numbers used by the wallmaker... */
#define WALLMAKE_FLOOR  ((char)0)
#define WALLMAKE_TOP    ((char)1)
#define WALLMAKE_WALL   ((char)8)
#define WALLMAKE_EDGEO  ((char)16)
#define WALLMAKE_EDGEI  ((char)19)

#define WALLMAKE_NORTH  0x00
#define WALLMAKE_EAST   0x01
#define WALLMAKE_SOUTH  0x02
#define WALLMAKE_WEST   0x03


#define WALLMAKE_NUMPATTERN_WALL    9
#define WALLMAKE_NUMPATTERN_FLOOR   4

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int x, y;

} WALLMAKE_XY;

typedef struct {

    int  mask;      /* This are the tiles to check (AND-mask)       */
    int  comp;      /* Compare with this pattern                    */
    char type;      /* Type to set at middle of pattern             */
    char dir;       /* Direction the fantape has to be rotated to   */

} WALLMAKE_PATTERN_T;

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

/* ------ Data for checking of adjacent tiles -------- */
static WALLMAKE_XY AdjacentXY[8] = {

    {  0, -1 }, { +1, -1 }, { +1,  0 }, { +1, +1 },
    {  0, +1 }, { -1, +1 }, { -1,  0 }, { -1, -1 }

};

/* --- Definition of preset tiles for 'simple' mode -- */
static FANDATA_T PresetTiles[] = {
    
    {  WALLMAKE_DEFAULT_TILE, 0, 0,  WALLMAKE_FLOOR },                         
    {  WALLMAKE_TOP_TILE,     0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_TOP },
    /* Walls, x/y values are rotated, if needed */
    {  64 + 10, 0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_WALL  },   /* Wall north            */
    {  64 + 1,  0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_EDGEO },   /* Outer edge north/east */
    {  64 + 3,  0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_EDGEI },   /* Inner edge north/west */
    { 0 }
    
};

static WALLMAKE_PATTERN_T PatternsW[WALLMAKE_NUMPATTERN_WALL] = {
    /* Patterns, if a wall is in the middle       */    
    { 0x1F, 0x11, WALLMAKE_WALL,  WALLMAKE_EAST  }, 
    { 0x7C, 0x44, WALLMAKE_WALL,  WALLMAKE_SOUTH },
    { 0XF1, 0x11, WALLMAKE_WALL,  WALLMAKE_WEST  },
    { 0xC7, 0x44, WALLMAKE_WALL,  WALLMAKE_NORTH },     
    { 0x07, 0x00, WALLMAKE_EDGEO, WALLMAKE_NORTH }, /* Outside North & West */
    { 0x1C, 0x00, WALLMAKE_EDGEO, WALLMAKE_EAST  }, /* Outside North & East */
    { 0x70, 0x00, WALLMAKE_EDGEO, WALLMAKE_SOUTH }, /* Outside South & East */
    { 0xC1, 0x00, WALLMAKE_EDGEO, WALLMAKE_WEST  }, /* Outside South & West */
    { 0x55, 0x55, WALLMAKE_TOP,   WALLMAKE_NORTH }  /* Top Wall             */
};

/* TODO: Add  patterns for case floor */
static WALLMAKE_PATTERN_T PatternsF[WALLMAKE_NUMPATTERN_FLOOR] = {
    { 0x07, 0x07, WALLMAKE_EDGEI, WALLMAKE_EAST  }, /* Inside North & West  */
    { 0x1C, 0x1C, WALLMAKE_EDGEI, WALLMAKE_SOUTH }, /* Inside South & East  */  
    { 0x70, 0x70, WALLMAKE_EDGEI, WALLMAKE_WEST  }, /* Inside South & West  */  
    { 0xC1, 0xC1, WALLMAKE_EDGEI, WALLMAKE_NORTH } /* Inside North & East  */      
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

/*
 * Name:
 *     wallmakeFloor
 * Description:
 *     A floor is created at given fan-position.
 *     Fills the given list with tile-types needed  
 * Input:
 *     pattern:  Pattern surrounding the fan as bits     
 *     adjacent: Numbers of adjacent fans 
 *     wi *;     To fill with info about fan-types needed 
 */
static void wallmakeFloor(char pattern, int  adjacent[8], WALLMAKER_INFO_T *wi)
{

}

/*
 * Name:
 *     wallmakeWall
 * Description:
 *     A wall is created at given fan-position.
 *     Fills the given list with tile-types needed  
 * Input:
 *     pattern:  Pattern surrounding the fan as bits     
 *     adjacent: Numbers of adjacent fans 
 *     wi *;     To fill with info about fan-types needed 
 */
static void wallmakeWall(char pattern, int  adjacent[8], WALLMAKER_INFO_T *wi)
{

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
 *     mesh*:    Pointer on mesh with info about map size
 *     fan:      To find the adjacent tiles for  
 *     is_floor: True: Set a floor, otherwie create a wall    
 *     wi *:     List with fan numbers and walltypes to create
  * Output: 
 *     Number of fans to create in fan-list
 */
int wallmakeMakeTile(MESH_T *mesh, int fan, int is_floor, WALLMAKER_INFO_T *wi)
{

    int  adjacent[8];
    char shape_no, rotdir;
    int  lut_idx;
    int  dir, i;
    char pattern, flags;
    int  base_x, base_y;    
  

    if (is_floor) {

        if (mesh -> fan[fan].type == WALLMAKE_FLOOR) {
            /* No change at all */
            return 0;
        }

    }
    else {

        if (mesh -> fan[fan].type != WALLMAKE_FLOOR) {
            /* No change at all */
            return 0;
        }

    }

    /* Set the chosen fan itself for changing */
    wi[0].pos     = fan;
    wi[0].ft.type = is_floor ? WALLMAKE_FLOOR : WALLMAKE_TOP; /* Set a 'top' tile for start, if wall */
    wi[0].dir     = 0;


    wallmakeGetAdjacent(mesh, fan, adjacent);   /* Sampling area    */

    /* Fill in the flags for the look-up-table */
    pattern = 0;
    for (dir = 0, flags = 0x01; dir < 8; dir++, flags <<= 0x01) {
        if (-1 == adjacent[dir]) {
            pattern |= flags;       /* Handle it like a wall */
        }
        else if (mesh -> fan[adjacent[dir]].type != WALLMAKE_FLOOR) {
            pattern |= flags;       /* It's a wall          */
        }
    }

    /* Now we have a pattern of wall and floors surrounding this fan */

    if (is_floor) {

        /* Handle creating floor */
        wallmakeFloor(pattern, adjacent, wi);
        

    }
    else {

        /* Handle creating a wall */
        wallmakeWall(pattern, adjacent, wi);
    }

    return 0;

}