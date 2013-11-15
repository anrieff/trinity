/***************************************************************************
 *   Copyright (C) 2009-2013 by Veselin Georgiev, Slavomir Kaslev et al    *
 *   admin@raytracing-bg.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
using namespace std;

string upCaseString(string s)
{
	transform(s.begin(), s.end(), s.begin(), [] (char c) { return toupper(c); } );
	return s;
}

string getExtension(string filename)
{
	for (int i = (int) filename.size() - 1; i >= 0; i--)
		if (filename[i] == '.') return filename.substr(i + 1);
	return filename;
}

bool mkdirIfNeeded(const char* dirname)
{
	struct stat st;
	if (stat(dirname, &st) == 0) return true;
	return mkdir(dirname, 0777) == 0;
}
