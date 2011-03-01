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

#include "TexImage.h"

#include "nvmath/Vector.h"
#include "nvmath/Matrix.h"
#include "nvmath/Color.h"
#include "nvmath/Half.h"

#include "nvimage/Filter.h"
#include "nvimage/ImageIO.h"
#include "nvimage/NormalMap.h"
#include "nvimage/BlockDXT.h"
#include "nvimage/ColorBlock.h"
#include "nvimage/PixelFormat.h"
#include "nvimage/ErrorMetric.h"

#include <float.h>

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
}

uint nv::countMipmaps(uint w, uint h, uint d)
{
    uint mipmap = 0;

    while (w != 1 || h != 1 || d != 1) {
        w = max(1U, w / 2);
        h = max(1U, h / 2);
        d = max(1U, d / 2);
        mipmap++;
    }

    return mipmap + 1;
}

uint nv::computeImageSize(uint w, uint h, uint d, uint bitCount, uint alignment, Format format)
{
    if (format == Format_RGBA) {
        return d * h * computePitch(w, bitCount, alignment);
    }
    else {
        // @@ Handle 3D textures. DXT and VTC have different behaviors.
        return ((w + 3) / 4) * ((h + 3) / 4) * blockSize(format);
    }
}

void nv::getTargetExtent(int & w, int & h, int & d, int maxExtent, RoundMode roundMode, TextureType textureType) {
    nvDebugCheck(w > 0);
    nvDebugCheck(h > 0);
    nvDebugCheck(d > 0);

    if (roundMode != RoundMode_None && maxExtent > 0)
    {
        // rounded max extent should never be higher than original max extent.
        maxExtent = previousPowerOfTwo(maxExtent);
    }

    // Scale extents without changing aspect ratio.
    int m = max(max(w, h), d);
    if (maxExtent > 0 && m > maxExtent)
    {
        w = max((w * maxExtent) / m, 1);
        h = max((h * maxExtent) / m, 1);
        d = max((d * maxExtent) / m, 1);
    }

    if (textureType == TextureType_2D)
    {
        d = 1;
    }
    else if (textureType == TextureType_Cube)
    {
        w = h = (w + h) / 2;
        d = 1;
    }

    // Round to power of two.
    if (roundMode == RoundMode_ToNextPowerOfTwo)
    {
        w = nextPowerOfTwo(w);
        h = nextPowerOfTwo(h);
        d = nextPowerOfTwo(d);
    }
    else if (roundMode == RoundMode_ToNearestPowerOfTwo)
    {
        w = nearestPowerOfTwo(w);
        h = nearestPowerOfTwo(h);
        d = nearestPowerOfTwo(d);
    }
    else if (roundMode == RoundMode_ToPreviousPowerOfTwo)
    {
        w = previousPowerOfTwo(w);
        h = previousPowerOfTwo(h);
        d = previousPowerOfTwo(d);
    }
}



TexImage::TexImage() : m(new TexImage::Private())
{
    m->addRef();
}

TexImage::TexImage(const TexImage & tex) : m(tex.m)
{
    if (m != NULL) m->addRef();
}

TexImage::~TexImage()
{
    if (m != NULL) m->release();
    m = NULL;
}

void TexImage::operator=(const TexImage & tex)
{
    if (tex.m != NULL) tex.m->addRef();
    if (m != NULL) m->release();
    m = tex.m;
}

