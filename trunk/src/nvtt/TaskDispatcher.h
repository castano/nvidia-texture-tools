
#include "nvtt.h"

// OpenMP
#if defined(HAVE_OPENMP)
#include <omp.h>
#endif

#if NV_OS_DARWIN && defined(HAVE_DISPATCH_H)
#include <dispatch/dispatch.h>
#endif

namespace nvtt {

    struct SequentialTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, size_t count) {
            for (size_t i = 0; i < count; i++) {
                task(context, i);
            }
        }
    };

#if NV_OS_DARWIN && defined(HAVE_DISPATCH_H)

    // Task dispatcher using Apple's Grand Central Dispatch.
    struct AppleTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, size_t count) {
            dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);
            dispatch_apply_f(count, q, context, task);
        }
    };

#endif

#if defined(HAVE_OPENMP)

    struct OpenMPTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, size_t count) {
            #pragma omp parallel
            {
                #pragma omp for
                for (size_t i = 0; i < count; i++) {
                    task(context, i);
                }
            }
        }
    };

#endif


} // namespace nvtt
