// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_FITTING_H
#define NV_MATH_FITTING_H

#include <nvmath/Vector.h>

namespace nv
{

	/// 3D Line.
	struct Line3
	{
		/// Ctor.
		Line3() : m_origin(zero), m_direction(zero)
		{
		}
		
		/// Copy ctor.
		Line3(const Line3 & l) : m_origin(l.m_origin), m_direction(l.m_direction)
		{
		}
		
		/// Ctor.
		Line3(Vector3::Arg o, Vector3::Arg d) : m_origin(o), m_direction(d)
		{
		}
	
		/// Normalize the line.
		void normalize()
		{
			m_direction = nv::normalize(m_direction);
		}
		
		/// Project a point onto the line.
		Vector3 projectPoint(Vector3::Arg point) const
		{
			nvDebugCheck(isNormalized(m_direction));
			
			Vector3 v = point - m_origin;
			return m_origin + m_direction * dot(m_direction, v);
		}
		
		/// Compute distance to line.
		float distanceToPoint(Vector3::Arg point) const
		{
			nvDebugCheck(isNormalized(m_direction));
			
			Vector3 v = point - m_origin;
			Vector3 l = v - m_direction * dot(m_direction, v);
			
			return length(l);
		}
		
		const Vector3 & origin() const { return m_origin; }
		void setOrigin(Vector3::Arg value) { m_origin = value; }
	
		const Vector3 & direction() const { return m_direction; }
		void setDirection(Vector3::Arg value) { m_direction = value; }
		
		
	private:	
		Vector3 m_origin;
		Vector3 m_direction;
	};

	
	namespace Fit
	{
		
		NVMATH_API Line3 bestLine(const Array<Vector3> & pointArray);
		NVMATH_API Vector4 bestPlane(const Array<Vector3> & pointArray);
		
	} // Fit namespace

} // nv namespace

#endif // _PI_MATHLIB_FITTING_H_
