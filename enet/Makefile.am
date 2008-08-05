SUBDIRS = include

libenetdocdir = ${prefix}/doc/libenet
libenetdoc_DATA = \
	README\
	LICENSE\
	ChangeLog\
	tutorial.txt\
	design.txt

EXTRA_DIST = $(libenetdoc_DATA)

lib_LIBRARIES = libenet.a
libenet_a_SOURCES = host.c list.c callbacks.c packet.c peer.c protocol.c unix.c win32.c
INCLUDES = -Iinclude/
