/*******************************************************************************
*  EGOMAP.C                                                                    *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Managing the map and the objects on it                                  *
*      (c)2011 Paul Mueller <pmtech@swissonline.ch>                            *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *
*******************************************************************************/


/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include <memory.h>
#include <math.h>
#include <stdio.h>          /* sprintf()    */


#include "sdlgl3d.h"
#include "editfile.h"       /* Loading the different files                  */
#include "editdraw.h"       /* Draw the whole map                           */
#include "egodefs.h"        /* Global definitions with no additional data   */
#include "char.h"           /* Load a character from file                   */
/* --- Own header --- */
#include "egomap.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EGOMAP_MAXPASSAGE   50

/* -- Maximum-Values for files -- */
#define EGOMAP_MAXOBJ     1024      /* Maximum of objects visible ob map    */
#define EGOMAP_MAXSPAWN    500      /* Maximum Lines in spawn list          */
#define EGOMAP_MAXPASSAGE   50

#define EGOMAP_TILEDIV        128   /* Size of tile     */

/* === Lights === */
#define EGOMAP_MAXLIGHT    100        /* Maximum number of lights     */  
#define EGOMAP_FADEBORDER   64        /* Darkness at the edge of map  */

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

typedef struct {

    int           x;
    int           y;
    unsigned char level;
    int           radius;

} LIGHT_T;

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

