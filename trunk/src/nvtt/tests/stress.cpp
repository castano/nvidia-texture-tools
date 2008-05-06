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

#define WIDTH        2048
#define HEIGHT       2048
#define INPUT_SIZE   (WIDTH*HEIGHT)
#define OUTPUT_SIZE  (WIDTH*HEIGHT/16*4)

static int s_input[INPUT_SIZE];
static int s_reference[OUTPUT_SIZE];
static int s_output[OUTPUT_SIZE];
static int s_frame = 0;

struct MyOutputHandler : public nvtt::OutputHandler
{
	MyOutputHandler() : m_ptr(NULL) {}
	
	virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
	{
		assert(size == sizeof(int) * OUTPUT_SIZE);
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

void precomp()
{
	unsigned int bitmaps[1024];

	int num = 0;

	printf("const static uint s_bitmapTableCTX[704] =\n{\n");

	for (int a = 1; a <= 15; a++)
	{
		  for (int b = a; b <= 15; b++)
		  {
				for (int c = b; c <= 15; c++)
				{
					int indices[16];

					int i = 0;
					for(; i < a; i++) {
						indices[i] = 0;
					}
					for(; i < a+b; i++) {
						indices[i] = 2;
					}
					for(; i < a+b+c; i++) {
						indices[i] = 3;
					}
					for(; i < 16; i++) {
						indices[i] = 1;
					}

					unsigned int bm = 0;
					for(i = 0; i < 16; i++) {
						bm |= indices[i] << (i * 2);
					}

					printf("\t0x%8X, // %d %d %d %d\n", bm, a-0, b-a, c-b, 16-c);

					bitmaps[num] = bm;
					num++;
				}
		  }
	}

	// Align to 32: 680 -> 704
	while (num < 704)
	{
		printf("\t0x80000000,\n");

		bitmaps[num] = 0x80000000; // 15 0 0 1;
		num++;
	}

	printf("}; // num = %d\n", num);

/*
	for( int i = imax; i >= 0; --i )
	{
		// second cluster [i,j) is one third along
		for( int m = i; m < 16; ++m )
		{
			indices[m] = 2;
		}
		const int jmax = ( i == 0 ) ? 15 : 16;
		for( int j = jmax; j >= i; --j )
		{
			// third cluster [j,k) is two thirds along
			for( int m = j; m < 16; ++m )
			{
				indices[m] = 3;
			}
			
			int kmax = ( j == 0 ) ? 15 : 16;
			for( int k = kmax; k >= j; --k )
			{
				// last cluster [k,n) is at the end
				if( k < 16 )
				{
					indices[k] = 1;
				}
				
				uint bitmap = 0;
				
				bool hasThree = false;
				for(int p = 0; p < 16; p++) {
					bitmap |= indices[p] << (p * 2);
				}
				
				bitmaps[num] = bitmap;
				num++;
			}
		}
	}
*/
}

int main(int argc, char *argv[])
{
//	precomp();

	nvtt::InputOptions inputOptions;
	inputOptions.setTextureLayout(nvtt::TextureType_2D, WIDTH, HEIGHT);

	for (int i = 0; i < INPUT_SIZE; i++)
	{
		s_input[i] = rand();
	}

	inputOptions.setMipmapData(s_input, WIDTH, HEIGHT);
	inputOptions.setMipmapGeneration(false);

	nvtt::CompressionOptions compressionOptions;
//	compressionOptions.setFormat(nvtt::Format_DXT3);
//	compressionOptions.setFormat(nvtt::Format_DXT1n);
//	compressionOptions.setFormat(nvtt::Format_CTX1);
	
	nvtt::OutputOptions outputOptions;
	outputOptions.setOutputHeader(false);

	MyOutputHandler outputHandler;
	outputOptions.setOutputHandler(&outputHandler);


	nvtt::Compressor compressor;
//	compressor.enableCudaAcceleration(false);

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

