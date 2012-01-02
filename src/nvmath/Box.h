// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_BOX_H
#define NV_MATH_BOX_H

#include "Vector.h"

#include <float.h> // FLT_MAX

namespace nv
{
    class Vector;
    class Stream;
    class Sphere;

    // Axis Aligned Bounding Box.
    class Box
    {
    public:

        Box();
        Box(const Box & b);
        Box(const Vector3 & mins, const Vector3 & maxs);

        Box & operator=(const Box & b);

        operator const float * () const { return reinterpret_cast<const float *>(this); }

        // Clear the bounds.
        void clearBounds();

        // Build a cube centered on center and with edge = 2*dist
        void cube(const Vector3 & center, float dist);

        // Build a box, given center and extents.
        void setCenterExtents(const Vector3 & center, const Vector3 & extents);

        // Get box center.
        Vector3 center() const;

        // Return extents of the box.
        Vector3 extents() const;

        // Return extents of the box.
        float extents(uint axis) const;

        // Add a point to this box.
        void addPointToBounds(const Vector3 & p);

        // Add a box to this box.
        void addBoxToBounds(const Box & b);

        // Translate box.
        void translate(const Vector3 & v);

        // Scale the box.
        void scale(float s);

        // Expand the box by a fixed amount.
        void expand(float r);

        // Get the area of the box.
        float area() const;
 
        // Get the volume of the box.
        float volume() const;

        // Return true if the box contains the given point.
        bool contains(const Vector3 & p) const;

        // Split the given box in 8 octants and assign the ith one to this box.
        void setOctant(const Box & box, const Vector3 & center, int i);

        friend Stream & operator<< (Stream & s, Box & box);

        Vector3 minCorner;
        Vector3 maxCorner;
    };

    float distanceSquared(const Box &box, const Vector3 &point);
    bool overlap(const Box &box, const Sphere &sphere);


} // nv namespace


#endif // NV_MATH_BOX_H