/* ----------- Lights ---------- */
static int LightAmbi    = 22;
static int LightAmbiCut = 1;
static int LightDirect  = 16;
static int NumLight     = 0;
static LIGHT_T LightList[EGOMAP_MAXLIGHT + 2];

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
        { 0.00f, 0.00f, 1.00f, 0.00f, 1.00f, 1.00f, 0.001f, 1.00f },
        { 0.00f, 0.00f, 1.00f, 0.00f, 1.00f, 1.00f, 0.001f, 1.00f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 192.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 192.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 192.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 192.00f }
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
          0.33f, 0.33f,  0.66f, 0.33f,  0.66f, 0.66f,  0.33f, 0.66f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.33f,  0.66f, 0.33f,  0.66f, 0.66f,  0.33f, 0.66f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  5, 0.33f, 0.33f,  42.00f,  42.00f, 0.00f },  
          {  6, 0.66f, 0.33f,  84.00f,  42.00f, 0.00f },
          { 10, 0.66f, 0.66f,  84.00f,  84.00f, 0.00f },
          {  9, 0.33f, 0.66f,  42.00f,  84.00f, 0.00f }
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  1.00f, 0.33f,  1.00f, 0.66f,
          0.66f, 1.00f,  0.33f, 1.00f,  0.00f, 0.66f,  0.00f, 0.33f,
          0.33f, 0.33f,  0.66f, 0.33f,  0.66f, 0.66f,  0.33f, 0.66f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  1.00f, 0.33f,  1.00f, 0.66f,
          0.66f, 1.00f,  0.33f, 1.00f,  0.00f, 0.66f,  0.00f, 0.33f,
          0.33f, 0.33f,  0.66f, 0.33f,  0.66f, 0.66f,  0.33f, 0.66f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  1, 0.33f, 0.00f,  42.00f,   0.00f, 0.00f },
          {  2, 0.66f, 0.00f,  84.00f,   0.00f, 0.00f },
          {  7, 1.00f, 0.33f, 128.00f,  42.00f, 0.00f },
          { 11, 1.00f, 0.66f, 128.00f,  84.00f, 0.00f },
          { 14, 0.66f, 1.00f,  84.00f, 128.00f, 0.00f },
          { 13, 0.33f, 1.00f,  42.00f, 128.00f, 0.00f },
          {  8, 0.00f, 0.66f,   0.00f,  84.00f, 0.00f },
          {  4, 0.00f, 0.33f,   0.00f,  42.00f, 0.00f },
          {  5, 0.33f, 0.33f,  42.00f,  42.00f, 0.00f },
          {  6, 0.66f, 0.33f,  84.00f,  42.00f, 0.00f },
          { 10, 0.66f, 0.66f,  84.00f,  84.00f, 0.00f },
          {  9, 0.33f, 0.66f,  42.00f,  84.00f, 0.00f }
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          1.00f, 0.33f,  1.00f, 0.66f,  0.00f, 0.66f,  0.00f, 0.33f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          1.00f, 0.33f,  1.00f, 0.66f,  0.00f, 0.66f,  0.00f, 0.33f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f,   0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f,   0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 192.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 192.00f },
          {  7, 1.00f, 0.33f, 128.00f,  42.00f, 128.00f },
          { 11, 1.00f, 0.66f, 128.00f,  84.00f, 192.00f },
          {  8, 0.00f, 0.66f,   0.00f,  84.00f, 192.00f },
          {  4, 0.00f, 0.33f,   0.00f,  42.00f, 128.00f },
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  0.66f, 1.00f,  0.33f, 1.00f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  0.66f, 1.00f,  0.33f, 1.00f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  1, 0.33f, 0.00f,  42.00f,   0.00f, 0.00f },
          {  2, 0.66f, 0.00f,  84.00f,   0.00f, 0.00f },
          { 14, 0.66f, 1.00f,  84.00f, 128.00f, 0.00f },
          { 13, 0.33f, 1.00f,  42.00f, 128.00f, 0.00f },
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.00f, 0.66f,  0.00f, 0.33f,  0.66f, 0.33f,  0.66f, 0.66f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.00f, 0.66f,  0.00f, 0.33f,  0.66f, 0.33f,  0.66f, 0.66f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  8, 0.00f, 0.66f,   0.00f,  84.00f, 0.00f },
          {  4, 0.00f, 0.33f,   0.00f,  42.00f, 0.00f },
          {  6, 0.66f, 0.33f,  84.00f,  42.00f, 0.00f },
          { 10, 0.66f, 0.66f,  84.00f,  84.00f, 0.00f }
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  0.66f, 0.66f,  0.33f, 0.66f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  0.66f, 0.66f,  0.33f, 0.66f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  1, 0.33f, 0.00f,  42.00f,   0.00f, 0.00f },
          {  2, 0.66f, 0.00f,  84.00f,   0.00f, 0.00f },
          { 10, 0.66f, 0.66f,  84.00f,  84.00f, 0.00f },
          {  9, 0.33f, 0.66f,  42.00f,  84.00f, 0.00f }
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  0.66f, 1.00f,  0.33f, 1.00f,
          0.00f, 0.66f,  0.00f, 0.33f,  0.33f, 0.33f,  0.33f, 0.66f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  0.66f, 1.00f,  0.33f, 1.00f,
          0.00f, 0.66f,  0.00f, 0.33f,  0.33f, 0.33f,  0.33f, 0.66f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  1, 0.33f, 0.00f,  42.00f,   0.00f, 0.00f }, 
          {  2, 0.66f, 0.00f,  84.00f,   0.00f, 0.00f },
          { 14, 0.66f, 1.00f,  84.00f, 128.00f, 0.00f },
          { 13, 0.33f, 1.00f,  42.00f, 128.00f, 0.00f },
          {  8, 0.00f, 0.66f,   0.00f,  84.00f, 0.00f },
          {  4, 0.00f, 0.33f,   0.00f,  42.00f, 0.00f },
          {  5, 0.33f, 0.33f,  42.00f,  42.00f, 0.00f },
          {  9, 0.33f, 0.66f,  42.00f,  84.00f, 0.00f }
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  1.00f, 0.33f,  1.00f, 0.66f,
          0.00f, 0.66f,  0.00f, 0.33f,  0.33f, 0.33f,  0.66f, 0.33f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  0.66f, 1.00f,  0.33f, 1.00f,
          0.00f, 0.66f,  0.00f, 0.33f,  0.33f, 0.33f,  0.33f, 0.66f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  1, 0.33f, 0.00f,  42.00f,   0.00f, 0.00f },
          {  2, 0.66f, 0.00f,  84.00f,   0.00f, 0.00f },
          {  7, 1.00f, 0.33f, 128.00f,  42.00f, 0.00f },
          { 11, 1.00f, 0.66f, 128.00f,  84.00f, 0.00f },
          {  8, 0.00f, 0.66f,   0.00f,  84.00f, 0.00f },
          {  4, 0.00f, 0.33f,   0.00f,  42.00f, 0.00f },
          {  5, 0.33f, 0.33f,  42.00f,  42.00f, 0.00f },
          {  6, 0.66f, 0.33f,  84.00f,  42.00f, 0.00f }
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  1.00f, 0.33f,  1.00f, 0.66f,
          0.66f, 1.00f,  0.33f, 1.00f,  0.66f, 0.33f,  0.66f, 0.66f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.33f, 0.00f,  0.66f, 0.00f,  1.00f, 0.33f,  1.00f, 0.66f,
          0.66f, 1.00f,  0.33f, 1.00f,  0.66f, 0.33f,  0.66f, 0.66f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  1, 0.33f, 0.00f,  42.00f,   0.00f, 0.00f },
          {  2, 0.66f, 0.00f,  84.00f,   0.00f, 0.00f },
          {  7, 1.00f, 0.33f, 128.00f,  42.00f, 0.00f },
          { 11, 1.00f, 0.66f, 128.00f,  84.00f, 0.00f },
          { 14, 0.66f, 1.00f,  84.00f, 128.00f, 0.00f },
          { 13, 0.33f, 1.00f,  42.00f, 128.00f, 0.00f },
          {  6, 0.66f, 0.33f,  84.00f,  42.00f, 0.00f },
          { 10, 0.66f, 0.66f,  84.00f,  82.00f, 0.00f } 
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.16f, 0.00f,  0.33f, 0.00f,  0.50f, 0.00f,  0.66f, 0.00f,
          0.83f, 0.00f,  0.83f, 1.00f,  0.66f, 1.00f,  0.50f, 1.00f,
          0.33f, 1.00f,  0.16f, 1.00f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          0.16f, 0.00f,  0.33f, 0.00f,  0.50f, 0.00f,  0.66f, 0.00f,
          0.83f, 0.00f,  0.83f, 1.00f,  0.66f, 1.00f,  0.50f, 1.00f,
          0.33f, 1.00f,  0.16f, 1.00f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },
          {  1, 0.16f, 0.00f,  21.00f,   0.00f, 0.00f },
          {  2, 0.33f, 0.00f,  42.00f,   0.00f, 0.00f },
          {  2, 0.50f, 0.00f,  64.00f,   0.00f, 0.00f },
          {  3, 0.66f, 0.00f,  85.00f,   0.00f, 0.00f },
          {  3, 0.83f, 0.00f, 106.00f,   0.00f, 0.00f },
          { 14, 0.83f, 1.00f, 106.00f, 128.00f, 0.00f },
          { 14, 0.66f, 1.00f,  85.00f, 128.00f, 0.00f },
          { 13, 0.50f, 1.00f,  64.00f, 128.00f, 0.00f }, 
          { 13, 0.33f, 1.00f,  42.00f, 128.00f, 0.00f }, 
          { 12, 0.16f, 1.00f,  21.00f, 128.00f, 0.00f }, 
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
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          1.00f, 0.16f,  1.00f, 0.33f,  1.00f, 0.50f,  1.00f, 0.66f,
          1.00f, 0.83f,  0.00f, 0.83f,  0.00f, 0.66f,  0.00f, 0.50f,
          0.00f, 0.33f,  0.00f, 0.16f },
        { 0.00f, 0.00f,  1.00f, 0.00f,  1.00f, 1.00f,  0.00f, 1.00f,
          1.00f, 0.16f,  1.00f, 0.33f,  1.00f, 0.50f,  1.00f, 0.66f,
          1.00f, 0.83f,  0.00f, 0.83f,  0.00f, 0.66f,  0.00f, 0.50f,
          0.00f, 0.33f,  0.00f, 0.16f },
        { {  0, 0.00f, 0.00f,   0.00f,   0.00f, 0.00f },
          {  3, 1.00f, 0.00f, 128.00f,   0.00f, 0.00f },
          { 15, 1.00f, 1.00f, 128.00f, 128.00f, 0.00f },
          { 12, 0.00f, 1.00f,   0.00f, 128.00f, 0.00f },  
          {  3, 1.00f, 0.16f, 128.00f,  21.00f, 0.00f },
          {  7, 1.00f, 0.33f, 128.00f,  42.00f, 0.00f },
          {  7, 1.00f, 0.50f, 128.00f,  64.00f, 0.00f },
          { 11, 1.00f, 0.66f, 128.00f,  85.00f, 0.00f },
          { 11, 1.00f, 0.83f, 128.00f, 106.00f, 0.00f },
          {  8, 0.00f, 0.83f,   0.00f, 106.00f, 0.00f },
          {  8, 0.00f, 0.66f,   0.00f,  85.00f, 0.00f },
          {  4, 0.00f, 0.50f,   0.00f,  64.00f, 0.00f },
          {  4, 0.00f, 0.33f,   0.00f,  41.00f, 0.00f },
          {  0, 0.00f, 0.16f,   0.00f,  21.00f, 0.00f }
        }
    }
};

