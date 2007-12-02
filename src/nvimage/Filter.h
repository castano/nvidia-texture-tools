// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_FILTER_H
#define NV_IMAGE_FILTER_H

#include <nvimage/nvimage.h>

namespace nv
{
	class Vector4;

	/// Base filter class.
	class Filter
	{
	public:
		NVIMAGE_API Filter(float width);

		NVIMAGE_API float width() const { return m_width; }
		NVIMAGE_API float sample(float x, float scale, int samples) const;

		virtual float evaluate(float x) const = 0;

	protected:
		const float m_width;
	};

	// Box filter.
	class BoxFilter : public Filter
	{
	public:
		NVIMAGE_API BoxFilter();
		NVIMAGE_API BoxFilter(float width);
		NVIMAGE_API virtual float evaluate(float x) const;
	};

	// Triangle (bilinear/tent) filter.
	class TriangleFilter : public Filter
	{
	public:
		NVIMAGE_API TriangleFilter();
		NVIMAGE_API TriangleFilter(float width);
		NVIMAGE_API virtual float evaluate(float x) const;
	};

	// Quadratic (bell) filter.
	class QuadraticFilter : public Filter
	{
	public:
		NVIMAGE_API QuadraticFilter();
		NVIMAGE_API virtual float evaluate(float x) const;
	};

	// Cubic filter from Thatcher Ulrich.
	class CubicFilter : public Filter
	{
	public:
		NVIMAGE_API CubicFilter();
		NVIMAGE_API virtual float evaluate(float x) const;
	};

	// Cubic b-spline filter from Paul Heckbert.
	class BSplineFilter : public Filter
	{
	public:
		NVIMAGE_API BSplineFilter();
		NVIMAGE_API virtual float evaluate(float x) const;
	};

	/// Mitchell & Netravali's two-param cubic
	/// @see "Reconstruction Filters in Computer Graphics", SIGGRAPH 88
	class MitchellFilter : public Filter
	{
	public:
		NVIMAGE_API MitchellFilter();
		NVIMAGE_API virtual float evaluate(float x) const;

		NVIMAGE_API void setParameters(float a, float b);

	private:
		float p0, p2, p3;
		float q0, q1, q2, q3;
	};

	// Lanczos3 filter.
	class LanczosFilter : public Filter
	{
	public:
		NVIMAGE_API LanczosFilter();
		NVIMAGE_API virtual float evaluate(float x) const;
	};

	// Sinc filter.
	class SincFilter : public Filter
	{
	public:
		NVIMAGE_API SincFilter(float w);
		NVIMAGE_API virtual float evaluate(float x) const;
	};

	// Kaiser filter.
	class KaiserFilter : public Filter
	{
	public:
		NVIMAGE_API KaiserFilter(float w);
		NVIMAGE_API virtual float evaluate(float x) const;
	
		NVIMAGE_API void setParameters(float a, float stretch);

	private:
		float alpha;
		float stretch;
	};



	/// A 1D kernel. Used to precompute filter weights.
	class Kernel1
	{
	public:
		NVIMAGE_API Kernel1(uint windowSize);
		NVIMAGE_API Kernel1(const Kernel1 & k);
		NVIMAGE_API ~Kernel1();
		
		NVIMAGE_API void normalize();
		
		float valueAt(uint x) const {
			return m_data[x];
		}
		
		uint windowSize() const {
			return m_windowSize;
		}
		/*
		NVIMAGE_API void initFilter(Filter::Enum filter, int samples = 1);
		NVIMAGE_API void initSinc(float stretch = 1);
		NVIMAGE_API void initKaiser(float alpha = 4.0f, float stretch = 1.0f, int sampes = 1);
		NVIMAGE_API void initMitchell(float b = 1.0f/3.0f, float c = 1.0f/3.0f);
		*/
		
		NVIMAGE_API void debugPrint();
		
	private:
		const uint m_windowSize;
		float * m_data;
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
			return m_data[y * m_windowSize + x];
		}
		
		uint windowSize() const {
			return m_windowSize;
		}
		
		NVIMAGE_API void initLaplacian();
		NVIMAGE_API void initEdgeDetection();
		NVIMAGE_API void initSobel();
		NVIMAGE_API void initPrewitt();
		
		NVIMAGE_API void initBlendedSobel(const Vector4 & scale);
		
	private:
		const uint m_windowSize;
		float * m_data;
	};


	/// A 1D polyphase kernel
	class PolyphaseKernel
	{
		NV_FORBID_COPY(PolyphaseKernel)
	public:
		NVIMAGE_API PolyphaseKernel(const Filter & f, uint srcLength, uint dstLength);
		NVIMAGE_API ~PolyphaseKernel();
		
		uint windowSize() const {
			return m_windowSize;
		}
		
		uint length() const {
			return m_length;
		}

		float width() const {
			return m_width;
		}

		float valueAt(uint column, uint x) const {
			nvDebugCheck(column < m_length);
			nvDebugCheck(x < m_windowSize);
			return m_data[column * m_windowSize + x];
		}

		NVIMAGE_API void debugPrint() const;
		
	private:
		uint m_windowSize;
		uint m_length;
		float m_width;
		float * m_data;
	};

} // nv namespace

#endif // NV_IMAGE_FILTER_H
