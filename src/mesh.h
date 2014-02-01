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
#ifndef __MESH_H__
#define __MESH_H__

#include <vector>
#include "vector.h"
#include "geometry.h"
#include "bbox.h"

// A node of the K-d tree. It is either a in-node (if axis is AXIS_X, AXIS_Y, AXIS_Z),
// in which case the 'splitPos' holds the split position, and data.children is an array
// of two children.
// If axis is AXIS_NONE, then it is a leaf node, and data.triangles holds a list of
// indices to triangles.
struct KDTreeNode {
	Axis axis;
	double splitPos;
	union {
		std::vector<int>* triangles; // 1 pointer to list of triangle indices
		KDTreeNode* children;        // 1 pointer to TWO children (children[0] and children[1])
	};
	
	KDTreeNode() {}
	// initialize this node as a leaf node:
	void initLeaf(const std::vector<int>& triangleList)
	{
		axis = AXIS_NONE;
		splitPos = 0;
		triangles = new std::vector<int>(triangleList);
	}
	// initialize this node as a in-node (a binary node with two children)
	void initBinary(Axis axis, double splitPos)
	{
		this->axis = axis;
		this->splitPos = splitPos;
		children = new KDTreeNode[2];
	}
	~KDTreeNode()
	{
		if (axis == AXIS_NONE) {
			delete triangles;
		} else {
			delete [] children;
		}
	}
};

class Mesh: public Geometry {
	std::vector<Vector> vertices; //!< An array with all vertices in the mesh
	std::vector<Vector> normals; //!< An array with all normals in the mesh
	std::vector<Vector> uvs; //!< An array with all texture coordinates in the mesh
	std::vector<Triangle> triangles; //!< An array that holds all triangles
	
	// intersect a ray with a single triangle. Return true if an intersection exists, and it's
	// closer to the minimum distance, stored in data.dist
	bool intersectTriangle(const RRay& ray, IntersectionData& data, Triangle& T);
	void initMesh(void);
	
	bool faceted; //!< whether the normals interpolation is disabled or not
	bool backfaceCulling; //!< whether the backfaceCulling optimization is enabled (default: yes)
	bool hasNormals; //!< whether the .obj file contained normals. If not, no normal smoothing can be used.
	bool autoSmooth; //!< create smooth normals if the OBJ file lacks them
	BBox boundingBox; //!< a bounding box, which optimizes our whole
	
	bool loadFromOBJ(const char* filename); //!< load a mesh from an .OBJ file.
	bool useKDTree; //!< whether to use a KD-tree to speed-up intersections
	KDTreeNode* kdroot; //!< a pointer to the root of the KDTree. Can be NULL if no tree is built.
	
	void build(KDTreeNode& node, const BBox& bbox, const std::vector<int>& triangles, int depth);
	bool intersectKD(KDTreeNode& node, const BBox& bbox, const RRay& ray, IntersectionData& data);
public:
	Mesh() { faceted = false; backfaceCulling = true; useKDTree = true; autoSmooth = true; }
	~Mesh();
	const char* getName();
	bool intersect(const Ray& ray, IntersectionData& info);
	bool isInside(const Vector& p) const { return false; } //FIXME!!
	
	void setFaceted(bool faceted) { this->faceted = faceted; }
	
	void fillProperties(ParsedBlock& pb)
	{
		char fileName[256];
		if (pb.getFilenameProp("file", fileName))
			loadFromOBJ(fileName);
		else
			pb.requiredProp("file");
		pb.getBoolProp("faceted", &faceted);
		pb.getBoolProp("backfaceCulling", &backfaceCulling);
		pb.getBoolProp("useKDTree", &useKDTree);
		pb.getBoolProp("autoSmooth", &autoSmooth);
		initMesh();
	}
};

#endif // __MESH_H__