/* --- Game objects buffer --- */
static EDITFILE_PASSAGE_T Passages[EGOMAP_MAXPASSAGE + 2];
static EDITFILE_SPAWNPT_T SpawnPts[EGOMAP_MAXSPAWN + 2];       /* Holds data about chosen spawn point  */

/* ---- Map data itself --- */
static MESH_T Mesh;                                 /* Mesh of map                  */
static SDLGL3D_OBJECT TileObj;                      /* Tile for collision detection */
static SDLGL3D_OBJECT MapObj[EGOMAP_MAXOBJ + 2];   /* Objects on map               */

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/* ============ OBJECT-FUNCTIONS ========== */

/*
 * Name:
 *     egomapNewObject
 * Description:
 *     Returns the number of an empty object slot, if available
 * Input:
 *     obj_type: Type of object
 *     type_no:  Number in list of 'obj_type'
 *     x, y, z:  Position
 *     dir:      Direction
 * Output:
 *     > 0: Number of valid object slot 
 */
static int egomapNewObject(char obj_type, int type_no, float x, float y, float z, char dir)
{
    int i;
    SDLGL3D_OBJECT *pobj;


    for(i = 1; i < EGOMAP_MAXOBJ; i++)
    {
        if(MapObj[i].obj_type == 0 || MapObj[i].obj_type < 0)
        {
            pobj = &MapObj[i];

            // Initialize it
            pobj -> obj_type = obj_type;
            pobj -> type_no  = type_no;
            // Set position
            pobj -> pos[SDLGL3D_X] = x;
            pobj -> pos[SDLGL3D_Y] = y;
            pobj -> pos[SDLGL3D_Z] = z;

            // Set the rotation around y = Direction of
            pobj -> rot[SDLGL3D_Y] = 90.0 * dir;

            // Initialize the rest
            sdlgl3dInitObject(pobj);

            // Return its number
            return i;
        }
    }

    return 0;
}

