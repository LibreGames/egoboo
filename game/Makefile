# Do not run this file. Run the Makefile in the parent directory, instead

EGO_BIN := $(PROJ_NAME)

#---------------------
# the source files

EGO_SRC         := $(*.c)
EGO_PLATFORM    := platform/file_linux.c platform/sys_linux.c
EGO_FILE_FORMAT := $(wildcard ./file_formats/*.c)
EGO_EXTENSIONS  := $(wildcard ./extensions/*.c)

EGO_OBJ := ${EGO_SRC:.c=.o} ${EGO_FILE_FORMAT:.c=.o} ${EGO_EXTENSIONS:.c=.o} ${EGO_PLATFORM:.c=.o}

#---------------------
# the enet library

ifndef ($(ENET_LIB),"")
	# this should be specified by the root Makefile
	ENET_LIB := ../enet/libenet
endif


#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

#---------------------
# the compiler options

CC      := gcc
INC     := -I. -I.. -I../enet/include ${SDLCONF_I} -I./extensions -I./file_formats -I./platform
LDFLAGS := ${SDLCONF_L} -lSDL_ttf -lSDL_mixer -lGL -lGLU -lSDL_image -lphysfs -l$(ENET_LIB)

# use different options if the environmental variable PREFIX is defined
ifdef ($(PREFIX),"")
	OPT := -Os -Wall
else
	OPT := -Os -Wall -DPREFIX=\"${PREFIX}\" -D_NIX_PREFIX
endif

CFLAGS  := ${OPT} ${INC}

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: ${EGO_BIN}

${EGO_BIN}: ${EGO_OBJ} 
	${CC} -o $@ $^ ${LDFLAGS} ${CFLAGS}

clean:
	rm -f ${ENET_OBJ} ${EGO_OBJ} ${EGO_BIN}

