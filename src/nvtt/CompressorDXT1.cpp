
#include "CompressorDXT1.h"
#include "ClusterFit.h"

#include "nvmath/nvmath.h"

#include <string.h> // memset
#include <limits.h> // INT_MAX
#include <float.h> // FLT_MAX


using namespace nv;

/// Swap two values.
/*template <typename T>
inline void swap(T & a, T & b)
{
    T temp(a);
    a = b;
    b = temp;
}*/


///////////////////////////////////////////////////////////////////////////////////////////////////
// Basic Types

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

namespace nv {
    struct BlockDXT1 {
        Color16 col0;
        Color16 col1;
        uint32 indices;
    };


    /*struct Vector3 {
        float x, y, z;
    };*/

    inline Vector3::Vector3() {}
    inline Vector3::Vector3(float f) : x(f), y(f), z(f) {}
    inline Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    inline Vector3::Vector3(Vector3::Arg v) : x(v.x), y(v.y), z(v.z) {}

    inline const Vector3 & Vector3::operator=(Vector3::Arg v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }

    inline void Vector3::operator+=(Vector3::Arg v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
    }

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

    /*inline float saturate(float x) {
        return x < 0 ? 0 : (x > 1 ? 1 : x);
    }*/

    inline Vector3 saturate(Vector3 v) {
        return { saturate(v.x), saturate(v.y), saturate(v.z) };
    }

    inline Vector3 min(Vector3 a, Vector3 b) {
        return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z) };
    }

    inline Vector3 max(Vector3 a, Vector3 b) {
        return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z) };
    }

    inline bool operator==(const Vector3 & a, const Vector3 & b) {
        return memcmp(&a, &b, sizeof(Vector3));
    }

    inline void Vector3::set(float x, float y, float z) {
        this->x = x; this->y = y; this->z = z;
    }

    inline Vector4::Vector4(Vector3::Arg v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    inline Vector3 Vector4::xyz() const
    {
        return Vector3(x, y, z);
    }

}

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