/* ============ MAP-FUNCTIONS ============= */

/*
 * Name:
 *     egomapSetVrta
 * Description:
 *     Set the 'vrta'-value for given vertex
 * Input:
 *     mesh *: Pointer on mesh  to handle
 *     vert:   Number of vertex to set
 */
static int egomapSetVrta(MESH_T *mesh, int vert)
{
	/* @todo: Get all needed functions from cartman code */
	int newa, x, y, z;
	int brz, deltaz, cnt;
	int newlevel, distance, disx, disy;
	/* int brx, bry, dist; */ 

	/* To make life easier  */
	x = mesh -> vrtx[vert];
	y = mesh -> vrty[vert];
	z = mesh -> vrtz[vert];

	// Directional light
	/*
	brx = x + 64;
	bry = y + 64;

	brz = get_level(brx, y) +
		  get_level(x, bry) +
		  get_level(x + 46, y + 46);
	*/
	
	brz = z;
	if (z < -128) z = -128;
	if (brz < -128) brz = -128;
	deltaz = z + z + z - brz;
	newa = (deltaz * LightDirect >> 8);

	// Point lights !!!BAD!!!
	newlevel = 0;
	for (cnt = 0; cnt < NumLight; cnt++) {
		disx = x - LightList[cnt].x;
		disy = y - LightList[cnt].y;
		distance = (disx * disx) + (disy * disy);
		if (distance < (LightList[cnt].radius * LightList[cnt].radius))
		{
			newlevel += ((LightList[cnt].level * (LightList[cnt].radius - distance)) / LightList[cnt].radius);
		}
	}
	newa += newlevel;

	// Bounds
	if (newa < -LightAmbiCut) newa = -LightAmbiCut;
	newa += LightAmbi;
	if (newa <= 0) newa = 1;
	if (newa > 255) newa = 255;
	mesh -> vrta[vert] = (unsigned char)newa;

	// Edge fade
	/*
	dist = dist_from_border(mesh  -> vrtx[vert], mesh -> vrty[vert]);
	if (dist <= EGOMAP_FADEBORDER)
	{
		newa = newa * dist / EGOMAP_FADEBORDER;
		if (newa == 0)  newa = 1;
		mesh  ->  vrta[vert] = newa;
	}
	*/

	return newa;

}

/*
 * Name:
 *     egomapCompleteMapData
 * Description:
 *     Completes loaded / generated map data with additional values needed
 *     for work.
 *       - Set number of fans
 *       - Set size of complete mesh
 *       - Set number of first vertex for fans
 * Input:
 *     mesh*:       Pointer on mesh to handle
 * Output:
 *    None
 */
static void egomapCompleteMapData(MESH_T *mesh)
{

	mesh -> numfan  = mesh -> tiles_x * mesh -> tiles_y;

	mesh -> edgex = (mesh -> tiles_x * EGOMAP_TILEDIV) - 1;
	mesh -> edgey = (mesh -> tiles_y * EGOMAP_TILEDIV) - 1;
	mesh -> edgez = 180 * 16;

	mesh -> watershift = 3;
	if (mesh -> tiles_x > 16)   mesh -> watershift++;
	if (mesh -> tiles_x > 32)   mesh -> watershift++;
	if (mesh -> tiles_x > 64)   mesh -> watershift++;
	if (mesh -> tiles_x > 128)  mesh -> watershift++;
	if (mesh -> tiles_x > 256)  mesh -> watershift++;

	/* Now set the number of first vertex for each fan */
	egomapSetFanStart(mesh);

	mesh -> numfreevert = (MAXTOTALMESHVERTICES - 10) - mesh -> numvert;

	/* Set flag that map has been loaded */
	mesh -> map_loaded = 1;

}

