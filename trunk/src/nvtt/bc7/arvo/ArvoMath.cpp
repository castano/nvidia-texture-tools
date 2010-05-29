/***************************************************************************
* Math.C                                                                   *
*                                                                          *
* Some basic math functions.                                               *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      arvo    06/21/93    Initial coding.                                 *
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
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include "ArvoMath.h"
#include "form.h"

namespace ArvoMath {
	static const float  Epsilon = 1.0E-5;
	static const double LogTwo  = log( 2.0 );

#define BinCoeffMax 500

	double RelErr( double x, double y )
	{
		double z = x - y;
		if( x < 0.0 ) x = -x;
		if( y < 0.0 ) y = -y;
		return z / ( x > y ? x : y );
	}

	/***************************************************************************
	*  A R C   Q U A D                                                         *
	*                                                                          *
	* Returns the theta / ( 2*PI ) where the input variables x and y are       *
	* such that  x == COS( theta ) and  y == SIN( theta ).                     *
	*                                                                          *
	***************************************************************************/
	float ArcQuad( float x, float y )
	{
		if( Abs( x ) > Epsilon )
		{
			float temp = OverTwoPi * atan( Abs( y ) / Abs( x ) );
			if( x < 0.0 ) temp = 0.5 - temp;
			if( y < 0.0 ) temp = 1.0 - temp;
			return( temp );
		}
		else if( y >  Epsilon ) return( 0.25 );
		else if( y < -Epsilon ) return( 0.75 );
		else return( 0.0 ); 
	}

	/***************************************************************************
	*  A R C   T A N                                                           *
	*                                                                          *
	* Returns the angle theta such that x = COS( theta ) & y = SIN( theta ).   *
	*                                                                          *
	***************************************************************************/
	float ArcTan( float x, float y )
	{
		if( Abs( x ) > Epsilon )
		{
			float temp = atan( Abs( y ) / Abs( x ) );
			if( x < 0.0 ) temp = Pi    - temp;
			if( y < 0.0 ) temp = TwoPi - temp;
			return( temp );
		}
		else if( y >  Epsilon ) return(     PiOverTwo );
		else if( y < -Epsilon ) return( 3 * PiOverTwo );
		else return( 0.0 ); 
	}

	/***************************************************************************
	*  M A C H I N E   E P S I L O N                                           *
	*                                                                          *
	* Returns the machine epsilon.                                             *
	*                                                                          *
	***************************************************************************/
	float MachineEpsilon()
	{
		float x = 1.0;
		float y;
		float z = 1.0 + x;
		while( z > 1.0 )
		{
			y = x;
			x /= 2.0;
			z = (float)( 1.0 + (float)x );  // Avoid double precision!
		}
		return (float)y;
	}

	/***************************************************************************
	*  L O G   G A M M A                                                       *
	*                                                                          *
	*  Computes the natural log of the gamma function using the Lanczos        *
	*  approximation formula.  Gamma is defined by                             *
	*                                                                          *
	*                                 ( z - 1 )   -t                           *
	*         gamma( z ) = Integral[ t           e    dt ]                     *
	*                                                                          *
	*                                                                          *
	*  where the integral ranges from 0 to infinity.  The gamma function       *
	*  satisfies                                                               *
	*                    gamma( n + 1 ) = n!                                   *
	*                                                                          *
	*  This algorithm has been adapted from "Numerical Recipes", p. 157.       *
	*                                                                          *
	***************************************************************************/
	double LogGamma( double x )
	{
		static const double 
			coeff0 =  7.61800917300E+1,
			coeff1 = -8.65053203300E+1,
			coeff2 =  2.40140982200E+1,
			coeff3 = -1.23173951600E+0,
			coeff4 =  1.20858003000E-3,
			coeff5 = -5.36382000000E-6,
			stp    =  2.50662827465E+0,
			half   =  5.00000000000E-1,
			fourpf =  4.50000000000E+0,
			one    =  1.00000000000E+0,
			two    =  2.00000000000E+0, 
			three  =  3.00000000000E+0,
			four   =  4.00000000000E+0, 
			five   =  5.00000000000E+0;
		double r = coeff0 / ( x        ) + coeff1 / ( x + one   ) +
			coeff2 / ( x + two  ) + coeff3 / ( x + three ) +
			coeff4 / ( x + four ) + coeff5 / ( x + five  ) ;
		double s = x + fourpf;
		double t = ( x - half ) * log( s ) - s;
		return t + log( stp * ( r + one ) );
	}

	/***************************************************************************
	*  L O G   F A C T                                                         *
	*                                                                          *
	*  Returns the natural logarithm of n factorial.  For efficiency, some     *
	*  of the values are cached, so they need be computed only once.           *
	*                                                                          *
	***************************************************************************/
	double LogFact( int n )
	{
		static const int Cache_Size = 100;
		static double c[ Cache_Size ] = { 0.0 }; // Cache some of the values.
		if( n <= 1 ) return 0.0;
		if( n < Cache_Size )
		{
			if( c[n] == 0.0 ) c[n] = LogGamma((double)(n+1));
			return c[n];
		}
		return LogGamma((double)(n+1)); // gamma(n+1) == n!
	}

	/***************************************************************************
	*  M U L T I N O M I A L    C O E F F                                      *
	*                                                                          *
	*  Returns the multinomial coefficient ( n; X1 X2 ... Xk ) which is        *
	*  defined to be n! / ( X1! X2! ... Xk! ).  This is done by computing      *
	*  exp( log(n!) - log(X1!) - log(X2!) - ... - log(Xk!) ).  The value of    *
	*  n is obtained by summing the Xi's.                                      *
	*                                                                          *
	***************************************************************************/
	double MultinomialCoeff( int k, int X[] )
	{
		int i;
		// Find n by summing the coefficients.

		int  n = X[0];
		for( i = 1; i < k; i++ ) n += X[i];

		// Compute log(n!) then subtract log(X!) for each X.

		double LogCoeff = LogFact( n );
		for( i = 0; i < k; i++ ) LogCoeff -= LogFact( X[i] );

		// Round the exponential of the result to the nearest integer.

		return floor( exp( LogCoeff ) + 0.5 );
	}


	double MultinomialCoeff( int i, int j, int k )
	{
		int    n = i + j + k;
		double x = LogFact( n ) - LogFact( i ) - LogFact( j ) - LogFact( k );
		return floor( exp( x ) + 0.5 );
	}

	/***************************************************************************
	*  B I N O M I A L    C O E F F S                                          *
	*                                                                          *
	*  Generate all n+1 binomial coefficents for a given n.  This is done by   *
	*  computing the n'th row of Pascal's triange, starting from the top.      *
	*  No additional storage is required.                                      *
	*                                                                          *
	***************************************************************************/
	void BinomialCoeffs( int n, long *coeff )
	{
		coeff[0] = 1;
		for( int i = 1; i <= n; i++ )
		{
			long a = coeff[0];
			long b = coeff[1];
			for( int j = 1; j < i; j++ )  // Make next row of Pascal's triangle.
			{
				coeff[j] = a + b; // Overwrite the old row.
				a = b;
				b = coeff[j+1];
			}
			coeff[i] = 1;  // The last entry in any row is always 1.
		}
	}

	void BinomialCoeffs( int n, double *coeff )
	{
		coeff[0] = 1.0;
		for( int i = 1; i <= n; i++ )
		{
			double a = coeff[0];
			double b = coeff[1];
			for( int j = 1; j < i; j++ )  // Make next row of Pascal's triangle.
			{
				coeff[j] = a + b; // Overwrite the old row.
				a = b;
				b = coeff[j+1];
			}
			coeff[i] = 1.0;  // The last entry in any row is always 1.
		}
	}

	const double *BinomialCoeffs( int n )
	{
		static double *coeff[ BinCoeffMax + 1 ] = { 0 };
		if( n > BinCoeffMax || n < 0 ) 
		{
			std::cerr << form( "%d is outside of (0,%d) in BinomialCoeffs", n, BinCoeffMax );
			return NULL;
		}
		if( coeff[n] == NULL ) // Fill in this entry.
		{
			double *c = new double[ n + 1 ];
			if( c == NULL )
			{
				std::cerr << form( "Could not allocate for BinomialCoeffs(%d)", n );
				return NULL;
			}
			BinomialCoeffs( n, c );
			coeff[n] = c;
		}
		return coeff[n];
	}

	/***************************************************************************
	*  B I N O M I A L    C O E F F                                            *
	*                                                                          *
	*  Compute a given binomial coefficient.  Several rows of Pascal's         *
	*  triangle are stored for efficiently computing the small coefficients.   *
	*  Higher-order terms are computed using LogFact.                          *
	*                                                                          *
	***************************************************************************/
	double BinomialCoeff( int n, int k )
	{
		double b;
		int    p = n - k;
		if( k <= 1 || p <= 1 )  // Check for errors and special cases.
		{
			if( k == 0 || p == 0 ) return 1;
			if( k == 1 || p == 1 ) return n;
			std::cerr << form( "BinomialCoeff(%d,%d) is undefined", n, k );
			return 0;
		}
		static const int  // Store part of Pascal's triange for small coeffs.
			n0[] = { 1 },
			n1[] = { 1, 1 },
			n2[] = { 1, 2, 1 },
			n3[] = { 1, 3, 3, 1 },
			n4[] = { 1, 4, 6, 4, 1 },
			n5[] = { 1, 5, 10, 10, 5, 1 },
			n6[] = { 1, 6, 15, 20, 15, 6, 1 },
			n7[] = { 1, 7, 21, 35, 35, 21, 7, 1 },
			n8[] = { 1, 8, 28, 56, 70, 56, 28, 8, 1 },
			n9[] = { 1, 9, 36, 84, 126, 126, 84, 36, 9, 1 };
		switch( n )
		{
		case 0 : b = n0[k]; break;
		case 1 : b = n1[k]; break;
		case 2 : b = n2[k]; break;
		case 3 : b = n3[k]; break;
		case 4 : b = n4[k]; break;
		case 5 : b = n5[k]; break;
		case 6 : b = n6[k]; break;
		case 7 : b = n7[k]; break;
		case 8 : b = n8[k]; break;
		case 9 : b = n9[k]; break;
		default:
			{
				double x = LogFact( n ) - LogFact( p ) - LogFact( k );
				b = floor( exp( x ) + 0.5 );
			}
		}
		return b;
	}


	/***************************************************************************
	*  L O G   D O U B L E   F A C T   (Log of double factorial)               *
	*                                                                          *
	*  Return log( n!! ) where the double factorial is defined by              *
	*                                                                          *
	*      (2 n + 1)!! = 1 * 3 * 5 * ... * (2n + 1)    (Odd integers)          *
	*                                                                          *
	*      (2 n)!!     = 2 * 4 * 6 * ... * 2n          (Even integers)         *
	*                                                                          *
	*  and is related to the single factorial via                              *
	*                                                                          *
	*      (2 n + 1)!! = (2 n + 1)! / ( 2^n n! )       (Odd integers)          *
	*                                                                          *
	*      (2 n)!!     = 2^n n!                        (Even integers)         *
	*                                                                          *
	***************************************************************************/
	double LogDoubleFact( int n )   // log( n!! )
	{
		int    k = n / 2;
		double f = LogFact( k ) + k * LogTwo;
		if( Odd(n) ) f = LogFact( n ) - f;
		return f;
	}
};
