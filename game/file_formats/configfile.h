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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file file_formats/configfile.h
/// @details Configuration file loading code.

#include "egoboo_typedef.h"

#include "file_common.h"
#include <stdlib.h>

#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
typedef enum ConfigFile_retval_enum ConfigFile_retval;
enum ConfigFile_retval_enum
{
    ConfigFile_fail     = 0,
    ConfigFile_succeed  = 1
};

enum
{
    MAX_CONFIG_SECTION_LENGTH    = 64,
    MAX_CONFIG_KEY_LENGTH        = 64,
    MAX_CONFIG_VALUE_LENGTH      = 256,
    MAX_CONFIG_COMMENTARY_LENGTH = 256
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// A single value in the congiguration file, specified by ["TAG"] = "VALUE"
typedef struct s_ConfigFileValue ConfigFileValue_t;
typedef ConfigFileValue_t *      ConfigFileValuePtr_t;

//--------------------------------------------------------------------------------------------
/// One section of the congiguration file, delimited by {"BLAH"}
typedef struct s_ConfigFileSection ConfigFileSection_t;
typedef ConfigFileSection_t      * ConfigFileSectionPtr_t;

//--------------------------------------------------------------------------------------------
/// The congiguration file
typedef struct s_ConfigFile ConfigFile_t;
typedef ConfigFile_t *      ConfigFilePtr_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern ConfigFilePtr_t   ConfigFile_create();
extern ConfigFile_retval ConfigFile_destroy( ConfigFilePtr_t * ptmp );

extern ConfigFilePtr_t   LoadConfigFile( const char *szFileName, bool_t force );
extern ConfigFile_retval SaveConfigFile( ConfigFilePtr_t pConfigFile );
extern ConfigFile_retval SaveConfigFileAs( ConfigFilePtr_t pConfigFile, const char *szFileName );

extern ConfigFile_retval ConfigFile_GetValue_String( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, char *pValue, Sint32 pValueBufferLength );
extern ConfigFile_retval ConfigFile_GetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, bool_t *pBool );
extern ConfigFile_retval ConfigFile_GetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, Sint32 *pInt );

extern ConfigFile_retval ConfigFile_SetValue_String( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, const char *pValue );
extern ConfigFile_retval ConfigFile_SetValue_Boolean( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, bool_t pBool );
extern ConfigFile_retval ConfigFile_SetValue_Int( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, int pInt );
extern ConfigFile_retval ConfigFile_SetValue_Float( ConfigFilePtr_t pConfigFile, const char *pSection, const char *pKey, float pFloat );

#define _CONFIGFILE_H_
