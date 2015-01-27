/*******************************************************************************
*  EGOMODEL.C                                                                  *
*	- Load and display for Quake 2 Models      		                           *
*									                                           *
*   SDLGL - Library                                                            *
*   Copyright (c) 2000, by Mustata Bogdan (LoneRunner)                         *
*   Adjusted for use with SDLGL Paul Mueller <bitnapper>                       *
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
* INCLUDES                                                                     *
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <alloc.h>      /* malloc() */

#include "sdlgl.h"
#include "sdlgltex.h"
#include "egofile.h"    // Create filename using actual set directory for object

// Own header
#include "egomodel.h"       /* Own header   */

/* List of standard actions for MD2-Models:
    FRAME       NAME
    0- 9      "STAND"
    40-45     "RUN"
    46-53     "ATTACK"
    54-57     "PAIN1"
    58-61     "PAIN2"
    62-65     "PAIN3"
    66-71     "JUMP"
    72-83     "FLIP"
    84-94     "SALUTE"
    95-111    "TAUNT"
    112-122   "WAVE"
    123-134   "POINT"
    135-153   "CRSTAND"
    154-159   "CRWALK"
    160-168   "CRATTACK"
    169-172   "CRPAIN"
    173-177   "CRDEATH"
    178-183   "DEATH1"
    184-189   "DEATH2"
    190-197   "DEATH3"
*/

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

#define MD2_MAX_TRIANGLES		4096
#define MD2_MAX_VERTICES		2048
#define MD2_MAX_TEXCOORDS		2048
#define MD2_MAX_FRAMES			512
#define MD2_MAX_ANIMATION       80
#define MD2_MAX_SKINS			8
#define MD2_MAX_FRAMESIZE		(MD2_MAX_VERTICES * 4 + 128)

/* --------- Own stuff ---- */
#define MD2_MAX_ICONS MD2_MAX_SKINS
#define MAX_MODEL 180

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
 {
    int magic;
    int version;
    int skinWidth;
    int skinHeight;
    int frameSize;
    int numSkins;
    int numVertices;
    int numTexCoords;
    int numTriangles;
    int numGlCommands;
    int numFrames;
    int offsetSkins;
    int offsetTexCoords;
    int offsetTriangles;
    int offsetFrames;
    int offsetGlCommands;
    int offsetEnd;

} MD2_HEADER;

typedef struct
{
   unsigned char vertex[3];
   unsigned char lightNormalIndex;

} MD2_ALIAS_TRIANGLEVERTEX;

typedef struct
{
    float vertex[3];
    int   lightNormalIndex;     /* float normal[3]; */

} MD2_TRIANGLEVERTEX;

typedef struct
{
    short vertexIndices[3];
    short textureIndices[3];

} MD2_TRIANGLE;

typedef struct
{
    short s, t;

} MD2_TEXCOORDINATE;

typedef struct
{
    float scale[3];
    float translate[3];
    char  name[16];
    MD2_ALIAS_TRIANGLEVERTEX alias_vertices[1];

} MD2_ALIAS_FRAME;

typedef struct
{
    char name[16];
    MD2_TRIANGLEVERTEX *vertices;

} MD2_FRAME;

typedef char MD2_SKIN[64];

typedef struct
{
    float s, t;
    int   vertexIndex;

} MD2_GLCOMMANDVERTEX;

typedef struct
{
    char	nameAnimation[16];  /* NameLen = 15 + 0 for end of string */
    int		firstFrame;
    int		lastFrame;

} ANIMATION_T;

typedef struct
{
    int id;             /* Model is loaded. 0 means not loaded */
    MD2_HEADER	        header;
    MD2_SKIN	        *skins;
    MD2_TEXCOORDINATE   *texCoords;
    MD2_TRIANGLE        *triangles;
    MD2_FRAME	        *frames;
    int                 *glCommandBuffer;

    // Textures
    GLuint icon_tex[MD2_MAX_ICONS];
    GLuint skin_tex[MD2_MAX_SKINS];
    int num_icon;       // Number of icons available
    int num_skin;       // Number of skins available
    /* ---- Animation ---- */
    ANIMATION_T anim[MD2_MAX_ANIMATION + 1];
    int count_anim;
    // Model is loaded, yes / no
    char md2_loaded;

} MD2_MODEL;

/******************************************************************************
* DATA                                                                        *
******************************************************************************/

