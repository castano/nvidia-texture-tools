/***************************************************************************
* SVD.C                                                                    *
*                                                                          *
* Singular Value Decomposition.                                            *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date          Description                                   *
*                                                                          *
*      arvo    08/22/2000    Copied to CIT library.                        *
*      arvo    06/28/1993    Rewritten from "Numerical Recipes" C-code.    *
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
#include <math.h>
#include <assert.h>
#include "ArvoMath.h"
#include "Vector.h"
#include "Matrix.h"
#include "SVD.h"

namespace ArvoMath {
	static const int MaxIterations = 30;

	static double svd_pythag( double a, double b )
	{
		double at = Abs(a);
		double bt = Abs(b);
		if( at > bt )
			return at * sqrt( 1.0 + Sqr( bt / at ) );
		else if( bt > 0.0 )
			return bt * sqrt( 1.0 + Sqr( at / bt ) );
		else return 0.0;
	}

	static inline double SameSign( double a, double b ) 
	{
		double t;
		if( b >= 0.0 ) t = Abs( a );
		else t = -Abs( a );
		return t;
	}

	static int ComputeRank( const Matrix &D, double epsilon )
	{
		int rank = 0;
		for( int i = 0; i < D.Rows(); i++ )
			if( Abs(D(i,i)) > epsilon ) rank++;
		return rank;
	}

	SVD::SVD( ) : Q_(0), D_(0), R_(0)
	{
	}

	SVD::SVD( const Matrix &M ) : Q_(0), D_(0), R_(0)
	{
		(*this) = M;
	}

	void SVD::operator=( const Matrix &A )
	{
		if( A.Rows() >= A.Cols() ) Q_ = A;
		else
		{
			Q_ = Matrix( A.Cols() );
			for( int i = 0; i < A.Rows(); i++ )
				for( int j = 0; j < A.Cols(); j++ ) Q_(i,j) = A(i,j);
		}
		R_ = Matrix( A.Cols() );
		Decompose( Q_, D_, R_ );
	}

	const Matrix &SVD::Q( double epsilon ) const
	{
		int rank = 0;
		if( epsilon != 0.0 ) rank = ComputeRank( D_, epsilon );
		return Q_;
	}

	const Matrix &SVD::D( double epsilon ) const
	{
		int rank = 0;
		if( epsilon != 0.0 ) rank = ComputeRank( D_, epsilon );
		return D_;
	}

	const Matrix &SVD::R( double epsilon ) const
	{
		int rank = 0;
		if( epsilon != 0.0 ) rank = ComputeRank( D_, epsilon );
		return R_;
	}

	int SVD::Rank( double epsilon ) const
	{
		return ComputeRank( D_, epsilon );
	}

	int SVD::Decompose( Matrix &Q, Matrix &D, Matrix &R )
	{
		int    i, j, k, l, m, n, p, q, iter;
		double c, f, h, s, x, y, z;
		double norm  = 0.0;
		double g     = 0.0;
		double scale = 0.0;

		m = Q.Rows();
		n = Q.Cols();

		Vector Temp( n );
		Vector diag( n );

		for( i = 0; i < n; i++ ) 
		{

			Temp(i) = scale * g;
			scale   = 0.0;
			g       = 0.0;
			s       = 0.0;
			l       = i + 1;

			if( i < m )
			{
				for( k = i; k < m; k++ ) scale += Abs( Q(k,i) );
				if( scale != 0.0 ) 
				{
					for( k = i; k < m; k++ ) 
					{
						Q(k,i) /= scale;
						s += Sqr( Q(k,i) );
					}
					f = Q(i,i);
					g = -SameSign( sqrt(s), f );
					h = f * g - s;
					Q(i,i) = f - g;
					if( i != n - 1 )
					{
						for( j = l; j < n; j++ ) 
						{
							s = 0.0;
							for( k = i; k < m; k++ ) s += Q(k,i) * Q(k,j);
							f = s / h;
							for( k = i; k < m; k++ ) Q(k,j) += f * Q(k,i);
						}
					}
					for( k = i; k < m; k++ ) Q(k,i) *= scale;
				}
			}

			diag(i) = scale * g;
			g       = 0.0;
			s       = 0.0;
			scale   = 0.0;

			if( i < m && i != n - 1 ) 
			{
				for( k = l; k < n; k++ ) scale += Abs( Q(i,k) );
				if( scale != 0.0 ) 
				{
					for( k = l; k < n; k++ ) 
					{
						Q(i,k) /= scale;
						s += Sqr( Q(i,k) );
					}
					f = Q(i,l);
					g = -SameSign( sqrt(s), f );
					h = f * g - s;
					Q(i,l) = f - g;
					for( k = l; k < n; k++ ) Temp(k) = Q(i,k) / h;
					if( i != m - 1 ) 
					{
						for( j = l; j < m; j++ ) 
						{
							s = 0.0;
							for( k = l; k < n; k++ ) s += Q(j,k) * Q(i,k);
							for( k = l; k < n; k++ ) Q(j,k) += s * Temp(k);
						}
					}
					for( k = l; k < n; k++ ) Q(i,k) *= scale;
				}
			}
			norm = Max( norm, Abs( diag(i) ) + Abs( Temp(i) ) );
		}


		for( i = n - 1; i >= 0; i-- ) 
		{
			if( i < n - 1 ) 
			{
				if( g != 0.0 ) 
				{
					for( j = l; j < n; j++ ) R(i,j) = ( Q(i,j) / Q(i,l) ) / g;
					for( j = l; j < n; j++ ) 
					{
						s = 0.0;
						for( k = l; k < n; k++ ) s += Q(i,k) * R(j,k);
						for( k = l; k < n; k++ ) R(j,k) += s * R(i,k);
					}
				}
				for( j = l; j < n; j++ ) 
				{
					R(i,j) = 0.0;
					R(j,i) = 0.0;
				}
			}
			R(i,i) = 1.0;
			g = Temp(i);
			l = i;
		}


		for( i = n - 1; i >= 0; i-- ) 
		{
			l = i + 1;
			g = diag(i);
			if( i < n - 1 ) for( j = l; j < n; j++ ) Q(i,j) = 0.0;
			if( g != 0.0 ) 
			{
				g = 1.0 / g;
				if( i != n - 1 ) 
				{
					for( j = l; j < n; j++ ) 
					{
						s = 0.0;
						for( k = l; k < m; k++ ) s += Q(k,i) * Q(k,j);
						f = ( s / Q(i,i) ) * g;
						for( k = i; k < m; k++ ) Q(k,j) += f * Q(k,i);
					}
				}
				for( j = i; j < m; j++ ) Q(j,i) *= g;
			} 
			else 
			{
				for( j = i; j < m; j++ ) Q(j,i) = 0.0;
			}
			Q(i,i) += 1.0;
		}


		for( k = n - 1; k >= 0; k-- ) 
		{
			for( iter = 1; iter <= MaxIterations; iter++ ) 
			{
				int jump;

				for( l = k; l >= 0; l-- )
				{
					q = l - 1;
					if( Abs( Temp(l) ) + norm == norm ) { jump = 1; break; }
					if( Abs( diag(q) ) + norm == norm ) { jump = 0; break; }
				}

				if( !jump )
				{
					c = 0.0;
					s = 1.0;
					for( i = l; i <= k; i++ )
					{
						f = s * Temp(i);
						Temp(i) *= c;
						if( Abs( f ) + norm == norm ) break;
						g = diag(i);
						h = svd_pythag( f, g );
						diag(i) = h;
						h = 1.0 / h;
						c = g * h;
						s = -f * h;
						for( j = 0; j < m; j++ ) 
						{
							y = Q(j,q);
							z = Q(j,i);
							Q(j,q) = y * c + z * s;
							Q(j,i) = z * c - y * s;
						}
					}
				}

				z = diag(k);
				if( l == k ) 
				{
					if( z < 0.0 ) 
					{
						diag(k) = -z;
						for( j = 0; j < n; j++ ) R(k,j) *= -1.0; 
					}
					break;
				}
				if( iter >= MaxIterations ) return 0;
				x = diag(l);
				q = k - 1;
				y = diag(q);
				g = Temp(q);
				h = Temp(k);
				f = ( ( y - z ) * ( y + z ) + ( g - h ) * ( g + h ) ) / ( 2.0 * h * y );
				g = svd_pythag( f, 1.0 );
				f = ( ( x - z ) * ( x + z ) + h * ( ( y / ( f + SameSign( g, f ) ) ) - h ) ) / x;
				c = 1.0;
				s = 1.0;
				for( j = l; j <= q; j++ ) 
				{
					i = j + 1;
					g = Temp(i);
					y = diag(i);
					h = s * g;
					g = c * g;
					z = svd_pythag( f, h );
					Temp(j) = z;
					c = f / z;
					s = h / z;
					f = x * c + g * s;
					g = g * c - x * s;
					h = y * s;
					y = y * c;
					for( p = 0; p < n; p++ ) 
					{
						x = R(j,p);
						z = R(i,p);
						R(j,p) = x * c + z * s;
						R(i,p) = z * c - x * s;
					}
					z = svd_pythag( f, h );
					diag(j) = z;
					if( z != 0.0 ) 
					{
						z = 1.0 / z;
						c = f * z;
						s = h * z;
					}
					f = c * g + s * y;
					x = c * y - s * g;
					for( p = 0; p < m; p++ ) 
					{
						y = Q(p,j);
						z = Q(p,i);
						Q(p,j) = y * c + z * s;
						Q(p,i) = z * c - y * s;
					}
				}
				Temp(l) = 0.0;
				Temp(k) = f;
				diag(k) = x;
			}
		}

		// Sort the singular values into descending order.

		for( i = 0; i < n - 1; i++ )
		{
			double biggest = diag(i);  // Biggest singular value so far.
			int    bindex  = i;        // The row/col it occurred in.
			for( j = i + 1; j < n; j++ )
			{
				if( diag(j) > biggest ) 
				{
					biggest = diag(j);
					bindex  = j;
				}            
			}
			if( bindex != i )  // Need to swap rows and columns.
			{
				Q.SwapCols( i, bindex );  // Swap columns in Q.
				R.SwapRows( i, bindex );  // Swap rows in R.
				diag.Swap ( i, bindex );  // Swap elements in diag.
			}
		}

		D = Diag( diag );
		return 1;
	}


	const Matrix &SVD::PseudoInverse( double epsilon )
	{
		if( Null(P_) )
		{
			Matrix D_Inverse( D_ );
			for( int i = 0; i < D_Inverse.Rows(); i++ )
			{
				if( Abs( D_Inverse(i,i) ) > epsilon )
					D_Inverse(i,i) = 1.0 / D_Inverse(i,i);
				else D_Inverse(i,i) = 0.0;
			}
			P_ = Q_ * D_Inverse * R_;
		}
		return P_;
	}
};
