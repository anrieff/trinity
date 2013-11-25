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
#ifndef __BITMAPEXT_H__
#define __BITMAPEXT_H__

#include <functional>
#include "bitmap.h"

// overloads the base class Bitmap, adding rescaling, and load/save to a few other file formats
class BitmapExt: public Bitmap {
	bool tryLoadPFM(FILE* f);
public:
	/// scale the bitmap down, so that the maximum of the new (width, height) becomes newMaxDim.
	/// the rescaling is uniform. If the bitmap is already smaller than the prescribed size, it is left unchanged.
	void rescale(int newMaxDim);
	
	Color* getData() { return data; }
	
	virtual bool loadImage(const char* filename); //!< Loads an image (autodetected format)
	bool loadHDR(const char* filename); //!< Loads a HDR-format file
	bool loadPFM(const char* filename); //!< Loads a PFM-format file
};

enum Format {
	SPHERICAL,
	ANGULAR,
	VCROSS,
	HCROSS,
	DIR,
	//
	UNDEFINED,
};

extern const char* CubeOrderNames[6];

class Vector;
class EnvironmentConverter {
	BitmapExt** maps;
	int numMaps;
	Format format;
	
	EnvironmentConverter(const EnvironmentConverter&) = delete;
	EnvironmentConverter& operator = (const EnvironmentConverter&) = delete;
	
	void convertCubemapToSpherical(int outSize);
	void convertSphericalToCubemap(int outSize);
	void projectCubeSide(BitmapExt& bmp, std::function<Vector(double, double)> mapSideToDir, int idx);
public:
	EnvironmentConverter();
	~EnvironmentConverter();
	
	bool load(const char* filename, Format inputFormat);
	bool save(const char* filename);
	void convert(Format targetFormat, int outSize);
	void multiply(float mult);
	
	BitmapExt& getMap(int index) { return *maps[index]; }
};

#endif // __BITMAPEXT_H__
