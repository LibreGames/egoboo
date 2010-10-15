/*******************************************************************************
*  SDLGL3D.C                                                                   *
*      - Structs and functions for handling a 3D - View with Z-Axis up         *
*                                                                              *
*   SDLGL - Library                                                            *
*      Copyright (c)2004-2010 Paul Mueller <pmtech@swissonline.ch>             *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *
*******************************************************************************/

/*******************************************************************************
* INCLUDES							                                           *
*******************************************************************************/

#include <math.h>
#include <memory.h>

#include "sdlgl.h"      /* Include Opengl-Headers and library functions */


#include "sdlgl3d.h"    /* Own header                                   */

/*******************************************************************************
* DEFINES							                                           *
*******************************************************************************/

#define SDLGL3D_CAMERA_EXTENT  15.0
#define SDLGL3D_I_CAMERA_FOV 60.0 /* Instead of 90.0 degrees */ 

/* ------- */
#define SDLGL3D_I_MAXVISITILES 128

#define SDLGL3D_VISITILEDEPTH 5         /* Number of squares visible in depth */

#define SDLGL3D_I_VIEWMINWIDTH  96      /* For zoom...              */
#define SDLGL3D_I_VIEWWIDTH     128     
#define SDLGL3D_I_ASPECTRATIO   0.75    /* Handed over by caller    */

#define SDLGL3D_I_ZMIN   48.0   
#define SDLGL3D_I_ZMAX 1385.0   

/* ------- GRID_DATA ------ */
#define SDLGL3D_TILESIZE     128.0

/* Internal defines */
#define NO_CLIP     0x00
#define SOME_CLIP   0x01
#define NOT_VISIBLE 0x02


#define SDLGL3D_MAXOBJECT 512 

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    /* Type of camera and size of frustum for zooming */
    int   type;
    SDLGL3D_FRUSTUM f;          /* Frustum data, including info for visibility */
    float cameradist;           /* Distance behind object for third person camera */
    SDLGL3D_OBJECT *object;     /* The object the camera is attached to           */
    SDLGL3D_OBJECT campos;
    /* Frustum data for calculation of visibility */
    char  bound;                /* Camera is bound to an x,y-rectangle  */
    float bx, by, bx2, by2;    
    
} SDLGL3D_CAMERA;

/*****************************************************************************
* DATA								             *
*****************************************************************************/

/* ---- Additional info to calculate visbility on base of tiles */
static int MapW = 32;
static int MapH = 32;
static float TileSize = 128.0;      /* For tiles visible in FOV */
/* This are the cameras. 0: Standard camera */
static SDLGL3D_CAMERA Camera[4] = {
    { SDLGL3D_CAMERATYPE_THIRDPERSON,
      {
        SDLGL3D_I_CAMERA_FOV, /* Field of view */
        SDLGL3D_I_VIEWWIDTH,
        SDLGL3D_I_ASPECTRATIO, /* Aspect ration of view-plane */
	    SDLGL3D_I_ZMIN, SDLGL3D_I_ZMAX,
 	  },
 	  800.0
    }

};

static SDLGL3D_OBJECT Object_List[SDLGL3D_MAXOBJECT + 62];  /* List of objects to handle    */
/* Additional objects are used for visible tiles */
static int NumVisiTiles = 0;
static SDLGL3D_VISITILE Visi_Tiles[SDLGL3D_I_MAXVISITILES + 2];

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/*
 * Name:
 *     CheckBoxAgainstFrustum
 * Description:
 *      Given the box centered at (cx,cz) and extending +/- HalfSize units
 *      along both axes, this function checks to see if the box overlaps the
 *      given frustum.  If the box is totally outside the frustum, then
 *     returns NOT_VISIBLE; if the box is totally inside the frustum, then
 *     returns NO_CLIP; otherwise, returns SOME_CLIP.
 *
 * Code from: Game Programming Gems, Section 4, Chapter 11
 *	      "Loose Octrees"
 *	       -thatcher 4/6/2000 Copyright Thatcher Ulrich
 */
