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

#include "CompressorDX9.h"
#include "QuickCompressDXT.h"
#include "OptimalCompressDXT.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"
#include "ClusterFit.h"

// squish
#include "squish/colourset.h"
#include "squish/weightedclusterfit.h"

#include "nvtt.h"

#include "nvimage/Image.h"
#include "nvimage/ColorBlock.h"
#include "nvimage/BlockDXT.h"

#include "nvmath/Vector.inl"
#include "nvmath/Color.inl"

#include "nvcore/Memory.h"

#include <new> // placement new

// s3_quant
#if defined(HAVE_S3QUANT)
#include "s3tc/s3_quant.h"
#endif

// ati tc
#if defined(HAVE_ATITC)
typedef int BOOL;
typedef _W64 unsigned long ULONG_PTR;
typedef ULONG_PTR DWORD_PTR;
#include "atitc/ATI_Compress.h"
#endif

// squish
#if defined(HAVE_SQUISH)
//#include "squish/squish.h"
#include "squish-1.10/squish.h"
#endif

// d3dx
#if defined(HAVE_D3DX)
#include <d3dx9.h>
#endif

// stb
#if defined(HAVE_STB)
#define STB_DEFINE
#include "stb/stb_dxt.h"
#endif

using namespace nv;
using namespace nvtt;


void FastCompressorDXT1::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT1 * block = new(output) BlockDXT1;
    QuickCompress::compressDXT1(rgba, block);
}

void FastCompressorDXT1a::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT1 * block = new(output) BlockDXT1;
    QuickCompress::compressDXT1a(rgba, block);
}

void FastCompressorDXT3::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT3 * block = new(output) BlockDXT3;
    QuickCompress::compressDXT3(rgba, block);
}

void FastCompressorDXT5::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT5 * block = new(output) BlockDXT5;
    QuickCompress::compressDXT5(rgba, block);
}

void FastCompressorDXT5n::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    rgba.swizzle(4, 1, 5, 0); // 0xFF, G, 0, R

    BlockDXT5 * block = new(output) BlockDXT5;
    QuickCompress::compressDXT5(rgba, block);
}


namespace nv {
    float compress_dxt1(const Vector3 input_colors[16], const float input_weights[16], const Vector3 & color_weights, BlockDXT1 * output);
}

#if 1
void CompressorDXT1::compressBlock(ColorSet & set, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
#if 1
    // @@ This setup is the same for all compressors.
    Vector3 input_colors[16];
    float input_weights[16];

    uint x, y;
    for (y = 0; y < set.h; y++) {
        for (x = 0; x < set.w; x++) {
            input_colors[4*y+x] = set.color(x, y).xyz();
            input_weights[4*y+x] = 1.0f;
            if (alphaMode == nvtt::AlphaMode_Transparency) input_weights[4*y+x] = set.color(x, y).z;
        }
        for (; x < 4; x++) {
            input_colors[4*y+x] = Vector3(0);
            input_weights[4*y+x] = 0.0f;
        }
    }
    for (; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            input_colors[4*y+x] = Vector3(0);
            input_weights[4*y+x] = 0.0f;
        }
    }

    compress_dxt1(input_colors, input_weights, compressionOptions.colorWeight.xyz(), (BlockDXT1 *)output);

#else
    set.setUniformWeights();
    set.createMinimalSet(/*ignoreTransparent*/false);

    BlockDXT1 * block = new(output) BlockDXT1;
    
    if (set.isSingleColor(/*ignoreAlpha*/true))
    {
        Color32 c = toColor32(set.colors[0]);
        OptimalCompress::compressDXT1(c, block);
    }
    /*else if (set.colorCount == 2) {
        QuickCompress::compressDXT1(..., block);
    }*/
    else
    {
        ClusterFit fit;
        fit.setColorWeights(compressionOptions.colorWeight);
        fit.setColorSet(&set);

        Vector3 start, end;
        fit.compress4(&start, &end);

        if (fit.compress3(&start, &end)) {
            QuickCompress::outputBlock3(set, start, end, block);
        }
        else {
            QuickCompress::outputBlock4(set, start, end, block);        
        }
    }
