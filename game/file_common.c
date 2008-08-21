//********************************************************************************************
//* Egoboo - file_common.c
//*
//* File operations that are shared between various operating systems.
//* OS-specific code goes in file_*.c (such as file_win.c)
//*
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

#include "file_common.h"
#include "Log.h"

#ifndef MAX_PATH
#define MAX_PATH 260 // Same value that Windows uses...
#endif

//--------------------------------------------------------------------------------------------
bool_t fs_find_info_delete(FS_FIND_INFO * i)
{
  bool_t retval = bfalse;

  if(NULL == i) return bfalse;
  if(FS_UNKNOWN == i->type) return bfalse;

  // this is not strictly necessary in C, since EGOBOO_DELETE() or free()
  // doesn't care about the type of its argument
  switch(i->type)
  {
    case FS_WIN32:
      EGOBOO_DELETE(i->W);
      retval = btrue;
      break;

    case FS_LIN:
      EGOBOO_DELETE(i->L);
      retval = btrue;
      break;

    case FS_MAC:
      EGOBOO_DELETE(i->M);
      retval = btrue;
      break;

    default:
    case FS_UNKNOWN:
      // do nothing since the data structure is corrupt
      break;
  };

  i->type = FS_UNKNOWN;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// FIXME: Doesn't handle deleting directories recursively yet.
void fs_removeDirectoryAndContents( EGO_CONST char *dirname )
{
  // ZZ> This function deletes all files in a directory,
  //     and the directory itself

  char filePath[MAX_PATH];
  EGO_CONST char *fileName;
  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  // List all the files in the directory
  fileName = fs_findFirstFile( &fs_finfo, dirname, NULL, "*" );
  while ( NULL != fileName  )
  {
    // Ignore files that start with a ., like .svn for example.
    if ( fileName[0] != '.' )
    {
      snprintf( filePath, MAX_PATH, "%s" SLASH_STRING "%s", dirname, fileName );
      if ( fs_fileIsDirectory( filePath ) )
      {
        // this is recursive delete
        //fs_removeDirectoryAndContents(filePath);
      }
      else
      {
        fs_deleteFile( filePath );
      }
    }

    fileName = fs_findNextFile(&fs_finfo);
  }
  fs_findClose(&fs_finfo);
  fs_removeDirectory( dirname );
}

//--------------------------------------------------------------------------------------------
void fs_copyDirectory( EGO_CONST char *sourceDir, EGO_CONST char *destDir )
{
  // ZZ> This function copies all files in a directory

  char srcPath[MAX_PATH], destPath[MAX_PATH];
  EGO_CONST char *fileName;
  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  // List all the files in the directory
  fileName = fs_findFirstFile( &fs_finfo, sourceDir, NULL, "*" );
  if ( NULL != fileName  )
  {
    // Make sure the destination directory exists
    fs_createDirectory( destDir );

    do
    {
      // Ignore files that begin with a .
      if ( fileName[0] != '.' )
      {
        snprintf( srcPath, MAX_PATH, "%s" SLASH_STRING "%s", sourceDir, fileName );
        snprintf( destPath, MAX_PATH, "%s" SLASH_STRING "%s", destDir, fileName );
        fs_copyFile( srcPath, destPath );
      }

      fileName = fs_findNextFile(&fs_finfo);
    }
    while ( NULL != fileName  );
  }
  fs_findClose(&fs_finfo);
}

//--------------------------------------------------------------------------------------------
/*bool_t fcopy_line(FILE * fileread, FILE * filewrite)
{
  // BB > copy a line of arbitrary length, in chunks of length
  //      sizeof(linebuffer)

  char linebuffer[64];

  if(NULL == fileread || NULL == filewrite) return bfalse;
  if( feof(fileread) || feof(filewrite) ) return bfalse;

  fgets(linebuffer, sizeof(linebuffer), fileread);
  fputs(linebuffer, filewrite);
  while( strlen(linebuffer) == sizeof(linebuffer) )
  {
    fgets(linebuffer, sizeof(linebuffer), fileread);
    fputs(linebuffer, filewrite);
  }

  return btrue;
};*/


//--------------------------------------------------------------------------------------------
FILE * fs_fileOpen( PRIORITY priority, EGO_CONST char * src, EGO_CONST char * fname, EGO_CONST char * mode )
{
  // BB > an alias to the standard fopen() command.  Allows proper logging of
  //      bad calls to fopen()

  FILE * fptmp = NULL;

  if ( NULL == src ) src = "";

  if (  !VALID_CSTR(fname)  )
  {
    log_warning( "%s - fs_fileOpen() - \n\tinvalid file name\n", src );
  }
  else if (  !VALID_CSTR(mode)  )
  {
    log_warning( "%s - fs_fileOpen() - \n\tinvalid file mode\n", src );
  }
  else
  {
    fptmp = fopen( fname, mode );
    if ( NULL == fptmp )
    {
      switch ( priority )
      {
        case PRI_NONE:
        // do nothing
          break;

        case PRI_WARN:
          log_warning( "%s - fs_fileOpen() - \n\tcould not open file \"%s\" in mode \"%s\"\n", src, fname, mode );
          break;

        default:
        case PRI_FAIL:
          log_error( "%s - fs_fileOpen() - \n\tcould not open file \"%s\" in mode \"%s\"\n", src, fname, mode );
          break;
      };

    }
    else
    {
      switch ( priority )
      {
        case PRI_WARN:
        case PRI_FAIL:
          log_info( "%s - fs_fileOpen() - \n\tsuccessfully opened file \"%s\" in mode \"%s\"\n", src, fname, mode );
          break;
      };

    }
  }

  return fptmp;
};

//--------------------------------------------------------------------------------------------
void fs_fileClose( FILE * pfile )
{
  // BB > an alias to the standard fclose() command.

  if ( NULL == pfile )
  {
    log_warning( "fs_fileClose() - \n\tinvalid file\n" );
  }
  else
  {
    fclose( pfile );
  };
};
