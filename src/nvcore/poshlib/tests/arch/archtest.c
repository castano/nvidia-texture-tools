#include "../../posh.c"

#include <stdio.h>

int main( void )
{
   printf( "archtest:\n" );
   printf( "--------\n" );

   printf( "%s", POSH_GetArchString() );
   printf( "byte min:  %d\n", POSH_BYTE_MIN );
   printf( "byte max:  %d\n", POSH_BYTE_MAX );
   printf( "i16  min:  %d\n",  POSH_I16_MIN );
   printf( "i16  max:  %d\n",  POSH_I16_MAX );
   printf( "i32  min:  %d\n",  POSH_I32_MIN );
   printf( "i32  max:  %d\n",  POSH_I32_MAX );
   printf( "u16  min:  %u\n", POSH_U16_MIN );
   printf( "u16  max:  %u\n", POSH_U16_MAX );
   printf( "u32  min:  %u\n", POSH_U32_MIN );
   printf( "u32  max:  %u\n", POSH_U32_MAX );
#ifdef POSH_64BIT_INTEGER
   printf( "i64  min:  %"POSH_I64_PRINTF_PREFIX"d\n", POSH_I64_MIN );
   printf( "i64  max:  %"POSH_I64_PRINTF_PREFIX"d\n", POSH_I64_MAX );
   printf( "u64  min:  %"POSH_I64_PRINTF_PREFIX"u\n", POSH_U64_MIN );
   printf( "u64  max:  %"POSH_I64_PRINTF_PREFIX"u\n", POSH_U64_MAX );
#endif

   return 0;
}
