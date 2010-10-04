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

#define WALLMAKE_NUMPATTERN_WALL     8
#define WALLMAKE_NUMPATTERN_MIDWALL 13
#define WALLMAKE_NUMPATTERN_FLOOR   24

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    char mask;      /* This are the tiles to check (AND-mask)       */
    char comp;      /* Compare with this pattern                    */
    char type;      /* Type to set at middle of pattern             */
    char dir;       /* Direction the fantype has to be rotated to   */

} WALLMAKE_PATTERN_T;

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

/* Patterns for case wall is in set: For each adjacent tile 3 patterns */
static WALLMAKE_PATTERN_T PatternsW[WALLMAKE_NUMPATTERN_WALL] = {
    /* Patterns for adjacent walls, if wall is set at center, special handling  */
    /* --- 0 -- */
    { 0x83, 0x01, WALLMAKE_WALL,  WALLMAKE_NORTH },
    /* --- 1 -- */
    { 0x07, 0x07, WALLMAKE_EDGEO, WALLMAKE_NORTH }, /* 'EDGEO' and direction are info for handling */
    /* --- 2 -- */
    { 0x0E, 0x0E, WALLMAKE_WALL,  WALLMAKE_EAST },
    /* --- 3 -- */
    { 0x1C, 0x1C, WALLMAKE_EDGEO, WALLMAKE_EAST }, 
    /* --- 4 -- */
    { 0x38, 0x38, WALLMAKE_WALL,  WALLMAKE_SOUTH },
    /* --- 5 -- */
    { 0x70, 0x70, WALLMAKE_EDGEO, WALLMAKE_SOUTH }, 
    /* --- 6 -- */
    { 0xE0, 0xE0, WALLMAKE_WALL,  WALLMAKE_WEST  }, 
    /* --- 7 -- */
    { 0xC1, 0xC1, WALLMAKE_EDGEO, WALLMAKE_WEST  } 
   
};

static WALLMAKE_PATTERN_T PatternsMW[WALLMAKE_NUMPATTERN_MIDWALL] = {
    /* Patterns for adjustment of tile type wall, if wall is set at center  */
    { 0xFF, 0xFF, WALLMAKE_TOP,   WALLMAKE_NORTH },
    { 0x83, 0x00, WALLMAKE_WALL,  WALLMAKE_NORTH },    
    { 0x07, 0x00, WALLMAKE_EDGEO, WALLMAKE_NORTH }, 
    { 0x07, 0x05, WALLMAKE_EDGEI, WALLMAKE_WEST },
    { 0x0E, 0x00, WALLMAKE_WALL,  WALLMAKE_EAST },
    { 0x1C, 0x00, WALLMAKE_EDGEO, WALLMAKE_EAST },
    { 0x1C, 0x14, WALLMAKE_EDGEI, WALLMAKE_NORTH }, 
    { 0x38, 0x00, WALLMAKE_WALL,  WALLMAKE_SOUTH }, 
    { 0x70, 0x00, WALLMAKE_EDGEO, WALLMAKE_SOUTH },     
    { 0x70, 0x50, WALLMAKE_EDGEI, WALLMAKE_EAST },
    { 0xE0, 0x00, WALLMAKE_WALL,  WALLMAKE_WEST },
    { 0xC1, 0x00, WALLMAKE_EDGEO, WALLMAKE_WEST },
    { 0xC1, 0x41, WALLMAKE_EDGEI, WALLMAKE_SOUTH }
};

/* Patterns for case floor is in set: For each adjacent tile 3 patterns */
static WALLMAKE_PATTERN_T PatternsF[WALLMAKE_NUMPATTERN_FLOOR] = {
    /* --- 0 -- */
    { 0x83, 0x83, WALLMAKE_WALL,  WALLMAKE_SOUTH },
    { 0x83, 0x81, WALLMAKE_EDGEO, WALLMAKE_EAST },
    { 0x83, 0x03, WALLMAKE_EDGEO, WALLMAKE_SOUTH }, 
    /* --- 1 -- */
    { 0x07, 0x07, WALLMAKE_EDGEI, WALLMAKE_EAST },  
    { 0x07, 0x03, WALLMAKE_WALL,  WALLMAKE_SOUTH },
    { 0x07, 0x06, WALLMAKE_WALL,  WALLMAKE_WEST },  
    /* --- 2 -- */
    { 0x0E, 0x0E, WALLMAKE_WALL,  WALLMAKE_WEST },
    { 0x0E, 0x06, WALLMAKE_EDGEO, WALLMAKE_SOUTH },
    { 0x0E, 0x0C, WALLMAKE_EDGEO, WALLMAKE_WEST },
    /* --- 3 -- */
    { 0x1C, 0x1C, WALLMAKE_EDGEI, WALLMAKE_SOUTH },
    { 0x1C, 0x0C, WALLMAKE_WALL,  WALLMAKE_WEST },
    { 0x1C, 0x18, WALLMAKE_WALL,  WALLMAKE_NORTH },
    /* --- 4 -- */
    { 0x38, 0x38, WALLMAKE_WALL,  WALLMAKE_NORTH },  
    { 0x38, 0x18, WALLMAKE_EDGEO, WALLMAKE_WEST }, 
    { 0x38, 0x30, WALLMAKE_EDGEO, WALLMAKE_NORTH },
    /* --- 5 -- */
    { 0x70, 0x70, WALLMAKE_EDGEI, WALLMAKE_WEST },  
    { 0x70, 0x30, WALLMAKE_WALL,  WALLMAKE_NORTH },
    { 0x70, 0x60, WALLMAKE_WALL,  WALLMAKE_EAST },
    /* --- 6 -- */
    { 0xE0, 0xE0, WALLMAKE_WALL,  WALLMAKE_EAST }, 
    { 0xE0, 0x60, WALLMAKE_EDGEO, WALLMAKE_NORTH },
    { 0xE0, 0xC0, WALLMAKE_EDGEO, WALLMAKE_EAST },
    /* --- 7 -- */
    { 0xC1, 0xC1, WALLMAKE_EDGEI, WALLMAKE_NORTH },  
    { 0xC1, 0xC0, WALLMAKE_WALL,  WALLMAKE_EAST }, 
    { 0xC1, 0x81, WALLMAKE_WALL,  WALLMAKE_SOUTH },

};

