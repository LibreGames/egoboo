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
* DEFINES								                                       *
*******************************************************************************/

/* ===== define sthe map mesh ===== */
#define MAXMESHTYPE                     64          // Number of mesh types
#define MAXMESHLINE                     64          // Number of lines in a fan schematic
#define MAXMESHVERTICES                 16          // Max number of vertices in a fan
#define MAXMESHFAN                      (128*128)   // Size of map in fans
#define MAXTOTALMESHVERTICES            ((MAXMESHFAN*(MAXMESHVERTICES / 2)) - 10)
#define MAXMESHCOMMAND                  4             // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32            // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32            // Max trigs in each command

/* ===== File handling commands ===== */
#define EDITFILE_ACT_NEW     0x01  /* Return empty record  */
#define EDITFILE_ACT_LOAD    0x02  /* Load existing data   */
#define EDITFILE_ACT_SAVE    0x03  /* Save given data      */

/* -- Definiton of directories */
#define EDITFILE_WORKDIR     1  /* Main directory for editor            */
#define EDITFILE_BASICDATDIR 2  /* Basic data for game                  */
#define EDITFILE_GAMEDATDIR  3  /* Gamedata directory in main directory */
#define EDITFILE_OBJECTDIR   4  /* Directory for the objects            */
#define EDITFILE_MODULEDIR   5  /* Directory for the modules            */
#define EDITFILE_EGOBOODIR   6  /* Main directory of game for globals   */
#define EDITFILE_GLOBPARTDIR 7  /* Directory for global particles       */

/*******************************************************************************
* TYPEDEFS 								                                       *
*******************************************************************************/

/* ====== The mesh file ===== */
typedef struct {

    unsigned char ref;      /* Light reference      */
    float u, v;             /* Texture mapping info: Has to be multiplied by 'sub-uv' */    
    float x, y, z;          /* Default position of vertex at 0, 0, 0    */

} EDITOR_VTX;

typedef struct {

    char *name;                             // Name of this fan type
    char default_fx;                        // Default flags to set, if new fan (for walls)
    unsigned char default_tx_no;            // Default texture to set
    char  numvertices;			            // meshcommandnumvertices
    int   count;			                // meshcommands
    int   size[MAXMESHCOMMAND];             // meshcommandsize
    int   vertexno[MAXMESHCOMMANDENTRIES];  // meshcommandvrt, number of vertex
    float uv[MAXMESHVERTICES * 2];          // meshcommandu, meshcommandv
    float biguv[MAXMESHVERTICES * 2];       // meshcommandu, meshcommandv
                                            // For big texture images
    EDITOR_VTX vtx[MAXMESHVERTICES];        // Holds it's u/v position and default extent x/y/z
    
} COMMAND_T;

typedef struct {
  
    unsigned char tx_no;    /* Number of texture:                           */
                            /* (tx_no >> 6) & 3: Number of wall texture     */
                            /* tx_no & 0x3F:     Number of part of texture  */ 
    unsigned char tx_flags; /* Special flags                                */
    unsigned char fx;		/* Tile special effects flags                   */
    char          type;     /* Tile fan type (index into COMMAND_T)         */
    /* --- Additional info for fan                                          */
    unsigned char twist;    /* Surface normal for slopes (?)                */
    int           vs;       /* Number of first vertex for this fan          */
    char          psg_no;   /* > 0: Number of passage for this tile         */
                            /* & 0x80h: Fan is chosen yes/no                */
    int           obj_no;   /* > 0: Number of first object on this fan      */
    
} FANDATA_T;

typedef struct {

    float x, y, z;          /* Vertex x / y / z                     */           
    unsigned char a;        /* Ambient lighting                     */

} MESH_VTX_T;              /* Planned for later adjustement how to store vertices */