void TexImage::detach()
{
    if (m->refCount() > 1)
    {
        m->release();
        m = new TexImage::Private(*m);
        m->addRef();
        nvDebugCheck(m->refCount() == 1);
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

bool TexImage::isNull() const
{
    return m->image == NULL;
}

int TexImage::width() const
{
    if (m->image != NULL) return m->image->width();
    return 0;
}

int TexImage::height() const
{
    if (m->image != NULL) return m->image->height();
    return 0;
}

int TexImage::depth() const
{
    if (m->image != NULL) return 1;
    return 0;
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

int TexImage::countMipmaps() const
{
    if (m->image == NULL) return 0;
    return ::countMipmaps(m->image->width(), m->image->height(), 1);
}

float TexImage::alphaTestCoverage(float alphaRef/*= 0.5*/) const
{
    if (m->image == NULL) return 0.0f;

    return m->image->alphaTestCoverage(alphaRef, 3);
}

float TexImage::average(int channel) const
{
    if (m->image == NULL) return 0.0f;

    float sum = 0.0f;
    const float * c = m->image->channel(channel);

    const uint count = m->image->width() * m->image->height();
    for (uint i = 0; i < count; i++) {
        sum += c[i];
    }

    return sum / count;
}

const float * TexImage::data() const
{
    return m->image->channel(0);
}

void TexImage::histogram(int channel, float rangeMin, float rangeMax, int binCount, int * binPtr) const
{
    // We assume it's clear in case we want to accumulate multiple histograms.
    //memset(bins, 0, sizeof(int)*count);

    if (m->image == NULL) return;

    const float * c = m->image->channel(channel);

    float scale = float(binCount) / rangeMax;
    float bias = - scale * rangeMin;

    const uint count = m->image->width() * m->image->height();
    for (uint i = 0; i < count; i++) {
        float f = c[i] * scale + bias;
        int idx = ifloor(f);
        if (idx < 0) idx = 0;
        if (idx > binCount-1) idx = binCount-1;
        binPtr[idx]++;
    }
}

void TexImage::range(int channel, float * rangeMin, float * rangeMax)
{
    Vector2 range(FLT_MAX, -FLT_MAX);

    FloatImage * img = m->image;
    float * c = img->channel(channel);

    const uint count = img->width() * img->height();
    for (uint p = 0; p < count; p++) {
        float f = c[p];
        if (f < range.x) range.x = f;
        if (f > range.y) 
            range.y = f;
    }

    *rangeMin = range.x;
    *rangeMax = range.y;
}


bool TexImage::load(const char * fileName, bool * hasAlpha/*= NULL*/)
{
    AutoPtr<FloatImage> img(ImageIO::loadFloat(fileName));
    if (img == NULL) {
        return false;
    }

    detach();

    if (hasAlpha != NULL) {
        *hasAlpha = (img->componentNum() == 4);
    }

    // @@ Have loadFloat allocate the image with the desired number of channels.
    img->resizeChannelCount(4);

    delete m->image;
    m->image = img.release();

    return true;
}

bool TexImage::save(const char * fileName) const
{
    if (m->image != NULL)
    {
        return ImageIO::saveFloat(fileName, m->image, 0, 4);
    }

    return false;
}

bool TexImage::setImage2D(nvtt::InputFormat format, int w, int h, const void * data)
{
    detach();

    if (m->image == NULL) {
        m->image = new FloatImage();
    }
    m->image->allocate(4, w, h);

    const int count = w * h;

    float * rdst = m->image->channel(0);
    float * gdst = m->image->channel(1);
    float * bdst = m->image->channel(2);
    float * adst = m->image->channel(3);

    if (format == InputFormat_BGRA_8UB)
    {
        const Color32 * src = (const Color32 *)data;

        try {
            for (int i = 0; i < count; i++)
            {
                rdst[i] = float(src[i].r) / 255.0f;
                gdst[i] = float(src[i].g) / 255.0f;
                bdst[i] = float(src[i].b) / 255.0f;
                adst[i] = float(src[i].a) / 255.0f;
            }
        }
        catch(...) {
            return false;
        }
    }
    else if (format == InputFormat_RGBA_16F)
    {
        const uint16 * src = (const uint16 *)data;

        try {
            for (int i = 0; i < count; i++)
            {
                ((uint32 *)rdst)[i] = half_to_float(src[4*i+0]);
                ((uint32 *)gdst)[i] = half_to_float(src[4*i+1]);
                ((uint32 *)bdst)[i] = half_to_float(src[4*i+2]);
                ((uint32 *)adst)[i] = half_to_float(src[4*i+3]);
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

bool TexImage::setImage2D(InputFormat format, int w, int h, const void * r, const void * g, const void * b, const void * a)
{
    detach();

    if (m->image == NULL) {
        m->image = new FloatImage();
    }
    m->image->allocate(4, w, h);

    const int count = w * h;

    float * rdst = m->image->channel(0);
    float * gdst = m->image->channel(1);
    float * bdst = m->image->channel(2);
    float * adst = m->image->channel(3);

    if (format == InputFormat_BGRA_8UB)
    {
        const uint8 * rsrc = (const uint8 *)r;
        const uint8 * gsrc = (const uint8 *)g;
        const uint8 * bsrc = (const uint8 *)b;
        const uint8 * asrc = (const uint8 *)a;

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
    else if (format == InputFormat_RGBA_16F)
    {
        const uint16 * rsrc = (const uint16 *)r;
        const uint16 * gsrc = (const uint16 *)g;
        const uint16 * bsrc = (const uint16 *)b;
        const uint16 * asrc = (const uint16 *)a;

        try {
            for (int i = 0; i < count; i++) ((uint32 *)rdst)[i] = half_to_float(rsrc[i]);
            for (int i = 0; i < count; i++) ((uint32 *)gdst)[i] = half_to_float(gsrc[i]);
            for (int i = 0; i < count; i++) ((uint32 *)bdst)[i] = half_to_float(bsrc[i]);
            for (int i = 0; i < count; i++) ((uint32 *)adst)[i] = half_to_float(asrc[i]);
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

bool TexImage::setImage2D(Format format, Decoder decoder, int w, int h, const void * data)
{
    if (format != nvtt::Format_BC1 && format != nvtt::Format_BC2 && format != nvtt::Format_BC3 && format != nvtt::Format_BC4 && format != nvtt::Format_BC5)
    {
        return false;
    }

    detach();

    if (m->image == NULL) {
        m->image = new FloatImage();
    }
    m->image->allocate(4, w, h);

    const int bw = (w + 3) / 4;
    const int bh = (h + 3) / 4;

    const uint bs = blockSize(format);

    const uint8 * ptr = (const uint8 *)data;

    try {
        for (int y = 0; y < bh; y++)
        {
            for (int x = 0; x < bw; x++)
            {
                ColorBlock colors;

		        if (format == nvtt::Format_BC1)
		        {
		            const BlockDXT1 * block = (const BlockDXT1 *)ptr;

		            if (decoder == Decoder_D3D10) {
			            block->decodeBlock(&colors, false);
		            }
		            else if (decoder == Decoder_D3D9) {
			            block->decodeBlock(&colors, false);
		            }
		            else if (decoder == Decoder_NV5x) {
			            block->decodeBlockNV5x(&colors);
		            }
		        }
		        else if (format == nvtt::Format_BC2)
		        {
		            const BlockDXT3 * block = (const BlockDXT3 *)ptr;

		            if (decoder == Decoder_D3D10) {
			            block->decodeBlock(&colors, false);
		            }
		            else if (decoder == Decoder_D3D9) {
			            block->decodeBlock(&colors, false);
		            }
		            else if (decoder == Decoder_NV5x) {
		    	        block->decodeBlockNV5x(&colors);
		            }
		        }
		        else if (format == nvtt::Format_BC3)
		        {
		            const BlockDXT5 * block = (const BlockDXT5 *)ptr;

		            if (decoder == Decoder_D3D10) {
			            block->decodeBlock(&colors, false);
		            }
		            else if (decoder == Decoder_D3D9) {
			            block->decodeBlock(&colors, false);
		            }
		            else if (decoder == Decoder_NV5x) {
    			        block->decodeBlockNV5x(&colors);
		            }
		        }
		        else if (format == nvtt::Format_BC4)
		        {
                    const BlockATI1 * block = (const BlockATI1 *)ptr;
                    block->decodeBlock(&colors, decoder == Decoder_D3D9);
                }
                else if (format == nvtt::Format_BC5)
                {
                    const BlockATI2 * block = (const BlockATI2 *)ptr;
                    block->decodeBlock(&colors, decoder == Decoder_D3D9);
                }

		        for (int yy = 0; yy < 4; yy++)
		        {
		            for (int xx = 0; xx < 4; xx++)
		            {
			            Color32 c = colors.color(xx, yy);

			            if (x * 4 + xx < w && y * 4 + yy < h)
			            {
			                m->image->pixel(x*4 + xx, y*4 + yy, 0) = float(c.r) * 1.0f/255.0f;
			                m->image->pixel(x*4 + xx, y*4 + yy, 1) = float(c.g) * 1.0f/255.0f;
			                m->image->pixel(x*4 + xx, y*4 + yy, 2) = float(c.b) * 1.0f/255.0f;
			                m->image->pixel(x*4 + xx, y*4 + yy, 3) = float(c.a) * 1.0f/255.0f;
			            }
		            }
		        }

		        ptr += bs;
	        }
	    }
    }
    catch(...) {
        return false;
    }

    return true;
}


static void getDefaultFilterWidthAndParams(int filter, float * filterWidth, float params[2])
{
    if (filter == ResizeFilter_Box) {
        *filterWidth = 0.5f;
    }
    else if (filter == ResizeFilter_Triangle) {
        *filterWidth = 1.0f;
    }
    else if (filter == ResizeFilter_Kaiser)
    {
        *filterWidth = 3.0f;
        params[0] = 4.0f;
        params[1] = 1.0f;
    }
    else //if (filter == ResizeFilter_Mitchell)
    {
        *filterWidth = 2.0f;
        params[0] = 1.0f / 3.0f;
        params[1] = 1.0f / 3.0f;
    }
}

void TexImage::resize(int w, int h, ResizeFilter filter)
{
    float filterWidth;
    float params[2];
    getDefaultFilterWidthAndParams(filter, &filterWidth, params);

    resize(w, h, filter, filterWidth, params);
}

void TexImage::resize(int w, int h, ResizeFilter filter, float filterWidth, const float * params)
{
    FloatImage * img = m->image;
    if (img == NULL || (w == img->width() && h == img->height())) {
        return;
    }

    detach();

    FloatImage::WrapMode wrapMode = (FloatImage::WrapMode)m->wrapMode;

    if (m->alphaMode == AlphaMode_Transparency)
    {
        if (filter == ResizeFilter_Box)
        {
            BoxFilter filter(filterWidth);
            img = img->resize(filter, w, h, wrapMode, 3);
        }
        else if (filter == ResizeFilter_Triangle)
        {
            TriangleFilter filter(filterWidth);
            img = img->resize(filter, w, h, wrapMode, 3);
        }
        else if (filter == ResizeFilter_Kaiser)
        {
            KaiserFilter filter(filterWidth);
            if (params != NULL) filter.setParameters(params[0], params[1]);
            img = img->resize(filter, w, h, wrapMode, 3);
        }
        else //if (filter == ResizeFilter_Mitchell)
        {
            nvDebugCheck(filter == ResizeFilter_Mitchell);
            MitchellFilter filter;
            if (params != NULL) filter.setParameters(params[0], params[1]);
            img = img->resize(filter, w, h, wrapMode, 3);
        }
    }
    else
    {
        if (filter == ResizeFilter_Box)
        {
            BoxFilter filter(filterWidth);
            img = img->resize(filter, w, h, wrapMode);
        }
        else if (filter == ResizeFilter_Triangle)
        {
            TriangleFilter filter(filterWidth);
            img = img->resize(filter, w, h, wrapMode);
        }
        else if (filter == ResizeFilter_Kaiser)
        {
            KaiserFilter filter(filterWidth);
            if (params != NULL) filter.setParameters(params[0], params[1]);
            img = img->resize(filter, w, h, wrapMode);
        }
        else //if (filter == ResizeFilter_Mitchell)
        {
            nvDebugCheck(filter == ResizeFilter_Mitchell);
            MitchellFilter filter;
            if (params != NULL) filter.setParameters(params[0], params[1]);
            img = img->resize(filter, w, h, wrapMode);
        }
    }

    delete m->image;
    m->image = img;
}

void TexImage::resize(int maxExtent, RoundMode roundMode, ResizeFilter filter)
{
    float filterWidth;
    float params[2];
    getDefaultFilterWidthAndParams(filter, &filterWidth, params);

    resize(maxExtent, roundMode, filter, filterWidth, params);
}

void TexImage::resize(int maxExtent, RoundMode roundMode, ResizeFilter filter, float filterWidth, const float * params)
{
    if (m->image == NULL) return;

    int w = m->image->width();
    int h = m->image->height();
    int d = 1;

    getTargetExtent(w, h, d, maxExtent, roundMode, nvtt::TextureType_2D);

    resize(w, h, filter, filterWidth, params);
}

bool TexImage::buildNextMipmap(MipmapFilter filter)
{
    float filterWidth;
    float params[2];
    getDefaultFilterWidthAndParams(filter, &filterWidth, params);

    return buildNextMipmap(filter, filterWidth, params);
}

bool TexImage::buildNextMipmap(MipmapFilter filter, float filterWidth, const float * params)
{
    FloatImage * img = m->image;
    if (img == NULL || (img->width() == 1 && img->height() == 1)) {
        return false;
    }

    detach();

    FloatImage::WrapMode wrapMode = (FloatImage::WrapMode)m->wrapMode;

    if (m->alphaMode == AlphaMode_Transparency)
    {
        if (filter == MipmapFilter_Box)
        {
            BoxFilter filter(filterWidth);
            img = img->downSample(filter, wrapMode, 3);
        }
        else if (filter == MipmapFilter_Triangle)
        {
            TriangleFilter filter(filterWidth);
            img = img->downSample(filter, wrapMode, 3);
        }
        else if (filter == MipmapFilter_Kaiser)
        {
            nvDebugCheck(filter == MipmapFilter_Kaiser);
            KaiserFilter filter(filterWidth);
            if (params != NULL) filter.setParameters(params[0], params[1]);
            img = img->downSample(filter, wrapMode, 3);
        }
    }
    else
    {
        if (filter == MipmapFilter_Box)
        {
            if (filterWidth == 0.5f) {
                img = img->fastDownSample();
            }
            else {
                BoxFilter filter(filterWidth);
                img = img->downSample(filter, wrapMode);
            }
        }
        else if (filter == MipmapFilter_Triangle)
        {
            TriangleFilter filter(filterWidth);
            img = img->downSample(filter, wrapMode);
        }
        else //if (filter == MipmapFilter_Kaiser)
        {
            nvDebugCheck(filter == MipmapFilter_Kaiser);
            KaiserFilter filter(filterWidth);
            if (params != NULL) filter.setParameters(params[0], params[1]);
            img = img->downSample(filter, wrapMode);
        }
    }

    delete m->image;
    m->image = img;

    return true;
}

// Color transforms.
void TexImage::toLinear(float gamma)
{
    if (m->image == NULL) return;
    if (equal(gamma, 1.0f)) return;

    detach();

    m->image->toLinear(0, 3, gamma);
}

void TexImage::toGamma(float gamma)
{
    if (m->image == NULL) return;
    if (equal(gamma, 1.0f)) return;

    detach();

    m->image->toGamma(0, 3, gamma);
}

void TexImage::transform(const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4])
{
    if (m->image == NULL) return;

    detach();

    Matrix xform(
        Vector4(w0[0], w0[1], w0[2], w0[3]),
        Vector4(w1[0], w1[1], w1[2], w1[3]),
        Vector4(w2[0], w2[1], w2[2], w2[3]),
        Vector4(w3[0], w3[1], w3[2], w3[3]));

    Vector4 voffset(offset[0], offset[1], offset[2], offset[3]);

    m->image->transform(0, xform, voffset);
}

void TexImage::swizzle(int r, int g, int b, int a)
{
    if (m->image == NULL) return;
    if (r == 0 && g == 1 && b == 2 && a == 3) return;

    detach();

    m->image->swizzle(0, r, g, b, a);
}

// color * scale + bias
void TexImage::scaleBias(int channel, float scale, float bias)
{
    if (m->image == NULL) return;
    if (equal(scale, 1.0f) && equal(bias, 0.0f)) return;

    detach();

    m->image->scaleBias(channel, 1, scale, bias);
}

void TexImage::clamp(int channel, float low, float high)
{
    if (m->image == NULL) return;

    detach();

    m->image->clamp(channel, 1, low, high);
}

void TexImage::packNormal()
{
    scaleBias(0, 0.5f, 0.5f);
    scaleBias(1, 0.5f, 0.5f);
    scaleBias(2, 0.5f, 0.5f);
}

void TexImage::expandNormal()
{
    scaleBias(0, 2.0f, -1.0f);
    scaleBias(1, 2.0f, -1.0f);
    scaleBias(2, 2.0f, -1.0f);
}


void TexImage::blend(float red, float green, float blue, float alpha, float t)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const int count = img->width() * img->height();
    for (int i = 0; i < count; i++)
    {
        r[i] = lerp(r[i], red, t);
        g[i] = lerp(g[i], green, t);
        b[i] = lerp(b[i], blue, t);
        a[i] = lerp(a[i], alpha, t);
    }
}

void TexImage::premultiplyAlpha()
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const int count = img->width() * img->height();
    for (int i = 0; i < count; i++)
    {
        r[i] *= a[i];
        g[i] *= a[i];
        b[i] *= a[i];
    }
}


void TexImage::toGreyScale(float redScale, float greenScale, float blueScale, float alphaScale)
{
    if (m->image == NULL) return;

    detach();

    float sum = redScale + greenScale + blueScale + alphaScale;
    redScale /= sum;
    greenScale /= sum;
    blueScale /= sum;
    alphaScale /= sum;

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const int count = img->width() * img->height();
    for (int i = 0; i < count; i++)
    {
        float grey = r[i] * redScale + g[i] * greenScale + b[i] * blueScale + a[i] * alphaScale;
        a[i] = b[i] = g[i] = r[i] = grey;
    }
}

// Draw colored border.
void TexImage::setBorder(float r, float g, float b, float a)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    const int w = img->width();
    const int h = img->height();

    for (int i = 0; i < w; i++)
    {
        img->pixel(i, 0, 0) = r;
        img->pixel(i, 0, 1) = g;
        img->pixel(i, 0, 2) = b;
        img->pixel(i, 0, 3) = a;

        img->pixel(i, h-1, 0) = r;
        img->pixel(i, h-1, 1) = g;
        img->pixel(i, h-1, 2) = b;
        img->pixel(i, h-1, 3) = a;
    }

    for (int i = 0; i < h; i++)
    {
        img->pixel(0, i, 0) = r;
        img->pixel(0, i, 1) = g;
        img->pixel(0, i, 2) = b;
        img->pixel(0, i, 3) = a;

        img->pixel(w-1, i, 0) = r;
        img->pixel(w-1, i, 1) = g;
        img->pixel(w-1, i, 2) = b;
        img->pixel(w-1, i, 3) = a;
    }
}

// Fill image with the given color.
void TexImage::fill(float red, float green, float blue, float alpha)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const int count = img->width() * img->height();
    for (int i = 0; i < count; i++)
    {
        r[i] = red;
        g[i] = green;
        b[i] = blue;
        a[i] = alpha;
    }
}


void TexImage::scaleAlphaToCoverage(float coverage, float alphaRef/*= 0.5f*/)
{
    if (m->image == NULL) return;

    detach();

    m->image->scaleAlphaToCoverage(coverage, alphaRef, 3);
}


// Ideally you should compress/quantize the RGB and M portions independently.
// Once you have M quantized, you would compute the corresponding RGB and quantize that.
void TexImage::toRGBM(float range/*= 1*/, float threshold/*= 0.25*/)
{
    if (m->image == NULL) return;

    detach();

    float irange = 1.0f / range;

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        float R = nv::clamp(r[i] * irange, 0.0f, 1.0f);
        float G = nv::clamp(g[i] * irange, 0.0f, 1.0f);
        float B = nv::clamp(b[i] * irange, 0.0f, 1.0f);

        float M = max(max(R, G), max(B, 1e-6f)); // Avoid division by zero.

        r[i] = R / M;
        g[i] = G / M;
        b[i] = B / M;
        a[i] = M;
    }
}

void TexImage::fromRGBM(float range/*= 1*/)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        float M = a[i] * range;

        r[i] *= M;
        g[i] *= M;
        b[i] *= M;
        a[i] = 1.0f;
    }
}


// Y is in the [0, 1] range, while CoCg are in the [-1, 1] range.
void TexImage::toYCoCg()
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        float R = r[i];
        float G = g[i];
        float B = b[i];

        float Y = (2*G + R + B) * 0.25f;
        float Co = (R - B);
        float Cg = (2*G - R - B) * 0.5f;

        r[i] = Co;
        g[i] = Cg;
        b[i] = 1.0f;
        a[i] = Y;
    }
}

// img.toYCoCg();
// img.blockScaleCoCg();
// img.scaleBias(0, 0.5, 0.5);
// img.scaleBias(1, 0.5, 0.5);

// @@ Add support for threshold.
// We could do something to prevent scale values from adjacent blocks from being too different to each other
// and minimize bilinear interpolation artifacts.
void TexImage::blockScaleCoCg(int bits/*= 5*/, float threshold/*= 0.0*/)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    const uint w = img->width();
    const uint h = img->height();
    const uint bw = max(1U, w/4);
    const uint bh = max(1U, h/4);

    for (uint bj = 0; bj < bh; bj++) {
        for (uint bi = 0; bi < bw; bi++) {

            // Compute per block scale.
            float m = 1.0f / 255.0f;
            for (uint j = 0; j < 4; j++) {
                const uint y = bj*4 + j;
                if (y >= h) continue;

                for (uint i = 0; i < 4; i++) {
                    const uint x = bi*4 + i;
                    if (x >= w) continue;

                    float Co = img->pixel(x, y, 0);
                    float Cg = img->pixel(x, y, 1);

                    m = max(m, fabsf(Co));
                    m = max(m, fabsf(Cg));
                }
            }

            float scale = PixelFormat::quantizeCeil(m, bits, 8);
            nvDebugCheck(scale >= m);

            // Store block scale in blue channel and scale CoCg.
            for (uint j = 0; j < 4; j++) {
                for (uint i = 0; i < 4; i++) {
                    uint x = min(bi*4 + i, w);
                    uint y = min(bj*4 + j, h);

                    float & Co = img->pixel(x, y, 0);
                    float & Cg = img->pixel(x, y, 1);

                    Co /= scale;
                    nvDebugCheck(fabsf(Co) <= 1.0f);

                    Cg /= scale;
                    nvDebugCheck(fabsf(Cg) <= 1.0f);

                    img->pixel(x, y, 2) = scale;
                }
            }
        }
    }
}

