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

#include "nvtt.h"
#include "CompressionOptions.h"

using namespace nv;
using namespace nvtt;


/// Constructor. Sets compression options to the default values.
CompressionOptions::CompressionOptions() : m(*new CompressionOptions::Private())
{
	reset();
}


/// Destructor.
CompressionOptions::~CompressionOptions()
{
	delete &m;
}


/// Set default compression options.
void CompressionOptions::reset()
{
	m.format = Format_DXT1;
	m.quality = Quality_Normal;
	m.colorWeight.set(1.0f, 1.0f, 1.0f);
	m.useCuda = true;
	m.bitcount = 32;
	m.bmask = 0x000000FF;
	m.gmask = 0x0000FF00;
	m.rmask = 0x00FF0000;
	m.amask = 0xFF000000;
}


/// Set desired compression format.
void CompressionOptions::setFormat(Format format)
{
	m.format = format;
}


/// Set compression quality settings.
void CompressionOptions::setQuality(Quality quality, float errorThreshold /*= 0.5f*/)
{
	m.quality = quality;
	m.errorThreshold = errorThreshold;
}


/// Set the weights of each color channel. 
/// The choice for these values is subjective. In many case uniform color weights 
/// (1.0, 1.0, 1.0) work very well. A popular choice is to use the NTSC luma encoding 
/// weights (0.2126, 0.7152, 0.0722), but I think that blue contributes to our 
/// perception more than a 7%. A better choice in my opinion is (3, 4, 2). Ideally
/// the compressor should use a non linear colour metric as described here:
/// http://www.compuphase.com/cmetric.htm
void CompressionOptions::setColorWeights(float red, float green, float blue)
{
	float total = red + green + blue;
	float x = blue / total;
	float y = green / total;

	m.colorWeight.set(x, y, 1.0f - x - y);
}


/// Enable or disable hardware compression.
void CompressionOptions::enableHardwareCompression(bool enable)
{
	m.useCuda = enable;
}


/// Set color mask to describe the RGB/RGBA format.
void CompressionOptions::setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
{
	m.bitcount = bitcount;
	m.rmask = rmask;
	m.gmask = gmask;
	m.bmask = bmask;
	m.amask = amask;
}

/// Use external compressor.
void CompressionOptions::setExternalCompressor(const char * name)
{
	m.externalCompressor = name;
}

