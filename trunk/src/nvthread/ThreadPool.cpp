// This code is in the public domain -- castano@gmail.com

#include "ThreadPool.h"
#include "Mutex.h"
#include "Thread.h"
#include "Atomic.h"

#include "nvcore/Utils.h"

// Most of the time it's not necessary to protect the thread pool, but if it doesn't add a significant overhead, then it'd be safer to do it.
#define PROTECT_THREAD_POOL 1


using namespace nv;

#if PROTECT_THREAD_POOL 
Mutex s_pool_mutex;
#endif

AutoPtr<ThreadPool> s_pool;


/*static*/ ThreadPool * ThreadPool::acquire()
{
#if PROTECT_THREAD_POOL 
    s_pool_mutex.lock();    // @@ If same thread tries to lock twice, this should assert.
#endif

    if (s_pool == NULL) {
        ThreadPool * p = new ThreadPool;
        nvDebugCheck(s_pool == p);
    }

    return s_pool.ptr();
}

/*static*/ void ThreadPool::release(ThreadPool * pool)
{
    nvDebugCheck(pool == s_pool);

    // Make sure the threads of the pool are idle.
    s_pool->wait();

#if PROTECT_THREAD_POOL 
    s_pool_mutex.unlock();
#endif
}




/*static*/ void ThreadPool::workerFunc(void * arg) {
    uint i = U32((uintptr_t)arg); // This is OK, because workerCount should always be much smaller than 2^32

    while(true) 
    {
        s_pool->startEvents[i].wait();

        nv::ThreadFunc * func = loadAcquirePointer(&s_pool->func);

        if (func == NULL) {
            return;
        }
        
        func(s_pool->arg);

        s_pool->finishEvents[i].post();
    }
}


ThreadPool::ThreadPool() 
{
    s_pool = this;  // Worker threads need this to be initialized before they start.

    workerCount = nv::hardwareThreadCount();
    workers = new Thread[workerCount];

    startEvents = new Event[workerCount];
    finishEvents = new Event[workerCount];

    nvCompilerWriteBarrier(); // @@ Use a memory fence?

    for (uint i = 0; i < workerCount; i++) {
        workers[i].start(workerFunc, (void *)i);
    }

    allIdle = true;
}

ThreadPool::~ThreadPool()
{
    // Set threads to terminate.
    start(NULL, NULL);

    // Wait until threads actually exit.
    Thread::wait(workers, workerCount);

    delete [] workers;
    delete [] startEvents;
    delete [] finishEvents;
}

void ThreadPool::start(ThreadFunc * func, void * arg)
{
    // Wait until threads are idle.
    wait();

    // Set our desired function.
    storeReleasePointer(&this->func, func);
    storeReleasePointer(&this->arg, arg);

    allIdle = false;

    // Resume threads.
    Event::post(startEvents, workerCount);
}

void ThreadPool::wait()
{
    if (!allIdle)
    {
        // Wait for threads to complete.
        Event::wait(finishEvents, workerCount);

        allIdle = true;
    }
}