static char CheckBoxAgainstFrustum(int cx, int cy, int HalfSize,
                                   SDLGL3D_FRUSTUM *f, int posx, int posy)
{
    int	OrCodes = 0, AndCodes = ~0;

    int	i, j, k;

    /* Check each box corner against each edge of the frustum. */
    for (j = 0; j < 2; j++) {

    	for (i = 0; i < 2; i++) {
	        // int	mask = 1 << (j * 2 + i);

    	    int x = cx + (i == 0 ? -HalfSize : HalfSize);
	        int y = cy + (j == 0 ? -HalfSize : HalfSize);

    	    /* Check the corner against the two sides of the frustum. */
            int	Code = 0;
	        int	Bit = 1;
    	    for (k = 0; k < 3; k++, Bit <<= 1) {

	        	 float dot = f -> nx[k] * (x - posx) + f -> ny[k] * (y - posy);

	        	 if (dot < 0) {
    		         // The point is outside this edge.
	    	        Code |= Bit;
		        }

            }

    	    OrCodes |= Code;
	        AndCodes &= Code;
        }

    }

    // Based on bit-codes, return culling results.
    if (OrCodes == 0) {
	    /* The box is completely within the frustum (no box vert is outside any frustum edge). */
	    return NO_CLIP;
    } else if (AndCodes != 0) {
	/* All the box verts are outside one of the frustum edges.     */
	    return NOT_VISIBLE;
    } else {
	    return SOME_CLIP;
    }

}

/*
 * Name:
 *     sdlgl3dGetVisiTiles
 * Description:
 *     Calculates the tiles visible in FOV.
 *     Fills in the array 'Visi_Tiles' 
 * Input:
 *     f *: Pointer on frustum to calc visible tiles for
 * Output:
 *     Number of visible tiles in frustum 
 */
static int sdlgl3dGetVisiTiles(SDLGL3D_FRUSTUM *f) 
{

    int tx1, ty1, tx2, ty2, ltx;         
    
    
    NumVisiTiles = 0;
    
    tx1 = f -> vx1 / TileSize;
    tx2 = f -> vx2 / TileSize;
    ty1 = f -> vy1 / TileSize;
    ty2 = f -> vy2 / TileSize;
    
    while(ty1 <= ty2) {
        ltx = tx1;
        while(ltx <= tx2) {
            
            Visi_Tiles[NumVisiTiles].no    = (ty1 * MapW) + ltx;
            Visi_Tiles[NumVisiTiles].mid_x = (ltx * TileSize) + 64;
            Visi_Tiles[NumVisiTiles].mid_y = (ty1 * TileSize) + 64;;
            NumVisiTiles++;
            if (NumVisiTiles >= (SDLGL3D_I_MAXVISITILES - 1)) {
                Visi_Tiles[NumVisiTiles].no = -1;    /* Sign end of list */
                return NumVisiTiles;
            }
            ltx++;
        }
        ty1++;
    }
    
    Visi_Tiles[NumVisiTiles].no = -1;      /* Sign end of array */
    
    /* 
        TODO: 
            1. Only use tiles which are checked by 'CheckBoxAgainstFrustum()'
            2. Sort tiles by distance from camx, camy ==> Far to Near   
    */
    
    return NumVisiTiles;

}

/*
 * Name:
 *     sdlgl3dSetupFrustumNormals
 * Description:
 *	Sets the normals of the frustum boundaries using the given parameters.
 * Input:
 *      SDLGL3D_CAMERA *: Camera with frustum to fill with the values. Holding the
 *                        cameras position, too and the FOV-Value
 * Code from: Game Programming Gems, Section 4, Chapter 11
 *	      "Loose Octrees"
 *	       -thatcher 4/6/2000 Copyright Thatcher Ulrich
 */
