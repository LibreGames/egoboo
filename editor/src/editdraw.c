/*******************************************************************************
*  EDITDRAW.H                                                                  *
*	- Load and write files for the editor	                                   *
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

#include <stdio.h>
#include <memory.h>

#include "sdlgl.h"      /* OpenGL-Stuff                                 */
#include "sdlgl3d.h"    /* Helper routines for drawing of 3D-Screen     */    
#include "sdlgltex.h"   /* Texture handling                             */


#include "editdraw.h"   /* Own header                                   */   

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITDRAW_MAXWALLSUBTEX   64     /* Images per wall texture   */
#define EDITDRAW_MAXWALLTEX       4     /* Maximum wall textures     */ 
#define EDITDRAW_TILEDIV        128

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

static float DefaultUV[] = { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00 };
static COMMAND_T MeshCommand[MAXMESHTYPE] = {
    {  "0: Two Faced Ground",
        0,          /* Default FX: None         */
        4,		    /* Total number of vertices */
    	1,    		/*  1 Command               */
    	{ 4 },		/* Commandsize (how much vertices per command)  */
    	{ 1, 2, 3, 0 },
    	{ 0.001, 0.001, 0.124, 0.001, 0.124, 0.124, 0.001, 0.124 },
        { 0.001, 0.001, 0.249, 0.001, 0.249, 0.249, 0.001, 0.249 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 }
        }
    },
    {   "1: Two Faced Ground",
        0,          /* Default FX: None         */
    	4,
        1,
        { 4 },
        { 0, 1, 2, 3 },
        { 0.001, 0.001, 0.124, 0.001, 0.124, 0.124, 0.001, 0.124 },
        { 0.001, 0.001, 0.249, 0.001, 0.249, 0.249, 0.001, 0.249 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  5, 0.50, 0.50,  64.00,  64.00, 0.00 }
        }

    },
    {   "2: Four Faced Ground",
        0,          /* Default FX: None         */
        5,
        1,
        { 6 },
        { 4, 3, 0, 1, 2, 3 },
        { 0.001, 0.001, 0.124, 0.001, 0.124, 0.124, 0.001, 0.124, 0.0625, 0.0625 },
        { 0.001, 0.001, 0.249, 0.001, 0.249, 0.249, 0.001, 0.249, 0.125, 0.125 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  5, 0.50, 0.50,  64.00,  64.00, 0.00 }         
        }
    },
    {   "3: Eight Faced Ground",
        0,          /* Default FX: None         */
        9,
        1,
	    { 10 },
        { 8, 3, 7, 0, 4, 1, 5, 2, 6, 3 }, /* Number of vertices */
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,  0.50, 0.00,
          1.00, 0.50,  0.50, 1.00,  0.00, 0.50,  0.50, 0.50 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,  0.50, 0.00,
          1.00, 0.50,  0.50, 1.00,  0.00, 0.50,  0.50, 0.50 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  2, 0.50, 0.00,  64.00,   0.00, 0.00 },
          { 11, 1.00, 0.50, 128.00,  64.00, 0.00 },
          { 13, 0.50, 1.00,  64.00, 128.00, 0.00 },
          {  4, 0.00, 0.50,   0.00,  64.00, 0.00 },
          {  5, 0.50, 0.50,  64.00,  64.00, 0.00 }
        }
    },
    {   "4:  Ten Face Pillar",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Defualt FX   */
        8,
        2,
        { 8, 6 },
        {  7, 3, 0, 4, 5, 6, 2, 3,
           5, 4, 0, 1, 2, 6 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 0.00 },  
          {  6, 0.66, 0.33,  84.00,  42.00, 0.00 }, 
          { 10, 0.66, 0.66,  84.00,  84.00, 0.00 }, 
          {  9, 0.33, 0.66,  42.00,  84.00, 0.00 }
        }
    },
    {   "5: Eighteen Faced Pillar",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
  	    16,
        4,
        { 10, 8, 4, 4 },
        { 15, 3, 10, 11, 12, 13, 14, 8, 9, 3,
          13, 12, 4, 5, 1, 6, 7, 14,
          12, 11, 0, 4,
          14, 7, 2, 8 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 0.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 0.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 0.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 0.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 0.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 0.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 0.00 },
          {  6, 0.66, 0.33,  84.00,  42.00, 0.00 },
          { 10, 0.66, 0.66,  84.00,  84.00, 0.00 },
          {  9, 0.33, 0.66,  42.00,  84.00, 0.00 }
        }
    },
    {   "6: Blank", 0, 0, 0 },
    {   "7: Blank", 0, 0, 0 },
    {   "8: Six Faced Wall (WE)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    8,
      	2,
      	{ 6, 4 },
      	{  5, 2, 3, 6, 7, 4,
           4, 7, 0, 1 },
      	{ 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.00, 0.66,  0.00, 0.33 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.00, 0.66,  0.00, 0.33 },
        { {  0, 0.00, 0.00,   0.00,   0.00,   0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00,   0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 192.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 192.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 128.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 192.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 192.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 128.00 },
        }  

    },
    {   "9: Six Faced Wall (NS)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    8,
      	2,
      	{ 6, 4 },
      	{ 7, 3, 0, 4, 5, 6,
          6, 5, 1, 2 },
      	{ 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 0.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 0.00 }, 
        }
    },
    {   "10: Blank", 0, 0, 0 },
    {   "11: Blank", 0, 0, 0 },
    {   "12: Eight Faced Wall (W)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    8,
        2,
        { 8, 4 },
        { 7, 3, 4, 5, 6, 1, 2, 3,
          1, 6, 5, 0 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.33,  0.66, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 0.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 0.00 },
          {  6, 0.66, 0.33,  84.00,  42.00, 0.00 },
          { 10, 0.66, 0.66,  84.00,  84.00, 0.00 }
        }
    },
    {   "13: Eight Faced Wall (N)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
    	8,
        2,
        { 8, 4 },
        { 7, 3, 0, 4, 5, 6, 2, 3,
          2, 6, 5, 1 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 0.66,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          { 10, 0.66, 0.66,  84.00,  84.00, 0.00 },
          {  9, 0.33, 0.66,  42.00,  84.00, 0.00 }
        }
    },
    {  "14: Eight Faced Wall (E)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
        8,
        2,
        { 8, 4 },
        { 6, 3, 0, 1, 4, 5, 7, 3,
          3, 7, 5, 2 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.33, 0.33,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.33, 0.33,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 0.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 0.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 0.00 },
          {  9, 0.33, 0.66,  42.00,  84.00, 0.00 },
        } 
    },
    {   "15: Eight Faced Wall (S)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
    	8,
       	2,
       	{ 8, 4 },
       	{ 7, 5, 6, 0, 1, 2, 4, 5,
          0, 6, 5, 3 },
       	{ 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.66, 1.00,  0.33, 1.00,  0.33, 0.33,  0.66, 0.33 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.66, 1.00,  0.33, 1.00,  0.33, 0.33,  0.66, 0.33 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 0.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 0.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 0.00 },
          {  6, 0.66, 0.33,  84.00,  42.00, 0.00 }
        }
    },
    {   "16: Ten Faced Wall (WS)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
    	10,
         2,
        { 8, 6 },
        { 9, 3, 6, 7, 8, 4, 5, 3,
          8, 7, 0, 1, 2, 4 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.66, 0.33,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
    	  0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.66, 0.33,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00,   0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00,   0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00,   0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 192.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 128.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 192.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 192.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 128.00 },
          {  6, 0.66, 0.33,  84.00,  42.00, 128.00 },
          {  9, 0.33, 0.66,  42.00,  84.00, 192.00 }
        }
          
    },
    {   "17: Ten Faced Wall (NW)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    10,
        2,
        { 8, 6 },
        { 8, 6, 7, 0, 4, 5, 9, 6,
          9, 5, 1, 2, 3, 6 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 0.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 0.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 0.00 },
          { 10, 0.66, 0.66,  84.00,  84.00, 0.00 } 
        }
    },
    {   "18: Ten Faced Wall (NE)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
    	10,
        2,
        { 8, 6 },
        { 8, 9, 4, 5, 1, 6, 7, 9,
          9, 7, 2, 3, 0, 4 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 0.33,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 0.33,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 0.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 0.00 },
          {  6, 0.66, 0.33,  84.00,  42.00, 0.00 },
          {  9, 0.33, 0.66,  42.00,  84.00, 0.00 }
        }
    },
    {   "19:  Ten Faced Wall (ES)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    10,
        2,
        { 8, 6 },
        { 9, 7, 8, 4, 5, 2, 6, 7,
          8, 7, 3, 0, 1, 4 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
	      0.33, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
	      0.33, 0.33,  0.66, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 192.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 192.00 },
          { 15, 1.00, 1.00, 128.00, 128.00,   0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 192.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 192.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 128.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 128.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 192.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 192.00 },
          { 10, 0.66, 0.66,  84.00,  84.00, 128.00 }
        }
        
    },
    {   "20: Twelve Faced Wall (WSE)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
        12,
        3,
        { 9, 5, 4 },
        { 11, 3, 8, 9, 4, 10, 6, 7, 3,
          10, 4, 5, 2, 6,
          4, 9, 0, 1 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 0.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 0.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 0.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 0.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 0.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 0.00 },
          { 10, 0.66, 0.66,  84.00,  84.00, 0.00 },
          {  9, 0.33, 0.66,  42.00,  84.00, 0.00 } 
        }
    },
    {   "21: Twelve Faced Wall (NWS)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    12,
        3,
        { 9, 5, 4 },
        { 10, 8, 9, 0, 4, 5, 6, 11, 8,
          11, 6, 7, 3, 8,
          6, 5, 1, 2 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 }, 
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 0.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 0.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 0.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 0.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 0.00 },
          {  9, 0.33, 0.66,  42.00,  84.00, 0.00 }
        }
    },
    {   "22: Twelve Faced Wall (ENW)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    12,
        3,
        { 9, 5, 4 },
        { 11, 8, 10, 4, 5, 1, 6, 7, 8,
          10, 8, 9, 0, 4,
           8, 7, 2, 3 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.66, 0.33 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.33, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 0.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 0.00 },
          {  8, 0.00, 0.66,   0.00,  84.00, 0.00 },
          {  4, 0.00, 0.33,   0.00,  42.00, 0.00 },
          {  5, 0.33, 0.33,  42.00,  42.00, 0.00 },
          {  6, 0.66, 0.33,  84.00,  42.00, 0.00 }
        }
    },
    {   "23:  Twelve Faced Wall (SEN)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
	    12,
        3,
        { 9, 5, 4 },
        { 11, 9, 4, 10, 6, 7, 2, 8, 9,
          10, 4, 5, 1, 6,
          4, 9, 3, 0 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.66, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.66, 0.33,  0.66, 0.66 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  1, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.66, 0.00,  84.00,   0.00, 0.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 0.00 },
          { 11, 1.00, 0.66, 128.00,  84.00, 0.00 },
          { 14, 0.66, 1.00,  84.00, 128.00, 0.00 },
          { 13, 0.33, 1.00,  42.00, 128.00, 0.00 },
          {  6, 0.66, 0.33,  84.00,  42.00, 0.00 },
          { 10, 0.66, 0.66,  84.00,  82.00, 0.00 } 
        }
    },
    {   "24: Twelve Faced Stair (WE)",
        MPDFX_IMPASS,           /* Default FX   */
	    14,
        3,
        { 6, 6, 6 },
        { 13, 3, 0, 4, 5, 12,
          11, 12, 5, 6, 7, 10,
          9, 10, 7, 8, 1, 2 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.16, 0.00,  0.33, 0.00,  0.50, 0.00,  0.66, 0.00,
          0.83, 0.00,  0.83, 1.00,  0.66, 1.00,  0.50, 1.00,
          0.33, 1.00,  0.16, 1.00 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.16, 0.00,  0.33, 0.00,  0.50, 0.00,  0.66, 0.00,
          0.83, 0.00,  0.83, 1.00,  0.66, 1.00,  0.50, 1.00,
          0.33, 1.00,  0.16, 1.00 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },  
          {  1, 0.16, 0.00,  21.00,   0.00, 0.00 },
          {  2, 0.33, 0.00,  42.00,   0.00, 0.00 },
          {  2, 0.50, 0.00,  64.00,   0.00, 0.00 },
          {  3, 0.66, 0.00,  85.00,   0.00, 0.00 },
          {  3, 0.83, 0.00, 106.00,   0.00, 0.00 },
          { 14, 0.83, 1.00, 106.00, 128.00, 0.00 }, 
          { 14, 0.66, 1.00,  85.00, 128.00, 0.00 },
          { 13, 0.50, 1.00,  64.00, 128.00, 0.00 }, 
          { 13, 0.33, 1.00,  42.00, 128.00, 0.00 }, 
          { 12, 0.16, 1.00,  21.00, 128.00, 0.00 }, 
        }
    },
    {   "25: Twelve Faced Stair (NS)",
        MPDFX_IMPASS,           /* Default FX   */
    	14,
        3,
        { 6, 6, 6 },
        { 13, 0, 1, 4, 5, 12,
          11, 12, 5, 6, 7, 10,
          9, 10, 7, 8, 2, 3 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.16,  1.00, 0.33,  1.00, 0.50,  1.00, 0.66,
          1.00, 0.83,  0.00, 0.83,  0.00, 0.66,  0.00, 0.50,
          0.00, 0.33,  0.00, 0.16 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.16,  1.00, 0.33,  1.00, 0.50,  1.00, 0.66,
          1.00, 0.83,  0.00, 0.83,  0.00, 0.66,  0.00, 0.50,
          0.00, 0.33,  0.00, 0.16 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },  
          {  3, 1.00, 0.16, 128.00,  21.00, 0.00 },
          {  7, 1.00, 0.33, 128.00,  42.00, 0.00 },
          {  7, 1.00, 0.50, 128.00,  64.00, 0.00 },
          { 11, 1.00, 0.66, 128.00,  85.00, 0.00 },
          { 11, 1.00, 0.83, 128.00, 106.00, 0.00 },
          {  8, 0.00, 0.83,   0.00, 106.00, 0.00 },
          {  8, 0.00, 0.66,   0.00,  85.00, 0.00 },
          {  4, 0.00, 0.50,   0.00,  64.00, 0.00 },
          {  4, 0.00, 0.33,   0.00,  41.00, 0.00 },
          {  0, 0.00, 0.16,   0.00,  21.00, 0.00 }
        }
    }
};