void TexImage::fromYCoCg()
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        float Co = r[i];
        float Cg = g[i];
        float scale = b[i] * 0.5f;
        float Y = a[i];

        Co *= scale;
        Cg *= scale;

        float R = Y + Co - Cg;
        float G = Y + Cg;
        float B = Y - Co - Cg;

        r[i] = R;
        g[i] = G;
        b[i] = B;
        a[i] = 1.0f;
    }
}

void TexImage::toLUVW(float range/*= 1.0f*/)
{
    if (m->image == NULL) return;

    detach();

    float irange = 1.0f / range;

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);
    float * a = img->channel(3);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        float R = nv::clamp(r[i] * irange, 0.0f, 1.0f);
        float G = nv::clamp(g[i] * irange, 0.0f, 1.0f);
        float B = nv::clamp(b[i] * irange, 0.0f, 1.0f);

        float L = max(sqrtf(R*R + G*G + B*B), 1e-6f); // Avoid division by zero.

        r[i] = R / L;
        g[i] = G / L;
        b[i] = B / L;
        a[i] = L / sqrtf(3);
    }
}

void TexImage::fromLUVW(float range/*= 1.0f*/)
{
    // Decompression is the same as in RGBM.
    fromRGBM(range * sqrtf(3));
}

void TexImage::abs(int channel)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * c = img->channel(channel);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        c[i] = fabsf(c[i]);
    }
}

