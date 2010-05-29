/***************************************************************************
* Matrix.h                                                                 *
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
#ifndef __MATRIX_INCLUDED__
#define __MATRIX_INCLUDED__

#include <iostream>
#include "Vector.h"

namespace ArvoMath {

	class Matrix {
	public:
		Matrix( const Matrix & );
		Matrix( int num_rows = 0, int num_cols = 0, float value = 0.0 );
		~Matrix();
		Matrix &operator=( const Matrix &M );
		Matrix &operator=( float s );
		Vector  GetCol( int col ) const;
		Vector  GetRow( int row ) const;
		void    SetCol( int col, const Vector & );
		void    SetRow( int row, const Vector & );
		Matrix  GetBlock( int imin, int imax, int jmin, int jmax ) const;
		void    SetBlock( int imin, int imax, int jmin, int jmax, const Matrix & );
		void    SetBlock( int imin, int imax, int jmin, int jmax, const Vector & );
		Matrix &SwapRows( int i1, int i2 );
		Matrix &SwapCols( int j1, int j2 );
		void    SetSize( int rows, int cols = 0 );
		double  Cofactor( int i, int j ) const;
		static  const Matrix Null;

	public: // Inlined functions.
		inline float  operator()( int i, int j ) const { return elem[ i * cols + j ]; }
		inline float &operator()( int i, int j )       { return elem[ i * cols + j ]; }
		inline int    Rows  () const { return rows; }
		inline int    Cols  () const { return cols; }
		inline float *Array () const { return elem; }

	private:
		int    rows; // Number of rows in the matrix.
		int    cols; // Number of columns in the matrix.
		float *elem; // Pointer to the actual data.
	};


	extern Vector  operator *  ( const Matrix &, const Vector & );
	extern Vector  operator *  ( const Vector &, const Matrix & );
	extern Vector& operator *= (       Vector &, const Matrix & );
	extern Matrix  Outer       ( const Vector &, const Vector & );  // Outer product.
	extern Matrix  operator +  ( const Matrix &, const Matrix & );
	extern Matrix  operator -  ( const Matrix &                 );
	extern Matrix  operator -  ( const Matrix &, const Matrix & );
	extern Matrix  operator *  ( const Matrix &, const Matrix & );
	extern Matrix  operator *  ( const Matrix &,       float    );
	extern Matrix  operator *  (       float  ,  const Matrix & );
	extern Matrix  operator /  ( const Matrix &,       float    );
	extern Matrix& operator += (       Matrix &, const Matrix & );
	extern Matrix& operator *= (       Matrix &,       float    );
	extern Matrix& operator *= (       Matrix &, const Matrix & );
	extern Matrix& operator /= (       Matrix &,       float    );
	extern Matrix  Ident       (       int    n );
	extern Matrix  Householder ( const Vector & );
	extern Matrix  Rotation    ( const Vector &Axis, float angle );
	extern Matrix  Rotation    ( const Vector &Axis, const Vector &Origin, float angle );
	extern Matrix  Rotation    (       float, float, float ); // For random 3D rotations.
	extern Matrix  Xrotation   (       float    );
	extern Matrix  Yrotation   (       float    );
	extern Matrix  Zrotation   (       float    );
	extern Matrix  Diag        ( const Vector & );
	extern Vector  Diag        ( const Matrix & );
	extern Matrix  Diag        ( float, float, float );
	extern Matrix  Adjoint     ( const Matrix & );
	extern Matrix  Adjoint     ( const Matrix &, double &det );
	extern Matrix  AATransp    ( const Matrix & );
	extern Matrix  ATranspA    ( const Matrix & );
	extern double  OneNorm     ( const Matrix & );
	extern double  SupNorm     ( const Matrix & );
	extern double  Determinant ( const Matrix & );
	extern double  Trace       ( const Matrix & );
	extern Matrix  Transp      ( const Matrix & );
	extern Matrix  Inverse     ( const Matrix & );
	extern int     Null        ( const Matrix & );
	extern int     Square      ( const Matrix & );
	extern Vector  ToVector    ( const Matrix & ); // Only for vector-shaped matrices.

	enum pivot_type {
		pivot_off,
		pivot_partial,
		pivot_total
	};

	extern int GaussElimination( 
		const Matrix &A, 
		const Vector &b, // This is the right-hand side.
		Vector       &x, // This is the matrix we are solving for.
		pivot_type = pivot_off
		);

	extern int LeastSquares( 
		const Matrix &A, 
		const Vector &b, 
		Vector       &x
		);

	extern int WeightedLeastSquares( 
		const Matrix &A, 
		const Vector &b, 
		const Vector &w, 
		Vector       &x 
		);

	std::ostream &operator<<( 
		std::ostream &out, 
		const Matrix &
		);
};

#endif
