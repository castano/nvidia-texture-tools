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

#include <nvcore/Ptr.h>
#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>
#include <nvcore/Containers.h>

#include <nvimage/Image.h>
#include <nvimage/ImageIO.h>
#include <nvimage/FloatImage.h>
#include <nvimage/Filter.h>
#include <nvimage/DirectDrawSurface.h>

#include <nvmath/Color.h>
#include <nvmath/Vector.h>

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


int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	float scale = 0.5f;
	nv::Path input;
	nv::Path output;

	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		// Input options.
		if (strcmp("-s", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') {
				scale = atof(argv[i+1]);
				i++;
			}
		}
		else if (argv[i][0] != '-')
		{
			input = argv[i];

			if (i+1 < argc && argv[i+1][0] != '-') {
				output = argv[i+1];
			}

			break;
		}
	}

	if (input.isNull() || output.isNull())
	{
		printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");	
		
		printf("usage: resize [options] input [output]\n\n");
		
		printf("Diff options:\n");
		printf("  -s scale \tScale factor (default = 0.5).\n");

		return 1;
	}

	nv::Image image;
	if (!loadImage(image, input)) return 0;

	nv::FloatImage fimage(&image);
//	fimage.toLinear(0, 3);
	
//	nv::AutoPtr<nv::FloatImage> fresult(fimage.fastDownSample());
	
//	nv::Kernel1 k(10);
//	k.initKaiser(4, scale, 20);
//	nv::AutoPtr<nv::FloatImage> fresult(fimage.downSample(k, image.width() * scale, image.height() * scale, nv::FloatImage::WrapMode_Clamp));
	
	nv::AutoPtr<nv::FloatImage> fresult(fimage.downSample(image.width() * scale, image.height() * scale, nv::FloatImage::WrapMode_Mirror));
	
	
	nv::AutoPtr<nv::Image> result(fresult->createImageGammaCorrect(1.0));

	nv::StdOutputStream stream(output);
	nv::ImageIO::saveTGA(stream, result.ptr());
	
	return 0;
}

