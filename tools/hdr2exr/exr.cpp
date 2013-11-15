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
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <Iex.h>
#include <vector>
#include "bitmap.h"

bool Bitmap::loadEXR(const char* filename)
{
	try {
		Imf::RgbaInputFile exr(filename);
		Imf::Array2D<Imf::Rgba> pixels;
		Imath::Box2i dw = exr.dataWindow();
		width  = dw.max.x - dw.min.x + 1;
		height = dw.max.y - dw.min.y + 1;
		pixels.resizeErase(height, width);
		exr.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
		exr.readPixels(dw.min.y, dw.max.y);
		data = new Color[width * height];
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++) {
				Color& pixel = data[y * width + x];
				pixel.r = pixels[y + dw.min.y][x + dw.min.x].r;
				pixel.g = pixels[y + dw.min.y][x + dw.min.x].g;
				pixel.b = pixels[y + dw.min.y][x + dw.min.x].b;
			}
		return true;
	}
	catch (Iex::BaseExc ex) {
		width = height = 0;
		data = NULL;
		return false;
	}
}

bool Bitmap::saveEXR(const char* filename)
{
	try {
		Imf::RgbaOutputFile file(filename, width, height, Imf::WRITE_RGBA);
		std::vector<Imf::Rgba> temp(width * height);
		for (int i = 0; i < width * height; i++) {
			temp[i].r = data[i].r;
			temp[i].g = data[i].g;
			temp[i].b = data[i].b;
			temp[i].a = 1.0f;
		}
		file.setFrameBuffer(&temp[0], 1, width);
		file.writePixels(height);
	}
	catch (Iex::BaseExc ex) {
		return false;
	}
	return true;
}
