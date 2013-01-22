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

#define SDLGL3D_MAX_CAMERA     4
#define SDLGL3D_CAMERA_EXTENT  15.0
#define SDLGL3D_I_CAMERA_FOV   90.0

/* ------- */
#define SDLGL3D_I_MAXVISITILES 128

#define SDLGL3D_I_VIEWMINWIDTH  60.0    /* For zoom...              */
#define SDLGL3D_I_VIEWWIDTH     96.0
#define SDLGL3D_I_ASPECTRATIO   0.75    /* Handed over by caller    */

#define SDLGL3D_I_ZMIN   48.0
#define SDLGL3D_I_ZMAX 1385.0

/* Internal defines */
#define NO_CLIP     0x00
#define SOME_CLIP   0x01
#define NOT_VISIBLE 0x02

#define SDLGL3D_OBJCAM   0x70       /* Type of object                        */
#define SDLGL3D_FIRSTOBJ 5          /* Objects 1,,4 are reserved for cameras */
#define SDLGL3D_MAXOBJ   1024

/*******************************************************************************
* TYPEDEFS							                                        *
*******************************************************************************/

typedef struct
 {
    char mode;                  /* Type of camera and size of frustum for zooming */
                                /* SDLGL3D_CAM_INACTIVE: Not active               */  
    SDLGL3D_FRUSTUM f;          /* Frustum data, including info for visibility    */

    float cam_dist;             /* Distance behind object for third person camera */
    int   map_w, map_h;         /* Size of Map in tiles                           */
    float tile_size;            /* Size of tile square                            */
    // @todo: Camera is attached to object, behaves depending on 'mode'              */
    SDLGL3D_OBJECT *obj;        /* The object the camera is attached to           */
                                /* Can be camera itself ('campos')                */
    SDLGL3D_OBJECT campos;
    /* Frustum data for calculation of visibility */
    char  bound;                /* Camera is bound to an x,y-rectangle            */
    float bx, by, bx2, by2;
    float mx, my, mx2, my2;     /* Bounding rectangle of FOV (min, max)           */ 
    /* --- Position to look at (fixed) --- */
    SDLGL3D_V3D look_at;

} SDLGL3D_CAMERA;

/*****************************************************************************
* DATA								                                         *
*****************************************************************************/

/* This are the cameras. 0: Standard camera */
static SDLGL3D_CAMERA Camera[SDLGL3D_MAX_CAMERA] =
{
    { SDLGL3D_CAM_MODESTD,
        {
            SDLGL3D_I_CAMERA_FOV,           /* Field of view               */
            SDLGL3D_I_VIEWWIDTH,
            SDLGL3D_I_ASPECTRATIO,          /* Aspect ration of view-plane */
            SDLGL3D_I_ZMIN, SDLGL3D_I_ZMAX,
        },
        800.0,
        /* ---- Additional info to calculate visbility on base of tiles */
        32, 32,
        128
    }
};

/* Additional objects are used for visible tiles */
static int NumVisiTiles = 0;
static SDLGL3D_VISITILE Visi_Tiles[SDLGL3D_I_MAXVISITILES + 2];
static SDLGL3D_OBJECT Obj3D[SDLGL3D_MAXOBJ + 2];    /* Maximum number of visible objects */

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
 *     cam *: Pointer on camera with  frustum
 * Output:
 *     Number of visible tiles in frustum
 */
