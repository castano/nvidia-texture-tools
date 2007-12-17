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
#include "OutputOptions.h"
#include "cuda/CudaUtils.h"
#include "cuda/CudaCompressDXT.h"


using namespace nv;
using namespace nvtt;

namespace
{
	
	static int blockSize(Format format)
	{
		if (format == Format_DXT1 || format == Format_DXT1a) {
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

static void outputHeader(const InputOptions::Private & inputOptions, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	// Output DDS header.
	if (outputOptions.outputHandler != NULL && outputOptions.outputHeader)
	{
		DDSHeader header;
		
		header.setWidth(inputOptions.targetWidth);
		header.setHeight(inputOptions.targetHeight);
		
		int mipmapCount = inputOptions.realMipmapCount();
		nvDebugCheck(mipmapCount > 0);
		
		header.setMipmapCount(mipmapCount);
		
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
		
		if (compressionOptions.format == Format_RGBA)
		{
			header.setPitch(4 * inputOptions.targetWidth);
			header.setPixelFormat(compressionOptions.bitcount, compressionOptions.rmask, compressionOptions.gmask, compressionOptions.bmask, compressionOptions.amask);
		}
		else
		{
			header.setLinearSize(computeImageSize(inputOptions.targetWidth, inputOptions.targetHeight, compressionOptions.bitcount, compressionOptions.format));
			
			if (compressionOptions.format == Format_DXT1 || compressionOptions.format == Format_DXT1a) {
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
				if (inputOptions.isNormalMap) header.setNormalFlag(true);
			}
			else if (compressionOptions.format == Format_BC4) {
				header.setFourCC('A', 'T', 'I', '1');
			}
			else if (compressionOptions.format == Format_BC5) {
				header.setFourCC('A', 'T', 'I', '2');
				if (inputOptions.isNormalMap) header.setNormalFlag(true);
			}
		}
		
		// Swap bytes if necessary.
		header.swapBytes();
		
		nvStaticCheck(sizeof(DDSHeader) == 128 + 20);
		if (header.hasDX10Header())
		{
			outputOptions.outputHandler->writeData(&header, 128 + 20);
		}
		else
		{
			outputOptions.outputHandler->writeData(&header, 128);
		}
		
		// Revert swap.
		header.swapBytes();
	}
}


static bool compressMipmap(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
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
	else if (compressionOptions.format == Format_DXT1a)
	{
		// @@ Only fast compression mode for now.
		fastCompressDXT1a(image, outputOptions);
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

	if (inputOptions.isNormalMap)
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

	if (inputOptions.isNormalMap || inputOptions.outputGamma == 1.0f)
	{
		return floatImage->createImage();
	}
	else
	{
		return floatImage->createImageGammaCorrect(inputOptions.outputGamma);
	}
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
		TriangleFilter filter;
		result = floatImage->downSample(filter, (FloatImage::WrapMode)inputOptions.wrapMode);
	}
	else /*if (inputOptions.mipmapFilter == MipmapFilter_Kaiser)*/
	{
		nvDebugCheck(inputOptions.mipmapFilter == MipmapFilter_Kaiser);
		KaiserFilter filter(inputOptions.kaiserWidth);
		filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
		result = floatImage->downSample(filter, (FloatImage::WrapMode)inputOptions.wrapMode);
	}
	
	// Normalize mipmap.
	if ((inputOptions.isNormalMap || inputOptions.convertToNormalMap) && inputOptions.normalizeMipmaps)
	{
		normalizeNormalMap(result);
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
			else if (format == Format_DXT1a)
			{
				Quantize::BinaryAlpha(img, inputOptions.alphaThreshold);
			}
		}
	}
}

// Process the input, convert to normal map, normalize or convert to linear space.
static FloatImage * processInput(const InputOptions::Private & inputOptions, int idx)
{
	const InputOptions::Private::Image & mipmap = inputOptions.images[idx];
	
	if (inputOptions.convertToNormalMap)
	{
		// Scale height factor by 1 / 2 ^ m		// @@ Compute scale factor exactly...
		Vector4 heightScale = inputOptions.heightFactors / float(1 << idx);
		return createNormalMap(mipmap.data.ptr(), (FloatImage::WrapMode)inputOptions.wrapMode, heightScale, inputOptions.bumpFrequencyScale);
	}
	else if (inputOptions.isNormalMap)
	{
		if (inputOptions.normalizeMipmaps)
		{
			FloatImage * img = new FloatImage(mipmap.data.ptr());
			img->normalize(0);
			return img;
		}		
	}
	else
	{
		if (inputOptions.inputGamma != inputOptions.outputGamma)
		{
			FloatImage * img = new FloatImage(mipmap.data.ptr());
			img->toLinear(0, 3, inputOptions.inputGamma);
			return img;
		}
	}
	
	return NULL;
}




struct ImagePair
{
	ImagePair() : m_floatImage(NULL), m_fixedImage(NULL), m_deleteFixedImage(false) {}
	~ImagePair()
	{
		if (m_deleteFixedImage) {
			delete m_fixedImage;
		}
	}
	
	void setFloatImage(FloatImage * image)
	{
		m_floatImage = image;
		if (m_deleteFixedImage) delete m_fixedImage;
		m_fixedImage = NULL;
	}
	
	void setFixedImage(Image * image, bool deleteImage)
	{
		m_floatImage = NULL;
		if (m_deleteFixedImage) delete m_fixedImage;
		m_fixedImage = image;
		m_deleteFixedImage = deleteImage;
	}
	
	FloatImage * floatImage() const { return m_floatImage.ptr(); }
	Image * fixedImage() const { return m_fixedImage; }
	
	void toFixed(const InputOptions::Private & inputOptions)
	{
		if (m_floatImage != NULL)
		{
			// Convert to fixed.
			m_fixedImage = toFixedImage(m_floatImage.ptr(), inputOptions);
		}
	}
	
private:
	AutoPtr<FloatImage> m_floatImage;
	Image * m_fixedImage;
	bool m_deleteFixedImage;
};




// Find the first mipmap provided that is greater or equal to the target image size.
static int findMipmap(const InputOptions::Private & inputOptions, uint f, int firstMipmap, uint w, uint h, uint d)
{
	nvDebugCheck(firstMipmap >= 0);

	int bestIdx = -1;
	
	for (int m = firstMipmap; m < int(inputOptions.mipmapCount); m++)
	{
		int idx = f * inputOptions.mipmapCount + m;
		const InputOptions::Private::Image & mipmap = inputOptions.images[idx];
		
		if (mipmap.width >= int(w) && mipmap.height >= int(h) && mipmap.depth >= int(d))
		{
			if (mipmap.data != NULL)
			{
				bestIdx = idx;
			}
		}
		else
		{
			// Do not look further down.
			break;
		}
	}
	
	return bestIdx;
}



static int findImage(const InputOptions::Private & inputOptions, uint f, uint w, uint h, uint d, int inputImageIdx, ImagePair * pair)
{
	nvDebugCheck(w > 0 && h > 0);
	nvDebugCheck(inputImageIdx >= 0 && inputImageIdx < int(inputOptions.mipmapCount));
	nvDebugCheck(pair != NULL);
	
	int bestIdx = findMipmap(inputOptions, f, inputImageIdx, w, h, d);
	const InputOptions::Private::Image & mipmap = inputOptions.images[bestIdx];
	
	if (mipmap.width == w && mipmap.height == h && mipmap.depth == d)
	{
		// Generate from input image.
		AutoPtr<FloatImage> processedImage( processInput(inputOptions, bestIdx) );
		
		if (processedImage != NULL)
		{
			pair->setFloatImage(processedImage.release());
		}
		else
		{
			pair->setFixedImage(mipmap.data.ptr(), false);
		}
		
		return bestIdx;
	}
	else
	{
		if (pair->floatImage() == NULL && pair->fixedImage() == NULL)
		{
			// Generate from input image and resize.
			AutoPtr<FloatImage> processedImage( processInput(inputOptions, bestIdx) );
			
			if (processedImage == NULL)
			{
				processedImage = new FloatImage(mipmap.data.ptr());
			}
			
			// Resize image. @@ Add more filters. @@ Distinguish between downscaling and reconstruction filters.
			BoxFilter boxFilter;
			pair->setFloatImage(processedImage->downSample(boxFilter, w, h, (FloatImage::WrapMode)inputOptions.wrapMode));
		}
		else
		{
			// Generate from previous mipmap.
			if (pair->floatImage() == NULL)
			{
				nvDebugCheck(pair->fixedImage() != NULL);
				pair->setFloatImage(toFloatImage(pair->fixedImage(), inputOptions));
			}
			
			// Create mipmap.
			pair->setFloatImage(createMipmap(pair->floatImage(), inputOptions));
		}
	}


	return bestIdx;	// @@ ???
}


static bool compressMipmaps(uint f, const InputOptions::Private & inputOptions, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	uint w = inputOptions.targetWidth;
	uint h = inputOptions.targetHeight;
	uint d = inputOptions.targetDepth;
	
	int inputImageIdx = findMipmap(inputOptions, f, 0, w, h, d);
	if (inputImageIdx == -1)
	{
		// First mipmap missing.
		if (outputOptions.errorHandler != NULL) outputOptions.errorHandler->error(Error_InvalidInput);
		return false;
	}
	
	ImagePair pair;
	
	for (uint m = 0; m < inputOptions.mipmapCount; m++)
	{
		if (outputOptions.outputHandler)
		{
			int size = computeImageSize(w, h, compressionOptions.bitcount, compressionOptions.format);
			outputOptions.outputHandler->mipmap(size, w, h, d, f, m);
		}
		
		inputImageIdx = findImage(inputOptions, f, w, h, d, inputImageIdx, &pair);
		
		// @@ Where to do the color transform?
		// - Color transform may not be linear, so we cannot do before computing mipmaps.
		// - Should be done in linear space, that is, after gamma correction.
		
		
		pair.toFixed(inputOptions);
		
		// @@ Quantization should be done in compressMipmap! @@ It should not modify the input image!!!
		quantize(pair.fixedImage(), inputOptions, compressionOptions.format);
		
		compressMipmap(pair.fixedImage(), outputOptions, compressionOptions);
		
		// Compute extents of next mipmap:
		w = max(1U, w / 2);
		h = max(1U, h / 2);
		d = max(1U, d / 2);
	}
	
	return true;
}







static bool compress(const InputOptions::Private & inputOptions, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	// Make sure enums match.
	nvStaticCheck(FloatImage::WrapMode_Clamp == (FloatImage::WrapMode)WrapMode_Clamp);
	nvStaticCheck(FloatImage::WrapMode_Mirror == (FloatImage::WrapMode)WrapMode_Mirror);
	nvStaticCheck(FloatImage::WrapMode_Repeat == (FloatImage::WrapMode)WrapMode_Repeat);

	// Get output handler.
	if (!outputOptions.openFile())
	{
		if (outputOptions.errorHandler) outputOptions.errorHandler->error(Error_FileOpen);
		// @@ Should return here?
	}
	
	inputOptions.computeTargetExtents();
	
	uint mipmapCount = inputOptions.realMipmapCount();
	nvDebugCheck(mipmapCount > 0);
	
	// Output DDS header.
	outputHeader(inputOptions, outputOptions, compressionOptions);

	for (uint f = 0; f < inputOptions.faceCount; f++)
	{
		if (!compressMipmaps(f, inputOptions, outputOptions, compressionOptions))
		{
			return false;
		}
		
		/*
		Image * lastImage = NULL;
		AutoPtr<FloatImage> floatImage(NULL);
		
		uint w = inputOptions.targetWidth;
		uint h = inputOptions.targetHeight;
		uint d = inputOptions.targetDepth;
		
		for (uint m = 0; m < mipmapCount; m++)
		{
			if (outputOptions.outputHandler)
			{
				int size = computeImageSize(w, h, bitCount, format);
				outputOptions.outputHandler->mipmap(size, w, h, d, f, m);
			}
			
			// @@ Write a more sofisticated get input image, that:
			// - looks for the nearest image in the input mipmap chain, resizes it to desired extents.
			// - uses previous floating point image, if available.
			// - uses previous byte image if available.
			
			
			int idx = f * inputOptions.mipmapCount + m;
			InputOptions::Private::Image & mipmap = inputOptions.images[idx];
			
			// @@ Prescale not implemented yet.
			nvCheck(w == mipmap.width);
			nvCheck(h == mipmap.height);
			nvCheck(d == mipmap.depth);
			
			Image * img = NULL; // Image to compress.
			
			if (mipmap.data != NULL) // Mipmap provided.
			{
				// Convert to normal map.
				if (inputOptions.convertToNormalMap)
				{
					// Scale height factor by 1 / 2 ^ m
					Vector4 heightScale = inputOptions.heightFactors / float(1 << m);
					floatImage = createNormalMap(mipmap.data.ptr(), (FloatImage::WrapMode)inputOptions.wrapMode, heightScale, inputOptions.bumpFrequencyScale);
				}
				else
				{
					lastImage = img = mipmap.data.ptr();
					
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
					floatImage = toFloatImage(lastImage, inputOptions);
				}
				
				// Create mipmap.
				floatImage = createMipmap(floatImage.ptr(), inputOptions);
			}
			
			if (floatImage != NULL)
			{
				// Convert to fixed.
				img = toFixedImage(floatImage.ptr(), inputOptions);
			}
			
			// @@ Where to do the color transform?
			// - Color transform may not be linear, so we cannot do before computing mipmaps.
			// - Should be done in linear space, that is, after gamma correction.

			// @@ Error! gamma correction is not performed when mipmap data provided. (only if inputGamma != outputGamma)

			// @@ This code is too complicated, too prone to erros, and hard to understand. Must be simplified!
			
			
			// @@ Quantization should be done in compressMipmap!
			quantize(img, inputOptions, format);
			
			compressMipmap(img, outputOptions, compressionOptions);
			
			if (img != mipmap.data)
			{
				delete img;
			}
			
			// Compute extents of next mipmap:
			w = max(1U, w / 2);
			h = max(1U, h / 2);
			d = max(1U, d / 2);
		}
		*/
	}

	outputOptions.closeFile();
	
	return true;
}


/// Compress the input texture with the given compression options.
bool nvtt::compress(const InputOptions & inputOptions, const OutputOptions & outputOptions, const CompressionOptions & compressionOptions)
{
	// @@ Hack this is necessary because of the pimpl transition.
	initOptions(const_cast<OutputOptions *>(&outputOptions));
	
	return ::compress(inputOptions.m, outputOptions.m, compressionOptions.m);
}


/// Estimate the size of compressing the input with the given options.
int nvtt::estimateSize(const InputOptions & inputOptions, const CompressionOptions & compressionOptions)
{
	const Format format = compressionOptions.m.format;
	const uint bitCount = compressionOptions.m.bitcount;

	inputOptions.m.computeTargetExtents();
	
	uint mipmapCount = inputOptions.m.realMipmapCount();
	
	int size = 0;
	
	for (uint f = 0; f < inputOptions.m.faceCount; f++)
	{
		uint w = inputOptions.m.targetWidth;
		uint h = inputOptions.m.targetHeight;
		uint d = inputOptions.m.targetDepth;
		
		for (uint m = 0; m < mipmapCount; m++)
		{
			size += computeImageSize(w, h, bitCount, format);
			
			// Compute extents of next mipmap:
			w = max(1U, w / 2);
			h = max(1U, h / 2);
			d = max(1U, d / 2);
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
		case Error_FileOpen:
			return "Error opening file";
		case Error_FileWrite:
			return "Error writing through output handler";
		case Error_Unknown:
		//default:
			return "Unknown error";
	}
	
	return "Invalid error";
}