static MD2_MODEL Models[MAX_MODEL + 2];

static unsigned char wirec[3] = { 255, 255, 255 };
static int  WireFrame = 0;

/******************************************************************************
* DATA                                                                        *
******************************************************************************/

/* This is id's normal table for computing light values */
static float md2normals[][3] =
{{-0.5257,0.0000,0.8507},{-0.4429,0.2389,0.8642},{-0.2952,0.0000,0.9554},
 {-0.3090,0.5000,0.8090},{-0.1625,0.2629,0.9511},{0.0000,0.0000,1.0000},
 {0.0000,0.8507,0.5257},{-0.1476,0.7166,0.6817},{0.1476,0.7166,0.6817},
 {0.0000,0.5257,0.8507},{0.3090,0.5000,0.8090},{0.5257,0.0000,0.8507},
 {0.2952,0.0000,0.9554},{0.4429,0.2389,0.8642},{0.1625,0.2629,0.9511},
 {-0.6817,0.1476,0.7166},{-0.8090,0.3090,0.5000},{-0.5878,0.4253,0.6882},
 {-0.8507,0.5257,0.0000},{-0.8642,0.4429,0.2389},{-0.7166,0.6817,0.1476},
 {-0.6882,0.5878,0.4253},{-0.5000,0.8090,0.3090},{-0.2389,0.8642,0.4429},
 {-0.4253,0.6882,0.5878},{-0.7166,0.6817,-0.1476},{-0.5000,0.8090,-0.3090},
 {-0.5257,0.8507,0.0000},{0.0000,0.8507,-0.5257},{-0.2389,0.8642,-0.4429},
 {0.0000,0.9554,-0.2952},{-0.2629,0.9511,-0.1625},{0.0000,1.0000,0.0000},
 {0.0000,0.9554,0.2952},{-0.2629,0.9511,0.1625},{0.2389,0.8642,0.4429},
 {0.2629,0.9511,0.1625},{0.5000,0.8090,0.3090},{0.2389,0.8642,-0.4429},
 {0.2629,0.9511,-0.1625},{0.5000,0.8090,-0.3090},{0.8507,0.5257,0.0000},
 {0.7166,0.6817,0.1476},{0.7166,0.6817,-0.1476},{0.5257,0.8507,0.0000},
 {0.4253,0.6882,0.5878},{0.8642,0.4429,0.2389},{0.6882,0.5878,0.4253},
 {0.8090,0.3090,0.5000},{0.6817,0.1476,0.7166},{0.5878,0.4253,0.6882},
 {0.9554,0.2952,0.0000},{1.0000,0.0000,0.0000},{0.9511,0.1625,0.2629},
 {0.8507,-0.5257,0.0000},{0.9554,-0.2952,0.0000},{0.8642,-0.4429,0.2389},
 {0.9511,-0.1625,0.2629},{0.8090,-0.3090,0.5000},{0.6817,-0.1476,0.7166},
 {0.8507,0.0000,0.5257},{0.8642,0.4429,-0.2389},{0.8090,0.3090,-0.5000},
 {0.9511,0.1625,-0.2629},{0.5257,0.0000,-0.8507},{0.6817,0.1476,-0.7166},
 {0.6817,-0.1476,-0.7166},{0.8507,0.0000,-0.5257},{0.8090,-0.3090,-0.5000},
 {0.8642,-0.4429,-0.2389},{0.9511,-0.1625,-0.2629},{0.1476,0.7166,-0.6817},
 {0.3090,0.5000,-0.8090},{0.4253,0.6882,-0.5878},{0.4429,0.2389,-0.8642},
 {0.5878,0.4253,-0.6882},{0.6882,0.5878,-0.4253},{-0.1476,0.7166,-0.6817},
 {-0.3090,0.5000,-0.8090},{0.0000,0.5257,-0.8507},{-0.5257,0.0000,-0.8507},
 {-0.4429,0.2389,-0.8642},{-0.2952,0.0000,-0.9554},{-0.1625,0.2629,-0.9511},
 {0.0000,0.0000,-1.0000},{0.2952,0.0000,-0.9554},{0.1625,0.2629,-0.9511},
 {-0.4429,-0.2389,-0.8642},{-0.3090,-0.5000,-0.8090},{-0.1625,-0.2629,-0.9511},
 {0.0000,-0.8507,-0.5257},{-0.1476,-0.7166,-0.6817},{0.1476,-0.7166,-0.6817},
 {0.0000,-0.5257,-0.8507},{0.3090,-0.5000,-0.8090},{0.4429,-0.2389,-0.8642},
 {0.1625,-0.2629,-0.9511},{0.2389,-0.8642,-0.4429},{0.5000,-0.8090,-0.3090},
 {0.4253,-0.6882,-0.5878},{0.7166,-0.6817,-0.1476},{0.6882,-0.5878,-0.4253},
 {0.5878,-0.4253,-0.6882},{0.0000,-0.9554,-0.2952},{0.0000,-1.0000,0.0000},
 {0.2629,-0.9511,-0.1625},{0.0000,-0.8507,0.5257},{0.0000,-0.9554,0.2952},
 {0.2389,-0.8642,0.4429},{0.2629,-0.9511,0.1625},{0.5000,-0.8090,0.3090},
 {0.7166,-0.6817,0.1476},{0.5257,-0.8507,0.0000},{-0.2389,-0.8642,-0.4429},
 {-0.5000,-0.8090,-0.3090},{-0.2629,-0.9511,-0.1625},{-0.8507,-0.5257,0.0000},
 {-0.7166,-0.6817,-0.1476},{-0.7166,-0.6817,0.1476},{-0.5257,-0.8507,0.0000},
 {-0.5000,-0.8090,0.3090},{-0.2389,-0.8642,0.4429},{-0.2629,-0.9511,0.1625},
 {-0.8642,-0.4429,0.2389},{-0.8090,-0.3090,0.5000},{-0.6882,-0.5878,0.4253},
 {-0.6817,-0.1476,0.7166},{-0.4429,-0.2389,0.8642},{-0.5878,-0.4253,0.6882},
 {-0.3090,-0.5000,0.8090},{-0.1476,-0.7166,0.6817},{-0.4253,-0.6882,0.5878},
 {-0.1625,-0.2629,0.9511},{0.4429,-0.2389,0.8642},{0.1625,-0.2629,0.9511},
 {0.3090,-0.5000,0.8090},{0.1476,-0.7166,0.6817},{0.0000,-0.5257,0.8507},
 {0.4253,-0.6882,0.5878},{0.5878,-0.4253,0.6882},{0.6882,-0.5878,0.4253},
 {-0.9554,0.2952,0.0000},{-0.9511,0.1625,0.2629},{-1.0000,0.0000,0.0000},
 {-0.8507,0.0000,0.5257},{-0.9554,-0.2952,0.0000},{-0.9511,-0.1625,0.2629},
 {-0.8642,0.4429,-0.2389},{-0.9511,0.1625,-0.2629},{-0.8090,0.3090,-0.5000},
 {-0.8642,-0.4429,-0.2389},{-0.9511,-0.1625,-0.2629},{-0.8090,-0.3090,-0.5000},
 {-0.6817,0.1476,-0.7166},{-0.6817,-0.1476,-0.7166},{-0.8507,0.0000,-0.5257},
 {-0.6882,0.5878,-0.4253},{-0.5878,0.4253,-0.6882},{-0.4253,0.6882,-0.5878},
 {-0.4253,-0.6882,-0.5878},{-0.5878,-0.4253,-0.6882},{-0.6882,-0.5878,-0.4253},
 {0,0,0}  // Spikey mace
};

