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

#include "CompressorRGBE.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"

#include "nvimage/Image.h"
#include "nvimage/FloatImage.h"

#include "nvmath/Color.h"

#include "nvcore/Debug.h"

using namespace nv;
using namespace nvtt;

static Color32 toRgbe8(float r, float g, float b)
{
    Color32 c;
    float v = max(max(r, g), b);
    if (v < 1e-32) {
        c.r = c.g = c.b = c.a = 0;
    }
    else {
        int e;
        v = frexp(v, &e) * 256.0f / v;
        c.r = uint8(clamp(r * v, 0.0f, 255.0f));
        c.g = uint8(clamp(g * v, 0.0f, 255.0f));
        c.b = uint8(clamp(b * v, 0.0f, 255.0f));
        c.a = e + 128;
    }

    return c;
}


void CompressorRGBE::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, const void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck (compressionOptions.format == nvtt::Format_RGBE);

    uint srcPitch = w;
    uint srcPlane = w * h;

    // Allocate output scanline.
	Color32 * dst = new Color32[w];

	for (uint y = 0; y < h; y++)
	{
        const uint * src = (const uint *)data + y * srcPitch;
        const float * fsrc = (const float *)data + y * srcPitch;

		for (uint x = 0; x < w; x++)
		{
            float r, g, b;

            if (inputFormat == nvtt::InputFormat_BGRA_8UB) {
                Color32 c = Color32(src[x]);
                r = float(c.r) / 255.0f;
                g = float(c.g) / 255.0f;
                b = float(c.b) / 255.0f;
            }
            else {
                nvDebugCheck (inputFormat == nvtt::InputFormat_RGBA_32F);

			    // Color components not interleaved.
			    r = fsrc[x + 0 * srcPlane];
			    g = fsrc[x + 1 * srcPlane];
			    b = fsrc[x + 2 * srcPlane];
            }
            
            dst[x] = toRgbe8(r, g, b);
        }

		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(dst, w * 4);
		}
    }

    delete [] dst;
}
