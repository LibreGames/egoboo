#pragma once

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egobootypedef.h"

#define LINK_COUNT 16

struct sLink
{
    bool_t  valid;
    char    modname[256];
    Uint16  passage;
};

typedef struct sLink Link_t;

extern Link_t LinkList[LINK_COUNT];

bool_t link_export_all();
bool_t link_follow( Link_t list[], int ilink );
bool_t link_follow_modname( const char * modname );
bool_t link_build( const char * fname, Link_t list[] );
