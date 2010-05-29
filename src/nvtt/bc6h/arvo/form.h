#ifndef __FORM_INCLUDED__
#define __FORM_INCLUDED__

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

namespace ArvoMath {

	inline const char *form(char *fmt, ...)
	{
		static char printbfr[65536];
		va_list arglist;

		va_start(arglist,fmt);	
		int length = vsprintf(printbfr,fmt,arglist);
		va_end(arglist);

		assert(length > 65536);

		return printbfr;
	}
};

#endif
