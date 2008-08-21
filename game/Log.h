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
/// @file
/// @brief Message Logging
/// @details Basic logging functionality.

void log_init( void );
void log_shutdown( void );

void log_setLoggingLevel( int level );

// these send messages to "log.txt"
void log_message( const char *format, ... );
void log_info( const char *format, ... );
void log_warning( const char *format, ... );
void log_error( const char *format, ... );

// these send messages to "debug.txt"
void log_debug( const char *format, ... );