static int sdlgl3dGetVisiTiles(SDLGL3D_CAMERA *cam)
{

    /*
    int tx1, ty1, tx2, ty2, ltx;
    int w_add[2], h_add[2];
    */


    NumVisiTiles = 0;

    /*
        TODO: New function to get the tile-numbers:
        From left to right, from back to front   
     */
    /*
    tx1 = cam -> f.bx[0] / cam -> tile_size;
    tx2 = cam -> f.bx[3] / cam -> tile_size;
    ty1 = cam -> f.by[0] / cam -> tile_size;
    ty2 = cam -> f.by[3] / cam -> tile_size;

    while(ty1 <= ty2) {
        ltx = tx1;
        while(ltx <= tx2) {

            Visi_Tiles[NumVisiTiles].no    = (ty1 * cam -> map_w) + ltx;
            Visi_Tiles[NumVisiTiles].mid_x = (ltx * cam -> tile_size) + 64;
            Visi_Tiles[NumVisiTiles].mid_y = (ty1 * cam -> tile_size) + 64;
            NumVisiTiles++;
            if (NumVisiTiles >= (SDLGL3D_I_MAXVISITILES - 1)) {
                Visi_Tiles[NumVisiTiles].no = -1;    // Sign end of list
                return NumVisiTiles;
            }
            ltx++;
        }
        ty1++;
    }
    */

    Visi_Tiles[NumVisiTiles].tile_no = -1;      /* Sign end of array */

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
    float povx, povy;
    float vx_down, vy_down;


    obj = cam -> obj;

    if (obj == 0) {

        /* Play it save */
        obj = &cam -> campos;

    }

    rotz = obj -> rot[2];

    /* --- Third point is the cameras position */
    povx = obj -> pos[0];
    povy = obj -> pos[1];

    cam -> f.rightangle = rotz - cam -> f.fov / 2;
    cam -> f.leftangle  = rotz + cam -> f.fov / 2;

    cam -> f.nx[0] = sin(DEG2RAD(cam -> f.rightangle));
    cam -> f.ny[0] = cos(DEG2RAD(cam -> f.rightangle));

    cam -> f.nx[1] = sin(DEG2RAD(cam -> f.leftangle));
    cam -> f.ny[1] = cos(DEG2RAD(cam -> f.leftangle));

    cam -> f.nx[2] = sin(DEG2RAD(rotz));
    cam -> f.ny[2] = cos(DEG2RAD(rotz));

    /* -- Calculate the bounding rectangle for calculation of visible tiles -- */
    /* assume FOV = 90 degrees TODO: Calculate with any angle                  */
    /* --- Edges of backplane: Left / Right --- */
    cam -> f.bx[0] = povx + (cam -> f.nx[0] * cam -> f.zmax * 1.5);
    cam -> f.by[0] = povy + (cam -> f.ny[0] * cam -> f.zmax * 1.5);
    cam -> f.bx[1] = povx + (cam -> f.nx[1] * cam -> f.zmax * 1.5);
    cam -> f.by[1] = povy + (cam -> f.ny[1] * cam -> f.zmax * 1.5);
    vx_down = -cam -> f.nx[2] * cam -> f.zmax;
    vy_down = -cam -> f.ny[2] * cam -> f.zmax;
    /* --- Edges of front plane, left / right --- */
    cam -> f.bx[2] = cam -> f.bx[0] + vx_down;
    cam -> f.by[2] = cam -> f.by[0] + vy_down;
    cam -> f.bx[3] = cam -> f.bx[1] + vx_down;
    cam -> f.by[3] = cam -> f.by[1] + vy_down;

    /* ---------- Set up visible tiles ----- */
    cam -> f.num_visi_tile = sdlgl3dGetVisiTiles(cam);

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
    float dist_add;
    char  move_dir;


    /* Allways presume its a positive direction */
    move_dir = +1;

    switch(move_cmd) {

        case SDLGL3D_MOVE_BACKWARD:
            move_dir = -1;
        case SDLGL3D_MOVE_FORWARD:
            speed = moveobj -> speed * move_dir * moveobj -> speed_modifier;
            /* Only in x/y plane */
            moveobj -> pos[0] += (moveobj -> dir[0] * speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> dir[1] * speed * secondspassed);
            break;

        case SDLGL3D_MOVE_LEFT:
            move_dir = -1;
        case SDLGL3D_MOVE_RIGHT:
            speed = moveobj -> speed * move_dir * moveobj -> speed_modifier;
            /* Only in x/y plane */
            moveobj -> pos[0] -= (moveobj -> dir[1] * speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> dir[0] * speed * secondspassed);
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

            moveobj -> dir[0] = sin(DEG2RAD(moveobj -> rot[SDLGL3D_Z]));
            moveobj -> dir[1] = cos(DEG2RAD(moveobj -> rot[SDLGL3D_Z]));
            break;
            
        case SDLGL3D_MOVE_3D:
            moveobj -> pos[0] += (moveobj -> dir[0] * moveobj -> speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> dir[1] * moveobj -> speed * secondspassed);
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
            /* TODO: Adjust far plane, Set a maximum for zoom out */
            /* Do zoom by changing of distance */
            dist_add = (100.0 * secondspassed);
            if (moveobj -> pos[2] < 600.0) {

                moveobj -> pos[0] += (dist_add * moveobj -> dir[0]);
                moveobj -> pos[1] += (dist_add * moveobj -> dir[1]);
                moveobj -> pos[2] += (dist_add * moveobj -> dir[2]);

            }
            break;
        case SDLGL3D_MOVE_ZOOMIN:
            /* Do zoom by changing of distance */
            dist_add = (100.0 * secondspassed);
            if (moveobj -> pos[2] > 200.0) {

                moveobj -> pos[0] -= (dist_add * moveobj -> dir[0]);
                moveobj -> pos[1] -= (dist_add * moveobj -> dir[1]);
                moveobj -> pos[2] -= (dist_add * moveobj -> dir[2]);

            }
            break;

        case SDLGL3D_MOVE_CAMDIST:
            // @todo
            break;
            // @todo: Add further flags for particle movement

    }

}

