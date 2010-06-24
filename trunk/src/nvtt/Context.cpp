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

#include "Context.h"

#include <nvtt/nvtt.h>

#include <nvcore/Memory.h>
#include <nvcore/Ptr.h>

#include <nvimage/DirectDrawSurface.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/BlockDXT.h>
#include <nvimage/Image.h>
#include <nvimage/FloatImage.h>
#include <nvimage/Filter.h>
#include <nvimage/Quantize.h>
#include <nvimage/NormalMap.h>
#include <nvimage/PixelFormat.h>
#include <nvimage/ColorSpace.h>

#include "InputOptions.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"
#include "TexImage.h"

#include "CompressorDX9.h"
#include "CompressorDX10.h"
#include "CompressorDX11.h"
#include "CompressorRGB.h"
#include "CompressorRGBE.h"
#include "cuda/CudaUtils.h"
#include "cuda/CudaCompressorDXT.h"


using namespace nv;
using namespace nvtt;


namespace
{

    static int blockSize(Format format)
    {
        if (format == Format_DXT1 || format == Format_DXT1a || format == Format_DXT1n) {
            return 8;
        }
        else if (format == Format_DXT3) {
            return 16;
        }
        else if (format == Format_DXT5 || format == Format_DXT5n) {
            return 16;
        }
        else if (format == Format_BC4) {
            return 8;
        }
        else if (format == Format_BC5) {
            return 16;
        }
        else if (format == Format_CTX1) {
            return 8;
        }
        else if (format == Format_BC6) {
            return 16;
        }
        else if (format == Format_BC7) {
            return 16;
        }
        return 0;
    }

    inline uint computePitch(uint w, uint bitsize)
    {
        uint p = w * ((bitsize + 7) / 8);

        // Align to 32 bits.
        return ((p + 3) / 4) * 4;
    }

    static int computeImageSize(uint w, uint h, uint d, uint bitCount, Format format)
    {
        if (format == Format_RGBA) {
            return d * h * computePitch(w, bitCount);
        }
        else {
            // @@ Handle 3D textures. DXT and VTC have different behaviors.
            return ((w + 3) / 4) * ((h + 3) / 4) * blockSize(format);
        }
    }

} // namespace

namespace nvtt
{
    // Mipmap could be:
    // - a pointer to an input image.
    // - a fixed point image.
    // - a floating point image.
    struct Mipmap
    {
        Mipmap() : m_inputImage(NULL) {}
        ~Mipmap() {}

        // Reference input image.
        void setFromInput(const InputOptions::Private & inputOptions, uint idx)
        {
            m_inputImage = inputOptions.image(idx);
            m_fixedImage = NULL;
            m_floatImage = NULL;

            if (const FloatImage * floatImage = inputOptions.floatImage(idx))
            {
                m_floatImage = floatImage->clone();
            }
        }

        // Assign and take ownership of given image.
        void setImage(FloatImage * image)
        {
            m_inputImage = NULL;
            m_fixedImage = NULL;
            m_floatImage = image;
        }


        // Convert linear float image to fixed image ready for compression.
        void toFixedImage(const InputOptions::Private & inputOptions)
        {
            if (this->asFixedImage() == NULL)
            {
                nvDebugCheck(m_floatImage != NULL);

                if (inputOptions.isNormalMap || inputOptions.outputGamma == 1.0f)
                {
                    m_fixedImage = m_floatImage->createImage();
                }
                else
                {
                    m_fixedImage = m_floatImage->createImageGammaCorrect(inputOptions.outputGamma);
                }
            }
        }

        // Convert input image to linear float image.
        void toFloatImage(const InputOptions::Private & inputOptions)
        {
            if (m_floatImage == NULL)
            {
                nvDebugCheck(this->asFixedImage() != NULL);

                m_floatImage = new FloatImage(this->asFixedImage());

                if (inputOptions.isNormalMap)
                {
                    // Expand normals to [-1, 1] range.
                    //	floatImage->expandNormals(0);
                }
                else if (inputOptions.inputGamma != 1.0f)
                {
                    // Convert to linear space.
                    m_floatImage->toLinear(0, 3, inputOptions.inputGamma);
                }
            }
        }

        const FloatImage * asFloatImage() const
        {
            return m_floatImage.ptr();
        }

        FloatImage * asMutableFloatImage()
        {
            m_inputImage = NULL;
            return m_floatImage.ptr();
        }

        const Image * asFixedImage() const
        {
            if (m_inputImage != NULL)
            {
                return m_inputImage;
            }
            return m_fixedImage.ptr();
        }

        Image * asMutableFixedImage()
        {
            if (m_inputImage != NULL)
            {
                // Do not modify input image, create a copy.
                m_fixedImage = new Image(*m_inputImage);
                m_inputImage = NULL;
            }
            return m_fixedImage.ptr();
        }


    private:
        const Image * m_inputImage;
        AutoPtr<Image> m_fixedImage;
        AutoPtr<FloatImage> m_floatImage;
    };

} // nvtt namespace


Compressor::Compressor() : m(*new Compressor::Private())
{
    // CUDA initialization.
    m.cudaSupported = cuda::isHardwarePresent();
    m.cudaEnabled = m.cudaSupported;

    if (m.cudaEnabled)
    {
#pragma message(NV_FILE_LINE "FIXME: This code is duplicated below.")
        // Select fastest CUDA device.
        int device = cuda::getFastestDevice();
        if (!cuda::setDevice(device))
        {
            m.cudaEnabled = false;
            m.cuda = NULL;
        }
        else
        {
            m.cuda = new CudaContext();

            if (!m.cuda->isValid())
            {
                m.cudaEnabled = false;
                m.cuda = NULL;
            }
        }
    }
}

Compressor::~Compressor()
{
    delete &m;
    cuda::exit();
}


