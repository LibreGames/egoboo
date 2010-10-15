/*******************************************************************************
*  SDLGL3D.H                                                                   *
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

#ifndef _SDLGL_3D_H_
#define _SDLGL_3D_H_

/*******************************************************************************
* DEFINES 							                                           *
*******************************************************************************/

#ifndef	NULL
#define	NULL	0
#endif /* NULL */

#ifndef	TRUE
#define	TRUE	1
#define	FALSE	0
#endif /* TRUE */



#ifdef SDLGL_EPS
#undef SDLGL_EPS
#endif /* SDLGL_EPS */
#define SDLGL_EPS	1e-5

#ifdef SDLGL_INFNTY
#undef SDLGL_INFNTY
#endif /*  SDLGL_INFNTY */
#define SDLGL_INFNTY	1e6

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif /* M_PI */

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif /* M_SQRT2 */

/* Follows by general purpose helpfull macros: */
#ifndef MIN
#define MIN(x, y)		((x) > (y) ? (y) : (x))
#endif /* MIN */
#ifndef MAX
#define MAX(x, y)		((x) > (y) ? (x) : (y))
#endif /* MAX */
#define BOUND(x, Min, Max)	(MAX(MIN((x), Max), Min))

#define ABS(x)			((x) > 0 ? (x) : (-(x)))
#define SQR(x)			((x) * (x))
#define SIGN(x)			((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

#define SWAP(type, x, y)	{ type temp = (x); (x) = (y); (y) = temp; }

#define REAL_TO_INT(R)		((int) ((R) > 0.0 ? (R) + 0.5 : (R) - 0.5))
#define REAL_PTR_TO_INT(R)	REAL_TO_INT(*(R))

#define APX_EQ(x, y)		(ABS((x) - (y)) < SDLGL_EPS)
#define APX_EQ_EPS(x, y, EPS)	(ABS((x) - (y)) < EPS)
#define PT_APX_EQ(Pt1, Pt2)	(APX_EQ(Pt1[0], Pt2[0]) && \
				 APX_EQ(Pt1[1], Pt2[1]) && \
				 APX_EQ(Pt1[2], Pt2[2]))
#define PT_APX_EQ_EPS(Pt1, Pt2, EPS) \
				(APX_EQ_EPS(Pt1[0], Pt2[0], EPS) && \
				 APX_EQ_EPS(Pt1[1], Pt2[1], EPS) && \
				 APX_EQ_EPS(Pt1[2], Pt2[2], EPS))
#define PLANE_APX_EQ(Pl1, Pl2)	(APX_EQ(Pl1[0], Pl2[0]) && \
				 APX_EQ(Pl1[1], Pl2[1]) && \
				 APX_EQ(Pl1[2], Pl2[2]) && \
				 APX_EQ(Pl1[3], Pl2[3]))
#define PT_APX_EQ_ZERO_EPS(Pt, EPS) \
				(APX_EQ_EPS(Pt[0], 0.0, EPS) && \
				 APX_EQ_EPS(Pt[1], 0.0, EPS) && \
				 APX_EQ_EPS(Pt[2], 0.0, EPS))
#define PT_EQ_ZERO(Pt)		(Pt[0] == 0.0 && Pt[1] == 0.0 && Pt[2] == 0.0)

#define SDLGL_UEPS		1e-6

#define SDLGL_APX_EQ(x, y)		(ABS((x) - (y)) < SDLGL_UEPS)
#define SDLGL_PT_APX_EQ(Pt1, Pt2)	(SDLGL_APX_EQ(Pt1[0], Pt2[0]) && \
					 SDLGL_APX_EQ(Pt1[1], Pt2[1]) && \
					 SDLGL_APX_EQ(Pt1[2], Pt2[2]))


#define PT_CLEAR(Pt)		{ Pt[0] = Pt[1] = Pt[2] = 0.0; }

#define PT_SCALE(Pt, Scalar)	{ Pt[0] *= Scalar; \
				  Pt[1] *= Scalar; \
				  Pt[2] *= Scalar; \
			        }

#define PT_SCALE2(Res, Pt, Scalar) \
				{ Res[0] = Pt[0] * Scalar; \
				  Res[1] = Pt[1] * Scalar; \
				  Res[2] = Pt[2] * Scalar; \
			        }

/* The memcpy is sometimes defined to get (char *) pointers and sometimes    */
/* (void *) pointers. To be compatible with both it is coerced to (char *).  */
#define UV_COPY(PtDest, PtSrc)	  memcpy((char *) (PtDest), (char *) (PtSrc), \
							2 * sizeof(float))
#define PT_COPY(PtDest, PtSrc)	  memcpy((char *) (PtDest), (char *) (PtSrc), \
							3 * sizeof(float))
#define PLANE_COPY(PlDest, PlSrc) memcpy((char *) (PlDest), (char *) (PlSrc), \
							4 * sizeof(float))
#define MAT_COPY(Dest, Src)	  memcpy((char *) (Dest), (char *) (Src), \
							16 * sizeof(float))
