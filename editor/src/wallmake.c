/*******************************************************************************
*  WALLMAKE.C                                                                  *
*	- Functions for setting of 'auto-walls'              	                   *
*    Based on the EgoMap Wallmaker code by Fabian Lemke                        *    
*									                                           *
*   Copyright (C) 2011  Paul Mueller <pmtech@swissonline.ch>                   *
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


#include "wallmake.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define WM_NUM_PATTERN_F    3

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

static WALLMAKE_PATTERN_T PatternsW[] = {
    /* Patterns for adjustment of tile type wall, if wall is set at center  */
    { 0xFF, 0xFF, WALLMAKE_TOP,   WALLMAKE_NORTH },
    /* Inner edge wall */
    { 0xFF, 0xF7, WALLMAKE_EDGEI, WALLMAKE_SOUTH }, /* WALLMAKE_SOUTH */
    { 0xFF, 0xDF, WALLMAKE_EDGEI, WALLMAKE_WEST },  /* WALLMAKE_WEST */
    { 0xFF, 0x7F, WALLMAKE_EDGEI, WALLMAKE_NORTH }, /* WALLMAKE_NORTH */
    { 0xFF, 0xFD, WALLMAKE_EDGEI, WALLMAKE_EAST },  /* WALLMAKE_EAST */
    /* Straight wall */
    { 0x7D, 0x7C, WALLMAKE_WALL,  WALLMAKE_NORTH },
    { 0xF5, 0xF1, WALLMAKE_WALL,  WALLMAKE_EAST },
    { 0xD7, 0xC7, WALLMAKE_WALL,  WALLMAKE_SOUTH },
    { 0x5F, 0x1F, WALLMAKE_WALL,  WALLMAKE_WEST },
    /* Outer edge wall */
    { 0x75, 0x70, WALLMAKE_EDGEO, WALLMAKE_NORTH },
    { 0xD5, 0xC1, WALLMAKE_EDGEO, WALLMAKE_EAST },
    { 0x57, 0x07, WALLMAKE_EDGEO, WALLMAKE_SOUTH },
    { 0x5D, 0x1C, WALLMAKE_EDGEO, WALLMAKE_WEST },
    { 0 }

};

/* Patterns for case floor is in set */
/* -- Patterns for edges: 1, 3, 5, 7 -- */
static WALLMAKE_PATTERN_T PatternsFE[] = {
    /* --- 1 (Edge only) -- */
    { 0x07, 0x07, WALLMAKE_EDGEI, WALLMAKE_EAST  }, /* WALLMAKE_EAST */
    /* --- 3 (Edge only) -- */
    { 0x1C, 0x1C, WALLMAKE_EDGEI, WALLMAKE_SOUTH }, /* WALLMAKE_SOUTH */
    /* --- 5 (Edge only) -- */
    { 0x70, 0x70, WALLMAKE_EDGEI, WALLMAKE_WEST },  /* WALLMAKE_WEST  */
    /* --- 7 (Edge only) -- */
    { 0xC1, 0xC1, WALLMAKE_EDGEI, WALLMAKE_NORTH }, /* WALLMAKE_NORTH */
    { 0 }

};
/* Patterns for case floor is in set: For each adjacent tile 3 patterns */
static WALLMAKE_PATTERN_T PatternsF[] = {
    /* --- 0 -- */
    { 0x83, 0x83, WALLMAKE_WALL,  WALLMAKE_SOUTH },
    { 0x83, 0x81, WALLMAKE_EDGEO, WALLMAKE_EAST },
    { 0x83, 0x03, WALLMAKE_EDGEO, WALLMAKE_SOUTH },
    /* --- 2 -- */
    { 0x0E, 0x0E, WALLMAKE_WALL,  WALLMAKE_WEST },
    { 0x0E, 0x06, WALLMAKE_EDGEO, WALLMAKE_SOUTH },
    { 0x0E, 0x0C, WALLMAKE_EDGEO, WALLMAKE_WEST },
    /* --- 4 -- */
    { 0x38, 0x38, WALLMAKE_WALL,  WALLMAKE_NORTH },
    { 0x38, 0x18, WALLMAKE_EDGEO, WALLMAKE_WEST },
    { 0x38, 0x30, WALLMAKE_EDGEO, WALLMAKE_NORTH },
    /* --- 6 -- */
    { 0xE0, 0xE0, WALLMAKE_WALL,  WALLMAKE_EAST },
    { 0xE0, 0x60, WALLMAKE_EDGEO, WALLMAKE_NORTH },
    { 0xE0, 0xC0, WALLMAKE_EDGEO, WALLMAKE_EAST },
    { 0 }

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
 *     wi*:      Pointer on wallinfo to fill with info about tile types to adjust
 */
static void wallmakeFloor(char pattern, WALLMAKER_INFO_T *wi)
{

    int i, j;
    WALLMAKE_PATTERN_T *pf;


    /* --- First the edges --- */
    for (i = 1, j = 0; i < 8; i += 2, j++){

        if ((pattern & PatternsFE[j].mask) == PatternsFE[j].comp) {

            wi[i].type = PatternsFE[j].type;
            wi[i].dir  = PatternsFE[j].dir;

        }

    }

    /* Then the straight walls */
    for(i = 0, pf = &PatternsF[0]; i < 8; i += 2, pf += WM_NUM_PATTERN_F) {

        /* Loop trough all patterns for this wall-info */
        for (j = 0; j < WM_NUM_PATTERN_F; j++) {

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

    WALLMAKE_PATTERN_T *pf;


    /* Adjust center wall based on given 'pattern' */
    pf = &PatternsW[0];

    while(pf -> mask) {

        if ((pattern & pf -> mask) == pf -> comp) {
            /* Adjust middle and adjacent tiles */
            wi[0].type = pf -> type;
            wi[0].dir  = pf -> dir;
            return;

        }

        pf++;

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
 *     wi *: List of center tile and its adjacent tiles [0] ist the center tile
  * Output:
 *     Number of walls adjusted to correct draw direction an correct wall type
 */
int wallmakeMakeTile(WALLMAKER_INFO_T *wi)
{

    char pattern, flag;
    int dir;


    pattern = 0;
    for (dir = 1, flag = 0x01; dir < 9; dir++, flag <<= 0x01) {
        if (wi[dir].type != WALLMAKE_FLOOR) {
            pattern |= flag;       /* It's a wall          */
        }
    }

    /* Now we have a pattern of wall and floors surrounding this fan */
    if (wi[0].type == WALLMAKE_FLOOR) {

        /* Adjusts all adjacent wall-tiles */
        wallmakeFloor(pattern, &wi[1]);
        return 9;

    }
    else {

        /* Adjusts the tile i the middle, depending on surrounding pattern */
        wallmakeWall(pattern, wi);
        return 1;

    }

}