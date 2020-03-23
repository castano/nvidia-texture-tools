
#define  _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdlib.h>

//#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

#include "../extern/libsquish-1.15/squish.h"

#include "../extern/CMP_Core/source/CMP_Core.h"

#include "nvtt/CompressorDXT1.h"

#include "nvmath/Vector.h"
#include "nvmath/Color.h"

#include "nvcore/Timer.h"
#include "nvcore/Array.inl"

using namespace nv;

typedef unsigned char u8;
typedef unsigned int u32;


// Defer statement:
#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)

template<typename T>
struct ExitScope
{
    T lambda;
    ExitScope(T lambda)
        : lambda(lambda)
    {
    }
    ~ExitScope() { lambda(); }

private:
    ExitScope& operator=(const ExitScope&);
};

class ExitScopeHelp
{
public:
    template<typename T>
    ExitScope<T> operator+(T t) { return t; }
};

#define defer const auto& __attribute__((unused)) CONCAT(defer__, __LINE__) = ExitScopeHelp() + [&]()


static float mse_to_psnr(float mse) {
    float rms = sqrtf(mse);
    float psnr = rms ? (float)clamp(log10(255.0 / rms) * 20.0, 0.0, 300.0) : 1e+10f;
    return psnr;
}

/*
void image_metrics::calc(const image &a, const image &b, uint32_t first_chan, uint32_t total_chans, bool avg_comp_error, bool use_601_luma)
{
    //assert((first_chan < 4U) && (first_chan + total_chans <= 4U));

    const uint32_t width = std::min(a.get_width(), b.get_width());
    const uint32_t height = std::min(a.get_height(), b.get_height());

    double hist[256];
    memset(hist, 0, sizeof(hist));

    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            const color_rgba &ca = a(x, y), &cb = b(x, y);

            for (uint32_t c = 0; c < 3; c++)
                hist[iabs(ca[first_chan + c] - cb[first_chan + c])]++;
        }
    }

    m_max = 0;
    double sum = 0.0f, sum2 = 0.0f;
    for (uint32_t i = 0; i < 256; i++)
    {
        if (hist[i])
        {
            m_max = std::max<float>(m_max, (float)i);
            double v = i * hist[i];
            sum += v;
            sum2 += i * v;
        }
    }

    double total_values = (double)width * (double)height;
    if (avg_comp_error)
        total_values *= (double)clamp<uint32_t>(total_chans, 1, 4);

    m_mean = (float)clamp<double>(sum / total_values, 0.0f, 255.0);
    m_mean_squared = (float)clamp<double>(sum2 / total_values, 0.0f, 255.0 * 255.0);
    m_rms = (float)sqrt(m_mean_squared);
    m_psnr = m_rms ? (float)clamp<double>(log10(255.0 / m_rms) * 20.0, 0.0f, 300.0f) : 1e+10f;
}
*/

// Returns mse.
float evaluate_dxt1_mse(uint8 * rgba, uint8 * block, int block_count, int decoder = 2) {
    double total = 0.0f;
    for (int b = 0; b < block_count; b++) {
        total += nv::evaluate_dxt1_error(rgba, (BlockDXT1 *)block, decoder) / 255.0;
        rgba += 4 * 4 * 4;
        block += 8;
    }
    return float(total / (3 * 16 * block_count));
}

#define MAKEFOURCC(str) (uint(str[0]) | (uint(str[1]) << 8) | (uint(str[2]) << 16) | (uint(str[3]) << 24 ))


