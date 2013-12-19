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
	
	// convert the shade point onto the light's canonic space:
	Vector shadePosCanonical = T.undoPoint(shadePos);
	
	// shade point "behind" the lamp?
	if (shadePosCanonical.y > 0) {
		color.makeZero();
		return;
	}
	
	// stratified sampling:
	float sx = (sampleIdx % xSubd + R.randfloat()) / xSubd;
	float sy = (sampleIdx / xSubd + R.randfloat()) / ySubd;
	
	Vector sampleCanonical(sx - 0.5, 0, sy - 0.5);
	samplePos = T.point(sampleCanonical);
	Vector shadePos_LS = shadePosCanonical - sampleCanonical;
	// return light color, attenuated by the angle of incidence
	// (the cosine between the light's direction and the normed ray toward the hitpos)
	color = this->color * (this->power * float(dot(Vector(0, -1, 0), shadePos_LS) / shadePos_LS.length()));
}
