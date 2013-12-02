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
#include <vector>
#include <iostream>
#include "sdl.h"
#include "matrix.h"
#include "camera.h"
#include "geometry.h"
#include "shading.h"
#include "environment.h"
#include "mesh.h"
#include "random_generator.h"
#include "scene.h"
using namespace std;

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]; //!< virtual framebuffer

/// traces a ray in the scene and returns the visible light that comes from that direction
Color raytrace(Ray ray)
{
	IntersectionData data;
	Node* closestNode = NULL;
	
	if (ray.depth > scene.settings.maxTraceDepth) return Color(0, 0, 0);

	if (ray.debug)
		cout << "  Raytrace[start = " << ray.start << ", dir = " << ray.dir << "]\n";

	data.dist = 1e99;
	
	// find closest intersection point:
	for (int i = 0; i < (int) scene.nodes.size(); i++)
		if (scene.nodes[i]->intersect(ray, data))
			closestNode = scene.nodes[i];

	// no intersection? use the environment, if present:
	if (!closestNode) {
		if (scene.environment != NULL) return scene.environment->getEnvironment(ray.dir);
		return Color(0, 0, 0);
	}
	
	if (ray.debug) {
		cout << "    Hit " << closestNode->geom->getName() << " at distance " << fixed << setprecision(2) << data.dist << endl;
		cout << "      Intersection point: " << data.p << endl;
		cout << "      Normal:             " << data.normal << endl;
		cout << "      UV coods:           " << data.u << ", " << data.v << endl;
	}
	
	// if the node we hit has a bump map, apply it here:
	if (closestNode->bump)
		closestNode->bump->modifyNormal(data);
	
	// use the shader of the closest node to shade the intersection:
	return closestNode->shader->shade(ray, data);
}

/// checks for visibility between points `from' and `to'
/// (from is assumed to be near a surface, whereas to is near a light)
bool testVisibility(const Vector& from, const Vector& to)
{
	Ray ray;
	ray.start = from;
	ray.dir = to - from;
	ray.dir.normalize();
	
	IntersectionData temp;
	temp.dist = (to - from).length();
	
	// if there's any obstacle between from and to, the points aren't visible.
	// we can stop at the first such object, since we don't care about the distance.
	for (int i = 0; i < (int) scene.nodes.size(); i++)
		if (scene.nodes[i]->intersect(ray, temp))
			return false;
	
	return true;
}

bool needsAA[VFB_MAX_SIZE][VFB_MAX_SIZE];

/// checks if two colors are "too different":
inline bool tooDifferent(const Color& a, const Color& b)
{
	const float THRESHOLD = 0.1; // max color threshold; if met on any of the three channels, consider the colors too different
	for (int comp = 0; comp < 3; comp++) {
		float theMax = max(a[comp], b[comp]);
		float theMin = min(a[comp], b[comp]);

		// compare a single channel of the two colors. If the difference between them is large,
		// but they aren't overexposed, the difference will be visible: needs anti-aliasing.
		if (theMax - theMin > THRESHOLD && theMin < 1.33f) 
			return true;
	}
	return false;
}

// trace a ray through pixel coords (x, y).
Color renderSample(double x, double y)
{
	return raytrace(scene.camera->getScreenRay(x, y));
}

// gets the color for a single pixel, without antialiasing
Color renderPixelNoAA(int x, int y)
{
	vfb[y][x] = renderSample(x, y);
	return vfb[y][x];
}

// gets the color for a single pixel, with antialiasing. Assumes the pixel
// already holds some value.
// This simply adds four more AA samples and averages the result.
Color renderPixelAA(int x, int y)
{
	const double kernel[5][2] = {
		{ 0, 0 },
		{ 0.3, 0.3 },
		{ 0.6, 0 },
		{ 0, 0.6 },
		{ 0.6, 0.6 },
	};
	Color accum = vfb[y][x];
	for (int samples = 1; samples < 5; samples++) {
		accum += renderSample(x + kernel[samples][0], y + kernel[samples][1]);
	}
	vfb[y][x] = accum / 5;
	return vfb[y][x];
}


