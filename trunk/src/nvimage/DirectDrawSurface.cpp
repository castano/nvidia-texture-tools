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

#include <nvcore/Debug.h>

#include <nvimage/DirectDrawSurface.h>

#include <string.h> // memset


using namespace nv;

#if !defined(MAKEFOURCC)
#	define MAKEFOURCC(ch0, ch1, ch2, ch3) \
		(uint(uint8(ch0)) | (uint(uint8(ch1)) << 8) | \
		(uint(uint8(ch2)) << 16) | (uint(uint8(ch3)) << 24 ))
#endif

namespace
{
	static const uint FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
	static const uint FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
	static const uint FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
	static const uint FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
	static const uint FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
	static const uint FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
	static const uint FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
	static const uint FOURCC_ATI1 = MAKEFOURCC('A', 'T', 'I', '1');
	static const uint FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');

	static const uint DDSD_CAPS = 0x00000001U;
	static const uint DDSD_PIXELFORMAT = 0x00001000U;
	static const uint DDSD_WIDTH = 0x00000004U;
	static const uint DDSD_HEIGHT = 0x00000002U;
	static const uint DDSD_PITCH = 0x00000008U;
	static const uint DDSD_MIPMAPCOUNT = 0x00020000U;
	static const uint DDSD_LINEARSIZE = 0x00080000U;
	static const uint DDSD_DEPTH = 0x00800000U;
		
	static const uint DDSCAPS_COMPLEX = 0x00000008U;
	static const uint DDSCAPS_TEXTURE = 0x00001000U;
	static const uint DDSCAPS_MIPMAP = 0x00400000U;
	static const uint DDSCAPS2_VOLUME = 0x00200000U;
	static const uint DDSCAPS2_CUBEMAP = 0x00000200U;

	static const uint DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U;
	static const uint DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U;
	static const uint DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U;
	static const uint DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U;
	static const uint DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000F000U;

	static const uint DDPF_RGB = 0x00000040U;
	static const uint DDPF_FOURCC = 0x00000004U;
	static const uint DDPF_ALPHAPIXELS = 0x00000001U;
}

DDSHeader::DDSHeader()
{
	this->fourcc = FOURCC_DDS;
	this->size = 124;
	this->flags  = (DDSD_CAPS|DDSD_PIXELFORMAT);
	this->height = 0;
	this->width = 0;
	this->pitch = 0;
	this->depth = 0;
	this->mipmapcount = 0;
	memset(this->reserved, 0, sizeof(this->reserved));

	// Store version information on the reserved header attributes.
	this->reserved[9] = MAKEFOURCC('N', 'V', 'T', 'T');
	this->reserved[10] = (0 << 16) | (1 << 8) | (0);	// major.minor.revision

	this->pf.size = 32;
	this->pf.flags = 0;
	this->pf.fourcc = 0;
	this->pf.bitcount = 0;
	this->pf.rmask = 0;
	this->pf.gmask = 0;
	this->pf.bmask = 0;
	this->pf.amask = 0;
	this->caps.caps1 = DDSCAPS_TEXTURE;
	this->caps.caps2 = 0;
	this->caps.caps3 = 0;
	this->caps.caps4 = 0;
	this->notused = 0;
}

void DDSHeader::setWidth(uint w)
{
	this->flags |= DDSD_WIDTH;
	this->width = w;
}

void DDSHeader::setHeight(uint h)
{
	this->flags |= DDSD_HEIGHT;
	this->height = h;
}

void DDSHeader::setDepth(uint d)
{
	this->flags |= DDSD_DEPTH;
	this->height = d;
}

void DDSHeader::setMipmapCount(uint count)
{
	if (count == 0)
	{
		this->flags &= ~DDSD_MIPMAPCOUNT;
		this->mipmapcount = 0;

		if (this->caps.caps2 == 0) {
			this->caps.caps1 = DDSCAPS_TEXTURE;
		}
		else {
			this->caps.caps1 = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX;
		}
	}
	else
	{
		this->flags |= DDSD_MIPMAPCOUNT;
		this->mipmapcount = count;

		this->caps.caps1 |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	}
}

