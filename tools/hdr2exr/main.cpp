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

/*
 * A simple util to convert environment maps between some common formats
 * The results are always in .EXR, half-format pixels.
 * 
 * See the usage for more info.
 * 
 * To compile this, you need the OpenEXR development headers and libs.
 * On Fedora, you need `OpenEXR-devel'. On Ubuntu, it's called `libopenexr-dev'
 */ 

#include <stdio.h>
#include <SDL/SDL.h>
#include <string>
#include "util.h"
#include "bitmap.h"
using namespace std;

const char* USAGE = ""
"Usage: hdr2exr <INPUT FILE> [OUTPUT FILE] [OPTIONS]\n"
"\n"
"The INPUT FILE can be .hdr/.pfm/.exr/.bmp or a directory (see below)\n"
"The OUTPUT FILE must be .exr (it is saved as 16 bpp half-float pixels).\n"
"If no OUTPUT FILE is given, the input image is just displayed in a window,\n"
"using spherical mapping (use the +/- buttons to adjust brightness)\n"
"\n"
"Options:\n"
"   -mult <multiplier> - multiply image's pixels after reading\n"
"   -size <pixels>     - the size of the larger side of the output\n"
"   -fmt FORMAT-SPECIFICATION\n"
"\n"
"The FORMAT-SPECIFICATION tells the utility if any kind of remapping is needed.\n"
"This is optional - if you don't give it, the output image copies the input\n"
"pixel-by-pixel.\n"
"\n"
"To elaborate some more, here are the common formats for representing\n"
"environment maps:\n"
"\n"
"Name      | Description\n"
"----------+------------------------------------------------------------------\n"
"spherical | A spherical environment. Each pixel corresponds to some theta/phi\n"
"          | spherical coordinates.\n"
"          |\n"
"angular   | Similar to spherical. The image is a disc. Used in Paul Debevec's\n"
"          | HDR probe repository for example.\n"
"          |\n"
"V-cross   | Six sides of a cubemap, placed next one to another in the form of\n"
"          | a cross. The six sides are of exactly the same size. The middle\n"
"          | side is assumed to be the +Z side.\n"
"          |\n"
"H-cross   | Similar to V-cross, however the cross's longer side is now\n"
"          | horizontal\n"
"          |\n"
"dir       | A directory with six files - posx, negx, posy, ..., negz - each\n"
"          | file represents a cubemap direction. When specifying this type,\n"
"          | the respective INPUT/OUTPUT argument should be the name of a\n"
"          | directory. The output dir will be created if it doesn't exist.\n"
"\n"
"If you want to convert between these representations, use a FORMAT-CONVERSION\n"
"specfier named like `source:dest'. The `:' is mandatory. E.g.,\n"
"\n"
"   $ hdr2exr source.hdr angular:spherical destination.exr\n"
"\n"
"Will convert a file named `source.hdr', assumed to be in angular format, to\n"
"an output file named `destination.exr', remapped as a spherical environment.\n"
"\n"
"If any of the two specificators is omitted, the default will be used, which is\n"
"`spherical'. The same applies to the preview mode (it will display the\n"
"environment map as if it is spherical format)\n";

string inFile, outFile;
Format inFmt = SPHERICAL, outFmt = SPHERICAL;
float mult = 1.0f;
int outSize = -1;

static bool error(const char* msg)
{
	printf("Error: %s\n", msg);
	return false;
}

static Format parseSingleFmtSpecifier(const string& par)
{
	if (par.empty()) return SPHERICAL;
	const struct
		{ Format fmt; const char* name; }
		FMTS[] = {
			{ SPHERICAL, "SPHERICAL" },
			{ ANGULAR,   "ANGULAR"   },
			{ VCROSS,    "V-CROSS"   },
			{ HCROSS,    "H-CROSS"   },
			{ DIR,       "DIR"       },
		};
	for (int i = 0; i < (int) sizeof(FMTS) / sizeof(FMTS[0]); i++)
		if (par == FMTS[i].name)
			return FMTS[i].fmt;
	return UNDEFINED;
}

