// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include <nvmath/Color.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/BlockDXT.h>

#include "FastCompressDXT.h"

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

#if defined(__MMX__)
#include <mmintrin.h>
#endif

#undef __VEC__
#if defined(__VEC__)
#include <altivec.h>
#undef bool
#endif
// Online Resources:
// - http://www.jasondorie.com/ImageLib.zip
// - http://homepage.hispeed.ch/rscheidegger/dri_experimental/s3tc_index.html
// - http://www.sjbrown.co.uk/?article=dxt

using namespace nv;


#if defined(__SSE2__) && 0

// @@ TODO

typedef __m128i VectorColor;

inline static __m128i loadColor(Color32 c)
{
	return ...;
}

inline static __m128i absoluteDifference(__m128i a, __m128i b)
{
	return ...;
}
	
inline uint colorDistance(__m128i a, __m128i b)
{
	return 0;
}

#elif defined(__MMX__) && 0

typedef __m64 VectorColor;

inline static __m64 loadColor(Color32 c)
{
	return _mm_unpacklo_pi8(_mm_cvtsi32_si64(c), _mm_setzero_si64());
}

inline static __m64 absoluteDifference(__m64 a, __m64 b)
{
	// = |a-b| or |b-a|
	return _mm_or_si64(_mm_subs_pu16(a, b), _mm_subs_pu16(b, a));
}
	
inline uint colorDistance(__m64 a, __m64 b)
{
	union {
		__m64 v;
		uint16 part[4];
	} s;
	
	s.v = absoluteDifference(a, b);
		
	// @@ This is very slow!
	return s.part[0] + s.part[1] + s.part[2] + s.part[3];
}

#define vectorEnd	_mm_empty

#elif defined(__VEC__)

typedef vector signed int VectorColor;

inline static vector signed int loadColor(Color32 c)
{
	return (vector signed int) (c.r, c.g, c.b, c.a);
}

// Get the absolute distance between the given colors.
inline static uint colorDistance(vector signed int c0, vector signed int c1)
{
	int result;
	vector signed int v = vec_sums(vec_abs(vec_sub(c0, c1)), (vector signed int)0);
	vec_ste(vec_splat(v, 3), 0, &result);
	return result;
}

inline void vectorEnd()
{
}

#else

typedef Color32 VectorColor;

inline static Color32 loadColor(Color32 c)
{
	return c;
}

inline static Color32 premultiplyAlpha(Color32 c)
{
	Color32 pm;
	pm.r = (c.r * c.a) >> 8;
	pm.g = (c.g * c.a) >> 8;
	pm.b = (c.b * c.a) >> 8;
	pm.a = c.a;
	return pm;
}

inline static uint sqr(uint s)
{
	return s*s;
}

// Get the absolute distance between the given colors.
inline static uint colorDistance(Color32 c0, Color32 c1)
{
	return sqr(c0.r - c1.r) + sqr(c0.g - c1.g) + sqr(c0.b - c1.b);
	//return abs(c0.r - c1.r) + abs(c0.g - c1.g) + abs(c0.b - c1.b);
}

inline void vectorEnd()
{
}

#endif


inline static uint computeIndices(const ColorBlock & rgba, const Color32 palette[4])
{
	const VectorColor vcolor0 = loadColor(palette[0]);
	const VectorColor vcolor1 = loadColor(palette[1]);
	const VectorColor vcolor2 = loadColor(palette[2]);
	const VectorColor vcolor3 = loadColor(palette[3]);
	
	uint indices = 0;
	for(int i = 0; i < 16; i++) {
		const VectorColor vcolor = loadColor(rgba.color(i));
		
		uint d0 = colorDistance(vcolor0, vcolor);
		uint d1 = colorDistance(vcolor1, vcolor);
		uint d2 = colorDistance(vcolor2, vcolor);
		uint d3 = colorDistance(vcolor3, vcolor);
		
		uint b0 = d0 > d3;
		uint b1 = d1 > d2;
		uint b2 = d0 > d2;
		uint b3 = d1 > d3;
		uint b4 = d2 > d3;
		
		uint x0 = b1 & b2;
		uint x1 = b0 & b3;
		uint x2 = b0 & b4;
		
		indices |= (x2 | ((x0 | x1) << 1)) << (2 * i);
	}

	vectorEnd();
	return indices;
}


// Compressor that uses bounding box.
void nv::compressBlock_BoundsRange(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 c0, c1;
	rgba.boundsRange(&c1, &c0);
	
	block->col0 = toColor16(c0);
	block->col1 = toColor16(c1);
	
	nvDebugCheck(block->col0.u > block->col1.u);
	
	// Use 4 color mode only.
	//if (block->col0.u < block->col1.u) {
	//	swap(block->col0.u, block->col1.u);
	//}
	
	Color32 palette[4];
	block->evaluatePalette4(palette);
	
	block->indices = computeIndices(rgba, palette);
}








// Encode DXT3 block.
void nv::compressBlock_BoundsRange(const ColorBlock & rgba, BlockDXT3 * block)
{
	compressBlock_BoundsRange(rgba, &block->color);
	compressBlock(rgba, &block->alpha);
}

