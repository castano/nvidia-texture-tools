#pragma once
#ifndef NV_MATH_HALF_H
#define NV_MATH_HALF_H

#include "nvmath.h"

namespace nv {

    uint32 half_to_float( uint16 h );
    uint16 half_from_float( uint32 f );

    // Does not handle NaN or infinity.
    uint32 fast_half_to_float( uint16 h );

} // nv namespace

#endif // NV_MATH_HALF_H
