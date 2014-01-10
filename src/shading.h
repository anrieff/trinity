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

class BRDF {
public:
	virtual Color eval(const IntersectionData& x, const Ray& w_in, const Ray& w_out);

	virtual void spawnRay(const IntersectionData& x, const Ray& w_in, 
		Ray& w_out, Color& colorEval, float& pdf);
};

/// An abstract class, representing a shader in our scene.
class Shader: public SceneElement, public BRDF {
protected:
	Color color;
public:
	Shader(const Color& color);
	virtual ~Shader() {}

	virtual Color shade(Ray ray, const IntersectionData& data) = 0;
	
	// from SceneElement:
	ElementType getElementType() const { return ELEM_SHADER; }

	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &color);
	}

};

/// An abstract class, representing a (2D) texture
class Texture: public SceneElement {
public:
	virtual ~Texture() {}
	
	virtual Color getTexColor(const Ray& ray, double u, double v, Vector& normal) = 0;
	virtual void modifyNormal(IntersectionData& data) {}
	
	// from SceneElement:
	ElementType getElementType() const { return ELEM_TEXTURE; }
};

/// A checker texture
class Checker: public Texture {
	Color color1, color2; /// the colors of the alternating squares
	double size; /// the size of a square side, in world units
public:
	Checker(const Color& color1 = Color(0, 0, 0), const Color& color2 = Color(1, 1, 1), double size = 1):
		color1(color1), color2(color2), size(size) {}
	Color getTexColor(const Ray& ray, double u, double v, Vector& normal);
	void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color1", &color1);
		pb.getColorProp("color2", &color2);
		pb.getDoubleProp("size", &size);
	}
};

class BitmapTexture: public Texture {
	Bitmap bmp;
	double scaling;
	float assumedGamma;
public:
	/// load a bitmap texture from file
	/// @param fileName: the path to the bitmap file. Can be .bmp or .exr
	/// @param scaling:  scaling for the input (u, v) coords. Larger values SHRINK the texture on-screen
	/// @param assumedGamma: assumed gamma compression of the input image.
	///     if assumedGamma == 1, no gamma decompression is done.
	///     if assumedGamma == 2.2 (a special value) - sRGB decompression is done.
	///     otherwise, gamma decompression with the given power is performed
	BitmapTexture() { scaling = 1; assumedGamma = 2.2f; } // default constructor, in which case the loading is done later.
	Color getTexColor(const Ray& ray, double u, double v, Vector& normal);
	
	void fillProperties(ParsedBlock& pb)
	{
		pb.getDoubleProp("scaling", &scaling);
		pb.getFloatProp("assumedGamma", &assumedGamma);
		if (!pb.getBitmapFileProp("file", bmp));
			pb.requiredProp("file");
		if (assumedGamma != 1) {
			if (assumedGamma == 2.2f)
				bmp.decompressGamma_sRGB();
			else if (assumedGamma > 0 && assumedGamma < 10)
				bmp.decompressGamma(assumedGamma);
		}
	}
};

/// A Lambert (flat) shader
class Lambert: public Shader {
	Texture* texture; //!< a diffuse texture, if not NULL.
public:
	Lambert(const Color& diffuseColor = Color(1, 1, 1), Texture* texture = NULL):
		Shader(diffuseColor), texture(texture) {}
	Color shade(Ray ray, const IntersectionData& data);
	void fillProperties(ParsedBlock& pb)
	{
		Shader::fillProperties(pb);
		pb.getTextureProp("texture", &texture);
	}

	Color eval(const IntersectionData& x, const Ray& w_in, const Ray& w_out);

	void spawnRay(const IntersectionData& x, const Ray& w_in, 
		Ray& w_out, Color& colorEval, float& pdf);
};

