/* Egoboo - configfile.h
 * Configuration file loading code.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_

#include "egobootypedef.h"

#include <stdio.h>
#include <stdlib.h>

#include "egoboostrutil.h"

#define MAX_CONFIG_SECTION_LENGTH  64
#define MAX_CONFIG_KEY_LENGTH   64
#define MAX_CONFIG_VALUE_LENGTH   256
#define MAX_CONFIG_COMMENTARY_LENGTH 256

typedef struct ConfigFileValue ConfigFileValue;
typedef struct ConfigFileValue
{
  char KeyName[MAX_CONFIG_KEY_LENGTH];
  char *Value;
  char *Commentary;
  ConfigFileValue *NextValue;
} *ConfigFileValuePtr;

typedef struct ConfigFileSection ConfigFileSection;
typedef struct ConfigFileSection
{
  char SectionName[MAX_CONFIG_SECTION_LENGTH];
  ConfigFileSection *NextSection;
  ConfigFileValuePtr FirstValue;
} *ConfigFileSectionPtr;

typedef struct ConfigFile
{
  FILE *f;
  ConfigFileSectionPtr ConfigSectionList;

  ConfigFileSectionPtr CurrentSection;
  ConfigFileValuePtr  CurrentValue;
} ConfigFile, *ConfigFilePtr;


// util
extern void ConvertToKeyCharacters(char *pStr);

//
extern ConfigFilePtr OpenConfigFile(const char *pPath);

//
extern Sint32 GetConfigValue(ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, char *pValue, Sint32 pValueBufferLength);
extern Sint32 GetConfigBooleanValue(ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, bool_t *pBool);
extern Sint32 GetConfigIntValue(ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, Sint32 *pInt);

//
extern Sint32 SetConfigValue(ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, const char *pValue);
extern Sint32 SetConfigBooleanValue(ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, int pBool);
extern Sint32 SetConfigIntValue(ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, int pInt);
extern Sint32 SetConfigFloatValue(ConfigFilePtr pConfigFile, const char *pSection, const char *pKey, float pFloat);

//
extern void CloseConfigFile(ConfigFilePtr pConfigFile);

//
extern void SaveConfigFile(ConfigFilePtr pConfigFile);
extern Sint32 SaveConfigFileAs(ConfigFilePtr pConfigFile, const char *pPath);


#endif // #ifndef _CONFIGFILE_H_

