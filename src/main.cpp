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
#include "lights.h"
#include "cxxptl_sdl.h"
using namespace std;

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]; //!< virtual framebuffer
bool testVisibility(const Vector& from, const Vector& to);

/// traces a ray in the scene and returns the visible light that comes from that direction
Color raytrace(const Ray& ray)
{
	IntersectionData data;
	Node* closestNode = NULL;
	
	if (ray.depth > scene.settings.maxTraceDepth) return Color(0, 0, 0);

	if (ray.flags & RF_DEBUG)
		cout << "  Raytrace[start = " << ray.start << ", dir = " << ray.dir << "]\n";

	data.dist = 1e99;
	
	// find closest intersection point:
	for (int i = 0; i < (int) scene.nodes.size(); i++)
		if (scene.nodes[i]->intersect(ray, data))
			closestNode = scene.nodes[i];

	// check if the closest intersection point is actually a light:
	bool hitLight = false;
	Color hitLightColor;
	for (int i = 0; i < (int) scene.lights.size(); i++) {
		if (scene.lights[i]->intersect(ray, data.dist)) {
			hitLight = true;
			hitLightColor = scene.lights[i]->getColor();
		}
	}
	if (hitLight) return hitLightColor;

	// no intersection? use the environment, if present:
	if (!closestNode) {
		if (scene.environment != NULL) return scene.environment->getEnvironment(ray.dir);
		return Color(0, 0, 0);
	}
	
	if (ray.flags & RF_DEBUG) {
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

Color pathtrace(const Ray& ray, const Color& pathMultiplier, Random& rgen)
{
	IntersectionData data;
	Node* closestNode = NULL;
	
	if (ray.depth > scene.settings.maxTraceDepth) return Color(0, 0, 0);

	data.dist = 1e99;
	
	// find closest intersection point:
	for (int i = 0; i < (int) scene.nodes.size(); i++)
		if (scene.nodes[i]->intersect(ray, data))
			closestNode = scene.nodes[i];

	// check if the closest intersection point is actually a light:
	bool hitLight = false;
	Color hitLightColor;
	for (int i = 0; i < (int) scene.lights.size(); i++) {
		if (scene.lights[i]->intersect(ray, data.dist)) {
			hitLight = true;
			hitLightColor = scene.lights[i]->getColor();
		}
	}
	if (hitLight) {
		/*
		 * if the ray actually hit a light, check if we need to pass this light back along the path.
		 * If the last surface along the path was a diffuse one (Lambert/Phong), we need to discard the
		 * light contribution, since for diffuse material we do explicit light sampling too, thus the
		 * light would be over-represented and the image a bit too bright. We may discard light checks
		 * for secondary rays altogether, but we would lose caustics and light reflections that way.
		 */
		if (ray.flags & RF_DIFFUSE)
			return Color(0, 0, 0);
		else
			return hitLightColor * pathMultiplier;
	}
	// no intersection? use the environment, if present:
	if (!closestNode) {
		if (scene.environment != NULL)
			return scene.environment->getEnvironment(ray.dir) * pathMultiplier;
		return Color(0, 0, 0);
	}
	
	Color resultDirect(0, 0, 0);
	
	// We continue building the path in two ways:
	// 1) (a.k.a. "direct illumination"): connect the current path end to a random light.
	//    This approximates the direct lighting towards the intersection point.
	if (!scene.lights.empty()) {
		// choose a random light:
		int lightIndex = rgen.randint(0, scene.lights.size() - 1);
		Light* light = scene.lights[lightIndex];
		int numLightSamples = light->getNumSamples();

		// choose a random sample of that light:
		int lightSampleIdx = rgen.randint(0, numLightSamples - 1);

		// sample the light and see if it came out nonzero:
		Vector pointOnLight;
		Color lightColor;
		light->getNthSample(lightSampleIdx, data.p, pointOnLight, lightColor);
		if (lightColor.intensity() > 0 && testVisibility(data.p + data.normal * 1e-6, pointOnLight)) {
			// w_out - the outgoing ray in the BRDF evaluation
			Ray w_out;
			w_out.start = data.p + data.normal * 1e-6;
			w_out.dir = pointOnLight - w_out.start;
			w_out.dir.normalize();
			//
			// calculate the light contribution in a manner, consistent with classic path tracing:
			float solidAngle = light->solidAngle(w_out.start); // solid angle of the light, as seen from x.
			// evaluate the BRDF:
			Color brdfAtPoint = closestNode->shader->eval(data, ray, w_out); 
			
			lightColor = light->getColor() * solidAngle / (2*PI);
			
			// the probability to choose a particular light among all lights: 1/N
			float pdfChooseLight = 1.0f / (float) scene.lights.size();
			// the probability to shoot a ray in a random direction: 1/2*pi
			float pdfInLight = 1 / (2*PI);
			
			// combined probability for that ray:
			float pdf = pdfChooseLight * pdfInLight;
			
			if (brdfAtPoint.intensity() > 0)
				// Kajia's rendering equation, evaluated at a single incoming/outgoing directions pair:
				                /* Li */    /*BRDFs@path*/    /*BRDF*/   /*ray probability*/
				resultDirect = lightColor * pathMultiplier * brdfAtPoint / pdf; 
		}
	}

	// 2) (a.k.a. "indirect illumination"): continue the path randomly, by asking the
	//    BRDF to choose a continuation direction
	Ray w_out;
	Color brdfEval; // brdf at the chosen direction
	float pdf; // the probability to choose that specific newRay
	// sample the BRDF:
	closestNode->shader->spawnRay(data, ray, w_out, brdfEval, pdf);
	
	if (pdf < 0) return Color(1, 0, 0);  // bogus BRDF; mark in red
	if (pdf == 0) return Color(0, 0, 0);  // terminate the path, as required
	Color resultGi;
	resultGi = pathtrace(w_out, pathMultiplier * brdfEval / pdf, rgen); // continue the path normally; accumulate the new term to the BRDF product
	
	return resultDirect + resultGi;
}


/// checks for visibility between points `from' and `to'
/// (from is assumed to be near a surface, whereas to is near a light)
bool testVisibility(const Vector& from, const Vector& to)
{
	Ray ray;
	ray.start = from;
	ray.dir = to - from;
	ray.dir.normalize();
	ray.flags |= RF_SHADOW;
	
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

// combine the results from the "left" and "right" camera for a single pixel.
// the implementation here creates an anaglyph image: it desaturates the input
// colors, masks them (left=red, right=cyan) and then merges them.
static inline Color combineStereo(Color left, Color right)
{
	left.adjustSaturation(0.25f);
	right.adjustSaturation(0.25f);
	return left * Color(1, 0, 0) + right * Color(0, 1, 1);
}

// trace a ray through pixel coords (x, y).
Color renderSample(double x, double y, int dx = 1, int dy = 1)
{
	if (scene.camera->dof) {
		Color average(0, 0, 0);
		Random& R = getRandomGen();
		for (int i = 0; i < scene.camera->numSamples; i++) {
			if (scene.camera->stereoSeparation == 0) // stereoscopic rendering?
				average += raytrace(scene.camera->getScreenRay(x + R.randdouble() * dx, y + R.randdouble() * dy));
			else {
				average += combineStereo(
					raytrace(scene.camera->getScreenRay(x + R.randdouble() * dx, y + R.randdouble() * dy, CAMERA_LEFT)),
					raytrace(scene.camera->getScreenRay(x + R.randdouble() * dx, y + R.randdouble() * dy, CAMERA_RIGHT))
				);
			}
		}
		return average / scene.camera->numSamples;
	} else if (scene.settings.gi) {
		Color average(0, 0, 0);
		Random& R = getRandomGen();
		for (int i = 0; i < scene.settings.numPaths; i++) {
			average += pathtrace(
				scene.camera->getScreenRay(x + R.randdouble() * dx, y + R.randdouble() * dy),
				Color(1, 1, 1),
				R
			);
		}
		return average / scene.settings.numPaths;
	} else {
		if (scene.camera->stereoSeparation == 0)
			return raytrace(scene.camera->getScreenRay(x, y));
		else
			// trace one ray through the left camera and one ray through the right, then combine the results	
			return combineStereo(
				raytrace(scene.camera->getScreenRay(x, y, CAMERA_LEFT)),
				raytrace(scene.camera->getScreenRay(x, y, CAMERA_RIGHT))
			);
	}
}

// gets the color for a single pixel, without antialiasing
Color renderPixelNoAA(int x, int y, int dx = 1, int dy = 1)
{
	vfb[y][x] = renderSample(x, y, dx, dy);
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

class TaskNoAA: public Parallel
{
	const vector<Rect>& buckets;
	InterlockedInt counter;
public:
	TaskNoAA(const vector<Rect>& buckets): buckets(buckets), counter(0)
	{
	}
	
	void entry(int thread_index, int thread_count)
	{
		// first pass: shoot just one ray per pixel
		int i;
		while ((i = counter++) < (int) buckets.size()) {
			const Rect& r = buckets[i];
			for (int y = r.y0; y < r.y1; y++)
				for (int x = r.x0; x < r.x1; x++)
					renderPixelNoAA(x, y);
			if (!scene.settings.interactive)
				if (!displayVFBRect(r, vfb))
					return;
		}
		
	}
};

class TaskAA: public Parallel {
	const vector<Rect>& buckets;
	InterlockedInt counter;
public:
	TaskAA(const vector<Rect>& buckets): buckets(buckets), counter(0)
	{
	}

	void entry(int threadIndex, int threadCount ) {
		int i;
		while ((i = counter++) < (int) buckets.size()) {
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
};


void renderScene(void)
{
	int W = frameWidth();
	int H = frameHeight();
	
	std::vector<Rect> buckets = getBucketsList();
	if (scene.settings.wantPrepass || scene.settings.gi) {
		// We render the whole screen in three passes.
		// 1) First pass - use very coarse resolution rendering, tracing a single ray for a 16x16 block:
		for (size_t i = 0; i < buckets.size(); i++) {
			Rect& r = buckets[i];
			for (int dy = 0; dy < r.h; dy += 16) {
				int ey = min(r.h, dy + 16);
				for (int dx = 0; dx < r.w; dx += 16) {
					int ex = min(r.w, dx + 16);
					Color c = renderPixelNoAA(r.x0 + dx, r.y0 + dy, ex - dx, ey - dy);
					if (!drawRect(Rect(r.x0 + dx, r.y0 + dy, r.x0 + ex, r.y0 + ey), c))
						return;
				}
			}
		}
	}

	static ThreadPool pool;
	TaskNoAA task1(buckets);
	pool.run(&task1, scene.settings.numThreads);

	if (scene.settings.wantAA && !scene.camera->dof && !scene.settings.gi) {
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
		if (scene.settings.wantAA && !scene.camera->dof) {
			TaskAA task2(buckets);
			pool.run(&task2, scene.settings.numThreads);
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
	ray.flags |= RF_DEBUG;
	if (scene.settings.gi)
		pathtrace(ray, Color(1, 1, 1), getRandomGen());
	else
		raytrace(ray);
	printf("Raytracing completed!\n");
}

const char* defaultScene = "data/boxed.trinity";

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

// handles the keyboard and mouse events in interactive mode
// the `dt' param is the last frame render time, in seconds.
void handleKbdMouse(bool& running, double dt)
{
	if (!running) return;
	static bool fastMotion = false;
	SDL_Event ev;
	// handle key presses
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
			case SDL_QUIT:
				running = false;
				return;
			case SDL_KEYDOWN:
			{
				switch (ev.key.keysym.sym) {
					case SDLK_ESCAPE:
						running = false;
						return;
					case 'p':
					{
						// an utility function, that tells us where the camera is.
						printf("Camera position: (%.3lf, %.3lf, %.3lf)\n", scene.camera->pos.x, scene.camera->pos.y, scene.camera->pos.z);
						printf("   yaw: %.3lf\n", scene.camera->yaw);
						printf(" pitch: %.3lf\n", scene.camera->pitch);
						printf("  roll: %.3lf\n", scene.camera->roll);
						break;
					}
					case 'r':
					{
						// activate/deactivate running (camera movement is 4x faster when running)
						fastMotion = !fastMotion;
						break;
					}
					default:
						break;
				}
				break;
			}
		}
	}
	const double KEYBOARD_SENSITIVITY = 5.0;
	const double MOUSE_SENSITIVITY = 0.05;
	Uint8 *keystate;
	int deltax, deltay;
	// fetch currently pressed keys:
	keystate = SDL_GetKeyState(NULL);
	double M = dt * (fastMotion ? 200 : 50);
	double R = dt * KEYBOARD_SENSITIVITY;
	// handle arrow keys (camera movement)
	if (keystate[SDLK_UP	]) scene.camera->move(0, +M);
	if (keystate[SDLK_DOWN	]) scene.camera->move(0, -M);
	if (keystate[SDLK_LEFT	]) scene.camera->move(-M, 0);
	if (keystate[SDLK_RIGHT	]) scene.camera->move(+M, 0);
	// handle keypad keys (camera lookaround)
	if (keystate[SDLK_KP2	]) scene.camera->rotate(0, -R);
	if (keystate[SDLK_KP4	]) scene.camera->rotate(+R, 0);
	if (keystate[SDLK_KP6	]) scene.camera->rotate(-R, 0);
	if (keystate[SDLK_KP8	]) scene.camera->rotate(0, +R);
	
	// handle mouse movement (camera lookaround)
	SDL_GetRelativeMouseState(&deltax, &deltay);
	scene.camera->rotate(-MOUSE_SENSITIVITY * deltax, -MOUSE_SENSITIVITY * deltay);
}

// a "main loop", that runs the interactive mode
void mainloop(void)
{
	if (scene.settings.fullscreen) SDL_ShowCursor(0); // hide the cursor in fullscreen mode
	int framesRendered = 0;
	Uint32 ticksStart = SDL_GetTicks();
	bool running = true;
	while (running) {
		Uint32 frameTicks = SDL_GetTicks(); // record how much time the frame took
		scene.beginFrame();
		renderScene();   // render
		framesRendered++;
		displayVFB(vfb); // display to user
		// determine how much time we spent in rendering...
		double renderTime = (SDL_GetTicks() - frameTicks) / 1000.0;
		// ... and use it when calculating camera movements, etc.
		handleKbdMouse(running, renderTime);
	}
	// calculate average FPS
	Uint32 ticks = SDL_GetTicks() - ticksStart;
	printf("%d frames for %u ms, avg. framerate: %.2f FPS.\n", framesRendered,
		   (unsigned) ticks, framesRendered * 1000.0f / ticks);
}

int main(int argc, char** argv)
{
	if (!parseCmdLine(argc, argv)) return 0;
	initRandom((Uint32) time(NULL));
	initColor();
	if (!scene.parseScene(defaultScene)) {
		printf("Could not parse the scene!\n");
		return -1;
	}
	if (scene.settings.numThreads == 0)
		scene.settings.numThreads = get_processor_count();
	if (scene.settings.interactive) 
		scene.settings.wantAA = scene.settings.wantPrepass = false;
	bool fullscreen = scene.settings.interactive && scene.settings.fullscreen;
	
	if (!initGraphics(scene.settings.frameWidth, scene.settings.frameHeight, fullscreen)) return -1;
	scene.beginRender();
	if (scene.settings.interactive) {
		mainloop();
	} else {
		Uint32 startTicks = SDL_GetTicks();
		renderScene_Threaded();
		float renderTime = (SDL_GetTicks() - startTicks) / 1000.0f;
		printf("Render time: %.2f seconds.\n", renderTime);
		setWindowCaption("trinity: rendertime: %.2fs", renderTime);
		displayVFB(vfb);
		waitForUserExit();
	}
	closeGraphics();
	return 0;
}