/*
void TexImage::blockLuminanceScale(float scale)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;

    //float * r = img->channel(0);
    //float * g = img->channel(1);
    //float * b = img->channel(2);
    //float * a = img->channel(3);

    const uint w = img->width();
    const uint h = img->height();
    const uint bw = max(1U, w/4);
    const uint bh = max(1U, h/4);

    Vector3 L = normalize(Vector3(1, 1, 1));

    for (uint bj = 0; bj < bh; bj++) {
        for (uint bi = 0; bi < bw; bi++) {

            // Compute block centroid.
            Vector3 centroid(0.0f);
            int count = 0;
            for (uint j = 0; j < 4; j++) {
                const uint y = bj*4 + j;
                if (y >= h) continue;

                for (uint i = 0; i < 4; i++) {
                    const uint x = bi*4 + i;
                    if (x >= w) continue;

                    float r = img->pixel(x, y, 0);
                    float g = img->pixel(x, y, 1);
                    float b = img->pixel(x, y, 2);
                    Vector3 rgb(r, g, b);

                    centroid += rgb;
                    count++;
                }
            }

            centroid /= float(count);

            // Project to luminance plane.
            for (uint j = 0; j < 4; j++) {
                const uint y = bj*4 + j;
                if (y >= h) continue;

                for (uint i = 0; i < 4; i++) {
                    const uint x = bi*4 + i;
                    if (x >= w) continue;

                    float & r = img->pixel(x, y, 0);
                    float & g = img->pixel(x, y, 1);
                    float & b = img->pixel(x, y, 2);
                    Vector3 rgb(r, g, b);

                    Vector3 delta = rgb - centroid;

                    delta -= scale * dot(delta, L) * L;

                    r = centroid.x + delta.x;
                    g = centroid.y + delta.y;
                    b = centroid.z + delta.z;
                }
            }
        }
    }
}
*/

