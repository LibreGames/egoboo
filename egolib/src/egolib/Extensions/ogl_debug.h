//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @defgroup _ogl_extensions_ Extensions to OpenGL

/// @file egolib/Extensions/ogl_debug.h
/// @ingroup _ogl_extensions_
/// @brief Definitions for the debugging extensions for OpenGL
/// @details

#pragma once

#include "egolib/Extensions/ogl_include.h"

#undef USE_GL_DEBUG

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern const char *next_cmd;
    extern int next_line;
    extern const char *next_file;

/// grab a text representation of an OpenGL error
    void handle_gl_error();

/// print out the text representation of an OpenGL error generated by handle_gl_error()
    void print_gl_command();

#if defined(USE_GL_DEBUG)

/// a wrapper function for automatically debugging OpenGL function calls
/// Usage: GL_DEBUG( gl* )( param1, param2, ... )
/// @warning Wrapping glEnd() in this manner will generat a multitude of odd errors.
#    define GL_DEBUG(XX) \
    (handle_gl_error(), \
     next_cmd = #XX, \
     next_line = __LINE__, \
     next_file = __FILE__, \
     XX)

/// a special wrapper function that is the replacement for "GL_DEBUG( glEnd )()"
/// this avoids the errors mentioned with the definition of GL_DEBUG()
#    define GL_DEBUG_END() \
    handle_gl_error( void ); \
    next_cmd = "UNKNOWN"; \
    next_line = -1; \
    next_file = "UNKNOWN"; \
    glEnd( void ); \
    glGetError();

#else

/// this macro is set to do nothing if USE_GL_DEBUG is not defined
#    define GL_DEBUG(XX)   XX

/// this macro is set to the normal glEnd() USE_GL_DEBUG is not defined
#    define GL_DEBUG_END() glEnd();

#endif
