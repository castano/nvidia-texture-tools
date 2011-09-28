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


/*static*/ void Event::post(Event * events, uint count) {
    for (uint i = 0; i < count; i++) {
        events[i].post();
    }
}

/*static*/ void Event::wait(Event * events, uint count) {
    // @@ Use wait for multiple objects?

    for (uint i = 0; i < count; i++) {
        events[i].wait();
    }
}

#elif NV_OS_UNIX
// @@ TODO
#pragma NV_MESSAGE("Implement event using pthreads!")
#endif	