/******************************************************************************
* FORWARD DECLARATIONS                                                        *
******************************************************************************/

static void egomodelDrawFrameSmp(MD2_MODEL *model, int actframe);
static void egomodeldrawFrameItp(MD2_MODEL *model, int actframe, int nextframe, float pol);

/******************************************************************************
* CODE                                                                        *
******************************************************************************/

/*
 * Name:
 *     Anim_get_Name
 * Description:
 *     Gets the name of the animation (strips the value from the
 *     animation name).
 *     If the value length  <= 1, then it's a single frame
 *     If the value length = 2, then it's a frame number
 *     if the value length = 3, then the right two values are the frame number
 * Input:
 *      fullframename  *: Animation name holding number
 *      framename  *:     Where to return the animation name
 */
static void Anim_get_Name(char *fullframename , char *framename)
{
    int len;


    /* 1) Create a copy of the framename */
    strcpy(framename , fullframename );
    while(*framename) {

        if ( ((*framename) >= '0') && ((*framename) <= '9')) {

            /* 2) We found the first value in the 'framename' */
            len = strlen(framename);
            switch(len) {

                case 1:     /* It's a single frame number...        */
                case 2:     /* It's a two space frame number...     */
                    *framename = 0;
                    return;

                case 3:
                    framename[1] = 0;   /* ... strip frame number   */
                    return;

            }

        }

        framename++;

    }

}

