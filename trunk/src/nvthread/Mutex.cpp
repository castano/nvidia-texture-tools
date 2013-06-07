// This code is in the public domain -- castano@gmail.com

#include "Mutex.h"

#if NV_OS_WIN32

#include "Win32.h"

#elif NV_OS_USE_PTHREAD

#include <pthread.h>
#include <errno.h> // EBUSY

#endif // NV_OS

using namespace nv;


#if NV_OS_WIN32

struct Mutex::Private {
    CRITICAL_SECTION mutex;
};


Mutex::Mutex () : m(new Private)
{
    InitializeCriticalSection(&m->mutex);
}

Mutex::~Mutex ()
{
    DeleteCriticalSection(&m->mutex);
}

void Mutex::lock()
{
    EnterCriticalSection(&m->mutex);
}

bool Mutex::tryLock()
{
    return TryEnterCriticalSection(&m->mutex) != 0;
}

void Mutex::unlock()
{
    LeaveCriticalSection(&m->mutex);
}

#elif NV_OS_USE_PTHREAD

struct Mutex::Private {
    pthread_mutex_t mutex;
};


Mutex::Mutex () : m(new Private)
{
    int result = pthread_mutex_init(&m->mutex , NULL);
    nvDebugCheck(result == 0);
}

Mutex::~Mutex ()
{
    int result = pthread_mutex_destroy(&m->mutex);
    nvDebugCheck(result == 0);
}

void Mutex::lock()
{
    int result = pthread_mutex_lock(&m->mutex);
    nvDebugCheck(result == 0);
}

bool Mutex::tryLock()
{
    int result = pthread_mutex_trylock(&m->mutex);
    nvDebugCheck(result == 0 || result == EBUSY);
    return result == 0;
}

void Mutex::unlock()
{
    int result = pthread_mutex_unlock(&m->mutex);
    nvDebugCheck(result == 0);
}

#endif // NV_OS_UNIX
