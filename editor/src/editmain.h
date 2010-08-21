/*******************************************************************************
*  EDITMAIN.H                                                                  *
*	- Main edit functions for map and other things      	                   *
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
*                                                                              *
*                                                                              *
* Last change: 2008-06-21                                                      *
*******************************************************************************/

#ifndef _EDITMAIN_H_
#define _EDITMAIN_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include "editor.h"             /* Definition FANDATA_T and COMMAND_T   */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITMAIN_DRAWMAP     0
#define EDITMAIN_NEWFLATMAP  1
#define EDITMAIN_NEWSOLIDMAP 2
#define EDITMAIN_LOADMAP     3
#define EDITMAIN_SAVEMAP     4
#define EDITMAIN_ROTFAN      5      /* Rotates the actual chosen fan if browsing        */ 
#define EDITMAIN_UPDATEFAN   6      /* Does an update on the actual fan from 'new_fan'  */

/* ---------- Edit-Flags -------- */
#define EDITMAIN_SHOW2DMAP 0x01         /* Display the 2DMap        */
/* --- Edit-States (how to set fans) */ 
#define EDITMAIN_EDIT_NONE      0x00
#define EDITMAIN_EDIT_SIMPLE    0x01
#define EDITMAIN_EDIT_FULL      0x02

/* --------- Other values ------- */
#define EDITMAIN_MAXSPAWN   150         /* Maximum Lines in spawn list  */
#define EDITMAIN_MAXPASSAGE  30

/* ---- Flags to toggle --------- */
#define EDITMAIN_TOGGLE_DRAWMODE    1
#define EDITMAIN_TOGGLE_FX          2
#define EDITMAIN_TOGGLE_TXHILO      3

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int  fan_chosen;    /* Actual chosen fan for info or editing */
    int  tx, ty;        /* Position of fan as x/y on map    */
    char display_flags; /* For display in main editor       */
    char draw_mode;     /* For copy into mesh - struct      */
    char brush_size;    /* From 'cartman'  0-3, (slider)    */    
    char brush_amount;  /* From 'cartman' -50,  50 (slider) */    
    char bft_type;      /* Fantype for 'browsing            */
    char fan_dir;       /* Direction of new fan             */
    int  fan_no;        /* Number of new fan                */
    FANDATA_T ft;       /* Copy of type of chosen fan       */ 
    COMMAND_T fd;       /* Extent data for new fan (bt_type)*/
    char msg[256];      /* Possible message from editor     */

} EDITMAIN_STATE_T;

typedef struct {

    char line_name[25];
    char item_name[20+1];
    char slot_no[3 + 1];
    char xpos[4 + 1];
    char ypos[4 + 1];
    char zpos[4 + 1];
    char dir[1 + 1];
    char mon[3 + 1];
    char skin[1 + 1];
    char pas[2 + 1];
    char con[2 + 1];
    char lvl[2 + 1];
    char stt[1 + 1];
    char gho[1 + 1];
    char team[12 + 1];

} SPAWN_OBJECT_T;       /* Description of a spawned object */

typedef struct {
    char line_name[25];
    char item_name[20+1];
    int  slot_no;       /* Use it for coloring the bounding boxes */
    float x_pos, y_pos, z_pos;
    int  view_dir;
} EDITOR_SPAWNPT_T;     /* Spawn-Point for display on map. From 'spawn.txt' */

typedef struct {
    char line_name[25];
    int  top_left[2];       /* X, Y, in tiles */
    int  bottom_right[2];   /* X, Y, in tiles */ 
} EDITOR_PASSAGE_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

EDITMAIN_STATE_T *editmainInit(void);
void editmainExit(void);
int  editmainMap(int command);
void editmainDrawMap2D(int x, int y, int w, int h);
SPAWN_OBJECT_T *editmainLoadSpawn(void);
void editmainToggleFlag(int which, unsigned char flag);
void editmainChooseFan(int cx, int cy, int w, int h);
char *editmainFanTypeName(int type_no);
void editmainChooseFanType(int dir, char *fan_name);
int  editmainSetSimple(int fan_no, int is_floor);
void editmain2DTex(int x, int y, int w, int h);
int  editmainFanSet(char edit_state, char is_floor);

#endif /* _EDITMAIN_H_	*/

