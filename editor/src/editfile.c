/*******************************************************************************
*  EDITFILE.C                                                                  *
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

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include <stdio.h>
#include <memory.h>

#include "sdlglcfg.h"   /* Read egoboo text files eg. passage, spawn    */

#include "editfile.h"     /* Own header */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITFILE_SLOPE      50          /* Twist stuff          */
#define MAPID               0x4470614d  /*  The string... MapD  */
#define EDITFILE_ZADJUST    16.0        /* Read/Write Z-Value   */
                                        /* Cartman uses 4 bit fixed point for Z */
                    
/*******************************************************************************
* DATA 								                                           *
*******************************************************************************/

// Directory where Egoboo resides
static char EditFileGameDir[128]   = "c:/egoboo/";
// Directory name of actual game module
static char EditFileModuleDir[64]   = "test.mod";
// Where the editor works in the Egoboo-Directory
static char EditFileWorkDir[256]   = "c:/egoboo/editor/test.mod/";
/* Directories to read from in Egoboo-Main-Directory */
// static char EditFileMenuDir[]      = "basicdat/menu/";
// static char EditFileMusicDir[]     = "basicdat/music/";
static char *EditFileGORDir[]   = {
    "armor",
    "items",
    "magic"
    "magic_item",
    "misc",
    "monsters",
    "pets",
    "players",
    "potions",
    "scrolls",
    "traps",
    "unique",
    "weapons"
    ""
};

/* ============== Data for Spawn-Points ============== */
/* static EDITFILE_SPAWNPT_T SpawnTemplate; */
static EDITFILE_SPAWNPT_T SpawnPt;

static SDLGLCFG_VALUE SpawnVal[] = {
	{ SDLGLCFG_VAL_STRING,  SpawnPt.obj_name, 30 },
	{ SDLGLCFG_VAL_STRING,  SpawnPt.item_name, 20 },
	{ SDLGLCFG_VAL_INT,     &SpawnPt.slot_no },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnPt.x_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnPt.y_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnPt.z_pos },
	{ SDLGLCFG_VAL_ONECHAR, &SpawnPt.view_dir },
	{ SDLGLCFG_VAL_INT,     &SpawnPt.money },
	{ SDLGLCFG_VAL_CHAR,    &SpawnPt.skin },
	{ SDLGLCFG_VAL_CHAR,    &SpawnPt.pas },
    { SDLGLCFG_VAL_CHAR,    &SpawnPt.con },
    { SDLGLCFG_VAL_CHAR,    &SpawnPt.lvl },
    { SDLGLCFG_VAL_BOOLEAN, &SpawnPt.stt },
    { SDLGLCFG_VAL_BOOLEAN, &SpawnPt.gho },
    { SDLGLCFG_VAL_ONECHAR, &SpawnPt.team },
	{ 0 }
};

static SDLGLCFG_LINEINFO SpawnRec = {
    &SpawnPt,
	&SpawnPt,
	0,
	sizeof(EDITFILE_SPAWNPT_T),
	&SpawnVal[0]
};

/* ============== Data for passages ============== */
/* static EDITFILE_PASSAGE_T PassageTemplate; */
static EDITFILE_PASSAGE_T Psg;

static SDLGLCFG_VALUE PassageVal[] = {
	{ SDLGLCFG_VAL_STRING,  Psg.line_name, 24 },
	{ SDLGLCFG_VAL_INT,     &Psg.topleft[0] },
	{ SDLGLCFG_VAL_INT,     &Psg.topleft[1] },
	{ SDLGLCFG_VAL_INT,     &Psg.bottomright[0] },
	{ SDLGLCFG_VAL_INT,     &Psg.bottomright[1] },
	{ SDLGLCFG_VAL_BOOLEAN, &Psg.open },
	{ SDLGLCFG_VAL_BOOLEAN, &Psg.shoot_trough },
	{ SDLGLCFG_VAL_BOOLEAN, &Psg.slippy_close },
	{ 0 }
};

static SDLGLCFG_LINEINFO PassageRec = {
    &Psg,
	&Psg,
	0,
	sizeof(EDITFILE_PASSAGE_T),
	&PassageVal[0]
};

/* ============== Data for ModDesc-Files ============== */
static EDITFILE_MODULE_T ModDescTemplate = {
    "", "NONE", "[NONE]", 0, 0, 1, 1, 0, "MAINQUEST",
    "*"
};
static EDITFILE_MODULE_T ModDesc;

