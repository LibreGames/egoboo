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

/// @file
/// @brief Misc. helper functions
/// @details Implementation of string modification and format conversion functions


//---------------------------------------------------------------------
///   Reads an integer value from a file
///   \param file the file to read
///   \return integer value
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
///   Read the integer after the next colon
///   \param file the file to read
///   \return integer value
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
///   Read a float value
///   \param file the file to read
///   \return float value
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
///   Read the float after the next colon
///   \param file the file to read
///   \return float value
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
///   Go to the next colon (without buffer)
///   \param file the file to read
//---------------------------------------------------------------------
void fgoto_colon(ifstream &file)
{
	fgoto_colon_buffer(file, "", 0);
}


//---------------------------------------------------------------------
///   Call fgoto_colon_yesno_buffer() without a buffer
///   \param file the file to read
///   \return true if a colon got found
//---------------------------------------------------------------------
bool fgoto_colon_yesno(ifstream &file)
{
	return fgoto_colon_yesno_buffer(file, "", 0);
}


//---------------------------------------------------------------------
///   Go to the next colon. If none found, quit the program
///   \param file the file to read
///   \param buffer string in front of the colon
///   \param buffer_len the length of the buffer
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
///   Gets the next colon. If none found, return false
///   \param file the file to read
///   \param buffer string in front of the colon
///   \param buffer_len length of the buffer
///   \return false if no colon was found
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


//---------------------------------------------------------------------
///   Reads the file until the first non-comment line was found
///   \param file the file to read
///   \return string buffer with the line
//---------------------------------------------------------------------
string fread_skip_comments(ifstream &file)
{
	string buffer;

	if (!file)
	{
		cout << "WARNING: fread_skip_comments: File is unknown" << endl;
		return "";
	}

	while(!file.eof())
	{
		getline(file, buffer);

		// No comment found: Return the value
		if (buffer.find("//") == string::npos)
		{
			return buffer;
		}
	}

	// No comments found
	return "";
}


//---------------------------------------------------------------------
///   Get all files from a directory
///   \param p_dirname directory to read
///   \param needle a file extension to search for
///   \param p_files the file vector for all found files
///   \return false if the directory was not found
//---------------------------------------------------------------------
bool read_files(string p_dirname, string needle, vector<string>& p_files)
{
	DIR *dir_handler;
	string filename;
	dirent *entry;

	if (!(dir_handler=opendir(p_dirname.c_str()))) {
		cout << "Cannot read directory: " << p_dirname << endl;
		return false;
	}

	while ((entry = readdir(dir_handler)) != NULL) {
		filename = entry->d_name;

		if (filename.length() >= (needle.length()+1) && filename.substr((filename.length()-needle.length()), needle.length()) == needle)
		{
			p_files.push_back(filename);
		}
	}

	closedir(dir_handler);
	return true;
}


//---------------------------------------------------------------------
///   Translate an Egoboo boolean into a real boolean value
///   \param p_string input string
///   \return true or false
//---------------------------------------------------------------------
bool translate_into_bool(string p_string)
{
	if (p_string == "FALSE")
		return false;

	if (p_string == "TRUE")
		return true;

	cout << "WARNING: Unknown BOOLEAN" << p_string << endl;
	cout << "  Returning FALSE" << endl;
	return false;
}


//---------------------------------------------------------------------
///   Translate a boolean into an Egoboo boolean
///   \param p_bool the boolean value
///   \return the egobool value ("TRUE" or "FALSE")
//---------------------------------------------------------------------
string translate_into_egobool(bool p_bool)
{
	if (p_bool)
		return "TRUE";

	return "FALSE";
}


//---------------------------------------------------------------------
///   Translate an unsigned int into a string
///   \param p_number the unsigned int to convert
///   \return the value as a string
//---------------------------------------------------------------------
string translate_into_string(unsigned int p_number)
{
	stringstream sstr;
	sstr << (int)p_number;
	return sstr.str();
}


//---------------------------------------------------------------------
///   Translate a string into an unsigned int
///   \param p_string the string to convert
///   \return the value as an unsigned int
//---------------------------------------------------------------------
unsigned int translate_into_uint(string p_string)
{
	int iTmp;
	stringstream sstr(p_string);
	sstr >> iTmp;
	return (unsigned int)iTmp;
}


//---------------------------------------------------------------------
///   Find and replace within a string
///   \param p_string the input string
///   \param p_needle the string to search for
///   \param p_replace the string to replace
///   \return the replaced string
//---------------------------------------------------------------------
string find_and_replace(string p_string, string p_needle, string p_replace)
{
	string tmp_str;
	tmp_str = p_string;

	size_t pos = tmp_str.find(p_needle);

	while (pos != string::npos)
	{
		tmp_str.replace(pos, p_needle.length(), p_replace);
		pos = tmp_str.find(p_needle);
	}

	return tmp_str;
}


//---------------------------------------------------------------------
///   Replace blanks with underscores
///   \param p_string input string
///   \return string with replaced values
//---------------------------------------------------------------------
string add_underscores(string p_string)
{
	return find_and_replace(p_string, " ", "_");
}


//---------------------------------------------------------------------
///   Replace underscores with blanks
///   \param p_string input string
///   \return string with replaced values
//---------------------------------------------------------------------
string remove_underscores(string p_string)
{
	return find_and_replace(p_string, "_", " ");
}


//---------------------------------------------------------------------
///   Tokenize a string based on a semicolon
///   \param str input string
///   \param tokens the output tokens
///   \return number of tokens, else false
//---------------------------------------------------------------------
int tokenize_semicolon(const string str, vector<string>& tokens)
{
	string token;
	istringstream iss(str);

	if (str.length() == 0)
	{
		cout << "tokenizer: input string is empty" << endl;
		return -1;
	}

	while (getline(iss, token, ';'))
	{
		tokens.push_back(token);
	}
	return tokens.size();
}


//---------------------------------------------------------------------
///   Tokenize a string by the first colon found
///   \param str input string
///   \param tokens the output tokens
///   \return string with replaced values
//---------------------------------------------------------------------
bool tokenize_colon(const string& str, vector<string>& tokens)
{
	int delimiter_pos;

	// Don't read comments
	if (str.substr(0, 1) == "#")
		return false;

	delimiter_pos = str.find_first_of(':');

	if (delimiter_pos == -1)
		return false;

	tokens.push_back(str.substr(0, delimiter_pos));

	// Move one character forward, because we don't want the colon
	delimiter_pos++;

	// Also we don't want the first space after the colon
	if (str.substr(delimiter_pos, 1) == " ")
		delimiter_pos++;

	tokens.push_back(str.substr(delimiter_pos, str.length()));

	return true;
}
