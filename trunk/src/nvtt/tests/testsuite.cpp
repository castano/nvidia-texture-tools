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

#include <nvtt/nvtt.h>
#include <nvimage/Image.h>
#include <nvimage/ImageIO.h>
#include <nvimage/BlockDXT.h>
#include <nvimage/ColorBlock.h>
#include <nvcore/Ptr.h>
#include <nvcore/Debug.h>
#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>
#include <nvcore/TextWriter.h>
#include <nvcore/FileSystem.h>
#include <nvcore/Timer.h>

#include <stdlib.h> // free
#include <string.h> // memcpy

#include "../tools/cmdline.h"

using namespace nv;

// Kodak image set
static const char * s_kodakImageSet[] = {
    "kodim01.png",
    "kodim02.png",
    "kodim03.png",
    "kodim04.png",
    "kodim05.png",
    "kodim06.png",
    "kodim07.png",
    "kodim08.png",
    "kodim09.png",
    "kodim10.png",
    "kodim11.png",
    "kodim12.png",
    "kodim13.png",
    "kodim14.png",
    "kodim15.png",
    "kodim16.png",
    "kodim17.png",
    "kodim18.png",
    "kodim19.png",
    "kodim20.png",
    "kodim21.png",
    "kodim22.png",
    "kodim23.png",
    "kodim24.png",
};

// Waterloo image set
static const char * s_waterlooImageSet[] = {
    "clegg.png",
    "frymire.png",
    "lena.png",
    "monarch.png",
    "peppers.png",
    "sail.png",
    "serrano.png",
    "tulips.png",
};

// Epic image set
static const char * s_epicImageSet[] = {
    "Bradley1.png",
    "Gradient.png",
    "MoreRocks.png",
    "Wall.png",
    "Rainbow.png",
    "Text.png",
};

// Farbrausch
static const char * s_farbrauschImageSet[] = {
    "t.2d.pn02.bmp",
    "t.aircondition.01.bmp",
    "t.bricks.02.bmp",
    "t.bricks.05.bmp",
    "t.concrete.cracked.01.bmp",
    "t.envi.colored02.bmp",
    "t.envi.colored03.bmp",
    "t.font.01.bmp",
    "t.sewers.01.bmp",
    "t.train.03.bmp",
    "t.yello.01.bmp",
};

// Lugaru
static const char * s_lugaruImageSet[] = {
    "lugaru-blood.png",
    "lugaru-bush.png",
    "lugaru-cursor.png",
    "lugaru-hawk.png",
};

// Quake3
static const char * s_quake3ImageSet[] = {
    "q3-blocks15cgeomtrn.tga",
    "q3-blocks17bloody.tga",
    "q3-dark_tin2.tga",
    "q3-fan_grate.tga",
    "q3-fan.tga",
    "q3-metal2_2.tga",
    "q3-panel_glo.tga",
    "q3-proto_fence.tga",
    "q3-wires02.tga",
};

enum Mode {
    Mode_BC1,
    Mode_BC3_Alpha,
    Mode_BC3_YCoCg,
    Mode_BC3_RGBM,
    Mode_BC3_Normal,
    Mode_BC5_Normal,
};
struct ImageSet
{
    const char * name;
    const char ** fileNames;
    int fileCount;
    Mode mode;
};

#define ARRAY_SIZE(a) sizeof(a)/sizeof(a[0])

static ImageSet s_imageSets[] = {
    {"Kodak - BC1",             s_kodakImageSet,        ARRAY_SIZE(s_kodakImageSet),        Mode_BC1},
    {"Kodak - BC3-YCoCg",       s_kodakImageSet,        ARRAY_SIZE(s_kodakImageSet),        Mode_BC3_YCoCg},
    {"Kodak - BC3-RGBM",        s_kodakImageSet,        ARRAY_SIZE(s_kodakImageSet),        Mode_BC3_RGBM},
    {"Waterloo - BC1",          s_waterlooImageSet,     ARRAY_SIZE(s_waterlooImageSet),     Mode_BC1},
    {"Waterloo - BC3-YCoCg",    s_waterlooImageSet,     ARRAY_SIZE(s_waterlooImageSet),     Mode_BC3_YCoCg},
    {"Epic - BC1",              s_epicImageSet,         ARRAY_SIZE(s_epicImageSet),         Mode_BC1},
    {"Epic - BC1-YCoCg",        s_epicImageSet,         ARRAY_SIZE(s_epicImageSet),         Mode_BC3_YCoCg},
    {"Farbraush - BC1",         s_farbrauschImageSet,   ARRAY_SIZE(s_farbrauschImageSet),   Mode_BC1},
    {"Farbraush - BC1-YCoCg",   s_farbrauschImageSet,   ARRAY_SIZE(s_farbrauschImageSet),   Mode_BC3_YCoCg},
    {"Lugaru - BC3",            s_lugaruImageSet,       ARRAY_SIZE(s_lugaruImageSet),       Mode_BC3_Alpha},
    {"Quake3 - BC3",            s_quake3ImageSet,       ARRAY_SIZE(s_quake3ImageSet),       Mode_BC3_Alpha},
};
const int s_imageSetCount = sizeof(s_imageSets)/sizeof(s_imageSets[0]);

