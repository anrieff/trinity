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
#ifndef __TRANSFORM_H__
#define __TRANSFORM_H__

#include "vector.h"
#include "matrix.h"
#include "util.h"

/// A transformation class, which implements model-view transform. Objects can be
/// arbitrarily scaled, rotated and translated.
class Transform {
public:
	Transform() { reset(); }

	void reset() {
		transform = Matrix(1);
		inverseTransform = inverseMatrix(transform);
		transposedInverse = Matrix(1);
		offset.makeZero();
	}

	void scale(double X, double Y, double Z) {
		Matrix scaling(X);
		scaling.m[1][1] = Y;
		scaling.m[2][2] = Z;

		transform = transform * scaling;
		inverseTransform = inverseMatrix(transform);
		transposedInverse = transpose(inverseTransform);
	}

	void rotate(double yaw, double pitch, double roll) {
		transform = transform *
			rotationAroundX(toRadians(pitch)) *
			rotationAroundY(toRadians(yaw)) *
			rotationAroundZ(toRadians(roll));
		inverseTransform = inverseMatrix(transform);
		transposedInverse = transpose(inverseTransform);
	}

	void translate(const Vector& V) {
		offset = V;
	}

	Vector point(Vector P) const {
		P = P * transform;
		P = P + offset;

		return P;
	}

	Vector undoPoint(Vector P) const {
		P = P - offset;
		P = P * inverseTransform;

		return P;
	}

	Vector direction(const Vector& dir) const {
		return dir * transform;
	}

	Vector normal(const Vector& dir) const {
		return dir * transposedInverse;
	}

	Vector undoDirection(const Vector& dir) const {
		return dir * inverseTransform;
	}

	Ray ray(const Ray& inputRay) const {
		Ray result = inputRay;
		result.start = point(inputRay.start);
		result.dir   = direction(inputRay.dir);
		return result;
	}

	Ray undoRay(const Ray& inputRay) const {
		Ray result = inputRay;
		result.start = undoPoint(inputRay.start);
		result.dir   = undoDirection(inputRay.dir);
		return result;
	}

private:
	Matrix transform;
	Matrix inverseTransform;
	Matrix transposedInverse;
	Vector offset;
};

#endif // __TRANSFORM_H__

