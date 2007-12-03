// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/Containers.h>
#include <nvcore/Ptr.h>

#include <nvmath/Color.h>

#include "FloatImage.h"
#include "Filter.h"
#include "Image.h"

#include <math.h>

using namespace nv;

namespace 
{
	static int round(float f)
	{
		return int(f);
	}

	static float frac(float f)
	{
		return f - floor(f);
	}
}


/// Ctor.
FloatImage::FloatImage() : m_width(0), m_height(0), 
	m_componentNum(0), m_count(0), m_mem(NULL)
{
}

/// Ctor. Init from image.
FloatImage::FloatImage(const Image * img) : m_width(0), m_height(0), 
	m_componentNum(0), m_count(0), m_mem(NULL)
{
	initFrom(img);
}

/// Dtor.
FloatImage::~FloatImage()
{
	free();
}


/// Init the floating point image from a regular image.
void FloatImage::initFrom(const Image * img)
{
	nvCheck(img != NULL);
	
	allocate(4, img->width(), img->height());
	
	float * red_channel = channel(0);
	float * green_channel = channel(1);
	float * blue_channel = channel(2);
	float * alpha_channel = channel(3);
	
	const uint count = m_width * m_height;
	for(uint i = 0; i < count; i++) {
		Color32 pixel = img->pixel(i);
		red_channel[i] = float(pixel.r) / 255.0f;
		green_channel[i] = float(pixel.g) / 255.0f;
		blue_channel[i] = float(pixel.b) / 255.0f;
		alpha_channel[i] = float(pixel.a) / 255.0f;
	}
}

/// Convert the floating point image to a regular image.
Image * FloatImage::createImage(uint base_component/*= 0*/, uint num/*= 4*/) const
{
	nvCheck(num <= 4);
	nvCheck(base_component + num <= m_componentNum);
	
	AutoPtr<Image> img(new Image());
	img->allocate(m_width, m_height);
	
	const uint size = m_width * m_height;
	for(uint i = 0; i < size; i++) {
		
		uint c;
		uint8 rgba[4]= {0, 0, 0, 0xff};

		for(c = 0; c < num; c++) {
			float f = m_mem[size * (base_component + c) + i];
			rgba[c] = nv::clamp(int(255.0f * f), 0, 255);
		}

		img->pixel(i) = Color32(rgba[0], rgba[1], rgba[2], rgba[3]);
	}
	
	return img.release();
}


/// Convert the floating point image to a regular image. Correct gamma of rgb, but not alpha.
Image * FloatImage::createImageGammaCorrect(float gamma/*= 2.2f*/) const
{
	nvCheck(m_componentNum == 4);
	
	AutoPtr<Image> img(new Image());
	img->allocate(m_width, m_height);
	
	const float * rChannel = this->channel(0);
	const float * gChannel = this->channel(1);
	const float * bChannel = this->channel(2);
	const float * aChannel = this->channel(3);

	const uint size = m_width * m_height;
	for(uint i = 0; i < size; i++)
	{
		const uint8 r = nv::clamp(int(255.0f * pow(rChannel[i], 1.0f/gamma)), 0, 255);
		const uint8 g = nv::clamp(int(255.0f * pow(gChannel[i], 1.0f/gamma)), 0, 255);
		const uint8 b = nv::clamp(int(255.0f * pow(bChannel[i], 1.0f/gamma)), 0, 255);
		const uint8 a = nv::clamp(int(255.0f * aChannel[i]), 0, 255);

		img->pixel(i) = Color32(r, g, b, a);
	}
	
	return img.release();
}

/// Allocate a 2d float image of the given format and the given extents.
void FloatImage::allocate(uint c, uint w, uint h)
{
	nvCheck(m_mem == NULL);
	m_width = w;
	m_height = h;
	m_componentNum = c;
	m_count = w * h * c;
	m_mem = reinterpret_cast<float *>(nv::mem::malloc(m_count * sizeof(float)));
}

/// Free the image, but don't clear the members.
void FloatImage::free()
{
	nvCheck(m_mem != NULL);
	nv::mem::free( reinterpret_cast<void *>(m_mem) );
	m_mem = NULL;
}

