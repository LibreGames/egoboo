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
#define EDITFILE_ACT_DELETE  6  /* Delete this record   */

/* -- Definiton of directories */
#define EDITFILE_WORKDIR     1  /* Main directory                       */
#define EDITFILE_BASICDATDIR 2  /* Basic data for game                  */
#define EDITFILE_GAMEDATDIR  3  /* Gamedata directory in main directory */
#define EDITFILE_OBJECTDIR   4  /* Directory for the objects            */
#define EDITFILE_EGOBOODIR   5

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

typedef struct {

  char mod_name[24 + 1];        /* With underscores */
  char ref_mod[24 + 1];         /*      Reference module ( Directory name or NONE )  */
  char ref_idsz[24 + 1];        /*      Required reference IDSZ ( or [NONE] ) : [MAIN] 6  */
  char number_of_imports;       /*      Number of imports ( 0 to 4 ) : 4 */
  char allow_export;            /*      Allow exporting of characters ( TRUE or FALSE )  */
  char min_player;              /*      Minimum number of players ( 1 to 4 )  */
  char max_player;              /* Maximum number of players ( 1 to 4 ) : 4  */
  char allow_respawn;           /*      Allow respawning ( TRUE or FALSE ) : TRUE  */
  char is_rts;                  /*      Is a RTS module (TRUE or FALSE) : FALSE (always FALSE) */
  char lev_rating[8 + 2];       /*      Level rating ( *, **, ***, ****, or ***** )  */
  /* // Module summary ( Must be 8 lines... Each line mush have at least an _ )  */
  char summary[8][80 + 2];
  /* Module expansion IDSZs ( with a colon in front )   */
  /*    :[TYPE] MAINQUEST //Module Type (MAINQUEST, SIDEQUEST or TOWN)  */
  char exp_idsz[5][18 + 1];

} EDITFILE_MODULE_T;           /* Data from 'menu.txt'                 */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void editfileSetWorkDir(char *dir_name);
char *editfileMakeFileName(int dir_no, char *fname);
int  editfileMapMesh(MESH_T *mesh, char *msg, char save);
int  editfileSpawn(int action, int rec_no, EDITFILE_SPAWNPT_T *spt);
int  editfilePassage(int action, int rec_no, EDITFILE_PASSAGE_T *psg);
int  editfileModuleDesc(int action, EDITFILE_MODULE_T *moddesc);

#endif  /* _EDITFILE_H_ */