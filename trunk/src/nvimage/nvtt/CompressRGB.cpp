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

#include <string.h>
#include <nvcore/Debug.h>
#include <nvimage/Image.h>

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

	static void convert_to_rgba8888(void * src, void * dst, uint w)
	{
		// @@ TODO
	}

	static void convert_to_bgra8888(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

	static void convert_to_rgb888(const void * src, void * dst, uint w)
	{
		// @@ TODO
	}

	static uint truncate(uint c, uint inbits, uint outbits)
	{
		nvDebugCheck(inbits > outbits);	
		c >>= inbits - outbits;
	}

	static uint bitexpand(uint c, uint inbits, uint outbits)
	{
		// @@ TODO
	}
	
	static void maskShiftAndSize(uint mask, uint & shift, uint & size)
	{
		shift = 0;
		while((mask & 1) == 0) {
			shift++;
			mask >>= 1;
		}
		
		while((mask & 1) == 1) {
			size++;
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

	uint rshift, rsize;
	maskShiftAndSize(compressionOptions.rmask, rshift, rsize);
	
	uint gshift, gsize;
	maskShiftAndSize(compressionOptions.gmask, gshift, gsize);
	
	uint bshift, bsize;
	maskShiftAndSize(compressionOptions.bmask, bshift, bsize);
	
	uint ashift, asize;
	maskShiftAndSize(compressionOptions.amask, ashift, asize);


	// Determine pitch.
	uint pitch = computePitch(w, compressionOptions.bitcount);

	void * dst = malloc(pitch);

	for (uint y = 0; y < h; y++)
	{
		const Color32 * src = image->scanline(y);

		convert_to_bgra8888(src, dst, w);

		if (false)
		{
		//	uint c = 0;
		//	c |= (src[i].r >> (8 - rsize)) << rshift;
		//	c |= (src[i].g >> (8 - gsize)) << gshift;
		//	c |= (src[i].b >> (8 - bsize)) << bshift;
		}

		/*
		if (rmask == 0xFF000000 && gmask == 0xFF0000 && bmask == 0xFF00 && amask == 0xFF)
		{
			convert_to_rgba8888(src, dst, w);
		}
		else if (rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0)
		{
			convert_to_rgb888(src, dst, w);
		}
		else
		{
			// @@ Not supported.
		}
		*/

		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(dst, pitch);
		}
	}

	free(dst);
}


