//***********************************************************************
//
//  - "Talk to me like I'm a 3 year old!" Programming Lessons -
//
//  $Author:  DigiBen  digiben@gametutorials.com
//
//  $Program:  Frustum Culling
//
//  $Description: Demonstrates checking if shapes are in view
//
//  $Date:   8/28/01
//
//***********************************************************************//

//
// @brief Implementation of Frustum Culling
// @details Demonstrates checking if shapes are in view



#include "frustum.h"
#include "ogl_debug.h"

#include <SDL_opengl.h>
#include <cmath>


//---------------------------------------------------------------------
//-   This normalizes a plane (A side) from a given frustum.
//---------------------------------------------------------------------
void c_frustum::NormalizePlane(FrustumSide side)
{
	// Here we calculate the magnitude of the normal to the plane (point A B C)
	// Remember that (A, B, C) is that same thing as the normal's (X, Y, Z).
	// To calculate magnitude you use the equation:  magnitude = sqrt( x^2 + y^2 + z^2)
	float magnitude = (float) sqrt(
		this->planes[side][A] * this->planes[side][A] +
		this->planes[side][B] * this->planes[side][B] +
		this->planes[side][C] * this->planes[side][C] );

	// Then we divide the plane's values by it's magnitude.
	// This makes it easier to work with.
	this->planes[side][A] /= magnitude;
	this->planes[side][B] /= magnitude;
	this->planes[side][C] /= magnitude;
	this->planes[side][D] /= magnitude;
}


