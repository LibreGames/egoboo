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

#include "editfile.h"   /* Description of passages and spawn points */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

/* --- Save files --- */
#define EGOMAP_SAVE_MAP     0x01    /* Save the map             */
#define EGOMAP_SAVE_PSG     0x02    /* Save the passages        */
#define EGOMAP_SAVE_SPAWN   0x04    /* save the spawn points    */   
#define EGOMAP_SAVE_ALL     0xFF 

/* --- Types of objects (for drawing) --- */
#define EGOMAP_OBJ_TILE   0x01      /* First always the tile            */
#define EGOMAP_OBJ_CHAR   0x02      /* Characters (MD2)                 */
#define EGOMAP_OBJ_PART   0x03      /* Particles (can be transparent)   */

/* --- Handling for map objects (Spawn points and passages) --- */
#define EGOMAP_NEW    0x01      
#define EGOMAP_GET    0x02
#define EGOMAP_SET    0x03
#define EGOMAP_CLEAR  0x04
// Actions for passage
#define EGOMAP_OPEN   0x05  
#define EGOMAP_CLOSE  0x06  
#define EGOMAP_SWITCH 0x06


#define EGOMAP_TILEDIV     128      /* Size of a tile       */

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
    // @todo: Support position of bottom tiles
    int  tile_z;        /* Z-Position of tile for jump and bump     */    
    
} EGOMAP_TILEINFO_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/* ============ Map functions ========== */
void egomapInit(void);
void egomapSetFanStart(MESH_T *mesh);
MESH_T *egomapLoad(char create, char *msg, int num_tile);
int  egomapSave(char *msg, char what);
int  egomapPassage(int psg_no, EDITFILE_PASSAGE_T *psg, char action, int *crect);
int  egomapSpawnPoint(int sp_no, EDITFILE_SPAWNPT_T *spt, char action, int *crect);

/* ============  Tile functions ========== */
char egomapGetFanData(int tx, int ty, FANDATA_T *fd);
char egomapGetTileInfo(int tx, int ty, EGOMAP_TILEINFO_T *ti);
void egomapGetAdjacent(int tx, int ty, EGOMAP_TILEINFO_T adjacent[8]);
void egomapGetAdjacentPos(int pos_x, int pos_y, EGOMAP_TILEINFO_T adjacent[4]);

/* ============  Draw command ========== */
void egomapDraw(FANDATA_T *fd, COMMAND_T *cd, int *crect);
void egomapDraw2DMap(int mx, int my, int mw, int mh, int tx, int *crect);

/* ============ OBJECT-FUNCTIONS ========== */
void egomapNewObject(char *obj_name, float x, float y, float z, char dir_code);
void egomapDropObject(int obj_no);      // Drop this object to map
void egomapDeleteObject(int obj_no);    // Delete this object from map
int  egomapGetChar(int tile_no);        // Get character info from this tile (flags: GET, PEEK)
void egomapDropChar(int char_no, float x, float y, float z, char dir); 
// @todo: egomapMoveAllObjects(void);   // Manages the linked lists on tile and invokes passages
// @todo: Add/remove objects from a single linked list
// int sdlgl3dListObject(int *from_base, int *to_base, int obj_no, char action); // action: ADD, REMOVE, PEEK, MOVE

/* ============  Game functions ========== */
void egomapHandlePassage(int psg_no, int action);

#endif  /* #define _EGOMAP_H_ */