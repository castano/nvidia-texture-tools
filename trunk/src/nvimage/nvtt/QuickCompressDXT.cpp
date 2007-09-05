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

#include "QuickCompressDXT.h"


using namespace nv;
using namespace QuickCompress;



inline static void extractColorBlockRGB(const ColorBlock & rgba, Vector3 block[16])
{
	for (int i = 0; i < 16; i++)
	{
		const Color32 c = rgba.color(i);
		block[i] = Vector3(c.r, c.g, c.b);
	}
}

// find minimum and maximum colors based on bounding box in color space
inline static void findMinMaxColorsBox(Vector3 block[16], Vector3 * __restrict maxColor, Vector3 * __restrict minColor)
{
    *maxColor = Vector3(0, 0, 0);
	*minColor = Vector3(255, 255, 255);
    
	for (int i = 0; i < 16; i++)
	{
        *maxColor = max(*maxColor, block[i]);
		*minColor = min(*minColor, block[i]);
    }
}

inline static void selectDiagonal(Vector3 block[16], Vector3 * __restrict maxColor, Vector3 * __restrict minColor)
{
	Vector3 center = (*maxColor + *minColor) * 0.5;

	Vector2 covariance = Vector2(zero);
	for (int i = 0; i < 16; i++)
	{
		Vector3 t = block[i] - center;
		covariance += t.xy() * t.z();
	}

	float x0 = maxColor->x();
	float y0 = maxColor->y();
	float x1 = minColor->x();
	float y1 = minColor->y();
	
	if (covariance.x() < 0) {
		swap(x0, x1);
	}
	if (covariance.y() < 0) {
		swap(y0, y1);
	}
	
	maxColor->set(x0, y0, maxColor->z());
	minColor->set(x1, y1, minColor->z());
}

inline static void insetBBox(Vector3 * __restrict maxColor, Vector3 * __restrict minColor)
{
	Vector3 inset = (*maxColor - *minColor) / 16.0f - (8.0f / 255.0f) / 16.0f;
	*maxColor = clamp(*maxColor - inset, 0.0f, 255.0f);
	*minColor = clamp(*minColor + inset, 0.0f, 255.0f);
}

inline static uint16 roundAndExpand(Vector3 * v)
{
	uint r = clamp(v->x() * (31.0f / 255.0f), 0.0f, 31.0f) + 0.5f;
	uint g = clamp(v->y() * (63.0f / 255.0f), 0.0f, 63.0f) + 0.5f;
	uint b = clamp(v->z() * (31.0f / 255.0f), 0.0f, 31.0f) + 0.5f;
	
	uint16 w = (r << 11) | (g << 5) | b;

	r = (r << 3) | (r >> 2);
	g = (g << 2) | (g >> 4);
	b = (b << 3) | (b >> 2);
	*v = Vector3(r, g, b);
	
	return w;
}

inline static float colorDistance(Vector3::Arg c0, Vector3::Arg c1)
{
	return dot(c0-c1, c0-c1);
}

inline static uint computeIndices(Vector3 block[16], Vector3::Arg maxColor, Vector3::Arg minColor)
{
	Vector3 c[4]; 
    c[0] = maxColor;
    c[1] = minColor;
    c[2] = lerp(c[0], c[1], 1.0/3.0);
    c[3] = lerp(c[0], c[1], 2.0/3.0);
	
	uint indices = 0;
	for(int i = 0; i < 16; i++)
	{
		float d0 = colorDistance(c[0], block[i]);
		float d1 = colorDistance(c[1], block[i]);
		float d2 = colorDistance(c[2], block[i]);
		float d3 = colorDistance(c[3], block[i]);
		
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

	return indices;
}

static void optimizeEndPoints(Vector3 block[16], BlockDXT1 * dxtBlock)
{
	float alpha2_sum = 0.0f;
	float beta2_sum = 0.0f;
	float alphabeta_sum = 0.0f;
	Vector3 alphax_sum(zero);
	Vector3 betax_sum(zero);
	
	for( int i = 0; i < 16; ++i )
	{
		const uint bits = dxtBlock->indices >> (2 * i);

		float beta = (bits & 1);
		if (bits & 2) beta = (1 + beta) / 3.0f;
		float alpha = 1.0f - beta;

		alpha2_sum += alpha * alpha;
		beta2_sum += beta * beta;
		alphabeta_sum += alpha * beta;
		alphax_sum += alpha * block[i];
		betax_sum += beta * block[i];
	}

	float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
	
	Vector3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
	Vector3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;

	a = clamp(a, 0, 255);
	b = clamp(b, 0, 255);
	
	uint16 color0 = roundAndExpand(&a);
	uint16 color1 = roundAndExpand(&b);

	if (color0 < color1)
	{
		swap(a, b);
		swap(color0, color1);
	}

	dxtBlock->col0 = Color16(color0);
	dxtBlock->col1 = Color16(color1);
	dxtBlock->indices = computeIndices(block, a, b);
}


void QuickCompress::compressDXT1(const ColorBlock & rgba, BlockDXT1 * dxtBlock)
{
	// read block
	Vector3 block[16];
	extractColorBlockRGB(rgba, block);
	
	// find min and max colors
	Vector3 maxColor, minColor;
	findMinMaxColorsBox(block, &maxColor, &minColor);
	
	selectDiagonal(block, &maxColor, &minColor);
	
	insetBBox(&maxColor, &minColor);
	
	uint16 color0 = roundAndExpand(&maxColor);
	uint16 color1 = roundAndExpand(&minColor);

	if (color0 < color1)
	{
		swap(maxColor, minColor);
		swap(color0, color1);
	}

	dxtBlock->col0 = Color16(color0);
	dxtBlock->col1 = Color16(color1);
	dxtBlock->indices = computeIndices(block, maxColor, minColor);

	optimizeEndPoints(block, dxtBlock);
}


