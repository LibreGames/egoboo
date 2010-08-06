/*******************************************************************************
*  SDLGL3D.C                                                                   *
*      - Structs and functions for handling a 3D - View with Z-Axis up         *
*                                                                              *
*   SDLGL - Library                                                            *
*      Copyright (c)2004 Paul Mueller <pmtech@swissonline.ch>                  *
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
#define SDLGL3D_CAMERA_FOV     90.0

/* ------- */
#define SDLGL3D_I_MAXVISITILES (9 + 7 + 5 + 3 + 3 + 3 + 9 + 9 + 11)

#define SDLGL3D_VISITILEDEPTH 5       /* Number of squares visible in depth */

#define SDLGL3D_I_VIEWWIDTH  96 /* 96 */ /* 128 */ /* 176 */
#define SDLGL3D_I_VIEWHEIGHT 72  /* 72 */ /* 96 */ /* 128 */

#define SDLGL3D_I_ZMIN 48.0     /* 100.0 */
#define SDLGL3D_I_ZMAX 1385.0    /* 900.0 -- 64 / 48       */

/* ------- GRID_DATA ------ */
#define SDLGL3D_TILESIZE     128.0

/* Internal defines */
#define NO_CLIP     0x00
#define SOME_CLIP   0x01
#define NOT_VISIBLE 0x02


#define SDLGL3D_MAXOBJECT 256

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    /* Type of camera and size of frustum for zooming */
    int   type;
    float viewwidth, viewheight,
          zmin, zmax;
    float cameradist;           /* Distance behind object for third person camera */
    SDLGL3D_OBJECT *object;     /* The object the camera is attached to           */
    SDLGL3D_OBJECT campos;
    /* Frustum data for calculation of visibility */
    float nx[3], ny[3];         /* Normals of the side lines.  They should point in towards the viewable area.	*/
    float leftangle, rightangle;
    float fow;

} SDLGL3D_CAMERA;

/*****************************************************************************
* DATA								             *
*****************************************************************************/

/* static SDLGL3D_OBJECT BasePoint; */       /* Position 0, 0, 0 */
/* This one owns the camera        */
static SDLGL3D_CAMERA Camera = {

    SDLGL3D_CAMERATYPE_THIRDPERSON,
    SDLGL3D_I_VIEWWIDTH, SDLGL3D_I_VIEWHEIGHT,
    SDLGL3D_I_ZMIN, SDLGL3D_I_ZMAX,
    256.0

};

static SDLGL3D_OBJECT Object_List[SDLGL3D_MAXOBJECT + 2];  /* List of objects to handle    */

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/*
 * Name:
 *     sdlgl3dSetupFrustumNormals
 * Description:
 *	Sets the normals of the frustum boundaries using the given parameters.
 * Input:
 *      SDLGL3D_CAMERA *: Camera with frustum to fill with the values. Holding the
 *                   cameras position, too
 *      FOV:	      Field of view for this frustum
 * Code from: Game Programming Gems, Section 4, Chapter 11
 *	      "Loose Octrees"
 *	       -thatcher 4/6/2000 Copyright Thatcher Ulrich
 */
