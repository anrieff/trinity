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
#ifndef __HEIGHTFIELD_H__
#define __HEIGHTFIELD_H__

#include "geometry.h"
#include "bbox.h"

class Heightfield: public Geometry {
	float* heights;
	float* maxH;
	Vector* normals;
	BBox bbox;
	bool useOptimization;
	int W, H;
	float getHeight(int x, int y) const;
	Vector getNormal(float x, float y) const;
	// this sturcture will hold the height of the highest peak around a single position,
	// within a radius of 1 texels, 2 texels, 4 texels, ... 2^k texels.
	// h[k] holds the highest peak within (1<<k) texels
	struct HighStruct {
		float h[16];
	};
	HighStruct* hsmap; //!< the actual storage for the accelerated structure
	int maxK;          //!< max level stored in the hsmap
	
	void buildStruct(void); //!< build the accelerated structure
	float getHighest(int x, int y, int k) const; //!< Gets the highest nearby peak around position (x, y) on the heightmap with distance no more than 2^k

public:
	Heightfield() { heights = NULL; maxH = NULL; normals = NULL; useOptimization = false; }
	~Heightfield();
	bool intersect(Ray ray, IntersectionData& info);
	bool isInside(const Vector& p ) const { return false; }
	void fillProperties(ParsedBlock& pb);
	const char* getName() { return "Heightfield"; }
};

#endif // __HEIGHTFIELD_H__

