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

#include <stdio.h> // printf
#include <stdlib.h> // rand?
#include <time.h> // clock



int main(int argc, char *argv[])
{
	nvtt::InputOptions inputOptions;
	inputOptions.setTextureLayout(nvtt::TextureType_2D, 1024, 1024);

	int * data = (int *)malloc(1024 * 1024 * 4);
	for (int i = 0; i < 1024 * 1024; i++)
	{
		data[i] = rand();
	}

	inputOptions.setMipmapData(data, 1024, 1024);
	inputOptions.setMipmapGeneration(false);

	nvtt::CompressionOptions compressionOptions;
	nvtt::OutputOptions outputOptions;
	nvtt::Compressor compressor;

	for (int i = 0; i < 1000; i++)
	{
		clock_t start = clock();

		compressor.process(inputOptions, compressionOptions, outputOptions);

		clock_t end = clock();
		printf("time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);
	}

	return 0;
}

