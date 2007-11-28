// This code is in the public domain -- castanyo@yahoo.es

/** @file Filter.cpp
 * @brief Image filters.
 *
 * Jonathan Blow articles:
 * http://number-none.com/product/Mipmapping, Part 1/index.html
 * http://number-none.com/product/Mipmapping, Part 2/index.html
 *
 * References from Thacher Ulrich:
 * See _Graphics Gems III_ "General Filtered Image Rescaling", Dale A. Schumacher
 *
 * References from Paul Heckbert:
 * A.V. Oppenheim, R.W. Schafer, Digital Signal Processing, Prentice-Hall, 1975
 *
 * R.W. Hamming, Digital Filters, Prentice-Hall, Englewood Cliffs, NJ, 1983
 *
 * W.K. Pratt, Digital Image Processing, John Wiley and Sons, 1978
 *
 * H.S. Hou, H.C. Andrews, "Cubic Splines for Image Interpolation and
 *	Digital Filtering", IEEE Trans. Acoustics, Speech, and Signal Proc.,
 *	vol. ASSP-26, no. 6, Dec. 1978, pp. 508-517
 *
 * Paul Heckbert's zoom library.
 * http://www.xmission.com/~legalize/zoom.html
 * 
 * Reconstruction Filters in Computer Graphics
 * http://www.mentallandscape.com/Papers_siggraph88.pdf 
 *
 */


#include <nvcore/Containers.h>	// swap
#include <nvmath/nvmath.h>	// fabs
#include <nvmath/Vector.h>	// Vector4
#include <nvimage/Filter.h>

using namespace nv;