static float  MeshTileOffUV[EDITDRAW_MAXWALLSUBTEX * 2];    /* Offset into wall texture */
static GLuint WallTex[EDITDRAW_MAXWALLTEX];
static COMMAND_T ChosenFanType;                             /* For display of this one  */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 *  Name:
 *	    editdrawChosenFanType
 *  Description:
 *      Draws the fan 'ChosenFanType' in wireframe-mode in white color.
 *      Elevated by 10 Units above Z=0
 * Input:
 *     None
 */
static void editdrawChosenFanType(void)
{
    
    int cnt, tnc;
    int entry;
    
    
    /* FIXME: Save and restore state */
    glPolygonMode(GL_FRONT, GL_LINE);
    sdlglSetColor(SDLGL_COL_WHITE);    
    
    entry = 0;
    
    for (cnt = 0; cnt < ChosenFanType.count; cnt++) {

        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < ChosenFanType.size[cnt]; tnc++) {       
                
                glVertex3f(ChosenFanType.vtx[ChosenFanType.vertexno[entry]].x,
                           ChosenFanType.vtx[ChosenFanType.vertexno[entry]].y,
                           ChosenFanType.vtx[ChosenFanType.vertexno[entry]].z + 10);

                entry++;

            }

        glEnd();

    } 

}

