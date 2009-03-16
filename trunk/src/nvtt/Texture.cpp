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
#include <nvmath/Color.h>

#include <nvimage/Filter.h>
#include <nvimage/ImageIO.h>
#include <nvimage/NormalMap.h>

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


TexImage::TexImage() : m(new TexImage::Private())
{
}

TexImage::TexImage(const TexImage & tex) : m(tex.m)
{
	m->addRef();
}

TexImage::~TexImage()
{
	m->release();
	m = NULL;
}

void TexImage::operator=(const TexImage & tex)
{
	tex.m->addRef();
	m = tex.m;
	m->release();
}

void TexImage::detach()
{
	if (m->refCount() > 1)
	{
		m = new TexImage::Private(*m);
		m->addRef();
		nvDebugCheck(m->refCount() == 1);
	}
}

void TexImage::setTextureType(TextureType type)
{
	if (m->type != type)
	{
		detach();

		m->type = type;

		if (type == TextureType_2D)
		{
			// @@ Free images.
			m->imageArray.resize(1, NULL);
		}
		else
		{
			nvCheck (type == TextureType_Cube);
			m->imageArray.resize(6, NULL);
		}
	}
}

void TexImage::setWrapMode(WrapMode wrapMode)
{
	if (m->wrapMode != wrapMode)
	{
		detach();
		m->wrapMode = wrapMode;
	}
}

void TexImage::setAlphaMode(AlphaMode alphaMode)
{
	if (m->alphaMode != alphaMode)
	{
		detach();
		m->alphaMode = alphaMode;
	}
}

void TexImage::setNormalMap(bool isNormalMap)
{
	if (m->isNormalMap != isNormalMap)
	{
		detach();
		m->isNormalMap = isNormalMap;
	}
}

int TexImage::width() const
{
	if (m->imageArray.count() > 0)
	{
		return m->imageArray[0]->width();
	}
	return 0;
}

int TexImage::height() const
{
	if (m->imageArray.count() > 0)
	{
		return m->imageArray[0]->height();
	}
	return 0;
}

int TexImage::depth() const
{
	return 0;
}

int TexImage::faceCount() const
{
	return m->imageArray.count();
}

TextureType TexImage::textureType() const
{
	return m->type;
}

WrapMode TexImage::wrapMode() const
{
	return m->wrapMode;
}

AlphaMode TexImage::alphaMode() const
{
	return m->alphaMode;
}

bool TexImage::isNormalMap() const
{
	return m->isNormalMap;
}

bool TexImage::load(const char * fileName)
{
	// @@ Add support for DDS textures!

	AutoPtr<FloatImage> img(ImageIO::loadFloat(fileName));

	if (img == NULL)
	{
		return false;
	}

	detach();

	m->imageArray.resize(1);
	m->imageArray[0] = img.release();

	return true;
}

bool TexImage::setImage2D(InputFormat format, int w, int h, int idx, const void * restrict data)
{
	if (idx >= m->imageArray.count())
	{
		return false;
	}

	FloatImage * img = m->imageArray[idx];
	if (img->width() != w || img->height() != h)
	{
		return false;
	}

	detach();

	const int count = w * h;

	float * restrict rdst = img->channel(0);
	float * restrict gdst = img->channel(1);
	float * restrict bdst = img->channel(2);
	float * restrict adst = img->channel(3);

	if (format == InputFormat_BGRA_8UB)
	{
		const Color32 * src = (const Color32 *)data;

		try {
			for (int i = 0; i < count; i++)
			{
				rdst[i] = src[i].r;
				gdst[i] = src[i].g;
				bdst[i] = src[i].b;
				adst[i] = src[i].a;
			}
		}
		catch(...) {
			return false;
		}
	}
	else if (format == InputFormat_RGBA_32F)
	{
		const float * src = (const float *)data;

		try {
			for (int i = 0; i < count; i++)
			{
				rdst[i] = src[4 * i + 0];
				gdst[i] = src[4 * i + 1];
				bdst[i] = src[4 * i + 2];
				adst[i] = src[4 * i + 3];
			}
		}
		catch(...) {
			return false;
		}
	}

	return true;
}

bool TexImage::setImage2D(InputFormat format, int w, int h, int idx, const void * restrict r, const void * restrict g, const void * restrict b, const void * restrict a)
{
	if (idx >= m->imageArray.count())
	{
		return false;
	}

	FloatImage * img = m->imageArray[idx];
	if (img->width() != w || img->height() != h)
	{
		return false;
	}

	detach();

	const int count = w * h;

	float * restrict rdst = img->channel(0);
	float * restrict gdst = img->channel(1);
	float * restrict bdst = img->channel(2);
	float * restrict adst = img->channel(3);

	if (format == InputFormat_BGRA_8UB)
	{
		const uint8 * restrict rsrc = (const uint8 *)r;
		const uint8 * restrict gsrc = (const uint8 *)g;
		const uint8 * restrict bsrc = (const uint8 *)b;
		const uint8 * restrict asrc = (const uint8 *)a;

		try {
			for (int i = 0; i < count; i++) rdst[i] = float(rsrc[i]) / 255.0f;
			for (int i = 0; i < count; i++) gdst[i] = float(gsrc[i]) / 255.0f;
			for (int i = 0; i < count; i++) bdst[i] = float(bsrc[i]) / 255.0f;
			for (int i = 0; i < count; i++) adst[i] = float(asrc[i]) / 255.0f;
		}
		catch(...) {
			return false;
		}
	}
	else if (format == InputFormat_RGBA_32F)
	{
		const float * rsrc = (const float *)r;
		const float * gsrc = (const float *)g;
		const float * bsrc = (const float *)b;
		const float * asrc = (const float *)a;

		try {
			memcpy(rdst, rsrc, count * sizeof(float));
			memcpy(gdst, gsrc, count * sizeof(float));
			memcpy(bdst, bsrc, count * sizeof(float));
			memcpy(adst, asrc, count * sizeof(float));
		}
		catch(...) {
			return false;
		}
	}

	return true;
}

