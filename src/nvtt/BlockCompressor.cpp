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

#include "BlockCompressor.h"
#include "OutputOptions.h"
#include "TaskDispatcher.h"

#include "nvimage/Image.h"
#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/Vector.inl"

#include "nvcore/Memory.h"

#include <new> // placement new


using namespace nv;
using namespace nvtt;

/*
// OpenMP
#if defined(HAVE_OPENMP)
#include <omp.h>
#endif

void ColorBlockCompressor::compress(nvtt::AlphaMode alphaMode, uint w, uint h, const float * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    const uint bs = blockSize();
    const uint bw = (w + 3) / 4;
    const uint bh = (h + 3) / 4;

#if defined(HAVE_OPENMP)
    bool singleThreaded = false;
#else
    bool singleThreaded = true;
#endif

    // Use a single thread to compress small textures.
    if (bw * bh < 16) singleThreaded = true;

    if (singleThreaded)
    {
        nvDebugCheck(bs <= 16);
        uint8 mem[16]; // @@ Output one row at a time!

        for (int y = 0; y < int(h); y += 4) {
            for (uint x = 0; x < w; x += 4) {

                ColorBlock rgba;
                rgba.init(w, h, data, x, y);

                compressBlock(rgba, alphaMode, compressionOptions, mem);

                if (outputOptions.outputHandler != NULL) {
                    outputOptions.outputHandler->writeData(mem, bs);
                }
            }
        }
    }
#if defined(HAVE_OPENMP)
    else
    {
        const uint size = bs * bw * bh;
        uint8 * mem = new uint8[size];

        #pragma omp parallel
        {
            #pragma omp for
            for (int i = 0; i < int(bw*bh); i++)
            {
                const uint x = i % bw;
                const uint y = i / bw;

		ColorBlock rgba;
		rgba.init(w, h, data, 4*x, 4*y);

		uint8 * ptr = mem + (y * bw + x) * bs;
		compressBlock(rgba, alphaMode, compressionOptions, ptr);
	    } // omp for
	} // omp parallel

	if (outputOptions.outputHandler != NULL) {
	    outputOptions.outputHandler->writeData(mem, size);
	}

        delete [] mem;
    }
#endif
}
*/


struct ColorBlockCompressorContext
{
    nvtt::AlphaMode alphaMode;
    uint w, h;
    const float * data;
    const nvtt::CompressionOptions::Private * compressionOptions;

    uint bw, bh, bs;
    uint8 * mem;
    ColorBlockCompressor * compressor;
};

// Each task compresses one block.
void ColorBlockCompressorTask(void * data, int i)
{
    ColorBlockCompressorContext * d = (ColorBlockCompressorContext *) data;

    uint x = i % d->bw;
    uint y = i / d->bw;

    //for (uint x = 0; x < d->bw; x++)
    {
        ColorBlock rgba;
        rgba.init(d->w, d->h, d->data, 4*x, 4*y);

        uint8 * ptr = d->mem + (y * d->bw + x) * d->bs;
        d->compressor->compressBlock(rgba, d->alphaMode, *d->compressionOptions, ptr);
    }
}

void ColorBlockCompressor::compress(nvtt::AlphaMode alphaMode, uint w, uint h, uint d, const float * data, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    ColorBlockCompressorContext context;
    context.alphaMode = alphaMode;
    context.w = w;
    context.h = h;
    context.data = data;
    context.compressionOptions = &compressionOptions;

    context.bs = blockSize();
    context.bw = (w + 3) / 4;
    context.bh = (h + 3) / 4;

    context.compressor = this;

    SequentialTaskDispatcher sequential;

    // Use a single thread to compress small textures.
    if (context.bh < 4) dispatcher = &sequential;

#if _DEBUG
    dispatcher = &sequential;
#endif

    const uint count = context.bw * context.bh;
    const uint size = context.bs * count;
    context.mem = new uint8[size];

    dispatcher->dispatch(ColorBlockCompressorTask, &context, count);

    outputOptions.writeData(context.mem, size);

    delete [] context.mem;
}


struct ColorSetCompressorContext
{
    nvtt::AlphaMode alphaMode;
    uint w, h;
    const float * data;
    const nvtt::CompressionOptions::Private * compressionOptions;

    uint bw, bh, bs;
    uint8 * mem;
    ColorSetCompressor * compressor;
};


// Each task compresses one block.
void ColorSetCompressorTask(void * data, int i)
{
    ColorSetCompressorContext * d = (ColorSetCompressorContext *) data;

    uint x = i % d->bw;
    uint y = i / d->bw;

    //for (uint x = 0; x < d->bw; x++)
    {
        ColorSet set;
        set.setColors(d->data, d->w, d->h, x * 4, y * 4);

        uint8 * ptr = d->mem + (y * d->bw + x) * d->bs;
        d->compressor->compressBlock(set, d->alphaMode, *d->compressionOptions, ptr);
    }
}


void ColorSetCompressor::compress(AlphaMode alphaMode, uint w, uint h, uint d, const float * data, nvtt::TaskDispatcher * dispatcher, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    ColorSetCompressorContext context;
    context.alphaMode = alphaMode;
    context.w = w;
    context.h = h;
    context.data = data;
    context.compressionOptions = &compressionOptions;

    context.bs = blockSize();
    context.bw = (w + 3) / 4;
    context.bh = (h + 3) / 4;

    context.compressor = this;

    SequentialTaskDispatcher sequential;

    // Use a single thread to compress small textures.
    if (context.bh < 4) dispatcher = &sequential;

#if _DEBUG
    dispatcher = &sequential;
#endif

    const uint count = context.bw * context.bh;
    const uint size = context.bs * count;
    context.mem = new uint8[size];

    dispatcher->dispatch(ColorSetCompressorTask, &context, count);

    outputOptions.writeData(context.mem, size);

    delete [] context.mem;
}