/*
 * Name:
 *     egomapCalcVrta
 * Description:
 *     Generates the 'a'  numbers for all files
 * Input:
 *     mesh*: Pointer on mesh to set twists for
 */
static void egomapCalcVrta(MESH_T *mesh)
{
	int num, cnt;
	int vert;
	int fan;


	for (fan = 0; fan < mesh -> numfan; fan++) {

		vert = mesh -> vrtstart[fan];
		num  = mesh -> pcmd[mesh -> fan[fan].type].numvertices;
		
		for (cnt = 0; cnt < num; cnt++) {
		
			egomapSetVrta(mesh, vert);
			vert++;
			
		}

	}

}

 /*
 * Name:
 *     egomapChangePassage
 * Description:
 *     Changes the info about passages on the map
 * Input:
 *     p_no:   Number of passage to set
 *     psg *:  Description of passage
 *     action: What to do
 */
 static void egomapChangePassage(char p_no, EDITFILE_PASSAGE_T *psg, char action)
 {

    int x_count, y_count;
    int tile_no;


    if (psg -> topleft[0] > 0 && psg -> topleft[1] > 0) {

        psg -> rec_no = p_no;   /* Save the number of the passage for the editor */

        if (EGOMAP_CLEAR == action) {

            /* Remove this passage */
            p_no = 0;

        }

        for (y_count = psg -> topleft[1]; y_count <= psg -> bottomright[1]; y_count++) {

            for (x_count = psg -> topleft[0]; x_count <= psg -> bottomright[0]; x_count++) {

                /* -- Tell the fan which passage it's belong to -- */
                tile_no = (y_count * Mesh.tiles_x) + x_count;
                Mesh.fan[tile_no].psg_no = p_no;

            }

        }

    }

 }

 /*
 * Name:
 *     egomapSpawnObject
 * Description:
 *     Spawns an object on map
 * Input:
 *     spt *:  Description of spawn point
 *     spt_no: Dunny for test purposes
 */
 static void egomapSpawnObject(EDITFILE_SPAWNPT_T *spt, int spt_no)
 {
    static inv_char = 0;   // Old character for inventory

    int char_no;
    // int attached_to;
    // char slot_no;



    /*
        attached_to = inv_char;

        if(spt -> view_dir == 'R')
        {
            slot_no = 0;
        }
        else if(spt -> view_dir == 'L')
        {
            slot_no = 1;
        }
        else if(spt -> view_dir == 'I')
        {
            slot_no = -1;
        }
        else
        {
            attached_to = 0;
            slot_no = 0;
        }

        @todo: char_no = charCreate(spt -> obj_name,
                                    attached_to,
                                    slot_no,
                                    spt -> team,
                                    spt -> stt,
                                    spt -> skin);
    */
    // Dummy for test purposes
    char_no = spt_no;

    if(char_no > 0)
    {
        if(spt -> x_pos > 0 && spt -> y_pos > 0)
        {
            inv_char = char_no;  /* Inventory belongs to this character/spawn point */
            // Is a visible object (on map)
            egomapDropChar(char_no, spt -> x_pos, spt -> y_pos, spt -> z_pos, spt -> view_dir);
        }
        else
        {
            // Is an inventory object
            // Put it to inventory
            // @todo: charInventoryAdd(attached_to, char_no, slot_no);
        }
    }
 }

/* ========================================================================== */
/* ============================= PUBLIC ROUTINE(S) ========================== */
/* ========================================================================== */

