/***************************************************************************
* Vec2.h                                                                   *
*                                                                          *
* Basic operations on 2-dimensional vectors.  This special case is useful  *
* because nearly all operations are performed inline.                      *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      arvo    05/22/98    Added TimedVec2, extending Vec2.                *
*      arvo    06/17/93    Initial coding.                                 *
*                                                                          *
*--------------------------------------------------------------------------*
* Copyright (C) 1999, James Arvo                                           *
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
#ifndef __VEC2_INCLUDED__
#define __VEC2_INCLUDED__

#include <math.h>
#include <iostream>
#include "ArvoMath.h"

namespace ArvoMath {

	class Vec2;       // 2-D floating-point vector.
	class TimedVec2;  // 2-D vector with a time stamp.
	class Mat2x2;     // 2x2 floating-point matrix.

	class Vec2 {
	public:
		Vec2(                  ) { x = 0.0;   y = 0.0;   }
		Vec2( float a, float b ) { x = a;     y = b;     }
		Vec2( const Vec2 &A    ) { x = A.X(); y = A.Y(); }
		~Vec2() {}
		Vec2 &operator=( float s       ) { return Set(     s,     s ); }
		Vec2 &operator=( const Vec2 &A ) { return Set( A.X(), A.Y() ); }
		float  X() const { return x; }
		float  Y() const { return y; }
		float &X()       { return x; }
		float &Y()       { return y; }
		float  operator[]( int i ) const { return *( &x + i ); }
		float &operator[]( int i )       { return *( &x + i ); }
		Vec2  &Set( float a, float b ) { x = a; y = b; return *this; }
		Vec2  &Set( const Vec2 &A    ) { return Set( A.X(), A.Y() ); }
	public:
		static const Vec2 Zero;
		static const Vec2 Xaxis;
		static const Vec2 Yaxis;
	protected:
		float x, y;
	};

	// This class simply adds a time field to the Vec2 class so that time-stamped
	// coordinates can be easily inserted into objects such as Polylines.

	class TimedVec2 : public Vec2 {
	public:
		TimedVec2() { time = 0; }
		TimedVec2( const Vec2 &p   , long u = 0 ) { Set( p ); time = u; }
		TimedVec2( float x, float y, long u = 0 ) { Set(x,y); time = u; }
		~TimedVec2() {}
		Vec2 &Coord()       { return *this; }
		Vec2  Coord() const { return *this; }
		long  Time () const { return  time; }
		void  SetTime( long u ) { time = u; }
	protected:
		long time;
	};

	class Mat2x2 {
	public:
		Mat2x2( ) { Set( 0, 0, 0, 0 ); }
		Mat2x2( float a, float b, float c, float d ) { Set( a, b, c, d ); }
		Mat2x2( const Vec2 &c1, const Vec2 &c2 );
		~Mat2x2( ) {}
		Mat2x2 &operator*=( float scale );
		Mat2x2  operator* ( float scale ) const;
		void Set( float a, float b, float c, float d ) 
		{ m[0][0] = a; m[0][1] = b; m[1][0] = c; m[1][1] = d; }
		float  operator()( int i, int j ) const { return m[i][j]; }
		float &operator()( int i, int j )       { return m[i][j]; }
	private:
		float m[2][2];
	};


	//==========================================
	//===  Miscellaneous external functions  ===                        
	//==========================================

	extern float Normalize( Vec2 &A );
	extern Vec2  Min ( const Vec2 &A, const Vec2 &B );
	extern Vec2  Max ( const Vec2 &A, const Vec2 &B );


	//==========================================
	//===  Norm-related functions           ===                        
	//==========================================

	inline double LenSqr ( const Vec2 &A ) { return Sqr(A[0]) + Sqr(A[1]); }
	inline double Len    ( const Vec2 &A ) { return sqrt( LenSqr( A ) ); }
	inline double OneNorm( const Vec2 &A ) { return Abs( A.X() ) + Abs( A.Y() ); }
	inline double TwoNorm( const Vec2 &A ) { return Len(A); }
	inline float  SupNorm( const Vec2 &A ) { return MaxAbs( A.X(), A.Y() ); }


	//==========================================
	//===  Addition                          ===                        
	//==========================================

	inline Vec2 operator+( const Vec2 &A, const Vec2 &B )
	{
		return Vec2( A.X() + B.X(), A.Y() + B.Y() );
	}

	inline Vec2& operator+=( Vec2 &A, const Vec2 &B )
	{
		A.X() += B.X();
		A.Y() += B.Y();
		return A;
	}


	//==========================================
	//===  Subtraction                       ===                        
	//==========================================

	inline Vec2 operator-( const Vec2 &A, const Vec2 &B )
	{
		return Vec2( A.X() - B.X(), A.Y() - B.Y() );
	}

	inline Vec2 operator-( const Vec2 &A )
	{
		return Vec2( -A.X(), -A.Y() );
	}

	inline Vec2& operator-=( Vec2 &A, const Vec2 &B )
	{
		A.X() -= B.X();
		A.Y() -= B.Y();
		return A;
	}


	//==========================================
	//===  Multiplication                    ===                        
	//==========================================

	inline Vec2 operator*( float c, const Vec2 &A )
	{
		return Vec2( c * A.X(), c * A.Y() );
	}

	inline Vec2 operator*( const Vec2 &A, float c )
	{
		return Vec2( c * A.X(), c * A.Y() );
	}

	inline float operator*( const Vec2 &A, const Vec2 &B )  // Inner product
	{
		return A.X() * B.X() + A.Y() * B.Y();
	}

	inline Vec2& operator*=( Vec2 &A, float c )
	{
		A.X() *= c;
		A.Y() *= c;
		return A;
	}

	//==========================================
	//===  Division                          ===                        
	//==========================================

	inline Vec2 operator/( const Vec2 &A, float c )
	{
		return Vec2( A.X() / c, A.Y() / c );
	}

	inline Vec2 operator/( const Vec2 &A, const Vec2 &B ) 
	{
		return A - B * (( A * B ) / LenSqr( B ));
	}


	//==========================================
	//===  Comparison                        ===                        
	//==========================================

	inline int operator==( const Vec2 &A, const Vec2 &B ) 
	{ 
		return A.X() == B.X() && A.Y() == B.Y(); 
	}

	inline int operator!=( const Vec2 &A, const Vec2 &B ) 
	{ 
		return A.X() != B.X() || A.Y() != B.Y(); 
	}

	inline int operator<=( const Vec2 &A, const Vec2 &B ) 
	{ 
		return A.X() <= B.X() && A.Y() <= B.Y(); 
	}

	inline int operator<( const Vec2 &A, const Vec2 &B ) 
	{ 
		return A.X() < B.X() && A.Y() < B.Y(); 
	}

	inline int operator>=( const Vec2 &A, const Vec2 &B ) 
	{ 
		return A.X() >= B.X() && A.Y() >= B.Y(); 
	}

	inline int operator>( const Vec2 &A, const Vec2 &B ) 
	{ 
		return A.X() > B.X() && A.Y() > B.Y();
	}

	//==========================================
	//===  Miscellaneous                     ===                        
	//==========================================

	inline float operator|( const Vec2 &A, const Vec2 &B )  // Inner product
	{
		return A * B;
	}

	inline Vec2 Unit( const Vec2 &A )
	{
		float c = LenSqr( A );
		if( c > 0.0 ) c = 1.0 / sqrt( c );
		return c * A;
	}

	inline Vec2 Unit( const Vec2 &A, float &len )
	{
		float c = LenSqr( A );
		if( c > 0.0 ) 
		{
			len = sqrt( c );
			return A / len;
		}
		len = 0.0;
		return A;
	}

	inline Vec2 Unit( float x, float y )
	{
		return Unit( Vec2( x, y ) );
	}

	inline double dist( const Vec2 &A, const Vec2 &B ) 
	{ 
		return Len( A - B ); 
	}

	inline float operator^( const Vec2 &A, const Vec2 &B )
	{
		return A.X() * B.Y() - A.Y() * B.X();
	}

	inline int Quadrant( const Vec2 &A )
	{
		if( A.Y() >= 0.0 ) return A.X() >= 0.0 ? 1 : 2;
		return A.X() >= 0.0 ? 4 : 3;
	}

	inline Vec2 OrthogonalTo( const Vec2 &A ) // A vector orthogonal to that given.
	{
		return Vec2( -A.Y(), A.X() );
	}

	inline Vec2 Interpolate( const Vec2 &A, const Vec2 &B, float t )
	{
		// Compute a point along the segment joining points A and B
		// according to the normalized parameter t in [0,1].
		return ( 1.0 - t ) * A + t * B;
	}

	//==========================================
	//===  Operations involving Matrices     ===                        
	//==========================================

	inline Mat2x2 Outer( const Vec2 &A, const Vec2 &B )  // Outer product.
	{
		Mat2x2 C;
		C(0,0) = A.X() * B.X();
		C(0,1) = A.X() * B.Y();
		C(1,0) = A.Y() * B.X();
		C(1,1) = A.Y() * B.Y();
		return C;
	}

	inline Vec2 operator*( const Mat2x2 &M, const Vec2 &A )
	{
		return Vec2( 
			M(0,0) * A.X() + M(0,1) * A.Y(),
			M(1,0) * A.X() + M(1,1) * A.Y()
			);
	}

	inline Mat2x2 &Mat2x2::operator*=( float scale )
	{
		m[0][0] *= scale;
		m[0][1] *= scale;
		m[1][0] *= scale;
		m[1][1] *= scale;
		return *this;
	}

	inline Mat2x2 Mat2x2::operator*( float scale ) const
	{
		return Mat2x2(
			scale * m[0][0], scale * m[0][1],       
			scale * m[1][0], scale * m[1][1]
			);
	}

	inline Mat2x2 operator*( float scale, const Mat2x2 &M )
	{
		return M * scale;
	}

	inline float Norm1( const Mat2x2 &A )
	{
		return Max( Abs(A(0,0)) + Abs(A(0,1)), Abs(A(1,0)) + Abs(A(1,1)) );
	}

	inline double det( const Mat2x2 &A )
	{
		return A(0,0) * A(1,1) - A(1,0) * A(0,1);
	}

	extern Vec2 Solve(  // Return solution x of the system Ax = b.
		const Mat2x2 &A, 
		const Vec2 &b 
		);

	//==========================================
	//===  Output routines                   ===                        
	//==========================================

	extern std::ostream &operator<<( std::ostream &out, const Vec2   & );
	extern std::ostream &operator<<( std::ostream &out, const Mat2x2 & );
};
#endif
