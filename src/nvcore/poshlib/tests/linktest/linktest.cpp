#include "testlib.hpp"
#include "testdll.h"

#include <stdio.h>

int main( void )
{
  printf( "linktest:\n" );
  printf( "---------\n" );
  printf( "linktest is a simple verification test that tests:\n" );
  printf( "  * correct linkage between C and C++\n" );
  printf( "  * proper handling when multiple libs use posh\n" );
  printf( "  * correct handling of DLL vs. LIB linkage (Windows)\n" );
  printf( "\n\n" );
  printf( "POSH_GetArchString() reporting:\n%s\n\n", POSH_GetArchString() );

  TestLib_Foo();
  TestDLL_Foo();

  printf( "\n\nlinktest succeeded!\n" );

  return 0;
}