/// A Phong shader
class Phong: public Shader {
	Texture* texture; //!< a diffuse texture, if not NULL.
	double exponent; //!< exponent ("shininess") of the material
	float strength; //!< strength of the cos^n specular component (0..1)
public:
	Phong(const Color& diffuseColor = Color(1, 1, 1), double exponent = 16.0, float strength = 1.0f, Texture* texture = NULL):
		Shader(diffuseColor), texture(texture), exponent(exponent),
		strength(strength) {}
	Color shade(Ray ray, const IntersectionData& data);
	void fillProperties(ParsedBlock& pb)
	{
		Shader::fillProperties(pb);
		pb.getDoubleProp("exponent", &exponent, 1e-6, 1e6);
		pb.getFloatProp("strength", &strength, 0, 1e6);
		pb.getTextureProp("texture", &texture);
	}
};

class Refl: public Shader {
	double glossiness;
	int numSamples;
	static void getRandomDiscPoint(double& x, double& y);
public:
	Refl(const Color& filter = Color(1, 1, 1), double glossiness = 1.0, 
		int numSamples = 20):
		Shader(filter), glossiness(glossiness), numSamples(numSamples) {}
	Color shade(Ray ray, const IntersectionData& data);
	void fillProperties(ParsedBlock& pb)
	{
		Shader::fillProperties(pb);
		pb.getDoubleProp("glossiness", &glossiness, 0, 1);
		pb.getIntProp("numSamples", &numSamples, 1);
	}
	Color eval(const IntersectionData& x, const Ray& w_in, const Ray& w_out);

	void spawnRay(const IntersectionData& x, const Ray& w_in, 
		Ray& w_out, Color& colorEval, float& pdf);
};

class Refr: public Shader {
	float ior;
public:
	Refr(const Color& filter = Color(1, 1, 1), float ior = 1.0f): Shader(filter), ior(ior) {}
	Color shade(Ray ray, const IntersectionData& data);
	void fillProperties(ParsedBlock& pb)
	{
		Shader::fillProperties(pb);
		pb.getFloatProp("ior", &ior, 1e-6, 10);
	}
	Color eval(const IntersectionData& x, const Ray& w_in, const Ray& w_out);

	void spawnRay(const IntersectionData& x, const Ray& w_in, 
		Ray& w_out, Color& colorEval, float& pdf);
};

class Layered: public Shader {
	struct Layer {
		Shader* shader;
		Color blend;
		Texture* texture;
	};
	
	static const int MAX_LAYERS = 32;
	Layer layers[MAX_LAYERS];
	int numLayers;
public:
	Layered(): Shader(Color(0, 0, 0)) { numLayers = 0; }
	void addLayer(Shader* shader, const Color& blend, Texture* texture = NULL);
	
	Color shade(Ray ray, const IntersectionData& data);
	void fillProperties(ParsedBlock& pb);
};

class Fresnel: public Texture {
	float ior;
public:
	Fresnel(float ior = 1.0f) : ior(ior) {}
	Color getTexColor(const Ray& ray, double u, double v, Vector& normal);
	void fillProperties(ParsedBlock& pb)
	{
		pb.getFloatProp("ior", &ior, 1e-6, 10);
	}
};

class BumpTexture: public Texture {
	Bitmap bmp;
	float strength;
public:
	BumpTexture() { strength = 1; }
	
	void modifyNormal(IntersectionData& data);
	Color getTexColor(const Ray& ray, double u, double v, Vector& normal)
	{
		return Color(0, 0, 0);
	}
	void fillProperties(ParsedBlock& pb)
	{
		pb.getBitmapFileProp("file", bmp);
		bmp.differentiate();
		pb.getFloatProp("strength", &strength);
	}
};

// a texture that generates a slight random bumps on any geometry, which computes dNdx, dNdy
class Bumps: public Texture {
	float strength;
public:
	Bumps() { strength = 0; }
	
	void modifyNormal(IntersectionData& data);
	Color getTexColor(const Ray& ray, double u, double v, Vector& normal);
	void fillProperties(ParsedBlock& pb)
	{
		pb.getFloatProp("strength", &strength);
	}
	
};

#endif // __SHADING_H__
