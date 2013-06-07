// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "ParallelFor.h"
#include "Thread.h"
#include "Atomic.h"
#include "ThreadPool.h"

#include "nvcore/Utils.h" // toI32

using namespace nv;

#define ENABLE_PARALLEL_FOR 1

static void worker(void * arg) {
    ParallelFor * owner = (ParallelFor *)arg;

    while(true) {
        // Consume one element at a time. @@ Might be more efficient to have custom grain.
        uint i = atomicIncrement(&owner->idx);
        if (i > owner->count) {
            break;
        }

        owner->task(owner->context, i - 1);
    } 
}


ParallelFor::ParallelFor(ForTask * task, void * context) : task(task), context(context) {
#if ENABLE_PARALLEL_FOR
    pool = ThreadPool::acquire();
#endif
}

ParallelFor::~ParallelFor() {
#if ENABLE_PARALLEL_FOR
    ThreadPool::release(pool);
#endif
}

void ParallelFor::run(uint count) {
#if ENABLE_PARALLEL_FOR
    storeRelease(&this->count, count);

    // Init atomic counter to zero.
    storeRelease(&idx, 0);

    // Start threads.
    pool->start(worker, this);

    // Wait for all threads to complete.
    pool->wait();

    nvDebugCheck(idx >= count);
#else
    for (int i = 0; i < toI32(count); i++) {
        task(context, i);
    }
#endif
}


