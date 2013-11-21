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
#include <string.h>
#include <assert.h>
#include <glob.h>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <functional>
#include "color.h"
#include "bitmapext.h"
#include "environment.h"
#include "util.h"
#include "vector.h"
using std::vector;
using std::string;
using std::sort;
using std::max;
using std::min;

const char* CubeOrderNames[6] = {
	"negx", "negy", "negz", "posx", "posy", "posz"
};
bool BitmapExt::loadImage(const char* filename)
{
	string ext = extensionUpper(filename);
	if (ext == "BMP")
		return loadBMP(filename);
	else if (ext == "EXR")
		return loadEXR(filename);
	else if (ext == "PFM")
		return loadPFM(filename);
	else if (ext == "HDR" || ext == "HDRI" || ext == "RGBE")
		return loadHDR(filename);
	else {
		printf("Bitmap::loadImageFile(): Unknown extension: `%s'\n", ext.c_str());
		return false;
	}
}

struct CoeffCacheElem {
	int src, dest;
	float* coefficients;
	CoeffCacheElem* next;
};

class CoeffCache {
	CoeffCacheElem* root;
public:
	CoeffCache() { root = NULL; }
	~CoeffCache() {
		CoeffCacheElem* e = root, *next;
		for (; e != NULL; e = next) {
			next = e->next;
			delete [] e->coefficients;
			delete e;
		}
	}
	float* getCoefficients(int src_len, int dest_len)
	{
		CoeffCacheElem* e = root;
		while (e) {
			if (e->src == src_len && e->dest == dest_len) return e->coefficients;
			e = e->next;
		}
		CoeffCacheElem* new_element = new CoeffCacheElem;
		new_element->src = src_len;
		new_element->dest = dest_len;
		new_element->coefficients = new float[dest_len];
		memset(new_element->coefficients, 0, sizeof(float) * dest_len);
		float ratio = (dest_len-1)/((float)src_len-1);
		for (int i = 0; i < src_len; i++) {
			float x = i * ratio;
			int xx = (int) x;
			float mul1 = 1.0f - (x-xx);
			new_element->coefficients[xx] += mul1;
			if (xx + 1 < dest_len)
				new_element->coefficients[xx + 1] += 1.0f - mul1;
		}
		for (int i = 0; i < dest_len; i++)
			new_element->coefficients[i] = 1.0f / new_element->coefficients[i];
			
		new_element->next = root;
		root = new_element;
		return new_element->coefficients;
	}
};

static CoeffCache coeffsCache;


static int arrayResize(Color *src, int src_len, Color *dest, int dest_len, float* coefficients)
{
	for (int i = 0; i < dest_len; i++) dest[i].makeZero();
	//
	float ratio = (dest_len-1)/((float)src_len-1);
	for (int i = 0; i < src_len; i++) {
		float x = i * ratio;
		int xx = (int) x;
		float mul1 = 1.0f - (x-xx);
		dest[xx].b += mul1 * src[i].b;
		dest[xx].g += mul1 * src[i].g;
		dest[xx].r += mul1 * src[i].r;
		if (xx + 1 < dest_len) {
			mul1 = 1.0f - mul1;
			dest[xx + 1].b += mul1 * src[i].b;
			dest[xx + 1].g += mul1 * src[i].g;
			dest[xx + 1].r += mul1 * src[i].r;
		}
	}
	for (int i = 0; i < dest_len; i++) {
		dest[i].b *= coefficients[i];
		dest[i].g *= coefficients[i];
		dest[i].r *= coefficients[i];
	}
	return 1;
}


void BitmapExt::rescale(int newMaxDim)
{
	if (max(width, height) <= newMaxDim) return;
	float scaleFactor = max(width, height) / float(newMaxDim);
	
	int newWidth  = (int) floor(width  / scaleFactor + 0.5f);
	int newHeight = (int) floor(height / scaleFactor + 0.5f);
	
	Color* newData = new Color[newWidth * newHeight];
	
	// resize by x:
	Color row[width];
	float* coefficients = coeffsCache.getCoefficients(width, newWidth);
	for (int y = 0; y < height; y++) {
		arrayResize(data + (y * width), width, row, newWidth, coefficients);
		memcpy(data + (y * width), row, newWidth * sizeof(Color));
	}
	
	// resize by y:
	coefficients = coeffsCache.getCoefficients(height, newHeight);
	Color column[height], newColumn[newHeight];
	for (int x = 0; x < newWidth; x++) {
		for (int y = 0; y < height; y++)
			column[y] = data[x + y * width];
		arrayResize(column, height, newColumn, newHeight, coefficients);
		for (int y = 0; y < newHeight; y++)
			newData[x + y * newWidth] = newColumn[y];
	}
	delete[] data;
	data = newData;
	height = newHeight;
	width = newWidth;
}


