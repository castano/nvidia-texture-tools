// This code is in the public domain -- Ignacio Castaño <castanyo@yahoo.es>

#ifndef NV_MATH_TRIANGLE_H
#define NV_MATH_TRIANGLE_H


#include <nvmath/nvmath.h>
#include <nvmath/Vector.h>
#include <nvmath/Box.h>
//#include <nvmath/Plane.h>

namespace nv
{

// Tomas Akenine-Möller box-triangle test.
NVMATH_API bool triBoxOverlap(Vector3::Arg boxcenter, Vector3::Arg boxhalfsize, const Vector3 * restrict triverts);
NVMATH_API bool triBoxOverlapNoBounds(Vector3::Arg boxcenter, Vector3::Arg boxhalfsize, const Vector3 * restrict triverts);


/// Triangle class with three vertices.
class Triangle
{
public:
	Triangle() {};

	Triangle(const Vector3 & v0, const Vector3 & v1, const Vector3 & v2)
	{
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
	}

	/// Get the bounds of the triangle.
	Box bounds() const {
		Box bounds;
		bounds.clearBounds();
		bounds.addPointToBounds(v[0]);
		bounds.addPointToBounds(v[1]);
		bounds.addPointToBounds(v[2]);
		return bounds;
	}

/*
	/// Get barycentric coordinates of the given point in this triangle.
	Vector3 barycentricCoordinates(Vector3::Arg p)
	{
		Vector3 bar;

		// p must lie in the triangle plane.
		Plane plane;
		plane.set(v[0], v[1], v[2]);
		nvCheck( equalf(plane.Distance(p), 0.0f) );

		Vector3 n;

		// Compute signed area of triangle <v0, v1, p>
		n = cross(v[1] - v[0], p - v[0]);
		bar.x = length(n);
		if (dot(n, plane.vector) < 0) {
			bar->x = -bar->x;
		}

		// Compute signed area of triangle <v1, v2, p>
		n = cross(v[2] - v[1], p - v[1]);
		bar->y = length(cross(e, d));
		if (dot(n, plane.vector) < 0) {
			bar->y = -bar->y;
		}

		// Compute signed area of triangle <v2, v0, p>
		n = cross(v[0] - v[2], p - v[2]);
		bar->z = length(n);
		if (dot(n, plane.vector) < 0) {
			bar->z = -bar->z;
		}

		// We cannot just do this because we need the signed areas.
	//	bar->x = Vector3Area(e0, d0);
	//	bar->y = Vector3Area(e1, d1);
	//	bar->z = Vector3Area(e2, d2);

	//	bar->x = Vector3TripleProduct(v[1], v[2], p);
	//	bar->y = Vector3TripleProduct(v[2], v[0], p);
	//	bar->z = Vector3TripleProduct(v[0], v[1], p);

	}
*/

	// Moller ray triangle test.
	bool TestRay_Moller(const Vector3 & orig, const Vector3 & dir, float * out_t, float * out_u, float * out_v);

	Vector3 v[3];
};


#if 0

/** A planar triangle. */
class Triangle2 {
public:

	Triangle2() {};
	Triangle2(const Vec2 & v0, const Vec2 & v1, const Vec2 & v2) {
		v[0] = v0;
		v[1] = v1;
		v[2] = v2;
	}

	/** Get the barycentric coordinates of the given point for this triangle. 
	 * http://stevehollasch.com/cgindex/math/barycentric.html
	 */
	void GetBarycentricCoordinates(const Vec2 & p, Vector3 * bar) const {
		float denom = 1.0f / (v[1].x - v[0].x) * (v[2].y - v[0].y) - (v[2].x - v[0].x) * (v[1].y - v[0].y);
		bar->x = ((v[1].x - p.x) * (v[2].y - p.y) - (v[2].x - p.x) * (v[1].y - p.y)) * denom;
		bar->y = ((v[2].x - p.x) * (v[0].y - p.y) - (v[0].x - p.x) * (v[2].y - p.y)) * denom;
		//bar->z = ((v[0].x - p.x) * (v[1].y - p.y) - (v[1].x - p.x) * (v[0].y - p.y)) * denom;
		bar->z = 1 - bar->x - bar->y;
	}


	Vec2 v[3];
};

#endif // 0


inline bool overlap(const Triangle & t, const Box & b)
{
	Vector3 center = b.center();
	Vector3 extents = b.extents();
	return triBoxOverlap(center, extents, t.v);
}

inline bool Overlap(const Box & b, const Triangle & t)
{
	return overlap(t, b);
}


inline bool overlapNoBounds(const Triangle & t, const Box & b)
{
	Vector3 center = b.center();
	Vector3 extents = b.extents();
	return triBoxOverlapNoBounds(center, extents, t.v);
}

} // nv namespace

#endif	// NV_MATH_TRIANGLE_H
