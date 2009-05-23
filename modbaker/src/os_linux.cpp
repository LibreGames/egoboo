#include "utility.h"
#include <sys/types.h>
#include <dirent.h>


//---------------------------------------------------------------------
//-   Get all files from a directory
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
			cout << "GETTING FILE " << filename << endl;
			p_files.push_back(filename);
		}
	}

	closedir(dir_handler);
	return true;
}