EnvironmentConverter::EnvironmentConverter()
{
	maps = NULL;
	numMaps = 0;
	format = UNDEFINED;
}

EnvironmentConverter::~EnvironmentConverter()
{
	if (numMaps && maps) {
		for (int i = 0; i < numMaps; i++) {
			delete maps[i];
			maps[i] = NULL;
		}
		delete [] maps;
		maps = NULL;
		numMaps = 0;
	}
}

bool EnvironmentConverter::load(const char* filename, Format inputFormat)
{
	format = inputFormat;
	numMaps = (format == DIR) ? 6 : 1;
	maps = new BitmapExt* [numMaps];
	for (int i = 0; i < numMaps; i++)
		maps[i] = new BitmapExt;
	if (format != DIR) {
		return maps[0]->loadImage(filename);
	} else {
		glob_t gl;
		char pattern[strlen(filename) + 10];
		strcpy(pattern, filename);
		strcat(pattern, "/*.*");
		if (glob(pattern, 0, NULL, &gl)) return false;
		vector<string> names;
		for (int i = 0; i < gl.gl_pathc; i++)
			names.push_back(gl.gl_pathv[i]);
		globfree(&gl);
		
		sort(names.begin(), names.end());
		if (names.size() != 6) return false;
		for (int i = 0; i < 6; i++) {
			if (names[i].find(CubeOrderNames[i]) == string::npos) {
				printf("EnvironmentConverter::load: Couldn't find a file %s in directory %s\n", CubeOrderNames[i], filename);
				return false;
			}
			if (!maps[i]->loadImage(names[i].c_str())) {
				printf("EnvironmentConverter::load: Couldn't load the file %s from directory %s\n", names[i].c_str(), filename);
				return false;
			}
		}
	}
	return true;
}

static bool mkdirIfNeeded(const char* dirname)
{
	struct stat st;
	if (stat(dirname, &st) == 0) return true;
	return mkdir(dirname, 0777) == 0;
}

bool EnvironmentConverter::save(const char* filename)
{
	if (format == SPHERICAL) {
		return maps[0]->saveEXR(filename);
	} else {
		assert(format == DIR);
		//filename is a dirname. Create all directories up to it:
		char s[strlen(filename) + 3];
		strcpy(s, filename);
		int l = (int) strlen(s);
		for (int i = 1; i < l; i++) if (s[i] == '/') {
			s[i] = 0;
			if (!mkdirIfNeeded(s)) return false;
			s[i] = '/';
		}
		if (s[l - 1] != '/')
			if (!mkdirIfNeeded(s)) return false;
		for (int i = 0; i < 6; i++) {
			char fullFileName[strlen(filename) + 16];
			strcpy(fullFileName, filename);
			if (filename[l - 1] != '/' && filename[l - 1] != '\\')
				strcat(fullFileName, "/");
			strcat(fullFileName, CubeOrderNames[i]);
			strcat(fullFileName, ".exr");
			if (!maps[i]->saveEXR(fullFileName)) return false;
		}
		return true;
	}
}

static void copyBmp(const BitmapExt* source, int offsetX, int offsetY, BitmapExt* dest)
{
	for (int y = 0; y < dest->getHeight(); y++)
		for (int x = 0; x < dest->getWidth(); x++)
			dest->setPixel(x, y, source->getPixel(x + offsetX, y + offsetY));
}

