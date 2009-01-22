#pragma once

///
/// @file
/// @brief Basic OpenGL Wrapper
/// @details Basic definitions for using OpenGL in Egoboo

#include <SDL_opengl.h>

#ifdef __cplusplus
#    include <cassert>
#    include <cstdio>
extern "C"
{
#else
#    include <assert.h>
#    include <stdio.h>
#endif

#if defined(DEBUG_ATTRIB) && USE_DEBUG
#    define ATTRIB_PUSH(TXT, BITS)    { GLint xx=0; glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); glPushAttrib(BITS); fprintf( stdout, "INFO: PUSH  ATTRIB: %s before attrib stack push. level == %d\n", TXT, xx); }
#    define ATTRIB_POP(TXT)           { GLint xx=0; glPopAttrib(); glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); fprintf( stdout, "INFO: POP   ATTRIB: %s after attrib stack pop. level == %d\n", TXT, xx); }
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX); fprintf( stdout, "INFO: OPEN ATTRIB_GUARD: before attrib stack push. level == %d\n", XX); }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); if(XX!=YY) { fprintf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); } else fprintf( stdout, "INFO: CLOSE ATTRIB_GUARD: after attrib stack pop. level == %d\n", XX); }
#elif USE_DEBUG
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX);  }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); assert(XX==YY); if(XX!=YY) { fprintf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); }  }
#else
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)
#    define ATTRIB_GUARD_CLOSE(XX,YY)
#endif

//--------------------------------------------------------------------------------------------
	typedef float glMatrix[16];
	typedef float glVector4[4];
	typedef float glVector3[3];
	typedef float glVector2[3];

//--------------------------------------------------------------------------------------------
// generic OpenGL lighting struct
	struct s_glLight
	{
		glVector4 emission, diffuse, specular;
		float     shininess[1];
	};
	typedef struct s_glLight glLight_t;

//--------------------------------------------------------------------------------------------
	GLboolean handle_opengl_error( void );

#ifdef __cplusplus
};
#endif
