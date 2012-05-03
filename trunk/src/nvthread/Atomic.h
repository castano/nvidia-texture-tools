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

#if NV_CC_CLANG && POSH_CPU_STRONGARM
// LLVM/Clang do not yet have functioning atomics as of 2.1
// #include <atomic>

#endif


namespace nv {

    // Load and stores.
    inline uint32 loadRelaxed(const uint32 * ptr) { return *ptr; }
    inline void storeRelaxed(uint32 * ptr, uint32 value) { *ptr = value; }

    inline uint32 loadAcquire(const volatile uint32 * ptr)
    {
        nvDebugCheck((intptr_t(ptr) & 3) == 0);

#if POSH_CPU_X86 || POSH_CPU_X86_64
        uint32 ret = *ptr;  // on x86, loads are Acquire
        nvCompilerReadBarrier();
        return ret;
#elif POSH_CPU_STRONGARM 
        // need more specific cpu type for armv7?
        // also utilizes a full barrier
        // currently treating laod like x86 - this could be wrong
        
        // this is the easiest but slowest way to do this
        nvCompilerReadWriteBarrier();
		uint32 ret = *ptr; // replace with ldrex?
        nvCompilerReadWriteBarrier();
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
        *ptr = value;   // on x86, stores are Release
        nvCompilerWriteBarrier();
#elif POSH_CPU_STRONGARM
        // this is the easiest but slowest way to do this
        nvCompilerReadWriteBarrier();
		*ptr = value; //strex?
		nvCompilerReadWriteBarrier();
#else
#error "Atomics not implemented."
#endif
    }


	template <typename T>
	inline void storeReleasePointer(volatile T * pTo, T from)
	{
        NV_COMPILER_CHECK(sizeof(T) == sizeof(intptr_t));
		nvDebugCheck((((intptr_t)pTo) % sizeof(intptr_t)) == 0);
		nvDebugCheck((((intptr_t)&from) % sizeof(intptr_t)) == 0);
		nvCompilerWriteBarrier();
		*pTo = from;    // on x86, stores are Release
	}
	
	template <typename T>
	inline T loadAcquirePointer(volatile T * ptr)
	{
        NV_COMPILER_CHECK(sizeof(T) == sizeof(intptr_t));
		nvDebugCheck((((intptr_t)ptr) % sizeof(intptr_t)) == 0);
		T ret = *ptr;   // on x86, loads are Acquire
		nvCompilerReadBarrier();
		return ret;
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
    
#elif NV_CC_CLANG && POSH_CPU_STRONGARM
    NV_COMPILER_CHECK(sizeof(uint32) == sizeof(long));
    
    inline uint32 atomicIncrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        
        // this should work in LLVM eventually, but not as of 2.1
        // return (uint32)AtomicIncrement((long *)value);
        
        // in the mean time,
        register uint32 result;
        asm volatile (
                      "1:   ldrexb  %0,  [%1]	\n\t"
                      "add     %0,   %0, #1     \n\t"
                      "strexb  r1,   %0, [%1]	\n\t"
                      "cmp     r1,   #0			\n\t"
                      "bne     1b"
                      : "=&r" (result)
                      : "r"(value)
                      : "r1"
                      );
        return result;

    }
    
    inline uint32 atomicDecrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        
        // this should work in LLVM eventually, but not as of 2.1:
        // return (uint32)sys::AtomicDecrement((long *)value);

        // in the mean time,
        
        register uint32 result;
        asm volatile (
                      "1:   ldrexb  %0,  [%1]	\n\t"
                      "sub     %0,   %0, #1     \n\t"
                      "strexb  r1,   %0, [%1]	\n\t"
                      "cmp     r1,   #0			\n\t"
                      "bne     1b"
                      : "=&r" (result)
                      : "r"(value)
                      : "r1"
                      );
        return result;
         
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
