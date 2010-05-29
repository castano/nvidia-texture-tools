/***************************************************************************
* Math.h                                                                   *
*                                                                          *
* Convenient constants, macros, and inline functions for basic math        *
* functions.                                                               *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
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
#ifndef __MATH_INCLUDED__
#define __MATH_INCLUDED__

#include <math.h>
#include <stdlib.h>

namespace ArvoMath {

#ifndef MAXFLOAT
#define MAXFLOAT 1.0E+20
#endif

	static const double
		Pi            = 3.14159265358979,
		PiSquared     = Pi * Pi,
		TwoPi         = 2.0 * Pi,
		FourPi        = 4.0 * Pi,
		PiOverTwo     = Pi / 2.0,
		PiOverFour    = Pi / 4.0,
		OverPi        = 1.0 / Pi,
		OverTwoPi     = 1.0 / TwoPi,
		OverFourPi    = 1.0 / FourPi,
		Infinity      = MAXFLOAT,
		Tiny          = 1.0 / MAXFLOAT,
		DegreesToRad  = Pi / 180.0,
		RadToDegrees  = 180.0 / Pi;

	inline int    Odd   ( int    k           ) { return k & 1; }
	inline int    Even  ( int    k           ) { return !(k & 1); }
	inline float  Abs   ( int    x           ) { return x > 0  ? x : -x; }
	inline float  Abs   ( float  x           ) { return x > 0. ? x : -x; }
	inline float  Abs   ( double x           ) { return x > 0. ? x : -x; }
	inline float  Min   ( float  x, float  y ) { return x < y ? x : y; }
	inline float  Max   ( float  x, float  y ) { return x > y ? x : y; }
	inline double dMin  ( double x, double y ) { return x < y ? x : y; }
	inline double dMax  ( double x, double y ) { return x > y ? x : y; }
	inline float  Sqr   ( int    x           ) { return x * x; }
	inline float  Sqr   ( float  x           ) { return x * x; }
	inline float  Sqr   ( double x           ) { return x * x; }
	inline float  Sqrt  ( double x           ) { return x > 0. ? sqrt(x) : 0.; }
	inline float  Cubed ( float  x           ) { return x * x * x; }
	inline int    Sign  ( float  x           ) { return x > 0. ? 1 : (x < 0. ? -1 : 0); }
	inline void   Swap  ( float &a, float &b ) { float c = a; a = b; b = c; }
	inline void   Swap  ( int   &a, int   &b ) { int   c = a; a = b; b = c; }
	inline double Sin   ( double x, int    n ) { return pow( sin(x), n ); }
	inline double Cos   ( double x, int    n ) { return pow( cos(x), n ); }
	inline float  ToSin ( double x           ) { return Sqrt( 1.0 - Sqr(x) ); }
	inline float  ToCos ( double x           ) { return Sqrt( 1.0 - Sqr(x) ); }
	inline float  MaxAbs( float  x, float  y ) { return Max( Abs(x), Abs(y) ); }
	inline float  MinAbs( float  x, float  y ) { return Min( Abs(x), Abs(y) ); }
	inline float  Pythag( double x, double y ) { return Sqrt( x*x + y*y ); }

	inline double ArcCos( double x )
	{
		double y;
		if( -1.0 <= x && x <= 1.0 ) y = acos( x );
		else if( x >  1.0 ) y = 0.0;
		else if( x < -1.0 ) y = Pi;
		return y;
	}

	inline double ArcSin( double x )
	{
		if( x < -1.0 ) x = -1.0;
		if( x >  1.0 ) x =  1.0;
		return asin( x );
	}

	inline float Clamp( float min, float &x, float max )
	{
		if( x < min ) x = min; else
			if( x > max ) x = max;
		return x;
	}

	inline double Clamp( float min, double &x, float max )
	{
		if( x < min ) x = min; else
			if( x > max ) x = max;
		return x;
	}

	inline float Max( float x, float y, float z )
	{
		float t;
		if( x >= y && x >= z ) t = x;
		else if( y >= z ) t = y;
		else t = z;
		return t;
	}

	inline float Min( float x, float y, float z )
	{
		float t;
		if( x <= y && x <= z ) t = x;
		else if( y <= z ) t = y;
		else t = z;
		return t;
	}

	inline double dMax( double x, double y, double z )
	{
		double t;
		if( x >= y && x >= z ) t = x;
		else if( y >= z ) t = y;
		else t = z;
		return t;
	}

	inline double dMin( double x, double y, double z )
	{
		double t;
		if( x <= y && x <= z ) t = x;
		else if( y <= z ) t = y;
		else t = z;
		return t;
	}

	inline float MaxAbs( float x, float y, float z )
	{
		return Max( Abs( x ), Abs( y ), Abs( z ) );
	}

	inline float Pythag( float x, float y, float z )
	{
		return sqrt( x * x  +  y * y  +  z * z );
	}

	extern float  ArcTan          ( float x, float y      );
	extern float  ArcQuad         ( float x, float y      );
	extern float  MachineEpsilon  (                       );
	extern double LogGamma        ( double x              );
	extern double LogFact         ( int n                 );
	extern double LogDoubleFact   ( int n                 );   // log( n!! )
	extern double BinomialCoeff   ( int n, int k          );
	extern void   BinomialCoeffs  ( int n, long   *coeffs );
	extern void   BinomialCoeffs  ( int n, double *coeffs );
	extern double MultinomialCoeff( int i, int j, int k   );
	extern double MultinomialCoeff( int k, int N[]        );
	extern double RelErr          ( double x, double y    );

#ifndef ABS
#define ABS( x ) ((x) > 0 ? (x) : -(x))
#endif

#ifndef MAX
#define MAX( x, y ) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN( x, y ) ((x) < (y) ? (x) : (y))
#endif

};

#endif







