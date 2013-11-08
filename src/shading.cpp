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

#include "shading.h"

Vector lightPos;
Color lightColor;
float lightPower;
Color ambientLight;

extern bool testVisibility(const Vector& from, const Vector& to);




Shader::Shader(const Color& color)
{
	this->color = color;
}

Color Checker::getTexColor(const Ray& ray, double u, double v, Vector& normal)
{
	// example - u = 150, -230
	// -> 1, -3
	int x = floor(u / size);
	int y = floor(v / size);
	int white = (x + y) % 2;
	Color result = white ? color2 : color1;
	return result;
}


Color Lambert::shade(Ray ray, const IntersectionData& data)
{
	Color diffuseColor = this->color;
	
	Vector N = faceforward(ray.dir, data.normal);
	if (texture) diffuseColor = texture->getTexColor(ray, data.u, data.v, N);
	
	Color lightContrib = ambientLight;
	
	if (testVisibility(data.p + N * 1e-6, lightPos)) {
		Vector lightDir = lightPos - data.p;
		lightDir.normalize();
		
		double cosTheta = dot(lightDir, N);
		
		lightContrib += lightColor * lightPower / (data.p - lightPos).lengthSqr() * cosTheta;
	}
	return diffuseColor * lightContrib;
}

Color Phong::shade(Ray ray, const IntersectionData& data)
{
	Color diffuseColor = this->color;
	
	Vector N = faceforward(ray.dir, data.normal);
	if (texture) diffuseColor = texture->getTexColor(ray, data.u, data.v, N);
	
	Color lightContrib = ambientLight;
	Color specular(0, 0, 0);
	
	if (testVisibility(data.p + N * 1e-6, lightPos)) {
		Vector lightDir = lightPos - data.p;
		lightDir.normalize();
		
		double cosTheta = dot(lightDir, N);
		
		Color baseLight = lightColor * lightPower / (data.p - lightPos).lengthSqr();
		
		lightContrib += baseLight * cosTheta;
		
		Vector R = reflect(-lightDir, N);
		
		double cosGamma = dot(R, -ray.dir);
		if (cosGamma > 0)
			specular += baseLight * pow(cosGamma, exponent) * strength;
	}
	return diffuseColor * lightContrib + specular;
}

BitmapTexture::BitmapTexture(const char* fileName, double scaling)
{
	bmp.loadBMP(fileName);
	this->scaling = scaling;
}

Color BitmapTexture::getTexColor(const Ray& ray, double u, double v, Vector& normal)
{
	u *= scaling;
	v *= scaling;
	u = u - floor(u);
	v = v - floor(v);
	
	return bmp.getPixel(u * bmp.getWidth(), v * bmp.getHeight());
}
