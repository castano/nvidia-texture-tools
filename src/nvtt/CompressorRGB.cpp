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

#include "CompressorRGB.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"

#include <nvimage/Image.h>
#include <nvimage/FloatImage.h>
#include <nvimage/PixelFormat.h>

#include <nvmath/Color.h>
#include <nvmath/Half.h>

#include <nvcore/Debug.h>

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

	inline void convert_to_a8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

	inline void convert_to_x8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

    static uint16 to_half(float f)
    {
	    union { float f; uint32 u; } c;
        c.f = f;
        return half_from_float(c.u);
    }

} // namespace



void PixelFormatConverter::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, const void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
	uint bitCount;
	uint rmask, rshift, rsize;
	uint gmask, gshift, gsize;
	uint bmask, bshift, bsize;
	uint amask, ashift, asize;

    if (compressionOptions.pixelType == nvtt::PixelType_Float)
    {
	    rsize = compressionOptions.rsize;
	    gsize = compressionOptions.gsize;
	    bsize = compressionOptions.bsize;
	    asize = compressionOptions.asize;

	    nvCheck(rsize == 0 || rsize == 16 || rsize == 32);
	    nvCheck(gsize == 0 || gsize == 16 || gsize == 32);
	    nvCheck(bsize == 0 || bsize == 16 || bsize == 32);
	    nvCheck(asize == 0 || asize == 16 || asize == 32);

	    bitCount = rsize + gsize + bsize + asize;
    }
    else
    {
	    if (compressionOptions.bitcount != 0)
	    {
		    bitCount = compressionOptions.bitcount;
		    nvCheck(bitCount == 8 || bitCount == 16 || bitCount == 24 || bitCount == 32);

		    rmask = compressionOptions.rmask;
		    gmask = compressionOptions.gmask;
		    bmask = compressionOptions.bmask;
		    amask = compressionOptions.amask;

		    PixelFormat::maskShiftAndSize(rmask, &rshift, &rsize);
		    PixelFormat::maskShiftAndSize(gmask, &gshift, &gsize);
		    PixelFormat::maskShiftAndSize(bmask, &bshift, &bsize);
		    PixelFormat::maskShiftAndSize(amask, &ashift, &asize);
	    }
	    else
	    {
		    rsize = compressionOptions.rsize;
		    gsize = compressionOptions.gsize;
		    bsize = compressionOptions.bsize;
		    asize = compressionOptions.asize;

		    bitCount = rsize + gsize + bsize + asize;
		    nvCheck(bitCount <= 32);

		    ashift = 0;
		    bshift = ashift + asize;
		    gshift = bshift + bsize;
		    rshift = gshift + gsize;

		    rmask = ((1 << rsize) - 1) << rshift;
		    gmask = ((1 << gsize) - 1) << gshift;
		    bmask = ((1 << bsize) - 1) << bshift;
		    amask = ((1 << asize) - 1) << ashift;
	    }
    }

	uint byteCount = (bitCount + 7) / 8;
    uint pitch = computePitch(w, bitCount);

    uint srcPitch = w;
    uint srcPlane = w * h;


    // Allocate output scanline.
	uint8 * dst = (uint8 *)mem::malloc(pitch + 4);

	for (uint y = 0; y < h; y++)
	{
        const uint * src = (const uint *)data + y * srcPitch;
        const float * fsrc = (const float *)data + y * srcPitch;

	    if (inputFormat == nvtt::InputFormat_BGRA_8UB && compressionOptions.pixelType == nvtt::PixelType_UnsignedNorm && bitCount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0xFF000000)
	    {
            convert_to_a8r8g8b8(src, dst, w);
        }
        else
        {
            uint8 * ptr = dst;

		    for (uint x = 0; x < w; x++)
		    {
                float r, g, b, a;

                if (inputFormat == nvtt::InputFormat_BGRA_8UB) {
                    Color32 c = Color32(src[x]);
                    r = float(c.r) / 255.0f;
                    g = float(c.g) / 255.0f;
                    b = float(c.b) / 255.0f;
                    a = float(c.a) / 255.0f;
                }
                else {
                    nvDebugCheck (inputFormat == nvtt::InputFormat_RGBA_32F);

			        //r = ((float *)src)[4 * x + 0]; // Color components not interleaved.
			        //g = ((float *)src)[4 * x + 1];
			        //b = ((float *)src)[4 * x + 2];
			        //a = ((float *)src)[4 * x + 3];
			        r = fsrc[x + 0 * srcPlane];
			        g = fsrc[x + 1 * srcPlane];
			        b = fsrc[x + 2 * srcPlane];
			        a = fsrc[x + 3 * srcPlane];
                }

                if (compressionOptions.pixelType == nvtt::PixelType_Float)
                {
			        if (rsize == 32) *((float *)ptr) = r;
			        else if (rsize == 16) *((uint16 *)ptr) = to_half(r);
			        ptr += rsize / 8;

			        if (gsize == 32) *((float *)ptr) = g;
			        else if (gsize == 16) *((uint16 *)ptr) = to_half(g);
			        ptr += gsize / 8;

			        if (bsize == 32) *((float *)ptr) = b;
			        else if (bsize == 16) *((uint16 *)ptr) = to_half(b);
			        ptr += bsize / 8;

			        if (asize == 32) *((float *)ptr) = a;
			        else if (asize == 16) *((uint16 *)ptr) = to_half(a);
			        ptr += asize / 8;
                }
                else
                {
                    Color32 c;
                    if (compressionOptions.pixelType == nvtt::PixelType_UnsignedNorm) {
                        c.r = uint8(clamp(r * 255, 0.0f, 255.0f));
                        c.g = uint8(clamp(g * 255, 0.0f, 255.0f));
                        c.b = uint8(clamp(b * 255, 0.0f, 255.0f));
                        c.a = uint8(clamp(a * 255, 0.0f, 255.0f));
                    }
                    // @@ Add support for nvtt::PixelType_SignedInt, nvtt::PixelType_SignedNorm, nvtt::PixelType_UnsignedInt

				    uint p = 0;
				    p |= PixelFormat::convert(c.r, 8, rsize) << rshift;
				    p |= PixelFormat::convert(c.g, 8, gsize) << gshift;
				    p |= PixelFormat::convert(c.b, 8, bsize) << bshift;
				    p |= PixelFormat::convert(c.a, 8, asize) << ashift;
    				
				    // Output one byte at a time.
				    for (uint i = 0; i < byteCount; i++)
				    {
					    *(dst + x * byteCount + i) = (p >> (i * 8)) & 0xFF;
				    }
                }
            }

		    // Zero padding.
		    for (uint x = w * byteCount; x < pitch; x++)
		    {
			    *(dst + x) = 0;
		    }
        }

		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(dst, pitch);
		}
    }

	mem::free(dst);
}
