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
                                        
/* ======= Maximum-Values for files ======= */
#define EDITFILE_MAXSPAWN    500        /* Maximum Lines in spawn list  */
#define EDITFILE_MAXPASSAGE   50
                                        
/*******************************************************************************
* DATA 								                                           *
*******************************************************************************/

static char EditFileWorkDir[256]   = "module/";
static char EditFileGORDir[128]    = "basicdat/globalobjects/";
static char *EditFileGORSubDir[]   = {
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
static EDITFILE_SPAWNPT_T SpawnTemplate;
static EDITFILE_SPAWNPT_T SpawnObjects[EDITFILE_MAXSPAWN + 2];

static SDLGLCFG_VALUE SpawnVal[] = {
	{ SDLGLCFG_VAL_STRING,  &SpawnObjects[0].line_name, 24 },
	{ SDLGLCFG_VAL_STRING,  &SpawnObjects[0].item_name, 20 },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].slot_no },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].x_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].y_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].z_pos },
	{ SDLGLCFG_VAL_ONECHAR, &SpawnObjects[0].view_dir },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].money },
	{ SDLGLCFG_VAL_CHAR,    &SpawnObjects[0].skin },
	{ SDLGLCFG_VAL_CHAR,    &SpawnObjects[0].pas },
    { SDLGLCFG_VAL_CHAR,    &SpawnObjects[0].con },
    { SDLGLCFG_VAL_CHAR,    &SpawnObjects[0].lvl },
    { SDLGLCFG_VAL_BOOLEAN, &SpawnObjects[0].stt },
    { SDLGLCFG_VAL_BOOLEAN, &SpawnObjects[0].gho },
    { SDLGLCFG_VAL_ONECHAR, &SpawnObjects[0].team },
	{ 0 }
};

static SDLGLCFG_LINEINFO SpawnRec = {
	&SpawnObjects[0],
	EDITFILE_MAXSPAWN,
	sizeof(EDITFILE_SPAWNPT_T),
	&SpawnVal[0]
};

/* ============== Data for passages ============== */
static EDITFILE_PASSAGE_T PassageTemplate;
static EDITFILE_PASSAGE_T Passages[EDITFILE_MAXPASSAGE + 2];

static SDLGLCFG_VALUE PassageVal[] = {
	{ SDLGLCFG_VAL_STRING,  &Passages[0].line_name, 24 },
	{ SDLGLCFG_VAL_INT,     &Passages[0].topleft[0] },
	{ SDLGLCFG_VAL_INT,     &Passages[0].topleft[1] },
	{ SDLGLCFG_VAL_INT,     &Passages[0].bottomright[0] },
	{ SDLGLCFG_VAL_INT,     &Passages[0].bottomright[1] },
	{ SDLGLCFG_VAL_BOOLEAN, &Passages[0].open },
	{ SDLGLCFG_VAL_BOOLEAN, &Passages[0].shoot_trough },
	{ SDLGLCFG_VAL_BOOLEAN, &Passages[0].slippy_close },
	{ 0 }
};

