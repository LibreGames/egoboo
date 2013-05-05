# Top level Makefile for Egoboo 2.x

#!!!! do not specify the prefix in this file
#!!!! if you want to specify a prefix, do it on the command line
#For instance: "make PREFIX=$HOME/.local"

#---------------------
# some project definitions

PROJ_NAME		:= egoboo
PROJ_VERSION	:= 2.x

#---------------------
# the target names

EGO_DIR	:= ./game
EGO_TARGET	:= $(PROJ_NAME)-$(PROJ_VERSION)

ENET_DIR			:= ./enet
ENET_TARGET			:= libenet
ENET_TARGET_EXTENSION	:= a

ENET_LIBRARY            := $(ENET_DIR)/lib/$(ENET_TARGET).$(ENET_TARGET_EXTENSION)

#------------------------------------
# user defined macros

ifndef ($(PREFIX),"")
	# define a value for prefix assuming that the program will be installed in the root directory
	PREFIX := /usr
endif

ifndef ($(INSTALL_DIR),"")
	# the user can specify a non-standard location for "install"
	INSTALL_DIR := ../install
endif

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: enet egoboo

$(ENET_LIBRARY):
	make -C $(ENET_DIR) all PREFIX=$(PREFIX)

clean:
	make -C $(ENET_DIR) clean
	make -C $(EGO_DIR) clean

enet: $(ENET_LIBRARY)

egoboo: enet
	make -C $(EGO_DIR) all PREFIX=$(PREFIX) EGO_TARGET=$(EGO_TARGET) ENET_LIB=$(ENET_LIBRARY)

egoboo_lua: enet
	make -F Makefile.lua -C game all PREFIX=$(PREFIX) PROJ_NAME=$(PROJ_NAME) ENET_LIB=$(ENET_LIBRARY)

install:

	######################################
	# This command will install egoboo using the
	# directory structure currently used in svn repository
	#

#	copy the binary to the games folder
	mkdir -p $(PREFIX)/games
	install -m 755 $(EGO_DIR)/$(PROJ_NAME) $(PREFIX)/games

#	call the installer in the required install directory
	make -C INSTALL_DIR install

	#####################################
	# Egoboo installation is finished
	#####################################
