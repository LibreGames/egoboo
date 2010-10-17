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

/// @file egoboo_rpc.h
/// @brief Definitions for the Remote Procedure Call module
///
/// @details This module allows inter-thread and network control of certain game functions.
/// The main use of this at the moment (since we have no server), is to make it possible
/// to do things like loading modules in a worker thread. All graphics functions must be
/// called from the main thread...

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a generic "remote procedure call" structure for handling "inter-thread" communication
struct ego_rpc
{
    int    index;      ///< the index of this request
    int    guid;       ///< the request id number
    bool_t allocated;  ///< is this request being used?
    bool_t finished;   ///< has the request been fully processed?
    bool_t abort;      ///< has the calling function requested an abort?

    int    data_type;  ///< a the type of the "inherited" data
    void * data;       ///< a pointer to the "inherited" data

    static bool_t get_valid( ego_rpc * prpc )              { return ( NULL != prpc ) && prpc->allocated; }
    static bool_t get_matches( ego_rpc * prpc, int guid )  { return ( NULL != prpc ) && prpc->allocated && ( guid == prpc->guid ); }
    static bool_t get_finished( ego_rpc * prpc, int guid ) { return !ego_rpc::get_matches( prpc, guid ) || ( prpc->finished ); }
    static bool_t set_abort( ego_rpc * prpc, int guid )    { if ( !ego_rpc::get_matches( prpc, guid ) ) return bfalse; prpc->abort = btrue; return btrue; }

    static ego_rpc * ctor( ego_rpc * prpc, int data_type, void * data ) ;
    static ego_rpc * dtor( ego_rpc * prpc );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// a "remote procedure call" structure for handling calls to TxTexture_load_one_vfs()
/// and TxTitleImage_load_one_vfs()

struct ego_tx_request : public ego_rpc
{
     // the function call parameters
    STRING filename;
    TX_REF itex_src;
    Uint32 key;

    // the function call return value(s)
    TX_REF ret_index;    /// the return value of the function

    static ego_tx_request * ctor( ego_tx_request * preq, int type );
    static ego_tx_request * dtor( ego_tx_request * preq );

    static ego_tx_request * load_TxTexture( const char *filename, int itex_src, Uint32 key );
    static ego_tx_request * load_TxTitleImage( const char *filename );

    static ego_rpc * get_rpc( ego_tx_request * ptr ) { return static_cast<ego_rpc *>(ptr); }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t rpc_system_begin();
void   rpc_system_end();
bool_t rpc_system_timestep();
