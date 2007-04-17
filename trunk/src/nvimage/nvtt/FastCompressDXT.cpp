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
#include "BlockDXT.h"
#include "FastCompressDXT.h"

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#if defined(__MMX__)
#include <mmintrin.h>
#include <xmmintrin.h>
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
		
		/*if (d0 < d1 && d0 < d2 && d0 < d3) {
			indices |= 0 << (2 * i);
		}
		else if (d1 < d2 && d1 < d3) {
			indices |= 1 << (2 * i);
		}
		else if (d2 < d3) {
			indices |= 2 << (2 * i);
		}
		else {
			indices |= 3 << (2 * i);
		}*/
		
		/*
		uint b0 = d0 > d2;
		uint b1 = d1 > d3;
		uint b2 = d0 > d3;
		uint b3 = d1 > d2;
		uint b4 = d0 > d1;
		uint b5 = d2 > d3;
		
		uint x0 = b1 & b2;
		uint x1 = b0 & b3;
		uint x2 = b2 & b5;
		uint x3 = !b3 & b4;
		
		indices |= ((x3 | x2) | ((x1 | x0) << 1)) << (2 * i);
		*/

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
	
	nvDebugCheck(block->col0.u >= block->col1.u);
	
	// Use 4 color mode only.
	//if (block->col0.u < block->col1.u) {
	//	swap(block->col0.u, block->col1.u);
	//}
	
	Color32 palette[4];
	block->evaluatePalette4(palette);
	
	block->indices = computeIndices(rgba, palette);
}


