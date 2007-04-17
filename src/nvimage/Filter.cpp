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
	if( x < 3.0f ) return(sincf(x) * sincf(x / 3.0f));
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

// Alternative bessel function from Paul Heckbert.
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
}

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

} // namespace



/// Ctor.
Kernel1::Kernel1(uint width) : w(width)
{
	data = new float[w];
}

/// Copy ctor.
Kernel1::Kernel1(const Kernel1 & k) : w(k.w)
{
	data = new float[w];
	for(uint i = 0; i < w; i++) {
		data[i] = k.data[i];
	}
}

/// Dtor.
Kernel1::~Kernel1()
{
	delete data;
}

/// Normalize the filter.
void Kernel1::normalize()
{
	float total = 0.0f;
	for(uint i = 0; i < w; i++) {
		total += data[i];
	}
	
	float inv = 1.0f / total;
	for(uint i = 0; i < w; i++) {
		data[i] *= inv;
	}
}


/// Init 1D Box filter.
void Kernel1::initFilter(Filter::Enum f)
{
	nvCheck((w & 1) == 0);
	nvCheck(f < Filter::Num);
	
	float (* filter_function)(float) = s_filter_array[f].function;
	const float support = s_filter_array[f].support;
	
	const float half_width = float(w / 2);
	const float offset = -half_width;
	const float nudge = 0.5f;
	
	for(uint i = 0; i < w; i++) {
		const float x = (i + offset) + nudge;
		data[i] = filter_function(x * support / half_width);
	}
	
	normalize();
}


/// Init 1D sinc filter.
void Kernel1::initSinc(float stretch /*= 1*/)
{
	nvCheck((w & 1) == 0);
	
	const float half_width = float(w / 2);
	const float offset = -half_width;
	const float nudge = 0.5f;
	
	for(uint i = 0; i < w; i++) {
		const float x = (i + offset) + nudge;
		data[i] = sincf(PI * x * stretch);
	}

	normalize();
}


/// Init 1D windowed Kaiser filter.
void Kernel1::initKaiser(float alpha, float stretch /*= 1*/)
{
	nvCheck((w & 1) == 0);
	
	const float half_width = float(w / 2);
	const float offset = -half_width;
	const float nudge = 0.5f;
	
	for(uint i = 0; i < w; i++) {
		const float x = (i + offset) + nudge;
		const float sinc_value = sincf(PI * x * stretch);
		const float window_value = filter_kaiser(x / half_width, alpha);
		
		data[i] = sinc_value * window_value;	// @@ sinc windowed by kaiser
	}

	normalize();
}


/// Init 1D Mitchell filter.
void Kernel1::initMitchell(float b, float c)
{
	nvCheck((w & 1) == 0);
	
	const float half_width = float(w / 2);
	const float offset = -half_width;
	const float nudge = 0.5f;
	
	for(uint i = 0; i < w; i++) {
		const float x = (i + offset) + nudge;
		data[i] = filter_mitchell(x / half_width, b, c);
	}
	
	normalize();
}


/// Print the kernel for debugging purposes.
void Kernel1::debugPrint()
{
	for(uint i = 0; i < w; i++) {
		nvDebug("%d: %f\n", i, data[i]);
	}
}



/// Ctor.
Kernel2::Kernel2(uint width) : w(width)
{
	data = new float[w*w];
}

/// Copy ctor.
Kernel2::Kernel2(const Kernel2 & k) : w(k.w)
{
	data = new float[w*w];
	for(uint i = 0; i < w*w; i++) {
		data[i] = k.data[i];
	}
}


/// Dtor.
Kernel2::~Kernel2()
{
	delete data;
}

/// Normalize the filter.
void Kernel2::normalize()
{
	float total = 0.0f;
	for(uint i = 0; i < w*w; i++) {
		total += fabs(data[i]);
	}
	
	float inv = 1.0f / total;
	for(uint i = 0; i < w*w; i++) {
		data[i] *= inv;
	}
}

