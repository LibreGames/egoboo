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

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITMAIN_DRAWMAP 0
#define EDITMAIN_NEWMAP  1
#define EDITMAIN_LOADMAP 2  
#define EDITMAIN_SAVEMAP 3

/* ---------- Edit-Flags -------- */
#define EDITMAIN_SHOW2DMAP 0x10        /* Display the 2DMap        */
                                            
/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int act_fan;                    /* Actual chosen fan for editing          */
    unsigned char draw_mode;        /* For copy into mesh - struct            */
    unsigned char fx;               /* Fx for chosen fan                      */
    unsigned short maintex_no;      /* Number of main texture to use for fan  */
    unsigned short subtex_no;       /* Number of 'sub-texture' to use for fan */

} EDITMAIN_STATE_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void editmainInit(EDITMAIN_STATE_T *edit_state);
void editmainExit(void);
int  editmainMap(int command);
void editmainDrawMap2D(int x, int y, int w, int h);

#endif /* _EDITMAIN_H_	*/