#endif
}
#elif 0


extern void compress_dxt1_bounding_box_exhaustive(const ColorBlock & input, BlockDXT1 * output);


void CompressorDXT1::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT1 * block = new(output) BlockDXT1;

    if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), block);
        //compress_dxt1_single_color_optimal(rgba.color(0), block);
    }
    else
    {
        // Do an exhaustive search inside the bounding box.
        compress_dxt1_bounding_box_exhaustive(rgba, block);
    }

    /*else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), 0);
        fit.SetColourSet(&colours, nvsquish::kDxt1);
        fit.Compress(output);
    }*/
}
#else
void CompressorDXT1::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    nvsquish::WeightedClusterFit fit;
    fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

    if (rgba.isSingleColor())
    {
        BlockDXT1 * block = new(output) BlockDXT1;
        OptimalCompress::compressDXT1(rgba.color(0), block);
    }
    else
    {
        nvsquish::ColourSet colours((uint8 *)rgba.colors(), 0);
        fit.SetColourSet(&colours, nvsquish::kDxt1);
        fit.Compress(output);
    }
}
#endif

void CompressorDXT1a::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    uint alphaMask = 0;
    for (uint i = 0; i < 16; i++)
    {
        if (rgba.color(i).a == 0) alphaMask |= (3 << (i * 2)); // Set two bits for each color.
    }

    const bool isSingleColor = rgba.isSingleColor();

    if (isSingleColor)
    {
        BlockDXT1 * block = new(output) BlockDXT1;
        OptimalCompress::compressDXT1a(rgba.color(0), alphaMask, block);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = nvsquish::kDxt1;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, nvsquish::kDxt1);

        fit.Compress(output);
    }
}

void CompressorDXT1_Luma::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT1 * block = new(output) BlockDXT1;
    OptimalCompress::compressDXT1_Luma(rgba, block);
}

void CompressorDXT3::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT3 * block = new(output) BlockDXT3;

    // Compress explicit alpha.
    OptimalCompress::compressDXT3A(rgba, &block->alpha);

    // Compress color.
    if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }
}

void CompressorDXT5::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT5 * block = new(output) BlockDXT5;

    // Compress alpha.
    if (compressionOptions.quality == Quality_Highest)
    {
        OptimalCompress::compressDXT5A(rgba, &block->alpha);
    }
    else
    {
        QuickCompress::compressDXT5A(rgba, &block->alpha);
    }

    // Compress color.
    if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }
}


void CompressorDXT5n::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT5 * block = new(output) BlockDXT5;

    // Compress Y.
    if (compressionOptions.quality == Quality_Highest)
    {
        OptimalCompress::compressDXT1G(rgba, &block->color);
    }
    else
    {
        if (rgba.isSingleColor(Color32(0, 0xFF, 0, 0))) // Mask all but green channel.
        {
                OptimalCompress::compressDXT1G(rgba.color(0).g, &block->color);
        }
        else
        {
            ColorBlock tile = rgba;
            tile.swizzle(4, 1, 5, 3); // leave alpha in alpha channel.

            nvsquish::WeightedClusterFit fit;
            fit.SetMetric(0, 1, 0);

            int flags = 0;
            if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

            nvsquish::ColourSet colours((uint8 *)tile.colors(), flags);
            fit.SetColourSet(&colours, 0);
            fit.Compress(&block->color);
        }
    }

    rgba.swizzle(4, 1, 5, 0); // 1, G, 0, R

    // Compress X.
    if (compressionOptions.quality == Quality_Highest)
    {
        OptimalCompress::compressDXT5A(rgba, &block->alpha);
    }
    else
    {
        QuickCompress::compressDXT5A(rgba, &block->alpha);
    }
}





