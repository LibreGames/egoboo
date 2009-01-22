#include "ogl_extensions.h"
#include "SDL_extensions.h"

#ifdef __cplusplus
extern "C"
{
#endif

	int powerOfTwo( int input );
	int copySurfaceToTexture( SDL_Surface *surface, GLuint texture, GLfloat *texCoords );

#ifdef __cplusplus
};
#endif
