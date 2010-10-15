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

/// @file egoboo_state_machine.h

//--------------------------------------------------------------------------------------------

/// The various states that an egoboo state machine can occupy
enum e_ego_actions
{
    ego_action_invalid = 0,  ///< Nothing to do.
    ego_action_beginning,    ///< The creation of the machine. Should be run once.
    ego_action_entering,     ///< The initialization of the machine. An entry point for re-initializing an already created machine. Run as many times as needed.
    ego_action_running,      ///< The normal actions of a running machine. Run as many times as desired.
    ego_action_leaving,      ///< The deinitialization of the machine. Run as many times as needed.
    ego_action_finishing     ///< The final destruction of the machine. Should be run once.
};
typedef enum e_ego_actions ego_actions_t;
