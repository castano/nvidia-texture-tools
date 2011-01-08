// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_MATH_BOX_H
#define NV_MATH_BOX_H

#include "Vector.h"

#include <float.h> // FLT_MAX

namespace nv
{
    class Stream;
    class Sphere;

    /// Axis Aligned Bounding Box.
    class Box
    {
    public:

        /// Default ctor.
        Box() { };

        /// Copy ctor.
        Box(const Box & b) : minCorner(b.minCorner), maxCorner(b.maxCorner) { }

        /// Init ctor.
        Box(Vector3::Arg mins, Vector3::Arg maxs) : minCorner(mins), maxCorner(maxs) { }

        // Assignment operator.
        Box & operator=(const Box & b) { minCorner = b.minCorner; maxCorner = b.maxCorner; return *this; }

        // Cast operators.
        operator const float * () const { return reinterpret_cast<const float *>(this); }

        /// Clear the bounds.
        void clearBounds()
        {
            minCorner.set(FLT_MAX, FLT_MAX, FLT_MAX);
            maxCorner.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        }

        /// Build a cube centered on center and with edge = 2*dist
        void cube(Vector3::Arg center, float dist)
        {
            setCenterExtents(center, Vector3(dist, dist, dist));
        }

        /// Build a box, given center and extents.
        void setCenterExtents(Vector3::Arg center, Vector3::Arg extents)
        {
            minCorner = center - extents;
            maxCorner = center + extents;
        }

        /// Get box center.
        Vector3 center() const
        {
            return (minCorner + maxCorner) * 0.5f;
        }

        /// Return extents of the box.
        Vector3 extents() const
        {
            return (maxCorner - minCorner) * 0.5f;
        }

        /// Return extents of the box.
        scalar extents(uint axis) const
        {
            nvDebugCheck(axis < 3);
            if (axis == 0) return (maxCorner.x - minCorner.x) * 0.5f;
            if (axis == 1) return (maxCorner.y - minCorner.y) * 0.5f;
            if (axis == 2) return (maxCorner.z - minCorner.z) * 0.5f;
            nvAssume(false);
            return 0.0f;
        }

        /// Add a point to this box.
        void addPointToBounds(Vector3::Arg p)
        {
            minCorner = min(minCorner, p);
            maxCorner = max(maxCorner, p);
        }

        /// Add a box to this box.
        void addBoxToBounds(const Box & b)
        {
            minCorner = min(minCorner, b.minCorner);
            maxCorner = max(maxCorner, b.maxCorner);
        }

        /// Translate box.
        void translate(Vector3::Arg v)
        {
            minCorner += v;
            maxCorner += v;
        }

        /// Scale the box.
        void scale(float s)
        {
            minCorner *= s;
            maxCorner *= s;
        }

        // Expand the box by a fixed amount.
        void expand(float r) {
            minCorner -= Vector3(r,r,r);
            maxCorner += Vector3(r,r,r);
        }

        /// Get the area of the box.
        float area() const
        {
            const Vector3 d = extents();
            return 8.0f * (d.x*d.y + d.x*d.z + d.y*d.z);
        }	

        /// Get the volume of the box.
        float volume() const
        {
            Vector3 d = extents();
            return 8.0f * (d.x * d.y * d.z);
        }

        /// Return true if the box contains the given point.
        bool contains(Vector3::Arg p) const
        {
            return 
                minCorner.x < p.x && minCorner.y < p.y && minCorner.z < p.z &&
                maxCorner.x > p.x && maxCorner.y > p.y && maxCorner.z > p.z;
        }

        /// Split the given box in 8 octants and assign the ith one to this box.
        void setOctant(const Box & box, Vector3::Arg center, int i)
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

        friend Stream & operator<< (Stream & s, Box & box);

        Vector3 minCorner;
        Vector3 maxCorner;
    };

    float distanceSquared(const Box &box, const Vector3 &point);
    bool overlap(const Box &box, const Sphere &sphere);


} // nv namespace


#endif // NV_MATH_BOX_H
