// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>

#ifndef CMDLINE_H
#define CMDLINE_H

#include <nvcore/Debug.h>

#include <stdarg.h>

struct MyMessageHandler : public nv::MessageHandler {
	MyMessageHandler() {
		nv::debug::setMessageHandler( this );
	}
	~MyMessageHandler() {
		nv::debug::resetMessageHandler();
	}

	virtual void log( const char * str, va_list arg ) {
		va_list val;
		va_copy(val, arg);
		vfprintf(stderr, str, arg);
		va_end(val);		
	}
};


struct MyAssertHandler : public nv::AssertHandler {
	MyAssertHandler() {
		nv::debug::setAssertHandler( this );
	}
	~MyAssertHandler() {
		nv::debug::resetAssertHandler();
	}
	
	// Handler method, note that func might be NULL!
	virtual int assert( const char *exp, const char *file, int line, const char *func ) {
		fprintf(stderr, "Assertion failed: %s\nIn %s:%d\n", exp, file, line);
		nv::debug::dumpInfo();
		exit(1);
	}
};


#endif // CMDLINE_H
