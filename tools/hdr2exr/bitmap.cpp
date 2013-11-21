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
#include "color.h"
#include "bitmap.h"
#include "util.h"
using std::vector;
using std::string;
using std::sort;
using std::max;
using std::min;

const char* CubeOrderNames[6] = {
	"negx", "negy", "negz", "posx", "posy", "posz"
};

Bitmap::Bitmap()
{
	width = height = -1;
	data = NULL;
}

Bitmap::~Bitmap()
{
	freeMem();
}

void Bitmap::freeMem(void)
{
	if (data) delete [] data;
	data = NULL;
	width = height = -1;
}

int Bitmap::getWidth(void) const { return width; }
int Bitmap::getHeight(void) const { return height; }
bool Bitmap::isOK(void) const { return (data != NULL); }

void Bitmap::generateEmptyImage(int w, int h)
{
	freeMem();
	if (w <= 0 || h <= 0) return;
	width = w;
	height = h;
	data = new Color[w * h];
	memset(data, 0, sizeof(data[0]) * w * h);
}

Color Bitmap::getPixel(int x, int y) const
{
	if (!data || x < 0 || x >= width || y < 0 || y >= height) return Color(0.0f, 0.0f, 0.0f);
	return data[x + y * width];
}

void Bitmap::setPixel(int x, int y, const Color& color)
{
	if (!data || x < 0 || x >= width || y < 0 || y >= height) return;
	data[x + y * width] = color;
}

class ImageOpenRAII {
	Bitmap *bmp;
public:
	bool imageIsOk;
	FILE* fp;
	ImageOpenRAII(Bitmap *bitmap)
	{
		fp = NULL;
		bmp = bitmap;
		imageIsOk = false;
	}
	~ImageOpenRAII()
	{
		if (!imageIsOk) bmp->freeMem();
		if (fp) fclose(fp); fp = NULL;
	}
	
};

const int BM_MAGIC = 19778;

struct BmpHeader {
	int fs;       // filesize
	int lzero;
	int bfImgOffset;  // basic header size
};
struct BmpInfoHeader {
	int ihdrsize; 	// info header size
	int x,y;      	// image dimensions
	unsigned short channels;// number of planes
	unsigned short bitsperpixel;
	int compression; // 0 = no compression
	int biSizeImage; // used for compression; otherwise 0
	int pixPerMeterX, pixPerMeterY; // dots per meter
	int colors;	 // number of used colors. If all specified by the bitsize are used, then it should be 0
	int colorsImportant; // number of "important" colors (wtf?..)
};

bool Bitmap::loadBMP(const char* filename)
{
	freeMem();
	ImageOpenRAII helper(this);
	
	BmpHeader hd;
	BmpInfoHeader hi;
	Color palette[256];
	int toread = 0;
	unsigned char *xx;
	int rowsz;
	unsigned short sign;
	FILE* fp = fopen(filename, "rb");
	
	if (fp == NULL) {
		printf("loadBMP: Can't open file: `%s'\n", filename);
		return false;
	}
	helper.fp = fp;
	if (!fread(&sign, 2, 1, fp)) return false;
	if (sign != BM_MAGIC) {
		printf("loadBMP: `%s' is not a BMP file.\n", filename);
		return false;
	}
	if (!fread(&hd, sizeof(hd), 1, fp)) return false;
	if (!fread(&hi, sizeof(hi), 1, fp)) return false;
	
	/* header correctness checks */
	if (!(hi.bitsperpixel == 8 || hi.bitsperpixel == 24 ||  hi.bitsperpixel == 32)) {
		printf("loadBMP: Cannot handle file format at %d bpp.\n", hi.bitsperpixel); 
		return false;
	}
	if (hi.channels != 1) {
		printf("loadBMP: cannot load multichannel .bmp!\n");
		return false;
	}
	/* ****** header is OK *******/
	
	// if image is 8 bits per pixel or less (indexed mode), read some pallete data
	if (hi.bitsperpixel <= 8) {
		toread = (1 << hi.bitsperpixel);
		if (hi.colors) toread = hi.colors;
		for (int i = 0; i < toread; i++) {
			unsigned temp;
			if (!fread(&temp, 1, 4, fp)) return false;
			palette[i] = Color(temp);
		}
	}
	toread = hd.bfImgOffset - (54 + toread*4);
	fseek(fp, toread, SEEK_CUR); // skip the rest of the header
	int k = hi.bitsperpixel / 8;
	rowsz = hi.x * k;
	if (rowsz % 4 != 0)
		rowsz = (rowsz / 4 + 1) * 4; // round the row size to the next exact multiple of 4
	xx = new unsigned char[rowsz];
	generateEmptyImage(hi.x, hi.y);
	if (!isOK()) {
		printf("loadBMP: cannot allocate memory for bitmap! Check file integrity!\n");
		delete [] xx;
		return false;
	}
	for (int j = hi.y - 1; j >= 0; j--) {// bitmaps are saved in inverted y
		if (!fread(xx, 1, rowsz, fp)) {
			printf("loadBMP: short read while opening `%s', file is probably incomplete!\n", filename);
			delete [] xx;
			return 0;
		}
		for (int i = 0; i < hi.x; i++){ // actually read the pixels
			if (hi.bitsperpixel > 8)
				setPixel(i, j, Color(xx[i*k+2]/255.0f, xx[i*k+1]/255.0f, xx[i*k]/255.0f));
			else
				setPixel(i, j,  palette[xx[i*k]]);
		}
	}
	delete [] xx;
	
	helper.imageIsOk = true;
	return true;
}