void EnvironmentConverter::convert(Format targetFormat, int outSize)
{
	if (format == ANGULAR) {
		// convert to spherical:
		BitmapExt& bmp = *maps[0];
		Color newRow[bmp.getWidth()];
		for (int y = 0; y < bmp.getHeight(); y++) {
			float ry = ((y / float(bmp.getHeight() - 1)) - 0.5f) * 2.0f;
			float scaling = sqrtf(1 - ry * ry);
			for (int x = 0; x < bmp.getHeight(); x++) {
				float fx = ((x / float(bmp.getWidth() - 1)) - 0.5) * 2.0f;
				fx *= scaling;
				int srcx = (int) floor(0.5f + (fx / 2.0f + 0.5) * (bmp.getWidth() - 1));
				if (srcx < 0) srcx = 0;
				if (srcx >= bmp.getWidth()) srcx = bmp.getWidth() - 1;
				newRow[x] = bmp.getPixel(srcx, y);
			}
			for (int x = 0; x < bmp.getWidth(); x++) {
				bmp.setPixel(x, y, newRow[x]);
			}
		}
		format = SPHERICAL;
	}
	if (format == VCROSS || format == HCROSS) {
		// convert CROSS format into separate cube sides:
		BitmapExt* bmp = maps[0];
		delete[] maps;
		
		maps = new BitmapExt*[6];
		numMaps = 6;
		
		int squareSize = max(bmp->getWidth(), bmp->getHeight()) / 4;
		for (int i = 0; i < 6; i++) {
			maps[i] = new BitmapExt;
			maps[i]->generateEmptyImage(squareSize, squareSize);
		}
		
		int S = squareSize;
		if (format == VCROSS) {
			copyBmp(bmp, S    , 0    , maps[POSY]);
			copyBmp(bmp, 0    , S    , maps[NEGX]);
			copyBmp(bmp, S    , S    , maps[POSZ]);
			copyBmp(bmp, 2 * S, S    , maps[POSX]);
			copyBmp(bmp, S    , 2 * S, maps[NEGY]);
			copyBmp(bmp, S    , 3 * S, maps[NEGZ]);
		} else {
			copyBmp(bmp, S    , 0    , maps[POSY]);
			copyBmp(bmp, 0    , S    , maps[NEGX]);
			copyBmp(bmp, S    , S    , maps[POSZ]);
			copyBmp(bmp, 2 * S, S    , maps[POSX]);
			copyBmp(bmp, 3 * S, S    , maps[NEGZ]);
			copyBmp(bmp, S    , 2 * S, maps[NEGY]);
		}
		
		delete bmp;
		format = DIR;
	}
	
	if (format == DIR && targetFormat == SPHERICAL)
		convertCubemapToSpherical(outSize);
	if (format == SPHERICAL && targetFormat == DIR)
		convertSphericalToCubemap(outSize);
}

void EnvironmentConverter::multiply(float mult)
{
	for (int i = 0; i < numMaps; i++) {
		Color* buff = maps[i]->getData();
		for (int j = 0; j < maps[i]->getWidth() * maps[i]->getHeight(); j++)
			buff[j] *= mult;
	}
}

void EnvironmentConverter::convertCubemapToSpherical(int outSize)
{
	BitmapExt* newMap = new BitmapExt;
	
	if (outSize == -1) {
		// auto-calculate the spherical size; vertical is 2*vertial_of_cubemap:
		outSize = 4 * maps[0]->getHeight();
	}

	newMap->generateEmptyImage(outSize, outSize / 2);
	const int SUPERSAMPLES = 4; // actual number of samples is the square of that number.
	int W = newMap->getWidth();
	Color* data = newMap->getData();
	
	Bitmap* bmps[6];
	for (int i = 0; i < 6; i++) bmps[i] = maps[i];
	CubemapEnvironment cubemap(bmps);
	
	// resample:
	for (int y = 0; y < outSize / 2; y++) {
		for (int yss = 0; yss < SUPERSAMPLES; yss++) {
			printf("\rConverting CubeMap->SphericalMap... %6.2lf%%", (y * SUPERSAMPLES + yss) * 100.0 / ((outSize / 2) * SUPERSAMPLES));
			fflush(stdout);
			double theta = (y * SUPERSAMPLES + yss) / (double) ((outSize / 2) * SUPERSAMPLES - 1); // [0..1]
			theta = -(theta * PI - PI/2); // [-PI/2 .. PI/2]
			double cosTheta = cos(theta);
			double sinTheta = sin(theta);
			for (int x = 0; x < outSize; x++) {
				for (int xss = 0; xss < SUPERSAMPLES; xss++) {
					double phi = (x * SUPERSAMPLES + xss) / (double) (outSize * SUPERSAMPLES - 1); // [0..1]
					phi = phi * 2 * PI; // [0 .. 2 PI]
					Vector dir(
						sin(phi + PI) * cosTheta,
						sinTheta,
						cos(phi + PI) * cosTheta
					);
					data[y * W + x] += cubemap.getEnvironment(dir);
				}
			}
		}
	}
	printf("\rConverted: CubeMap->SphericalMap           \n");
	// normalize:
	float mult = 1.0f / (SUPERSAMPLES * SUPERSAMPLES);
	for (int y = 0; y < newMap->getHeight(); y++)
		for (int x = 0; x < newMap->getWidth(); x++)
			data[y * W + x] *= mult;
	
	// destroy old maps:
	for (int i = 0; i < numMaps; i++)
		delete maps[i];
	delete[] maps;
	
	// set new fmt:
	format = SPHERICAL;
	numMaps = 1;
	maps = new BitmapExt* [numMaps];
	maps[0] = newMap;
}

