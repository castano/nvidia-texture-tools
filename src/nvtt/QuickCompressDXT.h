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

#ifndef NV_TT_QUICKCOMPRESSDXT_H
#define NV_TT_QUICKCOMPRESSDXT_H

#include <nvimage/nvimage.h>

namespace nv
{
	struct ColorBlock;
	struct BlockDXT1;
	struct BlockDXT3;
	struct BlockDXT5;
	struct AlphaBlockDXT3;
	struct AlphaBlockDXT5;
    class Vector3;

	namespace QuickCompress
	{
		void compressDXT1(const ColorBlock & rgba, BlockDXT1 * dxtBlock);
		void compressDXT1a(const ColorBlock & rgba, BlockDXT1 * dxtBlock);
		
		void compressDXT3(const ColorBlock & rgba, BlockDXT3 * dxtBlock);
		
		void compressDXT5A(const ColorBlock & rgba, AlphaBlockDXT5 * dxtBlock, int iterationCount=8);
		void compressDXT5(const ColorBlock & rgba, BlockDXT5 * dxtBlock, int iterationCount=8);

        void outputBlock4(const ColorBlock & rgba, const Vector3 & start, const Vector3 & end, BlockDXT1 * block);
        void outputBlock3(const ColorBlock & rgba, const Vector3 & start, const Vector3 & end, BlockDXT1 * block);
	}
} // nv namespace

#endif // NV_TT_QUICKCOMPRESSDXT_H
