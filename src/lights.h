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
#ifndef __LIGHTS_H__
#define __LIGHTS_H__

#include "vector.h"
#include "scene.h"
#include "transform.h"



/// @brief a generic Light interface
class Light: public SceneElement {
protected:
	Color color;
	float power;
public:
	Light() { color.makeZero(); power = 0; }
	virtual ~Light() {}
	
	Color getColor() const { return color * power; }

	/// get the number of samples this light requires (must be strictly positive)
	virtual int getNumSamples() = 0;
	
	/**
	 * gets the n-th sample
	 * @param sampleIdx - a sample index: 0 <= sampleIdx < getNumSamples().
	 * @param shadePos  - the point we're shading. Can be used to modulate light power if the
	 *                    light doesn't shine equally in all directions.
	 * @param samplePos [out] - the generated light sample position
	 * @param color [out] - the generated light "color". This is usually has large components (i.e.,
	 *                      it's base color * power
	 */
	virtual void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) = 0;
	
	virtual ElementType getElementType() const { return ELEM_LIGHT; }
	
	/// default parameters of all lights. DO NOT FORGET to call Light::fillProperties() in
	/// derived classes!!!
	virtual void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &color);
		pb.getFloatProp("power", &power);
	}
	
	/**
	 * intersects a ray with the light. The param intersectionDist is in/out;
	 * it's behaviour is similar to Intersectable::intersect()'s treatment of distances.
	 * @retval true, if the ray intersects the light, and the intersection distance is smaller
	 *               than the current value of intersectionDist (which is updated upon return)
	 * @retval false, otherwise.
	 */
	virtual bool intersect(const Ray& ray, double& intersectionDist) = 0;
};

/// The good ol' point light
class PointLight: public Light {
	Vector pos;
public:
	int getNumSamples();
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color);
	bool intersect(const Ray& ray, double& intersectionDist);
	
	void fillProperties(ParsedBlock& pb)
	{
		Light::fillProperties(pb);
		pb.getVectorProp("pos", &pos);
	}
	
};

/// A rectangle light; uses a transform to position in space and change shape. The canonic
/// light is a 1x1 square, positioned in (0, 0, 0), pointing in the direction of -Y. The
/// light is one-sided (the +Y hemisphere doesn't get any light).
class RectLight: public Light {
	Transform transform;
	int xSubd, ySubd;
public:
	RectLight(): Light() { xSubd = 2; ySubd = 2; transform.reset(); }
	int getNumSamples();
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color);
	bool intersect(const Ray& ray, double& intersectionDist);
	
	void fillProperties(ParsedBlock& pb)
	{
		Light::fillProperties(pb);
		pb.getIntProp("xSubd", &xSubd, 1);
		pb.getIntProp("ySubd", &ySubd, 1);
		pb.getTransformProp(transform);
	}
};

#endif // __LIGHTS_H__
