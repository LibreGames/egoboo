SUBDIRS = game

egoboodocdir = ${prefix}/doc/egoboo
egoboodoc_DATA = \
   README.Windows\
   README.Linux\
   BUGS\
   TODO\
   game/change.log

EXTRA_DIST = $(egoboodoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
   for specfile in *.spec; do \
      if test -f $$specfile; then \
         cp -p $$specfile $(distdir); \
      fi \
   done 

