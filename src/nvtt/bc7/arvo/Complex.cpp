/***************************************************************************
* Complex.C                                                                *
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
#include "Complex.h"
#include "form.h"

namespace ArvoMath {
	const Complex Complex::i( 0.0, 1.0 );

	std::ostream &operator<<( std::ostream &out, const Complex &z )
	{
		out << form( "(%f,%f) ", z.Real(), z.Imag() );
		return out;
	}

	Complex cos( const Complex &z )
	{
		return Complex( 
			::cos( z.Real() ) * ::cosh( z.Imag() ), 
			-::sin( z.Real() ) * ::sinh( z.Imag() )
			);
	}

	Complex sin( const Complex &z )
	{
		return Complex( 
			::sin( z.Real() ) * ::cosh( z.Imag() ), 
			::cos( z.Real() ) * ::sinh( z.Imag() )
			);
	}

	Complex cosh( const Complex &z )
	{
		return Complex( 
			::cosh( z.Real() ) * ::cos( z.Imag() ), 
			::sinh( z.Real() ) * ::sin( z.Imag() )
			);
	}

	Complex sinh( const Complex &z )
	{
		return Complex( 
			::sinh( z.Real() ) * ::cos( z.Imag() ), 
			::cosh( z.Real() ) * ::sin( z.Imag() )
			);
	}

	Complex log( const Complex &z )
	{
		float r = ::sqrt( z.Real() * z.Real() + z.Imag() * z.Imag() );
		float t = ::acos( z.Real() / r );
		if( z.Imag() < 0.0 ) t = 2.0 * 3.1415926 - t;
		return Complex( ::log(r), t );
	}
};