static void sdlgl3dSetupFrustumNormals(SDLGL3D_CAMERA *f, float FOV)
{

    int rotz;

    
    if (f -> type == SDLGL3D_CAMERATYPE_FIRSTPERSON) {
        rotz = f -> object -> rot[2];
    }
    else {
        rotz = f -> campos.rot[2];
    }

    f -> rightangle = rotz - FOV/2;
    f -> leftangle  = rotz + FOV/2;

    f -> nx[0] = sin(DEG2RAD(f -> rightangle));
    f -> ny[0] = cos(DEG2RAD(f -> rightangle));

    f -> nx[1] = sin(DEG2RAD(f -> leftangle));
    f -> ny[1] = cos(DEG2RAD(f -> leftangle));

    f -> nx[2] = sin(DEG2RAD(rotz));
    f -> ny[2] = cos(DEG2RAD(rotz));

}

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
static char CheckBoxAgainstFrustum(float cx, float cy, float HalfSize,
                                   SDLGL3D_CAMERA *f, int posx, int posy)
{
    int	OrCodes = 0, AndCodes = ~0;

    int	i, j, k;

    /* Check each box corner against each edge of the frustum. */
    for (j = 0; j < 2; j++) {

    	for (i = 0; i < 2; i++) {
	        // int	mask = 1 << (j * 2 + i);

    	    float x = cx + (i == 0 ? -HalfSize : HalfSize);
	        float y = cy + (j == 0 ? -HalfSize : HalfSize);

    	    /* Check the corner against the two sides of the frustum. */
            int	Code = 0;
	        int	Bit = 1;
    	    for (k = 0; k < 3; k++, Bit <<= 1) {

	        	 float	dot = f -> nx[k] * (x - posx) + f -> ny[k] * (y - posy);

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
 */
static void sdlgl3dIMoveSingleObj(SDLGL3D_OBJECT *moveobj, float secondspassed)
{

    float speed;


    switch(moveobj -> move_cmd) {

        case SDLGL3D_MOVE_AHEAD:
            speed = moveobj -> speed * moveobj -> move_dir;
            /* Only in x/y plane */
            moveobj -> pos[0] += (moveobj -> direction[0] * speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> direction[1] * speed * secondspassed);
            break;

        case SDLGL3D_MOVE_STRAFE:  /* SDLGL3D_MOVE_STRAFE: */
            speed = moveobj -> speed * moveobj -> move_dir;
            /* Only in x/y plane */
            moveobj -> pos[0] += (moveobj -> direction[1] * speed * secondspassed);
            moveobj -> pos[1] -= (moveobj -> direction[0] * speed * secondspassed);
            break;

        case SDLGL3D_MOVE_3D:
            moveobj -> pos[0] += (moveobj -> direction[0] * moveobj -> speed * secondspassed);
            moveobj -> pos[1] += (moveobj -> direction[1] * moveobj -> speed * secondspassed);
            moveobj -> pos[SDLGL3D_Z] += (moveobj -> zspeed * secondspassed);
            break;

        case SDLGL3D_MOVE_VERTICAL:
            speed = moveobj -> zspeed * moveobj -> move_dir;
            moveobj -> pos[SDLGL3D_Z] += (speed * secondspassed);
            break;

        case SDLGL3D_MOVE_TURN:
            speed = moveobj -> turnvel * moveobj -> move_dir;
            /* Clockwise */
            moveobj -> rot[SDLGL3D_Z] += (speed * secondspassed);
            if (moveobj -> rot[SDLGL3D_Z] > 360.0) {

                moveobj -> rot[SDLGL3D_Z] -= 360.0;

            }
            else if (moveobj -> rot[SDLGL3D_Z] < 0.0) {

                moveobj -> rot[SDLGL3D_Z] += 360.0;

            }

            moveobj -> direction[0] = sin(DEG2RAD(moveobj -> rot[SDLGL3D_Z]));
            moveobj -> direction[1] = cos(DEG2RAD(moveobj -> rot[SDLGL3D_Z]));
            break;

        case SDLGL3D_MOVE_ROTX:
            speed = moveobj -> turnvel * moveobj -> move_dir;

            moveobj -> rot[0] += (speed * secondspassed);      /* Clockwise */
            if (moveobj -> rot[0] > 360.0) {

                moveobj -> rot[0] -= 360.0;

            }
            else if (moveobj -> rot[0] < 0.0) {

                moveobj -> rot[0] += 360.0;

            }
            break;

        case SDLGL3D_MOVE_ROTY:
            speed = moveobj -> turnvel * moveobj -> move_dir;

            moveobj -> rot[1] += (speed * secondspassed);      /* Clockwise */
            if (moveobj -> rot[1] > 360.0) {

                moveobj -> rot[1] -= 360.0;

            }
            else if (moveobj -> rot[1] < 0.0) {

                moveobj -> rot[1] += 360.0;

            }
            break;

        case SDLGL3D_CAMERA_ZOOM:
            speed = (0.05 * secondspassed);
            if (moveobj -> move_dir > 0) {
                /* Zoom in: Reduce size of view */
                speed = 1.0 - speed;
                Camera.viewwidth  *= speed;
                Camera.viewheight *= speed;

            }
            else if (moveobj -> move_dir < 0) {

                speed = 1.0 + speed;
                Camera.viewwidth  *= speed;
                Camera.viewheight *= speed;

            }
            break;

    }

    if (Camera.object == moveobj) {

        /* FIXME: Move camera according to movement of attached object */
        sdlgl3dIMoveAttachedCamera(&Camera);
        /* If the camera was moved, set the frustum normals... */
        sdlgl3dSetupFrustumNormals(&Camera, SDLGL3D_CAMERA_FOV);

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
 *     solid: Draw it solid, true or false
 */
SDLGL3D_OBJECT *sdlgl3dBegin(int solid)
{

    SDLGL3D_OBJECT *viewobj;


    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();                 /* Save the callers View-Mode */
    glLoadIdentity();

    /* Set the view mode */
    glFrustum(-(Camera.viewwidth / 2), (Camera.viewwidth / 2),
              -(Camera.viewheight / 2), (Camera.viewheight / 2),
              Camera.zmin, Camera.zmax);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    if (Camera.type == SDLGL3D_CAMERATYPE_FIRSTPERSON) {

        viewobj = Camera.object;
        if (viewobj == 0) {

            /* Play it save */
            viewobj = &Camera.campos;

        }


    }
    else {

        viewobj = &Camera.campos;

    }

    glRotatef(viewobj -> rot[0], 1.0, 0.0, 0.0);
    glRotatef(viewobj -> rot[1], 0.0, 1.0, 0.0);
    glRotatef(viewobj -> rot[2], 0.0, 0.0, 1.0);
    glTranslatef(-viewobj -> pos[0], -viewobj -> pos[1], -viewobj -> pos[2]);

    /* Now we're ready to draw the grid */
    glPolygonMode(GL_FRONT, solid ? GL_FILL : GL_LINE);

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

    glDisable(GL_TEXTURE_2D);

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


    Camera.object = &Object_List[object_no];    /* Attach object to camera */

    if (camtype == SDLGL3D_CAMERATYPE_FIRSTPERSON) {

        Camera.cameradist = 0;

    }
    else {

        /* Create a third person camera behind the given 'moveobj' */
        distance = 2 * SDLGL3D_TILESIZE;
        Camera.cameradist = distance;

        Camera.campos.pos[0] = Camera.object -> pos[0] - (Camera.object -> direction[0] * distance);
        Camera.campos.pos[1] = Camera.object -> pos[1] - (Camera.object -> direction[1] * distance);
        Camera.campos.pos[2] = SDLGL3D_TILESIZE;

        Camera.campos.rot[2] = 30.0;

        Camera.campos.extent[0]  = SDLGL3D_CAMERA_EXTENT;
        Camera.campos.extenttype = SDLGL3D_EXTENT_CIRCLE;

    }

    Camera.type = camtype;

    /* Set the frustum normals... */
    sdlgl3dSetupFrustumNormals(&Camera, SDLGL3D_CAMERA_FOV);

}

/*
 * Name:
 *     sdlgl3dInitCamera
 * Description:
 *     Initializes the 3D-Camera. Moves the camera to given position with
 *     given rotations.
 * Input:
 *      x, ,y, z:
 *      rotx, roty, rotz:
 */
void sdlgl3dInitCamera(float x, float y, float z, int rotx, int roty, int rotz)
{

    Camera.campos.pos[0] = x;
    Camera.campos.pos[1] = y;
    Camera.campos.pos[SDLGL3D_Z] = z;

    Camera.campos.rot[0] = rotx;
    Camera.campos.rot[1] = roty;
    Camera.campos.rot[SDLGL3D_Z] = rotz;

    Camera.campos.direction[0] = sin(DEG2RAD(rotz));
    Camera.campos.direction[1] = cos(DEG2RAD(rotz));

    Camera.campos.speed   = 100.0;  /* Speed of camera in units / second    */
    Camera.campos.turnvel =  60.0;  /* Degrees per second                   */

    /* If the camera was moved, set the frustum normals... */
    sdlgl3dSetupFrustumNormals(&Camera, SDLGL3D_CAMERA_FOV);

}

/*
 * Name:
 *     sdlgl3dGetCameraInfo
 * Description:
 *     Initializes the 3D-Camera. Moves the camera to given position with
 *     given rotations.
 * Input:
 *     None
 * Output:
 *      Pointer on camera position
 */
SDLGL3D_OBJECT *sdlgl3dGetCameraInfo(void)
{

    return &Camera.campos;

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
 *     sdlgl3dManageObject
 * Description:
 *     Sets the commands for object with given number. < 0: Is camera
 * Input:
 *      object_no: Number of object to manage
 *      move_cmd:  Kind of movement
 *      move_dir:  Prefix additional to movement code
 */
void sdlgl3dManageObject(int object_no, char move_cmd, char move_dir)
{

    if (object_no < 0) {

        /* Manage the camera */
        Camera.campos.move_cmd = move_cmd;
        Camera.campos.move_dir = move_dir;

    }
    else if (object_no > 0) {

        /* Manage this object */


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


    objects = Object_List;

    while(objects -> obj_type) {

        if (objects -> move_cmd) {

            sdlgl3dIMoveSingleObj(objects, secondspassed);

        }

        objects++;

    }



    if (Camera.campos.move_cmd > 0) {

        /* FIXME: Take into account if camera is 'attached' as 3rd-Person camera */
        sdlgl3dIMoveSingleObj(&Camera.campos, secondspassed);

        /* If the camera was moved, set the frustum normals... */
        sdlgl3dSetupFrustumNormals(&Camera, SDLGL3D_CAMERA_FOV);

    }


}

/*
 * Name:
 *     sdlgl3dCollided
 * Description:
 *     Tests the two given objects for collision. Both objects can be
 *     either a SDLGL3D_EXTENT_CIRCLE or EXTENT_TYPESQUARE. If the two
 *     object did collide, the first objects position is adjusted that
 *     way, that the two objects doesn't overlap.
 * Input:
 *      o1, o2: Pointer on objects to test for collison.
 * Output:
 *      != 0, if the two objects collided.
 */
int sdlgl3dCollided(SDLGL3D_OBJECT *o1, SDLGL3D_OBJECT *o2)
{

    float DistanceMiddle;
    float DistanceX, DistanceY;
    float CollisionDistance, edgeCollisionDistance;
    float DifferenceX,DifferenceY;
    float edgeX, edgeY, edgeDist, edgeOverlap;
    SDLGL3D_OBJECT *saveo1, *otemp;

    if (o1 -> collisionflags & o2 -> collisionflags) {
        /* No collsion checking needed */
        return 0;
    }

    DistanceX = o1 -> pos[0] - o2 -> pos[0];
    DistanceY = o1 -> pos[1] - o2 -> pos[1];
    DistanceMiddle = sqrt((DistanceX * DistanceX) + (DistanceY * DistanceY));

    CollisionDistance = o1 -> extent[0] + o2 -> extent[0];        /* Radius */
    edgeCollisionDistance = CollisionDistance;

    if ( o1 -> extenttype == SDLGL3D_EXTENT_RECT) {
        edgeCollisionDistance += o1 -> extent[0] * 0.41421356;
    }

    if ( o2 -> extenttype == SDLGL3D_EXTENT_RECT) {
        edgeCollisionDistance +=  o2 -> extent[0] * 0.41421356;
    }

    if (DistanceMiddle <  edgeCollisionDistance) {

        if ((o2 -> extenttype == SDLGL3D_EXTENT_CIRCLE) && (o1 -> extenttype == SDLGL3D_EXTENT_CIRCLE)) {

            /* Correct object positions */
            o1 -> pos[0] = o2 -> pos[0] + (DistanceX * edgeCollisionDistance / DistanceMiddle);
            o1 -> pos[1] = o2 -> pos[1] + (DistanceY * edgeCollisionDistance / DistanceMiddle);
            return 1;

        }


        saveo1 = o1;

        if ((o2 -> extenttype == SDLGL3D_EXTENT_RECT) && (o1 -> extenttype == SDLGL3D_EXTENT_RECT)) {
            edgeX = -1;
            edgeY = -1;
        }
        else {

            if (o2 -> extenttype == SDLGL3D_EXTENT_CIRCLE){

                otemp = o1;
                o1 = o2;
                o2 = otemp;

            }

            edgeX = abs(DistanceX) - o2 -> extent[0];
            edgeY = abs(DistanceY) - o2 -> extent[0];

        }

        if (( edgeX <= 0 ) || (edgeY <= 0 )) {


            DifferenceX = CollisionDistance - abs(DistanceX);
            DifferenceY = CollisionDistance - abs(DistanceY);

            if ( DistanceX < 0 ){
                DifferenceX = -DifferenceX;
            }

            if ( DistanceY < 0 ){
                DifferenceY = -DifferenceY;
            }

            DistanceX = fabs(DistanceX);
            DistanceY = fabs(DistanceY);


            if ( DistanceY <  DistanceX ) {
                DistanceMiddle = DistanceX;
                DifferenceY = 0;

            }
            else {

                DistanceMiddle = DistanceY;
                DifferenceX = 0;

            }


            if (DistanceMiddle < CollisionDistance) {

                saveo1 -> pos[0] += DifferenceX;
                saveo1 -> pos[1] += DifferenceY;
                return 1;

            }

        }
        else {

            if (( o1 -> extenttype == SDLGL3D_EXTENT_CIRCLE) && (o2 -> extenttype == SDLGL3D_EXTENT_RECT)){

                edgeDist = sqrt((edgeX * edgeX) + (edgeY * edgeY));
                edgeOverlap = o1 -> extent[0] - edgeDist;

                if ( edgeOverlap > 0) {

                    /* Calculate correction measure */
                    DifferenceX = fabs(edgeX * edgeOverlap / edgeDist) ;
                    DifferenceY = fabs(edgeY * edgeOverlap / edgeDist) ;

                    if ( DistanceX < 0 ){
                        DifferenceX = -DifferenceX;
                    }

                    if ( DistanceY < 0 ){
                        DifferenceY = -DifferenceY;
                    }


                    /* Correct object positions */
                    saveo1 -> pos[0] += DifferenceX;
                    saveo1 -> pos[1] += DifferenceY;
                    return 1;

                }

           }

        }

    }

    return 0;

}
