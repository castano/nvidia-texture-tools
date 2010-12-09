
#include "nvtt.h"

// OpenMP
#if defined(HAVE_OPENMP)
#include <omp.h>
#endif

#if NV_OS_DARWIN && defined(HAVE_DISPATCH_H)
#include <dispatch/dispatch.h>
#endif

#if NV_OS_WIN32 && _MSC_VER >= 1600
#define HAVE_PPL 1
#endif

#if HAVE_PPL
#include <array>
//#include <ppl.h>
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
            n *= n;
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


} // namespace nvtt
