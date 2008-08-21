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

///
/// Configuration file loading.

#include "egoboo_types.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_CONFIG_SECTION_LENGTH  64
#define MAX_CONFIG_KEY_LENGTH   64
#define MAX_CONFIG_VALUE_LENGTH   256
#define MAX_CONFIG_COMMENTARY_LENGTH 256

typedef struct sConfigFileValue
{
  char KeyName[MAX_CONFIG_KEY_LENGTH];
  char *Value;
  char *Commentary;
  struct sConfigFileValue *NextValue;
} ConfigFileValue_t, *ConfigFileValuePtr_t;

typedef struct sConfigFileSection
{
  char SectionName[MAX_CONFIG_SECTION_LENGTH];
  struct sConfigFileSection *NextSection;
  ConfigFileValuePtr_t FirstValue;
} ConfigFileSection_t, *ConfigFileSectionPtr_t;

typedef struct sConfigFile
{
  FILE *f;
  ConfigFileSectionPtr_t ConfigSectionList;

  ConfigFileSectionPtr_t CurrentSection;
  ConfigFileValuePtr_t  CurrentValue;
} ConfigFile_t, *ConfigFilePtr_t;


// util
void ConvertToKeyCharacters( char *pStr );

//
ConfigFilePtr_t OpenConfigFile( EGO_CONST char *pPath );

//
Sint32 GetConfigValue( ConfigFilePtr_t pConfigFile, EGO_CONST char *pSection, EGO_CONST char *pKey, char *pValue, Sint32 pValueBufferLength );
Sint32 GetConfigBooleanValue( ConfigFilePtr_t pConfigFile, EGO_CONST char *pSection, EGO_CONST char *pKey, bool_t *pBool );
Sint32 GetConfigIntValue( ConfigFilePtr_t pConfigFile, EGO_CONST char *pSection, EGO_CONST char *pKey, Sint32 *pInt );

//
Sint32 SetConfigValue( ConfigFilePtr_t pConfigFile, EGO_CONST char *pSection, EGO_CONST char *pKey, EGO_CONST char *pValue );
Sint32 SetConfigBooleanValue( ConfigFilePtr_t pConfigFile, EGO_CONST char *pSection, EGO_CONST char *pKey, bool_t pBool );
Sint32 SetConfigIntValue( ConfigFilePtr_t pConfigFile, EGO_CONST char *pSection, EGO_CONST char *pKey, int pInt );
Sint32 SetConfigFloatValue( ConfigFilePtr_t pConfigFile, EGO_CONST char *pSection, EGO_CONST char *pKey, float pFloat );

//
void CloseConfigFile( ConfigFilePtr_t pConfigFile );

//
void SaveConfigFile( ConfigFilePtr_t pConfigFile );
Sint32 SaveConfigFileAs( ConfigFilePtr_t pConfigFile, EGO_CONST char *pPath );