/*
 * Name:
 *     sdlgl3dITilesOnLine
 * Description:
 *     Fills in the List in frustum with the numbers of tiles hit by the
 *     mouse-ray
 * Input:
 *      camera_no: For this camera
 */
static void sdlgl3dITilesOnLine(int camera_no)
{
    SDLGL3D_OBJECT *obj;
    SDLGL3D_CAMERA *cam;
    int x1, y1, x2, y2;
    int tile_no;
    float far_dist;
    /* ---- Get tile-numbers with bresenhams algorhytm ---- */
    int dx, dy;
    int ix, iy;
    int err, i;


    cam = &Camera[camera_no];
    obj = cam -> obj;

    x1 = obj -> pos[0] / cam -> tile_size;
    y1 = obj -> pos[1] / cam -> tile_size;

    far_dist = (cam -> f.zmax + cam -> tile_size) / cam -> tile_size;
    x2 = x1 + (cam -> f.m_ray2d[0] * far_dist);
    y2 = y1 + (cam -> f.m_ray2d[1] * far_dist);

    /* difference between starting and ending points    */
	dx = x2 - x1;
	dy = y2 - y1;

    // calculate direction of the vector and store in ix and iy
	if (dx >= 0) {
		ix = 1;
    }
    else {
		ix = -1;
		dx = abs(dx);
	}

    if (dy >= 0) {
		iy = 1;
    }
    else {
		iy = -1;
		dy = abs(dy);
	}

    tile_no = 0;

    if (dx > dy) {  /* dx is the major axis */

		err = dy - dx;

		for (i = 0; i <= dx; i++) {

            cam -> f.mou_tiles[tile_no] = (y1 * cam -> map_w) + x1;
            tile_no++;

			if (err >= 0) {
				err -= dx;
				y1 += iy;
			}
			err += dy;
			x1 += ix;
		}
	}
    else {  /* dy is the major axis */

		err = dx - dy;

		for (i = 0; i <= dy; i++) {

			cam -> f.mou_tiles[tile_no] = (y1 * cam -> map_w) + x1;
            tile_no++;

			if (err >= 0) {
				err -= dy;
				x1 += ix;
			}
			err += dx;
			y1 += iy;
		}
	}

    cam -> f.mou_tiles[tile_no] = -1;    /* Sign end of list */

}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */

/*
 * Name:
 *     sdlgl3dBegin
 * Description:
 *     Starts the 3D-Draw-Mode. Sets the projection matrix and the matrix for 
 *     given camera. 
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

    viewobj = Camera[camera_no].obj;
    
    if (viewobj == 0)
    {
        /* Play it save */
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
 *      sdlgl3dSetCameraMode
 * Description:
 *     Sets the camera mode and attaches given object or set given position as lookat 
 *     Attaches the camera to the given object.
 * Input:
 *     camera_no: Number of camera to attach to object
 *     mode:      Mode to set for this camera
 *     obj *:     Pointer on object to attach camera to
 *     x, y:      Position to look at if this move
 *     camtype:   Type of camera to attach: SDLGL3D_CAMERATYPE_*
 */
