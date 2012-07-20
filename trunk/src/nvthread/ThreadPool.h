// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_THREAD_THREADPOOL_H
#define NV_THREAD_THREADPOOL_H

#include "nvthread.h"

#include "Event.h"
#include "Thread.h"

// The thread pool creates one worker thread for each physical core. 
// The threads are idle waiting for their start events so that they do not consume any resources while inactive. 
// The thread pool runs the same function in all worker threads, the idea is to use this as the foundation of a custom task scheduler.
// When the thread pool starts, the main thread continues running, but the common use case is to inmmediately wait of the termination events of the worker threads.
// @@ The start and wait methods could probably be merged.

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
