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

/// @file egoboo_object_list.cpp
/// @brief
/// @details

#include "egoboo_object_list.inl"

#undef TEST_COMPILE_EGOBOO_OBJECT_LIST

#if defined(TEST_COMPILE_EGOBOO_OBJECT_LIST)

struct blah : public ego_obj
{
    typedef int data_type;

    int val;

    int & get_data() { return val; }
    int * get_pdata() { return &val; }

    /// External handle for iterating the "egoboo object process" state machine
    static blah * run( blah * pchr );
    /// External handle for getting an "egoboo object process" into the constructed state
    static blah * run_construct( blah * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the initialized state
    static blah * run_initialize( blah * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the active state
    static blah * run_activate( blah * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the deinitialized state
    static blah * run_deinitialize( blah * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the deconstructed state
    static blah * run_deconstruct( blah * pprt, int max_iterations );
};

t_ego_obj_lst<blah, 100> test;

void flah()
{
    t_ego_obj_lst<blah, 100>::iterator it;
    t_reference<blah> ref( 5 );

    test.init();
    test.deinit();

    test.update_used();

    test.free_one( ref );
    test.free_all();

    test.allocate( ref );
    test.cleanup();

    test.add_activation( ref );
    test.add_termination( ref );

    test.get_valid_ptr( ref );

    test.get_data( ref );
    test.get_pdata( ref );
    test.get_valid_pdata( ref );

    for ( it = test.used_begin(); !test.used_end( it ); test.used_increment( it ) );
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "char.h"
template <>
t_ego_obj_lst<ego_obj_chr, MAX_CHR>::iterator   t_ego_obj_lst<ego_obj_chr, MAX_CHR>::used_begin()                    { return used_map.iterator_begin(); }
template <>
bool_t     t_ego_obj_lst<ego_obj_chr, MAX_CHR>::used_end( t_ego_obj_lst<ego_obj_chr, MAX_CHR>::iterator & it )       { return used_map.iterator_end( it ); }
template <>
t_ego_obj_lst<ego_obj_chr, MAX_CHR>::iterator & t_ego_obj_lst<ego_obj_chr, MAX_CHR>::used_increment( t_ego_obj_lst<ego_obj_chr, MAX_CHR>::iterator & it ) { return used_map.iterator_increment( it ); }

#include "enchant.h"
template <>
t_ego_obj_lst<ego_obj_enc, MAX_ENC>::iterator   t_ego_obj_lst<ego_obj_enc, MAX_ENC>::used_begin()                    { return used_map.iterator_begin(); }
template <>
bool_t     t_ego_obj_lst<ego_obj_enc, MAX_ENC>::used_end( t_ego_obj_lst<ego_obj_enc, MAX_ENC>::iterator & it )       { return used_map.iterator_end( it ); }
template <>
t_ego_obj_lst<ego_obj_enc, MAX_ENC>::iterator & t_ego_obj_lst<ego_obj_enc, MAX_ENC>::used_increment( t_ego_obj_lst<ego_obj_enc, MAX_ENC>::iterator & it ) { return used_map.iterator_increment( it ); }

#include "particle.h"
template <>
t_ego_obj_lst<ego_obj_prt, MAX_PRT>::iterator   t_ego_obj_lst<ego_obj_prt, MAX_PRT>::used_begin()                    { return used_map.iterator_begin(); }
template <>
bool_t     t_ego_obj_lst<ego_obj_prt, MAX_PRT>::used_end( t_ego_obj_lst<ego_obj_prt, MAX_PRT>::iterator & it )       { return  used_map.iterator_end( it ); }
template <>
t_ego_obj_lst<ego_obj_prt, MAX_PRT>::iterator & t_ego_obj_lst<ego_obj_prt, MAX_PRT>::used_increment( t_ego_obj_lst<ego_obj_prt, MAX_PRT>::iterator & it ) { return used_map.iterator_increment( it ); }
