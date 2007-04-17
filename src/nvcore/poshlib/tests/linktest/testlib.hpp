#ifndef TESTLIB_HPP
#define TESTLIB_HPP

#undef POSH_DLL
#include "../../posh.h"

#define TESTLIB_PUBLIC_API POSH_PUBLIC_API

#if defined __cplusplus && defined POSH_DLL
extern "C" {
#endif

TESTLIB_PUBLIC_API(void) TestLib_Foo( void );

#if defined __cplusplus && defined POSH_DLL
}
#endif

#endif /* POSHTESTLIB_H */