/// Enable CUDA acceleration.
void Compressor::enableCudaAcceleration(bool enable)
{
    if (m.cudaSupported)
    {
        m.cudaEnabled = enable;
    }

    if (m.cudaEnabled && m.cuda == NULL)
    {
        // Select fastest CUDA device.
        int device = cuda::getFastestDevice();
        if (!cuda::setDevice(device))
        {
            m.cudaEnabled = false;
            m.cuda = NULL;
        }
        else
        {
            m.cuda = new CudaContext();

            if (!m.cuda->isValid())
            {
                m.cudaEnabled = false;
                m.cuda = NULL;
            }
        }
    }
}

/// Return true if CUDA acceleration is enabled, false otherwise.
bool Compressor::isCudaAccelerationEnabled() const
{
    return m.cudaEnabled;
}


/// Compress the input texture with the given compression options.
bool Compressor::process(const InputOptions & inputOptions, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const
{
    return m.compress(inputOptions.m, compressionOptions.m, outputOptions.m);
}

/// Estimate the size of compressing the input with the given options.
int Compressor::estimateSize(const InputOptions & inputOptions, const CompressionOptions & compressionOptions) const
{
    return m.estimateSize(inputOptions.m, compressionOptions.m);
}


// RAW api.
bool Compressor::compress2D(InputFormat format, int w, int h, void * data, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const
{
    return m.compress2D(format, AlphaMode_None, w, h, data, compressionOptions.m, outputOptions.m);
}

int Compressor::estimateSize(int w, int h, int d, const CompressionOptions & compressionOptions) const
{
    const CompressionOptions::Private & co = compressionOptions.m;

    const Format format = co.format;

    uint bitCount = co.getBitCount();

    return computeImageSize(w, h, d, bitCount, format);
}



/// Create a TexImage.
TexImage Compressor::createTexImage() const
{
    return *new TexImage();
}


bool Compressor::outputHeader(const TexImage & tex, int mipmapCount, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const
{
    return m.outputHeader(tex, mipmapCount, compressionOptions.m, outputOptions.m);
}

bool Compressor::compress(const TexImage & tex, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const
{
    foreach(i, tex.m->imageArray) {
        FloatImage * image = tex.m->imageArray[i];
        if (!m.compress2D(InputFormat_RGBA_32F, tex.m->alphaMode, image->width(), image->height(), image->channel(0), compressionOptions.m, outputOptions.m)) {
            return false;
        }
    }

    return true;
}

/// Estimate the size of compressing the given texture.
int Compressor::estimateSize(const TexImage & tex, const CompressionOptions & compressionOptions) const
{
    const uint w = tex.width();
    const uint h = tex.height();
    const uint d = tex.depth();
    const uint faceCount = tex.faceCount();

    return faceCount * estimateSize(w, h, d, compressionOptions);
}




bool Compressor::Private::compress(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
    // Make sure enums match.
    nvStaticCheck(FloatImage::WrapMode_Clamp == (FloatImage::WrapMode)WrapMode_Clamp);
    nvStaticCheck(FloatImage::WrapMode_Mirror == (FloatImage::WrapMode)WrapMode_Mirror);
    nvStaticCheck(FloatImage::WrapMode_Repeat == (FloatImage::WrapMode)WrapMode_Repeat);

    // Get output handler.
    if (!outputOptions.hasValidOutputHandler())
    {
        outputOptions.error(Error_FileOpen);
        return false;
    }

    inputOptions.computeTargetExtents();

    // Output DDS header.
    if (!outputHeader(inputOptions, compressionOptions, outputOptions))
    {
        return false;
    }

    for (uint f = 0; f < inputOptions.faceCount; f++)
    {
        if (!compressMipmaps(f, inputOptions, compressionOptions, outputOptions))
        {
            return false;
        }
    }

    return true;
}


// Output DDS header.
bool Compressor::Private::outputHeader(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
    // Output DDS header.
    if (outputOptions.outputHandler == NULL || !outputOptions.outputHeader)
    {
        return true;
    }

    if (outputOptions.container == Container_DDS || outputOptions.container == Container_DDS10)
    {
        DDSHeader header;
	
        header.setWidth(inputOptions.targetWidth);
        header.setHeight(inputOptions.targetHeight);

        int mipmapCount = inputOptions.realMipmapCount();
        nvDebugCheck(mipmapCount > 0);

        header.setMipmapCount(mipmapCount);

        bool supported = true;

        if (outputOptions.container == Container_DDS10)
        {
            if (compressionOptions.format == Format_RGBA)
            {
                int bitcount = compressionOptions.bitcount;
                if (bitcount == 0) {
                    bitcount = compressionOptions.rsize + compressionOptions.gsize + compressionOptions.bsize + compressionOptions.asize;
                }

                if (bitcount == 16)
                {
                    if (compressionOptions.rsize == 16)
                    {
                        header.setDX10Format(56); // R16_UNORM
                    }
                    else
                    {
                        // B5G6R5_UNORM
                        // B5G5R5A1_UNORM
                        supported = false;
                    }
                }
                else if (bitcount == 32)
                {
                    // B8G8R8A8_UNORM
                    // B8G8R8X8_UNORM
                    // R8G8B8A8_UNORM
                    // R10G10B10A2_UNORM
                    supported = false;
                }
                else {
                    supported = false;
                }
            }
            else
            {
                if (compressionOptions.format == Format_DXT1 || compressionOptions.format == Format_DXT1a || compressionOptions.format == Format_DXT1n) {
                    header.setDX10Format(70); // DXGI_FORMAT_BC1_TYPELESS
                    if (compressionOptions.format == Format_DXT1a) header.setHasAlphaFlag(true);
                    if (inputOptions.isNormalMap) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_DXT3) {
                    header.setDX10Format(73); // DXGI_FORMAT_BC2_TYPELESS
                }
                else if (compressionOptions.format == Format_DXT5) {
                    header.setDX10Format(76); // DXGI_FORMAT_BC3_TYPELESS
                }
                else if (compressionOptions.format == Format_DXT5n) {
                    header.setDX10Format(76); // DXGI_FORMAT_BC3_TYPELESS
                    if (inputOptions.isNormalMap) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_BC4) {
                    header.setDX10Format(79); // DXGI_FORMAT_BC4_TYPELESS
                }
                else if (compressionOptions.format == Format_BC5) {
                    header.setDX10Format(82); // DXGI_FORMAT_BC5_TYPELESS
                    if (inputOptions.isNormalMap) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_BC6) {
                    header.setDX10Format(94); // DXGI_FORMAT_BC6H_TYPELESS
                }
                else if (compressionOptions.format == Format_BC7) {
                    header.setDX10Format(97); // DXGI_FORMAT_BC7_TYPELESS
                    if (inputOptions.isNormalMap) header.setNormalFlag(true);
                }
                else {
                    supported = false;
                }
            }
        }
        else
        {
            if (compressionOptions.format == Format_RGBA)
            {
                // Get output bit count.
                header.setPitch(computePitch(inputOptions.targetWidth, compressionOptions.getBitCount()));

                if (compressionOptions.pixelType == PixelType_Float)
                {
                    if (compressionOptions.rsize == 16 && compressionOptions.gsize == 0 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(111); // D3DFMT_R16F
                    }
                    else if (compressionOptions.rsize == 16 && compressionOptions.gsize == 16 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(112); // D3DFMT_G16R16F
                    }
                    else if (compressionOptions.rsize == 16 && compressionOptions.gsize == 16 && compressionOptions.bsize == 16 && compressionOptions.asize == 16)
                    {
                        header.setFormatCode(113); // D3DFMT_A16B16G16R16F
                    }
                    else if (compressionOptions.rsize == 32 && compressionOptions.gsize == 0 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(114); // D3DFMT_R32F
                    }
                    else if (compressionOptions.rsize == 32 && compressionOptions.gsize == 32 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(115); // D3DFMT_G32R32F
                    }
                    else if (compressionOptions.rsize == 32 && compressionOptions.gsize == 32 && compressionOptions.bsize == 32 && compressionOptions.asize == 32)
                    {
                        header.setFormatCode(116); // D3DFMT_A32B32G32R32F
                    }
                    else
                    {
                        supported = false;
                    }
                }
                else // Fixed point
                {
                    const uint bitcount = compressionOptions.getBitCount();

                    if (compressionOptions.bitcount != 0)
                    {
                        // Masks already computed.
                        header.setPixelFormat(compressionOptions.bitcount, compressionOptions.rmask, compressionOptions.gmask, compressionOptions.bmask, compressionOptions.amask);
                    }
                    else if (bitcount <= 32)
                    {
                        // Compute pixel format masks.
                        const uint ashift = 0;
                        const uint bshift = ashift + compressionOptions.asize;
                        const uint gshift = bshift + compressionOptions.bsize;
                        const uint rshift = gshift + compressionOptions.gsize;

                        const uint rmask = ((1 << compressionOptions.rsize) - 1) << rshift;
                        const uint gmask = ((1 << compressionOptions.gsize) - 1) << gshift;
                        const uint bmask = ((1 << compressionOptions.bsize) - 1) << bshift;
                        const uint amask = ((1 << compressionOptions.asize) - 1) << ashift;

                        header.setPixelFormat(bitcount, rmask, gmask, bmask, amask);
                    }
                    else
                    {
                        supported = false;
                    }
                }
            }
            else
            {
                header.setLinearSize(computeImageSize(inputOptions.targetWidth, inputOptions.targetHeight, inputOptions.targetDepth, compressionOptions.bitcount, compressionOptions.format));

                if (compressionOptions.format == Format_DXT1 || compressionOptions.format == Format_DXT1a || compressionOptions.format == Format_DXT1n) {
                    header.setFourCC('D', 'X', 'T', '1');
                    if (inputOptions.isNormalMap) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_DXT3) {
                    header.setFourCC('D', 'X', 'T', '3');
                }
                else if (compressionOptions.format == Format_DXT5) {
                    header.setFourCC('D', 'X', 'T', '5');
                }
                else if (compressionOptions.format == Format_DXT5n) {
                    header.setFourCC('D', 'X', 'T', '5');
                    if (inputOptions.isNormalMap) {
                        header.setNormalFlag(true);
                        header.setSwizzleCode('A', '2', 'D', '5');
                        //header.setSwizzleCode('x', 'G', 'x', 'R');
                    }
                }
                else if (compressionOptions.format == Format_BC4) {
                    header.setFourCC('A', 'T', 'I', '1');
                }
                else if (compressionOptions.format == Format_BC5) {
                    header.setFourCC('A', 'T', 'I', '2');
                    if (inputOptions.isNormalMap) {
                        header.setNormalFlag(true);
                        header.setSwizzleCode('A', '2', 'X', 'Y');
                    }
                }
                else if (compressionOptions.format == Format_BC6) {
                    header.setFourCC('Z', 'O', 'H', ' ');
                }
                else if (compressionOptions.format == Format_BC7) {
                    header.setFourCC('Z', 'O', 'L', 'A');
                    if (inputOptions.isNormalMap) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_CTX1) {
                    header.setFourCC('C', 'T', 'X', '1');
                    if (inputOptions.isNormalMap) header.setNormalFlag(true);
                }
                else {
                    supported = false;
                }
            }
        }

        if (!supported)
        {
            // This container does not support the requested format.
            outputOptions.error(Error_UnsupportedOutputFormat);
            return false;
        }

        if (inputOptions.textureType == TextureType_2D) {
            header.setTexture2D();
        }
        else if (inputOptions.textureType == TextureType_Cube) {
            header.setTextureCube();
        }
        /*else if (inputOptions.textureType == TextureType_3D) {
            header.setTexture3D();
            header.setDepth(inputOptions.targetDepth);
        }*/

        // Swap bytes if necessary.
        header.swapBytes();

        uint headerSize = 128;
        if (header.hasDX10Header())
        {
            nvStaticCheck(sizeof(DDSHeader) == 128 + 20);
            headerSize = 128 + 20;
        }

        bool writeSucceed = outputOptions.outputHandler->writeData(&header, headerSize);
        if (!writeSucceed)
        {
            outputOptions.error(Error_FileWrite);
        }

        return writeSucceed;
    }

    return true;
}

bool Compressor::Private::outputHeader(const TexImage & tex, int mipmapCount, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions)
{
    if (tex.width() <= 0 || tex.height() <= 0 || tex.depth() <= 0 || mipmapCount <= 0)
    {
        outputOptions.error(Error_InvalidInput);
        return false;
    }

    if (!outputOptions.outputHeader)
    {
        return true;
    }

    // Output DDS header.
    if (outputOptions.container == Container_DDS || outputOptions.container == Container_DDS10)
    {
        DDSHeader header;

        header.setWidth(tex.width());
        header.setHeight(tex.height());
        header.setMipmapCount(mipmapCount);

        bool supported = true;

        if (outputOptions.container == Container_DDS10)
        {
            if (compressionOptions.format == Format_RGBA)
            {
                int bitcount = compressionOptions.bitcount;
                if (bitcount == 0) {
                    bitcount = compressionOptions.rsize + compressionOptions.gsize + compressionOptions.bsize + compressionOptions.asize;
                }

                if (bitcount == 16)
                {
                    if (compressionOptions.rsize == 16)
                    {
                        header.setDX10Format(56); // R16_UNORM
                    }
                    else
                    {
                        // B5G6R5_UNORM
                        // B5G5R5A1_UNORM
                        supported = false;
                    }
                }
                else if (bitcount == 32)
                {
                    // B8G8R8A8_UNORM
                    // B8G8R8X8_UNORM
                    // R8G8B8A8_UNORM
                    // R10G10B10A2_UNORM
                    supported = false;
                }
                else {
                    supported = false;
                }
            }
            else
            {
                if (compressionOptions.format == Format_DXT1 || compressionOptions.format == Format_DXT1a || compressionOptions.format == Format_DXT1n) {
                    header.setDX10Format(70); // DXGI_FORMAT_BC1_TYPELESS
                    if (compressionOptions.format == Format_DXT1a) header.setHasAlphaFlag(true);
                    if (tex.isNormalMap()) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_DXT3) {
                    header.setDX10Format(73); // DXGI_FORMAT_BC2_TYPELESS
                }
                else if (compressionOptions.format == Format_DXT5) {
                    header.setDX10Format(76); // DXGI_FORMAT_BC3_TYPELESS
                }
                else if (compressionOptions.format == Format_DXT5n) {
                    header.setDX10Format(76); // DXGI_FORMAT_BC3_TYPELESS
                    if (tex.isNormalMap()) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_BC4) {
                    header.setDX10Format(79); // DXGI_FORMAT_BC4_TYPELESS
                }
                else if (compressionOptions.format == Format_BC5) {
                    header.setDX10Format(82); // DXGI_FORMAT_BC5_TYPELESS
                    if (tex.isNormalMap()) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_BC6) {
                    header.setDX10Format(94); // DXGI_FORMAT_BC6H_TYPELESS
                }
                else if (compressionOptions.format == Format_BC7) {
                    header.setDX10Format(97); // DXGI_FORMAT_BC7_TYPELESS
                    if (tex.isNormalMap()) header.setNormalFlag(true);
                }
                else {
                    supported = false;
                }
            }
        }
        else
        {
            if (compressionOptions.format == Format_RGBA)
            {
                // Get output bit count.
                header.setPitch(computePitch(tex.width(), compressionOptions.getBitCount()));

                if (compressionOptions.pixelType == PixelType_Float)
                {
                    if (compressionOptions.rsize == 16 && compressionOptions.gsize == 0 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(111); // D3DFMT_R16F
                    }
                    else if (compressionOptions.rsize == 16 && compressionOptions.gsize == 16 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(112); // D3DFMT_G16R16F
                    }
                    else if (compressionOptions.rsize == 16 && compressionOptions.gsize == 16 && compressionOptions.bsize == 16 && compressionOptions.asize == 16)
                    {
                        header.setFormatCode(113); // D3DFMT_A16B16G16R16F
                    }
                    else if (compressionOptions.rsize == 32 && compressionOptions.gsize == 0 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(114); // D3DFMT_R32F
                    }
                    else if (compressionOptions.rsize == 32 && compressionOptions.gsize == 32 && compressionOptions.bsize == 0 && compressionOptions.asize == 0)
                    {
                        header.setFormatCode(115); // D3DFMT_G32R32F
                    }
                    else if (compressionOptions.rsize == 32 && compressionOptions.gsize == 32 && compressionOptions.bsize == 32 && compressionOptions.asize == 32)
                    {
                        header.setFormatCode(116); // D3DFMT_A32B32G32R32F
                    }
                    else
                    {
                        supported = false;
                    }
                }
                else // Fixed point
                {
                    const uint bitcount = compressionOptions.getBitCount();

                    if (compressionOptions.bitcount != 0)
                    {
                        // Masks already computed.
                        header.setPixelFormat(compressionOptions.bitcount, compressionOptions.rmask, compressionOptions.gmask, compressionOptions.bmask, compressionOptions.amask);
                    }
                    else if (bitcount <= 32)
                    {
                        // Compute pixel format masks.
                        const uint ashift = 0;
                        const uint bshift = ashift + compressionOptions.asize;
                        const uint gshift = bshift + compressionOptions.bsize;
                        const uint rshift = gshift + compressionOptions.gsize;

                        const uint rmask = ((1 << compressionOptions.rsize) - 1) << rshift;
                        const uint gmask = ((1 << compressionOptions.gsize) - 1) << gshift;
                        const uint bmask = ((1 << compressionOptions.bsize) - 1) << bshift;
                        const uint amask = ((1 << compressionOptions.asize) - 1) << ashift;

                        header.setPixelFormat(bitcount, rmask, gmask, bmask, amask);
                    }
                    else
                    {
                        supported = false;
                    }
                }
            }
            else
            {
                header.setLinearSize(computeImageSize(tex.width(), tex.height(), tex.depth(), compressionOptions.bitcount, compressionOptions.format));

                if (compressionOptions.format == Format_DXT1 || compressionOptions.format == Format_DXT1a || compressionOptions.format == Format_DXT1n) {
                    header.setFourCC('D', 'X', 'T', '1');
                    if (tex.isNormalMap()) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_DXT3) {
                    header.setFourCC('D', 'X', 'T', '3');
                }
                else if (compressionOptions.format == Format_DXT5) {
                    header.setFourCC('D', 'X', 'T', '5');
                }
                else if (compressionOptions.format == Format_DXT5n) {
                    header.setFourCC('D', 'X', 'T', '5');
                    if (tex.isNormalMap()) {
                        header.setNormalFlag(true);
                        header.setSwizzleCode('A', '2', 'D', '5');
                        //header.setSwizzleCode('x', 'G', 'x', 'R');
                    }
                }
                else if (compressionOptions.format == Format_BC4) {
                    header.setFourCC('A', 'T', 'I', '1');
                }
                else if (compressionOptions.format == Format_BC5) {
                    header.setFourCC('A', 'T', 'I', '2');
                    if (tex.isNormalMap()) {
                        header.setNormalFlag(true);
                        header.setSwizzleCode('A', '2', 'X', 'Y');
                    }
                }
                else if (compressionOptions.format == Format_BC6) {
                    header.setFourCC('Z', 'O', 'H', ' ');
                }
                else if (compressionOptions.format == Format_BC7) {
                    header.setFourCC('Z', 'O', 'L', 'A');
                    if (tex.isNormalMap()) header.setNormalFlag(true);
                }
                else if (compressionOptions.format == Format_CTX1) {
                    header.setFourCC('C', 'T', 'X', '1');
                    if (tex.isNormalMap()) header.setNormalFlag(true);
                }
                else {
                    supported = false;
                }
            }
        }

        if (!supported)
        {
            // This container does not support the requested format.
            outputOptions.error(Error_UnsupportedOutputFormat);
            return false;
        }

        if (tex.textureType() == TextureType_2D) {
            header.setTexture2D();
        }
        else if (tex.textureType() == TextureType_Cube) {
            header.setTextureCube();
        }
        /*else if (tex.textureType() == TextureType_3D) {
            header.setTexture3D();
            header.setDepth(tex.depth());
        }*/

        // Swap bytes if necessary.
        header.swapBytes();

        uint headerSize = 128;
        if (header.hasDX10Header())
        {
            nvStaticCheck(sizeof(DDSHeader) == 128 + 20);
            headerSize = 128 + 20;
        }

        bool writeSucceed = outputOptions.outputHandler->writeData(&header, headerSize);
        if (!writeSucceed)
        {
            outputOptions.error(Error_FileWrite);
        }

        return writeSucceed;
    }

    return true;
}


bool Compressor::Private::compressMipmaps(uint f, const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
    uint w = inputOptions.targetWidth;
    uint h = inputOptions.targetHeight;
    uint d = inputOptions.targetDepth;

    Mipmap mipmap;

    const uint mipmapCount = inputOptions.realMipmapCount();
    nvDebugCheck(mipmapCount > 0);

    for (uint m = 0; m < mipmapCount; m++)
    {
        int size = computeImageSize(w, h, d, compressionOptions.getBitCount(), compressionOptions.format);
        outputOptions.beginImage(size, w, h, d, f, m);

        if (!initMipmap(mipmap, inputOptions, w, h, d, f, m))
        {
            outputOptions.error(Error_InvalidInput);
            return false;
        }

        if (compressionOptions.pixelType == PixelType_Float)
        {
            mipmap.toFloatImage(inputOptions);

            // @@ Convert to linear space.

            // @@ HACK
            const FloatImage * image = mipmap.asFloatImage();
            nvDebugCheck(image != NULL);

            // @@ Ignore return value?
            compress2D(InputFormat_RGBA_32F, inputOptions.alphaMode, image->width(), image->height(), image->channel(0), compressionOptions, outputOptions);
        }
        else
        {
            // Convert linear float image to fixed image ready for compression.
            mipmap.toFixedImage(inputOptions);

            if (inputOptions.premultiplyAlpha)
            {
                premultiplyAlphaMipmap(mipmap, inputOptions);
            }

#pragma message(NV_FILE_LINE "TODO: All color transforms should be done here!")

            // Apply gamma space color transforms:
            if (inputOptions.colorTransform == ColorTransform_YCoCg)
            {
                ColorSpace::RGBtoYCoCg_R(mipmap.asMutableFixedImage());
            }
            else if (inputOptions.colorTransform == ColorTransform_ScaledYCoCg)
            {
                // @@ TODO
                //ColorSpace::RGBtoYCoCg_R(mipmap.asMutableFixedImage());
            }

            /*// Apply linear transforms in linear space.
            if (inputOptions.colorTransform == ColorTransform_Linear)
            {
                FloatImage * image = mipmap.asMutableFloatImage();
                nvDebugCheck(image != NULL);

                Vector4 offset(
                    inputOptions.colorOffsets[0],
                    inputOptions.colorOffsets[1],
                    inputOptions.colorOffsets[2],
                    inputOptions.colorOffsets[3]);

                image->transform(0, inputOptions.linearTransform, offset);
            }
            else if (inputOptions.colorTransform == ColorTransform_Swizzle)
            {
                FloatImage * image = mipmap.asMutableFloatImage();
                nvDebugCheck(image != NULL);

                image->swizzle(0, inputOptions.swizzleTransform[0], inputOptions.swizzleTransform[1], inputOptions.swizzleTransform[2], inputOptions.swizzleTransform[3]);
            }*/

            quantizeMipmap(mipmap, compressionOptions);

            const Image * image = mipmap.asFixedImage();
            nvDebugCheck(image != NULL);

            // @@ Ignore return value?
            compress2D(InputFormat_BGRA_8UB, inputOptions.alphaMode, image->width(), image->height(), image->pixels(), compressionOptions, outputOptions);
        }




        // Compute extents of next mipmap:
        w = max(1U, w / 2);
        h = max(1U, h / 2);
        d = max(1U, d / 2);
    }

    return true;
}

bool Compressor::Private::initMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f, uint m) const
{
    // Find image from input.
    int inputIdx = findExactMipmap(inputOptions, w, h, d, f);

    if ((inputIdx == -1 || inputOptions.convertToNormalMap) && m != 0)
    {
        // Generate from last, when mipmap not found, or normal map conversion enabled.
        downsampleMipmap(mipmap, inputOptions);
    }
    else
    {
        if (inputIdx != -1)
        {
            // If input mipmap found, then get from input.
            mipmap.setFromInput(inputOptions, inputIdx);
        }
        else
        {
            // If not found, resize closest mipmap.
            inputIdx = findClosestMipmap(inputOptions, w, h, d, f);

            if (inputIdx == -1)
            {
                return false;
            }

            mipmap.setFromInput(inputOptions, inputIdx);

            scaleMipmap(mipmap, inputOptions, w, h, d);
        }

        processInputImage(mipmap, inputOptions);
    }

    return true;
}

int Compressor::Private::findExactMipmap(const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f) const
{
    for (int m = 0; m < int(inputOptions.mipmapCount); m++)
    {
        int idx = f * inputOptions.mipmapCount + m;
        const InputOptions::Private::InputImage & inputImage = inputOptions.images[idx];

        if (inputImage.width == int(w) && inputImage.height == int(h) && inputImage.depth == int(d))
        {
            if (inputImage.hasValidData())
            {
                return idx;
            }
            return -1;
        }
        else if (inputImage.width < int(w) || inputImage.height < int(h) || inputImage.depth < int(d))
        {
            return -1;
        }
    }

    return -1;
}

int Compressor::Private::findClosestMipmap(const InputOptions::Private & inputOptions, uint w, uint h, uint d, uint f) const
{
    int bestIdx = -1;

    for (int m = 0; m < int(inputOptions.mipmapCount); m++)
    {
        int idx = f * inputOptions.mipmapCount + m;
        const InputOptions::Private::InputImage & inputImage = inputOptions.images[idx];

        if (inputImage.hasValidData())
        {
            int difference = (inputImage.width - w) + (inputImage.height - h) + (inputImage.depth - d);

            if (difference < 0)
            {
                if (bestIdx == -1)
                {
                    bestIdx = idx;
                }

                return bestIdx;
            }

            bestIdx = idx;
        }
    }

    return bestIdx;
}

// Create mipmap from the given image.
void Compressor::Private::downsampleMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions) const
{
    // Make sure that floating point linear representation is available.
    mipmap.toFloatImage(inputOptions);

    const FloatImage * floatImage = mipmap.asFloatImage();
    FloatImage::WrapMode wrapMode = (FloatImage::WrapMode)inputOptions.wrapMode;

    if (inputOptions.alphaMode == AlphaMode_Transparency)
    {
        if (inputOptions.mipmapFilter == MipmapFilter_Box)
        {
            BoxFilter filter;
            mipmap.setImage(floatImage->downSample(filter, wrapMode, 3));
        }
        else if (inputOptions.mipmapFilter == MipmapFilter_Triangle)
        {
            TriangleFilter filter;
            mipmap.setImage(floatImage->downSample(filter, wrapMode, 3));
        }
        else /*if (inputOptions.mipmapFilter == MipmapFilter_Kaiser)*/
        {
            nvDebugCheck(inputOptions.mipmapFilter == MipmapFilter_Kaiser);
            KaiserFilter filter(inputOptions.kaiserWidth);
            filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
            mipmap.setImage(floatImage->downSample(filter, wrapMode, 3));
        }
    }
    else
    {
        if (inputOptions.mipmapFilter == MipmapFilter_Box)
        {
            // Use fast downsample.
            mipmap.setImage(floatImage->fastDownSample());
        }
        else if (inputOptions.mipmapFilter == MipmapFilter_Triangle)
        {
            TriangleFilter filter;
            mipmap.setImage(floatImage->downSample(filter, wrapMode));
        }
        else /*if (inputOptions.mipmapFilter == MipmapFilter_Kaiser)*/
        {
            nvDebugCheck(inputOptions.mipmapFilter == MipmapFilter_Kaiser);
            KaiserFilter filter(inputOptions.kaiserWidth);
            filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
            mipmap.setImage(floatImage->downSample(filter, wrapMode));
        }
    }

    // Normalize mipmap.
    if ((inputOptions.isNormalMap || inputOptions.convertToNormalMap) && inputOptions.normalizeMipmaps)
    {
        normalizeNormalMap(mipmap.asMutableFloatImage());
    }
}


void Compressor::Private::scaleMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions, uint w, uint h, uint d) const
{
    mipmap.toFloatImage(inputOptions);

    // @@ Add more filters.
    // @@ Select different filters for downscaling and reconstruction.

    // Resize image.
    BoxFilter boxFilter;

    if (inputOptions.alphaMode == AlphaMode_Transparency)
    {
        mipmap.setImage(mipmap.asFloatImage()->resize(boxFilter, w, h, (FloatImage::WrapMode)inputOptions.wrapMode, 3));
    }
    else
    {
        mipmap.setImage(mipmap.asFloatImage()->resize(boxFilter, w, h, (FloatImage::WrapMode)inputOptions.wrapMode));
    }
}


void Compressor::Private::premultiplyAlphaMipmap(Mipmap & mipmap, const InputOptions::Private & inputOptions) const
{
    nvDebugCheck(mipmap.asFixedImage() != NULL);

    Image * image = mipmap.asMutableFixedImage();

    const uint w = image->width();
    const uint h = image->height();

    const uint count = w * h;

    for (uint i = 0; i < count; ++i)
    {
        Color32 c = image->pixel(i);

        c.r = (uint(c.r) * uint(c.a)) >> 8;
        c.g = (uint(c.g) * uint(c.a)) >> 8;
        c.b = (uint(c.b) * uint(c.a)) >> 8;

        image->pixel(i) = c;
    }
}

// Process an input image: Convert to normal map, normalize, or convert to linear space.
void Compressor::Private::processInputImage(Mipmap & mipmap, const InputOptions::Private & inputOptions) const
{
    if (inputOptions.convertToNormalMap)
    {
        mipmap.toFixedImage(inputOptions);

        Vector4 heightScale = inputOptions.heightFactors;
        mipmap.setImage(createNormalMap(mipmap.asFixedImage(), (FloatImage::WrapMode)inputOptions.wrapMode, heightScale, inputOptions.bumpFrequencyScale));
    }
    else if (inputOptions.isNormalMap)
    {
        if (inputOptions.normalizeMipmaps)
        {
            // If floating point image available, normalize in place.
            if (mipmap.asFloatImage() == NULL)
            {
                FloatImage * floatImage = new FloatImage(mipmap.asFixedImage());
                normalizeNormalMap(floatImage);
                mipmap.setImage(floatImage);
            }
            else
            {
                normalizeNormalMap(mipmap.asMutableFloatImage());
                mipmap.setImage(mipmap.asMutableFloatImage());
            }
        }
    }
    else
    {
        if (inputOptions.inputGamma != inputOptions.outputGamma ||
            inputOptions.colorTransform == ColorTransform_Linear ||
            inputOptions.colorTransform == ColorTransform_Swizzle)
        {
            mipmap.toFloatImage(inputOptions);
        }

#pragma message(NV_FILE_LINE "FIXME: Do not perform color transforms here!")

        /*// Apply linear transforms in linear space.
        if (inputOptions.colorTransform == ColorTransform_Linear)
        {
            FloatImage * image = mipmap.asMutableFloatImage();
            nvDebugCheck(image != NULL);

            Vector4 offset(
                inputOptions.colorOffsets[0],
                inputOptions.colorOffsets[1],
                inputOptions.colorOffsets[2],
                inputOptions.colorOffsets[3]);

            image->transform(0, inputOptions.linearTransform, offset);
        }
        else if (inputOptions.colorTransform == ColorTransform_Swizzle)
        {
            FloatImage * image = mipmap.asMutableFloatImage();
            nvDebugCheck(image != NULL);

            image->swizzle(0, inputOptions.swizzleTransform[0], inputOptions.swizzleTransform[1], inputOptions.swizzleTransform[2], inputOptions.swizzleTransform[3]);
        }*/
    }
}


// Quantize the given mipmap according to the compression options.
void Compressor::Private::quantizeMipmap(Mipmap & mipmap, const CompressionOptions::Private & compressionOptions) const
{
    nvDebugCheck(mipmap.asFixedImage() != NULL);

    if (compressionOptions.binaryAlpha)
    {
        if (compressionOptions.enableAlphaDithering)
        {
            Quantize::FloydSteinberg_BinaryAlpha(mipmap.asMutableFixedImage(), compressionOptions.alphaThreshold);
        }
        else
        {
            Quantize::BinaryAlpha(mipmap.asMutableFixedImage(), compressionOptions.alphaThreshold);
        }
    }

    if (compressionOptions.enableColorDithering || compressionOptions.enableAlphaDithering)
    {
        uint rsize = 8;
        uint gsize = 8;
        uint bsize = 8;
        uint asize = 8;

        if (compressionOptions.enableColorDithering)
        {
            if (compressionOptions.format >= Format_DXT1 && compressionOptions.format <= Format_DXT5)
            {
                rsize = 5;
                gsize = 6;
                bsize = 5;
            }
            else if (compressionOptions.format == Format_RGB)
            {
                uint rshift, gshift, bshift;
                PixelFormat::maskShiftAndSize(compressionOptions.rmask, &rshift, &rsize);
                PixelFormat::maskShiftAndSize(compressionOptions.gmask, &gshift, &gsize);
                PixelFormat::maskShiftAndSize(compressionOptions.bmask, &bshift, &bsize);
            }
        }

        if (compressionOptions.enableAlphaDithering)
        {
            if (compressionOptions.format == Format_DXT3)
            {
                asize = 4;
            }
            else if (compressionOptions.format == Format_RGB)
            {
                uint ashift;
                PixelFormat::maskShiftAndSize(compressionOptions.amask, &ashift, &asize);
            }
        }

        if (compressionOptions.binaryAlpha)
        {
            asize = 8; // Already quantized.
        }

        Quantize::FloydSteinberg(mipmap.asMutableFixedImage(), rsize, gsize, bsize, asize);
    }
}


CompressorInterface * Compressor::Private::chooseCpuCompressor(const CompressionOptions::Private & compressionOptions) const
{
    if (compressionOptions.format == Format_RGB)
    {
        return new PixelFormatConverter;
    }
    else if (compressionOptions.format == Format_DXT1)
    {
#if defined(HAVE_S3QUANT)
        if (compressionOptions.externalCompressor == "s3") return new S3CompressorDXT1;
        else
#endif

#if defined(HAVE_ATITC)
            if (compressionOptions.externalCompressor == "ati") return new AtiCompressorDXT1;
        else
#endif

#if defined(HAVE_SQUISH)
            if (compressionOptions.externalCompressor == "squish") return new SquishCompressorDXT1;
        else
#endif

#if defined(HAVE_D3DX)
            if (compressionOptions.externalCompressor == "d3dx") return new D3DXCompressorDXT1;
        else
#endif

#if defined(HAVE_D3DX)
            if (compressionOptions.externalCompressor == "stb") return new StbCompressorDXT1;
        else
#endif

            if (compressionOptions.quality == Quality_Fastest)
            {
            return new FastCompressorDXT1;
        }

        return new NormalCompressorDXT1;
    }
    else if (compressionOptions.format == Format_DXT1a)
    {
        if (compressionOptions.quality == Quality_Fastest)
        {
            return new FastCompressorDXT1a;
        }

        return new NormalCompressorDXT1a;
    }
    else if (compressionOptions.format == Format_DXT1n)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_DXT3)
    {
        if (compressionOptions.quality == Quality_Fastest)
        {
            return new FastCompressorDXT3;
        }

        return new NormalCompressorDXT3;
    }
    else if (compressionOptions.format == Format_DXT5)
    {
#if defined(HAVE_ATITC)
        if (compressionOptions.externalCompressor == "ati") return new AtiCompressorDXT5;
        else
#endif

            if (compressionOptions.quality == Quality_Fastest)
            {
            return new FastCompressorDXT5;
        }

        return new NormalCompressorDXT5;
    }
    else if (compressionOptions.format == Format_DXT5n)
    {
        if (compressionOptions.quality == Quality_Fastest)
        {
            return new FastCompressorDXT5n;
        }

        return new NormalCompressorDXT5n;
    }
    else if (compressionOptions.format == Format_BC4)
    {
        if (compressionOptions.quality == Quality_Fastest || compressionOptions.quality == Quality_Normal)
        {
            return new FastCompressorBC4;
        }

        return new ProductionCompressorBC4;
    }
    else if (compressionOptions.format == Format_BC5)
    {
        if (compressionOptions.quality == Quality_Fastest || compressionOptions.quality == Quality_Normal)
        {
            return new FastCompressorBC5;
        }

        return new ProductionCompressorBC5;
    }
    else if (compressionOptions.format == Format_CTX1)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_BC6)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_BC7)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_RGBE)
    {
        return new CompressorRGBE;
    }

    return NULL;
}


