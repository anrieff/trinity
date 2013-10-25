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
#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "vector.h"

struct IntersectionData {
	Vector p;
	Vector normal;
	double dist;
	
	double u, v;
};


class Geometry {
public:
	virtual ~Geometry() {}
	
	virtual bool intersect(Ray ray, IntersectionData& data) = 0;
};

class Plane: public Geometry {
	double y;
public:
	Plane(double _y) { y = _y; }
	
	bool intersect(Ray ray, IntersectionData& data);
};


class Shader;

class Node {
public:
	Geometry* geom;
	Shader* shader;
	
	Node() {}
	Node(Geometry* g, Shader* s) { geom = g; shader = s; }
};

#endif // __GEOMETRY_H__