static SDLGLCFG_NAMEDVALUE ModuleVal[] = {
    { SDLGLCFG_VAL_STRING,  ModDesc.mod_name, 24, "Module Name" }, 
    { SDLGLCFG_VAL_STRING,  ModDesc.ref_mod, 24, "Reference Directory" },  
    { SDLGLCFG_VAL_STRING,  ModDesc.ref_idsz, 11, "Required reference IDSZ" }, 
    { SDLGLCFG_VAL_CHAR,    &ModDesc.number_of_imports, 1, "Number of imports ( 0 to 4 )" }, 
    { SDLGLCFG_VAL_BOOLEAN, &ModDesc.allow_export, 1, "Exporting ( TRUE or FALSE )"  },
    { SDLGLCFG_VAL_CHAR,    &ModDesc.min_player, 1, "Minimum players ( 1 to 4 )" },  
    { SDLGLCFG_VAL_CHAR,    &ModDesc.max_player, 1, "Maximum players ( 1 to 4 )"  },  
    { SDLGLCFG_VAL_BOOLEAN, &ModDesc.allow_respawn, 1, "Respawning ( TRUE or FALSE )" }, 
    { SDLGLCFG_VAL_STRING,  ModDesc.mod_type, 11, "Module Type (MAINQUEST, SIDEQUEST or TOWN)" },  
    { SDLGLCFG_VAL_STRING,  &ModDesc.lev_rating, 8, "Level rating ( * to  ***** )" },
    { SDLGLCFG_VAL_LABEL, 0, 0, "//Module summary\n" }, /* Module summary */
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[0], 40 },   
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[1], 40 },
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[2], 40 },
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[3], 40 },
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[4], 40 },
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[5], 40 },
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[6], 40 },
    { SDLGLCFG_VAL_STRING,  ModDesc.summary[7], 40 },
    { SDLGLCFG_VAL_LABEL, 0, 0, "// Module expansion IDSZs ( with a colon in front )\n" }, 
    { SDLGLCFG_VAL_STRING, ModDesc.exp_idsz[0], 18 }, /*  */
    { SDLGLCFG_VAL_STRING, ModDesc.exp_idsz[1], 18 }, 
    { SDLGLCFG_VAL_STRING, ModDesc.exp_idsz[2], 18 },
    { SDLGLCFG_VAL_STRING, ModDesc.exp_idsz[3], 18 },
    { SDLGLCFG_VAL_STRING, ModDesc.exp_idsz[4], 18 },
    { 0 }
};

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 * Name:
 *     editfileMakeTwist
 * Description:
 *     Generates the number of the 'up'-vector for given fan
 * Input:
 *     mesh*: Pointer on mesh to set twists for
 */
static unsigned char editfileGetFanTwist(MESH_T *mesh, int fan)
{

    int zx, zy, vt0, vt1, vt2, vt3;
    unsigned char twist;


    vt0 = mesh -> vrtstart[fan];
    vt1 = vt0 + 1;
    vt2 = vt0 + 2;
    vt3 = vt0 + 3;

    zx = (mesh-> vrtz[vt0] + mesh -> vrtz[vt3] - mesh -> vrtz[vt1] - mesh -> vrtz[vt2]) / EDITFILE_SLOPE;
    zy = (mesh-> vrtz[vt2] + mesh -> vrtz[vt3] - mesh -> vrtz[vt0] - mesh -> vrtz[vt1]) / EDITFILE_SLOPE;

    /* x and y should be from -7 to 8   */
    if (zx < -7) zx = -7;
    if (zx > 8)  zx = 8;
    if (zy < -7) zy = -7;
    if (zy > 8)  zy = 8;

    /* Now between 0 and 15 */
    zx = zx + 7;
    zy = zy + 7;
    twist = (unsigned char)((zy << 4) + zx);

    return twist;

}

/*
 * Name:
 *     editfileMakeTwist
 * Description:
 *     Generates the number of the 'up'-vector for all fans
 * Input:
 *     mesh*: Pointer on mesh to set twists for
 */
static void editfileMakeTwist(MESH_T *mesh)
{

    int fan;


    fan = 0;
    while (fan < mesh -> numfan)
    {
        mesh -> fan[fan].twist = editfileGetFanTwist(mesh, fan);
        fan++;
    }

}

/*
 * Name:
 *     editfileLoadMapMesh
 * Description:
 *     Loads the map mesh into the data pointed on
 * Input:
 *     mesh*: Pointer on mesh to load
 *     msg *: Pointer on buffer for a message 
 * Output:
 *     Mesh could be loaded yes/no
 */