static SDLGLCFG_LINEINFO PassageRec = {
	&Passages[0],
	EDITFILE_MAXPASSAGE,
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
        mesh -> twist[fan] = editfileGetFanTwist(mesh, fan);
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
        fread(&mesh -> fan[0], sizeof(FANDATA_T), numfan, fileread);
        /* TODO: Check if enough memory is available for loading map */
        /* Load normal data */
        fread(&mesh -> twist[0], 1, numfan, fileread);

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
            numwritten += fwrite(&mesh -> fan[0], sizeof(FANDATA_T), mesh -> numfan, filewrite);
            /* Write twist data */
            numwritten += fwrite(&mesh -> twist[0], 1, mesh -> numfan, filewrite);

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
 *     Loads the map mesh into the data pointed on
 * Input:
 *     mesh*: Pointer on mesh to load
 *     msg *: Pointer on buffer for a message
 * Output:
 *     Mesh could be loaded yes/no
 */
void editfileSetWorkDir(char *dir_name)
{
    
    /* TODO: Check it, replace possible backslashes '\' with slashes forward */
    sprintf(EditFileWorkDir, "%s/", dir_name);

}

/*
 * Name:
 *     editfileMakeFileName
 * Description:
 *     Generates a filename with path.
 *     Path for objects is taken from the list of the GOR, if the
 *     object is not found in the modules directory
 * Input:
 *     dir_no:  Which directory to use for filename
 *     fname *: Name of file to create filename including path
 */
char *editfileMakeFileName(int dir_no, char *fname)
{

    static char file_name[512];
    
    FILE *f;  
    int i;


    file_name[0] = 0;

    switch(dir_no) {

        case EDITFILE_GAMEDATDIR:
            sprintf(file_name, "%sgamedat/%s", EditFileWorkDir, fname);
            break;
            
        case EDITFILE_BASICDATDIR:
            sprintf(file_name, "basicdat/%s", fname);
            break;

        case EDITFILE_OBJECTDIR:
            sprintf(file_name, "%sobjects/%s", EditFileWorkDir, fname);
            f = fopen(file_name, "r");
            if (f) {
                /* Directory found for this object */
                fclose(f);
            }
            else {
                /* Look for objects in the GOR */
                i = 0;
                while(EditFileGORSubDir[i]) {
                 
                    sprintf(file_name, "%s%s/%s", EditFileGORDir, EditFileGORSubDir[i], fname);
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

        case EDITFILE_WORKDIR:
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
 *     action: What to do
 *     rec_no: Number of record to read/write from buffer
 *     spt *:  Pointer on buffer for copy of chosen spawn point
 * Output:
 *     Number of record chosen    
 */
int editfileSpawn(int action, int rec_no, EDITFILE_SPAWNPT_T *spt)
{

    char *fname;
    int new_rec_no;


    fname = editfileMakeFileName(EDITFILE_GAMEDATDIR, "spawn.txt");

    switch(action) {
    
        case EDITFILE_ACT_LOAD:
            /* ------- Load the data from file ---- */
            sdlglcfgEgobooRecord(fname, &SpawnRec, 0);
            /* ----- Return the data from the first record --- */
            memcpy(spt, &SpawnObjects[1], sizeof(EDITFILE_SPAWNPT_T));
            rec_no = 1;
            break;
            
        case EDITFILE_ACT_SAVE:
            /* -------- Write data to file -------- */
            sdlglcfgEgobooRecord(fname, &SpawnRec, 1);
            return 1;

        case EDITFILE_ACT_GETDATA:  /* In given buffer      */
            if (SpawnObjects[rec_no].line_name[0] > 0) {
                memcpy(spt, &SpawnObjects[rec_no], sizeof(EDITFILE_SPAWNPT_T));
            }
            else {
                /* No data found at this record position */
                return 0;
            }
            break;

        case EDITFILE_ACT_SETDATA:
            if (rec_no > 0 && rec_no < EDITFILE_MAXSPAWN) {
                memcpy(&SpawnObjects[rec_no], spt, sizeof(EDITFILE_SPAWNPT_T));
            }
            break;

        case EDITFILE_ACT_NEW:
            new_rec_no = 1;
            while(Passages[rec_no].line_name[0] > 0) {
                new_rec_no++;
                if (new_rec_no >= EDITFILE_MAXPASSAGE) {
                    /* No space left for new passage */
                    return 0;
                }
            }
            rec_no = new_rec_no;
            break;

    }

    return rec_no;

}

/*
 * Name:
 *     editfilePassage
 * Description:
 *     Load/Save passages data from 'passage.txt' in actual work directory
 * Input:
 *     action: What to do
 *     rec_no: Number of record to read/write from buffer
 *     psg *:  Pointer where to return copy of passage asked for
 * Output:
 *    Number of record chosen
 */
int  editfilePassage(int action, int rec_no, EDITFILE_PASSAGE_T *psg)
{

    char *fname;
    int new_rec_no;


    fname = editfileMakeFileName(EDITFILE_GAMEDATDIR, "passage.txt");

    switch(action) {

        case EDITFILE_ACT_LOAD:
            sdlglcfgEgobooRecord(fname, &PassageRec, 0);
            /* ----- Return the data from the first record --- */
            memcpy(psg, &Passages[rec_no], sizeof(EDITFILE_PASSAGE_T));
            rec_no = 1;
            break;
            
        case EDITFILE_ACT_SAVE:
            /* -------- Write data to file -------- */
            sdlglcfgEgobooRecord(fname, &PassageRec, 1);
            return 1;

        case EDITFILE_ACT_GETDATA:  /* In given buffer      */
            if (Passages[rec_no].line_name[0] > 0) {
                memcpy(psg, &Passages[rec_no], sizeof(EDITFILE_PASSAGE_T));
            }
            else {
                /* No data found at this record position */
                return 0;
            }
            break;
            
        case EDITFILE_ACT_SETDATA:
            if (rec_no > 0 && rec_no < EDITFILE_MAXPASSAGE) {
                memcpy(&Passages[rec_no], psg, sizeof(EDITFILE_PASSAGE_T));
            }
            break;
            
        case EDITFILE_ACT_NEW:
            new_rec_no = 1;
            while(Passages[rec_no].line_name[0] > 0) {
                new_rec_no++;
                if (new_rec_no >= EDITFILE_MAXPASSAGE) {
                    /* No space left for new passage */
                    return 0;
                }
            }
            rec_no = new_rec_no;
            break;

    }

    return rec_no;

}

/*
 * Name:
 *     editfileModuleDesc
 * Description:
 *     Load/Save the data from 'menu.txt' in actual work directory
 * Input:
 *     action:    What to do
 *     moddesc *: Pointer where to get / return a copy of a module
 * Output:
 *    Number of record chosen
 */
int  editfileModuleDesc(int action, EDITFILE_MODULE_T *moddesc)
{

    char *fname;
    int i;


    fname = editfileMakeFileName(EDITFILE_GAMEDATDIR, "menu.txt");

    switch(action) {

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
            
        case EDITFILE_ACT_NEW:
            /* Return an module descriptor filled with default values */
            memcpy(moddesc, &ModDescTemplate, sizeof(EDITFILE_MODULE_T));
            break;            

    }

    return 1;

}