void FloatImage::clear(float f/*=0.0f*/)
{
	for(uint i = 0; i < m_count; i++) {
		m_mem[i] = f;
	}
}

void FloatImage::normalize(uint base_component)
{
	nvCheck(base_component + 3 <= m_componentNum);
	
	float * xChannel = this->channel(base_component + 0);
	float * yChannel = this->channel(base_component + 1);
	float * zChannel = this->channel(base_component + 2);

	const uint size = m_width * m_height;
	for(uint i = 0; i < size; i++) {
		
		Vector3 normal(xChannel[i], yChannel[i], zChannel[i]);
		normal = normalizeSafe(normal, Vector3(zero));
		
		xChannel[i] = normal.x();
		yChannel[i] = normal.y();
		zChannel[i] = normal.z();
	}
}

void FloatImage::packNormals(uint base_component)
{
	scaleBias(base_component, 3, 0.5f, 1.0f);
}

void FloatImage::expandNormals(uint base_component)
{
	scaleBias(base_component, 3, 2, -0.5);
}

void FloatImage::scaleBias(uint base_component, uint num, float scale, float bias)
{
	const uint size = m_width * m_height;
	
	for(uint c = 0; c < num; c++) {
		float * ptr = this->channel(base_component + c);
		
		for(uint i = 0; i < size; i++) {
			ptr[i] = scale * (ptr[i] + bias);
		}
	}
}

/// Clamp the elements of the image.
void FloatImage::clamp(float low, float high)
{
	for(uint i = 0; i < m_count; i++) {
		m_mem[i] = nv::clamp(m_mem[i], low, high);
	}
}

/// From gamma to linear space.
void FloatImage::toLinear(uint base_component, uint num, float gamma /*= 2.2f*/)
{
	exponentiate(base_component, num, gamma);
}

/// From linear to gamma space.
void FloatImage::toGamma(uint base_component, uint num, float gamma /*= 2.2f*/)
{
	exponentiate(base_component, num, 1.0f/gamma);
}

/// Exponentiate the elements of the image.
void FloatImage::exponentiate(uint base_component, uint num, float power)
{
	const uint size = m_width * m_height;

	for(uint c = 0; c < num; c++) {
		float * ptr = this->channel(base_component + c);
		
		for(uint i = 0; i < size; i++) {
			ptr[i] = pow(ptr[i], power);
		}
	}
}

#if 0
float FloatImage::nearest(float x, float y, int c, WrapMode wm) const
{
	if( wm == WrapMode_Clamp ) return nearest_clamp(x, y, c);
	/*if( wm == WrapMode_Repeat )*/ return nearest_repeat(x, y, c);
	//if( wm == WrapMode_Mirror ) return nearest_mirror(x, y, c);
}

float FloatImage::nearest_clamp(int x, int y, const int c) const
{
	const int w = m_width;
	const int h = m_height;
	int ix = ::clamp(x, 0, w-1);
	int iy = ::clamp(y, 0, h-1);
	return pixel(ix, iy, c);
}

float FloatImage::nearest_repeat(int x, int y, const int c) const
{
	const int w = m_width;
	const int h = m_height;
	int ix = x % w;
	int iy = y % h;
	return pixel(ix, iy, c);
}
#endif

float FloatImage::nearest(float x, float y, int c, WrapMode wm) const
{
	if( wm == WrapMode_Clamp ) return nearest_clamp(x, y, c);
	/*if( wm == WrapMode_Repeat )*/ return nearest_repeat(x, y, c);
	//if( wm == WrapMode_Mirror ) return nearest_mirror(x, y, c);
}

float FloatImage::linear(float x, float y, int c, WrapMode wm) const
{
	if( wm == WrapMode_Clamp ) return linear_clamp(x, y, c);
	/*if( wm == WrapMode_Repeat )*/ return linear_repeat(x, y, c);
	//if( wm == WrapMode_Mirror ) return linear_mirror(x, y, c);
}

