/***************************************************************************
* Vector.h                                                                 *
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
#ifndef __VECTOR_INCLUDED__
#define __VECTOR_INCLUDED__

#include <istream>
#include "Vec2.h"
#include "Vec3.h"

namespace ArvoMath {
	class Vector {
	public:
		Vector( int size = 0   );
		Vector( const Vector & );
		Vector( float, float );
		Vector( float, float, float );
		Vector( const float *x, int n );
		Vector &operator=( const Vector & );
		void    operator=( float );
		void    SetSize( int );
		Vector &Swap( int i, int j );
		Vector  GetBlock( int i, int j ) const;
		void    SetBlock( int i, int j, const Vector & );
		static  const Vector Null;

	public: // Inlined functions.
		inline float  operator()( int i ) const { return elem[i]; }
		inline float& operator()( int i )       { return elem[i]; }
		inline float* Array() const { return elem; }
		inline int    Size () const { return size; }
		inline ~Vector() { delete[] elem; }

	private:
		void   Create( int n = 0 ) { size = n; elem = new float[n]; }
		int    size;
		float* elem;
	};

	extern Vector  operator +  ( const Vector &, const Vector & );
	extern Vector  operator -  ( const Vector &, const Vector & ); // Binary minus.
	extern Vector  operator -  ( const Vector &                 ); // Unary minus.
	extern Vector  operator *  ( const Vector &,        float   );
	extern Vector  operator *  (       float   , const Vector & );
	extern Vector  operator /  ( const Vector &,        float   );
	extern Vector  operator /  ( const Vector &, const Vector & );
	extern Vector  operator ^  ( const Vector &, const Vector & );
	extern Vector& operator += (       Vector &, const Vector & );
	extern Vector& operator *= (       Vector &,        float   );
	extern Vector& operator /= (       Vector &,        float   );
	extern Vector  Min         ( const Vector &, const Vector & );
	extern Vector  Max         ( const Vector &, const Vector & );
	extern double  operator *  ( const Vector &, const Vector & );  // Inner product.
	extern double  dist        ( const Vector &, const Vector & );
	extern Vector  OrthogonalTo( const Vector & );  // Returns some orthogonal vector.
	extern Vector  Unit        ( const Vector & );
	extern double  Normalize   (       Vector & );
	extern double  OneNorm     ( const Vector & );
	extern double  TwoNorm     ( const Vector & );
	extern double  TwoNormSqr  ( const Vector & );
	extern double  SupNorm     ( const Vector & );
	extern int     Null        ( const Vector & );
	extern Vec2    ToVec2      ( const Vector & );
	extern Vec3    ToVec3      ( const Vector & );
	extern Vector  ToVector    ( const Vec2   & );
	extern Vector  ToVector    ( const Vec3   & );

	std::ostream &operator<<( 
		std::ostream &out, 
		const Vector &
		);
};
#endif






