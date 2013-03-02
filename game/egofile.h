/*******************************************************************************
*  EGOFILE.H                                                                  *
*	- Load and write files for egoboo                                         *
*									                                       *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
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

#ifndef _EGOFILE_H_
#define _EGOFILE_H_

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

/* ===== define sthe map mesh ===== */
#define MESH_MAXTYPE        64          // Number of mesh types
#define MESH_MAXLINE        64          // Number of lines in a fan schematic
#define MESH_MAXVERTICES    16          // Max number of vertices in a fan
#define MESH_MAXFAN         (128*128)   // Size of map in fans
#define MESH_MAXTOTALVRT    ((MESH_MAXFAN*(MESH_MAXVERTICES / 2)) - 10)
#define MESH_MAXCMD          4          // Draw up to 4 fans
#define MESH_MAXCMDENTRIES  32          // Fansquare command list size
#define MESH_MAXCMDSIZE     32          // Max trigs in each command

/* ===== File handling commands ===== */
#define EGOFILE_ACT_NEW     0x01  /* Return empty record  */
#define EGOFILE_ACT_LOAD    0x02  /* Load existing data   */
#define EGOFILE_ACT_SAVE    0x03  /* Save given data      */

/* -- Definiton of directories */
#define EGOFILE_WORKDIR     1  /* Main directory for editor            */
#define EGOFILE_BASICDATDIR 2  /* Basic data for game                  */
#define EGOFILE_GAMEDATDIR  3  /* Gamedata directory in main directory */
#define EGOFILE_MODULEDIR   4  /* Directory for the modules            */
#define EGOFILE_ACTOBJDIR   5  /* Directory for actual chosen object   */
#define EGOFILE_EGOBOODIR   6  /* Main directory of game for globals   */
#define EGOFILE_GLOBPARTDIR 7  /* Directory for global particles       */
#define EGOFILE_SAVEGAMEDIR 8  // Directory for savegames

/*******************************************************************************
* TYPEDEFS 								                                   *
*******************************************************************************/

/* ====== The mesh file ===== */
typedef struct
{
    unsigned char ref;      /* Light reference      */
    float u, v;             /* Texture mapping info: Has to be multiplied by 'sub-uv' */    
    float x, y, z;          /* Default position of vertex at 0, 0, 0    */

} EDITOR_VTX;

typedef struct
{
    char *name;                             // Name of this fan type
    char default_fx;                        // Default flags to set, if new fan (for walls)
    unsigned char default_tx_no;            // Default texture to set
    char  numvertices;			            // meshcommandnumvertices
    int   count;			                // meshcommands
    int   size[MESH_MAXCMD];             // meshcommandsize
    int   vertexno[MESH_MAXCMDENTRIES];  // meshcommandvrt, number of vertex
    float uv[MESH_MAXVERTICES * 2];          // meshcommandu, meshcommandv
    float biguv[MESH_MAXVERTICES * 2];       // meshcommandu, meshcommandv
                                            // For big texture images
    EDITOR_VTX vtx[MESH_MAXVERTICES];        // Holds it's u/v position and default extent x/y/z
    
} COMMAND_T;

typedef struct
{
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
    unsigned char fxg;		/* Tile special effects flags                   */
    
} FANDATA_T;

typedef struct
{
    float v[3];             /* Vertex x / y / z                     */           
    unsigned char a;        /* Ambient lighting                     */

} MESH_VTX_T;              

typedef struct
{
    unsigned char map_loaded;   // A map is loaded  into this struct
    unsigned char draw_mode;    // Flags for display of map
    int flags;                  // All purpose flags for game (script)
    int mem_size;               // Size of memory allocated (for editor)    
    
    int numvert;                // Number of vertices in map
    int numfreevert;            // Number of free vertices for edit
    int tiles_x, tiles_y;       // Size of mesh in tiles          
    int numfan;                 // Size of map in 'fans' (tiles_x * tiles_y)
    
    float edgex;                // Borders of mesh
    float edgey;                
    float edgez;                

    COMMAND_T  *pcmd;                       // Pointer on commands for map    
    FANDATA_T  fan[MESH_MAXFAN];            // Fan desription       
    int   vrtstart[MESH_MAXFAN];            // First vertex of given fan
    MESH_VTX_T vrt[MESH_MAXTOTALVRT + 10];  // Vertex position and lighting
                                            // Vertex base light, 0=unused
    
    /* Prepare usage of dynamically allocated memory */
    MESH_VTX_T *dyn_vrt;        // Pointer on list of map vertices
    char *data;                 // Allcated memory for map in one chunk (filesize)    
    
} MESH_T;

/* ====== The different file types ===== */
typedef struct
{
    char line_name[25];     /* Only for information purposes "-": Deleted in editor */
    int  topleft[2];
    int  bottomright[2];
    char open;
    char shoot_trough;      // @todo: Set map flags for 'allow passage small'
    char slippy_close;
    /* -- Additional info, for game -- */
    int  char_no;           ///< The character associated with this passage
    char state;             ///< State of passage in game -1: isopening / 0: inactive / 1: isclosing
    char ishop;             ///< This passage is a shop
    /* --- Info for editor -- */
    char psg_no;            ///< -1: Deleted
    char rec_no;            
    
} EGOFILE_PASSAGE_T;

typedef struct
{
    char obj_name[24 + 1];      /* Name of object to load [obj_name].obj   */
    char item_name[24 + 1];     ///< Never used ???
    int  slot_no;               ///< Not used anymore (the slots are set by loading time) 
    float pos[3];               /// x, y, z
    char view_dir;
    int  money;
    char skin;
    char pas;
    char con;       ///< is the content setting for this character. Used for armor chests. ???
    char lvl;
    char stt;       
    char gho;       ///< is T to make the character a ghost, F for default. Unused
    char team;                  
    // Info for edit and saving a character
    int  rec_no;
    int  inventory[12];   /* Number of characters attached to this for display  */      
    char *inv_name[12];   /* Object-Name of character in inventory              */
    
} EGOFILE_SPAWNPT_T;     /* Spawn-Point for display on map. From 'spawn.txt'   */

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
    // @todo: Use 'idsz
    char exp_idsz[5][18 + 1];   /* Module expansion IDSZs ( with a colon in front ) */
    /* For the editors internal use */
    int  seed;
    char mod_type_no;           /* Number of 'mod_type'                             */
    char lev_rating_no;         /* Number of 'lev_rating'                           */
    char dir_name[32];          /* Name of the module directory for loading         */
    
} EGOFILE_MODULE_T;            /* Data from 'menu.txt'                             */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

char egofileSetDir(int which, char *dir_name);
char *egofileMakeFileName(int dir_no, char *fname);
int  egofileMapMesh(MESH_T *mesh, char *msg, char save);
int  egofileSpawn(EGOFILE_SPAWNPT_T *spt, char action, int max_rec);
int  egofilePassage(EGOFILE_PASSAGE_T *psg, char action, int max_rec);
int  egofileModuleDesc(EGOFILE_MODULE_T *moddesc, char action);

#endif  /* _EGOFILE_H_ */