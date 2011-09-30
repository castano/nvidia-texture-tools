// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
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

#include <nvcore/StrLib.h>
#include <nvtt/nvtt.h>

#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE
#include <stdio.h> // printf


int main(int argc, char *argv[])
{
    // Init context.
    nvtt::Context context;

    // Load cubemap.
    nvtt::CubeSurface envmap;
    if (!envmap.load("envmap.dds", 0)) {
        printf("Error loading envmap.dds\n");
        return EXIT_FAILURE;
    }

    envmap.toLinear(2.2f);


    // Setup compression options.
    nvtt::CompressionOptions compressionOptions;
    compressionOptions.setFormat(nvtt::Format_RGBA);
    compressionOptions.setPixelType(nvtt::PixelType_Float);
    compressionOptions.setPixelFormat(16, 16, 16, 16);

    // Setup output options.
    nvtt::OutputOptions outputOptions;
    outputOptions.setFileName("filtered_envmap.dds");


    // Output header.
    context.outputHeader(nvtt::TextureType_Cube, 64, 64, 1, 4, false, compressionOptions, outputOptions);

    // Output filtered mipmaps.
    for (int m = 0; m < 4; m++) {
        int size = 64 / (1 << m);               // 64, 32, 16, 8
        float cosine_power = float(64) / (1 << (2 * m)); // 64, 16,  4, 1

        printf("filtering step: %d/4.\n", m+1);

        nvtt::CubeSurface filteredEnvmap = envmap.cosinePowerFilter(size, cosine_power);

        filteredEnvmap.toGamma(2.2f);

        context.compress(filteredEnvmap, m, compressionOptions, outputOptions);
    }

    printf("done.\n");

    return EXIT_SUCCESS;
}