void CompressorBC3_RGBM::compressBlock(ColorSet & src, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    BlockDXT5 * block = new(output)BlockDXT5;

    if (alphaMode == AlphaMode_Transparency) {
        src.setAlphaWeights();
    }
    else {
        src.setUniformWeights();
    }

    // Decompress the color block and find the M values that reproduce the input most closely. This should compensate for some of the DXT errors.

    // Compress the resulting M values optimally.

    // Repeat this several times until compression error does not improve?

    //Vector3 rgb_block[16];
    //float m_block[16];

    
    // Init RGB/M block.
    const float threshold = 0.15f; // @@ Use compression options.
#if 0
    nvsquish::WeightedClusterFit fit;

    ColorBlock rgba;
    for (int i = 0; i < 16; i++) {
        const Vector4 & c = src.color(i);
        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float M = max(max(R, G), max(B, threshold));
        float r = R / M;
        float g = G / M;
        float b = B / M;
        float a = c.w;

        rgba.color(i) = toColor32(Vector4(r, g, b, a));
    }

    if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }
#endif
#if 1
    ColorSet rgb;
    rgb.allocate(src.w, src.h);     // @@ Handle smaller blocks.

    if (src.colorCount != 16) {
        nvDebugBreak();
    }

    for (uint i = 0; i < src.colorCount; i++) {
        const Vector4 & c = src.color(i);

        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float M = max(max(R, G), max(B, threshold));
        float r = R / M;
        float g = G / M;
        float b = B / M;
        float a = c.w;

        rgb.colors[i] = Vector4(r, g, b, a);
        rgb.indices[i] = i;
        rgb.weights[i] = max(c.w, 0.001f);// src.weights[i];   // IC: For some reason 0 weights are causing problems, even if we eliminate the corresponding colors from the set.
    }

    rgb.createMinimalSet(/*ignoreTransparent=*/true);

    if (rgb.isSingleColor(/*ignoreAlpha=*/true)) {
        OptimalCompress::compressDXT1(toColor32(rgb.color(0)), &block->color);
    }
    else {
        ClusterFit fit;
        fit.setColorWeights(compressionOptions.colorWeight);
        fit.setColorSet(&rgb);

        Vector3 start, end;
        fit.compress4(&start, &end);

        QuickCompress::outputBlock4(rgb, start, end, &block->color);
    }
#endif

    // Decompress RGB/M block.
    nv::ColorBlock RGB;
    block->color.decodeBlock(&RGB);
    
#if 1
    AlphaBlock4x4 M;
    for (int i = 0; i < 16; i++) {
        const Vector4 & c = src.color(i);
        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float r = RGB.color(i).r / 255.0f;
        float g = RGB.color(i).g / 255.0f;
        float b = RGB.color(i).b / 255.0f;

        float m = (R / r + G / g + B / b) / 3.0f;
        //float m = max((R / r + G / g + B / b) / 3.0f, threshold);
        //float m = max(max(R / r, G / g), max(B / b, threshold));
        //float m = max(max(R, G), max(B, threshold));

        m = (m - threshold) / (1 - threshold);

        M.alpha[i] = U8(ftoi_round(saturate(m) * 255.0f));
        M.weights[i] = src.weights[i];
    }

    // Compress M.
    if (compressionOptions.quality == Quality_Fastest) {
        QuickCompress::compressDXT5A(M, &block->alpha);
    }
    else {
        OptimalCompress::compressDXT5A(M, &block->alpha);
    }
#else
    OptimalCompress::compressDXT5A_RGBM(src, RGB, &block->alpha);
#endif

#if 0
    // Decompress M.
    block->alpha.decodeBlock(&M);

    rgb.allocate(src.w, src.h);     // @@ Handle smaller blocks.

    for (uint i = 0; i < src.colorCount; i++) {
        const Vector4 & c = src.color(i);

        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        //float m = max(max(R, G), max(B, threshold));
        float m = float(M.alpha[i]) / 255.0f * (1 - threshold) + threshold;
        float r = R / m;
        float g = G / m;
        float b = B / m;
        float a = c.w;

        rgb.colors[i] = Vector4(r, g, b, a);
        rgb.indices[i] = i;
        rgb.weights[i] = max(c.w, 0.001f);// src.weights[i];   // IC: For some reason 0 weights are causing problems, even if we eliminate the corresponding colors from the set.
    }

    rgb.createMinimalSet(/*ignoreTransparent=*/true);

    if (rgb.isSingleColor(/*ignoreAlpha=*/true)) {
        OptimalCompress::compressDXT1(toColor32(rgb.color(0)), &block->color);
    }
    else {
        ClusterFit fit;
        fit.setMetric(compressionOptions.colorWeight);
        fit.setColourSet(&rgb);

        Vector3 start, end;
        fit.compress4(&start, &end);

        QuickCompress::outputBlock4(rgb, start, end, &block->color);
    }