// Encode DXT3 alpha block.
void nv::compressBlock(const ColorBlock & rgba, AlphaBlockDXT3 * block)
{
	block->alpha0 = rgba.color(0).a >> 4;
	block->alpha1 = rgba.color(1).a >> 4;
	block->alpha2 = rgba.color(2).a >> 4;
	block->alpha3 = rgba.color(3).a >> 4;
	block->alpha4 = rgba.color(4).a >> 4;
	block->alpha5 = rgba.color(5).a >> 4;
	block->alpha6 = rgba.color(6).a >> 4;
	block->alpha7 = rgba.color(7).a >> 4;
	block->alpha8 = rgba.color(8).a >> 4;
	block->alpha9 = rgba.color(9).a >> 4;
	block->alphaA = rgba.color(10).a >> 4;
	block->alphaB = rgba.color(11).a >> 4;
	block->alphaC = rgba.color(12).a >> 4;
	block->alphaD = rgba.color(13).a >> 4;
	block->alphaE = rgba.color(14).a >> 4;
	block->alphaF = rgba.color(15).a >> 4;
}



static uint computeAlphaIndices(const ColorBlock & rgba, AlphaBlockDXT5 * block)
{
	uint8 alphas[8];
	block->evaluatePalette(alphas);

	uint totalError = 0;

	for (uint i = 0; i < 16; i++)
	{
		uint8 alpha = rgba.color(i).a;

		uint besterror = 256*256;
		uint best = 8;
		for(uint p = 0; p < 8; p++)
		{
			int d = alphas[p] - alpha;
			uint error = d * d;

			if (error < besterror)
			{
				besterror = error;
				best = p;
			}
		}
		nvDebugCheck(best < 8);

		totalError += besterror;
		block->setIndex(i, best);
	}

	return totalError;
}

static uint computeAlphaError(const ColorBlock & rgba, const AlphaBlockDXT5 * block)
{
	uint8 alphas[8];
	block->evaluatePalette(alphas);

	uint totalError = 0;

	for (uint i = 0; i < 16; i++)
	{
		uint8 alpha = rgba.color(i).a;

		uint besterror = 256*256;
		uint best;
		for(uint p = 0; p < 8; p++)
		{
			int d = alphas[p] - alpha;
			uint error = d * d;

			if (error < besterror)
			{
				besterror = error;
				best = p;
			}
		}

		totalError += besterror;
	}

	return totalError;
}


void nv::compressBlock_BoundsRange(const ColorBlock & rgba, BlockDXT5 * block)
{
	Color32 c0, c1;
	rgba.boundsRangeAlpha(&c1, &c0);
	
	block->color.col0 = toColor16(c0);
	block->color.col1 = toColor16(c1);
	
	nvDebugCheck(block->color.col0.u > block->color.col1.u);
	
	Color32 palette[4];
	block->color.evaluatePalette4(palette);
	
	block->color.indices = computeIndices(rgba, palette);
	
	nvDebugCheck(c0.a <= c1.a);
	
	block->alpha.alpha0 = c0.a;
	block->alpha.alpha1 = c1.a;
	
	computeAlphaIndices(rgba, &block->alpha);
}


uint nv::compressBlock_BoundsRange(const ColorBlock & rgba, AlphaBlockDXT5 * block)
{
	uint8 alpha0 = 0;
	uint8 alpha1 = 255;

	// Get min/max alpha.
	for (uint i = 0; i < 16; i++)
	{
		uint8 alpha = rgba.color(i).a;
		alpha0 = max(alpha0, alpha);
		alpha1 = min(alpha1, alpha);
	}

	alpha0 = alpha0 - (alpha0 - alpha1) / 32;
	alpha1 = alpha1 + (alpha0 - alpha1) / 32;

	AlphaBlockDXT5 block0;
	block0.alpha0 = alpha0;
	block0.alpha1 = alpha1;
	uint error0 = computeAlphaIndices(rgba, &block0);

	AlphaBlockDXT5 block1;
	block1.alpha0 = alpha1;
	block1.alpha1 = alpha0;
	uint error1 = computeAlphaIndices(rgba, &block1);

	if (error0 < error1)
	{
		*block = block0;
		return error0;
	}
	else
	{
		*block = block1;
		return error1;
	}
}

uint nv::compressBlock_BruteForce(const ColorBlock & rgba, AlphaBlockDXT5 * block)
{
	uint8 mina = 255;
	uint8 maxa = 0;

	// Get min/max alpha.
	for (uint i = 0; i < 16; i++)
	{
		uint8 alpha = rgba.color(i).a;
		mina = min(mina, alpha);
		maxa = max(maxa, alpha);
	}

	block->alpha0 = maxa;
	block->alpha1 = mina;

	/*int centroidDist = 256;
	int centroid;

	// Get the closest to the centroid.
	for (uint i = 0; i < 16; i++)
	{
		uint8 alpha = rgba.color(i).a;
		int dist = abs(alpha - (maxa + mina) / 2);
		if (dist < centroidDist)
		{
			centroidDist = dist;
			centroid = alpha;
		}
	}*/

	if (maxa - mina > 8)
	{
		int besterror = computeAlphaError(rgba, block);
		int besta0 = maxa;
		int besta1 = mina;

		for (int a0 = mina+9; a0 < maxa; a0++)
		{
			for (int a1 = mina; a1 < a0-8; a1++)
			//for (int a1 = mina; a1 < maxa; a1++)
			{
				//nvCheck(abs(a1-a0) > 8);

				//if (abs(a0 - a1) < 8) continue;
				//if ((maxa-a0) + (a1-mina) + min(abs(centroid-a0), abs(centroid-a1)) > besterror)
				if ((maxa-a0) + (a1-mina) > besterror)
					continue;

				block->alpha0 = a0;
				block->alpha1 = a1;
				int error = computeAlphaError(rgba, block);

				if (error < besterror)
				{
					besterror = error;
					besta0 = a0;
					besta1 = a1;
				}
			}
		}

		block->alpha0 = besta0;
		block->alpha1 = besta1;
	}

	return computeAlphaIndices(rgba, block);
}



