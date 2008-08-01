#pragma once

#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct COrientation_t
{
  vect3           pos;             // position
  vect3           vel;             // velocity

  quaternion      rot;             // angular "position"
  vect3           omega;           // angular velocity

  // Euler angles
  Uint16          turn_lr;         // rotation about body z-axis, 0 to 65535
  Uint16          mapturn_lr;      // rotation about x-axis, 0 to 65535, 32768 is no rotation
  Uint16          mapturn_ud;      // rotation about y-axis, rotation 0 to 65535, 32768 is no rotation

};
typedef struct COrientation_t COrientation;

//--------------------------------------------------------------------------------------------
struct CPhysAccum_t
{
  vect3           acc;
  vect3           vel;
  vect3           pos;
};
typedef struct CPhysAccum_t CPhysAccum;

bool_t CPhysAccum_clear(CPhysAccum * paccum);

//--------------------------------------------------------------------------------------------
struct CPhysicsData_t
{
  egoboo_key ekey;

  float    hillslide;                 // Friction
  float    slippyfriction;            //
  float    airfriction;               //
  float    waterfriction;             //
  float    noslipfriction;            //
  float    platstick;                 //

  float    gravity;                   // Gravitational accel
};
typedef struct CPhysicsData_t CPhysicsData;

INLINE CPhysicsData * CPhysicsData_new(CPhysicsData * phys);
INLINE bool_t         CPhysicsData_delete(CPhysicsData * phys);
INLINE CPhysicsData * CPhysicsData_renew(CPhysicsData * phys);

//--------------------------------------------------------------------------------------------
struct collision_volume_t
{
  int   lod;
  float x_min, x_max;
  float y_min, y_max;
  float z_min, z_max;
  float xy_min, xy_max;
  float yx_min, yx_max;
};
typedef struct collision_volume_t CVolume;

CVolume CVolume_merge(CVolume * pv1, CVolume * pv2);
CVolume CVolume_intersect(CVolume * pv1, CVolume * pv2);
bool_t  CVolume_draw( CVolume * cv, bool_t draw_square, bool_t draw_diamond );


//--------------------------------------------------------------------------------------------
struct CVolume_Tree_t { CVolume leaf[8]; };
typedef struct CVolume_Tree_t CVolume_Tree;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void phys_integrate(COrientation * pori, COrientation * pori_old, CPhysAccum * paccum, float dFrame);
