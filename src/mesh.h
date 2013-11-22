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
#ifndef __MESH_H__
#define __MESH_H__

#include <vector>
#include "vector.h"
#include "geometry.h"

/// A structure to represent a single triangle in the mesh
struct Triangle {
	int v[3]; //!< holds indices to the three vertices of the triangle (indexes in the `vertices' array in the Mesh)
	int n[3]; //!< holds indices to the three normals of the triangle (indexes in the `normals' array)
	int t[3]; //!< holds indices to the three texture coordinates of the triangle (indexes in the `uvs' array)
	Vector gnormal; //!< The geometric normal of the mesh (AB ^ AC, normalized)
};

class Mesh: public Geometry {
	std::vector<Vector> vertices; //!< An array with all vertices in the mesh
	std::vector<Vector> normals; //!< An array with all normals in the mesh
	std::vector<Vector> uvs; //!< An array with all texture coordinates in the mesh
	std::vector<Triangle> triangles; //!< An array that holds all triangles
	void generateTruncatedIcosahedron(void);
	void generateTetraeder(void);
	
	double height, scaling;
	bool isSmooth;   //!< This flags controls whether we want interpolated normals (i.e. a smooth mesh)
public:
	Mesh(double height = 1, double scaling = 1, bool tetraeder = true);
	~Mesh();
	const char* getName();
	bool intersect(Ray ray, IntersectionData& info);
	bool isInside(const Vector& p) const { return false; } //FIXME!!
};

#endif // __MESH_H__
