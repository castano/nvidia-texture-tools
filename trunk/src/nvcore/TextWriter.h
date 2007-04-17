// This code is in the public domain -- castanyo@yahoo.es

#ifndef NVCORE_TEXTWRITER_H
#define NVCORE_TEXTWRITER_H

#include <nvcore/nvcore.h>
#include <nvcore/Stream.h>
#include <nvcore/StrLib.h>

// @@ NOT IMPLEMENTED !!!


namespace nv
{

	/// Text writer.
	class NVCORE_CLASS TextWriter
	{
	public:
	
		/// Ctor.
		TextWriter(Stream * s) : s(s), str(1024) {
			nvDebugCheck(s != NULL);
			nvCheck(s->IsSaving());
		}
	
		void write( const char * str, uint len );
		void write( const char * format, ... ) __attribute__((format (printf, 2, 3)));
		void write( const char * format, va_list arg );
	
	
	private:
	
		Stream * s;
		
		// Temporary string.
		StringBuilder str;
	
	};

} // nv namespace


#endif // NVCORE_TEXTWRITER_H
