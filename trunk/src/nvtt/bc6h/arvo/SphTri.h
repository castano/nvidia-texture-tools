/***************************************************************************
* SphTri.h                                                                 *
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
#ifndef __SPHTRI_INCLUDED__
#define __SPHTRI_INCLUDED__

#include "Vec3.h"
#include "Vec2.h"

namespace ArvoMath {

	/*
	*  The (Oblique) Spherical Triangle ABC.  Edge lengths (segments of great 
	*  circles) are a, b, and c.  The (dihedral) angles are Alpha, Beta, and Gamma.
	*
	*                      B
	*                      o
	*                     / \
	*                    /   \
	*                   /Beta \
	*                  /       \
	*               c /         \ a
	*                /           \ 
	*               /             \
	*              /               \
	*             /                 \
	*            /                   \
	*           /Alpha          Gamma \
	*          o-----------------------o
	*         A            b            C
	*
	*/

	class SphericalTriangle {

	public: // methods
		SphericalTriangle() { Init(); }
		SphericalTriangle( const SphericalTriangle &T ) { *this = T; }
		SphericalTriangle( const Vec3 &, const Vec3 &, const Vec3 & );
		SphericalTriangle & operator()( const Vec3 &, const Vec3 &, const Vec3 & );
		~SphericalTriangle( ) {}
		void   operator=( const SphericalTriangle &T ) { *this = T; }
		Vec3   Chart    ( float x, float y ) const;  // Const-Jacobian map from square.
		Vec2   Coord    ( const Vec3 &P    ) const;  // Get 2D coords of a point.
		int    Orient( ) const { return orient; }
		int    Inside( const Vec3 & ) const;
		float  SolidAngle() const { return area; }
		float  SignedSolidAngle() const { return -orient * area; } // CC is pos.
		const  Vec3 &A()  const { return A_       ; }
		const  Vec3 &B()  const { return B_       ; }
		const  Vec3 &C()  const { return C_       ; }
		float  a()        const { return a_       ; }
		float  b()        const { return b_       ; }
		float  c()        const { return c_       ; }
		float  Cos_a()    const { return cos_a    ; }
		float  Cos_b()    const { return cos_b    ; }
		float  Cos_c()    const { return cos_c    ; }
		float  Alpha()    const { return alpha    ; }
		float  Beta ()    const { return beta     ; }
		float  Gamma()    const { return gamma    ; }
		float  CosAlpha() const { return cos_alpha; }
		float  CosBeta () const { return cos_beta ; }
		float  CosGamma() const { return cos_gamma; }
		Vec3   VecIrrad() const; // Returns the vector irradiance.
		SphericalTriangle Dual() const;
		SphericalTriangle New_Alpha( float alpha ) const;
		SphericalTriangle New_Beta ( float beta  ) const;
		SphericalTriangle New_Gamma( float gamma ) const;

	private: // methods
		void Init( );
		void Init( const Vec3 &A, const Vec3 &B, const Vec3 &C );

	private: // data
		Vec3  A_, B_, C_, U;       // The vertices (and a temp vector).
		float a_, b_, c_;          // The edge lengths.
		float alpha, beta, gamma;  // The angles.
		float cos_a, cos_b, cos_c;
		float cos_alpha, cos_beta, cos_gamma;
		float area;
		float sin_alpha, product;  // Used in sampling algorithm.
		int   orient;              // Orientation.
	};

	inline double CosDihedralAngle( const Vec3 &A, const Vec3 &B, const Vec3 &C )
	{
		float x = Unit( A ^ B ) * Unit( C ^ B );
		if( x < -1.0 ) x = -1.0;
		if( x >  1.0 ) x =  1.0;
		return x;
	}

	inline double DihedralAngle( const Vec3 &A, const Vec3 &B, const Vec3 &C )
	{
		return acos( CosDihedralAngle( A, B, C ) );
	}

	extern std::ostream &operator<<( std::ostream &out, const SphericalTriangle & );
};
#endif