/*
void TexImage::toJPEGLS()
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        float R = nv::clamp(r[i], 0.0f, 1.0f);
        float G = nv::clamp(g[i], 0.0f, 1.0f);
        float B = nv::clamp(b[i], 0.0f, 1.0f);

        r[i] = R-G;
        g[i] = G;
        b[i] = B-G;
    }
}

void TexImage::fromJPEGLS()
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    float * r = img->channel(0);
    float * g = img->channel(1);
    float * b = img->channel(2);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++) {
        float R = nv::clamp(r[i], -1.0f, 1.0f);
        float G = nv::clamp(g[i], 0.0f, 1.0f);
        float B = nv::clamp(b[i], -1.0f, 1.0f);

        r[i] = R+G;
        g[i] = G;
        b[i] = B+G;
    }
}
*/


void TexImage::binarize(int channel, float threshold, bool dither)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    const uint w = img->width();
    const uint h = img->height();

    if (!dither) {
        float * c = img->channel(channel);
        const uint count = w * h;
        for (uint i = 0; i < count; i++) {
            c[i] = float(c[i] > threshold);
        }
    }
    else {
        float * row0 = new float[(w+2)];
        float * row1 = new float[(w+2)];
        memset(row0, 0, sizeof(float)*(w+2));
        memset(row1, 0, sizeof(float)*(w+2));

        for (uint y = 0; y < h; y++) {
            for (uint x = 0; x < w; x++) {

                float & f = img->pixel(x, y, channel);

                // Add error and quantize.
                float qf = float(f + row0[1+x] > threshold);

                // Compute new error:
                float diff = f - qf;

                // Store color.
                f = qf;

                // Propagate new error.
                row0[1+x+1] += (7.0f / 16.0f) * diff;
                row1[1+x-1] += (3.0f / 16.0f) * diff;
                row1[1+x+0] += (5.0f / 16.0f) * diff;
                row1[1+x+1] += (1.0f / 16.0f) * diff;
            }

            swap(row0, row1);
            memset(row1, 0, sizeof(float)*(w+2));
        }

        delete [] row0;
        delete [] row1;
    }
}

