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

#include "CompressDXT.h"
#include "FastCompressDXT.h"
#include "CompressRGB.h"
#include "InputOptions.h"
#include "CompressionOptions.h"
#include "cuda/CudaUtils.h"
#include "cuda/CudaCompressDXT.h"


using namespace nv;
using namespace nvtt;

namespace
{
	
	static int blockSize(Format format)
	{
		if (format == Format_DXT1 /*|| format == Format_DXT1a*/) {
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
		return 0;
	}

	inline uint computePitch(uint w, uint bitsize)
	{
		uint p = w * ((bitsize + 7) / 8);

		// Align to 32 bits.
		return ((p + 3) / 4) * 4;
	}

	static int computeImageSize(uint w, uint h, uint bitCount, Format format)
	{
		if (format == Format_RGBA) {
			return h * computePitch(w, bitCount);
		}
		else {
			return ((w + 3) / 4) * ((h + 3) / 4) * blockSize(format);
		}
	}
	
} // namespace





//
// compress
//

static void outputHeader(const InputOptions::Private & inputOptions, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	// Output DDS header.
	if (outputOptions.outputHandler != NULL && outputOptions.outputHeader)
	{
		DDSHeader header;
		
		InputOptions::Private::Image * img = inputOptions.images;
		nvCheck(img != NULL);
		
		header.setWidth(img->width);
		header.setHeight(img->height);
		
		int mipmapCount = inputOptions.mipmapCount;
		if (!inputOptions.generateMipmaps) mipmapCount = 0;
		else if (inputOptions.maxLevel != -1 && inputOptions.maxLevel < mipmapCount) mipmapCount = inputOptions.maxLevel;
		header.setMipmapCount(mipmapCount);

		if (inputOptions.textureType == TextureType_2D) {
			header.setTexture2D();
		}
		else if (inputOptions.textureType == TextureType_Cube) {
			header.setTextureCube();
		}		
		/*else if (inputOptions.textureType == TextureType_3D) {
			header.setTexture3D();
			header.setDepth(img->depth);
		}*/
		
		if (compressionOptions.format == Format_RGBA)
		{
			header.setPitch(4 * img->width);
			header.setPixelFormat(compressionOptions.bitcount, compressionOptions.rmask, compressionOptions.gmask, compressionOptions.bmask, compressionOptions.amask);
		}
		else
		{
			header.setLinearSize(computeImageSize(img->width, img->height, compressionOptions.bitcount, compressionOptions.format));
			
			if (compressionOptions.format == Format_DXT1 /*|| compressionOptions.format == Format_DXT1a*/) {
				header.setFourCC('D', 'X', 'T', '1');
			}
			else if (compressionOptions.format == Format_DXT3) {
				header.setFourCC('D', 'X', 'T', '3');
			}
			else if (compressionOptions.format == Format_DXT5) {
				header.setFourCC('D', 'X', 'T', '5');
			}
			else if (compressionOptions.format == Format_DXT5n) {
				header.setFourCC('D', 'X', 'T', '5');
				header.setNormalFlag(true);
			}
			else if (compressionOptions.format == Format_BC4) {
				header.setFourCC('A', 'T', 'I', '1');
			}
			else if (compressionOptions.format == Format_BC5) {
				header.setFourCC('A', 'T', 'I', '2');
				header.setNormalFlag(true);
			}
		}
		
		// Swap bytes if necessary.
		header.swapBytes();
		
		nvStaticCheck(sizeof(DDSHeader) == 128);
		outputOptions.outputHandler->writeData(&header, 128);
		
		// Revert swap.
		header.swapBytes();
	}
}


static bool compressMipmap(const Image * image, const OutputOptions & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvDebugCheck(image != NULL);

	if (compressionOptions.format == Format_RGBA || compressionOptions.format == Format_RGB)
	{
		compressRGB(image, outputOptions, compressionOptions);
	}
	else if (compressionOptions.format == Format_DXT1)
	{
#if defined(HAVE_S3QUANT)
		if (compressionOptions.externalCompressor == "s3")
		{
			s3CompressDXT1(image, outputOptions);
		}
		else
#endif

#if defined(HAVE_ATITC)
		if (compressionOptions.externalCompressor == "ati")
		{
			printf("ATI\n");
			atiCompressDXT1(image, outputOptions);
		}
		else
#endif
		if (compressionOptions.quality == Quality_Fastest)
		{
			fastCompressDXT1(image, outputOptions);
		}
		else
		{
			if (compressionOptions.useCuda && nv::cuda::isHardwarePresent())
			{
				cudaCompressDXT1(image, outputOptions, compressionOptions);
			}
			else
			{
				compressDXT1(image, outputOptions, compressionOptions);
			}
		}
	}
	else if (compressionOptions.format == Format_DXT3)
	{
		if (compressionOptions.quality == Quality_Fastest)
		{
			fastCompressDXT3(image, outputOptions);
		}
		else
		{
			if (compressionOptions.useCuda && nv::cuda::isHardwarePresent())
			{
				cudaCompressDXT3(image, outputOptions, compressionOptions);
			}
			else
			{
				compressDXT3(image, outputOptions, compressionOptions);
			}
		}
	}
	else if (compressionOptions.format == Format_DXT5)
	{
		if (compressionOptions.quality == Quality_Fastest)
		{
			fastCompressDXT5(image, outputOptions);
		}
		else
		{
			if (compressionOptions.useCuda && nv::cuda::isHardwarePresent())
			{
				cudaCompressDXT5(image, outputOptions, compressionOptions);
			}
			else
			{
				compressDXT5(image, outputOptions, compressionOptions);
			}
		}
	}
	else if (compressionOptions.format == Format_DXT5n)
	{
		if (compressionOptions.quality == Quality_Fastest)
		{
			fastCompressDXT5n(image, outputOptions);
		}
		else
		{
			compressDXT5n(image, outputOptions, compressionOptions);
		}
	}
	else if (compressionOptions.format == Format_BC4)
	{
		compressBC4(image, outputOptions, compressionOptions);
	}
	else if (compressionOptions.format == Format_BC5)
	{
		compressBC5(image, outputOptions, compressionOptions);
	}

	return true;
}


// Convert input image to linear float image.
static FloatImage * toFloatImage(const Image * image, const InputOptions::Private & inputOptions)
{
	nvDebugCheck(image != NULL);

	FloatImage * floatImage = new FloatImage(image);

	if (inputOptions.normalMap)
	{
		// Expand normals. to [-1, 1] range.
	//	floatImage->expandNormals(0);
	}
	else if (inputOptions.inputGamma != 1.0f)
	{
		// Convert to linear space.
		floatImage->toLinear(0, 3, inputOptions.inputGamma);
	}

	return floatImage;
}


// Convert linear float image to output image.
static Image * toFixedImage(const FloatImage * floatImage, const InputOptions::Private & inputOptions)
{
	nvDebugCheck(floatImage != NULL);

	return floatImage->createImageGammaCorrect(inputOptions.outputGamma);
}


// Create mipmap from the given image.
static FloatImage * createMipmap(const FloatImage * floatImage, const InputOptions::Private & inputOptions)
{
	FloatImage * result = NULL;
	
	if (inputOptions.mipmapFilter == MipmapFilter_Box)
	{
		// Use fast downsample.
		result = floatImage->fastDownSample();
	}
	else if (inputOptions.mipmapFilter == MipmapFilter_Triangle)
	{
		Kernel1 kernel(4);
		kernel.initFilter(Filter::Triangle);
		result = floatImage->downSample(kernel, (FloatImage::WrapMode)inputOptions.wrapMode);
	}
	else /*if (inputOptions.mipmapFilter == MipmapFilter_Kaiser)*/
	{
		Kernel1 kernel(10);
		kernel.initKaiser(8.0, 0.75f);
		result = floatImage->downSample(kernel, (FloatImage::WrapMode)inputOptions.wrapMode);
	}
	
	// Normalize mipmap.
	if (inputOptions.normalizeMipmaps)
	{
		normalize(result);
	}
	
	return result;
}


// Quantize the input image to the precision of the output format.
static void quantize(Image * img, const InputOptions::Private & inputOptions, Format format)
{
	if (inputOptions.enableColorDithering)
	{
		if (format >= Format_DXT1 && format <= Format_DXT5)
		{
			Quantize::FloydSteinberg_RGB16(img);
		}
	}
	if (inputOptions.binaryAlpha)
	{
		if (inputOptions.enableAlphaDithering)
		{
			Quantize::FloydSteinberg_BinaryAlpha(img, inputOptions.alphaThreshold);
		}
		else
		{
			Quantize::BinaryAlpha(img, inputOptions.alphaThreshold);
		}
	}
	else
	{
		if (inputOptions.enableAlphaDithering)
		{
			if (format == Format_DXT3)
			{
				Quantize::Alpha4(img);
			}
			/*else if (format == Format_DXT1a)
			{
				Quantize::BinaryAlpha(img, inputOptions.alphaThreshold);
			}*/
		}
	}
}


/// Compress the input texture with the given compression options.
bool nvtt::compress(const InputOptions & inputOptions, const OutputOptions & outputOptions, const CompressionOptions & compressionOptions)
{
	// Make sure enums match.
	nvStaticCheck(FloatImage::WrapMode_Clamp == (FloatImage::WrapMode)WrapMode_Clamp);
	nvStaticCheck(FloatImage::WrapMode_Mirror == (FloatImage::WrapMode)WrapMode_Mirror);
	nvStaticCheck(FloatImage::WrapMode_Repeat == (FloatImage::WrapMode)WrapMode_Repeat);

	// Output DDS header.
	outputHeader(inputOptions.m, outputOptions, compressionOptions.m);

	Format format = compressionOptions.m.format;
	const uint bitCount = compressionOptions.m.bitcount;

	for (int f = 0; f < inputOptions.m.faceCount; f++)
	{
		Image * lastImage = NULL;
		AutoPtr<FloatImage> floatImage(NULL);
		
		for (int m = 0; m < inputOptions.m.mipmapCount; m++)
		{
			int idx = f * inputOptions.m.mipmapCount + m;
			InputOptions::Private::Image & mipmap = inputOptions.m.images[idx];
			
			if (outputOptions.outputHandler)
			{
				int size = computeImageSize(mipmap.width, mipmap.height, bitCount, format);
				outputOptions.outputHandler->mipmap(size, mipmap.width, mipmap.height, mipmap.depth, mipmap.face, mipmap.mipLevel);
			}
			
			Image * img; // Image to compress.
			
			if (mipmap.data != NULL) // Mipmap provided.
			{
				// Convert to normal map.
				if (inputOptions.m.convertToNormalMap)
				{
					floatImage = createNormalMap(mipmap.data, (FloatImage::WrapMode)inputOptions.m.wrapMode, inputOptions.m.heightFactors, inputOptions.m.bumpFrequencyScale);
				}
				else
				{
					lastImage = img = mipmap.data;
					
					// Delete float image.
					floatImage = NULL;
				}
			}
			else // Create mipmap from last.
			{
				if (m == 0) {
					// First mipmap missing.
					if (outputOptions.errorHandler != NULL) outputOptions.errorHandler->error(Error_InvalidInput);
					return false;
				}
				
				if (floatImage == NULL)
				{
					nvDebugCheck(lastImage != NULL);
					floatImage = toFloatImage(lastImage, inputOptions.m);
				}
				
				// Create mipmap.
				floatImage = createMipmap(floatImage.ptr(), inputOptions.m);
			}
			
			if (floatImage != NULL)
			{
				// Convert to fixed.
				img = toFixedImage(floatImage.ptr(), inputOptions.m);
			}
			
			quantize(img, inputOptions.m, format);
			
			compressMipmap(img, outputOptions, compressionOptions.m);
			
			if (img != mipmap.data)
			{
				delete img;
			}
			
			if (!inputOptions.m.generateMipmaps || (inputOptions.m.maxLevel >= 0 && m >= inputOptions.m.maxLevel)) {
				// continue with next face.
				break;
			}
		}
	}

	return true;
}




/// Estimate the size of compressing the input with the given options.
int nvtt::estimateSize(const InputOptions & inputOptions, const CompressionOptions & compressionOptions)
{
	Format format = compressionOptions.m.format;
	const uint bitCount = compressionOptions.m.bitcount;

	int size = 0;
	
	for (int f = 0; f < inputOptions.m.faceCount; f++)
	{
		for (int m = 0; m < inputOptions.m.mipmapCount; m++)
		{
			int idx = f * inputOptions.m.mipmapCount + m;
			const InputOptions::Private::Image & img = inputOptions.m.images[idx];
			
			size += computeImageSize(img.width, img.height, bitCount, format);
			
			if (!inputOptions.m.generateMipmaps || (inputOptions.m.maxLevel >= 0 && m >= inputOptions.m.maxLevel)) {
				// continue with next face.
				break;
			}
		}
	}
	
	return size;
}


/// Return a string for the given error.
const char * nvtt::errorString(Error e)
{
	switch(e)
	{
		case Error_InvalidInput:
			return "Invalid input";
		case Error_UserInterruption:
			return "User interruption";
		case Error_UnsupportedFeature:
			return "Unsupported feature";
		case Error_CudaError:
			return "CUDA error";
		case Error_Unknown:
			return "Unknown error";
	}

	return NULL;
}