/*
 *  Name:
 *	    editdrawSingleFan
 *  Description:
 *	    Draws a single fan with given number from given mesh
 * Input:
 *      mesh *: Pointer on mesh info
 *      fan_no: Number of fan to draw
 *      col_no: Override for setting by texture-flag
 */
static void editdrawSingleFan(MESH_T *mesh, int fan_no, int col_no)
{

    COMMAND_T *mc;
    int *vert_x, *vert_y, *vert_z;
    int cnt, tnc, entry, *vertexno;
    int vert_base;
    char type;
    int actvertex;
    float *uv;			/* Pointer on Texture - coordinates	        */
    float *offuv;		/* Pointer on additional offset for start   */
                        /* of image in texture			            */
    unsigned char color[3];


    type = (char)(mesh -> fan[fan_no].type & 0x1F);  /* Maximum 31 fan types */
                                                     /* Others are flags     */

    mc   = &MeshCommand[type];

    vert_base = mesh -> vrtstart[fan_no];

    vert_x = &mesh -> vrtx[vert_base];
    vert_y = &mesh -> vrty[vert_base];
    vert_z = &mesh -> vrtz[vert_base];

    if (! (mesh -> draw_mode & EDIT_MODE_SOLID)) {
        /* Set color depending on texturing type */
        if (type & COMMAND_TEXTUREHI_FLAG) {    /* It's one with hi res texture */
            sdlglSetColor(SDLGL_COL_LIGHTBLUE);  /* color like cartman           */
    	}
    	else {
            sdlglSetColor(SDLGL_COL_LIGHTRED);
        }
        if (col_no > 0) {
            sdlglSetColor(col_no);
        }

    }
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED ) {

        if (type & COMMAND_TEXTUREHI_FLAG) {	/* It's one with hi res texture */
            uv = &mc -> biguv[0];
    	}
    	else {
            uv = &mc -> uv[0];
        }

    }

    offuv = &MeshTileOffUV[(mesh -> fan[fan_no].tx_no & 0x3F) * 2];

    /* Now bind the texture */
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {

        glBindTexture(GL_TEXTURE_2D, WallTex[((mesh -> fan[fan_no].tx_no >> 6) & 3)]);

    }

    entry    = 0;
    vertexno = mc -> vertexno;

    for (cnt = 0; cnt < mc -> count; cnt++) {

        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < mc -> size[cnt]; tnc++) {

                actvertex = vertexno[entry]; 	/* Number of vertex to draw */

                if (mesh -> draw_mode & EDIT_MODE_SOLID) {
                    /* Show ambient lighting */
                    color[0] = color[1] = color[2] = mesh -> vrta[actvertex];
                    
                    if (mesh -> draw_mode & EDIT_MODE_LIGHTMAX) {
                        color[0] = color[1] = color[2] = 255;
                    }
                    glColor3ubv(&color[0]);		/* Set the light color */

                }
                if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
                    glTexCoord2f(uv[(actvertex * 2) + 0] + offuv[0], uv[(actvertex * 2) + 1] + offuv[1]);
                }
                
                glVertex3i(vert_x[actvertex], vert_y[actvertex], vert_z[actvertex]);

                entry++;

            }

        glEnd();

    } 

}