static int editfileLoadMapMesh(MESH_T *mesh, char *msg)
{

    FILE* fileread;
    unsigned int uiTmp32;
    int cnt;
    float ftmp;
    int numfan;


    fileread = fopen(editfileMakeFileName(EDITFILE_GAMEDATDIR, "level.mpd"), "rb");

    if (NULL == fileread)
    {
        sprintf(msg, "%s", "Map file not found.");
        return 0;
    }

    if (fileread)
    {

        fread( &uiTmp32, 4, 1, fileread );
        if ( uiTmp32 != MAPID )
        {
            sprintf(msg, "%s", "Map file has invalid Map-ID.");
            return 0;

        }

        fread(&mesh -> numvert, 4, 1, fileread);
        fread(&mesh -> tiles_x, 4, 1, fileread);
        fread(&mesh -> tiles_y, 4, 1, fileread);

        numfan = mesh -> tiles_x * mesh -> tiles_y;
        
        /* Load fan data    */
        for (cnt = 0; cnt < numfan; cnt++) {
            fread(&mesh -> fan[cnt], 4, 1, fileread);
        }
        /* TODO: Check if enough memory is available for loading map */
        /* Load normal data */
        for (cnt = 0; cnt < numfan; cnt++) {
            fread(&mesh -> fan[cnt].twist, 1, 1, fileread);
        }

        /* Load vertex x data   */
        for (cnt = 0; cnt < mesh -> numvert; cnt++) {
            fread(&ftmp, 4, 1, fileread);
            mesh -> vrtx[cnt] = ftmp;
        }

        /* Load vertex y data   */
        for (cnt = 0; cnt < mesh -> numvert; cnt++) {
            fread(&ftmp, 4, 1, fileread);
            mesh -> vrty[cnt] = ftmp;
        }

        /* Load vertex z data   */
        for (cnt = 0; cnt < mesh -> numvert; cnt++) {
            fread(&ftmp, 4, 1, fileread);
            mesh -> vrtz[cnt] = ftmp / EDITFILE_ZADJUST;  /* Z is fixpoint int in cartman (16)*/
        }

        fread(&mesh -> vrta[0], 1, mesh -> numvert, fileread);   // !!!BAD!!!

        fclose(fileread);

        return 1;

    }

    return 0;

} 

/*
 * Name:
 *     editfileSaveMapMesh
 * Description:
 *     Saves the mesh in the data pointed on.
 *     Name of the save file is always 'level.mpd'
 * Input:
 *     mesh* : Pointer on mesh to save
 * Output:
 *     Save worked yes/no
 */
static int editfileSaveMapMesh(MESH_T *mesh, char *msg)
{

    FILE* filewrite;
    int itmp;
    float ftmp;
    int cnt;
    int numwritten;


    if (mesh -> map_loaded) {

        /* Generate the 'up'-vector-numbers */
        editfileMakeTwist(mesh);

        numwritten = 0;

        filewrite = fopen(editfileMakeFileName(EDITFILE_GAMEDATDIR, "level.mpd"), "wb");
        if (filewrite) {

            itmp = MAPID;

            numwritten += fwrite(&itmp, 4, 1, filewrite);
            numwritten += fwrite(&mesh -> numvert, 4, 1, filewrite);
            numwritten += fwrite(&mesh -> tiles_x, 4, 1, filewrite);
            numwritten += fwrite(&mesh -> tiles_y, 4, 1, filewrite);

            /* Write tiles data    */
            for (cnt = 0; cnt < mesh -> numfan; cnt++) {
                fwrite(&mesh -> fan[cnt], 4, 1, filewrite);
            }
            
            /* Write twist data */
            for (cnt = 0; cnt < mesh -> numfan; cnt++) {
                numwritten += fwrite(&mesh -> fan[cnt].twist, 1, 1, filewrite);
            }

            /* Write x-vertices */
            for (cnt = 0; cnt < mesh -> numvert; cnt++) {
                /* Change int to floatfor game */
                ftmp = mesh -> vrtx[cnt];
                numwritten += fwrite(&ftmp, 4, 1, filewrite);

            }

            /* Write y-vertices */
            for (cnt = 0; cnt < mesh -> numvert; cnt++) {
                /* Change int to float for game */
                ftmp = mesh -> vrty[cnt];
                numwritten += fwrite(&ftmp, 4, 1, filewrite);

            }

            /* Write z-vertices */
            for (cnt = 0; cnt < mesh -> numvert; cnt++) {
                /* Change int to float for game */
                ftmp = (mesh -> vrtz[cnt] * EDITFILE_ZADJUST);   /* Multiply it again to file format */
                numwritten += fwrite(&ftmp, 4, 1, filewrite);

            }

            /* Write the light values */
            fwrite(&mesh -> vrta[0], 1, mesh -> numvert, filewrite);   // !!!BAD!!!

            return numwritten;

        }

    }

    sprintf(msg, "%s", "Saving map has failed.");

	return 0;

}