static void sdlgl3dSetupFrustumNormals(SDLGL3D_CAMERA *cam)
{

    SDLGL3D_OBJECT *obj;
    int rotz;
    float minx, miny, maxx, maxy;
    float nx1, ny1, nx2, ny2, camx, camy;


    if (cam -> type == SDLGL3D_CAMERATYPE_FIRSTPERSON) {
        obj = cam -> object;

    }
    else {
        obj = &cam -> campos;
    }

    rotz = obj -> rot[2];
    camx = obj -> pos[0];
    camy = obj -> pos[1];

    cam -> f.rightangle = rotz - cam -> f.fov/2;
    cam -> f.leftangle  = rotz + cam -> f.fov/2;

    cam -> f.nx[0] = sin(DEG2RAD(cam -> f.rightangle));
    cam -> f.ny[0] = cos(DEG2RAD(cam -> f.rightangle));

    cam -> f.nx[1] = sin(DEG2RAD(cam -> f.leftangle));
    cam -> f.ny[1] = cos(DEG2RAD(cam -> f.leftangle));

    cam -> f.nx[2] = sin(DEG2RAD(rotz));
    cam -> f.ny[2] = cos(DEG2RAD(rotz));

    /* ------ Calculate the bounding rectangle of the frustum -------- */
    nx1 = cam -> f.nx[0] * cam -> f.zmax;
    ny1 = cam -> f.ny[0] * cam -> f.zmax;
    nx2 = cam -> f.nx[1] * cam -> f.zmax;
    ny2 = cam -> f.ny[1] * cam -> f.zmax;

    minx = MIN(nx1, nx2);
    miny = MIN(ny1, ny2);
    /* And now theres the position of the camer itself  */
    minx = MIN(minx, camx);
    miny = MIN(miny, camy);

    maxx = MAX(nx1, nx2);
    maxy = MAX(ny1, ny2);
    maxx = MAX(maxx, camx);
    maxy = MAX(maxy, camy);
    /* ------ And now writ it into the frustum info after binding to map --- */
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx > (MapW * TileSize)) maxx = (MapW * TileSize);
    if (maxy > (MapH * TileSize)) maxx = (MapH * TileSize);
    cam -> f.vx1 = minx;
    cam -> f.vy1 = miny;
    cam -> f.vx2 = maxx;
    cam -> f.vy2 = maxy;
    /* ---------- Set up visible tiles ----- */
    cam -> f.num_visi_tile = sdlgl3dGetVisiTiles(&cam -> f);

}

/*
 * Name:
 *     sdlgl3dIMoveAttachedCamera
 * Description:
 *     Moves the camera attached to an object, depending on type
 * Input:
 *      camera *:      Pointer on object to move. If NULL, move camera
 */
static void sdlgl3dIMoveAttachedCamera(SDLGL3D_CAMERA *camera)
{

    if (camera -> object) {

        /* If attached to an object, move it */
        if (camera -> type == SDLGL3D_CAMERATYPE_FIRSTPERSON) {

            /* Camera has same position as object */
            memcpy(&camera -> campos, camera -> object, sizeof(SDLGL3D_OBJECT));

        }
        else if (camera -> type == SDLGL3D_CAMERATYPE_THIRDPERSON) {

            

        }

    }

}


/*
 * Name:
 *     sdlgl3dIMoveSingleObj
 * Description:
 *     Moves the object in the given manner. Collision has to be
 * Input:
 *      moveobj *:     Pointer on object to move. If NULL, move camera
 *      secondspassed: Seconds passed since last call
 *      move_cmd:      Command to execute
 */
