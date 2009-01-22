//---------------------------------------------------------------------
// ModBaker - a module editor for Egoboo
//---------------------------------------------------------------------
// Copyright (C) 2009 Tobias Gall
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------
#include "utility.h"

//---------------------------------------------------------------------
//-   Read an integer value
//---------------------------------------------------------------------
int fget_int(ifstream &file)
{
	int itmp = 0;

	if (file.eof())
		return itmp;

	file >> itmp;

	return itmp;
}


//---------------------------------------------------------------------
//-   Read the integer after the next colon
//---------------------------------------------------------------------
int fget_next_int(ifstream &file)
{
	int itmp = 0;

	if (fgoto_colon_yesno(file))
	{
		itmp = fget_int(file);
	};

	return itmp;
}


//---------------------------------------------------------------------
//-   Read a float value
//---------------------------------------------------------------------
float fget_float(ifstream &file)
{
	float ftmp = 0;

	if (file.eof())
		return ftmp;

	file >> ftmp;

	return ftmp;
}


//---------------------------------------------------------------------
//-   Read the float after the next colon
//---------------------------------------------------------------------
float fget_next_float(ifstream &file)
{
	float ftmp = 0;

	if (fgoto_colon_yesno(file))
	{
		ftmp = fget_float(file);
	};

	return ftmp;
}


//---------------------------------------------------------------------
//-   Go to the next colon (without buffer)
//---------------------------------------------------------------------
void fgoto_colon(ifstream &file)
{
	fgoto_colon_buffer(file, "", 0);
}


//---------------------------------------------------------------------
//-   Call fgoto_colon_yesno_buffer() without a buffer
//---------------------------------------------------------------------
bool fgoto_colon_yesno(ifstream &file)
{
	return fgoto_colon_yesno_buffer(file, "", 0);
}


//---------------------------------------------------------------------
//-   Go to the next colon. If none found, quit the program
//---------------------------------------------------------------------
void fgoto_colon_buffer(ifstream &file, string buffer, size_t buffer_len)
{
	if (!fgoto_colon_yesno_buffer(file, buffer, buffer_len))
	{
		cout << "There are not enough colons in file! " << endl;
		Quit();
	};
}


//---------------------------------------------------------------------
//-   Gets the next colon. If none found, return false
//---------------------------------------------------------------------
bool fgoto_colon_yesno_buffer(ifstream &file, string buffer, size_t buffer_len)
{
	char ctmp;
	string pchar, pchar_end;
	bool bfound = false;

	if (buffer != "")
	{
		pchar = pchar_end = buffer;
		pchar_end += buffer_len;
	}

	while (!file.eof())
	{
		file >> ctmp;

		switch (ctmp)
		{
			case ':':
				return true;

			case '\0':
				// not enough colons in file!
				return false;

			case '\n':
				pchar = buffer;

				if (pchar < pchar_end)
				{
					pchar = '\0';
				}
				break;

			default:
				if (pchar < pchar_end)
				{
					pchar += ctmp;
				}
				break;
		}
	}

	return bfound;
}