/*
 * Name:
 *     Anim_add_Frame
 * Description:
 *
 * Input:
 *      model *:  Pointer on model to add animation to
 *      name *:   Name of aniamtion
 *      frame:    Frame to add
 */
static void Anim_add_Frame(MD2_MODEL *model, char name[16], int frame)
{

    int i;
    char animname[30];


    /* Look for special animation description->"all" */
    if (name[0] == 0) {     /* whole animation */

        strcpy(model->anim[0].nameAnimation, "all");
        model->anim[0].firstFrame = 0;
        model->anim[0].lastFrame  = frame - 1;
        return;

    }

    /* 1) Look up if animation is already created */
    Anim_get_Name(name, animname);

    for (i = 1; i < MD2_MAX_ANIMATION; i++)
    {
        if (! strcmp(model->anim[i].nameAnimation, animname))
        {
            /* Animation already available, new last frame */
            model->anim[i].lastFrame = frame;
            return;
        }
    }

    /* Generate a new animation */
    for (i = 1; i < MD2_MAX_ANIMATION; i++)
    {
        if (model->anim[i].nameAnimation[0] == 0)
        {
            /* We found an empty slot */
            strcpy(model->anim[i].nameAnimation, animname);
            model->anim[i].firstFrame = frame;
            model->count_anim++;
        }
    }
}

/*
 * Name:
 *     egomodelDrawFrameSmp
 * Description:
 *     Draws given frame from given model
 * Input:
 *      model *:  Pointer on model to draw
 *      actframe: Number of frame to draw
 */
static void egomodelDrawFrameSmp(MD2_MODEL *model, int actframe)
{

    int index;
    int count;
    int i = 1;
    int val = model->glCommandBuffer[0];
    MD2_GLCOMMANDVERTEX *cmd;
    MD2_FRAME *curFrame;


    if (WireFrame)
    {
        glColor3f((float) wirec[0]/256, (float) wirec[1]/256, (float) wirec[2]/256);
    }
    else
    {
        glColor3f(1.f, 1.f, 1.f);
    }

    curFrame  = &model->frames[actframe];

    while (val != 0)
    {
    	if (val > 0)
        {
	        glBegin(GL_TRIANGLE_STRIP);
   	        count = val;
        }
	    else
        {
	        glBegin(GL_TRIANGLE_FAN);
    	    count = -val;
        }

        while (count--)
        {
            cmd   = (MD2_GLCOMMANDVERTEX *)&model->glCommandBuffer[i];
            index = cmd->vertexIndex;
            i += 3;

     	    glTexCoord2fv(&cmd->s);
	        glNormal3fv(&md2normals[curFrame->vertices[index].lightNormalIndex][0]);
  	        glVertex3fv(&curFrame->vertices[index].vertex[0]);
        }

    	glEnd ();

    	val = model->glCommandBuffer[i];
        i++;
    }
}

/*
 * Name:
 *     egomodeldrawFrameItp
 * Description:
 *
 * Input:
 *      curFrame: Pointer on model to draw
 */
static void egomodeldrawFrameItp(MD2_MODEL *model, int actframe, int nextiframe, float pol)
{

    int index;
    int count;
    float x1, y1, z1;
    float x2, y2, z2;
    int i = 1;
    int val = model->glCommandBuffer[0];
    MD2_GLCOMMANDVERTEX *cmd;
    MD2_FRAME *curFrame, *nextFrame;


    curFrame  = &model->frames[actframe];
    nextFrame = &model->frames[nextiframe];

    if (WireFrame)
    {
        glColor3f((float) wirec[0]/256, (float) wirec[1]/256, (float) wirec[2]/256);
    }
    else
    {
        glColor3f(1.f, 1.f, 1.f);
    }


    while (val != 0)
    {
    	if (val > 0)
        {
	        glBegin(GL_TRIANGLE_STRIP);
	        count = val;
        }
    	else
        {
	        glBegin(GL_TRIANGLE_FAN);
	        count = -val;
        }

    	while (count--)
        {
            cmd = (MD2_GLCOMMANDVERTEX *)&model->glCommandBuffer[i];
            index = cmd->vertexIndex;
            i += 3;

     	    glTexCoord2fv(&cmd->s);

            x1 = curFrame->vertices[index].vertex[0];
	        y1 = curFrame->vertices[index].vertex[1];
	        z1 = curFrame->vertices[index].vertex[2];

    	    x2 = nextFrame->vertices[index].vertex[0];
	        y2 = nextFrame->vertices[index].vertex[1];
	        z2 = nextFrame->vertices[index].vertex[2];

            glNormal3fv(&md2normals[curFrame->vertices[index].lightNormalIndex][0]);    
       	    glVertex3f(x1 + pol * (x2 - x1), y1 + pol * (y2 - y1), z1 + pol * (z2 - z1));
        }

    	glEnd ();

        val = model->glCommandBuffer[i];
        i++;
    }
}

