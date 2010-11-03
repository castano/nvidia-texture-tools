// Copyright NVIDIA Corporation 2008 -- Ignacio Castano <icastano@nvidia.com>
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

#ifndef NV_TT_CONTEXT_H
#define NV_TT_CONTEXT_H

#include "nvcore/Ptr.h"

#include "nvtt/Compressor.h"
#include "nvtt/cuda/CudaCompressorDXT.h"
#include "nvtt.h"

namespace nv
{
    class Image;
}

namespace nvtt
{
    struct Mipmap;

    struct Compressor::Private
    {
        Private() {}

        bool compress(const TexImage & tex, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;
        bool compress(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;
        bool compress(AlphaMode alphaMode, int w, int h, int d, const float * data, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;

        //int estimateSize(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions) const;

        bool outputHeader(const TexImage & tex, int mipmapCount, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;
        bool outputHeader(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;

	nv::CompressorInterface * chooseCpuCompressor(const CompressionOptions::Private & compressionOptions) const;
	nv::CompressorInterface * chooseGpuCompressor(const CompressionOptions::Private & compressionOptions) const;

	//bool compressMipmaps(uint f, const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;

	//bool initMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f, uint m) const;

	//int findExactMipmap(const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f) const;
	//int findClosestMipmap(const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f) const;

	//void downsampleMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions) const;
	//void scaleMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions, uint w, uint h, uint d) const;
	//void premultiplyAlphaMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions) const;
	//void processInputImage(Mipmap & mipmap, const InputOptions::Private & inputOptions) const;
	//void quantizeMipmap(Mipmap & mipmap, const CompressionOptions::Private & compressionOptions) const;


	bool cudaSupported;
	bool cudaEnabled;

	nv::AutoPtr<nv::CudaContext> cuda;

    };

} // nvtt namespace


#endif // NV_TT_CONTEXT_H
