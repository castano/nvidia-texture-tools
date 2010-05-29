/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

// utility class holding common routines
#ifndef _UTILS_H
#define _UTILS_H

#include "arvo/Vec4.h"

using namespace ArvoMath;

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif

#define	PALETTE_LERP(a, b, i, bias, denom)	Utils::lerp(a, b, i, bias, denom)

#define	SIGN_EXTEND(x,nb)	((((x)&(1<<((nb)-1)))?((~0)<<(nb)):0)|(x))

#define	INDEXMODE_BITS 1		// 2 different index modes
#define	NINDEXMODES	(1<<(INDEXMODE_BITS))
#define	INDEXMODE_ALPHA_IS_3BITS 0
#define	INDEXMODE_ALPHA_IS_2BITS 1

#define	ROTATEMODE_BITS	2		// 4 different rotate modes
#define	NROTATEMODES	(1<<(ROTATEMODE_BITS))
#define	ROTATEMODE_RGBA_RGBA	0
#define	ROTATEMODE_RGBA_AGBR	1
#define	ROTATEMODE_RGBA_RABG	2
#define	ROTATEMODE_RGBA_RGAB	3

class Utils
{
public:
	// error metrics
	static double metric4(const Vec4& a, const Vec4& b);
	static double metric3(const Vec3& a, const Vec3& b, int rotatemode);
	static double metric1(float a, float b, int rotatemode);

	static double metric4premult(const Vec4& rgba0, const Vec4& rgba1);
	static double metric3premult_alphaout(const Vec3& rgb0, float a0, const Vec3& rgb1, float a1);
	static double metric3premult_alphain(const Vec3& rgb0, const Vec3& rgb1, int rotatemode);
	static double metric1premult(float rgb0, float a0, float rgb1, float a1, int rotatemode);

	static float  Utils::premult(float r, float a);

	// quantization and unquantization
	static int unquantize(int q, int prec);
	static int quantize(float value, int prec);

	// lerping
	static int lerp(int a, int b, int i, int bias, int denom);
	static Vec4 lerp(const Vec4& a, const Vec4 &b, int i, int bias, int denom);
};

#endif