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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Char.h"

namespace ArvoMath {

	typedef char *charPtr;

	// Treat "str" as a file name, and return just the directory
	// portion -- i.e. strip off the name of the leaf object (but
	// leave the final "/".
	const char *getPath( const char *str, char *buff )
	{
		int k;
		for( k = strlen( str ) - 1; k >= 0; k-- )
		{
			if( str[k] == Slash ) break;
		}
		for( int i = 0; i <= k; i++ ) buff[i] = str[i];
		buff[k+1] = NullChar;
		return buff;
	}

	// Treat "str" as a file name, and return just the file name
	// portion -- i.e. strip off everything up to and including
	// the final "/".
	const char *getFile( const char *str, char *buff )
	{
		int k;
		int len = strlen( str );
		for( k = len - 1; k >= 0; k-- )
		{
			if( str[k] == Slash ) break;
		}
		for( int i = 0; i < len - k; i++ ) buff[i] = str[ i + k + 1 ];
		return buff;
	}

	int getPrefix( const char *str, char *buff )
	{
		int len = 0;
		while( *str != NullChar && *str != Period ) 
		{
			*buff++ = *str++;
			len++;
		}
		*buff = NullChar;
		return len;
	}

	int getSuffix( const char *str, char *buff )
	{
		int n = strlen( str );
		int k = n - 1;
		while( k >= 0 && str[k] != Period ) k--;
		for( int i = k + 1; i < n; i++ ) *buff++ = str[i];
		*buff = NullChar;    
		return n - k - 1;
	}

	const char* toString( int number, char *buff )
	{
		static char local_buff[32];
		char *str = ( buff == NULL ) ? local_buff : buff;
		sprintf( str, "%d", number );
		return str;
	}

	const char* toString( float number, char *buff )
	{
		static char local_buff[32];
		char *str = ( buff == NULL ) ? local_buff : buff;
		sprintf( str, "%g", number );
		return str;
	}

	int isInteger( const char *str )
	{
		int n = strlen( str );
		for( int i = 0; i < n; i++ )
		{
			char c = str[i];
			if( isDigit(c) ) continue;
			if( c == Plus || c == Minus ) continue;
			if( c == Space ) continue;
			return 0;
		}
		return 1;
	}

	// Test to see if a string has a given suffix.
	int hasSuffix( const char *string, const char *suffix )
	{
		if( suffix == NULL ) return 1; // The null suffix always matches.
		if( string == NULL ) return 0; // The null string can only have a null suffix.
		int m = strlen( string );
		int k = strlen( suffix );
		if( k <= 0    ) return 1; // Empty suffix always matches.
		if( m < k + 1 ) return 0; // String is too short to have this suffix.

		// See if the file has the given suffix.
		int s = m - k;  // Beginning of suffix (if it matches).
		for( int i = 0; i < k; i++ )
			if( string[ s + i ] != suffix[ i ] ) return 0;
		return s;  // Always > 0.
	}

	// Test to see if a string has a given prefix.
	int hasPrefix( const char *string, const char *prefix )
	{
		if( prefix == NULL ) return 1; // The null prefix always matches.
		if( string == NULL ) return 0; // The null string can only have a null suffix.
		while( *prefix )
		{
			if( *prefix++ != *string++ ) return 0;
		}
		return 1;
	}

	// Test to see if the string contains the given character.
	int inString( char c, const char *str )
	{
		if( str == NULL || str[0] == NullChar ) return 0;
		while( *str != '\0' ) 
			if( *str++ == c ) return 1;
		return 0;
	}

	int nullString( const char *str )
	{
		return str == NULL || str[0] == NullChar;
	}

	const char *stripSuffix( const char *string, const char *suffix, char *buff )
	{
		static char local_buff[256];
		if( buff == NULL ) buff = local_buff;
		buff[0] = NullChar;
		if( !hasSuffix( string, suffix ) ) return NULL;
		int s = strlen( string ) - strlen( suffix );
		for( int i = 0; i < s; i++ )
		{
			buff[i] = string[i];
		}
		buff[s] = NullChar;
		return buff;
	}

	int getIndex( const char *pat, const char *str )
	{
		int p_len = strlen( pat );
		int s_len = strlen( str );
		if( p_len == 0 || s_len == 0 ) return -1;
		for( int i = 0; i <= s_len - p_len; i++ )
		{
			int match = 1;
			for( int j = 0; j < p_len; j++ )
			{
				if( str[ i + j ] != pat[ j ] ) { match = 0; break; }
			}
			if( match ) return i;
		}
		return -1;
	}

	int getSubstringAfter( const char *pat, const char *str, char *buff )
	{
		int ind = getIndex( pat, str );
		if( ind < 0 ) return -1;
		int p_len = strlen( pat );
		int k = 0;
		for( int i = ind + p_len; ; i++ )
		{
			buff[ k++ ] = str[ i ];
			if( str[ i ] == NullChar ) break;
		}
		return k;
	}

	const char *SubstringAfter( const char *pat, const char *str, char *user_buff )
	{
		static char temp[128];
		char *buff = ( user_buff != NULL ) ? user_buff : temp;
		int k = getSubstringAfter( pat, str, buff );
		if( k > 0 ) return buff;
		return str;
	}

