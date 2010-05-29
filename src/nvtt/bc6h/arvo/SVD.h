/***************************************************************************
* SVD.h                                                                    *
*                                                                          *
* Singular Value Decomposition.                                            *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date          Description                                   *
*                                                                          *
*      arvo    08/22/2000    Split off from Matrix.h                       *
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
#ifndef __SVD_INCLUDED__
#define __SVD_INCLUDED__

#include "Vector.h"
#include "Matrix.h"

namespace ArvoMath {

	class SVD {
	public:
		SVD( );
		SVD( const SVD    & );  // Copies the decomposition.
		SVD( const Matrix & );  // Performs the decomposition.
		~SVD() {};
		const Matrix &Q( double epsilon = 0.0 ) const;
		const Matrix &D( double epsilon = 0.0 ) const;
		const Matrix &R( double epsilon = 0.0 ) const;
		const Matrix &PseudoInverse( double epsilon = 0.0 );
		int   Rank( double epsilon = 0.0 ) const;
		void  operator=( const Matrix & );  // Performs the decomposition.
	private:
		int Decompose( Matrix &Q, Matrix &D, Matrix &R );
		Matrix Q_;
		Matrix D_;
		Matrix R_;
		Matrix P_; // Pseudo inverse.
		int    error;
	};
};
#endif
