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

#include "Texture.h"

#include <nvmath/Vector.h>
#include <nvmath/Matrix.h>

#include <nvimage/Filter.h>

using namespace nv;
using namespace nvtt;

namespace
{
	// 1 -> 1, 2 -> 2, 3 -> 2, 4 -> 4, 5 -> 4, ...
	static uint previousPowerOfTwo(const uint v)
	{
		return nextPowerOfTwo(v + 1) / 2;
	}

	static uint nearestPowerOfTwo(const uint v)
	{
		const uint np2 = nextPowerOfTwo(v);
		const uint pp2 = previousPowerOfTwo(v);

		if (np2 - v <= v - pp2)
		{
			return np2;
		}
		else
		{
			return pp2;
		}
	}
}


Texture::Texture() : m(new Texture::Private())
{
}

Texture::Texture(const Texture & tex) : m(tex.m)
{
	m->addRef();
}

Texture::~Texture()
{
	m->release();
	m = NULL;
}

void Texture::operator=(const Texture & tex)
{
	tex.m->addRef();
	m = tex.m;
	m->release();
}

void Texture::detach()
{
	if (m->refCount() > 1)
	{
		m = new Texture::Private(*m);
		m->addRef();
		nvDebugCheck(m->refCount() == 1);
	}
}

void Texture::setType(TextureType type)
{
	if (m->type != type)
	{
		detach();
		m->type = type;
	}
}

void Texture::setWrapMode(WrapMode wrapMode)
{
	if (m->wrapMode != wrapMode)
	{
		detach();
		m->wrapMode = wrapMode;
	}
}

void Texture::setAlphaMode(AlphaMode alphaMode)
{
	if (m->alphaMode != alphaMode)
	{
		detach();
		m->alphaMode = alphaMode;
	}
}

void Texture::setNormalMap(bool isNormalMap)
{
	if (m->isNormalMap != isNormalMap)
	{
		detach();
		m->isNormalMap = isNormalMap;
	}
}

bool Texture::load(const char * fileName)
{
	// @@ Not implemented.
	return false;
}

void Texture::setTexture2D(InputFormat format, int w, int h, int idx, void * data)
{
	// @@ Not implemented.
}


void Texture::resize(int w, int h, ResizeFilter filter)
{
	if (m->imageArray.count() > 0)
	{
		if (w == m->imageArray[0].width() && h == m->imageArray[0].height()) return;
	}

	// @TODO: if cubemap, make sure w == h.

	detach();

	foreach(i, m->imageArray)
	{
		FloatImage::WrapMode wrapMode = (FloatImage::WrapMode)m->wrapMode;

		if (m->alphaMode == AlphaMode_Transparency)
		{
			if (filter == ResizeFilter_Box)
			{
				BoxFilter filter;
				m->imageArray[i].resize(filter, w, h, wrapMode, 3);
			}
			else if (filter == ResizeFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i].resize(filter, w, h, wrapMode, 3);
			}
			else if (filter == ResizeFilter_Kaiser)
			{
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i].resize(filter, w, h, wrapMode, 3);
			}
			else //if (filter == ResizeFilter_Mitchell)
			{
				nvDebugCheck(filter == ResizeFilter_Mitchell);
				MitchellFilter filter;
				m->imageArray[i].resize(filter, w, h, wrapMode, 3);
			}
		}
		else
		{
			if (filter == ResizeFilter_Box)
			{
				BoxFilter filter;
				m->imageArray[i].resize(filter, w, h, wrapMode);
			}
			else if (filter == ResizeFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i].resize(filter, w, h, wrapMode);
			}
			else if (filter == ResizeFilter_Kaiser)
			{
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i].resize(filter, w, h, wrapMode);
			}
			else //if (filter == ResizeFilter_Mitchell)
			{
				nvDebugCheck(filter == ResizeFilter_Mitchell);
				MitchellFilter filter;
				m->imageArray[i].resize(filter, w, h, wrapMode);
			}
		}
	}
}