/*
 * Name:
 *     egomodelNewModel
 * Description:
 *     Returns the number of a new model, if available, else returns 0.
 * Input:
 *      None
 */
static int egomodelNewModel(void)
{
    int mdl_no;


    /* Look for a free model buffer */
    for (mdl_no = 1; mdl_no < MAX_MODEL; mdl_no++)
    {
        if (0 == Models[mdl_no].id)
        {
            /* We found an empty buffer */
            memset(&Models[mdl_no], 0, sizeof(MD2_MODEL));
            Models[mdl_no].id = mdl_no;       /* My id        */
            return mdl_no;                     /* Found one    */
        }
    }

    return 0;
}

/*
 * Name:
 *     egomodelAdjustZPosToZero
 * Description:
 *     Adjust the position of the model such way along the Z-Axis, that the
 *     lowest Z-Value of the 'stand' frame is at Z = 1.0
 *     If the lowest Z-Value is < -10.0, it's assumed that the model
 '     is 'flowing'.
 *     Checks the first frame for lowest Z-Value and uses it for adjustment
 * Input:
 *     model *:
 */
static void egomodelAdjustZPosToZero(MD2_MODEL *model)
{

    int i, j;
    float zmin, zact;


    zmin = 0.0;


    for (j=0; j < model->header.numVertices; j++) {

        zact = model->frames[0].vertices[j].vertex[2];
        if (zact < zmin) {

            zmin = zact;

        }

    }

    zmin -= 1.0;        /* +1.0 over zero */


    for (i=0; i < model->header.numFrames; i++) {

        for (j=0; j < model->header.numVertices; j++) {

    	    model->frames[i].vertices[j].vertex[2] -= zmin;   /* Adjust */

        }

    }

}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */

/*
 * Name:
 *     egomodelLoad
 * Description:
 *     Loads an MD2-Model. From actually set object directory
 * Input:
 *      bbox[2][3]: Where to return the bounding box values
 * Output:
 *      Number of model used for calling the draw function with
 *      0: Loading of model failed
 */
