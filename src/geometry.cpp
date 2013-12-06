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

#include "geometry.h"
#include "constants.h"
#include "vector.h"
#include <vector>
#include <algorithm>
using std::vector;


bool Plane::intersect(Ray ray, IntersectionData& data)
{
	// intersect a ray with a XZ plane:
	// if the ray is pointing to the horizon, or "up", but the plane is below us,
	// of if the ray is pointing down, and the plane is above us, we have no intersection
	if ((ray.start.y > y && ray.dir.y > -1e-9) || (ray.start.y < y && ray.dir.y < 1e-9))
		return false;
	else {
		double yDiff = ray.dir.y;
		double wantYDiff = ray.start.y - this->y;
		double mult = wantYDiff / -yDiff;
		
		// if the distance to the intersection (mult) doesn't optimize our current distance, bail out:
		if (mult > data.dist) return false;
		
		Vector p = ray.start + ray.dir * mult;
		if (fabs(p.x) > limit || fabs(p.z) > limit) return false;
		
		// calculate intersection:
		data.p = p;
		data.dist = mult;
		data.normal = Vector(0, 1, 0);
		data.dNdx = Vector(1, 0, 0);
		data.dNdy = Vector(0, 0, 1);
		data.u = data.p.x;
		data.v = data.p.z;
		data.g = this;
		return true;
	}
}

bool Sphere::intersect(Ray ray, IntersectionData& info)
{
	// compute the sphere intersection using a quadratic equation:
	Vector H = ray.start - center;
	double A = ray.dir.lengthSqr();
	double B = 2 * dot(H, ray.dir);
	double C = H.lengthSqr() - R*R;
	double Dscr = B*B - 4*A*C;
	if (Dscr < 0) return false; // no solutions to the quadratic equation - then we don't have an intersection.
	double x1, x2;
	x1 = (-B + sqrt(Dscr)) / (2*A);
	x2 = (-B - sqrt(Dscr)) / (2*A);
	double sol = x2; // get the closer of the two solutions...
	if (sol < 0) sol = x1; // ... but if it's behind us, opt for the other one
	if (sol < 0) return false; // ... still behind? Then the whole sphere is behind us - no intersection.
	
	// if the distance to the intersection doesn't optimize our current distance, bail out:
	if (sol > info.dist) return false;
	
	info.dist = sol;
	info.p = ray.start + ray.dir * sol;
	info.normal = info.p - center; // generate the normal by getting the direction from the center to the ip
	info.normal.normalize();
	double angle = atan2(info.p.z - center.z, info.p.x - center.x);
	info.u = (PI + angle)/(2*PI);
	info.v = 1.0 - (PI/2 + asin((info.p.y - center.y)/R)) / PI;
	info.dNdx = Vector(cos(angle + PI/2), 0, sin(angle + PI/2));
	info.dNdy = info.dNdx ^ info.normal;
	info.g = this;
	return true;
}

inline bool Cube::intersectCubeSide(const Ray& ray, const Vector& center, IntersectionData& data)
{
	if (fabs(ray.dir.y) < 1e-9) return false;

	double halfSide = this->side * 0.5;
	bool found = false;
	for (int side = -1; side <= 1; side += 2) {
		double yDiff = ray.dir.y;
		double wantYDiff = ray.start.y - (center.y + side * halfSide);
		double mult = wantYDiff / -yDiff;
		if (mult < 0) continue;
		if (mult > data.dist) continue;
		Vector p = ray.start + ray.dir * mult;
		if (p.x < center.x - halfSide ||
			p.x > center.x + halfSide ||
			p.z < center.z - halfSide ||
			p.z > center.z + halfSide) continue;
		data.p = ray.start + ray.dir * mult;
		data.dist = mult;
		data.normal = Vector(0, side, 0);
		data.dNdx = Vector(1, 0, 0);
		data.dNdy = Vector(0, 0, side);
		data.u = data.p.x - center.x;
		data.v = data.p.z - center.z;
		found = true;	
	}
	return found;
}

bool Cube::intersect(Ray ray, IntersectionData& data)
{
	// check for intersection with the negative Y and positive Y sides
	bool found = intersectCubeSide(ray, center, data);
	
	// check for intersection with the negative X and positive X sides
	if (intersectCubeSide(project(ray, 1, 0, 2), project(center, 1, 0, 2), data)) {
		found = true;
		data.normal = unproject(data.normal, 1, 0, 2);
		data.p = unproject(data.p, 1, 0, 2);
	}

	// check for intersection with the negative Z and positive Z sides
	if (intersectCubeSide(project(ray, 0, 2, 1), project(center, 0, 2, 1), data)) {
		found = true;
		data.normal = unproject(data.normal, 0, 2, 1);
		data.p = unproject(data.p, 0, 2, 1);
	}
	if (found) data.g = this;
	return found;
}