/*
 *  Name:
 *	    editdrawMap
 *  Description:
 *	    Draws a single fan with given number from given mesh
 * Input:
 *      mesh *:          Pointer on mesh to draw
 *      chosen_fan:      Sign this fan as chosen in 3D-Map
 *      chosen_fan_type: Draw this fan-type at 'chosen_fan'
 */
static void editdrawMap(MESH_T *mesh, int chosen_fan, int chosen_fan_type)
{

    char save_mode;
    int fan_no;


    if (! mesh -> map_loaded) {

        return;

    }

    glPolygonMode(GL_FRONT, mesh -> draw_mode & EDIT_MODE_SOLID ? GL_FILL : GL_LINE);
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
        glEnable(GL_TEXTURE_2D);
    }
    /* TODO: Draw the map, using different edit flags           */
    /* Needs list of visible tiles
       ( which must be built every time after the camera moved) */
    for (fan_no = 0; fan_no < mesh -> numfan;  fan_no++) {

        editdrawSingleFan(mesh, fan_no, 0);       

    }
    
    /* Sign fan, if chosen */
    if (chosen_fan >= 0) {
    
        save_mode = mesh -> draw_mode;
        mesh -> draw_mode = 0;      /* EDIT_MODE_SOLID; */
        editdrawSingleFan(mesh, chosen_fan, SDLGL_COL_YELLOW);
        mesh -> draw_mode = save_mode;
    
    }

    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
        glDisable(GL_TEXTURE_2D);
    }

    /* Draw the chosen fan type, if any */
    if (chosen_fan_type >= 0) {

        editdrawChosenFanType();

    }

}

