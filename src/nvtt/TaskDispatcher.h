
#include "nvtt.h"

// OpenMP
// http://en.wikipedia.org/wiki/OpenMP
#if defined(HAVE_OPENMP)
#include <omp.h>
#endif

// Gran Central Dispatch (GCD/libdispatch)
// http://developer.apple.com/mac/library/documentation/Performance/Reference/GCD_libdispatch_Ref/Reference/reference.html
#if NV_OS_DARWIN && defined(HAVE_DISPATCH_H)
#include <dispatch/dispatch.h>
#endif

#if NV_OS_WIN32 && _MSC_VER >= 1600
#define HAVE_PPL 1
#endif

// Parallel Patterns Library (PPL) is part of Microsoft's concurrency runtime: 
// http://msdn.microsoft.com/en-us/library/dd504870.aspx
#if defined(HAVE_PPL)
#include <array>
//#include <ppl.h>
#endif

// Intel Thread Building Blocks (TBB).
// http://www.threadingbuildingblocks.org/
#if defined(HAVE_TBB)
#include <tbb/parallel_for.h>
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

#if defined(HAVE_OPENMP)

    struct OpenMPTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, size_t count) {
            #pragma omp parallel for
            for (int i = 0; i < int(count); i++) {
                task(context, i);
            }
        }
    };

#endif

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

#if defined(HAVE_PPL)

    class CountingIterator
    {
    public:
        CountingIterator() : i(0) {}
        CountingIterator(const CountingIterator & rhs) : i(0) {}
        explicit CountingIterator(int x) : i(x) {}

        //const int & base() const;
        const int & operator*() const { return i; }
        CountingIterator & operator++() { i++; return *this; }
        CountingIterator & operator--() { i--; return *this; }

    private:
        int i;
    };

    struct TaskFunctor {
        TaskFunctor(Task * task, void * context) : task(task), context(context) {}
        void operator()(int & n) const {
            task(context, n);
        }
        Task * task;
        void * context;
    };

    // Using Microsoft's concurrency runtime.
    struct MicrosoftTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, size_t count)
        {
            CountingIterator begin(0);
            CountingIterator end((int)count);
            TaskFunctor func(task, context);

            std::for_each(begin, end, func);
            //std::parallel_for_each(begin, end, func);
        }
    };

#endif

#if defined(HAVE_TBB)

    struct TaskFunctor {
        TaskFunctor(Task * task, void * context) : task(task), context(context) {}
        void operator()(int & n) const {
            task(context, n);
        }
        Task * task;
        void * context;
    };

    struct IntelTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, size_t count) {
            parallel_for(blocked_range<size_t>(0, count, 1), TaskFunctor(task, context));
        }
    };

#endif

} // namespace nvtt
