
#include "CompressorDXT1.h"
#include "SingleColorLookup.h"
#include "ClusterFit.h"
#include "QuickCompressDXT.h"  // Deprecate.

#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/Color.inl"
#include "nvmath/Vector.inl"
#include "nvmath/Fitting.h"
#include "nvmath/ftoi.h"

#include "nvcore/Utils.h" // swap

#include <string.h> // memset


using namespace nv;


inline static void color_block_to_vector_block(const ColorBlock & rgba, Vector3 block[16])
{
	for (int i = 0; i < 16; i++)
	{
		const Color32 c = rgba.color(i);
		block[i] = Vector3(c.r, c.g, c.b);
	}
}

inline Vector3 r5g6b5_to_vector3(int r, int g, int b)
{
    Vector3 c;
    c.x = float((r << 3) | (r >> 2));
    c.y = float((g << 2) | (g >> 4));
    c.z = float((b << 3) | (b >> 2));
    return c;
}

inline Vector3 color_to_vector3(Color32 c)
{
    const float scale = 1.0f / 255.0f;
    return Vector3(c.r * scale, c.g * scale, c.b * scale);
}

inline Color32 vector3_to_color(Vector3 v)
{
    Color32 color;
    color.r = U8(ftoi_round(saturate(v.x) * 255));
    color.g = U8(ftoi_round(saturate(v.y) * 255));
    color.b = U8(ftoi_round(saturate(v.z) * 255));
    color.a = 255;
}



// Find first valid color.
static bool find_valid_color_rgb(const Vector3 * colors, const float * weights, int count, Vector3 * valid_color)
{
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0.0f) {
            *valid_color = colors[i];
            return true;
        }
    }

    // No valid colors.
    return false;
}

static bool is_single_color_rgb(const Vector3 * colors, const float * weights, int count, Vector3 color)
{
    for (int i = 0; i < count; i++) {
        if (weights[i] > 0.0f) {
            if (colors[i] != color) return false;
        }
    }

    return true;
}