struct MyOutputHandler : public nvtt::OutputHandler
{
    MyOutputHandler() : m_data(NULL), m_ptr(NULL) {}
    ~MyOutputHandler()
    {
        free(m_data);
    }

    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
    {
        m_size = size;
        m_width = width;
        m_height = height;
        free(m_data);
        m_data = (unsigned char *)malloc(size);
        m_ptr = m_data;
    }

    virtual bool writeData(const void * data, int size)
    {
        memcpy(m_ptr, data, size);
        m_ptr += size;
        return true;
    }

    nvtt::TexImage decompress(Mode mode, nvtt::Decoder decoder)
    {
        nvtt::Format format; 
        if (mode == Mode_BC1) format = nvtt::Format_BC1;
        else if (mode == Mode_BC5_Normal) format = nvtt::Format_BC5;
        else  format = nvtt::Format_BC3;

        nvtt::TexImage img;
        img.setImage2D(format, decoder, m_width, m_height, m_data);

        return img;
    }

    int m_size;
    int m_width;
    int m_height;
    unsigned char * m_data;
    unsigned char * m_ptr;
};


int main(int argc, char *argv[])
{
    MyAssertHandler assertHandler;
    MyMessageHandler messageHandler;

    const uint version = nvtt::version();
    const uint major = version / 100 / 100;
    const uint minor = (version / 100) % 100;
    const uint rev = version % 100;

    printf("NVIDIA Texture Tools %u.%u.%u - Copyright NVIDIA Corporation 2007\n\n", major, minor, rev);

    int setIndex = 0;
    bool fast = false;
    bool nocuda = false;
    bool showHelp = false;
    nvtt::Decoder decoder = nvtt::Decoder_Reference;
    const char * basePath = "";
    const char * outPath = "output";
    const char * regressPath = NULL;

    // Parse arguments.
    for (int i = 1; i < argc; i++)
    {
        if (strcmp("-set", argv[i]) == 0)
        {
            if (i+1 < argc && argv[i+1][0] != '-') {
                setIndex = atoi(argv[i+1]);
                i++;
            }
        }
        else if (strcmp("-dec", argv[i]) == 0)
        {
            if (i+1 < argc && argv[i+1][0] != '-') {
                decoder = (nvtt::Decoder)atoi(argv[i+1]);
                i++;
            }
        }
        else if (strcmp("-fast", argv[i]) == 0)
        {
            fast = true;
        }
        else if (strcmp("-nocuda", argv[i]) == 0)
        {
            nocuda = true;
        }
        else if (strcmp("-help", argv[i]) == 0)
        {
            showHelp = true;
        }
        else if (strcmp("-path", argv[i]) == 0)
        {
            if (i+1 < argc && argv[i+1][0] != '-') {
                basePath = argv[i+1];
                i++;
            }
        }
        else if (strcmp("-out", argv[i]) == 0)
        {
            if (i+1 < argc && argv[i+1][0] != '-') {
                outPath = argv[i+1];
                i++;
            }
        }
        else if (strcmp("-regress", argv[i]) == 0)
        {
            if (i+1 < argc && argv[i+1][0] != '-') {
                regressPath = argv[i+1];
                i++;
            }
        }
    }

    if (showHelp)
    {
        printf("usage: nvtestsuite [options]\n\n");

        printf("Input options:\n");
        printf("  -path <path>   \tInput image path.\n");
        printf("  -regress <path>\tRegression directory.\n");
        printf("  -set [0:2]     \tImage set.\n");
        printf("    0:           \tKodak.\n");
        printf("    1:           \tWaterloo.\n");
        printf("    2:           \tEpic.\n");
        printf("    3:           \tFarbrausch.\n");
        printf("    4:           \tLugaru.\n");
        printf("    5:           \tQuake 3.\n");
        printf("  -dec x         \tDecompressor.\n");
        printf("    0:           \tReference.\n");
        printf("    1:           \tNVIDIA.\n");

        printf("Compression options:\n");
        printf("  -fast          \tFast compression.\n");
        printf("  -nocuda        \tDo not use cuda compressor.\n");

        printf("Output options:\n");
        printf("  -out <path>    \tOutput directory.\n");

        return 1;
    }

    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(nvtt::Format_BC1);
    if (fast)
    {
        compressionOptions.setQuality(nvtt::Quality_Fastest);
    }
    else
    {
        compressionOptions.setQuality(nvtt::Quality_Production);
    }
    //compressionOptions.setExternalCompressor("ati");
    //compressionOptions.setExternalCompressor("squish");
    //compressionOptions.setExternalCompressor("d3dx");
    //compressionOptions.setExternalCompressor("stb");

    
    const ImageSet & set = s_imageSets[setIndex];

    if (set.mode == Mode_BC1) {
        compressionOptions.setFormat(nvtt::Format_BC1);
    }
    else if (set.mode == Mode_BC3_Alpha || set.mode == Mode_BC3_YCoCg || set.mode == Mode_BC3_RGBM) {
        compressionOptions.setFormat(nvtt::Format_BC3);
    }
    else if (set.mode == Mode_BC3_Normal) {
        compressionOptions.setFormat(nvtt::Format_BC3n);
    }
    else if (set.mode == Mode_BC5_Normal) {
        compressionOptions.setFormat(nvtt::Format_BC5);
    }
   


    nvtt::OutputOptions outputOptions;
    outputOptions.setOutputHeader(false);

    MyOutputHandler outputHandler;
    outputOptions.setOutputHandler(&outputHandler);

    nvtt::Context context;
    context.enableCudaAcceleration(!nocuda);

    FileSystem::changeDirectory(basePath);
    FileSystem::createDirectory(outPath);

    Path csvFileName;
    csvFileName.format("%s/result-%d.csv", outPath, setIndex);
    StdOutputStream csvStream(csvFileName.str());
    TextWriter csvWriter(&csvStream);

    float totalTime = 0;
    float totalRMSE = 0;
    int failedTests = 0;
    float totalDiff = 0;

    const char ** fileNames = set.fileNames;
    int fileCount = set.fileCount;

    Timer timer;

    nvtt::TexImage img;
    if (set.mode == Mode_BC3_Alpha) {
        img.setAlphaMode(nvtt::AlphaMode_Transparency);
    }
    if (set.mode == Mode_BC3_Normal || set.mode == Mode_BC5_Normal) {
        img.setNormalMap(true);
    }

    printf("Processing Set: %s\n", set.name);

    for (int i = 0; i < fileCount; i++)
    {
        if (!img.load(fileNames[i]))
        {
            printf("Input image '%s' not found.\n", fileNames[i]);
            return EXIT_FAILURE;
        }

        if (img.isNormalMap()) {
            img.normalizeNormalMap();
        }

        if (set.mode == Mode_BC3_YCoCg) {
            img.toYCoCg();
            img.blockScaleCoCg();
            img.scaleBias(0, 0.5, 0.5);
            img.scaleBias(1, 0.5, 0.5);
        }
        else if (set.mode == Mode_BC3_RGBM) {
            img.toRGBM();
        }

        printf("Compressing: \t'%s'\n", fileNames[i]);

        timer.start();

        context.compress(img, 0, 0, compressionOptions, outputOptions);

        timer.stop();
        printf("  Time: \t%.3f sec\n", timer.elapsed());
        totalTime += timer.elapsed();

        nvtt::TexImage img_out = outputHandler.decompress(set.mode, decoder);
        if (set.mode == Mode_BC3_Alpha) {
            img_out.setAlphaMode(nvtt::AlphaMode_Transparency);
        }
        if (set.mode == Mode_BC3_Normal || set.mode == Mode_BC5_Normal) {
            img_out.setNormalMap(true);
        }

        if (set.mode == Mode_BC3_YCoCg) {
            img_out.scaleBias(0, 1.0, -0.5);
            img_out.scaleBias(1, 1.0, -0.5);
            img_out.fromYCoCg();

            img.scaleBias(0, 1.0, -0.5);
            img.scaleBias(1, 1.0, -0.5);
            img.fromYCoCg();
        }
        else if (set.mode == Mode_BC3_RGBM) {
            img_out.fromRGBM();
            img.fromRGBM();
        }

        Path outputFileName;
        outputFileName.format("%s/%s", outPath, fileNames[i]);
        outputFileName.stripExtension();
        outputFileName.append(".png");
        if (!img_out.save(outputFileName.str()))
        {
            printf("Error saving file '%s'.\n", outputFileName.str());
        }

        float rmse = nvtt::rmsError(img, img_out);
        totalRMSE += rmse;

        printf("  RMSE:  \t%.4f\n", rmse);

        // Output csv file
        csvWriter << "\"" << fileNames[i] << "\"," << rmse << "\n";

        if (regressPath != NULL)
        {
            Path regressFileName;
            regressFileName.format("%s/%s", regressPath, fileNames[i]);
            regressFileName.stripExtension();
            regressFileName.append(".png");

            nvtt::TexImage img_reg;
            if (!img_reg.load(regressFileName.str()))
            {
                printf("Regression image '%s' not found.\n", regressFileName.str());
                return EXIT_FAILURE;
            }

            float rmse_reg = rmsError(img, img_reg);

            float diff = rmse_reg - rmse;
            totalDiff += diff;

            const char * text = "PASSED";
            if (equal(diff, 0)) text = "PASSED";
            else if (diff < 0) {
                text = "FAILED";
                failedTests++;
            }

            printf("  Diff: \t%.4f (%s)\n", diff, text);
        }

        fflush(stdout);
    }

    totalRMSE /= fileCount;
    totalDiff /= fileCount;

    printf("Total Results:\n");
    printf("  Total Time: \t%.3f sec\n", totalTime);
    printf("  Average RMSE:\t%.4f\n", totalRMSE);

    if (regressPath != NULL)
    {
        printf("Regression Results:\n");
        printf("  Diff: %.4f\n", totalDiff);
        printf("  %d/%d tests failed.\n", failedTests, fileCount);
    }

    return EXIT_SUCCESS;
}

