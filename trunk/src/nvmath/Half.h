#pragma once
#ifndef NV_MATH_HALF_H
#define NV_MATH_HALF_H

#include "nvmath.h"

namespace nv {

    uint32 half_to_float( uint16 h );
    uint16 half_from_float( uint32 f );

    // vin,vout must be 16 byte aligned. count must be a multiple of 8.
    void half_to_float_array(const uint16 * vin, float * vout, int count);

    void half_init_tables();

    extern uint32 mantissa_table[2048];
    extern uint32 exponent_table[64];
    extern uint32 offset_table[64];

    // Fast half to float conversion based on:
    // http://www.fox-toolkit.org/ftp/fasthalffloatconversion.pdf
    inline uint32 fast_half_to_float(uint16 h)
    {
        nvDebugCheck(mantissa_table[0] == 0); // Make sure table was initialized.
	    uint exp = h >> 10;
	    return mantissa_table[offset_table[exp] + (h & 0x3ff)] + exponent_table[exp];
    }


    inline uint16 to_half(float c) {
        union { float f; uint32 u; } f;
        f.f = c;
        return nv::half_from_float( f.u );
    }

    inline float to_float(uint16 c) {
        union { float f; uint32 u; } f;
        f.u = nv::fast_half_to_float( c );
        return f.f;
    }

} // nv namespace

#endif // NV_MATH_HALF_H
