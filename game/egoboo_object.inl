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

/// \file egoboo_typedef_cpp.inl
/// \details cpp-only inline and template implementations

#if !defined(__cplusplus)
#    error egoboo_typedef_cpp.inl should only be included if you are compling as c++
#endif

#include "egoboo_object.h"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

//---- object flags

/// Has the object been allocated?
#define OBJ_VALID_RAW(pobj)           ( pobj->require_bits( ego_obj::valid_bit ) )

/// Has the object been allocated and constructed?
#define OBJ_DEFINED_RAW(pobj)         ( pobj->require_bits( ego_obj::valid_bit ) && pobj->reject_bits( ego_obj::killed_bit ) )

/// Has the object been marked as terminated?
#define OBJ_TERMINATED_RAW(pobj)      ( pobj->require_bits( ego_obj::valid_bit ) && pobj->require_bits( ego_obj::killed_bit ) )

//---- object states

/// Has the object been created yet?
#define OBJ_CONSTRUCTING_RAW(pobj)    ( pobj->require_bits( ego_obj::valid_bit        ) && pobj->reject_bits( ego_obj::killed_bit ) && pobj->require_action( ego_obj_constructing   ) )

/// Is the object being initialized right now?
#define OBJ_INITIALIZING_RAW(pobj)    ( pobj->require_bits( ego_obj::full_constructed ) && pobj->reject_bits( ego_obj::killed_bit ) && pobj->require_action( ego_obj_initializing   ) )

/// Is the object actually doing something right now?
#define OBJ_PROCESSING_RAW(pobj)      ( pobj->require_bits( ego_obj::full_on          ) && pobj->reject_bits( ego_obj::killed_bit ) && pobj->require_action( ego_obj_processing     ) )

/// Is the object being deinitialized right now?
#define OBJ_DEINITIALIZING_RAW(pobj)  ( pobj->require_bits( ego_obj::full_constructed ) && pobj->reject_bits( ego_obj::killed_bit ) && pobj->require_action( ego_obj_deinitializing ) )

/// Is the object being deinitialized right now?
#define OBJ_DESTRUCTING_RAW(pobj)     ( pobj->require_bits( ego_obj::valid_bit        ) && pobj->reject_bits( ego_obj::killed_bit ) && pobj->require_action( ego_obj_destructing    ) )

/// Is the object "waiting to die" state?
#define OBJ_WAITING_RAW(pobj)         ( pobj->require_bits( ego_obj::valid_bit        ) && pobj->reject_bits( ego_obj::killed_bit ) && pobj->require_action( ego_obj_waiting        ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
template<typename _ty>
_ty * ego_object_engine::run_construct( _ty * pdata, const int max_iterations )
{
    int iterations;

    if ( !ego_obj::get_valid( pdata ) ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( ego_obj::get_action( pdata ) > ( const int )( ego_obj_constructing + 1 ) )
    {
        _ty * tmp_chr = ego_object_engine::run_deconstruct( pdata, max_iterations );
        if ( tmp_chr == pdata ) return NULL;
    }

    iterations = 0;
    while ( NULL != pdata && ego_obj::get_action( pdata ) <= ego_obj_constructing && iterations < max_iterations )
    {
        ego_obj * ptmp = ego_object_engine::run( pdata );
        if ( ptmp != pdata ) return NULL;

        iterations++;
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
_ty * ego_object_engine::run_initialize( _ty * pdata, const int max_iterations )
{
    int iterations;

    if ( !ego_obj::get_valid( pdata ) ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( ego_obj::get_action( * pdata ) > ( const int )( ego_obj_initializing + 1 ) )
    {
        _ty * tmp_chr = ego_object_engine::run_deconstruct( pdata, max_iterations );
        if ( tmp_chr == pdata ) return NULL;
    }

    iterations = 0;
    while ( NULL != pdata && ego_obj::get_action( pdata ) <= ego_obj_initializing && iterations < max_iterations )
    {
        ego_obj * ptmp = ego_object_engine::run( pdata );
        if ( ptmp != pdata ) return NULL;
        iterations++;
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
_ty * ego_object_engine::run_activate( _ty * pdata, const int max_iterations )
{
    int iterations;

    if ( !ego_obj::get_valid( pdata ) ) return NULL;

    // if the character is already beyond this stage, deconstruct it and start over
    if ( ego_obj::get_action( pdata ) > ( const int )( ego_obj_processing + 1 ) )
    {
        _ty * tmp_chr = ego_object_engine::run_deconstruct( pdata, max_iterations );
        if ( tmp_chr == pdata ) return NULL;
    }

    iterations = 0;
    while ( NULL != pdata && ego_obj::get_action( pdata ) < ego_obj_processing && iterations < max_iterations )
    {
        ego_obj * ptmp = ego_object_engine::run( pdata );
        if ( ptmp != pdata ) return NULL;
        iterations++;
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
_ty * ego_object_engine::run_deinitialize( _ty * pdata, const int max_iterations )
{
    int iterations;

    if ( !ego_obj::get_valid( pdata ) ) return NULL;

    // if the character is already beyond this stage, deinitialize it
    if ( ego_obj::get_action( pdata ) > ( const int )( ego_obj_deinitializing + 1 ) )
    {
        return pdata;
    }
    else if ( ego_obj::get_action( pdata ) < ego_obj_deinitializing )
    {
        pdata->end_processing();
    }

    iterations = 0;
    while ( NULL != pdata && ego_obj::get_action( pdata ) <= ego_obj_deinitializing && iterations < max_iterations )
    {
        ego_obj * ptmp = ego_object_engine::run( pdata );
        if ( ptmp != pdata ) return NULL;
        iterations++;
    }

    return pdata;
}

//--------------------------------------------------------------------------------------------
template<typename _ty>
_ty * ego_object_engine::run_deconstruct( _ty * pdata, const int max_iterations )
{
    int iterations;

    if ( !ego_obj::get_valid( pdata ) ) return NULL;

    // if the character is already beyond this stage, do nothing
    if ( ego_obj::get_action( pdata ) > ( const int )( ego_obj_destructing + 1 ) )
    {
        return pdata;
    }
    else if ( ego_obj::get_action( pdata ) < ego_obj_deinitializing )
    {
        // make sure that you deinitialize before destructing
        pdata->end_processing();
    }

    iterations = 0;
    while ( NULL != pdata && ego_obj::get_action( pdata ) <= ego_obj_destructing && iterations < max_iterations )
    {
        ego_obj * ptmp = ego_object_engine::run( pdata );
        if ( ptmp != pdata ) return NULL;
        iterations++;
    }

    return pdata;
}

