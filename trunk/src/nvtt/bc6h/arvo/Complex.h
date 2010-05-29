/***************************************************************************
* Complex.h                                                                *
*                                                                          *
* Complex numbers, complex arithmetic, and functions of a complex          *
* variable.                                                                *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      arvo    03/02/2000  Initial coding.                                 *
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
#ifndef __COMPLEX_INCLUDED__
#define __COMPLEX_INCLUDED__

#include <math.h>
#include <iostream>

namespace ArvoMath {

	class Complex {
	public:
		Complex()                   { x = 0; y = 0; }
		Complex( float a          ) { x = a; y = 0; }
		Complex( float a, float b ) { x = a; y = b; }
		Complex( const Complex &z ) { *this = z; }
		float &Real() { return x; }
		float &Imag() { return y; }
		float Real() const { return x; }
		float Imag() const { return y; }
		inline Complex &operator=( const Complex &z );
		static const Complex i;
	private:
		float x;
		float y;
	};

	inline Complex &Complex::operator=( const Complex &z ) 
	{ 
		x = z.Real(); 
		y = z.Imag(); 
		return *this;
	}

	inline float Real( const Complex &z )
	{
		return z.Real();
	}

	inline float Imag( const Complex &z )
	{
		return z.Imag();
	}

	inline Complex conj( const Complex &z )
	{
		return Complex( z.Real(), -z.Imag() );
	}

	inline double modsqr( const Complex &z )
	{
		return z.Real() * z.Real() + z.Imag() * z.Imag();
	}

	inline double modulus( const Complex &z )
	{
		return sqrt( z.Real() * z.Real() + z.Imag() * z.Imag() );
	}

	inline double arg( const Complex &z )
	{
		float t = acos( z.Real() / modulus(z) );
		if( z.Imag() < 0.0 ) t = 2.0 * 3.1415926 - t;
		return t;
	}

	inline Complex operator*( const Complex &z, float a )
	{
		return Complex( a * z.Real(), a * z.Imag() );
	}

	inline Complex operator*( float a, const Complex &z )
	{
		return Complex( a * z.Real(), a * z.Imag() );
	}

	inline Complex operator*( const Complex &z, const Complex &w )
	{
		return Complex( 
			z.Real() * w.Real() - z.Imag() * w.Imag(),
			z.Real() * w.Imag() + z.Imag() * w.Real()
			);
	}

	inline Complex operator+( const Complex &z, const Complex &w )
	{
		return Complex( z.Real() + w.Real(), z.Imag() + w.Imag() );
	}

	inline Complex operator-( const Complex &z, const Complex &w )
	{
		return Complex( z.Real() - w.Real(), z.Imag() - w.Imag() );
	}

	inline Complex operator-( const Complex &z )
	{
		return Complex( -z.Real(), -z.Imag() );
	}

	inline Complex operator/( const Complex &z, float w )
	{
		return Complex( z.Real() / w, z.Imag() / w );
	}

	inline Complex operator/( const Complex &z, const Complex &w )
	{
		return ( z * conj(w) ) / modsqr(w);
	}

	inline Complex operator/( float a, const Complex &w )
	{
		return conj(w) * ( a / modsqr(w) );
	}

	inline Complex &operator+=( Complex &z, const Complex &w )
	{
		z.Real() += w.Real();
		z.Imag() += w.Imag();
		return z;
	}

	inline Complex &operator*=( Complex &z, const Complex &w )
	{
		return z = ( z * w );
	}

	inline Complex &operator-=( Complex &z, const Complex &w )
	{
		z.Real() -= w.Real();
		z.Imag() -= w.Imag();
		return z;
	}

	inline Complex exp( const Complex &z )
	{
		float r = ::exp( z.Real() );
		return Complex( r * cos( z.Imag() ), r * sin( z.Imag() ) );
	}

	inline Complex pow( const Complex &z, int n )
	{
		float r = ::pow( modulus( z ), (double)n );
		float t = arg( z );
		return Complex( r * cos( n * t ), r * sin( n * t ) );
	}

	inline Complex polar( float r, float theta )
	{
		return Complex( r * cos( theta ), r * sin( theta ) );
	}


	extern Complex cos ( const Complex &z );
	extern Complex sin ( const Complex &z );
	extern Complex cosh( const Complex &z );
	extern Complex sinh( const Complex &z );
	extern Complex log ( const Complex &z );

	extern std::ostream &operator<<( 
		std::ostream &out, 
		const Complex & 
		);
};
#endif

