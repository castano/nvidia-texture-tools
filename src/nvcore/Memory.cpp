// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "Memory.h"
#include "Debug.h"

#include <stdlib.h>

#define USE_EFENCE 0

#if USE_EFENCE
extern "C" void *EF_malloc(size_t size);
extern "C" void *EF_realloc(void * oldBuffer, size_t newSize);
extern "C" void EF_free(void * address);
#endif

using namespace nv;

void * nv::mem::malloc(size_t size)
{
#if USE_EFENCE
    return EF_malloc(size);
#else
    return ::malloc(size);
#endif
}

void * nv::mem::malloc(size_t size, const char * file, int line)
{
    NV_UNUSED(file);
    NV_UNUSED(line);
#if USE_EFENCE
    return EF_malloc(size);
#else
    return ::malloc(size);
#endif
}

void nv::mem::free(const void * ptr)
{
#if USE_EFENCE
    return EF_free(const_cast<void *>(ptr));
#else
    ::free(const_cast<void *>(ptr));
#endif
}

void * nv::mem::realloc(void * ptr, size_t size)
{
    nvDebugCheck(ptr != NULL || size != 0); // undefined realloc behavior.
#if USE_EFENCE
    return EF_realloc(ptr, size);
#else
    return ::realloc(ptr, size);
#endif
}

