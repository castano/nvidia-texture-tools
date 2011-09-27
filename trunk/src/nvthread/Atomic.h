// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_THREAD_ATOMIC_H
#define NV_THREAD_ATOMIC_H

#include "nvthread.h"

#include "nvcore/Debug.h"


#if NV_CC_MSVC

#include <intrin.h> // Already included by nvthread.h

#pragma intrinsic(_InterlockedIncrement, _InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange, _InterlockedExchange)

/*
extern "C"
{
    #pragma intrinsic(_InterlockedIncrement, _InterlockedDecrement)
    LONG  __cdecl _InterlockedIncrement(long volatile *Addend);
    LONG  __cdecl _InterlockedDecrement(long volatile *Addend);

    #pragma intrinsic(_InterlockedCompareExchange, _InterlockedExchange)
    LONG  __cdecl _InterlockedCompareExchange(long volatile * Destination, long Exchange, long Compared);
    LONG  __cdecl _InterlockedExchange(long volatile * Target, LONG Value);
}
*/

#endif // NV_CC_MSVC


namespace nv {

    // Load and stores.
    inline uint32 loadRelaxed(const uint32 * ptr) { return *ptr; }
    inline void storeRelaxed(uint32 * ptr, uint32 value) { *ptr = value; }

    inline uint32 loadAcquire(const volatile uint32 * ptr)
    {
        nvDebugCheck((intptr_t(ptr) & 3) == 0);

#if POSH_CPU_X86 || POSH_CPU_X86_64
        nvCompilerReadBarrier();
        uint32 ret = *ptr;  // on x86, loads are Acquire
        nvCompilerReadBarrier();
        return ret;
#else
#error "Not implemented"
#endif
    }

    inline void storeRelease(volatile uint32 * ptr, uint32 value)
    {
        nvDebugCheck((intptr_t(ptr) & 3) == 0);
        nvDebugCheck((intptr_t(&value) & 3) == 0);

#if POSH_CPU_X86 || POSH_CPU_X86_64
        nvCompilerWriteBarrier();
        *ptr = value;   // on x86, stores are Release
        nvCompilerWriteBarrier();
#else
#error "Atomics not implemented."
#endif
    }


    // Atomics. @@ Assuming sequential memory order?

#if NV_CC_MSVC
    NV_COMPILER_CHECK(sizeof(uint32) == sizeof(long));

    inline uint32 atomicIncrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);

        return (uint32)_InterlockedIncrement((long *)value);
    }

    inline uint32 atomicDecrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);

        return (uint32)_InterlockedDecrement((long *)value);
    }
#elif NV_CC_GNUC
    // Many alternative implementations at:
    // http://www.memoryhole.net/kyle/2007/05/atomic_incrementing.html

    inline uint32 atomicIncrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);

        return __sync_fetch_and_add(value, 1);
    }

    inline uint32 atomicDecrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);

        return __sync_fetch_and_sub(value, 1);
    }
#else
#error "Atomics not implemented."
#endif




    // It would be nice to have C++0x-style atomic types, but I'm not in the mood right now. Only uint32 supported so far.
#if 0
    template <typename T>
    void increment(T * value);

    template <typename T>
    void decrement(T * value);

    template <>
    void increment(uint32 * value) {
    }

    template <>
    void increment(uint64 * value) {
    }



    template <typename T>
    class Atomic
    {
    public:
        explicit Atomic()  : m_value() { }
        explicit Atomic( T val ) : m_value(val) { }
        ~Atomic() { }

        T loadRelaxed()  const { return m_value; }
        void storeRelaxed(T val) { m_value = val; }

        //T loadAcquire() const volatile { return nv::loadAcquire(&m_value); }
        //void storeRelease(T val) volatile { nv::storeRelease(&m_value, val); }

        void increment() /*volatile*/ { nv::atomicIncrement(m_value); }
        void decrement() /*volatile*/ { nv::atomicDecrement(m_value); }

        void compareAndStore(T oldVal, T newVal) { nv::atomicCompareAndStore(&m_value, oldVal, newVal); }
        T compareAndExchange(T oldVal, T newVal) { nv::atomicCompareAndStore(&m_value, oldVal, newVal); }
        T exchange(T newVal) { nv::atomicExchange(&m_value, newVal); }

    private:
        // don't provide operator = or == ; make the client write Store( Load() )
        NV_FORBID_COPY(Atomic);
		
	NV_COMPILER_CHECK(sizeof(T) == sizeof(uint32) || sizeof(T) == sizeof(uint64));

        T m_value;
    };
#endif

} // nv namespace 


#endif // NV_THREADS_ATOMICS_H
