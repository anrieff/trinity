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

extern Vector lightPos;
extern Color lightColor;
extern float lightPower;


class Shader {
protected:
	Color color;
public:
	Shader(const Color& color);
	virtual ~Shader() {}

	virtual Color shade(Ray ray, const IntersectionData& data) = 0;
};

class CheckerShader: public Shader {
	Color color2;
	double size;
public:
	CheckerShader(const Color& c1, const Color& c2, double size = 1);
	Color shade(Ray ray, const IntersectionData& data);
};

#endif // __SHADING_H__
