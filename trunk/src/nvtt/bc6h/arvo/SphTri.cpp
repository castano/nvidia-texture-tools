/***************************************************************************
* SphTri.C                                                                 *
*                                                                          *
* This file defines the SphericalTriangle class definition, which          *
* supports member functions for Monte Carlo sampling, point containment,   *
* and other basic operations on spherical triangles.                       *
*                                                                          *
*   Changes:                                                               *
*     01/01/2000  arvo  Added New_{Alpha,Beta,Gamma} methods.              *
*     12/30/1999  arvo  Added VecIrrad method for "Vector Irradiance".     *
*     04/08/1995  arvo  Further optimized sampling algorithm.              *
*     10/11/1994  arvo  Added analytic sampling algorithm.                 *
*     06/14/1994  arvo  Initial implementation.                            *
*                                                                          *
*--------------------------------------------------------------------------*
* Copyright (C) 1995, 2000, James Arvo                                     *
*                                                                          *
* This program is free software; you can redistribute it and/or modify it  *
* under the terms of the GNU General Public License as published by the    *
* Free Software Foundation.  See http://www.fsf.org/copyleft/gpl.html      *
*                                                                          *
* This program is distributed in the hope that it will be useful, but      *
* WITHOUT EXPRESS OR IMPLIED WARRANTY of merchantability or fitness for    *
* any particular purpose.  See the GNU General Public License for more     *
* details.                                                                 *
*                                                                          *
***************************************************************************/
#include <iostream>
#include <math.h>
#include "SphTri.h"
#include "form.h"
namespace ArvoMath {
	/*-------------------------------------------------------------------------*
	* Constructor                                                             *
	*                                                                         *
	* Construct a spherical triangle from three (non-zero) vectors.  The      *
	* vectors needn't be of unit length.                                      *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	SphericalTriangle::SphericalTriangle( const Vec3 &A0, const Vec3 &B0, const Vec3 &C0 )
	{
		Init( A0, B0, C0 );
	}

	/*-------------------------------------------------------------------------*
	* Init                                                                    *
	*                                                                         *
	* Construct the spherical triange from three vertices.  Assume that the   *
	* sphere is centered at the origin.  The vectors A, B, and C need not     *
	* be normalized.                                                          *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	void SphericalTriangle::Init( const Vec3 &A0, const Vec3 &B0, const Vec3 &C0 )
	{
		// Normalize the three vectors -- these are the vertices.

		A_ = Unit( A0 );
		B_ = Unit( B0 );
		C_ = Unit( C0 );

		// Compute and save the cosines of the edge lengths.

		cos_a = B_ * C_;
		cos_b = A_ * C_;
		cos_c = A_ * B_;

		// Compute and save the edge lengths.

		a_ = ArcCos( cos_a );
		b_ = ArcCos( cos_b );
		c_ = ArcCos( cos_c );

		// Compute the cosines of the internal (i.e. dihedral) angles.

		cos_alpha = CosDihedralAngle( C_, A_, B_ );
		cos_beta  = CosDihedralAngle( A_, B_, C_ );
		cos_gamma = CosDihedralAngle( A_, C_, B_ );

		// Compute the (dihedral) angles.

		alpha = ArcCos( cos_alpha );
		beta  = ArcCos( cos_beta  );
		gamma = ArcCos( cos_gamma );

		// Compute the solid angle of the spherical triangle.

		area = alpha + beta + gamma - Pi;

		// Compute the orientation of the triangle.

		orient = Sign( A_ * ( B_ ^ C_ ) );

		// Initialize three variables that are used for sampling the triangle.

		U         = Unit( C_ / A_ );  // In plane of AC orthogonal to A.
		sin_alpha = sin( alpha );
		product   = sin_alpha * cos_c;
	}

	/*-------------------------------------------------------------------------*
	* Init                                                                    *
	*                                                                         *
	* Initialize all fields.  Create a null spherical triangle.               *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	void SphericalTriangle::Init()
	{
		a_ = 0;  A_ = 0;  cos_alpha = 0;  cos_a = 0;  alpha = 0;  
		b_ = 0;  B_ = 0;  cos_beta  = 0;  cos_b = 0;  beta  = 0;  
		c_ = 0;  C_ = 0;  cos_gamma = 0;  cos_c = 0;  gamma = 0;  
		area      = 0;
		orient    = 0;
		sin_alpha = 0;
		product   = 0;
		U         = 0;
	}

	/*-------------------------------------------------------------------------*
	* "( A, B, C )" operator.                                                 *
	*                                                                         *
	* Construct the spherical triange from three vertices.  Assume that the   *
	* sphere is centered at the origin.  The vectors A, B, and C need not     *
	* be normalized.                                                          *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	SphericalTriangle & SphericalTriangle::operator()( 
		const Vec3 &A0, 
		const Vec3 &B0, 
		const Vec3 &C0 )
	{
		Init( A0, B0, C0 );
		return *this;
	}

	/*-------------------------------------------------------------------------*
	* Inside                                                                  *
	*                                                                         *
	* Determine if the vector W is inside the triangle.  W need not be a      *
	* unit vector                                                             *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int SphericalTriangle::Inside( const Vec3 &W ) const
	{
		Vec3 Z = Orient() * W;
		if( Z * ( A() ^ B() ) < 0.0 ) return 0;
		if( Z * ( B() ^ C() ) < 0.0 ) return 0;
		if( Z * ( C() ^ A() ) < 0.0 ) return 0;
		return 1;
	}

	/*-------------------------------------------------------------------------*
	* Chart                                                                   *
	*                                                                         *
	* Generate samples from the current spherical triangle.  If x1 and x2 are *
	* random variables uniformly distributed over [0,1], then the returned    *
	* points are uniformly distributed over the solid angle.                  *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Vec3 SphericalTriangle::Chart( float x1, float x2 ) const
	{
		// Use one random variable to select the area of the sub-triangle.
		// Save the sine and cosine of the angle phi.

		register float phi = x1 * area - Alpha();
		register float s   = sin( phi );
		register float t   = cos( phi );

		// Compute the pair (u,v) that determines the new angle beta.

		register float u = t - cos_alpha;
		register float v = s + product  ;  // sin_alpha * cos_c

		// Compute the cosine of the new edge b.

		float q = ( cos_alpha * ( v * t - u * s ) - v ) / 
			( sin_alpha * ( u * t + v * s )     );

		// Compute the third vertex of the sub-triangle.

		Vec3 C_new = q * A() + Sqrt( 1.0 - q * q ) * U;

		// Use the other random variable to select the height z.

		float z = 1.0 - x2 * ( 1.0 - C_new * B() );

		// Construct the corresponding point on the sphere.

		Vec3 D = C_new / B();  // Remove B component of C_new.
		return z * B() + Sqrt( ( 1.0 - z * z ) / ( D * D ) ) * D;
	}

	/*-------------------------------------------------------------------------*
	* Coord                                                                   *
	*                                                                         *
	* Compute the two coordinates (x1,x2) corresponding to a point in the     *
	* spherical triangle.  This is the inverse of "Chart".                    *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Vec2 SphericalTriangle::Coord( const Vec3 &P1 ) const
	{
		Vec3 P = Unit( P1 );

		// Compute the new C vertex, which lies on the arc defined by B-P
		// and the arc defined by A-C.

		Vec3 C_new = Unit( ( B() ^ P ) ^ ( C() ^ A() ) );

		// Adjust the sign of C_new.  Make sure it's on the arc between A and C.

		if( C_new * ( A() + C() ) < 0.0 ) C_new = -C_new;

		// Compute x1, the area of the sub-triangle over the original area.

		float cos_beta  = CosDihedralAngle( A(), B(), C_new  );
		float cos_gamma = CosDihedralAngle( A(), C_new , B() );
		float sub_area  = Alpha() + acos( cos_beta ) + acos( cos_gamma ) - Pi;
		float x1        = sub_area / SolidAngle();

		// Now compute the second coordinate using the new C vertex.

		float z  = P * B();
		float x2 = ( 1.0 - z ) / ( 1.0 - C_new * B() );

		if( x1 < 0.0 ) x1 = 0.0;  if( x1 > 1.0 ) x1 = 1.0;
		if( x2 < 0.0 ) x2 = 0.0;  if( x2 > 1.0 ) x2 = 1.0;
		return Vec2( x1, x2 );
	}

	/*-------------------------------------------------------------------------*
	* Dual                                                                    *
	*                                                                         *
	* Construct the dual triangle of the current triangle, which is another   *
	* spherical triangle.                                                     *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	SphericalTriangle SphericalTriangle::Dual() const
	{
		Vec3 dual_A = B() ^ C();  if( dual_A * A() < 0.0 ) dual_A *= -1.0;
		Vec3 dual_B = A() ^ C();  if( dual_B * B() < 0.0 ) dual_B *= -1.0;
		Vec3 dual_C = A() ^ B();  if( dual_C * C() < 0.0 ) dual_C *= -1.0;
		return SphericalTriangle( dual_A, dual_B, dual_C );
	}

	/*-------------------------------------------------------------------------*
	* VecIrrad                                                                *
	*                                                                         *
	* Return the "vector irradiance" due to a light source of unit brightness *
	* whose spherical projection is this spherical triangle.  The negative of *
	* this vector dotted with the surface normal gives the (scalar)           *
	* irradiance at the origin.                                               *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Vec3 SphericalTriangle::VecIrrad() const
	{
		Vec3 Phi =
			a() * Unit( B() ^ C() ) +
			b() * Unit( C() ^ A() ) +
			c() * Unit( A() ^ B() ) ;
		if( Orient() ) Phi *= -1.0;
		return Phi;    
	}

	/*-------------------------------------------------------------------------*
	* New_Alpha                                                               *
	*                                                                         *
	* Returns a new spherical triangle derived from the original one by       *
	* moving the "C" vertex along the edge "BC" until the new "alpha" angle   *
	* equals the given argument.                                              *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	SphericalTriangle SphericalTriangle::New_Alpha( float alpha ) const
	{
		Vec3 V1( A() ), V2( B() ), V3( C() );
		Vec3 E1 = Unit( V2 ^ V1 );
		Vec3 E2 = E1 ^ V1;
		Vec3 G  = ( cos(alpha) * E1 ) + ( sin(alpha) * E2 );
		Vec3 D  = Unit( V3 / V2 );
		Vec3 C2 = ((G * D) * V2) - ((G * V2) * D);
		if( Triple( V1, V2, C2 ) > 0.0 ) C2 *= -1.0;
		return SphericalTriangle( V1, V2, C2 );
	}

	std::ostream &operator<<( std::ostream &out, const SphericalTriangle &T )
	{
		out << "SphericalTriangle:\n"
			<< "  " << T.A() << "\n"
			<< "  " << T.B() << "\n"
			<< "  " << T.C() << std::endl;
		return out;
	}

};