/// Transpose the kernel.
void Kernel2::transpose()
{
	for(uint i = 0; i < w; i++) {
		for(uint j = i+1; j < w; j++) {
			swap(data[i*w + j], data[j*w + i]);
		}
	}
}

/// Init laplacian filter, usually used for sharpening.
void Kernel2::initLaplacian()
{
	nvDebugCheck(w == 3);
//	data[0] = -1; data[1] = -1; data[2] = -1;
//	data[3] = -1; data[4] = +8; data[5] = -1;
//	data[6] = -1; data[7] = -1; data[8] = -1;	
	
	data[0] = +0; data[1] = -1; data[2] = +0;
	data[3] = -1; data[4] = +4; data[5] = -1;
	data[6] = +0; data[7] = -1; data[8] = +0;	
	
//	data[0] = +1; data[1] = -2; data[2] = +1;
//	data[3] = -2; data[4] = +4; data[5] = -2;
//	data[6] = +1; data[7] = -2; data[8] = +1;	
}


/// Init simple edge detection filter.
void Kernel2::initEdgeDetection()
{
	nvCheck(w == 3);
	data[0] = 0; data[1] = 0; data[2] = 0;
	data[3] = -1; data[4] = 0; data[5] = 1;
	data[6] = 0; data[7] = 0; data[8] = 0;
}

/// Init sobel filter.
void Kernel2::initSobel()
{
	if (w == 3)
	{
		data[0] = -1; data[1] = 0; data[2] = 1;
		data[3] = -2; data[4] = 0; data[5] = 2;
		data[6] = -1; data[7] = 0; data[8] = 1;
	}
	else if (w == 5)
	{
		float elements[] = {
            -1, -2, 0, 2, 1,
            -2, -3, 0, 3, 2,
            -3, -4, 0, 4, 3,
            -2, -3, 0, 3, 2,
            -1, -2, 0, 2, 1
		};

		for (int i = 0; i < 5*5; i++) {
			data[i] = elements[i];
		}
	}
	else if (w == 7)
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
			data[i] = elements[i];
		}
	}
	else if (w == 9)
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
			data[i] = elements[i];
		}
	}
}

/// Init prewitt filter.
void Kernel2::initPrewitt()
{
	if (w == 3)
	{
		data[0] = -1; data[1] = 0; data[2] = -1;
		data[3] = -1; data[4] = 0; data[5] = -1;
		data[6] = -1; data[7] = 0; data[8] = -1;
	}
	else if (w == 5)
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
			data[i] = elements[i];
		}
	}
}

/// Init blended sobel filter.
void Kernel2::initBlendedSobel(const Vector4 & scale)
{
	nvCheck(w == 9);

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
			data[i] = elements[i] * scale.w();
		}
	}
	{
		float elements[] = {
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
				data[i * 9 + e + 1] += elements[i * 7 + e] * scale.z();
			}
		}
	}
	{
		float elements[] = {
            -1, -2, 0, 2, 1,
            -2, -3, 0, 3, 2,
            -3, -4, 0, 4, 3,
            -2, -3, 0, 3, 2,
            -1, -2, 0, 2, 1
		};

		for (int i = 0; i < 5; i++) {
			for (int e = 0; e < 5; e++) {
				data[i * 9 + e + 2] += elements[i * 5 + e] * scale.y();
			}
		}
	}
	{
		float elements[] = {
            -1, 0, 1,
            -2, 0, 2,
            -1, 0, 1,
		};

		for (int i = 0; i < 3; i++) {
			for (int e = 0; e < 3; e++) {
				data[i * 9 + e + 3] += elements[i * 3 + e] * scale.x();
			}
		}
	}
}


/*PI_DECLARE_TEST(BesselTest) {

	for(int i = 0; i < 8; i++) {
		nvDebug("bessel0(%i) %f =? %f\n", i, bessel0(i), _bessel0(i));
		PI_TEST(equalf(bessel0(i), _bessel0(i)));
	}

	return PiTestUnit::Succeed;
}*/

