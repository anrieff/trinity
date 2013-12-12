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

#include <SDL/SDL.h>
#include "heightfield.h"
#include "bitmap.h"

Heightfield::~Heightfield()
{
	if (normals) delete[] normals; normals = NULL;
	if (heights) delete[] heights; heights = NULL;
	if (maxH) delete[] maxH; maxH = NULL;
}

float Heightfield::getHeight(int x, int y) const
{
	if (x < 0 || y < 0 || x >= W || y >= H) return (float) bbox.vmin.y;
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
	Vector p = ray.start + ray.dir * (dist + 1e-6); // step firmly inside the bbox
	
	Vector step = ray.dir;

	double mx = 1.0 / ray.dir.x; // mx = how much to go along ray.dir until the unit distance along X is traversed
	double mz = 1.0 / ray.dir.z; // same as mx, for Z

	while (bbox.inside(p)) {
		int x0 = (int) floor(p.x);
		int z0 = (int) floor(p.z);
		if (x0 < 0 || x0 >= W || z0 < 0 || z0 >= H) break; // if outside the [0..W)x[0..H) rect, get out
		// try to use the fast structure:
		if (useOptimization) {
			int k = 0;
			
			// explanation (see below): A is the highest peak around (x0, z0) at radius 2^k
			//  B is the minimum of the starting ray height, and the final height after going
			//  2^k units along the ray. I.e., A is the highest the terrain may go up to;
			//  B is the lowest the ray can go down to. If A < B, then no intersection is possible,
			//  and it is safe to skip 2^k along the ray, and we may even opt to skip more.
			
			//    while        [         A         ] < [                B                 ]
			while (k < maxK && getHighest(x0, z0, k) < min(p.y, p.y + ray.dir.y * (1 << k)))
				k++;
			
			k--; // decrease k, because at k, the test failed; k - 1 is OK
			if (k > 0) {
				p += ray.dir * (1 << k);
				continue;
			}
			// if the test failed at k = 0, we're too close to the terrain - check for intersection:
		}
		// calculate how much we need to go along ray.dir until we hit the next X voxel boundary:
		double lx = ray.dir.x > 0 ? (ceil(p.x) - p.x) * mx : (floor(p.x) - p.x) * mx;
		// same as lx, for the Z direction:
		double lz = ray.dir.z > 0 ? (ceil(p.z) - p.z) * mz : (floor(p.z) - p.z) * mz;
		// advance p along ray.dir until we hit the next X or Z gridline
		// also, go a little more than that, to assure we're firmly inside the next voxel:
		Vector p_next = p + step * (min(lx, lz) + 1e-6);
		// "p" is position before advancement; p_next is after we take a single step.
		// if any of those are below the height of the nearest four voxels of the heightfield,
		// we need to test the current voxel for intersection:
		if (min(p.y, p_next.y) < maxH[z0 * W + x0]) {
			double closestDist = INF;
			// form ABCD - the four corners of the current voxel, whose heights are taken from the heightmap
			// then form triangles ABD and BCD and try to intersect the ray with each of them:
			Vector A = Vector(x0, getHeight(x0, z0), z0);
			Vector B = Vector(x0 + 1, getHeight(x0 + 1, z0), z0);
			Vector C = Vector(x0 + 1, getHeight(x0 + 1, z0 + 1), z0 + 1);
			Vector D = Vector(x0, getHeight(x0, z0 + 1), z0 + 1);
			if (intersectTriangleFast(ray, A, B, D, closestDist) ||
			    intersectTriangleFast(ray, B, C, D, closestDist)) {
				// intersection found: ray hits either triangle ABD or BCD. Which one exactly isn't
				// important, because we calculate the normals by bilinear interpolation of the
				// precalculated normals at the four corners:
				if (closestDist > info.dist) return false;
				info.dist = closestDist;
				info.p = ray.start + ray.dir * closestDist;
				info.normal = getNormal((float) info.p.x, (float) info.p.z);
				info.u = info.p.x / W;
				info.v = info.p.z / H;
				info.g = this;
				return true;
			}
		}
		p = p_next;
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

	maxH = new float[W*H];
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			float& maxH = this->maxH[y * W + x];
			maxH = heights[y * W + x];
			if (x < W - 1) maxH = max(maxH, heights[y * W + x + 1]);
			if (y < H - 1) {
				maxH = max(maxH, heights[(y + 1) * W + x]);
				if (x < W - 1)
					maxH = max(maxH, heights[(y + 1) * W + x + 1]);
			}
		}
	
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
	useOptimization = false;
	pb.getBoolProp("useOptimization", &useOptimization);
	if (useOptimization) {
		Uint32 clk = SDL_GetTicks();
		buildStruct();
		clk = SDL_GetTicks() - clk;
		printf("Heightfield acceleration struct built in %.3lfs\n", clk / 1000.0);
	}
}

