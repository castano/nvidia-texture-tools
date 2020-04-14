

namespace icbc {

    void init();

    float compress_dxt1(const float input_colors[16 * 4], const float input_weights[16], const float color_weights[3], bool three_color_mode, bool hq, void * output);
    float compress_dxt1_fast(const float input_colors[16 * 4], const float input_weights[16], const float color_weights[3], void * output);
    void compress_dxt1_fast(const unsigned char input_colors[16 * 4], void * output);

    enum Decoder {
        Decoder_D3D10 = 0,
        Decoder_NVIDIA = 1,
        Decoder_AMD = 2
    };

    float evaluate_dxt1_error(const unsigned char rgba_block[16 * 4], const void * block, Decoder decoder = Decoder_D3D10);

}

#ifdef ICBC_IMPLEMENTATION

#ifndef ICBC_USE_SSE
#define ICBC_USE_SSE 2
#endif

#ifndef ICBC_DECODER
#define ICBC_DECODER 0       // 0 = d3d10, 1 = d3d9, 2 = nvidia, 3 = amd
#endif

#define ICBC_USE_SIMD ICBC_USE_SSE

// Some testing knobs:
#define ICBC_FAST_CLUSTER_FIT 0     // This ignores input weights for a moderate speedup.
#define ICBC_PERFECT_ROUND 0        // Enable perfect rounding in scalar code path only.

#include <stdint.h>
#include <string.h> // memset
#include <math.h>   // floorf
#include <float.h>  // FLT_MAX

#ifndef ICBC_ASSERT
#define ICBC_ASSERT assert
#include <assert.h>
#endif

