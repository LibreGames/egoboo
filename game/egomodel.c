/*******************************************************************************
*  SDLGLMD2.C                                                                  *
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



#include "sdlglmd2.h"       /* Own header   */

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
#define MD2_MAX_ANIMATION       32
#define MD2_MAX_SKINS			8
#define MD2_MAX_FRAMESIZE		(MD2_MAX_VERTICES * 4 + 128)

/* --------- Own stuff ---- */
#define MD2_MAX_ICONS MD2_MAX_SKINS
#define MAX_MODEL 100

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct {

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

typedef struct {

   unsigned char vertex[3];
   unsigned char lightNormalIndex;

} MD2_ALIAS_TRIANGLEVERTEX;

typedef struct {

    float vertex[3];
    int   lightNormalIndex;     /* float normal[3]; */

} MD2_TRIANGLEVERTEX;

typedef struct {

    short vertexIndices[3];
    short textureIndices[3];

} MD2_TRIANGLE;

typedef struct {

    short s, t;

} MD2_TEXCOORDINATE;

typedef struct {

    float scale[3];
    float translate[3];
    char  name[16];
    MD2_ALIAS_TRIANGLEVERTEX alias_vertices[1];

} MD2_ALIAS_FRAME;

typedef struct {

    char name[16];
    MD2_TRIANGLEVERTEX *vertices;

} MD2_FRAME;

typedef char MD2_SKIN[64];

typedef struct {

    float s, t;
    int   vertexIndex;

} MD2_GLCOMMANDVERTEX;

typedef struct ANIMATION_S {

    char	nameAnimation[16];  /* NameLen = 15 + 0 for end of string */
    int		firstFrame;
    int		lastFrame;

} ANIMATION_T;

typedef struct {

    int id;             /* Model is loaded. 0 means not loaded */
    MD2_HEADER	        header;
    MD2_SKIN	        *skins;
    MD2_TEXCOORDINATE   *texCoords;
    MD2_TRIANGLE        *triangles;
    MD2_FRAME	        *frames;
    int                 *glCommandBuffer;

    /* --- Bounding box for collision testing */
    float bbox[2][3];   /* Minimum point and maximum point (X,Y,Z)*/

    /* --- Textures --- */
    GLuint icontex[MD2_MAX_ICONS];
    GLuint skintex[MD2_MAX_SKINS];

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

static void sdlglmd2DrawFrameSmp(MD2_MODEL *model, int actframe);
static void sdlglmd2drawFrameItp(MD2_MODEL *model, int actframe, int nextframe, float pol);

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


    /* Look for special animation description -> "all" */
    if (name[0] == 0) {     /* whole animation */

        strcpy(model -> anim[0].nameAnimation, "all");
        model -> anim[0].firstFrame = 0;
        model -> anim[0].lastFrame  = frame - 1;
        return;

    }

    /* 1) Look up if animation is already created */
    Anim_get_Name(name, animname);

    for (i = 1; i < MD2_MAX_ANIMATION; i++) {

        if (! strcmp(model -> anim[i].nameAnimation, animname)) {

            /* Animation already available, new last frame */
            model -> anim[i].lastFrame = frame;
            return;

        }

    }

    /* Generate a new animation */
    for (i = 1; i < MD2_MAX_ANIMATION; i++) {

        if (model -> anim[i].nameAnimation[0] == 0) {

            /* We found an empty slot */
            strcpy(model -> anim[i].nameAnimation, animname);
            model -> anim[i].firstFrame = frame;
            model -> count_anim++;

        }

    }

}

/*
 * Name:
 *     sdlglmd2DrawFrameSmp
 * Description:
 *     Draws given frame from given model
 * Input:
 *      model *:  Pointer on model to draw
 *      actframe: Number of frame to draw
 */
static void sdlglmd2DrawFrameSmp(MD2_MODEL *model, int actframe)
{

    int index;
    int count;
    int i = 1;
    int val = model -> glCommandBuffer[0];
    MD2_GLCOMMANDVERTEX *cmd;
    MD2_FRAME *curFrame;


    if (WireFrame) {
        glColor3f((float) wirec[0]/256, (float) wirec[1]/256, (float) wirec[2]/256);
    }
    else {
        glColor3f(1.f, 1.f, 1.f);
    }

    curFrame  = &model -> frames[actframe];

    while (val != 0) {

    	if (val > 0) {

	        glBegin(GL_TRIANGLE_STRIP);
   	        count = val;

        }
	    else {

	        glBegin(GL_TRIANGLE_FAN);
    	    count = -val;

        }

        while (count--) {

            cmd   = (MD2_GLCOMMANDVERTEX *)&model -> glCommandBuffer[i];
            index = cmd -> vertexIndex;
            i += 3;

     	    glTexCoord2fv(&cmd -> s);
	        glNormal3fv(&md2normals[curFrame -> vertices[index].lightNormalIndex][0]);
  	        glVertex3fv(&curFrame -> vertices[index].vertex[0]);

        }

    	glEnd ();

    	val = model -> glCommandBuffer[i];
        i++;

    }

}

/*
 * Name:
 *     sdlglmd2drawFrameItp
 * Description:
 *
 * Input:
 *      curFrame: Pointer on model to draw
 */
static void sdlglmd2drawFrameItp(MD2_MODEL *model, int actframe, int nextiframe, float pol)
{

    int index;
    int count;
    float x1, y1, z1;
    float x2, y2, z2;
    int i = 1;
    int val = model -> glCommandBuffer[0];
    MD2_GLCOMMANDVERTEX *cmd;
    MD2_FRAME *curFrame, *nextFrame;


    curFrame  = &model -> frames[actframe];
    nextFrame = &model -> frames[nextiframe];

    if (WireFrame) {

        glColor3f((float) wirec[0]/256, (float) wirec[1]/256, (float) wirec[2]/256);

    }
    else {

        glColor3f(1.f, 1.f, 1.f);
        
    }


    while (val != 0) {

    	if (val > 0) {

	        glBegin(GL_TRIANGLE_STRIP);
	        count = val;

        }
    	else {
	        glBegin(GL_TRIANGLE_FAN);
	        count = -val;
        }

    	while (count--) {

            cmd = (MD2_GLCOMMANDVERTEX *)&model -> glCommandBuffer[i];
            index = cmd -> vertexIndex;
            i += 3;

     	    glTexCoord2fv(&cmd -> s);

            x1 = curFrame -> vertices[index].vertex[0];
	        y1 = curFrame -> vertices[index].vertex[1];
	        z1 = curFrame -> vertices[index].vertex[2];

    	    x2 = nextFrame -> vertices[index].vertex[0];
	        y2 = nextFrame -> vertices[index].vertex[1];
	        z2 = nextFrame -> vertices[index].vertex[2];

       	    glVertex3f( x1 + pol * (x2 - x1), y1 + pol * (y2 - y1), z1 + pol * (z2 - z1) );

        }

    	glEnd ();

        val = model -> glCommandBuffer[i];
        i++;

    }

}

/*
 * Name:
 *     sdlglmd2ChangeFrame
 * Description:
 *     Changes the animation frame, using info in 'anim *, model *' and
 *     ticks passed
 * Input:
 *     anim *:      Animation to check for changing frame
 *     model *:     Model to check frame for
 *     tickspassed: Ticks passed since last call
 */
static void sdlglmd2ChangeFrame(SDLGLMD2_ANIM *anim, MD2_MODEL *model, int tickspassed)
{

    if (anim -> animSleep < anim -> animSpeed) {

        anim -> animSleep += tickspassed;

    }

    if (anim -> animSleep < anim -> animSpeed) {

        /* Keep actual frame */
        return;

    }
    else {

        anim -> animSleep = 0; /* Reset sleep */

    }

    if (anim -> nextFrame > 0) {

        if (anim -> curFrame + anim -> nextFrame < model -> anim[anim -> curAnim].lastFrame) {

            anim -> curFrame += anim -> nextFrame;

        }
        else {

            anim -> curFrame = model -> anim[anim -> curAnim].firstFrame;

        }

     	if (anim -> curFrame + anim -> nextFrame < model -> anim[anim -> curAnim].lastFrame) {

            anim -> nextiFrame = anim -> curFrame + anim -> nextFrame;

        }
	    else {

            /* Loop animation */
            anim -> nextiFrame = model -> anim[anim -> curAnim].firstFrame;

        }

    }
    else if (anim -> nextFrame < 0) {

        if (anim -> curFrame + anim -> nextFrame > model -> anim[anim -> curAnim].firstFrame) {

            anim -> curFrame += anim -> nextFrame;

        }
        else {

            /* Loop animation */
            anim -> curFrame = model -> anim[anim -> curAnim].lastFrame /* -1 */;

        }

    	if (anim -> curFrame + anim -> nextFrame > model -> anim[anim -> curAnim].firstFrame) {

            anim -> nextiFrame = anim -> curFrame + anim -> nextFrame;

        }
        else {

            anim -> nextiFrame = model -> anim[anim -> curAnim].lastFrame /* -1 */;

        }

    }
    else {

        anim -> nextiFrame = anim -> curFrame;

    }

}

/*
 * Name:
 *     sdlglmd2NewModel
 * Description:
 *     Returns the number of a new model, if available, else returns 0.
 * Input:
 *      None
 */
static int sdlglmd2NewModel(void)
{

    int modelno;


    /* Look for a free model buffer */
    for (modelno = 1; modelno < MAX_MODEL; modelno++) {

        if (0 == Models[modelno].id) {

            /* We found an empty buffer */
            memset(&Models[modelno], 0, sizeof(MD2_MODEL));
            Models[modelno].id = modelno;       /* My id        */
            return modelno;                     /* Found one    */

        }

    }

    return 0;

}

/*
 * Name:
 *     sdlglmd2AdjustZPosToZero
 * Description:
 *     Adjust the position of the model such way along the Z-Axis, that the
 *     lowest Z-Value of the 'stand' frame is at Z = 1.0
 *     If the lowest Z-Value is < -10.0, it's assumed that the model
 '     is 'flowing'.
 *     Checks the first frame for lowest Z-Value and uses it for adjustment
 * Input:
 *     model *:
 */
static void sdlglmd2AdjustZPosToZero(MD2_MODEL *model)
{

    int i, j;
    float zmin, zact;


    zmin = 0.0;


    for (j=0; j < model -> header.numVertices; j++) {

        zact = model -> frames[0].vertices[j].vertex[2];
        if (zact < zmin) {

            zmin = zact;

        }

    }

    zmin -= 1.0;        /* +1.0 over zero */


    for (i=0; i < model -> header.numFrames; i++) {

        for (j=0; j < model -> header.numVertices; j++) {

    	    model -> frames[i].vertices[j].vertex[2] -= zmin;   /* Adjust */

        }

    }

}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */

/*
 * Name:
 *     sdlglmd2LoadModel
 * Description:
 *     Loads an MD2-Model.
 * Input:
 *      model_name *: Name of file to load the model from
 * Output:
 *      Number of model used for calling the draw function.
 *      0: Loading of model failed
 */
int sdlglmd2LoadModel(const char *model_name)
{

    unsigned char buffer[MD2_MAX_FRAMESIZE];
    MD2_ALIAS_FRAME *frame;
    MD2_MODEL *model;
    FILE *md2file;
    static char ver[5];
    int i, j, modelno;


    modelno = sdlglmd2NewModel();   /* Get buffer for model */

    if (! modelno) {

        return 0;

    }

    if(model_name[0] == 0)
    {
        // Need model only for icons (simple editor stage)
        return modelno;
    }

    model = &Models[modelno];


    md2file = fopen(model_name, "rb");
    if (md2file == NULL) {

        // sdlgllogWrite("ERROR: model %s open failed\n", model_name);
        model -> id = 0;
        return 0;

    }

    // sdlgllogWrite("\n%s -----------------------------\n", model_name);
    fread(&model -> header, 1, sizeof(MD2_HEADER), md2file);

    sprintf(ver, "%c%c%c%c", model -> header.magic, model -> header.magic >> 8,
                             model -> header.magic >> 16, model -> header.magic >> 24);

    if ( strcmp(ver, "IDP2") || model -> header.version != 8) {

    	// sdlgllogWrite("Bad header for MD2 file.\n");
    	fclose(md2file);

        return 0;

    }

    /*
    sdlgllogWrite("magic: %c%c%c%c\n", model -> header.magic, model -> header.magic>>8,
                                       model -> header.magic>>16, model -> header.magic>>24);
    sdlgllogWrite("version: %d\n", model -> header.version);
    */
    Anim_add_Frame(model, "", model -> header.numFrames);

    fseek(md2file, model -> header.offsetSkins, SEEK_SET);

    model -> skins = (MD2_SKIN *) malloc(sizeof(MD2_SKIN) * model -> header.numSkins);

    for (i=0; i < model -> header.numSkins; i++) {

        fread(&model -> skins[i], sizeof(MD2_SKIN), 1, md2file);
        // sdlgllogWrite("skins.name: %s", model -> skins[i]);

    }

    // sdlgllogWrite("texture coordinates ....");
    fseek(md2file, model -> header.offsetTexCoords, SEEK_SET);

    model -> texCoords = (MD2_TEXCOORDINATE *) malloc(sizeof(MD2_TEXCOORDINATE) * model -> header.numTexCoords);

    fread(model -> texCoords, sizeof(MD2_TEXCOORDINATE), model -> header.numTexCoords, md2file);

    // sdlgllogWrite("triangles ....");
    fseek(md2file, model -> header.offsetTriangles, SEEK_SET);

    model -> triangles = (MD2_TRIANGLE *) malloc(sizeof(MD2_TRIANGLE) * model -> header.numTriangles);

    fread(model -> triangles, sizeof(MD2_TRIANGLE), model -> header.numTriangles, md2file);

    // sdlgllogWrite("alias frames ....");

    fseek(md2file, model -> header.offsetFrames, SEEK_SET);
    model -> frames = (MD2_FRAME *) malloc(sizeof(MD2_FRAME) * model -> header.numFrames);

    for (i=0; i < model -> header.numFrames; i++) {

        frame = (MD2_ALIAS_FRAME *) buffer;

        model -> frames[i].vertices = (MD2_TRIANGLEVERTEX *) malloc(sizeof(MD2_TRIANGLEVERTEX) * model -> header.numVertices);

        fread(frame, 1, model -> header.frameSize, md2file);

        strcpy(model -> frames[i].name, frame -> name);

        /* FIXME: insert Anim_add_Frame(model, frames[i].name, i) */
        Anim_add_Frame(model, model -> frames[i].name, i);

        for (j=0; j < model -> header.numVertices; j++) {

    	    model -> frames[i].vertices[j].vertex[0] = (float) ((int) frame->alias_vertices[j].vertex[0]) * frame->scale[0] + frame->translate[0];
	        model -> frames[i].vertices[j].vertex[1] = (float) ((int) frame->alias_vertices[j].vertex[1]) * frame->scale[1] + frame->translate[1];
	        model -> frames[i].vertices[j].vertex[2] = (float) ((int) frame->alias_vertices[j].vertex[2]) * frame->scale[2] + frame->translate[2];

    	    model -> frames[i].vertices[j].lightNormalIndex = frame->alias_vertices[j].lightNormalIndex;

        }

    }

    // sdlgllogWrite("gl command ....");
    fseek(md2file, model -> header.offsetGlCommands, SEEK_SET);

    model -> glCommandBuffer = (int *) malloc(sizeof(int) * model -> header.numGlCommands);
    fread(model -> glCommandBuffer, sizeof(int), model -> header.numGlCommands, md2file);

    fclose(md2file);

    // sdlgllogClose();

    /* Set the bounding box values */
    model -> bbox[0][0] = -26.0;    /* X */
    model -> bbox[0][1] = -26.0;    /* Y */
    model -> bbox[0][2] = 0.0;      /* Z */

    model -> bbox[1][0] = 26.0;     /* X */
    model -> bbox[1][1] = 26.0;     /* Y */
    model -> bbox[1][2] = 60.0;     /* Z */

    // Model is loaded
    model -> md2_loaded = 1;
    
    return modelno;

}

/*
 * Name:
 *     sdlglmd2DrawModel
 * Description:
 *      Draws model with given number, given frame, using given skin
 *      (if skin is unavailable, the default skin is used)
 * Input:
 *      modelno:  Number of model to draw
 *      frame:    Number of frame to draw
 *      skinno:   Number of skin to use
 * Output:
 *      frameno:  Number of frame effective drawn
 */
int sdlglmd2DrawModel(int modelno, int frameno, int skinno)
{

    MD2_MODEL *model;
    GLuint skintex;


    model = &Models[modelno];

    skintex = model -> skintex[skinno];
    if (! skintex) {

        skintex = model -> skintex[0];

    }

    if (frameno >= model -> header.numFrames) {

        frameno = 0;

    }

    glBindTexture(GL_TEXTURE_2D, skintex);

    sdlglmd2DrawFrameSmp(model, frameno);

    return frameno;

}

/*
 * Name:
 *     sdlglmd2DrawModelAnimated
 * Description:
 *      Draws the model animated, using the info from 'anim *' for animation.
 *      Draws given model with given skin
 *      (if skin is unavailable, the default skin is used)
 * Input:
 *      modelno:     Number of model to draw
 *      skinno:      Number of skin to use
 *      tickspassed: Ticks passed since last call
 */
void sdlglmd2DrawModelAnimated(int modelno, int skinno, SDLGLMD2_ANIM *anim, int tickspassed)
{

    MD2_MODEL *model;
    GLuint skintex;


    model = &Models[modelno];
    /*
    if (anim -> animSleep < anim -> animSpeed) {

        anim -> animSleep += tickspassed;

    }
    */
    sdlglmd2ChangeFrame(anim, model, tickspassed);

    skintex = model -> skintex[skinno];
    if (! skintex) {

        skintex = model -> skintex[0];

    }

    glBindTexture(GL_TEXTURE_2D, skintex);

    if (! anim -> interp) {

        sdlglmd2DrawFrameSmp(model, anim -> curFrame);

    }
    else {

        sdlglmd2drawFrameItp(model, anim -> curFrame, anim -> nextiFrame,
                             (float) anim -> animSleep/anim -> animSpeed);
    }

}

/*
 * Name:
 *     sdlglmd2SetAnimation
 * Description:
 *      Fills the given animation with info for given "animname", if such
 *      animation is available
 * Input:
 *      anim *:      Pointer on animation to set animation for
 *      modelno:     Number of model to search for animation
 *      animname *:  Name of animation
 */
void sdlglmd2SetAnimation(SDLGLMD2_ANIM *anim, int modelno, char *animname)
{

    MD2_MODEL *model;
    int i;


    /* Look for 'animname *' in animations for given model */
    model = &Models[modelno];


    for (i = 0; i < model -> count_anim; i++) {

        if (! strcmp(model -> anim[i].nameAnimation, animname)) {

            /* We found the animation looked for */
            anim -> curFrame   = model -> anim[i].firstFrame;
            anim -> nextFrame  = +1;        /* next frame   */
            anim -> curAnim    = i;
            anim -> animStatus = 2;         /* play                 */
            anim -> animSpeed  = 100;       /* 100 ticks each frame */
            anim -> animSleep  = 0;
            anim -> interp     = 0;         /* No interpolation     */
            anim -> lastFrame  = model -> anim[i].lastFrame;
            anim -> nextiFrame = anim -> curFrame;

            if (anim -> curFrame < anim -> lastFrame) {

                anim -> nextiFrame++;

            }
            else {

                anim -> nextFrame = 0;

            }

            return;

        }

    }

}

/*
 * Name:
 *     sdlglmd2FreeModels
 * Description:
 *     This function frees all models
 * Input:
 *      None
 */
void sdlglmd2FreeModels(void)
{

    int modelno, i;
    MD2_MODEL *model;

    
    for (modelno = 1; modelno < MAX_MODEL; modelno++) {

        if (Models[modelno].id > 0) {

            model = &Models[modelno];

            if(model -> md2_loaded)
            {
                // Free only if loaded, otherwise only icons
                if (model -> skins) {
                    free(model -> skins);
                }

                if (model -> texCoords) {
                    free(model -> texCoords);
                }

                if (model -> triangles) {
                    free(model -> triangles);
                }

                if (model -> frames) {

            	    for (i=0; i < model -> header.numFrames; i++) {

            	        if (model -> frames[i].vertices)  {
                            free(model -> frames[i].vertices);
                        }

                    }

                    free(model -> frames);

                }

                if (model -> glCommandBuffer) {

                    free(model -> glCommandBuffer);

                }

            }

            // Delete icons
            for (i = 0; i < MD2_MAX_ICONS; i++) {

                if (model -> icontex[i] != 0) {

                    glDeleteTextures(1, &model -> icontex[i]);

                }

            }

            // Delete its skins
            for (i = 0; i < MD2_MAX_SKINS; i++) {

                if (model -> skintex[i] != 0) {

                    glDeleteTextures(1, &model -> skintex[i]);

                }

            }

            // Sign the slot as free
            Models[modelno].id = 0;
        }
    }
}

/*
 * Name:
 *     sdlglmd2LoadSkin
 * Description:
 *     Loads the texture with the given name for the given model into the
 *     given skin slot.
 * Input:
 *     filename *:
 *     modelno:
 *     skinno:
 */
void sdlglmd2LoadSkin(const char *filename, int modelno, int skinno)
{
    Models[modelno].skintex[skinno] = sdlgltexLoadSingle(filename);
}

/*
 * Name:
 *     sdlglmd2LoadIcon
 * Description:
 *     Loads the texture with the given name for the given model into the
 *     given icon slot.
 * Input:
 *     filename *:
 *     modelno:
 *     skinno:
 */
void sdlglmd2LoadIcon(const char *filename, int modelno, int skinno)
{
    Models[modelno].skintex[skinno] = sdlgltexLoadSingle(filename);
}

/*
 * Name:
 *     sdlglmd2GetIconTexNo
 * Description:
 *     Returns the texture number of the given icon, if available.
 *     If not, then it uses the default texture in slot 0.
 * Input:
 *     modelno: For this model 
 *     iconno:  Return this icon
 */
int sdlglmd2GetIconTexNo(int modelno, int iconno)
{
    return Models[modelno].icontex[iconno];
}