static void sdlgl3dIMoveSingleObj(SDLGL3D_OBJECT *moveobj, char move_cmd, float secondspassed)
{

    float speed;
    char  move_dir;


    /* Allways presume its a positive direction */
    move_dir = +1;
    switch(move_cmd) {

        case SDLGL3D_MOVE_BACKWARD:
            move_dir = -1;
        case SDLGL3D_MOVE_FORWARD:        
            speed = moveobj -> speed * move_dir * moveobj -> speed_modifier;
            /* Only in x/y plane */
            moveobj -> pos[0] += (moveobj -> direction[0] * speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> direction[1] * speed * secondspassed);
            break;

        case SDLGL3D_MOVE_LEFT:
            move_dir = -1;
        case SDLGL3D_MOVE_RIGHT:
            speed = moveobj -> speed * move_dir * moveobj -> speed_modifier;
            /* Only in x/y plane */
            moveobj -> pos[0] -= (moveobj -> direction[1] * speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> direction[0] * speed * secondspassed);
            break;

        case SDLGL3D_MOVE_DOWN:
            move_dir = -1;
        case SDLGL3D_MOVE_UP:
            speed = moveobj -> zspeed * move_dir;
            moveobj -> pos[SDLGL3D_Z] += (speed * secondspassed);
            break;

        case SDLGL3D_MOVE_TURNLEFT:
            move_dir = -1;
        case SDLGL3D_MOVE_TURNRIGHT:
            speed = moveobj -> turnvel * move_dir;
            /* Clockwise */
            moveobj -> rot[SDLGL3D_Z] -= (speed * secondspassed);
            if (moveobj -> rot[SDLGL3D_Z] < 0.0 ) {

                moveobj -> rot[SDLGL3D_Z] += 360.0;

            }
            else if (moveobj -> rot[SDLGL3D_Z]> 360.0) {

                moveobj -> rot[SDLGL3D_Z] -= 360.0;

            }

            moveobj -> direction[0] = sin(DEG2RAD(moveobj -> rot[SDLGL3D_Z]));
            moveobj -> direction[1] = cos(DEG2RAD(moveobj -> rot[SDLGL3D_Z]));
            break;
            
        case SDLGL3D_MOVE_3D:
            moveobj -> pos[0] += (moveobj -> direction[0] * moveobj -> speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> direction[1] * moveobj -> speed * secondspassed);
            moveobj -> pos[SDLGL3D_Z] += (moveobj -> zspeed * secondspassed);
            break;

        case SDLGL3D_MOVE_ROTX:
            speed = moveobj -> turnvel * move_dir;

            moveobj -> rot[0] += (speed * secondspassed);      /* Clockwise */
            if (moveobj -> rot[0] > 360.0) {

                moveobj -> rot[0] -= 360.0;

            }
            else if (moveobj -> rot[0] < 0.0) {

                moveobj -> rot[0] += 360.0;

            }
            break;

        case SDLGL3D_MOVE_ROTY:
            speed = moveobj -> turnvel * move_dir;

            moveobj -> rot[1] += (speed * secondspassed);      /* Clockwise */
            if (moveobj -> rot[1] > 360.0) {

                moveobj -> rot[1] -= 360.0;

            }
            else if (moveobj -> rot[1] < 0.0) {

                moveobj -> rot[1] += 360.0;

            }
            break;

        case SDLGL3D_MOVE_ZOOMOUT:
            Camera[0].f.viewwidth  += (5.0 * secondspassed);
            /* TODO: Set a maximum for zoom out */
            break;   
        case SDLGL3D_MOVE_ZOOMIN:
            /* Zoom in: Reduce size of view */
            Camera[0].f.viewwidth  -= (5.0 * secondspassed);
            if (Camera[0].f.viewwidth < SDLGL3D_I_VIEWMINWIDTH) {

                Camera[0].f.viewwidth = SDLGL3D_I_VIEWMINWIDTH;

            }
            break;
        
        case SDLGL3D_MOVE_CAMDIST:
            /* TODO: This one */
            break;

    }

    if (Camera[0].object == moveobj) {

        /* FIXME: Move camera according to movement of attached object */
        sdlgl3dIMoveAttachedCamera(&Camera[0]);
        /* If the camera was moved, set the frustum normals... */
        sdlgl3dSetupFrustumNormals(&Camera[0]);

        /* And recalculate the display list */

    }

}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */

/*
 * Name:
 *     sdlgl3dBegin
 * Description:
 *     Draws the whole view. Preserves the callers view state
 * Input:
 *     camera_no: Number of camera
 *     solid:     Draw it solid, true or false
 */
