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
	/*
	 * The checker texture works like that. Partition the whole 2D space
	 * in squares of squareSize. Use division and floor()ing to get the
	 * integral coordinates of the square, which our point happens to be. Then,
	 * use the parity of the sum of those coordinates to decide which color to return.
	 */
	// example - u = 150, v = -230, size = 100
	// -> 1, -3
	int x = floor(u / size);
	int y = floor(v / size);
	int white = (x + y) % 2;
	Color result = white ? color2 : color1;
	return result;
}


Color Lambert::shade(Ray ray, const IntersectionData& data)
{
	// turn the normal vector towards us (if needed):
	Vector N = faceforward(ray.dir, data.normal);

	// fetch the material color. This is ether the solid color, or a color
	// from the texture, if it's set up.
	Color diffuseColor = this->color;
	if (texture) diffuseColor = texture->getTexColor(ray, data.u, data.v, N);
	
	Color lightContrib = ambientLight;
	
	if (testVisibility(data.p + N * 1e-6, lightPos)) {
		Vector lightDir = lightPos - data.p;
		lightDir.normalize();
		
		// get the Lambertian cosine of the angle between the geometry's normal and
		// the direction to the light. This will scale the lighting:
		double cosTheta = dot(lightDir, N);
		
		lightContrib += lightColor * lightPower / (data.p - lightPos).lengthSqr() * cosTheta;
	}
	return diffuseColor * lightContrib;
}

Color Phong::shade(Ray ray, const IntersectionData& data)
{
	// turn the normal vector towards us (if needed):
	Vector N = faceforward(ray.dir, data.normal);

	Color diffuseColor = this->color;
	if (texture) diffuseColor = texture->getTexColor(ray, data.u, data.v, N);
	
	Color lightContrib = ambientLight;
	Color specular(0, 0, 0);
	
	if (testVisibility(data.p + N * 1e-6, lightPos)) {
		Vector lightDir = lightPos - data.p;
		lightDir.normalize();
		
		// get the Lambertian cosine of the angle between the geometry's normal and
		// the direction to the light. This will scale the lighting:
		double cosTheta = dot(lightDir, N);

		// baseLight is the light that "arrives" to the intersection point
		Color baseLight = lightColor * lightPower / (data.p - lightPos).lengthSqr();
		
		lightContrib += baseLight * cosTheta; // lambertian contribution
		
		// R = vector after the ray from the light towards the intersection point
		// is reflected at the intersection:
		Vector R = reflect(-lightDir, N);
		
		double cosGamma = dot(R, -ray.dir);
		if (cosGamma > 0)
			specular += baseLight * pow(cosGamma, exponent) * strength; // specular contribution
	}
	// specular is not multiplied by diffuseColor, since we want the specular hilights to be
	// independent on the material color. I.e., a blue ball has white hilights
	// (this is true for most materials, and false for some, e.g. gold)
	return diffuseColor * lightContrib + specular;
}

BitmapTexture::BitmapTexture(const char* fileName, double scaling)
{
	if (extensionUpper(fileName) == "EXR")
		bmp.loadEXR(fileName);
	else
		bmp.loadBMP(fileName);
	this->scaling = scaling;
}

Color BitmapTexture::getTexColor(const Ray& ray, double u, double v, Vector& normal)
{
	u *= scaling;
	v *= scaling;
	u = u - floor(u);
	v = v - floor(v); // u, v range in [0..1)
	float tx = (float) u * bmp.getWidth(); // u is in [0..textureWidth)
	float ty = (float) v * bmp.getHeight(); // v is in [0..textureHeight)
	return bmp.getPixel(tx, ty); // fetch a single pixel from the bitmap
}
