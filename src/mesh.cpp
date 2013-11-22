/***************************************************************************
 *   Copyright (C) 2009 by Veselin Georgiev, Slavomir Kaslev et al         *
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
#include "mesh.h"
#include "constants.h"
#include "color.h"

Mesh::Mesh(double height, double scaling, bool tetraeder)
{
	this->height = height;
	isSmooth = false;
	this->scaling = scaling;
	if (tetraeder) generateTetraeder(); // if 'algorithm' is 1, generate a tetraeder; otherwise, a soccer ball.
	else generateTruncatedIcosahedron();
}

Mesh::~Mesh()
{
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
	for (int i = 0; i < (int) vertices.size(); i++)
		vertices[i] *= scaling;
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

bool Mesh::intersect(Ray ray, IntersectionData& data)
{
	bool found = false;
	for (int i = 0; i < (int) triangles.size(); i++) {
		// For each triangle
		Triangle& T = triangles[i];
		// 1. Fetch all vertices and form the sides AB and AC
		Vector A = vertices[T.v[0]];
		Vector B = vertices[T.v[1]];
		Vector C = vertices[T.v[2]];
		Vector AB = B - A;
		Vector AC = C - A;
		Vector H = ray.start - A;
		/* 2. Solve the equation:
		 *
		 * A + lambda2 * AB + lambda3 * AC = ray.start + gamma * ray.dir
		 *
		 * which can be rearranged as:
		 * lambda2 * AB + lambda3 * AC - gamma * ray.dir = ray.start - A
		 *
		 * Which is a linear system of three rows and three unknowns, which we solve using Carmer's rule
		 */
		// Find the determinant of the left part of the equation
		double Dcr = -((AB ^ AC) * ray.dir);
		// check for zero; if it is zero, then the triangle and the ray are parallel
		if (fabs(Dcr) < 1e-9) continue;
		// find the reciprocal of the determinant. We would use this quantity later in order
		// to multiply by rDcr instead of divide by Dcr (division is much slower)
		double rDcr = 1.0 / Dcr;
		// calculate `gamma' by substituting the right part of the equation in the third column of the matrix,
		// getting the determinant, and dividing by Dcr)
		double gamma = (AB ^ AC) * H * rDcr;
		
		// Is the intersection point behind us?
		if (gamma <= 0) continue;
		// Is the intersection point worse than what we currently have?
		if (gamma > data.dist) continue;
		// Calculate lambda2
		double lambda2 = -((H ^ AC) * ray.dir) * rDcr;
		// Check if it is in range (barycentric coordinates)
		if (lambda2 < 0 || lambda2 > 1) continue;
		// Calculate lambda3 and check if it is in range as well
		double lambda3 = -((AB ^ H) * ray.dir) * rDcr;
		if (lambda3 < 0 || lambda3 > 1 || lambda2 + lambda3 > 1) continue;
		// If we reached here, we have the currently best intersection at our hand - update minDist and info
		data.dist = gamma;
		data.p = ray.start + ray.dir * gamma;
		//
		data.g = this;
		data.dist = gamma;
		// Calculate the texture coordinates by substituting the barycentric coordinates lambda2 and lambda3 in the following formula:
		//
		// <interpolated> = <quantity_at_vertex_A> + (<quantity_at_vertex_B> - <quantity_at_vertex_A) * lambda2 +
		//                  (<quantity_at_vertex_C> - <quantity_at_vertex_A>) * lambda3
		// We want to interpolate the texture coordinates:
		Vector texCoords  =  uvs[T.t[0]] + (uvs[T.t[1]] - uvs[T.t[0]]) * lambda2 + (uvs[T.t[2]] - uvs[T.t[0]]) * lambda3;
		//   <interpolated>  <texcoordA>    <texcoordB>   <texcoordA>               <texcoordC>   <texcoordA>
		data.u = texCoords.x;
		data.v = texCoords.y;
		if (isSmooth) {
			// if we require normal smoothing, we use the same interpolation formula to get the interpolated normal
			Vector n = normals[T.n[0]] + (normals[T.n[1]] - normals[T.n[0]]) * lambda2 + (normals[T.n[2]] - normals[T.n[0]]) * lambda3;
			// .. and we don't forget to normalize it later
			n.normalize();
			data.normal = n;
		} else {
			// no smooth normals - just use the geometric normal of the triangle
			data.normal = T.gnormal;
		}
		found = true;
	}
	return found;
}