void DDSHeader::setTexture2D()
{
	// nothing to do here.
}

void DDSHeader::setTexture3D()
{
	this->caps.caps2 = DDSCAPS2_VOLUME;
}

void DDSHeader::setTextureCube()
{
	this->caps.caps1 |= DDSCAPS_COMPLEX;
	this->caps.caps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES;
}

void DDSHeader::setLinearSize(uint size)
{
	this->flags &= ~DDSD_PITCH;
	this->flags |= DDSD_LINEARSIZE;
	this->pitch = size;
}

void DDSHeader::setPitch(uint pitch)
{
	this->flags &= ~DDSD_LINEARSIZE;
	this->flags |= DDSD_PITCH;
	this->pitch = pitch;
}

void DDSHeader::setFourCC(uint8 c0, uint8 c1, uint8 c2, uint8 c3)
{
	// set fourcc pixel format.
	this->pf.flags = DDPF_FOURCC;
	this->pf.fourcc = MAKEFOURCC(c0, c1, c2, c3);
	this->pf.bitcount = 0;
	this->pf.rmask = 0;
	this->pf.gmask = 0;
	this->pf.bmask = 0;
	this->pf.amask = 0;
}

void DDSHeader::setPixelFormat(uint bitcount, uint rmask, uint gmask, uint bmask, uint amask)
{
	// Make sure the masks are correct.
	nvCheck((rmask & gmask) == 0);
	nvCheck((rmask & bmask) == 0);
	nvCheck((rmask & amask) == 0);
	nvCheck((gmask & bmask) == 0);
	nvCheck((gmask & amask) == 0);
	nvCheck((bmask & amask) == 0);

	this->pf.flags = DDPF_RGB;

	if (amask != 0) {
		this->pf.flags |= DDPF_ALPHAPIXELS;
	}

	if (bitcount == 0)
	{
		// Compute bit count from the masks.
		uint total = rmask | gmask | bmask | amask;
		while(total != 0) {
			bitcount++;
			total >>= 1;
		}
		// @@ Align to 8?
	}

	this->pf.fourcc = 0;
	this->pf.bitcount = bitcount;
	this->pf.rmask = rmask;
	this->pf.gmask = gmask;
	this->pf.bmask = bmask;
	this->pf.amask = amask;
}


void DDSHeader::swapBytes()
{
	this->fourcc = POSH_LittleU32(this->fourcc);
	this->size = POSH_LittleU32(this->size);
	this->flags = POSH_LittleU32(this->flags);
	this->height = POSH_LittleU32(this->height);
	this->width = POSH_LittleU32(this->width);
	this->pitch = POSH_LittleU32(this->pitch);
	this->depth = POSH_LittleU32(this->depth);
	this->mipmapcount = POSH_LittleU32(this->mipmapcount);
	
	for(int i = 0; i < 11; i++) {
		this->reserved[i] = POSH_LittleU32(this->reserved[i]);
	}

	this->pf.size = POSH_LittleU32(this->pf.size);
	this->pf.flags = POSH_LittleU32(this->pf.flags);
	this->pf.fourcc = POSH_LittleU32(this->pf.fourcc);
	this->pf.bitcount = POSH_LittleU32(this->pf.bitcount);
	this->pf.rmask = POSH_LittleU32(this->pf.rmask);
	this->pf.gmask = POSH_LittleU32(this->pf.gmask);
	this->pf.bmask = POSH_LittleU32(this->pf.bmask);
	this->pf.amask = POSH_LittleU32(this->pf.amask);
	this->caps.caps1 = POSH_LittleU32(this->caps.caps1);
	this->caps.caps2 = POSH_LittleU32(this->caps.caps2);
	this->caps.caps3 = POSH_LittleU32(this->caps.caps3);
	this->caps.caps4 = POSH_LittleU32(this->caps.caps4);
	this->notused = POSH_LittleU32(this->notused);
}