SDLGL3D_OBJECT *sdlgl3dBegin(int camera_no, int solid)
{

    SDLGL3D_OBJECT *viewobj;
    float viewwidth, viewheight;


    glMatrixMode(GL_PROJECTION);
    glPushMatrix();                 /* Save the callers View-Mode */
    glLoadIdentity();

    /* Set the view mode */
    viewwidth  = Camera[camera_no].f.viewwidth / 2;
    viewheight = (viewwidth * Camera[camera_no].f.aspect_ratio);
    
    glFrustum(viewwidth, -viewwidth, -viewheight, viewheight, Camera[camera_no].f.zmin, Camera[camera_no].f.zmax);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (Camera[camera_no].type == SDLGL3D_CAMERATYPE_FIRSTPERSON) {

        viewobj = Camera[camera_no].object;
        if (viewobj == 0) {

            /* Play it save */
            viewobj = &Camera[camera_no].campos;

        }


    }
    else {

        viewobj = &Camera[camera_no].campos;

    }

    glRotatef(viewobj -> rot[0], 1.0, 0.0, 0.0);
    glRotatef(viewobj -> rot[1], 0.0, 1.0, 0.0);
    glRotatef(viewobj -> rot[2], 0.0, 0.0, 1.0);
    glTranslatef(-viewobj -> pos[0], -viewobj -> pos[1], -viewobj -> pos[2]);

    /* Now we're ready to draw the grid */
    glPolygonMode(GL_FRONT, solid ? GL_FILL : GL_LINE);

    glFrontFace(GL_CW);

    return viewobj;

}

/*
 * Name:
 *     sdlgl3dEnd
 * Description:
 *     Ends 3D-Mode. Restores the callers view state
 * Input:
 *     None
 */
void sdlgl3dEnd(void)
{

    /* Restore the users screen settings */
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

}

/*
 * Name:
 *      sdlgl3dAttachCameraToObj
 * Description:
 *     Attaches the camera to the given object.
 * Input:
 *     object_no: Number of object to attach the camera to
 *     camtype:   Type of camera to attach: SDLGL3D_CAMERATYPE_*
 */
void sdlgl3dAttachCameraToObj(int object_no, char camtype)
{

    float distance;


    Camera[0].object = &Object_List[object_no];    /* Attach object to camera */

    if (camtype == SDLGL3D_CAMERATYPE_FIRSTPERSON) {

        Camera[0].cameradist = 0;

    }
    else {

        /* Create a third person camera behind the given 'moveobj' */
        distance = 2 * SDLGL3D_TILESIZE;
        Camera[0].cameradist = distance;

        Camera[0].campos.pos[0] = Camera[0].object -> pos[0]
                                  - (Camera[0].object -> direction[0] * distance);
        Camera[0].campos.pos[1] = Camera[0].object -> pos[1]
                                  - (Camera[0].object -> direction[1] * distance);
        Camera[0].campos.pos[2] = SDLGL3D_TILESIZE;

        Camera[0].campos.rot[2] = 30.0;

    }

    Camera[0].type = camtype;

    /* Set the frustum normals... */
    sdlgl3dSetupFrustumNormals(&Camera[0]);

}

/*
 * Name:
 *     sdlgl3dInitCamera
 * Description:
 *     Initializes the 3D-Camera. Moves the camera to given position with
 *     given rotations.
 * Input:
 *      camera_no:        Initialize this camera  
 *      rotx, roty, rotz: Roatation in each axis
 *      aspect_ratio:     Of screen for 3D-View   
 */
void sdlgl3dInitCamera(int camera_no, int rotx, int roty, int rotz, float aspect_ratio)
{

    Camera[camera_no].campos.pos[0] = 0;
    Camera[camera_no].campos.pos[1] = 0;
    Camera[camera_no].campos.pos[SDLGL3D_Z] = 0;

    Camera[camera_no].campos.rot[0] = rotx;
    Camera[camera_no].campos.rot[1] = roty;
    Camera[camera_no].campos.rot[SDLGL3D_Z] = rotz;

    Camera[camera_no].campos.direction[0] = sin(DEG2RAD(rotz));
    Camera[camera_no].campos.direction[1] = cos(DEG2RAD(rotz));

    Camera[camera_no].campos.speed   = 300.0;  /* Speed of camera in units / second    */
    Camera[camera_no].campos.turnvel =  60.0;  /* Degrees per second                   */
    
    Camera[camera_no].f.aspect_ratio = aspect_ratio;
    Camera[camera_no].f.fov = SDLGL3D_I_CAMERA_FOV; /* TODO: To be set by caller.. */
    /* If the camera was moved, set the frustum normals... */
    sdlgl3dSetupFrustumNormals(&Camera[camera_no]);

}

