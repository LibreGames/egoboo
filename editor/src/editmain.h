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

#include "editfile.h"             /* Definition FANDATA_T and COMMAND_T   */

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
#define EDITMAIN_MAX_MAPSIZE  128
#define EDITMAIN_MAXSELECT    25        /* Maximum fans to be selected  */

/* ---- Flags to toggle --------- */
#define EDITMAIN_TOGGLE_DRAWMODE    1
#define EDITMAIN_TOGGLE_EDITSTATE   2
#define EDITMAIN_TOGGLE_FANTEXNO    3 

/* --- different edit modes --- */
#define EDITMAIN_MODE_OFF     ((char)1)   /* 'View' map                        */
#define EDITMAIN_MODE_MAP     ((char)2)   /* 'Carve' out map with mouse        */
#define EDITMAIN_MODE_FAN     ((char)3)   /* Result of fan dialog              */
#define EDITMAIN_MODE_FAN_FX  ((char)4) 
#define EDITMAIN_MODE_FAN_TEX ((char)5)
#define EDITMAIN_MODE_EQUIP   ((char)6) 
#define EDITMAIN_MODE_MODULE  ((char)8) /* Change info in module description */
#define EDITMAIN_MODE_VERTEX  ((char)9) 
#define EDITMAIN_MODE_FREE    ((char)10)  

/* --- Commands in Edit-Mode --- */
#define EDITMAIN_SET_MAP      ((char)22)  
#define EDITMAIN_SET_FAN      ((char)23)
#define EDITMAIN_SET_FAN_FX   ((char)24) 
#define EDITMAIN_SET_FAN_TEX  ((char)24) 

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int  minimap_tw,
         minimap_w,
         minimap_h;
    int  tx, ty;        /* Position of fan as x/y on map    */
    char display_flags; /* For display in main editor       */
    char draw_mode;     /* For copy into mesh - struct      */
    char edit_mode;     /* EDITMAIN_MODE_...                */
    char bft_no;        /* Number of fan-type in fan-set    */
    char fan_dir;       /* Direction of new fan             */    
    FANDATA_T ft;       /* Copy of actual chosen fan        */
    COMMAND_T cm;       /* Extent data for new fan type     */
    char msg[256];      /* Possible message from editor     */
    int  map_size;      /* Map-Size chosen by user          */
    char map_loaded;    /* A Map is loaded, yes/no          */
    /* --- Additional map info to return if no map editing itself --- */
    int  mi_fan_no;
    /* ---- Choosing multiple tiles, save drag of mouse --- */
    int  drag_x, drag_y, drag_w, drag_h;
    int  crect[5];      /* Extent of chosen tile, if any    */  
    char fan_name[128]; /* Name of fan, if free is chosen   */
    
} EDITMAIN_INFO_T;

typedef struct {

    int x, y;

} EDITMAIN_XY;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void editmainInit(EDITMAIN_INFO_T *es, int map_size);
void editmainExit(void);
int  editmainMap(EDITMAIN_INFO_T *es, int command);
char editmainToggleFlag(EDITMAIN_INFO_T *es, int which, unsigned char flag);
void editmainChooseFan(EDITMAIN_INFO_T *es, int is_floor);
void editmainChooseFanType(EDITMAIN_INFO_T *es, int dir);
void editmain2DTex(EDITMAIN_INFO_T *es, int x, int y, int w, int h);
void editmainChooseTex(EDITMAIN_INFO_T *es, int cx, int cy, int w, int h);

#endif /* _EDITMAIN_H_	*/