//---------------------------------------------------------------------
//-   This extracts our frustum from the projection and modelview
//-   matrix.
//---------------------------------------------------------------------
void c_frustum::CalculateFrustum(float proj[], float modl[])
{
	float clip[16];        // This will hold the clipping planes

	if ((NULL == proj) || (NULL == modl))
		return;

	// store a copy of the projection matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(proj);

	// multiply the matrices
	glMultMatrixf(modl);

	// grab the matrix and place it in the matrix clip
	glGetFloatv(GL_PROJECTION_MATRIX, clip);

	// restore the old projection matrix
	glPopMatrix();

	// set the matrix mode to GL_MODELVIEW
	glMatrixMode(GL_MODELVIEW);

	//// glGetFloatv() is used to extract information about our OpenGL world.
	//// Below, we pass in GL_PROJECTION_MATRIX to abstract our projection matrix.
	//// It then stores the matrix into an array of [16].
	//glGetFloatv( GL_PROJECTION_MATRIX, proj );

	//// By passing in GL_MODELVIEW_MATRIX, we can abstract our model view matrix.
	//// This also stores it in an array of [16].
	//glGetFloatv( GL_MODELVIEW_MATRIX, modl );

	//// Now that we have our modelview and projection matrix, if we combine these 2 matrices,
	//// it will give us our clipping planes.  To combine 2 matrices, we multiply them.

	//clip[ 0] = ModList[ 0].l * proj[ 0] + ModList[ 1].l * proj[ 4] + ModList[ 2].l * proj[ 8] + ModList[ 3].l * proj[12];
	//clip[ 1] = ModList[ 0].l * proj[ 1] + ModList[ 1].l * proj[ 5] + ModList[ 2].l * proj[ 9] + ModList[ 3].l * proj[13];
	//clip[ 2] = ModList[ 0].l * proj[ 2] + ModList[ 1].l * proj[ 6] + ModList[ 2].l * proj[10] + ModList[ 3].l * proj[14];
	//clip[ 3] = ModList[ 0].l * proj[ 3] + ModList[ 1].l * proj[ 7] + ModList[ 2].l * proj[11] + ModList[ 3].l * proj[15];

	//clip[ 4] = ModList[ 4].l * proj[ 0] + ModList[ 5].l * proj[ 4] + ModList[ 6].l * proj[ 8] + ModList[ 7].l * proj[12];
	//clip[ 5] = ModList[ 4].l * proj[ 1] + ModList[ 5].l * proj[ 5] + ModList[ 6].l * proj[ 9] + ModList[ 7].l * proj[13];
	//clip[ 6] = ModList[ 4].l * proj[ 2] + ModList[ 5].l * proj[ 6] + ModList[ 6].l * proj[10] + ModList[ 7].l * proj[14];
	//clip[ 7] = ModList[ 4].l * proj[ 3] + ModList[ 5].l * proj[ 7] + ModList[ 6].l * proj[11] + ModList[ 7].l * proj[15];

	//clip[ 8] = ModList[ 8].l * proj[ 0] + ModList[ 9].l * proj[ 4] + ModList[10].l * proj[ 8] + ModList[11].l * proj[12];
	//clip[ 9] = ModList[ 8].l * proj[ 1] + ModList[ 9].l * proj[ 5] + ModList[10].l * proj[ 9] + ModList[11].l * proj[13];
	//clip[10] = ModList[ 8].l * proj[ 2] + ModList[ 9].l * proj[ 6] + ModList[10].l * proj[10] + ModList[11].l * proj[14];
	//clip[11] = ModList[ 8].l * proj[ 3] + ModList[ 9].l * proj[ 7] + ModList[10].l * proj[11] + ModList[11].l * proj[15];

	//clip[12] = ModList[12].l * proj[ 0] + ModList[13].l * proj[ 4] + ModList[14].l * proj[ 8] + ModList[15].l * proj[12];
	//clip[13] = ModList[12].l * proj[ 1] + ModList[13].l * proj[ 5] + ModList[14].l * proj[ 9] + ModList[15].l * proj[13];
	//clip[14] = ModList[12].l * proj[ 2] + ModList[13].l * proj[ 6] + ModList[14].l * proj[10] + ModList[15].l * proj[14];
	//clip[15] = ModList[12].l * proj[ 3] + ModList[13].l * proj[ 7] + ModList[14].l * proj[11] + ModList[15].l * proj[15];

	// Now we actually want to get the sides of the frustum.  To do this we take
	// the clipping planes we received above and extract the sides from them.

	// This will extract the RIGHT side of the frustum
	this->planes[RIGHT][A] = clip[ 3] - clip[ 0];
	this->planes[RIGHT][B] = clip[ 7] - clip[ 4];
	this->planes[RIGHT][C] = clip[11] - clip[ 8];
	this->planes[RIGHT][D] = clip[15] - clip[12];

	// Now that we have a normal (A,B,C) and a distance (D) to the plane,
	// we want to normalize that normal and distance.

	// Normalize the RIGHT side
	NormalizePlane(RIGHT);

	// This will extract the LEFT side of the frustum
	this->planes[LEFT][A] = clip[ 3] + clip[ 0];
	this->planes[LEFT][B] = clip[ 7] + clip[ 4];
	this->planes[LEFT][C] = clip[11] + clip[ 8];
	this->planes[LEFT][D] = clip[15] + clip[12];

	// Normalize the LEFT side
	NormalizePlane(LEFT);

	// This will extract the BOTTOM side of the frustum
	this->planes[BOTTOM][A] = clip[ 3] + clip[ 1];
	this->planes[BOTTOM][B] = clip[ 7] + clip[ 5];
	this->planes[BOTTOM][C] = clip[11] + clip[ 9];
	this->planes[BOTTOM][D] = clip[15] + clip[13];

	// Normalize the BOTTOM side
	NormalizePlane(BOTTOM);

	// This will extract the TOP side of the frustum
	this->planes[TOP][A] = clip[ 3] - clip[ 1];
	this->planes[TOP][B] = clip[ 7] - clip[ 5];
	this->planes[TOP][C] = clip[11] - clip[ 9];
	this->planes[TOP][D] = clip[15] - clip[13];

	// Normalize the TOP side
	NormalizePlane(TOP);

	// This will extract the BACK side of the frustum
	this->planes[BACK][A] = clip[ 3] - clip[ 2];
	this->planes[BACK][B] = clip[ 7] - clip[ 6];
	this->planes[BACK][C] = clip[11] - clip[10];
	this->planes[BACK][D] = clip[15] - clip[14];

	// Normalize the BACK side
	NormalizePlane(BACK);

	// This will extract the FRONT side of the frustum
	this->planes[FRONT][A] = clip[ 3] + clip[ 2];
	this->planes[FRONT][B] = clip[ 7] + clip[ 6];
	this->planes[FRONT][C] = clip[11] + clip[10];
	this->planes[FRONT][D] = clip[15] + clip[14];

	// Normalize the FRONT side
	NormalizePlane(FRONT);
}