#endif

#if 0
    block->color.decodeBlock(&RGB);

    //AlphaBlock4x4 M;
    //M.initWeights(src);
    
    for (int i = 0; i < 16; i++) {
        const Vector4 & c = src.color(i);
        float R = saturate(c.x);
        float G = saturate(c.y);
        float B = saturate(c.z);

        float r = RGB.color(i).r / 255.0f;
        float g = RGB.color(i).g / 255.0f;
        float b = RGB.color(i).b / 255.0f;

        float m = (R / r + G / g + B / b) / 3.0f;
        //float m = max((R / r + G / g + B / b) / 3.0f, threshold);
        //float m = max(max(R / r, G / g), max(B / b, threshold));
        //float m = max(max(R, G), max(B, threshold));

        m = (m - threshold) / (1 - threshold);

        M.alpha[i] = U8(ftoi_round(saturate(m) * 255.0f));
        M.weights[i] = src.weights[i];
    }

    // Compress M.
    if (compressionOptions.quality == Quality_Fastest) {
        QuickCompress::compressDXT5A(M, &block->alpha);
    }
    else {
        OptimalCompress::compressDXT5A(M, &block->alpha);
    }
#endif



#if 0
    src.fromRGBM(M, threshold);

    src.createMinimalSet(/*ignoreTransparent=*/true);

    if (src.isSingleColor(/*ignoreAlpha=*/true)) {
        OptimalCompress::compressDXT1(src.color(0), &block->color);
    }
    else {
        // @@ Use our improved compressor.
        ClusterFit fit;
        fit.setMetric(compressionOptions.colorWeight);
        fit.setColourSet(&src);

        Vector3 start, end;
        fit.compress4(&start, &end);

        if (fit.compress3(&start, &end)) {
            QuickCompress::outputBlock3(src, start, end, block->color);
        }
        else {
            QuickCompress::outputBlock4(src, start, end, block->color);
        }
    }
#endif // 0

    // @@ Decompress color and compute M that best approximates src with these colors? Then compress M again?



    // RGBM encoding.
    // Maximize precision.
    // - Number of possible grey levels:
    //   - Naive:  2^3 = 8
    //   - Better: 2^3 + 2^2 = 12
    //   - How to choose threshold? 
    //     - Ideal = Adaptive per block, don't know where to store.
    //     - Adaptive per lightmap. How to compute optimal?
    //     - Fixed: 0.25 in our case. Lightmaps scaled to a fixed [0, 1] range.

    // - Optimal compressor: Interpolation artifacts.

    // - Color transform. 
    //    - Measure error in post-tone-mapping color space. 
    //    - Assume a simple tone mapping operator. We know minimum and maximum exposure, but don't know exact exposure in game.
    //    - Guess based on average lighmap color? Use fixed exposure, in scaled lightmap space.

    // - Enhanced DXT compressor.
    //    - Typical RGBM encoding as follows:
    //      rgb -> M = max(rgb), RGB=rgb/M -> RGBM
    //    - If we add a compression step (M' = M) and M' < M, then rgb may be greater than 1.
    //      - We could ensure that M' >= M during compression.
    //      - We could clamp RGB anyway.
    //      - We could add a fixed scale value to take into account compression errors and avoid clamping.


    


    // Compress color.
    /*if (rgba.isSingleColor())
    {
        OptimalCompress::compressDXT1(rgba.color(0), &block->color);
    }
    else
    {
        nvsquish::WeightedClusterFit fit;
        fit.SetMetric(compressionOptions.colorWeight.x, compressionOptions.colorWeight.y, compressionOptions.colorWeight.z);

        int flags = 0;
        if (alphaMode == nvtt::AlphaMode_Transparency) flags |= nvsquish::kWeightColourByAlpha;

        nvsquish::ColourSet colours((uint8 *)rgba.colors(), flags);
        fit.SetColourSet(&colours, 0);
        fit.Compress(&block->color);
    }*/
}