void Texture::resize(int maxExtent, RoundMode roundMode, ResizeFilter filter)
{
	if (m->imageArray.count() > 0)
	{
		int w = m->imageArray[0].width();
		int h = m->imageArray[0].height();

		nvDebugCheck(w > 0);
		nvDebugCheck(h > 0);

		if (roundMode != RoundMode_None)
		{
			// rounded max extent should never be higher than original max extent.
			maxExtent = previousPowerOfTwo(maxExtent);
		}

		// Scale extents without changing aspect ratio.
		int maxwh = max(w, h);
		if (maxExtent != 0 && maxwh > maxExtent)
		{
			w = max((w * maxExtent) / maxwh, 1);
			h = max((h * maxExtent) / maxwh, 1);
		}

		// Round to power of two.
		if (roundMode == RoundMode_ToNextPowerOfTwo)
		{
			w = nextPowerOfTwo(w);
			h = nextPowerOfTwo(h);
		}
		else if (roundMode == RoundMode_ToNearestPowerOfTwo)
		{
			w = nearestPowerOfTwo(w);
			h = nearestPowerOfTwo(h);
		}
		else if (roundMode == RoundMode_ToPreviousPowerOfTwo)
		{
			w = previousPowerOfTwo(w);
			h = previousPowerOfTwo(h);
		}

		resize(w, h, filter);
	}
}

bool Texture::buildNextMipmap(MipmapFilter filter)
{
	if (m->imageArray.count() > 0)
	{
		int w = m->imageArray[0].width();
		int h = m->imageArray[0].height();

		nvDebugCheck(w > 0);
		nvDebugCheck(h > 0);

        if (w == 1 && h == 1)
        {
            return false;
        }
    }

    detach();

	foreach(i, m->imageArray)
	{
		FloatImage::WrapMode wrapMode = (FloatImage::WrapMode)m->wrapMode;

		if (m->alphaMode == AlphaMode_Transparency)
		{
			if (filter == MipmapFilter_Box)
			{
				BoxFilter filter;
				m->imageArray[i].downSample(filter, wrapMode, 3);
			}
			else if (filter == MipmapFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i].downSample(filter, wrapMode, 3);
			}
			else if (filter == MipmapFilter_Kaiser)
			{
				nvDebugCheck(filter == MipmapFilter_Kaiser);
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i].downSample(filter, wrapMode, 3);
			}
		}
		else
		{
			if (filter == MipmapFilter_Box)
			{
				m->imageArray[i].fastDownSample();
			}
			else if (filter == MipmapFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i].downSample(filter, wrapMode);
			}
			else //if (filter == MipmapFilter_Kaiser)
			{
				nvDebugCheck(filter == MipmapFilter_Kaiser);
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i].downSample(filter, wrapMode);
			}
		}
	}

    return true;
}

// Color transforms.
void Texture::toLinear(float gamma)
{
	if (equal(gamma, 1.0f)) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i].toLinear(0, 3, gamma);
	}
}

void Texture::toGamma(float gamma)
{
	if (equal(gamma, 1.0f)) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i].toGamma(0, 3, gamma);
	}
}

void Texture::transform(const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4])
{
	detach();

	Matrix xform(
		Vector4(w0[0], w0[1], w0[2], w0[3]),
		Vector4(w1[0], w1[1], w1[2], w1[3]),
		Vector4(w2[0], w2[1], w2[2], w2[3]),
		Vector4(w3[0], w3[1], w3[2], w3[3]));

	Vector4 voffset(offset[0], offset[1], offset[2], offset[3]);

	foreach(i, m->imageArray)
	{
		m->imageArray[i].transform(0, xform, voffset);
	}
}

void Texture::swizzle(int r, int g, int b, int a)
{
	if (r == 0 && g == 1 && b == 2 && a == 3) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i].swizzle(0, r, g, b, a);
	}
}

void Texture::scaleBias(int channel, float scale, float bias)
{
	if (equal(scale, 1.0f) && equal(bias, 0.0f)) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i].scaleBias(channel, 1, scale, bias);
	}
}

void Texture::normalizeNormals()
{
	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i].normalize(0);
	}
}

void Texture::blend(float r, float g, float b, float a)
{
	detach();

	foreach(i, m->imageArray)
	{
		// @@ Not implemented.
	}
}

void Texture::premultiplyAlpha()
{
	detach();

	// @@ Not implemented.
}