namespace
{

// support = 0.5
inline static float filter_box(float x)
{
    if( x < -0.5f ) return 0.0f;
    if( x <= 0.5 ) return 1.0f;
    return 0.0f;
}

// support = 1.0
inline static float filter_triangle(float x)
{
    if( x < -1.0f ) return 0.0f;
    if( x < 0.0f ) return 1.0f + x;
    if( x < 1.0f ) return 1.0f - x;
    return 0.0f;
}

// support = 1.5
inline static float filter_quadratic(float x)
{
	if( x < 0.0f ) x = -x;
    if( x < 0.5f ) return 0.75f - x * x;
    if( x < 1.5f ) { 
    	float t = x - 1.5f;
    	return 0.5f * t * t;
    }
    return 0.0f;
}

// @@ Filter from tulrich. 
// support 1.0
inline static float filter_cubic(float x)
{
	// f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1
	if( x < 0.0f ) x = -x;
	if( x < 1.0f ) return((2.0f * x - 3.0f) * x * x + 1.0f);
	return 0.0f;
}


// @@ Paul Heckbert calls this cubic instead of spline.
// support = 2.0
inline static float filter_spline(float x)
{
    if( x < 0.0f ) x = -x;
    if( x < 1.0f ) return (4.0f + x * x * (-6.0f + x * 3.0f)) / 6.0f;
    if( x < 2.0f ) { 
    	float t = 2.0f - x;
    	return t * t * t / 6.0f;
    }
    return 0.0f;
}

/// Sinc function.
inline float sincf( const float x )
{
	if( fabs(x) < NV_EPSILON ) {
		return 1.0 ;
		//return 1.0f + x*x*(-1.0f/6.0f + x*x*1.0f/120.0f);
	}
	else {
		return sin(x) / x;
	}
}

// support = 3.0
inline static float filter_lanczos3(float x)
{
	if( x < 0.0f ) x = -x;
	if( x < 3.0f ) return sincf(x) * sincf(x / 3.0f);
	return 0.0f;
}



// Mitchell & Netravali's two-param cubic
// see "Reconstruction Filters in Computer Graphics", SIGGRAPH 88
// support = 2.0
inline static float filter_mitchell(float x, float b, float c)
{
	// @@ Coefficients could be precomputed.
	// @@ if b and c are fixed, these are constants.
	const float p0 = (6.0f -  2.0f * b) / 6.0f;
	const float p2 = (-18.0f + 12.0f * b + 6.0f * c) / 6.0f;
	const float p3 = (12.0f - 9.0f * b - 6.0f * c) / 6.0f;
	const float q0 = (8.0f * b + 24.0f * c) / 6.0f;
	const float q1 = (-12.0f * b - 48.0f * c) / 6.0f;
	const float q2 = (6.0f * b + 30.0f * c) / 6.0f;
	const float q3 = (-b - 6.0f * c) / 6.0f;

	if( x < 0.0f ) x = -x;
	if( x < 1.0f ) return p0 + x * x * (p2 + x * p3);
	if( x < 2.0f ) return q0 + x * (q1 + x * (q2 + x * q3));
	return 0.0f;
}

inline static float filter_mitchell(float x)
{
	return filter_mitchell(x, 1.0f/3.0f, 1.0f/3.0f);
}

// Bessel function of the first kind from Jon Blow's article.
// http://mathworld.wolfram.com/BesselFunctionoftheFirstKind.html
// http://en.wikipedia.org/wiki/Bessel_function
static float bessel0(float x)
{
	const float EPSILON_RATIO = 1E-6;
	float xh, sum, pow, ds;
	int k;

	xh = 0.5 * x;
	sum = 1.0;
	pow = 1.0;
	k = 0;
	ds = 1.0;
	while (ds > sum * EPSILON_RATIO) {
		++k;
		pow = pow * (xh / k);
		ds = pow * pow;
		sum = sum + ds;
	}

	return sum;
}

/*// Alternative bessel function from Paul Heckbert.
static float _bessel0(float x)
{
	const float EPSILON_RATIO = 1E-6;
    float sum = 1.0f;
    float y = x * x / 4.0f;
    float t = y;
    for(int i = 2; t > EPSILON_RATIO; i++) {
		sum += t;
		t *= y / float(i * i);
    }
    return sum;
}*/

// support = 1.0
inline static float filter_kaiser(float x, float alpha)
{
	return bessel0(alpha * sqrtf(1 - x * x)) / bessel0(alpha);
}

inline static float filter_kaiser(float x)
{
	return filter_kaiser(x, 4.0f);
}


// Array of filters.
static Filter s_filter_array[] = {
	{filter_box, 		0.5f},	// Box
	{filter_triangle, 	1.0f},	// Triangle
	{filter_quadratic, 	1.5f},	// Quadratic
	{filter_cubic, 		1.0f},	// Cubic
	{filter_spline,		2.0f},	// Spline
	{filter_lanczos3,	3.0f},	// Lanczos
	{filter_mitchell,	1.0f},	// Mitchell
	{filter_kaiser,		1.0f},	// Kaiser
};


inline static float sampleFilter(Filter::Function func, float x, float scale, int samples)
{
	float sum = 0;

	for(int s = 0; s < samples; s++)
	{
		sum += func((x + (float(s) + 0.5f) * (1.0f / float(samples))) * scale);
	}
	
	return sum;
}



} // namespace



/// Ctor.
Kernel1::Kernel1(uint ws) : m_windowSize(ws)
{
	m_data = new float[m_windowSize];
}

/// Copy ctor.
Kernel1::Kernel1(const Kernel1 & k) : m_windowSize(k.m_windowSize)
{
	m_data = new float[m_windowSize];
	for(uint i = 0; i < m_windowSize; i++) {
		m_data[i] = k.m_data[i];
	}
}

/// Dtor.
Kernel1::~Kernel1()
{
	delete m_data;
}

/// Normalize the filter.
void Kernel1::normalize()
{
	float total = 0.0f;
	for(uint i = 0; i < m_windowSize; i++) {
		total += m_data[i];
	}
	
	float inv = 1.0f / total;
	for(uint i = 0; i < m_windowSize; i++) {
		m_data[i] *= inv;
	}
}


/// Init 1D filter.
void Kernel1::initFilter(Filter::Enum f, int samples /*= 1*/)
{
	nvDebugCheck(f < Filter::Num);
	nvDebugCheck(samples >= 1);
	
	float (* filter_function)(float) = s_filter_array[f].function;
	const float support = s_filter_array[f].support;
	
	const float halfWindowSize = float(m_windowSize) / 2.0f;
	const float scale = support / halfWindowSize;
	
	for(uint i = 0; i < m_windowSize; i++)
	{
		m_data[i] = sampleFilter(filter_function, i - halfWindowSize, scale, samples);
	}
	
	normalize();
}


