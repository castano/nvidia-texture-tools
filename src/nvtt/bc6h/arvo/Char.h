/***************************************************************************
* Char.h                                                                   *
*                                                                          *
* Convenient constants, macros, and inline functions for manipulation of   *
* characters and strings.                                                  *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      arvo    07/01/93    Initial coding.                                 *
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
#ifndef __CHAR_INCLUDED__
#define __CHAR_INCLUDED__

#include <string>
#include <iostream>

namespace ArvoMath {

	static const char 
		Apostrophe  = '\'' ,
		Asterisk    = '*'  ,
		Atsign      = '@'  ,
		Backslash   = '\\' ,
		Bell        = '\7' ,
		Colon       = ':'  ,
		Comma       = ','  ,
		Dash        = '-'  ,
		DoubleQuote = '"'  ,
		EqualSign   = '='  ,
		Exclamation = '!'  ,
		GreaterThan = '>'  ,
		Hash        = '#'  ,
		Lbrack      = '['  ,
		Lcurley     = '{'  ,
		LessThan    = '<'  ,
		Lparen      = '('  ,
		Minus       = '-'  ,
		NewLine     = '\n' ,
		NullChar    = '\0' ,
		Percent     = '%'  ,
		Period      = '.'  ,
		Pound       = '#'  ,
		Plus        = '+'  ,
		Rbrack      = ']'  ,
		Rcurley     = '}'  ,
		Rparen      = ')'  ,
		Semicolon   = ';'  ,
		Space       = ' '  ,
		Slash       = '/'  ,
		Star        = '*'  ,
		Tab         = '\t' ,
		Tilde       = '~'  ,
		Underscore  = '_'  ;

	inline int  isWhite( char c ) { return c == Space || c == NewLine || c == Tab; }
	inline int  isUcase( char c ) { return 'A' <= c && c <= 'Z'; }
	inline int  isLcase( char c ) { return 'a' <= c && c <= 'z'; }
	inline int  isAlpha( char c ) { return isUcase( c ) || isLcase( c ); }
	inline int  isDigit( char c ) { return '0' <= c && c <= '9'; }
	inline char ToLower( char c ) { return isUcase( c ) ? c + ( 'a' - 'A' ) : c; }
	inline char ToUpper( char c ) { return isLcase( c ) ? c + ( 'A' - 'a' ) : c; }

	extern const char *getPath( 
		const char *str, 
		char *buff 
		);

	extern const char *getFile( 
		const char *str, 
		char *buff 
		);

	extern int getPrefix( 
		const char *str, 
		char *buff 
		);

	extern int getSuffix( 
		const char *str, 
		char *buff 
		);

	extern int isInteger( 
		const char *str
		);

	extern int hasSuffix( 
		const char *string, 
		const char *suffix 
		);

	extern int hasPrefix( 
		const char *string, 
		const char *prefix 
		);

	extern int inString( 
		char c, 
		const char *str 
		);

	extern int nullString( 
		const char *str 
		);

	extern const char *stripSuffix(  // Return NULL if unsuccessful.
		const char *string,  // The string to truncate.
		const char *suffix,  // The suffix to remove.
		char  *buff = NULL   // Defaults to internal buffer.
		);

	extern const char* toString( 
		int  n,            // An integer to convert to a string.
		char *buff = NULL  // Defauts to internal buffer.
		);

	extern const char* toString( 
		float x,           // A float to convert to a string.
		char *buff = NULL  // Defauts to internal buffer.
		);

	extern int getIndex( // The index of the start of a pattern in a string.
		const char *pat, // The pattern to look for.
		const char *str  // The string to search.
		);

	extern int getSubstringAfter( 
		const char *pat, 
		const char *str, 
		char *buff 
		);

	extern const char *SubstringAfter( 
		const char *pat, 
		const char *str,
		char *buff = NULL  // Defauts to internal buffer.
		);

	extern const char *metaString(
		const char *str,   // Make this a string within a string.
		char *buff = NULL  // Defauts to internal buffer.
		);

	extern const char *stripQuotes(
		const char *str,   // This is the opposite of metaString.
		char *buff = NULL  // Defauts to internal buffer.
		);

	extern int getIntFlag( 
		const char *flags, // List of assignment statements.
		const char *flag,  // A specific flag to look for.
		int &value         // The variable to assign the value to.
		);

	extern int getFloatFlag( 
		const char *flags, // List of assignment statements.
		const char *flag,  // A specific flag to look for.
		float &value       // The variable to assign the value to.
		);

	extern int getstring( 
		std::istream &in, 
		const char *str 
		);

	enum sort_type {
		sort_alphabetic,    // Standard dictionary ordering.
		sort_lexicographic  // Sort first by length, then alphabetically.
	};

	class SortedList {

	public:
		SortedList( sort_type = sort_alphabetic, int ascending = 1 );
		~SortedList();
		SortedList &operator<<( const char * );
		int Size() const { return num_elements; }
		const char *operator()( int i );
		void Clear();
		void SetOrder( sort_type = sort_alphabetic, int ascending = 1 );

	private:
		void Sort();
		void InsertionSort( int start, int size, int step );
		void Swap( int i, int j );
		void Expand();
		int  inOrder( int i, int j ) const;
		int  num_elements;
		int  max_elements;
		int  sorted;
		int  ascend;
		sort_type type;
		char **list;
	};


	inline int Match( const char *s, const char *t )
	{
		return s != NULL && 
			(t != NULL && strcmp( s, t ) == 0);
	}

	inline int Match( const char *s, const char *t1, const char *t2 )
	{
		return s != NULL && (
			(t1 != NULL && strcmp( s, t1 ) == 0) ||
			(t2 != NULL && strcmp( s, t2 ) == 0) );
	}

	union long_union_float {
		long  i;
		float f;
	};

	inline long float_as_long( float x )
	{
		long_union_float u;
		u.f = x;
		return u.i;
	}

	inline float long_as_float( long i )
	{
		long_union_float u;
		u.i = i;
		return u.f;
	}

	extern std::istream &skipWhite( std::istream &in );
};
#endif
