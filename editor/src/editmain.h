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

/* ---------- Edit-Flags -------- */
#define EDITMAIN_SHOW2DMAP 0x01         /* Display the 2DMap        */

/* --------- Other values ------- */
#define EDITMAIN_MAX_MAPSIZE  64
#define EDITMAIN_MAXSELECT    25        /* Maximum fans to be selected  */

/* ---- Flags to toggle --------- */
#define EDITMAIN_TOGGLE_DRAWMODE    1
#define EDITMAIN_TOGGLE_EDITSTATE   2
#define EDITMAIN_TOGGLE_FANTEXNO    3 

/* --- different edit modes --- */
#define EDITMAIN_STATE_OFF     ((char)1)   /* 'View' map                        */
#define EDITMAIN_STATE_MAP     ((char)2)   /* 'Carve' out map with mouse        */
#define EDITMAIN_STATE_FAN     ((char)3)   /* Result of fan dialog              */
#define EDITMAIN_STATE_PASSAGE ((char)4)
#define EDITMAIN_STATE_OBJECT  ((char)5)
#define EDITMAIN_STATE_MODULE  ((char)6)   /* Change info in module description */
#define EDITMAIN_STATE_VERTEX  ((char)7)
#define EDITMAIN_STATE_FREE    ((char)8) 

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int  fan_selected[EDITMAIN_MAXSELECT + 1];    
    int  minimap_w,
         minimap_h;
    int  tx, ty;        /* Position of fan as x/y on map    */
    char display_flags; /* For display in main editor       */
    char draw_mode;     /* For copy into mesh - struct      */
    char edit_mode;     /* EDITOR_TOOL_...                  */
    char bft_no;        /* Number of fan-type in fan-set    */
    char fan_dir;       /* Direction of new fan             */    
    FANDATA_T ft;       /* Copy of actual chosen fan        */
    COMMAND_T fd;       /* Extent data for new fan type     */
    char msg[256];      /* Possible message from editor     */
    int  map_size;      /* Map-Size chosen by user          */
    /* --- Additional map info to return if no map editing itself       */
    int  mi_fan_no;
    char mi_type;
    int  mi_t_number; 
    /* ---- Choosing multiple tiles, save drag of mouse --- */
    int  drag_x, drag_y, drag_w, drag_h;
    char fan_name[128]; /* Name of fan, if free is chosen   */
    
} EDITMAIN_INFO_T;

typedef struct {

    int x, y;

} EDITMAIN_XY;

typedef struct {

    int           x;
    int           y;
    unsigned char level;
    int           radius;
    
} LIGHT_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void editmainInit(EDITMAIN_INFO_T *es, int map_size, int minimap_tile_w);
void editmainExit(void);
int  editmainMap(EDITMAIN_INFO_T *es, int command);
void editmainDrawMap2D(EDITMAIN_INFO_T *es, int x, int y);
char editmainToggleFlag(EDITMAIN_INFO_T *es, int which, unsigned char flag);
void editmainChooseFan(EDITMAIN_INFO_T *es, int cx, int cy, int is_floor);
void editmainChooseFanExt(EDITMAIN_INFO_T *es);
void editmainChooseFanType(EDITMAIN_INFO_T *es, int dir);
void editmain2DTex(EDITMAIN_INFO_T *es, int x, int y, int w, int h);
void editmainChooseTex(EDITMAIN_INFO_T *es, int cx, int cy, int w, int h);

void editmainClearMapInfo(void);
void editmainSetMapInfo(char which, int number, int x, int y, int x2, int y2);
void editmainGetMapInfo(EDITMAIN_INFO_T *es, int mou_x, int mou_y);  
    
#endif /* _EDITMAIN_H_	*/

