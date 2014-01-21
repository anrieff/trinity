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
#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "vector.h"
#include "scene.h"

enum {
	CAMERA_CENTER,
	CAMERA_LEFT,
	CAMERA_RIGHT,
};

class Camera: public SceneElement {
	// these internal vectors describe three of the ends of the imaginary
	// ray shooting screen
	Vector upLeft, upRight, downLeft;
	Vector frontDir, rightDir, upDir;
public:
	Vector pos; //!< position of the camera in 3D.
	double yaw; //!< Yaw angle in degrees (rot. around the Y axis, meaningful values: [0..360])
	double pitch; //!< Pitch angle in degrees (rot. around the X axis, meaningful values: [-90..90])
	double roll; //!< Roll angle in degrees (rot. around the Z axis, meaningful values: [-180..180])
	double fov; //!< The Field of view in degrees (meaningful values: [3..160])
	double aspect; //!< The aspect ratio of the camera frame. Should usually be frameWidth/frameHeight,
	double focalPlaneDist;
	double fNumber;
	bool dof; // on or off
	int numSamples;
	double discMultiplier;
	double stereoSeparation;
	
	Camera() { dof = false; fNumber = 1.0; focalPlaneDist = 1; numSamples = 25;
		stereoSeparation = 0; }
	
	// from SceneElement:
	void beginFrame(); //!< must be called before each frame. Computes the corner variables, needed for getScreenRay()
	ElementType getElementType() const { return ELEM_CAMERA; }
	
	void fillProperties(ParsedBlock& pb)
	{
		if (!pb.getVectorProp("pos", &pos))
			pb.requiredProp("pos");
		pb.getDoubleProp("aspect", &aspect, 1e-6);
		pb.getDoubleProp("fov", &fov, 0.0001, 179);
		pb.getDoubleProp("yaw", &yaw);
		pb.getDoubleProp("pitch", &pitch, -90, 90);
		pb.getDoubleProp("roll", &roll);
		pb.getDoubleProp("focalPlaneDist", &focalPlaneDist);
		pb.getDoubleProp("fNumber", &fNumber);
		pb.getBoolProp("dof", &dof);
		pb.getIntProp("numSamples", &numSamples);
		pb.getDoubleProp("stereoSeparation", &stereoSeparation);
		discMultiplier = 10.0 / fNumber;
	}
	
	/// generates a screen ray through a pixel (x, y - screen coordinates, not necessarily integer).
	/// if the camera parameter is present - offset the rays' start to the left or to the right,
	/// for use in stereoscopic rendering
	Ray getScreenRay(double x, double y, int camera = CAMERA_CENTER);
	
	void move(double dx, double dz);
	void rotate(double dx, double dz);
};

#endif // __CAMERA_H__