// Compressor that uses the best fit axis.
void nv::compressBlock_BestFitAxis(const ColorBlock & rgba, BlockDXT1 * block)
{
	Color32 c0, c1;
	rgba.bestFitRange(&c0, &c1);
	
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


// Compressor that tests all pairs in the best fit axis.
void nv::compressBlock_AnalyzeBestFitAxis(const ColorBlock & rgba, BlockDXT1 * block)
{
	uint best_error = uint(-1);
	Color16 best_col0, best_col1;
	
	Color32 palette[4];
	

	// Find bounds of the search space.
	int r_min = 32;
	int r_max = 0;
	int g_min = 64;
	int g_max = 0;
	int b_min = 32;
	int b_max = 0;
	
	for(uint i = 0; i < 16; i++) {
		Color16 color = toColor16(rgba.color(i));
		
		r_min = min(r_min, int(color.r));
		r_max = max(r_max, int(color.r));
		g_min = min(g_min, int(color.g));
		g_max = max(g_max, int(color.g));
		b_min = min(b_min, int(color.b));
		b_max = max(b_max, int(color.b));
	}
	
	const int r_pad = 4 * max(1, (r_max - r_min));
	const int g_pad = 4 * max(1, (g_max - g_min));
	const int b_pad = 4 * max(1, (b_max - b_min));

	r_min = max(0, r_min - r_pad);
	r_max = min(31, r_max + r_pad);
	g_min = max(0, g_min - g_pad);
	g_max = min(63, g_max + g_pad);
	b_min = max(0, b_min - b_pad);
	b_max = min(31, b_max + b_pad);
	
	const Line3 line = rgba.bestFitLine();
	
	if( fabs(line.direction().x()) > fabs(line.direction().y()) && fabs(line.direction().x()) > fabs(line.direction().z()) ) {
		for(int r0 = r_min; r0 <= r_max; r0++) {
			const float x0 = (r0 << 3) | (r0 >> 2);
			const float t0 = (x0 - line.origin().x()) / line.direction().x();
			const float y0 = line.origin().y() + t0 * line.direction().y();
			const float z0 = line.origin().z() + t0 * line.direction().z();
			
			const int g0 = clamp(int(y0), 0, 255) >> 2;
			const int b0 = clamp(int(z0), 0, 255) >> 3;
			
			for(int r1 = r_min; r1 <= r_max; r1++) {
				const float x1 = (r1 << 3) | (r1 >> 2);
				const float t1 = (x1 - line.origin().x()) / line.direction().x();
				const float y1 = line.origin().y() + t1 * line.direction().y();
				const float z1 = line.origin().z() + t1 * line.direction().z();
				
				const int g1 = clamp(int(y1), 0, 255) >> 2;
				const int b1 = clamp(int(z1), 0, 255) >> 3;
				
				// Test one pixel around.
				for (int i0 = -1; i0 <= 1; i0++) {
					for (int j0 = -1; j0 <= 1; j0++) {
						for (int i1 = -1; i1 <= 1; i1++) {
							for (int j1 = -1; j1 <= 1; j1++) { 
								if( g0+i0 >= 0 && g0+i0 < 64 && g1+i1 >= 0 && g1+i1 < 64 &&
									b0+j0 >= 0 && b0+j0 < 32 && b1+j1 >= 0 && b1+j1 < 32 )
								{
									block->col0.r = r0;
									block->col0.g = g0 + i0;
									block->col0.b = b0 + j0;
									block->col1.r = r1;
									block->col1.g = g1 + i1;
									block->col1.b = b1 + j1;
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
					}
				}
			}
		}
	}
	else if( fabs(line.direction().y()) > fabs(line.direction().z()) ) {
		for(int g0 = g_min; g0 <= g_max; g0++) {
			const float y0 = (g0 << 2) | (g0 >> 4);
			const float t0 = (y0 - line.origin().y()) / line.direction().y();
			const float x0 = line.origin().x() + t0 * line.direction().x();
			const float z0 = line.origin().z() + t0 * line.direction().z();
			
			const int r0 = clamp(int(x0), 0, 255) >> 3;
			const int b0 = clamp(int(z0), 0, 255) >> 3;
			
			for(int g1 = g_min; g1 <= g_max; g1++) {
				const float y1 = (g1 << 2) | (g1 >> 4);
				const float t1 = (y1 - line.origin().y()) / line.direction().y();
				const float x1 = line.origin().x() + t1 * line.direction().x();
				const float z1 = line.origin().z() + t1 * line.direction().z();
				
				const int r1 = clamp(int(x1), 0, 255) >> 2;
				const int b1 = clamp(int(z1), 0, 255) >> 3;
				
				// Test one pixel around.
				for (int i0 = -1; i0 <= 1; i0++) {
					for (int j0 = -1; j0 <= 1; j0++) {
						for (int i1 = -1; i1 <= 1; i1++) {
							for (int j1 = -1; j1 <= 1; j1++) { 
								if( r0+i0 >= 0 && r0+i0 < 32 && r1+i1 >= 0 && r1+i1 < 32 &&
									b0+j0 >= 0 && b0+j0 < 32 && b1+j1 >= 0 && b1+j1 < 32 )
								{
									block->col0.r = r0 + i0;
									block->col0.g = g0;
									block->col0.b = b0 + j0;
									block->col1.r = r1 + i1;
									block->col1.g = g1;
									block->col1.b = b1 + j1;
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
					}
				}
			}
		}
	}
	else {
		for(int b0 = b_min; b0 <= b_max; b0++) {
			const float z0 = (b0 << 3) | (b0 >> 2);
			const float t0 = (z0 - line.origin().z()) / line.direction().z();
			const float y0 = line.origin().y() + t0 * line.direction().y();
			const float x0 = line.origin().x() + t0 * line.direction().x();
			
			const int g0 = clamp(int(y0), 0, 255) >> 2;
			const int r0 = clamp(int(x0), 0, 255) >> 3;
			
			for(int b1 = b_min; b1 <= b_max; b1++) {
				const float z1 = (b1 << 3) | (b1 >> 2);
				const float t1 = (z1 - line.origin().z()) / line.direction().z();
				const float y1 = line.origin().y() + t1 * line.direction().y();
				const float x1 = line.origin().x() + t1 * line.direction().x();
				
				const int g1 = clamp(int(y1), 0, 255) >> 2;
				const int r1 = clamp(int(x1), 0, 255) >> 3;
				
				// Test one pixel around.
				for (int i0 = -1; i0 <= 1; i0++) {
					for (int j0 = -1; j0 <= 1; j0++) {
						for (int i1 = -1; i1 <= 1; i1++) {
							for (int j1 = -1; j1 <= 1; j1++) { 
								if( g0+i0 >= 0 && g0+i0 < 64 && g1+i1 >= 0 && g1+i1 < 64 &&
									r0+j0 >= 0 && r0+j0 < 32 && r1+j1 >= 0 && r1+j1 < 32 )
								{
									block->col0.r = r0 + j0;
									block->col0.g = g0 + i0;
									block->col0.b = b0;
									block->col1.r = r1 + j1;
									block->col1.g = g1 + i1;
									block->col1.b = b1;
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
					}
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

		float beta = (bits & 1);
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
	
	nvDebugCheck(block->color.col0.u >= block->color.col1.u);
	
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

	int centroidDist = 256;
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
	}

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