#define GEN_COPY(Dest, Src, Size) memcpy((char *) (Dest), (char *) (Src), Size)
#define ZAP_MEM(Dest, Size)	  memset((char *) (Dest), 0, Size);

#define PT_SQR_LENGTH(Pt)	(SQR(Pt[0]) + SQR(Pt[1]) + SQR(Pt[2]))
#define PT_LENGTH(Pt)		sqrt(SQR(Pt[0]) + SQR(Pt[1]) + SQR(Pt[2]))

#define PT_RESET(Pt)		Pt[0] = Pt[1] = Pt[2] = 0.0
#define UV_RESET(Uv)		Uv[0] = Uv[1] = 0.0

#define PT_NORMALIZE_ZERO	1e-30
#define PT_NORMALIZE(Pt)	{    float Size = PT_LENGTH(Pt); \
				     if (Size < PT_NORMALIZE_ZERO) { \
					 fprintf(stderr,"Attempt to normalize a zero length vector\n"); \
				     } \
				     else { \
					 Pt[0] /= Size; \
					 Pt[1] /= Size; \
				         Pt[2] /= Size; \
				     } \
				}

#define PT_BLEND(Res, Pt1, Pt2, t) \
				{ Res[0] = Pt1[0] * t + Pt2[0] * (1 - t); \
				  Res[1] = Pt1[1] * t + Pt2[1] * (1 - t); \
				  Res[2] = Pt1[2] * t + Pt2[2] * (1 - t); \
			        }

#define PT_ADD(Res, Pt1, Pt2)	{ Res[0] = Pt1[0] + Pt2[0]; \
				  Res[1] = Pt1[1] + Pt2[1]; \
				  Res[2] = Pt1[2] + Pt2[2]; \
			        }

#define PT_SUB(Res, Pt1, Pt2)	{ Res[0] = Pt1[0] - Pt2[0]; \
				  Res[1] = Pt1[1] - Pt2[1]; \
				  Res[2] = Pt1[2] - Pt2[2]; \
				}

#define PT_SWAP(Pt1, Pt2)	{ SWAP(float, Pt1[0], Pt2[0]); \
				  SWAP(float, Pt1[1], Pt2[1]); \
				  SWAP(float, Pt1[2], Pt2[2]); \
				}

#define PT_PT_DIST(Pt1, Pt2)    sqrt(SQR(Pt1[0] - Pt2[0]) + \
				     SQR(Pt1[1] - Pt2[1]) + \
				     SQR(Pt1[2] - Pt2[2]))

#define PT_PT_DIST_SQR(Pt1, Pt2) (SQR(Pt1[0] - Pt2[0]) + \
				  SQR(Pt1[1] - Pt2[1]) + \
				  SQR(Pt1[2] - Pt2[2]))

#define DOT_PROD(Pt1, Pt2)	(Pt1[0] * Pt2[0] + \
				 Pt1[1] * Pt2[1] + \
				 Pt1[2] * Pt2[2])

#define CROSS_PROD(PtRes, Pt1, Pt2) \
				{ PtRes[0] = Pt1[1] * Pt2[2] - Pt1[2] * Pt2[1]; \
				  PtRes[1] = Pt1[2] * Pt2[0] - Pt1[0] * Pt2[2]; \
				  PtRes[2] = Pt1[0] * Pt2[1] - Pt1[1] * Pt2[0]; }

#define LIST_PUSH(New, List)       { (New) -> Pnext = (List); (List) = (New); }
#define LIST_POP(Head, List)	   { (Head) = (List); \
				     (List) = (List) -> Pnext; \
				     (Head) -> Pnext = NULL; }
#define LIST_LAST_ELEM(Elem)	   { if (Elem) \
					 while ((Elem) -> Pnext) \
					     (Elem) = (Elem) -> Pnext; }

#define DEG2RAD(Deg)		((Deg) * M_PI / 180.0)
#define RAD2DEG(Rad)		((Rad) * 180.0 / M_PI)

/* ------- Some object tags ------ */
#define SDLGL_HASNORMALS  0x0001
#define SDLGL_HASCOLOR    0x0002
#define SDLGL_HASTEXCOORD 0x0004

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

/* ------ Movement of objects using the movement vector ---------- */
#define SDLGL3D_MOVE_FORWARD	0x01	/* ------ Number of flag --- */
#define SDLGL3D_MOVE_BACKWARD	0x02
#define SDLGL3D_MOVE_LEFT	    0x03
#define SDLGL3D_MOVE_RIGHT	    0x04
#define SDLGL3D_MOVE_UP		    0x05
#define SDLGL3D_MOVE_DOWN	    0x06
#define SDLGL3D_MOVE_TURNLEFT	0x07    /* Z-Rotation               */
#define SDLGL3D_MOVE_TURNRIGHT	0x08
#define SDLGL3D_MOVE_3D		    0x09	/* Along movement-vector, for 'auto-move' eg. particles */
#define SDLGL3D_MOVE_ROTX	    0x0A
#define SDLGL3D_MOVE_ROTY	    0x0B
    /* Camera commands */
