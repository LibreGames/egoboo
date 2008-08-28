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
/// @file
/// @brief Implementation of Egoboo Virtual File System for Linux
/// @details *nix compatible file handling

#include "file_common.h"
#include "egoboo_strutil.h"

#include <stdio.h>
#include <unistd.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#include "egoboo_math.inl"


struct s_fs_find_info_lin
{
  glob_t last_find_glob;
  size_t glob_find_index;
};

typedef struct s_fs_find_info_lin FS_FIND_INFO_LIN;

//File Routines-----------------------------------------------------------
FS_FIND_INFO * fs_find_info_new(FS_FIND_INFO * i)
{
  //fprintf( stdout, "fs_find_info_new()\n");

  if(NULL ==i) return i;

  i->type = FS_LIN;
  i->L    = EGOBOO_NEW( FS_FIND_INFO_LIN );

  return i;
};

void fs_init()
{
}

int fs_fileIsDirectory( const char *filename )
{
  struct stat stats;

  if ( !stat( filename, &stats ) )
    return S_ISDIR( stats.st_mode );

  return 0;
}

int fs_createDirectory( const char *dirname )
{
  /// @details ZZ@> This function makes a new directory

  return mkdir( dirname, 0755 );
}

int fs_removeDirectory( const char *dirname )
{
  /// @details ZZ@> This function removes a directory

  return rmdir( dirname );
}

void fs_deleteFile( const char *filename )
{
  /// @details ZZ@> This function deletes a file

  unlink( filename );
}

void fs_copyFile( const char *source, const char *dest )
{
  /// @details ZZ@> This function copies a file on the local machine

  FILE *sourcef;
  FILE *destf;
  char buf[4096];
  int bytes_read;

  sourcef = fopen( source, "r" );
  if ( !sourcef )
    return;

  destf = fopen( dest, "w" );
  if ( !destf )
  {
    fclose( sourcef );
    return;
  }

  while (( bytes_read = fread( buf, 1, sizeof( buf ), sourcef ) ) )
    fwrite( buf, 1, bytes_read, destf );

  fclose( sourcef );
  fclose( destf );
}

void empty_import_directory( void )
{
  /// @details ZZ@> This function deletes all the TEMP????.OBJ subdirectories in the IMPORT directory

  system( "rm -rf import/temp*.obj\n" );
}



// Read the first directory entry
EGO_CONST char *fs_findFirstFile(FS_FIND_INFO * i, const char *searchDir, const char *searchBody, const char *searchExtension )
{
  size_t szlen = 0;
  char pattern[PATH_MAX];
  char *last_slash;

  // all this szlen stuff is to MAKE SURE we do not go over the valid path length
  // we could possibly issue an error if we go over

  pattern[0] = EOS;
  strncat( pattern, searchDir, PATH_MAX );
  szlen = strlen( pattern );
  str_append_slash( pattern, PATH_MAX );
  szlen++;

  if ( NULL != searchBody  )
  {
    strncat( pattern, searchBody, MAX(0, PATH_MAX - szlen) );
    szlen = strlen( pattern );
  }

  if ( NULL == searchExtension  )
  {
    strncat( pattern, "*", MAX(0, PATH_MAX - szlen) );
    szlen = strlen( pattern );
  }
  else
  {
    // possibly we should use
    // str_append_slash( pattern, MAX(0, PATH_MAX - szlen) );
    // here if searchBody is defined, but since this case doen't occur
    // it is hard to tell what is natural...
    strncat( pattern, searchExtension, MAX(0, PATH_MAX - szlen) );
    szlen = strlen( pattern );
  }

  i->L->last_find_glob.gl_offs = 0;
  glob( pattern, GLOB_NOSORT, NULL, &i->L->last_find_glob );

  if ( !i->L->last_find_glob.gl_pathc )
    return NULL;

  i->L->glob_find_index = 0;
  last_slash = strrchr( i->L->last_find_glob.gl_pathv[i->L->glob_find_index], '/' );

  if ( last_slash )
    return last_slash + 1;

  return NULL; /* should never happen */
}

// Read the next directory entry (NULL if done)
EGO_CONST char *fs_findNextFile( FS_FIND_INFO * i )
{
  char *last_slash;

  ++i->L->glob_find_index;
  if ( i->L->glob_find_index >= i->L->last_find_glob.gl_pathc )
    return NULL;

  last_slash = strrchr( i->L->last_find_glob.gl_pathv[i->L->glob_find_index], '/' );

  if ( last_slash )
    return last_slash + 1;

  return NULL; /* should never happen */
}

// Close anything left open
void fs_findClose(FS_FIND_INFO * i)
{
  globfree( &(i->L->last_find_glob) );

  fs_find_info_delete(i);
}
