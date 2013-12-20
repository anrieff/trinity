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

#include <string.h>
#include "lights.h"
#include "shading.h"
#include "random_generator.h"

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
	
	Color lightContrib = scene.settings.ambientLight;
	
	for (int i = 0; i < (int) scene.lights.size(); i++) {
		int numSamples = scene.lights[i]->getNumSamples();
		Color avgColor(0, 0, 0);
		for (int j = 0; j < numSamples; j++) {
			Vector lightPos;
			Color lightColor;
			scene.lights[i]->getNthSample(j, data.p, lightPos, lightColor);
			if (lightColor.intensity() != 0 && testVisibility(data.p + N * 1e-6, lightPos)) {
				Vector lightDir = lightPos - data.p;
				lightDir.normalize();
				
				// get the Lambertian cosine of the angle between the geometry's normal and
				// the direction to the light. This will scale the lighting:
				double cosTheta = dot(lightDir, N);
				if (cosTheta > 0)
					avgColor += lightColor / (data.p - lightPos).lengthSqr() * cosTheta;
			}
		}
		lightContrib += avgColor / numSamples;
	}
	return diffuseColor * lightContrib;
}

Color Phong::shade(Ray ray, const IntersectionData& data)
{
	// turn the normal vector towards us (if needed):
	Vector N = faceforward(ray.dir, data.normal);

	Color diffuseColor = this->color;
	if (texture) diffuseColor = texture->getTexColor(ray, data.u, data.v, N);
	
	Color lightContrib = scene.settings.ambientLight;
	Color specular(0, 0, 0);
	
	for (int i = 0; i < (int) scene.lights.size(); i++) {
		int numSamples = scene.lights[i]->getNumSamples();
		Color avgColor(0, 0, 0);
		Color avgSpecular(0, 0, 0);
		for (int j = 0; j < numSamples; j++) {
			Vector lightPos;
			Color lightColor;
			scene.lights[i]->getNthSample(j, data.p, lightPos, lightColor);
			if (lightColor.intensity() != 0 && testVisibility(data.p + N * 1e-6, lightPos)) {
				Vector lightDir = lightPos - data.p;
				lightDir.normalize();
				
				// get the Lambertian cosine of the angle between the geometry's normal and
				// the direction to the light. This will scale the lighting:
				double cosTheta = dot(lightDir, N);

				// baseLight is the light that "arrives" to the intersection point
				Color baseLight = lightColor / (data.p - lightPos).lengthSqr();
				if (cosTheta > 0)
					avgColor += baseLight * cosTheta; // lambertian contribution
				
				// R = vector after the ray from the light towards the intersection point
				// is reflected at the intersection:
				Vector R = reflect(-lightDir, N);
				
				double cosGamma = dot(R, -ray.dir);
				if (cosGamma > 0)
					avgSpecular += baseLight * pow(cosGamma, exponent) * strength; // specular contribution
			
			}
		}
		lightContrib += avgColor / numSamples;
		specular += avgSpecular / numSamples;
	}
	// specular is not multiplied by diffuseColor, since we want the specular hilights to be
	// independent on the material color. I.e., a blue ball has white hilights
	// (this is true for most materials, and false for some, e.g. gold)
	return diffuseColor * lightContrib + specular;
}

static Color getTexValue(const Bitmap& bmp, double u, double v)
{
	u = u - floor(u);
	v = v - floor(v); // u, v range in [0..1)
	float tx = (float) u * bmp.getWidth(); // u is in [0..textureWidth)
	float ty = (float) v * bmp.getHeight(); // v is in [0..textureHeight)
	return bmp.getFilteredPixel(tx, ty); // fetch from the bitmap with bilinear filtering
}

Color BitmapTexture::getTexColor(const Ray& ray, double u, double v, Vector& normal)
{
	u *= scaling;
	v *= scaling;
	return getTexValue(bmp, u, v);
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
		// avoid combinatorial explosion with inter-reflecting glossy surfaces:
		int samplesWanted = (ray.flags & RF_GLOSSY) ? 5 : numSamples;
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
			newRay.flags |= RF_GLOSSY;
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

void Layered::fillProperties(ParsedBlock& pb)
{
	char name[128];
	char value[256];
	int srcLine;
	for (int i = 0; i < pb.getBlockLines(); i++) {
		// fetch and parse all lines like "layer <shader>, <color>[, <texture>]"
		pb.getBlockLine(i, srcLine, name, value);
		if (!strcmp(name, "layer")) {
			char shaderName[200];
			char textureName[200] = "";
			bool err = false;
			if (!getFrontToken(value, shaderName)) {
				err = true;
			} else {
				stripPunctuation(shaderName);
			}
			if (!strlen(value)) err = true;
			if (!err && value[strlen(value) - 1] != ')') {
				if (!getLastToken(value, textureName)) {
					err = true;
				} else {
					stripPunctuation(textureName);
				}
			}
			if (!err && !strcmp(textureName, "NULL")) strcpy(textureName, "");
			Shader* shader = NULL;
			Texture* texture = NULL;
			if (!err) {
				shader = pb.getParser().findShaderByName(shaderName);
				err = (shader == NULL);
			}
			if (!err && strlen(textureName)) {
				texture = pb.getParser().findTextureByName(textureName);
				err = (texture == NULL);
			}
			if (err) throw SyntaxError(srcLine, "Expected a line like `layer <shader>, <color>[, <texture>]'");
			double x, y, z;
			get3Doubles(srcLine, value, x, y, z);
			addLayer(shader, Color((float) x, (float) y, (float) z), texture);
		}
	}
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

void BumpTexture::modifyNormal(IntersectionData& data)
{
	Color bumpVal = getTexValue(bmp, data.u, data.v) * strength;
	
	data.normal += data.dNdx * bumpVal[0] + data.dNdy * bumpVal[1];
	data.normal.normalize();
}

void Bumps::modifyNormal(IntersectionData& data)
{
	if (strength > 0) {
		float freqX[3] = { 0.5, 1.21, 1.9 }, freqZ[3] = { 0.4, 1.13, 1.81 };
		float fm = 0.2;
		float intensityX[3] = { 0.1, 0.08, 0.05 }, intensityZ[3] = { 0.1, 0.08, 0.05 };
		double dx = 0, dy = 0;
		for (int i = 0; i < 3; i++) {
			dx += sin(fm * freqX[i] * data.u) * intensityX[i] * strength; 
			dy += sin(fm * freqZ[i] * data.v) * intensityZ[i] * strength;
		}
		data.normal += dx * data.dNdx + dy * data.dNdy;
		data.normal.normalize();
	}
}

Color Bumps::getTexColor(const Ray& ray, double u, double v, Vector& normal)
{
	return Color(0, 0, 0);
}