	const char *metaString( const char *str, char *user_buff )
	{
		static char temp[128];
		char *buff = ( user_buff != NULL ) ? user_buff : temp;
		sprintf( buff, "\"%s\"", str );
		return buff;
	}

	// This is the opposite of metaString.
	const char *stripQuotes( const char *str, char *user_buff )
	{
		static char temp[128];
		char *buff = ( user_buff != NULL ) ? user_buff : temp;
		char *b = buff;
		for(;;)
		{
			if( *str != DoubleQuote ) *b++ = *str;
			if( *str == NullChar ) break; 
			str++;
		}
		return buff;
	}

	int getIntFlag( const char *flags, const char *flag, int &value )
	{
		while( *flags )
		{
			if( hasPrefix( flags, flag ) )
			{
				int k = strlen( flag );
				if( flags[k] == '=' )
				{
					value = atoi( flags + k + 1 );
					return 1;
				}
			}
			flags++;
		}
		return 0;
	}

	int getFloatFlag( const char *flags, const char *flag, float &value )
	{
		while( *flags )
		{
			if( hasPrefix( flags, flag ) )
			{
				int k = strlen( flag );
				if( flags[k] == '=' )
				{
					value = atof( flags + k + 1 );
					return 1;
				}
			}
			flags++;
		}
		return 0;
	}

	SortedList::SortedList( sort_type type_, int ascend_ )
	{
		type         = type_;
		ascend       = ascend_;
		num_elements = 0;
		max_elements = 0;
		sorted       = 1;
		list         = NULL;
	}

	SortedList::~SortedList()
	{
		Clear();
		delete[] list;
	}

	void SortedList::Clear()
	{
		// Delete all the private copies of the strings and re-initialize the
		// list.  Reuse the same list, expanding it when necessary.
		for( int i = 0; i < num_elements; i++ ) 
		{
			delete list[i];
			list[i] = NULL;
		}
		num_elements = 0;
		sorted       = 1;
	}

	SortedList &SortedList::operator<<( const char *str )
	{
		// Add a new string to the end of the list, expanding the list if necessary.
		// Mark the list as unsorted, so that the next reference to an element will
		// cause the list to be sorted again.
		if( num_elements == max_elements ) Expand();
		list[ num_elements++ ] = strdup( str );
		sorted = 0;
		return *this;
	}

	const char *SortedList::operator()( int i )
	{
		// Return the i'th element of the list.  Sort first if necessary.
		static char *null = "";
		if( num_elements == 0 || i < 0 || i >= num_elements ) return null;
		if( !sorted ) Sort();
		return list[i];
	}

	void SortedList::Expand()
	{
		// Create a new list of twice the size and copy the old list into it.
		// This doubles "max_elements", but leaves "num_elements" unchanged.
		if( max_elements == 0 ) max_elements = 1;
		max_elements *= 2;
		charPtr *new_list = new charPtr[ max_elements ];
		for( int i = 0; i < max_elements; i++ ) 
			new_list[i] = ( i < num_elements ) ? list[i] : NULL;
		delete[] list;
		list = new_list;
	}

	void SortedList::Swap( int i, int j )
	{
		char *temp = list[i];
		list[i] = list[j];
		list[j] = temp;
	}

	int SortedList::inOrder( int p, int q ) const
	{
		int test;
		if( type == sort_alphabetic )
			test = ( strcmp( list[p], list[q] ) <= 0 );
		else
		{
			int len_p = strlen( list[p] );
			int len_q = strlen( list[q] );
			test = ( len_p <  len_q ) || 
				( len_p == len_q && strcmp( list[p], list[q] ) <= 0 );
		}
		if( ascend ) return test;
		return !test;
	}

	// This is an insertion sort that operates on subsets of the
	// input defined by the step length.
	void SortedList::InsertionSort( int start, int size, int step ) 
	{
		for( int i = 0; i + step < size; i += step )
		{
			for( int j = i; j >= 0; j -= step )
			{
				int p = start + j;
				int q = p + step;
				if( inOrder( p, q ) ) break;
				Swap( p, q );
			}
		}
	}

	// This is a Shell sort.
	void SortedList::Sort()
	{
		for( int step  = num_elements / 2; step > 1; step /= 2 )
			for( int start = 0; start < step; start++ )
				InsertionSort( start, num_elements  - start, step );
		InsertionSort( 0, num_elements, 1 );
		sorted = 1;
	}

	void SortedList::SetOrder( sort_type type_, int ascend_ )
	{
		if( type_ != type || ascend_ != ascend )
		{
			type   = type_;
			ascend = ascend_;
			sorted = 0;
		}
	}

	int getstring( std::istream &in, const char *str )
	{
		char ch;
		if( str == NULL ) return 1;
		while( *str != NullChar )
		{
			in >> ch;
			if( *str != ch ) return 0;
			str++;
		}
		return 1;
	}

	std::istream &skipWhite( std::istream &in )
	{
		char c;
		while( in.get(c) ) 
		{
			if( !isWhite( c ) ) 
			{
				in.putback(c);
				break;
			}
		}
		return in;
	}
};
