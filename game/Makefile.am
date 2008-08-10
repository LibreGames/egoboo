AM_CPPFLAGS = \
   -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
   -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
   -DPACKAGE_DATA_DIR=\""$(datadir)"\" $(EGOBOO_CFLAGS)

AM_CFLAGS =\
    -Wall\
    -g

bin_PROGRAMS = egoboo

egoboo_SOURCES = \
   AStar.c  \
   camera.c  \
   char.c  \
   Client.c  \
   Clock.c  \
   configfile.c  \
   egoboo_math.c  \
   egoboo_rpc.c  \
   egoboo_strutil.c  \
   egoboo_types.c  \
   egoboo_utility.c  \
   enchant.c  \
   file_common.c  \
   file_lin.c  \
   Font.c  \
   Frustum.c  \
   game.c  \
   graphic.c  \
   graphic_fan.c  \
   graphic_mad.c  \
   graphic_prt.c  \
   id_normals.c  \
   input.c  \
   Log.c  \
   Mad.c  \
   Md2.c  \
   Menu.c  \
   mesh.c  \
   module.c  \
   NetFile.c  \
   Network.c  \
   object.c  \
   ogl_include.c  \
   ogl_texture.c  \
   particle.c  \
   passage.c  \
   Physics.c  \
   script.c  \
   SDL_GetPixel.c  \
   SDL_PutPixel.c  \
   SDL_rotozoom.c  \
   Server.c  \
   sound.c  \
   System_lin.c  \
   Task.c  \
   Ticker.c  \
   Timer.c  \
   Ui.c  \
   ../enet/callbacks.c  \
   ../enet/host.c  \
   ../enet/list.c  \
   ../enet/packet.c  \
   ../enet/peer.c  \
   ../enet/protocol.c  \
   ../enet/unix.c

egoboo_CPPFLAGS = \
   -DHAS_SOCKLEN_T

egoboo_CFLAGS = \
   $(EGOBOO_CFLAGS) \
   -I../enet/include \
   -w

egoboo_LDFLAGS =

egoboo_LDADD = $(EGOBOO_LIBS) \
   -lSDL_ttf \
   -lSDL_mixer \
   -lSDL_image \
   -lGL \
   -lGLU 