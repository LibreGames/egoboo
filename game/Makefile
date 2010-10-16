# Do not run this file. Run the Makefile in the parent directory, instead

EGO_BIN := $(PROJ_NAME)

#---------------------
# the source files

EGO_SRC  := \
	bbox.cpp bsp.cpp camera.cpp char.cpp client.cpp clock.cpp collision.cpp \
	egoboo_console.c egoboo_display.cpp egoboo_endian.cpp egoboo_fileutil.cpp \
	egoboo_math.cpp egoboo_setup.cpp egoboo_strutil.cpp egoboo_typedef.cpp \
	egoboo_vfs.cpp enchant.cpp file_common.cpp font_bmp.cpp font_ttf.cpp game.cpp \
	graphic.cpp graphic_fan.cpp graphic_mad.cpp graphic_prt.cpp hash.cpp IDSZ_map.cpp \
	input.cpp lighting.cpp link.cpp log.cpp mad.cpp md2.cpp menu.cpp mesh.cpp network.cpp \
	particle.cpp passage.cpp profile.cpp quest.cpp script.cpp script_compile.cpp \
	script_functions.cpp server.cpp sound.cpp texture.cpp ui.cpp egoboo.cpp physics.cpp\
	egoboo_process.cpp ChrList.cpp EncList.cpp PrtList.cpp egoboo_object.cpp

EGO_PLATFORM    := platform/file_linux.c platform/sys_linux.c
EGO_FILE_FORMAT := $(wildcard ./file_formats/*.c)
EGO_EXTENSIONS  := $(wildcard ./extensions/*.c)

EGO_OBJ := ${EGO_SRC:.c=.o} ${EGO_FILE_FORMAT:.c=.o} ${EGO_EXTENSIONS:.c=.o} ${EGO_PLATFORM:.c=.o}

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

#---------------------
# the compiler options

CC      := gcc
INC     := -I. -I.. -I../enet/include ${SDLCONF_I} -I./extensions -I./file_formats -I./platform
LDFLAGS := ${SDLCONF_L} -L../enet/lib -lSDL_ttf -lSDL_mixer -lGL -lGLU -lSDL_image -lphysfs -lenet

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

