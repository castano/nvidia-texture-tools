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

#ifndef NV_IMAGE_DIRECTDRAWSURFACE_H
#define NV_IMAGE_DIRECTDRAWSURFACE_H

#include <nvcore/nvcore.h>

namespace nv
{

	struct DDSPixelFormat {
		uint size;
		uint flags;
		uint fourcc;
		uint bitcount;
		uint rmask;
		uint gmask;
		uint bmask;
		uint amask;
	};

	struct DDSCaps {
		uint caps1;
		uint caps2;
		uint caps3;
		uint caps4;
	};

	/// DDS file header.
	struct DDSHeader {
		uint fourcc;
		uint size;
		uint flags;
		uint height;
		uint width;
		uint pitch;
		uint depth;
		uint mipmapcount;
		uint reserved[11];
		DDSPixelFormat pf;
		DDSCaps caps;
		uint notused;

		// Helper methods.
		DDSHeader();
		void setWidth(uint w);
		void setHeight(uint h);
		void setDepth(uint d);
		void setMipmapCount(uint count);
		void setLinearSize(uint size);
		void setPitch(uint pitch);
		void setFourCC(uint8 c0, uint8 c1, uint8 c2, uint8 c3);
		void setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask);
		void setTexture2D();
		void setTexture3D();
		void setTextureCube();
		
		void swapBytes();
	};


} // nv namespace

#endif // NV_IMAGE_DIRECTDRAWSURFACE_H
