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

#include "camera.h"
#include "matrix.h"
#include "util.h"
#include "sdl.h"
#include "random_generator.h"
#include "geometry.h"

bool dispersionOn = false;

void Camera::beginFrame(void)
{
	double x = -aspect;
	double y = +1;
	
	Vector corner = Vector(x, y, 1);
	Vector center = Vector(0, 0, 1);
	
	double lenXY = (corner - center).length();
	double wantedLength = tan(toRadians(fov / 2));
	
	double scaling = wantedLength / lenXY;
	
	x *= scaling;
	y *= scaling;
	
	
	this->upLeft = Vector(x, y, 1);
	this->upRight = Vector(-x, y, 1);
	this->downLeft = Vector(x, -y, 1);
	
	Matrix rotation = rotationAroundZ(toRadians(roll))
	                * rotationAroundX(toRadians(pitch))
	                * rotationAroundY(toRadians(yaw));
	upLeft *= rotation;
	upRight *= rotation;
	downLeft *= rotation;
	rightDir = Vector(1, 0, 0) * rotation;
	upDir    = Vector(0, 1, 0) * rotation;
	frontDir = Vector(0, 0, 1) * rotation;
	
	upLeft += pos;
	upRight += pos;
	downLeft += pos;
}

Ray Camera::getScreenRay(double x, double y, int camera)
{
	Ray result; // A, B -     C = A + (B - A) * x
	result.start = this->pos;
	Vector target = upLeft + 
		(upRight - upLeft) * (x / (double) frameWidth()) +
		(downLeft - upLeft) * (y / (double) frameHeight());
	
	// A - camera; B = target
	result.dir = target - this->pos;
	
	result.dir.normalize();
	
	if (camera != CAMERA_CENTER) {
		// offset left/right for stereoscopic rendering
		result.start += rightDir * (camera == CAMERA_RIGHT ? +stereoSeparation : -stereoSeparation);
	}
	
	if (!dof) return result;
	
	double cosTheta = dot(result.dir, frontDir);
	double M = focalPlaneDist / cosTheta;
	
	Vector T = result.start + result.dir * M;
	
	Random& R = getRandomGen();
	double dx, dy;
	R.unitDiscSample(dx, dy);
	
	dx *= discMultiplier;
	dy *= discMultiplier;
	
	result.start = this->pos + dx * rightDir + dy * upDir;
	if (camera != CAMERA_CENTER) {
		result.start += rightDir * (camera == CAMERA_RIGHT ? +stereoSeparation : -stereoSeparation);
	}
	result.dir = (T - result.start);
	result.dir.normalize();
	return result;
}

void Camera::move(double dx, double dz)
{
	pos += dx * rightDir;
	pos += dz * frontDir;
}

void Camera::rotate(double dx, double dz)
{
	pitch += dz;
	if (pitch >  90) pitch = 90;
	if (pitch < -90) pitch = -90;
	
	yaw += dx;
}


/**
 * @class SphericalLensCamera
 */
 
class SphericalLensCamera;

class Lens {
	Sphere s1, s2;
	CsgInter geom;
public:
	
	friend class SphericalLensCamera;
	void construct(double lensDist, double convexity)
	{
		double r = (1 + convexity*convexity) / (2 * convexity);
		double lensPos1 = lensDist - (r - convexity);
		double lensPos2 = lensDist + (r - convexity);
		// rays intersect s2 first, and s1 second (on the outside)
		s1 = Sphere(Vector(0, 0, lensPos1), r);
		s2 = Sphere(Vector(0, 0, lensPos2), r);
		geom = CsgInter(&s1, &s2);
	}
	bool traceRay(const Ray& inRay, Ray& outRay, double abbeNum)
	{
		double IOR_crown = 1.52; // a typical IOR for a crown glass
		IOR_crown += (555 - inRay.wavelength) / 100.0 * abbeNum;
		double invIOR_crown = 1.0 / IOR_crown;
		IntersectionData info;
		info.dist = 1e99;
		if (!s2.intersect(inRay, info)) return false;
//		if (!geom.intersect(inRay, info)) return false;
		Ray midRay;
		//Vector z = Vector(-0.41711363380418204, 0.37625948485441257, 0.82731192216222948);
		midRay.dir = refract(inRay.dir, info.normal, invIOR_crown);
		midRay.dir.normalize();
		midRay.start = info.p + midRay.dir * 1e-6;
		info.dist = 1e99;
//		if (!geom.intersect(midRay, info)) return false;
		if (!s1.intersect(midRay, info)) return false;
		outRay.start = info.p;
		outRay.dir = refract(midRay.dir, faceforward(info.normal, midRay.dir), IOR_crown);
		if (outRay.dir.lengthSqr() == 0) return false;
		outRay.dir.normalize();
		outRay.wavelength = inRay.wavelength;
		return true;
	}
};

SphericalLensCamera::SphericalLensCamera()
{
	convexity = 0.1;
	lensDist = 1.0;
	sensorScaling = 1.0;
	abbeNum = 0;
	lens = NULL;
}

SphericalLensCamera::~SphericalLensCamera()
{
	if (lens) delete lens;
}

void SphericalLensCamera::fillProperties(ParsedBlock& block)
{
	Camera::fillProperties(block);
	block.getDoubleProp("convexity", &convexity);
	block.getDoubleProp("lensDist", &lensDist);
	block.getDoubleProp("sensorScaling", &sensorScaling);
}

void SphericalLensCamera::beginFrame()
{
	Camera::beginFrame();
	sensorTopLeft = Vector(aspect, -1.0, 0.0) * 0.5 * sensorScaling;
	sensorDx = -aspect / frameWidth() * sensorScaling;
	sensorDy = 1.0 / frameHeight() * sensorScaling;
	T.reset();
	T.rotate(yaw, pitch, roll);
	T.translate(pos);
	if (!lens) lens = new Lens;
	lens->construct(lensDist, convexity);
}

Ray SphericalLensCamera::getScreenRay(double x, double y, int camera)
{
	Ray result;
	Ray input;
	input.start = sensorTopLeft + Vector(x * sensorDx, y * sensorDy, 0);
	Random& r = getRandomGen(-1);
	do {
		double x, y;
		r.unitDiscSample(x, y);
		Vector lensPoint(x / fNumber, y / fNumber, lensDist - convexity);
		input.dir = lensPoint - input.start;
		input.dir.normalize();
		input.wavelength = 555;
		if (abbeNum > 0)
			input.wavelength = r.randfloat() * (780 - 380) + 380;
	} while (!lens->traceRay(input, result, abbeNum));
	return T.ray(result);
}

void SphericalLensCamera::moveLens(double delta)
{
	lensDist += delta;
	lens->s1.center.z += delta;
	lens->s2.center.z += delta;
}

void SphericalLensCamera::multiplyAperture(double multiplier)
{
	fNumber *= multiplier;
}

void SphericalLensCamera::multiplySensorSize(double mult)
{
	sensorScaling *= mult;
	sensorTopLeft = Vector(aspect, -1.0, 0.0) * 0.5 * sensorScaling;
	sensorDx = -aspect / frameWidth() * sensorScaling;
	sensorDy = 1.0 / frameHeight() * sensorScaling;
}

void SphericalLensCamera::addAbbe(double amount)
{
	abbeNum += amount;
	if (abbeNum < 0) abbeNum = 0;
	dispersionOn = abbeNum > 0;
}