// Find similar colors and combine them together.
static int reduce_colors(const Vector3 * input_colors, const float * input_weights, Vector3 * colors, float * weights)
{
    int n = 0;
    for (int i = 0; i < 16; i++)
    {
        Vector3 ci = input_colors[i];
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



// Different ways of estimating the error.
static float evaluate_mse(const Vector3 & p, const Vector3 & c) {
    return square(p.x-c.x) + square(p.y-c.y) + square(p.z-c.z);
}

/*static float evaluate_mse(const Vector3 & p, const Vector3 & c, const Vector3 & w) {
    return ww.x * square(p.x-c.x) + ww.y * square(p.y-c.y) + ww.z * square(p.z-c.z);
}*/

static int evaluate_mse_rgb(const Color32 & p, const Color32 & c) {
    return square(int(p.r)-c.r) + square(int(p.g)-c.g) + square(int(p.b)-c.b);
}

static float evaluate_mse(const Vector3 palette[4], const Vector3 & c) {
    float e0 = evaluate_mse(palette[0], c);
    float e1 = evaluate_mse(palette[1], c);
    float e2 = evaluate_mse(palette[2], c);
    float e3 = evaluate_mse(palette[3], c);
    return min(min(e0, e1), min(e2, e3));
}

static int evaluate_mse(const Color32 palette[4], const Color32 & c) {
    int e0 = evaluate_mse_rgb(palette[0], c);
    int e1 = evaluate_mse_rgb(palette[1], c);
    int e2 = evaluate_mse_rgb(palette[2], c);
    int e3 = evaluate_mse_rgb(palette[3], c);
    return min(min(e0, e1), min(e2, e3));
}

static float evaluate_mse(const Vector3 palette[4], const Vector3 & c, int index) {
    return evaluate_mse(palette[index], c);
}

static int evaluate_mse(const Color32 palette[4], const Color32 & c, int index) {
    return evaluate_mse_rgb(palette[index], c);
}


static float evaluate_mse(const BlockDXT1 * output, Vector3 colors[16]) {
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
        error += evaluate_mse(vector_palette, colors[i], index);
    }

    return error;
}

static int evaluate_mse(const BlockDXT1 * output, Color32 color, int index) {
    Color32 palette[4];
    output->evaluatePalette(palette, /*d3d9=*/false);

    return evaluate_mse(palette, color, index);
}


/*void output_block3(const ColorSet & set, const Vector3 & start, const Vector3 & end, BlockDXT1 * block)
{
    Vector3 minColor = start * 255.0f;
    Vector3 maxColor = end * 255.0f;
    uint16 color0 = roundAndExpand(&minColor);
    uint16 color1 = roundAndExpand(&maxColor);

    if (color0 > color1) {
        swap(maxColor, minColor);
        swap(color0, color1);
    }

    block->col0 = Color16(color0);
    block->col1 = Color16(color1);
    block->indices = compute_indices3(colors, weights, count, maxColor / 255.0f, minColor / 255.0f);

    //optimizeEndPoints3(set, block);
}*/






// Single color compressor, based on:
// https://mollyrocket.com/forums/viewtopic.php?t=392
float nv::compress_dxt1_single_color_optimal(Color32 c, BlockDXT1 * output)
{
    output->col0.r = OMatch5[c.r][0];
    output->col0.g = OMatch6[c.g][0];
    output->col0.b = OMatch5[c.b][0];
    output->col1.r = OMatch5[c.r][1];
    output->col1.g = OMatch6[c.g][1];
    output->col1.b = OMatch5[c.b][1];
    output->indices = 0xaaaaaaaa;
    
    if (output->col0.u < output->col1.u)
    {
        swap(output->col0.u, output->col1.u);
        output->indices ^= 0x55555555;
    }

    return (float) evaluate_mse(output, c, output->indices & 3);
}


float nv::compress_dxt1_single_color_optimal(const Vector3 & color, BlockDXT1 * output)
{
    return compress_dxt1_single_color_optimal(vector3_to_color(color), output);
}


// Low quality baseline compressor.
float nv::compress_dxt1_least_squares_fit(const Vector3 * input_colors, const Vector3 * colors, const float * weights, int count, BlockDXT1 * output)
{
    // @@ Iterative best end point fit.

    return FLT_MAX;
}


static Color32 bitexpand_color16_to_color32(Color16 c16) {
    Color32 c32;
    c32.b = (c16.b << 3) | (c16.b >> 2);
    c32.g = (c16.g << 2) | (c16.g >> 4);
    c32.r = (c16.r << 3) | (c16.r >> 2);
    c32.a = 0xFF;

    //c32.u = ((c16.u << 3) & 0xf8) | ((c16.u << 5) & 0xfc00) | ((c16.u << 8) & 0xf80000);
    //c32.u |= (c32.u >> 5) & 0x070007;
    //c32.u |= (c32.u >> 6) & 0x000300;

    return c32;
}

static Color32 bitexpand_color16_to_color32(int r, int g, int b) {
    Color32 c32;
    c32.b = (b << 3) | (b >> 2);
    c32.g = (g << 2) | (g >> 4);
    c32.r = (r << 3) | (r >> 2);
    c32.a = 0xFF;
    return c32;
}

static Color16 truncate_color32_to_color16(Color32 c32) {
    Color16 c16;
    c16.b = (c32.b >> 3);
    c16.g = (c32.g >> 2);
    c16.r = (c32.r >> 3);
    return c16;
}




static float evaluate_palette4(Color32 palette[4]) {
    palette[2].r = (2 * palette[0].r + palette[1].r) / 3;
    palette[2].g = (2 * palette[0].g + palette[1].g) / 3;
    palette[2].b = (2 * palette[0].b + palette[1].b) / 3;
    palette[3].r = (2 * palette[1].r + palette[0].r) / 3;
    palette[3].g = (2 * palette[1].g + palette[0].g) / 3;
    palette[3].b = (2 * palette[1].b + palette[0].b) / 3;
}

static float evaluate_palette3(Color32 palette[4]) {
    palette[2].r = (palette[0].r + palette[1].r) / 2;
    palette[2].g = (palette[0].g + palette[1].g) / 2;
    palette[2].b = (palette[0].b + palette[1].b) / 2;
    palette[3].r = 0;
    palette[3].g = 0;
    palette[3].b = 0;
}

static float evaluate_palette_error(Color32 palette[4], const Color32 * colors, const float * weights, int count) {
    
	float total = 0.0f;
	for (int i = 0; i < count; i++) {
        total += (weights[i] * weights[i]) * evaluate_mse(palette, colors[i]);
	}

	return total;
}




float nv::compress_dxt1_bounding_box_exhaustive(const Vector3 input_colors[16], const Vector3 * colors, const float * weights, int count, int max_volume, BlockDXT1 * output)
{
    // Compute bounding box.
    Vector3 min_color(1.0f);
    Vector3 max_color(0.0f);

    for (int i = 0; i < count; i++) {
        min_color = min(min_color, colors[i]);
        max_color = max(max_color, colors[i]);
    }

    // Convert to 5:6:5
    int min_r = ftoi_floor(31 * min_color.x);
    int min_g = ftoi_floor(63 * min_color.y);
    int min_b = ftoi_floor(31 * min_color.z);
    int max_r = ftoi_ceil(31 * max_color.x);
    int max_g = ftoi_ceil(63 * max_color.y);
    int max_b = ftoi_ceil(31 * max_color.z);

    // Expand the box.
    int range_r = max_r - min_r;
    int range_g = max_g - min_g;
    int range_b = max_b - min_b;

    min_r = max(0, min_r - (range_r + 1) / 1 - 1);
    min_g = max(0, min_g - (range_g + 1) / 1 - 1);
    min_b = max(0, min_b - (range_b + 1) / 1 - 1);

    max_r = min(31, max_r + (range_r + 1) / 2 + 1);
    max_g = min(63, max_g + (range_g + 1) / 2 + 1);
    max_b = min(31, max_b + (range_b + 1) / 2 + 1);

    // Estimate size of search space.
    int volume = (max_r-min_r+1) * (max_g-min_g+1) * (max_b-min_b+1);

    // if size under search_limit, then proceed. Note that search_limit is sqrt of number of evaluations.
    if (volume > max_volume) {
        return FLT_MAX;
    }

    Color32 colors32[16];
    for (int i = 0; i < count; i++) {
        colors32[i] = toColor32(Vector4(colors[i], 1));
    }

    float best_error = FLT_MAX;
    Color32 best0, best1;

    for(int r0 = min_r; r0 <= max_r; r0++)
    for(int r1 = max_r; r1 >= r0; r1--)
    for(int g0 = min_g; g0 <= max_g; g0++)
    for(int g1 = max_g; g1 >= g0; g1--)
    for(int b0 = min_b; b0 <= max_b; b0++)
    for(int b1 = max_b; b1 >= b0; b1--)
    {
        Color32 palette[4];
        palette[0] = bitexpand_color16_to_color32(r1, g1, b1);
        palette[1] = bitexpand_color16_to_color32(r0, g0, b0);
        
        // Evaluate error in 4 color mode.
        evaluate_palette4(palette);

        float error = evaluate_palette_error(palette, colors32, weights, count);

        if (error < best_error) {
            best_error = error;
            best0 = palette[0];
            best1 = palette[1];
        }

#if 0
        // Evaluate error in 3 color mode.
        evaluate_palette3(palette);

        float error = evaluate_palette_error(palette, colors, weights, count);

        if (error < best_error) {
            best_error = error;
            best0 = palette[1];
            best1 = palette[0];
        }
#endif
    }

    output->col0 = truncate_color32_to_color16(best0);
    output->col1 = truncate_color32_to_color16(best1);

    if (output->col0.u <= output->col1.u) {
        //output->indices = computeIndices3(colors, best0, best1);
    }
    else {
        //output->indices = computeIndices4(colors, best0, best1);
    }

    return FLT_MAX;
}


float nv::compress_dxt1_cluster_fit(const Vector3 input_colors[16], const Vector3 * colors, const float * weights, int count, BlockDXT1 * output)
{
    ClusterFit fit;
    //fit.setColorWeights(compressionOptions.colorWeight);
    fit.setColorWeights(Vector4(1));                // @@ Set color weights.
    fit.setColorSet(colors, weights, count);

    // start & end are in [0, 1] range.
    Vector3 start, end;
    fit.compress4(&start, &end);

    if (fit.compress3(&start, &end)) {
        //output_block3(input_colors, start, end, block);
        // @@ Output block.
    }
    else {
        //output_block4(input_colors, start, end, block);
        // @@ Output block. 
    }
}



float nv::compress_dxt1(const Vector3 input_colors[16], const float input_weights[16], BlockDXT1 * output)
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

    if (count == 1) {
        return compress_dxt1_single_color_optimal(colors[0], output);
    }

    // If high quality:
    //error = compress_dxt1_bounding_box_exhaustive(colors, weigths, count, 3200, error, output);
    //if (error < FLT_MAX) return error;

    // This is pretty fast and in some cases can produces better quality than cluster fit.
//    error = compress_dxt1_least_squares_fit(colors, weigths, error, output);

    // 
    float error = compress_dxt1_cluster_fit(input_colors, colors, weights, count, output);

    return error;
}

