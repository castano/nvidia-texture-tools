/*****************************************************************************
** 
**   MODULE NAME  SI_units.h       International System of Units (SI)
**
**   DESCRIPTION
**       The purpose of this header file is to provide a simple and efficient
**       mechanism for associating physically meaningful units with floating
**       point numbers.  No extra space is required, and no runtime overhead
**       is introduced; all type-checking occurs at compile time.
**
**
**   HISTORY
**      Name	Date	    Description
**
**      arvo    02/09/92    Replaced conversion macros with inline functions.
**      arvo    10/16/91    Initial implementation.
**
**
**   (c) Copyright 1991, 1992
**       Program of Computer Graphics, Cornell University, Ithaca, NY
**       ALL RIGHTS RESERVED
**
*****************************************************************************/

#ifndef SI_UNITS_H
#define SI_UNITS_H

#include <iostream.h>

namespace ArvoMath {

	const float
		SI_deci  = 1.0E-1,
		SI_centi = 1.0E-2,
		SI_milli = 1.0E-3,
		SI_micro = 1.0E-6,
		SI_nano  = 1.0E-9,
		SI_kilo  = 1.0E+3,
		SI_mega  = 1.0E+6,
		SI_giga  = 1.0E+9,
		SI_tera  = 1.0E+12;

	/*******************************************************************************
	*                                                                              *
	*   I N T E R N A T I O N A L    S Y S T E M    O F    U N I T S               *
	*                                                                              *
	********************************************************************************
	*                                                                              *
	* DIMENSION           CLASS           INITIALIZER     SYMBOL   BASE UNITS      *
	*                                                                              *
	* length              SI_length        meter            m        m             *
	* time                SI_time          second           s        s             *
	* mass                SI_mass          kilogram         kg       kg            *
	* angle               SI_angle         radian           rad      rad           *
	* solid angle         SI_solid_angle   steradian        sr       sr            *
	* temperature         SI_temperature   kelvin           K        K             *
	* luminous intensity  SI_lum_inten     candela          cd       cd            *
	* area                SI_area          meter2           m2       m2            *
	* volume              SI_volume        meter3           m3       m3            *
	* frequency           SI_frequency     hertz            Hz       1/s           *
	* force               SI_force         newton           N        m kg/s2       *
	* energy              SI_energy        joule            J        m2 kg/s2      *
	* power               SI_power         watt             W        m2 kg/s3      *
	* radiance            SI_radiance      watts_per_m2sr   W/m2sr   kg/(s3 sr)    *
	* irradiance          SI_irradiance    watts_per_m2     W/m2     kg/s3         *
	* radiant intensity   SI_rad_inten     watts_per_sr     W/sr     m2 kg/(s3 sr) *
	* luminance           SI_luminance     candela_per_m2   cd/m2    cd/m2         *
	* illuminance         SI_illuminance   lux              lx       cd sr/m2      *
	* luminous flux       SI_lum_flux      lumen            lm       cd sr         *
	* luminous energy     SI_lum_energy    talbot           tb       cd sr s       *
	*                                                                              *
	*******************************************************************************/

	class SI_dimensionless {
	public:
		float Value() const { return value; }
		ostream& Put( ostream &s, char *a ) { return s << value << " " << a; }
	protected:
		SI_dimensionless() { value = 0; }
		SI_dimensionless( float x ){ value = x; }
		float value;
	};