void sdlgl3dSetCameraMode(int camera_no, char mode, SDLGL3D_OBJECT *obj, float x, float y)
{

    SDLGL3D_CAMERA *cam;


    cam = &Camera[camera_no];
    
    switch(mode) {
    
        case SDLGL3D_CAM_MODESTD:
            /* Move camera itself           */
            break;                
        
        case SDLGL3D_CAM_FOLLOW:
            /* Follow an object             */
            cam -> obj = obj;
            /* Attach object to camera */        
            /* Create a third person camera behind the given 'moveobj' */
            cam -> cam_dist = cam -> tile_size * 3;

            cam -> campos.pos[0] = cam -> obj -> pos[0]
                                   - (cam -> obj -> dir[0] * cam -> cam_dist);
            cam -> campos.pos[1] = cam -> obj -> pos[1]
                                   - (cam -> obj -> dir[1] * cam -> cam_dist);
            cam -> campos.pos[2] = cam -> tile_size;
            cam -> campos.rot[2] = cam -> obj -> rot[2];
            break;
        case SDLGL3D_CAM_LOOKAT:
            /* Attached to a position       */
            cam -> look_at[0] = x;
            cam -> look_at[1] = y;
            cam -> cam_dist = cam -> tile_size * 3;
            break;
            
        case SDLGL3D_CAM_FIRSTPERS:
            /* Move to position of object   */
            cam -> cam_dist = 0;
            break;
            
        default:
            return;
    
    }

    cam -> mode = mode;

    /* Set the frustum normals... */
    sdlgl3dSetupFrustumNormals(cam);

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

    SDLGL3D_CAMERA *cam;


    cam = &Camera[camera_no];

    cam -> campos.pos[0] = 0;
    cam -> campos.pos[1] = 0;
    cam -> campos.pos[SDLGL3D_Z] = 0;

    cam -> campos.rot[0] = rotx;
    cam -> campos.rot[1] = roty;
    cam -> campos.rot[SDLGL3D_Z] = rotz;

    cam -> campos.dir[0] = sin(DEG2RAD(rotz));
    cam -> campos.dir[1] = cos(DEG2RAD(rotz));

    cam -> campos.speed   = 300.0;  /* Speed of camera in units / second    */
    cam -> campos.turnvel =  60.0;  /* Degrees per second                   */

    cam -> f.aspect_ratio = aspect_ratio;
    cam -> f.fov = SDLGL3D_I_CAMERA_FOV; /* TODO: To be set by caller.. */

    cam -> obj = &cam -> campos;

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

    return Camera[camera_no].obj;

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

    SDLGL3D_CAMERA *cam;
    SDLGL3D_OBJECT *obj;
    float cam_dist;


    cam = &Camera[camera_no];
    obj = cam -> obj;
    
    if (relative) {        
        cam_dist = cam -> cam_dist;
        /* Calculate now position for camera */
        obj -> pos[0] = x;
        obj -> pos[1] = y;
        /* TODO: Move back 'cam_dist' from chosen position */
        obj -> pos[0] -= obj -> dir[0] * cam_dist;
        obj -> pos[1] -= obj -> dir[1] * cam_dist;

    }
    else {
        obj -> pos[0] = x;
        obj -> pos[1] = y;
        obj -> pos[SDLGL3D_Z] = z;
    }

    /* Adjust the frustum normals */
    sdlgl3dSetupFrustumNormals(cam);

}


/*
 * Name:
 *     sdlgl3dMoveCamera
 * Description:
 *     Moves the camera(s) on map
 * Input:
 *     secondspassed: Since last call
 */