//---------------------------------------------------------------------
//-   The code below will allow us to make checks within the frustum.
//-   For example, if we want to see if a point, a sphere, or a cube
//-   lies inside of the frustum.
//-   Because all of our planes point INWARDS
//-   (The normals are all pointing inside the frustum)
//-   we then can assume that if a point is in FRONT
//-   of all of the planes, it's inside.
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//-   This determines if a point is inside of the frustum
//---------------------------------------------------------------------
bool c_frustum::PointInFrustum(float pos[])
{
	// If you remember the plane equation (A*pos[0] + B*pos[1] + C*pos[2] + D = 0), then the rest
	// of this code should be quite obvious and easy to figure out yourself.
	// In case don't know the plane equation, it might be a good idea to look
	// at our Plane Collision tutorial at www.GameTutorials.com in OpenGL Tutorials.
	// I will briefly go over it here.  (A,B,C) is the (X,Y,Z) of the normal to the plane.
	// They are the same thing... but just called ABC because you don't want to say:
	// (pos[0]*pos[0] + pos[1]*pos[1] + pos[2]*pos[2] + d = 0).  That would be wrong, so they substitute them.
	// the (pos[0], pos[1], pos[2]) in the equation is the point that you are testing.  The D is
	// The distance the plane is from the origin.  The equation ends with "= 0" because
	// that is btrue when the point (pos[0], pos[1], pos[2]) is ON the plane.  When the point is NOT on
	// the plane, it is either a negative number (the point is behind the plane) or a
	// positive number (the point is in front of the plane).  We want to check if the point
	// is in front of the plane, so all we have to do is go through each point and make
	// sure the plane equation goes out to a positive number on each side of the frustum.
	// The result (be it positive or negative) is the distance the point is front the plane.

	int i;

	if (NULL == pos)
		return false;

	// Go through all the sides of the frustum
	for (i = 0; i < 6; i++)
	{
		// Calculate the plane equation and check if the point is behind a side of the frustum
		if ( this->planes[i][A] * pos[0] + this->planes[i][B] * pos[1] + this->planes[i][C] * pos[2] + this->planes[i][D] <= 0 )
		{
			// The point was behind a side, so it ISN'T in the frustum
			return false;
		}
	}

	// The point was inside of the frustum (In front of ALL the sides of the frustum)
	return true;
}


//---------------------------------------------------------------------
//-   This determines if a sphere is inside of our frustum
//-   by it's center and radius.
//---------------------------------------------------------------------
bool c_frustum::SphereInFrustum(float pos[], float radius)
{
	// Now this function is almost identical to the PointInFrustum(), except we
	// now have to deal with a radius around the point.  The point is the center of
	// the radius.  So, the point might be outside of the frustum, but it doesn't
	// mean that the rest of the sphere is.  It could be half and half.  So instead of
	// checking if it's less than 0, we need to add on the radius to that.  Say the
	// equation produced -2, which means the center of the sphere is the distance of
	// 2 behind the plane.  Well, what if the radius was 5?  The sphere is still inside,
	// so we would say, if(-2 < -5) then we are outside.  In that case it's bfalse,
	// so we are inside of the frustum, but a distance of 3.  This is reflected below.

	int i;

	if (NULL == pos)
		return  false;

	// Go through all the sides of the frustum
	for (i = 0; i < 6; i++)
	{
		// If the center of the sphere is farther away from the plane than the radius
		if ( this->planes[i][A] * pos[0] + this->planes[i][B] * pos[1] + this->planes[i][C] * pos[2] + this->planes[i][D] <= -radius )
		{
			// The distance was greater than the radius so the sphere is outside of the frustum
			return false;
		}
	}

	// The sphere was inside of the frustum!
	return true;
}