void TexImage::resize(int w, int h, ResizeFilter filter)
{
	if (m->imageArray.count() > 0)
	{
		if (w == m->imageArray[0]->width() && h == m->imageArray[0]->height()) return;
	}

	// @@ TODO: if cubemap, make sure w == h.

	detach();

	foreach(i, m->imageArray)
	{
		FloatImage::WrapMode wrapMode = (FloatImage::WrapMode)m->wrapMode;

		if (m->alphaMode == AlphaMode_Transparency)
		{
			if (filter == ResizeFilter_Box)
			{
				BoxFilter filter;
				m->imageArray[i]->resize(filter, w, h, wrapMode, 3);
			}
			else if (filter == ResizeFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i]->resize(filter, w, h, wrapMode, 3);
			}
			else if (filter == ResizeFilter_Kaiser)
			{
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i]->resize(filter, w, h, wrapMode, 3);
			}
			else //if (filter == ResizeFilter_Mitchell)
			{
				nvDebugCheck(filter == ResizeFilter_Mitchell);
				MitchellFilter filter;
				m->imageArray[i]->resize(filter, w, h, wrapMode, 3);
			}
		}
		else
		{
			if (filter == ResizeFilter_Box)
			{
				BoxFilter filter;
				m->imageArray[i]->resize(filter, w, h, wrapMode);
			}
			else if (filter == ResizeFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i]->resize(filter, w, h, wrapMode);
			}
			else if (filter == ResizeFilter_Kaiser)
			{
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i]->resize(filter, w, h, wrapMode);
			}
			else //if (filter == ResizeFilter_Mitchell)
			{
				nvDebugCheck(filter == ResizeFilter_Mitchell);
				MitchellFilter filter;
				m->imageArray[i]->resize(filter, w, h, wrapMode);
			}
		}
	}
}

void TexImage::resize(int maxExtent, RoundMode roundMode, ResizeFilter filter)
{
	if (m->imageArray.count() > 0)
	{
		int w = m->imageArray[0]->width();
		int h = m->imageArray[0]->height();

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

bool TexImage::buildNextMipmap(MipmapFilter filter)
{
	if (m->imageArray.count() > 0)
	{
		int w = m->imageArray[0]->width();
		int h = m->imageArray[0]->height();

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
				m->imageArray[i]->downSample(filter, wrapMode, 3);
			}
			else if (filter == MipmapFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i]->downSample(filter, wrapMode, 3);
			}
			else if (filter == MipmapFilter_Kaiser)
			{
				nvDebugCheck(filter == MipmapFilter_Kaiser);
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i]->downSample(filter, wrapMode, 3);
			}
		}
		else
		{
			if (filter == MipmapFilter_Box)
			{
				m->imageArray[i]->fastDownSample();
			}
			else if (filter == MipmapFilter_Triangle)
			{
				TriangleFilter filter;
				m->imageArray[i]->downSample(filter, wrapMode);
			}
			else //if (filter == MipmapFilter_Kaiser)
			{
				nvDebugCheck(filter == MipmapFilter_Kaiser);
				//KaiserFilter filter(inputOptions.kaiserWidth);
				//filter.setParameters(inputOptions.kaiserAlpha, inputOptions.kaiserStretch);
				KaiserFilter filter(3);
				m->imageArray[i]->downSample(filter, wrapMode);
			}
		}
	}

    return true;
}

// Color transforms.
void TexImage::toLinear(float gamma)
{
	if (equal(gamma, 1.0f)) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i]->toLinear(0, 3, gamma);
	}
}

void TexImage::toGamma(float gamma)
{
	if (equal(gamma, 1.0f)) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i]->toGamma(0, 3, gamma);
	}
}

void TexImage::transform(const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4])
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
		m->imageArray[i]->transform(0, xform, voffset);
	}
}

void TexImage::swizzle(int r, int g, int b, int a)
{
	if (r == 0 && g == 1 && b == 2 && a == 3) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i]->swizzle(0, r, g, b, a);
	}
}

void TexImage::scaleBias(int channel, float scale, float bias)
{
	if (equal(scale, 1.0f) && equal(bias, 0.0f)) return;

	detach();

	foreach(i, m->imageArray)
	{
		m->imageArray[i]->scaleBias(channel, 1, scale, bias);
	}
}

void TexImage::blend(float r, float g, float b, float a)
{
	detach();

	foreach(i, m->imageArray)
	{
		// @@ Not implemented.
	}
}

void TexImage::premultiplyAlpha()
{
	detach();

	// @@ Not implemented.
}


void TexImage::toGreyScale(float redScale, float greenScale, float blueScale, float alphaScale)
{
	detach();

	// @@ Not implemented.
}

// Set normal map options.
void TexImage::toNormalMap(float sm, float medium, float big, float large)
{
	detach();


	// @@ Not implemented.
}

void TexImage::toHeightMap()
{
	detach();

	// @@ Not implemented.
}

void TexImage::normalizeNormalMap()
{
	//nvCheck(m->isNormalMap);

	detach();

	foreach(i, m->imageArray)
	{
		nv::normalizeNormalMap(m->imageArray[i]);
	}
}

// Compress.
void TexImage::outputCompressed(const CompressionOptions & compressionOptions, const OutputOptions & outputOptions)
{
	// @@ Not implemented.
}