int egomodelLoad(float bbox[2][3])
{
    char *fname;
    char file_name[64];
    unsigned char buffer[MD2_MAX_FRAMESIZE];
    MD2_ALIAS_FRAME *frame;
    MD2_MODEL *model;
    FILE *md2file;
    static char ver[5];
    int i, j, mdl_no;

    
    // Create filename
    fname = egofileMakeFileName(EGOFILE_ACTOBJDIR, "tris.md2");
    
    mdl_no = egomodelNewModel();   /* Get buffer for model */

    if (! mdl_no)
    {
        return 0;
    }

    model = &Models[mdl_no];
    
    md2file = fopen(fname, "rb");
    if (NULL == md2file)
    {
        model->id = 0;
        return 0;
    }

    fread(&model->header, 1, sizeof(MD2_HEADER), md2file);

    sprintf(ver, "%c%c%c%c", model->header.magic, model->header.magic >> 8,
                             model->header.magic >> 16, model->header.magic >> 24);

    if ( strcmp(ver, "IDP2") || model->header.version != 8)
    {
    	fclose(md2file);
        model->id = 0;
        return 0;
    }
    Anim_add_Frame(model, "", model->header.numFrames);

    fseek(md2file, model->header.offsetSkins, SEEK_SET);

    model->skins = (MD2_SKIN *) malloc(sizeof(MD2_SKIN) * model->header.numSkins);

    for (i=0; i < model->header.numSkins; i++)
    {
        fread(&model->skins[i], sizeof(MD2_SKIN), 1, md2file);
    }

    fseek(md2file, model->header.offsetTexCoords, SEEK_SET);

    model->texCoords = (MD2_TEXCOORDINATE *) malloc(sizeof(MD2_TEXCOORDINATE) * model->header.numTexCoords);

    fread(model->texCoords, sizeof(MD2_TEXCOORDINATE), model->header.numTexCoords, md2file);

    // sdlgllogWrite("triangles ....");
    fseek(md2file, model->header.offsetTriangles, SEEK_SET);

    model->triangles = (MD2_TRIANGLE *) malloc(sizeof(MD2_TRIANGLE) * model->header.numTriangles);

    fread(model->triangles, sizeof(MD2_TRIANGLE), model->header.numTriangles, md2file);

    // sdlgllogWrite("alias frames ....");

    fseek(md2file, model->header.offsetFrames, SEEK_SET);
    model->frames = (MD2_FRAME *) malloc(sizeof(MD2_FRAME) * model->header.numFrames);

    for (i=0; i < model->header.numFrames; i++)
    {
        frame = (MD2_ALIAS_FRAME *) buffer;

        model->frames[i].vertices = (MD2_TRIANGLEVERTEX *) malloc(sizeof(MD2_TRIANGLEVERTEX) * model->header.numVertices);

        fread(frame, 1, model->header.frameSize, md2file);

        strcpy(model->frames[i].name, frame->name);

        Anim_add_Frame(model, model->frames[i].name, i);

        for (j=0; j < model->header.numVertices; j++)
        {
    	    model->frames[i].vertices[j].vertex[0] = (float) ((int) frame->alias_vertices[j].vertex[0]) * frame->scale[0] + frame->translate[0];
	        model->frames[i].vertices[j].vertex[1] = (float) ((int) frame->alias_vertices[j].vertex[1]) * frame->scale[1] + frame->translate[1];
	        model->frames[i].vertices[j].vertex[2] = (float) ((int) frame->alias_vertices[j].vertex[2]) * frame->scale[2] + frame->translate[2];

    	    model->frames[i].vertices[j].lightNormalIndex = frame->alias_vertices[j].lightNormalIndex;
        }
    }

    fseek(md2file, model->header.offsetGlCommands, SEEK_SET);

    model->glCommandBuffer = (int *) malloc(sizeof(int) * model->header.numGlCommands);
    fread(model->glCommandBuffer, sizeof(int), model->header.numGlCommands, md2file);

    fclose(md2file);

    // @todo: Calculate boundiing box using first frame of model
    /* Set the bounding box values */
    bbox[0][0] = -26.0;    /* X */
    bbox[0][1] = -26.0;    /* Y */
    bbox[0][2] = 0.0;      /* Z */

    bbox[1][0] = 26.0;     /* X */
    bbox[1][1] = 26.0;     /* Y */
    bbox[1][2] = 60.0;     /* Z */

    // Model is loaded
    model->md2_loaded = 1;

    // Load up to four skins 
    j = 0;    
    for(i = 0; i < 4; i++)
    {   
        // Create filename
        sprintf(file_name, "tris%d.bmp");
        fname = egofileMakeFileName(EGOFILE_ACTOBJDIR, file_name);
       
        model->skin_tex[i] = sdlgltexLoadSingle(file_name);
        
        if(model->skin_tex[i])
        {
            j++;
        }
        else
        {
            break;
        }
    }
    // Save number of skins
    model->num_skin = j;
    
    // Load up to four icons
    j = 0;  
    for(i = 0; i < 4; i++)
    {
        // Create filename
        sprintf(file_name, "%sicon%d.bmp");
        fname = egofileMakeFileName(EGOFILE_ACTOBJDIR, file_name);
        
        model->icon_tex[i] = sdlgltexLoadSingle(file_name);
        
        if(model->icon_tex[i])
        {
            j++;
        }
        else
        {
            break;
        }
    }
    // Save number of icons
    model->num_icon = j;
    
    return mdl_no;
}

/*
 * Name:
 *     egomodelDraw
 * Description:
 *      Draws model with given number, given frame, using given skin
 *      (if skin is unavailable, the default skin is used)
 * Input:
 *      mdl_no:  Number of model to draw
 *      frame:    Number of frame to draw
 *      skin_no:   Number of skin to use
 * Output:
 *      frame_no:  Number of frame effective drawn
 */
