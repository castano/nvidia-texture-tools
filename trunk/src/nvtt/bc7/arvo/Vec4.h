/***************************************************************************
* Vec4.h                                                                   *
*                                                                          *
* Basic operations on 4-dimensional vectors.  This special case is useful  *
* because many operations are performed inline.                            *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      walt    6/26/07     Edited Vec3 to make this new class              *
*      arvo    10/27/94    Reorganized (removed Col & Row distinction).    *
*      arvo    06/14/93    Initial coding.                                 *
*                                                                          *
*--------------------------------------------------------------------------*
* Copyright (C) 1994, James Arvo                                           *
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
#ifndef __Vec4_INCLUDED__
#define __Vec4_INCLUDED__

#include <math.h>
#include <iostream>
#include "Vec2.h"
#include "Vec3.h"

namespace ArvoMath {

	class Vec4 {
	public:
		Vec4( float c = 0.0             ) { x =     c; y =     c; z =     c; w =     c; }
		Vec4( float a, float b, float c, float d ) { x =     a; y =     b; z =     c; w = d; }
		Vec4( const Vec4 &A             ) { x = A.X(); y = A.Y(); z = A.Z(); w = A.W(); }
		Vec4( const Vec3 &A, float d    ) { x = A.X(); y = A.Y(); z = A.Z(); w = d;     }
		void operator=( float c         ) { x =     c; y =     c; z =     c; w =     c; }
		void operator=( const Vec4 &A   ) { x = A.X(); y = A.Y(); z = A.Z(); w = A.W(); }
		void operator=( const Vec3 &A   ) { x = A.X(); y = A.Y(); z = A.Z(); w =   0.0; }
		void operator=( const Vec2 &A   ) { x = A.X(); y = A.Y(); z =   0.0; w =   0.0; }
		~Vec4() {}
		float   X() const { return x; }
		float   Y() const { return y; }
		float   Z() const { return z; }
		float   W() const { return w; }
		float & X()       { return x; }
		float & Y()       { return y; }
		float & Z()       { return z; }
		float & W()       { return w; }
		float   operator[]( int i ) const { return *( &x + i ); }
		float & operator[]( int i )       { return *( &x + i ); }
	private:
		float x, y, z, w;
	};

	//==========================================
	//===      Norm-related functions        ===                        
	//==========================================

	inline double LenSqr ( const Vec4 &A ) { return Sqr(A[0]) + Sqr(A[1]) + Sqr(A[2]) + Sqr(A[3]); }
	inline double Len    ( const Vec4 &A ) { return Sqrt( LenSqr( A ) ); }
	inline double Norm1  ( const Vec4 &A ) { return Abs(A[0]) + Abs(A[1]) + Abs(A[2]) + Abs(A[3]); }
	inline double Norm2  ( const Vec4 &A ) { return Len( A ); }
	inline float  SupNorm( const Vec4 &A ) { return MaxAbs( A[0], A[1], A[2], A[3] ); }


	//==========================================
	//===            Addition                ===                        
	//==========================================

	inline Vec4 operator+( const Vec4 &A, const Vec4 &B )
	{
		return Vec4( A.X() + B.X(), A.Y() + B.Y(), A.Z() + B.Z(), A.W() + B.W() );
	}

	inline Vec4& operator+=( Vec4 &A, const Vec4 &B )
	{
		A.X() += B.X();
		A.Y() += B.Y();
		A.Z() += B.Z();
		A.W() += B.W();
		return A;
	}


	//==========================================
	//===            Subtraction             ===                        
	//==========================================

	inline Vec4 operator-( const Vec4 &A, const Vec4 &B )
	{
		return Vec4( A.X() - B.X(), A.Y() - B.Y(), A.Z() - B.Z(), A.W() - B.W());
	}

	inline Vec4 operator-( const Vec4 &A )
	{
		return Vec4( -A.X(), -A.Y(), -A.Z(), -A.W() );
	}

	inline Vec4& operator-=( Vec4 &A, const Vec4 &B )
	{
		A.X() -= B.X();
		A.Y() -= B.Y();
		A.Z() -= B.Z();
		A.W() -= B.W();
		return A;
	}


	//==========================================
	//===         Multiplication             ===                        
	//==========================================

	inline Vec4 operator*( float a, const Vec4 &x )
	{
		return Vec4( a * x.X(), a * x.Y(), a * x.Z(), a * x.W() );
	}

	inline Vec4 operator*( const Vec4 &x, float a )
	{
		return Vec4( a * x.X(), a * x.Y(), a * x.Z(), a * x.W() );
	}

	inline float operator*( const Vec4 &A, const Vec4 &B )  // Inner product.
	{
		return A.X() * B.X() + A.Y() * B.Y() + A.Z() * B.Z() + A.W() * B.W();
	}

	inline Vec4& operator*=( Vec4 &A, float a )
	{
		A.X() *= a;
		A.Y() *= a;
		A.Z() *= a;
		A.W() *= a;
		return A;
	}

	//==========================================
	//===             Division               ===                        
	//==========================================

	inline Vec4 operator/( const Vec4 &A, double c )
	{
		double t = 1.0 / c;
		return Vec4( A.X() * t, A.Y() * t, A.Z() * t, A.W() * t);
	}

	inline Vec4& operator/=( Vec4 &A, double a )
	{
		A.X() /= a;
		A.Y() /= a;
		A.Z() /= a;
		A.W() /= a;
		return A;
	}

	inline Vec4 operator/( const Vec4 &A, const Vec4 &B )  // Remove component parallel to B.
	{
		Vec4 C;  // Cumbersome due to compiler falure.
		double x = LenSqr( B );
		if( x > 0.0 ) C = A - B * (( A * B ) / x); else C = A;
		return C;
	}

	inline void operator/=( Vec4 &A, const Vec4 &B ) // Remove component parallel to B.
	{
		double x = LenSqr( B );
		if( x > 0.0 ) A -= B * (( A * B ) / x);
	}


	//==========================================
	//===          Miscellaneous             ===                        
	//==========================================

	inline float operator|( const Vec4 &A, const Vec4 &B )  // Inner product.
	{
		return A * B;
	}

	inline Vec4 Unit( const Vec4 &A )
	{
		double d = LenSqr( A );
		return d > 0.0 ? A / sqrt(d) : Vec4(0,0,0,0);
	}

	inline Vec4 Unit( float x, float y, float z, float w )
	{
		return Unit( Vec4( x, y, z, w ) );
	}

	inline Vec4 Ortho( const Vec4 &A, const Vec4 &B )
	{
		return Unit( A / B );
	}

	inline int operator==( const Vec4 &A, float x )
	{
		return (A[0] == x) && (A[1] == x) && (A[2] == x) && (A[3] == x);
	}

//	inline Vec4 operator^( const Vec4 &A, const Vec4 &B ) there is no 4ED "cross product" of 2 4D vectors -- we need six dimensions

	inline double dist( const Vec4 &A, const Vec4 &B ) 
	{ 
		return Len( A - B ); 
	}

//	inline double Dihedral( const Vec4 &A, const Vec4 &B, const Vec4 &C )

	inline Vec4 operator>>( const Vec4 &A, const Vec4 &B )  // Project A onto B.
	{
		Vec4 C;
		double x = LenSqr( B );
		if( x > 0.0 ) C = B * (( A * B ) / x);
		return C;
	}

	inline Vec4 operator<<( const Vec4 &A, const Vec4 &B ) // Project B onto A.
	{
		return B >> A;
	}

//	inline double Triple( const Vec4 &A, const Vec4 &B, const Vec4 &C )

	//==========================================
	//===  Output routines                   ===                        
	//==========================================

	extern std::ostream &operator<<( std::ostream &out, const Vec4   & );
};
#endif
