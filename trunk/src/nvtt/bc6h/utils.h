/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

// utility class holding common routines
#pragma once
#ifndef _UTILS_H
#define _UTILS_H

#include "nvmath/Vector.h"


#define	PALETTE_LERP(a, b, i, denom)	Utils::lerp(a, b, i, denom)

#define	SIGN_EXTEND(x,nb)	((((signed(x))&(1<<((nb)-1)))?((~0)<<(nb)):0)|(signed(x)))

enum Field {
    FIELD_M = 1,	// mode
    FIELD_D = 2,	// distribution/shape
    FIELD_RW = 10+0, FIELD_RX = 10+1, FIELD_RY = 10+2, FIELD_RZ = 10+3,	// red channel endpoints or deltas
    FIELD_GW = 20+0, FIELD_GX = 20+1, FIELD_GY = 20+2, FIELD_GZ = 20+3,	// green channel endpoints or deltas
    FIELD_BW = 30+0, FIELD_BX = 30+1, FIELD_BY = 30+2, FIELD_BZ = 30+3,	// blue channel endpoints or deltas
};

// some constants
#define	F16S_MASK	0x8000		// f16 sign mask
#define	F16EM_MASK	0x7fff		// f16 exp & mantissa mask
#define	U16MAX		0xffff
#define	S16MIN		(-0x8000)
#define	S16MAX		0x7fff
#define	INT16_MASK	0xffff
#define	F16MAX	(0x7bff)		// MAXFLT bit pattern for halfs

enum Format { UNSIGNED_F16, SIGNED_F16 };

class Utils
{
public:
    static ::Format FORMAT;	// this is a global -- we're either handling unsigned or unsigned half values

    // error metrics
    static double norm(const nv::Vector3 &a, const nv::Vector3 &b);
    static double mpsnr_norm(const nv::Vector3 &a, int exposure, const nv::Vector3 &b);

    // conversion & clamp
    static int ushort_to_format(unsigned short input);
    static unsigned short format_to_ushort(int input);

    // clamp to format
    static void clamp(nv::Vector3 &v);

    // quantization and unquantization
    static int finish_unquantize(int q, int prec);
    static int unquantize(int q, int prec);
    static int quantize(float value, int prec);

    static void parse(const char *encoding, int &ptr, Field & field, int &endbit, int &len);

    // lerping
    static int lerp(int a, int b, int i, int denom);
    static nv::Vector3 lerp(const nv::Vector3 & a, const nv::Vector3 & b, int i, int denom);
};

#endif // _UTILS_H
