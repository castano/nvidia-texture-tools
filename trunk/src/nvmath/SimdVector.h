// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "Vector.h" // Vector3, Vector4

// Set some reasonable defaults.
#ifndef NV_USE_ALTIVEC
#   define NV_USE_ALTIVEC NV_CPU_PPC
//#   define NV_USE_ALTIVEC defined(__VEC__)
#endif

#ifndef NV_USE_SSE
#   if NV_CPU_X86 || NV_CPU_X86_64
#       define NV_USE_SSE 2
#   endif
#   if defined(__SSE2__)
#       define NV_USE_SSE 2
#   elif defined(__SSE__)
#       define NV_USE_SSE 1
#   else
#       define NV_USE_SSE 0
#   endif
#endif

// Internally set NV_USE_SIMD when either altivec or sse is available.
#if NV_USE_ALTIVEC && NV_USE_SSE
#	error "Cannot enable both altivec and sse!"
#endif

#if NV_USE_ALTIVEC
#   include "SimdVector_VE.h"
#endif

#if NV_USE_SSE
#   include "SimdVector_SSE.h"
#endif