/*
 *  Name:
 *	    editdraw2DCameraPos
 *  Description:
 *	    Draw the Camera-Frustum in size od 2D-Map
 * Input:
 *      x, y: Positon in 2D
 */
static void editdraw2DCameraPos(int x, int y)
{

    SDLGL3D_OBJECT *campos;
    float nx[4], ny[4];         /* For the frustum normals              */
    float zmax;                 /* Far plane: Extent of frustum normals */
    SDLGL_RECT draw_rect;
    
    
    /* TODO: Calculate now hardcoded value of '32.0' */
    campos = sdlgl3dGetCameraInfo(0, &nx[0], &ny[0], &zmax);

    draw_rect.x = x + (campos -> pos[0] / 32.0);
    draw_rect.y = y + (campos -> pos[1] / 32.0);
    zmax /= 32.0;

    /* Now draw the camera angle */
    glBegin(GL_LINES);
        sdlglSetColor(SDLGL_COL_LIGHTGREEN);
        glVertex2i(draw_rect.x, draw_rect.y);       /* Right edge of frustum */
        glVertex2i(draw_rect.x + (nx[0] * zmax),
                   draw_rect.y + (ny[0] * zmax));

        sdlglSetColor(SDLGL_COL_LIGHTRED);          /* Left edge of frustum */
        glVertex2i(draw_rect.x, draw_rect.y);
        glVertex2i(draw_rect.x + (nx[1] * zmax),
                   draw_rect.y + (ny[1] * zmax));
        sdlglSetColor(SDLGL_COL_WHITE);
        glVertex2i(draw_rect.x, draw_rect.y);       /* Direction */
        glVertex2i(draw_rect.x + (nx[2] * zmax),
                   draw_rect.y + (ny[2] * zmax));
    glEnd();

}

