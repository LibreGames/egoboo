//********************************************************************************************
//*
//*    This file is part of Cartman.
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
/// @brief Cartman mouse interface.

int             msx, msy, msxold, msyold, mous.latch.b, mcx, mcy;
int    mstlx, mstly, msbrx, msbry;
int    mousespeed = 2;

//------------------------------------------------------------------------------
void moson(void)
{
  install_mouse();
  show_mouse(NULL);
  set_mouse_speed(mousespeed, mousespeed);


  return;
}

//------------------------------------------------------------------------------
void mosdo(void)
{
  msxold = msx;
  msyold = msy;
  get_mouse_mickeys(&mcx, &mcy);
  msx+=mcx;
  msy+=mcy;
  mous.latch.b = mouse_b;
//  limit(0, &msx, OUTX-1);
//  limit(0, &msy, OUTY-1);
  limit(mstlx, &msx, msbrx);
  limit(mstly, &msy, msbry);

  return;
}

//------------------------------------------------------------------------------
