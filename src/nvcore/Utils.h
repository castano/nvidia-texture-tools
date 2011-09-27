// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_CORE_UTILS_H
#define NV_CORE_UTILS_H

#include "nvcore.h"
#include "Debug.h" // nvDebugCheck

// Just in case. Grrr.
#undef min
#undef max

namespace nv
{
    // Less error prone than casting. From CB:
    // http://cbloomrants.blogspot.com/2011/06/06-17-11-c-casting-is-devil.html
    inline int8  asSigned(uint8 x)  { return (int8) x; }
    inline int16 asSigned(uint16 x) { return (int16) x; }
    inline int32 asSigned(uint32 x) { return (int32) x; }
    inline int64 asSigned(uint64 x) { return (int64) x; }

    inline uint8  asUnsigned(int8 x)  { return (uint8) x; }
    inline uint16 asUnsigned(int16 x) { return (uint16) x; }
    inline uint32 asUnsigned(int32 x) { return (uint32) x; }
    inline uint64 asUnsigned(int64 x) { return (uint64) x; }

    template <typename T> inline uint32 toU32(T x) {
        nvDebugCheck(x <= UINT32_MAX);
        nvDebugCheck(x >= 0);
        return (uint32) x;
    }

    /*
    template <typename T> inline int8 toI8(T x) { 
        nvDebugCheck(x <= INT8_MAX);
        nvDebugCheck(x >= INT8_MIN);
        int8 y = (int8) x;
        nvDebugCheck(x == (T)y);
        return y;
    }
    
    template <typename T> inline uint8 toU8(T x) { 
        nvDebugCheck(x <= UINT8_MAX);
        nvDebugCheck(x >= 0);
        return (uint8) x;
    }

    template <typename T> inline int16 toI16(T x) { 
        nvDebugCheck(x <= INT16_MAX);
        nvDebugCheck(x >= INT16_MIN);
        return (int16) x;
    }
    
    template <typename T> inline uint16 toU16(T x) { 
        nvDebugCheck(x <= UINT16_MAX);
        nvDebugCheck(x >= 0);
        return (uint16) x;
    }
    
    template <typename T> inline int32 toI32(T x) { 
        nvDebugCheck(x <= INT32_MAX);
        nvDebugCheck(x >= INT32_MIN);
        return (int32) x;
    }

    template <typename T> inline uint32 toU32(T x) { 
        nvDebugCheck(x <= UINT32_MAX);
        nvDebugCheck(x >= 0);
        return (uint32) x;
    }
    
    template <typename T> inline int64 toI64(T x) { 
        nvDebugCheck(x <= INT64_MAX);
        nvDebugCheck(x >= INT64_MIN);
        return (int64) x;
    }
    
    template <typename T> inline uint64 toU64(T x) { 
        nvDebugCheck(x <= UINT64_MAX);
        nvDebugCheck(x >= 0);
        return (uint64) x;
    }
    */
    
    /// Swap two values.
    template <typename T> 
    inline void swap(T & a, T & b)
    {
        T temp = a; 
        a = b; 
        b = temp;
    }

    /// Return the maximum of the two arguments.
    template <typename T> 
    inline const T & max(const T & a, const T & b)
    {
        //return std::max(a, b);
        if( a < b ) {
            return b; 
        }
        return a;
    }

    /// Return the maximum of the three arguments.
    template <typename T> 
    inline const T & max(const T & a, const T & b, const T & c)
    {
        return max(a, max(b, c));
    }

    /// Return the minimum of two values.
    template <typename T> 
    inline const T & min(const T & a, const T & b)
    {
        //return std::min(a, b);
        if( b < a ) {
            return b; 
        }
        return a;
    }

    /// Return the maximum of the three arguments.
    template <typename T> 
    inline const T & min(const T & a, const T & b, const T & c)
    {
        return min(a, min(b, c));
    }

    /// Clamp between two values.
    template <typename T> 
    inline const T & clamp(const T & x, const T & a, const T & b)
    {
        return min(max(x, a), b);
    }

    /** Return the next power of two. 
    * @see http://graphics.stanford.edu/~seander/bithacks.html
    * @warning Behaviour for 0 is undefined.
    * @note isPowerOfTwo(x) == true -> nextPowerOfTwo(x) == x
    * @note nextPowerOfTwo(x) = 2 << log2(x-1)
    */
    inline uint nextPowerOfTwo( uint x )
    {
        nvDebugCheck( x != 0 );
#if 1	// On modern CPUs this is supposed to be as fast as using the bsr instruction.
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x+1;	
#else
        uint p = 1;
        while( x > p ) {
            p += p;
        }
        return p;
#endif
    }

    /// Return true if @a n is a power of two.
    inline bool isPowerOfTwo( uint n )
    {
        return (n & (n-1)) == 0;
    }


    inline uint sdbmHash(const void * data_in, uint size, uint h = 5381)
    {
        const uint8 * data = (const uint8 *) data_in;
        uint i = 0;
        while (i < size) {
            h = (h << 16) + (h << 6) - h + (uint) data[i++];
        }
        return h;
    }

    // Note that this hash does not handle NaN properly.
    inline uint sdbmFloatHash(const float * f, uint count, uint h = 5381)
    {
        for (uint i = 0; i < count; i++) {
            //nvDebugCheck(nv::isFinite(*f));
            union { float f; uint32 i; } x = { *f };
            if (x.i == 0x80000000) x.i = 0;
            h = sdbmHash(&x, 4, h);
        }
        return h;
    }


    // Some hash functors:
    template <typename Key> struct Hash 
    {
        uint operator()(const Key & k) const {
            return sdbmHash(&k, sizeof(Key));
        }
    };
    template <> struct Hash<int>
    {
        uint operator()(int x) const { return x; }
    };
    template <> struct Hash<uint>
    {
        uint operator()(uint x) const { return x; }
    };
    template <> struct Hash<float>
    {
        uint operator()(float f) const {
            return sdbmFloatHash(&f, 1);
        }
    };

    template <typename Key> struct Equal
    {
        bool operator()(const Key & k0, const Key & k1) const {
            return k0 == k1;
        }
    };


} // nv namespace

#endif // NV_CORE_UTILS_H
