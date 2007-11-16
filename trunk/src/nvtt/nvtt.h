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

#ifndef NV_TT_H
#define NV_TT_H

#include <nvcore/nvcore.h>

//#define HAVE_S3QUANT
//#define HAVE_ATITC

// Function linkage
#if NVTT_SHARED
#ifdef NVTT_EXPORTS
#define NVTT_API DLL_EXPORT
#define NVTT_CLASS DLL_EXPORT_CLASS
#else
#define NVTT_API DLL_IMPORT
#define NVTT_CLASS DLL_IMPORT
#endif
#else
#define NVTT_API
#define NVTT_CLASS
#endif

// Public interface.
namespace nvtt
{
	/// Supported compression formats.
	enum Format
	{
		// No compression.
		Format_RGB,
		Format_RGBA = Format_RGB,

		// DX9 formats.
		Format_DXT1,
		Format_DXT1a,	// DXT1 with binary alpha.
		Format_DXT3,
		Format_DXT5,
		Format_DXT5n,	// Compressed HILO: R=0, G=x, B=0, A=y
		
		// DX10 formats.
		Format_BC1 = Format_DXT1,
		Format_BC1a = Format_DXT1a,
		Format_BC2 = Format_DXT3,
		Format_BC3 = Format_DXT5,
		Format_BC3n = Format_DXT5n,
		Format_BC4,		// ATI1
		Format_BC5,		// 3DC, ATI2

		// OpenGL formats.
		Format_LATC = Format_BC5,
	};
	
	/// Quality modes.
	enum Quality
	{
		Quality_Fastest,
		Quality_Normal,
		Quality_Production,
		Quality_Highest,
	};

	/// Compression options. This class describes the desired compression format and other compression settings.
	class CompressionOptions
	{
	public:
		NVTT_API CompressionOptions();
		NVTT_API ~CompressionOptions();
		
		NVTT_API void reset();
		
		NVTT_API void setFormat(Format format);
		NVTT_API void setQuality(Quality quality, float errorThreshold = 0.5f);
		NVTT_API void setColorWeights(float red, float green, float blue);
		NVTT_API void enableHardwareCompression(bool enable);
		
		NVTT_API void setExternalCompressor(const char * name);

		// Set color mask to describe the RGB/RGBA format.
		NVTT_API void setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);

	//private:
		struct Private;
		Private & m;
	};


	/// Wrap modes. // This matches FloatImage::WrapMode.
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
	//	TextureType_3D,
	};
	
	/// Input formats.
	enum InputFormat
	{
		InputFormat_BGRA_8UB,
	//	InputFormat_RGBE_8UB,
	//	InputFormat_BGRA_32F,
	};
	
	/// Mipmap downsampling filters.
	enum MipmapFilter
	{
		MipmapFilter_Box,		///< Box filter is quite good and very fast.
		MipmapFilter_Triangle,	///< Triangle filter blurs the results too much, but that might be what you want.
		MipmapFilter_Kaiser,	///< Kaiser-windowed Sinc filter is the best downsampling filter.
	};
	
	enum ColorTransform
	{
		ColorTransform_None,
		ColorTransform_Linear,
		ColorTransform_CoYCg,
		ColorTransform_CoSCgY,
	};

	/// Input options. Specify format and layout of the input texture.
	struct InputOptions
	{
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
		NVTT_API void setFormat(InputFormat fmt, bool alphaTransparency);

		// Set gamma settings.
		NVTT_API void setGamma(float inputGamma, float outputGamma);

		// Set texture wrappign mode.
		NVTT_API void setWrapMode(WrapMode mode);

		// Set mipmapping options.
		NVTT_API void setMipmapping(bool generateMipmaps, MipmapFilter filter = MipmapFilter_Kaiser, int maxLevel = -1);
		
		// Set quantization options.
		NVTT_API void setQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold = 127);

		// Set normal map options.
		NVTT_API void setNormalMap(bool b);
		NVTT_API void setConvertToNormalMap(bool convert);
		NVTT_API void setHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale);
		NVTT_API void setNormalFilter(float small, float medium, float big, float large);
		NVTT_API void setNormalizeMipmaps(bool b);

		// Set color transforms.
		NVTT_API void setColorTransform(ColorTransform t);
		NVTT_API void setLinearTransfrom(int channel, float w0, float w1, float w2, float w3);

	//private:
		struct Private;
		Private & m;
	};
	
	
	/// Output handler.
	struct OutputHandler
	{
		virtual ~OutputHandler() {}
		
		/// Indicate the start of a new compressed image that's part of the final texture.
		virtual void mipmap(int size, int width, int height, int depth, int face, int miplevel) = 0;
		
		/// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
		virtual void writeData(const void * data, int size) = 0;
	};

	/// Error codes.
	enum Error
	{
		Error_InvalidInput,
		Error_UserInterruption,
		Error_UnsupportedFeature,
		Error_CudaError,
		Error_Unknown,
	};
	
	/// Error handler.
	struct ErrorHandler
	{
		virtual ~ErrorHandler() {}
		
		// Signal error.
		virtual void error(Error e) = 0;
	};


	/// Output Options. This class holds pointers to the interfaces that are used to report the output of 
	/// the compressor to the user.
	struct OutputOptions
	{
		OutputOptions() : outputHandler(NULL), outputHeader(true) { reset(); }
		OutputOptions(OutputHandler * oh, ErrorHandler * eh) : outputHandler(oh), errorHandler(eh), outputHeader(true) { reset(); }
		
		// Set default options.
		NVTT_API void reset();
		
		OutputHandler * outputHandler;
		ErrorHandler * errorHandler;
		bool outputHeader;
	};
	
	
	// Main entrypoint of the compression library.
	NVTT_API bool compress(const InputOptions & inputOptions, const OutputOptions & outputOptions, const CompressionOptions & compressionOptions);
	
	// Estimate the size of compressing the input with the given options.
	NVTT_API int estimateSize(const InputOptions & inputOptions, const CompressionOptions & compressionOptions);
	
	// Return string for the given error.
	NVTT_API const char * errorString(Error e);

} // nvtt namespace

#endif // NV_TT_H