bool output_dxt_dds (u32 w, u32 h, const u8* data, const char * filename) {

    const u32 DDSD_CAPS = 0x00000001;
    const u32 DDSD_PIXELFORMAT = 0x00001000;
    const u32 DDSD_WIDTH = 0x00000004;
    const u32 DDSD_HEIGHT = 0x00000002;
    const u32 DDSD_LINEARSIZE = 0x00080000;
    const u32 DDPF_FOURCC = 0x00000004;
    const u32 DDSCAPS_TEXTURE = 0x00001000;

    struct DDS {
        u32 fourcc = MAKEFOURCC("DDS ");
        u32 size = 124;
        u32 flags = DDSD_CAPS|DDSD_PIXELFORMAT|DDSD_WIDTH|DDSD_HEIGHT|DDSD_LINEARSIZE;
        u32 height;
        u32 width;
        u32 pitch;
        u32 depth;
        u32 mipmapcount;
        u32 reserved [11];
        struct {
            u32 size = 32;
            u32 flags = DDPF_FOURCC;
            u32 fourcc = MAKEFOURCC("DXT1");
            u32 bitcount;
            u32 rmask;
            u32 gmask;
            u32 bmask;
            u32 amask;
        } pf;
        struct {
            u32 caps1 = DDSCAPS_TEXTURE;
            u32 caps2;
            u32 caps3;
            u32 caps4;
        } caps;
        u32 notused;
    } dds;
    static_assert(sizeof(DDS) == 128, "DDS size must be 128");

    dds.width = w;
    dds.height = h;
    dds.pitch = 8 * ((w+3)/4 * (h+3)/4); // linear size

    FILE * fp = fopen(filename, "wb");
    if (fp == nullptr) return false;

    // Write header:
    fwrite(&dds, sizeof(dds), 1, fp);

    // Write dxt data:
    fwrite(data, dds.pitch, 1, fp);

    fclose(fp);

    return true;
}

const int COMPRESSOR_COUNT = 7;
struct Stats {
    const char * compressorName;
    Array<float> mseArray;
    Array<float> timeArray;
};


bool test_bc1(const char * inputFileName, int index, Stats * stats) {

    int w, h, n;
    unsigned char *input_data = stbi_load(inputFileName, &w, &h, &n, 4);
    defer { stbi_image_free(input_data); };

    if (input_data == nullptr) {
        printf("Failed to load input image '%s'.\n", inputFileName);
        return false;
    }


    int block_count = (w / 4) * (h / 4);
    u8 * rgba_block_data = (u8 *)malloc(block_count * 4 * 4 * 4);
    defer { free(rgba_block_data); };

    int bw = 4 * (w / 4); // Round down.
    int bh = 4 * (h / 4);

    // Convert to block layout.
    for (int y = 0, b = 0; y < bh; y += 4) {
        for (int x = 0; x < bw; x += 4, b++) {
            for (int yy = 0; yy < 4; yy++) {
                for (int xx = 0; xx < 4; xx++) {
                    if (x + xx < w && y + yy < h) {
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 0] = input_data[((y + yy) * w + x + xx) * 4 + 0];
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 1] = input_data[((y + yy) * w + x + xx) * 4 + 1];
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 2] = input_data[((y + yy) * w + x + xx) * 4 + 2];
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 3] = input_data[((y + yy) * w + x + xx) * 4 + 3];
                    }
                    else {
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 0] = 0;
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 1] = 0;
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 2] = 0;
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 3] = 0;
                    }
                }
            }
        }
    }

    u8 * block_data = (u8 *)malloc(block_count * 8);

    Timer timer;

    // Warmup.
    for (int b = 0; b < block_count; b++) {
        stb_compress_dxt_block(block_data + b * 8, rgba_block_data + b * 4 * 4 * 4, 0, STB_DXT_NORMAL);
    }

#if _DEBUG
    const int repeat_count = 1;
#else
    const int repeat_count = 1; // 8
