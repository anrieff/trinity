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
	
	upLeft += pos;
	upRight += pos;
	downLeft += pos;
}

Ray Camera::getScreenRay(double x, double y)
{
	Ray result; // A, B -     C = A + (B - A) * x
	result.start = this->pos;
	Vector target = upLeft + 
		(upRight - upLeft) * (x / (double) frameWidth()) +
		(downLeft - upLeft) * (y / (double) frameHeight());
	
	// A - camera; B = target
	result.dir = target - this->pos;
	
	result.dir.normalize();
	
	return result;
}