/* ========================================================================== */
/* ============================= THE PUBLIC ROUTINES ======================== */
/* ========================================================================== */

/*
 * Name:
 *     editdrawInitData
 * Description:
 *     Initializes the command data needed for drawing
 * Input:
 *     None
 * Output:
 *     Pointer on command list
 */
COMMAND_T *editdrawInitData(void)
{

    char texturename[128];
    COMMAND_T *mcmd;
    int entry, cnt;


    mcmd = &MeshCommand[3];

    /* Correct all of them silly texture positions for seamless tiling */

	for (entry = 2; entry < (MAXMESHTYPE/2); entry++) {
 	
        for (cnt = 0; cnt < (mcmd -> numvertices * 2); cnt += 2) {

         	mcmd -> uv[cnt]     = mcmd -> uv[cnt] / 8;
         	mcmd -> uv[cnt + 1] = mcmd -> uv[cnt + 1] / 8;
         	/* Do for big tiles too */
         	mcmd -> biguv[cnt]     = mcmd -> biguv[cnt] / 4;
         	mcmd -> biguv[cnt + 1] = mcmd -> biguv[cnt + 1] / 4;
            /* FIXME: Adjust the texture offsets by +/- 0.001 */
     	
     	}
     	
     	mcmd++;
 	
 	}     

    // Make tile texture offsets
    for (entry = 0; entry < (EDITDRAW_MAXWALLSUBTEX * 2); entry += 2) {

        // Make tile texture offsets
        MeshTileOffUV[entry]     = (((entry / 2) & 7) * 0.125) + 0.001;
        MeshTileOffUV[entry + 1] = (((entry / 2) / 8) * 0.125) + 0.001;

    }

    /* Load the basic textures */
    for (cnt = 0; cnt < 4; cnt++) {
    
        sprintf(texturename, "module/tile%d.bmp", cnt);

        WallTex[cnt] = sdlgltexLoadSingle(texturename);
        
    }

    return MeshCommand;

}

