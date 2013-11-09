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
#include "sdl.h"
#include "matrix.h"
#include "camera.h"
#include "geometry.h"
#include "shading.h"
using namespace std;

Color vfb[VFB_MAX_SIZE][VFB_MAX_SIZE]; //!< virtual framebuffer

Camera* camera;

vector<Geometry*> geometries;
vector<Shader*> shaders;
vector<Node*> nodes;


Color raytrace(Ray ray)
{
	IntersectionData data;
	Node* closestNode = NULL;
	
	data.dist = 1e99;
	
	for (int i = 0; i < (int) nodes.size(); i++)
		if (nodes[i]->geom->intersect(ray, data))
			closestNode = nodes[i];

	if (!closestNode) return Color(0, 0, 0);
	
	return closestNode->shader->shade(ray, data);
}

bool testVisibility(const Vector& from, const Vector& to)
{
	Ray ray;
	ray.start = from;
	ray.dir = to - from;
	ray.dir.normalize();
	
	IntersectionData temp;
	temp.dist = (to - from).length();
	
	for (int i = 0; i < (int) nodes.size(); i++)
		if (nodes[i]->geom->intersect(ray, temp))
			return false;
	
	return true;
}


void createNode(Geometry* geometry, Shader* shader)
{
	geometries.push_back(geometry);
	shaders.push_back(shader);
	Node* node = new Node(geometry, shader);
	nodes.push_back(node);
}

void initializeScene(void)
{
	camera = new Camera;
	camera->yaw = 0;
	camera->pitch = -30;
	camera->roll = 0;
	camera->fov = 90;
	camera->aspect = 4. / 3.0;
	camera->pos = Vector(0,165,0);
	
	camera->beginFrame();
	
	lightPos = Vector(-90, 700, 350);
	lightColor = Color(1, 1, 1);
	lightPower = 350000;
	ambientLight = Color(0.12, 0.12, 0.12);
	
	Plane* plane = new Plane(-0.01);
	geometries.push_back(plane);
	
	Texture* texture = new BitmapTexture("data/floor.bmp", 0.005);
	Checker* checker = new Checker(Color(1, 1, 1), Color(0, 0, 0), 35);
	Lambert* lambert = new Lambert(Color(1, 1, 1), checker);
	Node* floor = new Node(plane, lambert);
	shaders.push_back(lambert);
	nodes.push_back(floor);

	Texture* world = new BitmapTexture("data/world.bmp", 2);
	
	Sphere* sphere = new Sphere(Vector(100, 50, 320), 50);
	Lambert* sphereshader = new Lambert(Color(1,1,1), world);
	createNode(sphere, sphereshader);
	
	
//	for (int i = 0; i < 3; i++)
//		createNode(new Cube(Vector(-100, 30, 256 - 50*i), 30), new Lambert(Color(1, 0, 0)));

	CsgOp* diff = new CsgDiff(
		new Cube(Vector(-100, 60, 200), 100),
		new Sphere(Vector(-100, 60, 200), 70)
	);
	
	createNode(diff, new Phong(Color(1,1, 0), 60, 1));

	for (int i = 0; i < 3; i++)
		createNode(new Sphere(Vector(100, 15, 256 - 50*i), 15), new Phong(Color(0, 0, 1), 80, 1));
}

bool needsAA[VFB_MAX_SIZE][VFB_MAX_SIZE];

inline bool tooDifferent(const Color& a, const Color& b)
{
	const float THRESHOLD = 0.1;
	return (fabs(a.r - b.r) > THRESHOLD ||
		     fabs(a.g - b.g) > THRESHOLD ||
		     fabs(a.b - b.b) > THRESHOLD);
}

void renderScene(void)
{
	const double kernel[5][2] = {
		{ 0, 0 },
		{ 0.3, 0.3 },
		{ 0.6, 0 },
		{ 0, 0.6 },
		{ 0.6, 0.6 },
	};
	
	int W = frameWidth();
	int H = frameHeight();
	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++)
			vfb[y][x] = raytrace(camera->getScreenRay(x, y));

	for (int y = 0; y < H; y++)
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
			
			for (int i = 0; i < 5; i++)
				if (tooDifferent(neighs[i], average)) {
					needsAA[y][x] = true;
					break;
				}
		}

	for (int y = 0; y < H; y++)
		for (int x = 0; x < W; x++)
			if (needsAA[y][x])
				vfb[y][x] = Color(1, 0, 0);

	/*
	for (int y = 0; y < frameHeight(); y++)
		for (int x = 0; x < frameWidth(); x++) {
			Color result(0, 0, 0);
			for (int i = 0; i < 5; i++)
				result += raytrace(camera->getScreenRay(x + kernel[i][0], y + kernel[i][1]));
			vfb[y][x] = result / 5.0f;
		}
	*/
}

int main(int argc, char** argv)
{
	if (!initGraphics(RESX, RESY)) return -1;
	initializeScene();
	Uint32 startTicks = SDL_GetTicks();
	renderScene();
	Uint32 renderTime = SDL_GetTicks() - startTicks;
	printf("Render time: %.2lf seconds.\n", renderTime/1000.0);
	displayVFB(vfb);
	waitForUserExit();
	closeGraphics();
	return 0;
}
