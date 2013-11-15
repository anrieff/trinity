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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <algorithm>
#include "bitmap.h"
using std::swap;

static bool scanToNextWS(char data[], char res[], int& curIdx, int maxLen)
{
	int j = 0;
	int i = curIdx;
	while (i < maxLen && !isspace(data[i])) {
		res[j++] = data[i++];
	}
	if (i >= maxLen) return false;
	curIdx = i + 1;
	res[j] = 0;
	return true;
}

bool Bitmap::tryLoadPFM(FILE* f)
{
	char data[128], s[128];
	/* 
	 * A description of the file format is available at http://netpbm.sourceforge.net/doc/pfm.html
	 * PFM file start with a text header like
	 *
	 * PF
	 * 1024 768
	 * -1.000
	 *
	 * which describes the image size, endianines (because of the negative third row). After these three
	 * lines, a raster of binary (4-byte float) data follows.
	 */
	if (sizeof(data) != fread(data, 1, sizeof(data), f)) return false;
	int i = 0;
	if (!scanToNextWS(data, s, i, sizeof(data))) return false;
	if (strlen(s) != 2 || s[0] != 'P' || toupper(s[1] != 'F')) return false;
	bool gray = s[1] == 'f';
	int w, h;
	int bigEndian;
	double scaling;
	if (!scanToNextWS(data, s, i, sizeof(data)) || 1 != sscanf(s, "%d", &w)) return false;
	if (!scanToNextWS(data, s, i, sizeof(data)) || 1 != sscanf(s, "%d", &h)) return false;
	if (!scanToNextWS(data, s, i, sizeof(data)) || 1 != sscanf(s, "%lf", &scaling)) return false;
	bigEndian = scaling > 0;
	fseek(f, i, SEEK_SET);
	int lineSize = w * (gray ? 1 : 3) * 4;
	char *line = new char[lineSize];
	float* fline = (float*) line;
	generateEmptyImage(w, h);
	for (int y = 0; y < h; y++) {
		if (lineSize != (int) fread(line, 1, lineSize, f)) {
			delete[] line;
			return false;
		}
		if (bigEndian) {
			for (int i = 0; i < lineSize; i += 4) {
				swap(line[i], line[i + 3]);
				swap(line[i + 1], line[i + 2]);
			}
		}
		if (gray) {
			for (int x = 0; x < w; x++)
				setPixel(x, h - 1 - y, Color(fline[x], fline[x], fline[x]));
		} else {
			for (int x = 0; x < w; x++)
				setPixel(x, h - 1 - y, Color(fline[x*3], fline[x*3+1], fline[x*3+2]));
		}
	}
	delete[] line;
	return true;
}


bool Bitmap::loadPFM(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if (!f) return false;
	bool loadResult = tryLoadPFM(f);
	fclose(f);
	return loadResult;
}
