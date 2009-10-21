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

#ifndef NV_TT_COMPRESSDXT_H
#define NV_TT_COMPRESSDXT_H

#include <nvimage/nvimage.h>
#include "nvtt.h"

namespace nv
{
	class Image;
	struct ColorBlock;

	struct CompressorInterface
	{
		virtual ~CompressorInterface() {}
		virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions) = 0;
	};

	struct FixedBlockCompressor : public CompressorInterface
	{
		virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);

		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output) = 0;
		virtual uint blockSize() const = 0;
	};


	// Fast CPU compressors.
	struct FastCompressorDXT1 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 8; }
	};

	struct FastCompressorDXT1a : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 8; }
	};

	struct FastCompressorDXT3 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};

	struct FastCompressorDXT5 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};

	struct FastCompressorDXT5n : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};

	struct FastCompressorBC4 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 8; }
	};

	struct FastCompressorBC5 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};


	// Normal CPU compressors.
	struct NormalCompressorDXT1 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 8; }
	};

	struct NormalCompressorDXT1a : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 8; }
	};

	struct NormalCompressorDXT3 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};

	struct NormalCompressorDXT5 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};

	struct NormalCompressorDXT5n : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};


	// Production CPU compressors.
	struct ProductionCompressorBC4 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 8; }
	};

	struct ProductionCompressorBC5 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 16; }
	};


	// External compressors.
#if defined(HAVE_S3QUANT)
	struct S3CompressorDXT1 : public CompressorInterface
	{
		virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
	};
#endif
	
#if defined(HAVE_ATITC)
	struct AtiCompressorDXT1 : public CompressorInterface
	{
		virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
	};

	struct AtiCompressorDXT5 : public CompressorInterface
	{
		virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
	};
#endif

#if defined(HAVE_SQUISH)
	struct SquishCompressorDXT1 : public CompressorInterface
	{
		virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
	};
#endif

#if defined(HAVE_D3DX)
	struct D3DXCompressorDXT1 : public CompressorInterface
	{
		virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
	};
#endif

#if defined(HAVE_STB)
	struct StbCompressorDXT1 : public FixedBlockCompressor
	{
		virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
		virtual uint blockSize() const { return 8; }
	};
#endif

} // nv namespace


#endif // NV_TT_COMPRESSDXT_H