float FloatImage::nearest_clamp(float x, float y, const int c) const
{
	const int w = m_width;
	const int h = m_height;
	int ix = ::clamp(round(x * w), 0, w-1);
	int iy = ::clamp(round(y * h), 0, h-1);
	return pixel(ix, iy, c);
}

float FloatImage::nearest_repeat(float x, float y, const int c) const
{
	const int w = m_width;
	const int h = m_height;
	int ix = round(frac(x) * w);
	int iy = round(frac(y) * h);
	return pixel(ix, iy, c);
}

float FloatImage::nearest_mirror(float x, float y, const int c) const
{
	// @@ TBD
	return 0.0f;
}

float FloatImage::linear_clamp(float x, float y, const int c) const
{
	const int w = m_width;
	const int h = m_height;
	
	x *= w;
	y *= h;
	
	const float fracX = frac(x);
	const float fracY = frac(y);
	
	const int ix0 = ::clamp(round(x), 0, w-1);
	const int iy0 = ::clamp(round(y), 0, h-1);
	const int ix1 = ::clamp(round(x)+1, 0, w-1);
	const int iy1 = ::clamp(round(y)+1, 0, h-1);

	float f1 = pixel(ix0, iy0, c);
	float f2 = pixel(ix1, iy0, c);
	float f3 = pixel(ix0, iy1, c);
	float f4 = pixel(ix1, iy1, c);
	
	float i1 = lerp(f1, f2, fracX);
	float i2 = lerp(f3, f4, fracX);

	return lerp(i1, i2, fracY);
}

float FloatImage::linear_repeat(float x, float y, int c) const
{
	const int w = m_width;
	const int h = m_height;
	
	const float fracX = frac(x * w);
	const float fracY = frac(y * h);
	
	int ix0 = round(frac(x) * w);
	int iy0 = round(frac(y) * h);
	int ix1 = round(frac(x + 1.0f/w) * w);
	int iy1 = round(frac(y + 1.0f/h) * h);
	
	float f1 = pixel(ix0, iy0, c);
	float f2 = pixel(ix1, iy0, c);
	float f3 = pixel(ix0, iy1, c);
	float f4 = pixel(ix1, iy1, c);
	
	float i1 = lerp(f1, f2, fracX);
	float i2 = lerp(f3, f4, fracX);

	return lerp(i1, i2, fracY);
}

float FloatImage::linear_mirror(float x, float y, int c) const
{
	// @@ TBD
	return 0.0f;
}


