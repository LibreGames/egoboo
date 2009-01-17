#ifndef frustum_h
#define frustum_h
// We create an enum of the sides so we don't have to call each side 0 or 1.
// This way it makes it more understandable and readable when dealing with frustum sides.
enum e_frustum_side
{
	RIGHT = 0,  ///< The RIGHT side of the frustum
	LEFT = 1,   ///< The LEFT  side of the frustum
	BOTTOM = 2, ///< The BOTTOM side of the frustum
	TOP  = 3,   ///< The TOP side of the frustum
	BACK = 4,   ///< The BACK side of the frustum
	FRONT = 5   ///< The FRONT side of the frustum
};

typedef enum e_frustum_side FrustumSide;

// Like above, instead of saying a number for the ABC and D of the plane, we
// want to be more descriptive.
enum e_plane_data
{
	A = 0,    ///< The X value of the plane's normal
	B = 1,    ///< The Y value of the plane's normal
	C = 2,    ///< The Z value of the plane's normal
	D = 3     ///< The distance the plane is from the origin
};

typedef enum e_plane_data PlaneData;

typedef float frustum_data_t[6][4];

class c_frustum
{
	private:
		frustum_data_t planes;

	public:
		void NormalizePlane(FrustumSide);
		void CalculateFrustum(float *, float *);
		bool PointInFrustum(float *);
		bool SphereInFrustum(float *, float);
		bool CubeInFrustum(float *, float);
		bool BBoxInFrustum(float *, float *);
};
#endif


/////////////////////////////////////////////////////////////////////////////////
//
// * QUICK NOTES *
//
// WOZZERS!  That seemed like an incredible amount to look at, but if you break it
// down, it's not.  Frustum culling is a VERY useful thing when it comes to 3D.
// If you want a large world, there is no way you are going to send it down the
// 3D pipeline every frame and let OpenGL take care of it for you.  That would
// give you a 0.001 frame rate.  If you hit '+' and bring the sphere count up to
// 1000, then take off culling, you will see the HUGE difference it makes.
// Also, you wouldn't really be rendering 1000 spheres.  You would most likely
// use the sphere code for larger objects.  Let me explain.  Say you have a bunch
// of objects, well... all you need to do is give the objects a radius, and then
// test that radius against the frustum.  If that sphere is in the frustum, then you
// render that object.  Also, you won't be rendering a high poly sphere so it won't
// be so slow.  This goes for bounding box's too (Cubes).  If you don't want to
// do a cube, it is really easy to convert the code for rectangles.  Just pass in
// a width and height, instead of just a length.  Remember, it's HALF the length of
// the cube, not the full length.  So it would be half the width and height for a rect.
//
// This is a perfect starter for an octree tutorial.  Wrap you head around the concepts
// here and then see if you can apply this to making an octree.  Hopefully we will have
// a tutorial up and running for this subject soon.  Once you have frustum culling,
// the next step is getting space partitioning.  Either it being a BSP tree of an Octree.
//
// Let's go over a brief overview of the things we learned here:
//
// 1) First we need to abstract the frustum from OpenGL.  To do that we need the
//    projection and modelview matrix.  To get the projection matrix we use:
//
//   glGetFloatv( GL_PROJECTION_MATRIX, /* An Array of 16 floats */ );
//    Then, to get the modelview matrix we use:
//
//   glGetFloatv( GL_MODELVIEW_MATRIX, /* An Array of 16 floats */ );
//
//   These 2 functions gives us an array of 16 floats (The matrix).
//
// 2) Next, we need to combine these 2 matrices.  We do that by matrix multiplication.
//
// 3) Now that we have the 2 matrixes combined, we can abstract the sides of the frustum.
//    This will give us the normal and the distance from the plane to the origin (ABC and D).
//
// 4) After abstracting a side, we want to normalize the plane data.  (A B C and D).
//
// 5) Now we have our frustum, and we can check points against it using the plane equation.
//    Once again, the plane equation (A*x + B*y + C*z + D = 0) says that if, point (X,Y,Z)
//    times the normal of the plane (A,B,C), plus the distance of the plane from origin,
//    will equal 0 if the point (X, Y, Z) lies on that plane.  If it is behind the plane
//    it will be a negative distance, if it's in front of the plane (the way the normal is facing)
//    it will be a positive number.
//
//
// If you need more help on the plane equation and why this works, download our
// Ray Plane Intersection Tutorial at www.GameTutorials.com.
//
// That's pretty much it with frustums.  There is a lot more we could talk about, but
// I don't want to complicate this tutorial more than I already have.
//
// I want to thank Mark Morley for his tutorial on frustum culling.  Most of everything I got
// here comes from his teaching.  If you want more in-depth, visit his tutorial at:
//
// http://www.markmorley.com/opengl/frustumculling.html
//
// Good luck!
//
//
// Ben Humphrey (DigiBen)
// Game Programmer
// DigiBen@GameTutorials.com
// Co-Web Host of www.GameTutorials.com
//
//
