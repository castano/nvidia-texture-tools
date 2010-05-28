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

#include <nvtt/nvtt.h>

#include <stdlib.h> // EXIT_SUCCESS, EXIT_FAILURE


int main(int argc, char *argv[])
{
    if (argc != 2) return EXIT_FAILURE;

	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setFormat(nvtt::Format_BC1);

	nvtt::OutputOptions outputOptions;
	outputOptions.setFileName("output.dds");

	nvtt::Context context;
	nvtt::TexImage image = context.createTexImage();

	image.load(argv[1]);

	context.outputHeader(image, image.countMipmaps(), compressionOptions, outputOptions);

	float gamma = 2.2f;
	image.toLinear(gamma);

    float coverage = image.alphaTestCoverage();

	while (image.buildNextMipmap(nvtt::MipmapFilter_Box))
	{
		nvtt::TexImage tmpImage = image;
		tmpImage.toGamma(gamma);

        tmpImage.scaleAlphaToCoverage(coverage);

		context.compress(tmpImage, compressionOptions, outputOptions);
	}

	return EXIT_SUCCESS;
}

