// mac-file.m

// Egoboo, Copyright (C) 2000 Aaron Bishop

#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSError.h>
#include <string.h>
#include <SDL.h>
#include <SDL_endian.h>

// Proto.h is not included in this file because Egoboo's BOOL conflicts
// with Objective-C's built-in BOOL

NSDirectoryEnumerator* fs_dirEnum = nil;
NSString *fs_dirEnumExtension = nil;
NSString *fs_dirEnumPath = nil;

#define MAX_PATH 260
char fs_configPath[MAX_PATH];
char fs_userPath[MAX_PATH];
char fs_dataPath[MAX_PATH];
char fs_binaryPath[MAX_PATH];
NSString *fs_workingDir = nil;

//---------------------------------------------------------------------------------------------
//File Routines-------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void sys_fs_init()
{
	// JF> This function determines the temporary, import,
	// game data and save paths
	NSString *homeDir, *gameDir, *workingDir;
	const char *str;

	homeDir = NSHomeDirectory();
	gameDir = [[NSBundle mainBundle] bundlePath];
	workingDir = [[NSFileManager defaultManager] currentDirectoryPath];
	fs_workingDir = workingDir;

	NSLog(@"fs_init: Home directory is %@", homeDir);
	NSLog(@"fs_init: Game directory is %@", gameDir);

	str = [homeDir UTF8String];
	strncpy(fs_userPath, str, MAX_PATH);
	strncat(fs_userPath, "/Documents/Egoboo", MAX_PATH);

	str = [gameDir UTF8String];
	strncpy(fs_binaryPath, str, MAX_PATH);
    strncpy(fs_dataPath, fs_binaryPath, MAX_PATH);
    strncat(fs_dataPath, "/Contents/Resources", MAX_PATH);
    strncpy(fs_configPath, fs_dataPath, MAX_PATH);

	[homeDir release];
	[gameDir release];
    printf( "Game directories are:\n\tBinaries: %s\n\tData: %s\n\tUser Data: %s\n\tConfig Files: %s\n",
           fs_binaryPath, fs_dataPath, fs_userPath, fs_configPath );
	// Don't release the working directory; it's kept as a global.
}

const char *fs_getDataDirectory()
{
	return fs_dataPath;
}

const char *fs_getConfigDirectory()
{
	return fs_configPath;
}

const char *fs_getUserDirectory()
{
	return fs_userPath;
}

const char *fs_getBinaryDirectory()
{
	return fs_binaryPath;
}

int fs_createDirectory(const char *dirName)
{
	BOOL ok;

	NSString *path = [[NSString alloc] initWithUTF8String: dirName];
    NSError *error;
	ok = [[NSFileManager defaultManager] createDirectoryAtPath:path withIntermediateDirectories:YES attributes:nil error:&error];
    if (ok == NO) NSLog(@"fs_createDirectory error: %@", error);
	[path release];

	if (ok == YES) return 1;
	return 0;
}

#if 0
int fs_removeDirectory(const char *dirName)
{
	BOOL ok;
	NSString *path = [[NSString alloc] initWithUTF8String:dirName];
	ok = [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
	[path release];

	if (ok == YES) return 1;
	return 0;
}

void fs_deleteFile(const char *fileName)
{
	NSString *path = [[NSString alloc] initWithUTF8String:fileName];
	[[NSFileManager defaultManager] removeItemAtPath:path error:nil];
	[path release];
}

void fs_copyFile(const char *source, const char *dest)
{
	NSString *srcPath, *destPath;

	srcPath = [[NSString alloc] initWithUTF8String:source];
	destPath = [[NSString alloc] initWithUTF8String:dest];

	[[NSFileManager defaultManager] copyItemAtPath:srcPath toPath:destPath error:nil];

	[srcPath release];
	[destPath release];
}
#endif

int fs_fileIsDirectory(const char *filename)
{
	// Returns 1 if this file is a directory
	BOOL isDir = NO;
	NSString *path;
	NSFileManager *manager;

	path = [[NSString alloc] initWithUTF8String: filename];
	manager = [NSFileManager defaultManager];

	if ([manager fileExistsAtPath: path isDirectory: &isDir] && isDir)
	{
		return 1;
	}

	return 0;

}

#if 0
//---------------------------------------------------------------------------------------------
//Directory Functions--------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

// Return the next file in the directory that matches the criteria specified in fs_findFirstFile
const char *fs_findNextFile()
{
    static char tmp[MAX_PATH];
	NSString *fileName;
	NSString *pathName;

	if(fs_dirEnum == nil)
	{
		return NULL;
	}

	while (( fileName = [fs_dirEnum nextObject] ))
	{
		// Also, don't go down directories recursively.
		pathName = [[NSString stringWithFormat: @"%@/%@", fs_dirEnumPath, fileName] retain];
		if(fs_fileIsDirectory([pathName UTF8String]))
		{
			[fs_dirEnum skipDescendents];
		}
		[pathName release];

		if(fs_dirEnumExtension != nil)
		{
			if ([[fileName pathExtension] isEqualToString: fs_dirEnumExtension])
			{
                const char * fn = [fileName UTF8String];
                strncpy(tmp, fn, MAX_PATH);
				return tmp;
			}
		} else
		{
            const char * fn = [fileName UTF8String];
            strncpy(tmp, fn, MAX_PATH);
            return tmp;
		}
	}

	return NULL;
}

// Stop the current find operation
void fs_findClose()
{
	if (fs_dirEnum != nil)
	{
		[fs_dirEnum release];
		fs_dirEnum = nil;
	}

	if (fs_dirEnumPath != nil)
	{
		[fs_dirEnumPath release];
		fs_dirEnumPath = nil;
	}

	if (fs_dirEnumExtension != nil)
	{
		[fs_dirEnumExtension release];
		fs_dirEnumExtension = nil;
	}
}

// Begin enumerating files in a directory.  The enumeration is not recursive; subdirectories
// won't be searched.  If 'extension' is not NULL, only files with the given extension will
// be returned.
const char *fs_findFirstFile(const char *path, const char *extension)
{
	NSString *searchPath;

	if(fs_dirEnum != nil)
	{
		fs_findClose();
	}

	// If the path given is a relative one, we need to derive the full path
	// for it by appending the current working directory
	if(path[0] != '/')
	{
		searchPath = [NSString stringWithFormat: @"%@/%s", fs_workingDir, path];
	} else
	{
		searchPath = [NSString stringWithUTF8String: path];
	}

	fs_dirEnum = [[NSFileManager defaultManager] enumeratorAtPath: searchPath];


	if(extension != NULL)
	{
		fs_dirEnumExtension = [NSString stringWithUTF8String: extension];
	}

	fs_dirEnumPath = searchPath;

	if(fs_dirEnum == nil)
	{
		return NULL;
	}

	return fs_findNextFile();
}

void empty_import_directory()
{
	fs_removeDirectory("./import");
	fs_createDirectory("./import");
}
#endif
