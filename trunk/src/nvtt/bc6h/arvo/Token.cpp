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
#include <stdlib.h>
#include <string.h>
#include "Token.h"
#include "Char.h"

namespace ArvoMath {

	FILE*  Token::debug = NULL;  // Static data member of Token class.
	int    Token::argc  = 0;
	char** Token::argv  = NULL;

	typedef TokMacro *TokMacroPtr;

	static const int True      = 1;
	static const int False     = 0;
	static const int HashConst = 217;  // Size of hash-table for macros.


	TokFrame::TokFrame()
	{
		next   = NULL;
		source = NULL;
		fname  = NULL;
		line   = 0;
		column = 0;
	}

	TokFrame::~TokFrame()
	{
		if( fname != NULL ) delete[] fname;
		if( source != NULL ) fclose( source );
	}

	void TokFrame::operator=( const TokFrame &frame )
	{
		next   = frame.next;
		source = frame.source;
		fname  = strdup( frame.fname );
		line   = frame.line;
		column = frame.column;
	}

	static int HashName( const char *str )
	{
		static int prime[5] = { 7, 11, 17, 23, 3 };
		int k = 0;
		int h = 0;
		while( *str != NullChar )
		{
			h += (*str++) * prime[k++];
			if( k == 5 ) k = 0;
		}
		if( h < 0 ) h = 0;  // Check for overflow.
		return h % HashConst;
	}

	TokMacro *Token::MacroLookup( const char *str ) const
	{
		if( table == NULL ) return NULL;
		int i = HashName( str );
		for( TokMacro *m = table[i]; m != NULL; m = m->next )
		{
			if( strcmp( str, m->macro ) == 0 ) return m;
		}
		return NULL;
	}

	int Token::MacroReplace( char *str, int &length, TokType &type ) const
	{
		TokMacro *m = MacroLookup( str );
		if( m == NULL ) return 0;
		strcpy( str, m->repl );
		length = strlen( str );
		type   = m->type;
		return 1;
	}