bool Bitmap::saveBMP(const char* filename)
{
	FILE* fp = fopen(filename, "wb");
	if (!fp) return false;
	BmpHeader hd;
	BmpInfoHeader hi;
	char xx[16384 * 3];


	// fill in the header:
	int rowsz = width * 3;
	if (rowsz % 4)
		rowsz += 4 - (rowsz % 4); // each row in of the image should be filled with zeroes to the next multiple-of-four boundary
	hd.fs = rowsz * height + 54; //std image size
	hd.lzero = 0;
	hd.bfImgOffset = 54;
	hi.ihdrsize = 40;
	hi.x = width; hi.y = height;
	hi.channels = 1;
	hi.bitsperpixel = 24; //RGB format
	// set the rest of the header to default values:
	hi.compression = hi.biSizeImage = 0;
	hi.pixPerMeterX = hi.pixPerMeterY = 0;
	hi.colors = hi.colorsImportant = 0;
	
	fwrite(&BM_MAGIC, 2, 1, fp); // write 'BM'
	fwrite(&hd, sizeof(hd), 1, fp); // write file header
	fwrite(&hi, sizeof(hi), 1, fp); // write image header
	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {
			unsigned t = getPixel(x, y).toRGB32();
			xx[x * 3    ] = (0xff     & t);
			xx[x * 3 + 1] = (0xff00   & t) >> 8;
			xx[x * 3 + 2] = (0xff0000 & t) >> 16;
		}
		fwrite(xx, rowsz, 1, fp);
	}
	fclose(fp);
	return true;
}

bool Bitmap::loadImageFile(const char* filename)
{
	string ext = upCaseString(getExtension(filename));
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


void Bitmap::rescale(int newMaxDim)
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


Environment::Environment()
{
	maps = NULL;
	numMaps = 0;
	format = UNDEFINED;
}

Environment::~Environment()
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

bool Environment::load(const char* filename, Format inputFormat)
{
	format = inputFormat;
	numMaps = (format == DIR) ? 6 : 1;
	maps = new Bitmap* [numMaps];
	for (int i = 0; i < numMaps; i++)
		maps[i] = new Bitmap;
	if (format != DIR) {
		return maps[0]->loadImageFile(filename);
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
				printf("Environment::load: Couldn't find a file %s in directory %s\n", CubeOrderNames[i], filename);
				return false;
			}
			if (!maps[i]->loadImageFile(names[i].c_str())) {
				printf("Environment::load: Couldn't load the file %s from directory %s\n", names[i].c_str(), filename);
				return false;
			}
		}
	}
	return true;
}

bool Environment::save(const char* filename)
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

static void copyBmp(const Bitmap* source, int offsetX, int offsetY, Bitmap* dest)
{
	for (int y = 0; y < dest->getHeight(); y++)
		for (int x = 0; x < dest->getWidth(); x++)
			dest->setPixel(x, y, source->getPixel(x + offsetX, y + offsetY));
}

void Environment::convert(Format targetFormat, int outSize)
{
	if (format == ANGULAR) {
		// convert to spherical:
		Bitmap& bmp = *maps[0];
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
		Bitmap* bmp = maps[0];
		delete[] maps;
		
		maps = new Bitmap*[6];
		numMaps = 6;
		
		int squareSize = max(bmp->getWidth(), bmp->getHeight()) / 4;
		for (int i = 0; i < 6; i++) {
			maps[i] = new Bitmap;
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
}

void Environment::multiply(float mult)
{
	for (int i = 0; i < numMaps; i++) {
		Color* buff = maps[i]->getData();
		for (int j = 0; j < maps[i]->getWidth() * maps[i]->getHeight(); j++)
			buff[j] *= mult;
	}
}