int egomodelDraw(int mdl_no, int frame_no, int skin_no)
{
    MD2_MODEL *model;
    GLuint skin_tex;


    model = &Models[mdl_no];

    skin_tex = model->skin_tex[skin_no];
    if (! skin_tex)
    {
        // Use default skin
        skin_tex = model->skin_tex[0];
    }

    if (frame_no >= model->header.numFrames)
    {
        frame_no = 0;
    }

    glBindTexture(GL_TEXTURE_2D, skin_tex);

    egomodelDrawFrameSmp(model, frame_no);

    return frame_no;
}


/*
 * Name:
 *     egomodelFreeModels
 * Description:
 *     This function frees all models
 * Input:
 *      None
 */
void egomodelFreeModels(void)
{

    int mdl_no, i;
    MD2_MODEL *model;

    
    for (mdl_no = 1; mdl_no < MAX_MODEL; mdl_no++)
    {
        if (Models[mdl_no].id > 0)
        {
            model = &Models[mdl_no];

            if(model->md2_loaded)
            {
                // Free only if loaded, otherwise only icons
                if (model->skins)
                {
                    free(model->skins);
                }

                if (model->texCoords)
                {
                    free(model->texCoords);
                }

                if (model->triangles)
                {
                    free(model->triangles);
                }

                if (model->frames)
                {
            	    for (i=0; i < model->header.numFrames; i++)
                    {
            	        if (model->frames[i].vertices)
                        {
                            free(model->frames[i].vertices);
                        }
                    }

                    free(model->frames);
                }

                if (model->glCommandBuffer)
                {
                    free(model->glCommandBuffer);
                }
            }

            // Delete its skins
            glDeleteTextures(model->num_skin, &model->skin_tex[0]);
            // Delete icons
            glDeleteTextures(model->num_icon, &model->icon_tex[0]);
            
            // Sign the slot as free
            model->id = 0;
            model->md2_loaded = 0;
        }
    }
}

/*
 * Name:
 *     egomodelLoadIcon
 * Description:
 *     Loads the texture with the given name for the given model into the
 *     given icon slot.
 * Input:
 *     pobj *: Pointer on object to draw
 */
void egomodelDrawObject(SDLGL3D_OBJECT *pobj)
{
    MD2_MODEL *model;
    GLuint skin_tex;
    int nextiframe;


    model = &Models[pobj->type_no];  // Number of model
    
    // Animation and clock handling is done by caller
    skin_tex = model->skin_tex[pobj->skin_no];
    if (! skin_tex)
    {
        skin_tex = model->skin_tex[0];
    }

    glBindTexture(GL_TEXTURE_2D, skin_tex);

    if (! pobj->interp)
    {
        egomodelDrawFrameSmp(model, pobj->cur_frame);
    }
    else
    {
        if(pobj->cur_frame >= pobj->end_frame)
        {
            // @todo: Support animation back and forth
            nextiframe = pobj->base_frame;
        }
        else
        {
            nextiframe = pobj->cur_frame + 1;
        }
        egomodeldrawFrameItp(model, pobj->cur_frame, nextiframe, 
                             1.0 - (pobj->anim_clock / pobj->anim_speed));
    }
}

/*
 * Name:
 *     egomodelSetAnimation
 * Description:
 *      Fills the given animation with info for given "animname", if such
 *      animation is available
 * Input:
 *      obj_no: Set it for this object
 *      action: Name of action to set animation for 
 * Output:
 *      Success yes/no
 */
char egomodelSetAnimation(int obj_no, short int action)
{
    SDLGL3D_OBJECT *pobj;
    MD2_MODEL *model;
    int i;
    
    
    pobj = sdlgl3dGetObject(obj_no);
    
    if(! pobj->id > 0) return 0;
    
    /* Look for 'action' in animations for given model */
    model = &Models[pobj->type_no];

    for (i = 0; i < model->count_anim; i++)
    {
        if (action == *(short int *)model->anim[i].nameAnimation)
        {
            /* We found the animation looked for */
            pobj->base_frame = model->anim[i].firstFrame;
            pobj->cur_frame  = model->anim[i].firstFrame;
            pobj->end_frame  = model->anim[i].lastFrame;
            pobj->anim_speed = 0.1;     // 10 Frames per second
            pobj->anim_clock = 0.1;
            pobj->interp     = 0;       // No interpolation
            return 1;
        }
    }
    
    // Given animation not found
    return 0;
}