/*
 * Name:
 *     egomapInit
 * Description:
 *     Initializes the map data
 * Input:
 *     None 
 */
 void egomapInit(void)
 {

    COMMAND_T *mcmd;
    int entry, cnt;

    
    mcmd = MeshCommand;

    /* Correct all of them silly texture positions for seamless tiling */
    for (entry = 0; entry < (MAXMESHTYPE/2); entry++) {

        for (cnt = 0; cnt < (mcmd -> numvertices * 2); cnt++) {

            mcmd -> uv[cnt]    *= 0.1211f;   /* 31 / 256 */
            mcmd -> biguv[cnt] *= 0.2461f;   /* 63 / 256 */

        }

        mcmd++;

    }

    /* --- Clear buffer for map objects --- */
    memset(MapObj, 0, (EGOMAP_MAXOBJ + 1) * sizeof(SDLGL3D_OBJECT));

    /* --- Some more work --- */
    TileObj.bradius = 64.0;
    for (cnt = 0; cnt < 2; cnt++) {

        TileObj.bbox[cnt][0] = 64.0;
        TileObj.bbox[cnt][1] = 64.0;
        TileObj.bbox[cnt][2] = 128.0;   /* Height of a wall */

    }

    /* Calculate the UV-Data */
    editdrawInitData();

 }

 /*
 * Name:
 *     egomapSetFanStart
 * Description:
 *     Sets the 'vrtstart' for all fans in given map
 * Input:
 *     mesh*: Pointer on mesh to handle
 */
void egomapSetFanStart(MESH_T *mesh)
{

	int fan_no, vertex_no;

	for (fan_no = 0, vertex_no = 0; fan_no < mesh -> numfan; fan_no++) {

		mesh -> vrtstart[fan_no] = vertex_no;
        mesh -> fan[fan_no].vs   = vertex_no;       /* New info in fan-struct */
		vertex_no += mesh -> pcmd[mesh -> fan[fan_no].type & 0x1F].numvertices;

	}

}

 /*
 * Name:
 *     egomapLoad
 * Description:
 *     Loads
 *     Initializes the map data
 * Input:
 *     create:   Create an empty mesh yes/no
 *     msg *:    Where to return a possible error message
 *     num_tile: Number of fans square, if new map is to be returned
 */
MESH_T *egomapLoad(char create, char *msg, int num_tile)
{

    EDITFILE_PASSAGE_T *psg;
    EDITFILE_SPAWNPT_T *spt;
    int i;
    
    
    memset(&Mesh, 0, sizeof(MESH_T));

    /* Set pointer on commands */
    Mesh.pcmd = MeshCommand;

    if (create) {
        /* --- ToDo: Only create if no map-file exists at all, for safety reasons --- */
        Mesh.tiles_x = num_tile;
        Mesh.tiles_y = num_tile;
        /* --- Create an empty map for the caller to fill -- */
        egomapCompleteMapData(&Mesh);
        return &Mesh;

    }
    else {
        /* Load map data from file */
        if (editfileMapMesh(&Mesh, msg, 0)) {

            /* --- Load additional data for this map --- */
            /* Load the passages */
            psg = &Passages[1];
            editfilePassage(psg, EDITFILE_ACT_LOAD, EGOMAP_MAXPASSAGE);
            i = 1;
            while(psg -> line_name[0] != 0) {

                egomapChangePassage((char)i, psg, EGOMAP_SET);

                psg++;
                i++;

            }
            /* Load the spawn points */
            spt = &SpawnPts[1];
            editfileSpawn(spt, EDITFILE_ACT_LOAD, EGOMAP_MAXSPAWN);

            i = 1;
            while(spt -> obj_name[0] != 0)
            {
                egomapSpawnObject(spt, i);
                spt++;
                i++;
            }
            
            /* --- Complete the map info --- */
            egomapCompleteMapData(&Mesh);
            /* --- Success --- */
            return &Mesh;

        }

    }

    /* -- Failed -- */
    return 0;

}

/*
 * Name:
 *     egomapSave
 * Description:
 *     Saves the actual map 
 * Input:
 *     msg *: Where to return a possible error message 
 *     what:  What to save 
 * Output:
 *     Success yes/no    
 */