/// Init 1D sinc filter.
void Kernel1::initSinc(float stretch /*= 1*/)
{
	const float halfWindowSize = float(m_windowSize) / 2;
	const float nudge = 0.5f;
	
	for(uint i = 0; i < m_windowSize; i++) {
		const float x = (i - halfWindowSize) + nudge;
		m_data[i] = sincf(PI * x * stretch);
	}

	normalize();
}


/// Init 1D Kaiser-windowed sinc filter.
void Kernel1::initKaiser(float alpha /*= 4*/, float stretch /*= 0.5*/, int samples/*= 1*/)
{
	const float halfWindowSize = float(m_windowSize) / 2;

	const float s_scale = 1.0f / float(samples);
	const float x_scale = 1.0f / halfWindowSize;
	
	for(uint i = 0; i < m_windowSize; i++)
	{
		float sum = 0;
		for(int s = 0; s < samples; s++)
		{
			float x = i - halfWindowSize + (s + 0.5f) * s_scale;
			
			const float sinc_value = sincf(PI * x * stretch);
			const float window_value = filter_kaiser(x * x_scale, alpha);	// @@ should the window be streched? I don't think so.
			
			sum += sinc_value * window_value;
		}
		m_data[i] = sum;
	}

	normalize();
}


/// Init 1D Mitchell filter.
void Kernel1::initMitchell(float b, float c)
{
	const float halfWindowSize = float(m_windowSize) / 2;
	const float nudge = 0.5f;
	
	for (uint i = 0; i < m_windowSize; i++) {
		const float x = (i - halfWindowSize) + nudge;
		m_data[i] = filter_mitchell(x / halfWindowSize, b, c);
	}
	
	normalize();
}


/// Print the kernel for debugging purposes.
void Kernel1::debugPrint()
{
	for (uint i = 0; i < m_windowSize; i++) {
		nvDebug("%d: %f\n", i, m_data[i]);
	}
}



/// Ctor.
Kernel2::Kernel2(uint ws) : m_windowSize(ws)
{
	m_data = new float[m_windowSize * m_windowSize];
}

/// Copy ctor.
Kernel2::Kernel2(const Kernel2 & k) : m_windowSize(k.m_windowSize)
{
	m_data = new float[m_windowSize * m_windowSize];
	for (uint i = 0; i < m_windowSize * m_windowSize; i++) {
		m_data[i] = k.m_data[i];
	}
}


/// Dtor.
Kernel2::~Kernel2()
{
	delete m_data;
}

/// Normalize the filter.
void Kernel2::normalize()
{
	float total = 0.0f;
	for(uint i = 0; i < m_windowSize*m_windowSize; i++) {
		total += fabs(m_data[i]);
	}
	
	float inv = 1.0f / total;
	for(uint i = 0; i < m_windowSize*m_windowSize; i++) {
		m_data[i] *= inv;
	}
}

/// Transpose the kernel.
void Kernel2::transpose()
{
	for(uint i = 0; i < m_windowSize; i++) {
		for(uint j = i+1; j < m_windowSize; j++) {
			swap(m_data[i*m_windowSize + j], m_data[j*m_windowSize + i]);
		}
	}
}

/// Init laplacian filter, usually used for sharpening.
void Kernel2::initLaplacian()
{
	nvDebugCheck(m_windowSize == 3);
//	m_data[0] = -1; m_data[1] = -1; m_data[2] = -1;
//	m_data[3] = -1; m_data[4] = +8; m_data[5] = -1;
//	m_data[6] = -1; m_data[7] = -1; m_data[8] = -1;	
	
	m_data[0] = +0; m_data[1] = -1; m_data[2] = +0;
	m_data[3] = -1; m_data[4] = +4; m_data[5] = -1;
	m_data[6] = +0; m_data[7] = -1; m_data[8] = +0;	
	
//	m_data[0] = +1; m_data[1] = -2; m_data[2] = +1;
//	m_data[3] = -2; m_data[4] = +4; m_data[5] = -2;
//	m_data[6] = +1; m_data[7] = -2; m_data[8] = +1;	
}


