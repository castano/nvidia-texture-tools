/***************************************************************************
* Matrix.C                                                                 *
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
#include <math.h>
#include "ArvoMath.h"
#include "Vector.h"
#include "Matrix.h"
#include "form.h"

namespace ArvoMath {
	const Matrix Matrix::Null(0);

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  C O N S T R U C T O R S                                                *
	*                                                                         *
	*-------------------------------------------------------------------------*/

	// Create a new matrix of the given size.  If n_cols is zero (the default), 
	// it is assumed that the matrix is to be square; that is, n_rows x n_rows.  
	// The matrix is filled with "value", which defaults to zero.
	Matrix::Matrix( int n_rows, int n_cols, float value ) 
	{
		assert( n_rows >= 0 && n_cols >= 0 );
		rows = 0;
		cols = 0;
		elem = NULL;
		SetSize( n_rows, n_cols );
		float *e = elem;
		for( register int i = 0; i < rows * cols; i++ ) *e++ = value;
	}

	// Copy constructor.
	Matrix::Matrix( const Matrix &M ) 
	{
		rows = 0;
		cols = 0;
		elem = NULL;
		SetSize( M.Rows(), M.Cols() );
		register float *e = elem;
		register float *m = M.Array();
		for( register int i = 0; i < rows * cols; i++ ) *e++ = *m++;
	}

	Matrix::~Matrix() 
	{
		SetSize( 0, 0 );
	}

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  M I S C E L L A N E O U S   M E T H O D S                              *
	*                                                                         *
	*-------------------------------------------------------------------------*/

	// Re-shape the matrix.  If the number of elements in the new matrix is
	// different from the original matrix, the original data is deleted and
	// replaced with a new array.  If new_cols is zero (the default), it is
	// assumed to be the same as new_rows -- i.e. a square matrix.
	void Matrix::SetSize( int new_rows, int new_cols )
	{
		if( new_cols == 0 ) new_cols = new_rows;
		int n = new_rows * new_cols;
		if( rows * cols != n )
		{
			if( elem != NULL ) delete[] elem;
			elem = ( n == 0 ) ? NULL : new float[ n ];
		}
		rows = new_rows;
		cols = new_cols;
	}

	Vector Matrix::GetCol( int j ) const
	{
		Vector C( rows );
		float *e = elem + j;
		float *c = C.Array();
		for( int i = 0; i < rows; i++ )
		{
			*c++ = *e;
			e += cols;
		}
		return C;
	}

	Vector Matrix::GetRow( int i ) const
	{
		Vector R( cols );
		float *e = elem + ( i * cols );
		float *r = R.Array();
		for( int j = 0; j < cols; j++ ) *r++ = *e++;
		return R;
	}

	void Matrix::SetCol( int j, const Vector &C )
	{
		assert( rows == C.Size() );
		float *e = elem + j;
		float *c = C.Array();
		for( int i = 0; i < rows; i++ )
		{
			*e = *c++;
			e += cols;
		}
	}

	void Matrix::SetRow( int i, const Vector &R )
	{
		assert( cols == R.Size() );
		float *e = elem + ( i * cols );
		float *r = R.Array();
		for( int j = 0; j < cols; j++ ) *e++ = *r++;
	}

	Matrix Matrix::GetBlock( int imin, int imax, int jmin, int jmax ) const
	{
		if( imax < imin || jmax < jmin ) return Matrix(0,0);
		Matrix M( imax - imin + 1, jmax - jmin + 1 );
		for( int i = imin; i <= imax; i++ )
			for( int j = jmin; j <= jmax; j++ )
			{
				M( i - imin, j - jmin ) = (*this)( i, j );
			}
			return M;
	}

	void Matrix::SetBlock( int imin, int imax, int jmin, int jmax, const Matrix &B )
	{
		int ni = imax - imin + 1;
		int nj = jmax - jmin + 1;
		assert( ni == B.Rows() );
		assert( nj == B.Cols() );
		int k = imin * cols + jmin;
		for( int i = 0; i < ni; i++ )
			for( int j = 0; j < nj; j++ )
			{
				elem[ k + i * cols + j ] = B(i,j);
			}
	}

	void Matrix::SetBlock( int imin, int imax, int jmin, int jmax, const Vector &V )
	{
		int k = imin * cols + jmin;
		if( imin == imax )
		{
			int nj = jmax - jmin + 1;
			assert( nj == V.Size() );
			for( int j = 0; j < nj; j++ ) elem[ k + j ] = V(j);
		}
		else if( jmin == jmax )
		{
			int ni = imax - imin + 1;
			assert( ni == V.Size() );
			for( int i = 0; i < ni; i++ ) elem[ k + i * cols ] = V(i);
		}
		else 
		{
			// This assertion will be false, and will signal an error.
			assert( imin == imax || jmin == jmax );
		}
	}

	Matrix &Matrix::SwapRows( int i1, int i2 )
	{
		float temp;
		float *r1 = elem + ( i1 * cols );
		float *r2 = elem + ( i2 * cols );
		for( register int j = 0; j < cols; j++ )
		{
			temp = *r1;
			*r1  = *r2;
			*r2  = temp;
			r1++;
			r2++;
		}
		return *this;
	}

	Matrix &Matrix::SwapCols( int j1, int j2 )
	{
		float temp;
		float *c1 = elem + j1;
		float *c2 = elem + j2;
		for( register int i = 0; i < rows; i++ )
		{
			temp = *c1;
			*c1  = *c2;
			*c2  = temp;
			c1 += cols;
			c2 += cols;
		}
		return *this;
	}

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  A S S I G N M E N T    O P E R A T O R S                               *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Matrix& Matrix::operator=( const Matrix &M ) 
	{
		SetSize( M.Rows(), M.Cols() );
		register float *e = elem;
		register float *m = M.Array();
		for( register int i = 0; i < rows * cols; i++ ) *e++ = *m++;
		return *this;
	}

	Matrix& Matrix::operator=( float s ) 
	{
		register float *e = elem;
		for( register int i = 0; i < rows * cols; i++ ) *e++ = s;
		return *this;
	}

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  O P E R A T O R S                                                      *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Vector operator*( const Matrix &M, const Vector &A ) 
	{
		// Handle the special case with translation built in.
		if( M.Cols() == 4 && M.Rows() == 4 && A.Size() == 3 )
		{
			Vector C(3);
			C(0) = M(0,0) * A(0) + M(0,1) * A(1) + M(0,2) * A(2) + M(0,3);
			C(1) = M(1,0) * A(0) + M(1,1) * A(1) + M(1,2) * A(2) + M(1,3);
			C(2) = M(2,0) * A(0) + M(2,1) * A(1) + M(2,2) * A(2) + M(2,3);
			return C;
		}
		assert( M.Cols() == A.Size() );
		Vector C( M.Rows() );
		float *m = M.Array();
		for( int i = 0; i < M.Rows(); i++ ) 
		{
			register float *a  = A.Array();
			register double sum = (*m++) * (*a++);
			for( register int j = 1; j < M.Cols(); j++ ) 
				sum += (*m++) * (*a++);
			C(i) = sum;
		}
		return C;
	}

	Vector operator*( const Vector &A, const Matrix &M ) 
	{
		assert( A.Size() == M.Rows() );
		Vector C( M.Cols() );
		for( register int j = 0; j < M.Cols(); j++ ) 
		{
			register double sum = 0.0;
			register float *a = A.Array();
			for( register int i = 0; i < M.Rows(); i++ ) 
				sum += (*a++) * M(i,j);
			C(j) = sum;
		}
		return C;
	}

	Vector& operator*=( Vector &A, const Matrix &M ) 
	{
		// Handle the special case with translation built in.
		if( M.Cols() == 4 && M.Rows() == 4 && A.Size() == 3 )  
		{
			float x = M(0,0) * A(0) + M(0,1) * A(1) + M(0,2) * A(2) + M(0,3);
			float y = M(1,0) * A(0) + M(1,1) * A(1) + M(1,2) * A(2) + M(1,3);
			float z = M(2,0) * A(0) + M(2,1) * A(1) + M(2,2) * A(2) + M(2,3);
			A(0) = x;
			A(1) = y;
			A(2) = z;
			return A;
		}
		assert( M.Cols() == A.Size() );
		Vector C( M.Rows() );
		float *m = M.Array();
		for( register int i = 0; i < M.Rows(); i++ ) 
		{
			double sum = 0.0;
			for( register int j = 0; j < A.Size(); j++ ) 
				sum += (*m++) * A(j);
			C(i) = sum;
		}
		return A = C;
	}

	Matrix& operator*=( Matrix &M, float s ) 
	{
		register float *m = M.Array();
		for( register int i = 0; i < M.Rows() * M.Cols(); i++ ) *m++ *= s;
		return M;
	}

	Matrix& operator/=( Matrix &M, float s ) 
	{
		assert( s != 0.0 );
		register float *m = M.Array();
		for( register int i = 0; i < M.Rows() * M.Cols(); i++ ) *m++ /= s;
		return M;
	}

	Matrix operator+( const Matrix &A, const Matrix &B ) 
	{
		assert( A.Rows() == B.Rows() );
		assert( A.Cols() == B.Cols() );
		Matrix C( A.Rows(), A.Cols() );
		register float *a = A.Array();
		register float *b = B.Array();
		register float *c = C.Array();
		for( register int i = 0; i < A.Rows() * A.Cols(); i++ ) (*c++) = (*a++) + (*b++);
		return C;
	}

	Matrix operator-( const Matrix &A, const Matrix &B ) 
	{
		assert( A.Rows() == B.Rows() );
		assert( A.Cols() == B.Cols() );
		Matrix C( A.Rows(), A.Cols() );
		register float *a = A.Array();
		register float *b = B.Array();
		register float *c = C.Array();
		for( register int i = 0; i < A.Rows() * A.Cols(); i++ ) (*c++) = (*a++) - (*b++);
		return C;
	}

	Matrix operator-( const Matrix &A )
	{
		Matrix B( A.Cols(), A.Rows() );
		register float *a = A.Array();
		register float *b = B.Array();
		for( register int i = 0; i < A.Rows() * A.Cols(); i++ )
		{
			*b++ = -(*a++);
		}
		return B;
	}

	Matrix& operator+=( Matrix &A, const Matrix &B ) 
	{
		assert( A.Rows() == B.Rows() );
		assert( A.Cols() == B.Cols() );
		register float *a = A.Array();
		register float *b = B.Array();
		for( register int i = 0; i < A.Rows() * A.Cols(); i++ ) (*a++) += (*b++);
		return A;
	}

	Matrix operator*( const Matrix &A, const Matrix &B )
	{
		assert( A.Cols() == B.Rows() );
		Matrix M( A.Rows(), B.Cols() );
		for( register int i = 0; i < A.Rows(); i++ )
			for( register int j = 0; j < B.Cols(); j++ )
			{
				double sum = 0.0;
				for( register int k = 0; k < A.Cols(); k++ ) sum += A(i,k) * B(k,j);
				M(i,j) = sum;
			}
			return M;
	}

	Matrix operator*( float s, const Matrix &A )
	{
		Matrix B( A.Cols(), A.Rows() );
		register float *a = A.Array();
		register float *b = B.Array();
		for( register int i = 0; i < A.Rows() * A.Cols(); i++ )
		{
			*b++ = s * (*a++);
		}
		return B;
	}

	Matrix operator*( const Matrix &A, float s )
	{
		Matrix B( A.Cols(), A.Rows() );
		register float *a = A.Array();
		register float *b = B.Array();
		for( register int i = 0; i < A.Rows() * A.Cols(); i++ )
		{
			*b++ = s * (*a++);
		}
		return B;
	}

	Matrix operator/( const Matrix &A, float s )
	{
		assert( s != 0.0 );
		Matrix B( A.Cols(), A.Rows() );
		register float *a = A.Array();
		register float *b = B.Array();
		for( register int i = 0; i < A.Rows() * A.Cols(); i++ )
		{
			*b++ = (*a++) / s;
		}
		return B;
	}

	Matrix& operator*=( Matrix &A, const Matrix &B )
	{
		assert( A.Cols() == B.Rows() );
		Vector R( B.Cols() );
		for( register int i = 0; i < A.Rows(); i++ )
		{
			for( register int j = 0; j < B.Cols(); j++ )  // Compute the ith row of A * B.
			{
				double sum = A(i,0) * B(0,j);
				for( register int k = 1; k < A.Cols(); k++ ) sum += A(i,k) * B(k,j);
				R(j) = sum;
			}
			// Copy the new i'th row back into A.
			for( register int k = 0; k < A.Cols(); k++ ) A(i,k) = R(k); 
		}
		return A;
	}

	/*-------------------------------------------------------------------------*
	*                                                                         *
	*  M I S C E L L A N E O U S   F U N C T I O N S                          *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Matrix Transp( const Matrix &M )
	{
		Matrix T( M.Cols(), M.Rows() );
		register float *m = M.Array();
		for( register int i = 0; i < M.Rows(); i++ )
			for( register int j = 0; j < M.Cols(); j++ ) T(j,i) = *m++;
		return T;
	}

	// Computes A * Transp(A).
	Matrix AATransp( const Matrix &A )
	{
		int n = A.Rows();
		Matrix B( n, n );
		for( register int i = 0; i < n; i++ )
			for( register int j = 0; j < n; j++ ) 
			{
				double sum = 0.0;
				for( register int k = 0; k < A.Cols(); k++ ) 
					sum += A(i,k) * A(j,k);
				B(i,j) = sum;
			}
			return B;
	}

	// Computes Transp(A) * A.
	Matrix ATranspA( const Matrix &A )
	{
		int n = A.Cols();
		Matrix B( n, n );
		for( register int i = 0; i < n; i++ )
			for( register int j = 0; j < n; j++ ) 
			{
				double sum = 0.0;
				for( register int k = 0; k < A.Rows(); k++ ) 
					sum += A(k,i) * A(k,j);
				B(i,j) = sum;
			}
			return B;
	}

	// Computes the outer product of the vectors A and B.
	Matrix Outer( const Vector &A, const Vector &B ) 
	{
		Matrix M( A.Size(), B.Size() );
		for( register int i = 0; i < A.Size(); i++ )
		{
			float c = A(i);
			for( register int j = 0; j < B.Size(); j++ ) M(i,j) = c * B(j);
		}
		return M;
	}

	// Computes the L1-norm of the matrix A, which is the maximum absolute
	// row sum.
	double OneNorm( const Matrix &A )
	{
		double norm = 0.0;
		for( register int i = 0; i < A.Rows(); i++ )
		{
			double sum = 0.0;
			for( register int j = 0; j < A.Cols(); j++ ) sum += Abs( A(i,j) );
			if( sum > norm ) norm = sum;
		}
		return norm;
	}

	// Computes the L-infinity norm of the matrix A, which is the maximum 
	// absolute column sum.
	double SupNorm( const Matrix &A )
	{
		double norm = 0.0;
		for( register int j = 0; j < A.Cols(); j++ )
		{
			double sum = 0.0;
			for( register int i = 0; i < A.Rows(); i++ ) sum += Abs( A(i,j) );
			if( sum > norm ) norm = sum;
		}
		return norm;
	}

	// Returns the square matrix with the elements of the vector d along
	// its diagonal.
	Matrix Diag( const Vector &d ) 
	{
		Matrix D( d.Size() );
		for( register int i = 0; i < d.Size(); i++ ) D(i,i) = d(i);
		return D;
	}

	// Returns the 3 x 3 diagonal matrix with x, y, and z as its diagonal
	// elements.
	Matrix Diag( float x, float y, float z )
	{
		Matrix D(3,3);
		D(0,0) = x;
		D(1,1) = y;
		D(2,2) = z;
		return D;
	}

	// Returns the vector consisting of the diagonal elements of the
	// matrix M, which need not be square.
	Vector Diag( const Matrix &M )
	{
		int m = Min( M.Rows(), M.Cols() );
		Vector V(m);
		for( register int i = 0; i < m; i++ ) V(i) = M(i,i);
		return V;
	}

	// Returns the n x n identity matrix.
	Matrix Ident( int n )
	{
		Matrix I( n );
		for( register int i = 0; i < n; i++ ) I(i,i) = 1.0;
		return I;
	}

	// Determines whether the matrix M is "Null" -- i.e. has zero rows
	// or columns.
	int Null( const Matrix &M ) 
	{
		return M.Rows() == 0 || M.Cols() == 0;
	}

	int Square( const Matrix &M )
	{
		return M.Rows() == M.Cols();
	}

	// Convert a "vector-shaped" matrix to a vector.  That is, represent a
	// matrix with a single row or a single column as a vector.
	Vector ToVector( const Matrix &M ) 
	{
		if( M.Rows() == 1 )
		{
			Vector V( M.Cols() );
			for( int j = 0; j < M.Cols(); j++ ) V(j) = M(0,j);
			return V;
		}
		else if( M.Cols() == 1 )
		{
			Vector V( M.Rows() );
			for( int i = 0; i < M.Rows(); i++ ) V(i) = M(i,0);
			return V;
		}
		else 
		{
			// Report an error.     
			assert( M.Rows() == 1 || M.Cols() == 1 );
		}
		return Vector();
	}

	std::ostream &operator<<( std::ostream &out, const Matrix &M )
	{
		if( M.Rows() == 0 || M.Cols() == 0 )
		{
			out << "NULL" << std::endl;
		}
		else for( register int i = 0; i < M.Rows(); i++ )
		{
			out << form( "%3d: ", i );
			for( register int j = 0; j < M.Cols(); j++ )
				out << form( " %10.5g", M(i,j) );
			out << std::endl;
		}
		return out;
	}

	/*-------------------------------------------------------------------------*
	* R O T A T I O N                                                         *
	*                                                                         * 
	* Builds a 3x3 modeling matrix that performs a rotation about an          *
	* arbitrary axis.  The rotation is right-handed about this axis and       *
	* "angle" is taken to be in radians.  The only error that can occur is    *
	* when "axis" is the zero-vector.                                         *
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	Matrix Rotation( const Vector &Axis, float angle )
	{
		// Compute a unit quaternion (a,b,c,d) that performs the rotation.

		float t = TwoNormSqr( Axis );
		if( t == 0.0 ) return Matrix(3,3);
		t = sin( angle * 0.5 ) / sqrt( t );

		// Fill in the entries of the quaternion.

		float a = cos( angle * 0.5 );
		float b = t * Axis(0);
		float c = t * Axis(1);
		float d = t * Axis(2);

		// Compute all the double products of a, b, c, and d, except a * a.

		float bb = b * b;
		float cc = c * c;
		float dd = d * d;
		float ab = a * b;
		float ac = a * c;
		float ad = a * d;
		float bc = b * c;
		float bd = b * d;
		float cd = c * d;

		// Fill in the entries of the rotation matrix.

		Matrix R(3,3);

		R(0,0) = 1.0 - 2.0 * ( cc + dd );
		R(0,1) =       2.0 * ( bc + ad );
		R(0,2) =       2.0 * ( bd - ac );

		R(1,0) =       2.0 * ( bc - ad );
		R(1,1) = 1.0 - 2.0 * ( bb + dd );
		R(1,2) =       2.0 * ( cd + ab );

		R(2,0) =       2.0 * ( bd + ac );
		R(2,1) =       2.0 * ( cd - ab );
		R(2,2) = 1.0 - 2.0 * ( bb + cc );

		return R;
	}

	/*-------------------------------------------------------------------------*
	* R O T A T I O N                                                         *
	*                                                                         * 
	* Builds a 4x4 modeling matrix that performs a rotation about an          *
	* arbitrary axis through an arbitrary point.  The rotation is             *
	* right-handed about this axis and "angle" is taken to be in radians.     *
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	Matrix Rotation( const Vector &Axis, const Vector &Origin, float angle )
	{
		Matrix R = Rotation( Axis, angle );   // A simple 3x3 rotation.
		Matrix M = Ident(4);                  // A 4x4 including translation.

		// Compute the last row of the matrix (the translation) using the
		// 3x3 rotation matrix.  We need to compute the last row of the 4x4
		// matrix that performs Translate( -Origin ) * Rotate * Translate( Origin ).
		//
		//       | I   p | | R   0 | | I  -p |   | R   p - Rp |
		//       |       | |       | |       | = |            |
		//       | 0   1 | | 0   1 | | 0   1 |   | 0      1   |
		//
		// So, the desired column is  p - R p.

		Vector V( Origin - R * Origin );
		for( int i = 0; i < 3; i++ )
		{
			M(i,3) = V(i);
			for( int j = 0; j < 3; j++ )
				M(i,j) = R(i,j);
		}
		return M;
	}

	/*-------------------------------------------------------------------------*
	* X  R O T A T I O N                                                      *
	*                                                                         * 
	* Builds a 3x3 modeling matrix that performs a rotation about the X-axis. *
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	Matrix Xrotation( float angle )
	{
		Matrix M = Ident(3);
		float c = cos( angle );
		float s = sin( angle );
		M(1,1) = c;  M(1,2) = -s;
		M(2,1) = s;  M(2,2) =  c;
		return M;
	}

	/*-------------------------------------------------------------------------*
	* Y  R O T A T I O N                                                      *
	*                                                                         * 
	* Builds a 3x3 modeling matrix that performs a rotation about the Y-axis. *
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	Matrix Yrotation( float angle )
	{
		Matrix M = Ident(3);
		float c = cos( angle );
		float s = sin( angle );
		M(0,0) = c;  M(0,2) = -s;
		M(2,0) = s;  M(2,2) =  c;
		return M;
	}

	/*-------------------------------------------------------------------------*
	* Z  R O T A T I O N                                                      *
	*                                                                         * 
	* Builds a 3x3 modeling matrix that performs a rotation about the Z-axis. *
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	Matrix Zrotation( float angle )
	{
		Matrix M = Ident(3);
		float c = cos( angle );
		float s = sin( angle );
		M(0,0) = c;  M(0,1) = -s;
		M(1,0) = s;  M(1,1) =  c;
		return M;
	}

	/*-------------------------------------------------------------------------*
	* H O U S E H O L D E R                                                   *
	*                                                                         * 
	* Returns the Householder reflection matrix that reflects through the     *  
	* plane orthogonal to V.  The vector V is not assumed to be normalized.   *  
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	Matrix Householder( const Vector &V )
	{
		Matrix I = Ident( V.Size() );
		float  c = 2.0 / ( V * V );
		return I - Outer( c * V, V );
	}

	/*=========================================================================*
	*  R O T A T I O N                Author: Jim Arvo, 1991                  *
	*                                                                         *
	*  This routine maps three values (x1, x2, x3) in the range [0,1] into    *
	*  a 3x3 rotation matrix, M.  Uniformly distributed random variables      *
	*  x1, x2, and x3 create uniformly distributed random rotation matrices.  *
	*  To create small uniformly distributed "perturbations", supply          *
	*  samples in the following ranges                                        *
	*                                                                         *
	*      x1 in [ 0, d ]                                                     *
	*      x2 in [ 0, 1 ]                                                     *
	*      x3 in [ 0, d ]                                                     *
	*                                                                         *
	* where 0 < d < 1 controls the size of the perturbation.  Any of the      *
	* random variables may be stratified (or "jittered") for a slightly more  *
	* even distribution.                                                      *
	*                                                                         *
	*=========================================================================*/
	Matrix Rotation( float x1, float x2, float x3 )
	{
		Matrix M(3,3);
		float theta = x1 * TwoPi; // Rotation about the pole (Z). 
		float phi   = x2 * TwoPi; // For direction of pole deflection.
		float z     = x3 * 2.0;   // For magnitude of pole deflection.

		// Compute a vector V used for distributing points over the sphere
		// via the reflection I - V Transpose(V).  This formulation of V
		// will guarantee that if x1 and x2 are uniformly distributed,
		// the reflected points will be uniform on the sphere.  Note that V
		// has length sqrt(2) to eliminate the 2 in the Householder matrix.

		float r  = sqrt( z );
		float Vx = sin( phi ) * r;
		float Vy = cos( phi ) * r;
		float Vz = sqrt( 2.0 - z );    

		// Compute the row vector S = Transpose(V) * R, where R is a simple
		// rotation by theta about the z-axis.  No need to compute Sz since
		// it's just Vz.

		float st = sin( theta );
		float ct = cos( theta );
		float Sx = Vx * ct - Vy * st;
		float Sy = Vx * st + Vy * ct;

		// Construct the rotation matrix  ( V Transpose(V) - I ) R, which
		// is equivalent to V S - R.

		M(0,0) = Vx * Sx - ct;
		M(0,1) = Vx * Sy - st;
		M(0,2) = Vx * Vz;

		M(1,0) = Vy * Sx + st;
		M(1,1) = Vy * Sy - ct;
		M(1,2) = Vy * Vz;

		M(2,0) = Vz * Sx;
		M(2,1) = Vz * Sy;
		M(2,2) = 1.0 - z;   // This equals Vz * Vz - 1.0 

		return M;
	}

	/*-------------------------------------------------------------------------*
	* P A R T I A L   P I V O T                                               *
	*                                                                         * 
	* Look for the element with the largest magnitude on or below the         *
	* diagonal in column "col" of the matrix A.  Bring this element to the    *
	* diagonal by a row interchange.  Perform the same row interchange on b.  *
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	static int PartialPivot( int col, Matrix &A, Vector &b )
	{
		int n = A.Cols();
		float a_max = Abs( A( col, col ) );
		int   i_max = col;
		for( int i = col + 1; i < n; i++ )
		{
			float temp = Abs( A( i, col ) );
			if( temp > a_max )
			{
				a_max = temp;
				i_max = i;
			}
		}
		if( a_max == 0.0 ) return 0;
		if( i_max != col )
		{
			A.SwapRows( col, i_max );
			b.Swap    ( col, i_max );
		}
		return 1;
	}

	/*-------------------------------------------------------------------------*
	* G A U S S I A N   E L I M I N A T I O N                                 *
	*                                                                         * 
	* Solves the linear system A x = b using Gaussian elimination, with or    *
	* without partial pivoting.                                               *
	*                                                                         *  
	*-------------------------------------------------------------------------*/
	int GaussElimination( const Matrix &A, const Vector &b, Vector &x, pivot_type pivot )
	{
		assert( Square( A ) );
		assert( A.Rows() == b.Size() );
		Matrix B( A );
		Vector c( b );
		x.SetSize( A.Cols() );
		int m = B.Rows();
		register int i, j, k;

		// Perform Gaussian elimination on the copies, B and c.

		for( i = 0; i < m; i++ )
		{
			if( pivot == pivot_partial ) PartialPivot( i, B, c );

			for( j = i + 1; j < m; j++ )
			{
				double scale = -B(j,i) / B(i,i);
				for( k = i; k < m; k++ )
					B(j,k) += scale * B(i,k);
				B(j,i) = 0.0;
				c(j) += scale * c(i);
			}
		}

		// Now solve by back substitution.

		for( i = m - 1; i >= 0; i-- )
		{
			double a = 0.0;
			for( j = i + 1; j < m; j++ ) a += B(i,j) * x(j);
			x(i) = ( c(i) - a ) / B(i,i);
		}

		return 1;
	}

	/*-------------------------------------------------------------------------*
	*  L E A S T   S Q U A R E S                                              *
	*                                                                         *
	* Solves the normal equations associated with the system A x = b, which   *
	* are given by  Transp(A) A x = Transp(A) b.                              *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int LeastSquares( const Matrix &A, const Vector &b, Vector &x )
	{
		//
		// Set up and solve the normal equations Transp(A) A x = Transp(A) b.
		// Note that Transp(A) * b is computed here as b * A.
		//
		GaussElimination( ATranspA(A), b * A, x );
		return 1;
	}

	/*-------------------------------------------------------------------------*
	*  D E T E R M I N A N T                                                  *
	*                                                                         *
	* Computes the determinant of the n by n matrix M using Householder       *
	* transformations.                                                        *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	double Determinant( const Matrix &M )
	{
		static const float MachEps = MachineEpsilon();
		assert( Square(M) );

		double dot;
		int    k;
		Matrix A    = M;    // Make a copy that we can destroy.
		double det  = 1.0;  // Multiply diagonal elements as they are generated.
		int    sign = 1;	// Keep track of sign (each reflection has det -1).
		int    n    = M.Cols();

		for( int i = 0; i < n - 1; i++ ) 
		{
			// Compute the 2-norm of the first column of the (n-i)x(n-i) submatrix.

			dot = 0.0;
			for( k = i; k < n; k++ ) dot += Sqr( A(k,i) );

			double Xnorm = sqrt( dot );
			if( Xnorm == 0.0 ) return 0.0;

			// This norm is another diagonal element of the upper triangular
			// matrix, so we multiply it into the running product for det.

			det *= Xnorm;		

			// If X is already of the right form we must not perform the
			// processing because V will be zero.

			float x1   = Abs( A(i,i) );
			float diff = Abs( Xnorm - x1 );
			if( diff < MachEps * Max( Xnorm, x1 ) ) continue;  // This column is okay as is.

			// Each Householder transformation has a determinant of -1,
			// so we must keep track of how many we apply.

			sign *= -1;

			// Compute the V vector, which will define the Householder
			// transformation via  H = I - V transp(V).  Leave it in the
			// i'th column of A.  V = sqrt(2) * Normalized( X - ( Xnorm, 0, 0,... ) ).

			float scale = 1.0 / sqrt( Xnorm * Abs( A(i,i) - Xnorm ) );  // sqrt(2) / || p ||
			A(i,i) = ( A(i,i) - Xnorm ) * scale;        
			for( k = i + 1; k < n; k++ ) A(k,i) *= scale;

			// Now apply the transformation I - V Transp(V) to all the remaining columns, 
			// except for the first row.

			for( int j = i + 1; j < n; j++ ) 
			{
				// Compute Y dot V.

				dot = 0.0;
				for( k = i; k < n; k++ ) dot += A(k,i) * A(k,j);

				// Subtract V ( V dot A(*,j) ) from A(*,j), ignoring the first row.

				for( k = i + 1; k < n; k++ ) A(k,j) -= A(k,i) * dot;

			} // for j

		} // for i

		// Now multiply in the very last element of the matrix and
		// the accumulated sign.

		return det * A(n-1,n-1) * sign;
	}	

	/*-------------------------------------------------------------------------*
	*  C O F A C T O R                                                        *
	*                                                                         *
	* Computes the (i,j) cofactor of the n by n matrix M.                     *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	double Matrix::Cofactor( int omit_i, int omit_j ) const
	{
		assert( Square( *this ) );
		assert( omit_i >= 0 && omit_j >= 0 );
		assert( omit_i < Rows() );
		assert( omit_j < Cols() );

		// Create a new matrix that is smaller by one in both dimensions and
		// copy the old matrix into it, omitting the specified row and column.

		Matrix A( Rows() - 1, Cols() - 1 );
		for( int i = 0; i < Rows() - 1; i++ )
		{
			int ii = ( i < omit_i ) ? i : i + 1;
			for( int j = 0; j < Cols() - 1; j++ )
			{
				int jj = ( j < omit_j ) ? j : j + 1;
				A( i, j ) = (*this)(ii,jj);
			}
		}

		// Return the determinant of the smaller matrix.

		return Determinant( A );
	}

	/*-------------------------------------------------------------------------*
	*  A D J O I N T                                                          *
	*                                                                         *
	* Computes the adjoint of a matrix.                                       *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Matrix Adjoint( const Matrix &M )
	{
		double det;
		return Adjoint( M, det );  // Discard the determinant.
	}

	Matrix Adjoint( const Matrix &M, double &det )
	{
		int n = M.Rows();
		det   = 0.0;
		Matrix A( n, n );
		assert( Square(M) );
		if( n == 3 )
		{
			A(0,0) = M(1,1) * M(2,2) - M(1,2) * M(2,1);
			A(0,1) = M(1,2) * M(2,0) - M(1,0) * M(2,2);
			A(0,2) = M(1,0) * M(2,1) - M(1,1) * M(2,0);

			A(1,0) = M(0,2) * M(2,1) - M(0,1) * M(2,2);
			A(1,1) = M(0,0) * M(2,2) - M(0,2) * M(2,0);
			A(1,2) = M(0,1) * M(2,0) - M(0,0) * M(2,1);

			A(2,0) = M(0,1) * M(1,2) - M(0,2) * M(1,1);
			A(2,1) = M(0,2) * M(1,0) - M(0,0) * M(1,2);
			A(2,2) = M(0,0) * M(1,1) - M(0,1) * M(1,0);

			det = A(0,0) * M(0,0) + A(1,0) * M(1,0) + A(2,0) * M(2,0);
		}
		else
		{
			for( register int i = 0; i < n; i++ )
			{
				for( register int j = 0; j < n; j++ )
				{
					if( Odd( i + j ) )
						A(i,j) = -M.Cofactor(i,j);
					else A(i,j) =  M.Cofactor(i,j);
				}
				det += M(i,0) * A(i,0);
			}
		}
		return A;
	}

	/*-------------------------------------------------------------------------*
	*  I N V E R S E                                                          *
	*                                                                         *
	* Computes the inverse of a square matrix.                                *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Matrix Inverse( const Matrix &M )
	{
		assert( Square( M ) );
		int n = M.Cols();
		Matrix Inv( n, n );
		Vector b( n ), x( n );

		for( int i = 0; i < n; i++ )
		{
			if( i > 0 ) b( i - 1 ) = 0.0;
			b(i) = 1.0;
			GaussElimination( M, b, x );
			Inv.SetCol( i, x );
		}
		return Inv;
	}

	/*-------------------------------------------------------------------------*
	*  T R A C E                                                              *
	*                                                                         *
	* Computes the trace of a square matrix.                                  *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	extern double Trace( const Matrix &M )
	{
		assert( Square(M) );
		double trace = M(0,0);
		for( int i = 1; i < M.Cols(); i++ ) trace += M(i,i);
		return trace;
	}
};



/*

C
C  Subroutine GAUSS solves the the system Ax = b by using Gaussian elimination.
C

SUBROUTINE GAUSS( A, B, X, LDA, N, IFLAG )
REAL A( LDA, N ), B( N ), X( N )

DO 300 I = 1 , N - 1
I2 = I
CALL PIVOT( A, B, LDA, N, I2, IFLAG )
IF ( IFLAG .LT. 0 ) RETURN
DO 200 J = I + 1 , N
TEMP = A( J , I ) / A( I , I )
A( J , I ) = 0.0
B( J ) = B( J ) - TEMP * B( I )
DO 100 K = I + 1 , N
A( J , K ) = A( J , K ) - TEMP * A( I , K )
100           CONTINUE
200       CONTINUE
300   CONTINUE

X( N ) = B( N ) / A( N , N )
DO 500 I = N - 1 , 1 , -1
TEMP = 0.0
DO 400 J = I + 1 , N
TEMP = TEMP + A( I , J ) * X( J )
400       CONTINUE
X( I ) = ( B( I ) - TEMP ) / A( I , I )
500   CONTINUE

RETURN
END



SUBROUTINE PIVOT( A, B, LDA, N, J, IFLAG )
REAL A( LDA, N ), B( N ), AMAX, TEMP
DATA TOL / 1.0E-6 /

IFLAG = -1
IF ( J .GT. N ) RETURN
IF ( J .EQ. N .AND. ABS( A(N,N) ) .LT. TOL ) RETURN
IF ( J .EQ. N ) GO TO 40

AMAX  = ABS( A( J , J ) )
INDEX = J
10   DO 20 I = J + 1 , N
IF ( ABS( A( I , J ) ) .LE. AMAX ) GO TO 20
AMAX = ABS( A( I , J ) )
INDEX = I
20   CONTINUE

IF ( AMAX .LT. TOL ) RETURN

TEMP = B( J )
B( J ) = B( INDEX )
B( INDEX ) = TEMP

DO 30 K = 1 , N
TEMP = A( J , K )
A( J , K ) = A( INDEX , K )
A( INDEX , K ) = TEMP
30   CONTINUE

40   IFLAG = 1
RETURN
END


*/