static bool parseFmtSpecifier(string par)
{
	auto i = par.find(':');
	if (i == string::npos) return false;
	
	inFmt = parseSingleFmtSpecifier(upCaseString(par.substr(0, i)));
	outFmt = parseSingleFmtSpecifier(upCaseString(par.substr(i + 1)));
	if (inFmt == UNDEFINED || outFmt == UNDEFINED) return false;
	if (outFmt != DIR && outFmt != SPHERICAL) {
		printf("Error: the output format must be one of { dir, spherical }\n");
		return false;
	}
	return true;
}

static bool parseCmdLine(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		string par = argv[i];
		if (par == "-h" || par == "--help") {
			printf("%s", USAGE);
			return false;
		}
		if (par == "-mult") {
			i++;
			if (i >= argc || 1 != sscanf(argv[i], "%f", &mult) || mult < 0)
				return error("expecting a float value >= 0 after -mult");
		} else if (par == "-size") {
			i++;
			if (i >= argc || 1 != sscanf(argv[i], "%d", &outSize) || outSize <= 0 || outSize > 32000)
				return error("expecting an integer value in [1..32000] after -size");
		} else if (par == "-fmt") {
			i++;
			if (i >= argc || !parseFmtSpecifier(argv[i]))
				return error("expecting a format specifier after -fmt");
		} else {
			if (inFile.empty())
				inFile = par;
			else if (outFile.empty())
				outFile = par;
			else {
				printf("Error: too much input files, or `%s' is a parameter that I can't handle\n", par.c_str());
			}
		}
	}
	if (inFile.empty())
		return error("no input files");
	return true;
}

void displayBitmap(const Bitmap& bmp, const char* msg = NULL)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Surface* screen = SDL_SetVideoMode(bmp.getWidth(), bmp.getHeight(), 32, 0);
	Uint8* buffer = (Uint8*) screen->pixels;
	
	if (msg) SDL_WM_SetCaption(msg, NULL);
	
	bool running = true;
	int cMult = 0;
	int lastMult = -99;

	while (running) {
		if (lastMult != cMult) {
			float m = pow(2.0f, cMult / 6.0f);
			printf("Multiplier = %f, total multiplier = %f\n", m, m * mult);
			for (int y = 0; y < bmp.getHeight(); y++) {
				Uint32* row = (Uint32*) (buffer + (y * screen->pitch));
				for (int x = 0; x < bmp.getWidth(); x++) {
					row[x] = (bmp.getPixel(x, y) * m).toRGB32();
				}
			}
			SDL_Flip(screen);
			lastMult = cMult;
		}
		SDL_Event ev;
		SDL_WaitEvent(&ev);
		
		switch (ev.type) {
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
			{
				switch (ev.key.keysym.sym) {
					case SDLK_ESCAPE:
						running = false;
						break;
						
					case SDLK_PLUS:
					case SDLK_KP_PLUS:
						cMult++;
						break;
						
					case SDLK_MINUS:
					case SDLK_KP_MINUS:
						cMult--;
						break;
						
					default:
						break;
				}
				break;
			}
			default:
				break;
		}
	}
	SDL_Quit();
}

int main(int argc, char** argv)
{
	if (!parseCmdLine(argc, argv)) {
		printf("%s", USAGE);
		return 1;
	}
	Environment env;
	if (!env.load(inFile.c_str(), inFmt)) {
		printf("Cannot load input environment `%s'\n", inFile.c_str());
		return 2;
	}
	
	if (mult != 1) env.multiply(mult);
	env.convert(outFmt, outSize);
	
	if (outFile != "") {
		env.save(outFile.c_str());
	} else {
		if (outFmt != SPHERICAL) {
			printf("Error: displaying an image cannot be done in a format other than spherical!\n");
			return 3;
		}
		Bitmap& img = env.getMap(0);
		int oldW = img.getWidth(), oldH = img.getHeight();
		// rescale to fit screen:
		img.rescale(outSize > 0 ? outSize : 1024);
		// display on screen:
		char msg[500];
		if (img.getWidth() != oldW)
			sprintf(msg, "%s: %dx%d pixels (this preview: %dx%d pixels)", inFile.c_str(), oldW, oldH, img.getWidth(), img.getHeight());
		else 
			sprintf(msg, "%s: %dx%d pixels", inFile.c_str(), oldW, oldH);
		displayBitmap(img, msg);
	}
	return 0;
}
