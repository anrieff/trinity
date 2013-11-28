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
#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include "color.h"
#include "vector.h"
#include "scene.h"

enum CubeOrder {
	NEGX,
	NEGY,
	NEGZ,
	POSX,
	POSY,
	POSZ,
};

class Environment: public SceneElement {
public:
	virtual ~Environment() {}
	/// gets a color from the environment at the specified direction
	virtual Color getEnvironment(const Vector& dir) = 0;
	
	ElementType getElementType() const { return ELEM_ENVIRONMENT; }
};

class Bitmap;
class CubemapEnvironment: public Environment {
	Bitmap* maps[6];
	bool owned;
	
	Color getSide(const Bitmap& bmp, double x, double y);
	bool loadMaps(const char* folder);
public:
	CubemapEnvironment() { owned = true; } // default constructor in which the loading of textures is done later.
 	/// loads a cubemap from 6 separate images, from the specified folder.
 	/// The images have to be named "posx.bmp", "negx.bmp", "posy.bmp", ...
 	/// (or they may be .exr images, not .bmp).
 	/// The folder specification shouldn't include a trailing slash; 
 	/// e.g. "/images/cubemaps/cathedral" is OK.
	CubemapEnvironment(const char* folder);
	
	/// initialize from 6 bitmaps. In this case, the class won't take "ownership" of the
	/// 6 Bitmap objects; they won't be freed in ~CubemapEnvironment.
	CubemapEnvironment(Bitmap** maps);
	~CubemapEnvironment();
	Color getEnvironment(const Vector& dir);
	
	void fillProperties(ParsedBlock& pb)
	{
		Environment::fillProperties(pb);
		char folder[256];
		if (!pb.getFilenameProp("folder", folder)) pb.requiredProp("folder");
		if (!loadMaps(folder)) {
			fprintf(stderr, "CubemapEnvironment: Could not load maps from `%s'\n", folder);
		}
	}

};

#endif // __ENVIRONMENT_H__