int egomapSave(char *msg, char what)
{

    if (what & EGOMAP_SAVE_PSG) {
        /* --- Save the passages file --- */
    }

    if (what & EGOMAP_SAVE_SPAWN) {
        /* --- Save the spawn points --- */
    }

    if (what & EGOMAP_SAVE_MAP) {
        /* --- Do save the map -- */
        egomapCalcVrta(&Mesh);
    	return editfileMapMesh(&Mesh, msg, 1);

    }

    return 1;

}
 
 /*
 * Name:
 *     egomapPassage
 * Description:
 *     Changes the info about passages on the map
 * Input:
 *     psg_no:  Number of passage to edit, if GET
 *     psg *:   Description of passage to handle
 *     action:  What to do   
 *     crect *: Chosen area for the passage, if new
 * Output:
 *     Data for passage is available yes/no
 */
 int egomapPassage(int psg_no, EDITFILE_PASSAGE_T *psg, char action, int *crect)
 {   

    char i;

    
    switch(action) {
    
        case EGOMAP_NEW:
            /* Look for buffer available */
            for (i = 1; i < EGOMAP_MAXPASSAGE; i++) {

                if (Passages[i].rec_no <= 0) {
                
                    /* Give it a name and make it valid */
                    sprintf(Passages[i].line_name, "%02d", i); 
                    Passages[i].rec_no = i;
                    Passages[i].topleft[0]     = crect[0];
                    Passages[i].topleft[1]     = crect[1];
                    Passages[i].bottomright[0] = crect[2];
                    Passages[i].bottomright[1] = crect[3];
                    /* -- Return data to caller -- */
                    memcpy(psg, &Passages[i], sizeof(EDITFILE_PASSAGE_T));
                    /* --- Put it to map --- */
                    egomapChangePassage(psg -> rec_no, psg, EGOMAP_SET);
                    /* TODO: Save them to file */
                    return 1;
                    
                }

            }
            break;
        case EGOMAP_GET:
            if (psg_no > 0) {
            
                memcpy(psg, &Passages[psg_no], sizeof(EDITFILE_PASSAGE_T));        
                return 1;

            }
            break;
            
        case EGOMAP_SET:
            if (psg -> rec_no > 0 && psg -> rec_no < EGOMAP_MAXPASSAGE) {
                memcpy(&Passages[psg -> rec_no], psg, sizeof(EDITFILE_PASSAGE_T));
                /* --- Put it to map --- */
                egomapChangePassage(psg -> rec_no, psg, EGOMAP_SET);
                /* TODO: Save them to file */
            }           
            break;
        case EGOMAP_CLEAR:         
            if (psg -> rec_no > 0 && psg -> rec_no < EGOMAP_MAXPASSAGE) {
                /* --- Remove it from map --- */
                egomapChangePassage(psg -> rec_no, psg, EGOMAP_CLEAR);
                /* --- Clear the record --- */
                memset(psg, 0, sizeof(EDITFILE_PASSAGE_T));
                psg -> rec_no = -1;     /* Sign it as free */
            }
            break;
        
    }

    return 0;
    
 }
 
 /*
 * Name:
 *     egomapSpawnPoint
 * Description:
 *     Handles the data of a given spawn point
 * Input:
 *     sp_no:   Number of spawn point to handle, if 'get'
 *     spt *:   Description of spawn-point
 *     action:  What to do
 *     crect *: Chosen tile, if new spawn point
 * Output:
 *     Success yes/no
 */
 int egomapSpawnPoint(int sp_no, EDITFILE_SPAWNPT_T *spt, char action, int *crect)
 {

    int tile_no;    
    int i;


    switch(action) {

        case EGOMAP_NEW:
            /* Look for buffer available */
            for (i = 1; i < EGOMAP_MAXSPAWN; i++) {

                if (SpawnPts[i].rec_no <= 0) {

                    /* Give it a name and make it valid */
                    SpawnPts[i].rec_no = i;
                    SpawnPts[i].x_pos  = crect[0] + 0.5;
                    SpawnPts[i].y_pos  = crect[1] + 0.5;
                    SpawnPts[i].z_pos  = 0.5;
                    /* -- Write number of object to map -- */
                    tile_no = (Mesh.tiles_x * crect[1]) + crect[0];
                    Mesh.fan[tile_no].obj_no = i;
                    /* -- Return data to caller -- */
                    memcpy(spt, &SpawnPts[i], sizeof(EDITFILE_SPAWNPT_T));
                    return 1;

                }

            }
            break;
        case EGOMAP_GET:
            if (sp_no > 0) {

                memcpy(spt, &SpawnPts[sp_no], sizeof(EDITFILE_SPAWNPT_T));
                return 1;

            }
            break;
        case EGOMAP_SET:
            if (spt -> rec_no > 0 && spt -> rec_no < EGOMAP_MAXSPAWN) {
                memcpy(&SpawnPts[spt -> rec_no], spt, sizeof(EDITFILE_SPAWNPT_T));
                /* TODO: Save them to file  ? */
            }
            break;
        case EGOMAP_CLEAR:
            tile_no = Mesh.tiles_x * floor(spt -> y_pos) + floor(spt -> x_pos);
            Mesh.fan[tile_no].obj_no = 0;       /* Remove it from map */
            if (spt -> rec_no > 0 && spt -> rec_no < EGOMAP_MAXSPAWN) {
                memset(spt, 0, sizeof(EDITFILE_SPAWNPT_T));
                spt -> rec_no = -1;     /* Sign it as free */
            }
            break;

    }

    return 0;

 }

 /*
 * Name:
 *     egomapGetTileInfo
 * Description:
 *     Returns the info about the tile at given x/y-Position 
 *     fd -> psg_no > 0: Passage with this number on this fan
 *     fd -> obj_no > 0: Spawn point with this number on this fan
 * Input:
 *     tx, ty: Position on map
 *     fd *:   Pointer on buffer where to return the data
 */
 void egomapGetTileInfo(int tx, int ty, FANDATA_T *fd)
 {

    int tile_no;    
    
    
    tile_no = (Mesh.tiles_x * ty) + tx;
    memcpy(fd, &Mesh.fan[tile_no], sizeof(FANDATA_T));
 
 } 
 
 /*
 * Name:
 *     egomapDraw
 * Description:
 *     Draw the whole map and allt he objects on it
 * Input:
 *     fd *:    Fandata from editor
 *     cm *:    Command data from editor
 *     crect *: Pointer on edges of chosen rectangle, if any
 */