/// Fast downsampling using box filter. 
///
/// The extents of the image are divided by two and rounded down.
///
/// When the size of the image is odd, this uses a polyphase box filter as explained in:
/// http://developer.nvidia.com/object/np2_mipmapping.html
///
FloatImage * FloatImage::fastDownSample() const
{
	nvDebugCheck(m_width != 1 || m_height != 1);
	
	AutoPtr<FloatImage> dst_image( new FloatImage() );

	const uint w = max(1, m_width / 2);
	const uint h = max(1, m_height / 2);
	dst_image->allocate(m_componentNum, w, h);

	// 1D box filter.
	if (m_width == 1 || m_height == 1)
	{
		const uint w = m_width * m_height;
		
		if (w & 1)
		{
			const float scale = 1.0f / (2 * w + 1);
			
			for(uint c = 0; c < m_componentNum; c++)
			{
				const float * src = this->channel(c);
				float * dst = dst_image->channel(c);
				
				for(uint x = 0; x < w; x++)
				{
					const float w0 = (w - x);
					const float w1 = (w - 0);
					const float w2 = (1 + x);
					
					*dst++ = scale * (w0 * src[0] + w1 * src[1] + w2 * src[2]);
					src += 2;
				}
			}
		}
		else
		{
			for(uint c = 0; c < m_componentNum; c++)
			{
				const float * src = this->channel(c);
				float * dst = dst_image->channel(c);
				
				for(uint x = 0; x < w; x++)
				{
					*dst = 0.5f * (src[0] + src[1]);
					dst++;
					src += 2;
				}
			}
		}
	}
	
	// Regular box filter.
	else if ((m_width & 1) == 0 && (m_height & 1) == 0)
	{
		for(uint c = 0; c < m_componentNum; c++)
		{
			const float * src = this->channel(c);
			float * dst = dst_image->channel(c);
			
			for(uint y = 0; y < h; y++)
			{
				for(uint x = 0; x < w; x++)
				{
					*dst = 0.25f * (src[0] + src[1] + src[m_width] + src[m_width + 1]);
					dst++;
					src += 2;
				}
				
				src += m_width;
			}
		}
	}
	
	// Polyphase filters.
	else if (m_width & 1 && m_height & 1)
	{
		nvDebugCheck(m_width == 2 * w + 1);
		nvDebugCheck(m_height == 2 * h + 1);
		
		const float scale = 1.0f / (m_width * m_height);
		
		for(uint c = 0; c < m_componentNum; c++)
		{
			const float * src = this->channel(c);
			float * dst = dst_image->channel(c);
			
			for(uint y = 0; y < h; y++)
			{
				const float v0 = (h - y);
				const float v1 = (h - 0);
				const float v2 = (1 + y);
				
				for (uint x = 0; x < w; x++)
				{
					const float w0 = (w - x);
					const float w1 = (w - 0);
					const float w2 = (1 + x);
					
					float f = 0.0f;
					f += v0 * (w0 * src[0 * m_width + 2 * x] + w1 * src[0 * m_width + 2 * x + 1] + w2 * src[0 * m_width + 2 * x + 2]);
					f += v1 * (w0 * src[1 * m_width + 2 * x] + w1 * src[1 * m_width + 2 * x + 1] + w2 * src[0 * m_width + 2 * x + 2]);
					f += v2 * (w0 * src[2 * m_width + 2 * x] + w1 * src[2 * m_width + 2 * x + 1] + w2 * src[0 * m_width + 2 * x + 2]);
					
					*dst = f * scale;
					dst++;
				}
				
				src += 2 * m_width;
			}
		}
	}
	else if (m_width & 1)
	{
		nvDebugCheck(m_width == 2 * w + 1);
		const float scale = 1.0f / (2 * m_width);
		
		for(uint c = 0; c < m_componentNum; c++)
		{
			const float * src = this->channel(c);
			float * dst = dst_image->channel(c);
			
			for(uint y = 0; y < h; y++)
			{
				for (uint x = 0; x < w; x++)
				{
					const float w0 = (w - x);
					const float w1 = (w - 0);
					const float w2 = (1 + x);
					
					float f = 0.0f;
					f += w0 * (src[2 * x + 0] + src[m_width + 2 * x + 0]);
					f += w1 * (src[2 * x + 1] + src[m_width + 2 * x + 1]);
					f += w2 * (src[2 * x + 2] + src[m_width + 2 * x + 2]);
					
					*dst = f * scale;
					dst++;
				}
				
				src += 2 * m_width;
			}
		}
	}
	else if (m_height & 1)
	{
		nvDebugCheck(m_height == 2 * h + 1);
		
		const float scale = 1.0f / (2 * m_height);
		
		for(uint c = 0; c < m_componentNum; c++)
		{
			const float * src = this->channel(c);
			float * dst = dst_image->channel(c);
			
			for(uint y = 0; y < h; y++)
			{
				const float v0 = (h - y);
				const float v1 = (h - 0);
				const float v2 = (1 + y);
				
				for (uint x = 0; x < w; x++)
				{
					float f = 0.0f;
					f += v0 * (src[0 * m_width + 2 * x] + src[0 * m_width + 2 * x + 1]);
					f += v1 * (src[1 * m_width + 2 * x] + src[1 * m_width + 2 * x + 1]);
					f += v2 * (src[2 * m_width + 2 * x] + src[2 * m_width + 2 * x + 1]);
					
					*dst = f * scale;
					dst++;
				}
				
				src += 2 * m_width;
			}
		}
	}
	
	return dst_image.release();
}