inline Vector3 color_to_vector3(Color32 c)
{
    return Vector3(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
}

inline Color32 vector3_to_color32(Vector3 v)
{
    Color32 color;
    color.r = uint8(saturate(v.x) * 255 + 0.5f);
    color.g = uint8(saturate(v.y) * 255 + 0.5f);
    color.b = uint8(saturate(v.z) * 255 + 0.5f);
    color.a = 255;
    return color;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Input block processing.

// Find first valid color.
/*static bool find_valid_color_rgb(const Vector3 * colors, const float * weights, int count, Vector3 * valid_color)
{
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0.0f) {
            *valid_color = colors[i];
            return true;
        }
    }

    // No valid colors.
    return false;
}*/

/*static bool is_single_color_rgb(const Vector3 * colors, const float * weights, int count, Vector3 color)
{
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0.0f) {
            if (colors[i] != color) return false;
        }
    }

    return true;
}*/

// Find similar colors and combine them together.
static int reduce_colors(const Vector4 * input_colors, const float * input_weights, Vector3 * colors, float * weights)
{
    int n = 0;
    for (int i = 0; i < 16; i++)
    {
        Vector3 ci = input_colors[i].xyz();
        float wi = input_weights[i];

        if (wi > 0) {
            // Find matching color.
            int j;
            for (j = 0; j < n; j++) {
                if (equal(colors[j].x, ci.x) && equal(colors[j].y, ci.y) && equal(colors[j].z, ci.z)) {
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

    nvDebugCheck(n <= 16);

    return n;
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

        // Find matching color.
        int j;
        for (j = 0; j < n; j++) {
            if (equal(colors[j].x, ci.x) && equal(colors[j].y, ci.y) && equal(colors[j].z, ci.z)) {
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

    nvDebugCheck(n <= 16);

    return n;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Palette evaluation.

#define DECODER 0

inline void evaluate_palette4(Color16 c0, Color16 c1, Color32 palette[4], bool d3d9_bias) {
#if DECODER == 0 || DECODER == 1
    palette[2].r = (2 * palette[0].r + palette[1].r + d3d9_bias) / 3;
    palette[2].g = (2 * palette[0].g + palette[1].g + d3d9_bias) / 3;
    palette[2].b = (2 * palette[0].b + palette[1].b + d3d9_bias) / 3;
    palette[3].r = (2 * palette[1].r + palette[0].r + d3d9_bias) / 3;
    palette[3].g = (2 * palette[1].g + palette[0].g + d3d9_bias) / 3;
    palette[3].b = (2 * palette[1].b + palette[0].b + d3d9_bias) / 3;
#else
    int dg = palette[1].g - palette[0].g;
    palette[2].r = ((2 * c0.r + c1.r) * 22) / 8;
    palette[2].g = (256 * palette[0].g + dg * 80 + dg / 4 + 128) / 256;
    palette[2].b = ((2 * c0.b + c1.b) * 22) / 8;
    palette[3].r = ((2 * c1.r + c0.r) * 22) / 8;
    palette[3].g = (256 * palette[1].g - dg * 80 - dg / 4 + 128) / 256;
    palette[3].b = ((2 * c1.b + c0.b) * 22) / 8;
#endif
}

inline void evaluate_palette3(Color16 c0, Color16 c1, Color32 palette[4]) {
#if DECODER == 0 || DECODER == 1
    palette[2].r = (palette[0].r + palette[1].r) / 2;
    palette[2].g = (palette[0].g + palette[1].g) / 2;
    palette[2].b = (palette[0].b + palette[1].b) / 2;
#else
    int dg = palette[1].g - palette[0].g;
    palette[2].r = ((c0.r + c1.r) * 33) / 8;
    palette[2].g = (256 * palette[0].g + dg * 128 + dg / 4 + 128) / 256;
    palette[2].b = ((c0.b + c1.b) * 33) / 8;
#endif
    palette[3].r = 0;
    palette[3].g = 0;
    palette[3].b = 0;
}

static void evaluate_palette(Color16 c0, Color16 c1, Color32 palette[4], bool d3d9_bias) {
    palette[0] = bitexpand_color16_to_color32(c0);
    palette[1] = bitexpand_color16_to_color32(c1);
    if (c0.u > c1.u) {
        evaluate_palette4(c0, c1, palette, d3d9_bias);
    }
    else {
        evaluate_palette3(c0, c1, palette);
    }
}

static void evaluate_palette_nv(Color16 c0, Color16 c1, Color32 palette[4]) {
    palette[0].r = (3 * c0.r * 22) / 8;
    palette[0].g = (c0.g << 2) | (c0.g >> 4);
    palette[0].b = (3 * c0.b * 22) / 8;
    palette[1].a = 255;
    palette[1].r = (3 * c1.r * 22) / 8;
    palette[1].g = (c1.g << 2) | (c1.g >> 4);
    palette[1].b = (3 * c1.b * 22) / 8;
    palette[1].a = 255;

    int gdiff = palette[1].g - palette[0].g;
    if (c0.u > c1.u) {
        palette[2].r = ((2 * c0.r + c1.r) * 22) / 8;
        palette[2].g = (256 * palette[0].g + gdiff / 4 + 128 + gdiff * 80) / 256;
        palette[2].b = ((2 * c0.b + c1.b) * 22) / 8;
        palette[2].a = 0xFF;

        palette[3].r = ((2 * c1.r + c0.r) * 22) / 8;
        palette[3].g = (256 * palette[1].g - gdiff / 4 + 128 - gdiff * 80) / 256;
        palette[3].b = ((2 * c1.b + c0.b) * 22) / 8;
        palette[3].a = 0xFF;
    }
    else {
        palette[2].r = ((c0.r + c1.r) * 33) / 8;
        palette[2].g = (256 * palette[0].g + gdiff / 4 + 128 + gdiff * 128) / 256;
        palette[2].b = ((c0.b + c1.b) * 33) / 8;
        palette[2].a = 0xFF;
        palette[3].u = 0;
    }
}

static void evaluate_palette(Color16 c0, Color16 c1, Color32 palette[4]) {
#if DECODER == 0
    evaluate_palette(c0, c1, palette, false);
#elif DECODER == 1
    evaluate_palette(c0, c1, palette, true);
#elif DECODER == 2
    evaluate_palette_nv(c0, c1, palette);
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
    Vector3 d = (p * 255 - c * 255) * w;
    return dot(d, d);
}

static float evaluate_mse(const Color32 & p, const Vector3 & c, const Vector3 & w) {
    Vector3 d = (Vector3(p.r, p.g, p.b) - c * 255) * w;
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
    //evaluate_palette_nv(output->col0, output->col1, palette);

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
        int index = (output->indices >> (2*i)) & 3; // @@ Is this the right order?
        error += evaluate_mse(vector_palette[index], colors[i]);
    }

    return error;
}
#endif

static float evaluate_mse(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, const BlockDXT1 * output) {
    Color32 palette[4];
    evaluate_palette(output->col0, output->col1, palette);
    //evaluate_palette_nv5x(output->col0, output->col1, palette);

    // convert palette to float.
    /*Vector3 vector_palette[4];
    for (int i = 0; i < 4; i++) {
        vector_palette[i] = color_to_vector3(palette[i]);
    }*/

    // evaluate error for each index.
    float error = 0.0f;
    for (int i = 0; i < 16; i++) {
        int index = (output->indices >> (2 * i)) & 3;
        error += input_weights[i] * evaluate_mse(palette[index], input_colors[i].xyz(), color_weights);
    }
    return error;
}

float nv::evaluate_dxt1_error(const uint8 rgba_block[16*4], const BlockDXT1 * block, int decoder) {
    Color32 palette[4];
    if (decoder == 2) {
        evaluate_palette_nv(block->col0, block->col1, palette);

    }
    else {
        evaluate_palette(block->col0, block->col1, palette, /*d3d9=*/decoder);
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
        float d0 = evaluate_mse(palette[0], input_colors[i].xyz(), color_weights);
        float d1 = evaluate_mse(palette[1], input_colors[i].xyz(), color_weights);
        float d2 = evaluate_mse(palette[2], input_colors[i].xyz(), color_weights);
        float d3 = evaluate_mse(palette[3], input_colors[i].xyz(), color_weights);

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
        float d0 = evaluate_mse(palette[0], input_colors[i], Vector3(1));
        float d1 = evaluate_mse(palette[1], input_colors[i], Vector3(1));
        float d2 = evaluate_mse(palette[2], input_colors[i], Vector3(1));
        float d3 = evaluate_mse(palette[3], input_colors[i], Vector3(1));

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
        float d0 = evaluate_mse(palette[0], input_colors[i].xyz(), color_weights);
        float d1 = evaluate_mse(palette[1], input_colors[i].xyz(), color_weights);
        float d2 = evaluate_mse(palette[2], input_colors[i].xyz(), color_weights);
        float d3 = evaluate_mse(palette[3], input_colors[i].xyz(), color_weights);

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
    Vector3 alphax_sum(0.0f);
    Vector3 betax_sum(0.0f);

    for (int i = 0; i < count; i++)
    {
        const uint bits = indices >> (2 * i);

        float beta = float(bits & 1);
        if (bits & 2) beta = (1 + beta) / 3.0f;
        float alpha = 1.0f - beta;

        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i].xyz();
        betax_sum += beta * colors[i].xyz();
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
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    Vector3 alphax_sum(0.0f);
    Vector3 betax_sum(0.0f);

    for (int i = 0; i < count; i++)
    {
        const uint bits = indices >> (2 * i);

        float beta = float(bits & 1);
        if (bits & 2) beta = (1 + beta) / 3.0f;
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


// Least squares fitting of color end points for the given indices. @@ This does not support black/transparent index. @@ Take weights into account.
static bool optimize_end_points3(uint indices, const Vector3 * colors, /*const float * weights,*/ int count, Vector3 * a, Vector3 * b)
{
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    Vector3 alphax_sum(0.0f);
    Vector3 betax_sum(0.0f);

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

// @@ After optimization we need to round end points. Round in all possible directions, and pick best.



// find minimum and maximum colors based on bounding box in color space
inline static void fit_colors_bbox(const Vector3 * colors, int count, Vector3 * restrict c0, Vector3 * restrict c1)
{
    *c0 = Vector3(0);
    *c1 = Vector3(1);

    for (int i = 0; i < count; i++) {
        *c0 = max(*c0, colors[i]);
        *c1 = min(*c1, colors[i]);
    }
}

inline static void select_diagonal(const Vector3 * colors, int count, Vector3 * restrict c0, Vector3 * restrict c1)
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

    c0->set(x0, y0, c0->z);
    c1->set(x1, y1, c1->z);
}

inline static void inset_bbox(Vector3 * restrict c0, Vector3 * restrict c1)
{
    Vector3 inset = (*c0 - *c1) / 16.0f - Vector3((8.0f / 255.0f) / 16.0f);
    *c0 = saturate(*c0 - inset);
    *c1 = saturate(*c1 + inset);
}



// Single color lookup tables from:
// https://github.com/nothings/stb/blob/master/stb_dxt.h
static uint8 match5[256][2];
static uint8 match6[256][2];

static int Mul8Bit(int a, int b)
{
    int t = a * b + 128;
    return (t + (t >> 8)) >> 8;
}

static inline int Lerp13(int a, int b)
{
#ifdef DXT_USE_ROUNDING_BIAS
    // with rounding bias
    return a + Mul8Bit(b - a, 0x55);
#else
    // without rounding bias
    // replace "/ 3" by "* 0xaaab) >> 17" if your compiler sucks or you really need every ounce of speed.
    return (a * 2 + b) / 3;
#endif
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

// @@ Make this explicit.
NV_AT_STARTUP(nv::init_dxt1());

void nv::init_dxt1()
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


/*float nv::compress_dxt1_single_color_optimal(Color32 c, BlockDXT1 * output)
{
    ::compress_dxt1_single_color_optimal(c, output);

    // Multiply by 16^2, the weight associated to a single color.
    // Divide by 255*255 to covert error to [0-1] range.
    return (256.0f / (255*255)) * evaluate_mse(output, c, output->indices & 3);
}*/

/*float nv::compress_dxt1_single_color_optimal(const Vector3 & color, BlockDXT1 * output)
{
    return compress_dxt1_single_color_optimal(vector3_to_color32(color), output);
}*/


// Compress block using the average color.
float nv::compress_dxt1_single_color(const nv::Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, BlockDXT1 * output)
{
    // Compute block average.
    Vector3 color_sum(0);
    float weight_sum = 0;

    for (int i = 0; i < count; i++) {
        color_sum += colors[i] * weights[i];
        weight_sum += weights[i];
    }

    // Compress optimally.
    ::compress_dxt1_single_color_optimal(vector3_to_color32(color_sum / weight_sum), output);

    // Decompress block color.
    Color32 palette[4];
    evaluate_palette(output->col0, output->col1, palette);
    //output->evaluatePalette(palette, /*d3d9=*/false);

    Vector3 block_color = color_to_vector3(palette[output->indices & 0x3]);

    // Evaluate error.
    float error = 0;
    for (int i = 0; i < count; i++) {
        error += weights[i] * evaluate_mse(block_color, colors[i], color_weights);
    }
    return error;
}


float nv::compress_dxt1_bounding_box_exhaustive(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, int max_volume, BlockDXT1 * output)
{
    // Compute bounding box.
    Vector3 min_color(1.0f);
    Vector3 max_color(0.0f);

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
                evaluate_palette4(c0, c1, palette, false);
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


void nv::compress_dxt1_cluster_fit(const Vector4 input_colors[16], const Vector3 * colors, const float * weights, int count, const Vector3 & color_weights, bool three_color_mode, BlockDXT1 * output)
{
    ClusterFit fit;
    fit.setColorWeights(Vector4(color_weights, 1));
    fit.setColorSet(colors, weights, count);

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


/*static unsigned int stb__MatchColorsBlock(uint8 *block, uint8 *color)
{
    uint mask = 0;
    int dir[3];
    dir[0] = color[0 * 4 + 0] - color[1 * 4 + 0];
    dir[1] = color[0 * 4 + 1] - color[1 * 4 + 1];
    dir[2] = color[0 * 4 + 2] - color[1 * 4 + 2];
    int dots[16];
    int stops[4];
    int i;

    for (i = 0;i < 16;i++)
        dots[i] = block[i * 4 + 0] * dir[0] + block[i * 4 + 1] * dir[1] + block[i * 4 + 2] * dir[2];

    for (i = 0;i < 4;i++)
        stops[i] = color[i * 4 + 0] * dir[0] + color[i * 4 + 1] * dir[1] + color[i * 4 + 2] * dir[2];

    // think of the colors as arranged on a line; project point onto that line, then choose
    // next color out of available ones. we compute the crossover points for "best color in top
    // half"/"best in bottom half" and then the same inside that subinterval.
    //
    // relying on this 1d approximation isn't always optimal in terms of euclidean distance,
    // but it's very close and a lot faster.
    // http://cbloomrants.blogspot.com/2008/12/12-08-08-dxtc-summary.html

    int c0Point = (stops[1] + stops[3]);
    int halfPoint = (stops[3] + stops[2]);
    int c3Point = (stops[2] + stops[0]);

    for (i = 15;i >= 0;i--) {
        int dot = 2 * dots[i];
        mask <<= 2;

        uint sel;
        if (dot < halfPoint)
            sel = (dot < c0Point) ? 1 : 3;
        else
            sel = (dot < c3Point) ? 2 : 0;

        mask |= sel;
    }

    return mask;
}*/

float nv::compress_dxt1(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, bool three_color_mode, bool hq, BlockDXT1 * output)
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


    float error = FLT_MAX;

    // Sometimes the single color compressor produces better results than the exhaustive. This introduces discontinuities between blocks that
    // use different compressors. For this reason, this is not enabled by default.
    if (0) {
        error = compress_dxt1_single_color(colors, weights, count, color_weights, output);

        if (error == 0.0f || count == 1) {
            // Early out.
            return error;
        }
    }

    // This is too expensive, even with a low threshold.
    // If high quality:
    if (/* DISABLES CODE */ (0)) {
        BlockDXT1 exhaustive_output;
        float exhaustive_error = compress_dxt1_bounding_box_exhaustive(input_colors, colors, weights, count, color_weights, three_color_mode, 1400, &exhaustive_output);

        if (exhaustive_error != FLT_MAX) {
            float exhaustive_error2 = evaluate_mse(input_colors, input_weights, color_weights, &exhaustive_output);

            // The exhaustive compressor does not use color_weights, so the results may be different.
            //nvCheck(equal(exhaustive_error, exhaustive_error2));

            if (exhaustive_error2 < error) {
                *output = exhaustive_output;
                error = exhaustive_error;
            }
        }
    }

    // Cluster fit cannot handle single color blocks, so encode them optimally if we haven't encoded them already.
    if (error == FLT_MAX && count == 1) {
        ::compress_dxt1_single_color_optimal(vector3_to_color32(colors[0]), output);
        return evaluate_mse(input_colors, input_weights, color_weights, output);
    }

    if (count > 1) {
        // Fast box fit encoding:
        {
            BlockDXT1 box_fit_output;

            Vector3 colors[16];
            for (int i = 0; i < 16; i++) {
                colors[i] = input_colors[i].xyz();
            }
            int count = 16;

            // Quick end point selection.
            Vector3 c0, c1;
            fit_colors_bbox(colors, count, &c0, &c1);
            inset_bbox(&c0, &c1);
            select_diagonal(colors, count, &c0, &c1);
            output_block4(input_colors, color_weights, c0, c1, &box_fit_output);

            float box_fit_error = evaluate_mse(input_colors, input_weights, color_weights, &box_fit_output);
            if (box_fit_error < error) {
                error = box_fit_error;
                *output = box_fit_output;

                // Refine color for the selected indices.
                if (optimize_end_points4(output->indices, input_colors, 16, &c0, &c1)) {
                    output_block4(input_colors, color_weights, c0, c1, &box_fit_output);

                    box_fit_error = evaluate_mse(input_colors, input_weights, color_weights, &box_fit_output);
                    if (box_fit_error < error) {
                        error = box_fit_error;
                        *output = box_fit_output;
                    }
                }
            }
        }

        // Try cluster fit.
        BlockDXT1 cluster_fit_output;
        compress_dxt1_cluster_fit(input_colors, colors, weights, count, color_weights, three_color_mode, &cluster_fit_output);

        float cluster_fit_error = evaluate_mse(input_colors, input_weights, color_weights, &cluster_fit_output);

        if (cluster_fit_error < error) {
            *output = cluster_fit_output;
            error = cluster_fit_error;
        }

        if (hq) {
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
                if (refined_error < error) {
                    *output = refined;
                    error = refined_error;
                    lastImprovement = i;
                }

                // Early out if the last 32 steps didn't improve error.
                if (i - lastImprovement > 32) break;
            }
        }
    }

    return error;
}


// Once we have an index assignment we have colors grouped in 1-4 clusters.
// If 1 clusters -> Use optimal compressor.
// If 2 clusters -> Try: (0, 1), (1, 2), (0, 2), (0, 3) - [0, 1]
// If 3 clusters -> Try: (0, 1, 2), (0, 1, 3), (0, 2, 3) - [0, 1, 2]
// If 4 clusters -> Try: (0, 1, 2, 3)

// @@ How do we do the initial index/cluster assignment? Use standard cluster fit.



float nv::compress_dxt1_fast(const Vector4 input_colors[16], const float input_weights[16], const Vector3 & color_weights, BlockDXT1 * output)
{
    Vector3 colors[16];
    for (int i = 0; i < 16; i++) {
        colors[i] = input_colors[i].xyz();
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
        ::compress_dxt1_single_color_optimal(vector3_to_color32(c0), output);
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


void nv::compress_dxt1_fast2(const uint8 input_colors[16*4], BlockDXT1 * output) {
    /*Vector3 colors[16];
    float weights[16];
    int count = reduce_colors(input_colors, colors, weights);

    if (count == 0) {
        // Output trivial block.
        output->col0.u = 0;
        output->col1.u = 0;
        output->indices = 0;
        return;
    }


    float error = FLT_MAX;
    error = compress_dxt1_single_color(colors, weights, count, Vector3(1.0f), output);

    if (error == 0.0f || count == 1) {
        // Early out.
        return;
    }*/

    Vector3 vec_colors[16];
    for (int i = 0; i < 16; i++) {
        vec_colors[i] = Vector3(input_colors[4 * i + 0] / 255.0f, input_colors[4 * i + 1] / 255.0f, input_colors[4 * i + 2] / 255.0f);
    }

    // Quick end point selection.
    Vector3 c0, c1;
    //fit_colors_bbox(colors, count, &c0, &c1);
    //select_diagonal(colors, count, &c0, &c1);
    fit_colors_bbox(vec_colors, 16, &c0, &c1);
    if (c0 == c1) {
        ::compress_dxt1_single_color_optimal(vector3_to_color32(c0), output);
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


/*static int Mul8Bit(int a, int b)
{
    int t = a * b + 128;
    return (t + (t >> 8)) >> 8;
}*/

static bool compute_least_squares_endpoints(const uint8 *block, uint32 mask, Vector3 *pmax, Vector3 *pmin)
{
    static const int w1Tab[4] = { 3,0,2,1 };
    static const int prods[4] = { 0x090000,0x000900,0x040102,0x010402 };
    // ^some magic to save a lot of multiplies in the accumulating loop...
    // (precomputed products of weights for least squares system, accumulated inside one 32-bit register)

    int akku = 0;
    int At1_r, At1_g, At1_b;
    int At2_r, At2_g, At2_b;
    unsigned int cm = mask;

    if ((mask ^ (mask << 2)) < 4) // all pixels have the same index?
    {
        return false;
    }
    else {
        At1_r = At1_g = At1_b = 0;
        At2_r = At2_g = At2_b = 0;
        for (int i = 0;i < 16;++i, cm >>= 2) {
            int step = cm & 3;
            int w1 = w1Tab[step];
            int r = block[i * 4 + 0];
            int g = block[i * 4 + 1];
            int b = block[i * 4 + 2];

            akku += prods[step];
            At1_r += w1 * r;
            At1_g += w1 * g;
            At1_b += w1 * b;
            At2_r += r;
            At2_g += g;
            At2_b += b;
        }

        At2_r = 3 * At2_r - At1_r;
        At2_g = 3 * At2_g - At1_g;
        At2_b = 3 * At2_b - At1_b;

        // extract solutions and decide solvability
        int xx = akku >> 16;
        int yy = (akku >> 8) & 0xff;
        int xy = (akku >> 0) & 0xff;

        float f = 3.0f / 255.0f / (xx*yy - xy * xy);

        // solve.
        pmax->x = (At1_r*yy - At2_r * xy) * f;
        pmax->y = (At1_r*yy - At2_r * xy) * f;
        pmax->z = (At1_r*yy - At2_r * xy) * f;

        pmin->x = (At2_r*xx - At1_r * xy) * f;
        pmin->y = (At2_r*xx - At1_r * xy) * f;
        pmin->z = (At2_r*xx - At1_r * xy) * f;

        return true;
    }
}


static uint32 bc1_find_sels(const uint8 *input_colors, uint32_t lr, uint32_t lg, uint32_t lb, uint32_t hr, uint32_t hg, uint32_t hb)
{
    uint32_t block_r[4], block_g[4], block_b[4];

    block_r[0] = (lr << 3) | (lr >> 2); block_g[0] = (lg << 2) | (lg >> 4); block_b[0] = (lb << 3) | (lb >> 2);
    block_r[3] = (hr << 3) | (hr >> 2); block_g[3] = (hg << 2) | (hg >> 4); block_b[3] = (hb << 3) | (hb >> 2);
    block_r[1] = (block_r[0] * 2 + block_r[3]) / 3; block_g[1] = (block_g[0] * 2 + block_g[3]) / 3; block_b[1] = (block_b[0] * 2 + block_b[3]) / 3;
    block_r[2] = (block_r[3] * 2 + block_r[0]) / 3; block_g[2] = (block_g[3] * 2 + block_g[0]) / 3; block_b[2] = (block_b[3] * 2 + block_b[0]) / 3;

    int ar = block_r[3] - block_r[0], ag = block_g[3] - block_g[0], ab = block_b[3] - block_b[0];

    int dots[4];
    for (uint32_t i = 0; i < 4; i++)
        dots[i] = (int)block_r[i] * ar + (int)block_g[i] * ag + (int)block_b[i] * ab;

    int t0 = dots[0] + dots[1], t1 = dots[1] + dots[2], t2 = dots[2] + dots[3];

    ar *= 2; ag *= 2; ab *= 2;

    uint sels = 0;
    for (uint32_t i = 0; i < 16; i++)
    {
        const int d = input_colors[4*i+0] * ar + input_colors[4*i+1] * ag + input_colors[4*i+2] * ab;
        static const uint8_t s_sels[4] = { 3, 2, 1, 0 };

        // Rounding matters here!
        // d <= t0: <=, not <, to the later LS step "sees" a wider range of selectors. It matters for quality.
        sels |= s_sels[(d <= t0) + (d < t1) + (d < t2)] << (2 * i);
    }
    return sels;
}


void nv::compress_dxt1_fast_geld(const uint8 input_colors[16 * 4], BlockDXT1 * block) {

    int fr = input_colors[0];
    int fg = input_colors[1];
    int fb = input_colors[2];

    int total_r = fr, total_g = fg, total_b = fb;
    int max_r = fr, max_g = fg, max_b = fb;
    int min_r = fr, min_g = fg, min_b = fb;
    uint32 grayscale_flag = (fr == fg) && (fr == fb);
    for (uint32 i = 1; i < 16; i++)
    {
        const int r = input_colors[4*i+0], g = input_colors[4 * i + 1], b = input_colors[4 * i + 2];
        grayscale_flag &= ((r == g) && (r == b));
        max_r = max(max_r, r); max_g = max(max_g, g); max_b = max(max_b, b);
        min_r = min(min_r, r); min_g = min(min_g, g); min_b = min(min_b, b);
        total_r += r; total_g += g; total_b += b;
    }

    int lr, lg, lb;
    int hr, hg, hb;

    if (grayscale_flag) {
        // Grayscale blocks are a common enough case to specialize.
        lr = lb = Mul8Bit(min_r, 31);
        lg = Mul8Bit(min_r, 63);

        hr = hb = Mul8Bit(max_r, 31);
        hg = Mul8Bit(max_r, 63);
    }
    else {
        int avg_r = (total_r + 8) >> 4, avg_g = (total_g + 8) >> 4, avg_b = (total_b + 8) >> 4;

        // Find the shortest vector from a AABB corner to the block's average color.
        // This is to help avoid outliers.

        uint32_t dist[3][2];
        dist[0][0] = square(min_r - avg_r) << 3; dist[0][1] = square(max_r - avg_r) << 3;
        dist[1][0] = square(min_g - avg_g) << 3; dist[1][1] = square(max_g - avg_g) << 3;
        dist[2][0] = square(min_b - avg_b) << 3; dist[2][1] = square(max_b - avg_b) << 3;

        uint32_t min_d0 = (dist[0][0] + dist[1][0] + dist[2][0]);
        uint32_t d4 = (dist[0][0] + dist[1][0] + dist[2][1]) | 4;
        min_d0 = min(min_d0, d4);

        uint32_t min_d1 = (dist[0][1] + dist[1][0] + dist[2][0]) | 1;
        uint32_t d5 = (dist[0][1] + dist[1][0] + dist[2][1]) | 5;
        min_d1 = min(min_d1, d5);

        uint32_t d2 = (dist[0][0] + dist[1][1] + dist[2][0]) | 2;
        min_d0 = min(min_d0, d2);

        uint32_t d3 = (dist[0][1] + dist[1][1] + dist[2][0]) | 3;
        min_d1 = min(min_d1, d3);

        uint32_t d6 = (dist[0][0] + dist[1][1] + dist[2][1]) | 6;
        min_d0 = min(min_d0, d6);

        uint32_t d7 = (dist[0][1] + dist[1][1] + dist[2][1]) | 7;
        min_d1 = min(min_d1, d7);

        uint32_t min_d = min(min_d0, min_d1);
        uint32_t best_i = min_d & 7;

        const int delta_r = (best_i & 1) ? (max_r - avg_r) : (avg_r - min_r);
        const int delta_g = (best_i & 2) ? (max_g - avg_g) : (avg_g - min_g);
        const int delta_b = (best_i & 4) ? (max_b - avg_b) : (avg_b - min_b);

        // Now we have a smaller AABB going from the block's average color to a cornerpoint of the larger AABB.
        // Project all pixels colors along the 4 vectors going from a smaller AABB cornerpoint to the opposite cornerpoint, find largest projection.
        // One of these vectors will be a decent approximation of the block's PCA.
        const int saxis0_r = delta_r, saxis0_g = delta_g, saxis0_b = delta_b;

        int low_dot0 = INT_MAX, high_dot0 = INT_MIN;
        int low_dot1 = INT_MAX, high_dot1 = INT_MIN;
        int low_dot2 = INT_MAX, high_dot2 = INT_MIN;
        int low_dot3 = INT_MAX, high_dot3 = INT_MIN;

        int low_c0, low_c1, low_c2, low_c3;
        int high_c0, high_c1, high_c2, high_c3;

        for (uint32_t i = 0; i < 16; i++)
        {
            const int dotx = input_colors[4*i+0] * saxis0_r;
            const int doty = input_colors[4*i+1] * saxis0_g;
            const int dotz = input_colors[4*i+2] * saxis0_b;

            const int dot0 = ((dotz + dotx + doty) << 4) + i;
            const int dot1 = ((dotz - dotx - doty) << 4) + i;
            const int dot2 = ((dotz - dotx + doty) << 4) + i;
            const int dot3 = ((dotz + dotx - doty) << 4) + i;

            if (dot0 < low_dot0)
            {
                low_dot0 = dot0;
                low_c0 = i;
            }
            if ((dot0 ^ 15) > high_dot0)
            {
                high_dot0 = dot0 ^ 15;
                high_c0 = i;
            }

            if (dot1 < low_dot1)
            {
                low_dot1 = dot1;
                low_c1 = i;
            }
            if ((dot1 ^ 15) > high_dot1)
            {
                high_dot1 = dot1 ^ 15;
                high_c1 = i;
            }

            if (dot2 < low_dot2)
            {
                low_dot2 = dot2;
                low_c2 = i;
            }
            if ((dot2 ^ 15) > high_dot2)
            {
                high_dot2 = dot2 ^ 15;
                high_c2 = i;
            }

            if (dot3 < low_dot3)
            {
                low_dot3 = dot3;
                low_c3 = i;
            }
            if ((dot3 ^ 15) > high_dot3)
            {
                high_dot3 = dot3 ^ 15;
                high_c3 = i;
            }
        }


        uint32_t low_c = low_dot0 & 15, high_c = ~high_dot0 & 15, r = (high_dot0 & ~15) - (low_dot0 & ~15);

        uint32_t tr = (high_dot1 & ~15) - (low_dot1 & ~15);
        if (tr > r)
            low_c = low_dot1 & 15, high_c = ~high_dot1 & 15, r = tr;

        tr = (high_dot2 & ~15) - (low_dot2 & ~15);
        if (tr > r)
            low_c = low_dot2 & 15, high_c = ~high_dot2 & 15, r = tr;

        tr = (high_dot3 & ~15) - (low_dot3 & ~15);
        if (tr > r)
            low_c = low_dot3 & 15, high_c = ~high_dot3 & 15;

        lr = Mul8Bit(input_colors[low_c*4+0], 31);
        lg = Mul8Bit(input_colors[low_c*4+1], 63);
        lb = Mul8Bit(input_colors[low_c*4+2], 31);

        hr = Mul8Bit(input_colors[high_c*4+0], 31);
        hg = Mul8Bit(input_colors[high_c*4+1], 63);
        hb = Mul8Bit(input_colors[high_c*4+2], 31);
    }

    uint32 selectors = bc1_find_sels(input_colors, lr, lg, lb, hr, hg, hb);

    Vector3 c0, c1;
    if (!compute_least_squares_endpoints(input_colors, selectors, &c0, &c1)) {
        // @@ Single color compressor.
        Color32 c;
        c.r = lr;
        c.g = lg;
        c.b = lb;
        ::compress_dxt1_single_color_optimal(c, block);
    }
    else {
        Color16 color0 = vector3_to_color16(c0);
        Color16 color1 = vector3_to_color16(c1);

        if (color0.u < color1.u) {
            swap(color0, color1);
        }

        Color32 palette[4];
        evaluate_palette(color0, color1, palette);

        block->col0 = color0;
        block->col1 = color1;
        block->indices = bc1_find_sels(input_colors, color0.r, color0.g, color0.b, color1.r, color1.g, color1.b);
    }

    /*// Quick end point selection.
    Vector3 c0, c1;
    //fit_colors_bbox(colors, count, &c0, &c1);
    //select_diagonal(colors, count, &c0, &c1);
    fit_colors_bbox(vec_colors, 16, &c0, &c1);
    if (c0 == c1) {
        ::compress_dxt1_single_color_optimal(vector3_to_color32(c0), output);
        return;
    }
    inset_bbox(&c0, &c1);
    select_diagonal(vec_colors, 16, &c0, &c1);
    output_block4(vec_colors, c0, c1, output);

    // Refine color for the selected indices.
    if (optimize_end_points4(output->indices, vec_colors, 16, &c0, &c1)) {
        output_block4(vec_colors, c0, c1, output);
    }*/
}
