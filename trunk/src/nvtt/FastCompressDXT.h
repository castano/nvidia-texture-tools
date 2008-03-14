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

#ifndef NV_TT_FASTCOMPRESSDXT_H
#define NV_TT_FASTCOMPRESSDXT_H

#include <nvimage/nvimage.h>

namespace nv
{
	struct ColorBlock;
	struct BlockDXT1;
	struct BlockDXT3;
	struct BlockDXT5;
	struct AlphaBlockDXT3;
	struct AlphaBlockDXT5;

	// Color compression:

	// Compressor that uses the extremes of the luminance axis.
	void compressBlock_DiameterAxis(const ColorBlock & rgba, BlockDXT1 * block);

	// Compressor that uses the extremes of the luminance axis.
	void compressBlock_LuminanceAxis(const ColorBlock & rgba, BlockDXT1 * block);

	// Compressor that uses bounding box.
	void compressBlock_BoundsRange(const ColorBlock & rgba, BlockDXT1 * block);

	// Compressor that uses bounding box and takes alpha into account.
	void compressBlock_BoundsRangeAlpha(const ColorBlock & rgba, BlockDXT1 * block);


	// Simple, but slow compressor that tests all color pairs.
	void compressBlock_TestAllPairs(const ColorBlock & rgba, BlockDXT1 * block);
	
	// Brute force 6d search along the best fit axis.
	void compressBlock_AnalyzeBestFitAxis(const ColorBlock & rgba, BlockDXT1 * block);

	// Spatial greedy search.
	void refineSolution_1dSearch(const ColorBlock & rgba, BlockDXT1 * block);
	void refineSolution_3dSearch(const ColorBlock & rgba, BlockDXT1 * block);
	void refineSolution_6dSearch(const ColorBlock & rgba, BlockDXT1 * block);
	
	// Brute force compressor for DXT5n
	void compressGreenBlock_BruteForce(const ColorBlock & rgba, BlockDXT1 * block);

	// Minimize error of the endpoints.
	void optimizeEndPoints(const ColorBlock & rgba, BlockDXT1 * block);
	
	uint blockError(const ColorBlock & rgba, const BlockDXT1 & block);
	uint blockError(const ColorBlock & rgba, const AlphaBlockDXT5 & block);

	// Alpha compression:
	void compressBlock(const ColorBlock & rgba, AlphaBlockDXT3 * block);
	void compressBlock_BoundsRange(const ColorBlock & rgba, BlockDXT3 * block);
	void compressBlock_BoundsRange(const ColorBlock & rgba, BlockDXT5 * block);

	uint compressBlock_BoundsRange(const ColorBlock & rgba, AlphaBlockDXT5 * block);
	uint compressBlock_BruteForce(const ColorBlock & rgba, AlphaBlockDXT5 * block);
	uint compressBlock_Iterative(const ColorBlock & rgba, AlphaBlockDXT5 * block);

} // nv namespace

#endif // NV_TT_FASTCOMPRESSDXT_H
