
/// @file
/// @brief Egoboo OpenGL interface
/// @details Implements the most basic code that Egoboo uses to interface with OpenGL

#include "ogl_include.h"

//--------------------------------------------------------------------------------------------
GLboolean handle_opengl_error()
{
	GLboolean error = GL_TRUE;

	switch ( glGetError() )
	{
		case GL_INVALID_ENUM:      fprintf( stderr, "GLenum argument out of range" ); break;
		case GL_INVALID_VALUE:     fprintf( stderr, "Numeric argument out of range" ); break;
		case GL_INVALID_OPERATION: fprintf( stderr, "Operation illegal in current state" ); break;
		case GL_STACK_OVERFLOW:    fprintf( stderr, "Command would cause a stack overflow" ); break;
		case GL_STACK_UNDERFLOW:   fprintf( stderr, "Command would cause a stack underflow" ); break;
		case GL_OUT_OF_MEMORY:     fprintf( stderr, "Not enough memory left to execute command" ); break;
		default: error = GL_FALSE; break;
	};

	if ( error ) fflush( stderr );

	return error;
};
