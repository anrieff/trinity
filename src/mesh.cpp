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
#include <algorithm>
#include "mesh.h"
#include "constants.h"
#include "color.h"
using std::max;

Mesh::Mesh(double height, bool tetraeder)
{
	this->height = height;
	this->tetraeder = tetraeder;
	faceted = false;
	initMesh();
	boundingSphere = NULL;
}

void Mesh::initMesh(void)
{
	if (tetraeder) generateTetraeder(); // if 'algorithm' is 1, generate a tetraeder; otherwise, a soccer ball.
	else generateTruncatedIcosahedron();
	
	Vector center(0, 0, 0);
	double maxDist = 0;
	for (size_t i = 0; i < vertices.size(); i++)
		maxDist = max(maxDist, vertices[i].length());
	boundingSphere = new Sphere(center, maxDist);
}

Mesh::~Mesh()
{
	if (boundingSphere)
		delete boundingSphere;
}

const char* Mesh::getName()
{
	static char temp[200];
	sprintf(temp, "Mesh with %d vertices, %d triangles\n", (int) vertices.size(), (int) triangles.size());
	return temp;
}

void Mesh::generateTetraeder(void)
{
	double hBase = 0;
	double hTop = height;
	Vector center(0, height * 0.25, 0);
	for (int i = 0; i < 3; i++) {
		double angle = i / 3.0 * 2 * PI;
		vertices.push_back(Vector(cos(angle), hBase, sin(angle)));
	}
	vertices.push_back(Vector(0, hTop, 0));
	for (int i = 0; i < 4; i++) {
		Vector t = vertices[i] - center;
		t.normalize();
		normals.push_back(t);
		uvs.push_back(Vector((PI + atan2(t.z, t.x)) / PI, 1.0 - (PI/2 + asin(t.y)) / PI, 0));
	}
	for (int i = 0; i < 4; i++) {
		Triangle t;
		int k = 0;
		for (int j = 0; j < 4; j++) if (j != i) {
			t.v[k] = j;
			t.n[k] = j;
			t.t[k] = j;
			k++;
		}
		Vector a = vertices[t.v[1]] - vertices[t.v[0]];
		Vector b = vertices[t.v[2]] - vertices[t.v[0]];
		t.gnormal = a ^ b;
		t.gnormal.normalize();
		triangles.push_back(t);
	}
}

void Mesh::generateTruncatedIcosahedron(void)
{
	FILE* f = fopen("data/truncatedIcosahedron.txt", "rt");
	if (!f) return;
	Vector a, b, c;
	Color z;
	int i = 0;
	Vector center(0, 0, 0);
	while (12 == fscanf(f, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%f%f%f", &a.x, &a.y, &a.z, &b.x, &b.y, &b.z,
	                    &c.x, &c.y, &c.z, &z.r, &z.g, &z.b)) {
		vertices.push_back(a);
		vertices.push_back(b);
		vertices.push_back(c);
		center += a;
		center += b;
		center += c;
		Triangle T;
		for (int j = 0; j < 3; j++) {
			T.v[j] = T.n[j] = T.t[j] = 3 * i + j;
		}
		if (z.intensity() < 0.5f) {
			uvs.push_back(Vector(-0.1, 0.1, 0));
			uvs.push_back(Vector(-0.1, 0.1, 0));
			uvs.push_back(Vector(-0.1, 0.1, 0));
		} else {
			uvs.push_back(Vector(0.1, 0.1, 0));
			uvs.push_back(Vector(0.1, 0.1, 0));
			uvs.push_back(Vector(0.1, 0.1, 0));
		}
		T.gnormal = (b - a) ^ (c - a);
		T.gnormal.normalize();
		triangles.push_back(T);
		i++;
	}
	fclose(f);
	center /= (3*i);
	for (int i = 0; i < (int) vertices.size(); i++) {
		vertices[i] = vertices[i] - center;
		vertices[i] *= 0.005;
		Vector t = vertices[i];
		t.normalize();
		normals.push_back(t);
	}
}

bool Mesh::intersectTriangle(const Ray& ray, IntersectionData& data, Triangle& T)
{
	bool inSameDirection = (dot(ray.dir, T.gnormal) > 0);
	if (inSameDirection) return false; // backface culling
	//              B                     A
	Vector AB = vertices[T.v[1]] - vertices[T.v[0]];
	Vector AC = vertices[T.v[2]] - vertices[T.v[0]];
	Vector D = -ray.dir;
	//              0               A
	Vector H = ray.start - vertices[T.v[0]];

	/* 2. Solve the equation:
	 *
	 * A + lambda2 * AB + lambda3 * AC = ray.start + gamma * ray.dir
	 *
	 * which can be rearranged as:
	 * lambda2 * AB + lambda3 * AC + gamma * D = ray.start - A
	 *
	 * Which is a linear system of three rows and three unknowns, which we solve using Carmer's rule
	 */

	// Find the determinant of the left part of the equation:
	double Dcr = (AB ^ AC) * D;
	
	// are the ray and triangle parallel?
	if (fabs(Dcr) < 1e-12) return false;
	
	double lambda2 = ( ( H ^ AC) * D ) / Dcr;
	double lambda3 = ( (AB ^  H) * D ) / Dcr;
	double gamma   = ( (AB ^ AC) * H ) / Dcr;

	// is intersection behind us, or too far?
	if (gamma < 0 || gamma > data.dist) return false;
	
	// is the intersection outside the triangle?
	if (lambda2 < 0 || lambda2 > 1 || lambda3 < 0 || lambda3 > 1 || lambda2 + lambda3 > 1)
		return false;
	//
	
	// intersection found, and it's closer to the current one in data.
	// store intersection point.
	data.p = ray.start + ray.dir * gamma;
	data.dist = gamma;
	data.g = this;
	
	double lambda1 = 1 - lambda2 - lambda3;
	if (faceted) {
		data.normal = T.gnormal;
	} else {
		// interpolate normals using the barycentric coords:
		data.normal = normals[T.n[0]] * lambda1 +
					  normals[T.n[1]] * lambda2 +
					  normals[T.n[2]] * lambda3;
		data.normal.normalize();
	}
	
	// interpolate the UV texture coords using barycentric coords:
	Vector uv = uvs[T.t[0]] * lambda1 +
				uvs[T.t[1]] * lambda2 +
				uvs[T.t[2]] * lambda3;
	data.u = uv.x;
	data.v = uv.y;
	return true;
}

bool Mesh::intersect(Ray ray, IntersectionData& data)
{
	bool found = false;
	IntersectionData temp = data;
	// if the ray doesn't intersect the bounding shpere, it is of no use
	// to continue: it can't possibly intersect the mesh.
	if (!boundingSphere->intersect(ray, temp)) return false;
	
	// naive algorithm - iterate and check for intersection all triangles:
	for (size_t i = 0; i < triangles.size(); i++) {
		if (intersectTriangle(ray, data, triangles[i]))
			found = true;
	}
	
	return found;
}
