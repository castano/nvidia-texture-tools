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

#ifndef NV_TT_COMPRESSOR_H
#define NV_TT_COMPRESSOR_H

#include "nvtt.h"

namespace nv
{
	class Image;
}

namespace nvtt
{

	struct Compressor::Private
	{
		Private() {}

		bool compress(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const;
		int estimateSize(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions) const;

	private:

		bool outputHeader(const InputOptions::Private & inputOptions, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions) const;
		bool compressMipmap(const nv::Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions) const;


	public:

		bool cudaSupported;
		bool cudaEnabled;
	};

} // nvtt namespace


#endif // NV_TT_COMPRESSOR_H
