#pragma once

///
/// @file
/// @brief Frustum Culling.
/// @details This file originally developed by DigiBen  <digiben@gametutorials.com>
///   This file is not an official part of Egoboo. It has been added to experiment with frustum culling.

#include "egoboo_types.h"

typedef float frustum_data_t[6][4];

struct sFrustum
{
  // This holds the A B C and D values for each side of our frustum.
  frustum_data_t planes;
};
typedef struct sFrustum Frustum_t;

extern Frustum_t gFrustum;

// Call this every time the camera moves to update the frustum
void Frustum_CalculateFrustum( Frustum_t * pf, float proj[], float modl[] );

// This takes a 3D point and returns TRUE if it's inside of the frustum
bool_t Frustum_PointInFrustum( Frustum_t * pf, float pos[] );

// This takes a 3D point and a radius and returns TRUE if the sphere is inside of the frustum
bool_t Frustum_SphereInFrustum( Frustum_t * pf, float pos[], float radius );

// This takes the center and half the length of the cube.
bool_t Frustum_CubeInFrustum( Frustum_t * pf, float pos[], float size );

// This takes two vectors forming the corners of an axis-aligned bounding box
bool_t Frustum_BBoxInFrustum( Frustum_t * pf, float corner1[], float corner2[] );
