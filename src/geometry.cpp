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
	if (ray.dir.y >= 0) return false;
	else {
		double yDiff = ray.dir.y;
		double wantYDiff = ray.start.y - this->y;
		double mult = wantYDiff / -yDiff;
		if (mult > data.dist) return false;
		data.p = ray.start + ray.dir * mult;
		data.dist = mult;
		data.normal = Vector(0, 1, 0);
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
	
	if (sol > info.dist) return false;
	
	info.dist = sol;
	info.p = ray.start + ray.dir * sol;
	info.normal = info.p - center; // generate the normal by getting the direction from the center to the ip
	info.normal.normalize();
	info.u = (PI + atan2(info.p.z - center.z, info.p.x - center.x))/(2*PI);
	info.v = 1.0 - (PI/2 + asin((info.p.y - center.y)/R)) / PI;
	info.g = this;
	return true;
}

static bool intersectCubeSide(const Ray& ray, const Vector& center, double sideLength, IntersectionData& data)
{
	if (fabs(ray.dir.y) < 1e-9) return false;

	double halfSide = sideLength * 0.5;
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
		data.u = data.p.x - center.x;
		data.v = data.p.z - center.z;
		found = true;	
	}
	return found;
}

bool Cube::intersect(Ray ray, IntersectionData& data)
{
	bool found = intersectCubeSide(ray, center, side, data);
	if (intersectCubeSide(project(ray, 1, 0, 2), project(center, 1, 0, 2), side, data)) {
		found = true;
		data.normal = unproject(data.normal, 1, 0, 2);
		data.p = unproject(data.p, 1, 0, 2);
	}
	if (intersectCubeSide(project(ray, 0, 2, 1), project(center, 0, 2, 1), side, data)) {
		found = true;
		data.normal = unproject(data.normal, 0, 2, 1);
		data.p = unproject(data.p, 0, 2, 1);
	}
	if (found) data.g = this;
	return found;
}

static void findAllIntersections(Geometry* geom, Ray ray, vector<IntersectionData>& l)
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
	
	all = L;
	for (int i = 0; i < (int) R.size(); i++)
		all.push_back(R[i]);
	
	std::sort(all.begin(), all.end(),
		[] (const IntersectionData& a, const IntersectionData& b) { return a.dist < b.dist; });
	
	bool inL, inR;
	inL = L.size() % 2 == 1;
	inR = R.size() % 2 == 1;
	
	for (int i = 0; i < (int) all.size(); i++) {
		IntersectionData& current = all[i];
		
		if (current.g == left)
			inL = !inL;
		else
			inR = !inR;
		
		if (boolOp(inL, inR)) {
			if (current.dist > data.dist) return false;
			data = current;
			return true;
		}
	}
	return false;
}
