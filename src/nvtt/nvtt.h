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

#pragma once
#ifndef NVTT_H
#define NVTT_H

// Function linkage
#if NVTT_SHARED

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__ || defined __MINGW32__
#  ifdef NVTT_EXPORTS
#    define NVTT_API __declspec(dllexport)
#  else
#    define NVTT_API __declspec(dllimport)
#  endif
#endif

#if defined __GNUC__ >= 4
#  ifdef NVTT_EXPORTS
#    define NVTT_API __attribute__((visibility("default")))
#  endif
#endif

#endif // NVTT_SHARED

#if !defined NVTT_API
#  define NVTT_API
#endif

#define NVTT_VERSION 20100

#define NVTT_FORBID_COPY(Class) \
    private: \
        Class(const Class &); \
        void operator=(const Class &); \
    public:

#define NVTT_DECLARE_PIMPL(Class) \
    public: \
        struct Private; \
        Private & m


// Public interface.
namespace nvtt
{
    // Forward declarations.
    struct TexImage;

    /// Supported compression formats.
    enum Format
    {
        // No compression.
        Format_RGB,
        Format_RGBA = Format_RGB,

        // DX9 formats.
        Format_DXT1,
        Format_DXT1a,   // DXT1 with binary alpha.
        Format_DXT3,
        Format_DXT5,
        Format_DXT5n,   // Compressed HILO: R=1, G=y, B=0, A=x

        // DX10 formats.
        Format_BC1 = Format_DXT1,
        Format_BC1a = Format_DXT1a,
        Format_BC2 = Format_DXT3,
        Format_BC3 = Format_DXT5,
        Format_BC3n = Format_DXT5n,
        Format_BC4,     // ATI1
        Format_BC5,     // 3DC, ATI2

        Format_DXT1n,   // Not supported on CPU yet.
        Format_CTX1,    // Not supported on CPU yet.

        Format_BC6,     // Not supported yet.
        Format_BC7,     // Not supported yet.

        Format_RGBE,
    };

    /// Pixel types. These basically indicate how the output should be interpreted, but do not have any influence over the input.
    enum PixelType
    {
        PixelType_UnsignedNorm = 0,
        PixelType_SignedNorm = 1,   // Not supported yet.
        PixelType_UnsignedInt = 2,  // Not supported yet.
        PixelType_SignedInt = 3,    // Not supported yet.
        PixelType_Float = 4,
        PixelType_UnsignedFloat = 5,
    };

    /// Quality modes.
    enum Quality
    {
        Quality_Fastest,
        Quality_Normal,
        Quality_Production,
        Quality_Highest,
    };

    /// DXT decoder.
    enum Decoder
    {
        Decoder_D3D10,
        Decoder_D3D9,
        Decoder_NV5x,
    };


    /// Compression options. This class describes the desired compression format and other compression settings.
    struct CompressionOptions
    {
        NVTT_FORBID_COPY(CompressionOptions);
        NVTT_DECLARE_PIMPL(CompressionOptions);

        NVTT_API CompressionOptions();
        NVTT_API ~CompressionOptions();

        NVTT_API void reset();

        NVTT_API void setFormat(Format format);
        NVTT_API void setQuality(Quality quality);
        NVTT_API void setColorWeights(float red, float green, float blue, float alpha = 1.0f);

        NVTT_API void setExternalCompressor(const char * name);

        // Set color mask to describe the RGB/RGBA format.
        NVTT_API void setPixelFormat(unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask);
        NVTT_API void setPixelFormat(unsigned char rsize, unsigned char gsize, unsigned char bsize, unsigned char asize);

        NVTT_API void setPixelType(PixelType pixelType);

        NVTT_API void setPitchAlignment(int pitchAlignment);

        NVTT_API void setQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold = 127);

        NVTT_API void setTargetDecoder(Decoder decoder);

