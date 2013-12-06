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

#include "heightfield.h"
#include "bitmap.h"

float Heightfield::getHeight(int x, int y) const
{
	return heights[y * W + x];
}

Vector Heightfield::getNormal(float x, float y) const
{
	// we have precalculated the normals at each integer position.
	// Here, we do bilinear filtering on the four nearest integral positions:
	int x0 = (int) floor(x);
	int y0 = (int) floor(y);
	float p = (x - x0);
	float q = (y - y0);
	int x1 = min(W - 1, x0 + 1);
	int y1 = min(H - 1, y0 + 1);
	x0 = max(0, x0);
	y0 = max(0, y0);
	Vector v = 
		normals[y0 * W + x0] * ((1 - p) * (1 - q)) +
		normals[y0 * W + x1] * ((    p) * (1 - q)) +
		normals[y1 * W + x0] * ((1 - p) * (    q)) +
		normals[y1 * W + x1] * ((    p) * (    q));
	v.normalize();
	return v;
}

bool Heightfield::intersect(Ray ray, IntersectionData& info)
{
	double dist = bbox.closestIntersection(ray);
	if (dist >= info.dist) return false;
	Vector p = ray.start + ray.dir * dist;
	
	Vector step = ray.dir;
	double distHoriz = sqrt(sqr(step.x) + sqr(step.z));
	step /= distHoriz;
	
	p += step;
	while (bbox.inside(p)) {
		if (p.y < getHeight((int) floor(p.x), (int) floor(p.z))) {
			double dist = (p - ray.start).length();
			if (dist > info.dist) return false;
			info.p = p;
			info.dist = dist;
			info.normal = getNormal(p.x, p.z);
			info.u = p.x / W;
			info.v = p.z / H;
			info.g = this;
			return true;
		}
		p += step;
	}
	return false;
}

void Heightfield::fillProperties(ParsedBlock& pb)
{
	Bitmap bmp;
	if (!pb.getBitmapFileProp("file", bmp)) pb.requiredProp("file");
	W = bmp.getWidth();
	H = bmp.getHeight();
	double blur = 0;
	pb.getDoubleProp("blur", &blur, 0, 1000);
	// do we have blur? if no, just fetch the source image and store it:
	heights = new float[W * H];
	float minY = LARGE_FLOAT, maxY = -LARGE_FLOAT;
	if (blur <= 0) {
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++) {
				float h = bmp.getPixel(x, y).intensity();
				heights[y * W + x] = h;
				minY = min(minY, h);
				maxY = max(maxY, h);
			}
	} else {
		// We have blur...
		// 1) convert image to greyscale (if not already):
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				float f = bmp.getPixel(x, y).intensity();
				bmp.setPixel(x, y, Color(f, f, f));
			}
		}
		// 2) calculate the gaussian coefficients, see http://en.wikipedia.org/wiki/Gaussian_blur
		static float gauss[128][128];
		int R = min(128, nearestInt(float(3 * blur)));
		for (int y = 0; y < R; y++)
			for (int x = 0; x < R; x++)
				gauss[y][x] = float(exp(-(sqr(x) + sqr(y))/(2 * sqr(blur))) / (2 * PI * sqr(blur)));
		// 3) apply gaussian blur with the specified number of blur units:
		// (this is potentially slow for large blur radii)
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				float sum = 0;
				for (int dy = -R + 1; dy < R; dy++)
					for (int dx = -R + 1; dx < R; dx++)
						sum += gauss[abs(dy)][abs(dx)] * bmp.getPixel(x + dx, y + dy).r;
				heights[y * W + x] = sum;
				minY = min(minY, sum);
				maxY = max(maxY, sum);
			}
		}
	}
	
	bbox.vmin = Vector(0, minY, 0);
	bbox.vmax = Vector(W, maxY, H);
	
	normals = new Vector[W * H];
	for (int y = 0; y < H - 1; y++)
		for (int x = 0; x < W - 1; x++) {
			float h0 = heights[y * W + x];
			float hdx = heights[y * W + x + 1];
			float hdy = heights[(y + 1) * W + x];
			Vector vdx = Vector(1, hdx - h0, 0);
			Vector vdy = Vector(0, hdy - h0, 1);
			Vector norm = vdy ^ vdx;
			norm.normalize();
			normals[y * W + x] = norm;
		}
	
}