/*
 * Name:
 *     editfileSetUnderlines
 * Description:
 *     Replaces spaces with underlines in given string
 * Input:
 *     text *: To replace spaces by underlines
 */
static void editfileSetUnderlines(char *text)
{

    while(*text) {

        if (*text == ' ') {

            *text = '_';

        }

        text++;

    }

}

/* ========================================================================== */
/* ============================= THE PUBLIC ROUTINES ======================== */
/* ========================================================================== */

/*
 * Name:
 *     editfileSetWorkDir
 * Description:
 *     Sets the work directory for the file functions
 * Input:
 *     which:      Which directory to set
 *     dir_name *: Direcory-Name to set (If EDITFILE_MODULEDIR only module name)
 * Output:
 *     Mesh could be loaded yes/no
 */
void editfileSetWorkDir(int which, char *dir_name)
{


    /* TODO: Check it, replace possible backslashes '\' with slashes forward    */
    /* TODO: Check if there's already a slash attached to the given 'dir_name'  */
    switch(which)
    {
        case EDITFILE_EGOBOODIR:
            sprintf(EditFileGameDir, "%s/", dir_name);
            break;

        case EDITFILE_WORKDIR:
            sprintf(EditFileWorkDir, "%s/", dir_name);
            break;

        case EDITFILE_MODULEDIR:
            sprintf(EditFileModuleDir, "%s/", dir_name);
            break;
    }

}

/*
 * Name:
 *     editfileMakeFileName
 * Description:
 *     Generates a filename with path.
 *     EDITFILE_OBJECTDIR:  
 *      Path for objects is taken from the list of the GOR, if the
 *      object is not found in the modules directory
 *      Looks for the file 'data.txt' to check if the object exists at all 
 * Input:
 *     dir_no:  Which directory to use for filename
 *     fname *: Name of file to create filename including path / name of object
 * Output:
 *     Valid Filename with complete path except EDITFILE_OBJECTDIR: Only path without file name  
 */
char *editfileMakeFileName(int dir_no, char *fname)
{

    static char file_name[512];     // Here the filename is returned with path, except the objects directory
    
    char obj_file_name[512];        // The objects directory with file name
    FILE *f;  
    int i;


    file_name[0] = 0;

    switch(dir_no) {

        case EDITFILE_GAMEDATDIR:
            sprintf(file_name, "%sgamedat/%s", EditFileWorkDir, fname);
            break;

        case EDITFILE_BASICDATDIR:
            sprintf(file_name, "%sbasicdat/%s", fname);
            break;

        case EDITFILE_OBJECTDIR:
            // @todo: Look up basic data in game directory, if game-level is loaded for browsing
            // Look up "data.txt" and return the directory without file name, because filename is the objects name
            // In this case only return the directory without file name for multiple use
            sprintf(file_name, "%sobjects/%s.obj/", EditFileWorkDir, fname);
            sprintf(obj_file_name, "%sdata.txt", file_name);
            
            f = fopen(obj_file_name, "r");
            if (f) {
                /* Directory found for this object */
                fclose(f);
            }
            else {
                /* Look for objects in the GOR */
                i = 0;
                while(EditFileGORDir[i]) {

                    sprintf(file_name, "%sbasicdat/globalobjects/%s/%s.obj/", EditFileGameDir, EditFileGORDir[i], fname);
                    sprintf(obj_file_name, "%sdata.txt", file_name);
                    
                    f = fopen(file_name, "r");
                    if (f) {
                        /* Directory found for this object */
                        fclose(f);
                        return file_name;
                    }

                    i++;
                }

                file_name[0] = 0;
            }
            break;

        case EDITFILE_MODULEDIR:
            sprintf(file_name, "%s%s%s", EditFileGameDir, EditFileModuleDir, fname);
            break;

        case EDITFILE_EGOBOODIR:
            sprintf(file_name, "%s%s", EditFileGameDir, fname);
            break;

        case EDITFILE_GLOBPARTDIR:
            sprintf(file_name, "%sbasicdat/globalparticles/%s", EditFileGameDir, fname);
            break;

        default:
            sprintf(file_name, "%s%s", EditFileWorkDir, fname);
    }

    return file_name;

}

/*
 * Name:
 *     editfileMesh
 * Description:
 *     Load/Save an egoboo mesh
 * Input:
 *     mesh *: Pointer on mesh to save
 *     msg *:  About what failed if load/save failed
 *     save:   Save it yes/no  
 * Output:
 *     Success yes/no
 */