	/*******************************************************************************
	* The following macro is used for creating new quantity classes and their      *
	* corresponding initializing functions and abbreviations.  This macro is       *
	* not intended to be used outside of this file -- it is a compact means of     *
	* defining generic operations for each quantity (e.g. scaling & comparing).    *
	*******************************************************************************/

#define SI_Make( C, Initializer, Symbol )                                  \
	struct C : SI_dimensionless {                                          \
	C                 (         ) : SI_dimensionless(   ) {};          \
	C                 ( float x ) : SI_dimensionless( x ) {};          \
	C     operator *  ( float x ) { return C( value *  x         ); }  \
	C     operator /  ( float x ) { return C( value /  x         ); }  \
	C     operator /= ( float x ) { return C( value /= x         ); }  \
	C     operator *= ( float x ) { return C( value *= x         ); }  \
	C     operator +  ( C     x ) { return C( value +  x.Value() ); }  \
	C     operator -  (         ) { return C(-value              ); }  \
	C     operator -  ( C     x ) { return C( value -  x.Value() ); }  \
	C     operator += ( C     x ) { return C( value += x.Value() ); }  \
	C     operator -= ( C     x ) { return C( value -= x.Value() ); }  \
	C     operator =  ( C     x ) { return C( value =  x.Value() ); }  \
	int   operator >  ( C     x ) { return  ( value >  x.Value() ); }  \
	int   operator <  ( C     x ) { return  ( value <  x.Value() ); }  \
	int   operator >= ( C     x ) { return  ( value >= x.Value() ); }  \
	int   operator <= ( C     x ) { return  ( value <= x.Value() ); }  \
	float operator /  ( C     x ) { return  ( value /  x.Value() ); }  \
	};                                                                 \
	inline ostream& operator<<(ostream &s, C x) {return x.Put(s,Symbol);}  \
	inline C Initializer( float x      )   { return C( x );             }  \
	inline C operator * ( float x, C y )   { return C( x * y.Value() ); }

	/*******************************************************************************
	* The following macros define permissible arithmetic operations among          *
	* variables with different physical meanings.  This ensures that the           *
	* result of any such operation is ALWAYS another meaningful quantity.          *
	*******************************************************************************/

#define SI_Square( A, B )                                                  \
	inline B operator*( A x, A y ) { return B( x.Value() * y.Value() ); }  \
	inline A operator/( B x, A y ) { return A( x.Value() / y.Value() ); }

#define SI_Recip( A, B )                                                   \
	inline B operator/( float x, A y ) { return B( x / y.Value() ); }      \
	inline A operator/( float x, B y ) { return A( x / y.Value() ); }      \
	inline float operator*( A x, B y ) { return x.Value() * y.Value(); }   \
	inline float operator*( B x, A y ) { return x.Value() * y.Value(); }

#define SI_Times( A, B, C )                                                \
	inline C operator*( A x, B y ) { return C( x.Value() * y.Value() ); }  \
	inline C operator*( B x, A y ) { return C( x.Value() * y.Value() ); }  \
	inline A operator/( C x, B y ) { return A( x.Value() / y.Value() ); }  \
	inline B operator/( C x, A y ) { return B( x.Value() / y.Value() ); }

	/*******************************************************************************
	* The following macros create classes for a variety of quantities.  These      *
	* include base qunatities such as "time" and "length" as well as derived       *
	* quantities such as "power" and "volume".  Each quantity is provided with     *
	* an initialization function in SI units and an abbreviation for printing.     *
	*******************************************************************************/

	SI_Make( SI_length         , meter           , "m"      ); // Base Units:
	SI_Make( SI_mass           , kilogram        , "kg"     );
	SI_Make( SI_time           , second          , "s"      );
	SI_Make( SI_lum_inten      , candela         , "cd"     );
	SI_Make( SI_temperature    , kelvin          , "K"      );
	SI_Make( SI_angle          , radian          , "rad"    ); // Supplementary:
	SI_Make( SI_solid_angle    , steradian       , "sr"     );
	SI_Make( SI_area           , meter2          , "m2"     ); // Derived units:
	SI_Make( SI_volume         , meter3          , "m3"     ); 
	SI_Make( SI_frequency      , hertz           , "Hz"     ); 
	SI_Make( SI_force          , newton          , "N"      );
	SI_Make( SI_energy         , joule           , "J"      );
	SI_Make( SI_power          , watt            , "W"      );
	SI_Make( SI_radiance       , watts_per_m2sr  , "W/m2sr" );
	SI_Make( SI_irradiance     , watts_per_m2    , "W/m2"   );
	SI_Make( SI_rad_inten      , watts_per_sr    , "W/sr"   );
	SI_Make( SI_luminance      , candela_per_m2  , "cd/m2"  );
	SI_Make( SI_illuminance    , lux             , "lx"     );
	SI_Make( SI_lum_flux       , lumen           , "lm"     );
	SI_Make( SI_lum_energy     , talbot          , "tb"     );
	SI_Make( SI_time2          , second2         , "s2"     ); // Intermediate: 
	SI_Make( SI_sa_area        , meter2_sr       , "m2sr"   );
	SI_Make( SI_inv_area       , inv_meter2      , "1/m2"   ); 
	SI_Make( SI_inv_solid_angle, inv_steradian   , "1/sr"   );
	SI_Make( SI_length_temp    , meters_kelvin   , "m K"    );
	SI_Make( SI_power_area     , watts_m2        , "W m2"   );
	SI_Make( SI_power_per_volume, watts_per_m3   , "W/m3"   );

