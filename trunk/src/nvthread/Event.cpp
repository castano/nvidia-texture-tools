// This code is in the public domain -- castano@gmail.com

#include "Event.h"

#if NV_OS_WIN32
#include "Win32.h"
#elif NV_OS_UNIX
#include <pthread.h>
#endif

using namespace nv;

#if NV_OS_WIN32

struct Event::Private {
    HANDLE handle;
};

Event::Event() : m(new Private) {
    m->handle = CreateEvent(NULL, FALSE, FALSE, NULL);
}

Event::~Event() {
    CloseHandle(m->handle);
}

void Event::post() {
    SetEvent(m->handle);
}

void Event::wait() {
    WaitForSingleObject(m->handle, INFINITE);
}

#elif NV_OS_UNIX

#pragma NV_MESSAGE("Implement event using pthreads!")

struct Event::Private {
    pthread_cond_t pt_cond;
    pthread_mutex_t pt_mutex;
};

Event::Event() : m(new Private) {
    // pthread equivalent of auto-reset event
    pthread_cond_init(&m->pt_cond, NULL);
    pthread_mutex_init(&m->pt_mutex, NULL);
}

Event::~Event() {
    pthread_cond_destroy(&m->pt_cond);
    pthread_mutex_destroy(&m->pt_mutex);
}

void Event::post() {
    pthread_cond_signal(&m->pt_cond);
}

void Event::wait() {
    pthread_cond_wait(&m->pt_cond, &m->pt_mutex);
}

#endif // NV_OS_UNIX


/*static*/ void Event::post(Event * events, uint count) {
    for (uint i = 0; i < count; i++) {
        events[i].post();
    }
}

/*static*/ void Event::wait(Event * events, uint count) {
    // @@ Use wait for multiple objects in win32?

    for (uint i = 0; i < count; i++) {
        events[i].wait();
    }
}