// Uniform quantizer.
// Assumes input is in [0, 1] range. Output is in the [0, 1] range, but rounded to the middle of each bin.
// If exactEndPoints is true, [0, 1] are represented exactly, and the correponding bins are half the size, so quantization is not truly uniform.
void TexImage::quantize(int channel, int bits, bool exactEndPoints, bool dither)
{
    if (m->image == NULL) return;

    detach();

    FloatImage * img = m->image;
    const uint w = img->width();
    const uint h = img->height();

    float scale, offset;

    if (exactEndPoints) {
        scale = float((1 << bits) - 1);
        offset = 0.0f;
    }
    else {
        scale = float(1 << bits);
        offset = 0.5f;
    }

    if (!dither) {
        float * c = img->channel(channel);
        const uint count = w * h;
        for (uint i = 0; i < count; i++) {
            c[i] = floorf(c[i] * scale + offset) / scale;
        }
    }
    else {
        float * row0 = new float[(w+2)];
        float * row1 = new float[(w+2)];
        memset(row0, 0, sizeof(float)*(w+2));
        memset(row1, 0, sizeof(float)*(w+2));

        for (uint y = 0; y < h; y++) {
            for (uint x = 0; x < w; x++) {

                float & f = img->pixel(x, y, channel);

                // Add error and quantize.
                float qf = floorf((f + row0[1+x]) * scale + offset) / scale;

                // Compute new error:
                float diff = f - qf;

                // Store color.
                f = qf;

                // Propagate new error.
                row0[1+x+1] += (7.0f / 16.0f) * diff;
                row1[1+x-1] += (3.0f / 16.0f) * diff;
                row1[1+x+0] += (5.0f / 16.0f) * diff;
                row1[1+x+1] += (1.0f / 16.0f) * diff;
            }

            swap(row0, row1);
            memset(row1, 0, sizeof(float)*(w+2));
        }

        delete [] row0;
        delete [] row1;
    }
}



