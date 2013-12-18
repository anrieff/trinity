#include "lights.h"
#include "random_generator.h"


int PointLight::getNumSamples()
{
	return 1;
}

void PointLight::getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color)
{
	samplePos = pos;
	color = this->color * this->power;
}


int RectLight::getNumSamples()
{
	return xSubd * ySubd;
}

void RectLight::getNthSample(int sampleIdx, const Vector& shadePos, Vector& samplePos, Color& color)
{
	Random& R = getRandomGen();
	
	Vector shadePosCanonical = T.undoPoint(shadePos);
	
	if (shadePosCanonical.y > 0) {
		color.makeZero();
		return;
	}
	
	float sx = (sampleIdx % xSubd + R.randfloat()) / xSubd;
	float sy = (sampleIdx / xSubd + R.randfloat()) / ySubd;
	
	Vector sampleCanonical(sx - 0.5, 0, sy - 0.5);
	samplePos = T.point(sampleCanonical);
	color = this->color * this->power;
}