void sdlgl3dMoveCamera(float secondspassed)
{

    int  flags;
    char move_cmd;


    /* @todo: Move each camera which is active -- Add an 'active'-flag*/
    /* Move camera based on camera mode */
    switch(Camera[0].mode) {

         case SDLGL3D_CAM_FOLLOW:
            /* TODO: Follow an object attached to camera    */
            /* break; */

        case SDLGL3D_CAM_LOOKAT:
            /* TODO: Attached to a position         */
            /* break; */
        case SDLGL3D_CAM_FIRSTPERS:
            /* TODO: Camera at position of object   */
            /* break; */

        case SDLGL3D_CAM_MODESTD:
            /* Move camera itself                   */
            if (Camera[0].campos.move_cmd > 0) {

                 /* If the camera was moved, set the frustum normals... */
                for (move_cmd = 1, flags = 0x02; move_cmd < SDLGL3D_MOVE_MAXCMD; move_cmd++, flags <<= 1) {

                    if (flags & Camera[0].campos.move_cmd) {
                        /* Move command is active */

                        sdlgl3dIMoveSingleObj(&Camera[0].campos, move_cmd, secondspassed);

                    }

                }

            }

            if (Camera[0].bound) {
                /* TODO: Do not bind if attached to an object */
                if (Camera[0].obj -> pos[0] < Camera[0].bx) {
                    Camera[0].obj -> pos[0] = Camera[0].bx;
                }
                if (Camera[0].obj -> pos[1] < Camera[0].by) {
                    Camera[0].obj -> pos[1] = Camera[0].by;
                }
                if (Camera[0].obj -> pos[0] > Camera[0].bx2) {
                    Camera[0].obj -> pos[0] = Camera[0].bx2;
                }
                if (Camera[0].obj -> pos[1] > Camera[0].by2) {
                    Camera[0].obj -> pos[1] = Camera[0].by2;
                }

            }

            sdlgl3dSetupFrustumNormals(&Camera[0]);
            break;

    }

}

/* ===== Object-Functions ==== */

/*
 * Name:
 *     sdlgl3dCreateObject
 * Description:
 *     Creates an object using the data set in 'info_obj'. 
 *     Sets its first movement command. Usually 'SDLGL3D_MOVE_3D' for 'auto-movement', e.g. particles 
 * Input:
 *     info_obj *: Pointer on object-data to initialize new object with
 *     move_cmd:   >0: Movement command to set at creation time  
 * Output:
 *     Number of the object created, if successful
 */
int sdlgl3dCreateObject(SDLGL3D_OBJECT *info_obj, char move_cmd)
{
    int i;
    SDLGL3D_OBJECT *new_obj;
    
    
    // Slots 1..4 are reserved for the cameras
    for(i = SDLGL3D_FIRSTOBJ; i < SDLGL3D_MAXOBJ; i++)
    {
        if(Obj3D[i].id <= 0)
        {
            // We found an empty slot
            new_obj = &Obj3D[i];
            
            // Copy initalizing code into object
            memcpy(new_obj, info_obj, sizeof(SDLGL3D_OBJECT));
            // Sign it as active
            new_obj -> id = i;
            
            // Set movement command, if any
            if(move_cmd > 0)
            {
                // Set its basic movement command (
                new_obj -> move_cmd = (1 << move_cmd);
            }
            
            /* ------- Create the direction vector -------- */
            new_obj -> dir[0] = sin(DEG2RAD(new_obj -> rot[2]));
            new_obj -> dir[1] = cos(DEG2RAD(new_obj -> rot[2]));
            // Is a valid object
            return i;
        }
    }
    // No free slot found
    return 0;
}

/*
 * Name:
 *     sdlgl3dDeleteObject
 * Description:
 *     Deletes object with given number, signs it as free slot 
 * Input:
 *     obj_no: Number of object to delete (< 0: Clear all objects)
 */
void sdlgl3dDeleteObject(int obj_no)
{
    if(obj_no < 0)
    {
        memset(&Obj3D[0], 0, sizeof(SDLGL3D_OBJECT) * SDLGL3D_MAXOBJ);
    }
    else if(obj_no > 0 && obj_no < SDLGL3D_MAXOBJ)
    {
        memset(&Obj3D[obj_no], 0, sizeof(SDLGL3D_OBJECT));
        // Marks as deleted
        Obj3D[obj_no].id = -1;
    }
}

