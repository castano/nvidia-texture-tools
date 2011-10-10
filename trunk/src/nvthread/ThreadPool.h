// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_THREAD_THREADPOOL_H
#define NV_THREAD_THREADPOOL_H

#include "nvthread.h"

#include "Event.h"
#include "Thread.h"

namespace nv {

    class Thread;
    class Event;

    class ThreadPool {
        NV_FORBID_COPY(ThreadPool);
    public:

        static ThreadPool * acquire();
        static void release(ThreadPool *);

        ThreadPool();
        ~ThreadPool();

        void start(ThreadFunc * func, void * arg);
        void wait();

    private:

        static void workerFunc(void * arg);

        uint workerCount;
        Thread * workers;
        Event * startEvents;
        Event * finishEvents;

        uint allIdle;

        // Current function:
        ThreadFunc * func;
        void * arg;
    };

} // namespace nv


#endif // NV_THREAD_THREADPOOL_H
