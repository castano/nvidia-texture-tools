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
#include <nvimage/nvtt/nvtt.h>

#include "cmdline.h"



int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	bool normal = false;

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

		else if (argv[i][0] != '-')
		{
			input0 = argv[i];

			if (i+1 < argc && argv[i+1][0] != '-') {
				input1 = argv[i+1];
			}

			break;
		}
	}

	if (input0.empty() || input1.empty())
	{
		printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");
		
		printf("usage: nvimgdiff [options] fileA fileB\n\n");
		
		printf("Diff options:\n");
		printf("  -normal \tCompare images as if they were normal maps.\n");

		return 1;
	}

	// @@ Open input0, input1
	// Compute RMS error and other errors.
	// 
	
	return 0;
}