void renderScene(void)
{
	int W = frameWidth();
	int H = frameHeight();
	
	std::vector<Rect> buckets = getBucketsList();
	if (scene.settings.wantPrepass) {
		// We render the whole screen in three passes.
		// 1) First pass - use very coarse resolution rendering, tracing a single ray for a 16x16 block:
		for (size_t i = 0; i < buckets.size(); i++) {
			Rect& r = buckets[i];
			for (int dy = 0; dy < r.h; dy += 16) {
				int ey = min(r.h, dy + 16);
				for (int dx = 0; dx < r.w; dx += 16) {
					int ex = min(r.w, dx + 16);
					Color c = renderPixelNoAA(r.x0 + dx + (ex - dx) / 2, r.y0 + dy + (ey - dy) / 2);
					if (!drawRect(Rect(r.x0 + dx, r.y0 + dy, r.x0 + ex, r.y0 + ey), c))
						return;
				}
			}
		}
	}

	
	// first pass: shoot just one ray per pixel
	for (size_t i = 0; i < buckets.size(); i++) {
		const Rect& r = buckets[i];
		for (int y = r.y0; y < r.y1; y++)
			for (int x = r.x0; x < r.x1; x++)
				renderPixelNoAA(x, y);
		if (!displayVFBRect(r, vfb))
			return;
	}

	if (scene.settings.wantAA) {
		// second pass: find pixels, that need anti-aliasing, by analyzing their neighbours
		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				Color neighs[5];
				neighs[0] = vfb[y][x];
				
				neighs[1] = vfb[y][x     > 0 ? x - 1 : x];
				neighs[2] = vfb[y][x + 1 < W ? x + 1 : x];

				neighs[3] = vfb[y     > 0 ? y - 1 : y][x];
				neighs[4] = vfb[y + 1 < H ? y + 1 : y][x];
				
				Color average(0, 0, 0);
				
				for (int i = 0; i < 5; i++)
					average += neighs[i];
				average /= 5.0f;
				
				for (int i = 0; i < 5; i++) {
					if (tooDifferent(neighs[i], average)) {
						needsAA[y][x] = true;
						break;
					}
				}
			}
		}
	}

	bool previewAA = false; // change to true to make it just display which pixels are selected for anti-aliasing
	
	if (previewAA) {
		for (int y = 0; y < H; y++)
			for (int x = 0; x < W; x++)
				if (needsAA[y][x])
					vfb[y][x] = Color(1, 0, 0);
	} else {
		/* 
		 * A third pass, shooting additional rays for pixels that need them.
		 * Note that all pixels already are sampled with a ray at offset (0, 0),
		 * which coincides with sample #0 of our antialiasing kernel. So, instead
		 * of shooting five rays (the kernel size), we can just shoot the remaining
		 * four rays, adding with what we currently have in the pixel, and average
		 * after that.
		 */
		if (scene.settings.wantAA) {
			for (size_t i = 0; i < buckets.size(); i++) {
				const Rect& r = buckets[i];
				if (!markRegion(r))
					return;
				for (int y = r.y0; y < r.y1; y++)
					for (int x = r.x0; x < r.x1; x++)
						if (needsAA[y][x]) renderPixelAA(x, y);
				if (!displayVFBRect(r, vfb))
					return;
			}
		}
	}
}

int renderSceneThread(void* /*unused*/)
{
	scene.beginFrame();
	renderScene();
	rendering = false;
	return 0;
}

/// handle a mouse click
void handleMouse(SDL_MouseButtonEvent *mev)
{
	if (mev->button != 1) return; // only consider the left mouse button
	printf("Mouse click from (%d, %d)\n", (int) mev->x, (int) mev->y);
	Ray ray = scene.camera->getScreenRay(mev->x, mev->y);
	ray.debug = true;
	raytrace(ray);
	printf("Raytracing completed!\n");
}

const char* defaultScene = "data/testmesh.trinity";

static bool parseCmdLine(int argc, char** argv)
{
	if (argc < 2) return true;
	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printf("Usage: retrace [scenefile]\n");
		return false;
	}
	defaultScene = argv[1];
	return true;
}

int main(int argc, char** argv)
{
	if (!parseCmdLine(argc, argv)) return 0;
	initRandom((Uint32) time(NULL));
	if (!scene.parseScene(defaultScene)) {
		printf("Could not parse the scene!\n");
		return -1;
	}
	if (!initGraphics(scene.settings.frameWidth, scene.settings.frameHeight)) return -1;
	scene.beginRender();
	Uint32 startTicks = SDL_GetTicks();
	renderScene_Threaded();
	float renderTime = (SDL_GetTicks() - startTicks) / 1000.0f;
	printf("Render time: %.2f seconds.\n", renderTime);
	setWindowCaption("trinity: rendertime: %.2fs", renderTime);
	displayVFB(vfb);
	waitForUserExit();
	closeGraphics();
	return 0;
}