#endif

    {
        memset(block_data, 0, block_count * 8);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                stb_compress_dxt_block(block_data + b * 8, rgba_block_data + b * 4 * 4 * 4, 0, STB_DXT_NORMAL);
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("stb_dxt \t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "stb_dxt.dds");
        stats->compressorName = "stb";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }

    {
        memset(block_data, 0, block_count * 8);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                stb_compress_dxt_block(block_data + b * 8, rgba_block_data + b * 4 * 4 * 4, 0, STB_DXT_HIGHQUAL);
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("stb_dxt hq \t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "stb_dxt_hq.dds");
        stats->compressorName = "stb-hq";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }

    {
        memset(block_data, 0, block_count * 8);
        Vector3 color_weights(1);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                Vector4 input_colors[16];
                float input_weights[16];
                for (int j = 0; j < 16; j++) {
                    input_colors[j].x = rgba_block_data[b * 4 * 4 * 4 + j * 4 + 0] / 255.0f;
                    input_colors[j].y = rgba_block_data[b * 4 * 4 * 4 + j * 4 + 1] / 255.0f;
                    input_colors[j].z = rgba_block_data[b * 4 * 4 * 4 + j * 4 + 2] / 255.0f;
                    input_colors[j].w = 255.0f;
                    input_weights[j] = 1.0f;
                }

                compress_dxt1_fast(input_colors, input_weights, color_weights, (BlockDXT1*)(block_data + b * 8));
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("nvtt fast \t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "nvtt_fast.dds");
        stats->compressorName = "nvtt-fast";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }

    {
        memset(block_data, 0, block_count * 8);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                //compress_dxt1_fast2(rgba_block_data + b * 4 * 4 * 4, (BlockDXT1*)(block_data + b * 8));
                compress_dxt1_fast_geld(rgba_block_data + b * 4 * 4 * 4, (BlockDXT1*)(block_data + b * 8));
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("nvtt fast2 \t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "nvtt_fast2.dds");
        stats->compressorName = "nvtt-geld";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }

    {
        memset(block_data, 0, block_count * 8);
        Vector3 color_weights(1);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                Vector4 input_colors[16];
                float input_weights[16];
                for (int j = 0; j < 16; j++) {
                    input_colors[j].x = rgba_block_data[b * 4 * 4 * 4 + j * 4 + 0] / 255.0f;
                    input_colors[j].y = rgba_block_data[b * 4 * 4 * 4 + j * 4 + 1] / 255.0f;
                    input_colors[j].z = rgba_block_data[b * 4 * 4 * 4 + j * 4 + 2] / 255.0f;
                    input_colors[j].w = 1.0f;
                    input_weights[j] = 1.0f;
                }

                compress_dxt1(input_colors, input_weights, color_weights, false, (BlockDXT1*)(block_data + b * 8));
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("nvtt hq  \t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "nvtt_hq.dds");
        stats->compressorName = "nvtt-hq";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }

    {
        memset(block_data, 0, block_count * 8);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                squish::Compress(rgba_block_data + b * 4 * 4 * 4, block_data + b * 8, squish::kDxt1);
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("squish   \t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "squish.dds");
        stats->compressorName = "squish";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }

    /*{
        memset(block_data, 0, block_count * 8);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                squish::Compress(rgba_block_data + b * 4 * 4 * 4, block_data + b * 8, squish::kDxt1 | squish::kColourIterativeClusterFit);
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("squish hq\t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "squish_hq.dds");
        stats->compressorName = "squish-hq";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }*/

    {
        memset(block_data, 0, block_count * 8);

        timer.start();
        for (int i = 0; i < repeat_count; i++) {
            for (int b = 0; b < block_count; b++) {
                CompressBlockBC1(rgba_block_data + b * 4 * 4 * 4, 16, block_data + b * 8, nullptr);
            }
        }
        timer.stop();

        float mse = evaluate_dxt1_mse(rgba_block_data, block_data, block_count);
        //printf("squish   \t%f\t-> %f %f\n", timer.elapsed(), sqrt(mse), mse_to_psnr(mse));

        //output_dxt_dds(bw, bh, block_data, "squish.dds");
        stats->compressorName = "cmp";
        stats->mseArray[index] = mse;
        stats->timeArray[index] = timer.elapsed();
        stats++;
    }

    return false;
}



bool analyze_bc1(const char * inputFileName) {

    int w, h, n;
    unsigned char *input_data = stbi_load(inputFileName, &w, &h, &n, 4);
    defer { stbi_image_free(input_data); };

    if (input_data == nullptr) {
        printf("Failed to load input image '%s'.\n", inputFileName);
        return false;
    }

    int block_count = (w / 4) * (h / 4);
    u8 * rgba_block_data = (u8 *)malloc(block_count * 4 * 4 * 4);
    defer { free(rgba_block_data); };

    int bw = 4 * (w / 4); // Round down.
    int bh = 4 * (h / 4);

    // Convert to block layout.
    for (int y = 0, b = 0; y < bh; y += 4) {
        for (int x = 0; x < bw; x += 4, b++) {
            for (int yy = 0; yy < 4; yy++) {
                for (int xx = 0; xx < 4; xx++) {
                    if (x + xx < w && y + yy < h) {
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 0] = input_data[((y + yy) * w + x + xx) * 4 + 0];
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 1] = input_data[((y + yy) * w + x + xx) * 4 + 1];
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 2] = input_data[((y + yy) * w + x + xx) * 4 + 2];
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 3] = input_data[((y + yy) * w + x + xx) * 4 + 3];
                    }
                    else {
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 0] = 0;
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 1] = 0;
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 2] = 0;
                        rgba_block_data[b * 4 * 4 * 4 + (yy * 4 + xx) * 4 + 3] = 0;
                    }
                }
            }
        }
    }

    u8 * block_data = (u8 *)malloc(block_count * 8);
    memset(block_data, 0, block_count * 8);

    Timer timer;

    int stb_better_than_nvtt_fast = 0;
    int stb_better_than_nvtt_hq = 0;
    int squish_better_than_nvtt_hq = 0;

    int this_should_never_happen = 0;
    int this_should_never_happen_either = 0;
        
    Vector3 color_weights(1);

    for (int b = 0; b < block_count; b++) {

        uint8 * rgba_block = rgba_block_data + b * 4 * 4 * 4;
        uint8 * dxt_block = block_data + b * 8;

        Vector4 input_colors[16];
        float input_weights[16];
        for (int j = 0; j < 16; j++) {
            input_colors[j].x = rgba_block[j * 4 + 0] / 255.0f;
            input_colors[j].y = rgba_block[j * 4 + 1] / 255.0f;
            input_colors[j].z = rgba_block[j * 4 + 2] / 255.0f;
            input_colors[j].w = 255.0f;
            input_weights[j] = 1.0f;
        }

        // Compare all the different modes on the same block:

        stb_compress_dxt_block(dxt_block, rgba_block, 0, STB_DXT_NORMAL);
        float mse_stb = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        stb_compress_dxt_block(dxt_block, rgba_block, 0, STB_DXT_HIGHQUAL);
        float mse_stb_hq = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        compress_dxt1_fast(input_colors, input_weights, color_weights, (BlockDXT1*)dxt_block);
        float mse_nvtt_fast = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        compress_dxt1_fast2(rgba_block, (BlockDXT1*)dxt_block);
        float mse_nvtt_fast2 = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        compress_dxt1_fast_geld(rgba_block, (BlockDXT1*)dxt_block);
        float mse_nvtt_geld = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        compress_dxt1(input_colors, input_weights, color_weights, false, (BlockDXT1*)dxt_block);
        float mse_nvtt_hq = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        squish::Compress(rgba_block, dxt_block, squish::kDxt1);
        float mse_squish = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        squish::Compress(rgba_block, dxt_block, squish::kDxt1 | squish::kColourIterativeClusterFit);
        float mse_squish_hq = nv::evaluate_dxt1_error(rgba_block, (BlockDXT1 *)dxt_block);

        if (mse_stb < mse_nvtt_fast) {
            stb_better_than_nvtt_fast++;
        }
        if (mse_stb < mse_nvtt_hq) {
            stb_better_than_nvtt_hq++;
        }
        if (mse_squish < mse_nvtt_hq) {
            squish_better_than_nvtt_hq++;
        }
        if (mse_nvtt_fast < mse_nvtt_hq) {
            this_should_never_happen++;
        }
        if (mse_nvtt_fast2 < mse_nvtt_fast) {
            this_should_never_happen_either++;
        }
    }

    return true;
}




