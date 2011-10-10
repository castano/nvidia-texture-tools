// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_CORE_UTILS_H
#define NV_CORE_UTILS_H

#include "nvcore.h"
#include "Debug.h" // nvDebugCheck

// Just in case. Grrr.
#undef min
#undef max

#define NV_INT8_MIN    (-128)
#define NV_INT8_MAX    127
#define NV_INT16_MIN    (-32767-1)
#define NV_INT16_MAX    32767
#define NV_UINT16_MAX   0xffff
#define NV_INT32_MIN    (-2147483647-1)
#define NV_INT32_MAX    2147483647
#define NV_UINT32_MAX   0xffffffff
#define NV_INT64_MAX    POSH_I64(9223372036854775807)
#define NV_INT64_MIN    (-POSH_I64(9223372036854775808))
#define NV_UINT64_MAX   POSH_U64(0xffffffffffffffff)

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


    // uint32 casts:
    template <typename T> inline uint32 toU32(T x) { return x; }
    template <> inline uint32 toU32<uint64>(uint64 x) { nvDebugCheck(x <= NV_UINT32_MAX); return (uint32)x; }
    template <> inline uint32 toU32<int64>(int64 x) { nvDebugCheck(x >= 0 && x <= NV_UINT32_MAX); return (uint32)x; }
    //template <> inline uint32 toU32<uint32>(uint32 x) { return x; }
    template <> inline uint32 toU32<int32>(int32 x) { nvDebugCheck(x >= 0); return (uint32)x; }
    //template <> inline uint32 toU32<uint16>(uint16 x) { return x; }
    template <> inline uint32 toU32<int16>(int16 x) { nvDebugCheck(x >= 0); return (uint32)x; }
    //template <> inline uint32 toU32<uint8>(uint8 x) { return x; }
    template <> inline uint32 toU32<int8>(int8 x) { nvDebugCheck(x >= 0); return (uint32)x; }

    // int32 casts:
    template <typename T> inline int32 toI32(T x) { return x; }
    template <> inline int32 toI32<uint64>(uint64 x) { nvDebugCheck(x <= NV_INT32_MAX); return (int32)x; }
    template <> inline int32 toI32<int64>(int64 x) { nvDebugCheck(x >= NV_INT32_MIN && x <= NV_UINT32_MAX); return (int32)x; }
    template <> inline int32 toI32<uint32>(uint32 x) { nvDebugCheck(x <= NV_INT32_MAX); return (int32)x; }
    //template <> inline int32 toI32<int32>(int32 x) { return x; }
    //template <> inline int32 toI32<uint16>(uint16 x) { return x; }
    //template <> inline int32 toI32<int16>(int16 x) { return x; }
    //template <> inline int32 toI32<uint8>(uint8 x) { return x; }
    //template <> inline int32 toI32<int8>(int8 x) { return x; }

    
    /// Swap two values.
    template <typename T> 
    inline void swap(T & a, T & b)
    {
        T temp = a; 
        a = b; 
        b = temp;
    }

    /// Return the maximum of the two arguments. For floating point values, it returns the second value if the first is NaN.
    template <typename T> 
    inline const T & max(const T & a, const T & b)
    {
        return (b < a) ? a : b;
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
        return (a < b) ? a : b;
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