// Set normal map options.
void TexImage::toNormalMap(float sm, float medium, float big, float large)
{
    if (m->image == NULL) return;

    detach();

    const Vector4 filterWeights(sm, medium, big, large);

    const FloatImage * img = m->image;
    m->image = nv::createNormalMap(img, (FloatImage::WrapMode)m->wrapMode, filterWeights);

#pragma NV_MESSAGE("TODO: Pack and expand normals explicitly?")
    m->image->packNormals(0);

    delete img;

    m->isNormalMap = true;
}

void TexImage::normalizeNormalMap()
{
    if (m->image == NULL) return;
    if (!m->isNormalMap) return;

    detach();

    nv::normalizeNormalMap(m->image);
}

void TexImage::transformNormals(NormalTransform xform)
{
    if (m->image == NULL) return;

    detach();

    m->image->expandNormals(0);

    const uint count = m->image->width() * m->image->height();
    for (uint i = 0; i < count; i++) {
        float & x = m->image->pixel(i, 0);
        float & y = m->image->pixel(i, 1);
        float & z = m->image->pixel(i, 2);
        Vector3 n(x, y, z);

        n = normalizeSafe(n, Vector3(0.0f), 0.0f);

        if (xform == NormalTransform_Orthographic) {
            n.z = 0.0f;
        }
        else if (xform == NormalTransform_Stereographic) {
            n.x = n.x / (1 + n.z);
            n.y = n.y / (1 + n.z);
            n.z = 0.0f;
        }
        else if (xform == NormalTransform_Paraboloid) {
            float a = (n.x * n.x) + (n.y * n.y);
            float b = n.z;
            float c = -1.0f;
            float discriminant = b * b - 4.0f * a * c;
            float t = (-b + sqrtf(discriminant)) / (2.0f * a);
            n.x = n.x * t;
            n.y = n.y * t;
            n.z = 0.0f;
        }
        else if (xform == NormalTransform_DualParaboloid) {
            // Use Newton's method to solve equation:
            // f(t) = 1 - zt - (x^2+y^2)t^2 + x^2y^2t^4 = 0
            // f'(t) = - z - 2(x^2+y^2)t + 4x^2y^2t^3

            // Initial approximation:
            float a = (n.x * n.x) + (n.y * n.y);
            float b = n.z;
            float c = -1.0f;
            float discriminant = b * b - 4.0f * a * c;
            float t = (-b + sqrtf(discriminant)) / (2.0f * a);

            float d = fabs(n.z * t - (1 - n.x*n.x*t*t) * (1 - n.y*n.y*t*t));

            while (d > 0.0001) {
                float ft = 1 - n.z * t - (n.x*n.x + n.y*n.y)*t*t + n.x*n.x*n.y*n.y*t*t*t*t;
                float fit = - n.z - 2*(n.x*n.x + n.y*n.y)*t + 4*n.x*n.x*n.y*n.y*t*t*t;
                t -= ft / fit;
                d = fabs(n.z * t - (1 - n.x*n.x*t*t) * (1 - n.y*n.y*t*t));
            };

            n.x = n.x * t;
            n.y = n.y * t;
            n.z = 0.0f;
        }

        x = n.x;
        y = n.y;
        z = n.z;
    }

    m->image->packNormals(0);
}

