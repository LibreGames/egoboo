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
void fs_init( void );
EGO_CONST char *fs_getTempDirectory( void );
EGO_CONST char *fs_getImportDirectory( void );
EGO_CONST char *fs_getGameDirectory( void );
EGO_CONST char *fs_getSaveDirectory( void );

FILE * fs_fileOpen( PRIORITY pri, EGO_CONST char * src, EGO_CONST char * fname, EGO_CONST char * mode );
void fs_fileClose( FILE * pfile );
int  fs_fileIsDirectory( EGO_CONST char *filename );
int  fs_createDirectory( EGO_CONST char *dirname );
int  fs_removeDirectory( EGO_CONST char *dirname );
void fs_deleteFile( EGO_CONST char *filename );
void fs_copyFile( EGO_CONST char *source, EGO_CONST char *dest );
void fs_removeDirectoryAndContents( EGO_CONST char *dirname );
void fs_copyDirectory( EGO_CONST char *sourceDir, EGO_CONST char *destDir );
void empty_import_directory( void );
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
EGO_CONST char *fs_findFirstFile(FS_FIND_INFO * i, EGO_CONST char *searchPath, EGO_CONST char *searchBody, EGO_CONST char *searchExtension );
EGO_CONST char *fs_findNextFile(FS_FIND_INFO * i);
void        fs_findClose(FS_FIND_INFO * i);
