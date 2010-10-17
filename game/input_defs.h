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

/// @file input_defs.h
/// @details Keyboard, mouse, and joystick handling code.

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXJOYSTICK          2     ///<the maximum number of supported joysticks

/// Which input control
/// @details Used by the controls[] structure and the control_is_pressed() function to query the state of various controls.
enum e_input_device
{
    INPUT_DEVICE_KEYBOARD = 0,
    INPUT_DEVICE_MOUSE,
    INPUT_DEVICE_JOY,

    // aliases
    INPUT_DEVICE_BEGIN = INPUT_DEVICE_KEYBOARD,
    INPUT_DEVICE_END   = INPUT_DEVICE_JOY,
    INPUT_DEVICE_JOY_A = INPUT_DEVICE_JOY + 0,
    INPUT_DEVICE_JOY_B = INPUT_DEVICE_JOY + 1
};
typedef enum  e_input_device INPUT_DEVICE;