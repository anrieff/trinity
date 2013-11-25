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
#include "environment.h"
#include "bitmap.h"

bool CubemapEnvironment::loadMaps(const char* folder)
{
	// the maps are stored in order - negx, negy, negz, posx, posy, posz
	const char* prefixes[2] = {"neg", "pos"};
	const char* axes[3] = {"x", "y", "z"};
	const char* suffixes[2] = {".bmp", ".exr"};
	memset(maps, 0, sizeof(maps));
	int n = 0;
	for (int pi = 0; pi < 2; pi++)
		for (int axis = 0; axis < 3; axis++) {
			Bitmap* map = new Bitmap;
			char fn[256];
			for (int si = 0; si < 2; si++) {
				sprintf(fn, "%s/%s%s%s", folder, prefixes[pi], axes[axis], suffixes[si]);
				if (fileExists(fn) && map->loadImage(fn)) break;
			}
			if (!map->isOK()) return false;
			maps[n++] = map;
		}
	return true;
}

CubemapEnvironment::CubemapEnvironment(const char* folder)
{
	memset(maps, 0, sizeof(maps));
	loadMaps(folder);
	owned = true;
}

CubemapEnvironment::CubemapEnvironment(Bitmap** inputmaps)
{
	for (int i = 0; i < 6; i++) maps[i] = inputmaps[i];
	owned = false;
}


CubemapEnvironment::~CubemapEnvironment()
{
	if (owned) {
		for (int i = 0; i < 6; i++) {
			if (maps[i]) {
				delete maps[i];
				maps[i] = NULL;
			}
		}
	}
}

// a helper function (see getEnvironment()) that accepts two coordinates within the square
// (-1, -1) .. (+1, +1), and transforms them to (0, 0)..(W, H) where W, H are the bitmap width and height.
Color CubemapEnvironment::getSide(const Bitmap& bmp, double x, double y)
{
	// Nearest neighbour:
	return bmp.getFilteredPixel(float((x + 1) * 0.5 * (bmp.getWidth()-1)), float((y + 1) * 0.5 * (bmp.getHeight()-1)));
}

Color CubemapEnvironment::getEnvironment(const Vector& indir)
{
	Vector vec = indir;
	// Get a color from a cube-map
	// First, we get at which dimension, the absolute value of the direction is largest
	// (it is 0, 1 or 2, which is, respectively, X, Y or Z)
	int maxDim = vec.maxDimension();
	
	// Normalize the vector, so that now its largest dimension is either +1, or -1
	Vector t = vec / fabs(vec[maxDim]);
	
	// Create a state of (maximalDimension * 2) + (1 if it is negative)
	// so that:
	// if state is 0, the max dimension is 0 (X) and it is positive -> we hit the +X side
	// if state is 1, the max dimension is 0 (X) and it is negative -> we hit the -X side
	// if state is 2, the max dimension is 1 (Y) and it is positive -> we hit the +Y side
	// state is 3 -> -Y
	// state is 4 -> +Z
	// state is 5 -> -Z
	int state = ((t[maxDim] < 0) ? 0 : 3) + maxDim;
	switch (state) {
		// for each case, we have to use the other two dimensions as coordinates within the bitmap for
		// that side. The ordering of plusses and minuses is specific for the arrangement of
		// bitmaps we use (the orientations are specific for vertical-cross type format, where each
		// cube side is taken verbatim from a 3:4 image of V-cross environment texture.
		
		// In every case, the other two coordinates are real numbers in the square (-1, -1)..(+1, +1)
		// We use the getSide() helper function, to convert these coordinates to texture coordinates and fetch
		// the color value from the bitmap.
		case 0: return getSide(*maps[NEGX], t.z, -t.y);
		case 1: return getSide(*maps[NEGY], t.x, -t.z);
		case 2: return getSide(*maps[NEGZ], t.x, t.y);
		case 3: return getSide(*maps[POSX], -t.z, -t.y);
		case 4: return getSide(*maps[POSY], t.x, t.z);
		case 5: return getSide(*maps[POSZ], t.x, -t.y);
		default: return Color(0.0f, 0.0f, 0.0f);
	}
}