/// Init simple edge detection filter.
void Kernel2::initEdgeDetection()
{
	nvCheck(m_windowSize == 3);
	m_data[0] = 0; m_data[1] = 0; m_data[2] = 0;
	m_data[3] =-1; m_data[4] = 0; m_data[5] = 1;
	m_data[6] = 0; m_data[7] = 0; m_data[8] = 0;
}

/// Init sobel filter.
void Kernel2::initSobel()
{
	if (m_windowSize == 3)
	{
		m_data[0] = -1; m_data[1] = 0; m_data[2] = 1;
		m_data[3] = -2; m_data[4] = 0; m_data[5] = 2;
		m_data[6] = -1; m_data[7] = 0; m_data[8] = 1;
	}
	else if (m_windowSize == 5)
	{
		float elements[] = {
            -1, -2, 0, 2, 1,
            -2, -3, 0, 3, 2,
            -3, -4, 0, 4, 3,
            -2, -3, 0, 3, 2,
            -1, -2, 0, 2, 1
		};

		for (int i = 0; i < 5*5; i++) {
			m_data[i] = elements[i];
		}
	}
	else if (m_windowSize == 7)
	{
		float elements[] = {
            -1, -2, -3, 0, 3, 2, 1,
            -2, -3, -4, 0, 4, 3, 2,
            -3, -4, -5, 0, 5, 4, 3,
            -4, -5, -6, 0, 6, 5, 4,
            -3, -4, -5, 0, 5, 4, 3,
            -2, -3, -4, 0, 4, 3, 2,
            -1, -2, -3, 0, 3, 2, 1
		};

		for (int i = 0; i < 7*7; i++) {
			m_data[i] = elements[i];
		}
	}
	else if (m_windowSize == 9)
	{
		float elements[] = {
            -1, -2, -3, -4, 0, 4, 3, 2, 1,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -5, -6, -7, -8, 0, 8, 7, 6, 5,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -1, -2, -3, -4, 0, 4, 3, 2, 1
		};
		
		for (int i = 0; i < 9*9; i++) {
			m_data[i] = elements[i];
		}
	}
}

/// Init prewitt filter.
void Kernel2::initPrewitt()
{
	if (m_windowSize == 3)
	{
		m_data[0] = -1; m_data[1] = 0; m_data[2] = -1;
		m_data[3] = -1; m_data[4] = 0; m_data[5] = -1;
		m_data[6] = -1; m_data[7] = 0; m_data[8] = -1;
	}
	else if (m_windowSize == 5)
	{
		// @@ Is this correct?
		float elements[] = {
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2,
            -2, -1, 0, 1, 2
		};

		for (int i = 0; i < 5*5; i++) {
			m_data[i] = elements[i];
		}
	}
}

/// Init blended sobel filter.
void Kernel2::initBlendedSobel(const Vector4 & scale)
{
	nvCheck(m_windowSize == 9);

	{
		const float elements[] = {
            -1, -2, -3, -4, 0, 4, 3, 2, 1,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -5, -6, -7, -8, 0, 8, 7, 6, 5,
            -4, -5, -6, -7, 0, 7, 6, 5, 4,
            -3, -4, -5, -6, 0, 6, 5, 4, 3,
            -2, -3, -4, -5, 0, 5, 4, 3, 2,
            -1, -2, -3, -4, 0, 4, 3, 2, 1
		};
		
		for (int i = 0; i < 9*9; i++) {
			m_data[i] = elements[i] * scale.w();
		}
	}
	{
		const float elements[] = {
            -1, -2, -3, 0, 3, 2, 1,
            -2, -3, -4, 0, 4, 3, 2,
            -3, -4, -5, 0, 5, 4, 3,
            -4, -5, -6, 0, 6, 5, 4,
            -3, -4, -5, 0, 5, 4, 3,
            -2, -3, -4, 0, 4, 3, 2,
            -1, -2, -3, 0, 3, 2, 1,
		};

		for (int i = 0; i < 7; i++) {
			for (int e = 0; e < 7; e++) {
				m_data[i * 9 + e + 1] += elements[i * 7 + e] * scale.z();
			}
		}
	}
	{
		const float elements[] = {
            -1, -2, 0, 2, 1,
            -2, -3, 0, 3, 2,
            -3, -4, 0, 4, 3,
            -2, -3, 0, 3, 2,
            -1, -2, 0, 2, 1
		};

		for (int i = 0; i < 5; i++) {
			for (int e = 0; e < 5; e++) {
				m_data[i * 9 + e + 2] += elements[i * 5 + e] * scale.y();
			}
		}
	}
	{
		const float elements[] = {
            -1, 0, 1,
            -2, 0, 2,
            -1, 0, 1,
		};

		for (int i = 0; i < 3; i++) {
			for (int e = 0; e < 3; e++) {
				m_data[i * 9 + e + 3] += elements[i * 3 + e] * scale.x();
			}
		}
	}
}


