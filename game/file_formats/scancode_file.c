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

/// \file file_formats/scancode_file.c
/// \brief Functions to read and write Egoboo's basicdat/scantag.txt file
/// \details

#include "scancode_file.h"

#include "../log.h"
#include "../input_defs.h"

#include "../egoboo_vfs.h"
#include "../egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int       scantag_count = 0;
scantag_t scantag[MAXTAG];

static void   scantag_reset();
static bool_t scantag_read_one( vfs_FILE *fileread );

//--------------------------------------------------------------------------------------------
// Tag Reading
//--------------------------------------------------------------------------------------------
void scantag_reset()
{
    /// \author ZZ
    /// \details  This function resets the tags
    scantag_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t scantag_read_one( vfs_FILE *fileread )
{
    /// \author ZZ
    /// \details  This function finds the next tag, returning btrue if it found one

    bool_t retval;

    retval = goto_colon( NULL, fileread, btrue ) && ( scantag_count < MAXTAG );
    if ( retval )
    {
        fget_string( fileread, scantag[scantag_count].name, SDL_arraysize( scantag[scantag_count].name ) );
        scantag[scantag_count].value = fget_int( fileread );
        scantag_count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void scantag_read_all_vfs( const char *szFilename )
{
    /// \author ZZ
    /// \details  This function reads the scancode.txt file
    vfs_FILE* fileread;
    int cnt;

    scantag_reset();

    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread )
    {
        log_error( "Cannot read %s.", szFilename );
    }

    // read in all the scantags from the file
    while ( scantag_read_one( fileread ) );

    vfs_close( fileread );

    // make extra scantags to support joystick buttons up to 32 bits
    for ( cnt = 0; cnt < 32; cnt++ )
    {
        STRING str_tmp;

        SDL_snprintf( str_tmp, SDL_arraysize( str_tmp ), "JOY_%d", cnt );
        if ( -1 != scantag_get_value( str_tmp ) ) continue;

        strncpy( scantag[scantag_count].name, str_tmp, SDL_arraysize( scantag[scantag_count].name ) );
        scantag[scantag_count].value = 1 << cnt;
        scantag_count++;
    }
}

//--------------------------------------------------------------------------------------------
int scantag_get_value( const char *string )
{
    /// \author ZZ
    /// \details  This function matches the string with its tag, and returns the value...
    ///    It will return 255 if there are no matches.

    int cnt;

    for ( cnt = 0; cnt < scantag_count; cnt++ )
    {
        if ( 0 == strcmp( string, scantag[cnt].name ) )
        {
            // They match
            return scantag[cnt].value;
        }
    }

    // No matches
    return -1;
}

//--------------------------------------------------------------------------------------------
const char* scantag_get_string( const Sint32 device, const int tag, const bool_t is_key )
{
    /// \author ZF
    /// \details  This translates a input tag value to a string

    int cnt;

    Sint32 loc_device = device;
    bool_t loc_is_key = is_key;

    if ( loc_device >= INPUT_DEVICE_JOY ) loc_device = INPUT_DEVICE_JOY;
    if ( loc_device == INPUT_DEVICE_KEYBOARD ) loc_is_key = btrue;

    for ( cnt = 0; cnt < scantag_count; cnt++ )
    {
        // do not search invalid keys
        if ( loc_is_key )
        {
            if ( 'K' != scantag[cnt].name[0] ) continue;
        }
        else
        {
            switch ( loc_device )
            {
                case INPUT_DEVICE_MOUSE:
                    if ( 'M' != scantag[cnt].name[0] ) continue;
                    break;

                case INPUT_DEVICE_JOY:
                    if ( 'J' != scantag[cnt].name[0] ) continue;
                    break;
            }
        };
        if ( tag == scantag[cnt].value )
        {
            return scantag[cnt].name;
        }
    }

    // No matches
    return "N/A";
}