CompressorInterface * Compressor::Private::chooseGpuCompressor(const CompressionOptions::Private & compressionOptions) const
{
    nvDebugCheck(cudaSupported);

    if (compressionOptions.quality == Quality_Fastest)
    {
        // Do not use CUDA compressors in fastest quality mode.
        return NULL;
    }

#if defined HAVE_CUDA
    if (compressionOptions.format == Format_DXT1)
    {
        return new CudaCompressorDXT1(*cuda);
    }
    else if (compressionOptions.format == Format_DXT1a)
    {
#pragma message(NV_FILE_LINE "TODO: Implement CUDA DXT1a compressor.")
    }
    else if (compressionOptions.format == Format_DXT1n)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_DXT3)
    {
        //return new CudaCompressorDXT3(*cuda);
    }
    else if (compressionOptions.format == Format_DXT5)
    {
        //return new CudaCompressorDXT5(*cuda);
    }
    else if (compressionOptions.format == Format_DXT5n)
    {
        // @@ Return CUDA compressor.
    }
    else if (compressionOptions.format == Format_BC4)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_BC5)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_CTX1)
    {
        // @@ Return CUDA compressor.
    }
    else if (compressionOptions.format == Format_BC6)
    {
        // Not supported.
    }
    else if (compressionOptions.format == Format_BC7)
    {
        // Not supported.
    }
