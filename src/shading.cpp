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
#include "random_generator.h"

Vector lightPos;
Color lightColor;
float lightPower;
Color ambientLight;

extern bool testVisibility(const Vector& from, const Vector& to);
extern Color raytrace(Ray ray);



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

BitmapTexture::BitmapTexture(const char* fileName, double scaling, float assumedGamma)
{
	bmp.loadImage(fileName);
	this->scaling = scaling;
	if (assumedGamma != 1) {
		if (assumedGamma == 2.2f)
			bmp.decompressGamma_sRGB();
		else if (assumedGamma > 0 && assumedGamma < 10)
			bmp.decompressGamma(assumedGamma);
	}
}

Color BitmapTexture::getTexColor(const Ray& ray, double u, double v, Vector& normal)
{
	u *= scaling;
	v *= scaling;
	u = u - floor(u);
	v = v - floor(v); // u, v range in [0..1)
	float tx = (float) u * bmp.getWidth(); // u is in [0..textureWidth)
	float ty = (float) v * bmp.getHeight(); // v is in [0..textureHeight)
	return bmp.getFilteredPixel(tx, ty); // fetch from the bitmap with bilinear filtering
}

Color Refl::shade(Ray ray, const IntersectionData& data)
{
	Vector N = faceforward(ray.dir, data.normal);
	
	if (glossiness == 1) {
	 	// The material is't glossy: simple reflection, launch a single ray:
		Vector reflected = reflect(ray.dir, N);
		
		Ray newRay = ray;
		newRay.start = data.p + N * 1e-6;
		newRay.dir = reflected;
		newRay.depth = ray.depth + 1;
		return raytrace(newRay) * color;
	} else {
		// generate an orthonormed system; the new vectors a and b will be orthogonal
		// to each other, and to N, in the same time.
		Random& rnd = getRandomGen();
		Vector a, b;
		orthonormedSystem(N, a, b);
		Color result(0, 0, 0);
		double scaling = tan((1 - glossiness) * PI/2);
		// hack: avoid combinatorial explosion with inter-reflecting glossy surfaces:
		int samplesWanted = ray.depth == 0 ? numSamples : 5;
		for (int i = 0; i < samplesWanted; i++) {
			Vector reflected;
			do {
				double x, y;
				// get a random point on the unit disc and scale it:
				rnd.unitDiscSample(x, y);
				x *= scaling;
				y *= scaling;
				
				// modify the normal according to the random offset:
				Vector newNormal = N + a * x + b * y;
				newNormal.normalize();
				
				// reflect the incoming ray around the new normal:
				reflected = reflect(ray.dir, newNormal);
			} while (dot(reflected, N) < 0); // check if the reflection is valid.
			
			// sample the resulting valid ray, and add to the sum
			Ray newRay = ray;
			newRay.start = data.p + N * 1e-6;
			newRay.dir = reflected;
			newRay.depth = ray.depth + 1;
			result += raytrace(newRay) * color;
		}
		return result / samplesWanted;
	}
}

Color Refr::shade(Ray ray, const IntersectionData& data)
{
	Vector N = faceforward(ray.dir, data.normal);
	
	// refract() expects the ratio of IOR_WE_ARE_EXITING : IOR_WE_ARE_ENTERING.
	// the ior parameter has the ratio of this material to vacuum, so if we're
	// entering the geometry, be sure to take the reciprocal
	float eta = ior;
	if (dot(ray.dir, data.normal) < 0)
		eta = 1.0f / eta;
	
	Vector refracted = refract(ray.dir, N, eta);
	
	// total inner refraction:
	if (refracted.lengthSqr() == 0) return Color(0, 0, 0);
	
	Ray newRay = ray;
	newRay.start = data.p + ray.dir * 1e-6;
	newRay.dir = refracted;
	newRay.depth = ray.depth + 1;
	return raytrace(newRay) * color;
}

void Layered::addLayer(Shader* shader, const Color& blend, Texture* texture)
{
	Layer& l = layers[numLayers];
	l.shader = shader;
	l.blend = blend;
	l.texture = texture;
	numLayers++;
}

Color Layered::shade(Ray ray, const IntersectionData& data)
{
	Color result(0, 0, 0);
	Vector N = data.normal;
	for (int i = 0; i < numLayers; i++) {
		Layer& l = layers[i];
		Color opacity = l.texture ? 
			l.texture->getTexColor(ray, data.u, data.v, N) : l.blend; 
		Color transparency = Color(1, 1, 1) - opacity;
		result = transparency * result + opacity * l.shader->shade(ray, data);
	}
	return result;
}

// Schlick's approximation
static float fresnel(const Vector& i, const Vector& n, float ior)
{
	float f = sqr((1.0f - ior) / (1.0f + ior));
	float NdotI = (float) -dot(n, i);
	return f + (1.0f - f) * pow(1.0f - NdotI, 5.0f);
}


Color Fresnel::getTexColor(const Ray& ray, double u, double v, Vector& normal)
{
	// fresnel() expects the IOR_WE_ARE_ENTERING : IOR_WE_ARE_EXITING, so
	// in the case we're exiting the geometry, be sure to take the reciprocal
	float eta = ior;
	if (dot(normal, ray.dir) > 0)
		eta = 1.0f / eta;
	Vector N = faceforward(ray.dir, normal);
	float fr = fresnel(ray.dir, N, eta);
	return Color(fr, fr, fr);
}