void egomapDraw(FANDATA_T *fd, COMMAND_T *cm, int *crect)
{

    editdraw3DView(&Mesh, fd, cm, crect);
 
}

 /*
 * Name:
 *     egomapDraw2DMap
 * Description:
 *     Draw a 2D-Map of the mesh 
 * Input:
 *     mx, my:  Draw at this position on screen
 *     mw, mh:  Size of map on Screen  
 *     tw:      Tile-Width for minimap 
 *     crect *: Edges of chosen rectangle, if any
 */
void egomapDraw2DMap(int mx, int my, int mw, int mh, int tw, int *crect)
{

    if (tw > 0 && Mesh.map_loaded) {

         editdraw2DMap(&Mesh, mx, my, mw, mh, tw, crect);

    }

}

/* ============ OBJECT-FUNCTIONS ========== */

/*
 * Name:
 *     egomapAddObject
 * Description:
 *     Adds an object to the map. Loads it from file if needed 
 * Input:
 *     obj_name *: Name of object to load 
 *     x, y, z: Position
 *     dir:     Direction the object looks
 */
void egomapAddObject(char type, char obj_name, float x, float y, float z, char dir)
{
    // int type_no;
    char dir_no;
    int char_no;


    switch(dir)
    {
        case 'E':
            dir_no = 1;
            break;
        case 'S':
            dir_no = 2;
            break;
        case 'W':
            dir_no = 3;
            break;
        default:
            dir_no = 0; /* North or inventory thing */
    }

    if(EGOMAP_OBJ_CHAR == type)
    {
        // @todo: Load the character object (if needed from file)
        // char_no = charCreate(obj_name, 0, 0, 0, 0, 0);
        char_no = type;
        // Drop it to the map
        egomapDropChar(char_no, x, y, z, dir_no);
    }
    else if(EGOMAP_OBJ_PART == type)
    {
        // @todo: Create a particle
        // type_no = type;
    }
}

/*
 * Name:
 *     egomapDeleteObject
 * Description:
 *     Deletes an object from the map
 * Input:
 *     obj_no: Number of object to delete
 */
void egomapDeleteObject(int obj_no)
{
    if (obj_no > 0 && obj_no < EGOMAP_MAXOBJ)
    {
        memset(&MapObj[obj_no], 0, sizeof(SDLGL3D_OBJECT));
        MapObj[obj_no].obj_type = -1;                       /* Sign it as deleted */
    }
}

/*
 * Name:
 *     egomapGetChar
 * Description:
 *     Get the number of the character on this tile, if any
 * Input:
 *     tile_no: Number of tile to check for character
 * Output:
 *     > 0: Number of valid character, if found
 */
int egomapGetChar(int tile_no)
{
    int obj_no;


    obj_no = Mesh.fan[tile_no].obj_no;

    if(obj_no > 0 && MapObj[obj_no].obj_type == EGOMAP_OBJ_CHAR)
    {
        // Return number of character
        return MapObj[obj_no].type_no;
    }

    return 0;
}

/*
 * Name:
 *     egomapDropChar
 * Description:
 *     Drops a character to the map at this position.
 *     Usage: E.g. for dropping an item from inventory
 * Input:
 *     char_no: Number of charactr to drop at this position
 *     x, y, z: Number of tile to check for character
 *     dir:     Direction
 */
void egomapDropChar(int char_no, float x, float y, float z, char dir)
{
    int obj_no;
    int tile_no;


    obj_no = egomapNewObject(EGOMAP_OBJ_CHAR, char_no, x, y, z, dir);

    if(obj_no > 0)
    {
       // Put it to the map
       // @todo: Create a linked list if needed (more then one object on tile)
       tile_no = (floor(y) * Mesh.tiles_x) + floor(x);
       Mesh.fan[tile_no].obj_no = obj_no;
    }
}

/* ============ ____-FUNCTIONS ========== */