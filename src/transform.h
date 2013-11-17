#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "vector.h"
#include "matrix.h"
#include "util.h"

class Transform {

public:

    Transform() { reset(); }

    void reset() {
        transform = Matrix(1);
        inverseTransform = inverseMatrix(transform);
    }

    void scale(double X, double Y, double Z) {
        Matrix scaling(X);
        scaling.m[1][1] = Y;
        scaling.m[2][2] = Z;

        transform = transform * scaling;
        inverseTransform = inverseMatrix(scaling) * inverseTransform;
    }

    void rotate(double yaw, double pitch, double roll) {
        transform = transform *
                        rotationAroundX(toRadians(pitch)) *
                        rotationAroundY(toRadians(yaw)) *
                        rotationAroundZ(toRadians(roll));
        inverseTransform =
                        inverseMatrix(rotationAroundZ(toRadians(roll))) *
                        inverseMatrix(rotationAroundY(toRadians(yaw))) *
                        inverseMatrix(rotationAroundX(toRadians(pitch))) *
                        inverseTransform;
    }

    void translate(const Vector& V) {
        offset = V;
    }

    Vector point(Vector P) {
        P = P * transform;
        P = P + offset;

        return P;
    }

    Vector undoPoint(Vector P) {
        P = P - offset;
        P = P * inverseTransform;

        return P;
    }

    Vector direction(Vector dir) {
        return dir = dir * transform;
    }

    Vector undoDirection(Vector dir) {
        return dir = dir * inverseTransform;
    }

private:

    Matrix transform;
    Matrix inverseTransform;
    Vector offset;

};

#endif
