// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "Fitting.h"
#include "Vector.inl"
#include "Plane.inl"

#include "nvcore/Utils.h" // max, swap

#include <float.h> // FLT_MAX

using namespace nv;

// @@ Move to EigenSolver.h

// @@ We should be able to do something cheaper...
static Vector3 estimatePrincipleComponent(const float * __restrict matrix)
{
	const Vector3 row0(matrix[0], matrix[1], matrix[2]);
	const Vector3 row1(matrix[1], matrix[3], matrix[4]);
	const Vector3 row2(matrix[2], matrix[4], matrix[5]);

	float r0 = lengthSquared(row0);
	float r1 = lengthSquared(row1);
	float r2 = lengthSquared(row2);

	if (r0 > r1 && r0 > r2) return row0;
	if (r1 > r2) return row1;
	return row2;
}


static inline Vector3 firstEigenVector_PowerMethod(const float *__restrict matrix)
{
    if (matrix[0] == 0 && matrix[3] == 0 && matrix[5] == 0)
    {
        return Vector3(0.0f);
    }

    Vector3 v = estimatePrincipleComponent(matrix);

    const int NUM = 8;
    for (int i = 0; i < NUM; i++)
    {
        float x = v.x * matrix[0] + v.y * matrix[1] + v.z * matrix[2];
        float y = v.x * matrix[1] + v.y * matrix[3] + v.z * matrix[4];
        float z = v.x * matrix[2] + v.y * matrix[4] + v.z * matrix[5];

        float norm = max(max(x, y), z);

        v = Vector3(x, y, z) / norm;
    }

    return v;
}


Vector3 nv::Fit::computeCentroid(int n, const Vector3 *__restrict points)
{
    Vector3 centroid(0.0f);

    for (int i = 0; i < n; i++)
    {
        centroid += points[i];
    }
    centroid /= float(n);

    return centroid;
}

Vector3 nv::Fit::computeCentroid(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric)
{
    Vector3 centroid(0.0f);
    float total = 0.0f;

    for (int i = 0; i < n; i++)
    {
        total += weights[i];
        centroid += weights[i]*points[i];
    }
    centroid /= total;

    return centroid;
}


Vector3 nv::Fit::computeCovariance(int n, const Vector3 *__restrict points, float *__restrict covariance)
{
    // compute the centroid
    Vector3 centroid = computeCentroid(n, points);

    // compute covariance matrix
    for (int i = 0; i < 6; i++)
    {
        covariance[i] = 0.0f;
    }

    for (int i = 0; i < n; i++)
    {
        Vector3 v = points[i] - centroid;

        covariance[0] += v.x * v.x;
        covariance[1] += v.x * v.y;
        covariance[2] += v.x * v.z;
        covariance[3] += v.y * v.y;
        covariance[4] += v.y * v.z;
        covariance[5] += v.z * v.z;
    }

    return centroid;
}

Vector3 nv::Fit::computeCovariance(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric, float *__restrict covariance)
{
    // compute the centroid
    Vector3 centroid = computeCentroid(n, points, weights, metric);

    // compute covariance matrix
    for (int i = 0; i < 6; i++)
    {
        covariance[i] = 0.0f;
    }

    for (int i = 0; i < n; i++)
    {
        Vector3 a = (points[i] - centroid) * metric;
        Vector3 b = weights[i]*a;

        covariance[0] += a.x * b.x;
        covariance[1] += a.x * b.y;
        covariance[2] += a.x * b.z;
        covariance[3] += a.y * b.y;
        covariance[4] += a.y * b.z;
        covariance[5] += a.z * b.z;
    }

    return centroid;
}

Vector3 nv::Fit::computePrincipalComponent(int n, const Vector3 *__restrict points)
{
    float matrix[6];
    computeCovariance(n, points, matrix);

    return firstEigenVector_PowerMethod(matrix);
}

Vector3 nv::Fit::computePrincipalComponent(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric)
{
    float matrix[6];
    computeCovariance(n, points, weights, metric, matrix);

    return firstEigenVector_PowerMethod(matrix);
}


Plane nv::Fit::bestPlane(int n, const Vector3 *__restrict points)
{
    // compute the centroid and covariance
    float matrix[6];
    Vector3 centroid = computeCovariance(n, points, matrix);

    if (matrix[0] == 0 || matrix[3] == 0 || matrix[5] == 0)
    {
        // If no plane defined, then return a horizontal plane.
        return Plane(Vector3(0, 0, 1), centroid);
    }

#pragma NV_MESSAGE("TODO: need to write an eigensolver!")

    // - Numerical Recipes in C is a good reference. Householder transforms followed by QL decomposition seems to be the best approach.
    // - The one from magic-tools is now LGPL. For the 3D case it uses a cubic root solver, which is not very accurate.
    // - Charles' Galaxy3 contains an implementation of the tridiagonalization method, but is under BPL.

    //EigenSolver3 solver(matrix);

    return Plane();
}


int nv::Fit::compute4Means(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric, Vector3 *__restrict cluster)
{
    // Compute principal component.
    float matrix[6];
    Vector3 centroid = computeCovariance(n, points, weights, metric, matrix);
    Vector3 principal = firstEigenVector_PowerMethod(matrix);

    // Pick initial solution.
    int mini, maxi;
    mini = maxi = 0;

    float mindps, maxdps;
    mindps = maxdps = dot(points[0] - centroid, principal);

    for (int i = 1; i < n; ++i)
    {
        float dps = dot(points[i] - centroid, principal);

        if (dps < mindps) {
            mindps = dps;
            mini = i;
        }
        else {
            maxdps = dps;
            maxi = i;
        }
    }

    cluster[0] = centroid + mindps * principal;
    cluster[1] = centroid + maxdps * principal;
    cluster[2] = (2 * cluster[0] + cluster[1]) / 3;
    cluster[3] = (2 * cluster[1] + cluster[0]) / 3;

    // Now we have to iteratively refine the clusters.
    while (true)
    {
        Vector3 newCluster[4] = { Vector3(0.0f), Vector3(0.0f), Vector3(0.0f), Vector3(0.0f) };
        float total[4] = {0, 0, 0, 0};

        for (int i = 0; i < n; ++i)
        {
            // Find nearest cluster.
            int nearest = 0;
            float mindist = FLT_MAX;
            for (int j = 0; j < 4; j++)
            {
                float dist = lengthSquared((cluster[j] - points[i]) * metric);
                if (dist < mindist)
                {
                    mindist = dist;
                    nearest = j;
                }
            }

            newCluster[nearest] += weights[i] * points[i];
            total[nearest] += weights[i];
        }

        for (int j = 0; j < 4; j++)
        {
            if (total[j] != 0)
                newCluster[j] /= total[j];
        }

        if (equal(cluster[0], newCluster[0]) && equal(cluster[1], newCluster[1]) && 
            equal(cluster[2], newCluster[2]) && equal(cluster[3], newCluster[3]))
        {
            return (total[0] != 0) + (total[1] != 0) + (total[2] != 0) + (total[3] != 0);
        }

        cluster[0] = newCluster[0];
        cluster[1] = newCluster[1];
        cluster[2] = newCluster[2];
        cluster[3] = newCluster[3];

        // Sort clusters by weight.
        for (int i = 0; i < 4; i++)
        {
            for (int j = i; j > 0 && total[j] > total[j - 1]; j--)
            {
                swap( total[j], total[j - 1] );
                swap( cluster[j], cluster[j - 1] );
            }
        }
    }
}

