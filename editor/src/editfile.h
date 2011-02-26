/*******************************************************************************
*  EDITFILE.H                                                                  *
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

#ifndef _EDITFILE_H_
#define _EDITFILE_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include "editor.h"     /* Definition of map-mesh MESH_T and COMMAND_T  */
#include "sdlglcfg.h"   /* Read egoboo text files eg. passage, spawn    */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITFILE_LASTREC    -1  /* Get last record in data list */    

#define EDITFILE_ACT_LOAD    1
#define EDITFILE_ACT_SAVE    2
#define EDITFILE_ACT_GETDATA 3  /* In given buffer      */
#define EDITFILE_ACT_SETDATA 4  /* From given buffer    */      
#define EDITFILE_ACT_NEW     5  /* Return empty record  */   

/* -- Definiton of directories */
#define EDITFILE_WORKDIR    1   /* Main directory                       */
#define EDITFILE_GAMEDATDIR 2   /* Gamedata directory in main directory */

/*******************************************************************************
* TYPEDEFS 								                                       *
*******************************************************************************/

/* ====== The different file types ===== */
typedef struct {

    char line_name[25];         /* Only for information purposes */
    int topleft[2];
    int bottomright[2];
    char open;
    char shoot_trough;
    char slippy_close;
    
} EDITFILE_PASSAGE_T;

typedef struct {

    char line_name[25];     /* Name of object to load   */
    char item_name[20+1];
    int  slot_no;           /* Use it for coloring the bounding boxes */
    float x_pos, y_pos, z_pos;
    char view_dir;
    int  money;
    char skin;
    char pas;
    char con;
    char lvl;
    char stt;
    char gho;
    char team;
    
} EDITFILE_SPAWNPT_T;     /* Spawn-Point for display on map. From 'spawn.txt' */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void editfileSetWorkDir(char *dir_name);
char *editfileMakeFileName(int dir_no, char *fname);
int  editfileMapMesh(MESH_T *mesh, char *msg, char save);
int  editfileSpawn(int action, int rec_no, EDITFILE_SPAWNPT_T *spt);
int  editfilePassage(int action, int rec_no, EDITFILE_PASSAGE_T *psg);

#endif  /* _EDITFILE_H_ */