namespace icbc {

///////////////////////////////////////////////////////////////////////////////////////////////////
// Basic Templates

template <typename T> inline void swap(T & a, T & b) {
    T temp(a);
    a = b;
    b = temp;
}

template <typename T> inline T max(const T & a, const T & b) {
    return (b < a) ? a : b;
}

template <typename T> inline T min(const T & a, const T & b) {
    return (a < b) ? a : b;
}

template <typename T> inline T clamp(const T & x, const T & a, const T & b) {
    return min(max(x, a), b);
}

template <typename T> inline T square(const T & a) {
    return a * a;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Basic Types

typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint32_t uint;


struct Color16 {
    union {
        struct {
            uint16 b : 5;
            uint16 g : 6;
            uint16 r : 5;
        };
        uint16 u;
    };
};

struct Color32 {
    union {
        struct {
            uint8 b, g, r, a;
        };
        uint32 u;
    };
};

struct BlockDXT1 {
    Color16 col0;
    Color16 col1;
    uint32 indices;
};


struct Vector3 {
    float x;
    float y;
    float z;

    inline void operator+=(Vector3 v) {
        x += v.x; y += v.y; z += v.z;
    }
    inline void operator*=(Vector3 v) {
        x *= v.x; y *= v.y; z *= v.z;
    }
    inline void operator*=(float s) {
        x *= s; y *= s; z *= s;
    }
};

struct Vector4 {
    union {
        struct {
            float x, y, z, w;
        };
        Vector3 xyz;
    };
};


inline Vector3 operator*(Vector3 v, float s) {
    return { v.x * s, v.y * s, v.z * s };
}

inline Vector3 operator*(float s, Vector3 v) {
    return { v.x * s, v.y * s, v.z * s };
}

inline Vector3 operator*(Vector3 a, Vector3 b) {
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}

inline float dot(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 operator+(Vector3 a, Vector3 b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vector3 operator-(Vector3 a, Vector3 b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Vector3 operator/(Vector3 v, float s) {
    return { v.x / s, v.y / s, v.z / s };
}

inline float saturate(float x) {
    return clamp(x, 0.0f, 1.0f);
}

inline Vector3 saturate(Vector3 v) {
    return { saturate(v.x), saturate(v.y), saturate(v.z) };
}

inline Vector3 min(Vector3 a, Vector3 b) {
    return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z) };
}

inline Vector3 max(Vector3 a, Vector3 b) {
    return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z) };
}

inline Vector3 round(Vector3 v) {
    return { floorf(v.x+0.5f), floorf(v.y + 0.5f), floorf(v.z + 0.5f) };
}

inline Vector3 floor(Vector3 v) {
    return { floorf(v.x), floorf(v.y), floorf(v.z) };
}

inline bool operator==(const Vector3 & a, const Vector3 & b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline Vector3 scalar_to_vector3(float f) {
    return {f, f, f};
}

inline float lengthSquared(Vector3 v) {
    return dot(v, v);
}

inline bool equal(float a, float b, float epsilon = 0.0001) {
    // http://realtimecollisiondetection.net/blog/?p=89
    //return fabsf(a - b) < epsilon * max(1.0f, max(fabsf(a), fabsf(b)));
    return fabsf(a - b) < epsilon;
}

inline bool equal(Vector3 a, Vector3 b, float epsilon) {
    return equal(a.x, b.x, epsilon) && equal(a.y, b.y, epsilon) && equal(a.z, b.z, epsilon);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD

#ifndef ICBC_ALIGN_16
#if __GNUC__
#   define ICBC_ALIGN_16 __attribute__ ((__aligned__ (16)))
#else // _MSC_VER
#   define ICBC_ALIGN_16 __declspec(align(16))
#endif
#endif

#if ICBC_USE_SIMD

#include <xmmintrin.h>
#include <emmintrin.h>

#define SIMD_INLINE inline
#define SIMD_NATIVE __forceinline

class SimdVector
{
public:
    __m128 vec;

    typedef SimdVector const& Arg;

    SIMD_NATIVE SimdVector() {}

    SIMD_NATIVE explicit SimdVector(__m128 v) : vec(v) {}

    SIMD_NATIVE explicit SimdVector(float f) {
        vec = _mm_set1_ps(f);
    }

    SIMD_NATIVE explicit SimdVector(const float * v)
    {
        vec = _mm_load_ps(v);
    }

    SIMD_NATIVE SimdVector(float x, float y, float z, float w)
    {
        vec = _mm_setr_ps(x, y, z, w);
    }

    SIMD_NATIVE SimdVector(const SimdVector & arg) : vec(arg.vec) {}

    SIMD_NATIVE SimdVector & operator=(const SimdVector & arg)
    {
        vec = arg.vec;
        return *this;
    }

    SIMD_INLINE float toFloat() const
    {
        ICBC_ALIGN_16 float f;
        _mm_store_ss(&f, vec);
        return f;
    }

    SIMD_INLINE Vector3 toVector3() const
    {
        ICBC_ALIGN_16 float c[4];
        _mm_store_ps(c, vec);
        return { c[0], c[1], c[2] };
    }

#define SSE_SPLAT( a ) ((a) | ((a) << 2) | ((a) << 4) | ((a) << 6))
    SIMD_NATIVE SimdVector splatX() const { return SimdVector(_mm_shuffle_ps(vec, vec, SSE_SPLAT(0))); }
    SIMD_NATIVE SimdVector splatY() const { return SimdVector(_mm_shuffle_ps(vec, vec, SSE_SPLAT(1))); }
    SIMD_NATIVE SimdVector splatZ() const { return SimdVector(_mm_shuffle_ps(vec, vec, SSE_SPLAT(2))); }
    SIMD_NATIVE SimdVector splatW() const { return SimdVector(_mm_shuffle_ps(vec, vec, SSE_SPLAT(3))); }
#undef SSE_SPLAT

    SIMD_NATIVE SimdVector& operator+=(Arg v)
    {
        vec = _mm_add_ps(vec, v.vec);
        return *this;
    }

    SIMD_NATIVE SimdVector& operator-=(Arg v)
    {
        vec = _mm_sub_ps(vec, v.vec);
        return *this;
    }

    SIMD_NATIVE SimdVector& operator*=(Arg v)
    {
        vec = _mm_mul_ps(vec, v.vec);
        return *this;
    }
};


SIMD_NATIVE SimdVector operator+(SimdVector::Arg left, SimdVector::Arg right)
{
    return SimdVector(_mm_add_ps(left.vec, right.vec));
}

SIMD_NATIVE SimdVector operator-(SimdVector::Arg left, SimdVector::Arg right)
{
    return SimdVector(_mm_sub_ps(left.vec, right.vec));
}

SIMD_NATIVE SimdVector operator*(SimdVector::Arg left, SimdVector::Arg right)
{
    return SimdVector(_mm_mul_ps(left.vec, right.vec));
}

// Returns a*b + c
SIMD_INLINE SimdVector multiplyAdd(SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c)
{
    return SimdVector(_mm_add_ps(_mm_mul_ps(a.vec, b.vec), c.vec));
}

// Returns -( a*b - c )
SIMD_INLINE SimdVector negativeMultiplySubtract(SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c)
{
    return SimdVector(_mm_sub_ps(c.vec, _mm_mul_ps(a.vec, b.vec)));
}

SIMD_INLINE SimdVector reciprocal(SimdVector::Arg v)
{
    // get the reciprocal estimate
    __m128 estimate = _mm_rcp_ps(v.vec);

    // one round of Newton-Rhaphson refinement
    __m128 diff = _mm_sub_ps(_mm_set1_ps(1.0f), _mm_mul_ps(estimate, v.vec));
    return SimdVector(_mm_add_ps(_mm_mul_ps(diff, estimate), estimate));
}

SIMD_NATIVE SimdVector min(SimdVector::Arg left, SimdVector::Arg right)
{
    return SimdVector(_mm_min_ps(left.vec, right.vec));
}

SIMD_NATIVE SimdVector max(SimdVector::Arg left, SimdVector::Arg right)
{
    return SimdVector(_mm_max_ps(left.vec, right.vec));
}

SIMD_INLINE SimdVector truncate(SimdVector::Arg v)
{
#if (ICBC_USE_SSE == 1)
    // convert to ints
    __m128 input = v.vec;
    __m64 lo = _mm_cvttps_pi32(input);
    __m64 hi = _mm_cvttps_pi32(_mm_movehl_ps(input, input));

    // convert to floats
    __m128 part = _mm_movelh_ps(input, _mm_cvtpi32_ps(input, hi));
    __m128 truncated = _mm_cvtpi32_ps(part, lo);

    // clear out the MMX multimedia state to allow FP calls later
    _mm_empty();
    return SimdVector(truncated);
#else
    // use SSE2 instructions
    return SimdVector(_mm_cvtepi32_ps(_mm_cvttps_epi32(v.vec)));
#endif
}

SIMD_INLINE SimdVector select(SimdVector::Arg off, SimdVector::Arg on, SimdVector::Arg bits)
{
    __m128 a = _mm_andnot_ps(bits.vec, off.vec);
    __m128 b = _mm_and_ps(bits.vec, on.vec);

    return SimdVector(_mm_or_ps(a, b));
}

SIMD_INLINE bool compareAnyLessThan(SimdVector::Arg left, SimdVector::Arg right)
{
    __m128 bits = _mm_cmplt_ps(left.vec, right.vec);
    int value = _mm_movemask_ps(bits);
    return value != 0;
}

#endif // ICBC_USE_SIMD


///////////////////////////////////////////////////////////////////////////////////////////////////
// Color conversion functions.

static const float midpoints5[32] = {
    0.015686f, 0.047059f, 0.078431f, 0.111765f, 0.145098f, 0.176471f, 0.207843f, 0.241176f, 0.274510f, 0.305882f, 0.337255f, 0.370588f, 0.403922f, 0.435294f, 0.466667f, 0.5f,
    0.533333f, 0.564706f, 0.596078f, 0.629412f, 0.662745f, 0.694118f, 0.725490f, 0.758824f, 0.792157f, 0.823529f, 0.854902f, 0.888235f, 0.921569f, 0.952941f, 0.984314f, 1.0f
};

static const float midpoints6[64] = {
    0.007843f, 0.023529f, 0.039216f, 0.054902f, 0.070588f, 0.086275f, 0.101961f, 0.117647f, 0.133333f, 0.149020f, 0.164706f, 0.180392f, 0.196078f, 0.211765f, 0.227451f, 0.245098f, 
    0.262745f, 0.278431f, 0.294118f, 0.309804f, 0.325490f, 0.341176f, 0.356863f, 0.372549f, 0.388235f, 0.403922f, 0.419608f, 0.435294f, 0.450980f, 0.466667f, 0.482353f, 0.500000f, 
    0.517647f, 0.533333f, 0.549020f, 0.564706f, 0.580392f, 0.596078f, 0.611765f, 0.627451f, 0.643137f, 0.658824f, 0.674510f, 0.690196f, 0.705882f, 0.721569f, 0.737255f, 0.754902f, 
    0.772549f, 0.788235f, 0.803922f, 0.819608f, 0.835294f, 0.850980f, 0.866667f, 0.882353f, 0.898039f, 0.913725f, 0.929412f, 0.945098f, 0.960784f, 0.976471f, 0.992157f, 1.0f
};

/*void init_tables() {
    for (int i = 0; i < 31; i++) {
        float f0 = float(((i+0) << 3) | ((i+0) >> 2)) / 255.0f;
        float f1 = float(((i+1) << 3) | ((i+1) >> 2)) / 255.0f;
        midpoints5[i] = (f0 + f1) * 0.5;
    }
    midpoints5[31] = 1.0f;

    for (int i = 0; i < 63; i++) {
        float f0 = float(((i+0) << 2) | ((i+0) >> 4)) / 255.0f;
        float f1 = float(((i+1) << 2) | ((i+1) >> 4)) / 255.0f;
        midpoints6[i] = (f0 + f1) * 0.5;
    }
    midpoints6[63] = 1.0f;
}*/

static Color16 vector3_to_color16(const Vector3 & v) {

    // Truncate.
    uint r = uint(clamp(v.x * 31.0f, 0.0f, 31.0f));
	uint g = uint(clamp(v.y * 63.0f, 0.0f, 63.0f));
	uint b = uint(clamp(v.z * 31.0f, 0.0f, 31.0f));

    // Round exactly according to 565 bit-expansion.
    r += (v.x > midpoints5[r]);
    g += (v.y > midpoints6[g]);
    b += (v.z > midpoints5[b]);

    Color16 c;
    c.u = (r << 11) | (g << 5) | b;
    return c;
}

static Color32 bitexpand_color16_to_color32(Color16 c16) {
    Color32 c32;
    //c32.b = (c16.b << 3) | (c16.b >> 2);
    //c32.g = (c16.g << 2) | (c16.g >> 4);
    //c32.r = (c16.r << 3) | (c16.r >> 2);
    //c32.a = 0xFF;

    c32.u = ((c16.u << 3) & 0xf8) | ((c16.u << 5) & 0xfc00) | ((c16.u << 8) & 0xf80000);
    c32.u |= (c32.u >> 5) & 0x070007;
    c32.u |= (c32.u >> 6) & 0x000300;

    return c32;
}

inline Vector3 color_to_vector3(Color32 c) {
    return { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f };
}

inline Color32 vector3_to_color32(Vector3 v) {
    Color32 color;
    color.r = uint8(saturate(v.x) * 255 + 0.5f);
    color.g = uint8(saturate(v.y) * 255 + 0.5f);
    color.b = uint8(saturate(v.z) * 255 + 0.5f);
    color.a = 255;
    return color;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Input block processing.

// Find similar colors and combine them together.
static int reduce_colors(const Vector4 * input_colors, const float * input_weights, Vector3 * colors, float * weights)
{
#if 0
    for (int i = 0; i < 16; i++) {
        colors[i] = input_colors[i].xyz;
        weights[i] = input_weights[i];
    }
    return 16;
#else
    int n = 0;
    for (int i = 0; i < 16; i++)
    {
        Vector3 ci = input_colors[i].xyz;
        float wi = input_weights[i];

        float threshold = 1.0 / 256;

        if (wi > 0) {
            // Find matching color.
            int j;
            for (j = 0; j < n; j++) {
                if (equal(colors[j], ci, threshold)) {
                    weights[j] += wi;
                    break;
                }
            }

            // No match found. Add new color.
            if (j == n) {
                colors[n] = ci;
                weights[n] = wi;
                n++;
            }
        }
    }

    ICBC_ASSERT(n <= 16);

    return n;
#endif
}

static int reduce_colors(const uint8 * input_colors, Vector3 * colors, float * weights)
{
    int n = 0;
    for (int i = 0; i < 16; i++)
    {
        Vector3 ci;
        ci.x = float(input_colors[4 * i + 0]);
        ci.y = float(input_colors[4 * i + 1]);
        ci.z = float(input_colors[4 * i + 2]);

        float threshold = 1.0 / 256;

        // Find matching color.
        int j;
        for (j = 0; j < n; j++) {
            if (equal(colors[j], ci, threshold)) {
                weights[j] += 1.0f;
                break;
            }
        }

        // No match found. Add new color.
        if (j == n) {
            colors[n] = ci;
            weights[n] = 1.0f;
            n++;
        }
    }

    ICBC_ASSERT(n <= 16);

    return n;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
// Cluster Fit

class ClusterFit
{
public:
    ClusterFit() {}

    void setErrorMetric(const Vector3 & metric);

    void setColorSet(const Vector3 * colors, const float * weights, int count, const Vector3 & metric);
    void setColorSet(const Vector4 * colors, const Vector3 & metric);

    float bestError() const;

    bool compress3(Vector3 * start, Vector3 * end);
    bool compress4(Vector3 * start, Vector3 * end);

    bool fastCompress3(Vector3 * start, Vector3 * end);
    bool fastCompress4(Vector3 * start, Vector3 * end);


private:

    uint m_count;

#if ICBC_USE_SIMD
    ICBC_ALIGN_16 SimdVector m_weighted[16]; // color | weight
    SimdVector m_metric;                // vec3
    SimdVector m_metricSqr;             // vec3
    SimdVector m_xxsum;                 // color | weight
    SimdVector m_xsum;                  // color | weight (wsum)
    SimdVector m_besterror;             // scalar
#else
    Vector3 m_weighted[16];
    float m_weights[16];
    Vector3 m_metric;
    Vector3 m_metricSqr;
    Vector3 m_xxsum;
    Vector3 m_xsum;
    float m_wsum;
    float m_besterror;
#endif
};


static Vector3 computeCentroid(int n, const Vector3 *__restrict points, const float *__restrict weights)
{
    Vector3 centroid = { 0 };
    float total = 0.0f;

    for (int i = 0; i < n; i++)
    {
        total += weights[i];
        centroid += weights[i] * points[i];
    }
    centroid *= (1.0f / total);

    return centroid;
}

static Vector3 computeCovariance(int n, const Vector3 *__restrict points, const float *__restrict weights, float *__restrict covariance)
{
    // compute the centroid
    Vector3 centroid = computeCentroid(n, points, weights);

    // compute covariance matrix
    for (int i = 0; i < 6; i++)
    {
        covariance[i] = 0.0f;
    }

    for (int i = 0; i < n; i++)
    {
        Vector3 a = (points[i] - centroid);    // @@ I think weight should be squared, but that seems to increase the error slightly.
        Vector3 b = weights[i] * a;

        covariance[0] += a.x * b.x;
        covariance[1] += a.x * b.y;
        covariance[2] += a.x * b.z;
        covariance[3] += a.y * b.y;
        covariance[4] += a.y * b.z;
        covariance[5] += a.z * b.z;
    }

    return centroid;
}

// @@ We should be able to do something cheaper...
static Vector3 estimatePrincipalComponent(const float * __restrict matrix)
{
    const Vector3 row0 = { matrix[0], matrix[1], matrix[2] };
    const Vector3 row1 = { matrix[1], matrix[3], matrix[4] };
    const Vector3 row2 = { matrix[2], matrix[4], matrix[5] };

    float r0 = lengthSquared(row0);
    float r1 = lengthSquared(row1);
    float r2 = lengthSquared(row2);

    if (r0 > r1 && r0 > r2) return row0;
    if (r1 > r2) return row1;
    return row2;
}

static inline Vector3 firstEigenVector_PowerMethod(const float *__restrict matrix)
{
    if (matrix[0] == 0 && matrix[3] == 0 && matrix[5] == 0)
    {
        return {0};
    }

    Vector3 v = estimatePrincipalComponent(matrix);

    const int NUM = 8;
    for (int i = 0; i < NUM; i++)
    {
        float x = v.x * matrix[0] + v.y * matrix[1] + v.z * matrix[2];
        float y = v.x * matrix[1] + v.y * matrix[3] + v.z * matrix[4];
        float z = v.x * matrix[2] + v.y * matrix[4] + v.z * matrix[5];

        float norm = max(max(x, y), z);

        v = { x, y, z };
        v *= (1.0f / norm);
    }

    return v;
}

static Vector3 computePrincipalComponent_PowerMethod(int n, const Vector3 *__restrict points, const float *__restrict weights)
{
    float matrix[6];
    computeCovariance(n, points, weights, matrix);

    return firstEigenVector_PowerMethod(matrix);
}


void ClusterFit::setErrorMetric(const Vector3 & metric)
{
#if ICBC_USE_SIMD
    ICBC_ALIGN_16 Vector4 tmp;
    tmp.xyz = metric;
    tmp.w = 1;
    m_metric = SimdVector(&tmp.x);
#else
    m_metric = metric;
#endif
    m_metricSqr = m_metric * m_metric;
}

void ClusterFit::setColorSet(const Vector3 * colors, const float * weights, int count, const Vector3 & metric)
{
    setErrorMetric(metric);

    // initialise the best error
#if ICBC_USE_SIMD
    m_besterror = SimdVector(FLT_MAX);
#else
    m_besterror = FLT_MAX;
#endif

    m_count = count;

    // I've tried using a lower quality approximation of the principal direction, but the best fit line seems to produce best results.
    Vector3 principal = computePrincipalComponent_PowerMethod(count, colors, weights);

    // build the list of values
    int order[16];
    float dps[16];
    for (uint i = 0; i < m_count; ++i)
    {
        order[i] = i;
        dps[i] = dot(colors[i], principal);
    }

    // stable sort
    for (uint i = 0; i < m_count; ++i)
    {
        for (uint j = i; j > 0 && dps[j] < dps[j - 1]; --j)
        {
            swap(dps[j], dps[j - 1]);
            swap(order[j], order[j - 1]);
        }
    }

    // weight all the points
#if ICBC_USE_SIMD
    m_xxsum = SimdVector(0.0f);
    m_xsum = SimdVector(0.0f);
#else
    m_xxsum = { 0.0f };
    m_xsum = { 0.0f };
    m_wsum = 0.0f;
#endif

    for (uint i = 0; i < m_count; ++i)
    {
        int p = order[i];
#if ICBC_USE_SIMD
        ICBC_ALIGN_16 Vector4 tmp;
        tmp.xyz = colors[p];
        tmp.w = 1;
        m_weighted[i] = SimdVector(&tmp.x) * SimdVector(weights[p]);
        m_xxsum += m_weighted[i] * m_weighted[i];
        m_xsum += m_weighted[i];
#else
        m_weighted[i] = colors[p] * weights[p];
        m_xxsum += m_weighted[i] * m_weighted[i];
        m_xsum += m_weighted[i];
        m_weights[i] = weights[p];
        m_wsum += m_weights[i];
#endif
    }
}

void ClusterFit::setColorSet(const Vector4 * colors, const Vector3 & metric)
{
    setErrorMetric(metric);

    // initialise the best error
#if ICBC_USE_SIMD
    m_besterror = SimdVector(FLT_MAX);
#else
    m_besterror = FLT_MAX;
#endif

    m_count = 16;

    static const float weights[16] = {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1};
    Vector3 vc[16];
    for (int i = 0; i < 16; i++) vc[i] = colors[i].xyz;

    // I've tried using a lower quality approximation of the principal direction, but the best fit line seems to produce best results.
    Vector3 principal = computePrincipalComponent_PowerMethod(16, vc, weights);

    // build the list of values
    int order[16];
    float dps[16];
    for (uint i = 0; i < m_count; ++i)
    {
        order[i] = i;
        dps[i] = dot(colors[i].xyz, principal);
    }

    // stable sort
    for (uint i = 0; i < m_count; ++i)
    {
        for (uint j = i; j > 0 && dps[j] < dps[j - 1]; --j)
        {
            swap(dps[j], dps[j - 1]);
            swap(order[j], order[j - 1]);
        }
    }

    // weight all the points
#if ICBC_USE_SIMD
    m_xxsum = SimdVector(0.0f);
    m_xsum = SimdVector(0.0f);
#else
    m_xxsum = { 0.0f };
    m_xsum = { 0.0f };
    m_wsum = 0.0f;
#endif

    for (uint i = 0; i < 16; ++i)
    {
        int p = order[i];
#if ICBC_USE_SIMD
        ICBC_ALIGN_16 Vector4 tmp;
        tmp.xyz = colors[p].xyz;
        tmp.w = 1;
        m_weighted[i] = SimdVector(&tmp.x);
        m_xxsum += m_weighted[i] * m_weighted[i];
        m_xsum += m_weighted[i];
#else
        m_weighted[i] = colors[p].xyz;
        m_xxsum += m_weighted[i] * m_weighted[i];
        m_xsum += m_weighted[i];
        m_weights[i] = 1.0f;
        m_wsum += m_weights[i];
#endif
    }
}

float ClusterFit::bestError() const
{
#if ICBC_USE_SIMD
    SimdVector x = m_xxsum * m_metricSqr;
    SimdVector error = m_besterror + x.splatX() + x.splatY() + x.splatZ();
    return error.toFloat();
#else
    return m_besterror + dot(m_xxsum, m_metricSqr);
#endif
}

struct Precomp {
    float alpha2_sum;
    float beta2_sum;
    float alphabeta_sum;
    float factor;
};

static const ICBC_ALIGN_16 Precomp s_threeElement[153] = {
    { 0.000000f, 16.000000f, 0.000000f, FLT_MAX }, // 0 (0 0 16)
    { 0.250000f, 15.250000f, 0.250000f, 0.266667f }, // 1 (0 1 15)
    { 0.500000f, 14.500000f, 0.500000f, 0.142857f }, // 2 (0 2 14)
    { 0.750000f, 13.750000f, 0.750000f, 0.102564f }, // 3 (0 3 13)
    { 1.000000f, 13.000000f, 1.000000f, 0.083333f }, // 4 (0 4 12)
    { 1.250000f, 12.250000f, 1.250000f, 0.072727f }, // 5 (0 5 11)
    { 1.500000f, 11.500000f, 1.500000f, 0.066667f }, // 6 (0 6 10)
    { 1.750000f, 10.750000f, 1.750000f, 0.063492f }, // 7 (0 7 9)
    { 2.000000f, 10.000000f, 2.000000f, 0.062500f }, // 8 (0 8 8)
    { 2.250000f, 9.250000f, 2.250000f, 0.063492f }, // 9 (0 9 7)
    { 2.500000f, 8.500000f, 2.500000f, 0.066667f }, // 10 (0 10 6)
    { 2.750000f, 7.750000f, 2.750000f, 0.072727f }, // 11 (0 11 5)
    { 3.000000f, 7.000000f, 3.000000f, 0.083333f }, // 12 (0 12 4)
    { 3.250000f, 6.250000f, 3.250000f, 0.102564f }, // 13 (0 13 3)
    { 3.500000f, 5.500000f, 3.500000f, 0.142857f }, // 14 (0 14 2)
    { 3.750000f, 4.750000f, 3.750000f, 0.266667f }, // 15 (0 15 1)
    { 4.000000f, 4.000000f, 4.000000f, FLT_MAX }, // 16 (0 16 0)
    { 1.000000f, 15.000000f, 0.000000f, 0.066667f }, // 17 (1 0 15)
    { 1.250000f, 14.250000f, 0.250000f, 0.056338f }, // 18 (1 1 14)
    { 1.500000f, 13.500000f, 0.500000f, 0.050000f }, // 19 (1 2 13)
    { 1.750000f, 12.750000f, 0.750000f, 0.045977f }, // 20 (1 3 12)
    { 2.000000f, 12.000000f, 1.000000f, 0.043478f }, // 21 (1 4 11)
    { 2.250000f, 11.250000f, 1.250000f, 0.042105f }, // 22 (1 5 10)
    { 2.500000f, 10.500000f, 1.500000f, 0.041667f }, // 23 (1 6 9)
    { 2.750000f, 9.750000f, 1.750000f, 0.042105f }, // 24 (1 7 8)
    { 3.000000f, 9.000000f, 2.000000f, 0.043478f }, // 25 (1 8 7)
    { 3.250000f, 8.250000f, 2.250000f, 0.045977f }, // 26 (1 9 6)
    { 3.500000f, 7.500000f, 2.500000f, 0.050000f }, // 27 (1 10 5)
    { 3.750000f, 6.750000f, 2.750000f, 0.056338f }, // 28 (1 11 4)
    { 4.000000f, 6.000000f, 3.000000f, 0.066667f }, // 29 (1 12 3)
    { 4.250000f, 5.250000f, 3.250000f, 0.085106f }, // 30 (1 13 2)
    { 4.500000f, 4.500000f, 3.500000f, 0.125000f }, // 31 (1 14 1)
    { 4.750000f, 3.750000f, 3.750000f, 0.266667f }, // 32 (1 15 0)
    { 2.000000f, 14.000000f, 0.000000f, 0.035714f }, // 33 (2 0 14)
    { 2.250000f, 13.250000f, 0.250000f, 0.033613f }, // 34 (2 1 13)
    { 2.500000f, 12.500000f, 0.500000f, 0.032258f }, // 35 (2 2 12)
    { 2.750000f, 11.750000f, 0.750000f, 0.031496f }, // 36 (2 3 11)
    { 3.000000f, 11.000000f, 1.000000f, 0.031250f }, // 37 (2 4 10)
    { 3.250000f, 10.250000f, 1.250000f, 0.031496f }, // 38 (2 5 9)
    { 3.500000f, 9.500000f, 1.500000f, 0.032258f }, // 39 (2 6 8)
    { 3.750000f, 8.750000f, 1.750000f, 0.033613f }, // 40 (2 7 7)
    { 4.000000f, 8.000000f, 2.000000f, 0.035714f }, // 41 (2 8 6)
    { 4.250000f, 7.250000f, 2.250000f, 0.038835f }, // 42 (2 9 5)
    { 4.500000f, 6.500000f, 2.500000f, 0.043478f }, // 43 (2 10 4)
    { 4.750000f, 5.750000f, 2.750000f, 0.050633f }, // 44 (2 11 3)
    { 5.000000f, 5.000000f, 3.000000f, 0.062500f }, // 45 (2 12 2)
    { 5.250000f, 4.250000f, 3.250000f, 0.085106f }, // 46 (2 13 1)
    { 5.500000f, 3.500000f, 3.500000f, 0.142857f }, // 47 (2 14 0)
    { 3.000000f, 13.000000f, 0.000000f, 0.025641f }, // 48 (3 0 13)
    { 3.250000f, 12.250000f, 0.250000f, 0.025157f }, // 49 (3 1 12)
    { 3.500000f, 11.500000f, 0.500000f, 0.025000f }, // 50 (3 2 11)
    { 3.750000f, 10.750000f, 0.750000f, 0.025157f }, // 51 (3 3 10)
    { 4.000000f, 10.000000f, 1.000000f, 0.025641f }, // 52 (3 4 9)
    { 4.250000f, 9.250000f, 1.250000f, 0.026490f }, // 53 (3 5 8)
    { 4.500000f, 8.500000f, 1.500000f, 0.027778f }, // 54 (3 6 7)
    { 4.750000f, 7.750000f, 1.750000f, 0.029630f }, // 55 (3 7 6)
    { 5.000000f, 7.000000f, 2.000000f, 0.032258f }, // 56 (3 8 5)
    { 5.250000f, 6.250000f, 2.250000f, 0.036036f }, // 57 (3 9 4)
    { 5.500000f, 5.500000f, 2.500000f, 0.041667f }, // 58 (3 10 3)
    { 5.750000f, 4.750000f, 2.750000f, 0.050633f }, // 59 (3 11 2)
    { 6.000000f, 4.000000f, 3.000000f, 0.066667f }, // 60 (3 12 1)
    { 6.250000f, 3.250000f, 3.250000f, 0.102564f }, // 61 (3 13 0)
    { 4.000000f, 12.000000f, 0.000000f, 0.020833f }, // 62 (4 0 12)
    { 4.250000f, 11.250000f, 0.250000f, 0.020942f }, // 63 (4 1 11)
    { 4.500000f, 10.500000f, 0.500000f, 0.021277f }, // 64 (4 2 10)
    { 4.750000f, 9.750000f, 0.750000f, 0.021858f }, // 65 (4 3 9)
    { 5.000000f, 9.000000f, 1.000000f, 0.022727f }, // 66 (4 4 8)
    { 5.250000f, 8.250000f, 1.250000f, 0.023952f }, // 67 (4 5 7)
    { 5.500000f, 7.500000f, 1.500000f, 0.025641f }, // 68 (4 6 6)
    { 5.750000f, 6.750000f, 1.750000f, 0.027972f }, // 69 (4 7 5)
    { 6.000000f, 6.000000f, 2.000000f, 0.031250f }, // 70 (4 8 4)
    { 6.250000f, 5.250000f, 2.250000f, 0.036036f }, // 71 (4 9 3)
    { 6.500000f, 4.500000f, 2.500000f, 0.043478f }, // 72 (4 10 2)
    { 6.750000f, 3.750000f, 2.750000f, 0.056338f }, // 73 (4 11 1)
    { 7.000000f, 3.000000f, 3.000000f, 0.083333f }, // 74 (4 12 0)
    { 5.000000f, 11.000000f, 0.000000f, 0.018182f }, // 75 (5 0 11)
    { 5.250000f, 10.250000f, 0.250000f, 0.018605f }, // 76 (5 1 10)
    { 5.500000f, 9.500000f, 0.500000f, 0.019231f }, // 77 (5 2 9)
    { 5.750000f, 8.750000f, 0.750000f, 0.020101f }, // 78 (5 3 8)
    { 6.000000f, 8.000000f, 1.000000f, 0.021277f }, // 79 (5 4 7)
    { 6.250000f, 7.250000f, 1.250000f, 0.022857f }, // 80 (5 5 6)
    { 6.500000f, 6.500000f, 1.500000f, 0.025000f }, // 81 (5 6 5)
    { 6.750000f, 5.750000f, 1.750000f, 0.027972f }, // 82 (5 7 4)
    { 7.000000f, 5.000000f, 2.000000f, 0.032258f }, // 83 (5 8 3)
    { 7.250000f, 4.250000f, 2.250000f, 0.038835f }, // 84 (5 9 2)
    { 7.500000f, 3.500000f, 2.500000f, 0.050000f }, // 85 (5 10 1)
    { 7.750000f, 2.750000f, 2.750000f, 0.072727f }, // 86 (5 11 0)
    { 6.000000f, 10.000000f, 0.000000f, 0.016667f }, // 87 (6 0 10)
    { 6.250000f, 9.250000f, 0.250000f, 0.017316f }, // 88 (6 1 9)
    { 6.500000f, 8.500000f, 0.500000f, 0.018182f }, // 89 (6 2 8)
    { 6.750000f, 7.750000f, 0.750000f, 0.019324f }, // 90 (6 3 7)
    { 7.000000f, 7.000000f, 1.000000f, 0.020833f }, // 91 (6 4 6)
    { 7.250000f, 6.250000f, 1.250000f, 0.022857f }, // 92 (6 5 5)
    { 7.500000f, 5.500000f, 1.500000f, 0.025641f }, // 93 (6 6 4)
    { 7.750000f, 4.750000f, 1.750000f, 0.029630f }, // 94 (6 7 3)
    { 8.000000f, 4.000000f, 2.000000f, 0.035714f }, // 95 (6 8 2)
    { 8.250000f, 3.250000f, 2.250000f, 0.045977f }, // 96 (6 9 1)
    { 8.500000f, 2.500000f, 2.500000f, 0.066667f }, // 97 (6 10 0)
    { 7.000000f, 9.000000f, 0.000000f, 0.015873f }, // 98 (7 0 9)
    { 7.250000f, 8.250000f, 0.250000f, 0.016736f }, // 99 (7 1 8)
    { 7.500000f, 7.500000f, 0.500000f, 0.017857f }, // 100 (7 2 7)
    { 7.750000f, 6.750000f, 0.750000f, 0.019324f }, // 101 (7 3 6)
    { 8.000000f, 6.000000f, 1.000000f, 0.021277f }, // 102 (7 4 5)
    { 8.250000f, 5.250000f, 1.250000f, 0.023952f }, // 103 (7 5 4)
    { 8.500000f, 4.500000f, 1.500000f, 0.027778f }, // 104 (7 6 3)
    { 8.750000f, 3.750000f, 1.750000f, 0.033613f }, // 105 (7 7 2)
    { 9.000000f, 3.000000f, 2.000000f, 0.043478f }, // 106 (7 8 1)
    { 9.250000f, 2.250000f, 2.250000f, 0.063492f }, // 107 (7 9 0)
    { 8.000000f, 8.000000f, 0.000000f, 0.015625f }, // 108 (8 0 8)
    { 8.250000f, 7.250000f, 0.250000f, 0.016736f }, // 109 (8 1 7)
    { 8.500000f, 6.500000f, 0.500000f, 0.018182f }, // 110 (8 2 6)
    { 8.750000f, 5.750000f, 0.750000f, 0.020101f }, // 111 (8 3 5)
    { 9.000000f, 5.000000f, 1.000000f, 0.022727f }, // 112 (8 4 4)
    { 9.250000f, 4.250000f, 1.250000f, 0.026490f }, // 113 (8 5 3)
    { 9.500000f, 3.500000f, 1.500000f, 0.032258f }, // 114 (8 6 2)
    { 9.750000f, 2.750000f, 1.750000f, 0.042105f }, // 115 (8 7 1)
    { 10.000000f, 2.000000f, 2.000000f, 0.062500f }, // 116 (8 8 0)
    { 9.000000f, 7.000000f, 0.000000f, 0.015873f }, // 117 (9 0 7)
    { 9.250000f, 6.250000f, 0.250000f, 0.017316f }, // 118 (9 1 6)
    { 9.500000f, 5.500000f, 0.500000f, 0.019231f }, // 119 (9 2 5)
    { 9.750000f, 4.750000f, 0.750000f, 0.021858f }, // 120 (9 3 4)
    { 10.000000f, 4.000000f, 1.000000f, 0.025641f }, // 121 (9 4 3)
    { 10.250000f, 3.250000f, 1.250000f, 0.031496f }, // 122 (9 5 2)
    { 10.500000f, 2.500000f, 1.500000f, 0.041667f }, // 123 (9 6 1)
    { 10.750000f, 1.750000f, 1.750000f, 0.063492f }, // 124 (9 7 0)
    { 10.000000f, 6.000000f, 0.000000f, 0.016667f }, // 125 (10 0 6)
    { 10.250000f, 5.250000f, 0.250000f, 0.018605f }, // 126 (10 1 5)
    { 10.500000f, 4.500000f, 0.500000f, 0.021277f }, // 127 (10 2 4)
    { 10.750000f, 3.750000f, 0.750000f, 0.025157f }, // 128 (10 3 3)
    { 11.000000f, 3.000000f, 1.000000f, 0.031250f }, // 129 (10 4 2)
    { 11.250000f, 2.250000f, 1.250000f, 0.042105f }, // 130 (10 5 1)
    { 11.500000f, 1.500000f, 1.500000f, 0.066667f }, // 131 (10 6 0)
    { 11.000000f, 5.000000f, 0.000000f, 0.018182f }, // 132 (11 0 5)
    { 11.250000f, 4.250000f, 0.250000f, 0.020942f }, // 133 (11 1 4)
    { 11.500000f, 3.500000f, 0.500000f, 0.025000f }, // 134 (11 2 3)
    { 11.750000f, 2.750000f, 0.750000f, 0.031496f }, // 135 (11 3 2)
    { 12.000000f, 2.000000f, 1.000000f, 0.043478f }, // 136 (11 4 1)
    { 12.250000f, 1.250000f, 1.250000f, 0.072727f }, // 137 (11 5 0)
    { 12.000000f, 4.000000f, 0.000000f, 0.020833f }, // 138 (12 0 4)
    { 12.250000f, 3.250000f, 0.250000f, 0.025157f }, // 139 (12 1 3)
    { 12.500000f, 2.500000f, 0.500000f, 0.032258f }, // 140 (12 2 2)
    { 12.750000f, 1.750000f, 0.750000f, 0.045977f }, // 141 (12 3 1)
    { 13.000000f, 1.000000f, 1.000000f, 0.083333f }, // 142 (12 4 0)
    { 13.000000f, 3.000000f, 0.000000f, 0.025641f }, // 143 (13 0 3)
    { 13.250000f, 2.250000f, 0.250000f, 0.033613f }, // 144 (13 1 2)
    { 13.500000f, 1.500000f, 0.500000f, 0.050000f }, // 145 (13 2 1)
    { 13.750000f, 0.750000f, 0.750000f, 0.102564f }, // 146 (13 3 0)
    { 14.000000f, 2.000000f, 0.000000f, 0.035714f }, // 147 (14 0 2)
    { 14.250000f, 1.250000f, 0.250000f, 0.056338f }, // 148 (14 1 1)
    { 14.500000f, 0.500000f, 0.500000f, 0.142857f }, // 149 (14 2 0)
    { 15.000000f, 1.000000f, 0.000000f, 0.066667f }, // 150 (15 0 1)
    { 15.250000f, 0.250000f, 0.250000f, 0.266667f }, // 151 (15 1 0)
    { 16.000000f, 0.000000f, 0.000000f, FLT_MAX }, // 152 (16 0 0)
}; // 153 three cluster elements

static const ICBC_ALIGN_16 Precomp s_fourElement[969] = {
    { 0.000000f, 16.000000f, 0.000000f, FLT_MAX }, // 0 (0 0 0 16)
    { 0.111111f, 15.444445f, 0.222222f, 0.600000f }, // 1 (0 0 1 15)
    { 0.222222f, 14.888889f, 0.444444f, 0.321429f }, // 2 (0 0 2 14)
    { 0.333333f, 14.333333f, 0.666667f, 0.230769f }, // 3 (0 0 3 13)
    { 0.444444f, 13.777778f, 0.888889f, 0.187500f }, // 4 (0 0 4 12)
    { 0.555556f, 13.222222f, 1.111111f, 0.163636f }, // 5 (0 0 5 11)
    { 0.666667f, 12.666667f, 1.333333f, 0.150000f }, // 6 (0 0 6 10)
    { 0.777778f, 12.111111f, 1.555556f, 0.142857f }, // 7 (0 0 7 9)
    { 0.888889f, 11.555555f, 1.777778f, 0.140625f }, // 8 (0 0 8 8)
    { 1.000000f, 11.000000f, 2.000000f, 0.142857f }, // 9 (0 0 9 7)
    { 1.111111f, 10.444445f, 2.222222f, 0.150000f }, // 10 (0 0 10 6)
    { 1.222222f, 9.888889f, 2.444444f, 0.163636f }, // 11 (0 0 11 5)
    { 1.333333f, 9.333333f, 2.666667f, 0.187500f }, // 12 (0 0 12 4)
    { 1.444444f, 8.777778f, 2.888889f, 0.230769f }, // 13 (0 0 13 3)
    { 1.555556f, 8.222222f, 3.111111f, 0.321429f }, // 14 (0 0 14 2)
    { 1.666667f, 7.666667f, 3.333333f, 0.600000f }, // 15 (0 0 15 1)
    { 1.777778f, 7.111111f, 3.555556f, FLT_MAX }, // 16 (0 0 16 0)
    { 0.444444f, 15.111111f, 0.222222f, 0.150000f }, // 17 (0 1 0 15)
    { 0.555556f, 14.555555f, 0.444444f, 0.126761f }, // 18 (0 1 1 14)
    { 0.666667f, 14.000000f, 0.666667f, 0.112500f }, // 19 (0 1 2 13)
    { 0.777778f, 13.444445f, 0.888889f, 0.103448f }, // 20 (0 1 3 12)
    { 0.888889f, 12.888889f, 1.111111f, 0.097826f }, // 21 (0 1 4 11)
    { 1.000000f, 12.333333f, 1.333333f, 0.094737f }, // 22 (0 1 5 10)
    { 1.111111f, 11.777778f, 1.555556f, 0.093750f }, // 23 (0 1 6 9)
    { 1.222222f, 11.222222f, 1.777778f, 0.094737f }, // 24 (0 1 7 8)
    { 1.333333f, 10.666667f, 2.000000f, 0.097826f }, // 25 (0 1 8 7)
    { 1.444444f, 10.111111f, 2.222222f, 0.103448f }, // 26 (0 1 9 6)
    { 1.555556f, 9.555555f, 2.444444f, 0.112500f }, // 27 (0 1 10 5)
    { 1.666667f, 9.000000f, 2.666667f, 0.126761f }, // 28 (0 1 11 4)
    { 1.777778f, 8.444445f, 2.888889f, 0.150000f }, // 29 (0 1 12 3)
    { 1.888889f, 7.888889f, 3.111111f, 0.191489f }, // 30 (0 1 13 2)
    { 2.000000f, 7.333333f, 3.333333f, 0.281250f }, // 31 (0 1 14 1)
    { 2.111111f, 6.777778f, 3.555556f, 0.600000f }, // 32 (0 1 15 0)
    { 0.888889f, 14.222222f, 0.444444f, 0.080357f }, // 33 (0 2 0 14)
    { 1.000000f, 13.666667f, 0.666667f, 0.075630f }, // 34 (0 2 1 13)
    { 1.111111f, 13.111111f, 0.888889f, 0.072581f }, // 35 (0 2 2 12)
    { 1.222222f, 12.555555f, 1.111111f, 0.070866f }, // 36 (0 2 3 11)
    { 1.333333f, 12.000000f, 1.333333f, 0.070313f }, // 37 (0 2 4 10)
    { 1.444444f, 11.444445f, 1.555556f, 0.070866f }, // 38 (0 2 5 9)
    { 1.555556f, 10.888889f, 1.777778f, 0.072581f }, // 39 (0 2 6 8)
    { 1.666667f, 10.333333f, 2.000000f, 0.075630f }, // 40 (0 2 7 7)
    { 1.777778f, 9.777778f, 2.222222f, 0.080357f }, // 41 (0 2 8 6)
    { 1.888889f, 9.222222f, 2.444444f, 0.087379f }, // 42 (0 2 9 5)
    { 2.000000f, 8.666667f, 2.666667f, 0.097826f }, // 43 (0 2 10 4)
    { 2.111111f, 8.111111f, 2.888889f, 0.113924f }, // 44 (0 2 11 3)
    { 2.222222f, 7.555556f, 3.111111f, 0.140625f }, // 45 (0 2 12 2)
    { 2.333333f, 7.000000f, 3.333333f, 0.191489f }, // 46 (0 2 13 1)
    { 2.444444f, 6.444445f, 3.555556f, 0.321429f }, // 47 (0 2 14 0)
    { 1.333333f, 13.333333f, 0.666667f, 0.057692f }, // 48 (0 3 0 13)
    { 1.444444f, 12.777778f, 0.888889f, 0.056604f }, // 49 (0 3 1 12)
    { 1.555556f, 12.222222f, 1.111111f, 0.056250f }, // 50 (0 3 2 11)
    { 1.666667f, 11.666667f, 1.333333f, 0.056604f }, // 51 (0 3 3 10)
    { 1.777778f, 11.111111f, 1.555556f, 0.057692f }, // 52 (0 3 4 9)
    { 1.888889f, 10.555555f, 1.777778f, 0.059603f }, // 53 (0 3 5 8)
    { 2.000000f, 10.000000f, 2.000000f, 0.062500f }, // 54 (0 3 6 7)
    { 2.111111f, 9.444445f, 2.222222f, 0.066667f }, // 55 (0 3 7 6)
    { 2.222222f, 8.888889f, 2.444444f, 0.072581f }, // 56 (0 3 8 5)
    { 2.333333f, 8.333333f, 2.666667f, 0.081081f }, // 57 (0 3 9 4)
    { 2.444444f, 7.777778f, 2.888889f, 0.093750f }, // 58 (0 3 10 3)
    { 2.555556f, 7.222222f, 3.111111f, 0.113924f }, // 59 (0 3 11 2)
    { 2.666667f, 6.666667f, 3.333333f, 0.150000f }, // 60 (0 3 12 1)
    { 2.777778f, 6.111111f, 3.555556f, 0.230769f }, // 61 (0 3 13 0)
    { 1.777778f, 12.444445f, 0.888889f, 0.046875f }, // 62 (0 4 0 12)
    { 1.888889f, 11.888889f, 1.111111f, 0.047120f }, // 63 (0 4 1 11)
    { 2.000000f, 11.333333f, 1.333333f, 0.047872f }, // 64 (0 4 2 10)
    { 2.111111f, 10.777778f, 1.555556f, 0.049180f }, // 65 (0 4 3 9)
    { 2.222222f, 10.222222f, 1.777778f, 0.051136f }, // 66 (0 4 4 8)
    { 2.333333f, 9.666667f, 2.000000f, 0.053892f }, // 67 (0 4 5 7)
    { 2.444444f, 9.111111f, 2.222222f, 0.057692f }, // 68 (0 4 6 6)
    { 2.555556f, 8.555555f, 2.444444f, 0.062937f }, // 69 (0 4 7 5)
    { 2.666667f, 8.000000f, 2.666667f, 0.070313f }, // 70 (0 4 8 4)
    { 2.777778f, 7.444445f, 2.888889f, 0.081081f }, // 71 (0 4 9 3)
    { 2.888889f, 6.888889f, 3.111111f, 0.097826f }, // 72 (0 4 10 2)
    { 3.000000f, 6.333333f, 3.333333f, 0.126761f }, // 73 (0 4 11 1)
    { 3.111111f, 5.777778f, 3.555556f, 0.187500f }, // 74 (0 4 12 0)
    { 2.222222f, 11.555555f, 1.111111f, 0.040909f }, // 75 (0 5 0 11)
    { 2.333333f, 11.000000f, 1.333333f, 0.041860f }, // 76 (0 5 1 10)
    { 2.444444f, 10.444445f, 1.555556f, 0.043269f }, // 77 (0 5 2 9)
    { 2.555556f, 9.888889f, 1.777778f, 0.045226f }, // 78 (0 5 3 8)
    { 2.666667f, 9.333333f, 2.000000f, 0.047872f }, // 79 (0 5 4 7)
    { 2.777778f, 8.777778f, 2.222222f, 0.051429f }, // 80 (0 5 5 6)
    { 2.888889f, 8.222222f, 2.444444f, 0.056250f }, // 81 (0 5 6 5)
    { 3.000000f, 7.666667f, 2.666667f, 0.062937f }, // 82 (0 5 7 4)
    { 3.111111f, 7.111111f, 2.888889f, 0.072581f }, // 83 (0 5 8 3)
    { 3.222222f, 6.555556f, 3.111111f, 0.087379f }, // 84 (0 5 9 2)
    { 3.333333f, 6.000000f, 3.333333f, 0.112500f }, // 85 (0 5 10 1)
    { 3.444444f, 5.444445f, 3.555556f, 0.163636f }, // 86 (0 5 11 0)
    { 2.666667f, 10.666667f, 1.333333f, 0.037500f }, // 87 (0 6 0 10)
    { 2.777778f, 10.111111f, 1.555556f, 0.038961f }, // 88 (0 6 1 9)
    { 2.888889f, 9.555555f, 1.777778f, 0.040909f }, // 89 (0 6 2 8)
    { 3.000000f, 9.000000f, 2.000000f, 0.043478f }, // 90 (0 6 3 7)
    { 3.111111f, 8.444445f, 2.222222f, 0.046875f }, // 91 (0 6 4 6)
    { 3.222222f, 7.888889f, 2.444444f, 0.051429f }, // 92 (0 6 5 5)
    { 3.333333f, 7.333333f, 2.666667f, 0.057692f }, // 93 (0 6 6 4)
    { 3.444444f, 6.777778f, 2.888889f, 0.066667f }, // 94 (0 6 7 3)
    { 3.555556f, 6.222222f, 3.111111f, 0.080357f }, // 95 (0 6 8 2)
    { 3.666667f, 5.666667f, 3.333333f, 0.103448f }, // 96 (0 6 9 1)
    { 3.777778f, 5.111111f, 3.555556f, 0.150000f }, // 97 (0 6 10 0)
    { 3.111111f, 9.777778f, 1.555556f, 0.035714f }, // 98 (0 7 0 9)
    { 3.222222f, 9.222222f, 1.777778f, 0.037657f }, // 99 (0 7 1 8)
    { 3.333333f, 8.666667f, 2.000000f, 0.040179f }, // 100 (0 7 2 7)
    { 3.444444f, 8.111111f, 2.222222f, 0.043478f }, // 101 (0 7 3 6)
    { 3.555556f, 7.555555f, 2.444444f, 0.047872f }, // 102 (0 7 4 5)
    { 3.666667f, 7.000000f, 2.666667f, 0.053892f }, // 103 (0 7 5 4)
    { 3.777778f, 6.444445f, 2.888889f, 0.062500f }, // 104 (0 7 6 3)
    { 3.888889f, 5.888889f, 3.111111f, 0.075630f }, // 105 (0 7 7 2)
    { 4.000000f, 5.333333f, 3.333333f, 0.097826f }, // 106 (0 7 8 1)
    { 4.111111f, 4.777778f, 3.555556f, 0.142857f }, // 107 (0 7 9 0)
    { 3.555556f, 8.888889f, 1.777778f, 0.035156f }, // 108 (0 8 0 8)
    { 3.666667f, 8.333333f, 2.000000f, 0.037657f }, // 109 (0 8 1 7)
    { 3.777778f, 7.777778f, 2.222222f, 0.040909f }, // 110 (0 8 2 6)
    { 3.888889f, 7.222222f, 2.444444f, 0.045226f }, // 111 (0 8 3 5)
    { 4.000000f, 6.666667f, 2.666667f, 0.051136f }, // 112 (0 8 4 4)
    { 4.111111f, 6.111111f, 2.888889f, 0.059603f }, // 113 (0 8 5 3)
    { 4.222222f, 5.555555f, 3.111111f, 0.072581f }, // 114 (0 8 6 2)
    { 4.333333f, 5.000000f, 3.333333f, 0.094737f }, // 115 (0 8 7 1)
    { 4.444445f, 4.444445f, 3.555556f, 0.140625f }, // 116 (0 8 8 0)
    { 4.000000f, 8.000000f, 2.000000f, 0.035714f }, // 117 (0 9 0 7)
    { 4.111111f, 7.444445f, 2.222222f, 0.038961f }, // 118 (0 9 1 6)
    { 4.222222f, 6.888889f, 2.444444f, 0.043269f }, // 119 (0 9 2 5)
    { 4.333333f, 6.333333f, 2.666667f, 0.049180f }, // 120 (0 9 3 4)
    { 4.444445f, 5.777778f, 2.888889f, 0.057692f }, // 121 (0 9 4 3)
    { 4.555556f, 5.222222f, 3.111111f, 0.070866f }, // 122 (0 9 5 2)
    { 4.666667f, 4.666667f, 3.333333f, 0.093750f }, // 123 (0 9 6 1)
    { 4.777778f, 4.111111f, 3.555556f, 0.142857f }, // 124 (0 9 7 0)
    { 4.444445f, 7.111111f, 2.222222f, 0.037500f }, // 125 (0 10 0 6)
    { 4.555556f, 6.555555f, 2.444444f, 0.041860f }, // 126 (0 10 1 5)
    { 4.666667f, 6.000000f, 2.666667f, 0.047872f }, // 127 (0 10 2 4)
    { 4.777778f, 5.444445f, 2.888889f, 0.056604f }, // 128 (0 10 3 3)
    { 4.888889f, 4.888889f, 3.111111f, 0.070313f }, // 129 (0 10 4 2)
    { 5.000000f, 4.333333f, 3.333333f, 0.094737f }, // 130 (0 10 5 1)
    { 5.111111f, 3.777778f, 3.555556f, 0.150000f }, // 131 (0 10 6 0)
    { 4.888889f, 6.222222f, 2.444444f, 0.040909f }, // 132 (0 11 0 5)
    { 5.000000f, 5.666667f, 2.666667f, 0.047120f }, // 133 (0 11 1 4)
    { 5.111111f, 5.111111f, 2.888889f, 0.056250f }, // 134 (0 11 2 3)
    { 5.222222f, 4.555555f, 3.111111f, 0.070866f }, // 135 (0 11 3 2)
    { 5.333333f, 4.000000f, 3.333333f, 0.097826f }, // 136 (0 11 4 1)
    { 5.444445f, 3.444444f, 3.555556f, 0.163636f }, // 137 (0 11 5 0)
    { 5.333333f, 5.333333f, 2.666667f, 0.046875f }, // 138 (0 12 0 4)
    { 5.444445f, 4.777778f, 2.888889f, 0.056604f }, // 139 (0 12 1 3)
    { 5.555556f, 4.222222f, 3.111111f, 0.072581f }, // 140 (0 12 2 2)
    { 5.666667f, 3.666667f, 3.333333f, 0.103448f }, // 141 (0 12 3 1)
    { 5.777778f, 3.111111f, 3.555556f, 0.187500f }, // 142 (0 12 4 0)
    { 5.777778f, 4.444445f, 2.888889f, 0.057692f }, // 143 (0 13 0 3)
    { 5.888889f, 3.888889f, 3.111111f, 0.075630f }, // 144 (0 13 1 2)
    { 6.000000f, 3.333333f, 3.333333f, 0.112500f }, // 145 (0 13 2 1)
    { 6.111111f, 2.777778f, 3.555556f, 0.230769f }, // 146 (0 13 3 0)
    { 6.222222f, 3.555556f, 3.111111f, 0.080357f }, // 147 (0 14 0 2)
    { 6.333333f, 3.000000f, 3.333333f, 0.126761f }, // 148 (0 14 1 1)
    { 6.444445f, 2.444444f, 3.555556f, 0.321429f }, // 149 (0 14 2 0)
    { 6.666667f, 2.666667f, 3.333333f, 0.150000f }, // 150 (0 15 0 1)
    { 6.777778f, 2.111111f, 3.555556f, 0.600000f }, // 151 (0 15 1 0)
    { 7.111111f, 1.777778f, 3.555556f, FLT_MAX }, // 152 (0 16 0 0)
    { 1.000000f, 15.000000f, 0.000000f, 0.066667f }, // 153 (1 0 0 15)
    { 1.111111f, 14.444445f, 0.222222f, 0.062500f }, // 154 (1 0 1 14)
    { 1.222222f, 13.888889f, 0.444444f, 0.059603f }, // 155 (1 0 2 13)
    { 1.333333f, 13.333333f, 0.666667f, 0.057692f }, // 156 (1 0 3 12)
    { 1.444444f, 12.777778f, 0.888889f, 0.056604f }, // 157 (1 0 4 11)
    { 1.555556f, 12.222222f, 1.111111f, 0.056250f }, // 158 (1 0 5 10)
    { 1.666667f, 11.666667f, 1.333333f, 0.056604f }, // 159 (1 0 6 9)
    { 1.777778f, 11.111111f, 1.555556f, 0.057692f }, // 160 (1 0 7 8)
    { 1.888889f, 10.555555f, 1.777778f, 0.059603f }, // 161 (1 0 8 7)
    { 2.000000f, 10.000000f, 2.000000f, 0.062500f }, // 162 (1 0 9 6)
    { 2.111111f, 9.444445f, 2.222222f, 0.066667f }, // 163 (1 0 10 5)
    { 2.222222f, 8.888889f, 2.444444f, 0.072581f }, // 164 (1 0 11 4)
    { 2.333333f, 8.333333f, 2.666667f, 0.081081f }, // 165 (1 0 12 3)
    { 2.444444f, 7.777778f, 2.888889f, 0.093750f }, // 166 (1 0 13 2)
    { 2.555556f, 7.222222f, 3.111111f, 0.113924f }, // 167 (1 0 14 1)
    { 2.666667f, 6.666667f, 3.333333f, 0.150000f }, // 168 (1 0 15 0)
    { 1.444444f, 14.111111f, 0.222222f, 0.049180f }, // 169 (1 1 0 14)
    { 1.555556f, 13.555555f, 0.444444f, 0.047872f }, // 170 (1 1 1 13)
    { 1.666667f, 13.000000f, 0.666667f, 0.047120f }, // 171 (1 1 2 12)
    { 1.777778f, 12.444445f, 0.888889f, 0.046875f }, // 172 (1 1 3 11)
    { 1.888889f, 11.888889f, 1.111111f, 0.047120f }, // 173 (1 1 4 10)
    { 2.000000f, 11.333333f, 1.333333f, 0.047872f }, // 174 (1 1 5 9)
    { 2.111111f, 10.777778f, 1.555556f, 0.049180f }, // 175 (1 1 6 8)
    { 2.222222f, 10.222222f, 1.777778f, 0.051136f }, // 176 (1 1 7 7)
    { 2.333333f, 9.666667f, 2.000000f, 0.053892f }, // 177 (1 1 8 6)
    { 2.444444f, 9.111111f, 2.222222f, 0.057692f }, // 178 (1 1 9 5)
    { 2.555556f, 8.555555f, 2.444444f, 0.062937f }, // 179 (1 1 10 4)
    { 2.666667f, 8.000000f, 2.666667f, 0.070313f }, // 180 (1 1 11 3)
    { 2.777778f, 7.444445f, 2.888889f, 0.081081f }, // 181 (1 1 12 2)
    { 2.888889f, 6.888889f, 3.111111f, 0.097826f }, // 182 (1 1 13 1)
    { 3.000000f, 6.333333f, 3.333333f, 0.126761f }, // 183 (1 1 14 0)
    { 1.888889f, 13.222222f, 0.444444f, 0.040359f }, // 184 (1 2 0 13)
    { 2.000000f, 12.666667f, 0.666667f, 0.040179f }, // 185 (1 2 1 12)
    { 2.111111f, 12.111111f, 0.888889f, 0.040359f }, // 186 (1 2 2 11)
    { 2.222222f, 11.555555f, 1.111111f, 0.040909f }, // 187 (1 2 3 10)
    { 2.333333f, 11.000000f, 1.333333f, 0.041860f }, // 188 (1 2 4 9)
    { 2.444444f, 10.444445f, 1.555556f, 0.043269f }, // 189 (1 2 5 8)
    { 2.555556f, 9.888889f, 1.777778f, 0.045226f }, // 190 (1 2 6 7)
    { 2.666667f, 9.333333f, 2.000000f, 0.047872f }, // 191 (1 2 7 6)
    { 2.777778f, 8.777778f, 2.222222f, 0.051429f }, // 192 (1 2 8 5)
    { 2.888889f, 8.222222f, 2.444444f, 0.056250f }, // 193 (1 2 9 4)
    { 3.000000f, 7.666667f, 2.666667f, 0.062937f }, // 194 (1 2 10 3)
    { 3.111111f, 7.111111f, 2.888889f, 0.072581f }, // 195 (1 2 11 2)
    { 3.222222f, 6.555556f, 3.111111f, 0.087379f }, // 196 (1 2 12 1)
    { 3.333333f, 6.000000f, 3.333333f, 0.112500f }, // 197 (1 2 13 0)
    { 2.333333f, 12.333333f, 0.666667f, 0.035294f }, // 198 (1 3 0 12)
    { 2.444444f, 11.777778f, 0.888889f, 0.035714f }, // 199 (1 3 1 11)
    { 2.555556f, 11.222222f, 1.111111f, 0.036437f }, // 200 (1 3 2 10)
    { 2.666667f, 10.666667f, 1.333333f, 0.037500f }, // 201 (1 3 3 9)
    { 2.777778f, 10.111111f, 1.555556f, 0.038961f }, // 202 (1 3 4 8)
    { 2.888889f, 9.555555f, 1.777778f, 0.040909f }, // 203 (1 3 5 7)
    { 3.000000f, 9.000000f, 2.000000f, 0.043478f }, // 204 (1 3 6 6)
    { 3.111111f, 8.444445f, 2.222222f, 0.046875f }, // 205 (1 3 7 5)
    { 3.222222f, 7.888889f, 2.444444f, 0.051429f }, // 206 (1 3 8 4)
    { 3.333333f, 7.333333f, 2.666667f, 0.057692f }, // 207 (1 3 9 3)
    { 3.444444f, 6.777778f, 2.888889f, 0.066667f }, // 208 (1 3 10 2)
    { 3.555556f, 6.222222f, 3.111111f, 0.080357f }, // 209 (1 3 11 1)
    { 3.666667f, 5.666667f, 3.333333f, 0.103448f }, // 210 (1 3 12 0)
    { 2.777778f, 11.444445f, 0.888889f, 0.032258f }, // 211 (1 4 0 11)
    { 2.888889f, 10.888889f, 1.111111f, 0.033088f }, // 212 (1 4 1 10)
    { 3.000000f, 10.333333f, 1.333333f, 0.034221f }, // 213 (1 4 2 9)
    { 3.111111f, 9.777778f, 1.555556f, 0.035714f }, // 214 (1 4 3 8)
    { 3.222222f, 9.222222f, 1.777778f, 0.037657f }, // 215 (1 4 4 7)
    { 3.333333f, 8.666667f, 2.000000f, 0.040179f }, // 216 (1 4 5 6)
    { 3.444444f, 8.111111f, 2.222222f, 0.043478f }, // 217 (1 4 6 5)
    { 3.555556f, 7.555555f, 2.444444f, 0.047872f }, // 218 (1 4 7 4)
    { 3.666667f, 7.000000f, 2.666667f, 0.053892f }, // 219 (1 4 8 3)
    { 3.777778f, 6.444445f, 2.888889f, 0.062500f }, // 220 (1 4 9 2)
    { 3.888889f, 5.888889f, 3.111111f, 0.075630f }, // 221 (1 4 10 1)
    { 4.000000f, 5.333333f, 3.333333f, 0.097826f }, // 222 (1 4 11 0)
    { 3.222222f, 10.555555f, 1.111111f, 0.030508f }, // 223 (1 5 0 10)
    { 3.333333f, 10.000000f, 1.333333f, 0.031690f }, // 224 (1 5 1 9)
    { 3.444444f, 9.444445f, 1.555556f, 0.033210f }, // 225 (1 5 2 8)
    { 3.555556f, 8.888889f, 1.777778f, 0.035156f }, // 226 (1 5 3 7)
    { 3.666667f, 8.333333f, 2.000000f, 0.037657f }, // 227 (1 5 4 6)
    { 3.777778f, 7.777778f, 2.222222f, 0.040909f }, // 228 (1 5 5 5)
    { 3.888889f, 7.222222f, 2.444444f, 0.045226f }, // 229 (1 5 6 4)
    { 4.000000f, 6.666667f, 2.666667f, 0.051136f }, // 230 (1 5 7 3)
    { 4.111111f, 6.111111f, 2.888889f, 0.059603f }, // 231 (1 5 8 2)
    { 4.222222f, 5.555556f, 3.111111f, 0.072581f }, // 232 (1 5 9 1)
    { 4.333333f, 5.000000f, 3.333333f, 0.094737f }, // 233 (1 5 10 0)
    { 3.666667f, 9.666667f, 1.333333f, 0.029703f }, // 234 (1 6 0 9)
    { 3.777778f, 9.111111f, 1.555556f, 0.031250f }, // 235 (1 6 1 8)
    { 3.888889f, 8.555555f, 1.777778f, 0.033210f }, // 236 (1 6 2 7)
    { 4.000000f, 8.000000f, 2.000000f, 0.035714f }, // 237 (1 6 3 6)
    { 4.111111f, 7.444445f, 2.222222f, 0.038961f }, // 238 (1 6 4 5)
    { 4.222222f, 6.888889f, 2.444444f, 0.043269f }, // 239 (1 6 5 4)
    { 4.333333f, 6.333333f, 2.666667f, 0.049180f }, // 240 (1 6 6 3)
    { 4.444445f, 5.777778f, 2.888889f, 0.057692f }, // 241 (1 6 7 2)
    { 4.555555f, 5.222222f, 3.111111f, 0.070866f }, // 242 (1 6 8 1)
    { 4.666667f, 4.666667f, 3.333333f, 0.093750f }, // 243 (1 6 9 0)
    { 4.111111f, 8.777778f, 1.555556f, 0.029703f }, // 244 (1 7 0 8)
    { 4.222222f, 8.222222f, 1.777778f, 0.031690f }, // 245 (1 7 1 7)
    { 4.333333f, 7.666667f, 2.000000f, 0.034221f }, // 246 (1 7 2 6)
    { 4.444445f, 7.111111f, 2.222222f, 0.037500f }, // 247 (1 7 3 5)
    { 4.555555f, 6.555555f, 2.444444f, 0.041860f }, // 248 (1 7 4 4)
    { 4.666667f, 6.000000f, 2.666667f, 0.047872f }, // 249 (1 7 5 3)
    { 4.777778f, 5.444445f, 2.888889f, 0.056604f }, // 250 (1 7 6 2)
    { 4.888889f, 4.888889f, 3.111111f, 0.070313f }, // 251 (1 7 7 1)
    { 5.000000f, 4.333333f, 3.333333f, 0.094737f }, // 252 (1 7 8 0)
    { 4.555555f, 7.888889f, 1.777778f, 0.030508f }, // 253 (1 8 0 7)
    { 4.666667f, 7.333333f, 2.000000f, 0.033088f }, // 254 (1 8 1 6)
    { 4.777778f, 6.777778f, 2.222222f, 0.036437f }, // 255 (1 8 2 5)
    { 4.888889f, 6.222222f, 2.444444f, 0.040909f }, // 256 (1 8 3 4)
    { 5.000000f, 5.666667f, 2.666667f, 0.047120f }, // 257 (1 8 4 3)
    { 5.111111f, 5.111111f, 2.888889f, 0.056250f }, // 258 (1 8 5 2)
    { 5.222222f, 4.555555f, 3.111111f, 0.070866f }, // 259 (1 8 6 1)
    { 5.333333f, 4.000000f, 3.333333f, 0.097826f }, // 260 (1 8 7 0)
    { 5.000000f, 7.000000f, 2.000000f, 0.032258f }, // 261 (1 9 0 6)
    { 5.111111f, 6.444445f, 2.222222f, 0.035714f }, // 262 (1 9 1 5)
    { 5.222222f, 5.888889f, 2.444444f, 0.040359f }, // 263 (1 9 2 4)
    { 5.333333f, 5.333333f, 2.666667f, 0.046875f }, // 264 (1 9 3 3)
    { 5.444445f, 4.777778f, 2.888889f, 0.056604f }, // 265 (1 9 4 2)
    { 5.555556f, 4.222222f, 3.111111f, 0.072581f }, // 266 (1 9 5 1)
    { 5.666667f, 3.666667f, 3.333333f, 0.103448f }, // 267 (1 9 6 0)
    { 5.444445f, 6.111111f, 2.222222f, 0.035294f }, // 268 (1 10 0 5)
    { 5.555556f, 5.555555f, 2.444444f, 0.040179f }, // 269 (1 10 1 4)
    { 5.666667f, 5.000000f, 2.666667f, 0.047120f }, // 270 (1 10 2 3)
    { 5.777778f, 4.444445f, 2.888889f, 0.057692f }, // 271 (1 10 3 2)
    { 5.888889f, 3.888889f, 3.111111f, 0.075630f }, // 272 (1 10 4 1)
    { 6.000000f, 3.333333f, 3.333333f, 0.112500f }, // 273 (1 10 5 0)
    { 5.888889f, 5.222222f, 2.444444f, 0.040359f }, // 274 (1 11 0 4)
    { 6.000000f, 4.666667f, 2.666667f, 0.047872f }, // 275 (1 11 1 3)
    { 6.111111f, 4.111111f, 2.888889f, 0.059603f }, // 276 (1 11 2 2)
    { 6.222222f, 3.555556f, 3.111111f, 0.080357f }, // 277 (1 11 3 1)
    { 6.333333f, 3.000000f, 3.333333f, 0.126761f }, // 278 (1 11 4 0)
    { 6.333333f, 4.333333f, 2.666667f, 0.049180f }, // 279 (1 12 0 3)
    { 6.444445f, 3.777778f, 2.888889f, 0.062500f }, // 280 (1 12 1 2)
    { 6.555556f, 3.222222f, 3.111111f, 0.087379f }, // 281 (1 12 2 1)
    { 6.666667f, 2.666667f, 3.333333f, 0.150000f }, // 282 (1 12 3 0)
    { 6.777778f, 3.444444f, 2.888889f, 0.066667f }, // 283 (1 13 0 2)
    { 6.888889f, 2.888889f, 3.111111f, 0.097826f }, // 284 (1 13 1 1)
    { 7.000000f, 2.333333f, 3.333333f, 0.191489f }, // 285 (1 13 2 0)
    { 7.222222f, 2.555556f, 3.111111f, 0.113924f }, // 286 (1 14 0 1)
    { 7.333333f, 2.000000f, 3.333333f, 0.281250f }, // 287 (1 14 1 0)
    { 7.666667f, 1.666667f, 3.333333f, 0.600000f }, // 288 (1 15 0 0)
    { 2.000000f, 14.000000f, 0.000000f, 0.035714f }, // 289 (2 0 0 14)
    { 2.111111f, 13.444445f, 0.222222f, 0.035294f }, // 290 (2 0 1 13)
    { 2.222222f, 12.888889f, 0.444444f, 0.035156f }, // 291 (2 0 2 12)
    { 2.333333f, 12.333333f, 0.666667f, 0.035294f }, // 292 (2 0 3 11)
    { 2.444444f, 11.777778f, 0.888889f, 0.035714f }, // 293 (2 0 4 10)
    { 2.555556f, 11.222222f, 1.111111f, 0.036437f }, // 294 (2 0 5 9)
    { 2.666667f, 10.666667f, 1.333333f, 0.037500f }, // 295 (2 0 6 8)
    { 2.777778f, 10.111111f, 1.555556f, 0.038961f }, // 296 (2 0 7 7)
    { 2.888889f, 9.555555f, 1.777778f, 0.040909f }, // 297 (2 0 8 6)
    { 3.000000f, 9.000000f, 2.000000f, 0.043478f }, // 298 (2 0 9 5)
    { 3.111111f, 8.444445f, 2.222222f, 0.046875f }, // 299 (2 0 10 4)
    { 3.222222f, 7.888889f, 2.444444f, 0.051429f }, // 300 (2 0 11 3)
    { 3.333333f, 7.333333f, 2.666667f, 0.057692f }, // 301 (2 0 12 2)
    { 3.444444f, 6.777778f, 2.888889f, 0.066667f }, // 302 (2 0 13 1)
    { 3.555556f, 6.222222f, 3.111111f, 0.080357f }, // 303 (2 0 14 0)
    { 2.444444f, 13.111111f, 0.222222f, 0.031250f }, // 304 (2 1 0 13)
    { 2.555556f, 12.555555f, 0.444444f, 0.031359f }, // 305 (2 1 1 12)
    { 2.666667f, 12.000000f, 0.666667f, 0.031690f }, // 306 (2 1 2 11)
    { 2.777778f, 11.444445f, 0.888889f, 0.032258f }, // 307 (2 1 3 10)
    { 2.888889f, 10.888889f, 1.111111f, 0.033088f }, // 308 (2 1 4 9)
    { 3.000000f, 10.333333f, 1.333333f, 0.034221f }, // 309 (2 1 5 8)
    { 3.111111f, 9.777778f, 1.555556f, 0.035714f }, // 310 (2 1 6 7)
    { 3.222222f, 9.222222f, 1.777778f, 0.037657f }, // 311 (2 1 7 6)
    { 3.333333f, 8.666667f, 2.000000f, 0.040179f }, // 312 (2 1 8 5)
    { 3.444444f, 8.111111f, 2.222222f, 0.043478f }, // 313 (2 1 9 4)
    { 3.555556f, 7.555556f, 2.444444f, 0.047872f }, // 314 (2 1 10 3)
    { 3.666667f, 7.000000f, 2.666667f, 0.053892f }, // 315 (2 1 11 2)
    { 3.777778f, 6.444445f, 2.888889f, 0.062500f }, // 316 (2 1 12 1)
    { 3.888889f, 5.888889f, 3.111111f, 0.075630f }, // 317 (2 1 13 0)
    { 2.888889f, 12.222222f, 0.444444f, 0.028481f }, // 318 (2 2 0 12)
    { 3.000000f, 11.666667f, 0.666667f, 0.028939f }, // 319 (2 2 1 11)
    { 3.111111f, 11.111111f, 0.888889f, 0.029605f }, // 320 (2 2 2 10)
    { 3.222222f, 10.555555f, 1.111111f, 0.030508f }, // 321 (2 2 3 9)
    { 3.333333f, 10.000000f, 1.333333f, 0.031690f }, // 322 (2 2 4 8)
    { 3.444444f, 9.444445f, 1.555556f, 0.033210f }, // 323 (2 2 5 7)
    { 3.555556f, 8.888889f, 1.777778f, 0.035156f }, // 324 (2 2 6 6)
    { 3.666667f, 8.333333f, 2.000000f, 0.037657f }, // 325 (2 2 7 5)
    { 3.777778f, 7.777778f, 2.222222f, 0.040909f }, // 326 (2 2 8 4)
    { 3.888889f, 7.222222f, 2.444444f, 0.045226f }, // 327 (2 2 9 3)
    { 4.000000f, 6.666667f, 2.666667f, 0.051136f }, // 328 (2 2 10 2)
    { 4.111111f, 6.111111f, 2.888889f, 0.059603f }, // 329 (2 2 11 1)
    { 4.222222f, 5.555556f, 3.111111f, 0.072581f }, // 330 (2 2 12 0)
    { 3.333333f, 11.333333f, 0.666667f, 0.026786f }, // 331 (2 3 0 11)
    { 3.444444f, 10.777778f, 0.888889f, 0.027523f }, // 332 (2 3 1 10)
    { 3.555556f, 10.222222f, 1.111111f, 0.028481f }, // 333 (2 3 2 9)
    { 3.666667f, 9.666667f, 1.333333f, 0.029703f }, // 334 (2 3 3 8)
    { 3.777778f, 9.111111f, 1.555556f, 0.031250f }, // 335 (2 3 4 7)
    { 3.888889f, 8.555555f, 1.777778f, 0.033210f }, // 336 (2 3 5 6)
    { 4.000000f, 8.000000f, 2.000000f, 0.035714f }, // 337 (2 3 6 5)
    { 4.111111f, 7.444445f, 2.222222f, 0.038961f }, // 338 (2 3 7 4)
    { 4.222222f, 6.888889f, 2.444444f, 0.043269f }, // 339 (2 3 8 3)
    { 4.333333f, 6.333333f, 2.666667f, 0.049180f }, // 340 (2 3 9 2)
    { 4.444445f, 5.777778f, 2.888889f, 0.057692f }, // 341 (2 3 10 1)
    { 4.555555f, 5.222222f, 3.111111f, 0.070866f }, // 342 (2 3 11 0)
    { 3.777778f, 10.444445f, 0.888889f, 0.025862f }, // 343 (2 4 0 10)
    { 3.888889f, 9.888889f, 1.111111f, 0.026866f }, // 344 (2 4 1 9)
    { 4.000000f, 9.333333f, 1.333333f, 0.028125f }, // 345 (2 4 2 8)
    { 4.111111f, 8.777778f, 1.555556f, 0.029703f }, // 346 (2 4 3 7)
    { 4.222222f, 8.222222f, 1.777778f, 0.031690f }, // 347 (2 4 4 6)
    { 4.333333f, 7.666667f, 2.000000f, 0.034221f }, // 348 (2 4 5 5)
    { 4.444445f, 7.111111f, 2.222222f, 0.037500f }, // 349 (2 4 6 4)
    { 4.555555f, 6.555555f, 2.444444f, 0.041860f }, // 350 (2 4 7 3)
    { 4.666667f, 6.000000f, 2.666667f, 0.047872f }, // 351 (2 4 8 2)
    { 4.777778f, 5.444445f, 2.888889f, 0.056604f }, // 352 (2 4 9 1)
    { 4.888889f, 4.888889f, 3.111111f, 0.070313f }, // 353 (2 4 10 0)
    { 4.222222f, 9.555555f, 1.111111f, 0.025568f }, // 354 (2 5 0 9)
    { 4.333333f, 9.000000f, 1.333333f, 0.026866f }, // 355 (2 5 1 8)
    { 4.444445f, 8.444445f, 1.555556f, 0.028481f }, // 356 (2 5 2 7)
    { 4.555555f, 7.888889f, 1.777778f, 0.030508f }, // 357 (2 5 3 6)
    { 4.666667f, 7.333333f, 2.000000f, 0.033088f }, // 358 (2 5 4 5)
    { 4.777778f, 6.777778f, 2.222222f, 0.036437f }, // 359 (2 5 5 4)
    { 4.888889f, 6.222222f, 2.444444f, 0.040909f }, // 360 (2 5 6 3)
    { 5.000000f, 5.666667f, 2.666667f, 0.047120f }, // 361 (2 5 7 2)
    { 5.111111f, 5.111111f, 2.888889f, 0.056250f }, // 362 (2 5 8 1)
    { 5.222222f, 4.555556f, 3.111111f, 0.070866f }, // 363 (2 5 9 0)
    { 4.666667f, 8.666667f, 1.333333f, 0.025862f }, // 364 (2 6 0 8)
    { 4.777778f, 8.111111f, 1.555556f, 0.027523f }, // 365 (2 6 1 7)
    { 4.888889f, 7.555555f, 1.777778f, 0.029605f }, // 366 (2 6 2 6)
    { 5.000000f, 7.000000f, 2.000000f, 0.032258f }, // 367 (2 6 3 5)
    { 5.111111f, 6.444445f, 2.222222f, 0.035714f }, // 368 (2 6 4 4)
    { 5.222222f, 5.888889f, 2.444444f, 0.040359f }, // 369 (2 6 5 3)
    { 5.333333f, 5.333333f, 2.666667f, 0.046875f }, // 370 (2 6 6 2)
    { 5.444445f, 4.777778f, 2.888889f, 0.056604f }, // 371 (2 6 7 1)
    { 5.555555f, 4.222222f, 3.111111f, 0.072581f }, // 372 (2 6 8 0)
    { 5.111111f, 7.777778f, 1.555556f, 0.026786f }, // 373 (2 7 0 7)
    { 5.222222f, 7.222222f, 1.777778f, 0.028939f }, // 374 (2 7 1 6)
    { 5.333333f, 6.666667f, 2.000000f, 0.031690f }, // 375 (2 7 2 5)
    { 5.444445f, 6.111111f, 2.222222f, 0.035294f }, // 376 (2 7 3 4)
    { 5.555555f, 5.555555f, 2.444444f, 0.040179f }, // 377 (2 7 4 3)
    { 5.666667f, 5.000000f, 2.666667f, 0.047120f }, // 378 (2 7 5 2)
    { 5.777778f, 4.444445f, 2.888889f, 0.057692f }, // 379 (2 7 6 1)
    { 5.888889f, 3.888889f, 3.111111f, 0.075630f }, // 380 (2 7 7 0)
    { 5.555555f, 6.888889f, 1.777778f, 0.028481f }, // 381 (2 8 0 6)
    { 5.666667f, 6.333333f, 2.000000f, 0.031359f }, // 382 (2 8 1 5)
    { 5.777778f, 5.777778f, 2.222222f, 0.035156f }, // 383 (2 8 2 4)
    { 5.888889f, 5.222222f, 2.444444f, 0.040359f }, // 384 (2 8 3 3)
    { 6.000000f, 4.666667f, 2.666667f, 0.047872f }, // 385 (2 8 4 2)
    { 6.111111f, 4.111111f, 2.888889f, 0.059603f }, // 386 (2 8 5 1)
    { 6.222222f, 3.555556f, 3.111111f, 0.080357f }, // 387 (2 8 6 0)
    { 6.000000f, 6.000000f, 2.000000f, 0.031250f }, // 388 (2 9 0 5)
    { 6.111111f, 5.444445f, 2.222222f, 0.035294f }, // 389 (2 9 1 4)
    { 6.222222f, 4.888889f, 2.444444f, 0.040909f }, // 390 (2 9 2 3)
    { 6.333333f, 4.333333f, 2.666667f, 0.049180f }, // 391 (2 9 3 2)
    { 6.444445f, 3.777778f, 2.888889f, 0.062500f }, // 392 (2 9 4 1)
    { 6.555556f, 3.222222f, 3.111111f, 0.087379f }, // 393 (2 9 5 0)
    { 6.444445f, 5.111111f, 2.222222f, 0.035714f }, // 394 (2 10 0 4)
    { 6.555556f, 4.555555f, 2.444444f, 0.041860f }, // 395 (2 10 1 3)
    { 6.666667f, 4.000000f, 2.666667f, 0.051136f }, // 396 (2 10 2 2)
    { 6.777778f, 3.444444f, 2.888889f, 0.066667f }, // 397 (2 10 3 1)
    { 6.888889f, 2.888889f, 3.111111f, 0.097826f }, // 398 (2 10 4 0)
    { 6.888889f, 4.222222f, 2.444444f, 0.043269f }, // 399 (2 11 0 3)
    { 7.000000f, 3.666667f, 2.666667f, 0.053892f }, // 400 (2 11 1 2)
    { 7.111111f, 3.111111f, 2.888889f, 0.072581f }, // 401 (2 11 2 1)
    { 7.222222f, 2.555556f, 3.111111f, 0.113924f }, // 402 (2 11 3 0)
    { 7.333333f, 3.333333f, 2.666667f, 0.057692f }, // 403 (2 12 0 2)
    { 7.444445f, 2.777778f, 2.888889f, 0.081081f }, // 404 (2 12 1 1)
    { 7.555556f, 2.222222f, 3.111111f, 0.140625f }, // 405 (2 12 2 0)
    { 7.777778f, 2.444444f, 2.888889f, 0.093750f }, // 406 (2 13 0 1)
    { 7.888889f, 1.888889f, 3.111111f, 0.191489f }, // 407 (2 13 1 0)
    { 8.222222f, 1.555556f, 3.111111f, 0.321429f }, // 408 (2 14 0 0)
    { 3.000000f, 13.000000f, 0.000000f, 0.025641f }, // 409 (3 0 0 13)
    { 3.111111f, 12.444445f, 0.222222f, 0.025862f }, // 410 (3 0 1 12)
    { 3.222222f, 11.888889f, 0.444444f, 0.026239f }, // 411 (3 0 2 11)
    { 3.333333f, 11.333333f, 0.666667f, 0.026786f }, // 412 (3 0 3 10)
    { 3.444444f, 10.777778f, 0.888889f, 0.027523f }, // 413 (3 0 4 9)
    { 3.555556f, 10.222222f, 1.111111f, 0.028481f }, // 414 (3 0 5 8)
    { 3.666667f, 9.666667f, 1.333333f, 0.029703f }, // 415 (3 0 6 7)
    { 3.777778f, 9.111111f, 1.555556f, 0.031250f }, // 416 (3 0 7 6)
    { 3.888889f, 8.555555f, 1.777778f, 0.033210f }, // 417 (3 0 8 5)
    { 4.000000f, 8.000000f, 2.000000f, 0.035714f }, // 418 (3 0 9 4)
    { 4.111111f, 7.444445f, 2.222222f, 0.038961f }, // 419 (3 0 10 3)
    { 4.222222f, 6.888889f, 2.444444f, 0.043269f }, // 420 (3 0 11 2)
    { 4.333333f, 6.333333f, 2.666667f, 0.049180f }, // 421 (3 0 12 1)
    { 4.444445f, 5.777778f, 2.888889f, 0.057692f }, // 422 (3 0 13 0)
    { 3.444444f, 12.111111f, 0.222222f, 0.024000f }, // 423 (3 1 0 12)
    { 3.555556f, 11.555555f, 0.444444f, 0.024457f }, // 424 (3 1 1 11)
    { 3.666667f, 11.000000f, 0.666667f, 0.025070f }, // 425 (3 1 2 10)
    { 3.777778f, 10.444445f, 0.888889f, 0.025862f }, // 426 (3 1 3 9)
    { 3.888889f, 9.888889f, 1.111111f, 0.026866f }, // 427 (3 1 4 8)
    { 4.000000f, 9.333333f, 1.333333f, 0.028125f }, // 428 (3 1 5 7)
    { 4.111111f, 8.777778f, 1.555556f, 0.029703f }, // 429 (3 1 6 6)
    { 4.222222f, 8.222222f, 1.777778f, 0.031690f }, // 430 (3 1 7 5)
    { 4.333333f, 7.666667f, 2.000000f, 0.034221f }, // 431 (3 1 8 4)
    { 4.444445f, 7.111111f, 2.222222f, 0.037500f }, // 432 (3 1 9 3)
    { 4.555555f, 6.555556f, 2.444444f, 0.041860f }, // 433 (3 1 10 2)
    { 4.666667f, 6.000000f, 2.666667f, 0.047872f }, // 434 (3 1 11 1)
    { 4.777778f, 5.444445f, 2.888889f, 0.056604f }, // 435 (3 1 12 0)
    { 3.888889f, 11.222222f, 0.444444f, 0.023018f }, // 436 (3 2 0 11)
    { 4.000000f, 10.666667f, 0.666667f, 0.023684f }, // 437 (3 2 1 10)
    { 4.111111f, 10.111111f, 0.888889f, 0.024523f }, // 438 (3 2 2 9)
    { 4.222222f, 9.555555f, 1.111111f, 0.025568f }, // 439 (3 2 3 8)
    { 4.333333f, 9.000000f, 1.333333f, 0.026866f }, // 440 (3 2 4 7)
    { 4.444445f, 8.444445f, 1.555556f, 0.028481f }, // 441 (3 2 5 6)
    { 4.555555f, 7.888889f, 1.777778f, 0.030508f }, // 442 (3 2 6 5)
    { 4.666667f, 7.333333f, 2.000000f, 0.033088f }, // 443 (3 2 7 4)
    { 4.777778f, 6.777778f, 2.222222f, 0.036437f }, // 444 (3 2 8 3)
    { 4.888889f, 6.222222f, 2.444444f, 0.040909f }, // 445 (3 2 9 2)
    { 5.000000f, 5.666667f, 2.666667f, 0.047120f }, // 446 (3 2 10 1)
    { 5.111111f, 5.111111f, 2.888889f, 0.056250f }, // 447 (3 2 11 0)
    { 4.333333f, 10.333333f, 0.666667f, 0.022556f }, // 448 (3 3 0 10)
    { 4.444445f, 9.777778f, 0.888889f, 0.023438f }, // 449 (3 3 1 9)
    { 4.555555f, 9.222222f, 1.111111f, 0.024523f }, // 450 (3 3 2 8)
    { 4.666667f, 8.666667f, 1.333333f, 0.025862f }, // 451 (3 3 3 7)
    { 4.777778f, 8.111111f, 1.555556f, 0.027523f }, // 452 (3 3 4 6)
    { 4.888889f, 7.555555f, 1.777778f, 0.029605f }, // 453 (3 3 5 5)
    { 5.000000f, 7.000000f, 2.000000f, 0.032258f }, // 454 (3 3 6 4)
    { 5.111111f, 6.444445f, 2.222222f, 0.035714f }, // 455 (3 3 7 3)
    { 5.222222f, 5.888889f, 2.444444f, 0.040359f }, // 456 (3 3 8 2)
    { 5.333333f, 5.333333f, 2.666667f, 0.046875f }, // 457 (3 3 9 1)
    { 5.444445f, 4.777778f, 2.888889f, 0.056604f }, // 458 (3 3 10 0)
    { 4.777778f, 9.444445f, 0.888889f, 0.022556f }, // 459 (3 4 0 9)
    { 4.888889f, 8.888889f, 1.111111f, 0.023684f }, // 460 (3 4 1 8)
    { 5.000000f, 8.333333f, 1.333333f, 0.025070f }, // 461 (3 4 2 7)
    { 5.111111f, 7.777778f, 1.555556f, 0.026786f }, // 462 (3 4 3 6)
    { 5.222222f, 7.222222f, 1.777778f, 0.028939f }, // 463 (3 4 4 5)
    { 5.333333f, 6.666667f, 2.000000f, 0.031690f }, // 464 (3 4 5 4)
    { 5.444445f, 6.111111f, 2.222222f, 0.035294f }, // 465 (3 4 6 3)
    { 5.555555f, 5.555555f, 2.444444f, 0.040179f }, // 466 (3 4 7 2)
    { 5.666667f, 5.000000f, 2.666667f, 0.047120f }, // 467 (3 4 8 1)
    { 5.777778f, 4.444445f, 2.888889f, 0.057692f }, // 468 (3 4 9 0)
    { 5.222222f, 8.555555f, 1.111111f, 0.023018f }, // 469 (3 5 0 8)
    { 5.333333f, 8.000000f, 1.333333f, 0.024457f }, // 470 (3 5 1 7)
    { 5.444445f, 7.444445f, 1.555556f, 0.026239f }, // 471 (3 5 2 6)
    { 5.555555f, 6.888889f, 1.777778f, 0.028481f }, // 472 (3 5 3 5)
    { 5.666667f, 6.333333f, 2.000000f, 0.031359f }, // 473 (3 5 4 4)
    { 5.777778f, 5.777778f, 2.222222f, 0.035156f }, // 474 (3 5 5 3)
    { 5.888889f, 5.222222f, 2.444444f, 0.040359f }, // 475 (3 5 6 2)
    { 6.000000f, 4.666667f, 2.666667f, 0.047872f }, // 476 (3 5 7 1)
    { 6.111111f, 4.111111f, 2.888889f, 0.059603f }, // 477 (3 5 8 0)
    { 5.666667f, 7.666667f, 1.333333f, 0.024000f }, // 478 (3 6 0 7)
    { 5.777778f, 7.111111f, 1.555556f, 0.025862f }, // 479 (3 6 1 6)
    { 5.888889f, 6.555555f, 1.777778f, 0.028213f }, // 480 (3 6 2 5)
    { 6.000000f, 6.000000f, 2.000000f, 0.031250f }, // 481 (3 6 3 4)
    { 6.111111f, 5.444445f, 2.222222f, 0.035294f }, // 482 (3 6 4 3)
    { 6.222222f, 4.888889f, 2.444444f, 0.040909f }, // 483 (3 6 5 2)
    { 6.333333f, 4.333333f, 2.666667f, 0.049180f }, // 484 (3 6 6 1)
    { 6.444445f, 3.777778f, 2.888889f, 0.062500f }, // 485 (3 6 7 0)
    { 6.111111f, 6.777778f, 1.555556f, 0.025641f }, // 486 (3 7 0 6)
    { 6.222222f, 6.222222f, 1.777778f, 0.028125f }, // 487 (3 7 1 5)
    { 6.333333f, 5.666667f, 2.000000f, 0.031359f }, // 488 (3 7 2 4)
    { 6.444445f, 5.111111f, 2.222222f, 0.035714f }, // 489 (3 7 3 3)
    { 6.555555f, 4.555555f, 2.444444f, 0.041860f }, // 490 (3 7 4 2)
    { 6.666667f, 4.000000f, 2.666667f, 0.051136f }, // 491 (3 7 5 1)
    { 6.777778f, 3.444444f, 2.888889f, 0.066667f }, // 492 (3 7 6 0)
    { 6.555555f, 5.888889f, 1.777778f, 0.028213f }, // 493 (3 8 0 5)
    { 6.666667f, 5.333333f, 2.000000f, 0.031690f }, // 494 (3 8 1 4)
    { 6.777778f, 4.777778f, 2.222222f, 0.036437f }, // 495 (3 8 2 3)
    { 6.888889f, 4.222222f, 2.444444f, 0.043269f }, // 496 (3 8 3 2)
    { 7.000000f, 3.666667f, 2.666667f, 0.053892f }, // 497 (3 8 4 1)
    { 7.111111f, 3.111111f, 2.888889f, 0.072581f }, // 498 (3 8 5 0)
    { 7.000000f, 5.000000f, 2.000000f, 0.032258f }, // 499 (3 9 0 4)
    { 7.111111f, 4.444445f, 2.222222f, 0.037500f }, // 500 (3 9 1 3)
    { 7.222222f, 3.888889f, 2.444444f, 0.045226f }, // 501 (3 9 2 2)
    { 7.333333f, 3.333333f, 2.666667f, 0.057692f }, // 502 (3 9 3 1)
    { 7.444445f, 2.777778f, 2.888889f, 0.081081f }, // 503 (3 9 4 0)
    { 7.444445f, 4.111111f, 2.222222f, 0.038961f }, // 504 (3 10 0 3)
    { 7.555556f, 3.555556f, 2.444444f, 0.047872f }, // 505 (3 10 1 2)
    { 7.666667f, 3.000000f, 2.666667f, 0.062937f }, // 506 (3 10 2 1)
    { 7.777778f, 2.444444f, 2.888889f, 0.093750f }, // 507 (3 10 3 0)
    { 7.888889f, 3.222222f, 2.444444f, 0.051429f }, // 508 (3 11 0 2)
    { 8.000000f, 2.666667f, 2.666667f, 0.070313f }, // 509 (3 11 1 1)
    { 8.111111f, 2.111111f, 2.888889f, 0.113924f }, // 510 (3 11 2 0)
    { 8.333333f, 2.333333f, 2.666667f, 0.081081f }, // 511 (3 12 0 1)
    { 8.444445f, 1.777778f, 2.888889f, 0.150000f }, // 512 (3 12 1 0)
    { 8.777778f, 1.444444f, 2.888889f, 0.230769f }, // 513 (3 13 0 0)
    { 4.000000f, 12.000000f, 0.000000f, 0.020833f }, // 514 (4 0 0 12)
    { 4.111111f, 11.444445f, 0.222222f, 0.021277f }, // 515 (4 0 1 11)
    { 4.222222f, 10.888889f, 0.444444f, 0.021845f }, // 516 (4 0 2 10)
    { 4.333333f, 10.333333f, 0.666667f, 0.022556f }, // 517 (4 0 3 9)
    { 4.444445f, 9.777778f, 0.888889f, 0.023438f }, // 518 (4 0 4 8)
    { 4.555555f, 9.222222f, 1.111111f, 0.024523f }, // 519 (4 0 5 7)
    { 4.666667f, 8.666667f, 1.333333f, 0.025862f }, // 520 (4 0 6 6)
    { 4.777778f, 8.111111f, 1.555556f, 0.027523f }, // 521 (4 0 7 5)
    { 4.888889f, 7.555555f, 1.777778f, 0.029605f }, // 522 (4 0 8 4)
    { 5.000000f, 7.000000f, 2.000000f, 0.032258f }, // 523 (4 0 9 3)
    { 5.111111f, 6.444445f, 2.222222f, 0.035714f }, // 524 (4 0 10 2)
    { 5.222222f, 5.888889f, 2.444444f, 0.040359f }, // 525 (4 0 11 1)
    { 5.333333f, 5.333333f, 2.666667f, 0.046875f }, // 526 (4 0 12 0)
    { 4.444445f, 11.111111f, 0.222222f, 0.020270f }, // 527 (4 1 0 11)
    { 4.555555f, 10.555555f, 0.444444f, 0.020882f }, // 528 (4 1 1 10)
    { 4.666667f, 10.000000f, 0.666667f, 0.021635f }, // 529 (4 1 2 9)
    { 4.777778f, 9.444445f, 0.888889f, 0.022556f }, // 530 (4 1 3 8)
    { 4.888889f, 8.888889f, 1.111111f, 0.023684f }, // 531 (4 1 4 7)
    { 5.000000f, 8.333333f, 1.333333f, 0.025070f }, // 532 (4 1 5 6)
    { 5.111111f, 7.777778f, 1.555556f, 0.026786f }, // 533 (4 1 6 5)
    { 5.222222f, 7.222222f, 1.777778f, 0.028939f }, // 534 (4 1 7 4)
    { 5.333333f, 6.666667f, 2.000000f, 0.031690f }, // 535 (4 1 8 3)
    { 5.444445f, 6.111111f, 2.222222f, 0.035294f }, // 536 (4 1 9 2)
    { 5.555555f, 5.555556f, 2.444444f, 0.040179f }, // 537 (4 1 10 1)
    { 5.666667f, 5.000000f, 2.666667f, 0.047120f }, // 538 (4 1 11 0)
    { 4.888889f, 10.222222f, 0.444444f, 0.020089f }, // 539 (4 2 0 10)
    { 5.000000f, 9.666667f, 0.666667f, 0.020882f }, // 540 (4 2 1 9)
    { 5.111111f, 9.111111f, 0.888889f, 0.021845f }, // 541 (4 2 2 8)
    { 5.222222f, 8.555555f, 1.111111f, 0.023018f }, // 542 (4 2 3 7)
    { 5.333333f, 8.000000f, 1.333333f, 0.024457f }, // 543 (4 2 4 6)
    { 5.444445f, 7.444445f, 1.555556f, 0.026239f }, // 544 (4 2 5 5)
    { 5.555555f, 6.888889f, 1.777778f, 0.028481f }, // 545 (4 2 6 4)
    { 5.666667f, 6.333333f, 2.000000f, 0.031359f }, // 546 (4 2 7 3)
    { 5.777778f, 5.777778f, 2.222222f, 0.035156f }, // 547 (4 2 8 2)
    { 5.888889f, 5.222222f, 2.444444f, 0.040359f }, // 548 (4 2 9 1)
    { 6.000000f, 4.666667f, 2.666667f, 0.047872f }, // 549 (4 2 10 0)
    { 5.333333f, 9.333333f, 0.666667f, 0.020270f }, // 550 (4 3 0 9)
    { 5.444445f, 8.777778f, 0.888889f, 0.021277f }, // 551 (4 3 1 8)
    { 5.555555f, 8.222222f, 1.111111f, 0.022500f }, // 552 (4 3 2 7)
    { 5.666667f, 7.666667f, 1.333333f, 0.024000f }, // 553 (4 3 3 6)
    { 5.777778f, 7.111111f, 1.555556f, 0.025862f }, // 554 (4 3 4 5)
    { 5.888889f, 6.555555f, 1.777778f, 0.028213f }, // 555 (4 3 5 4)
    { 6.000000f, 6.000000f, 2.000000f, 0.031250f }, // 556 (4 3 6 3)
    { 6.111111f, 5.444445f, 2.222222f, 0.035294f }, // 557 (4 3 7 2)
    { 6.222222f, 4.888889f, 2.444444f, 0.040909f }, // 558 (4 3 8 1)
    { 6.333333f, 4.333333f, 2.666667f, 0.049180f }, // 559 (4 3 9 0)
    { 5.777778f, 8.444445f, 0.888889f, 0.020833f }, // 560 (4 4 0 8)
    { 5.888889f, 7.888889f, 1.111111f, 0.022113f }, // 561 (4 4 1 7)
    { 6.000000f, 7.333333f, 1.333333f, 0.023684f }, // 562 (4 4 2 6)
    { 6.111111f, 6.777778f, 1.555556f, 0.025641f }, // 563 (4 4 3 5)
    { 6.222222f, 6.222222f, 1.777778f, 0.028125f }, // 564 (4 4 4 4)
    { 6.333333f, 5.666667f, 2.000000f, 0.031359f }, // 565 (4 4 5 3)
    { 6.444445f, 5.111111f, 2.222222f, 0.035714f }, // 566 (4 4 6 2)
    { 6.555555f, 4.555555f, 2.444444f, 0.041860f }, // 567 (4 4 7 1)
    { 6.666667f, 4.000000f, 2.666667f, 0.051136f }, // 568 (4 4 8 0)
    { 6.222222f, 7.555555f, 1.111111f, 0.021845f }, // 569 (4 5 0 7)
    { 6.333333f, 7.000000f, 1.333333f, 0.023499f }, // 570 (4 5 1 6)
    { 6.444445f, 6.444445f, 1.555556f, 0.025568f }, // 571 (4 5 2 5)
    { 6.555555f, 5.888889f, 1.777778f, 0.028213f }, // 572 (4 5 3 4)
    { 6.666667f, 5.333333f, 2.000000f, 0.031690f }, // 573 (4 5 4 3)
    { 6.777778f, 4.777778f, 2.222222f, 0.036437f }, // 574 (4 5 5 2)
    { 6.888889f, 4.222222f, 2.444444f, 0.043269f }, // 575 (4 5 6 1)
    { 7.000000f, 3.666667f, 2.666667f, 0.053892f }, // 576 (4 5 7 0)
    { 6.666667f, 6.666667f, 1.333333f, 0.023438f }, // 577 (4 6 0 6)
    { 6.777778f, 6.111111f, 1.555556f, 0.025641f }, // 578 (4 6 1 5)
    { 6.888889f, 5.555555f, 1.777778f, 0.028481f }, // 579 (4 6 2 4)
    { 7.000000f, 5.000000f, 2.000000f, 0.032258f }, // 580 (4 6 3 3)
    { 7.111111f, 4.444445f, 2.222222f, 0.037500f }, // 581 (4 6 4 2)
    { 7.222222f, 3.888889f, 2.444444f, 0.045226f }, // 582 (4 6 5 1)
    { 7.333333f, 3.333333f, 2.666667f, 0.057692f }, // 583 (4 6 6 0)
    { 7.111111f, 5.777778f, 1.555556f, 0.025862f }, // 584 (4 7 0 5)
    { 7.222222f, 5.222222f, 1.777778f, 0.028939f }, // 585 (4 7 1 4)
    { 7.333333f, 4.666667f, 2.000000f, 0.033088f }, // 586 (4 7 2 3)
    { 7.444445f, 4.111111f, 2.222222f, 0.038961f }, // 587 (4 7 3 2)
    { 7.555555f, 3.555556f, 2.444444f, 0.047872f }, // 588 (4 7 4 1)
    { 7.666667f, 3.000000f, 2.666667f, 0.062937f }, // 589 (4 7 5 0)
    { 7.555555f, 4.888889f, 1.777778f, 0.029605f }, // 590 (4 8 0 4)
    { 7.666667f, 4.333333f, 2.000000f, 0.034221f }, // 591 (4 8 1 3)
    { 7.777778f, 3.777778f, 2.222222f, 0.040909f }, // 592 (4 8 2 2)
    { 7.888889f, 3.222222f, 2.444444f, 0.051429f }, // 593 (4 8 3 1)
    { 8.000000f, 2.666667f, 2.666667f, 0.070313f }, // 594 (4 8 4 0)
    { 8.000000f, 4.000000f, 2.000000f, 0.035714f }, // 595 (4 9 0 3)
    { 8.111111f, 3.444444f, 2.222222f, 0.043478f }, // 596 (4 9 1 2)
    { 8.222222f, 2.888889f, 2.444444f, 0.056250f }, // 597 (4 9 2 1)
    { 8.333333f, 2.333333f, 2.666667f, 0.081081f }, // 598 (4 9 3 0)
    { 8.444445f, 3.111111f, 2.222222f, 0.046875f }, // 599 (4 10 0 2)
    { 8.555555f, 2.555556f, 2.444444f, 0.062937f }, // 600 (4 10 1 1)
    { 8.666667f, 2.000000f, 2.666667f, 0.097826f }, // 601 (4 10 2 0)
    { 8.888889f, 2.222222f, 2.444444f, 0.072581f }, // 602 (4 11 0 1)
    { 9.000000f, 1.666667f, 2.666667f, 0.126761f }, // 603 (4 11 1 0)
    { 9.333333f, 1.333333f, 2.666667f, 0.187500f }, // 604 (4 12 0 0)
    { 5.000000f, 11.000000f, 0.000000f, 0.018182f }, // 605 (5 0 0 11)
    { 5.111111f, 10.444445f, 0.222222f, 0.018750f }, // 606 (5 0 1 10)
    { 5.222222f, 9.888889f, 0.444444f, 0.019438f }, // 607 (5 0 2 9)
    { 5.333333f, 9.333333f, 0.666667f, 0.020270f }, // 608 (5 0 3 8)
    { 5.444445f, 8.777778f, 0.888889f, 0.021277f }, // 609 (5 0 4 7)
    { 5.555555f, 8.222222f, 1.111111f, 0.022500f }, // 610 (5 0 5 6)
    { 5.666667f, 7.666667f, 1.333333f, 0.024000f }, // 611 (5 0 6 5)
    { 5.777778f, 7.111111f, 1.555556f, 0.025862f }, // 612 (5 0 7 4)
    { 5.888889f, 6.555555f, 1.777778f, 0.028213f }, // 613 (5 0 8 3)
    { 6.000000f, 6.000000f, 2.000000f, 0.031250f }, // 614 (5 0 9 2)
    { 6.111111f, 5.444445f, 2.222222f, 0.035294f }, // 615 (5 0 10 1)
    { 6.222222f, 4.888889f, 2.444444f, 0.040909f }, // 616 (5 0 11 0)
    { 5.444445f, 10.111111f, 0.222222f, 0.018182f }, // 617 (5 1 0 10)
    { 5.555555f, 9.555555f, 0.444444f, 0.018908f }, // 618 (5 1 1 9)
    { 5.666667f, 9.000000f, 0.666667f, 0.019780f }, // 619 (5 1 2 8)
    { 5.777778f, 8.444445f, 0.888889f, 0.020833f }, // 620 (5 1 3 7)
    { 5.888889f, 7.888889f, 1.111111f, 0.022113f }, // 621 (5 1 4 6)
    { 6.000000f, 7.333333f, 1.333333f, 0.023684f }, // 622 (5 1 5 5)
    { 6.111111f, 6.777778f, 1.555556f, 0.025641f }, // 623 (5 1 6 4)
    { 6.222222f, 6.222222f, 1.777778f, 0.028125f }, // 624 (5 1 7 3)
    { 6.333333f, 5.666667f, 2.000000f, 0.031359f }, // 625 (5 1 8 2)
    { 6.444445f, 5.111111f, 2.222222f, 0.035714f }, // 626 (5 1 9 1)
    { 6.555555f, 4.555556f, 2.444444f, 0.041860f }, // 627 (5 1 10 0)
    { 5.888889f, 9.222222f, 0.444444f, 0.018480f }, // 628 (5 2 0 9)
    { 6.000000f, 8.666667f, 0.666667f, 0.019397f }, // 629 (5 2 1 8)
    { 6.111111f, 8.111111f, 0.888889f, 0.020501f }, // 630 (5 2 2 7)
    { 6.222222f, 7.555555f, 1.111111f, 0.021845f }, // 631 (5 2 3 6)
    { 6.333333f, 7.000000f, 1.333333f, 0.023499f }, // 632 (5 2 4 5)
    { 6.444445f, 6.444445f, 1.555556f, 0.025568f }, // 633 (5 2 5 4)
    { 6.555555f, 5.888889f, 1.777778f, 0.028213f }, // 634 (5 2 6 3)
    { 6.666667f, 5.333333f, 2.000000f, 0.031690f }, // 635 (5 2 7 2)
    { 6.777778f, 4.777778f, 2.222222f, 0.036437f }, // 636 (5 2 8 1)
    { 6.888889f, 4.222222f, 2.444444f, 0.043269f }, // 637 (5 2 9 0)
    { 6.333333f, 8.333333f, 0.666667f, 0.019108f }, // 638 (5 3 0 8)
    { 6.444445f, 7.777778f, 0.888889f, 0.020270f }, // 639 (5 3 1 7)
    { 6.555555f, 7.222222f, 1.111111f, 0.021687f }, // 640 (5 3 2 6)
    { 6.666667f, 6.666667f, 1.333333f, 0.023438f }, // 641 (5 3 3 5)
    { 6.777778f, 6.111111f, 1.555556f, 0.025641f }, // 642 (5 3 4 4)
    { 6.888889f, 5.555555f, 1.777778f, 0.028481f }, // 643 (5 3 5 3)
    { 7.000000f, 5.000000f, 2.000000f, 0.032258f }, // 644 (5 3 6 2)
    { 7.111111f, 4.444445f, 2.222222f, 0.037500f }, // 645 (5 3 7 1)
    { 7.222222f, 3.888889f, 2.444444f, 0.045226f }, // 646 (5 3 8 0)
    { 6.777778f, 7.444445f, 0.888889f, 0.020134f }, // 647 (5 4 0 7)
    { 6.888889f, 6.888889f, 1.111111f, 0.021635f }, // 648 (5 4 1 6)
    { 7.000000f, 6.333333f, 1.333333f, 0.023499f }, // 649 (5 4 2 5)
    { 7.111111f, 5.777778f, 1.555556f, 0.025862f }, // 650 (5 4 3 4)
    { 7.222222f, 5.222222f, 1.777778f, 0.028939f }, // 651 (5 4 4 3)
    { 7.333333f, 4.666667f, 2.000000f, 0.033088f }, // 652 (5 4 5 2)
    { 7.444445f, 4.111111f, 2.222222f, 0.038961f }, // 653 (5 4 6 1)
    { 7.555555f, 3.555556f, 2.444444f, 0.047872f }, // 654 (5 4 7 0)
    { 7.222222f, 6.555555f, 1.111111f, 0.021687f }, // 655 (5 5 0 6)
    { 7.333333f, 6.000000f, 1.333333f, 0.023684f }, // 656 (5 5 1 5)
    { 7.444445f, 5.444445f, 1.555556f, 0.026239f }, // 657 (5 5 2 4)
    { 7.555555f, 4.888889f, 1.777778f, 0.029605f }, // 658 (5 5 3 3)
    { 7.666667f, 4.333333f, 2.000000f, 0.034221f }, // 659 (5 5 4 2)
    { 7.777778f, 3.777778f, 2.222222f, 0.040909f }, // 660 (5 5 5 1)
    { 7.888889f, 3.222222f, 2.444444f, 0.051429f }, // 661 (5 5 6 0)
    { 7.666667f, 5.666667f, 1.333333f, 0.024000f }, // 662 (5 6 0 5)
    { 7.777778f, 5.111111f, 1.555556f, 0.026786f }, // 663 (5 6 1 4)
    { 7.888889f, 4.555555f, 1.777778f, 0.030508f }, // 664 (5 6 2 3)
    { 8.000000f, 4.000000f, 2.000000f, 0.035714f }, // 665 (5 6 3 2)
    { 8.111111f, 3.444444f, 2.222222f, 0.043478f }, // 666 (5 6 4 1)
    { 8.222222f, 2.888889f, 2.444444f, 0.056250f }, // 667 (5 6 5 0)
    { 8.111111f, 4.777778f, 1.555556f, 0.027523f }, // 668 (5 7 0 4)
    { 8.222222f, 4.222222f, 1.777778f, 0.031690f }, // 669 (5 7 1 3)
    { 8.333333f, 3.666667f, 2.000000f, 0.037657f }, // 670 (5 7 2 2)
    { 8.444445f, 3.111111f, 2.222222f, 0.046875f }, // 671 (5 7 3 1)
    { 8.555555f, 2.555556f, 2.444444f, 0.062937f }, // 672 (5 7 4 0)
    { 8.555555f, 3.888889f, 1.777778f, 0.033210f }, // 673 (5 8 0 3)
    { 8.666667f, 3.333333f, 2.000000f, 0.040179f }, // 674 (5 8 1 2)
    { 8.777778f, 2.777778f, 2.222222f, 0.051429f }, // 675 (5 8 2 1)
    { 8.888889f, 2.222222f, 2.444444f, 0.072581f }, // 676 (5 8 3 0)
    { 9.000000f, 3.000000f, 2.000000f, 0.043478f }, // 677 (5 9 0 2)
    { 9.111111f, 2.444444f, 2.222222f, 0.057692f }, // 678 (5 9 1 1)
    { 9.222222f, 1.888889f, 2.444444f, 0.087379f }, // 679 (5 9 2 0)
    { 9.444445f, 2.111111f, 2.222222f, 0.066667f }, // 680 (5 10 0 1)
    { 9.555555f, 1.555556f, 2.444444f, 0.112500f }, // 681 (5 10 1 0)
    { 9.888889f, 1.222222f, 2.444444f, 0.163636f }, // 682 (5 11 0 0)
    { 6.000000f, 10.000000f, 0.000000f, 0.016667f }, // 683 (6 0 0 10)
    { 6.111111f, 9.444445f, 0.222222f, 0.017341f }, // 684 (6 0 1 9)
    { 6.222222f, 8.888889f, 0.444444f, 0.018145f }, // 685 (6 0 2 8)
    { 6.333333f, 8.333333f, 0.666667f, 0.019108f }, // 686 (6 0 3 7)
    { 6.444445f, 7.777778f, 0.888889f, 0.020270f }, // 687 (6 0 4 6)
    { 6.555555f, 7.222222f, 1.111111f, 0.021687f }, // 688 (6 0 5 5)
    { 6.666667f, 6.666667f, 1.333333f, 0.023438f }, // 689 (6 0 6 4)
    { 6.777778f, 6.111111f, 1.555556f, 0.025641f }, // 690 (6 0 7 3)
    { 6.888889f, 5.555555f, 1.777778f, 0.028481f }, // 691 (6 0 8 2)
    { 7.000000f, 5.000000f, 2.000000f, 0.032258f }, // 692 (6 0 9 1)
    { 7.111111f, 4.444445f, 2.222222f, 0.037500f }, // 693 (6 0 10 0)
    { 6.444445f, 9.111111f, 0.222222f, 0.017045f }, // 694 (6 1 0 9)
    { 6.555555f, 8.555555f, 0.444444f, 0.017893f }, // 695 (6 1 1 8)
    { 6.666667f, 8.000000f, 0.666667f, 0.018908f }, // 696 (6 1 2 7)
    { 6.777778f, 7.444445f, 0.888889f, 0.020134f }, // 697 (6 1 3 6)
    { 6.888889f, 6.888889f, 1.111111f, 0.021635f }, // 698 (6 1 4 5)
    { 7.000000f, 6.333333f, 1.333333f, 0.023499f }, // 699 (6 1 5 4)
    { 7.111111f, 5.777778f, 1.555556f, 0.025862f }, // 700 (6 1 6 3)
    { 7.222222f, 5.222222f, 1.777778f, 0.028939f }, // 701 (6 1 7 2)
    { 7.333333f, 4.666667f, 2.000000f, 0.033088f }, // 702 (6 1 8 1)
    { 7.444445f, 4.111111f, 2.222222f, 0.038961f }, // 703 (6 1 9 0)
    { 6.888889f, 8.222222f, 0.444444f, 0.017717f }, // 704 (6 2 0 8)
    { 7.000000f, 7.666667f, 0.666667f, 0.018789f }, // 705 (6 2 1 7)
    { 7.111111f, 7.111111f, 0.888889f, 0.020089f }, // 706 (6 2 2 6)
    { 7.222222f, 6.555555f, 1.111111f, 0.021687f }, // 707 (6 2 3 5)
    { 7.333333f, 6.000000f, 1.333333f, 0.023684f }, // 708 (6 2 4 4)
    { 7.444445f, 5.444445f, 1.555556f, 0.026239f }, // 709 (6 2 5 3)
    { 7.555555f, 4.888889f, 1.777778f, 0.029605f }, // 710 (6 2 6 2)
    { 7.666667f, 4.333333f, 2.000000f, 0.034221f }, // 711 (6 2 7 1)
    { 7.777778f, 3.777778f, 2.222222f, 0.040909f }, // 712 (6 2 8 0)
    { 7.333333f, 7.333333f, 0.666667f, 0.018750f }, // 713 (6 3 0 7)
    { 7.444445f, 6.777778f, 0.888889f, 0.020134f }, // 714 (6 3 1 6)
    { 7.555555f, 6.222222f, 1.111111f, 0.021845f }, // 715 (6 3 2 5)
    { 7.666667f, 5.666667f, 1.333333f, 0.024000f }, // 716 (6 3 3 4)
    { 7.777778f, 5.111111f, 1.555556f, 0.026786f }, // 717 (6 3 4 3)
    { 7.888889f, 4.555555f, 1.777778f, 0.030508f }, // 718 (6 3 5 2)
    { 8.000000f, 4.000000f, 2.000000f, 0.035714f }, // 719 (6 3 6 1)
    { 8.111111f, 3.444444f, 2.222222f, 0.043478f }, // 720 (6 3 7 0)
    { 7.777778f, 6.444445f, 0.888889f, 0.020270f }, // 721 (6 4 0 6)
    { 7.888889f, 5.888889f, 1.111111f, 0.022113f }, // 722 (6 4 1 5)
    { 8.000000f, 5.333333f, 1.333333f, 0.024457f }, // 723 (6 4 2 4)
    { 8.111111f, 4.777778f, 1.555556f, 0.027523f }, // 724 (6 4 3 3)
    { 8.222222f, 4.222222f, 1.777778f, 0.031690f }, // 725 (6 4 4 2)
    { 8.333333f, 3.666667f, 2.000000f, 0.037657f }, // 726 (6 4 5 1)
    { 8.444445f, 3.111111f, 2.222222f, 0.046875f }, // 727 (6 4 6 0)
    { 8.222222f, 5.555555f, 1.111111f, 0.022500f }, // 728 (6 5 0 5)
    { 8.333333f, 5.000000f, 1.333333f, 0.025070f }, // 729 (6 5 1 4)
    { 8.444445f, 4.444445f, 1.555556f, 0.028481f }, // 730 (6 5 2 3)
    { 8.555555f, 3.888889f, 1.777778f, 0.033210f }, // 731 (6 5 3 2)
    { 8.666667f, 3.333333f, 2.000000f, 0.040179f }, // 732 (6 5 4 1)
    { 8.777778f, 2.777778f, 2.222222f, 0.051429f }, // 733 (6 5 5 0)
    { 8.666667f, 4.666667f, 1.333333f, 0.025862f }, // 734 (6 6 0 4)
    { 8.777778f, 4.111111f, 1.555556f, 0.029703f }, // 735 (6 6 1 3)
    { 8.888889f, 3.555556f, 1.777778f, 0.035156f }, // 736 (6 6 2 2)
    { 9.000000f, 3.000000f, 2.000000f, 0.043478f }, // 737 (6 6 3 1)
    { 9.111111f, 2.444444f, 2.222222f, 0.057692f }, // 738 (6 6 4 0)
    { 9.111111f, 3.777778f, 1.555556f, 0.031250f }, // 739 (6 7 0 3)
    { 9.222222f, 3.222222f, 1.777778f, 0.037657f }, // 740 (6 7 1 2)
    { 9.333333f, 2.666667f, 2.000000f, 0.047872f }, // 741 (6 7 2 1)
    { 9.444445f, 2.111111f, 2.222222f, 0.066667f }, // 742 (6 7 3 0)
    { 9.555555f, 2.888889f, 1.777778f, 0.040909f }, // 743 (6 8 0 2)
    { 9.666667f, 2.333333f, 2.000000f, 0.053892f }, // 744 (6 8 1 1)
    { 9.777778f, 1.777778f, 2.222222f, 0.080357f }, // 745 (6 8 2 0)
    { 10.000000f, 2.000000f, 2.000000f, 0.062500f }, // 746 (6 9 0 1)
    { 10.111111f, 1.444444f, 2.222222f, 0.103448f }, // 747 (6 9 1 0)
    { 10.444445f, 1.111111f, 2.222222f, 0.150000f }, // 748 (6 10 0 0)
    { 7.000000f, 9.000000f, 0.000000f, 0.015873f }, // 749 (7 0 0 9)
    { 7.111111f, 8.444445f, 0.222222f, 0.016667f }, // 750 (7 0 1 8)
    { 7.222222f, 7.888889f, 0.444444f, 0.017613f }, // 751 (7 0 2 7)
    { 7.333333f, 7.333333f, 0.666667f, 0.018750f }, // 752 (7 0 3 6)
    { 7.444445f, 6.777778f, 0.888889f, 0.020134f }, // 753 (7 0 4 5)
    { 7.555555f, 6.222222f, 1.111111f, 0.021845f }, // 754 (7 0 5 4)
    { 7.666667f, 5.666667f, 1.333333f, 0.024000f }, // 755 (7 0 6 3)
    { 7.777778f, 5.111111f, 1.555556f, 0.026786f }, // 756 (7 0 7 2)
    { 7.888889f, 4.555555f, 1.777778f, 0.030508f }, // 757 (7 0 8 1)
    { 8.000000f, 4.000000f, 2.000000f, 0.035714f }, // 758 (7 0 9 0)
    { 7.444445f, 8.111111f, 0.222222f, 0.016575f }, // 759 (7 1 0 8)
    { 7.555555f, 7.555555f, 0.444444f, 0.017578f }, // 760 (7 1 1 7)
    { 7.666667f, 7.000000f, 0.666667f, 0.018789f }, // 761 (7 1 2 6)
    { 7.777778f, 6.444445f, 0.888889f, 0.020270f }, // 762 (7 1 3 5)
    { 7.888889f, 5.888889f, 1.111111f, 0.022113f }, // 763 (7 1 4 4)
    { 8.000000f, 5.333333f, 1.333333f, 0.024457f }, // 764 (7 1 5 3)
    { 8.111111f, 4.777778f, 1.555556f, 0.027523f }, // 765 (7 1 6 2)
    { 8.222222f, 4.222222f, 1.777778f, 0.031690f }, // 766 (7 1 7 1)
    { 8.333333f, 3.666667f, 2.000000f, 0.037657f }, // 767 (7 1 8 0)
    { 7.888889f, 7.222222f, 0.444444f, 0.017613f }, // 768 (7 2 0 7)
    { 8.000000f, 6.666667f, 0.666667f, 0.018908f }, // 769 (7 2 1 6)
    { 8.111111f, 6.111111f, 0.888889f, 0.020501f }, // 770 (7 2 2 5)
    { 8.222222f, 5.555555f, 1.111111f, 0.022500f }, // 771 (7 2 3 4)
    { 8.333333f, 5.000000f, 1.333333f, 0.025070f }, // 772 (7 2 4 3)
    { 8.444445f, 4.444445f, 1.555556f, 0.028481f }, // 773 (7 2 5 2)
    { 8.555555f, 3.888889f, 1.777778f, 0.033210f }, // 774 (7 2 6 1)
    { 8.666667f, 3.333333f, 2.000000f, 0.040179f }, // 775 (7 2 7 0)
    { 8.333333f, 6.333333f, 0.666667f, 0.019108f }, // 776 (7 3 0 6)
    { 8.444445f, 5.777778f, 0.888889f, 0.020833f }, // 777 (7 3 1 5)
    { 8.555555f, 5.222222f, 1.111111f, 0.023018f }, // 778 (7 3 2 4)
    { 8.666667f, 4.666667f, 1.333333f, 0.025862f }, // 779 (7 3 3 3)
    { 8.777778f, 4.111111f, 1.555556f, 0.029703f }, // 780 (7 3 4 2)
    { 8.888889f, 3.555556f, 1.777778f, 0.035156f }, // 781 (7 3 5 1)
    { 9.000000f, 3.000000f, 2.000000f, 0.043478f }, // 782 (7 3 6 0)
    { 8.777778f, 5.444445f, 0.888889f, 0.021277f }, // 783 (7 4 0 5)
    { 8.888889f, 4.888889f, 1.111111f, 0.023684f }, // 784 (7 4 1 4)
    { 9.000000f, 4.333333f, 1.333333f, 0.026866f }, // 785 (7 4 2 3)
    { 9.111111f, 3.777778f, 1.555556f, 0.031250f }, // 786 (7 4 3 2)
    { 9.222222f, 3.222222f, 1.777778f, 0.037657f }, // 787 (7 4 4 1)
    { 9.333333f, 2.666667f, 2.000000f, 0.047872f }, // 788 (7 4 5 0)
    { 9.222222f, 4.555555f, 1.111111f, 0.024523f }, // 789 (7 5 0 4)
    { 9.333333f, 4.000000f, 1.333333f, 0.028125f }, // 790 (7 5 1 3)
    { 9.444445f, 3.444444f, 1.555556f, 0.033210f }, // 791 (7 5 2 2)
    { 9.555555f, 2.888889f, 1.777778f, 0.040909f }, // 792 (7 5 3 1)
    { 9.666667f, 2.333333f, 2.000000f, 0.053892f }, // 793 (7 5 4 0)
    { 9.666667f, 3.666667f, 1.333333f, 0.029703f }, // 794 (7 6 0 3)
    { 9.777778f, 3.111111f, 1.555556f, 0.035714f }, // 795 (7 6 1 2)
    { 9.888889f, 2.555556f, 1.777778f, 0.045226f }, // 796 (7 6 2 1)
    { 10.000000f, 2.000000f, 2.000000f, 0.062500f }, // 797 (7 6 3 0)
    { 10.111111f, 2.777778f, 1.555556f, 0.038961f }, // 798 (7 7 0 2)
    { 10.222222f, 2.222222f, 1.777778f, 0.051136f }, // 799 (7 7 1 1)
    { 10.333333f, 1.666667f, 2.000000f, 0.075630f }, // 800 (7 7 2 0)
    { 10.555555f, 1.888889f, 1.777778f, 0.059603f }, // 801 (7 8 0 1)
    { 10.666667f, 1.333333f, 2.000000f, 0.097826f }, // 802 (7 8 1 0)
    { 11.000000f, 1.000000f, 2.000000f, 0.142857f }, // 803 (7 9 0 0)
    { 8.000000f, 8.000000f, 0.000000f, 0.015625f }, // 804 (8 0 0 8)
    { 8.111111f, 7.444445f, 0.222222f, 0.016575f }, // 805 (8 0 1 7)
    { 8.222222f, 6.888889f, 0.444444f, 0.017717f }, // 806 (8 0 2 6)
    { 8.333333f, 6.333333f, 0.666667f, 0.019108f }, // 807 (8 0 3 5)
    { 8.444445f, 5.777778f, 0.888889f, 0.020833f }, // 808 (8 0 4 4)
    { 8.555555f, 5.222222f, 1.111111f, 0.023018f }, // 809 (8 0 5 3)
    { 8.666667f, 4.666667f, 1.333333f, 0.025862f }, // 810 (8 0 6 2)
    { 8.777778f, 4.111111f, 1.555556f, 0.029703f }, // 811 (8 0 7 1)
    { 8.888889f, 3.555556f, 1.777778f, 0.035156f }, // 812 (8 0 8 0)
    { 8.444445f, 7.111111f, 0.222222f, 0.016667f }, // 813 (8 1 0 7)
    { 8.555555f, 6.555555f, 0.444444f, 0.017893f }, // 814 (8 1 1 6)
    { 8.666667f, 6.000000f, 0.666667f, 0.019397f }, // 815 (8 1 2 5)
    { 8.777778f, 5.444445f, 0.888889f, 0.021277f }, // 816 (8 1 3 4)
    { 8.888889f, 4.888889f, 1.111111f, 0.023684f }, // 817 (8 1 4 3)
    { 9.000000f, 4.333333f, 1.333333f, 0.026866f }, // 818 (8 1 5 2)
    { 9.111111f, 3.777778f, 1.555556f, 0.031250f }, // 819 (8 1 6 1)
    { 9.222222f, 3.222222f, 1.777778f, 0.037657f }, // 820 (8 1 7 0)
    { 8.888889f, 6.222222f, 0.444444f, 0.018145f }, // 821 (8 2 0 6)
    { 9.000000f, 5.666667f, 0.666667f, 0.019780f }, // 822 (8 2 1 5)
    { 9.111111f, 5.111111f, 0.888889f, 0.021845f }, // 823 (8 2 2 4)
    { 9.222222f, 4.555555f, 1.111111f, 0.024523f }, // 824 (8 2 3 3)
    { 9.333333f, 4.000000f, 1.333333f, 0.028125f }, // 825 (8 2 4 2)
    { 9.444445f, 3.444444f, 1.555556f, 0.033210f }, // 826 (8 2 5 1)
    { 9.555555f, 2.888889f, 1.777778f, 0.040909f }, // 827 (8 2 6 0)
    { 9.333333f, 5.333333f, 0.666667f, 0.020270f }, // 828 (8 3 0 5)
    { 9.444445f, 4.777778f, 0.888889f, 0.022556f }, // 829 (8 3 1 4)
    { 9.555555f, 4.222222f, 1.111111f, 0.025568f }, // 830 (8 3 2 3)
    { 9.666667f, 3.666667f, 1.333333f, 0.029703f }, // 831 (8 3 3 2)
    { 9.777778f, 3.111111f, 1.555556f, 0.035714f }, // 832 (8 3 4 1)
    { 9.888889f, 2.555556f, 1.777778f, 0.045226f }, // 833 (8 3 5 0)
    { 9.777778f, 4.444445f, 0.888889f, 0.023438f }, // 834 (8 4 0 4)
    { 9.888889f, 3.888889f, 1.111111f, 0.026866f }, // 835 (8 4 1 3)
    { 10.000000f, 3.333333f, 1.333333f, 0.031690f }, // 836 (8 4 2 2)
    { 10.111111f, 2.777778f, 1.555556f, 0.038961f }, // 837 (8 4 3 1)
    { 10.222222f, 2.222222f, 1.777778f, 0.051136f }, // 838 (8 4 4 0)
    { 10.222222f, 3.555556f, 1.111111f, 0.028481f }, // 839 (8 5 0 3)
    { 10.333333f, 3.000000f, 1.333333f, 0.034221f }, // 840 (8 5 1 2)
    { 10.444445f, 2.444444f, 1.555556f, 0.043269f }, // 841 (8 5 2 1)
    { 10.555555f, 1.888889f, 1.777778f, 0.059603f }, // 842 (8 5 3 0)
    { 10.666667f, 2.666667f, 1.333333f, 0.037500f }, // 843 (8 6 0 2)
    { 10.777778f, 2.111111f, 1.555556f, 0.049180f }, // 844 (8 6 1 1)
    { 10.888889f, 1.555556f, 1.777778f, 0.072581f }, // 845 (8 6 2 0)
    { 11.111111f, 1.777778f, 1.555556f, 0.057692f }, // 846 (8 7 0 1)
    { 11.222222f, 1.222222f, 1.777778f, 0.094737f }, // 847 (8 7 1 0)
    { 11.555555f, 0.888889f, 1.777778f, 0.140625f }, // 848 (8 8 0 0)
    { 9.000000f, 7.000000f, 0.000000f, 0.015873f }, // 849 (9 0 0 7)
    { 9.111111f, 6.444445f, 0.222222f, 0.017045f }, // 850 (9 0 1 6)
    { 9.222222f, 5.888889f, 0.444444f, 0.018480f }, // 851 (9 0 2 5)
    { 9.333333f, 5.333333f, 0.666667f, 0.020270f }, // 852 (9 0 3 4)
    { 9.444445f, 4.777778f, 0.888889f, 0.022556f }, // 853 (9 0 4 3)
    { 9.555555f, 4.222222f, 1.111111f, 0.025568f }, // 854 (9 0 5 2)
    { 9.666667f, 3.666667f, 1.333333f, 0.029703f }, // 855 (9 0 6 1)
    { 9.777778f, 3.111111f, 1.555556f, 0.035714f }, // 856 (9 0 7 0)
    { 9.444445f, 6.111111f, 0.222222f, 0.017341f }, // 857 (9 1 0 6)
    { 9.555555f, 5.555555f, 0.444444f, 0.018908f }, // 858 (9 1 1 5)
    { 9.666667f, 5.000000f, 0.666667f, 0.020882f }, // 859 (9 1 2 4)
    { 9.777778f, 4.444445f, 0.888889f, 0.023438f }, // 860 (9 1 3 3)
    { 9.888889f, 3.888889f, 1.111111f, 0.026866f }, // 861 (9 1 4 2)
    { 10.000000f, 3.333333f, 1.333333f, 0.031690f }, // 862 (9 1 5 1)
    { 10.111111f, 2.777778f, 1.555556f, 0.038961f }, // 863 (9 1 6 0)
    { 9.888889f, 5.222222f, 0.444444f, 0.019438f }, // 864 (9 2 0 5)
    { 10.000000f, 4.666667f, 0.666667f, 0.021635f }, // 865 (9 2 1 4)
    { 10.111111f, 4.111111f, 0.888889f, 0.024523f }, // 866 (9 2 2 3)
    { 10.222222f, 3.555556f, 1.111111f, 0.028481f }, // 867 (9 2 3 2)
    { 10.333333f, 3.000000f, 1.333333f, 0.034221f }, // 868 (9 2 4 1)
    { 10.444445f, 2.444444f, 1.555556f, 0.043269f }, // 869 (9 2 5 0)
    { 10.333333f, 4.333333f, 0.666667f, 0.022556f }, // 870 (9 3 0 4)
    { 10.444445f, 3.777778f, 0.888889f, 0.025862f }, // 871 (9 3 1 3)
    { 10.555555f, 3.222222f, 1.111111f, 0.030508f }, // 872 (9 3 2 2)
    { 10.666667f, 2.666667f, 1.333333f, 0.037500f }, // 873 (9 3 3 1)
    { 10.777778f, 2.111111f, 1.555556f, 0.049180f }, // 874 (9 3 4 0)
    { 10.777778f, 3.444444f, 0.888889f, 0.027523f }, // 875 (9 4 0 3)
    { 10.888889f, 2.888889f, 1.111111f, 0.033088f }, // 876 (9 4 1 2)
    { 11.000000f, 2.333333f, 1.333333f, 0.041860f }, // 877 (9 4 2 1)
    { 11.111111f, 1.777778f, 1.555556f, 0.057692f }, // 878 (9 4 3 0)
    { 11.222222f, 2.555556f, 1.111111f, 0.036437f }, // 879 (9 5 0 2)
    { 11.333333f, 2.000000f, 1.333333f, 0.047872f }, // 880 (9 5 1 1)
    { 11.444445f, 1.444444f, 1.555556f, 0.070866f }, // 881 (9 5 2 0)
    { 11.666667f, 1.666667f, 1.333333f, 0.056604f }, // 882 (9 6 0 1)
    { 11.777778f, 1.111111f, 1.555556f, 0.093750f }, // 883 (9 6 1 0)
    { 12.111111f, 0.777778f, 1.555556f, 0.142857f }, // 884 (9 7 0 0)
    { 10.000000f, 6.000000f, 0.000000f, 0.016667f }, // 885 (10 0 0 6)
    { 10.111111f, 5.444445f, 0.222222f, 0.018182f }, // 886 (10 0 1 5)
    { 10.222222f, 4.888889f, 0.444444f, 0.020089f }, // 887 (10 0 2 4)
    { 10.333333f, 4.333333f, 0.666667f, 0.022556f }, // 888 (10 0 3 3)
    { 10.444445f, 3.777778f, 0.888889f, 0.025862f }, // 889 (10 0 4 2)
    { 10.555555f, 3.222222f, 1.111111f, 0.030508f }, // 890 (10 0 5 1)
    { 10.666667f, 2.666667f, 1.333333f, 0.037500f }, // 891 (10 0 6 0)
    { 10.444445f, 5.111111f, 0.222222f, 0.018750f }, // 892 (10 1 0 5)
    { 10.555555f, 4.555555f, 0.444444f, 0.020882f }, // 893 (10 1 1 4)
    { 10.666667f, 4.000000f, 0.666667f, 0.023684f }, // 894 (10 1 2 3)
    { 10.777778f, 3.444444f, 0.888889f, 0.027523f }, // 895 (10 1 3 2)
    { 10.888889f, 2.888889f, 1.111111f, 0.033088f }, // 896 (10 1 4 1)
    { 11.000000f, 2.333333f, 1.333333f, 0.041860f }, // 897 (10 1 5 0)
    { 10.888889f, 4.222222f, 0.444444f, 0.021845f }, // 898 (10 2 0 4)
    { 11.000000f, 3.666667f, 0.666667f, 0.025070f }, // 899 (10 2 1 3)
    { 11.111111f, 3.111111f, 0.888889f, 0.029605f }, // 900 (10 2 2 2)
    { 11.222222f, 2.555556f, 1.111111f, 0.036437f }, // 901 (10 2 3 1)
    { 11.333333f, 2.000000f, 1.333333f, 0.047872f }, // 902 (10 2 4 0)
    { 11.333333f, 3.333333f, 0.666667f, 0.026786f }, // 903 (10 3 0 3)
    { 11.444445f, 2.777778f, 0.888889f, 0.032258f }, // 904 (10 3 1 2)
    { 11.555555f, 2.222222f, 1.111111f, 0.040909f }, // 905 (10 3 2 1)
    { 11.666667f, 1.666667f, 1.333333f, 0.056604f }, // 906 (10 3 3 0)
    { 11.777778f, 2.444444f, 0.888889f, 0.035714f }, // 907 (10 4 0 2)
    { 11.888889f, 1.888889f, 1.111111f, 0.047120f }, // 908 (10 4 1 1)
    { 12.000000f, 1.333333f, 1.333333f, 0.070313f }, // 909 (10 4 2 0)
    { 12.222222f, 1.555556f, 1.111111f, 0.056250f }, // 910 (10 5 0 1)
    { 12.333333f, 1.000000f, 1.333333f, 0.094737f }, // 911 (10 5 1 0)
    { 12.666667f, 0.666667f, 1.333333f, 0.150000f }, // 912 (10 6 0 0)
    { 11.000000f, 5.000000f, 0.000000f, 0.018182f }, // 913 (11 0 0 5)
    { 11.111111f, 4.444445f, 0.222222f, 0.020270f }, // 914 (11 0 1 4)
    { 11.222222f, 3.888889f, 0.444444f, 0.023018f }, // 915 (11 0 2 3)
    { 11.333333f, 3.333333f, 0.666667f, 0.026786f }, // 916 (11 0 3 2)
    { 11.444445f, 2.777778f, 0.888889f, 0.032258f }, // 917 (11 0 4 1)
    { 11.555555f, 2.222222f, 1.111111f, 0.040909f }, // 918 (11 0 5 0)
    { 11.444445f, 4.111111f, 0.222222f, 0.021277f }, // 919 (11 1 0 4)
    { 11.555555f, 3.555556f, 0.444444f, 0.024457f }, // 920 (11 1 1 3)
    { 11.666667f, 3.000000f, 0.666667f, 0.028939f }, // 921 (11 1 2 2)
    { 11.777778f, 2.444444f, 0.888889f, 0.035714f }, // 922 (11 1 3 1)
    { 11.888889f, 1.888889f, 1.111111f, 0.047120f }, // 923 (11 1 4 0)
    { 11.888889f, 3.222222f, 0.444444f, 0.026239f }, // 924 (11 2 0 3)
    { 12.000000f, 2.666667f, 0.666667f, 0.031690f }, // 925 (11 2 1 2)
    { 12.111111f, 2.111111f, 0.888889f, 0.040359f }, // 926 (11 2 2 1)
    { 12.222222f, 1.555556f, 1.111111f, 0.056250f }, // 927 (11 2 3 0)
    { 12.333333f, 2.333333f, 0.666667f, 0.035294f }, // 928 (11 3 0 2)
    { 12.444445f, 1.777778f, 0.888889f, 0.046875f }, // 929 (11 3 1 1)
    { 12.555555f, 1.222222f, 1.111111f, 0.070866f }, // 930 (11 3 2 0)
    { 12.777778f, 1.444444f, 0.888889f, 0.056604f }, // 931 (11 4 0 1)
    { 12.888889f, 0.888889f, 1.111111f, 0.097826f }, // 932 (11 4 1 0)
    { 13.222222f, 0.555556f, 1.111111f, 0.163636f }, // 933 (11 5 0 0)
    { 12.000000f, 4.000000f, 0.000000f, 0.020833f }, // 934 (12 0 0 4)
    { 12.111111f, 3.444444f, 0.222222f, 0.024000f }, // 935 (12 0 1 3)
    { 12.222222f, 2.888889f, 0.444444f, 0.028481f }, // 936 (12 0 2 2)
    { 12.333333f, 2.333333f, 0.666667f, 0.035294f }, // 937 (12 0 3 1)
    { 12.444445f, 1.777778f, 0.888889f, 0.046875f }, // 938 (12 0 4 0)
    { 12.444445f, 3.111111f, 0.222222f, 0.025862f }, // 939 (12 1 0 3)
    { 12.555555f, 2.555556f, 0.444444f, 0.031359f }, // 940 (12 1 1 2)
    { 12.666667f, 2.000000f, 0.666667f, 0.040179f }, // 941 (12 1 2 1)
    { 12.777778f, 1.444444f, 0.888889f, 0.056604f }, // 942 (12 1 3 0)
    { 12.888889f, 2.222222f, 0.444444f, 0.035156f }, // 943 (12 2 0 2)
    { 13.000000f, 1.666667f, 0.666667f, 0.047120f }, // 944 (12 2 1 1)
    { 13.111111f, 1.111111f, 0.888889f, 0.072581f }, // 945 (12 2 2 0)
    { 13.333333f, 1.333333f, 0.666667f, 0.057692f }, // 946 (12 3 0 1)
    { 13.444445f, 0.777778f, 0.888889f, 0.103448f }, // 947 (12 3 1 0)
    { 13.777778f, 0.444444f, 0.888889f, 0.187500f }, // 948 (12 4 0 0)
    { 13.000000f, 3.000000f, 0.000000f, 0.025641f }, // 949 (13 0 0 3)
    { 13.111111f, 2.444444f, 0.222222f, 0.031250f }, // 950 (13 0 1 2)
    { 13.222222f, 1.888889f, 0.444444f, 0.040359f }, // 951 (13 0 2 1)
    { 13.333333f, 1.333333f, 0.666667f, 0.057692f }, // 952 (13 0 3 0)
    { 13.444445f, 2.111111f, 0.222222f, 0.035294f }, // 953 (13 1 0 2)
    { 13.555555f, 1.555556f, 0.444444f, 0.047872f }, // 954 (13 1 1 1)
    { 13.666667f, 1.000000f, 0.666667f, 0.075630f }, // 955 (13 1 2 0)
    { 13.888889f, 1.222222f, 0.444444f, 0.059603f }, // 956 (13 2 0 1)
    { 14.000000f, 0.666667f, 0.666667f, 0.112500f }, // 957 (13 2 1 0)
    { 14.333333f, 0.333333f, 0.666667f, 0.230769f }, // 958 (13 3 0 0)
    { 14.000000f, 2.000000f, 0.000000f, 0.035714f }, // 959 (14 0 0 2)
    { 14.111111f, 1.444444f, 0.222222f, 0.049180f }, // 960 (14 0 1 1)
    { 14.222222f, 0.888889f, 0.444444f, 0.080357f }, // 961 (14 0 2 0)
    { 14.444445f, 1.111111f, 0.222222f, 0.062500f }, // 962 (14 1 0 1)
    { 14.555555f, 0.555556f, 0.444444f, 0.126761f }, // 963 (14 1 1 0)
    { 14.888889f, 0.222222f, 0.444444f, 0.321429f }, // 964 (14 2 0 0)
    { 15.000000f, 1.000000f, 0.000000f, 0.066667f }, // 965 (15 0 0 1)
    { 15.111111f, 0.444444f, 0.222222f, 0.150000f }, // 966 (15 0 1 0)
    { 15.444445f, 0.111111f, 0.222222f, 0.600000f }, // 967 (15 1 0 0)
    { 16.000000f, 0.000000f, 0.000000f, FLT_MAX }, // 968 (16 0 0 0)
}; // 969 four cluster elements

#if ICBC_USE_SIMD

bool ClusterFit::compress3(Vector3 * start, Vector3 * end)
{
    const int count = m_count;
    const SimdVector one = SimdVector(1.0f);
    const SimdVector zero = SimdVector(0.0f);
    const SimdVector half(0.5f, 0.5f, 0.5f, 0.25f);
    const SimdVector two = SimdVector(2.0);
    const SimdVector grid(31.0f, 63.0f, 31.0f, 0.0f);
    const SimdVector gridrcp(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f, 0.0f);

    // declare variables
    SimdVector beststart = SimdVector(0.0f);
    SimdVector bestend = SimdVector(0.0f);
    SimdVector besterror = SimdVector(FLT_MAX);

    SimdVector x0 = zero;

    // check all possible clusters for this total order
    for (int c0 = 0; c0 <= count; c0++)
    {
        SimdVector x1 = zero;

        for (int c1 = 0; c1 <= count - c0; c1++)
        {
            const SimdVector x2 = m_xsum - x1 - x0;

            //Vector3 alphax_sum = x0 + x1 * 0.5f;
            //float alpha2_sum = w0 + w1 * 0.25f;
            const SimdVector alphax_sum = multiplyAdd(x1, half, x0); // alphax_sum, alpha2_sum
            const SimdVector alpha2_sum = alphax_sum.splatW();

            //const Vector3 betax_sum = x2 + x1 * 0.5f;
            //const float beta2_sum = w2 + w1 * 0.25f;
            const SimdVector betax_sum = multiplyAdd(x1, half, x2); // betax_sum, beta2_sum
            const SimdVector beta2_sum = betax_sum.splatW();

            //const float alphabeta_sum = w1 * 0.25f;
            const SimdVector alphabeta_sum = (x1 * half).splatW(); // alphabeta_sum

            // const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
            const SimdVector factor = reciprocal(negativeMultiplySubtract(alphabeta_sum, alphabeta_sum, alpha2_sum*beta2_sum));

            SimdVector a = negativeMultiplySubtract(betax_sum, alphabeta_sum, alphax_sum*beta2_sum) * factor;
            SimdVector b = negativeMultiplySubtract(alphax_sum, alphabeta_sum, betax_sum*alpha2_sum) * factor;

            // clamp to the grid
            a = min(one, max(zero, a));
            b = min(one, max(zero, b));
            a = truncate(multiplyAdd(grid, a, half)) * gridrcp;
            b = truncate(multiplyAdd(grid, b, half)) * gridrcp;

            // compute the error (we skip the constant xxsum)
            SimdVector e1 = multiplyAdd(a*a, alpha2_sum, b*b*beta2_sum);
            SimdVector e2 = negativeMultiplySubtract(a, alphax_sum, a*b*alphabeta_sum);
            SimdVector e3 = negativeMultiplySubtract(b, betax_sum, e2);
            SimdVector e4 = multiplyAdd(two, e3, e1);

            // apply the metric to the error term
            SimdVector e5 = e4 * m_metricSqr;
            SimdVector error = e5.splatX() + e5.splatY() + e5.splatZ();

            // keep the solution if it wins
            if (compareAnyLessThan(error, besterror))
            {
                besterror = error;
                beststart = a;
                bestend = b;
            }

            x1 += m_weighted[c0 + c1];
        }

        x0 += m_weighted[c0];
    }

    // save the block if necessary
    if (compareAnyLessThan(besterror, m_besterror))
    {
        *start = beststart.toVector3();
        *end = bestend.toVector3();

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::compress4(Vector3 * start, Vector3 * end)
{
    const int count = m_count;
    const SimdVector one = SimdVector(1.0f);
    const SimdVector zero = SimdVector(0.0f);
    const SimdVector half = SimdVector(0.5f);
    const SimdVector two = SimdVector(2.0);
    const SimdVector onethird(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 9.0f);
    const SimdVector twothirds(2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f, 4.0f / 9.0f);
    const SimdVector twonineths = SimdVector(2.0f / 9.0f);
    const SimdVector grid(31.0f, 63.0f, 31.0f, 0.0f);
    const SimdVector gridrcp(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f, 0.0f);

    // declare variables
    SimdVector beststart = SimdVector(0.0f);
    SimdVector bestend = SimdVector(0.0f);
    SimdVector besterror = SimdVector(FLT_MAX);

    SimdVector x0 = zero;

    // check all possible clusters for this total order
    for (int c0 = 0; c0 <= count; c0++)
    {
        SimdVector x1 = zero;

        for (int c1 = 0; c1 <= count - c0; c1++)
        {
            SimdVector x2 = zero;

            for (int c2 = 0; c2 <= count - c0 - c1; c2++)
            {
                const SimdVector x3 = m_xsum - x2 - x1 - x0;

                //const Vector3 alphax_sum = x0 + x1 * (2.0f / 3.0f) + x2 * (1.0f / 3.0f);
                //const float alpha2_sum = w0 + w1 * (4.0f/9.0f) + w2 * (1.0f/9.0f);
                const SimdVector alphax_sum = multiplyAdd(x2, onethird, multiplyAdd(x1, twothirds, x0)); // alphax_sum, alpha2_sum
                const SimdVector alpha2_sum = alphax_sum.splatW();

                //const Vector3 betax_sum = x3 + x2 * (2.0f / 3.0f) + x1 * (1.0f / 3.0f);
                //const float beta2_sum = w3 + w2 * (4.0f/9.0f) + w1 * (1.0f/9.0f);
                const SimdVector betax_sum = multiplyAdd(x2, twothirds, multiplyAdd(x1, onethird, x3)); // betax_sum, beta2_sum
                const SimdVector beta2_sum = betax_sum.splatW();

                //const float alphabeta_sum = (w1 + w2) * (2.0f/9.0f);
                const SimdVector alphabeta_sum = twonineths * (x1 + x2).splatW(); // alphabeta_sum

                //const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
                const SimdVector factor = reciprocal(negativeMultiplySubtract(alphabeta_sum, alphabeta_sum, alpha2_sum*beta2_sum));

                SimdVector a = negativeMultiplySubtract(betax_sum, alphabeta_sum, alphax_sum*beta2_sum) * factor;
                SimdVector b = negativeMultiplySubtract(alphax_sum, alphabeta_sum, betax_sum*alpha2_sum) * factor;

                // clamp to the grid
                a = min(one, max(zero, a));
                b = min(one, max(zero, b));
                a = truncate(multiplyAdd(grid, a, half)) * gridrcp;
                b = truncate(multiplyAdd(grid, b, half)) * gridrcp;

                // compute the error (we skip the constant xxsum)
                // error = a*a*alpha2_sum + b*b*beta2_sum + 2.0f*( a*b*alphabeta_sum - a*alphax_sum - b*betax_sum );
                SimdVector e1 = multiplyAdd(a*a, alpha2_sum, b*b*beta2_sum);
                SimdVector e2 = negativeMultiplySubtract(a, alphax_sum, a*b*alphabeta_sum);
                SimdVector e3 = negativeMultiplySubtract(b, betax_sum, e2);
                SimdVector e4 = multiplyAdd(two, e3, e1);

                // apply the metric to the error term
                SimdVector e5 = e4 * m_metricSqr;
                SimdVector error = e5.splatX() + e5.splatY() + e5.splatZ();

                // keep the solution if it wins
                if (compareAnyLessThan(error, besterror))
                {
                    besterror = error;
                    beststart = a;
                    bestend = b;
                }

                x2 += m_weighted[c0 + c1 + c2];
            }

            x1 += m_weighted[c0 + c1];
        }

        x0 += m_weighted[c0];
    }

    // save the block if necessary
    if (compareAnyLessThan(besterror, m_besterror))
    {
        *start = beststart.toVector3();
        *end = bestend.toVector3();

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::fastCompress3(Vector3 * start, Vector3 * end)
{
    const int count = m_count;
    const SimdVector one = SimdVector(1.0f);
    const SimdVector zero = SimdVector(0.0f);
    const SimdVector half(0.5f, 0.5f, 0.5f, 0.25f);
    const SimdVector two = SimdVector(2.0);
    const SimdVector grid(31.0f, 63.0f, 31.0f, 0.0f);
    const SimdVector gridrcp(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f, 0.0f);

    // declare variables
    SimdVector beststart = SimdVector(0.0f);
    SimdVector bestend = SimdVector(0.0f);
    SimdVector besterror = SimdVector(FLT_MAX);

    SimdVector x0 = zero;

    // check all possible clusters for this total order
    for (int c0 = 0, i = 0; c0 <= count; c0++)
    {
        SimdVector x1 = zero;

        for (int c1 = 0; c1 <= count - c0; c1++, i++)
        {
            const SimdVector constants = SimdVector((const float *)&s_threeElement[i]);

            const SimdVector alpha2_sum = constants.splatX();
            const SimdVector beta2_sum = constants.splatY();
            const SimdVector alphabeta_sum = constants.splatZ();
            const SimdVector factor = constants.splatW();

            const SimdVector alphax_sum = multiplyAdd(x1, half, x0);
            const SimdVector betax_sum = m_xsum - alphax_sum;

            SimdVector a = negativeMultiplySubtract(betax_sum, alphabeta_sum, alphax_sum*beta2_sum) * factor;
            SimdVector b = negativeMultiplySubtract(alphax_sum, alphabeta_sum, betax_sum*alpha2_sum) * factor;

            // clamp to the grid
            a = min(one, max(zero, a));
            b = min(one, max(zero, b));
            a = truncate(multiplyAdd(grid, a, half)) * gridrcp;
            b = truncate(multiplyAdd(grid, b, half)) * gridrcp;

            // compute the error (we skip the constant xxsum)
            SimdVector e1 = multiplyAdd(a*a, alpha2_sum, b*b*beta2_sum);
            SimdVector e2 = negativeMultiplySubtract(a, alphax_sum, a*b*alphabeta_sum);
            SimdVector e3 = negativeMultiplySubtract(b, betax_sum, e2);
            SimdVector e4 = multiplyAdd(two, e3, e1);

            // apply the metric to the error term
            SimdVector e5 = e4 * m_metricSqr;
            SimdVector error = e5.splatX() + e5.splatY() + e5.splatZ();

            // keep the solution if it wins
            if (compareAnyLessThan(error, besterror))
            {
                besterror = error;
                beststart = a;
                bestend = b;
            }

            x1 += m_weighted[c0 + c1];
        }

        x0 += m_weighted[c0];
    }

    // save the block if necessary
    if (compareAnyLessThan(besterror, m_besterror))
    {
        *start = beststart.toVector3();
        *end = bestend.toVector3();

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::fastCompress4(Vector3 * start, Vector3 * end)
{
    const SimdVector one = SimdVector(1.0f);
    const SimdVector zero = SimdVector(0.0f);
    const SimdVector half = SimdVector(0.5f);
    const SimdVector two = SimdVector(2.0);
    const SimdVector onethird(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 9.0f);
    const SimdVector twothirds(2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f, 4.0f / 9.0f);
    const SimdVector grid(31.0f, 63.0f, 31.0f, 0.0f);
    const SimdVector gridrcp(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f, 0.0f);

    // declare variables
    SimdVector beststart = SimdVector(0.0f);
    SimdVector bestend = SimdVector(0.0f);
    SimdVector besterror = SimdVector(FLT_MAX);

    SimdVector x0 = zero;

    // check all possible clusters for this total order
    for (int c0 = 0, i = 0; c0 <= 16; c0++)
    {
        SimdVector x1 = zero;

        for (int c1 = 0; c1 <= 16 - c0; c1++)
        {
            SimdVector x2 = zero;

            for (int c2 = 0; c2 <= 16 - c0 - c1; c2++, i++)
            {
                const SimdVector constants = SimdVector((const float *)&s_fourElement[i]); 

                const SimdVector alpha2_sum = constants.splatX();
                const SimdVector beta2_sum = constants.splatY();
                const SimdVector alphabeta_sum = constants.splatZ();
                const SimdVector factor = constants.splatW();
                
                const SimdVector alphax_sum = multiplyAdd(x2, onethird, multiplyAdd(x1, twothirds, x0));
                const SimdVector betax_sum = m_xsum - alphax_sum;

                SimdVector a = negativeMultiplySubtract(betax_sum, alphabeta_sum, alphax_sum*beta2_sum) * factor;
                SimdVector b = negativeMultiplySubtract(alphax_sum, alphabeta_sum, betax_sum*alpha2_sum) * factor;

                // clamp to the grid
                a = min(one, max(zero, a));
                b = min(one, max(zero, b));
                a = truncate(multiplyAdd(grid, a, half)) * gridrcp;
                b = truncate(multiplyAdd(grid, b, half)) * gridrcp;

                // compute the error (we skip the constant xxsum)
                // error = a*a*alpha2_sum + b*b*beta2_sum + 2.0f*( a*b*alphabeta_sum - a*alphax_sum - b*betax_sum );
                SimdVector e1 = multiplyAdd(a*a, alpha2_sum, b*b*beta2_sum);
                SimdVector e2 = negativeMultiplySubtract(a, alphax_sum, a*b*alphabeta_sum);
                SimdVector e3 = negativeMultiplySubtract(b, betax_sum, e2);
                SimdVector e4 = multiplyAdd(two, e3, e1);

                // apply the metric to the error term
                SimdVector e5 = e4 * m_metricSqr;
                SimdVector error = e5.splatX() + e5.splatY() + e5.splatZ();

                // keep the solution if it wins
                if (compareAnyLessThan(error, besterror))
                {
                    besterror = error;
                    beststart = a;
                    bestend = b;
                }

                x2 += m_weighted[c0 + c1 + c2];
            }

            x1 += m_weighted[c0 + c1];
        }

        x0 += m_weighted[c0];
    }

    // save the block if necessary
    if (compareAnyLessThan(besterror, m_besterror))
    {
        *start = beststart.toVector3();
        *end = bestend.toVector3();

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

#else

// This is the ideal way to round, but it's too expensive to do this in the inner loop.
inline Vector3 round565(const Vector3 & v) {
    static const Vector3 grid = { 31.0f, 63.0f, 31.0f };
    static const Vector3 gridrcp = { 1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f };

    Vector3 q = floor(grid * v);
    q.x += (v.x > midpoints5[int(q.x)]);
    q.y += (v.y > midpoints6[int(q.y)]);
    q.z += (v.z > midpoints5[int(q.z)]);
    q *= gridrcp;
    return q;
}

bool ClusterFit::compress3(Vector3 * start, Vector3 * end)
{
    const uint count = m_count;
    const Vector3 grid = { 31.0f, 63.0f, 31.0f };
    const Vector3 gridrcp = { 1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f };

    // declare variables
    Vector3 beststart = { 0.0f };
    Vector3 bestend = { 0.0f };
    float besterror = FLT_MAX;

    Vector3 x0 = { 0.0f };
    float w0 = 0.0f;

    // check all possible clusters for this total order
    for (uint c0 = 0; c0 <= count; c0++)
    {
        Vector3 x1 = { 0.0f };
        float w1 = 0.0f;

        for (uint c1 = 0; c1 <= count - c0; c1++)
        {
            float w2 = m_wsum - w0 - w1;

            // These factors could be entirely precomputed.
            float const alpha2_sum = w0 + w1 * 0.25f;
            float const beta2_sum = w2 + w1 * 0.25f;
            float const alphabeta_sum = w1 * 0.25f;
            float const factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

            Vector3 const alphax_sum = x0 + x1 * 0.5f;
            Vector3 const betax_sum = m_xsum - alphax_sum;

            Vector3 a = (alphax_sum*beta2_sum - betax_sum * alphabeta_sum) * factor;
            Vector3 b = (betax_sum*alpha2_sum - alphax_sum * alphabeta_sum) * factor;

            // clamp to the grid
            a = saturate(a);
            b = saturate(b);
#if ICBC_PERFECT_ROUND
            a = round565(a);
            b = round565(b);
#else
            a = round(grid * a) * gridrcp;
            b = round(grid * b) * gridrcp;
#endif

            // compute the error
            Vector3 e1 = a * a*alpha2_sum + b * b*beta2_sum + 2.0f*(a*b*alphabeta_sum - a * alphax_sum - b * betax_sum);

            // apply the metric to the error term
            float error = dot(e1, m_metricSqr);

            // keep the solution if it wins
            if (error < besterror)
            {
                besterror = error;
                beststart = a;
                bestend = b;
            }

            x1 += m_weighted[c0 + c1];
            w1 += m_weights[c0 + c1];
        }

        x0 += m_weighted[c0];
        w0 += m_weights[c0];
    }

    // save the block if necessary
    if (besterror < m_besterror)
    {

        *start = beststart;
        *end = bestend;

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::compress4(Vector3 * start, Vector3 * end)
{
    const uint count = m_count;
    const Vector3 grid = { 31.0f, 63.0f, 31.0f };
    const Vector3 gridrcp = { 1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f };

    // declare variables
    Vector3 beststart = { 0.0f };
    Vector3 bestend = { 0.0f };
    float besterror = FLT_MAX;

    Vector3 x0 = { 0.0f };
    float w0 = 0.0f;

    // check all possible clusters for this total order
    for (uint c0 = 0; c0 <= count; c0++)
    {
        Vector3 x1 = { 0.0f };
        float w1 = 0.0f;

        for (uint c1 = 0; c1 <= count - c0; c1++)
        {
            Vector3 x2 = { 0.0f };
            float w2 = 0.0f;

            for (uint c2 = 0; c2 <= count - c0 - c1; c2++)
            {
                float w3 = m_wsum - w0 - w1 - w2;

                float const alpha2_sum = w0 + w1 * (4.0f / 9.0f) + w2 * (1.0f / 9.0f);
                float const beta2_sum = w3 + w2 * (4.0f / 9.0f) + w1 * (1.0f / 9.0f);
                float const alphabeta_sum = (w1 + w2) * (2.0f / 9.0f);
                float const factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

                Vector3 const alphax_sum = x0 + x1 * (2.0f / 3.0f) + x2 * (1.0f / 3.0f);
                Vector3 const betax_sum = m_xsum - alphax_sum;

                Vector3 a = (alphax_sum*beta2_sum - betax_sum * alphabeta_sum)*factor;
                Vector3 b = (betax_sum*alpha2_sum - alphax_sum * alphabeta_sum)*factor;

                // clamp to the grid
                a = saturate(a);
                b = saturate(b);
#if ICBC_PERFECT_ROUND
                a = round565(a);
                b = round565(b);
#else
                a = round(grid * a) * gridrcp;
                b = round(grid * b) * gridrcp;
#endif
                // @@ It would be much more accurate to evaluate the error exactly. 

                // compute the error
                Vector3 e1 = a * a*alpha2_sum + b * b*beta2_sum + 2.0f*(a*b*alphabeta_sum - a * alphax_sum - b * betax_sum);

                // apply the metric to the error term
                float error = dot(e1, m_metricSqr);

                // keep the solution if it wins
                if (error < besterror)
                {
                    besterror = error;
                    beststart = a;
                    bestend = b;
                }

                x2 += m_weighted[c0 + c1 + c2];
                w2 += m_weights[c0 + c1 + c2];
            }

            x1 += m_weighted[c0 + c1];
            w1 += m_weights[c0 + c1];
        }

        x0 += m_weighted[c0];
        w0 += m_weights[c0];
    }

    // save the block if necessary
    if (besterror < m_besterror)
    {
        *start = beststart;
        *end = bestend;

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::fastCompress3(Vector3 * start, Vector3 * end)
{
    const uint count = m_count;
    const Vector3 grid = { 31.0f, 63.0f, 31.0f };
    const Vector3 gridrcp = { 1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f };

    // declare variables
    Vector3 beststart = { 0.0f };
    Vector3 bestend = { 0.0f };
    float besterror = FLT_MAX;

    Vector3 x0 = { 0.0f };
    float w0 = 0.0f;

    // check all possible clusters for this total order
    for (uint c0 = 0, i = 0; c0 <= count; c0++)
    {
        Vector3 x1 = { 0.0f };
        float w1 = 0.0f;

        for (uint c1 = 0; c1 <= count - c0; c1++, i++)
        {
            float const alpha2_sum = s_threeElement[i].alpha2_sum;
            float const beta2_sum = s_threeElement[i].beta2_sum;
            float const alphabeta_sum = s_threeElement[i].alphabeta_sum;
            float const factor = s_threeElement[i].factor;

            Vector3 const alphax_sum = x0 + x1 * 0.5f;
            Vector3 const betax_sum = m_xsum - alphax_sum;

            Vector3 a = (alphax_sum*beta2_sum - betax_sum * alphabeta_sum) * factor;
            Vector3 b = (betax_sum*alpha2_sum - alphax_sum * alphabeta_sum) * factor;

            // clamp to the grid
            a = saturate(a);
            b = saturate(b);
#if ICBC_PERFECT_ROUND
            a = round565(a);
            b = round565(b);
#else
            a = round(grid * a) * gridrcp;
            b = round(grid * b) * gridrcp;
#endif

            // compute the error
            Vector3 e1 = a * a*alpha2_sum + b * b*beta2_sum + 2.0f*(a*b*alphabeta_sum - a * alphax_sum - b * betax_sum);

            // apply the metric to the error term
            float error = dot(e1, m_metricSqr);

            // keep the solution if it wins
            if (error < besterror)
            {
                besterror = error;
                beststart = a;
                bestend = b;
            }

            x1 += m_weighted[c0 + c1];
            w1 += m_weights[c0 + c1];
        }

        x0 += m_weighted[c0];
        w0 += m_weights[c0];
    }

    // save the block if necessary
    if (besterror < m_besterror)
    {

        *start = beststart;
        *end = bestend;

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::fastCompress4(Vector3 * start, Vector3 * end)
{
    const uint count = m_count;
    const Vector3 grid = { 31.0f, 63.0f, 31.0f };
    const Vector3 gridrcp = { 1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f };

    // declare variables
    Vector3 beststart = { 0.0f };
    Vector3 bestend = { 0.0f };
    float besterror = FLT_MAX;

    Vector3 x0 = { 0.0f };
    float w0 = 0.0f;

    // check all possible clusters for this total order
    for (uint c0 = 0, i = 0; c0 <= count; c0++)
    {
        Vector3 x1 = { 0.0f };
        float w1 = 0.0f;

        for (uint c1 = 0; c1 <= count - c0; c1++)
        {
            Vector3 x2 = { 0.0f };
            float w2 = 0.0f;

            for (uint c2 = 0; c2 <= count - c0 - c1; c2++, i++)
            {
                float const alpha2_sum = s_fourElement[i].alpha2_sum;
                float const beta2_sum = s_fourElement[i].beta2_sum;
                float const alphabeta_sum = s_fourElement[i].alphabeta_sum;
                float const factor = s_fourElement[i].factor;

                Vector3 const alphax_sum = x0 + x1 * (2.0f / 3.0f) + x2 * (1.0f / 3.0f);
                Vector3 const betax_sum = m_xsum - alphax_sum;

                Vector3 a = (alphax_sum*beta2_sum - betax_sum * alphabeta_sum)*factor;
                Vector3 b = (betax_sum*alpha2_sum - alphax_sum * alphabeta_sum)*factor;

                // clamp to the grid
                a = saturate(a);
                b = saturate(b);
#if ICBC_PERFECT_ROUND
                a = round565(a);
                b = round565(b);
#else
                a = round(grid * a) * gridrcp;
                b = round(grid * b) * gridrcp;
#endif
                // @@ It would be much more accurate to evaluate the error exactly. 

                // compute the error
                Vector3 e1 = a * a*alpha2_sum + b * b*beta2_sum + 2.0f*(a*b*alphabeta_sum - a * alphax_sum - b * betax_sum);

                // apply the metric to the error term
                float error = dot(e1, m_metricSqr);

                // keep the solution if it wins
                if (error < besterror)
                {
                    besterror = error;
                    beststart = a;
                    bestend = b;
                }

                x2 += m_weighted[c0 + c1 + c2];
                w2 += m_weights[c0 + c1 + c2];
            }

            x1 += m_weighted[c0 + c1];
            w1 += m_weights[c0 + c1];
        }

        x0 += m_weighted[c0];
        w0 += m_weights[c0];
    }

    // save the block if necessary
    if (besterror < m_besterror)
    {
        *start = beststart;
        *end = bestend;

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

#endif // ICBC_USE_SIMD


///////////////////////////////////////////////////////////////////////////////////////////////////
// Palette evaluation.

// D3D10
inline void evaluate_palette4_d3d10(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[2].r = (2 * palette[0].r + palette[1].r) / 3;
    palette[2].g = (2 * palette[0].g + palette[1].g) / 3;
    palette[2].b = (2 * palette[0].b + palette[1].b) / 3;
    palette[2].a = 0xFF;

    palette[3].r = (2 * palette[1].r + palette[0].r) / 3;
    palette[3].g = (2 * palette[1].g + palette[0].g) / 3;
    palette[3].b = (2 * palette[1].b + palette[0].b) / 3;
    palette[3].a = 0xFF;
}
inline void evaluate_palette3_d3d10(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[2].r = (palette[0].r + palette[1].r) / 2;
    palette[2].g = (palette[0].g + palette[1].g) / 2;
    palette[2].b = (palette[0].b + palette[1].b) / 2;
    palette[2].a = 0xFF;
    palette[3].u = 0;
}
static void evaluate_palette_d3d10(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[0] = bitexpand_color16_to_color32(c0);
    palette[1] = bitexpand_color16_to_color32(c1);
    if (c0.u > c1.u) {
        evaluate_palette4_d3d10(c0, c1, palette);
    }
    else {
        evaluate_palette3_d3d10(c0, c1, palette);
    }
}

// NV
inline void evaluate_palette4_nv(Color16 c0, Color16 c1, Color32 palette[4]) {
    int gdiff = palette[1].g - palette[0].g;
    palette[2].r = ((2 * c0.r + c1.r) * 22) / 8;
    palette[2].g = (256 * palette[0].g + gdiff / 4 + 128 + gdiff * 80) / 256;
    palette[2].b = ((2 * c0.b + c1.b) * 22) / 8;
    palette[2].a = 0xFF;

    palette[3].r = ((2 * c1.r + c0.r) * 22) / 8;
    palette[3].g = (256 * palette[1].g - gdiff / 4 + 128 - gdiff * 80) / 256;
    palette[3].b = ((2 * c1.b + c0.b) * 22) / 8;
    palette[3].a = 0xFF;
}
inline void evaluate_palette3_nv(Color16 c0, Color16 c1, Color32 palette[4]) {
    int gdiff = palette[1].g - palette[0].g;
    palette[2].r = ((c0.r + c1.r) * 33) / 8;
    palette[2].g = (256 * palette[0].g + gdiff / 4 + 128 + gdiff * 128) / 256;
    palette[2].b = ((c0.b + c1.b) * 33) / 8;
    palette[2].a = 0xFF;
    palette[3].u = 0;
}
static void evaluate_palette_nv(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[0] = bitexpand_color16_to_color32(c0);
    palette[1] = bitexpand_color16_to_color32(c1);

    if (c0.u > c1.u) {
        evaluate_palette4_nv(c0, c1, palette);
    }
    else {
        evaluate_palette3_nv(c0, c1, palette);
    }
}

// AMD
inline void evaluate_palette4_amd(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[2].r = (43 * palette[0].r + 21 * palette[1].r + 32) / 8;
    palette[2].g = (43 * palette[0].g + 21 * palette[1].g + 32) / 8;
    palette[2].b = (43 * palette[0].b + 21 * palette[1].b + 32) / 8;
    palette[2].a = 0xFF;

    palette[3].r = (43 * palette[1].r + 21 * palette[0].r + 32) / 8;
    palette[3].g = (43 * palette[1].g + 21 * palette[0].g + 32) / 8;
    palette[3].b = (43 * palette[1].b + 21 * palette[0].b + 32) / 8;
    palette[3].a = 0xFF;
}
inline void evaluate_palette3_amd(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[2].r = (c0.r + c1.r + 1) / 2;
    palette[2].g = (c0.g + c1.g + 1) / 2;
    palette[2].b = (c0.b + c1.b + 1) / 2;
    palette[2].a = 0xFF;
    palette[3].u = 0;
}
static void evaluate_palette_amd(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[0] = bitexpand_color16_to_color32(c0);
    palette[1] = bitexpand_color16_to_color32(c1);

    if (c0.u > c1.u) {
        evaluate_palette4_amd(c0, c1, palette);
    }
    else {
        evaluate_palette3_amd(c0, c1, palette);
    }
}

// Use ICBC_DECODER to determine decoder used.
inline void evaluate_palette4(Color16 c0, Color16 c1, Color32 palette[4]) {
#if ICBC_DECODER == Decoder_D3D10
    evaluate_palette4_d3d10(c0, c1, palette);
#elif ICBC_DECODER == Decoder_NVIDIA
    evaluate_palette4_nv(c0, c1, palette);
#elif ICBC_DECODER == Decoder_AMD
    evaluate_palette4_amd(c0, c1, palette);
#endif
}
inline void evaluate_palette3(Color16 c0, Color16 c1, Color32 palette[4]) {
#if ICBC_DECODER == Decoder_D3D10
    evaluate_palette3_d3d10(c0, c1, palette);
#elif ICBC_DECODER == Decoder_NVIDIA
    evaluate_palette3_nv(c0, c1, palette);
#elif ICBC_DECODER == Decoder_AMD
    evaluate_palette3_amd(c0, c1, palette);
#endif
}
inline void evaluate_palette(Color16 c0, Color16 c1, Color32 palette[4]) {
#if ICBC_DECODER == Decoder_D3D10
    evaluate_palette_d3d10(c0, c1, palette);
#elif ICBC_DECODER == Decoder_NVIDIA
    evaluate_palette_nv(c0, c1, palette);
#elif ICBC_DECODER == Decoder_AMD
    evaluate_palette_amd(c0, c1, palette);
#endif
}

static void evaluate_palette(Color16 c0, Color16 c1, Vector3 palette[4]) {
    Color32 palette32[4];
    evaluate_palette(c0, c1, palette32);

    for (int i = 0; i < 4; i++) {
        palette[i] = color_to_vector3(palette32[i]);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Error evaluation.

// Different ways of estimating the error.

static float evaluate_mse(const Vector3 & p, const Vector3 & c, const Vector3 & w) {
    Vector3 d = (p - c) * w * 255;
    return dot(d, d);
}

static float evaluate_mse(const Color32 & p, const Vector3 & c, const Vector3 & w) {
    Vector3 d = (color_to_vector3(p) - c) * w * 255;
    return dot(d, d);
}


/*static float evaluate_mse(const Vector3 & p, const Vector3 & c, const Vector3 & w) {
    return ww.x * square(p.x-c.x) + ww.y * square(p.y-c.y) + ww.z * square(p.z-c.z);
}*/

static int evaluate_mse(const Color32 & p, const Color32 & c) {
    return (square(int(p.r)-c.r) + square(int(p.g)-c.g) + square(int(p.b)-c.b));
}

/*static float evaluate_mse(const Vector3 palette[4], const Vector3 & c, const Vector3 & w) {
    float e0 = evaluate_mse(palette[0], c, w);
    float e1 = evaluate_mse(palette[1], c, w);
    float e2 = evaluate_mse(palette[2], c, w);
    float e3 = evaluate_mse(palette[3], c, w);
    return min(min(e0, e1), min(e2, e3));
}*/

static int evaluate_mse(const Color32 palette[4], const Color32 & c) {
    int e0 = evaluate_mse(palette[0], c);
    int e1 = evaluate_mse(palette[1], c);
    int e2 = evaluate_mse(palette[2], c);
    int e3 = evaluate_mse(palette[3], c);
    return min(min(e0, e1), min(e2, e3));
}

// Returns MSE error in [0-255] range.
static int evaluate_mse(const BlockDXT1 * output, Color32 color, int index) {
    Color32 palette[4];
    evaluate_palette(output->col0, output->col1, palette);

    return evaluate_mse(palette[index], color);
}

// Returns weighted MSE error in [0-255] range.
static float evaluate_palette_error(Color32 palette[4], const Color32 * colors, const float * weights, int count) {
    
    float total = 0.0f;
    for (int i = 0; i < count; i++) {
        total += weights[i] * evaluate_mse(palette, colors[i]);
    }

    return total;
}

static float evaluate_palette_error(Color32 palette[4], const Color32 * colors, int count) {

    float total = 0.0f;
    for (int i = 0; i < count; i++) {
        total += evaluate_mse(palette, colors[i]);
    }

    return total;
}

#if 0
static float evaluate_mse(const BlockDXT1 * output, const Vector3 colors[16]) {
    Color32 palette[4];
    output->evaluatePalette(palette, /*d3d9=*/false);

    // convert palette to float.
    Vector3 vector_palette[4];
    for (int i = 0; i < 4; i++) {
        vector_palette[i] = color_to_vector3(palette[i]);
    }

    // evaluate error for each index.
    float error = 0.0f;
    for (int i = 0; i < 16; i++) {
        int index = (output->indices >> (2*i)) & 3;
        error += evaluate_mse(vector_palette[index], colors[i]);
    }

    return error;
}
#endif

static float evaluate_mse(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, const BlockDXT1 * output) {
    Color32 palette[4];
    evaluate_palette(output->col0, output->col1, palette);

    // convert palette to float.
    /*Vector3 vector_palette[4];
    for (int i = 0; i < 4; i++) {
        vector_palette[i] = color_to_vector3(palette[i]);
    }*/

    // evaluate error for each index.
    float error = 0.0f;
    for (int i = 0; i < 16; i++) {
        int index = (output->indices >> (2 * i)) & 3;
        error += input_weights[i] * evaluate_mse(palette[index], input_colors[i].xyz, color_weights);
    }
    return error;
}

float evaluate_dxt1_error(const uint8 rgba_block[16*4], const BlockDXT1 * block, Decoder decoder) {
    Color32 palette[4];
    if (decoder == Decoder_NVIDIA) {
        evaluate_palette_nv(block->col0, block->col1, palette);
    }
    else if (decoder == Decoder_AMD) {
        evaluate_palette_amd(block->col0, block->col1, palette);
    }
    else {
        evaluate_palette(block->col0, block->col1, palette);
    }

    // evaluate error for each index.
    float error = 0.0f;
    for (int i = 0; i < 16; i++) {
        int index = (block->indices >> (2 * i)) & 3;
        Color32 c;
        c.r = rgba_block[4 * i + 0];
        c.g = rgba_block[4 * i + 1];
        c.b = rgba_block[4 * i + 2];
        c.a = 255;
        error += evaluate_mse(palette[index], c);
    }
    return error;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Index selection

static uint compute_indices4(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 palette[4]) {
    
    uint indices = 0;
    for (int i = 0; i < 16; i++) {
        float d0 = evaluate_mse(palette[0], input_colors[i].xyz, color_weights);
        float d1 = evaluate_mse(palette[1], input_colors[i].xyz, color_weights);
        float d2 = evaluate_mse(palette[2], input_colors[i].xyz, color_weights);
        float d3 = evaluate_mse(palette[3], input_colors[i].xyz, color_weights);

        uint b0 = d0 > d3;
        uint b1 = d1 > d2;
        uint b2 = d0 > d2;
        uint b3 = d1 > d3;
        uint b4 = d2 > d3;

        uint x0 = b1 & b2;
        uint x1 = b0 & b3;
        uint x2 = b0 & b4;

        indices |= (x2 | ((x0 | x1) << 1)) << (2 * i);
    }

    return indices;
}


static uint compute_indices4(const Vector3 input_colors[16], const Vector3 palette[4]) {

    uint indices = 0;
    for (int i = 0; i < 16; i++) {
        float d0 = evaluate_mse(palette[0], input_colors[i], {1,1,1});
        float d1 = evaluate_mse(palette[1], input_colors[i], {1,1,1});
        float d2 = evaluate_mse(palette[2], input_colors[i], {1,1,1});
        float d3 = evaluate_mse(palette[3], input_colors[i], {1,1,1});

        uint b0 = d0 > d3;
        uint b1 = d1 > d2;
        uint b2 = d0 > d2;
        uint b3 = d1 > d3;
        uint b4 = d2 > d3;

        uint x0 = b1 & b2;
        uint x1 = b0 & b3;
        uint x2 = b0 & b4;

        indices |= (x2 | ((x0 | x1) << 1)) << (2 * i);
    }

    return indices;
}


static uint compute_indices(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 palette[4]) {
    
    uint indices = 0;
    for (int i = 0; i < 16; i++) {
        float d0 = evaluate_mse(palette[0], input_colors[i].xyz, color_weights);
        float d1 = evaluate_mse(palette[1], input_colors[i].xyz, color_weights);
        float d2 = evaluate_mse(palette[2], input_colors[i].xyz, color_weights);
        float d3 = evaluate_mse(palette[3], input_colors[i].xyz, color_weights);

        uint index;
        if (d0 < d1 && d0 < d2 && d0 < d3) index = 0;
        else if (d1 < d2 && d1 < d3) index = 1;
        else if (d2 < d3) index = 2;
        else index = 3;

		indices |= index << (2 * i);
	}

	return indices;
}


static void output_block3(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 & v0, const Vector3 & v1, BlockDXT1 * block)
{
    Color16 color0 = vector3_to_color16(v0);
    Color16 color1 = vector3_to_color16(v1);

    if (color0.u > color1.u) {
        swap(color0, color1);
    }

    Vector3 palette[4];
    evaluate_palette(color0, color1, palette);

    block->col0 = color0;
    block->col1 = color1;
    block->indices = compute_indices(input_colors, color_weights, palette);
}

static void output_block4(const Vector4 input_colors[16], const Vector3 & color_weights, const Vector3 & v0, const Vector3 & v1, BlockDXT1 * block)
{
    Color16 color0 = vector3_to_color16(v0);
    Color16 color1 = vector3_to_color16(v1);

    if (color0.u < color1.u) {
        swap(color0, color1);
    }

    Vector3 palette[4];
    evaluate_palette(color0, color1, palette);

    block->col0 = color0;
    block->col1 = color1;
    block->indices = compute_indices4(input_colors, color_weights, palette);
}


static void output_block4(const Vector3 input_colors[16], const Vector3 & v0, const Vector3 & v1, BlockDXT1 * block)
{
    Color16 color0 = vector3_to_color16(v0);
    Color16 color1 = vector3_to_color16(v1);

    if (color0.u < color1.u) {
        swap(color0, color1);
    }

    Vector3 palette[4];
    evaluate_palette(color0, color1, palette);

    block->col0 = color0;
    block->col1 = color1;
    block->indices = compute_indices4(input_colors, palette);
}

// Least squares fitting of color end points for the given indices. @@ Take weights into account.
static bool optimize_end_points4(uint indices, const Vector4 * colors, /*const float * weights,*/ int count, Vector3 * a, Vector3 * b)
{
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    Vector3 alphax_sum = { 0,0,0 };
    Vector3 betax_sum = { 0,0,0 };

    for (int i = 0; i < count; i++)
    {
        const uint bits = indices >> (2 * i);

        float beta = float(bits & 1);
        if (bits & 2) beta = (1 + beta) / 3.0f;
        float alpha = 1.0f - beta;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i].xyz;
        betax_sum += beta * colors[i].xyz;
    }

    float denom = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
    if (equal(denom, 0.0f)) return false;

    float factor = 1.0f / denom;

    *a = saturate((alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor);
    *b = saturate((betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor);

    return true;
}

// Least squares optimization with custom factors.
// This allows us passing the standard [1, 0, 2/3 1/3] weights by default, but also use alternative mappings when the number of clusters is not 4.
static bool optimize_end_points4(uint indices, const Vector3 * colors, int count, float factors[4], Vector3 * a, Vector3 * b)
{
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    Vector3 alphax_sum = { 0,0,0 };
    Vector3 betax_sum = { 0,0,0 };

    for (int i = 0; i < count; i++)
    {
        const uint idx = (indices >> (2 * i)) & 3;
        float alpha = factors[idx];
        float beta = 1 - alpha;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i];
        betax_sum += beta * colors[i];
    }

    float denom = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
    if (equal(denom, 0.0f)) return false;

    float factor = 1.0f / denom;

    *a = saturate((alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor);
    *b = saturate((betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor);

    return true;
}

static bool optimize_end_points4(uint indices, const Vector3 * colors, int count, Vector3 * a, Vector3 * b)
{
    float factors[4] = { 1, 0, 2.f / 3, 1.f / 3 };
    return optimize_end_points4(indices, colors, count, factors, a, b);
}


// Least squares fitting of color end points for the given indices. @@ This does not support black/transparent index. @@ Take weights into account.
static bool optimize_end_points3(uint indices, const Vector3 * colors, /*const float * weights,*/ int count, Vector3 * a, Vector3 * b)
{
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    Vector3 alphax_sum = { 0,0,0 };
    Vector3 betax_sum = { 0,0,0 };

    for (int i = 0; i < count; i++)
    {
        const uint bits = indices >> (2 * i);

        float beta = float(bits & 1);
        if (bits & 2) beta = 0.5f;
        float alpha = 1.0f - beta;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i];
        betax_sum += beta * colors[i];
    }

    float denom = alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum;
    if (equal(denom, 0.0f)) return false;

    float factor = 1.0f / denom;

    *a = saturate((alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor);
    *b = saturate((betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor);

    return true;
}


// find minimum and maximum colors based on bounding box in color space
inline static void fit_colors_bbox(const Vector3 * colors, int count, Vector3 * __restrict c0, Vector3 * __restrict c1)
{
    *c0 = { 0,0,0 };
    *c1 = { 1,1,1 };

    for (int i = 0; i < count; i++) {
        *c0 = max(*c0, colors[i]);
        *c1 = min(*c1, colors[i]);
    }
}

inline static void select_diagonal(const Vector3 * colors, int count, Vector3 * __restrict c0, Vector3 * __restrict c1)
{
    Vector3 center = (*c0 + *c1) * 0.5f;

    /*Vector3 center = colors[0];
    for (int i = 1; i < count; i++) {
        center = center * float(i-1) / i + colors[i] / i;
    }*/
    /*Vector3 center = colors[0];
    for (int i = 1; i < count; i++) {
        center += colors[i];
    }
    center /= count;*/

    float cov_xz = 0.0f;
    float cov_yz = 0.0f;
    for (int i = 0; i < count; i++) {
        Vector3 t = colors[i] - center;
        cov_xz += t.x * t.z;
        cov_yz += t.y * t.z;
    }

    float x0 = c0->x;
    float y0 = c0->y;
    float x1 = c1->x;
    float y1 = c1->y;

    if (cov_xz < 0) {
        swap(x0, x1);
    }
    if (cov_yz < 0) {
        swap(y0, y1);
    }

    *c0 = { x0, y0, c0->z };
    *c1 = { x1, y1, c1->z };
}

inline static void inset_bbox(Vector3 * __restrict c0, Vector3 * __restrict c1)
{
    float bias = (8.0f / 255.0f) / 16.0f;
    Vector3 inset = (*c0 - *c1) / 16.0f - scalar_to_vector3(bias);
    *c0 = saturate(*c0 - inset);
    *c1 = saturate(*c1 + inset);
}



// Single color lookup tables from:
// https://github.com/nothings/stb/blob/master/stb_dxt.h
static uint8 match5[256][2];
static uint8 match6[256][2];

static inline int Lerp13(int a, int b)
{
    // replace "/ 3" by "* 0xaaab) >> 17" if your compiler sucks or you really need every ounce of speed.
    return (a * 2 + b) / 3;
}

static void PrepareOptTable(uint8 * table, const uint8 * expand, int size)
{
    for (int i = 0; i < 256; i++) {
        int bestErr = 256 * 100;

        for (int min = 0; min < size; min++) {
            for (int max = 0; max < size; max++) {
                int mine = expand[min];
                int maxe = expand[max];

                int err = abs(Lerp13(maxe, mine) - i) * 100;

                // DX10 spec says that interpolation must be within 3% of "correct" result,
                // add this as error term. (normally we'd expect a random distribution of
                // +-1.5% error, but nowhere in the spec does it say that the error has to be
                // unbiased - better safe than sorry).
                err += abs(max - min) * 3;

                if (err < bestErr) {
                    bestErr = err;
                    table[i * 2 + 0] = max;
                    table[i * 2 + 1] = min;
                }
            }
        }
    }
}

static void init_dxt1_tables()
{
    // Prepare single color lookup tables.
    uint8 expand5[32];
    uint8 expand6[64];
    for (int i = 0; i < 32; i++) expand5[i] = (i << 3) | (i >> 2);
    for (int i = 0; i < 64; i++) expand6[i] = (i << 2) | (i >> 4);

    PrepareOptTable(&match5[0][0], expand5, 32);
    PrepareOptTable(&match6[0][0], expand6, 64);
}

// Single color compressor, based on:
// https://mollyrocket.com/forums/viewtopic.php?t=392
static void compress_dxt1_single_color_optimal(Color32 c, BlockDXT1 * output)
{
    output->col0.r = match5[c.r][0];
    output->col0.g = match6[c.g][0];
    output->col0.b = match5[c.b][0];
    output->col1.r = match5[c.r][1];
    output->col1.g = match6[c.g][1];
    output->col1.b = match5[c.b][1];
    output->indices = 0xaaaaaaaa;
    
    if (output->col0.u < output->col1.u)
    {
        swap(output->col0.u, output->col1.u);
        output->indices ^= 0x55555555;
    }
}


// Compress block using the average color.
static float compress_dxt1_single_color(const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, BlockDXT1 * output)
{
    // Compute block average.
    Vector3 color_sum = { 0,0,0 };
    float weight_sum = 0;

    for (int i = 0; i < count; i++) {
        color_sum += colors[i] * weights[i];
        weight_sum += weights[i];
    }

    // Compress optimally.
    compress_dxt1_single_color_optimal(vector3_to_color32(color_sum / weight_sum), output);

    // Decompress block color.
    Color32 palette[4];
    evaluate_palette(output->col0, output->col1, palette);

    Vector3 block_color = color_to_vector3(palette[output->indices & 0x3]);

    // Evaluate error.
    float error = 0;
    for (int i = 0; i < count; i++) {
        error += weights[i] * evaluate_mse(block_color, colors[i], color_weights);
    }
    return error;
}


static float compress_dxt1_bounding_box_exhaustive(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, int max_volume, BlockDXT1 * output)
{
    // Compute bounding box.
    Vector3 min_color = { 1,1,1 };
    Vector3 max_color = { 0,0,0 };

    for (int i = 0; i < count; i++) {
        min_color = min(min_color, colors[i]);
        max_color = max(max_color, colors[i]);
    }

    // Convert to 5:6:5
    int min_r = int(31 * min_color.x);
    int min_g = int(63 * min_color.y);
    int min_b = int(31 * min_color.z);
    int max_r = int(31 * max_color.x + 1);
    int max_g = int(63 * max_color.y + 1);
    int max_b = int(31 * max_color.z + 1);

    // Expand the box.
    int range_r = max_r - min_r;
    int range_g = max_g - min_g;
    int range_b = max_b - min_b;

    min_r = max(0, min_r - range_r / 2 - 2);
    min_g = max(0, min_g - range_g / 2 - 2);
    min_b = max(0, min_b - range_b / 2 - 2);

    max_r = min(31, max_r + range_r / 2 + 2);
    max_g = min(63, max_g + range_g / 2 + 2);
    max_b = min(31, max_b + range_b / 2 + 2);

    // Estimate size of search space.
    int volume = (max_r-min_r+1) * (max_g-min_g+1) * (max_b-min_b+1);

    // if size under search_limit, then proceed. Note that search_volume is sqrt of number of evaluations.
    if (volume > max_volume) {
        return FLT_MAX;
    }

    // @@ Convert to fixed point before building box?
    Color32 colors32[16];
    for (int i = 0; i < count; i++) {
        colors32[i] = vector3_to_color32(colors[i]);
    }

    float best_error = FLT_MAX;
    Color16 best0, best1;           // @@ Record endpoints as Color16?

    Color16 c0, c1;
    Color32 palette[4];

    for(int r0 = min_r; r0 <= max_r; r0++)
    for(int g0 = min_g; g0 <= max_g; g0++)
    for(int b0 = min_b; b0 <= max_b; b0++)
    {
        c0.r = r0; c0.g = g0; c0.b = b0;
        palette[0] = bitexpand_color16_to_color32(c0);

        for(int r1 = min_r; r1 <= max_r; r1++)
        for(int g1 = min_g; g1 <= max_g; g1++)
        for(int b1 = min_b; b1 <= max_b; b1++)
        {
            c1.r = r1; c1.g = g1; c1.b = b1;
            palette[1] = bitexpand_color16_to_color32(c1);

            if (c0.u > c1.u) {
                // Evaluate error in 4 color mode.
                evaluate_palette4(c0, c1, palette);
            }
            else {
                if (three_color_mode) {
                    // Evaluate error in 3 color mode.
                    evaluate_palette3(c0, c1, palette);
                }
                else {
                    // Skip 3 color mode.
                    continue;
                }
            }

            float error = evaluate_palette_error(palette, colors32, weights, count);

            if (error < best_error) {
                best_error = error;
                best0 = c0;
                best1 = c1;
            }
        }
    }

    output->col0 = best0;
    output->col1 = best1;

    Vector3 vector_palette[4];
    evaluate_palette(output->col0, output->col1, vector_palette);

    output->indices = compute_indices(input_colors, color_weights, vector_palette);

    return best_error / (255 * 255);
}


static void compress_dxt1_cluster_fit(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, BlockDXT1 * output)
{
    ClusterFit fit;
    
#if ICBC_FAST_CLUSTER_FIT
    if (count > 15) {
        fit.setColorSet(input_colors, color_weights);

        // start & end are in [0, 1] range.
        Vector3 start, end;
        fit.fastCompress4(&start, &end);

        if (three_color_mode && fit.fastCompress3(&start, &end)) {
            output_block3(input_colors, color_weights, start, end, output);
        }
        else {
            output_block4(input_colors, color_weights, start, end, output);
        }
    }
    else 
#endif
    {
        fit.setColorSet(colors, weights, count, color_weights);

        // start & end are in [0, 1] range.
        Vector3 start, end;
        fit.compress4(&start, &end);

        if (three_color_mode && fit.compress3(&start, &end)) {
            output_block3(input_colors, color_weights, start, end, output);
        }
        else {
            output_block4(input_colors, color_weights, start, end, output);
        }
    }
}


static float refine_endpoints(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, bool three_color_mode, float input_error, BlockDXT1 * output) {
    // TODO:
    // - Optimize palette evaluation when updating only one channel.
    // - try all diagonals.

    // Things that don't help:
    // - Alternate endpoint updates.
    // - Randomize order.
    // - If one direction does not improve, test opposite direction next.

    static const int8 deltas[16][3] = {
        {1,0,0},
        {0,1,0},
        {0,0,1},

        {-1,0,0},
        {0,-1,0},
        {0,0,-1},

        {1,1,0},
        {1,0,1},
        {0,1,1},

        {-1,-1,0},
        {-1,0,-1},
        {0,-1,-1},

        {-1,1,0},
        //{-1,0,1},

        {1,-1,0},
        {0,-1,1},

        //{1,0,-1},
        {0,1,-1},
    };

    float best_error = input_error;

    int lastImprovement = 0;
    for (int i = 0; i < 256; i++) {
        BlockDXT1 refined = *output;
        int8 delta[3] = { deltas[i % 16][0], deltas[i % 16][1], deltas[i % 16][2] };

        if ((i / 16) & 1) {
            refined.col0.r += delta[0];
            refined.col0.g += delta[1];
            refined.col0.b += delta[2];
        }
        else {
            refined.col1.r += delta[0];
            refined.col1.g += delta[1];
            refined.col1.b += delta[2];
        }

        if (!three_color_mode) {
            if (refined.col0.u == refined.col1.u) refined.col1.g += 1;
            if (refined.col0.u < refined.col1.u) swap(refined.col0.u, refined.col1.u);
        }

        Vector3 palette[4];
        evaluate_palette(output->col0, output->col1, palette);

        refined.indices = compute_indices(input_colors, color_weights, palette);

        float refined_error = evaluate_mse(input_colors, input_weights, color_weights, &refined);
        if (refined_error < best_error) {
            best_error = refined_error;
            *output = refined;
            lastImprovement = i;
        }

        // Early out if the last 32 steps didn't improve error.
        if (i - lastImprovement > 32) break;
    }

    return best_error;
}


static float compress_dxt1(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, bool three_color_mode, bool hq, BlockDXT1 * output)
{
    Vector3 colors[16];
    float weights[16];
    int count = reduce_colors(input_colors, input_weights, colors, weights);

    if (count == 0) {
        // Output trivial block.
        output->col0.u = 0;
        output->col1.u = 0;
        output->indices = 0;
        return 0;
    }

    // Cluster fit cannot handle single color blocks, so encode them optimally.
    if (count == 1) {
        compress_dxt1_single_color_optimal(vector3_to_color32(colors[0]), output);
        return evaluate_mse(input_colors, input_weights, color_weights, output);
    }

    // Quick end point selection.
    Vector3 c0, c1;
    fit_colors_bbox(colors, count, &c0, &c1);
    inset_bbox(&c0, &c1);
    select_diagonal(colors, count, &c0, &c1);
    output_block4(input_colors, color_weights, c0, c1, output);

    float error = evaluate_mse(input_colors, input_weights, color_weights, output);

    // Refine color for the selected indices.
    if (optimize_end_points4(output->indices, input_colors, 16, &c0, &c1)) {
        BlockDXT1 optimized_block;
        output_block4(input_colors, color_weights, c0, c1, &optimized_block);

        float optimized_error = evaluate_mse(input_colors, input_weights, color_weights, &optimized_block);
        if (optimized_error < error) {
            error = optimized_error;
            *output = optimized_block;
        }
    }

    // @@ Use current endpoints as input for initial PCA approximation?

    // Try cluster fit.
    BlockDXT1 cluster_fit_output;
    compress_dxt1_cluster_fit(input_colors, colors, weights, count, color_weights, three_color_mode, &cluster_fit_output);

    float cluster_fit_error = evaluate_mse(input_colors, input_weights, color_weights, &cluster_fit_output);
    if (cluster_fit_error < error) {
        *output = cluster_fit_output;
        error = cluster_fit_error;
    }

    if (hq) {
        error = refine_endpoints(input_colors, input_weights, color_weights, three_color_mode, error, output);
    }

    return error;
}


// 
static bool centroid_end_points(uint indices, const Vector3 * colors, /*const float * weights,*/ float factor[4], Vector3 * c0, Vector3 * c1) {

    *c0 = { 0,0,0 };
    *c1 = { 0,0,0 };
    float w0_sum = 0;
    float w1_sum = 0;

    for (int i = 0; i < 16; i++) {
        int idx = (indices >> (2 * i)) & 3;
        float w0 = factor[idx];// * weights[i];
        float w1 = (1 - factor[idx]);// * weights[i];

        *c0 += colors[i] * w0;   w0_sum += w0;
        *c1 += colors[i] * w1;   w1_sum += w1;
    }

    *c0 *= (1.0f / w0_sum);
    *c1 *= (1.0f / w1_sum);

    return true;
}



static float compress_dxt1_test(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, BlockDXT1 * output)
{
    Vector3 colors[16];
    for (int i = 0; i < 16; i++) {
        colors[i] = input_colors[i].xyz;
    }
    int count = 16;

    // Quick end point selection.
    Vector3 c0, c1;
    fit_colors_bbox(colors, count, &c0, &c1);
    if (c0 == c1) {
        compress_dxt1_single_color_optimal(vector3_to_color32(c0), output);
        return evaluate_mse(input_colors, input_weights, color_weights, output);
    }
    inset_bbox(&c0, &c1);
    select_diagonal(colors, count, &c0, &c1);

    output_block4(colors, c0, c1, output);
    float best_error = evaluate_mse(input_colors, input_weights, color_weights, output);


    // Given an index assignment, we can compute end points in two different ways:
    // - least squares optimization.
    // - centroid.
    // Are these different? The first finds the end points that minimize the least squares error.
    // The second averages the input colors

    while (true) {
        float last_error = best_error;
        uint last_indices = output->indices;

        int cluster_counts[4] = { 0, 0, 0, 0 };
        for (int i = 0; i < 16; i++) {
            int idx = (output->indices >> (2 * i)) & 3;
            cluster_counts[idx] += 1;
        }
        int n = 0;
        for (int i = 0; i < 4; i++) n += int(cluster_counts[i] != 0);

        if (n == 4) {
            float factors[4] = { 1.0f, 0.0f, 2.0f / 3, 1.0f / 3 };
            if (optimize_end_points4(last_indices, colors, 16, factors, &c0, &c1)) {
                BlockDXT1 refined_block;
                output_block4(colors, c0, c1, &refined_block);
                float new_error = evaluate_mse(input_colors, input_weights, color_weights, &refined_block);
                if (new_error < best_error) {
                    best_error = new_error;
                    *output = refined_block;
                }
            }
        }
        else if (n == 3) {
            // 4 options:
            static const float tables[4][3] = {
                { 0, 2.f/3, 1.f/3 },    // 0, 1/3, 2/3
                { 1, 0,     1.f/3 },    // 0, 1/3, 1
                { 1, 0,     2.f/3 },    // 0, 2/3, 1
                { 1, 2.f/3, 1.f/3 },    // 1/2, 2/3, 1
            };

            for (int k = 0; k < 4; k++) {
                // Remap tables:
                float factors[4];
                for (int i = 0, j = 0; i < 4; i++) {
                    factors[i] = tables[k][j];
                    if (cluster_counts[i] != 0) j += 1;
                }
                if (optimize_end_points4(last_indices, colors, 16, factors, &c0, &c1)) {
                    BlockDXT1 refined_block;
                    output_block4(colors, c0, c1, &refined_block);
                    float new_error = evaluate_mse(input_colors, input_weights, color_weights, &refined_block);
                    if (new_error < best_error) {
                        best_error = new_error;
                        *output = refined_block;
                    }
                }
            }

            // @@ And 1 3-color block:
            // 0, 1/2, 1
        }
        else if (n == 2) {

            // 6 options:
            static const float tables[6][2] = {
                { 0, 1.f/3 },       // 0, 1/3
                { 0, 2.f/3 },       // 0, 2/3
                { 1, 0 },           // 0, 1
                { 2.f/3, 1.f/3 },   // 1/3, 2/3
                { 1, 1.f/3 },       // 1/3, 1
                { 1, 2.f/3 },       // 2/3, 1
            };

            for (int k = 0; k < 6; k++) {
                // Remap tables:
                float factors[4];
                for (int i = 0, j = 0; i < 4; i++) {
                    factors[i] = tables[k][j];
                    if (cluster_counts[i] != 0) j += 1;
                }
                if (optimize_end_points4(last_indices, colors, 16, factors, &c0, &c1)) {
                    BlockDXT1 refined_block;
                    output_block4(colors, c0, c1, &refined_block);
                    float new_error = evaluate_mse(input_colors, input_weights, color_weights, &refined_block);
                    if (new_error < best_error) {
                        best_error = new_error;
                        *output = refined_block;
                    }
                }
            }

            // @@ And 2 3-color blocks:
            // 0, 0.5
            // 0.5, 1
            // 0, 1     // This is equivalent to the 4 color mode.
        }

        // If error has not improved, stop.
        //if (best_error == last_error) break;

        // If error has not improved or indices haven't changed, stop.
        if (output->indices == last_indices || best_error < last_error) break;
    }

    if (false) {
        best_error = refine_endpoints(input_colors, input_weights, color_weights, false, best_error, output);
    }

    return best_error;
}



static float compress_dxt1_fast(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, BlockDXT1 * output)
{
    Vector3 colors[16];
    for (int i = 0; i < 16; i++) {
        colors[i] = input_colors[i].xyz;
    }
    int count = 16;

    /*float error = FLT_MAX;
    error = compress_dxt1_single_color(colors, input_weights, count, color_weights, output);

    if (error == 0.0f || count == 1) {
        // Early out.
        return error;
    }*/

    // Quick end point selection.
    Vector3 c0, c1;
    fit_colors_bbox(colors, count, &c0, &c1);
    if (c0 == c1) {
        compress_dxt1_single_color_optimal(vector3_to_color32(c0), output);
        return evaluate_mse(input_colors, input_weights, color_weights, output);
    }
    inset_bbox(&c0, &c1);
    select_diagonal(colors, count, &c0, &c1);
    output_block4(input_colors, color_weights, c0, c1, output);

    // Refine color for the selected indices.
    if (optimize_end_points4(output->indices, input_colors, 16, &c0, &c1)) {
        output_block4(input_colors, color_weights, c0, c1, output);
    }

    return evaluate_mse(input_colors, input_weights, color_weights, output);
}


static void compress_dxt1_fast(const uint8 input_colors[16*4], BlockDXT1 * output) {

    Vector3 vec_colors[16];
    for (int i = 0; i < 16; i++) {
        vec_colors[i] = { input_colors[4 * i + 0] / 255.0f, input_colors[4 * i + 1] / 255.0f, input_colors[4 * i + 2] / 255.0f };
    }

    // Quick end point selection.
    Vector3 c0, c1;
    //fit_colors_bbox(colors, count, &c0, &c1);
    //select_diagonal(colors, count, &c0, &c1);
    fit_colors_bbox(vec_colors, 16, &c0, &c1);
    if (c0 == c1) {
        compress_dxt1_single_color_optimal(vector3_to_color32(c0), output);
        return;
    }
    inset_bbox(&c0, &c1);
    select_diagonal(vec_colors, 16, &c0, &c1);
    output_block4(vec_colors, c0, c1, output);

    // Refine color for the selected indices.
    if (optimize_end_points4(output->indices, vec_colors, 16, &c0, &c1)) {
        output_block4(vec_colors, c0, c1, output);
    }
}

// Public API

void init() {
    init_dxt1_tables();
}

float compress_dxt1(const float input_colors[16 * 4], const float input_weights[16], const float rgb[3], bool three_color_mode, bool hq, void * output) {
    return compress_dxt1((Vector4*)input_colors, input_weights, { rgb[0], rgb[1], rgb[2] }, three_color_mode, hq, (BlockDXT1*)output);
}

float compress_dxt1_fast(const float input_colors[16 * 4], const float input_weights[16], const float rgb[3], void * output) {
    return compress_dxt1_fast((Vector4*)input_colors, input_weights, { rgb[0], rgb[1], rgb[2] }, (BlockDXT1*)output);
}

void compress_dxt1_fast(const unsigned char input_colors[16 * 4], void * output) {
    compress_dxt1_fast(input_colors, (BlockDXT1*)output);
}

void compress_dxt1_test(const float input_colors[16 * 4], const float input_weights[16], const float rgb[3], void * output) {
    compress_dxt1_test((Vector4*)input_colors, input_weights, { rgb[0], rgb[1], rgb[2] }, (BlockDXT1*)output);
}

float evaluate_dxt1_error(const unsigned char rgba_block[16 * 4], const void * dxt_block, Decoder decoder/*=Decoder_D3D10*/) {
    return evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block, decoder);
}

} // icbc
#endif // ICBC_IMPLEMENTATION
