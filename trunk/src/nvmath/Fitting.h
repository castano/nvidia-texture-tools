// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_MATH_FITTING_H
#define NV_MATH_FITTING_H

#include "nvmath/nvmath.h"
#include "nvmath/Vector.h"
#include "nvmath/Plane.h"

namespace nv
{
    namespace Fit
    {
        Vector3 computeCentroid(int n, const Vector3 * points);
        Vector3 computeCentroid(int n, const Vector3 * points, const float * weights, Vector3::Arg metric);

        Vector3 computeCovariance(int n, const Vector3 * points, float * covariance);
        Vector3 computeCovariance(int n, const Vector3 * points, const float * weights, Vector3::Arg metric, float * covariance);

        Vector3 computePrincipalComponent(int n, const Vector3 * points);
        Vector3 computePrincipalComponent(int n, const Vector3 * points, const float * weights, Vector3::Arg metric);

        Plane bestPlane(int n, const Vector3 * points);

        // Returns number of clusters [1-4].
        int compute4Means(int n, const Vector3 * points, const float * weights, Vector3::Arg metric, Vector3 * cluster);
    }

} // nv namespace

#endif // NV_MATH_FITTING_H