const char * image_set[] = {
    "testsuite/kodak/kodim01.png",
    "testsuite/kodak/kodim02.png",
    "testsuite/kodak/kodim03.png",
    "testsuite/kodak/kodim04.png",
    "testsuite/kodak/kodim05.png",
    "testsuite/kodak/kodim06.png",
    "testsuite/kodak/kodim07.png",
    "testsuite/kodak/kodim08.png",
    "testsuite/kodak/kodim09.png",
    "testsuite/kodak/kodim10.png",
    "testsuite/kodak/kodim11.png",
    "testsuite/kodak/kodim12.png",
    "testsuite/kodak/kodim13.png",
    "testsuite/kodak/kodim14.png",
    "testsuite/kodak/kodim15.png",
    "testsuite/kodak/kodim16.png",
    "testsuite/kodak/kodim17.png",
    "testsuite/kodak/kodim18.png",
    "testsuite/kodak/kodim19.png",
    "testsuite/kodak/kodim20.png",
    "testsuite/kodak/kodim21.png",
    "testsuite/kodak/kodim22.png",
    "testsuite/kodak/kodim23.png",
    "testsuite/kodak/kodim24.png",
    "testsuite/waterloo/clegg.png",
    "testsuite/waterloo/frymire.png",
    "testsuite/waterloo/lena.png",
    "testsuite/waterloo/monarch.png",
    "testsuite/waterloo/peppers.png",
    "testsuite/waterloo/sail.png",
    "testsuite/waterloo/serrano.png",
    "testsuite/waterloo/tulips.png",
};