/*
 * Name:
 *     sdlgl3dGetObject
 * Description:
 *     Returns a pointer on the object with given number.
 *     - If the object number is 0, a pointer on the first valid object is returned
 *     - If the object number is invalid, a pointer on the object 0 is returned 
 * Input:
 *     obj_no: Number of object to get pointer for
 * Output:
 *     Pointer on valid object 
 */
SDLGL3D_OBJECT *sdlgl3dGetObject(int obj_no)
{
    if(obj_no <= 1)
    {
        return &Obj3D[SDLGL3D_FIRSTOBJ];
    }
    
    if(obj_no >= SDLGL3D_FIRSTOBJ && obj_no < SDLGL3D_MAXOBJ)
    {
        // Return pointer on this object
        return &Obj3D[obj_no];    
    }
    
    // Pointer on empty object
    return &Obj3D[0];
}

/*
 * Name:
 *     sdlgl3dManageObject
 * Description:
 *     Sets the commands for object with given number. < 0: Is camera
 * Input:
 *      obj_no:    Number of object to manage
 *      move_cmd:  Kind of movement
 *      set:       Set Command yes/no
 */
void sdlgl3dManageObject(int obj_no, char move_cmd, char set)
{
    int flag;


    if (Obj3D[obj_no].obj_type > 0)
    {
        if (move_cmd == SDLGL3D_MOVE_STOPMOVE)
        {
            // Stop all movement
            Obj3D[obj_no].move_cmd = 0;
            return;
        }
        
        /* Manage this object. Set command */
        flag = (1 << move_cmd);
        
        if (set)
        {
            Obj3D[obj_no].move_cmd |= flag;
        }
        else
        {
            Obj3D[obj_no].move_cmd &= ~flag;
        }
    }
}

/*
 * Name:
 *     sdlgl3dMoveObjects
 * Description:
 *     Moves all objects in the internal object list
 * Input:      
 *      secondspassed: Seconds passed since last call
 */
void sdlgl3dMoveObjects(float secondspassed)
{
    SDLGL3D_OBJECT *obj_list;
    int  flags;
    char move_cmd;


    // Get pointer on first object
    obj_list = &Obj3D[SDLGL3D_FIRSTOBJ];

    while(obj_list -> id)
    {
        if (obj_list -> id > 0)
        {
            if(obj_list -> move_cmd > 0)
            {
                // If moving
                for (move_cmd = 1, flags = 0x02; move_cmd < SDLGL3D_MOVE_MAXCMD; move_cmd++, flags <<= 1)
                {
                    if (flags & obj_list -> move_cmd)
                    {
                        /* Move command is active */
                        sdlgl3dIMoveSingleObj(obj_list, move_cmd, secondspassed);
                    }
                }
            }

            // Do animation data for 'non-moving' objects, too
            if(obj_list->anim_speed > 0.0 && obj_list->base_frame < obj_list->end_frame)
            {
                // Animation is active and has more then one frame
                obj_list->anim_clock -= secondspassed;

                if(obj_list->anim_clock <= 0.0)
                {
                    obj_list->cur_frame++;
                    // @todo: Check if animation looping, support random frames
                    if(obj_list->cur_frame > obj_list->end_frame)
                    {
                        // Wrap around
                        obj_list->cur_frame = obj_list->base_frame;
                    }
                    // Set next countdown
                    obj_list->anim_clock += obj_list->anim_speed;
                }
            }
        }   

        obj_list++;
    }
}

/*
 * Name:
 *     sdlgl3dObjectList
 * Description:
 *     Manage linked list(s) with object numbers
 * Input:
 *      from *: Pointer on list base where to move 'from'
 *      to *:   Pointer on list base where to move 'to
 *      obj_no: Number of object to handle
 *      action: SDLGL3D_ADD_TO: Add obj_no 'to' list
 *              SDLGL3D_REMOVE_FROM: Remove obj_no 'from' list
 *              SDLGL3D_MOVE_FROM_TO: Move object 'from' => 'to'
 */
