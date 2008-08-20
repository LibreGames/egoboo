// Filesystem functions

#pragma once

#include "egoboo_types.h"

#include <stdio.h>

//---------------------------------------------------------------------------------------------

enum e_priority
{
  PRI_NONE = 0,
  PRI_WARN,
  PRI_FAIL
};
typedef enum e_priority PRIORITY;

//---------------------------------------------------------------------------------------------
void fs_init();
const char *fs_getTempDirectory();
const char *fs_getImportDirectory();
const char *fs_getGameDirectory();
const char *fs_getSaveDirectory();

FILE * fs_fileOpen( PRIORITY pri, const char * src, const char * fname, const char * mode );
void fs_fileClose( FILE * pfile );
int  fs_fileIsDirectory( const char *filename );
int  fs_createDirectory( const char *dirname );
int  fs_removeDirectory( const char *dirname );
void fs_deleteFile( const char *filename );
void fs_copyFile( const char *source, const char *dest );
void fs_removeDirectoryAndContents( const char *dirname );
void fs_copyDirectory( const char *sourceDir, const char *destDir );
void empty_import_directory();
int  DirGetAttrib( char *fromdir );

//---------------------------------------------------------------------------------------------
// Enumerate directory contents
enum e_fs_type
{
  FS_UNKNOWN = -1,
  FS_WIN32 = 0,
  FS_LIN,
  FS_MAC
};
typedef enum e_fs_type FS_TYPE;

struct s_fs_find_info_win32;
struct s_fs_find_info_lin;
struct s_fs_find_info_mac;

struct s_fs_find_info
{
  FS_TYPE type;

  union
  {
    struct s_fs_find_info_win32 * W;
    struct s_fs_find_info_lin   * L;
    struct s_fs_find_info_mac   * M;
  };

};

typedef struct s_fs_find_info FS_FIND_INFO;

FS_FIND_INFO * fs_find_info_new(FS_FIND_INFO * i);
bool_t         fs_find_info_delete(FS_FIND_INFO * i);

//---------------------------------------------------------------------------------------------
const char *fs_findFirstFile(FS_FIND_INFO * i, const char *searchPath, const char *searchBody, const char *searchExtension );
const char *fs_findNextFile(FS_FIND_INFO * i);
void        fs_findClose(FS_FIND_INFO * i);
