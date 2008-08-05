## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS =  \
	game\
	enet

egoboodocdir = ${prefix}/doc/egoboo
egoboodoc_DATA = \
	README.Windows\
	README.Linux\
	BUGS\
	TODO\
	game/change.log

EXTRA_DIST = $(egoboodoc_DATA)

AM_CPPFLAGS = 

INCLUDES = 

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

