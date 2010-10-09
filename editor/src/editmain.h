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
#define EDITMAIN_NEWMAP      2
#define EDITMAIN_LOADMAP     3
#define EDITMAIN_SAVEMAP     4
#define EDITMAIN_ROTFAN      5      /* Rotates the actual chosen fan if browsing        */ 
#define EDITMAIN_SETFANPROPERTY 6   /* Set the properties of a fan (fx and texture)     */       
#define EDITMAIN_CHOOSEPASSAGE  6   /* Choose a passage, if passages are loaded         */
#define EDITMAIN_CHOOSESPAWNPOS 7   /* Choose a spawn pos, if span positions are loaded */

/* ---------- Edit-Flags -------- */
#define EDITMAIN_SHOW2DMAP 0x01         /* Display the 2DMap        */

/* --- Edit-States (how to set fans) */ 
#define EDITMAIN_EDIT_NONE      0x00
#define EDITMAIN_EDIT_SIMPLE    0x01
#define EDITMAIN_EDIT_FREE      0x02    /* Edit complete single chosen fan      */
#define EDITMAIN_EDIT_FX        0x03    /* Edit FX-Flags of chosen fan(s)       */
#define EDITMAIN_EDIT_TEXTURE   0x04    /* Edit the textures of chosen fan(s)   */
#define EDITMAIN_EDIT_MAX       0x04    /* Maximum number of edit states        */

/* --------- Other values ------- */
#define EDITMAIN_MAX_MAPSIZE  64
#define EDITMAIN_MAXSELECT    25        /* Maximum fans to be selected  */

/* ---- Flags to toggle --------- */
#define EDITMAIN_TOGGLE_DRAWMODE    1
#define EDITMAIN_TOGGLE_EDITSTATE   2
#define EDITMAIN_TOGGLE_FANTEXSIZE  3 
#define EDITMAIN_TOGGLE_FANTEXNO    4 
#define EDITMAIN_TOGGLE_FANFX       5

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int  fan_selected[EDITMAIN_MAXSELECT + 1];    
    int  psg_no;        /* > 0: Number of passage chosen        */
    int  spawnpos_no;   /* > 0: Number of spawn position chosen */
    int  light_no;      /* > 0: Number of light chosen          */ 
    int  minimap_w,
         minimap_h;
    int  tx, ty;        /* Position of fan as x/y on map    */
    char display_flags; /* For display in main editor       */
    char draw_mode;     /* For copy into mesh - struct      */
    char edit_mode;     /* None/simple/free                 */
    char bft_no;        /* Number of fan-type in fan-set    */
    char fan_dir;       /* Direction of new fan             */
    FANDATA_T ft;       /* Copy of type of chosen fan       */ 
    COMMAND_T fd;       /* Extent data for new fan type     */
    char msg[256];      /* Possible message from editor     */
    int map_size;       /* Map-Size chosen by user          */

} EDITMAIN_STATE_T;

typedef struct {

    int x, y;

} EDITMAIN_XY;

typedef struct {

    char line_name[25];         /* Only for information purposes */
    EDITMAIN_XY topleft;
    EDITMAIN_XY bottomright;
    char open;
    char shoot_trough;
    char slippy_close;
    
} EDITMAIN_PASSAGE_T;

typedef struct {

    char line_name[25];
    char item_name[20+1];
    int  slot_no;           /* Use it for coloring the bounding boxes */
    float x_pos, y_pos, z_pos;
    char view_dir;
    int  money;
    int  skin;
    int  pas;
    int  con;
    int  lvl;
    char stt;
    char gho;
    char team;
    
} EDITMAIN_SPAWNPT_T;     /* Spawn-Point for display on map. From 'spawn.txt' */

typedef struct {
    int           x;
    int           y;
    unsigned char level;
    int           radius;
} LIGHT_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

EDITMAIN_STATE_T *editmainInit(int map_size, int minimap_w, int minimap_h);
void editmainExit(void);
int  editmainMap(int command, int info);
void editmainDrawMap2D(int x, int y);
char editmainToggleFlag(int which, unsigned char flag);
void editmainChooseFan(int cx, int cy, int is_floor);
void editmainChooseFanExt(int cx, int cy, int cw, int ch);
void editmainFanTypeName(char *fan_name);
void editmainChooseFanType(int dir, char *fan_name);
void editmain2DTex(int x, int y, int w, int h);
void editmainChooseTex(int cx, int cy, int w, int h);

#endif /* _EDITMAIN_H_	*/

