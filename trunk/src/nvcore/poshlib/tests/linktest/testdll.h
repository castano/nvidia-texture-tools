#ifndef TESTDLL_H
#define TESTDLL_H

#define POSH_DLL 1     //define this since poshtestdll is a DLL
#include "../../posh.h"
#undef POSH_DLL        //undefine so that another include of posh.h doesn't cause problems

#define TESTDLL_PUBLIC_API POSH_PUBLIC_API

#if defined __cplusplus
extern "C" {
#endif

TESTDLL_PUBLIC_API(void) TestDLL_Foo( void );

#if defined __cplusplus
}
#endif

#endif /* TESTDLL_H */
