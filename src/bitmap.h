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

#include <functional>
#include "color.h"

/// @brief a class that represents a bitmap (2d array of colors), e.g. a image
/// supports loading/saving to BMP
class Bitmap {
protected:
	int width, height;
	Color* data;
	
	void remapRGB(std::function<float(float)>); // remap R, G, B channels by a function
	void copy(const Bitmap& rhs);
public:
	Bitmap(); //!< Generates an empty bitmap
	virtual ~Bitmap();
	Bitmap(const Bitmap& rhs);
	Bitmap& operator = (const Bitmap& rhs);
	void freeMem(void); //!< Deletes the memory, associated with the bitmap
	int getWidth(void) const; //!< Gets the width of the image (X-dimension)
	int getHeight(void) const; //!< Gets the height of the image (Y-dimension)
	bool isOK(void) const; //!< Returns true if the bitmap is valid
	void generateEmptyImage(int width, int height); //!< Creates an empty image with the given dimensions
	Color getPixel(int x, int y) const; //!< Gets the pixel at coordinates (x, y). Returns black if (x, y) is outside of the image
	/// Gets a bilinear-filtered pixel from float coords (x, y). The coordinates wrap when near the edges.
	Color getFilteredPixel(float x, float y) const;
	void setPixel(int x, int y, const Color& col); //!< Sets the pixel at coordinates (x, y).
	
	bool loadBMP(const char* filename); //!< Loads an image from a BMP file. Returns false in the case of an error
	bool loadEXR(const char* filename); //!< Loads an EXR file
	virtual bool loadImage(const char* filename); //!< Loads an image (autodetected)

	/// Saves the image to a BMP file (with clamping, etc). Uses the sRGB colorspace.
	/// Returns false in the case of an error (e.g. read-only media)
	bool saveBMP(const char* filename);
	
	/// Saves the image into the EXR format, preserving the dynamic range, using Half for storage. Note that
	/// in contrast with saveBMP(), it does not use gamma-compression on saving.
	bool saveEXR(const char* filename);
	virtual bool saveImage(const char* filename); //!< Save the bitmap to an image (the format is detected from extension)
	enum OutputFormat { /// the two supported writing formats
		outputFormat_BMP,
		outputFormat_EXR,
	};
	
	void decompressGamma_sRGB(void); //!< assuming the pixel data is in sRGB, decompress to linear RGB values
	void decompressGamma(float gamma); //!< as above, but assume a specific gamma value
	
	void differentiate(void); //!< differentiate image (red = dx, green = dy, blue = 0)
};

#endif // __BITMAP_H__