typedef struct {

    unsigned char map_loaded;   // A map is loaded  into this struct
    unsigned char draw_mode;    // Flags for display of map
    int mem_size;               // Size of memory allocated (for editor)    
    
    int numvert;                // Number of vertices in map
    int numfreevert;            // Number of free vertices for edit
    int tiles_x, tiles_y;       // Size of mesh in tiles          
    int numfan;                 // Size of map in 'fans' (tiles_x * tiles_y)
    int watershift;             // Depends on size of map
    
    float edgex;                // Borders of mesh
    float edgey;                
    float edgez;                

    COMMAND_T *pcmd;            // Pointer on commands for map    
    FANDATA_T fan[MAXMESHFAN];                      // Fan desription       

    /* Prepare usage of dynamically allocated memory */
    MESH_VTX_T *vrt;            // Pointer on list of map vertices
    char *data;                 // Allcated memory for map in one chunk (filesize)    
    
    float vrtx[MAXTOTALMESHVERTICES + 10];          // Vertex position
    float vrty[MAXTOTALMESHVERTICES + 10];          //
    float vrtz[MAXTOTALMESHVERTICES + 10];          // Vertex elevation
    unsigned char vrta[MAXTOTALMESHVERTICES + 10];  // Vertex base light, 0=unused
    
    int  vrtstart[MAXMESHFAN];                      // First vertex of given fan    
    
} MESH_T;

/* ====== The different file types ===== */
typedef struct
{
    char line_name[25];     /* Only for information purposes "-": Deleted in editor */
    int topleft[2];
    int bottomright[2];
    char open;
    char shoot_trough;
    char slippy_close;
    /* -- Additional info -- */
    int  char_no;           ///< The character associated with this passage
    /* --- Info for editor -- */
    char psg_no;           ///< -1: Deleted
    char rec_no;
    
} EDITFILE_PASSAGE_T;

typedef struct
{
    char obj_name[30 + 1];      /* Name of object to load [obj_name].obj   */
    char item_name[20 + 1];     ///< Never used ???
    int  slot_no;               ///< Not used anymore (the slots are set by loading time) 
    float x_pos, y_pos, z_pos;  
    char view_dir;
    int  money;
    char skin;
    char pas;
    char con;       ///< is the content setting for this character. Used for armor chests. ???
    char lvl;
    char stt;       
    char gho;       ///< is T to make the character a ghost, F for default. Unused
    char team;                  
    /* --- Info for editor -- */
    int  rec_no;
    int  inventory[12];   /* Number of characters attached to this for display  */      
    char *inv_name[12];   /* Object-Name of character in inventory              */
    
} EDITFILE_SPAWNPT_T;     /* Spawn-Point for display on map. From 'spawn.txt'   */

typedef struct
{
    char mod_name[24 + 1];      /* With underscores                                 */
    char ref_mod[24 + 1];       /* Reference module ( Directory name or NONE )      */
    char ref_idsz[18 + 1];      /* Required reference IDSZ ( or [NONE] ) : [MAIN] 6 */
    char number_of_imports;     /* Number of imports ( 0 to 4 ) : 4                 */
    char allow_export;          /* Allow exporting of characters ( TRUE or FALSE )  */
    char min_player;            /* Minimum number of players ( 1 to 4 )             */
    char max_player;            /* Maximum number of players ( 1 to 4 ) : 4         */
    char allow_respawn;         /* Allow respawning ( TRUE or FALSE ) : TRUE        */
    char mod_type[11 + 1];      /* Module Type (MAINQUEST, SIDEQUEST or TOWN)       */
    char lev_rating[8 + 2];     /* Level rating ( *, **, ***, ****, or ***** )      */
    char summary[8][80 + 2];    /* Module summary                                   */
    char exp_idsz[5][18 + 1];   /* Module expansion IDSZs ( with a colon in front ) */
    /* For the editors internal use */
    char mod_type_no;           /* Number of 'mod_type'                             */
    char lev_rating_no;         /* Number of 'lev_rating'                           */
    char dir_name[32];          /* Name of the module directory for loading         */
    
} EDITFILE_MODULE_T;            /* Data from 'menu.txt'                             */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void editfileSetWorkDir(int which, char *dir_name);
char *editfileMakeFileName(int dir_no, char *fname);
int  editfileMapMesh(MESH_T *mesh, char *msg, char save);
int  editfileSpawn(EDITFILE_SPAWNPT_T *spt, char action, int max_rec);
int  editfilePassage(EDITFILE_PASSAGE_T *psg, char action, int max_rec);
int  editfileModuleDesc(EDITFILE_MODULE_T *moddesc, char action);

#endif  /* _EDITFILE_H_ */