/*
 * Name:
 *     editdrawFreeData
 * Description:
 *     Freses all data loaded by initialization code
 * Input:
 *     None
 */
void editdrawFreeData(void)
{
    
    /* Free the basic textures */
    glDeleteTextures(4, &WallTex[0]);

}

/*
 *  Name:
 *	    editdrawChooseFanType
 *  Description:
 *      Sets up the drawing-data for the chosen fan-type at given position
 * Input:
 *      fan_type: Type of fan to draw
 *      x, y:     Position in mesh
 */
void editdrawChooseFanType(int fan_type, int x, int y)
{
    
    char i;
    
    
    fan_type &= 0x1F;            /* Be on the save side with MAXTYPES */
       
    memcpy(&ChosenFanType, &MeshCommand[fan_type], sizeof(COMMAND_T));
    /* Now move it to the chosen position */
    x *= 128;
    y *= 128;
    
    for (i = 0; i < ChosenFanType.numvertices; i++) { 
        ChosenFanType.vtx[i].x += x;
        ChosenFanType.vtx[i].y += y;
    }

}

/*
 * Name:
 *     editdraw3DView
 * Description:
 *     Draws the whole 3D-View  
 * Input:
 *      mesh *:          Pointer on mesh to draw
 *      chosen_fan:      >= 0 Sign this fan as chosen in 3D-Map
 *      chosen_fan_type: >= 0 Display it over the position of chosen fan     
 */
void editdraw3DView(MESH_T *mesh, int chosen_fan, int chosen_fan_type)
{

    int w, h;
    int x, y, x2, y2;


    sdlgl3dBegin(0, 0);        /* Draw not solid */

    /* Draw a grid 64 x 64 squares for test of the camera view */
    /* Draw only if no map is loaded */
    if (! mesh -> map_loaded) {
    
        sdlglSetColor(SDLGL_COL_LIGHTGREEN);

        for (h = 0; h < 64; h++) {

            for (w = 0; w < 64; w++) {

                x = w * 128;
                y = h * 128;
                x2 = x + 128;
                y2 = y + 128;

                
                glBegin(GL_TRIANGLE_FAN);  /* Draw clockwise */
                    glVertex2i(x, y);
                    glVertex2i(x2, y);
                    glVertex2i(x2, y2);
                    glVertex2i(x, y2);
                glEnd();
                
            }

        }

        /* Draw Axes */
        glBegin(GL_LINES);
            sdlglSetColor(SDLGL_COL_RED);   /* X-Axis   */
            glVertex3i(  0,   0, 10);
            glVertex3i(256,   0, 10);
            sdlglSetColor(SDLGL_COL_GREEN);
            glVertex3i(  0,   0, 10);
            glVertex3i(  0, 256, 10);       /* Y-Axis   */
            sdlglSetColor(SDLGL_COL_BLUE);
            glVertex3i(  0, 0, 10);
            glVertex3i(  0, 0, 266);        /* Z-Axis   */
        glEnd();
        
    }
    
    editdrawMap(mesh, chosen_fan, chosen_fan_type);     
    
    sdlgl3dEnd();

}

/*
 * Name:
 *     editdraw2DMap
 * Description:
 *     Draws the map as 2D-View into given rectangle
 * Input:
 *     mesh *:     Pointer on mesh to draw
 *     x, y, w, h: Draw into this rectangle
 *     chosen_fan: This fan is chosen by user
 */
