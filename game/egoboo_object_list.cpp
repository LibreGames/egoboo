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

/// \file egoboo_object_list.cpp
/// \brief
/// \details

#include "egoboo_object_list.inl"

//--------------------------------------------------------------------------------------------
// a some code to test the instantiation of the structs in egoboo-object_list.h
//--------------------------------------------------------------------------------------------

#undef TEST_COMPILE_EGOBOO_OBJECT_LIST

#if defined(TEST_COMPILE_EGOBOO_OBJECT_LIST)

struct blah;

typedef t_ego_obj_container< blah, 100 > blah_cont;

struct blah : public ego_obj
{
    typedef int data_type;

    int val;

    int & get_data() { return val; }
    int * get_data_ptr() { return &val; }

    blah( const blah_cont *q );

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

typedef t_ego_obj_container< blah, 100 > blah_cont;

t_obj_lst_deque<blah, 100> test;

void flah()
{
    t_obj_lst_deque<blah, 100>::iterator_type it;
    t_obj_lst_deque<blah, 100>::lst_reference ref( 5 );

    test.init();
    test.deinit();

    test.update_used();

    test.free_one( ref );
    test.free_all();

    test.allocate( ref );
    test.cleanup();

    test.add_activation( ref );
    test.add_termination( ref );

    test.get_allocated_ptr( ref );

    test.get_data_ref( ref );
    test.get_data_ptr( ref );
    test.get_allocated_data_ptr( ref );

    for ( it = test.iterator_begin_allocated(); !test.iterator_finished( it ); test.iterator_increment_allocated( it ) );
}
#endif

//--------------------------------------------------------------------------------------------
// ego_obj_lst_state_data -
//--------------------------------------------------------------------------------------------
ego_obj_lst_state_data * ego_obj_lst_state_data::clear( ego_obj_lst_state_data * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr->in_used_list = bfalse;
    ptr->update_guid  = ego_uint( ~0L );

    return ptr;
}

////--------------------------------------------------------------------------------------------
//ego_obj_lst_state_data * ego_obj_lst_state_data::set_allocated( ego_obj_lst_state_data * ptr, bool_t val )
//{
//    if ( NULL == ptr ) return ptr;
//
//    ptr->allocated = val;
//
//    return ptr;
//}

//--------------------------------------------------------------------------------------------
ego_obj_lst_state_data * ego_obj_lst_state_data::set_used( ego_obj_lst_state_data * ptr, const bool_t val )
{
    if ( NULL == ptr ) return ptr;

    ptr->in_used_list = val;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_lst_state_data * ego_obj_lst_state_data::set_list_id( ego_obj_lst_state_data * ptr, const ego_uint val )
{
    if ( NULL == ptr ) return ptr;

    ptr->update_guid = val;

    return ptr;
}

//--------------------------------------------------------------------------------------------
// ego_obj_lst_state -
//--------------------------------------------------------------------------------------------
ego_obj_lst_state * ego_obj_lst_state::ctor_this( ego_obj_lst_state * ptr )
{
    ego_obj_lst_state_data::clear( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_obj_lst_state * ego_obj_lst_state::dtor_this( ego_obj_lst_state * ptr )
{
    ego_obj_lst_state_data::clear( ptr );

    return ptr;
}