// find all intersections of a ray with a geometry, storing the intersection points in the vector `l'
void CsgOp::findAllIntersections(Geometry* geom, Ray ray, vector<IntersectionData>& l)
{
	double currentLength = 0;
	while (1) {
		IntersectionData temp;
		temp.dist = 1e99;
		//
		if (!geom->intersect(ray, temp)) break;
		//
		temp.dist += currentLength;
		currentLength = temp.dist;
		l.push_back(temp);
		ray.start = temp.p + ray.dir * 1e-6;
	}
}

bool CsgOp::intersect(Ray ray, IntersectionData& data)
{
	vector<IntersectionData> L, R, all;
	
	findAllIntersections(left, ray, L);
	findAllIntersections(right, ray, R);
	
	// concatenate L an R, and sort them by distance along the ray:
	all = L;
	for (int i = 0; i < (int) R.size(); i++)
		all.push_back(R[i]);
	
	std::sort(all.begin(), all.end(),
		[] (const IntersectionData& a, const IntersectionData& b) { return a.dist < b.dist; });
	
	// if we have an even number of intersections -> we're outside the object. If odd, we're inside:
	bool inL, inR;
	inL = L.size() % 2 == 1;
	inR = R.size() % 2 == 1;
	
	for (int i = 0; i < (int) all.size(); i++) {
		IntersectionData& current = all[i];
		
		// at each intersection, we flip the `insidedness' of one of the two variables:
		if (current.g == left)
			inL = !inL;
		else
			inR = !inR;
		
		// if we entered the CSG just now, and this optimizes the current data.dist ->
		// then we've found the intersection.
		if (boolOp(inL, inR)) {
			if (current.dist > data.dist) return false;
			data = current;
			return true;
		}
	}
	return false;
}

bool CsgDiff::intersect(Ray ray, IntersectionData& data)
{
	if (!CsgOp::intersect(ray, data)) return false;
	/*
	 * Consider the following CsgDiff: a larger sphere with a smaller sphere somewhere on its side
	 * The result is the larger sphere, with some "eaten out" part. The question is:
	 * Where should the normals point, in the surface of the "eaten out" parts?
	 * These normals are generated by the smaller sphere, and point to the inside of the interior of
	 * the larger. They are obviously wrong.
	 *
	 * Solution: when we detect a situation like this, we flip the normals.
	 */
	 if (right->isInside(data.p - ray.dir * 1e-6) != right->isInside(data.p + ray.dir * 1e-6))
		data.normal = -data.normal;
	 return true;
}

// intersect a ray with a node, considering the Model transform attached to the node.
bool Node::intersect(Ray ray, IntersectionData& data)
{
	// world space -> object's canonic space
	ray.start = transform.undoPoint(ray.start);
	ray.dir = transform.undoDirection(ray.dir);
	
	// save the old "best dist", in case we need to restore it later
	double oldDist = data.dist; // *(1)
	double rayDirLength = ray.dir.length();
	data.dist *= rayDirLength;  // (2)
	ray.dir.normalize();        // (3)
	if (!geom->intersect(ray, data)) {
		data.dist = oldDist;    // (4)
		return false;
	}
	// The intersection found is in object space, convert to world space:
	data.normal = normalize(transform.normal(data.normal));
	data.dNdx = normalize(transform.direction(data.dNdx));
	data.dNdy = normalize(transform.direction(data.dNdy));
	data.p = transform.point(data.p);
	data.dist /= rayDirLength;  // (5)
	return true;
	
	/*
	 * ^^^
	 * Explanation for the numbered lines.
	 *
	 * Since the geometries in the scene may use different transforms, the only universal
	 * coordinate system are the world coords. The applies to distances, too. We use data.dist
	 * for culling geometries that won't improve the closest dist we've found so far in raytrace().
	 * So, if a transform contains scaling, we need to adjust data.dist so that it's still valid
	 * in the object's canonic space. Since ray.dir undergoes scaling as well, we use its length
	 * to multiply the data.dist in (2). This essentially transforms the distance from world space
	 * to object's canonic space. We save the old value of the distance in (1), in case there isn't
	 * an intersection (4). Of course, Geometry::intersect assumes that ray.dir is an unit vector,
	 * and we cater for that in (3). Finally, when the results arrive, convert the data.dist back
	 * from object to world space (5).
	 *
	 * This implements the "bonus" from HW5/medium/group1.
	 */
}

