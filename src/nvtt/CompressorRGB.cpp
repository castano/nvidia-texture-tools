// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
// Copyright (c) 2007-2009 NVIDIA Corporation -- Ignacio Castano <icastano@nvidia.com>
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

    struct BitStream
    {
        BitStream(uint8 * ptr) : ptr(ptr), buffer(0), bits(0) {
        }

        void putBits(uint p, int bitCount)
        {
            nvDebugCheck(bits < 8);
            nvDebugCheck(bitCount <= 32);

            uint64 buffer = (this->buffer << bitCount) | p;
            uint bits = this->bits + bitCount;

            while (bits >= 8)
            {
                *ptr++ = (buffer & 0xFF);
                
                buffer >>= 8;
                bits -= 8;
            }

            this->buffer = (uint8)buffer;
            this->bits = bits;
        }

        void putFloat(float f)
        {
            nvDebugCheck(bits == 0);
            *((float *)ptr) = f;
            ptr += 4;
        }

        void putHalf(float f)
        {
            nvDebugCheck(bits == 0);
            *((uint16 *)ptr) = to_half(f);
            ptr += 2;
        }

        void flush()
        {
            nvDebugCheck(bits < 8);
            if (bits) {
                *ptr++ = buffer;
                buffer = 0;
                bits = 0;
            }
        }

        void align(int alignment)
        {
            nvDebugCheck(alignment >= 1);
            flush();
            putBits(0, ((size_t)ptr % alignment) * 8);
        }

        uint8 * ptr;
        uint8 buffer;
        uint8 bits;
    };

} // namespace



void PixelFormatConverter::compress(nvtt::AlphaMode /*alphaMode*/, uint w, uint h, const float * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck (compressionOptions.format == nvtt::Format_RGBA);

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
            nvCheck(bitCount <= 32);

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

    const uint pitch = computePitch(w, bitCount, compressionOptions.pitchAlignment);
    const uint wh = w * h;

    // Allocate output scanline.
    uint8 * const dst = malloc<uint8>(pitch);

    for (uint y = 0; y < h; y++)
    {
        const uint * src = (const uint *)data + y * w;
        const float * fsrc = (const float *)data + y * w;

        BitStream stream(dst);

        for (uint x = 0; x < w; x++)
        {
            float r = fsrc[x + 0 * wh];
            float g = fsrc[x + 1 * wh];
            float b = fsrc[x + 2 * wh];
            float a = fsrc[x + 3 * wh];

            if (compressionOptions.pixelType == nvtt::PixelType_Float)
            {
                if (rsize == 32) stream.putFloat(r);
                else if (rsize == 16) stream.putHalf(r);

                if (gsize == 32) stream.putFloat(g);
                else if (gsize == 16) stream.putHalf(g);

                if (bsize == 32) stream.putFloat(b);
                else if (bsize == 16) stream.putHalf(b);

                if (asize == 32) stream.putFloat(a);
                else if (asize == 16) stream.putHalf(a);
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

                stream.putBits(p, bitCount);

                // Output one byte at a time.
                /*for (uint i = 0; i < byteCount; i++)
                {
                        *(dst + x * byteCount + i) = (p >> (i * 8)) & 0xFF;
                }*/
            }
        }

        // Zero padding.
        stream.align(compressionOptions.pitchAlignment);
        nvDebugCheck(stream.ptr == dst + pitch);

        /*for (uint x = w * byteCount; x < pitch; x++)
        {
                *(dst + x) = 0;
        }*/

        outputOptions.writeData(dst, pitch);
    }

    free(dst);
}