static float frac(float f)
{
	return f - floorf(f);
}

static bool isMonoPhase(float w)
{
	return isZero(frac(w));
}



PolyphaseKernel::PolyphaseKernel(float w, uint l) :
	m_width(w),
	m_size(ceilf(w) + 1),
	m_length(l)
{
	// size = width + (length - 1) * phase
	m_data = new float[m_size * m_length];
}

PolyphaseKernel::PolyphaseKernel(const PolyphaseKernel & k) :
	m_width(k.m_width),
	m_size(k.m_size),
	m_length(k.m_length)
{
	m_data = new float[m_size * m_length];
	memcpy(m_data, k.m_data, sizeof(float) * m_size * m_length);
}

PolyphaseKernel::~PolyphaseKernel()
{
	delete [] m_data;
}


/* @@ Should we precompute left & right?

	// scale factor
	double dScale = double(uDstSize) / double(uSrcSize);

	if(dScale < 1.0) {
		// minification
		dWidth = dFilterWidth / dScale; 
		dFScale = dScale; 
	} else {
		// magnification
		dWidth= dFilterWidth; 
	}

	// window size is the number of sampled pixels
	m_WindowSize = 2 * (int)ceil(dWidth) + 1; 
	m_LineLength = uDstSize; 

	// offset for discrete to continuous coordinate conversion
	double dOffset = (0.5 / dScale) - 0.5;

	for(u = 0; u < m_LineLength; u++) {
		// scan through line of contributions
		double dCenter = (double)u / dScale + dOffset;   // reverse mapping
		// find the significant edge points that affect the pixel
		int iLeft = MAX (0, (int)floor (dCenter - dWidth)); 
		int iRight = MIN ((int)ceil (dCenter + dWidth), int(uSrcSize) - 1); 

		...
	}
*/

void PolyphaseKernel::initFilter(Filter::Enum f, int samples/*= 1*/)
{
	nvCheck(f < Filter::Num);
	
	float (* filter_function)(float) = s_filter_array[f].function;
	const float support = s_filter_array[f].support;
	
	const float half_width = m_width / 2;
	const float scale = support / half_width;
	
	for (uint j = 0; j < m_length; j++)
	{
		const float phase = frac(m_width * j);
		const float offset = half_width + phase;
		
		nvDebug("%d: ", j);

		float total = 0.0f;
		for (uint i = 0; i < m_size; i++)
		{
			float sample = sampleFilter(filter_function, i - offset, scale, samples);

			nvDebug("(%5.3f | %d) ", sample, j + i - m_size/2);

			m_data[j * m_size + i] = sample;
			total += sample;
		}

		nvDebug("\n");
		
		// normalize weights.
		for (uint i = 0; i < m_size; i++)
		{
			m_data[j * m_size + i] /= total;
			//m_data[j * m_size + i] /= samples;
		}
	}
}

void PolyphaseKernel::initKaiser(float alpha /*= 4.0f*/, float stretch /*= 1.0f*/)
{
	
}

/// Print the kernel for debugging purposes.
void PolyphaseKernel::debugPrint()
{
	for (uint j = 0; j < m_length; j++)
	{
		nvDebug("%d: ", j);
		for (uint i = 0; i < m_size; i++)
		{
			nvDebug(" %6.4f", m_data[j * m_size + i]);
		}
		nvDebug("\n");
	}
}

