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
		offset.makeZero();
	}

	void scale(double X, double Y, double Z) {
		Matrix scaling(X);
		scaling.m[1][1] = Y;
		scaling.m[2][2] = Z;

		transform = transform * scaling;
		inverseTransform = inverseMatrix(scaling) * inverseTransform;
	}

	void rotate(double yaw, double pitch, double roll) {
		transform = transform *
			rotationAroundX(toRadians(pitch)) *
			rotationAroundY(toRadians(yaw)) *
			rotationAroundZ(toRadians(roll));
		inverseTransform = inverseMatrix(transform);
	}

	void translate(const Vector& V) {
		offset = V;
	}

	Vector point(Vector P) {
		P = P * transform;
		P = P + offset;

		return P;
	}

	Vector undoPoint(Vector P) {
		P = P - offset;
		P = P * inverseTransform;

		return P;
	}

	Vector direction(const Vector& dir) {
		return dir * transform;
	}

	Vector undoDirection(const Vector& dir) {
		return dir * inverseTransform;
	}

private:
	Matrix transform;
	Matrix inverseTransform;
	Vector offset;
};

#endif // __TRANSFORM_H__

