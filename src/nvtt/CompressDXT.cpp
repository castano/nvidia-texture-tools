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

#include <nvimage/Image.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/BlockDXT.h>

#include "nvtt.h"
#include "CompressDXT.h"
#include "FastCompressDXT.h"
#include "QuickCompressDXT.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"

// squish
#include "squish/colourset.h"
//#include "squish/clusterfit.h"
#include "squish/fastclusterfit.h"
#include "squish/weightedclusterfit.h"


// s3_quant
#if defined(HAVE_S3QUANT)
#include "s3tc/s3_quant.h"
#endif

// ati tc
#if defined(HAVE_ATITC)
#include "atitc/ATI_Compress.h"
#endif

//#include <time.h>

using namespace nv;
using namespace nvtt;


void nv::fastCompressDXT1(const Image * image, const OutputOptions::Private & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT1 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			
			if (rgba.isSingleColor())
			{
				QuickCompress::compressDXT1(rgba.color(0), &block);
			}
			else
			{
				QuickCompress::compressDXT1(rgba, &block);
			}
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressDXT1a(const Image * image, const OutputOptions::Private & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT1 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			QuickCompress::compressDXT1a(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressDXT3(const Image * image, const nvtt::OutputOptions::Private & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT3 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			QuickCompress::compressDXT3(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressDXT5(const Image * image, const nvtt::OutputOptions::Private & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			//QuickCompress::compressDXT5(rgba, &block);	// @@ Use fast version!!
			nv::compressBlock_BoundsRange(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressDXT5n(const Image * image, const nvtt::OutputOptions::Private & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			rgba.init(image, x, y);
			
			// copy X coordinate to alpha channel and Y coordinate to green channel.
			rgba.swizzleDXT5n();

			//QuickCompress::compressDXT5(rgba, &block);	// @@ Use fast version!!
			nv::compressBlock_BoundsRange(rgba, &block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::fastCompressBC4(const Image * image, const nvtt::OutputOptions::Private & outputOptions)
{
	// @@ TODO
	// compress red channel (X)
}


void nv::fastCompressBC5(const Image * image, const nvtt::OutputOptions::Private & outputOptions)
{
	// @@ TODO
	// compress red, green channels (X,Y)
}


void nv::doPrecomputation()
{
	static bool done = false;	// @@ Stop using statics for reentrancy.
	
	if (!done)
	{
		done = true;
		squish::FastClusterFit::DoPrecomputation();
	}
}


void nv::compressDXT1(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT1 block;

	doPrecomputation();

	//squish::WeightedClusterFit fit;
	//squish::ClusterFit fit;
	squish::FastClusterFit fit;
	fit.SetMetric(compressionOptions.colorWeight.x(), compressionOptions.colorWeight.y(), compressionOptions.colorWeight.z());

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			if (rgba.isSingleColor())
			{
				QuickCompress::compressDXT1(rgba.color(0), &block);
			}
			else
			{
				squish::ColourSet colours((uint8 *)rgba.colors(), 0);
				fit.SetColourSet(&colours, squish::kDxt1);
				fit.Compress(&block);
			}
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressDXT1a(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT1 block;

	squish::WeightedClusterFit fit;
	fit.SetMetric(compressionOptions.colorWeight.x(), compressionOptions.colorWeight.y(), compressionOptions.colorWeight.z());

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			// Compress color.
			squish::ColourSet colours((uint8 *)rgba.colors(), squish::kDxt1|squish::kWeightColourByAlpha);
			fit.SetColourSet(&colours, squish::kDxt1);
			fit.Compress(&block);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressDXT3(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT3 block;
	
	squish::WeightedClusterFit fit;
	fit.SetMetric(compressionOptions.colorWeight.x(), compressionOptions.colorWeight.y(), compressionOptions.colorWeight.z());

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			// Compress explicit alpha.
			QuickCompress::compressDXT3A(rgba, &block.alpha);
			
			// Compress color.
			squish::ColourSet colours((uint8 *)rgba.colors(), squish::kWeightColourByAlpha);
			fit.SetColourSet(&colours, 0);
			fit.Compress(&block.color);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}

void nv::compressDXT5(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;

	squish::WeightedClusterFit fit;
	fit.SetMetric(compressionOptions.colorWeight.x(), compressionOptions.colorWeight.y(), compressionOptions.colorWeight.z());

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);

			// Compress alpha.
			if (compressionOptions.quality == Quality_Highest)
			{
				compressBlock_BruteForce(rgba, &block.alpha);
			}
			else
			{
				QuickCompress::compressDXT5A(rgba, &block.alpha);
			}

			// Compress color.
			squish::ColourSet colours((uint8 *)rgba.colors(), squish::kWeightColourByAlpha);
			fit.SetColourSet(&colours, 0);
			fit.Compress(&block.color);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressDXT5n(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	BlockDXT5 block;
	
	doPrecomputation();

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);
			
			// copy X coordinate to green channel and Y coordinate to alpha channel.
			rgba.swizzleDXT5n();			
			
			// Compress X.
			if (compressionOptions.quality == Quality_Highest)
			{
				compressBlock_BruteForce(rgba, &block.alpha);
			}
			else
			{
				QuickCompress::compressDXT5A(rgba, &block.alpha);
			}
			
			// Compress Y.
			QuickCompress::compressDXT1G(rgba, &block.color);
			
			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressBC4(const Image * image, const nvtt::OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	ColorBlock rgba;
	AlphaBlockDXT5 block;
	
	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			rgba.init(image, x, y);

			if (compressionOptions.quality == Quality_Highest)
			{
				compressBlock_BruteForce(rgba, &block);
			}
			else
			{
				QuickCompress::compressDXT5A(rgba, &block);
			}

			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


void nv::compressBC5(const Image * image, const nvtt::OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	const uint w = image->width();
	const uint h = image->height();

	ColorBlock xcolor;
	ColorBlock ycolor;

	BlockATI2 block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			
			xcolor.init(image, x, y);
			xcolor.splatX();
			
			ycolor.init(image, x, y);
			ycolor.splatY();

			if (compressionOptions.quality == Quality_Highest)
			{
				compressBlock_BruteForce(xcolor, &block.x);
				compressBlock_BruteForce(ycolor, &block.y);
			}
			else
			{
				QuickCompress::compressDXT5A(xcolor, &block.x);
				QuickCompress::compressDXT5A(ycolor, &block.y);
			}

			if (outputOptions.outputHandler != NULL) {
				outputOptions.outputHandler->writeData(&block, sizeof(block));
			}
		}
	}
}


#if defined(HAVE_S3QUANT)

void nv::s3CompressDXT1(const Image * image, const nvtt::OutputOptions::Private & outputOptions)
{
	const uint w = image->width();
	const uint h = image->height();
	
	float error = 0.0f;

	BlockDXT1 dxtBlock3;
	BlockDXT1 dxtBlock4;
	ColorBlock block;

	for (uint y = 0; y < h; y += 4) {
		for (uint x = 0; x < w; x += 4) {
			block.init(image, x, y);

			// Init rgb block.
			RGBBlock rgbBlock;
			rgbBlock.n = 16;
			for (uint i = 0; i < 16; i++) {
				rgbBlock.colorChannel[i][0] = clamp(float(block.color(i).r) / 255.0f, 0.0f, 1.0f);
				rgbBlock.colorChannel[i][1] = clamp(float(block.color(i).g) / 255.0f, 0.0f, 1.0f);
				rgbBlock.colorChannel[i][2] = clamp(float(block.color(i).b) / 255.0f, 0.0f, 1.0f);
			}
			rgbBlock.weight[0] = 1.0f;
			rgbBlock.weight[1] = 1.0f;
			rgbBlock.weight[2] = 1.0f;

			rgbBlock.inLevel = 4;
			CodeRGBBlock(&rgbBlock);

			// Copy results to DXT block.
			dxtBlock4.col0.r = rgbBlock.endPoint[0][0];
			dxtBlock4.col0.g = rgbBlock.endPoint[0][1];
			dxtBlock4.col0.b = rgbBlock.endPoint[0][2];

			dxtBlock4.col1.r = rgbBlock.endPoint[1][0];
			dxtBlock4.col1.g = rgbBlock.endPoint[1][1];
			dxtBlock4.col1.b = rgbBlock.endPoint[1][2];

			dxtBlock4.setIndices(rgbBlock.index);

			if (dxtBlock4.col0.u < dxtBlock4.col1.u) {
				swap(dxtBlock4.col0.u, dxtBlock4.col1.u);
				dxtBlock4.indices ^= 0x55555555;
			}

			uint error4 = blockError(block, dxtBlock4);

			rgbBlock.inLevel = 3;

			CodeRGBBlock(&rgbBlock);

			// Copy results to DXT block.
			dxtBlock3.col0.r = rgbBlock.endPoint[0][0];
			dxtBlock3.col0.g = rgbBlock.endPoint[0][1];
			dxtBlock3.col0.b = rgbBlock.endPoint[0][2];

			dxtBlock3.col1.r = rgbBlock.endPoint[1][0];
			dxtBlock3.col1.g = rgbBlock.endPoint[1][1];
			dxtBlock3.col1.b = rgbBlock.endPoint[1][2];

			dxtBlock3.setIndices(rgbBlock.index);

			if (dxtBlock3.col0.u > dxtBlock3.col1.u) {
				swap(dxtBlock3.col0.u, dxtBlock3.col1.u);
				dxtBlock3.indices ^= (~dxtBlock3.indices  >> 1) & 0x55555555;
			}

			uint error3 = blockError(block, dxtBlock3);

			if (error3 < error4) {
				error += error3;

				if (outputOptions.outputHandler != NULL) {
					outputOptions.outputHandler->writeData(&dxtBlock3, sizeof(dxtBlock3));
				}
			}
			else {
				error += error4;

				if (outputOptions.outputHandler != NULL) {
					outputOptions.outputHandler->writeData(&dxtBlock4, sizeof(dxtBlock4));
				}
			}
		}
	}

	printf("error = %f\n", error/((w+3)/4 * (h+3)/4));
}

#endif // defined(HAVE_S3QUANT)


#if defined(HAVE_ATITC)

void nv::atiCompressDXT1(const Image * image, const OutputOptions::Private & outputOptions)
{
	// Init source texture
	ATI_TC_Texture srcTexture;
	srcTexture.dwSize = sizeof(srcTexture);
	srcTexture.dwWidth = image->width();
	srcTexture.dwHeight = image->height();
	srcTexture.dwPitch = image->width() * 4;
	srcTexture.format = ATI_TC_FORMAT_ARGB_8888;
	srcTexture.dwDataSize = ATI_TC_CalculateBufferSize(&srcTexture);
	srcTexture.pData = (ATI_TC_BYTE*) image->pixels();

	// Init dest texture
	ATI_TC_Texture destTexture;
	destTexture.dwSize = sizeof(destTexture);
	destTexture.dwWidth = image->width();
	destTexture.dwHeight = image->height();
	destTexture.dwPitch = 0;
	destTexture.format = ATI_TC_FORMAT_DXT1;
	destTexture.dwDataSize = ATI_TC_CalculateBufferSize(&destTexture);
	destTexture.pData = (ATI_TC_BYTE*) mem::malloc(destTexture.dwDataSize);

	// Compress
	ATI_TC_ConvertTexture(&srcTexture, &destTexture, NULL, NULL, NULL, NULL);

	if (outputOptions.outputHandler != NULL) {
		outputOptions.outputHandler->writeData(destTexture.pData, destTexture.dwDataSize);
	}
}

#endif // defined(HAVE_ATITC)