	/*-------------------------------------------------------------------------*
	*  D e b u g  P r i n t                                                   *
	*                                                                         *
	*  This routine is used to record the entire token stream in a file to    *
	*  use as a debugging aid.  It does not affect the action of the lexer;   *
	*  it merely records a "shadow" copy of all the tokens that are read by   *
	*  ANY Token instance.  The data that is written to the file is           *
	*                                                                         *
	*  <Line number>  <Column number>  <File name>  <Token>                   *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	static void DebugPrint( const Token &tok, FILE *fp )
	{
		fprintf( fp, "%3d %3d  ", tok.Line(), tok.Column() );
		fprintf( fp, "%s  "     , tok.FileName() ); 
		fprintf( fp, "%s\n"     , tok.Spelling() );
		fflush ( fp );
	}

	/*-------------------------------------------------------------------------*
	*  T o k e n   (Constructors)                                             *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Token::Token( const char *file_name )
	{
		Init();
		Open( file_name );
	}

	Token::Token( FILE *fp )
	{
		Init();
		Open( fp );
	}

	Token::Token( )
	{
		Init();
	}

	/*-------------------------------------------------------------------------*
	*  T o k e n   (Destructor)                                               *
	*                                                                         *
	*  Close all files and deletes all frames and paths.                      *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Token::~Token( )
	{
		// Don't try to delete "frame" as its a member of this class, not 
		// something that we've allocated.
		TokFrame *f = frame.next;
		while( f != NULL )
		{
			TokFrame *n = f->next;
			delete f;
			f = n;
		}
		ClearPaths();
	}

	/*-------------------------------------------------------------------------*
	*  O p e n                                                                *
	*                                                                         *
	*  Establish a new file to read from, either by name, or by pointer.      *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	void Token::Open( const char *file_name )
	{
		FILE *fp = fopen( file_name, "r" );
		if( fp == NULL ) return;
		Open( fp );
		frame.fname = strdup( file_name );
	}

	void Token::Open( FILE *fp )
	{
		frame.source = fp;
		frame.line   = 1;
		frame.column = 0;
		pushed       = NullChar;
	}

	/*-------------------------------------------------------------------------*
	*  O p e r a t o r  ==                                                    *
	*                                                                         *
	*  A token can be compared with a string, a single character, or a type.  *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Token::operator==( const char *s ) const
	{
		const char *t = spelling;
		if( case_sensitive )
		{
			do { if( *s != *t ) return False; } 
			while( *s++ && *t++ );
		}
		else
		{
			do { if( ToUpper(*s) != ToUpper(*t) ) return False; } 
			while( *s++ && *t++ );
		}
		return True;
	}

	int Token::operator==( char c ) const
	{
		if( length != 1 ) return False;
		if( case_sensitive ) return spelling[0] == c;
		else return ToUpper(spelling[0]) == ToUpper(c);
	}

	int Token::operator==( TokType _type_ ) const 
	{
		int match = 0;
		switch( _type_ )
		{ 
		case T_char   : match = ( type == T_string  && Len() == 1      ); break;
		case T_numeric: match = ( type == T_integer || type == T_float ); break;
		default       : match = ( type == _type_                       ); break;
		}
		return match;
	}

	/*-------------------------------------------------------------------------*
	*  O p e r a t o r  !=                                                    *
	*                                                                         *
	*  Define negations of the three types of "==" tests.                     *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Token::operator!=( const char *s ) const { return !( *this == s ); }
	int Token::operator!=( char        c ) const { return !( *this == c ); }
	int Token::operator!=( TokType     t ) const { return !( *this == t ); }

	/*-------------------------------------------------------------------------*
	*  E r r o r                                                              *
	*                                                                         *
	*  Print error message to "stderr" followed by optional "name".           *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	void Token::Error( TokError error, const char *name )
	{
		char *s;
		switch( error )
		{
		case T_malformed_float   : s = "malformed real number   "; break;
		case T_unterm_string     : s = "unterminated string     "; break;
		case T_unterm_comment    : s = "unterminated comment    "; break;
		case T_file_not_found    : s = "include file not found: "; break;
		case T_unknown_directive : s = "unknown # directive     "; break;
		case T_string_expected   : s = "string expected         "; break;
		case T_putback_error     : s = "putback overflow        "; break;
		case T_name_too_long     : s = "file name is too long   "; break;
		case T_no_endif          : s = "#endif directive missing"; break;
		case T_extra_endif       : s = "#endif with no #ifdef   "; break;
		case T_extra_else        : s = "#else with no #ifdef    "; break;
		default                  : s = "unknown error type      "; break;
		}
		fprintf( stderr, "LEXICAL ERROR, line %d, column %d: %s", 
			frame.line, frame.column, s );
		if( name == NULL )
			fprintf( stderr, "  \n"       );
		else fprintf( stderr, "%s\n", name );
		exit( 1 );
	}

	/*-------------------------------------------------------------------------*
	*  G e t c                                                                *
	*                                                                         *
	*  This routine fetches one character at a time from the current file     *
	*  being read.  It is responsible for keeping track of the column number  *
	*  and for handling single characters that have been "put back".          *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Token::Getc( int &c )
	{
		if( pushed != NullChar )  // Return the pushed character.
		{
			c = pushed;
			pushed = NullChar;
		}
		else  // Get a new character from the source file.
		{
			c = getc( frame.source );
			frame.column++;
		}
		return c;
	}

	/*-------------------------------------------------------------------------*
	*  N o n W h i t e                                                        *
	*                                                                         *
	*  This routine implements a simple finite state machine that skips       *
	*  white space and recognizes the two styles of comments used in C++.     *
	*  It returns the first non-white character not part of a comment.        *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Token::NonWhite( int &c )
	{
start_state:
		Getc( c );
		if( c == Space   ) goto start_state;
		if( c == Tab     ) goto start_state;
		if( c == NewLine ) goto start_new_line;
		if( c == Slash   ) goto start_comment;
		goto return_char;

start_comment:
		Getc( c );
		if( c == Star    ) goto in_comment1;  
		if( c == Slash   ) goto in_comment2;  
		Unget( c );
		c = Slash;
		goto return_char;

in_comment1:
		Getc( c );
		if( c == Star    ) goto end_comment1;
		if( c == NewLine ) goto newline_in_comment;
		if( c == EOF     ) goto return_char;
		goto in_comment1;

end_comment1:
		Getc( c );
		if( c == Slash   ) goto start_state;
		if( c == NewLine ) goto newline_in_comment;
		if( c == EOF     ) goto unterm_comment;
		goto in_comment1;

in_comment2:
		Getc( c );
		if( c == NewLine ) goto start_new_line;
		if( c == EOF     ) goto return_char;
		goto in_comment2;

unterm_comment:
		Error( T_unterm_comment );
		c = EOF;
		goto return_char;

start_new_line:
		frame.line++;
		frame.column = 0;
		goto start_state;

newline_in_comment:
		frame.line++;
		frame.column = 0;
		goto in_comment1;

return_char:
		Tcolumn = frame.column;  // This is where the token starts.
		return c;
	}

	/*-------------------------------------------------------------------------*
	*  N e x t R a w T o k                                                    *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Token::NextRawTok( )
	{
		static int Trans0[] = { 0, 1, 3, 3, 3 };  // Found a digit.
		static int Trans1[] = { 5, 6, 4, 6, 7 };  // Found a sign.
		static int Trans2[] = { 1, 6, 7, 6, 7 };  // Found decimal point.
		static int Trans3[] = { 2, 2, 7, 6, 7 };  // Found an exponent.
		static int Trans4[] = { 5, 6, 7, 6, 7 };  // Found something else.
		char       *tok     = spelling;
		int        state;
		int        c;

		length = 0;
		type   = T_null;

		// Skip comments and whitespace.

		if( NonWhite( c ) == EOF ) goto endtok;

		// Is this the beginning of an identifier?  If so, get the rest. 

		if( isAlpha( c ) )
		{
			type = T_ident;
			do  {
				*tok++ = c;
				length++;
				if( Getc( c ) == EOF ) goto endtok;
			}
			while( isAlpha( c ) || isDigit( c ) || c == Underscore );
			Unget( c );
			goto endtok;
		}

		// Is this the beginning of a number?

		else if( isDigit( c ) || c == Minus || c == Period )
		{
			char c1 = c;
			state = 0;
			for(;;)
			{
				*tok++ = c;
				length++;
				switch( Getc( c ) )
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9': state = Trans0[ state ]; break;
				case '+': 
				case '-': state = Trans1[ state ]; break;
				case '.': state = Trans2[ state ]; break;
				case 'e':
				case 'E': state = Trans3[ state ]; break;
				default : state = Trans4[ state ]; break;
				}
				switch( state )
				{
				case 5 : Unget( c ); 
					type = ( c1 == Period ) ? T_float : T_integer; 
					goto endtok;
				case 6 : Unget( c ); type = T_float  ; goto endtok;
				case 7 : Error( T_malformed_float ) ; break;
				default: continue;
				}
			} // for
		} // if numeric 

		// Is this the beginning of an operator?

		if( c == '*' || c == '>' || c == '<' || c == '+' || c == '-' || c == '!' )
		{
			char oldc = c;
			type = T_other;
			*tok++ = c;
			length++;
			if( Getc( c ) == EOF ) goto endtok;
			if( c == oldc || c == EqualSign )
			{
				*tok++ = c;
				length++;
			}
			else Unget( c );
			goto endtok;
		}

		// Is this the beginning of a string?

		else if( c == DoubleQuote )
		{
			type = T_string;
			while( Getc( c ) != EOF && length < MaxTokenLen )
			{
				if( c == DoubleQuote ) goto endtok;
				*tok++ = c;
				length++;
			}
			Error( T_unterm_string );
		}

		// Is this the beginning of a "#" directive?

		else if( c == Hash )
		{
			type = T_directive;
			NonWhite( c );
			while( isAlpha( c ) )
			{
				*tok++ = c;
				length++;
				Getc( c );
			}
			Unget( c );
			goto endtok;
		}

		// This must be a one-character token. 

		else
		{
			*tok++ = c;
			length = 1;
			type   = T_other;
		}

endtok: // Jump to here when token is completed.

		*tok = NullChar;  // Terminate the string.
		if( debug != NULL ) DebugPrint( *this, debug );

		return length;
	}

	/*-------------------------------------------------------------------------*
	*  N e x t T o k                                                          *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Token::NextTok( )
	{
		NextRawTok();

		// If the token is an identifier, see if it's a macro.
		// If the macro substitution is null, get another token.

		if( type == T_ident )
		{
			if( table != NULL )
			{
				if( MacroReplace( spelling, length, type ) && debug != NULL ) 
					DebugPrint( *this, debug );
			}
			if( type == T_nullmacro ) NextTok();
		}
		return length;
	}

	/*-------------------------------------------------------------------------*
	*  O p e r a t o r  - -                                                   *
	*                                                                         *
	*  Puts back the last token found.  Only one token can be put back.       *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Token & Token::operator--( )  // Put the last token back. 
	{
		if( put_back ) Error( T_putback_error );  // Can only handle one putback.
		put_back = 1; 
		return *this;
	}

	Token & Token::operator--( int )  // Postfix decrement.
	{
		fprintf( stderr, "Postfix decrement is not implemented for the Token class.\n" );
		return *this;
	}

	/*-------------------------------------------------------------------------*
	*  H a n d l e   D i r e c t i v e                                        *
	*                                                                         *
	*  Directive beginning with "#" must be handled by the lexer, as they     *
	*  determine the current source file via "#include", etc.                 *
	*                                                                         *
	*  Returns 1 if, after handling this directive, we now have the next      *
	*  token.                                                                 *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	int Token::HandleDirective( )
	{
		FILE *fp;
		char name[128];
		if( *this == "define" )
		{
			NextRawTok(); 
			strcpy( tempbuff, Spelling() );  // This is the macro name.
			int line = Line();
			NextRawTok();
			if( Line() == line )
				AddMacro( tempbuff, Spelling(), Type() );
			else
			{
				// If next token is on a different line; we went too far.
				AddMacro( tempbuff, "", T_nullmacro );
				return 1;  // Signal that we already have the next token.
			}
		}
		else if( *this == "include" )
		{
			NextRawTok();
			if( *this == "<" )
			{
				GetName( name, sizeof(name) );
				PushFrame( ResolveName( name ), name );
			}
			else if( type == T_string )
			{
				fp = fopen( spelling, "r" );
				if( fp == NULL ) Error( T_file_not_found, spelling );
				else PushFrame( fp, spelling );
			}
			else Error( T_string_expected );
		}
		else if( *this == "ifdef" )
		{
			NextRawTok();
			TokMacro *m = MacroLookup( Spelling() );
			if( m == NULL )  // Skip until else or endif.
			{
				while( *this != T_null )
				{
					NextRawTok();
					if( *this != T_directive ) continue;
					if( *this == "endif" ) break;
					if( *this == "else"  ) { if_nesting++; break; }  // Like m != NULL.
				}
				if( *this == T_null ) Error( T_no_endif );
				return 0; // Ready to get the next token.
			}
			else if_nesting++;
		}
		else if( *this == "ifndef" )
		{
			NextRawTok();
			TokMacro *m = MacroLookup( Spelling() );
			if( m != NULL )  // Skip until else or endif.
			{
				while( *this != T_null )
				{
					NextRawTok();
					if( *this != T_directive ) continue;
					if( *this == "endif" ) break;
					if( *this == "else"  ) { if_nesting++; break; }  // Like m == NULL.
				}
				if( *this == T_null ) Error( T_no_endif );
				return 0; // Ready to get the next token.
			}
			else if_nesting++;
		}
		else if( *this == "else" )  // Skip until #endif.
		{
			if( if_nesting == 0 ) Error( T_extra_else );
			while( *this != T_null )
			{
				NextRawTok();
				if( *this == T_directive && *this == "endif" ) break;
			}
			if( *this == T_null ) Error( T_no_endif );
			if_nesting--;
			return 0; // Ready to get next token.
		}
		else if( *this == "endif" )
		{
			if( if_nesting == 0 ) Error( T_extra_endif );
			if_nesting--;
			return 0; // Ready to get next token.
		}
		else if( *this == "error" )
		{
			int line = Line();
			NextTok(); // Allow macro substitution.
			if( Line() == line )
			{
				fprintf( stderr, "(preprocessor, line %d) %s\n", line, Spelling() );
				return 0; // Ready to get next token.
			}
			else
			{
				// If next token is on a different line; we went too far.
				fprintf( stderr, "(null preprocessor message, line %d)\n", line );
				return 1;  // Signal that we already have the next token.
			}
		}
		return 0;
	}


	/*-------------------------------------------------------------------------*
	*  O p e r a t o r  + +                                                   *
	*                                                                         *
	*  Grab the next token from the current source file.  If at end of file,  *
	*  pick up where we left off in the previous file.  If there is no        *
	*  previous file, return "T_null".                                        *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	Token & Token::operator++( )
	{
		if( put_back ) 
		{
			put_back = 0;
			return *this;
		}

		// If we've reached the end of an include file, pop the stack.

		for(;;)
		{
			NextTok();  
			if( type == T_directive ) 
			{
				if( HandleDirective() ) break;
			}
			else if( type == T_null ) 
			{
				fclose( frame.source );
				if( !PopFrame() ) break;
			}
			else break;  // We have a real token.
		}

		// Now fill in the value fields if the token is a number. 

		switch( type )
		{
		case T_integer : ivalue = atoi( spelling ); break;
		case T_float   : fvalue = atof( spelling ); break;
		case T_null    : if( if_nesting > 0 ) Error( T_no_endif ); break;
		default        : break;
		}

		return *this;
	}

	Token & Token::operator++( int )
	{
		fprintf( stderr, "Postfix increment is not implemented for the Token class.\n" );
		return *this;
	}

	/*-------------------------------------------------------------------------*
	*  T o k e n   Push & Pop Frame                                           *
	*                                                                         *
	*  These functions are used to create and destroy the context "frames"    *
	*  that are used to handle nested files (via "include").                  *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	void Token::PushFrame( FILE *fp, char *fname )
	{
		// Create a copy of the current (top-level) frame.

		TokFrame *n = new TokFrame;
		*n = frame;

		// Now overwrite the top-level frame with the new state.

		frame.next   = n;
		frame.source = fp;
		frame.line   = 1;
		frame.column = 0;
		frame.fname  = strdup( fname );
		pushed       = NullChar;
	}

	int Token::PopFrame()
	{
		if( frame.next == NULL ) return 0;
		TokFrame *old = frame.next;
		frame = *old;
		delete   old;  // Delete the frame that we just copied from.
		return 1;
	}

	/*-------------------------------------------------------------------------*
	*  Miscellaneous Functions                                                *
	*                                                                         *
	*-------------------------------------------------------------------------*/
	void Token::Init()
	{
		case_sensitive = 1;
		put_back       = 0;
		pushed         = NullChar;
		if_nesting     = 0;
		frame.source   = NULL;
		frame.next     = NULL;
		frame.fname    = NULL;
		first          = NULL;
		last           = NULL;
		table          = NULL;
		pushed         = NullChar;
		SearchArgs();  // Search command-line args for macro definitions.
	}

