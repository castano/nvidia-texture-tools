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

#include <nvcore/Debug.h>

#include <nvimage/Image.h>
#include <nvmath/Color.h>

#include "CompressRGB.h"
#include "CompressionOptions.h"


using namespace nv;
using namespace nvtt;

namespace 
{

	inline uint computePitch(uint w, uint bitsize)
	{
		uint p = w * ((bitsize + 7) / 8);

		// Align to 32 bits.
		return ((p + 3) / 4) * 4;
	}

	static void convert_to_a8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

	static void convert_to_x8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

	static uint convert(uint c, uint inbits, uint outbits)
	{
		if (inbits <= outbits) 
		{
			// truncate
			return c >> (inbits - outbits);
		}
		else
		{
			// bitexpand
			return (c << (outbits - inbits)) | convert(c, inbits, outbits - inbits);
		}
	}

	static void maskShiftAndSize(uint mask, uint * shift, uint * size)
	{
		*shift = 0;
		while((mask & 1) == 0) {
			++(*shift);
			mask >>= 1;
		}
		
		*size = 0;
		while((mask & 1) == 1) {
			++(*size);
			mask >>= 1;
		}
	}
	
} // namespace


// Pixel format converter.
void nv::compressRGB(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvCheck(image != NULL);

	const uint w = image->width();
	const uint h = image->height();

	const uint bitCount = compressionOptions.bitcount;
	nvCheck(bitCount == 8 || bitCount == 16 || bitCount == 24 || bitCount == 32);

	const uint byteCount = bitCount / 8;

	const uint rmask = compressionOptions.rmask;
	uint rshift, rsize;
	maskShiftAndSize(rmask, &rshift, &rsize);
	
	const uint gmask = compressionOptions.gmask;
	uint gshift, gsize;
	maskShiftAndSize(gmask, &gshift, &gsize);
	
	const uint bmask = compressionOptions.bmask;
	uint bshift, bsize;
	maskShiftAndSize(bmask, &bshift, &bsize);
	
	const uint amask = compressionOptions.amask;
	uint ashift, asize;
	maskShiftAndSize(amask, &ashift, &asize);

	// @@ Perform error diffusion dithering.

	// Determine pitch.
	uint pitch = computePitch(w, compressionOptions.bitcount);

	uint8 * dst = (uint8 *)mem::malloc(pitch + 4);

	for (uint y = 0; y < h; y++)
	{
		const Color32 * src = image->scanline(y);

		if (bitCount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0xFF000000)
		{
			convert_to_a8r8g8b8(src, dst, w);
		}
		else if (bitCount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0)
		{
			convert_to_x8r8g8b8(src, dst, w);
		}
		else
		{
			// Generic pixel format conversion.
			for (uint x = 0; x < w; x++)
			{
				uint c = 0;
				c |= convert(src[x].r, 8, rsize) << rshift;
				c |= convert(src[x].g, 8, gsize) << gshift;
				c |= convert(src[x].b, 8, bsize) << bshift;
				c |= convert(src[x].a, 8, asize) << ashift;
				
				*(uint *)(dst + x * byteCount) = c;
			}
		}

		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(dst, pitch);
		}
	}

	mem::free(dst);
}

