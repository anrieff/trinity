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
	
	lightPos = Vector(-30, 700, 250);
	lightColor = Color(1, 1, 1);
	lightPower = 350000;
	
	Plane* plane = new Plane(2);
	geometries.push_back(plane);
	
	Checker* checker = new Checker(Color(0, 0, 0), Color(0, 0.5, 1), 5);
	Lambert* lambert = new Lambert(Color(1, 1, 1), checker);
	Node* floor = new Node(plane, lambert);
	shaders.push_back(lambert);
	nodes.push_back(floor);
	
	for (int i = 0; i < 3; i++)
		createNode(new Sphere(Vector(-100, 15, 256 - 50*i), 15), new Lambert(Color(1, 0, 0)));

	for (int i = 0; i < 3; i++)
		createNode(new Sphere(Vector(100, 15, 256 - 50*i), 15), new Phong(Color(0, 0, 1), 80, 1));
} 

void renderScene(void)
{
	for (int y = 0; y < frameHeight(); y++)
		for (int x = 0; x < frameWidth(); x++) {
			Ray ray = camera->getScreenRay(x, y);
			vfb[y][x] = raytrace(ray);
		}
}

int main(int argc, char** argv)
{
	if (!initGraphics(RESX, RESY)) return -1;
	initializeScene();
	renderScene();
	displayVFB(vfb);
	waitForUserExit();
	closeGraphics();
	return 0;
}