void Heightfield::buildStruct(void)
{
	hsmap = new HighStruct[W*H];
	/*
	 * to build the first level, consider that, when we're inside some square
	 * and consider the highest possible elevation around with R = 1. This is the
	 * 5x5 square around that point (we need 3x3 to cover all possible squares
	 * one can reach with radius 1, and then also extend that further with 1 to
	 * account for the fact that the heightfield is not composed of solid blocks, but
	 * rather a combination of two triangles. The triangles may rise up quite higher,
	 * if the neighbouring points of the heightfield are higher; to account for them,
	 * we include an extra layer around the 3x3, thus yielding a 5x5.
	 */
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++) {
			float maxh = getHeight(x, y);
			for (int dy = -2; dy <= 2; dy++)
				for (int dx = -2; dx <= 2; dx++)
					maxh = max(maxh, getHeight(x + dx, y + dy));
			hsmap[y * W + x].h[0] = maxh;
		}
	/*
	 * Here's our structure-building algorithm
	 *   Consider the record for (x, y), for various values of k:
	 *   for k = 0, we have the highest texel in a 5x5 square, cenrtered in (x, y) (as explained above)
	 *   for k = 1, we have the highest texel for a square 7x7 centered in (x, y). We can get that by combining four 5x5 instances
	 *              (level 0) at offsets (-1, -1), (-1, 1), (1, -1), (1, 1). There are overlapping texels, but since we only need
	 *              the maximum, they don't really matter
	 *   for k = 2, we have the highest texel for 11x11 square at (x, y). Find that by integrating four 7x7 instances
	 *              at offsets (-2, -2), (-2, 2), (2, -2) and (2, 2).
	 *   for k = 3, the squares are 19x19
	 *   for k = 4, the squares are 35x35
	 * etc, etc...
	 *
	 *   generally, for any k, we have the highest texel for a (2^(k+1)+3)x(2^(k+1)+3) square (radius 2^k). To compute it, we get four
	 *   squares of (k-1) size (i.e. (2^k + 3)x(2^k + 3)) and compute the max of their maxes. The offsets from (x, y) are 2^(k-1).
	 */
	// maxK is the number of levels: maxK ~= log2(N)
	maxK = (int) (ceil(log((double) max(W, H)) / log(2.0))) + 1; // +1 to get the diagonal as well
	for (int k = 1; k < maxK; k++) {
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++) {
				int offset = 1 << (k - 1);
				float up_left    =  getHighest(x - offset, y - offset, k - 1);
				float up_right   =  getHighest(x + offset, y - offset, k - 1);
				float down_left  =  getHighest(x - offset, y + offset, k - 1);
				float down_right =  getHighest(x + offset, y + offset, k - 1);
				hsmap[y * W + x].h[k] = max(max(up_left, up_right), max(down_left, down_right));
			}
	}
}

float Heightfield::getHighest(int x, int y, int k) const
{
	if (x < 0) x = 0;
	if (x > W - 1) x = W - 1;
	if (y < 0) y = 0;
	if (y > H - 1) y = H - 1;
	return hsmap[x + y * W].h[k];
}



