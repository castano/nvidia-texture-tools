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

#include <nvimage/Image.h>
#include <nvimage/DirectDrawSurface.h>

#include <nvimage/ImageIO.h>

#include "cmdline.h"

// @@ Add decent error messages.
// @@ Add option to resize images.
// @@ Output DDS header according to surface type.
// @@ Output images.

int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	bool assembleCubeMap = true;
	bool assembleVolume = false;
	bool assembleTextureArray = false;
	
	nv::Array<nv::Path> files;
	
	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		// Input options.
		if (strcmp("-cube", argv[i]) == 0)
		{
			assembleCubeMap = true;
			assembleVolume = false;
			assembleTextureArray = false;
		}
		/*if (strcmp("-volume", argv[i]) == 0)
		{
			assembleCubeMap = false;
			assembleVolume = true;
			assembleTextureArray = false;
		}
		if (strcmp("-array", argv[i]) == 0)
		{
			assembleCubeMap = false;
			assembleVolume = false;
			assembleTextureArray = true;
		}*/
		
		else if (argv[i][0] != '-')
		{
			files.append(argv[i]);
		}
	}
	
	if (files.count() == 0)
	{
		printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");
		printf("usage: nvassemble [-cube|-volume|-array] 'file0' 'file1' ...\n\n");
		return 1;
	}
	
	if (assembleCubeMap && files.count() != 0)
	{
		printf("*** error, 6 files expected, but got %d\n", files.count());
		return 1;
	}
	
	// Load all files.
	nv::Array<nv::Image> images;
	
	uint w, h;
	bool hasAlpha = false;
	
	const uint imageCount = files.count();
	for (uint i = 0; i < imageCount; i++)
	{
		if (!images[i].load(files[i]))
		{
			printf("*** error loading file\n");
			return 1;
		}
		
		if (i == 0)
		{
			w = images[i].width();
			h = images[i].height();
		}
		else if (images[i].width() != w || images[i].height() != h)
		{
			printf("*** error, size of image '%s' does not match\n", files[i].str());
			return 1;
		}
		
		if (images[i].format() == nv::Image::Format_ARGB)
		{
			hasAlpha = true;
		}
	}
	
	
	nv::Path name("output.dds");
	
	nv::StdOutputStream stream(name);
	if (stream.isError()) {
		printf("Error opening '%s' for writting\n", name.str());
		return 1;
	}
	
	// @@ Output DDS header.
	
	// @@ Output images.

	return 0;
}

