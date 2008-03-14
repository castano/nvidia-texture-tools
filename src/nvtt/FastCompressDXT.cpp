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


inline static uint paletteError(const ColorBlock & rgba, Color32 palette[4])
{
	uint error = 0;
	
	const VectorColor vcolor0 = loadColor(palette[0]);
	const VectorColor vcolor1 = loadColor(palette[1]);
	const VectorColor vcolor2 = loadColor(palette[2]);
	const VectorColor vcolor3 = loadColor(palette[3]);

	for(uint i = 0; i < 16; i++) {
		const VectorColor vcolor = loadColor(rgba.color(i));
		
		uint d0 = colorDistance(vcolor, vcolor0);
		uint d1 = colorDistance(vcolor, vcolor1);
		uint d2 = colorDistance(vcolor, vcolor2);
		uint d3 = colorDistance(vcolor, vcolor3);
		
		error += min(min(d0, d1), min(d2, d3));
	}

	vectorEnd();
	return error;
}

	
	
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

inline static uint computeIndicesAlpha(const ColorBlock & rgba, const Color32 palette[4])
{
	const VectorColor vcolor0 = loadColor(palette[0]);
	const VectorColor vcolor1 = loadColor(palette[1]);
	const VectorColor vcolor2 = loadColor(palette[2]);
	const VectorColor vcolor3 = loadColor(palette[3]);
	
	uint indices = 0;
	for(int i = 0; i < 16; i++) {
		const VectorColor vcolor = premultiplyAlpha(loadColor(rgba.color(i)));
		
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


inline static Color16 saturate16(int r, int g, int b)
{
	Color16 c;
	c.r = clamp(0, 31, r);
	c.g = clamp(0, 63, g);
	c.b = clamp(0, 31, b);
	return c;
}


// Compressor that uses the luminance axis.
void nv::compressBlock_LuminanceAxis(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 c0, c1;
	rgba.luminanceRange(&c0, &c1);
	
	block->col0 = toColor16(c0);
	block->col1 = toColor16(c1);
	
	// Use 4 color mode only.
	if (block->col0.u < block->col1.u) {
		swap(block->col0.u, block->col1.u);
	}
	
	Color32 palette[4];
	block->evaluatePalette4(palette);
	
	block->indices = computeIndices(rgba, palette);
}


// Compressor that uses diameter axis.
void nv::compressBlock_DiameterAxis(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 c0, c1;
	rgba.diameterRange(&c0, &c1);
	
	block->col0 = toColor16(c0);
	block->col1 = toColor16(c1);
	
	// Use 4 color mode only.
	if (block->col0.u < block->col1.u) {
		swap(block->col0.u, block->col1.u);
	}
	
	Color32 palette[4];
	block->evaluatePalette4(palette);
	
	block->indices = computeIndices(rgba, palette);
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

// Compressor that uses bounding box and takes alpha into account.
void nv::compressBlock_BoundsRangeAlpha(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 c0, c1;
	rgba.boundsRange(&c1, &c0);
	
	if (rgba.hasAlpha())
	{
		block->col0 = toColor16(c1);
		block->col1 = toColor16(c0);
	}
	else
	{
		block->col0 = toColor16(c0);
		block->col1 = toColor16(c1);
	}
	
	Color32 palette[4];
	block->evaluatePalette(palette);
	
	block->indices = computeIndicesAlpha(rgba, palette);
}



// Compressor that tests all input color pairs.
void nv::compressBlock_TestAllPairs(const ColorBlock & rgba, BlockDXT1 * block)
{
	uint best_error = uint(-1);
	Color16 best_col0, best_col1;
	
	Color32 palette[4];
	
	// Test all color pairs.
	for(uint i = 0; i < 16; i++) {
		block->col0 = toColor16(rgba.color(i));
		
		for(uint ii = 0; ii < 16; ii++) {
			if( i != ii ) {
				block->col1 = toColor16(rgba.color(ii));
				block->evaluatePalette(palette);
				
				const uint error = paletteError(rgba, palette);
				if(error < best_error) {
					best_error = error;
					best_col0 = block->col0;
					best_col1 = block->col1;
				}
			}
		}
	}
	
	block->col0 = best_col0;
	block->col1 = best_col1;
	block->evaluatePalette(palette);
	
	block->indices = computeIndices(rgba, palette);
}


// Improve palette iteratively using alternate 3d search as suggested by Dave Moore.
void nv::refineSolution_3dSearch(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 palette[4];
	block->evaluatePalette(palette);
	
	uint best_error = paletteError(rgba, palette);
	Color16 best_col0 = block->col0;
	Color16 best_col1 = block->col1;
	
	const int W = 2;
	
	while(true) {
		bool changed = false;
		
		const int r0 = best_col0.r;
		const int g0 = best_col0.g;
		const int b0 = best_col0.b;
		
		for(int z = -W; z <= W; z++) {
			for(int y = -W; y <= W; y++) {
				for(int x = -W; x <= W; x++) {
					block->col0 = saturate16(r0 + x, g0 + y, b0 + z);
					block->evaluatePalette(palette);
					
					const uint error = paletteError(rgba, palette);
					if(error < best_error) {
						best_error = error;
						best_col0 = block->col0;
						best_col1 = block->col1;
						changed = true;
					}
				}
			}
		}
		
		const int r1 = best_col1.r;
		const int g1 = best_col1.g;
		const int b1 = best_col1.b;
		
		for(int z = -W; z <= W; z++) {
			for(int y = -W; y <= W; y++) {
				for(int x = -W; x <= W; x++) {
					block->col1 = saturate16(r1 + x, g1 + y, b1 + z);
					block->evaluatePalette(palette);
					
					const uint error = paletteError(rgba, palette);
					if(error < best_error) {
						best_error = error;
						best_col0 = block->col0;
						best_col1 = block->col1;
						changed = true;
					}
				}
			}
		}
		
		if( !changed ) {
			// Stop at local minima.
			break;
		}
	}
	
	block->col0 = best_col0;
	block->col1 = best_col1;
	block->evaluatePalette(palette);
	
	block->indices = computeIndices(rgba, palette);
}


// Improve the palette iteratively using 6d search as suggested by Charles Bloom.
void nv::refineSolution_6dSearch(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 palette[4];
	block->evaluatePalette(palette);
	
	uint best_error = paletteError(rgba, palette);
	Color16 best_col0 = block->col0;
	Color16 best_col1 = block->col1;
	
	const int W = 1;

	while(true) {
		bool changed = false;
		const int r0 = best_col0.r;
		const int g0 = best_col0.g;
		const int b0 = best_col0.b;
		const int r1 = best_col1.r;
		const int g1 = best_col1.g;
		const int b1 = best_col1.b;
		
		for(int z0 = -W; z0 <= W; z0++) {
			for(int y0 = -W; y0 <= W; y0++) {
				for(int x0 = -W; x0 <= W; x0++) {
					for(int z1 = -W; z1 <= W; z1++) {
						for(int y1 = -W; y1 <= W; y1++) {
							for(int x1 = -W; x1 <= W; x1++) {
								
								block->col0 = saturate16(r0 + x0, g0 + y0, b0 + z0);
								block->col1 = saturate16(r1 + x1, g1 + y1, b1 + z1);
								block->evaluatePalette(palette);
								
								const uint error = paletteError(rgba, palette);
								if(error < best_error) {
									best_error = error;
									best_col0 = block->col0;
									best_col1 = block->col1;
									changed = true;
								}
							}
						}
					}
				}
			}
		}
		
		if( !changed ) {
			// Stop at local minima.
			break;
		}
	}
	
	block->col0 = best_col0;
	block->col1 = best_col1;
	block->evaluatePalette(palette);
	
	block->indices = computeIndices(rgba, palette);
}



// Improve the palette iteratively using alternate 1d search as suggested by Walt Donovan.
void nv::refineSolution_1dSearch(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 palette[4];
	block->evaluatePalette(palette);
	
	uint best_error = paletteError(rgba, palette);
	Color16 best_col0 = block->col0;
	Color16 best_col1 = block->col1;
	
	const int W = 4;
	
	while(true) {
		bool changed = false;
		
		const int r0 = best_col0.r;
		const int g0 = best_col0.g;
		const int b0 = best_col0.b;
		
		for(int z = -W; z <= W; z++) {
			block->col0.b = clamp(b0 + z, 0, 31);
			block->evaluatePalette(palette);
					
			const uint error = paletteError(rgba, palette);
			if(error < best_error) {
				best_error = error;
				best_col0 = block->col0;
				best_col1 = block->col1;
				changed = true;
			}
		}
		
		for(int y = -W; y <= W; y++) {
			block->col0.g = clamp(g0 + y, 0, 63);
			block->evaluatePalette(palette);
				
			const uint error = paletteError(rgba, palette);
			if(error < best_error) {
				best_error = error;
				best_col0 = block->col0;
				best_col1 = block->col1;
				changed = true;
			}
		}
		
		for(int x = -W; x <= W; x++) {
			block->col0.r = clamp(r0 + x, 0, 31);
			block->evaluatePalette(palette);
			
			const uint error = paletteError(rgba, palette);
			if(error < best_error) {
				best_error = error;
				best_col0 = block->col0;
				best_col1 = block->col1;
				changed = true;
			}
		}
		
		
		const int r1 = best_col1.r;
		const int g1 = best_col1.g;
		const int b1 = best_col1.b;
		
		for(int z = -W; z <= W; z++) {
			block->col1.b = clamp(b1 + z, 0, 31);
			block->evaluatePalette(palette);
			
			const uint error = paletteError(rgba, palette);
			if(error < best_error) {
				best_error = error;
				best_col0 = block->col0;
				best_col1 = block->col1;
				changed = true;
			}
		}
		
		for(int y = -W; y <= W; y++) {
			block->col1.g = clamp(g1 + y, 0, 63);
			block->evaluatePalette(palette);
			
			const uint error = paletteError(rgba, palette);
			if(error < best_error) {
				best_error = error;
				best_col0 = block->col0;
				best_col1 = block->col1;
				changed = true;
			}
		}
		
		for(int x = -W; x <= W; x++) {
			block->col1.r = clamp(r1 + x, 0, 31);
			block->evaluatePalette(palette);
			
			const uint error = paletteError(rgba, palette);
			if(error < best_error) {
				best_error = error;
				best_col0 = block->col0;
				best_col1 = block->col1;
				changed = true;
			}
		}
		
		if( !changed ) {
			// Stop at local minima.
			break;
		}
	}
	
	block->col0 = best_col0;
	block->col1 = best_col1;
	block->evaluatePalette(palette);
	
	block->indices = computeIndices(rgba, palette);
}

static uint computeGreenError(const ColorBlock & rgba, const BlockDXT1 * block)
{
	Color32 colors[4];
	block->evaluatePalette4(colors);

	uint totalError = 0;

	for (uint i = 0; i < 16; i++)
	{
		uint8 green = rgba.color(i).g;
		
		uint besterror = 256*256;
		uint best;
		for(uint p = 0; p < 4; p++)
		{
			int d = colors[p].g - green;
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

// Brute force compressor for DXT5n
void nv::compressGreenBlock_BruteForce(const ColorBlock & rgba, BlockDXT1 * block)
{
	nvDebugCheck(block != NULL);
	
	uint8 ming = 63;
	uint8 maxg = 0;
	
	// Get min/max green.
	for (uint i = 0; i < 16; i++)
	{
		uint8 green = rgba.color(i).g >> 2;
		ming = min(ming, green);
		maxg = max(maxg, green);
	}

	block->col0.r = 31;
	block->col1.r = 31;
	block->col0.g = maxg;
	block->col1.g = ming;
	block->col0.b = 0;
	block->col1.b = 0;

	if (maxg - ming > 4)
	{
		int besterror = computeGreenError(rgba, block);
		int bestg0 = maxg;
		int bestg1 = ming;
		
		for (int g0 = ming+5; g0 < maxg; g0++)
		{
			for (int g1 = ming; g1 < g0-4; g1++)
			{
				if ((maxg-g0) + (g1-ming) > besterror)
					continue;
				
				block->col0.g = g0;
				block->col1.g = g1;
				int error = computeGreenError(rgba, block);
				
				if (error < besterror)
				{
					besterror = error;
					bestg0 = g0;
					bestg1 = g1;
				}
			}
		}
		
		block->col0.g = bestg0;
		block->col1.g = bestg1;
	}
	
	Color32 palette[4];
	block->evaluatePalette(palette);
	block->indices = computeIndices(rgba, palette);
}



uint nv::blockError(const ColorBlock & rgba, const BlockDXT1 & block)
{
	Color32 palette[4];
	block.evaluatePalette(palette);

	VectorColor vcolors[4];
	vcolors[0] = loadColor(palette[0]);
	vcolors[1] = loadColor(palette[1]);
	vcolors[2] = loadColor(palette[2]);
	vcolors[3] = loadColor(palette[3]);

	uint error = 0;
	for(uint i = 0; i < 16; i++) {
		const VectorColor vcolor = loadColor(rgba.color(i));

		int idx = (block.indices >> (2 * i)) & 3;

		uint d = colorDistance(vcolor, vcolors[idx]);
		error += d;
	}

	//nvDebugCheck(error == paletteError(rgba, palette));

	vectorEnd();
	return error;
}


uint nv::blockError(const ColorBlock & rgba, const AlphaBlockDXT5 & block)
{
	uint8 palette[8];
	block.evaluatePalette(palette);

	uint8 indices[16];
	block.indices(indices);

	uint error = 0;
	for(uint i = 0; i < 16; i++) {
		int d = palette[indices[i]] - rgba.color(i).a;
		error += uint(d * d);
	}

	return error;
}



void nv::optimizeEndPoints(const ColorBlock & rgba, BlockDXT1 * block)
{
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	Vector3 alphax_sum(zero);
	Vector3 betax_sum(zero);
	
	for( int i = 0; i < 16; ++i )
	{
		const uint bits = block->indices >> (2 * i);

		float beta = float(bits & 1);
		if (bits & 2) beta = (1 + beta) / 3.0f;
		float alpha = 1.0f - beta;

		const Vector3 x = toVector4(rgba.color(i)).xyz();
		
		alpha2_sum += alpha * alpha;
		beta2_sum += beta * beta;
		alphabeta_sum += alpha * beta;
		alphax_sum += alpha * x;
		betax_sum += beta * x;
	}

	float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
		
	Vector3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	Vector3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	Vector3 zero(0, 0, 0);
	Vector3 one(1, 1, 1);
	a = min(one, max(zero, a));
	b = min(one, max(zero, b));
	
	BlockDXT1 B;

	// Round a,b to 565.
	B.col0.r = uint16(a.x() * 31);
	B.col0.g = uint16(a.y() * 63);
	B.col0.b = uint16(a.z() * 31);
	B.col1.r = uint16(b.x() * 31);
	B.col1.g = uint16(b.y() * 63);
	B.col1.b = uint16(b.z() * 31);
	B.indices = block->indices;

	// Force 4 color mode.
	if (B.col0.u < B.col1.u)
	{
		swap(B.col0.u, B.col1.u);
		B.indices ^= 0x55555555;
	}
	else if (B.col0.u == B.col1.u)
	{
		block->indices = 0;
	}

	if (blockError(rgba, B) < blockError(rgba, *block))
	{
		*block = B;
	}
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

static void optimizeAlpha8(const ColorBlock & rgba, AlphaBlockDXT5 * block)
{
	float alpha2_sum = 0;
	float beta2_sum = 0;
	float alphabeta_sum = 0;
	float alphax_sum = 0;
	float betax_sum = 0;

	for (int i = 0; i < 16; i++)
	{
		uint idx = block->index(i);
		float alpha;
		if (idx < 2) alpha = 1.0f - idx;
		else alpha = (8.0f - idx) / 7.0f;
		
		float beta = 1 - alpha;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * rgba.color(i).a;
        betax_sum += beta * rgba.color(i).a;
	}

    const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

    float a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	uint alpha0 = uint(min(max(a, 0.0f), 255.0f));
	uint alpha1 = uint(min(max(b, 0.0f), 255.0f));

	if (alpha0 < alpha1)
	{
		swap(alpha0, alpha1);

		// Flip indices:
		for (int i = 0; i < 16; i++)
		{
			uint idx = block->index(i);
			if (idx < 2) block->setIndex(i, 1 - idx);
			else block->setIndex(i, 9 - idx);
		}
	}
	else if (alpha0 == alpha1)
	{
		for (int i = 0; i < 16; i++)
		{
			block->setIndex(i, 0);
		}
	}

	block->alpha0 = alpha0;
	block->alpha1 = alpha1;
}


static void optimizeAlpha6(const ColorBlock & rgba, AlphaBlockDXT5 * block)
{
	float alpha2_sum = 0;
	float beta2_sum = 0;
	float alphabeta_sum = 0;
	float alphax_sum = 0;
	float betax_sum = 0;

	for (int i = 0; i < 16; i++)
	{
		uint8 x = rgba.color(i).a;
		if (x == 0 || x == 255) continue;

		uint bits = block->index(i);
		if (bits == 6 || bits == 7) continue;

		float alpha;
		if (bits == 0) alpha = 1.0f;
		else if (bits == 1) alpha = 0.0f;
		else alpha = (6.0f - block->index(i)) / 5.0f;
		
		float beta = 1 - alpha;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * x;
        betax_sum += beta * x;
	}

    const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

    float a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	float b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	uint alpha0 = uint(min(max(a, 0.0f), 255.0f));
	uint alpha1 = uint(min(max(b, 0.0f), 255.0f));

	if (alpha0 > alpha1)
	{
		swap(alpha0, alpha1);
	}

	block->alpha0 = alpha0;
	block->alpha1 = alpha1;
}



static bool sameIndices(const AlphaBlockDXT5 & block0, const AlphaBlockDXT5 & block1)
{
	const uint64 mask = ~uint64(0xFFFF);
	return (block0.u | mask) == (block1.u | mask);
}


uint nv::compressBlock_Iterative(const ColorBlock & rgba, AlphaBlockDXT5 * resultblock)
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
	
	AlphaBlockDXT5 block;
	block.alpha0 = alpha0 - (alpha0 - alpha1) / 34;
	block.alpha1 = alpha1 + (alpha0 - alpha1) / 34;
	uint besterror = computeAlphaIndices(rgba, &block);
	
	AlphaBlockDXT5 bestblock = block;
	
	while(true)
	{
		optimizeAlpha8(rgba, &block);
		uint error = computeAlphaIndices(rgba, &block);
		
		if (error >= besterror)
		{
			// No improvement, stop.
			break;
		}
		if (sameIndices(block, bestblock))
		{
			bestblock = block;
			break;
		}
		
		besterror = error;
		bestblock = block;
	};
	
	// Copy best block to result;
	*resultblock = bestblock;

	return besterror;
}
