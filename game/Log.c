//********************************************************************************************
//* Egoboo - Log.c
//*
//* Basic logging functionality.
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

#include "Log.h"
#include "game.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define MAX_LOG_MESSAGE 1024

static FILE * _logFile = NULL;
static char   _logBuffer[MAX_LOG_MESSAGE];
static int    _logLevel = 1;
static FILE * _log_getFile();

static FILE *_debugFile = NULL;
static FILE * _log_getDebugFile();


FILE * _log_getFile()
{
  if ( NULL == _logFile  )
  {
    _logFile = fopen( "log.txt", "wt" );
  }

  return _logFile;
}

FILE * _log_getDebugFile()
{
  if ( NULL == _debugFile  )
  {
    _debugFile = fopen( "debug.txt", "wt" );
  }

  return _debugFile;
}

void log_init()
{
  _log_getFile();
  _log_getDebugFile();
}

void log_shutdown()
{
  if ( NULL != _logFile  )
  {
    fclose( _logFile );
    _logFile = NULL;
  }

  if ( NULL != _debugFile  )
  {
    fclose( _debugFile );
    _debugFile = NULL;
  }
}


static void writeLogMessage( const char *prefix, const char *format, va_list args )
{
  FILE * lfile = _log_getFile();

  if ( NULL != lfile  )
  {
    fputs( prefix, lfile );
    vsnprintf( _logBuffer, MAX_LOG_MESSAGE - 1, format, args );
    fputs( _logBuffer, lfile );
    fflush( lfile );
  }

#ifdef CONSOLE_MODE
  fprintf( stdout, prefix );
  vfprintf( stdout, format, args );
#endif
}

static void writeDebugMessage( const char *prefix, const char *format, va_list args )
{
  FILE * dfile = _log_getDebugFile();

  if ( NULL != dfile  )
  {
    vsnprintf( _logBuffer, MAX_LOG_MESSAGE - 1, format, args );
    fputs( prefix, dfile );
    fputs( _logBuffer, dfile );
    fflush( dfile );
  }

#ifdef CONSOLE_MODE
  fprintf( stdout, prefix );
  vfprintf( stdout, format, args );
#endif
}


void log_setLoggingLevel( int level )
{
  if ( level > 0 )
  {
    _logLevel = level;
  }
}

void log_message( const char *format, ... )
{
  va_list args;

  va_start( args, format );
  writeLogMessage( "", format, args );
  va_end( args );
}

void log_info( const char *format, ... )
{
  va_list args;

  if ( _logLevel >= 2 )
  {
    va_start( args, format );
    writeLogMessage( "INFO: ", format, args );
    va_end( args );
  }
}

void log_warning( const char *format, ... )
{
  va_list args;

  if ( _logLevel >= 1 )
  {
    va_start( args, format );
    writeLogMessage( "WARN: ", format, args );
    va_end( args );
  }
}

void log_error( const char *format, ... )
{
  // This function writes an error message into the logging document (log.txt)
  //Never test for an error condition you don't know how to handle.
  va_list args;

  va_start( args, format );
  writeLogMessage( "FATAL ERROR: ", format, args );
  va_end( args );

  //Close down various stuff and release memory
  fflush( _logFile );
  exit( -1 );
}


void log_debug( const char *format, ... )
{
  va_list args;

  if ( CData.DevMode && _logLevel >= 1 )
  {
    va_start( args, format );
    writeDebugMessage( "DEBUG: ", format, args );
    va_end( args );
  }


}
