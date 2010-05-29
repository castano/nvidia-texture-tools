/***************************************************************************
* Vec4.C                                                                   *
*                                                                          *
* Basic operations on 3-dimensional vectors.  This special case is useful  *
* because many operations are performed inline.                            *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      walt    6/26/07     Edited Vec4 to make this new class              *
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
#include "Vec4.h"
#include "form.h"

namespace ArvoMath {

	float Normalize( Vec4 &A )
	{
		float d = Len( A );
		if( d > 0.0 )
		{
			double c = 1.0 / d;
			A.X() *= c;
			A.Y() *= c;
			A.Z() *= c;
			A.W() *= c;
		}
		return( d );
	}

	double Angle( const Vec4 &A, const Vec4 &B )
	{
		double t = LenSqr(A) * LenSqr(B);
		if( t <= 0.0 ) return 0.0;
		return ArcCos( (A * B) / sqrt(t) );
	}

	Vec4 Min( const Vec4 &A, const Vec4 &B )
	{
		return Vec4( 
			Min( A.X(), B.X() ),
			Min( A.Y(), B.Y() ),
			Min( A.Z(), B.Z() ),
			Min( A.W(), B.W() ) );
	}

	Vec4 Max( const Vec4 &A, const Vec4 &B )
	{
		return Vec4( 
			Max( A.X(), B.X() ),
			Max( A.Y(), B.Y() ),
			Max( A.Z(), B.Z() ),
			Max( A.W(), B.W() ) );
	}

	std::ostream &operator<<( std::ostream &out, const Vec4 &A )
	{
		out << form( " %9.5f %9.5f %9.5f %9.5f", A.X(), A.Y(), A.Z(), A.W() ) << std::endl;
		return out;
	}
};
