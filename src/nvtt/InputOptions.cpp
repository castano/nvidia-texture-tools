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

#include <string.h> // memcpy

#include <nvcore/Memory.h>

#include "nvtt.h"
#include "InputOptions.h"

using namespace nv;
using namespace nvtt;

namespace
{

	static int countMipmaps(int w, int h, int d)
	{
		int mipmap = 0;
		
		while (w != 1 && h != 1) {
			w = max(1, w / 2);
			h = max(1, h / 2);
			d = max(1, d / 2);
			mipmap++;
		}
		
		return mipmap + 1;
	}

} // namespace


/// Constructor.
InputOptions::InputOptions() : m(*new InputOptions::Private())
{ 
	reset();
}

// Delete images.
InputOptions::~InputOptions()
{
	resetTextureLayout();
	
	delete &m;
}


// Reset input options.
void InputOptions::reset()
{
	m.wrapMode = WrapMode_Repeat;
	m.textureType = TextureType_2D;
	m.inputFormat = InputFormat_BGRA_8UB;

	m.enableColorDithering = false;
	m.enableAlphaDithering = false;
	m.binaryAlpha = false;
	m.alphaThreshold = 127;

	m.alphaTransparency = true;

	m.inputGamma = 2.2f;
	m.outputGamma = 2.2f;
	
	m.colorTransform = ColorTransform_None;
	m.linearTransform = Matrix(identity);

	m.generateMipmaps = false;
	m.maxLevel = -1;
	m.mipmapFilter = MipmapFilter_Box;

	m.kaiserWidth = 10;
	m.kaiserAlpha = 8.0f;
	m.kaiserStretch = 0.75f;

	m.normalizeMipmaps = false;
	m.convertToNormalMap = false;
	m.heightFactors.set(0.0f, 0.0f, 0.0f, 1.0f);
	m.bumpFrequencyScale = Vector4(1.0f, 0.5f, 0.25f, 0.125f) / (1.0f + 0.5f + 0.25f + 0.125f);
}


// Setup the input image.
void InputOptions::setTextureLayout(TextureType type, int width, int height, int depth /*= 1*/)
{
	// Validate arguments.
	nvCheck(width >= 0);
	nvCheck(height >= 0);
	nvCheck(depth >= 0);

	// Correct arguments.
	if (width == 0) width = 1;
	if (height == 0) height = 1;
	if (depth == 0) depth = 1;

	// Delete previous images.
	resetTextureLayout();
	
	m.textureType = type;
	
	// Allocate images.
	m.mipmapCount = countMipmaps(width, height, depth);
	m.faceCount = (type == TextureType_Cube) ? 6 : 1;
	m.imageCount = m.mipmapCount * m.faceCount;
	
	m.images = new Private::Image[m.imageCount];
	
	for(int f = 0; f < m.faceCount; f++)
	{
		int w = width;
		int h = height;
		int d = depth;

		for (int mipLevel = 0; mipLevel < m.mipmapCount; mipLevel++)
		{
			Private::Image & img = m.images[f * m.mipmapCount + mipLevel];
			img.width = w;
			img.height = h;
			img.depth = d;
			img.mipLevel = mipLevel;
			img.face = f;
			
			img.data = NULL;
			
			w = max(1, w / 2);
			h = max(1, h / 2);
			d = max(1, d / 2);
		}
	}
}


void InputOptions::resetTextureLayout()
{
	if (m.images != NULL)
	{
		// Delete image array.
		delete [] m.images;
		m.images = NULL;

		m.faceCount = 0;
		m.mipmapCount = 0;
		m.imageCount = 0;
	}
}


// Copies the data to our internal structures.
bool InputOptions::setMipmapData(const void * data, int width, int height, int depth /*= 1*/, int face /*= 0*/, int mipLevel /*= 0*/)
{
	nvCheck(depth == 1);
	
	const int idx = face * m.mipmapCount + mipLevel;
	
	if (m.images[idx].width != width || m.images[idx].height != height || m.images[idx].depth != depth || m.images[idx].mipLevel != mipLevel || m.images[idx].face != face)
	{
		// Invalid dimension or index.
		return false;
	}
	
	m.images[idx].data = new nv::Image();
	m.images[idx].data->allocate(width, height);
	memcpy(m.images[idx].data->pixels(), data, width * height * 4); 
	
	return true;
}


/// Describe the format of the input.
void InputOptions::setFormat(InputFormat format, bool alphaTransparency)
{
	m.inputFormat = format;
	m.alphaTransparency = alphaTransparency;
}


/// Set gamma settings.
void InputOptions::setGamma(float inputGamma, float outputGamma)
{
	m.inputGamma = inputGamma;
	m.outputGamma = outputGamma;
}


/// Set texture wrappign mode.
void InputOptions::setWrapMode(WrapMode mode)
{
	m.wrapMode = mode;
}


/// Set mipmapping options.
void InputOptions::setMipmapping(bool generateMipmaps, MipmapFilter filter/*= MipmapFilter_Box*/, int maxLevel/*= -1*/)
{
	m.generateMipmaps = generateMipmaps;
	m.mipmapFilter = filter;
	m.maxLevel = maxLevel;
}

/// Set Kaiser filter parameters.
void InputOptions::setKaiserParameters(int width, float alpha, float stretch)
{
	m.kaiserWidth = width;
	m.kaiserAlpha = alpha;
	m.kaiserStretch = stretch;
}

/// Set quantization options.
/// @warning Do not enable dithering unless you know what you are doing. Quantization 
/// introduces errors. It's better to let the compressor quantize the result to 
/// minimize the error, instead of quantizing the data before handling it to
/// the compressor.
void InputOptions::setQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold/*= 127*/)
{
	m.enableColorDithering = colorDithering;
	m.enableAlphaDithering = alphaDithering;
	m.binaryAlpha = binaryAlpha;
	m.alphaThreshold = alphaThreshold;
}


/// Indicate whether input is a normal map or not.
void InputOptions::setNormalMap(bool b)
{
	m.normalMap = b;
}

/// Enable normal map conversion.
void InputOptions::setConvertToNormalMap(bool convert)
{
	m.convertToNormalMap = convert;
}

/// Set height evaluation factors.
void InputOptions::setHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale)
{
	// Do not normalize height factors.
//	float total = redScale + greenScale + blueScale + alphaScale;
	m.heightFactors = Vector4(redScale, greenScale, blueScale, alphaScale);
}

/// Set normal map conversion filter.
void InputOptions::setNormalFilter(float small, float medium, float big, float large)
{
	float total = small + medium + big + large;
	m.bumpFrequencyScale = Vector4(small, medium, big, large) / total;
}

/// Enable mipmap normalization.
void InputOptions::setNormalizeMipmaps(bool normalize)
{
	m.normalizeMipmaps = normalize;
}

/// Set color transform.
void InputOptions::setColorTransform(ColorTransform t)
{
	m.colorTransform = t;
}

// Set linear transform for the given channel.
void InputOptions::setLinearTransfrom(int channel, float w0, float w1, float w2, float w3)
{
	nvCheck(channel >= 0 && channel < 4);

	Vector4 w(w0, w1, w2, w3);
	//m.linearTransform.setRow(channel, w);
}