	SI_Square( SI_length       , SI_area            );
	SI_Square( SI_time         , SI_time2           );
	SI_Recip ( SI_time         , SI_frequency       );
	SI_Recip ( SI_area         , SI_inv_area        );
	SI_Recip ( SI_solid_angle  , SI_inv_solid_angle );

	SI_Times( SI_area          , SI_length         , SI_volume      );
	SI_Times( SI_force         , SI_length         , SI_energy      );
	SI_Times( SI_power         , SI_time           , SI_energy      );
	SI_Times( SI_lum_flux      , SI_time           , SI_lum_energy  );
	SI_Times( SI_lum_inten     , SI_solid_angle    , SI_lum_flux    );
	SI_Times( SI_radiance      , SI_solid_angle    , SI_irradiance  );
	SI_Times( SI_rad_inten     , SI_solid_angle    , SI_power       );
	SI_Times( SI_irradiance    , SI_area           , SI_power       );
	SI_Times( SI_illuminance   , SI_area           , SI_lum_flux    );
	SI_Times( SI_solid_angle   , SI_area           , SI_sa_area     );
	SI_Times( SI_radiance      , SI_sa_area        , SI_power       );
	SI_Times( SI_irradiance    , SI_inv_solid_angle, SI_radiance    );
	SI_Times( SI_power         , SI_inv_solid_angle, SI_rad_inten   );
	SI_Times( SI_length        , SI_temperature    , SI_length_temp );
	SI_Times( SI_power         , SI_area           , SI_power_area  );

	/*******************************************************************************
	* Following are some useful non-SI units.  These units can be used in place of *
	* the unit-initializers above.  Thus, a variable of type SI_length, for example*
	* may be initialized in "meters", "inches", or "centimeters".  In all cases,   *
	* however, the value is converted to the underlying SI unit (e.g. meters).     *
	*******************************************************************************/

#define SI_Convert( SI, New, Old ) inline SI New( float x ) { return x * Old; }

	SI_Convert( SI_time        , minute     ,         second(     60.0 ) );
	SI_Convert( SI_time        , hour       ,         minute(     60.0 ) );
	SI_Convert( SI_force       , dyne       ,         newton(   1.0E-5 ) );
	SI_Convert( SI_energy      , erg        ,          joule(   1.0E-7 ) );
	SI_Convert( SI_power       , kilowatt   ,           watt(  SI_kilo ) );
	SI_Convert( SI_mass        , gram       ,       kilogram( SI_milli ) );
	SI_Convert( SI_length      , inch       ,          meter(  2.54E-2 ) );
	SI_Convert( SI_length      , foot       ,           inch(     12.0 ) );
	SI_Convert( SI_length      , centimeter ,          meter( SI_centi ) );
	SI_Convert( SI_length      , micron     ,          meter( SI_micro ) );
	SI_Convert( SI_length      , angstrom   ,          meter(  1.0E-10 ) );
	SI_Convert( SI_area        , barn       ,         meter2(  1.0E-28 ) );
	SI_Convert( SI_angle       , degree     ,         radian( 0.017453 ) );
	SI_Convert( SI_illuminance , phot       ,            lux(   1.0E+4 ) );
	SI_Convert( SI_illuminance , footcandle ,            lux(  9.29E-2 ) );
	SI_Convert( SI_luminance   , stilb      , candela_per_m2(   1.0E+4 ) );

	/*******************************************************************************
	* Often there are multiple names for a single quantity.  Below are some        *
	* synonyms for the quantities defined above.  These can be used in place of    *
	* the original quantities and may be clearer in some contexts.                 *
	*******************************************************************************/

	typedef SI_power       SI_radiant_flux;
	typedef SI_irradiance  SI_radiant_flux_density;
	typedef SI_irradiance  SI_radiant_exitance;
	typedef SI_radiance    SI_intensity;
	typedef SI_irradiance  SI_radiosity;
};
#endif