#define SDLGL3D_MOVE_ZOOMIN	    0x0C
#define SDLGL3D_MOVE_ZOOMOUT	0x0D
#define SDLGL3D_MOVE_CAMDIST	0x0E	/* Change distance to object attached to */
#define SDLGL3D_MOVE_STOPMOVE   0x0F 
#define SDLGL3D_MOVE_MAXCMD     0x0F

/* ----- Camera types ------- */
#define SDLGL3D_CAMERATYPE_STANDARD     0x00
#define SDLGL3D_CAMERATYPE_FIRSTPERSON  0x01
#define SDLGL3D_CAMERATYPE_THIRDPERSON  0x02

/* -------- Object-Types ------ */
#define SDLGL3D_OBJ_NONE     0

/* Values in vector */
#define SDLGL3D_X   0
#define SDLGL3D_Y   1
#define SDLGL3D_Z   2

/* ----- Extents for collision ------- */
#define SDLGL3D_EXTENT_CIRCLE 0x01
#define SDLGL3D_EXTENT_RECT   0x02 

/* ----- Results of collision detection in movement ----- */
#define SDLGL3D_EVENT_TILECHANGED   2
#define SDLGL3D_EVENT_MOVED         1
#define SDLGL3D_EVENT_NONE          0

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef float SDLGL3D_VECTOR[3];  /* For X, Y, Z coordinates of vector.  */

typedef struct {

    char    obj_type;          /* > 0. Callers side identification     */
                               /* 0: End of list for SDLGL3D           */
    int     display_type;      /* Number of geometry for this one      */
    /* ---------- Extent for collision detection --------------------- */
    float   bbox[2][3];        /* Bounding box for collision detection */
    float   bradius;           /* Radius of Bounding box               */ 
    /* -------- Object info ------------------------------------------ */ 
    int     tags;              /* Visible and so on                    */
    SDLGL3D_VECTOR pos;        /* Position                             */
    SDLGL3D_VECTOR direction;  /* Actual direction, unit vector        */
    SDLGL3D_VECTOR rot;        /* Rotation angles                      */
    int     move_cmd;          /* Commands for movement (combined)     */  
    float   speed;             /* Ahead speed in units/second          */
    float   zspeed;            /* Z-Movement: -towards bottom          */
                               /*             +towards ceiling         */
                               /*              in units/second         */
    float   turnvel;           /* Rotation velocity in degrees/second  */
    /* Link for object list in collision - detection                   */
    int     on_tile;           /* Object is on this tile               */
    char    visi_code;         /* Visibility ob object in frustum      */ 
    char    speed_modifier;    /* Multiply speed with this one, if > 0 */ 
    
} SDLGL3D_OBJECT;

typedef struct {

    float fov;
    float viewwidth;
    float aspect_ratio;         /* For zoom and setup of frustum        */
    float zmin, zmax;           
    float nx[3], ny[3];              
    float leftangle, rightangle;
    int   vx1, vy1, vx2, vy2;   /* Bounding rectangle of field of view  */
    int   num_visi_tile;        /* Number of tiles visible in FOV       */

} SDLGL3D_FRUSTUM;

typedef struct {
    
    int mid_x, mid_y;   /* Of tile, for distance sorting            */
    int no;             /* Number of tile, for display by caller    */

} SDLGL3D_VISITILE;

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

SDLGL3D_OBJECT *sdlgl3dBegin(int camera_no, int solid);
void sdlgl3dEnd(void);
void sdlgl3dAttachCameraToObj(int obj_no, char camtype);
void sdlgl3dInitCamera(int camera_no, int rotx, int roty, int rotz, float aspect_ratio);
void sdlgl3dBindCamera(int camera_no, float x, float y, float x2, float y2); 
SDLGL3D_OBJECT *sdlgl3dGetCameraInfo(int camera_no, SDLGL3D_FRUSTUM *f);
void sdlgl3dInitObject(SDLGL3D_OBJECT *moveobj);
void sdlgl3dManageCamera(int camera_no, char move_cmd, char set, char speed_modifier);
void sdlgl3dMoveToPosCamera(int camera_no, float x, float y, float z, int relative);
void sdlgl3dManageObject(int obj_no, char move_cmd, char set);
void sdlgl3dMoveObjects(float secondspassed);
void sdlgl3dInitVisiMap(int map_w, int map_h, float tile_size);
SDLGL3D_VISITILE *sdlgl3dGetVisiTileList(int *num_tile);
/* TODO:
    --- List of objects for drawing by caller, sorted from farest to nearest ---
    int sdlgl3dVisibleObjects(SDLGL3D_OBJECT **objects);
    --- return-value: Number ob objects in list ---    
    --- add/remove (if object_no) object from list ---
    int sdlgl3dSetObject(SDLGL3D_OBJECT *object, int int object_no);
*/

#endif  /* _SDLGL_3D_H_ */
