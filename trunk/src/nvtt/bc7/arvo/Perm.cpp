/***************************************************************************
* Perm.C                                                                   *
*                                                                          *
* This file defines permutation class: that is, a class for creating and   *
* manipulating finite sequences of distinct integers.  The main feature    *
* of the class is the "++" operator that can be used to step through all   *
* N! permutations of a sequence of N integers.  As the set of permutations *
* forms a multiplicative group, a multiplication operator and an           *
* exponentiation operator are also defined.                                *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      arvo    07/01/93    Added the Partition class.                      *
*      arvo    03/23/93    Initial coding.                                 *
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
#include <stdio.h>
#include <string.h>
#include "Perm.h"
#include "ArvoMath.h"
#include "Char.h"

namespace ArvoMath {

	/***************************************************************************
	*                                                              
	*  L O C A L   F U N C T I O N S
	*
	***************************************************************************/

	static void Reverse( int *p, int n )
	{
		int k = n >> 1;
		int m = n - 1;
		for( int i = 0; i < k; i++ ) Swap( p[i], p[m-i] );
	}

	static void Error( char *msg )
	{
		fprintf( stderr, "ERROR: Perm, %s.\n", msg );
	}

	/***************************************************************************
	**
	**  M E M B E R   F U N C T I O N S
	**
	***************************************************************************/

	Perm::Perm( int Left, int Right )
	{
		a = ( Left < Right ) ? Left : Right;
		b = ( Left > Right ) ? Left : Right;
		p = new int[ Size() ];
		Reset( *this );
	}

	Perm::Perm( const Perm &Q )
	{
		a = Q.Min();
		b = Q.Max();
		p = new int[ Q.Size() ];
		for( int i = 0; i < Size(); i++ ) p[i] = Q[i];
	}

	Perm::Perm( const char *str )
	{
		(*this) = str;
	}

	Perm &Perm::operator=( const char *str )
	{
		int  k, m = 0, n = 0;
		char dig[10];
		char c;
		if( p != NULL ) delete[] p;
		p = new int[ strlen(str)/2 + 1 ];
		for(;;)
		{
			c = *str++;
			if( isDigit(c) ) dig[m++] = c;
			else if( m > 0 )
			{ 
				dig[m] = NullChar;
				sscanf( dig, "%d", &k );
				if( n == 0 ) a = k; else if( k < a ) a = k;
				if( n == 0 ) b = k; else if( k > b ) b = k;
				p[n++] = k;
				m = 0; 
			}
			if( c == NullChar ) break;
		}
		for( int i = 0; i < n; i++ )
		{
			int N = i + a;
			int okay = 0;
			for( int j = 0; j < n; j++ )
				if( p[j] == N ) { okay = 1; break; }
				if( !okay )
				{
					Error( "string is not a valid permutation" );
					return *this;
				}
		}
		return *this;
	}

	void Perm::Get( char *str ) const
	{
		for( int i = 0; i < Size(); i++ )
			str += sprintf( str, "%d ", p[i] );
		*str = NullChar;
	}

	int Perm::Next()
	{
		int i, m, k = 0;
		int N, M = 0;

		// Look for the first element of p that is larger than its successor.
		// If no such element exists, we are done.

		M = p[0];                      // M is always the "previous" value.
		for( i = 1; i < Size(); i++ )  // Now start with second element.
		{
			if( p[i] > M ) { k = i; break; }
			M = p[i];
		}
		if( k == 0 ) return 0; // Already in descending order.
		m = k - 1;

		// Find the largest entry before k that is less than p[k].
		// One exists because p[k] is bigger than M, i.e. p[k-1].

		N = p[k];
		for( i = 0; i < k - 1; i++ )
		{
			if( p[i] < N && p[i] > M ) { M = p[i]; m = i; }
		}
		Swap( p[m], p[k] ); // Entries 0..k-1 are still decreasing.
		Reverse( p, k );    // Make first k elements increasing.
		return 1;
	}

	int Perm::Prev()
	{
		int i, m, k = 0;
		int N, M = 0;

		// Look for the first element of p that is less than its successor.
		// If no such element exists, we are done.

		M = p[0];                      // M will always be the "previous" value.
		for( i = 1; i < Size(); i++ )  // Start with the second element.
		{
			if( p[i] < M ) { k = i; break; }
			M = p[i];
		}
		if( k == 0 ) return 0; // Already in ascending order.
		m = k - 1;

		// Find the smallest entry before k that is greater than p[k].
		// One exists because p[k] is less than M, i.e. p[k-1].

		N = p[k];
		for( i = 0; i < k - 1; i++ )
		{
			if( p[i] > N && p[i] < M ) { M = p[i]; m = i; }
		}
		Swap( p[m], p[k] ); // Entries 0..k-1 are still increasing.
		Reverse( p, k );    // Make first k elements decreasing.
		return 1;
	}


	/***************************************************************************
	**
	**  O P E R A T O R S
	**
	***************************************************************************/

	int Perm::operator++()
	{
		return Next();
	}

	int Perm::operator--()
	{
		return Prev();
	}

	Perm &Perm::operator+=( int n )
	{
		int i;
		if( n > 0 ) for( i = 0; i < n; i++ ) if( !Next() ) break;
		if( n < 0 ) for( i = n; i < 0; i++ ) if( !Prev() ) break;
		return *this;
	}

	Perm &Perm::operator-=( int n )
	{
		int i;
		if( n > 0 ) for( i = 0; i < n; i++ ) if( !Prev() ) break;
		if( n < 0 ) for( i = n; i < 0; i++ ) if( !Next() ) break;
		return *this;
	}

	int Perm::operator[]( int n ) const
	{
		if( n < 0 || Size() <= n ) 
		{
			Error( "permutation index[] out of range" );
			return 0;
		}
		return p[ n ];
	}

	int Perm::operator()( int n ) const
	{
		if( n < Min() || Max() < n ) 
		{
			Error( "permutation index() out of range" );
			return 0;
		}
		return p[ n - Min() ];
	}

	Perm &Perm::operator=( const Perm &Q )
	{
		if( Size() != Q.Size() )
		{
			delete[] p;
			p = new int[ Q.Size() ];
		}
		a = Q.Min();
		b = Q.Max();
		for( int i = 0; i < Size(); i++ ) p[i] = Q[i];
		return *this;
	}

	Perm Perm::operator*( const Perm &Q ) const
	{
		if( Min() != Q.Min() ) return Perm(0);
		if( Max() != Q.Max() ) return Perm(0);
		Perm A( Min(), Max() );
		for( int i = 0; i < Size(); i++ ) A.Elem(i) = p[ Q[i] - Min() ];
		return A;
	}

	Perm Perm::operator^( int n ) const
	{
		Perm A( Min(), Max() );
		int pn = n;
		if( n < 0 ) // First compute the inverse.
		{
			for( int i = 0; i < Size(); i++ )
				A.Elem( p[i] - Min() ) = i + Min();
			pn = -n;
		}
		for( int i = 0; i < Size(); i++ )
		{
			int k = ( n < 0 ) ? A[i] : p[i];
			for( int j = 1; j < pn; j++ ) k = p[ k - Min() ];
			A.Elem(i) = k;
		}
		return A;
	}

	Perm &Perm::operator()( int i, int j )
	{
		Swap( p[ i - Min() ], p[ j - Min() ] );
		return *this;
	}

	int Perm::operator==( const Perm &Q ) const
	{
		int i;
		if( Min() != Q.Min() ) return 0;
		if( Max() != Q.Max() ) return 0;
		for( i = 0; i < Size(); i++ ) if( p[i] != Q[i] ) return 0;
		return 1;
	}

	int Perm::operator<=( const Perm &Q ) const
	{
		int i;
		if( Min() != Q.Min() ) return 0;
		if( Max() != Q.Max() ) return 0;
		for( i = 0; i < Size(); i++ ) if( p[i] != Q[i] ) return p[i] < Q[i];
		return 1;
	}

	void Reset( Perm &P )
	{
		for( int i = 0; i < P.Size(); i++ ) P.Elem(i) = P.Min() + i;
	}

	int End( const Perm &P )
	{
		int c = P[0];
		for( int i = 1; i < P.Size(); i++ ) 
		{
			if( c < P[i] ) return 0;
			c = P[i];
		}
		return 1;
	}

	void Print( const Perm &P )
	{
		if( P.Size() > 0 )
		{
			printf( "%d", P[0] );
			for( int i = 1; i < P.Size(); i++ ) printf( " %d", P[i] );
			printf( "\n" );
		}
	}

	int Even( const Perm &P )
	{
		return !Odd( P );
	}

	int Odd( const Perm &P )
	{
		int count = 0;
		Perm Q( P );
		for( int i = P.Min(); i < P.Max(); i++ )
		{
			if( Q(i) == i ) continue;
			for( int j = P.Min(); j <= P.Max(); j++ )
			{
				if( j == i ) continue;
				if( Q(j) == i )
				{
					Q(i,j);
					count = ( j - i ) + ( count % 2 );
				}
			}
		}
		return count % 2;
	}


	/***************************************************************************
	**
	**  P A R T I T I O N S
	**
	***************************************************************************/

	Partition::Partition( )
	{
		Bin   = NULL;
		bins  = 0;
		balls = 0;
	}

	Partition::Partition( const Partition &Q )
	{
		Bin   = new int[ Q.Bins() ];
		bins  = Q.Bins();
		balls = Q.Balls();
		for( int i = 0; i < bins; i++ ) Bin[i] = Q[i];
	}

	Partition::Partition( int bins_, int balls_ )
	{
		bins  = bins_;    
		balls = balls_;
		Bin   = new int[ bins ];
		Reset( *this );
	}

	void Partition::operator+=( int bin )  // Add a ball to this bin.
	{
		if( bin < 0 || bin >= bins ) fprintf( stderr, "ERROR -- bin number out of range.\n" );
		balls++;
		Bin[ bin ]++;
	}

	int Partition::operator==( const Partition &P ) const  // Compare two partitions.
	{
		if( Balls() != P.Balls() ) return 0;
		if( Bins () != P.Bins () ) return 0;
		for( int i = 0; i < bins; i++ )
		{
			if( Bin[i] != P[i] ) return 0;
		}
		return 1;
	}

	void Partition::operator=( int n )  // Set to the n'th configuration.
	{
		Reset( *this );
		for( int i = 0; i < n; i++ ) ++(*this);
	}

	int Partition::operator!=( const Partition &P ) const
	{
		return !( *this == P );
	}

	void Partition::operator=( const Partition &Q )
	{
		if( bins != Q.Bins() )
		{
			delete[] Bin;
			Bin = new int[ Q.Bins() ];
		}
		bins  = Q.Bins();
		balls = Q.Balls();
		for( int i = 0; i < bins; i++ ) Bin[i] = Q[i];
	}

	void Partition::Get( char *str ) const
	{
		for( int i = 0; i < bins; i++ )
			str += sprintf( str, "%d ", Bin[i] );
		*str = NullChar;
	}

	int Partition::operator[]( int i ) const
	{
		if( i < 0 || i >= bins ) return 0;
		else return Bin[i];
	}

	long Partition::NumCombinations() const  // How many distinct configurations.
	{
		// Think of the k "bins" as being k - 1 "partitions" mixed in with
		// the n "balls".  If the balls and partitions were each distinguishable
		// objects, there would be (n + k - 1)! distinct configurations.  
		// But since both the balls and the partitions are  indistinguishable, 
		// we simply divide by n! (k - 1)!.  This is the binomial coefficient 
		// ( n + k - 1, n ).
		//
		if( balls == 0 ) return 0;
		if( bins  == 1 ) return 1;
		return (long)floor( BinomialCoeff( balls + bins - 1, balls ) + 0.5 );
	}

	/***************************************************************************
	*  O P E R A T O R + +   (Next Partition)                                  *
	*                                                                          *
	*  Rearranges the n "balls" in k "bins" into the next configuration.       *
	*  The first config is assumed to be all balls in the first bin -- i.e.    *
	*  Bin[0].  All possible groupings are generated, each exactly once.  The  *
	*  function returns 1 if successful, 0 if the last config has already been *
	*  reached.  (Algorithm by Harold Zatz)                                    *
	*                                                                          *
	***************************************************************************/
	int Partition::operator++()
	{
		int i;
		if( Bin[0] > 0 )
		{
			Bin[1] += 1;
			Bin[0] -= 1;
		}
		else
		{
			for( i = 1; Bin[i] == 0; i++ );
			if( i == bins - 1 ) return 0;
			Bin[i+1] += 1;
			Bin[0] = Bin[i] - 1;
			Bin[i] = 0;
		}
		return 1;
	}

	void Reset( Partition &P )
	{
		P.Bin[0] = P.Balls();
		for( int i = 1; i < P.Bins(); i++ ) P.Bin[i] = 0;
	}

	int End( const Partition &P )
	{
		return P[ P.Bins() - 1 ] == P.Balls();
	}

	void Print( const Partition &P )
	{
		if( P.Bins() > 0 )
		{
			printf( "%d", P[0] );
			for( int i = 1; i < P.Bins(); i++ ) printf( " %d", P[i] );
			printf( "\n" );
		}
	}
};
