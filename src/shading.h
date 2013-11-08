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
#ifndef __SHADING_H__
#define __SHADING_H__

#include "color.h"
#include "vector.h"
#include "geometry.h"
#include "bitmap.h"

extern Vector lightPos;
extern Color lightColor;
extern float lightPower;
extern Color ambientLight;


class Shader {
protected:
	Color color;
public:
	Shader(const Color& color);
	virtual ~Shader() {}

	virtual Color shade(Ray ray, const IntersectionData& data) = 0;
};

class Texture {
public:
	virtual ~Texture() {}
	
	virtual Color getTexColor(const Ray& ray, double u, double v, Vector& normal) = 0;
};

class Checker: public Texture {
	Color color1, color2;
	double size;
public:
	Checker(const Color& color1, const Color& color2, double size = 1):
		color1(color1), color2(color2), size(size) {}
	Color getTexColor(const Ray& ray, double u, double v, Vector& normal);
};

class BitmapTexture: public Texture {
	Bitmap bmp;
	double scaling;
public:
	BitmapTexture(const char* fileName, double scaling = 1);
	Color getTexColor(const Ray& ray, double u, double v, Vector& normal);
	
};

class Lambert: public Shader {
	Texture* texture;
public:
	Lambert(const Color& diffuseColor, Texture* texture = NULL):
		Shader(diffuseColor), texture(texture) {}
	Color shade(Ray ray, const IntersectionData& data);
};

class Phong: public Shader {
	Texture* texture;
	double exponent;
	float strength;
public:
	Phong(const Color& diffuseColor, double exponent, float strength = 1, Texture* texture = NULL):
		Shader(diffuseColor), texture(texture), exponent(exponent),
		strength(strength) {}
	Color shade(Ray ray, const IntersectionData& data);
};

#endif // __SHADING_H__
