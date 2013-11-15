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

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <string>

std::string upCaseString(std::string s); //!< returns an uppercased version of the string
std::string getExtension(std::string filename); //!< gets the extension of a file name (without the dot)
bool mkdirIfNeeded(const char* dirname);

class FileRAII {
	FILE* held;
public:
	FileRAII(FILE* init): held(init) {}
	~FileRAII() { if (held) fclose(held); held = NULL; }
	FileRAII(const FileRAII&) = delete;
	FileRAII& operator = (const FileRAII&) = delete;
};

#endif // __UTIL_H__
