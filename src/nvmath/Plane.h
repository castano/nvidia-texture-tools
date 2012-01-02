// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_MATH_PLANE_H
#define NV_MATH_PLANE_H

#include "nvmath.h"
#include "Vector.h"

namespace nv
{
    class Matrix;

    class NVMATH_CLASS Plane
    {
    public:
        typedef Plane const & Arg;

        Plane();
        Plane(float x, float y, float z, float w);
        Plane(Vector4::Arg v);
        Plane(Vector3::Arg v, float d);
        Plane(Vector3::Arg normal, Vector3::Arg point);

        const Plane & operator=(Plane::Arg v);

        Vector3 vector() const;
        float offset() const;

        const Vector4 & asVector() const;
        Vector4 & asVector();

        void operator*=(float s);

    private:
        Vector4 p;
    };

    Plane transformPlane(const Matrix&, Plane::Arg);

    Vector3 planeIntersection(Plane::Arg a, Plane::Arg b, Plane::Arg c);


} // nv namespace

#endif // NV_MATH_PLANE_H
