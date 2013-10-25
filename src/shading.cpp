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


Shader::Shader(const Color& color)
{
	this->color = color;
}

CheckerShader::CheckerShader(const Color& c1, const Color& c2, double size)
: Shader(c1)
{
	color2 = c2;
	this->size = size;
}


Color CheckerShader::shade(Ray ray, const IntersectionData& data)
{
	// example - u = 150, -230
	// -> 1, -3
	int x = floor(data.u / size);
	int y = floor(data.v / size);
	int white = (x + y) % 2;
	Color result = white ? color2 : color;
	
	result = result * lightColor * lightPower / (data.p - lightPos).lengthSqr();
	Vector lightDir = lightPos - data.p;
	lightDir.normalize();
	
	double cosTheta = dot(lightDir, data.normal);
	result = result * cosTheta;
	return result;
}
