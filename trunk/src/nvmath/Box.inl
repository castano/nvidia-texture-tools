// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_BOX_INL
#define NV_MATH_BOX_INL

#include "Box.h"
#include "Vector.inl"

#include <float.h> // FLT_MAX

namespace nv
{
    // Default ctor.
    Box::Box() { };

    // Copy ctor.
    Box::Box(const Box & b) : minCorner(b.minCorner), maxCorner(b.maxCorner) { }

    // Init ctor.
    Box::Box(const Vector3 & mins, const Vector3 & maxs) : minCorner(mins), maxCorner(maxs) { }

    // Assignment operator.
    Box & Box::operator=(const Box & b) { minCorner = b.minCorner; maxCorner = b.maxCorner; return *this; }

    // Clear the bounds.
    void Box::clearBounds()
    {
        minCorner.set(FLT_MAX, FLT_MAX, FLT_MAX);
        maxCorner.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    // Build a cube centered on center and with edge = 2*dist
    void Box::cube(const Vector3 & center, float dist)
    {
        setCenterExtents(center, Vector3(dist, dist, dist));
    }

    // Build a box, given center and extents.
    void Box::setCenterExtents(const Vector3 & center, const Vector3 & extents)
    {
        minCorner = center - extents;
        maxCorner = center + extents;
    }

    // Get box center.
    Vector3 Box::center() const
    {
        return (minCorner + maxCorner) * 0.5f;
    }

    // Return extents of the box.
    Vector3 Box::extents() const
    {
        return (maxCorner - minCorner) * 0.5f;
    }

    // Return extents of the box.
    float Box::extents(uint axis) const
    {
        nvDebugCheck(axis < 3);
        if (axis == 0) return (maxCorner.x - minCorner.x) * 0.5f;
        if (axis == 1) return (maxCorner.y - minCorner.y) * 0.5f;
        if (axis == 2) return (maxCorner.z - minCorner.z) * 0.5f;
        nvAssume(false);
        return 0.0f;
    }

    // Add a point to this box.
    void Box::addPointToBounds(const Vector3 & p)
    {
        minCorner = min(minCorner, p);
        maxCorner = max(maxCorner, p);
    }

    // Add a box to this box.
    void Box::addBoxToBounds(const Box & b)
    {
        minCorner = min(minCorner, b.minCorner);
        maxCorner = max(maxCorner, b.maxCorner);
    }

    // Translate box.
    void Box::translate(const Vector3 & v)
    {
        minCorner += v;
        maxCorner += v;
    }

    // Scale the box.
    void Box::scale(float s)
    {
        minCorner *= s;
        maxCorner *= s;
    }

    // Expand the box by a fixed amount.
    void Box::expand(float r) {
        minCorner -= Vector3(r,r,r);
        maxCorner += Vector3(r,r,r);
    }

    // Get the area of the box.
    float Box::area() const
    {
        const Vector3 d = extents();
        return 8.0f * (d.x*d.y + d.x*d.z + d.y*d.z);
    }	

    // Get the volume of the box.
    float Box::volume() const
    {
        Vector3 d = extents();
        return 8.0f * (d.x * d.y * d.z);
    }

    // Return true if the box contains the given point.
    bool Box::contains(const Vector3 & p) const
    {
        return 
            minCorner.x < p.x && minCorner.y < p.y && minCorner.z < p.z &&
            maxCorner.x > p.x && maxCorner.y > p.y && maxCorner.z > p.z;
    }

    // Split the given box in 8 octants and assign the ith one to this box.
    void Box::setOctant(const Box & box, const Vector3 & center, int i)
    {
        minCorner = box.minCorner;
        maxCorner = box.maxCorner;

        if (i & 4) minCorner.x = center.x;
        else       maxCorner.x = center.x;
        if (i & 2) minCorner.y = center.y;
        else       maxCorner.y = center.y;
        if (i & 1) minCorner.z = center.z;
        else       maxCorner.z = center.z;
    }

} // nv namespace


#endif // NV_MATH_BOX_INL