void editdraw2DMap(MESH_T *mesh, int x, int y, int w, int h, int chosen_fan)
{

    SDLGL_RECT draw_rect;
    int rx2, ry2;
    int fan_no;


    if (! mesh -> map_loaded) {

        return;         /* Draw only if a mpa is loaded */

    }

    draw_rect.x = x;
    draw_rect.y = y;
    draw_rect.w = w / mesh -> tiles_x;   /* With generates size of square */
    draw_rect.h = draw_rect.w;

    fan_no = 0;
    glBegin(GL_QUADS);
        for (h = 0; h < mesh -> tiles_y; h++) {

            for (w = 0; w < mesh -> tiles_x; w++) {
                /* Set color depeding on WALL-FX-Flags    */
                if (mesh -> fan[fan_no].fx & MPDFX_WALL) {
                    sdlglSetColor(SDLGL_COL_BLACK);
                }
                else if (0 == mesh -> fan[fan_no].fx) {
                    sdlglSetColor(SDLGL_COL_BLUE);
                }
                else {
                    /* Colors >= 15: Others then standard colors */
                    sdlglSetColor(mesh -> fan[fan_no].type + 15);
                }
                rx2 = draw_rect.x + draw_rect.w;
                ry2 = draw_rect.y + draw_rect.h;

                glVertex2i(draw_rect.x,  ry2);
                glVertex2i(rx2, ry2);
                glVertex2i(rx2, draw_rect.y);
            	glVertex2i(draw_rect.x,  draw_rect.y);

                draw_rect.x += draw_rect.w;
                fan_no++;

            }

            /* Next line    */
            draw_rect.x = x;
            draw_rect.y += draw_rect.h;
        
        }

    glEnd();

    /* Draw chosen tile                   */
    if (chosen_fan >= 0) {

        draw_rect.x = x + ((chosen_fan % mesh -> tiles_x) * draw_rect.w);
        draw_rect.y = y + ((chosen_fan / mesh -> tiles_x) * draw_rect.h);

        rx2 = draw_rect.x + draw_rect.w;
        ry2 = draw_rect.y + draw_rect.h;

        sdlglSetColor(SDLGL_COL_YELLOW);
        glBegin(GL_LINE_LOOP);
            glVertex2i(draw_rect.x,  ry2);
            glVertex2i(rx2, ry2);
            glVertex2i(rx2, draw_rect.y);
            glVertex2i(draw_rect.x,  draw_rect.y);
        glEnd();

    }

    editdraw2DCameraPos(x, y);

}

/*
 * Name:
 *     editdraw2DTex
 * Description:
 *     Draws the texture into chosen rectangle and draws a rectangle
 *     around the texture part used.
 * Input:
 *     x, y, w, h: Draw into this rectangle
 *     tx_no:      This texture
 *     tx_big:     Is a big texture
 */
void editdraw2DTex(int x, int y, int w, int h, char tx_no, char tx_big)
{

    int x2, y2;
    int part_size, sub_x, sub_y;
    int main_tx_no, subtex_no;  
    
    
    main_tx_no = (tx_no >> 6) & 3;    
    subtex_no  = tx_no & 0x3F;

    x2 = x + w;
    y2 = y + h;
    /* Draw the texture, counter-clockwise */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, WallTex[main_tx_no]);
    sdlglSetColor(SDLGL_COL_WHITE);
    glBegin(GL_TRIANGLE_FAN);
        glTexCoord2fv(&DefaultUV[0]);
        glVertex2i(x, y);
        glTexCoord2fv(&DefaultUV[6]);
        glVertex2i(x, y2);
        glTexCoord2fv(&DefaultUV[4]);
        glVertex2i(x2, y2);
        glTexCoord2fv(&DefaultUV[2]);
        glVertex2i(x2, y);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    /* ---- Draw a rectangle around the chosen texture-part */
    if (tx_big) {
        part_size = w / 4;
        sub_x = ((subtex_no & 0xF) % 4);
        sub_y = ((subtex_no & 0xF) / 4);
    }
    else {
        part_size = w / 8;
        sub_x = (subtex_no % 8);
        sub_y = (subtex_no / 8);
    }
    
    x += sub_x * part_size;
    y += sub_y * part_size;
    x2 = x + part_size;
    y2 = y + part_size;
    glBegin(GL_LINE_LOOP);
    glBegin(GL_QUADS);
        glVertex2i(x, y);
        glVertex2i(x, y2);
        glVertex2i(x2, y2);
        glVertex2i(x2, y);            
    glEnd();
    
}