// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_MATH_FITTING_H
#define NV_MATH_FITTING_H

#include "nvmath.h"

namespace nv
{
    class Vector3;
    class Plane;

    namespace Fit
    {
        Vector3 computeCentroid(int n, const Vector3 * points);
        Vector3 computeCentroid(int n, const Vector3 * points, const float * weights, const Vector3 & metric);

        Vector3 computeCovariance(int n, const Vector3 * points, float * covariance);
        Vector3 computeCovariance(int n, const Vector3 * points, const float * weights, const Vector3 & metric, float * covariance);

        Vector3 computePrincipalComponent(int n, const Vector3 * points);
        Vector3 computePrincipalComponent(int n, const Vector3 * points, const float * weights, const Vector3 & metric);

        Plane bestPlane(int n, const Vector3 * points);

        // Returns number of clusters [1-4].
        int compute4Means(int n, const Vector3 * points, const float * weights, const Vector3 & metric, Vector3 * cluster);
    }

} // nv namespace

#endif // NV_MATH_FITTING_H