/*
 * Name:
 *     sdlgl3dBindCamera
 * Description:
 *     Sets the maximum movement in x,y- direction for the camera 
 * Input:
 *      camera_no:        Set bounding for this camera 
 *      x, ,y, x2, y2:       Position of camera  
 */
void sdlgl3dBindCamera(int camera_no, float x, float y, float x2, float y2)
{

    Camera[camera_no].bound = 1;
    Camera[camera_no].bx    = x;
    Camera[camera_no].by    = y;
    Camera[camera_no].bx2   = x2;
    Camera[camera_no].by2   = y2;
    
}

/*
 * Name:
 *     sdlgl3dGetCameraInfo
 * Description:
 *     Initializes the 3D-Camera. Moves the camera to given position with
 *     given rotations.
 * Input:
 *     camera_no:  Number of camera to get info for
 *     f *:        Where to return the data about hte frustum
 * Output:
 *      Pointer on camera position
 */
SDLGL3D_OBJECT *sdlgl3dGetCameraInfo(int camera_no, SDLGL3D_FRUSTUM *f)
{

    memcpy(f, &Camera[camera_no].f, sizeof(SDLGL3D_FRUSTUM));

    return &Camera[camera_no].campos;

}

/*
 * Name:
 *     sdlgl3dInitObject
 * Description:
 *     Initializes the given object for the use in sdlgl3d. It's assumed that
 *     the object has filled in the position and so on.
 *     Including speed.
 * Input:
 *     moveobj *: Pointer on object to initialize
 */
void sdlgl3dInitObject(SDLGL3D_OBJECT *moveobj)
{

    /* ------- Create the direction vector -------- */
    moveobj -> direction[0] = sin(DEG2RAD(moveobj -> rot[2]));
    moveobj -> direction[1] = cos(DEG2RAD(moveobj -> rot[2]));

}

/*
 * Name:
 *     sdlgl3dManageCamera
 * Description:
 *     Sets the commands for object with given number. < 0: Is camera
 * Input:
 *      camera_no: Number of camera to manage
 *      move_cmd:  Kind of movement
 *      set:       Set it, yes no
 *      speed_modifier: Multiply movement speed with this one   
 */
void sdlgl3dManageCamera(int camera_no, char move_cmd, char set, char speed_modifier)
{

    int flag;


    if (move_cmd == SDLGL3D_MOVE_STOPMOVE) {
        /* Stop all movement */
        Camera[camera_no].campos.move_cmd = 0;
        return;
    }

    flag = (1 << move_cmd);
    if (set) {
        Camera[camera_no].campos.move_cmd |= flag;
        if (speed_modifier < 1) {
            speed_modifier = 1;
        }
        Camera[camera_no].campos.speed_modifier = speed_modifier;
    }
    else {
        Camera[camera_no].campos.move_cmd &= ~flag;
    }

}

/*
 * Name:
 *     sdlgl3dMoveToPosCamera
 * Description:
 *      Moves the camera to given position x/y/z
 * Input:
 *      camera_no: Number of camera to set to position
 *      x, y, z:   Position of camera to move to
 *      relative:  Move relative to actual position
 */
void sdlgl3dMoveToPosCamera(int camera_no, float x, float y, float z, int relative)
{

    float cameradist;


    if (relative) {        
        cameradist = Camera[camera_no].cameradist;
        /* Calculate now position for camera */
        Camera[camera_no].campos.pos[0] = x;
        Camera[camera_no].campos.pos[1] = y;
        /* TODO: Move back 'cameradist' from chosen position */
        Camera[camera_no].campos.pos[0] -= Camera[camera_no].campos.direction[0] * cameradist;
        Camera[camera_no].campos.pos[1] -= Camera[camera_no].campos.direction[1] * cameradist;

    }
    else {
        Camera[camera_no].campos.pos[0] = x;
        Camera[camera_no].campos.pos[1] = y;
        Camera[camera_no].campos.pos[SDLGL3D_Z] = z;
    }
    /* Adjust the frustum normals */
    sdlgl3dSetupFrustumNormals(&Camera[camera_no]);

}

