
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


class Light: public SceneElement {
protected:
	Color color;
	float power;
public:
	Light() { color.makeZero(); power = 0; }
	virtual ~Light() {}
	
	virtual int getNumSamples() = 0;
	virtual void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color) = 0;
	
	virtual ElementType getElementType() const { return ELEM_LIGHT; }
	virtual void fillProperties(ParsedBlock& pb)
	{
		pb.getColorProp("color", &color);
		pb.getFloatProp("power", &power);
	}
};

class PointLight: public Light {
	Vector pos;
public:
	int getNumSamples();
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color);
	
	void fillProperties(ParsedBlock& pb)
	{
		Light::fillProperties(pb);
		pb.getVectorProp("pos", &pos);
	}
	
};

class RectLight: public Light {
	Transform T;
	int xSubd, ySubd;
public:
	RectLight(): Light() { xSubd = 2; ySubd = 2; T.reset(); }
	int getNumSamples();
	void getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color);
	
	void fillProperties(ParsedBlock& pb)
	{
		Light::fillProperties(pb);
		pb.getIntProp("xSubd", &xSubd);
		pb.getIntProp("ySubd", &ySubd);
		pb.getTransformProp(T);
	}
	
};

#endif // __LIGHTS_H__