/*
/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::downSample(const Kernel1 & kernel, WrapMode wm) const
{
	const uint w = max(1, m_width / 2);
	const uint h = max(1, m_height / 2);
	
	return downSample(kernel, w, h, wm);
}


/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::downSample(const Kernel1 & kernel, uint w, uint h, WrapMode wm) const
{
	nvCheck(!(kernel.windowSize() & 1));	// Make sure that kernel m_width is even.

	AutoPtr<FloatImage> tmp_image( new FloatImage() );
	tmp_image->allocate(m_componentNum, w, m_height);
	
	AutoPtr<FloatImage> dst_image( new FloatImage() );	
	dst_image->allocate(m_componentNum, w, h);
	
	const float xscale = float(m_width) / float(w);
	const float yscale = float(m_height) / float(h);
	
	for(uint c = 0; c < m_componentNum; c++) {
		float * tmp_channel = tmp_image->channel(c);
		
		for(uint y = 0; y < m_height; y++) {
			for(uint x = 0; x < w; x++) {
				
				float sum = this->applyKernelHorizontal(&kernel, uint(x*xscale), y, c, wm);
				
				const uint tmp_index = tmp_image->index(x, y);
				tmp_channel[tmp_index] = sum;
			}
		}
		
		float * dst_channel = dst_image->channel(c);
		
		for(uint y = 0; y < h; y++) {
			for(uint x = 0; x < w; x++) {
				
				float sum = tmp_image->applyKernelVertical(&kernel, uint(x*xscale), uint(y*yscale), c, wm);
				
				const uint dst_index = dst_image->index(x, y);
				dst_channel[dst_index] = sum;
			}
		}
	}
	
	return dst_image.release();
}
*/

/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::downSample(const Filter & filter, WrapMode wm) const
{
	const uint w = max(1, m_width / 2);
	const uint h = max(1, m_height / 2);

	return downSample(filter, w, h, wm);
}


/// Downsample applying a 1D kernel separately in each dimension.
FloatImage * FloatImage::downSample(const Filter & filter, uint w, uint h, WrapMode wm) const
{
	// @@ Use monophase filters when frac(m_width / w) == 0

	AutoPtr<FloatImage> tmp_image( new FloatImage() );
	AutoPtr<FloatImage> dst_image( new FloatImage() );	
	
	PolyphaseKernel xkernel(filter, m_width, w, 32);
	PolyphaseKernel ykernel(filter, m_height, h, 32);
	
	// @@ Select fastest filtering order:
	//if (w * m_height <= h * m_width)
	{
		tmp_image->allocate(m_componentNum, w, m_height);
		dst_image->allocate(m_componentNum, w, h);
		
		Array<float> tmp_column(h);
		tmp_column.resize(h);
		
		for (uint c = 0; c < m_componentNum; c++)
		{
			float * tmp_channel = tmp_image->channel(c);
			
			for (uint y = 0; y < m_height; y++) {
				this->applyKernelHorizontal(xkernel, y, c, wm, tmp_channel + y * w);
			}
			
			float * dst_channel = dst_image->channel(c);
			
			for (uint x = 0; x < w; x++) {
				tmp_image->applyKernelVertical(ykernel, x, c, wm, tmp_column.unsecureBuffer());
				
				for (uint y = 0; y < h; y++) {
					dst_channel[y * w + x] = tmp_column[y];
				}
			}
		}
	}
	/*else
	{
		tmp_image->allocate(m_componentNum, m_width, h);
		dst_image->allocate(m_componentNum, w, h);
		
		Array<float> tmp_column(h);
		tmp_column.resize(h);
		
		for (uint c = 0; c < m_componentNum; c++)
		{
			float * tmp_channel = tmp_image->channel(c);

			for (uint x = 0; x < w; x++) {
				tmp_image->applyKernelVertical(ykernel, x, c, wm, tmp_column.unsecureBuffer());
				
				for (uint y = 0; y < h; y++) {
					tmp_channel[y * w + x] = tmp_column[y];
				}
			}

			float * dst_channel = dst_image->channel(c);

			for (uint y = 0; y < m_height; y++) {
				this->applyKernelHorizontal(xkernel, y, c, wm, dst_channel + y * w);
			}
		}
	}*/
	
	return dst_image.release();
}



