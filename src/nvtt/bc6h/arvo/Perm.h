/***************************************************************************
* Perm.h                                                                   *
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
#ifndef __PERM_INCLUDED__
#define __PERM_INCLUDED__

namespace ArvoMath {

	class Perm {
	public:
		Perm( const Perm & );                   // Initialize from a permutation.
		Perm( int a = 0, int b = 0 );           // Create permutation of ints a...b.
		Perm( const char * );                   // Create from string of numbers.
		~Perm() { delete p; }                    // Destructor.
		void  Get( char * ) const;              // Gets a string representation.
		int   Size() const { return b - a + 1;} // The number of elements.
		int   Min () const { return a; }        // The smallest value.
		int   Max () const { return b; }        // The largest value.
		int   operator++();                     // Make "next" permutation.
		int   operator--();                     // Make "previous" permutation.
		Perm &operator+=( int n );              // Advances by n permutations.
		Perm &operator-=( int n );              // Decrement by n permutations.
		Perm &operator =( const char * ) ;      // Resets from string of numbers.
		Perm &operator =( const Perm & ) ;      // Copy from another permutation.
		Perm &operator()( int i, int j ) ;      // Swap entries i and j.
		int   operator()( int n        ) const; // Index from Min() to Max().
		int   operator[]( int n        ) const; // Index from 0 to Size() - 1.
		Perm  operator ^( int n        ) const; // Exponentiation: -1 means inverse.
		Perm  operator *( const Perm & ) const; // Multiplication means composition.
		int   operator==( const Perm & ) const; // True if all elements match.
		int   operator<=( const Perm & ) const; // Lexicographic order relation.
	private:
		int& Elem( int i ) { return p[i]; }
		int  Next();
		int  Prev();
		int  a, b;
		int  *p;
		friend void Reset( Perm & );
	};


	// A "Partition" is a collection of k indistinguishable "balls" in n "bins".  
	// The Partition class encapsulates this notion and provides a convenient means 
	// of generating all possible partitions of k objects among n bins exactly once.  
	// Starting with all objects in bin zero, the ++ operator creates new and distinct
	// distributions among the bins until all objects are in the last bin.

	class Partition {
	public:
		Partition( );                              // Creates a null partition.
		Partition( const Partition & );            // Initialize from another partition.
		Partition( int bins, int balls );          // Specify # of bins & balls.
		~Partition() { delete Bin; }                // Descructor.
		void Get( char * ) const;                  // Gets a string representation.
		int  Bins () const { return bins;  }       // The number of bins.
		int  Balls() const { return balls; }       // The number of balls.
		void operator+=( int bin );                // Add a ball to this bin.
		void operator =( int n   );                // Set to the n'th configuration.
		void operator =( const Partition& );       // Copy from another partition.
		int  operator==( const Partition& ) const; // Compare two partitions.
		int  operator!=( const Partition& ) const; // Compare two partitions.
		int  operator++();                         // Make "next" partition.
		int  operator[]( int i ) const;            // Return # of balls in bin i.
		long NumCombinations() const;              // Number of distinct configurations.
	private:
		int  bins;
		int  balls;
		int* Bin;
		friend void Reset( Partition & );
	};


	// Predicates for determining when a permutation or partition is the last of
	// the sequence, functions for printing, resetting, and miscellaneous operations.

	extern int  End  ( const Partition & );  // True if all balls in last bin.
	extern int  End  ( const Perm      & );  // True if descending.
	extern int  Even ( const Perm      & );  // True if even # of 2-cycles.
	extern int  Odd  ( const Perm      & );  // True if odd # of 2-cycles.
	extern void Print( const Partition & );  // Write to standard out.
	extern void Print( const Perm      & );  // Write to standard out.
	extern void Reset(       Partition & );  // Reset to all balls in bin 0.
	extern void Reset(       Perm      & );  // Reset to ascending order.
};
#endif