char sdlgl3dObjectList(int *from, int *to, int obj_no, int action)
{
    SDLGL3D_OBJECT *pactual;
    int oldbase;
    int act_obj_no;
    char found;


    switch(action)
    {
        case SDLGL3D_REMOVE_FROM:
        case SDLGL3D_MOVE_FROM_TO:
            if (NULL == *from)
            {
                // Invalid pointer
                return 0;
            }

            //
            act_obj_no = *from;
            found = 0;

            do
            {
                pactual = &Obj3D[act_obj_no];

                if (act_obj_no == obj_no)
                {
                    found = 1;
                    // found it, remove it
                    (*from) = pactual -> next_obj;
                    pactual -> next_obj = 0;         // Set next to zero
                    break;
                }

                from = &pactual -> next_obj;    // Where to attach next, if
                                                // actual object_no is removed
                act_obj_no = pactual -> next_obj;

            }
            while (act_obj_no);

            if (action == SDLGL3D_REMOVE_FROM || !found)
            {
                // Only remove 'obj_no' from List, or nothing to add 'to'
                break;
            }
        case SDLGL3D_ADD_TO:
            if (NULL == *to)
            {
                // Invalid pointer
                return 0;
            }

            oldbase = (*to);    // Save posible actual 'obj_no' for attachment
                                // If none available, is anyway 0!
            (*to) = obj_no;     // Insert new 'obj_no' at base
            // Attach previous object(s) to base
            Obj3D[obj_no].next_obj = oldbase;
    }

    return 0;
}


/* ===== Visibility-Functions ==== */

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

    int i;


    for (i = 0; i < SDLGL3D_MAX_CAMERA; i++) {

        Camera[i].map_w     = map_w;
        Camera[i].map_h     = map_h;
        Camera[i].tile_size = tile_size;

    }
}

/*
 * Name:
 *     sdlgl3dMouse
 * Description:
 *     Fills in the data in the frustum structure for given camera with
 *     values for visibility checking.
 *     The info generated can be fetched by 'sdlgl3dGetCameraInfo()'
 * Input:
 *      camera_no:    Use this camera
 *      scrw, scrh:   Extent of screen
 *      moux, mouy:   Position of mouse
 */
void sdlgl3dMouse(int camera_no, int scrw, int scrh, int moux, int mouy)
{
    SDLGL3D_OBJECT *obj;
    SDLGL3D_CAMERA *cam;
    SDLGL3D_V3D m_ray;
    float left, top, vh;
    float mx, my;
    float angle;


    cam = &Camera[camera_no];

    left  = -(cam -> f.viewwidth / 2);
    vh    = cam -> f.viewwidth * cam -> f.aspect_ratio;
    top   = -(vh / 2);

    mx = moux * cam -> f.viewwidth / scrw;
    my = mouy * vh / scrh;

    /* ---- Position at 0, 0, 0 ----- */
    m_ray[0] = -(left + mx);
    m_ray[1] = cam -> f.zmin;
    m_ray[2] = top + my;

    cam -> f.m_ray2d[0] = m_ray[0];
    cam -> f.m_ray2d[1] = m_ray[1];
    cam -> f.m_ray2d[2] = 0;

    PT_NORMALIZE(cam -> f.m_ray2d);

    obj = cam -> obj;

    angle = DEG2RAD(obj -> rot[2]) + atan2(m_ray[0], m_ray[1]);

    cam -> f.m_ray2d[0] = sin(angle);
    cam -> f.m_ray2d[1] = cos(angle);

    /* And now create the ray in 3D for collision tests */
    cam -> f.mouse_ray[0] = cam -> f.m_ray2d[0] * m_ray[0];
    cam -> f.mouse_ray[1] = cam -> f.m_ray2d[1] * m_ray[1];
    cam -> f.mouse_ray[2] = m_ray[2];

    PT_NORMALIZE(cam -> f.mouse_ray);

    /* TODO: Rotate this around all three axes */

    /* Generate tile numbers hit by mouse */
    sdlgl3dITilesOnLine(camera_no);
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