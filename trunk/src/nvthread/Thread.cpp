// This code is in the public domain -- castano@gmail.com

#include "Thread.h"

#if NV_OS_WIN32
    #include "Win32.h"
#elif NV_OS_USE_PTHREAD
    #include <pthread.h>
    #include <unistd.h> // usleep
#endif

using namespace nv;

struct Thread::Private
{
#if NV_OS_WIN32
    HANDLE thread;
#elif NV_OS_USE_PTHREAD
    pthread_t thread;
#endif

    ThreadFunc * func;
    void * arg;
};


#if NV_OS_WIN32

unsigned long __stdcall threadFunc(void * arg) {
    Thread::Private * thread = (Thread::Private *)arg;
    thread->func(thread->arg);
    return 0;
}

#elif NV_OS_USE_PTHREAD

extern "C" void * threadFunc(void * arg) {
    Thread::Private * thread = (Thread::Private *)arg;
    thread->func(thread->arg);
    pthread_exit(0);
}

#endif


Thread::Thread() : p(new Private)
{
    p->thread = 0;
}

Thread::~Thread()
{
    nvDebugCheck(p->thread == 0);
}

void Thread::start(ThreadFunc * func, void * arg)
{
    p->func = func;
    p->arg = arg;

#if NV_OS_WIN32
    p->thread = CreateThread(NULL, 0, threadFunc, p.ptr(), 0, NULL);
    //p->thread = (HANDLE)_beginthreadex (0, 0, threadFunc, p.ptr(), 0, NULL);     // @@ So that we can call CRT functions...
    nvDebugCheck(p->thread != NULL);
#elif NV_OS_USE_PTHREAD
    int result = pthread_create(&p->thread, NULL, threadFunc, p.ptr());
    nvDebugCheck(result == 0);
#endif
}

void Thread::wait()
{
#if NV_OS_WIN32
    DWORD status = WaitForSingleObject (p->thread, INFINITE);
    nvCheck (status ==  WAIT_OBJECT_0);
    BOOL ok = CloseHandle (p->thread);
    p->thread = NULL;
    nvCheck (ok);
#elif NV_OS_USE_PTHREAD
    int result = pthread_join(p->thread, NULL);
    p->thread = 0;
    nvDebugCheck(result == 0);
#endif
}

bool Thread::isRunning () const
{
#if NV_OS_WIN32
    return p->thread != NULL;
#elif NV_OS_USE_PTHREAD
    return p->thread != 0;
#endif
}

/*static*/ void Thread::spinWait(uint count)
{
    for (uint i = 0; i < count; i++) {}
}

/*static*/ void Thread::yield()
{
#if NV_OS_WIN32
    SwitchToThread();
#elif NV_OS_USE_PTHREAD
    int result = sched_yield();
    nvDebugCheck(result == 0);
#endif
}

/*static*/ void Thread::sleep(uint ms)
{
#if NV_OS_WIN32
    Sleep(ms);
#elif NV_OS_USE_PTHREAD
    usleep(1000 * ms);
#endif
}

/*static*/ void Thread::wait(Thread * threads, uint count)
{
/*#if NV_OS_WIN32
    // @@ Is there any advantage in doing this?
    nvDebugCheck(count < MAXIMUM_WAIT_OBJECTS);

    HANDLE * handles = new HANDLE[count];
    for (uint i = 0; i < count; i++) {
        handles[i] = threads->p->thread;
    }

    DWORD result = WaitForMultipleObjects(count, handles, TRUE, INFINITE);

    for (uint i = 0; i < count; i++) {
        CloseHandle (threads->p->thread);
        threads->p->thread = 0;
    }

    delete [] handles;
#else*/
    for (uint i = 0; i < count; i++) {
        threads[i].wait();
    }
//#endif
}