int  editfileMapMesh(MESH_T *mesh, char *msg, char save)
{

    if (save) {
        return editfileSaveMapMesh(mesh, msg);
    }
    else {
        return editfileLoadMapMesh(mesh, msg);
    }

}

/*
 * Name:
 *     editfileSpawn
 * Description:
 *     Load/Save spawn data from 'spawn.txt' in actual work directory
 * Input:
 *     spt *:   Where to load the spawnpoints to
 *     action:  What to do 
 *     max_rec: Maximum number of records in given buffer
 * Output:
 *     Success yes/no   
 */
int editfileSpawn(EDITFILE_SPAWNPT_T *spt, char action, int max_rec)
{

    char *fname;


    fname = editfileMakeFileName(EDITFILE_GAMEDATDIR, "spawn.txt");
    
    SpawnRec.recbuf = spt;
    SpawnRec.maxrec = max_rec;

    switch(action) {
    
        case EDITFILE_ACT_LOAD:
            /* ------- Load the data from file ---- */
            sdlglcfgEgobooRecord(fname, &SpawnRec, 0);            
            break;
            
        case EDITFILE_ACT_SAVE:
            /* -------- Write data to file -------- */
            sdlglcfgEgobooRecord(fname, &SpawnRec, 1);
            return 1;

    }

    return 1;

}

/*
 * Name:
 *     editfilePassage
 * Description:
 *     Load/Save passages data from 'passage.txt' in actual work directory
 * Input:
 *     psg *:   Pointer where to return the data of all passages
 *     action:  What to do
 *     max_rec: MAximum number of records in given passage-buffer 
 *     rec_no: Number of record to read/write from buffer
 * Output:
 *    Success yes/no
 */
int  editfilePassage(EDITFILE_PASSAGE_T *psg, char action, int max_rec)
{

    char *fname;


    fname = editfileMakeFileName(EDITFILE_GAMEDATDIR, "passage.txt");
    
    PassageRec.recbuf = psg;
    PassageRec.maxrec = max_rec;

    switch(action) {
            
        case EDITFILE_ACT_LOAD:
            
            sdlglcfgEgobooRecord(fname, &PassageRec, 0);
            break;

        case EDITFILE_ACT_SAVE:
            /* -------- Write data to file -------- */
            sdlglcfgEgobooRecord(fname, &PassageRec, 1);

    }

    return 1;

}

/*
 * Name:
 *     editfileModuleDesc
 * Description:
 *     Load/Save the data from 'menu.txt' in actual work directory
 * Input:
 *     moddesc *: Pointer where to get / return a copy of a module descriptor
 *     action:    Which action to execute 
 * Output:
 *     Success yes/no
 */
int  editfileModuleDesc(EDITFILE_MODULE_T *moddesc, char action)
{
    
    FILE *f;
    char *fname;
    int i;
    EDITFILE_MODULE_T *src;


    fname = editfileMakeFileName(EDITFILE_GAMEDATDIR, "menu.txt");

    switch(action) {
    
        case EDITFILE_ACT_NEW:
            /* Return an module descriptor filled with default values   */
            /* ... if it doesn't exist yet                              */
            f = fopen(fname, "rt");
            if (f) {
                fclose(f);
                /* File already exists, load it */
                sdlglcfgEgobooValues(fname, ModuleVal, 0);
                src = &ModDesc;                
            }
            else {
                src =  &ModDescTemplate;
            }
            memcpy(moddesc, src, sizeof(EDITFILE_MODULE_T));
            break;  

        case EDITFILE_ACT_LOAD:
            sdlglcfgEgobooValues(fname, ModuleVal, 0);
            /* ----- Return the data from internal record --- */
            memcpy(moddesc, &ModDesc, sizeof(EDITFILE_MODULE_T));
            break;
            
        case EDITFILE_ACT_SAVE:
            /* -------- Write data to file -------- */
            memcpy(&ModDesc, moddesc, sizeof(EDITFILE_MODULE_T));
            /* ----- Now insert Underlines and so on for egoboo ---- */
            editfileSetUnderlines(ModDesc.mod_name);
            for(i = 0; i < 8; i++) {

                if (ModDesc.summary[i][0] == 0) {

                    ModDesc.summary[i][0] = '_';
                    ModDesc.summary[i][1] = 0;

                }
                else {

                    editfileSetUnderlines(ModDesc.summary[i]);

                }
                
            }
            sdlglcfgEgobooValues(fname, ModuleVal, 1);
            break;                 

    }

    return 1;

}