void TexImage::reconstructNormals(NormalTransform xform)
{
    if (m->image == NULL) return;

    detach();

    m->image->expandNormals(0);

    const uint count = m->image->width() * m->image->height();
    for (uint i = 0; i < count; i++) {
        float & x = m->image->pixel(i, 0);
        float & y = m->image->pixel(i, 1);
        float & z = m->image->pixel(i, 2);
        Vector3 n(x, y, z);

        if (xform == NormalTransform_Orthographic) {
            n.z = sqrtf(1 - nv::clamp(n.x * n.x + n.y * n.y, 0.0f, 1.0f));
        }
        else if (xform == NormalTransform_Stereographic) {
            float denom = 2.0f / (1 + nv::clamp(n.x * n.x + n.y * n.y, 0.0f, 1.0f));
            n.x *= denom;
            n.y *= denom;
            n.z = denom - 1;
        }
        else if (xform == NormalTransform_Paraboloid) {
            n.x = n.x;
            n.y = n.y;
            n.z = 1.0f - nv::clamp(n.x * n.x + n.y * n.y, 0.0f, 1.0f);
            n = normalizeSafe(n, Vector3(0.0f), 0.0f);
        }
        else if (xform == NormalTransform_DualParaboloid) {
            n.x = n.x;
            n.y = n.y;
            n.z = nv::clamp((1 - n.x * n.x) * (1 - n.y * n.y), 0.0f, 1.0f);
            n = normalizeSafe(n, Vector3(0.0f), 0.0f);
        }

        x = n.x;
        y = n.y;
        z = n.z;
    }

    m->image->packNormals(0);
}

void TexImage::flipVertically()
{
    if (m->image == NULL) return;

    detach();

    m->image->flip();
}

bool TexImage::copyChannel(const TexImage & srcImage, int srcChannel)
{
    return copyChannel(srcImage, srcChannel, srcChannel);
}

bool TexImage::copyChannel(const TexImage & srcImage, int srcChannel, int dstChannel)
{
    if (srcChannel < 0 || srcChannel > 3 || dstChannel < 0 || dstChannel > 3) return false;

    FloatImage * dst = m->image;
    const FloatImage * src = srcImage.m->image;

    if (dst == NULL || src == NULL || dst->width() != src->width() || dst->height() != src->height()) {
        return false;
    }
    nvDebugCheck(dst->componentNum() == 4 && src->componentNum() == 4);

    detach();

    const uint w = src->width();
    const uint h = src->height();

    memcpy(dst->channel(dstChannel), src->channel(srcChannel), w*h*sizeof(float));

    return true;
}

bool TexImage::addChannel(const TexImage & srcImage, int srcChannel, int dstChannel, float scale)
{
    if (srcChannel < 0 || srcChannel > 3 || dstChannel < 0 || dstChannel > 3) return false;

    FloatImage * dst = m->image;
    const FloatImage * src = srcImage.m->image;

    if (dst == NULL || src == NULL || dst->width() != src->width() || dst->height() != src->height()) {
        return false;
    }
    nvDebugCheck(dst->componentNum() == 4 && src->componentNum() == 4);

    detach();

    const uint w = src->width();
    const uint h = src->height();

    float * d = dst->channel(dstChannel);
    const float * s = src->channel(srcChannel);
    for (uint i = 0; i < w*h; i++) {
        d[i] += s[i] * scale;
    }

    return true;
}



float nvtt::rmsError(const TexImage & reference, const TexImage & image)
{
    return nv::rmsColorError(reference.m->image, image.m->image, reference.alphaMode() == nvtt::AlphaMode_Transparency);
}


float nvtt::rmsAlphaError(const TexImage & reference, const TexImage & image)
{
    return nv::rmsAlphaError(reference.m->image, image.m->image);
}


float nvtt::cieLabError(const TexImage & reference, const TexImage & image)
{
    return nv::cieLabError(reference.m->image, image.m->image);
}


TexImage nvtt::diff(const TexImage & reference, const TexImage & image, float scale)
{
    const FloatImage * ref = reference.m->image;
    const FloatImage * img = image.m->image;

    if (img == NULL || ref == NULL || img->width() != ref->width() || img->height() != ref->height()) {
        return TexImage();
    }
    nvDebugCheck(img->componentNum() == 4);
    nvDebugCheck(ref->componentNum() == 4);

    nvtt::TexImage diffImage;
    FloatImage * diff = diffImage.m->image = new FloatImage;
    diff->allocate(4, img->width(), img->height());

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++)
    {
        float r0 = img->pixel(i, 0);
        float g0 = img->pixel(i, 1);
        float b0 = img->pixel(i, 2);
        //float a0 = img->pixel(i, 3);
        float r1 = ref->pixel(i, 0);
        float g1 = ref->pixel(i, 1);
        float b1 = ref->pixel(i, 2);
        float a1 = ref->pixel(i, 3);

        float dr = r0 - r1;
        float dg = g0 - g1;
        float db = b0 - b1;
        //float da = a0 - a1;

        if (reference.alphaMode() == nvtt::AlphaMode_Transparency)
        {
            dr *= a1;
            dg *= a1;
            db *= a1;
        }

        diff->pixel(i, 0) = dr * scale;
        diff->pixel(i, 1) = dg * scale;
        diff->pixel(i, 2) = db * scale;
        diff->pixel(i, 3) = a1;
    }

    return diffImage;
}