/*
 * Name:
 *     sdlgl3dManageObject
 * Description:
 *     Sets the commands for object with given number. < 0: Is camera
 * Input:
 *      object_no: Number of object to manage
 *      move_cmd:  Kind of movement
 *      set:       Set Command yes/no
 */
void sdlgl3dManageObject(int object_no, char move_cmd, char set)
{

    int flag;


    if (object_no > 0) {

        if (move_cmd == SDLGL3D_MOVE_STOPMOVE) {
            /* Stop all movement */
           Object_List[object_no].move_cmd = 0;
           return;
        }
        /* Manage this object. Set command */
        flag = (1 << move_cmd);
        if (set) {
            Object_List[object_no].move_cmd |= flag;
        }
        else {
            Object_List[object_no].move_cmd &= ~flag;
        }

    }

}

/*
 * Name:
 *     sdlgl3dMoveObjects
 * Description:
 *     Moves all objects
 *     TODO: Callback of user if collision happens
 * Input:
 *      move_cmd:      Kind of movement
 *      secondspassed: Seconds passed since last call
 */
void sdlgl3dMoveObjects(float secondspassed)
{

    SDLGL3D_OBJECT *objects;
    int  flags;
    char move_cmd;


    objects = Object_List;

    while(objects -> obj_type) {

        if (objects -> move_cmd) {

            for (move_cmd = 1, flags = 0x02; move_cmd < SDLGL3D_MOVE_MAXCMD; move_cmd++, flags <<= 1) {

                if (flags & objects -> move_cmd) {
                    /* Move command is active */
                    sdlgl3dIMoveSingleObj(objects, move_cmd, secondspassed);

                }

            }

        }

        objects++;

    }

    /* TODO: Move each camera which is active -- Add an 'active'-flag*/
    if (Camera[0].campos.move_cmd > 0) {

        /* FIXME: Take into account if camera is 'attached' as 3rd-Person camera */
        for (move_cmd = 1, flags = 0x02; move_cmd < SDLGL3D_MOVE_MAXCMD; move_cmd++, flags <<= 1) {

            if (flags & Camera[0].campos.move_cmd) {
                /* Move command is active */  
                sdlgl3dIMoveSingleObj(&Camera[0].campos, move_cmd, secondspassed);

            }

        }

        /* If the camera was moved, set the frustum normals... */
        sdlgl3dSetupFrustumNormals(&Camera[0]);

        if (Camera[0].bound) {
            /* TODO: Do not bind if attached to an object */
            if (Camera[0].campos.pos[0] < Camera[0].bx) {
                Camera[0].campos.pos[0] = Camera[0].bx;
            }
            if (Camera[0].campos.pos[1] < Camera[0].by) {
                Camera[0].campos.pos[1] = Camera[0].by;
            }
            if (Camera[0].campos.pos[0] > Camera[0].bx2) {
                Camera[0].campos.pos[0] = Camera[0].bx2;
            }
            if (Camera[0].campos.pos[1] > Camera[0].by2) {
                Camera[0].campos.pos[1] = Camera[0].by2;
            }
        
        }

    }

}

/*
 * Name:
 *     sdlgl3dInitVisiMap
 * Description:
 *     To check visibility by map squares, the visibility infos have to be handed over 
 * Input:
 *      map_w, map_h: Size of map in tiles 
 *      tile_size:    Size of single tile in units 
 */
void sdlgl3dInitVisiMap(int map_w, int map_h, float tile_size)
{

    MapW     = map_w;
    MapH     = map_h;
    TileSize = tile_size;      /* For tiles visible in FOV */
    
}

/*
 * Name:
 *     sdlgl3dGetVisiTileList
 * Description:
 *     Returns the list of tiles visible in FOV 
 * Input:
 *      num_tile *:   Where to return the number of tiles in list   
 * Output:
 *      Pointer on list of visible tiles
 */
SDLGL3D_VISITILE *sdlgl3dGetVisiTileList(int *num_tile)
{
    
    *num_tile = NumVisiTiles;
    
    return &Visi_Tiles[0];
    
}