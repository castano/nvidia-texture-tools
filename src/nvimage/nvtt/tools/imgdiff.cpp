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
#include <nvcore/StdStream.h>
#include <nvcore/Containers.h>

#include <nvimage/Image.h>
#include <nvimage/DirectDrawSurface.h>
#include <nvimage/nvtt/nvtt.h>

#include <nvmath/Color.h>

#include <math.h>

#include "cmdline.h"


static bool loadImage(nv::Image & image, const char * fileName)
{
	if (nv::strCaseCmp(nv::Path::extension(fileName), ".dds") == 0)
	{
		nv::DirectDrawSurface dds(fileName);
		if (!dds.isValid())
		{
			printf("The file '%s' is not a valid DDS file.\n", fileName);
			return false;
		}
		
		dds.mipmap(&image, 0, 0); // get first image
	}
	else
	{
		// Regular image.
		if (!image.load(fileName))
		{
			printf("The file '%s' is not a supported image type.\n", fileName);
			return false;
		}
	}

	return true;
}

// @@ Compute per-tile errors.
struct Error
{
	Error()
	{
		samples = 0;
		mabse = 0.0f;
		mse = 0.0f;
		maxabse = 0.0f;
	}

	void addSample(float e)
	{
		samples++;
		mabse += fabs(e);
		maxabse = nv::max(maxabse, fabs(e));
		mse += e * e;
	}

	void done()
	{
		mabse /= samples;
		mse /= samples;
		rmse = sqrt(mse);
		psnr = (rmse == 0) ? 999.0f : 20.0f * log10(32767.0 / rmse);
	}

	void print()
	{
		printf("Mean absolute error: %f\n", mabse);
		printf("Max absolute error: %f\n", maxabse);
		printf("Root mean squared error: %f\n", rmse);
		printf("Peak signal to noise ratio in dB: %f\n", psnr);
	}

	int samples;
	float mabse;
	float maxabse;
	float mse;
	float rmse;
	float psnr;
};


int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	bool normal = false;
	bool alpha = false;

	nv::Path input0;
	nv::Path input1;
	nv::Path output;

	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		// Input options.
		if (strcmp("-normal", argv[i]) == 0)
		{
			normal = true;
		}
		if (strcmp("-alpha", argv[i]) == 0)
		{
			alpha = true;
		}

		else if (argv[i][0] != '-')
		{
			input0 = argv[i];

			if (i+1 < argc && argv[i+1][0] != '-') {
				input1 = argv[i+1];
			}

			break;
		}
	}

	if (input0.isNull() || input1.isNull())
	{
		printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");
		
		printf("usage: nvimgdiff [options] inputA inputB [output]\n\n");
		
		printf("Diff options:\n");
		printf("  -normal \tCompare images as if they were normal maps.\n");
		printf("  -alpha  \tCompare alpha channels.\n");

		return 1;
	}

	nv::Image image0, image1;
	if (!loadImage(image0, input0)) return 0;
	if (!loadImage(image1, input1)) return 0;

	const uint w0 = image0.width();
	const uint h0 = image0.height();
	const uint w1 = image1.width();
	const uint h1 = image1.height();
	const uint w = nv::min(w0, w1);
	const uint h = nv::min(h0, h1);

	// Compute errors.
	Error error_r;
	Error error_g;
	Error error_b;
	Error error_a;
	Error error_total;

	for (uint i = 0; i < h; i++)
	{
		for (uint e = 0; e < w; e++)
		{
			const nv::Color32 c0(image0.pixel(e, i));
			const nv::Color32 c1(image1.pixel(e, i));

			float r = c0.r - c1.r;
			float g = c0.g - c1.g;
			float b = c0.b - c1.b;
			float a = c0.a - c1.a;

			error_r.addSample(r);
			error_g.addSample(g);
			error_b.addSample(b);
			error_a.addSample(a);
			
			error_total.addSample(r);
			error_total.addSample(g);
			error_total.addSample(b);
		}
	}

	error_r.done();
	error_g.done();
	error_b.done();
	error_a.done();
	error_total.done();

	printf("Image size compared: %dx%d\n", w, h);
	if (w != w0 || w != w1 || h != h0 || h != h1) {
		printf("--- NOTE: only the overlap between the 2 images (%d,%d) and (%d,%d) was compared\n", w0, h0, w1, h1);
	}
	printf("Total pixels: %d\n", w*h);

	error_total.print();

	// @@ Write image difference.
	
	return 0;
}

