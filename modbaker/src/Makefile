#
#   Makefile for ModBaker - a Egoboo module editor
#
CC = g++ -Wall

# What to build ?
MODULES := core

# Look for include files in each of the modules
CFLAGS += $(patsubst %, -I%, $(MODULES))

# each module will add to this
MOD_SRC :=

# include the description for each module
include $(patsubst %, %/module.mk, $(MODULES))

# determine the object files
MOD_OBJ := $(patsubst %.cpp, %.o, $(filter %.cpp, $(MOD_SRC)))

MB_BIN := modbaker
MB_SRC := controls.cpp edit.cpp frustum.cpp global.cpp \
          mesh.cpp modbaker.cpp \
          ogl_debug.cpp ogl_extensions.cpp ogl_include.cpp \
          renderer.cpp renderer_misc.cpp renderer_primitives.cpp \
          SDL_extensions.cpp SDL_GL_extensions.cpp utility.cpp \
          window.cpp
MB_OBJ := ${MB_SRC:.cpp=.o}

# Compiler flags
LDFLAGS := $(shell sdl-config --cflags --libs) -lguichan -lguichan_sdl -lguichan_opengl -lSDL_image -lSDL_ttf -lGL -lGLU

# Normal building
${MB_BIN}:
	${CC} ${MOD_SRC} ${MB_SRC} -o ${MB_BIN} ${LDFLAGS}

# Build modbaker with gdb for debugging
${MB_BIN}_debug:
	${CC} -g ${MOD_SRC} ${MB_SRC} -o ${MB_BIN} ${LDFLAGS}

all:
	${MB_BIN}

clean:
	rm -f ${MOD_OBJ} ${MB_OBJ} ${MB_BIN}
