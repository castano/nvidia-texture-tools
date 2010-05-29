/***************************************************************************
* Vec3.C                                                                   *
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
#include <stdio.h>
#include <math.h>
#include "ArvoMath.h"
#include "Vec3.h"
#include "form.h"

namespace ArvoMath {

	float Normalize( Vec3 &A )
	{
		float d = Len( A );
		if( d > 0.0 )
		{
			double c = 1.0 / d;
			A.X() *= c;
			A.Y() *= c;
			A.Z() *= c;
		}
		return( d );
	}

	double Angle( const Vec3 &A, const Vec3 &B )
	{
		double t = LenSqr(A) * LenSqr(B);
		if( t <= 0.0 ) return 0.0;
		return ArcCos( (A * B) / sqrt(t) );
	}

	/*-------------------------------------------------------------------------*
	* O R T H O N O R M A L                                                   *
	*                                                                         *
	* On Input  A, B....: Two linearly independent 3-space vectors.           *
	*                                                                         *
	* On Return A.......: Unit vector pointing in original A direction.       *
	*           B.......: Unit vector orthogonal to A and in subspace spanned *
	*                     by original A and B vectors.                        *
	*           C.......: Unit vector orthogonal to both A and B, chosen so   *
	*                     that A-B-C forms a right-handed coordinate system.  *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Orthonormal( Vec3 &A, Vec3 &B, Vec3 &C )
	{
		if( Normalize( A ) == 0.0 ) return 1;
		B /= A;
		if( Normalize( B ) == 0.0 ) return 1;
		C = A ^ B;
		return 0;
	}

	int Orthonormal( Vec3 &A, Vec3 &B )
	{
		if( Normalize( A ) == 0.0 ) return 1;
		B /= A;
		if( Normalize( B ) == 0.0 ) return 1;
		return 0;
	}

	/*-------------------------------------------------------------------------*
	* O R T H O G O N A L  T O                                                *
	*                                                                         *
	* Returns a vector that is orthogonal to A (but of arbitrary length).     *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Vec3 OrthogonalTo( const Vec3 &A )
	{
		float c = 0.5 * SupNorm( A );
		if( c ==       0.0  ) return Vec3(    1.0,    0.0,    0.0 );
		if( c <= Abs(A.X()) ) return Vec3( -A.Y(),  A.X(),    0.0 );
		if( c <= Abs(A.Y()) ) return Vec3(    0.0, -A.Z(),  A.Y() );
		return Vec3(  A.Z(),    0.0, -A.X() );
	}

	Vec3 Min( const Vec3 &A, const Vec3 &B )
	{
		return Vec3( 
			Min( A.X(), B.X() ),
			Min( A.Y(), B.Y() ),
			Min( A.Z(), B.Z() ));
	}

	Vec3 Max( const Vec3 &A, const Vec3 &B )
	{
		return Vec3( 
			Max( A.X(), B.X() ),
			Max( A.Y(), B.Y() ),
			Max( A.Z(), B.Z() ));
	}

	std::ostream &operator<<( std::ostream &out, const Vec3 &A )
	{
		out << form( " %9.5f %9.5f %9.5f", A.X(), A.Y(), A.Z() ) << std::endl;
		return out;
	}
};