//---------------------------------------------------------------------
//-   This determines if a cube is in or around our frustum
//-   by it's center and 1/2 it's length
//---------------------------------------------------------------------
bool c_frustum::CubeInFrustum(float pos[], float size)
{
	// This test is a bit more work, but not too much more complicated.
	// Basically, what is going on is, that we are given the center of the cube,
	// and half the length.  Think of it like a radius.  Then we checking each point
	// in the cube and seeing if it is inside the frustum.  If a point is found in front
	// of a side, then we skip to the next side.  If we get to a plane that does NOT have
	// a point in front of it, then it will return bfalse.

	// *Note* - This will sometimes say that a cube is inside the frustum when it isn't.
	// This happens when all the corners of the bounding box are not behind any one plane.
	// This is rare and shouldn't effect the overall rendering speed.

	int i;

	if (NULL == pos)
		return false;

	for (i = 0; i < 6; i++)
	{
		if ( this->planes[i][A] * ( pos[0] - size ) + this->planes[i][B] * ( pos[1] - size ) + this->planes[i][C] * ( pos[2] - size ) + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * ( pos[0] + size ) + this->planes[i][B] * ( pos[1] - size ) + this->planes[i][C] * ( pos[2] - size ) + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * ( pos[0] - size ) + this->planes[i][B] * ( pos[1] + size ) + this->planes[i][C] * ( pos[2] - size ) + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * ( pos[0] + size ) + this->planes[i][B] * ( pos[1] + size ) + this->planes[i][C] * ( pos[2] - size ) + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * ( pos[0] - size ) + this->planes[i][B] * ( pos[1] - size ) + this->planes[i][C] * ( pos[2] + size ) + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * ( pos[0] + size ) + this->planes[i][B] * ( pos[1] - size ) + this->planes[i][C] * ( pos[2] + size ) + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * ( pos[0] - size ) + this->planes[i][B] * ( pos[1] + size ) + this->planes[i][C] * ( pos[2] + size ) + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * ( pos[0] + size ) + this->planes[i][B] * ( pos[1] + size ) + this->planes[i][C] * ( pos[2] + size ) + this->planes[i][D] > 0 )
			continue;

		// If we get here, it isn't in the frustum
		return false;
	}

	return true;
}


//---------------------------------------------------------------------
//-   This determines if a bounding box is in or around our frustum
//-   by it's center and 1/2 it's length
//---------------------------------------------------------------------
bool c_frustum::BBoxInFrustum(float corner1[], float corner2[])
{
	int i;

	if ( NULL == corner1 || NULL == corner2 )
		return true;

	for ( i = 0; i < 6; i++ )
	{
		if ( this->planes[i][A] * corner1[0] + this->planes[i][B] * corner1[1] + this->planes[i][C] * corner1[2] + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * corner1[0] + this->planes[i][B] * corner1[1] + this->planes[i][C] * corner2[2] + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * corner1[0] + this->planes[i][B] * corner2[1] + this->planes[i][C] * corner1[2] + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * corner1[0] + this->planes[i][B] * corner2[1] + this->planes[i][C] * corner2[2] + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * corner2[0] + this->planes[i][B] * corner1[1] + this->planes[i][C] * corner1[2] + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * corner2[0] + this->planes[i][B] * corner1[1] + this->planes[i][C] * corner2[2] + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * corner2[0] + this->planes[i][B] * corner2[1] + this->planes[i][C] * corner1[2] + this->planes[i][D] > 0 )
			continue;

		if ( this->planes[i][A] * corner2[0] + this->planes[i][B] * corner2[1] + this->planes[i][C] * corner2[2] + this->planes[i][D] > 0 )
			continue;

		// If we get here, it isn't in the frustum
		return false;
	}

	return true;
}
