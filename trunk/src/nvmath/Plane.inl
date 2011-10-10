// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_MATH_PLANE_INL
#define NV_MATH_PLANE_INL

#include "Plane.h"
#include "Vector.inl"

namespace nv
{
    inline Plane::Plane() {}
    inline Plane::Plane(float x, float y, float z, float w) : p(x, y, z, w) {}
    inline Plane::Plane(Vector4::Arg v) : p(v) {}
    inline Plane::Plane(Vector3::Arg v, float d) : p(v, d) {}
    inline Plane::Plane(Vector3::Arg normal, Vector3::Arg point) : p(normal, dot(normal, point)) {}

    inline const Plane & Plane::operator=(Plane::Arg v) { p = v.p; return *this; }

    inline Vector3 Plane::vector() const { return p.xyz(); }
    inline scalar Plane::offset() const { return p.w; }

    inline const Vector4 & Plane::asVector() const { return p; }
    inline Vector4 & Plane::asVector() { return p; }

    // Normalize plane.
    inline Plane normalize(Plane::Arg plane, float epsilon = NV_EPSILON)
    {
        const float len = length(plane.vector());
        nvDebugCheck(!isZero(len, epsilon));
        const float inv = 1.0f / len;
        return Plane(plane.asVector() * inv);
    }

    // Get the signed distance from the given point to this plane.
    inline float distance(Plane::Arg plane, Vector3::Arg point)
    {
        return dot(plane.vector(), point) - plane.offset();
    }

    inline void Plane::operator*=(scalar s)
    {
        scale(p, s);
    }

} // nv namespace

#endif // NV_MATH_PLANE_H
