/***************************************************************************
* Vec3.h                                                                   *
*                                                                          *
* Basic operations on 3-dimensional vectors.  This special case is useful  *
* because many operations are performed inline.                            *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
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
#ifndef __VEC3_INCLUDED__
#define __VEC3_INCLUDED__

#include <math.h>
#include <iostream>
#include "Vec2.h"

namespace ArvoMath {

	class Vec3 {
	public:
		Vec3( float c = 0.0             ) { x =     c; y =     c; z =     c; }
		Vec3( float a, float b, float c ) { x =     a; y =     b; z =     c; }
		Vec3( const Vec3 &A             ) { x = A.X(); y = A.Y(); z = A.Z(); }
		void operator=( float c         ) { x =     c; y =     c; z =     c; }
		void operator=( const Vec3 &A   ) { x = A.X(); y = A.Y(); z = A.Z(); }
		void operator=( const Vec2 &A   ) { x = A.X(); y = A.Y(); z =   0.0; }
		~Vec3() {}
		float   X() const { return x; }
		float   Y() const { return y; }
		float   Z() const { return z; }
		float & X()       { return x; }
		float & Y()       { return y; }
		float & Z()       { return z; }
		float   operator[]( int i ) const { return *( &x + i ); }
		float & operator[]( int i )       { return *( &x + i ); }
	private:
		float x, y, z;
	};

	//class Mat3x3 {
	//public:
	//	inline Mat3x3( );
	//	Mat3x3( const Mat3x3 &M ) { *this = M; }
	//	Mat3x3( const Vec3 &, const Vec3 &, const Vec3 & );  // Three columns.
	//	~Mat3x3( ) {}
	//	float    operator()( int i, int j ) const { return m[i][j]; }
	//	float  & operator()( int i, int j )       { return m[i][j]; }
	//	Mat3x3 & operator=( float          );
	//	Mat3x3 & operator=( const Mat3x3 & );
	//	inline   void ScaleRows( float, float, float );
	//	inline   void ScaleCols( float, float, float );
	//	void     Col( int n, const Vec3 & );
	//	const    float *Base() const { return &(m[0][0]); }
	//private:
	//	float m[3][3];
	//};

	//class Mat4x4 {
	//public:
	//	Mat4x4( );
	//	Mat4x4( const Mat4x4 &M ) { *this = M; }
	//	Mat4x4( const Mat3x3 &M ) ;
	//	~Mat4x4( ) {}
	//	float    operator()( int i, int j ) const { return m[i][j]; }
	//	float  & operator()( int i, int j )       { return m[i][j]; }
	//	Mat4x4 & operator=( float          );
	//	Mat4x4 & operator=( const Mat4x4 & );
	//	void     Row( int i, int j, const Vec3 & );
	//	void     Col( int i, int j, const Vec3 & );
	//	void     ScaleRows( float, float, float, float );
	//	void     ScaleCols( float, float, float, float );
	//	const    float *Base() const { return &(m[0][0]); }
	//private:
	//	float m[4][4];
	//};


	//==========================================
	//===  External operators                ===                        
	//==========================================

	//extern Vec3     operator * ( const Mat4x4 &, const Vec3   & );
	//extern Vec3     operator * ( const Vec3   &, const Mat4x4 & );
	//extern Mat3x3   operator * (        float  , const Mat3x3 & );
	//extern Mat3x3   operator * ( const Mat3x3 &,       float    );
	//extern Mat3x3   operator / ( const Mat3x3 &,       double   );
	//extern Mat3x3 & operator *=(       Mat3x3 &,       float    );
	//extern Mat3x3 & operator *=(       Mat3x3 &, const Mat3x3 & );
	//extern Mat3x3   operator * ( const Mat3x3 &, const Mat3x3 & );
	//extern Mat3x3   operator + ( const Mat3x3 &, const Mat3x3 & );
	//extern Mat3x3 & operator +=(       Mat3x3 &, const Mat3x3 & );
	//extern Mat3x3   operator - ( const Mat3x3 &, const Mat3x3 & );
	//extern Mat3x3 & operator -=(       Mat3x3 &, const Mat3x3 & );
	//extern Mat4x4   operator * (        float  , const Mat4x4 & );
	//extern Mat4x4   operator * ( const Mat4x4 &,       float    );
	//extern Mat4x4   operator / ( const Mat4x4 &,       float    );
	//extern Mat4x4 & operator *=(       Mat4x4 &,       float    );
	//extern Mat4x4   operator * ( const Mat4x4 &, const Mat4x4 & );
	//extern Mat4x4   operator + ( const Mat4x4 &, const Mat4x4 & );
	//extern Mat4x4 & operator +=(       Mat4x4 &, const Mat4x4 & );
	//extern Mat4x4   operator - ( const Mat4x4 &, const Mat4x4 & );
	//extern Mat4x4 & operator -=(       Mat4x4 &, const Mat4x4 & );


	//==========================================
	//===  Miscellaneous external functions  ===                        
	//==========================================

	//extern Vec3   OrthogonalTo( const Vec3   & ); // A vector orthogonal to that given.
	//extern Vec3   Min         ( const Vec3   &, const Vec3 &         );
	//extern Vec3   Max         ( const Vec3   &, const Vec3 &         );
	//extern double Angle       ( const Vec3   &, const Vec3 &         );
	//extern int    Orthonormal (       Vec3   &,       Vec3 &         );
	//extern int    Orthonormal (       Vec3   &,       Vec3 &, Vec3 & );
	//extern float  Trace       ( const Mat3x3 & );
	//extern float  Normalize   (       Vec3   & );
	//extern float  Norm1       ( const Mat3x3 & );
	//extern float  SupNorm     ( const Mat3x3 & );
	//extern double Determinant ( const Mat3x3 & );
	//extern Mat3x3 Transp      ( const Mat3x3 & );
	//extern Mat3x3 Householder ( const Vec3   &, const Vec3 & );
	//extern Mat3x3 Householder ( const Vec3   & );
	//extern Mat3x3 Rotation3x3 (       float, float, float ); // Values in [0,1].
	//extern Mat3x3 Inverse     ( const Mat3x3 & );
	//extern Mat3x3 Diag3x3     ( const Vec3   & );
	//extern Mat3x3 Diag3x3     (       float, float, float );
	//extern Mat3x3 Rotation3x3 ( const Vec3   &Axis,                     float angle );
	//extern Mat4x4 Rotation4x4 ( const Vec3   &Axis, const Vec3 &Origin, float angle );


	//==========================================
	//===      Norm-related functions        ===                        
	//==========================================

	inline double LenSqr ( const Vec3 &A ) { return Sqr(A[0]) + Sqr(A[1]) + Sqr(A[2]); }
	inline double Len    ( const Vec3 &A ) { return Sqrt( LenSqr( A ) ); }
	inline double Norm1  ( const Vec3 &A ) { return Abs(A[0]) + Abs(A[1]) + Abs(A[2]); }
	inline double Norm2  ( const Vec3 &A ) { return Len( A ); }
	inline float  SupNorm( const Vec3 &A ) { return MaxAbs( A[0], A[1], A[2] ); }


	//==========================================
	//===            Addition                ===                        
	//==========================================

	inline Vec3 operator+( const Vec3 &A, const Vec3 &B )
	{
		return Vec3( A.X() + B.X(), A.Y() + B.Y(), A.Z() + B.Z() );
	}

	inline Vec3& operator+=( Vec3 &A, const Vec3 &B )
	{
		A.X() += B.X();
		A.Y() += B.Y();
		A.Z() += B.Z();
		return A;
	}


	//==========================================
	//===            Subtraction             ===                        
	//==========================================

	inline Vec3 operator-( const Vec3 &A, const Vec3 &B )
	{
		return Vec3( A.X() - B.X(), A.Y() - B.Y(), A.Z() - B.Z() );
	}

	inline Vec3 operator-( const Vec3 &A )
	{
		return Vec3( -A.X(), -A.Y(), -A.Z() );
	}

	inline Vec3& operator-=( Vec3 &A, const Vec3 &B )
	{
		A.X() -= B.X();
		A.Y() -= B.Y();
		A.Z() -= B.Z();
		return A;
	}


	//==========================================
	//===         Multiplication             ===                        
	//==========================================

	inline Vec3 operator*( float a, const Vec3 &x )
	{
		return Vec3( a * x.X(), a * x.Y(), a * x.Z() );
	}

	inline Vec3 operator*( const Vec3 &x, float a )
	{
		return Vec3( a * x.X(), a * x.Y(), a * x.Z() );
	}

	inline float operator*( const Vec3 &A, const Vec3 &B )  // Inner product.
	{
		return A.X() * B.X() + A.Y() * B.Y() + A.Z() * B.Z();
	}

	inline Vec3& operator*=( Vec3 &A, float a )
	{
		A.X() *= a;
		A.Y() *= a;
		A.Z() *= a;
		return A;
	}

	//inline Vec3& operator*=( Vec3 &A, const Mat3x3 &M )  // A = M * A
	//{
	//	float x = M(0,0) * A.X() + M(0,1) * A.Y() + M(0,2) * A.Z();
	//	float y = M(1,0) * A.X() + M(1,1) * A.Y() + M(1,2) * A.Z();
	//	float z = M(2,0) * A.X() + M(2,1) * A.Y() + M(2,2) * A.Z();
	//	A.X() = x;
	//	A.Y() = y;
	//	A.Z() = z;
	//	return A;
	//}

	//inline Vec3& operator*=( Vec3 &A, const Mat4x4 &M )  // A = M * A
	//{
	//	float x = M(0,0) * A.X() + M(0,1) * A.Y() + M(0,2) * A.Z() + M(0,3);
	//	float y = M(1,0) * A.X() + M(1,1) * A.Y() + M(1,2) * A.Z() + M(1,3);
	//	float z = M(2,0) * A.X() + M(2,1) * A.Y() + M(2,2) * A.Z() + M(2,3);
	//	A.X() = x;
	//	A.Y() = y;
	//	A.Z() = z;
	//	return A;
	//}


	//==========================================
	//===             Division               ===                        
	//==========================================

	inline Vec3 operator/( const Vec3 &A, double c )
	{
		double t = 1.0 / c;
		return Vec3( A.X() * t, A.Y() * t, A.Z() * t );
	}

	inline Vec3& operator/=( Vec3 &A, double a )
	{
		A.X() /= a;
		A.Y() /= a;
		A.Z() /= a;
		return A;
	}

	inline Vec3 operator/( const Vec3 &A, const Vec3 &B )  // Remove component parallel to B.
	{
		Vec3 C;  // Cumbersome due to compiler falure.
		double x = LenSqr( B );
		if( x > 0.0 ) C = A - B * (( A * B ) / x); else C = A;
		return C;
	}

	inline void operator/=( Vec3 &A, const Vec3 &B ) // Remove component parallel to B.
	{
		double x = LenSqr( B );
		if( x > 0.0 ) A -= B * (( A * B ) / x);
	}


	//==========================================
	//===          Miscellaneous             ===                        
	//==========================================

	inline float operator|( const Vec3 &A, const Vec3 &B )  // Inner product.
	{
		return A * B;
	}

	inline Vec3 Unit( const Vec3 &A )
	{
		double d = LenSqr( A );
		return d > 0.0 ? A / sqrt(d) : Vec3(0,0,0);
	}

	inline Vec3 Unit( float x, float y, float z )
	{
		return Unit( Vec3( x, y, z ) );
	}

	inline Vec3 Ortho( const Vec3 &A, const Vec3 &B )
	{
		return Unit( A / B );
	}

	inline int operator==( const Vec3 &A, float x )
	{
		return (A[0] == x) && (A[1] == x) && (A[2] == x);
	}

	inline Vec3 operator^( const Vec3 &A, const Vec3 &B )
	{
		return Vec3( 
			A.Y() * B.Z() - A.Z() * B.Y(),
			A.Z() * B.X() - A.X() * B.Z(),
			A.X() * B.Y() - A.Y() * B.X() );
	}

	inline double dist( const Vec3 &A, const Vec3 &B ) 
	{ 
		return Len( A - B ); 
	}

	inline double Dihedral( const Vec3 &A, const Vec3 &B, const Vec3 &C )
	{
		return ArcCos( Unit( A ^ B ) * Unit( C ^ B ) );
	}

	inline Vec3 operator>>( const Vec3 &A, const Vec3 &B )  // Project A onto B.
	{
		Vec3 C;
		double x = LenSqr( B );
		if( x > 0.0 ) C = B * (( A * B ) / x);
		return C;
	}

	inline Vec3 operator<<( const Vec3 &A, const Vec3 &B ) // Project B onto A.
	{
		return B >> A;
	}

	inline double Triple( const Vec3 &A, const Vec3 &B, const Vec3 &C )
	{
		return ( A ^ B ) * C;
	}


	//==========================================
	//===  Operations involving Matrices     ===                        
	//==========================================

	//inline Mat3x3 Outer( const Vec3 &A, const Vec3 &B )  // Outer product.
	//{
	//	Mat3x3 C;
	//	C(0,0) = A.X() * B.X();
	//	C(0,1) = A.X() * B.Y();
	//	C(0,2) = A.X() * B.Z();
	//	C(1,0) = A.Y() * B.X();
	//	C(1,1) = A.Y() * B.Y();
	//	C(1,2) = A.Y() * B.Z();
	//	C(2,0) = A.Z() * B.X();
	//	C(2,1) = A.Z() * B.Y();
	//	C(2,2) = A.Z() * B.Z();
	//	return C;
	//}

	//inline Vec3 operator*( const Mat3x3 &M, const Vec3 &A )
	//{
	//	return Vec3(
	//		M(0,0) * A[0] + M(0,1) * A[1] + M(0,2) * A[2],
	//		M(1,0) * A[0] + M(1,1) * A[1] + M(1,2) * A[2],
	//		M(2,0) * A[0] + M(2,1) * A[1] + M(2,2) * A[2]);
	//}

	//inline Vec3 operator*( const Vec3 &A, const Mat3x3 &M )
	//{
	//	return Vec3( 
	//		A[0] * M(0,0) + A[1] * M(1,0) + A[2] * M(2,0),
	//		A[0] * M(0,1) + A[1] * M(1,1) + A[2] * M(2,1),
	//		A[0] * M(0,2) + A[1] * M(1,2) + A[2] * M(2,2));
	//}

	////==========================================
	////===      Operations on Matrices        ===                        
	////==========================================

	//inline Mat3x3 operator+( const Mat3x3 &A, const Mat3x3 &B )
	//{
	//	Mat3x3 C;
	//	C(0,0) = A(0,0) + B(0,0);  C(0,1) = A(0,1) + B(0,1);  C(0,2) = A(0,2) + B(0,2);
	//	C(1,0) = A(1,0) + B(1,0);  C(1,1) = A(1,1) + B(1,1);  C(1,2) = A(1,2) + B(1,2);
	//	C(2,0) = A(2,0) + B(2,0);  C(2,1) = A(2,1) + B(2,1);  C(2,2) = A(2,2) + B(2,2);
	//	return C;
	//}

	//inline Mat3x3 operator-( const Mat3x3 &A, const Mat3x3 &B )
	//{
	//	Mat3x3 C;
	//	C(0,0) = A(0,0) - B(0,0);  C(0,1) = A(0,1) - B(0,1);  C(0,2) = A(0,2) - B(0,2);
	//	C(1,0) = A(1,0) - B(1,0);  C(1,1) = A(1,1) - B(1,1);  C(1,2) = A(1,2) - B(1,2);
	//	C(2,0) = A(2,0) - B(2,0);  C(2,1) = A(2,1) - B(2,1);  C(2,2) = A(2,2) - B(2,2);
	//	return C;
	//}

	//inline Mat3x3 operator*( const Mat3x3 &A, const Mat3x3 &B )
	//{
	//	Mat3x3 C;
	//	C(0,0) = A(0,0) * B(0,0) + A(0,1) * B(1,0) + A(0,2) * B(2,0);
	//	C(0,1) = A(0,0) * B(0,1) + A(0,1) * B(1,1) + A(0,2) * B(2,1);
	//	C(0,2) = A(0,0) * B(0,2) + A(0,1) * B(1,2) + A(0,2) * B(2,2);
	//	C(1,0) = A(1,0) * B(0,0) + A(1,1) * B(1,0) + A(1,2) * B(2,0);
	//	C(1,1) = A(1,0) * B(0,1) + A(1,1) * B(1,1) + A(1,2) * B(2,1);
	//	C(1,2) = A(1,0) * B(0,2) + A(1,1) * B(1,2) + A(1,2) * B(2,2);
	//	C(2,0) = A(2,0) * B(0,0) + A(2,1) * B(1,0) + A(2,2) * B(2,0);
	//	C(2,1) = A(2,0) * B(0,1) + A(2,1) * B(1,1) + A(2,2) * B(2,1);
	//	C(2,2) = A(2,0) * B(0,2) + A(2,1) * B(1,2) + A(2,2) * B(2,2);
	//	return C;
	//}

	//inline void Mat3x3::ScaleRows( float a, float b, float c )
	//{
	//	m[0][0] *= a;  m[0][1] *= a;  m[0][2] *= a;
	//	m[1][0] *= b;  m[1][1] *= b;  m[1][2] *= b;
	//	m[2][0] *= c;  m[2][1] *= c;  m[2][2] *= c;
	//}

	//inline void Mat3x3::ScaleCols( float a, float b, float c )
	//{
	//	m[0][0] *= a;  m[0][1] *= b;  m[0][2] *= c;
	//	m[1][0] *= a;  m[1][1] *= b;  m[1][2] *= c;
	//	m[2][0] *= a;  m[2][1] *= b;  m[2][2] *= c;
	//}


	//==========================================
	//===       Special Matrices             ===                        
	//==========================================

	//inline Mat3x3::Mat3x3() 
	//{
	//	m[0][0] = 0;  m[0][1] = 0;  m[0][2] = 0;
	//	m[1][0] = 0;  m[1][1] = 0;  m[1][2] = 0;
	//	m[2][0] = 0;  m[2][1] = 0;  m[2][2] = 0; 
	//}

	//inline Mat3x3 Ident3x3()
	//{
	//	Mat3x3 I;
	//	I(0,0) = 1.0;
	//	I(1,1) = 1.0;
	//	I(2,2) = 1.0;
	//	return I;
	//}

	//inline Mat4x4 Ident4x4()
	//{
	//	Mat4x4 I;
	//	I(0,0) = 1.0;
	//	I(1,1) = 1.0;
	//	I(2,2) = 1.0;
	//	I(3,3) = 1.0;
	//	return I;
	//}

	//inline void Adjoint( const Mat3x3 &M, Mat3x3 &A )
	//{
	//	A(0,0) = M(1,1) * M(2,2) - M(1,2) * M(2,1);
	//	A(0,1) = M(1,2) * M(2,0) - M(1,0) * M(2,2);
	//	A(0,2) = M(1,0) * M(2,1) - M(1,1) * M(2,0);

	//	A(1,0) = M(0,2) * M(2,1) - M(0,1) * M(2,2);
	//	A(1,1) = M(0,0) * M(2,2) - M(0,2) * M(2,0);
	//	A(1,2) = M(0,1) * M(2,0) - M(0,0) * M(2,1);

	//	A(2,0) = M(0,1) * M(1,2) - M(0,2) * M(1,1);
	//	A(2,1) = M(0,2) * M(1,0) - M(0,0) * M(1,2);
	//	A(2,2) = M(0,0) * M(1,1) - M(0,1) * M(1,0);
	//}

	//inline void TranspAdjoint( const Mat3x3 &M, Mat3x3 &A )
	//{
	//	A(0,0) = M(1,1) * M(2,2) - M(1,2) * M(2,1);
	//	A(1,0) = M(1,2) * M(2,0) - M(1,0) * M(2,2);
	//	A(2,0) = M(1,0) * M(2,1) - M(1,1) * M(2,0);

	//	A(0,1) = M(0,2) * M(2,1) - M(0,1) * M(2,2);
	//	A(1,1) = M(0,0) * M(2,2) - M(0,2) * M(2,0);
	//	A(2,1) = M(0,1) * M(2,0) - M(0,0) * M(2,1);

	//	A(0,2) = M(0,1) * M(1,2) - M(0,2) * M(1,1);
	//	A(1,2) = M(0,2) * M(1,0) - M(0,0) * M(1,2);
	//	A(2,2) = M(0,0) * M(1,1) - M(0,1) * M(1,0);
	//}

	//inline void Adjoint( const Mat3x3 &M, Mat3x3 &A, double &det )
	//{
	//	Adjoint( M, A );
	//	det = A(0,0) * M(0,0) + A(1,0) * M(1,0) + A(2,0) * M(2,0);
	//}

	//inline void TranspAdjoint( const Mat3x3 &M, Mat3x3 &A, double &det )
	//{
	//	TranspAdjoint( M, A );
	//	det = A(0,0) * M(0,0) + A(0,1) * M(1,0) + A(0,2) * M(2,0);
	//}


	//==========================================
	//===  Output routines                   ===                        
	//==========================================

	extern std::ostream &operator<<( std::ostream &out, const Vec3   & );
	//extern std::ostream &operator<<( std::ostream &out, const Mat3x3 & );
	//extern std::ostream &operator<<( std::ostream &out, const Mat4x4 & );
};
#endif