void EnvironmentConverter::projectCubeSide(BitmapExt& bmp, std::function<Vector(double, double)> mapSideToDir, int idx)
{
	int S = bmp.getWidth();
	Color* data = bmp.getData();
	const int SUPERSAMPLES = 4; // actual number of samples is the square of that number.
	for (int y = 0; y < S; y++) {
		for (int yss = 0; yss < SUPERSAMPLES; yss++) {
			printf("\rConverting SphericalMap->CubeMap[%s]... %6.2lf%%", CubeOrderNames[idx], (y * SUPERSAMPLES + yss) * 100.0 / (S * SUPERSAMPLES));
			fflush(stdout);
			double py = (y * SUPERSAMPLES + yss) / (double) (S * SUPERSAMPLES - 1); // [0..1]
			py = (py - 0.5) * 2; // [-1..1]
			for (int x = 0; x < S; x++) {
				for (int xss = 0; xss < SUPERSAMPLES; xss++) {
					double px = (x * SUPERSAMPLES + xss) / (double) (S * SUPERSAMPLES - 1); // [0..1]
					px = (px - 0.5) * 2; // [-1..1]
					Vector dir = mapSideToDir(px, py);
					dir.normalize();
					double theta = acos(dir.y); // [0..PI]
					double phi = atan2(dir.z, dir.x); // [-PI..PI]
					
					theta = theta / PI; // [0..1]
					phi   = -(phi + PI/2 + 2*PI) / (2 * PI);
					phi  -= floor(phi); // [0..1]
					data[y * S + x] += maps[0]->getPixel((int) (phi * maps[0]->getWidth()), (int) (theta * maps[0]->getHeight()));
				}
			}
		}
	}
	printf("\rConverted: SphericalMap->CubeMap[%s]                   \n", CubeOrderNames[idx]);
	float mult = 1.0f / (SUPERSAMPLES * SUPERSAMPLES);
	for (int y = 0; y < S; y++)
		for (int x = 0; x < S; x++)
			data[y * S + x] *= mult;
}

void EnvironmentConverter::convertSphericalToCubemap(int outSize)
{
	BitmapExt** newMaps = new BitmapExt* [6];
	
	if (outSize == -1) {
		// auto-calculate the cubemap size; size is vertial_of_spheremap / 2
		outSize = maps[0]->getHeight() / 2;
	}
	for (int i = 0; i < 6; i++) {
		newMaps[i] = new BitmapExt;
		newMaps[i]->generateEmptyImage(outSize, outSize);
	}
	//
	// a set of functions, to convert 2D parameters (coordinates on a cube side) onto a 3D point
	// (a point on the cube on the respective side)
	std::function<Vector(double, double)> remapFunctions[6] = {
		[] (double x, double y) { return Vector(-1, -y,  x); }, // NEGX side
		[] (double x, double y) { return Vector( x, -1, -y); }, // NEGY side
		[] (double x, double y) { return Vector( x,  y, -1); }, // NEGZ side
		[] (double x, double y) { return Vector(+1, -y, -x); }, // POSX side
		[] (double x, double y) { return Vector( x, +1,  y); }, // POSY side
		[] (double x, double y) { return Vector( x, -y, +1); }, // POSZ side
	};
	for (int side = NEGX; side <= POSZ; side++)
		projectCubeSide(*newMaps[side], remapFunctions[side], side);
	
	// destroy old maps:
	for (int i = 0; i < numMaps; i++)
		delete maps[i];
	delete[] maps;
	
	// set new fmt:
	format = DIR;
	numMaps = 6;
	maps = newMaps;
}
