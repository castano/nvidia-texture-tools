// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_THREAD_PARALLELFOR_H
#define NV_THREAD_PARALLELFOR_H

#include "nvthread.h"
//#include "Atomic.h" // atomic<uint>

namespace nv
{
    class Thread;
    class ThreadPool;

    typedef void ForTask(void * context, int id);

    struct ParallelFor {
        ParallelFor(ForTask * task, void * context);
        ~ParallelFor();

        void run(uint count);

        // Invariant:
        ForTask * task;
        void * context;
        ThreadPool * pool;
        //uint workerCount;   // @@ Move to thread pool.
        //Thread * workers;

        // State:
        uint count;
        /*atomic<uint>*/ uint idx;
    };

} // nv namespace


#endif // NV_THREAD_PARALLELFOR_H
