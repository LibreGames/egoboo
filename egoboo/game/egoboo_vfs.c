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

/// @file egoboo_vfs.c
/// @brief Implementation of the Egoboo virtual file system
/// @details

#include "egoboo_vfs.h"

#include "file_common.h"
#include "log.h"

#include "egoboo_strutil.h"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"

#include <physfs.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define VFS_MAX_PATH 1024

#define BAIL_MACRO(TEST,STR)    if( !(TEST) ) log_error( "egoboo vfs system encountered a fatal error - %s", STR );

#if defined(__FUNCSIG__)
#    define BAIL_IF_NOT_INIT()    if( !_vfs_initialized ) log_error( "egoboo vfs function called while the system was not initialized -- %s\n", __FUNCSIG__ );
#elif defined(__FUNCTION__)
#    define BAIL_IF_NOT_INIT()    if( !_vfs_initialized ) log_error( "egoboo vfs function called while the system was not initialized -- %s\n", __FUNCTION__ );
#else
#    define BAIL_IF_NOT_INIT()    if( !_vfs_initialized ) log_error( "egoboo vfs function called while the system was not initialized -- \"%s\"(line %d)\n", __FILE__, __LINE__ );
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef char VFS_PATH[VFS_MAX_PATH];

#define MAX_MOUNTINFO 128

/// The bits specifying the possible errors
/*enum e_vfs_error_bits
{
    VFS_EOF   = ( 1 << 0 ),
    VFS_ERROR = ( 1 << 1 )
};

/// What type of file is actually being referenced in u_vfs_fileptr
enum e_vfs_mode
{
    vfs_unknown = 0,
    vfs_cfile,
    vfs_physfs
};
typedef enum e_vfs_mode vfs_mode_t;

/// An anonymized pointer type
union u_vfs_fileptr
{
    void        * u;
    FILE        * c;
    PHYSFS_File * p;
};
typedef union u_vfs_fileptr vfs_fileptr_t;

/// A container holding either a FILE * or a PHYSFS_File *, and translated error states
struct vfs_FILE
{
    BIT_FIELD     flags;    // flags for stuff like ferror() that doesn't clear every time a filesystem call is made
    vfs_mode_t    type;
    vfs_fileptr_t ptr;
 };*/

struct vfs_FILE {
     PHYSFS_File * file;
     char fileName[256];
     bool_t errFlag;
     char errString[512];
 };

/// A container for holding all the data for a search
struct s_vfs_search_context
{
    char **    file_list;
    char **    ptr;

    char       path[VFS_MAX_PATH];
    char       ext[255];
    BIT_FIELD  bits;

    VFS_PATH found;
};

struct s_vfs_path_data
{
    VFS_PATH mount;

    VFS_PATH full_path;
    VFS_PATH root_path;
    VFS_PATH relative_path;
};
typedef struct s_vfs_path_data vfs_path_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int             _vfs_mount_info_count = 0;
static vfs_path_data_t _vfs_mount_info[MAX_MOUNTINFO];

static bool_t _vfs_atexit_registered = bfalse;

static bool_t _vfs_initialized = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void                   _vfs_exit( void );
static vfs_search_context_t * _vfs_search( vfs_search_context_t ** ctxt );

static bool_t       _vfs_mount_info_add( const char * mount_point, const char * root_path, const char * relative_path );
static int          _vfs_mount_info_matches( const char * mount_point, const char * local_path );
static bool_t       _vfs_mount_info_remove( int cnt );
static int          _vfs_mount_info_search( const char * some_path );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void vfs_init()
{
    VFS_PATH tmp_path;
    const char *userDir;

    fs_init();

    if ( _vfs_initialized ) return;

    // set the root path to be the Data Directory, regardless of the executable's path
    snprintf( tmp_path, SDL_arraysize( tmp_path ), "%s" SLASH_STR, fs_getDataDirectory() );
    PHYSFS_init( tmp_path );

    //---- !!!! make sure the basic directories exist !!!!
    
    userDir = fs_getUserDirectory();

    // ensure that the /user dierectory exists
    if ( !fs_fileIsDirectory( userDir ) )
    {
        fs_createDirectory( userDir );
    }

    // ensure that the /user/debug directory exists
    if ( !fs_fileIsDirectory( userDir ) )
    {
        printf( "WARNING: Cannot create write directory %s\n", userDir );
    }
#if 0
    else
    {
        char tmp_path[1024] = EMPTY_CSTR;

        snprintf( tmp_path, SDL_arraysize( tmp_path ), "%s/debug", userDir );

        str_convert_slash_sys( tmp_path, SDL_arraysize( tmp_path ) );
        fs_createDirectory( tmp_path );
    }
#endif

    // set the write directory to the root user directory
    PHYSFS_setWriteDir( userDir );
    
    // mkdir /user/debug
    PHYSFS_addToSearchPath(userDir, 0);
    if (!PHYSFS_exists( "/debug" ))
        PHYSFS_mkdir( "/debug" );
    PHYSFS_removeFromSearchPath(userDir);

    if ( !_vfs_atexit_registered )
    {
        atexit( _vfs_exit );
        _vfs_atexit_registered = btrue;
    }

    _vfs_initialized = btrue;
}

//--------------------------------------------------------------------------------------------
void _vfs_exit()
{
    PHYSFS_deinit();
}