        // Translate to and from D3D formats.
        NVTT_API unsigned int d3d9Format() const;
        //NVTT_API bool setD3D9Format(unsigned int format);
        //NVTT_API unsigned int dxgiFormat() const;
        //NVTT_API bool setDxgiFormat(unsigned int format);
    };

    /*
    // DXGI_FORMAT_R16G16_FLOAT
    compressionOptions.setPixelType(PixelType_Float);
    compressionOptions.setPixelFormat2(16, 16, 0, 0);

    // DXGI_FORMAT_R32G32B32A32_FLOAT
    compressionOptions.setPixelType(PixelType_Float);
    compressionOptions.setPixelFormat2(32, 32, 32, 32);
    */


    /// Wrap modes.
    enum WrapMode
    {
        WrapMode_Clamp,
        WrapMode_Repeat,
        WrapMode_Mirror,
    };

    /// Texture types.
    enum TextureType
    {
        TextureType_2D,
        TextureType_Cube,
    //  TextureType_3D,
    };

    /// Input formats.
    enum InputFormat
    {
        InputFormat_BGRA_8UB, // Normalized [0, 1] 8 bit fixed point.
        InputFormat_RGBA_16F, // 16 bit floating point.
        InputFormat_RGBA_32F, // 32 bit floating point.
    };

    /// Mipmap downsampling filters.
    enum MipmapFilter
    {
        MipmapFilter_Box,       ///< Box filter is quite good and very fast.
        MipmapFilter_Triangle,  ///< Triangle filter blurs the results too much, but that might be what you want.
        MipmapFilter_Kaiser,    ///< Kaiser-windowed Sinc filter is the best downsampling filter.
    };

    /// Texture resize filters.
    enum ResizeFilter
    {
        ResizeFilter_Box,
        ResizeFilter_Triangle,
        ResizeFilter_Kaiser,
        ResizeFilter_Mitchell,
    };

    /// Extents rounding mode.
    enum RoundMode
    {
        RoundMode_None,
        RoundMode_ToNextPowerOfTwo,
        RoundMode_ToNearestPowerOfTwo,
        RoundMode_ToPreviousPowerOfTwo,
    };

    /// Alpha mode.
    enum AlphaMode
    {
        AlphaMode_None,
        AlphaMode_Transparency,
        AlphaMode_Premultiplied,
    };

    /// Input options. Specify format and layout of the input texture.
    struct InputOptions
    {
        NVTT_FORBID_COPY(InputOptions);
        NVTT_DECLARE_PIMPL(InputOptions);

        NVTT_API InputOptions();
        NVTT_API ~InputOptions();

        // Set default options.
        NVTT_API void reset();

        // Setup input layout.
        NVTT_API void setTextureLayout(TextureType type, int w, int h, int d = 1);
        NVTT_API void resetTextureLayout();

        // Set mipmap data. Copies the data.
        NVTT_API bool setMipmapData(const void * data, int w, int h, int d = 1, int face = 0, int mipmap = 0);

        // Describe the format of the input.
        NVTT_API void setFormat(InputFormat format);

        // Set the way the input alpha channel is interpreted. @@ Not implemented!
        NVTT_API void setAlphaMode(AlphaMode alphaMode);

        // Set gamma settings.
        NVTT_API void setGamma(float inputGamma, float outputGamma);

        // Set texture wrapping mode.
        NVTT_API void setWrapMode(WrapMode mode);

        // Set mipmapping options.
        NVTT_API void setMipmapFilter(MipmapFilter filter);
        NVTT_API void setMipmapGeneration(bool enabled, int maxLevel = -1);
        NVTT_API void setKaiserParameters(float width, float alpha, float stretch);

        // Set normal map options.
        NVTT_API void setNormalMap(bool b);
        NVTT_API void setConvertToNormalMap(bool convert);
        NVTT_API void setHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale);
        NVTT_API void setNormalFilter(float sm, float medium, float big, float large);
        NVTT_API void setNormalizeMipmaps(bool b);

        // Set resizing options.
        NVTT_API void setMaxExtents(int d);
        NVTT_API void setRoundMode(RoundMode mode);
    };


    /// Output handler.
    struct OutputHandler
    {
        virtual ~OutputHandler() {}

        /// Indicate the start of a new compressed image that's part of the final texture.
        virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) = 0;

        /// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
        virtual bool writeData(const void * data, int size) = 0;
    };

    /// Error codes.
    enum Error
    {
        Error_Unknown,
        Error_InvalidInput,
        Error_UnsupportedFeature,
        Error_CudaError,
        Error_FileOpen,
        Error_FileWrite,
        Error_UnsupportedOutputFormat,
    };

    /// Error handler.
    struct ErrorHandler
    {
        virtual ~ErrorHandler() {}

        // Signal error.
        virtual void error(Error e) = 0;
    };

    /// Container.
    enum Container
    {
        Container_DDS,
        Container_DDS10,
    };


    /// Output Options. This class holds pointers to the interfaces that are used to report the output of
    /// the compressor to the user.
    struct OutputOptions
    {
        NVTT_FORBID_COPY(OutputOptions);
        NVTT_DECLARE_PIMPL(OutputOptions);

        NVTT_API OutputOptions();
        NVTT_API ~OutputOptions();

        // Set default options.
        NVTT_API void reset();

        NVTT_API void setFileName(const char * fileName);

        NVTT_API void setOutputHandler(OutputHandler * outputHandler);
        NVTT_API void setErrorHandler(ErrorHandler * errorHandler);

        NVTT_API void setOutputHeader(bool outputHeader);
        NVTT_API void setContainer(Container container);
        NVTT_API void setUserVersion(int version);
        NVTT_API void setSrgbFlag(bool b);
    };

    typedef void Task(void * context, int id);

    struct TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, int count) = 0;
    };

    /// Context.
    struct Compressor
    {
        NVTT_FORBID_COPY(Compressor);
        NVTT_DECLARE_PIMPL(Compressor);

        NVTT_API Compressor();
        NVTT_API ~Compressor();

        // Context settings.
        NVTT_API void enableCudaAcceleration(bool enable);
        NVTT_API bool isCudaAccelerationEnabled() const;
        NVTT_API void setTaskDispatcher(TaskDispatcher * disp);

        // InputOptions API.
        NVTT_API bool process(const InputOptions & inputOptions, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
        NVTT_API int estimateSize(const InputOptions & inputOptions, const CompressionOptions & compressionOptions) const;

        // TexImage API.
        NVTT_API bool outputHeader(const TexImage & tex, int mipmapCount, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
        NVTT_API bool compress(const TexImage & tex, int face, int mipmap, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
        NVTT_API int estimateSize(const TexImage & tex, int mipmapCount, const CompressionOptions & compressionOptions) const;

        // Raw API.
        NVTT_API bool compress(int w, int h, int d, int face, int mipmap, const float * rgba, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
        NVTT_API int estimateSize(int w, int h, int d, int mipmapCount, const CompressionOptions & compressionOptions) const;
    };

    // "Compressor" is deprecated. This should have been called "Context"
    typedef Compressor Context;

    enum NormalTransform {
        NormalTransform_Orthographic,
        NormalTransform_Stereographic,
        NormalTransform_Paraboloid,
    };

    /// A texture mipmap.
    struct TexImage
    {
        NVTT_API TexImage();
        NVTT_API TexImage(const TexImage & tex);
        NVTT_API ~TexImage();

        NVTT_API void operator=(const TexImage & tex);

        // Texture parameters.
        NVTT_API void setWrapMode(WrapMode mode);
        NVTT_API void setAlphaMode(AlphaMode alphaMode);
        NVTT_API void setNormalMap(bool isNormalMap);

        // Queries.
        NVTT_API bool isNull() const;
        NVTT_API int width() const;
        NVTT_API int height() const;
        NVTT_API int depth() const;
        NVTT_API WrapMode wrapMode() const;
        NVTT_API AlphaMode alphaMode() const;
        NVTT_API bool isNormalMap() const;
        NVTT_API int countMipmaps() const;
        NVTT_API float alphaTestCoverage(float alphaRef = 0.5) const;
        NVTT_API float average(int channel) const;
        NVTT_API const float * data() const;
        NVTT_API void histogram(int channel, float rangeMin, float rangeMax, int binCount, int * binPtr) const;
        NVTT_API void range(int channel, float * rangeMin, float * rangeMax);

        // Texture data.
        NVTT_API bool load(const char * fileName, bool * hasAlpha = 0);
        NVTT_API bool save(const char * fileName) const;
        NVTT_API bool setImage2D(InputFormat format, int w, int h, const void * data);
        NVTT_API bool setImage2D(InputFormat format, int w, int h, const void * r, const void * g, const void * b, const void * a);
        NVTT_API bool setImage2D(Format format, Decoder decoder, int w, int h, const void * data);

        // Resizing methods.
        NVTT_API void resize(int w, int h, ResizeFilter filter);
        NVTT_API void resize(int w, int h, ResizeFilter filter, float filterWidth, const float * params = 0);
        NVTT_API void resize(int maxExtent, RoundMode mode, ResizeFilter filter);
        NVTT_API void resize(int maxExtent, RoundMode mode, ResizeFilter filter, float filterWidth, const float * params = 0);
        NVTT_API bool buildNextMipmap(MipmapFilter filter);
        NVTT_API bool buildNextMipmap(MipmapFilter filter, float filterWidth, const float * params = 0);

        // Color transforms.
        NVTT_API void toLinear(float gamma);
        NVTT_API void toGamma(float gamma);
        NVTT_API void transform(const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4]);
        NVTT_API void swizzle(int r, int g, int b, int a);
        NVTT_API void scaleBias(int channel, float scale, float bias);
        NVTT_API void clamp(int channel, float low = 0.0f, float high = 1.0f);
        NVTT_API void packNormal();
        NVTT_API void expandNormal();
        NVTT_API void blend(float r, float g, float b, float a, float t);
        NVTT_API void premultiplyAlpha();
        NVTT_API void toGreyScale(float redScale, float greenScale, float blueScale, float alphaScale);
        NVTT_API void setBorder(float r, float g, float b, float a);
        NVTT_API void fill(float r, float g, float b, float a);
        NVTT_API void scaleAlphaToCoverage(float coverage, float alphaRef = 0.5f);
        NVTT_API void toRGBM(float range = 1.0f, float threshold = 0.0f);
        NVTT_API void fromRGBM(float range = 1.0f);
        NVTT_API void toYCoCg();
        NVTT_API void blockScaleCoCg(int bits = 5, float threshold = 0.0f);
        NVTT_API void fromYCoCg();
        NVTT_API void toLUVW(float range = 1.0f);
        NVTT_API void fromLUVW(float range = 1.0f);
        NVTT_API void abs(int channel);

        //NVTT_API void blockLuminanceScale(float scale);

        // Color quantization.
        NVTT_API void binarize(int channel, float threshold, bool dither);
        NVTT_API void quantize(int channel, int bits, bool exactEndPoints, bool dither);

        // Normal map transforms.
        NVTT_API void toNormalMap(float sm, float medium, float big, float large);
        NVTT_API void normalizeNormalMap();
        NVTT_API void transformNormals(NormalTransform xform);
        NVTT_API void reconstructNormals(NormalTransform xform);

        // Geometric transforms.
        NVTT_API void flipVertically();

        // Copy image data.
        NVTT_API bool copyChannel(const TexImage & srcImage, int srcChannel);
        NVTT_API bool copyChannel(const TexImage & srcImage, int srcChannel, int dstChannel);

        NVTT_API bool addChannel(const TexImage & img, int srcChannel, int dstChannel, float scale);

        // Error compare.
        NVTT_API friend float rmsError(const TexImage & reference, const TexImage & img);
        NVTT_API friend float rmsAlphaError(const TexImage & reference, const TexImage & img);
        NVTT_API friend float cieLabError(const TexImage & reference, const TexImage & img);
        NVTT_API friend TexImage diff(const TexImage & reference, const TexImage & img, float scale);

    private:
        void detach();

        struct Private;
        Private * m;
    };


    // Return string for the given error code.
    NVTT_API const char * errorString(Error e);

    // Return NVTT version.
    NVTT_API unsigned int version();

    NVTT_API float rmsError(const TexImage & reference, const TexImage & img);
    NVTT_API float rmsAlphaError(const TexImage & reference, const TexImage & img);
    NVTT_API float cieLabError(const TexImage & reference, const TexImage & img);
    NVTT_API TexImage diff(const TexImage & reference, const TexImage & img, float scale);


} // nvtt namespace

#endif // NVTT_H
