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

#include <vector>
#include "vector.h"
#include "scene.h"
#include "transform.h"

/// a structure, that holds info about an intersection. Filled in by Geometry::intersect() methods
class Geometry;
struct IntersectionData {
	Vector p; //!< intersection point in the world-space
	Vector normal; //!< the normal of the geometry at the intersection point
	Vector dNdx, dNdy; 
	double dist; //!< before intersect(): the max dist to look for intersection; after intersect() - the distance found
	
	double u, v; //!< 2D UV coordinates for texturing, etc.
	
	Geometry* g; //!< The geometry which was hit
};

/// An abstract class that represents any intersectable primitive in the scene.
class Intersectable {
public:
	virtual ~Intersectable() {}
	/**
	 *  @brief Intersect a geometry with a ray.
	 *  Returns true if an intersection is found, and it was closer than the current value of data.dist.
	 * 
	 *  @param ray - the ray to be traced
	 *  @param data - in the event an intersection is found, this is filled with info about the intersection point.
	 *  NOTE: the intersect() function MUST NOT touch any member of data, before it can prove the intersection
	 *        point will be closer to the current value of data.dist!
	 *  Note that this also means that if you want to find any intersection, you must initialize data.dist before
	 *  calling intersect. E.g., if you don't care about distance to intersection, initialize data.dist with 1e99
	 *
	 * @retval true if an intersection is found. The `data' struct should be filled in.
	 * @retval false if no intersection exists, or it is further than the current data.dist. In this case,
	 *         the `data' struct should remain unchanged.
	 */
	virtual bool intersect(Ray ray, IntersectionData& info) = 0;
	/// Checks if the given point is "inside" the geometry, for whatever definition of
	/// inside is appropriate for the object. Returns a boolean value accordingly.
	virtual bool isInside(const Vector& p) const = 0;
};


/// An abstract class, that describes a geometry in the scene.
class Geometry: public SceneElement, public Intersectable {
public:
	virtual ~Geometry() {}

	virtual const char* getName() = 0; //!< a virtual function, which returns the name of a geometry

	// from Intersectable:
	virtual bool intersect(Ray ray, IntersectionData& data) = 0;
	virtual bool isInside(const Vector& p) const = 0;
	
	// from SceneElement:
	ElementType getElementType() const { return ELEM_GEOMETRY; }
};

class Plane: public Geometry {
	double y; //!< y-intercept. The plane is parallel to XZ, the y-intercept is at this value
	double limit;
public:
	Plane(double _y = 0, double _limit = 1e99) { y = _y; limit = _limit; }

	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("y", &y);
		pb.getDoubleProp("limit", &limit);
	}
	bool intersect(Ray ray, IntersectionData& data);
	const char* getName() { return "Plane"; }
	bool isInside(const Vector& p) const { return false; }
};

class Sphere: public Geometry {
	Vector center;
	double R;
public:
	Sphere(const Vector& center = Vector(0, 0, 0), double R = 1): center(center), R(R) {}
	
	void fillProperties(ParsedBlock& pb)
	{
		pb.getVectorProp("center", &center);
		pb.getDoubleProp("R", &R);
	}

	bool intersect(Ray ray, IntersectionData& data);
	const char* getName() { return "Sphere"; }
	bool isInside(const Vector& p) const { return (center - p).lengthSqr() < R*R; }
};

class Cube: public Geometry {
	Vector center;
	double side;
	inline bool intersectCubeSide(const Ray& ray, const Vector& center, IntersectionData& data);
public:
	Cube(const Vector& center = Vector(0, 0, 0), double side = 1): center(center), side(side) {}

	void fillProperties(ParsedBlock& pb)
	{
		pb.getVectorProp("center", &center);
		pb.getDoubleProp("side", &side);
	}

	bool intersect(Ray ray, IntersectionData& data);	
	const char* getName() { return "Cube"; }
	bool isInside(const Vector& p) const { 
		return (fabs(p.x - center.x) <= side * 0.5 &&
				fabs(p.y - center.y) <= side * 0.5 &&
				fabs(p.z - center.z) <= side * 0.5);
	}
};

class CsgOp: public Geometry {
protected:
	Geometry *left, *right;
	void findAllIntersections(Geometry* geom, Ray ray, std::vector<IntersectionData>& l);
public:
	CsgOp(Geometry* left, Geometry* right) : left(left), right(right) {}

	void fillProperties(ParsedBlock& pb)
	{
		pb.requiredProp("left");
		pb.requiredProp("right");
		pb.getGeometryProp("left", &left);
		pb.getGeometryProp("right", &right);
	}
	
	bool intersect(Ray ray, IntersectionData& data);	
	
	virtual bool boolOp(bool inLeft, bool inRight) const = 0;
	bool isInside(const Vector& p) const { return boolOp(left->isInside(p), right->isInside(p)); }
};

class CsgUnion: public CsgOp {
public:
	CsgUnion(Geometry* left = NULL, Geometry* right = NULL): CsgOp(left, right) {}
	
	bool boolOp(bool inLeft, bool inRight) const { return inLeft || inRight; }
	const char* getName() { return "CsgUnion"; }
};

class CsgDiff: public CsgOp {
public:
	CsgDiff(Geometry* left = NULL, Geometry* right = NULL): CsgOp(left, right) {}

	bool intersect(Ray ray, IntersectionData& data); // override the generic intersector to handle a corner case
	
	bool boolOp(bool inLeft, bool inRight) const { return inLeft && !inRight; }
	const char* getName() { return "CsgDiff"; }
};

class CsgInter: public CsgOp {
public:
	CsgInter(Geometry* left = NULL, Geometry* right = NULL): CsgOp(left, right) {}
	
	bool boolOp(bool inLeft, bool inRight) const { return inLeft && inRight; }
	const char* getName() { return "CsgInter"; }
};

class Shader;
class BumpTexture;

/// A Node, which holds a geometry, linked to a shader.
/// it is also allowed to use any Intersectable in place of the geometry - even another node.
class Node: public SceneElement, public Intersectable {
public:
	Geometry* geom;
	Shader* shader;
	Transform transform;
	Texture* bump;
	
	Node() { bump = NULL; }
	Node(Geometry* g, Shader* s) { geom = g; shader = s; bump = NULL; }
	
	// from Intersectable:
	bool intersect(Ray ray, IntersectionData& data);
	bool isInside(const Vector& p) const { return geom->isInside(transform.undoPoint(p)); }

	// from SceneElement:
	ElementType getElementType() const { return ELEM_NODE; }
	void fillProperties(ParsedBlock& pb)
	{
		pb.getGeometryProp("geometry", &geom);
		pb.getShaderProp("shader", &shader);
		pb.getTransformProp(transform);
		pb.getTextureProp("bump", &bump);
	}
};

#endif // __GEOMETRY_H__
