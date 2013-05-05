# Top level Makefile for Egoboo 2.x

#!!!! do not specify the prefix in this file
#!!!! if you want to specify a prefix, do it on the command line
#For instance: "make PREFIX=$HOME/.local"

ifndef ($(PREFIX),"")
	# define a value for prefix assuming that the program will be installed in the root directory
	PREFIX := /usr
endif


PROJ_NAME := egoboo-2.x

.PHONY: all clean

all: enet egoboo

clean:
	make -C ./enet clean
	make -C ./game clean

enet:
	make -C ./enet all

egoboo:
	make -C ./game all PREFIX=$(PREFIX) PROJ_NAME=$(PROJ_NAME)
	
egoboo_lua:
	make -F Makefile.lua -C game all PREFIX=$(PREFIX) PROJ_NAME=$(PROJ_NAME)

install:

	######################################
	# Thank you for installing egoboo! 
	#
	# The default install of egoboo will require the commandline 
	#     "sudo make install"
	# and the required password
	#
	# If you do not have root access on this machine, 
	# you can specify a prefix on the command line: 
	#     "make install PREFIX=$$HOME/.local"
	# where the environment variable PREFIX specifies a
	# virtual root for your installation. In this example,
	# it is a local installation for this username only.
	#

#	copy the binary to the games folder
	mkdir -p ${PREFIX}/games
	install -m 755 ./game/${PROJ_NAME} ${PREFIX}/games

#	call the installer in the required install directory
	make -C ../install install

	#####################################
	# Egoboo installation is finished
	#####################################