	const char* Token::Spelling() const 
	{ 
		return spelling;    
	}

	char Token::Char() const 
	{ 
		return spelling[0];
	}

	const char* Token::FileName() const
	{ 
		static char *null_string = "";
		if( frame.fname == NULL ) return null_string;
		else return frame.fname; 
	}

	float Token::Fvalue() const
	{
		float val = 0.0;
		if( type == T_float   ) val = fvalue;
		if( type == T_integer ) val = ivalue;
		return val;
	}

	void Token::GetName( char *name, int max )
	{
		int c;
		for( int i = 1; i < max; i++ )
		{
			if( NonWhite(c) == '>' ) 
			{ 
				*name = NullChar; 
				return; 
			}
			*name++ = c;
		}
		Error( T_name_too_long );
	}

	void Token::AddPath( const char *new_path )
	{
		char *name = strdup( new_path );
		if( name == NULL ) return;
		TokPath *p = new TokPath;
		p->next = NULL;
		p->path = name;
		if( first == NULL ) first = p;
		else last->next = p;
		last = p;
	}

	void Token::ClearPaths()
	{
		TokPath *p = first;
		while( p != NULL )
		{
			TokPath *q = p->next;
			delete[] p->path;  // delete the string.
			delete   p;        // delete the path structure.
			p = q;
		}
		first = NULL;
		last  = NULL;
	}

