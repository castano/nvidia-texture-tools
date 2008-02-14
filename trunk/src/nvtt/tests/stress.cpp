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
#include <stdlib.h> // rand
#include <time.h> // clock
#include <string.h> // memcpy, memcmp
#include <assert.h>

#define FRAME_COUNT  1000

#define WIDTH        1024
#define HEIGHT       1024
#define INPUT_SIZE   (WIDTH*HEIGHT)
#define OUTPUT_SIZE  (WIDTH*HEIGHT/16*2)

static int s_input[INPUT_SIZE];
static int s_reference[OUTPUT_SIZE];
static int s_output[OUTPUT_SIZE];
static int s_frame = 0;

struct MyOutputHandler : public nvtt::OutputHandler
{
	MyOutputHandler() : m_ptr(NULL) {}
	
	virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
	{
		assert(size == OUTPUT_SIZE);
		assert(width == WIDTH);
		assert(height == HEIGHT);
		assert(depth == 1);
		assert(face == 0);
		assert(miplevel == 0);

		m_ptr = (unsigned char *)s_output;

		if (s_frame == 1)
		{
			// Save first result as reference.
			memcpy(s_reference, s_output, sizeof(int) * OUTPUT_SIZE);
		}
		else if (s_frame > 1)
		{
			// Compare against reference.
			if (memcmp(s_output, s_reference, sizeof(int) * OUTPUT_SIZE) != 0)
			{
				printf("Compressed image different to original.\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	
	virtual bool writeData(const void * data, int size)
	{
		memcpy(m_ptr, data, size);
		m_ptr += size;
		return true;
	}

	unsigned char * m_ptr;

};


int main(int argc, char *argv[])
{
	nvtt::InputOptions inputOptions;
	inputOptions.setTextureLayout(nvtt::TextureType_2D, 1024, 1024);

	for (int i = 0; i < 1024 * 1024; i++)
	{
		s_input[i] = rand();
	}

	inputOptions.setMipmapData(s_input, 1024, 1024);
	inputOptions.setMipmapGeneration(false);

	nvtt::CompressionOptions compressionOptions;
	
	nvtt::OutputOptions outputOptions;
	outputOptions.setOutputHeader(false);

	MyOutputHandler outputHandler;
	outputOptions.setOutputHandler(&outputHandler);


	nvtt::Compressor compressor;

	for (s_frame = 0; s_frame < FRAME_COUNT; s_frame++)
	{
		clock_t start = clock();

		printf("compressing frame %d:\n", s_frame);

		compressor.process(inputOptions, compressionOptions, outputOptions);

		clock_t end = clock();
		printf("time taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);
	}

	return EXIT_SUCCESS;
}

