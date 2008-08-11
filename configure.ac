AC_INIT(egoboo, 2.7.8)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

PKG_CHECK_MODULES(EGOBOO, sdl)
AC_SUBST(EGOBOO_CFLAGS)
AC_SUBST(EGOBOO_LIBS)

AC_CHECK_LIB(SDL_image, IMG_Load           , ,AC_MSG_ERROR([ *** Unable to find SDL_image library ]))
AC_CHECK_LIB(SDL_mixer, Mix_Linked_Version , ,AC_MSG_ERROR([ *** Unable to find SDL_mixer library ]))
AC_CHECK_LIB(SDL_ttf  , TTF_Linked_Version , ,AC_MSG_ERROR([ *** Unable to find SDL_ttf library   ])) 

AC_OUTPUT([
Makefile
game/Makefile
])
