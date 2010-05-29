/***************************************************************************
* Vector.C                                                                 *
*                                                                          *
* General Vector and Matrix classes, with all the associated methods.      *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      arvo    08/16/2000    Revamped for CIT tools.                       *
*      arvo    10/31/1994    Combined RowVec & ColVec into Vector.         *
*      arvo    06/30/1993    Added singular value decomposition class.     *
*      arvo    06/25/1993    Major revisions.                              *
*      arvo    09/08/1991    Initial implementation.                       *
*                                                                          *
*--------------------------------------------------------------------------*
* Copyright (C) 2000, James Arvo                                           *
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
#include <assert.h>
#include "ArvoMath.h"
#include "Vector.h"
#include "form.h"

namespace ArvoMath {

	const Vector Vector::Null(0);

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  C O N S T R U C T O R S                                                *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Vector::Vector( const float *x, int n )
	{
		Create( n );
		for( register int i = 0; i < size; i++ ) elem[i] = x[i];
	}

	Vector::Vector( const Vector &A )
	{
		Create( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) elem[i] = A(i);
	}

	Vector::Vector( int n )
	{
		Create( n );
		for( register int i = 0; i < n; i++ ) elem[i] = 0.0;
	}

	Vector::Vector( float x, float y )
	{
		Create( 2 );
		elem[0] = x;
		elem[1] = y;
	}

	Vector::Vector( float x, float y, float z )
	{
		Create( 3 );
		elem[0] = x;
		elem[1] = y;
		elem[2] = z;
	}

	void Vector::SetSize( int new_size )
	{
		if( size != new_size )
		{
			delete[] elem;
			Create( new_size );
			for( register int i = 0; i < new_size; i++ ) elem[i] = 0.0;
		}
	}

	Vector &Vector::Swap( int i, int j )
	{
		float temp = elem[i];
		elem[i]    = elem[j];
		elem[j]    = temp;
		return *this;
	}

	Vector Vector::GetBlock( int i, int j ) const
	{
		assert( 0 <= i && i <= j && j < size );
		int n = j - i + 1;
		Vector V( n );
		register float *v = V.Array();
		register float *e = elem + i;
		for( register int k = 0; k < n; k++ ) *v++ = *e++;
		return V;
	}

	void Vector::SetBlock( int i, int j, const Vector &V )
	{
		assert( 0 <= i && i <= j && j < size );
		int n = j - i + 1;
		assert( n == V.Size() );
		register float *v = V.Array();
		register float *e = elem + i;
		for( register int k = 0; k < n; k++ ) *e++ = *v++;
	}

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  O P E R A T O R S                                                      *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	double operator*( const Vector &A, const Vector &B )
	{
		assert( A.Size() == B.Size() );
		double sum = A(0) * B(0);
		for( register int i = 1; i < A.Size(); i++ ) sum += A(i) * B(i);
		return sum;
	}

	void Vector::operator=( float c )
	{
		for( register int i = 0; i < size; i++ ) elem[i] = c;
	}

	Vector operator*( const Vector &A, float s ) 
	{
		Vector C( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) C(i) = A(i) * s;
		return C;
	}

	Vector operator*( float s, const Vector &A ) 
	{
		Vector C( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) C(i) = A(i) * s;
		return C;
	}

	Vector operator/( const Vector &A, float s ) 
	{
		assert( s != 0.0 );
		Vector C( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) C(i) = A(i) / s;
		return C;
	}

	Vector& operator+=( Vector &A, const Vector &B ) 
	{
		assert( A.Size() == B.Size() );
		for( register int i = 0; i < A.Size(); i++ ) A(i) += B(i);
		return A;
	}

	Vector& operator*=( Vector &A, float scale ) 
	{
		for( register int i = 0; i < A.Size(); i++ ) A(i) *= scale;
		return A;
	}

	Vector& operator/=( Vector &A, float scale ) 
	{
		for( register int i = 0; i < A.Size(); i++ ) A(i) /= scale;
		return A;
	}

	Vector& Vector::operator=( const Vector &A )
	{
		SetSize( A.Size() );
		for( register int i = 0; i < size; i++ ) elem[i] = A(i);
		return *this;
	}

	Vector operator+( const Vector &A, const Vector &B ) 
	{
		assert( A.Size() == B.Size() );
		Vector C( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) C(i) = A(i) + B(i);
		return C;
	}

	Vector operator-( const Vector &A, const Vector &B ) 
	{
		assert( A.Size() == B.Size() );
		Vector C( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) C(i) = A(i) - B(i);
		return C;
	}

	Vector operator-( const Vector &A )  // Unary minus.
	{
		Vector B( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) B(i) = -A(i);
		return B;
	}

	Vector operator^( const Vector &A, const Vector &B )
	{
		Vector C(3);
		assert( A.Size() == B.Size() );
		if( A.Size() == 2 ) // Assume z components of A and B are zero.
		{
			C(0) = 0.0;
			C(1) = 0.0;
			C(2) = A(0) * B(1) - A(1) * B(0);
		}
		else 
		{
			assert( A.Size() == 3 );
			C(0) = A(1) * B(2) - A(2) * B(1);
			C(1) = A(2) * B(0) - A(0) * B(2);
			C(2) = A(0) * B(1) - A(1) * B(0);
		}
		return C;
	}

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  M I S C E L L A N E O U S   F U N C T I O N S                          *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Vector Min( const Vector &A, const Vector &B )
	{
		assert( A.Size() == B.Size() );
		Vector C( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) C(i) = Min( A(i), B(i) );
		return C;
	}

	Vector Max( const Vector &A, const Vector &B )
	{
		assert( A.Size() == B.Size() );
		Vector C( A.Size() );
		for( register int i = 0; i < A.Size(); i++ ) C(i) = Max( A(i), B(i) );
		return C;
	}

	Vector Unit( const Vector &A )
	{
		double norm = TwoNorm( A );
		assert( norm > 0.0 );
		return A * ( 1.0 / norm );
	}

	double Normalize( Vector &A )
	{
		double norm = TwoNorm( A );
		assert( norm > 0.0 );
		for( register int i = 0; i < A.Size(); i++ ) A(i) /= norm;
		return norm;
	}

	int Null( const Vector &A ) 
	{
		return A.Size() == 0;
	}

	double TwoNormSqr( const Vector &A )
	{
		double sum = A(0) * A(0);
		for( register int i = 1; i < A.Size(); i++ ) sum += A(i) * A(i);
		return sum;
	}

	double TwoNorm( const Vector &A )
	{
		return sqrt( TwoNormSqr( A ) );
	}

	double dist( const Vector &A, const Vector &B )
	{
		return TwoNorm( A - B );
	}

	double OneNorm( const Vector &A )
	{
		double norm = Abs( A(0) );
		for( register int i = 1; i < A.Size(); i++ ) norm += Abs( A(i) );
		return norm;
	}

	double SupNorm( const Vector &A )
	{
		double norm = Abs( A(0) );
		for( register int i = 1; i < A.Size(); i++ )
		{
			double a = Abs( A(i) );
			if( a > norm ) norm = a;
		}
		return norm;
	}

	Vec2 ToVec2( const Vector &V )
	{
		assert( V.Size() == 2 );
		return Vec2( V(0), V(1) );
	}

	Vec3 ToVec3( const Vector &V )
	{
		assert( V.Size() == 3 );
		return Vec3( V(0), V(1), V(2) );
	}

	Vector ToVector( const Vec2 &V )
	{
		return Vector( V.X(), V.Y() );
	}

	Vector ToVector( const Vec3 &V )
	{
		return Vector( V.X(), V.Y(), V.Z() );
	}

	//
	// Returns a vector that is orthogonal to A (but of arbitrary length). 
	//
	Vector OrthogonalTo( const Vector &A )
	{
		Vector B( A.Size() );
		double c = 0.5 * SupNorm( A );

		if( A.Size() < 2 ) 
		{
			// Just return the zero-vector.
		}
		else if( c == 0.0 ) 
		{
			B(0) = 1.0;
		}
		else for( register int i = 0; i < A.Size(); i++ )
		{
			if( Abs( A(i)) > c )
			{
				int k = ( i > 0 ) ? i - 1 : i + 1;
				B(k) = -A(i);
				B(i) =  A(k);
				break;
			}
		}
		return B;
	}

	std::ostream &operator<<( std::ostream &out, const Vector &A )
	{
		if( A.Size() == 0 )
		{
			out << "NULL";
		}
		else for( register int i = 0; i < A.Size(); i++ )
		{
			out << form( "%3d:  %10.5g\n", i, A(i) );
		}
		out << std::endl;
		return out;
	}


};
