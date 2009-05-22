#
#   Makefile for ModBaker - a Egoboo module editor
#
CC = g++ -Wall

# What to build ?
MB_BIN := modbaker
MB_SRC := controls.cpp edit.cpp frustum.cpp global.cpp \
          mesh.cpp mesh_misc.cpp modbaker.cpp \
          ogl_debug.cpp ogl_extensions.cpp ogl_include.cpp \
          os_linux.cpp \
          renderer.cpp renderer_misc.cpp renderer_primitives.cpp \
          SDL_extensions.cpp SDL_GL_extensions.cpp utility.cpp \
          window.cpp
MB_OBJ := ${MB_SRC:.cpp=.o}

# Compiler flags
LDFLAGS := $(shell sdl-config --cflags --libs) -lguichan -lguichan_sdl -lguichan_opengl -lSDL_image -lSDL_ttf -lGL -lGLU

# Normal building
${MB_BIN}:
	${CC} ${MB_SRC} -o ${MB_BIN} ${LDFLAGS}

# Build modbaker with gdb for debugging
${MB_BIN}_debug:
	${CC} -g ${MB_SRC} -o ${MB_BIN} ${LDFLAGS}

all:
	${MB_BIN}

clean:
	rm -f ${MB_OBJ} ${MB_BIN}
