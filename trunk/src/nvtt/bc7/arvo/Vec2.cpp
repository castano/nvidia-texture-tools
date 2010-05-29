/***************************************************************************
* Vec2.C                                                                   *
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
#include <math.h>
#include "ArvoMath.h"
#include "Vec2.h"
#include "form.h"

namespace ArvoMath {

	const Vec2 Vec2::Zero;
	const Vec2 Vec2::Xaxis( 1, 0 );
	const Vec2 Vec2::Yaxis( 0, 1 );

	// Most routines are now inline.

	float Normalize( Vec2 &A )
	{
		float d = Len( A );
		if( d != 0.0 )
		{
			A.X() /= d;
			A.Y() /= d;
		}
		return d;
	}

	Vec2 Min( const Vec2 &A, const Vec2 &B )
	{
		return Vec2( Min( A.X(), B.X() ), Min( A.Y(), B.Y() ) );
	}

	Vec2 Max( const Vec2 &A, const Vec2 &B )
	{
		return Vec2( Max( A.X(), B.X() ), Max( A.Y(), B.Y() ) );
	}

	std::ostream &operator<<( std::ostream &out, const Vec2 &A )
	{
		out << form( " %9.5f %9.5f\n", A.X(), A.Y() );
		return out;
	}

	std::ostream &operator<<( std::ostream &out, const Mat2x2 &M )
	{
		out << form( " %9.5f %9.5f\n", M(0,0), M(0,1) )
			<< form( " %9.5f %9.5f\n", M(1,0), M(1,1) )
			<< std::endl;
		return out;
	}

	Mat2x2::Mat2x2( const Vec2 &c1, const Vec2 &c2 ) 
	{ 
		m[0][0] = c1.X(); 
		m[1][0] = c1.Y(); 
		m[0][1] = c2.X();
		m[1][1] = c2.Y();
	}

	// Return solution x of the system Ax = b.
	Vec2 Solve( const Mat2x2 &A, const Vec2 &b )
	{
		float MachEps = MachineEpsilon();
		Vec2 x;
		double d = det( A );
		double n = Norm1( A );
		if( n <= MachEps || Abs(d) <= MachEps * n ) return Vec2::Zero;
		x.X() =  A(1,1) * b.X() - A(0,1) * b.Y();
		x.Y() = -A(1,0) * b.X() + A(0,0) * b.Y();
		return x / d;
	}
};
