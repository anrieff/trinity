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

#include "bitmapext.h"
#include "rgbe.h"
#include "util.h"
#include <vector>
using std::vector;

bool BitmapExt::loadHDR(const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (!fp) return false;
	//
	FileRAII holder(fp);
	rgbe_header_info header;
	if (RGBE_ReadHeader(fp, &width, &height, &header) != RGBE_RETURN_SUCCESS)
		return false;
	//
	data = new Color[width * height];
	vector<float> temp(width * 3);
	for (int y = 0; y < height; y++) {
		int res = RGBE_ReadPixels_RLE(fp, &temp[0], width, 1);
		if (res != RGBE_RETURN_SUCCESS) return false;
		for (int x = 0; x < width; x++) 
			data[x + y * width] = Color(temp[x * 3], temp[x * 3 + 1], temp[x * 3 + 2]);
	}
	return true;
}