#if defined(HAVE_ATITC)

void AtiCompressorDXT1::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    // Init source texture
    ATI_TC_Texture srcTexture;
    srcTexture.dwSize = sizeof(srcTexture);
    srcTexture.dwWidth = w;
    srcTexture.dwHeight = h;
    if (inputFormat == nvtt::InputFormat_BGRA_8UB)
    {
        srcTexture.dwPitch = w * 4;
        srcTexture.format = ATI_TC_FORMAT_ARGB_8888;
    }
    else
    {
        // @@ Floating point input is not swizzled.
        srcTexture.dwPitch = w * 16;
        srcTexture.format = ATI_TC_FORMAT_ARGB_32F;
    }
    srcTexture.dwDataSize = ATI_TC_CalculateBufferSize(&srcTexture);
    srcTexture.pData = (ATI_TC_BYTE*) data;

    // Init dest texture
    ATI_TC_Texture destTexture;
    destTexture.dwSize = sizeof(destTexture);
    destTexture.dwWidth = w;
    destTexture.dwHeight = h;
    destTexture.dwPitch = 0;
    destTexture.format = ATI_TC_FORMAT_DXT1;
    destTexture.dwDataSize = ATI_TC_CalculateBufferSize(&destTexture);
    destTexture.pData = (ATI_TC_BYTE*) mem::malloc(destTexture.dwDataSize);

    ATI_TC_CompressOptions options;
    options.dwSize = sizeof(options);
    options.bUseChannelWeighting = false;
    options.bUseAdaptiveWeighting = false;
    options.bDXT1UseAlpha = false;
    options.nCompressionSpeed = ATI_TC_Speed_Normal;
    options.bDisableMultiThreading = false;
    //options.bDisableMultiThreading = true;

    // Compress
    ATI_TC_ConvertTexture(&srcTexture, &destTexture, &options, NULL, NULL, NULL);

    if (outputOptions.outputHandler != NULL) {
            outputOptions.outputHandler->writeData(destTexture.pData, destTexture.dwDataSize);
    }

    mem::free(destTexture.pData);
}

void AtiCompressorDXT5::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    // Init source texture
    ATI_TC_Texture srcTexture;
    srcTexture.dwSize = sizeof(srcTexture);
    srcTexture.dwWidth = w;
    srcTexture.dwHeight = h;
    if (inputFormat == nvtt::InputFormat_BGRA_8UB)
    {
        srcTexture.dwPitch = w * 4;
        srcTexture.format = ATI_TC_FORMAT_ARGB_8888;
    }
    else
    {
        srcTexture.dwPitch = w * 16;
        srcTexture.format = ATI_TC_FORMAT_ARGB_32F;
    }
    srcTexture.dwDataSize = ATI_TC_CalculateBufferSize(&srcTexture);
    srcTexture.pData = (ATI_TC_BYTE*) data;

    // Init dest texture
    ATI_TC_Texture destTexture;
    destTexture.dwSize = sizeof(destTexture);
    destTexture.dwWidth = w;
    destTexture.dwHeight = h;
    destTexture.dwPitch = 0;
    destTexture.format = ATI_TC_FORMAT_DXT5;
    destTexture.dwDataSize = ATI_TC_CalculateBufferSize(&destTexture);
    destTexture.pData = (ATI_TC_BYTE*) mem::malloc(destTexture.dwDataSize);

    // Compress
    ATI_TC_ConvertTexture(&srcTexture, &destTexture, NULL, NULL, NULL, NULL);

    if (outputOptions.outputHandler != NULL) {
        outputOptions.outputHandler->writeData(destTexture.pData, destTexture.dwDataSize);
    }

    mem::free(destTexture.pData);
}