#endif // defined HAVE_CUDA

    return NULL;
}



// Compress the given mipmap.
/*bool Compressor::Private::compressMipmap(const Mipmap & mipmap, const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
    // @@ This is broken, we should not assume the fixed image is the valid one. @@ Compressor should do conversion if necessary.
    const Image * image = mipmap.asFixedImage();
    nvDebugCheck(image != NULL);

    return compress2D(InputFormat_BGRA_8UB, inputOptions.alphaMode, image->width(), image->height(), image->pixels(), compressionOptions, outputOptions);
}*/

bool Compressor::Private::compress2D(InputFormat inputFormat, AlphaMode alphaMode, int w, int h, const void * data, const CompressionOptions::Private & compressionOptions, const OutputOptions::Private & outputOptions) const
{
    // Decide what compressor to use.
    AutoPtr<CompressorInterface> compressor;
#if defined HAVE_CUDA
    if (cudaEnabled && w * h >= 512)
    {
        compressor = chooseGpuCompressor(compressionOptions);
    }
#endif
    if (compressor == NULL)
    {
        compressor = chooseCpuCompressor(compressionOptions);
    }

    if (compressor == NULL)
    {
        outputOptions.error(Error_UnsupportedFeature);
    }
    else
    {
        compressor->compress(inputFormat, alphaMode, w, h, data, compressionOptions, outputOptions);
    }

    return true;
}

int Compressor::Private::estimateSize(const InputOptions::Private & inputOptions, const CompressionOptions::Private & compressionOptions) const
{
    const Format format = compressionOptions.format;

    uint bitCount = compressionOptions.bitcount;
    if (format == Format_RGBA && bitCount == 0) bitCount = compressionOptions.rsize + compressionOptions.gsize + compressionOptions.bsize + compressionOptions.asize;

    inputOptions.computeTargetExtents();

    uint mipmapCount = inputOptions.realMipmapCount();

    int size = 0;

    for (uint f = 0; f < inputOptions.faceCount; f++)
    {
        uint w = inputOptions.targetWidth;
        uint h = inputOptions.targetHeight;
        uint d = inputOptions.targetDepth;

        for (uint m = 0; m < mipmapCount; m++)
        {
            size += computeImageSize(w, h, d, bitCount, format);

            // Compute extents of next mipmap:
            w = max(1U, w / 2);
            h = max(1U, h / 2);
            d = max(1U, d / 2);
        }
    }

    return size;
}