const char * roblox_set[] = {
    "Roblox/asphalt_side/diffuse.tga",
    "Roblox/asphalt_top/diffuse.tga",
    "Roblox/basalt/diffuse.tga",
    "Roblox/brick/diffuse.tga",
    "Roblox/cobblestone_side/diffuse.tga",
    "Roblox/cobblestone_top/diffuse.tga",
    "Roblox/concrete_side/diffuse.tga",
    "Roblox/concrete_top/diffuse.tga",
    "Roblox/crackedlava/diffuse.tga",
    "Roblox/glacier_bottom/diffuse.tga",
    "Roblox/glacier_side/diffuse.tga",
    "Roblox/glacier_top/diffuse.tga",
    "Roblox/grass_bottom/diffuse.tga",
    "Roblox/grass_side/diffuse.tga",
    "Roblox/grass_top/diffuse.tga",
    "Roblox/ground/diffuse.tga",
    "Roblox/ice_side/diffuse.tga",
    "Roblox/ice_top/diffuse.tga",
    "Roblox/leafygrass_side/diffuse.tga",
    "Roblox/leafygrass_top/diffuse.tga",
    "Roblox/limestone_side/diffuse.tga",
    "Roblox/limestone_top/diffuse.tga",
    "Roblox/mud/diffuse.tga",
    "Roblox/pavement_side/diffuse.tga",
    "Roblox/pavement_top/diffuse.tga",
    "Roblox/rock/diffuse.tga",
    "Roblox/salt_side/diffuse.tga",
    "Roblox/salt_top/diffuse.tga",
    "Roblox/sand_side/diffuse.tga",
    "Roblox/sand_top/diffuse.tga",
    "Roblox/sandstone_bottom/diffuse.tga",
    "Roblox/sandstone_side/diffuse.tga",
    "Roblox/sandstone_top/diffuse.tga",
    "Roblox/slate/diffuse.tga",
    "Roblox/snow/diffuse.tga",
    "Roblox/woodplanks/diffuse.tga",
};




int main(int argc, char *argv[])
{
    const char * inputFileName = "testsuite/kodak/kodim14.png";
    //const char * inputFileName = "testsuite/kodak/kodim18.png";
    //const char * inputFileName = "testsuite/kodak/kodim15.png";
    //const char * inputFileName = "testsuite/waterloo/frymire.png";
    // test_bc1(inputFileName, 0);

    analyze_bc1(inputFileName);

    //const char ** set = roblox_set;
    //int count = sizeof(roblox_set) / sizeof(char*);

    const char ** set = image_set;
    int count = sizeof(image_set) / sizeof(char*);

    Stats stats[COMPRESSOR_COUNT];

    for (int i = 0; i < COMPRESSOR_COUNT; i++) {
        stats[i].compressorName = nullptr;
        stats[i].mseArray.resize(count, 0.0f);
        stats[i].timeArray.resize(count, 0.0f);
    }

    for (int i = 0; i < count; i++) {
        printf("\nImage '%s'\n", set[i]);

        test_bc1(set[i], i, stats);

        for (int c = 0; c < COMPRESSOR_COUNT; c++) {
            if (stats[c].compressorName) {
                printf("%-16s %f\t%f\n", stats[c].compressorName, sqrtf(stats[c].mseArray[i]), stats[c].timeArray[i]);
            }
        }
    }

    // Print stats.
    printf("\nAverage Results:\n");
    for (int c = 0; c < COMPRESSOR_COUNT; c++) {
        if (stats[c].compressorName) {
            float sum = 0.0f;
            for (float it : stats[c].mseArray) {
                sum += it;
            }
            sum /= count;

            float time = 0.0f;
            for (float it : stats[c].timeArray) {
                time += it;
            }

            printf("%-16s %f\t%f\n", stats[c].compressorName, sqrtf(sum), time);
        }
    }

    return EXIT_SUCCESS;
}
