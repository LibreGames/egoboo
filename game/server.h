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

/// \file server.h
/// \details Basic skeleton for the server portion of a client-server architecture,
/// this is totally not in use yet.

/// A mockup of the struct that could be used to store the server state
struct ego_ServerState
{
    int dummy;
// GameState gameState;
};

/// Globally accessible server state
extern ego_ServerState ServerState;

int  sv_init();
void sv_shutDown();
void sv_frameStep();

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)

#define _server_h