#endif // defined(HAVE_ATITC)

#if defined(HAVE_SQUISH)

void SquishCompressorDXT1::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);
    nvDebugCheck(false);

#pragma message(NV_FILE_LINE "TODO: Convert input to fixed point ABGR format instead of ARGB")
    /*
    Image img(*image);
    int count = img.width() * img.height();
    for (int i = 0; i < count; i++)
    {
            Color32 c = img.pixel(i);
            img.pixel(i) = Color32(c.b, c.g, c.r, c.a);
    }

    int size = squish::GetStorageRequirements(img.width(), img.height(), squish::kDxt1);
    void * blocks = mem::malloc(size);

    squish::CompressImage((const squish::u8 *)img.pixels(), img.width(), img.height(), blocks, squish::kDxt1 | squish::kColourClusterFit);

    if (outputOptions.outputHandler != NULL) {
            outputOptions.outputHandler->writeData(blocks, size);
    }

    mem::free(blocks);
    */
}

#endif // defined(HAVE_SQUISH)


#if defined(HAVE_D3DX)

void D3DXCompressorDXT1::compress(nvtt::InputFormat inputFormat, nvtt::AlphaMode alphaMode, uint w, uint h, uint d, void * data, const nvtt::CompressionOptions::Private & compressionOptions, const nvtt::OutputOptions::Private & outputOptions)
{
    nvDebugCheck(d == 1);

    IDirect3D9 * d3d = Direct3DCreate9(D3D_SDK_VERSION);

    D3DPRESENT_PARAMETERS presentParams;
    ZeroMemory(&presentParams, sizeof(presentParams));
    presentParams.Windowed = TRUE;
    presentParams.SwapEffect = D3DSWAPEFFECT_COPY;
    presentParams.BackBufferWidth = 8;
    presentParams.BackBufferHeight = 8;
    presentParams.BackBufferFormat = D3DFMT_UNKNOWN;

    HRESULT err;

    IDirect3DDevice9 * device = NULL;
    err = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetDesktopWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams, &device);

    IDirect3DTexture9 * texture = NULL;
    err = D3DXCreateTexture(device, w, h, 1, 0, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &texture);

    IDirect3DSurface9 * surface = NULL;
    err = texture->GetSurfaceLevel(0, &surface);

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.bottom = h;
    rect.right = w;

    if (inputFormat == nvtt::InputFormat_BGRA_8UB)
    {
        err = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, data, D3DFMT_A8R8G8B8, w * 4, NULL, &rect, D3DX_DEFAULT, 0);
    }
    else
    {
        err = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, data, D3DFMT_A32B32G32R32F, w * 16, NULL, &rect, D3DX_DEFAULT, 0);
    }

    if (err != D3DERR_INVALIDCALL && err != D3DXERR_INVALIDDATA)
    {
        D3DLOCKED_RECT rect;
        ZeroMemory(&rect, sizeof(rect));

        err = surface->LockRect(&rect, NULL, D3DLOCK_READONLY);

	    if (outputOptions.outputHandler != NULL) {
	        int size = rect.Pitch * ((h + 3) / 4);
	        outputOptions.outputHandler->writeData(rect.pBits, size);
	    }

        err = surface->UnlockRect();
    }

    surface->Release();
    device->Release();
    d3d->Release();
}

#endif // defined(HAVE_D3DX)


#if defined(HAVE_STB)

void StbCompressorDXT1::compressBlock(ColorBlock & rgba, nvtt::AlphaMode alphaMode, const nvtt::CompressionOptions::Private & compressionOptions, void * output)
{
    rgba.swizzle(2, 1, 0, 3); // Swap R and B
    stb_compress_dxt_block((unsigned char *)output, (unsigned char *)rgba.colors(), 0, 0);
}


#endif // defined(HAVE_STB)
