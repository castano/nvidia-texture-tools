#pragma once
#ifndef NV_MATH_HALF_H
#define NV_MATH_HALF_H

#include "nvmath.h"

namespace nv {

    uint32 half_to_float( uint16 h );
    uint16 half_from_float( uint32 f );

    void half_init_tables();

    uint32 fast_half_to_float(uint16 h);

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
