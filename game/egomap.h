/*******************************************************************************
*  EGOMAP.H                                                                    *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Managing the map and the objects on it                                  *
*      (c) 2011-2013 Paul Mueller <muellerp61@bluewin.ch>                      *
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

#ifndef _EGOMAP_H_
#define _EGOMAP_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include "egofile.h"   /* Description of passages and spawn points */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

/* --- Save files --- */
#define EGOMAP_SAVE_MAP     0x01    /* Save the map             */
#define EGOMAP_SAVE_PSG     0x02    /* Save the passages        */
#define EGOMAP_SAVE_SPAWN   0x04    /* save the spawn points    */   
#define EGOMAP_SAVE_ALL     0xFF 

/* --- Handling for map objects (Spawn points and passages) --- */
#define EGOMAP_NEW    0x01      
#define EGOMAP_GET    0x02
#define EGOMAP_SET    0x03
#define EGOMAP_CLEAR  0x04
// Actions for passage
#define EGOMAP_PSGOPEN   0x05  
#define EGOMAP_PSGCLOSE  0x06  
#define EGOMAP_PSGSWITCH 0x07

// Actions for passage
#define EGOMAP_PSGSETOWNER 0x08

// Numbers of FX-flags (for script use)
#define EGOMAP_FXREF       ((char)-1)   
#define EGOMAP_FXSHA       ((char)0)    
#define EGOMAP_FXDRAWREF   ((char)1)   
#define EGOMAP_FXANIM      ((char)2)  
#define EGOMAP_FXWATER     ((char)3)   
#define EGOMAP_FXWALL      ((char)4) 
#define EGOMAP_FXIMPASS    ((char)5)
#define EGOMAP_FXDAMAGE    ((char)6)    
#define EGOMAP_FXSLIPPY    ((char)7) 


/*******************************************************************************
* TYPEDESF							                                           *
*******************************************************************************/

// Short information about a tile 
typedef struct 
{
    int  tile_no;       /* Number of tile                           */
    int  x, y;          /* X,Y-Position in map                      */
    char psg_no;        /* > 0: Number of passage for this tile     */
    int  obj_no;        /* > 0: Number of first object on this tile */    
    char fx;            /* MPDFX_...                                */
    char twist;         /* Twist for collision info                 */
    // @todo: Support position of bottom tiles
    int  tile_z;        /* Z-Position of tile for jump and bump     */    
    
} EGOMAP_TILEINFO_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/* ============ Map functions ========== */
void egomapInit(void);
void egomapSetFanStart(MESH_T *mesh);
char egomapLoad(char *mod_name, int savegame_no, char *msg);

/* ============  Tile functions ========== */
void egomapGetAdjacent(int tx, int ty, EGOMAP_TILEINFO_T adjacent[8]);
void egomapGetAdjacentPos(int pos_x, int pos_y, EGOMAP_TILEINFO_T adjacent[4]);

/* ============  Draw command ========== */
void egomapDraw(void);

/* ============ OBJECT-FUNCTIONS ========== */
void egomapPutObject(int obj_no);       // Put this object to map
void egomapDropObject(int obj_no);      // Drop this object to map (fall to bottom)
void egomapDeleteObject(int obj_no);    // Delete this object from map
int  egomapGetChar(int tile_no);        // Get character info from this tile (flags: GET, PEEK)
int  egomapPutChar(int char_no, float pos[3], char dir_code, int mdl_no);
void egomapMoveObjects(void);           // Moves all objects on map

/* ============ Game (script) functions ========== */
void egomapPassageFunc(int psg_no, int action, int value);
// @todo: int   egomapGlobalFlag(int action, int flag_no);
// @todo: int   egomapHandleTile(int action, int x, int y, inv val1, int val2);

#endif  /* #define _EGOMAP_H_ */