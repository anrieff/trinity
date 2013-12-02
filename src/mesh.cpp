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
#include <string>
#include <vector>
#include "mesh.h"
#include "constants.h"
#include "color.h"
using std::max;
using std::string;
using std::vector;

void Mesh::initMesh(void)
{
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

bool Mesh::intersectTriangle(const Ray& ray, IntersectionData& data, Triangle& T)
{
	bool inSameDirection = (dot(ray.dir, T.gnormal) > 0);
	if (backfaceCulling && inSameDirection) return false; // backface culling
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
	if (faceted || !hasNormals) {
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
	data.dNdx = T.dNdx;
	data.dNdy = T.dNdy;
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

static double getDouble(const string& s)
{
	double res;
	if (s == "") return 0;
	sscanf(s.c_str(), "%lf", &res);
	return res;
}

static int getInt(const string& s)
{
	int res;
	if (s == "") return 0;
	sscanf(s.c_str(), "%d", &res);
	return res;
}

Triangle::Triangle(std::string a, std::string b, std::string c)
{
	string items[3] = { a, b, c };
	
	for (int i = 0; i < 3; i++) {
		const string& item = items[i];
		
		vector<string> subItems = split(item, '/');
		v[i] = getInt(subItems[0]);
		if (subItems.size() > 1) {
			t[i] = getInt(subItems[1]);
		} else t[i] = 0;
		if (subItems.size() > 2) {
			n[i] = getInt(subItems[2]);
		} else n[i] = 0;
	}
}

void solve2D(double M[2][2], double H[2], double& p, double& q)
{
	// (p, q) * (M) = (H)
	
	double Dcr = M[0][0] * M[1][1] - M[1][0] * M[0][1];
	
	double rDcr = 1 / Dcr;
	
	p = (H[0] * M[1][1] - H[1] * M[0][1]) * rDcr;
	q = (M[0][0] * H[1] - M[1][0] * H[0]) * rDcr;
}



bool Mesh::loadFromOBJ(const char* filename)
{
	FILE* f = fopen(filename, "rt");
	
	if (!f) {
		printf("error: no such file: %s", filename);
		return false;
	}
	
	vertices.push_back(Vector(0, 0, 0));
	uvs.push_back(Vector(0, 0, 0));
	normals.push_back(Vector(0, 0, 0));
	hasNormals = false;
	
	
	char line[2048];
	
	while (fgets(line, sizeof(line), f)) {
		if (line[0] == '#') continue;
		
		vector<string> tokens = tokenize(string(line));
		
		if (tokens.empty()) continue;
		
		if (tokens[0] == "v") {
			Vector t(getDouble(tokens[1]),
			         getDouble(tokens[2]),
			         getDouble(tokens[3]));
			vertices.push_back(t);
			continue;
		}

		if (tokens[0] == "vn") {
			hasNormals = true;
			Vector t(getDouble(tokens[1]),
			         getDouble(tokens[2]),
			         getDouble(tokens[3]));
			normals.push_back(t);
			continue;
		}

		if (tokens[0] == "vt") {
			Vector t(getDouble(tokens[1]),
			         getDouble(tokens[2]),
			         0);
			uvs.push_back(t);
			continue;
		}
		
		if (tokens[0] == "f") {
			int n = tokens.size() - 3;
			
			for (int i = 0; i < n; i++) {
				Triangle T(tokens[1], tokens[2 + i], tokens[3 + i]);
				triangles.push_back(T);
			}
		}
	}
	
	for (int i = 0; i < (int) triangles.size(); i++) {
		Triangle& T = triangles[i];
		
		Vector AB = vertices[T.v[1]] - vertices[T.v[0]];
		Vector AC = vertices[T.v[2]] - vertices[T.v[0]];
		
		T.gnormal = AB ^ AC;
		T.gnormal.normalize();
		
		double px, py, qx, qy;
		
		Vector AB_2d = uvs[T.t[1]] - uvs[T.t[0]];
		Vector AC_2d = uvs[T.t[2]] - uvs[T.t[0]];
		
		double mat[2][2] = {
			{ AB_2d.x, AC_2d.x },
			{ AB_2d.y, AC_2d.y },
		};
		double h[2] = { 1, 0 };
		
		solve2D(mat, h, px, qx); // (AB_2d * px + AC_2d * qx == (1, 0)
		h[0] = 0; h[1] = 1;
		solve2D(mat, h, py, qy); // (AB_2d * py + AC_2d * qy == (0, 1)
		
		T.dNdx = AB * px + AC * qx;
		T.dNdx.normalize();
		T.dNdy = AB * py + AC * qy;
		T.dNdy.normalize();
	}
	
	fclose(f);
	return true;
}

