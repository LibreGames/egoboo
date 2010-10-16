# note if you change the prefix also update egoboo.sh
PREFIX	:= ${HOME}/.local

EGO_SRC  := \
	camera.cpp char.cpp client.cpp clock.cpp egoboo_console.c egoboo_endian.cpp \
	egoboo_fileutil.cpp egoboo_math.cpp egoboo_setup.cpp egoboo_strutil.cpp \
	egoboo_typedef.cpp egoboo_vfs.cpp enchant.cpp file_common.cpp \
	font_bmp.cpp font_ttf.cpp game.cpp graphic.cpp graphic_fan.cpp \
	graphic_mad.cpp graphic_prt.cpp input.cpp link.cpp log.cpp \
	mad.cpp md2.cpp menu.cpp mesh.cpp network.cpp particle.cpp \
	passage.cpp profile.cpp quest.cpp script.cpp script_compile.cpp \
	script_functions.cpp server.cpp sound.cpp texture.cpp ui.cpp 

EGO_LUA         := ego_wrap.cxx egoboo_console.c lua_console.c
EGO_PLATFORM    := platform/file_linux.c platform/sys_linux.c
EGO_FILE_FORMAT := $(wildcard ./file_formats/*.c)
EGO_EXTENSIONS  := $(wildcard ./extensions/*.c)

EGO_OBJ  := ${EGO_SRC:.c=.o} ${EGO_FILE_FORMAT:.c=.o} ${EGO_EXTENSIONS:.c=.o} ${EGO_PLATFORM:.c=.o} ${EGO_LUA:.c=.o}

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

CC      := gcc
OPT     := -Os -Wall -DPREFIX=\"${PREFIX}\"
INC     := -I. -I.. -I../enet/include -I/usr/include/lua5.1 ${SDLCONF_I} -I./extensions -I./file_formats -I./platform
CFLAGS  := ${OPT} ${INC} -DUSE_LUA_CONSOLE
LDFLAGS := ${SDLCONF_L} -lSDL_ttf -lSDL_mixer -lGL -lGLU -lSDL_image -lphysfs -llua5.1

.PHONY: all clean

EGO_BIN := egoboo-2.x

all: ${EGO_BIN}

${EGO_BIN}: ${EGO_OBJ} ${ENET_OBJ}
	${CC} -o $@ $^ ${LDFLAGS}

clean:
	rm -f ${ENET_OBJ} ${EGO_OBJ} ${EGO_BIN}