// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_CORE_UTILS_H
#define NV_CORE_UTILS_H

#include "nvcore.h"
#include "Debug.h" // nvDebugCheck

namespace nv
{

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

    template <typename Key> struct Equal
    {
        bool operator()(const Key & k0, const Key & k1) const {
            return k0 == k1;
        }
    };


} // nv namespace

#endif // NV_CORE_UTILS_H