	FILE *Token::ResolveName( const char *name )
	{
		char resolved[128];
		for( const TokPath *p = first; p != NULL; p = p->next )
		{
			strcpy( resolved, p->path );
			strcat( resolved, "/"     );
			strcat( resolved, name    );
			FILE *fp = fopen( resolved, "r" );
			if( fp != NULL ) return fp;
		}
		Error( T_file_not_found, name );
		return NULL;
	}

	void Token::CaseSensitive( int on_off = 1 ) 
	{ 
		case_sensitive = on_off; 
	}

	void Token::Debug( FILE *fp ) 
	{ 
		debug = fp;
	}

	void Token::AddMacro( const char *macro, const char *repl, TokType t )
	{
		if( table == NULL ) // Create and initialize the table.
		{
			table = new TokMacroPtr[ HashConst ];
			for( int j = 0; j < HashConst; j++ ) table[j] = NULL;
		}
		int i = HashName( macro );    
		TokMacro *m = new TokMacro;
		m->next   = table[i];
		m->macro  = strdup( macro );
		m->repl   = strdup( repl  );
		m->type   = t;
		table[i]  = m;
	}

	void Token::Args( int argc_, char *argv_[] )
	{
		argc = argc_;  // Set the static variables.
		argv = argv_;
	}

	void Token::SearchArgs( )
	{
		TokType type = T_null;
		for( int i = 1; i < argc; i++ )
		{
			if( strcmp( argv[i], "-macro" ) == 0 )
			{
				if( i+2 >= argc ) 
				{
					fprintf( stderr, "(Token) ERROR macro argument(s) missing\n" );
					return;
				}
				char *macro = argv[i+1];
				char *repl  = argv[i+2];
				if( isAlpha  ( repl[0] ) ) type = T_ident  ; else
					if( isInteger( repl    ) ) type = T_integer; else
						type = T_float  ;
				AddMacro( macro, repl, type );
				i += 2;
			}
		}
	}
};