/*******************************************************************************
* CODE							                                               *
*******************************************************************************/

/*
 * Name:
 *     wallmakeFloor
 * Description:
 *     A floor is created at given fan-position.
 *     Fills the given list with tile-types needed to set around center tile
 * Input:
 *     pattern:  Pattern surrounding the fan as bits
 *     wi*:      Pointer on wallinfo to fill with info about tile types to create
 */
static void wallmakeFloor(char pattern, WALLMAKER_INFO_T *wi)
{

    int i, j;
    WALLMAKE_PATTERN_T *pf;

    
    for(i = 0; i < 8; i++) {

        pf = &PatternsF[i * 3];
        
        /* Loop trough all patterns for this wall-info */
        for (j = 0; j < 3; j++) {
        
            if ((pattern & pf[j].mask) == pf[j].comp) {
        
                wi[i].type = pf[j].type;
                wi[i].dir  = pf[j].dir;
                
            }

        }

    }

}

/*
 * Name:
 *     wallmakeWall
 * Description:
 *     A wall is created at given fan-position.
 *     Fills the given list with tile-types needed to set around center tile.
 *     Index 0: Has to be the center tile, because that may need adjustment, too 
 * Input:
 *     pattern:  Pattern surrounding the fan as bits
 *     wi*:      Pointer on wallinfo to fill with info about tile types to create
 */
static void wallmakeWall(char pattern, WALLMAKER_INFO_T *wi)
{

    int i;


    /* Adjust center wall based on given 'pattern' */
    for (i = 0; i < WALLMAKE_NUMPATTERN_MIDWALL; i++) {

        if ((pattern & PatternsMW[i].mask) == PatternsMW[i].comp) {

            wi[0].type = PatternsMW[i].type;
            wi[0].dir  = PatternsMW[i].dir;
            break;
        }

    }
    
    wi++;

    for(i = 0; i < 8; i++) {

        /* TODO: Set type depending on actual type in info */
        /*
            wi[i].type = pw[j].type;
            wi[i].dir  = pw[j].dir;
        */       
            
    }

}

/* ========================================================================== */
/* ============================= THE PUBLIC ROUTINES ======================== */
/* ========================================================================== */

/*
 * Name:
 *     wallmakeMakeTile
 * Description:
 *     Returns a list of nine values in 'WALLMAKER_INFO_T'. 
 *     Index 0 is the fan given as argument. It's needed for a possible
 *     adjustment of the given tile if it's a wall.
 *     The list of adjacent walls have to be given like this 
 *     -------
 *     |7|0|1|
 *     |-+-+-|
 *     |6| |2|
 *     |-+-+-|
 *     |5|4|3|
 *     ------- 
 *     This is used for the pattern recognition 
 * Input:
 *     is_floor: True: Set a floor, otherwie create a wall
 *     wi *:     List of adjacent tiles[1..8], 0: Has to be the middle tile 
 */
int wallmakeMakeTile(int is_floor, WALLMAKER_INFO_T *wi)
{

    char pattern, flag;
    int dir;


    if (is_floor) {

        if (wi[0].type == WALLMAKE_FLOOR) {
            /* No change at all */
            return 0;
        }

        wi[0].type = WALLMAKE_FLOOR;
        wi[0].dir  = 0;

    }
    else {

        if (wi[0].type != WALLMAKE_FLOOR) {
            /* No change at all */
            return 0;
        }
        
        wi[0].type = WALLMAKE_TOP;    /* Set a 'top' tile for start, if wall */

    }
 

    pattern = 0;
    for (dir = 1, flag = 0x01; dir < 9; dir++, flag <<= 0x01) {
        if (wi[dir].type != WALLMAKE_FLOOR) {
            pattern |= flag;       /* It's a wall          */
        }
    }

    /* Now we have a pattern of wall and floors surrounding this fan */
    if (is_floor) {

        /* Handle creating floor */
        wallmakeFloor(pattern, &wi[1]);

    }
    else {

        /* Handle creating a wall, center position needed for adjustment */
        wallmakeWall(pattern, wi);
    }
    
    return 1;

}