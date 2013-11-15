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
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stdio.h>
#include "color.h"

/// @brief a class that represents a bitmap (2d array of colors), e.g. a image
/// supports loading/saving to BMP
class Bitmap {
	int width, height;
	Color* data;
	
	bool tryLoadPFM(FILE* f);
public:
	Bitmap(); //!< Generates an empty bitmap
	~Bitmap();
	void freeMem(void); //!< Deletes the memory, associated with the bitmap
	int getWidth(void) const; //!< Gets the width of the image (X-dimension)
	int getHeight(void) const; //!< Gets the height of the image (Y-dimension)
	bool isOK(void) const; //!< Returns true if the bitmap is valid
	void generateEmptyImage(int width, int height); //!< Creates an empty image with the given dimensions
	Color getPixel(int x, int y) const; //!< Gets the pixel at coordinates (x, y). Returns black if (x, y) is outside of the image
	void setPixel(int x, int y, const Color& col); //!< Sets the pixel at coordinates (x, y).
	Color* getData() { return data; }
	
	/// scale the bitmap down, so that the maximum of the new (width, height) becomes newMaxDim.
	/// the rescaling is uniform. If the bitmap is already smaller than the prescribed size, it is left unchanged.
	void rescale(int newMaxDim);
	
	bool loadImageFile(const char* filename); //!< Loads an image (autodetected format)
	bool loadBMP(const char* filename); //!< Loads an image from a BMP file. Returns false in the case of an error
	bool loadEXR(const char* filename); //!< Loads an EXR file
	bool loadHDR(const char* filename); //!< Loads a HDR-format file
	bool loadPFM(const char* filename); //!< Loads a PFM-format file
	
	bool saveBMP(const char* filename); //!< Saves the image to a BMP file (with clamping, etc). Returns false in the case of an error (e.g. read-only media)
	bool saveEXR(const char* filename); //!< Saves the image into the EXR format, preserving the dynamic range, using Half for storage.
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

enum CubeOrder {
	NEGX,
	NEGY,
	NEGZ,
	POSX,
	POSY,
	POSZ,
};

extern const char* CubeOrderNames[6];

class Environment {
	Bitmap** maps;
	int numMaps;
	Format format;
	
	Environment(const Environment&) = delete;
	Environment& operator = (const Environment&) = delete;
public:
	Environment();
	~Environment();
	
	bool load(const char* filename, Format inputFormat);
	bool save(const char* filename);
	void convert(Format targetFormat, int outSize);
	void multiply(float mult);
	
	Bitmap& getMap(int index) { return *maps[index]; }
};

#endif // __BITMAP_H__
