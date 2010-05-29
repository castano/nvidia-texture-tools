/***************************************************************************
* Token.h                                                                  *
*                                                                          *
* The Token class ecapsulates a lexical analyzer for C++-like syntax.      *
* A token instance is associated with one or more text files, and          *
* grabs C++ tokens from them sequentially.  There are many member          *
* functions designed to make parsing easy, such as "==" operators for      *
* strings and characters, and automatic conversion of numeric tokens       *
* into numeric values.                                                     *
*                                                                          *
* Files can be nested via #include directives, and both styles of C++      *
* comments are supported.                                                  *
*                                                                          *
*                                                                          *
*   HISTORY                                                                *
*      Name    Date        Description                                     *
*                                                                          *
*      arvo    10/05/99    Fixed bug in TokFrame string allocation.        *
*      arvo    01/15/95    Added ifdef, ifndef, else, and endif.           *
*      arvo    02/13/94    Added Debug() member function.                  *
*      arvo    01/22/94    Several sections rewritten.                     *
*      arvo    06/19/93    Converted to C++                                *
*      arvo    07/15/89    Rewritten for scene description parser.         *
*      arvo    01/22/89    Initial coding.                                 *
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
#ifndef __TOKEN_INCLUDED__
#define __TOKEN_INCLUDED__

#include <iostream>
#include <stdio.h>

namespace ArvoMath {

	const int MaxTokenLen = 128;

	typedef enum {
		T_null,   
		T_char,       // A string of length 1.
		T_string,
		T_integer,
		T_float,
		T_ident,
		T_other,
		T_numeric,    // Either T_float or T_int (use with == operator).
		T_directive,  // Directives like #include are not returned to the user.
		T_nullmacro
	} TokType;

	typedef enum {
		T_malformed_float,
		T_unterm_string,
		T_unterm_comment,
		T_file_not_found,
		T_unknown_directive,
		T_string_expected,
		T_putback_error,
		T_name_too_long,
		T_no_endif,
		T_extra_endif,
		T_extra_else
	} TokError;

	class TokFrame {
	public:
		TokFrame();
		TokFrame( const TokFrame &frame ) { *this = frame; }
		~TokFrame();
		void operator=( const TokFrame & );
	public:
		TokFrame *next;
		FILE     *source;
		char     *fname;
		int       line;    
		int       column;  
	};

	struct TokPath {
		char    *path;
		TokPath *next;
	};

	struct TokMacro {
		char     *macro;
		char     *repl;
		TokType   type;
		TokMacro *next;
	};

	class Token {

	public:
		// Constructors and destructor.

		Token();
		Token( const char *file_name );
		Token( FILE *file_pointer    );
		~Token();

		// Const data members for querying token information.

		TokType Type()    const { return type;       }  // The type of token found. 
		int     Len()     const { return length;     }  // The length of the token. 
		int     Line()    const { return frame.line; }  // The line it was found on.
		int     Column()  const { return Tcolumn;    }  // The column it began in.  
		long    Ivalue()  const { return ivalue;     }  // Token value if an integer.
		float   Fvalue()  const;                        // Token value if int or float.
		char    Char()    const;                        // The token (if a Len() == 1).

		// Operators.

		int     operator == ( const char* ) const;      // 1 if strings match.
		int     operator != ( const char* ) const;      // 0 if strings match.
		int     operator == ( char        ) const;      // 1 if token is this char.
		int     operator != ( char        ) const;      // 0 if token is this char.
		int     operator == ( TokType     ) const;      // 1 if token is of this type.
		int     operator != ( TokType     ) const;      // 0 if token is of this type.
		Token & operator ++ (             );            // (prefix) Get the next token.
		Token & operator -- (             );            // (prefix) Put back one token.
		Token & operator ++ ( int         );            // (postfix) Undefined.
		Token & operator -- ( int         );            // (postfix) Undefined.

		// State-setting member functions.

		void Open( FILE * );                            // Read already opened file.
		void Open( const char * );                      // Open the named file.
		void CaseSensitive( int on_off );               // Applies to == and != operators.
		void AddPath( const char * );                   // Adds path for <...> includes.
		void ClearPaths();                              // Remove all search paths.

		// Miscellaneous.

		const char* Spelling() const;                   // The token itself.
		const char* FileName() const;                   // Current file being lexed.
		static void Debug( FILE * );                    // Write all token streams to a file.
		static void Args ( int argc, char *argv[] );    // Search args for macro settings.
		void AddMacro( const char*, const char*, TokType type );
		void SearchArgs();

	private:

		// Private member functions.       

		void     Init();
		int      Getc ( int & );
		void     Unget( int c ) { pushed = c; }
		void     Error( TokError error, const char *name = NULL );
		int      NonWhite( int & );
		int      HandleDirective();
		int      NextRawTok();  // No macro substitutions.
		int      NextTok();
		void     PushFrame( FILE *fp, char *fname = NULL );
		int      PopFrame();
		void     GetName( char *name, int max );
		FILE     *ResolveName( const char *name );
		TokMacro *MacroLookup( const char *str ) const;
		int      MacroReplace( char *str, int &length, TokType &type ) const;

		// Private data members.       

		TokPath  *first;
		TokPath  *last;
		TokMacro **table;
		TokFrame frame;
		TokType  type;
		long     ivalue;  
		float    fvalue;  
		int      length;  
		int      Tcolumn;  
		int      put_back;    
		int      case_sensitive;
		int      pushed;
		int      if_nesting;
		char     spelling[ MaxTokenLen ];
		char     tempbuff[ MaxTokenLen ];

		// Static data members.       

		static int  argc;
		static char **argv;
		static FILE *debug;
	};


	// Predicate-style functions for testing token types.

	inline int Null   ( const Token &t ) { return t.Type() == T_null;    }
	inline int Numeric( const Token &t ) { return t.Type() == T_numeric; }
	inline int StringP( const Token &t ) { return t.Type() == T_string;  }
};
#endif