//--------------------------------------------------------------------------------------------
const char* vfs_getVersion()
{
    /// @details ZF@>  returns the current version of the PhysFS library which was used for compiling the binary
    PHYSFS_Version version;
    static STRING buffer = EMPTY_CSTR;

    //PHYSFS_getLinkedVersion(&version);        //Linked version number
    PHYSFS_VERSION( &version );         //Compiled version number
    snprintf( buffer, SDL_arraysize( buffer ), "%d.%d.%d", version.major, version.minor, version.patch );

    return buffer;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openReadB( const char * filename )
{
    return vfs_openRead(filename);
}

//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openWriteB( const char * filename )
{
    return vfs_openWrite(filename);
}

//--------------------------------------------------------------------------------------------
vfs_FILE * vfs_openAppendB( const char * filename )
{
    return vfs_openAppend(filename);
}
#if 0
//--------------------------------------------------------------------------------------------
const char * vfs_convert_fname_sys( const char * fname )
{
    // PHYSFS and this vfs use a C_SLASH_STR at the front of relative filenames. if this is not removed
    // when converting to system dependent filenames, it will reference the filename relative to the
    // root point, rather than relative to the current directory.
    //
    // Added complication: if you are trying to specify a root filename,
    // then you might expect a SLASH_CHR at the beginning of the pathname.
    // To fix this, call str_convert_slash_sys(), and then check to see if the
    // directory exists in the filesystem.
    //
    // This is not an ideal solution since we may be trying to specify a path for
    // a tile that needs to be created. The optimal solution might be to check to see if
    // the path belongs to a registered virtual mount point?

    static VFS_PATH local_fname = EMPTY_CSTR;

    size_t   offset;
    VFS_PATH copy_fname  = EMPTY_CSTR;
    char *   string_ptr;

    BAIL_IF_NOT_INIT();

    // test for a bad input filename
    if ( INVALID_CSTR( fname ) )
    {
        strncpy( local_fname, SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    if ( VFS_TRUE == _vfs_mount_info_search( fname ) )
    {
        // this path contains a virtual mount point
        EGOBOO_ASSERT( bfalse );
        return local_fname;
    }

    // make a local copy of the original filename, in case fname is
    // a string literal or a pointer to local_fname
    strncpy( copy_fname, fname, SDL_arraysize( copy_fname ) );

    // convert the path string to local notation
    string_ptr = str_convert_slash_sys( copy_fname, strlen( fname ) );
    if ( !VALID_CSTR( string_ptr ) )
    {
        strncpy( local_fname, SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // resolve the conflict between
    //    -1- directories relative to the PHYSFS root path starting with NET_SLASH
    // and
    //    -2- directories relative to the root of the filesystem beginning with a slash
    // by trying to find the original path
    //
    // The following method is not exactly secure, since it would allow access to generic
    // root directories using this code...

    // if the path already exists, just return the path
    if ( fs_fileExists( copy_fname ) )
    {
        strncpy( local_fname, copy_fname, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // if the path didn't exist it might be because it contains a file that has not yet been created...
    // no solution to that problem, yet.

    // ---- if we got to this point, we need to strip off any beginning slashes

    offset = 0;
    if ( '.' == copy_fname[0] && '.' == copy_fname[1] )
    {
        offset++;
    }

    // the string has already been converted to a system filename, so just check SLASH_CHR
    while ( SLASH_CHR == copy_fname[offset] && offset < SDL_arraysize( copy_fname ) )
    {
        offset++;
    }

    // copy the proper relative filename
    strncpy( local_fname, copy_fname + offset, SDL_arraysize( local_fname ) );

    return local_fname;
}
#endif

//--------------------------------------------------------------------------------------------
const char * vfs_convert_fname( const char * fname )
{
    VFS_PATH        copy_fname  = EMPTY_CSTR;
    static VFS_PATH local_fname = EMPTY_CSTR;

    BAIL_IF_NOT_INIT();

    // test for a bad iput filename
    if ( INVALID_CSTR( fname ) )
    {
        strncpy( local_fname, NET_SLASH_STR, SDL_arraysize( local_fname ) );
        return local_fname;
    }

    // make a copy of the original filename, in case fname is
    // a literal string or a pointer to local_fname
    strncpy( copy_fname, fname, SDL_arraysize( copy_fname ) );

    if ( _vfs_mount_info_search( copy_fname ) )
    {
        snprintf( local_fname, SDL_arraysize( local_fname ), "%s", copy_fname );
    }
    else if ( NET_SLASH_CHR == copy_fname[0] || WIN32_SLASH_CHR == copy_fname[0] )
    {
        snprintf( local_fname, SDL_arraysize( local_fname ), "%s", copy_fname );
    }
    else
    {
        snprintf( local_fname, SDL_arraysize( local_fname ), NET_SLASH_STR "%s", copy_fname );
    }

    return str_convert_slash_net( local_fname, strlen( local_fname ) );
}
#if 0

//--------------------------------------------------------------------------------------------
const char * _vfs_potential_mount_point( const char * some_path, const char ** pstripped_pos )
{
    // This helper function was devised to read the first potential directory name
    // from the given path. Because:
    //
    // 1) To use the PHYSFS_getMountPoint() function, the mount point you are testing
    //    must be a system-dependent pathname, ending with a system-dependent path
    //    separator.
    //
    // 2) if you use PHYSFS_getRealDir() to give you a path to a file, what it really does is
    //    resolve the mount point, rather than the whole path name (path name without the
    //    file, that is). To do the job we want in vfs_resolveReadFilename(), we need to be able
    //    to strip the mount point off of the path that we were given before prepending the
    //    path returned by PHYSFS_getRealDir()

    const char * ptmp, *path_begin, *path_end;
    static VFS_PATH found_path;

    found_path[0] = CSTR_END;

    BAIL_IF_NOT_INIT();

    if ( !VALID_CSTR( some_path ) ) return found_path;

    ptmp = some_path;

    path_begin = some_path;
    path_end   = some_path + VFS_MAX_PATH - 1;

    // strip any starting slashes
    for ( ptmp = path_begin; ptmp < path_end; ptmp++ )
    {
        if ( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp )
        {
            path_begin = ptmp;
            break;
        }
    }

    // identify the slash after the mount point
    ptmp = strpbrk( path_begin, "/\\" );
    if ( NULL != ptmp ) path_end = ptmp + 1;

    // make sre we do not have an empty string
    if ( path_end > path_begin )
    {
        size_t count = path_end - path_begin;
        strncpy( found_path, path_begin, count );
        found_path[count] = CSTR_END;
    }

    // export the beginning of the string after the mount point, if possible
    if ( NULL != pstripped_pos )
    {
        *pstripped_pos = path_end;
    }

    // return the potential mount point in system-dependent format
    return str_convert_slash_sys( found_path, strlen( found_path ) );
}

//--------------------------------------------------------------------------------------------
void vfs_listSearchPaths()
{
    //JJ> Lists all search paths that PhysFS uses (for debug use)

    char **i; int q;

    BAIL_IF_NOT_INIT();

    printf( "LISTING ALL PHYSFS SEARCH PATHS:\n" );
    printf( "----------------------------------\n" );
    for ( i = PHYSFS_getSearchPath(); *i != NULL; i++ )   printf( "[%s] is in the search path.\n", *i );
    printf( "----------------------------------\n" );
    
    for (q = 0; q < _vfs_mount_info_count; q++) {
        vfs_path_data_t data = _vfs_mount_info[q];
        printf("mount %s, relpath %s, fullpath %s, rootpath %s\n", data.mount, data.relative_path, data.full_path, data.root_path);
    }
}

//--------------------------------------------------------------------------------------------
const char * vfs_resolveReadFilename( const char * src_filename )
{
    static STRING read_name_str = EMPTY_CSTR;
    VFS_PATH      loc_fname = EMPTY_CSTR, szTemp = EMPTY_CSTR;
    int           retval_len = 0;
    const char   *retval = NULL;

    BAIL_IF_NOT_INIT();

    if ( INVALID_CSTR( src_filename ) ) return NULL;

    // make a copy of the szTemp, in case we are passed a string literal
    // as the argument of this function
    strncpy( szTemp, src_filename, SDL_arraysize( szTemp ) );

    // make a temporary copy of the given filename with system-dependent slashes
    // to see if the filename is already resolved
    strncpy( loc_fname, szTemp, SDL_arraysize( loc_fname ) );
    str_convert_slash_sys( loc_fname, SDL_arraysize( loc_fname ) );

    if ( fs_fileExists( loc_fname ) )
    {
        strncpy( read_name_str, loc_fname, SDL_arraysize( read_name_str ) );

        return read_name_str;
    }

    // make another copy of the local filename
    // and make sure that PHYSFS gets the filename with the slashes it wants
    strncpy( loc_fname, vfs_convert_fname( szTemp ), SDL_arraysize( loc_fname ) );

    retval = NULL;
    retval_len = 0;
    if ( PHYSFS_isDirectory( loc_fname ) )
    {
        retval = PHYSFS_getRealDir( loc_fname );

        if ( VALID_CSTR( retval ) )
        {
            const char * ptmp = vfs_mount_info_strip_path( loc_fname );

            if ( VALID_CSTR( ptmp ) )
            {
                snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s/%s", retval, ptmp );
            }
            else
            {
                snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s/", retval );
            }

            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
    }
    else
    {
        const char * tmp_dirname;
        const char * ptmp = loc_fname;

        // make PHYSFS grab the actual directory
        tmp_dirname = PHYSFS_getRealDir( loc_fname );

        if ( INVALID_CSTR( tmp_dirname ) )
        {
            // not found... just punt
            strncpy( read_name_str, loc_fname, SDL_arraysize( read_name_str ) );
            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
        else
        {
            ptmp = vfs_mount_info_strip_path( loc_fname );

            snprintf( read_name_str, SDL_arraysize( read_name_str ), "%s/%s", tmp_dirname, ptmp );
            retval     = read_name_str;
            retval_len = SDL_arraysize( read_name_str );
        }
    }

    if ( VALID_CSTR( retval ) && retval_len > 0 )
    {
        retval = str_convert_slash_sys(( char* )retval, retval_len );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * vfs_resolveWriteFilename( const char * src_filename )
{
    static VFS_PATH  szFname = EMPTY_CSTR;
    VFS_PATH szTemp;
    const  char    * write_dir;

    BAIL_IF_NOT_INIT();

    if ( INVALID_CSTR( src_filename ) ) return szFname;

    // make a copy of the src_filename, in case we are passed a string literal
    // as the argument of this function
    strncpy( szTemp, src_filename, SDL_arraysize( szTemp ) );

    write_dir = PHYSFS_getWriteDir();
    if ( NULL == write_dir )
    {
        log_warning( "PhysFS could not get write directory!\n" );
        return NULL;
    }

    // append the write_dir to the szTemp to get the total path
    snprintf( szFname, SDL_arraysize( szFname ), "%s" SLASH_STR "%s", write_dir, szTemp );

    // make sure that the slashes are correct for this system, and that they are not doubled

    return str_convert_slash_sys( szFname, SDL_arraysize( szFname ) );
}

#endif


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char ** vfs_enumerateFiles( const char * dir_name )
{
    BAIL_IF_NOT_INIT();

    return PHYSFS_enumerateFiles( vfs_convert_fname( dir_name ) );
}

//--------------------------------------------------------------------------------------------
void    vfs_freeList( void * listVar )
{
    BAIL_IF_NOT_INIT();

    PHYSFS_freeList( listVar );
}

//--------------------------------------------------------------------------------------------
void _vfs_findClose( vfs_search_context_t * ctxt )
{
    BAIL_IF_NOT_INIT();

    if ( NULL == ctxt ) return;

    if ( NULL != ctxt->file_list )
    {
        PHYSFS_freeList( ctxt->file_list );
        ctxt->file_list = NULL;
    }
    ctxt->ptr       = NULL;
}

//--------------------------------------------------------------------------------------------
vfs_search_context_t * _vfs_search( vfs_search_context_t ** pctxt )
{
    const char * retval = NULL;
    static VFS_PATH  path_buffer = EMPTY_CSTR;

    BAIL_IF_NOT_INIT();

    if ( NULL == pctxt ) return NULL;

    // uninitialized file list?
    if ( NULL == ( *pctxt ) || NULL == ( *pctxt )->file_list )
    {
        ( *pctxt )->found[0] = CSTR_END;
        return ( *pctxt );
    }

    // emptry file list?
    if ( NULL == *(( *pctxt )->file_list ) )
    {
        goto _vfs_search_file_error;
    }

    if ( NULL == ( *pctxt )->ptr )
    {
        // if we haven't begun the search yet, get started
        ( *pctxt )->ptr = ( *pctxt )->file_list;
    }
    else
    {
        ( *pctxt )->ptr++;
    }

    // NULL == *((*pctxt)->ptr) signals the end of the list
    // if we exhausted the list, reset everything
    if ( NULL == ( *pctxt )->ptr || NULL == *(( *pctxt )->ptr ) )
    {
        goto _vfs_search_file_error;
    }

    // search for the correct extension (if any)
    retval = NULL;
    if ( CSTR_END == *( *pctxt )->ext )
    {
        int  found;

        for ( /* nothing */; NULL != *(( *pctxt )->ptr ); ( *pctxt )->ptr++ )
        {
            int is_dir;
            char * loc_path;

            if ( INVALID_CSTR(( *pctxt )->path ) )
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), NET_SLASH_STR "%s", *(( *pctxt )->ptr ) );
            }
            else
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), "%s" NET_SLASH_STR "%s", ( *pctxt )->path, *(( *pctxt )->ptr ) );
            }

            loc_path = ( char * )vfs_convert_fname( path_buffer );

            // have we found the correct type of object?
            found  = VFS_FALSE;
            is_dir = vfs_isDirectory( loc_path );

            if ( 0 != ( VFS_SEARCH_FILE & ( *pctxt )->bits ) )
            {
                found = !is_dir;
            }
            else if ( 0 != ( VFS_SEARCH_DIR & ( *pctxt )->bits ) )
            {
                found = is_dir;
            }
            else
            {
                found = VFS_TRUE;
            }

            if ( found )
            {
                retval = loc_path;
                break;
            }
        }
    }
    else
    {
        size_t extension_length = strlen(( *pctxt )->ext );

        // scan through the list
        for ( /* nothing */; NULL != *(( *pctxt )->ptr ); ( *pctxt )->ptr++ )
        {
            int found, is_dir;
            size_t string_length;
            char * sztest;
            char * loc_path;

            //---- have we found the correct type of object?

            if ( INVALID_CSTR(( *pctxt )->path ) )
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), NET_SLASH_STR "%s", *(( *pctxt )->ptr ) );
            }
            else
            {
                snprintf( path_buffer, SDL_arraysize( path_buffer ), "%s" NET_SLASH_STR "%s", ( *pctxt )->path, *(( *pctxt )->ptr ) );
            }

            loc_path = ( char * )vfs_convert_fname( path_buffer );

            found = VFS_FALSE;
            is_dir = vfs_isDirectory( loc_path );
            if ( 0 != ( VFS_SEARCH_FILE & ( *pctxt )->bits ) )
            {
                found = !is_dir;
            }
            else if ( 0 != ( VFS_SEARCH_DIR & ( *pctxt )->bits ) )
            {
                found = is_dir;
            }
            else
            {
                found = VFS_TRUE;
            }

            if ( !found ) continue;

            //---- does the extension match?
            sztest = *(( *pctxt )->ptr );

            // get the length
            string_length = strlen( sztest );

            // grab the last bit of the test string
            if ((( signed )string_length - ( signed )extension_length ) >= 0 )
            {
                sztest += ( string_length - extension_length );
            }
            else
            {
                sztest = NULL;
            }
            if ( INVALID_CSTR( sztest ) ) continue;

            if ( 0 == strcmp( sztest, ( *pctxt )->ext ) )
            {
                retval = loc_path;
                break;
            }
        };
    }

    // reset the path buffer
    path_buffer[0] = CSTR_END;

    // test for the end condition again
    if ( NULL == ( *pctxt )->ptr || NULL == *(( *pctxt )->ptr ) )
    {
        vfs_findClose( pctxt );
        retval = NULL;
    }
    else
    {
        if ( 0 != ( VFS_SEARCH_BARE & ( *pctxt )->bits ) )
        {
            // do the "bare" option
            retval = NULL;
            if ( VALID_CSTR( *(( *pctxt )->ptr ) ) )
            {
                strncpy( path_buffer, *(( *pctxt )->ptr ), SDL_arraysize( path_buffer ) );
                retval = path_buffer;
            }
        }
        else
        {
            // return the full path
            if ( VALID_CSTR( retval ) )
            {
                strncpy( path_buffer, retval, SDL_arraysize( path_buffer ) );
                retval = path_buffer;
            }
            else
            {
                retval = NULL;
            }
        }
    }

    if ( NULL == retval )
    {
        if ( NULL != *pctxt )
        {
            ( *pctxt )->found[0] = CSTR_END;
        }
    }
    else
    {
        strncpy(( *pctxt )->found, path_buffer, SDL_arraysize(( *pctxt )->found ) );
    }

    return *pctxt;

_vfs_search_file_error:

    vfs_findClose( pctxt );
    return NULL;
}

//--------------------------------------------------------------------------------------------
vfs_search_context_t * vfs_findFirst( const char * search_path, const char * search_extension, Uint32 search_bits )
{
    vfs_search_context_t * ctxt;

    BAIL_IF_NOT_INIT();

    // create the new context
    ctxt = EGOBOO_NEW( vfs_search_context_t );
    if ( NULL == ctxt ) return NULL;

    // grab all the files
    ctxt->file_list = vfs_enumerateFiles( vfs_convert_fname( search_path ) );
    ctxt->ptr       = NULL;

    // no search list generated
    if ( NULL == ctxt->file_list )
    {
        return NULL;
    }

    // empty search list
    if ( NULL == *( ctxt->file_list ) )
    {
        vfs_findClose( &ctxt );
        return NULL;
    }

    // set the search extension
    if ( INVALID_CSTR( search_extension ) )
    {
        ctxt->ext[0] = CSTR_END;
    }
    else
    {
        snprintf( ctxt->ext, SDL_arraysize( ctxt->ext ), ".%s", search_extension );
    }

    // set the search path
    if ( INVALID_CSTR( search_path ) )
    {
        ctxt->path[0] = CSTR_END;
    }
    else
    {
        strncpy( ctxt->path, search_path, SDL_arraysize( ctxt->path ) );
    }

    ctxt->bits = search_bits;

    ctxt = _vfs_search( &ctxt );

    return ctxt;
}

//--------------------------------------------------------------------------------------------
vfs_search_context_t * vfs_findNext( vfs_search_context_t ** pctxt )
{
    // if there are no files, return an error value

    BAIL_IF_NOT_INIT();

    if ( NULL == pctxt || NULL == *pctxt ) return NULL;

    if ( NULL == ( *pctxt )->file_list )
    {
        return NULL;
    }

    *pctxt = _vfs_search( pctxt );

    return *pctxt;
}

//--------------------------------------------------------------------------------------------
void vfs_findClose( vfs_search_context_t ** ctxt )
{
    BAIL_IF_NOT_INIT();

    if ( NULL != ctxt )
    {
        _vfs_findClose( *ctxt );
        EGOBOO_DELETE(( *ctxt ) );
    }
}

// TODO: Change to PHYSFS only

static bool_t _vfs_removeDirectoryAndContents( const char * dirname, bool_t recursive )
{
    char ** fileList = PHYSFS_enumerateFiles(dirname);
    char ** i;
    VFS_PATH tmp;
    for (i = fileList; *i != NULL; i++) {
        snprintf(tmp, SDL_arraysize(tmp), "%s/%s", dirname, *i);
        if (PHYSFS_isDirectory(tmp) && recursive)
            _vfs_removeDirectoryAndContents(tmp, recursive);
        else
            PHYSFS_delete(tmp);
    }
    PHYSFS_freeList(fileList);
    return PHYSFS_delete(dirname) == 0 ? bfalse : btrue;
}
//--------------------------------------------------------------------------------------------
int vfs_removeDirectoryAndContents( const char * dirname, int recursive )
{
    // buffer the directory delete through PHYSFS, so that we so not access functions that
    // we have no right to! :)

    const char *  write_dir;

    BAIL_IF_NOT_INIT();

    // make sure that this is a valid directory
    /*write_dir = vfs_resolveWriteFilename( dirname );
    if ( !fs_fileIsDirectory( write_dir ) ) return VFS_FALSE;

    fs_removeDirectoryAndContents( write_dir, recursive );*/
    _vfs_removeDirectoryAndContents(dirname, recursive);

    return VFS_TRUE;
}



//--------------------------------------------------------------------------------------------
int vfs_copyDirectory( const char *sourceDir, const char *destDir )
{
    /// @details ZZ@> This function copies all files in a directory
    VFS_PATH srcPath = EMPTY_CSTR, destPath = EMPTY_CSTR;
    const char *fileName;

    vfs_search_context_t * ctxt;

    BAIL_IF_NOT_INIT();

    if ( INVALID_CSTR( sourceDir ) || INVALID_CSTR( destDir ) )
    {
        return VFS_FALSE;
    }

    // make sure the destination directory exists
    if ( !vfs_mkdir( destDir ) )
    {
        return VFS_FALSE;
    }

    // List all the files in the directory
    ctxt = vfs_findFirst( vfs_convert_fname( sourceDir ), NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    fileName = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( fileName ) )
    {
        // Ignore files that begin with a .
        if ( '.' != fileName[0] )
        {
            snprintf( srcPath, SDL_arraysize( srcPath ), "%s/%s", sourceDir, fileName );
            snprintf( destPath, SDL_arraysize( destPath ), "%s/%s", destDir, fileName );

            if ( !vfs_copyFile( srcPath, destPath ) )
            {
                log_debug( "vfs_copyDirectory() - Failed to copy from \"%s\" to \"%s\" (%s)\n", srcPath, destPath, vfs_getError() );
            }
        }
        ctxt = vfs_findNext( &ctxt );

        fileName = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );

    return VFS_TRUE;
}

//--------------------------------------------------------------------------------------------
void vfs_empty_temp_directories()
{
    BAIL_IF_NOT_INIT();

    vfs_removeDirectoryAndContents( "import", VFS_TRUE );
    vfs_removeDirectoryAndContents( "remote", VFS_TRUE );
}

//--------------------------------------------------------------------------------------------
int vfs_rewind( vfs_FILE * pfile )
{
    BAIL_IF_NOT_INIT();

    if ( NULL == pfile ) return 0;

    return vfs_seek( pfile, 0 );
}

//--------------------------------------------------------------------------------------------
int vfs_add_mount_point( const char * root_path, const char * relative_path, const char * mount_point, int append )
{
    /// @details BB@> a wrapper for PHYSFS_mount

    int retval = -1;
    const char * loc_dirname;
    VFS_PATH     dirname;

    BAIL_IF_NOT_INIT();

    // a bare slash is taken to mean the PHYSFS root directory, not the root of the currently mounted volume
    if ( !VALID_CSTR( mount_point ) || 0 == strcmp( mount_point, C_SLASH_STR ) ) return 0;

    // make a complete version of the pathname
    if ( VALID_CSTR( root_path ) && VALID_CSTR( relative_path ) )
    {
        snprintf( dirname, SDL_arraysize( dirname ), "%s" SLASH_STR "%s", root_path, relative_path );
    }
    else if ( VALID_CSTR( root_path ) )
    {
        strncpy( dirname, root_path, SDL_arraysize( dirname ) );
    }
    else if ( VALID_CSTR( relative_path ) )
    {
        strncpy( dirname, relative_path, SDL_arraysize( dirname ) );
    }
    else
    {
        return 0;
    }

    /// @note ZF@> 2010-06-30 vfs_convert_fname_sys() broke the Linux version
    /// @note BB@> 2010-06-30 the error in vfs_convert_fname_sys() might be fixed now
    //loc_dirname = vfs_convert_fname_sys( dirname );

    if ( _vfs_mount_info_add( mount_point, root_path, relative_path ) )
    {
        retval = PHYSFS_mount( dirname, mount_point, append );
        if ( 0 == retval )
        {
            // go back and remove the mount info, since PHYSFS rejected the
            // data we gave it
            int i = _vfs_mount_info_matches( mount_point, dirname );
            _vfs_mount_info_remove( i );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int vfs_remove_mount_point( const char * mount_point )
{
    /// @details BB@> Remove every single search path related to the given mount point.

    int retval, cnt;

    BAIL_IF_NOT_INIT();

    // don't allow it to remove the default directory
    if ( !VALID_CSTR( mount_point ) ) return 0;
    if ( 0 == strcmp( mount_point, C_SLASH_STR ) ) return 0;

    // assume we are going to fail
    retval = 0;

    // see if we have the mount point
    cnt = _vfs_mount_info_matches( mount_point, NULL );

    // does it exist in the list?
    if ( cnt < 0 ) return bfalse;

    while ( cnt >= 0 )
    {
        // we have to use the path name to remove the search path, not the mount point name
        PHYSFS_removeFromSearchPath( _vfs_mount_info[cnt].full_path );

        // remove the mount info from this index
        // PF> we remove it even if PHYSFS_removeFromSearchPath() fails or else we might get an infinite loop
        _vfs_mount_info_remove( cnt );

        cnt = _vfs_mount_info_matches( mount_point, NULL );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * vfs_search_context_get_current( vfs_search_context_t * ctxt )
{
    BAIL_IF_NOT_INIT();

    if ( NULL == ctxt ) return NULL;

    return ctxt->found;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int _vfs_mount_info_search( const char * some_path )
{
    /// @details BB@> check to see if the given path is actually relative to a registered
    ///               virtual mount point

    int cnt, retval = VFS_FALSE;
    VFS_PATH temp_path;

    BAIL_IF_NOT_INIT();

    if ( !VALID_CSTR( some_path ) ) return retval;

    for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
    {
        int len;

        if ( 0 == strcmp( _vfs_mount_info[cnt].mount, some_path ) )
        {
            retval = VFS_TRUE;
            break;
        }

        snprintf( temp_path, SDL_arraysize( temp_path ), "%s" NET_SLASH_STR, _vfs_mount_info[cnt].mount );
        len = strlen( temp_path );

        if ( 0 == strncmp( temp_path, some_path, len ) )
        {
            retval = VFS_TRUE;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char * vfs_mount_info_strip_path( const char * some_path )
{
    int cnt;
    size_t offset;
    const char * ptmp, * stripped_pos;

    BAIL_IF_NOT_INIT();

    stripped_pos = some_path;

    // strip any starting slashes
    for ( ptmp = some_path; ( CSTR_END != *ptmp ) && ptmp < some_path + VFS_MAX_PATH; ptmp++ )
    {
        if ( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp )
        {
            break;
        }
    }
    some_path = ptmp;

    ptmp   = NULL;
    offset = 0;
    for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
    {
        offset = strlen( _vfs_mount_info[cnt].mount );
        if ( offset <= 0 ) continue;

        if ( 0 == strncmp( some_path, _vfs_mount_info[cnt].mount, offset ) )
        {
            stripped_pos = some_path + offset;
            break;
        }
    }

    return stripped_pos;
}

//--------------------------------------------------------------------------------------------
int _vfs_mount_info_matches( const char * mount_point, const char * local_path )
{
    int cnt, retval;
    const char * ptmp;

    BAIL_IF_NOT_INIT();

    // set to an invalid value;
    retval = -1;

    // are there any in the list?
    if ( 0 == _vfs_mount_info_count ) return retval;

    // alias the mount point
    ptmp = mount_point;

    // strip any starting slashes
    if ( VALID_CSTR( ptmp ) )
    {
        for ( /* nothing */; ptmp < mount_point + VFS_MAX_PATH; ptmp++ )
        {
            if (( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp ) || CSTR_END == *ptmp )
            {
                break;
            }
        }
    }

    if ( VALID_CSTR( ptmp ) && VALID_CSTR( local_path ) )
    {
        // find the first path info with the given mount_point and local_path
        for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
        {
            if ( 0 == strncmp( _vfs_mount_info[cnt].mount,     mount_point, VFS_MAX_PATH ) &&
                 0 == strncmp( _vfs_mount_info[cnt].full_path, local_path,  VFS_MAX_PATH ) )
            {
                retval = cnt;
                break;
            }
        }
    }
    else if ( VALID_CSTR( ptmp ) )
    {
        // find the first path info with the given mount_point
        for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
        {
            if ( 0 == strncmp( _vfs_mount_info[cnt].mount, mount_point, VFS_MAX_PATH ) )
            {
                retval = cnt;
                break;
            }
        }
    }
    else if ( VALID_CSTR( local_path ) )
    {
        // find the first path info with the given local_path
        for ( cnt = 0; cnt < _vfs_mount_info_count; cnt++ )
        {
            if ( 0 == strncmp( _vfs_mount_info[cnt].full_path, local_path, VFS_MAX_PATH ) )
            {
                retval = cnt;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t _vfs_mount_info_add( const char * mount_point, const char * root_path, const char * relative_path )
{
    const char * ptmp;

    VFS_PATH     local_path;

    BAIL_IF_NOT_INIT();

    // can we add it?
    if ( _vfs_mount_info_count >= MAX_MOUNTINFO ) return bfalse;

    // if the mount point is not a string, do nothing
    if ( !VALID_CSTR( mount_point ) ) return bfalse;

    // make a complete version of the pathname
    if ( VALID_CSTR( root_path ) && VALID_CSTR( relative_path ) )
    {
        snprintf( local_path, SDL_arraysize( local_path ), "%s" SLASH_STR "%s", root_path, relative_path );
    }
    else if ( VALID_CSTR( root_path ) )
    {
        strncpy( local_path, root_path, SDL_arraysize( local_path ) );
    }
    else if ( VALID_CSTR( relative_path ) )
    {
        strncpy( local_path, relative_path, SDL_arraysize( local_path ) );
    }
    else
    {
        return bfalse;
    }

    // do we want to add it?
    if ( !VALID_CSTR( local_path ) ) return bfalse;

    if ( _vfs_mount_info_matches( mount_point, local_path ) >= 0 ) return bfalse;

    // strip any starting slashes
    for ( ptmp = mount_point; ptmp < mount_point + VFS_MAX_PATH; ptmp++ )
    {
        if (( NET_SLASH_CHR != *ptmp && WIN32_SLASH_CHR != *ptmp ) || CSTR_END == *ptmp )
        {
            break;
        }
    }

    if ( CSTR_END == *ptmp ) return bfalse;

    // save the mount point in a list for later detection
    strncpy( _vfs_mount_info[_vfs_mount_info_count].mount,     ptmp,       VFS_MAX_PATH );
    strncpy( _vfs_mount_info[_vfs_mount_info_count].full_path, local_path, VFS_MAX_PATH );

    _vfs_mount_info[_vfs_mount_info_count].root_path[0] = CSTR_END;
    if ( VALID_CSTR( root_path ) )
    {
        strncpy( _vfs_mount_info[_vfs_mount_info_count].root_path, root_path, VFS_MAX_PATH );
    }

    _vfs_mount_info[_vfs_mount_info_count].relative_path[0] = CSTR_END;
    if ( VALID_CSTR( relative_path ) )
    {
        strncpy( _vfs_mount_info[_vfs_mount_info_count].relative_path, relative_path, VFS_MAX_PATH );
    }

    _vfs_mount_info_count++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t _vfs_mount_info_remove( int cnt )
{
    BAIL_IF_NOT_INIT();

    // does it exist in the list?
    if ( cnt < 0 || cnt > _vfs_mount_info_count ) return bfalse;

    // fill in the hole in the list
    if ( _vfs_mount_info_count > 1 )
    {
        memmove( _vfs_mount_info + cnt, _vfs_mount_info + ( _vfs_mount_info_count - 1 ), sizeof( vfs_path_data_t ) );
    }

    // shorten the list
    _vfs_mount_info_count--;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void vfs_set_base_search_paths()
{
    BAIL_IF_NOT_INIT();

    // Put write dir first in search path...
    PHYSFS_addToSearchPath( fs_getUserDirectory(), 0 );

    // Put base path on search path...
    PHYSFS_addToSearchPath( fs_getDataDirectory(), 1 );
    
    // Put config path on search path, in Win/OSX does nothing since
    // data dir == conf dir
    PHYSFS_addToSearchPath( fs_getConfigDirectory(), 1 );
}

static int ensure_write_directory( const char * filename, bool_t is_directory )
{
    /// @details BB@>
    char tmp[256];
    int i;
    
    if (!is_directory) {
        char * loc = strrchr(filename, '/');
        if (loc == NULL || loc == filename)
            strncpy(tmp, "/", 256);
        else {
            for (i = 0; filename + i != loc && i < 254; ++i)
                tmp[i] = filename[i];
            tmp[i] = '/';
            tmp[i + 1] = '\0';
        }
    } else
        strncpy(tmp, filename, 256);
    
    return vfs_mkdir(tmp);
}

static void setFlagsAsNeeded(vfs_FILE * pfile) {
    const char * errStr;
    
    if (pfile == NULL) return;
    errStr = PHYSFS_getLastError();
    if (errStr == NULL) {
        pfile->errFlag = bfalse;
        strncpy(pfile->errString, "no error", SDL_arraysize(pfile->errString));
    } else {
        pfile->errFlag = btrue;
        strncpy(pfile->errString, errStr, SDL_arraysize(pfile->errString));
    }
}

// "Lower level" functions
vfs_FILE * vfs_openRead( const char * filename ) {
    vfs_FILE * file;
    
    file = (vfs_FILE *)malloc(sizeof(vfs_FILE));
    if (file == NULL)
        return NULL;
    
    file->file = PHYSFS_openRead(filename);
    if(file->file == NULL) {
        //log_warning("Couldn't open file '%s': %s\n", filename, PHYSFS_getLastError());
        free(file);
        return NULL;
    }
    
    strncpy(file->fileName, filename, 256);
    file->errFlag = bfalse;
    PHYSFS_getLastError(); // clear out any errors? :(
    
    return file;
}

vfs_FILE * vfs_openWrite( const char * filename ) {
    vfs_FILE * file;
    
    if ( !ensure_write_directory( filename, bfalse ) ) return NULL;
    
    file = (vfs_FILE *)malloc(sizeof(vfs_FILE));
    if (file == NULL)
        return NULL;
    
    file->file = PHYSFS_openWrite(filename);
    if(file->file == NULL) {
        //log_warning("Couldn't open file '%s': %s\n", filename, PHYSFS_getLastError());
        free(file);
        return NULL;
    }
    
    strncpy(file->fileName, filename, 256);
    file->errFlag = bfalse;
    PHYSFS_getLastError(); // clear out any errors? :(
    
    return file;
}

vfs_FILE * vfs_openAppend( const char * filename ) {
    vfs_FILE * file;
    
    if ( !ensure_write_directory( filename, bfalse ) ) return NULL;
    
    file = (vfs_FILE *)malloc(sizeof(vfs_FILE));
    if (file == NULL)
        return NULL;
    
    file->file = PHYSFS_openAppend(filename);
    if(file->file == NULL) {
        //log_warning("Couldn't open file '%s': %s\n", filename, PHYSFS_getLastError());
        free(file);
        return NULL;
    }
    
    strncpy(file->fileName, filename, 256);
    file->errFlag = bfalse;
    PHYSFS_getLastError(); // clear out any errors? :(
    
    return file;
}

int vfs_close( vfs_FILE * pfile ) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_close(pfile->file);
    pfile->file = NULL;
    free(pfile);
    
    return ret;
}

int vfs_flush( vfs_FILE * pfile ) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_flush(pfile->file);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_eof( vfs_FILE * pfile ) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_eof(pfile->file);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_error( vfs_FILE * pfile ) {
    if (pfile == NULL) return 0;
    return pfile->errFlag;
}

long vfs_tell( vfs_FILE * pfile ) {
    long ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_tell(pfile->file);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_seek( vfs_FILE * pfile, long offset ) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_seek(pfile->file, offset);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

long vfs_fileLength( vfs_FILE * pfile ) {
    long ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_fileLength(pfile->file);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_mkdir( const char * dirName ) {
    int ret = PHYSFS_mkdir(dirName);
    //if (!ret)
    //log_warning("Failed creating directory '%s': %s\n", dirName, PHYSFS_getLastError());
    return ret;
}

int vfs_delete_file( const char * fileName ) {
    return PHYSFS_delete(fileName);
}

int vfs_exists( const char * fileName ) {
    return PHYSFS_exists(fileName);
}

int vfs_isDirectory( const char * dirName ) {
    return PHYSFS_isDirectory(dirName);
}

int vfs_copyFile( const char *source, const char *dest ) {
    PHYSFS_File * in, * out;
    size_t length, pos;
    char * buffer;
    
    if (!ensure_write_directory(dest, bfalse)) return bfalse;
    
    in = PHYSFS_openRead(source);
    if (in == NULL) {
        return bfalse;
    }
    length = PHYSFS_fileLength(in);
    buffer = (char *)malloc(length);
    if (buffer == NULL) {
        log_error("Failed to allocate memory!\n");
        PHYSFS_close(in);
        return bfalse;
    }
    pos = 0;
    do {
        pos += PHYSFS_read(in, buffer + pos, 1, length - pos);
    } while (pos < length);
    PHYSFS_close(in);
    out = PHYSFS_openWrite(dest);
    if (out == NULL) {
        free(buffer);
        return bfalse;
    }
    pos = 0;
    do {
        pos += PHYSFS_write(out, buffer + pos, 1, length - pos);
    } while (pos < length);
    free(buffer);
    PHYSFS_close(out);
    return btrue;
}

size_t vfs_read( void * buf, size_t size, size_t count, vfs_FILE * pfile ) {
    size_t ret;
    
    if (pfile == NULL) return -1;
    ret = PHYSFS_read(pfile->file, buf, size, count);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

size_t vfs_write( void * buf, size_t size, size_t count, vfs_FILE * pfile ) {
    size_t ret;
    
    if (pfile == NULL) return -1;
    ret = PHYSFS_write(pfile->file, buf, size, count);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_read_Sint16( vfs_FILE * pfile, Sint16 * val) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_readSLE16(pfile->file, val);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_read_Uint16( vfs_FILE * pfile, Uint16 * val) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_readULE16(pfile->file, val);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_read_Sint32( vfs_FILE * pfile, Sint32 * val) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_readSLE32(pfile->file, val);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_read_Uint32( vfs_FILE * pfile, Uint32 * val) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_readULE32(pfile->file, val);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_read_Sint64( vfs_FILE * pfile, Sint64 * val) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_readSLE64(pfile->file, val);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_read_Uint64( vfs_FILE * pfile, Uint64 * val) {
    int ret;
    
    if (pfile == NULL) return 0;
    ret = PHYSFS_readULE64(pfile->file, val);
    setFlagsAsNeeded(pfile);
    
    return ret;
}

int vfs_getc( vfs_FILE * pfile ) {
    char tmp;
    int err = vfs_read(&tmp, 1, sizeof(char), pfile);
    return err ? tmp : -1;
}

int vfs_ungetc( int c, vfs_FILE * pfile ) {
    //log_error("vfs_ungetc called (filename is %s)\n", pfile->fileName);
    // fixme: actually buffer this character
    vfs_seek( pfile, vfs_tell( pfile ) - 1 );
    return 1;
}

int vfs_putc( int c, vfs_FILE * pfile ) {
    return vfs_write(&c, 1, sizeof(char), pfile);
}

int vfs_puts( const char * str, vfs_FILE * pfile ) {
    if (pfile == NULL) return 0;
    int ret = PHYSFS_write(pfile->file, str, strlen(str), 1);
    setFlagsAsNeeded(pfile);
    return ret;
}

char * vfs_gets( char * buf, int buf_size, vfs_FILE * pfile ) {
    int i;
    if (pfile == NULL) return NULL;
    for (i = 0; i < buf_size; i++) {
        int c =  vfs_getc(pfile);
        if (c == -1) {
            if (i == 0) return NULL;
            break;
        }
        if (c == 0) break;
        buf[i] = (char) c;
        if (c == '\n') {
            if (i + 1 <= buf_size) i++;
            break;
        }
    }
    buf[i] = '\0';
    return buf;
}

int vfs_printf( vfs_FILE * pfile, const char * format, ... ) {
    if (pfile == NULL) return 0;
    PHYSFS_File * out = pfile->file;
    char buf[4097];
    va_list args;
    int written;
    va_start(args, format);
    written = vsnprintf(buf, SDL_arraysize(buf), format, args);
    if (written > 0)
        written = PHYSFS_write(out, buf, sizeof(char), written);
    va_end(args);
    setFlagsAsNeeded(pfile);
    return written;
}

// TODO: get rid of meeeeee
int vfs_scanf( vfs_FILE * pfile, const char * format, ... ) {
    log_error("vfs_scanf called (filename: %s)\n", pfile->fileName);
    return -1;
}

static int funcSeek(struct SDL_RWops* context, int offset, int whence)
{
    PHYSFS_File* file = (PHYSFS_File*) context->hidden.unknown.data1;
    int res;
    switch(whence) {
        case SEEK_SET:
            res = PHYSFS_seek(file, offset);
            break;
        case SEEK_CUR:
            res = PHYSFS_seek(file, PHYSFS_tell(file) + offset);
            break;
        case SEEK_END:
            res = PHYSFS_seek(file, PHYSFS_fileLength(file) + offset);
            break;
        default:
            res = 0;
            assert(bfalse);
            break;
    }
    if(res == 0) {
        //std::cerr << "Error seeking in file: %s" << PHYSFS_getLastError() << std::endl;
        log_warning("Error seeking in file: %s\n", PHYSFS_getLastError());
        return -1;
    }
    
    return (int) PHYSFS_tell(file);
}

static int funcRead(struct SDL_RWops* context, void* ptr, int size, int maxnum)
{
    PHYSFS_File* file = (PHYSFS_File*) context->hidden.unknown.data1;
    
    int res = PHYSFS_read(file, ptr, size, maxnum);
    return res;
}

static int funcWrite(struct SDL_RWops* context, const void* ptr, int size, int maxnum) {
    PHYSFS_File* file = (PHYSFS_File*) context->hidden.unknown.data1;
    int res = PHYSFS_write(file, ptr, size, maxnum);
    return res;
}

static int funcClose(struct SDL_RWops* context)
{
    PHYSFS_File* file = (PHYSFS_File*) context->hidden.unknown.data1;
    
    PHYSFS_close(file);
    SDL_FreeRW(context); // what
    return 0;
}

SDL_RWops* vfs_getReadRWops(const char* filename)
{
    // check this as PHYSFS seems to be buggy and still returns a
    // valid pointer in this case
    if (filename[0] == '\0') return NULL;
    
    PHYSFS_File* file = PHYSFS_openRead(filename);
    if(!file) {
        //std::cerr << "Couldn't open '" << filename << "': " << PHYSFS_getLastError();
        //log_warning("Couldn't open %s: %s\n", filename, PHYSFS_getLastError());
        return NULL;
    }
    
    SDL_RWops* ops = SDL_AllocRW();
    ops->type = 0;
    ops->hidden.unknown.data1 = file;
    ops->seek = funcSeek;
    ops->read = funcRead;
    ops->write = 0;
    ops->close = funcClose;
    //ops->size = funcSize;
    return ops;
}

SDL_RWops* vfs_getWriteRWops(const char* filename)
{
    // check this as PHYSFS seems to be buggy and still returns a
    // valid pointer in this case
    if (filename[0] == '\0') return NULL;
    
    PHYSFS_file* file = PHYSFS_openWrite(filename);
    if(!file) {
        //std::cerr << "Couldn't open '" << filename << "': " << PHYSFS_getLastError();
        //log_warning("Couldn't open %s: %s\n", filename, PHYSFS_getLastError());
        return NULL;
    }
    
    SDL_RWops* ops = SDL_AllocRW();
    ops->type = 0;
    ops->hidden.unknown.data1 = file;
    ops->seek = funcSeek;
    ops->read = 0;
    ops->write = funcWrite;
    ops->close = funcClose;
    //ops->size = funcSize;
    return ops;
}
// end SuperTux code

const char * vfs_getError() {
    static char errString[1024];
    const char * errStr = PHYSFS_getLastError();
    if (errStr == NULL) errStr = "no error";
    strncpy(errString, errStr, SDL_arraysize(errString));
    return errString;
}

const char * vfs_getFileError(vfs_FILE * pfile) {
    return pfile->errString;
}