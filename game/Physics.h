#pragma once

#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct sOrientation
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
typedef struct sOrientation Orientation_t;

//--------------------------------------------------------------------------------------------
struct sPhysAccum
{
  vect3           acc;
  vect3           vel;
  vect3           pos;
};
typedef struct sPhysAccum PhysAccum_t;

bool_t CPhysAccum_clear(PhysAccum_t * paccum);

//--------------------------------------------------------------------------------------------
struct sPhysicsData
{
  egoboo_key_t ekey;

  float    hillslide;                 // Friction
  float    slippyfriction;            //
  float    airfriction;               //
  float    waterfriction;             //
  float    noslipfriction;            //
  float    platstick;                 //

  float    gravity;                   // Gravitational accel
};
typedef struct sPhysicsData PhysicsData_t;

INLINE PhysicsData_t * CPhysicsData_new(PhysicsData_t * phys);
INLINE bool_t          CPhysicsData_delete(PhysicsData_t * phys);
INLINE PhysicsData_t * CPhysicsData_renew(PhysicsData_t * phys);

//--------------------------------------------------------------------------------------------
// collision volume difinition
struct sCVolume
{
  int   lod;
  float x_min, x_max;
  float y_min, y_max;
  float z_min, z_max;
  float xy_min, xy_max;
  float yx_min, yx_max;
};
typedef struct sCVolume CVolume_t;

CVolume_t CVolume_merge(CVolume_t * pv1, CVolume_t * pv2);
CVolume_t CVolume_intersect(CVolume_t * pv1, CVolume_t * pv2);
bool_t    CVolume_draw( CVolume_t * cv, bool_t draw_square, bool_t draw_diamond );

//--------------------------------------------------------------------------------------------
struct sCVolume_Tree { CVolume_t leaf[8]; };
typedef struct sCVolume_Tree CVolume_Tree_t;

//--------------------------------------------------------------------------------------------
INLINE void cv_list_add( CVolume_t * cv);
INLINE void cv_list_clear( void );
INLINE void cv_list_draw( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void phys_integrate(Orientation_t * pori, Orientation_t * pori_old, PhysAccum_t * paccum, float dFrame);