/// Apply 2D kernel at the given coordinates and return result.
float FloatImage::applyKernel(const Kernel2 * k, int x, int y, int c, WrapMode wm) const
{
	nvDebugCheck(k != NULL);
	
	const uint kernelWindow = k->windowSize();
	const int kernelOffset = int(kernelWindow / 2) - 1;
	
	const float * channel = this->channel(c);

	float sum = 0.0f;
	for (uint i = 0; i < kernelWindow; i++)
	{
		const int src_y = int(y + i) - kernelOffset;
		
		for (uint e = 0; e < kernelWindow; e++)
		{
			const int src_x = int(x + e) - kernelOffset;
			
			int idx = this->index(src_x, src_y, wm);
			
			sum += k->valueAt(e, i) * channel[idx];
		}
	}
	
	return sum;
}


/// Apply 1D vertical kernel at the given coordinates and return result.
float FloatImage::applyKernelVertical(const Kernel1 * k, int x, int y, int c, WrapMode wm) const
{
	nvDebugCheck(k != NULL);
	
	const uint kernelWindow = k->windowSize();
	const int kernelOffset = int(kernelWindow / 2) - 1;
	
	const float * channel = this->channel(c);

	float sum = 0.0f;
	for (uint i = 0; i < kernelWindow; i++)
	{
		const int src_y = int(y + i) - kernelOffset;
		const int idx = this->index(x, src_y, wm);
		
		sum += k->valueAt(i) * channel[idx];
	}
	
	return sum;
}

/// Apply 1D horizontal kernel at the given coordinates and return result.
float FloatImage::applyKernelHorizontal(const Kernel1 * k, int x, int y, int c, WrapMode wm) const
{
	nvDebugCheck(k != NULL);
	
	const uint kernelWindow = k->windowSize();
	const int kernelOffset = int(kernelWindow / 2) - 1;
	
	const float * channel = this->channel(c);

	float sum = 0.0f;
	for (uint e = 0; e < kernelWindow; e++)
	{
		const int src_x = int(x + e) - kernelOffset;
		const int idx = this->index(src_x, y, wm);
		
		sum += k->valueAt(e) * channel[idx];
	}
	
	return sum;
}


/// Apply 1D vertical kernel at the given coordinates and return result.
void FloatImage::applyKernelVertical(const PolyphaseKernel & k, int x, int c, WrapMode wm, float * output) const
{
	const uint length = k.length();
	const float scale = float(length) / float(m_height);
	const float iscale = 1.0f / scale;

	const float width = k.width();
	const int windowSize = k.windowSize();

	const float * channel = this->channel(c);

	for (uint i = 0; i < length; i++)
	{
		const float center = (0.5f + i) * iscale;
		
		const int left = floor(center - width);
		const int right = ceil(center + width);
		nvCheck(right - left <= windowSize);
		
		float sum = 0;
		for (int j = 0; j < windowSize; ++j)
		{
			const int idx = this->index(x, j+left, wm);
			
			sum += k.valueAt(i, j) * channel[idx];
		}
		
		output[i] = sum;
	}
}

/// Apply 1D horizontal kernel at the given coordinates and return result.
void FloatImage::applyKernelHorizontal(const PolyphaseKernel & k, int y, int c, WrapMode wm, float * output) const
{
	const uint length = k.length();
	const float scale = float(length) / float(m_width);
	const float iscale = 1.0f / scale;

	const float width = k.width();
	const int windowSize = k.windowSize();

	const float * channel = this->channel(c);

	for (uint i = 0; i < length; i++)
	{
		const float center = (0.5f + i) * iscale;
		
		const int left = floorf(center - width);
		const int right = ceilf(center + width);
		nvDebugCheck(right - left <= windowSize);
		
		float sum = 0;
		for (int j = 0; j < windowSize; ++j)
		{
			const int idx = this->index(left + j, y, wm);
			
			sum += k.valueAt(i, j) * channel[idx];
		}
		
		output[i] = sum;
	}
}

