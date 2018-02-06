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

#ifndef NVTT_BLOCKCOMPRESSOR_H
#define NVTT_BLOCKCOMPRESSOR_H

#include "Compressor.h"

namespace nv
{
    struct ColorBlock;
    class Vector4;

    struct ColorBlockCompressor : public CompressorInterface
    {
        virtual void compress(nvtt::AlphaMode alphaMode, uint w, uint h, uint d, const float * rgba, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);

        virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output) = 0;
        virtual uint blockSize() const = 0;
    };

    struct FloatColorCompressor : public CompressorInterface
    {
        virtual void compress(nvtt::AlphaMode alphaMode, uint w, uint h, uint d, const float * rgba, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);

        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output) = 0;
        virtual uint blockSize(const nvtt::CompressionOptions::Private & compressionOptions) const = 0;
    };


    // BC1
    struct FastCompressorDXT1 : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private &) const { return 8; }
    };
    struct CompressorDXT1 : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private &) const { return 8; }
    };

    // BC3
    struct CompressorBC3_RGBM : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private &) const { return 16; }
    };


    // ETC
    struct CompressorETC1 : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private &) const { return 8; }
    };
    struct CompressorETC2_R : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private & ) const { return 8; }
    };
    struct CompressorETC2_RG : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private & ) const { return 16; }
    };
    struct CompressorETC2_RGB : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private & ) const { return 8; }
    };
    struct CompressorETC2_RGBA : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private & ) const { return 16; }
    };
    struct CompressorETC2_RGBM : public FloatColorCompressor
    {
        virtual void compressBlock(Vector4 colors[16], float weights[16], const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize(const nvtt::CompressionOptions::Private &) const { return 16; }
    };
    
    
    // External compressors.
#if defined(HAVE_ATITC)
    struct AtiCompressorDXT1 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };

    struct AtiCompressorDXT5 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };
#endif

#if defined(HAVE_SQUISH)
    struct SquishCompressorDXT1 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };
#endif

#if defined(HAVE_D3DX)
    struct D3DXCompressorDXT1 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };
#endif

#if defined(HAVE_STB)
    struct StbCompressorDXT1 : public ColorBlockCompressor
    {
        virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize() const { return 8; }
    };
#endif

#if NV_USE_CRUNCH
    struct CrunchCompressorETC1 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };
#endif

#if NV_USE_INTEL_ISPC_TC
    struct IspcCompressorBC1 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };

    struct IspcCompressorBC3 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };

    struct IspcCompressorBC7 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };

    struct IspcCompressorETC1 : public CompressorInterface
    {
        virtual void compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };
#endif

#if defined(HAVE_ETCLIB)
    struct EtcLibCompressor : public CompressorInterface
    {
        virtual void compress(nvtt::AlphaMode alphaMode, uint w, uint h, uint d, const float * data, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };
#endif

#if defined(HAVE_RGETC)
    struct RgEtcCompressor : public ColorBlockCompressor
    {
        virtual void compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output);
        virtual uint blockSize() const { return 8; }
    };
#endif

#if defined(HAVE_PVRTEXTOOL)
    struct CompressorPVR : public CompressorInterface
    {
        virtual void compress(nvtt::AlphaMode alphaMode, uint w, uint h, uint d, const float * data, nvtt::TaskDispatcher * dispatcher, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions);
    };
#endif

} // nv namespace


#endif // NVTT_BLOCKCOMPRESSOR_H
