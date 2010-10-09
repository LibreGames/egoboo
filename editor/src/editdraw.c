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
#include "sdlglstr.h"


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
    {  "0: Ground Tile",
        0,          /* Default FX: None         */
        1,          /* Default texture          */
        4,		    /* Total number of vertices */
    	1,    		/*  1 Command               */
    	{ 4 },		/* Commandsize (how much vertices per command)  */
    	{ 1, 2, 3, 0 },
    	{ 0.00, 0.00, 1.00, 0.00, 1.00, 1.00, 0.00, 1.00 },
        { 0.00, 0.00, 1.00, 0.00, 1.00, 1.00, 0.00, 1.00 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 }
        }
    },
    {   "1: Top Tile",             /* Top-Tile for 'simple' edit mode  */
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX:                      */
        63,                             /* Default texture                  */
    	4,
        1,
        { 4 },
        { 0, 1, 2, 3 },
        { 0.00, 0.00, 1.00, 0.00, 1.00, 1.00, 0.001, 1.00 },
        { 0.00, 0.00, 1.00, 0.00, 1.00, 1.00, 0.001, 1.00 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 192.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 192.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 192.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 192.00 }
        }

    },
    {   "2: Four Faced Ground",
        0,          /* Default FX: None         */
        0,          /* Default texture          */
        5,
        1,
        { 6 },
        { 4, 3, 0, 1, 2, 3 },
        { 0.00, 0.00, 1.00, 0.00, 1.00, 1.00, 0.00, 1.00, 0.50, 0.50 },
        { 0.00, 0.00, 1.00, 0.00, 1.00, 1.00, 0.00, 1.00, 0.50, 0.50 },
        { {  0, 0.00, 0.00,   0.00,   0.00, 0.00 },
          {  3, 1.00, 0.00, 128.00,   0.00, 0.00 },
          { 15, 1.00, 1.00, 128.00, 128.00, 0.00 },
          { 12, 0.00, 1.00,   0.00, 128.00, 0.00 },
          {  5, 0.50, 0.50,  64.00,  64.00, 0.00 }         
        }
    },
    {   "3: Eight Faced Ground",
        0,          /* Default FX: None         */
        0,          /* Default texture          */
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
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX   */
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
    {   "8: Wall Straight (WE)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX       */
        63 + 33,                        /* Default texture  */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
    {   "16: Wall Outer Edge (WS)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX       */
        63 + 51,                        /* Default texture  */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
    {   "19: Wall Inner Edge (ES)",
        (MPDFX_WALL | MPDFX_IMPASS),    /* Default FX       */
        63 +  1,                        /* Default texture  */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
        0,                              /* Default texture          */
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
        MPDFX_IMPASS,           /* Default FX       */
        0,                      /* Default texture  */
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
        MPDFX_IMPASS,           /* Default FX       */
        0,                      /* Default texture  */
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
 *     ft *: Fantype holding possible texture number of this fan
 *     fd *: Data of fan to draw 
 */
static void editdrawChosenFanType(FANDATA_T *ft, COMMAND_T *fd)
{
    
    int cnt, tnc;
    int entry;
    
    
    /* FIXME: Save and restore state */
    glPolygonMode(GL_FRONT, GL_LINE);
    sdlglSetColor(SDLGL_COL_WHITE);

    if (ft -> tx_no > 0) {
        /* Empty code to keep compiler quiet: 2010-08-18 */
    }
    
    entry = 0;
    
    for (cnt = 0; cnt < fd -> count; cnt++) {

        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < fd -> size[cnt]; tnc++) {
                
                glVertex3f(fd -> vtx[fd -> vertexno[entry]].x,
                           fd -> vtx[fd -> vertexno[entry]].y,
                           fd -> vtx[fd -> vertexno[entry]].z + 10);

                entry++;

            }

        glEnd();

    } 

}

/*
 *  Name:
 *	    editdrawTransparentFan2D
 *  Description:
 *	    Draws a list of fans with given number on 3D-Mesh
 *      Transparent, with no texture 
 * Input:
 *      mesh *:     Pointer on mesh info
 *      tilesize *: Rectangle size of tile on minimap an position of minimap
 *      fan_no*:    List of fan numbers to draw
 *      col_no:     Override for setting by texture-flag
 *      trans:      Transparency of fan drawn
 */
static void editdrawTransparentFan2D(MESH_T *mesh, SDLGL_RECT *tilesize, int *fan_no, int col_no, unsigned char trans)
{

    int rx, ry, rx2, ry2;
    unsigned char color[4];    
    
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);     
    glPolygonMode(GL_FRONT, GL_FILL);
        
    sdlglGetColor(col_no, color);
        
    color[3] = trans;
    glColor4ubv(color); 
   
    while(*fan_no >= 0) {

        rx = tilesize -> x + ((*fan_no % mesh -> tiles_x) * tilesize -> w);
        ry = tilesize -> y + ((*fan_no / mesh -> tiles_x) * tilesize -> h);

        rx2 = rx + tilesize -> w;
        ry2 = ry + tilesize -> h;

        glBegin(GL_TRIANGLE_FAN);
            glVertex2i(rx,  ry2);
            glVertex2i(rx2, ry2);
            glVertex2i(rx2, ry);
            glVertex2i(rx,  ry);
        glEnd();

        fan_no++;
    
    }
    
    glDisable(GL_BLEND);

}

/*
 *  Name:
 *	    editdrawTransparentFan3D
 *  Description:
 *	    Draws a list of fans with given number on 3D-Mesh
 *      Transparent, with no texture 
 * Input:
 *      mesh *:  Pointer on mesh info
 *      fan_no*: List of fan numbers to draw
 *      col_no:  Override for setting by texture-flag
 *      trans:   Transparency of fan drawn
 */
static void editdrawTransparentFan3D(MESH_T *mesh, int *fan_no, int col_no, unsigned char trans)
{

    COMMAND_T *mc;
    float *vert_x, *vert_y, *vert_z;
    int cnt, tnc, entry, *vertexno;
    int vert_base;
    char type;
    int actvertex;
    unsigned char color[4];

    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);     
    glPolygonMode(GL_FRONT, GL_FILL);
    
    while(*fan_no >= 0) {
    
        type = (char)(mesh -> fan[*fan_no].type & 0x1F);  /* Maximum 31 fan types */
                                                         /* Others are flags     */

        mc   = &MeshCommand[type];

        vert_base = mesh -> vrtstart[*fan_no];

        vert_x = &mesh -> vrtx[vert_base];
        vert_y = &mesh -> vrty[vert_base];
        vert_z = &mesh -> vrtz[vert_base];

        sdlglGetColor(col_no, color);
        
        color[3] = trans;
        glColor4ubv(color);      
    
        entry    = 0;
        vertexno = mc -> vertexno;

        for (cnt = 0; cnt < mc -> count; cnt++) {

            glBegin (GL_TRIANGLE_FAN);

                for (tnc = 0; tnc < mc -> size[cnt]; tnc++) {

                    actvertex = vertexno[entry]; 	/* Number of vertex to draw */               
                    
                    glVertex3f(vert_x[actvertex], vert_y[actvertex], vert_z[actvertex] + 2);

                    entry++;

                }

            glEnd();

        } 
        
        fan_no++;
        
    }
    
    glDisable(GL_BLEND);

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
    float *vert_x, *vert_y, *vert_z;
    int cnt, tnc, entry, *vertexno;
    int vert_base;
    char type, bigtex;
    int actvertex;
    float *uv;			/* Pointer on Texture - coordinates	        */
    float *offuv;		/* Pointer on additional offset for start   */
                        /* of image in texture			            */
    unsigned char color[3];


    type   = (char)mesh -> fan[fan_no].type;
    bigtex = (char)(type & 0x20);
    type   &= 0x1F;     /* Maximum 31 fan types for drawing         */

    
    mc   = &MeshCommand[type];

    vert_base = mesh -> vrtstart[fan_no];

    vert_x = &mesh -> vrtx[vert_base];
    vert_y = &mesh -> vrty[vert_base];
    vert_z = &mesh -> vrtz[vert_base];

    if (! (mesh -> draw_mode & EDIT_MODE_SOLID)) {
        /* Set color depending on texturing type */
        if (bigtex) {                           /* It's one with hi res texture */
            sdlglSetColor(SDLGL_COL_LIGHTBLUE); /* color like cartman           */
    	}
    	else {
            sdlglSetColor(SDLGL_COL_LIGHTRED);
        }
        if (col_no > 0) {
            sdlglSetColor(col_no);
        }

    }
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED ) {

        if (bigtex) {               /* It's one with hi res texture */
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
                
                glVertex3f(vert_x[actvertex], vert_y[actvertex], vert_z[actvertex]);

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
 *     mesh *:   Pointer on mesh to draw
 *     ft *:     Pointer on chosen fan type
 *     fd *:     ft -> type >= 0 Display it over the position of chosen fan  
 *     chosen *: This fan(s) is chosen by user
 *     psg *:    Passage fan(s) chosen
 *     spawn *:  Spawn fan(s) chosen 
 */
static void editdrawMap(MESH_T *mesh, FANDATA_T *ft, COMMAND_T *fd, int *chosen, int *psg, int *spawn)
{

    int fan_no;


    if (! mesh -> map_loaded) {

        return;

    }

    glPolygonMode(GL_FRONT, mesh -> draw_mode & EDIT_MODE_SOLID ? GL_FILL : GL_LINE);
    glFrontFace(GL_CW); 
    
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
        glEnable(GL_TEXTURE_2D);
    }
    
    /* Draw the map, using different edit flags           */
    /* Needs list of visible tiles
       ( which must be built every time after the camera moved) */
    for (fan_no = 0; fan_no < mesh -> numfan;  fan_no++) {

        editdrawSingleFan(mesh, fan_no, 0);       

    }
    
    /* ------------ Sign fan(s), if chosen ----------------------  */
    if (*chosen >= 0) {  

        editdrawTransparentFan3D(mesh, chosen, SDLGL_COL_YELLOW, 128);
    
    }
    /* ------------ Draw actual chosen passage ---------------------- */
    if (*psg >= 0) {  

        editdrawTransparentFan3D(mesh, psg, SDLGL_COL_GREEN, 128);
    
    }
    /* ------------ Draw actual chosen spawn position --------------- */
    if (*spawn >= 0) {  

        editdrawTransparentFan3D(mesh, spawn, SDLGL_COL_RED, 128);
    
    }      
    
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
        glDisable(GL_TEXTURE_2D);
    }
    
    /* Draw the chosen fan type, if any */
    if (fd -> numvertices > 0) {

        editdrawChosenFanType(ft, fd);

    } 
    
}

/*
 *  Name:
 *	    editdraw2DCameraPos
 *  Description:
 *	    Draw the Camera-Frustum in size od 2D-Map
 * Input:
 *      x, y:   Positon in 2D
 *      tw, th: Downsize-Factor of map to minimap
 */
static void editdraw2DCameraPos(int x, int y, int tw, int th)
{

    SDLGL3D_OBJECT *campos;
    SDLGL3D_FRUSTUM f;          /* Information about the frustum        */
    SDLGL_RECT draw_rect;
    int zmax;


    /* TODO: Calculate now hardcoded value of '32.0' */
    campos = sdlgl3dGetCameraInfo(0, &f);

    draw_rect.x = x + (campos -> pos[0] / tw);
    draw_rect.y = y + (campos -> pos[1] / th);
    zmax = f.zmax / tw;       /* Fixme: Do correct division -- resive by diff to full*/

    /* Now draw the camera angle */
    glBegin(GL_LINES);
        sdlglSetColor(SDLGL_COL_LIGHTGREEN);
        glVertex2i(draw_rect.x, draw_rect.y);       /* Right edge of frustum */
        glVertex2i(draw_rect.x + (f.nx[0] * zmax),
                   draw_rect.y + (f.ny[0] * zmax));

        sdlglSetColor(SDLGL_COL_LIGHTRED);          /* Left edge of frustum */
        glVertex2i(draw_rect.x, draw_rect.y);
        glVertex2i(draw_rect.x + (f.nx[1] * zmax),
                   draw_rect.y + (f.ny[1] * zmax));
        sdlglSetColor(SDLGL_COL_WHITE);
        glVertex2i(draw_rect.x, draw_rect.y);       /* Direction            */
        glVertex2i(draw_rect.x + (f.nx[2] * zmax),
                   draw_rect.y + (f.ny[2] * zmax));
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


    mcmd = MeshCommand;

    /* Correct all of them silly texture positions for seamless tiling */
	for (entry = 0; entry < (MAXMESHTYPE/2); entry++) {
 	
        for (cnt = 0; cnt < (mcmd -> numvertices * 2); cnt++) {

            mcmd -> uv[cnt]    *= 0.1211;   /* 31 / 256 */
            mcmd -> biguv[cnt] *= 0.2461;   /* 63 / 256 */
                 	
     	}
     	
     	mcmd++;
 	
 	}     

    // Make tile texture start offsets
     for (entry = 0; entry < (EDITDRAW_MAXWALLSUBTEX * 2); entry += 2) {

        MeshTileOffUV[entry]     = (((entry / 2) & 7) * 0.125);    
        MeshTileOffUV[entry + 1] = (((entry / 2) / 8) * 0.125);   

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
 * Name:
 *     editdraw3DView
 * Description:
 *     Draws the whole 3D-View  
 * Input:
 *     mesh *:   Pointer on mesh to draw
 *     ft *:     Pointer on chosen fan type
 *     fd *:     ft -> type >= 0 Display it over the position of chosen fan     
 *     chosen *: This fan(s) is chosen by user
 *     psg *:    Passage fan(s) chosen
 *     spawn *:  Spawn fan(s) chosen 
 */
void editdraw3DView(MESH_T *mesh, FANDATA_T *ft, COMMAND_T *fd, int *chosen, int *psg, int *spawn)
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
        
    }
    
    editdrawMap(mesh, ft, fd, chosen, psg, spawn);     

    sdlgl3dEnd();

}

/*
 * Name:
 *     editdraw2DMap
 * Description:
 *     Draws the map as 2D-View into given rectangle
 * Input:
 *     mesh *:   Pointer on mesh to draw
 *     x, y:     Draw at this position
 *     chosen *: This fan(s) is chosen by user
 *     psg *:    Passage fan(s) chosen
 *     spawn *:  Spawn fan(s) chosen  
 */
void editdraw2DMap(MESH_T *mesh, int x, int y, int *chosen, int *psg, int *spawn)
{

    SDLGL_RECT draw_rect, draw_rect2;
    int fan_no;
    int rx2, ry2;
    int w, h;


    if (! mesh -> map_loaded) {

        return;         /* Draw only if a mpa is loaded */

    }

    draw_rect.x = x;
    draw_rect.y = y;
    draw_rect.w = mesh -> minimap_w / mesh -> tiles_x;   /* With generates size of square */
    draw_rect.h = mesh -> minimap_h / mesh -> tiles_y;

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

    draw_rect.x = x;
    draw_rect.y = y;

    /* ---------------------- Draw chosen tile(s) ---------------------- */
    if (*chosen >= 0) {
        editdrawTransparentFan2D(mesh, &draw_rect, chosen, SDLGL_COL_YELLOW, 128);
    }
    /* ------------ Draw chosen passage ---------- */
    if (*psg >= 0) {
        editdrawTransparentFan2D(mesh, &draw_rect, psg, SDLGL_COL_BLUE, 128);
    }
    /* ------------ Draw chosen spawn position --------------- */
    if (*spawn >= 0) {
        editdrawTransparentFan2D(mesh, &draw_rect, spawn, SDLGL_COL_BLUE, 128);
    }

    /* ------------ Draw grid for easier editing ------- */
    draw_rect.x = x;
    draw_rect.y = y;

    for (h = 0; h < mesh -> tiles_y; h++) {

        for (w = 0; w < mesh -> tiles_x; w++) {
            sdlglstrDrawRectColNo(&draw_rect, SDLGL_COL_GREEN, 0);
            draw_rect.x += draw_rect.w;
        }

        /* Next line    */
        draw_rect.x = x;
        draw_rect.y += draw_rect.h;

    }
    /* ---------- Draw border for 2D-Map ---------- */
    draw_rect2.x = x;
    draw_rect2.y = y;
    draw_rect2.w = mesh -> minimap_w;
    draw_rect2.h = mesh -> minimap_h;
    sdlglstrDrawRectColNo(&draw_rect2, SDLGL_COL_WHITE, 0);
    /* ------------ Draw the camera as last one -------- */
    editdraw2DCameraPos(x, y, 128 / draw_rect.w, 128 / draw_rect.h);

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
void editdraw2DTex(int x, int y, int w, int h, unsigned char tx_no, char tx_big)
{

    int x2, y2;
    int spart_size, part_size;
    unsigned char main_tx_no, subtex_no;
    
    
    main_tx_no = (unsigned char)((tx_no >> 6) & 3);
    subtex_no  = (unsigned char)(tx_no & 0x3F);

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
    spart_size = w / 8;
    part_size = w / 8;
    if (tx_big) {
        part_size *= 2;
    }

    x += (subtex_no % 8) * spart_size;
    y += (subtex_no / 8) * spart_size;
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

/*
 * Name:
 *     editdrawAdjustCamera
 * Description:
 *     Adjusts camera to view at given tile at tx/ty
 * Input:
 *     tx, ty:    Where to look
 */
void editdrawAdjustCamera(int tx, int ty)
{

    sdlgl3dMoveToPosCamera(0, tx * 128, ty * 128, 0, 1);
    
}

