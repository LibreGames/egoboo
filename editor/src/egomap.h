/*******************************************************************************
*  EGOMAP.H                                                                    *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Managing the map and the objects on it                                  *
*      (c)2011 Paul Mueller <pmtech@swissonline.ch>                            *
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

#define MPDFX_REF       0x00    /* MeshFX   */
#define MPDFX_SHA       0x01    
#define MPDFX_DRAWREF   0x02    
#define MPDFX_ANIM      0x04    
#define MPDFX_WATER     0x08    
#define MPDFX_WALL      0x10    
#define MPDFX_IMPASS    0x20    
#define MPDFX_DAMAGE    0x40    
#define MPDFX_SLIPPY    0x80  

/* ---- For EGOMAP_T: Kind of additional info ---- */
#define EGOMAP_CHOSEN   0x80        /* Fan is chosen by editor  */ 

/* --- Save files --- */
#define EGOMAP_SAVE_MAP     0x01    /* Save the map             */
#define EGOMAP_SAVE_PSG     0x02    /* Save the passages        */
#define EGOMAP_SAVE_SPAWN   0x04    /* save the spawn points    */   
#define EGOMAP_SAVE_ALL     0xFF 

/* --- Types of objects (for drawing) --- */
#define EGOMAP_OBJ_TILE   0x01    /* First always the tile            */
#define EGOMAP_OBJ_CHAR   0x02    /* Characters (MD2)                 */
#define EGOMAP_OBJ_PART   0x03    /* Particles (can be transparent)   */

/* --- Handling for map objects --- */
#define EGOMAP_GET   0x01
#define EGOMAP_SET   0x02
#define EGOMAP_CLEAR 0x03

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void egomapInit(void);
void egomapSetFanStart(MESH_T *mesh);
MESH_T *egomapLoad(char create, char *msg, int num_tile);
int  egomapSave(char *msg, char what);
void egomapPassage(char p_no, EDITFILE_PASSAGE_T *psg, char action);
void egomapSpawnPoint(int s_no, EDITFILE_SPAWNPT_T *spt, char action);
void egomapGetTileInfo(int tile_no, FANDATA_T *ft);

/* --- Draw command --- */
void egomapDraw(FANDATA_T *ft, COMMAND_T *fd);
void egomapDraw2DMap(int x, int y, int w, int h);

#endif  /* #define _EGOMAP_H_ */