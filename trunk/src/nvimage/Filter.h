// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_FILTER_H
#define NV_IMAGE_FILTER_H

#include <nvimage/nvimage.h>

namespace nv
{
	class Vector4;

	/// A filter function.
	struct Filter 
	{
		// Standard filters.
		enum Enum 
		{
			Box,
			Triangle,
			Quadratic,	// Bell
			Cubic,
			Spline,
			Lanczos,
			Mitchell,
			Kaiser,
			Num
		};

		float (*function)(float x);
		float support;
	};


	/// A 1D kernel. Used to precompute filter weights.
	class Kernel1
	{
	public:
		NVIMAGE_API Kernel1(uint width);
		NVIMAGE_API Kernel1(const Kernel1 & k);
		NVIMAGE_API ~Kernel1();
		
		NVIMAGE_API void normalize();
		
		float valueAt(uint x) const {
			return data[x];
		}
		
		uint width() const {
			return w;
		}
		
		NVIMAGE_API void initFilter(Filter::Enum filter);
		NVIMAGE_API void initSinc(float stretch = 1);
		NVIMAGE_API void initKaiser(float alpha = 4.0f, float stretch = 1.0f);
		NVIMAGE_API void initMitchell(float b = 1.0f/3.0f, float c = 1.0f/3.0f);
		
		NVIMAGE_API void debugPrint();
		
	private:
		const uint w;
		float * data;
	};


	/// A 2D kernel.
	class Kernel2 
	{
	public:
		NVIMAGE_API Kernel2(uint width);
		NVIMAGE_API Kernel2(const Kernel2 & k);
		NVIMAGE_API ~Kernel2();
		
		NVIMAGE_API void normalize();
		NVIMAGE_API void transpose();
		
		float valueAt(uint x, uint y) const {
			return data[y * w + x];
		}
		
		uint width() const {
			return w;
		}
		
		NVIMAGE_API void initLaplacian();
		NVIMAGE_API void initEdgeDetection();
		NVIMAGE_API void initSobel();
		NVIMAGE_API void initPrewitt();

		NVIMAGE_API void initBlendedSobel(const Vector4 & scale);

	private:
		const uint w;
		float * data;
	};


	// @@ Implement non linear filters:
	// Kuwahara filter
	// Median filter

} // nv namespace

#endif // NV_IMAGE_FILTER_H
