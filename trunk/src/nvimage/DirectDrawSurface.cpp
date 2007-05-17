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
#include <nvcore/Containers.h> // max
#include <nvcore/StdStream.h>

#include <nvimage/DirectDrawSurface.h>
#include <nvimage/ColorBlock.h>
#include <nvimage/Image.h>
#include <nvimage/BlockDXT.h>

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
	static const uint FOURCC_BC3N = MAKEFOURCC('B', 'C', '3', 'N');

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
	static const uint DDSCAPS2_CUBEMAP_ALL_FACES = 0x0000FC00U;

	static const uint DDPF_RGB = 0x00000040U;
	static const uint DDPF_FOURCC = 0x00000004U;
	static const uint DDPF_ALPHAPIXELS = 0x00000001U;
	static const uint DDPF_NORMAL = 0x80000000U;	// @@ Custom flag.

} // namespace

namespace nv
{
	static Stream & operator<< (Stream & s, DDSPixelFormat & pf)
	{
		s << pf.size;
		s << pf.flags;
		s << pf.fourcc;
		s << pf.bitcount;
		s << pf.rmask;
		s << pf.gmask;
		s << pf.bmask;
		s << pf.amask;
		return s;
	}

	static Stream & operator<< (Stream & s, DDSCaps & caps)
	{
		s << caps.caps1;
		s << caps.caps2;
		s << caps.caps3;
		s << caps.caps4;
		return s;
	}

	static Stream & operator<< (Stream & s, DDSHeader & header)
	{
		nvStaticCheck(sizeof(DDSHeader) == 128);
		s << header.fourcc;
		s << header.size;
		s << header.flags;
		s << header.height;
		s << header.width;
		s << header.pitch;
		s << header.depth;
		s << header.mipmapcount;
		s.serialize(header.reserved, 11 * sizeof(uint));
		s << header.pf;
		s << header.caps;
		s << header.notused;
		return s;
	}
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

void DDSHeader::setNormalFlag(bool b)
{
	if (b) this->pf.flags |= DDPF_NORMAL;
	else this->pf.flags &= ~DDPF_NORMAL;
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



DirectDrawSurface::DirectDrawSurface(const char * name) : stream(new StdInputStream(name))
{
	if (!stream->isError())
	{
		(*stream) << header;
	}
}

DirectDrawSurface::~DirectDrawSurface()
{
	delete stream;
}

bool DirectDrawSurface::isValid() const
{
	if (stream->isError())
	{
		return false;
	}
	
	if (header.fourcc != FOURCC_DDS || header.size != 124)
	{
		return false;
	}
	
	const uint required = (DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS|DDSD_PIXELFORMAT);
	if( (header.flags & required) != required ) {
		return false;
	}
	
	if (header.pf.size != 32) {
		return false;
	}
	
	if( !(header.caps.caps1 & DDSCAPS_TEXTURE) ) {
		return false;
	}
	
	return true;
}

bool DirectDrawSurface::isSupported() const
{
	nvDebugCheck(isValid());
	
	if (header.pf.flags & DDPF_FOURCC)
	{
		if (header.pf.fourcc != FOURCC_DXT1 && 
		    header.pf.fourcc != FOURCC_DXT2 &&
		    header.pf.fourcc != FOURCC_DXT3 &&
		    header.pf.fourcc != FOURCC_DXT4 &&
		    header.pf.fourcc != FOURCC_DXT5 &&
		    header.pf.fourcc != FOURCC_RXGB &&
		    header.pf.fourcc != FOURCC_ATI1 &&
		    header.pf.fourcc != FOURCC_ATI2 &&
			header.pf.fourcc != FOURCC_BC3N)
		{
			// Unknown fourcc code.
			return false;
		}
	}
	else if (header.pf.flags & DDPF_RGB)
	{
		if (header.pf.bitcount == 24)
		{
		}
		else if (header.pf.bitcount == 24)
		{
		}
		else
		{
			// Unsupported pixel format.
			return false;
		}
	}
	else
	{
		return false;
	}
	
	if (isTextureCube() && (header.caps.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES) != DDSCAPS2_CUBEMAP_ALL_FACES)
	{
		// Cubemaps must contain all faces.
		return false;
	}
	
	if (isTexture3D())
	{
		// @@ 3D textures not supported yet.
		return false;
	}
	
	return false;
}


uint DirectDrawSurface::mipmapCount() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_MIPMAPCOUNT) return header.mipmapcount;
	else return 0;
}


uint DirectDrawSurface::width() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_WIDTH) return header.width;
	else return 1;
}

uint DirectDrawSurface::height() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_HEIGHT) return header.height;
	else return 1;
}

uint DirectDrawSurface::depth() const
{
	nvDebugCheck(isValid());
	if (header.flags & DDSD_DEPTH) return header.depth;
	else return 1;
}

bool DirectDrawSurface::isTexture2D() const
{
	nvDebugCheck(isValid());
	return !isTexture3D() && !isTextureCube();
}

bool DirectDrawSurface::isTexture3D() const
{
	nvDebugCheck(isValid());
	return (header.caps.caps2 & DDSCAPS2_VOLUME) != 0;
}

bool DirectDrawSurface::isTextureCube() const
{
	nvDebugCheck(isValid());
	return (header.caps.caps2 & DDSCAPS2_CUBEMAP) != 0;
}

void DirectDrawSurface::mipmap(Image * img, uint face, uint mipmap)
{
	nvDebugCheck(isValid());
	
	stream->seek(offset(face, mipmap));
	
	uint w = width();
	uint h = height();
	
	// Compute width and height.
	for (uint m = 0; m < mipmap; m++)
	{
		w = max(1U, w / 2);
		h = max(1U, h / 2);
	}
	
	img->allocate(w, h);
	
	//	readLinearImage(stream, img);
	//	readBlockImage(stream, img);
}


void DirectDrawSurface::readLinearImage(Stream * stream, Image * img)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(img != NULL);
	
}

void DirectDrawSurface::readBlockImage(Stream * stream, Image * img)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(img != NULL);
	
	const uint bw = (img->width() + 3) / 4;
	const uint bh = (img->height() + 3) / 4;
	
	for (uint by = 0; by < bh; by++)
	{
		for (uint bx = 0; bx < bw; bx++)
		{
			ColorBlock block;
			
			// Read color block.
			readBlock(stream, &block);
			
			// Write color block.
			
		}
	}
}

void DirectDrawSurface::readBlock(Stream * stream, ColorBlock * rgba)
{
	nvDebugCheck(stream != NULL);
	nvDebugCheck(rgba != NULL);
	
	if (header.pf.fourcc == FOURCC_DXT1 ||
		header.pf.fourcc == FOURCC_DXT2 ||
	    header.pf.fourcc == FOURCC_DXT3 ||
	    header.pf.fourcc == FOURCC_DXT4 ||
	    header.pf.fourcc == FOURCC_DXT5 ||
	    header.pf.fourcc == FOURCC_RXGB ||
		header.pf.fourcc != FOURCC_BC3N)
	{
		// Read DXT1 block.
		BlockDXT1 block;
		
		if (header.pf.fourcc == FOURCC_BC3N)
		{
			// Write G only
		}
		else
		{
			// Write RGB.
		}
	}
	
	if (header.pf.fourcc == FOURCC_ATI2)
	{
		// Read DXT5 alpha block.
		// Write R.
	}
	
	if (header.pf.fourcc == FOURCC_DXT2 ||
	    header.pf.fourcc == FOURCC_DXT3)
	{
		// Read DXT3 alpha block.
	}
	
	if (header.pf.fourcc == FOURCC_DXT4 ||
	    header.pf.fourcc == FOURCC_DXT5 ||
		header.pf.fourcc == FOURCC_RXGB)
	{
		// Read DXT5 alpha block.
	}
	
	if (header.pf.fourcc == FOURCC_RXGB)
	{
		// swap G & A
	}
}


uint DirectDrawSurface::blockSize() const
{
	switch(header.pf.fourcc)
	{
		case FOURCC_DXT1:
		case FOURCC_ATI1:
			return 8;
		case FOURCC_DXT2:
		case FOURCC_DXT3:
		case FOURCC_DXT4:
		case FOURCC_DXT5:
		case FOURCC_RXGB:
		case FOURCC_ATI2:
		case FOURCC_BC3N:
			return 16;
	};

	// This should never happen.
	nvDebugCheck(false);
	return 0;
}

uint DirectDrawSurface::mipmapSize(uint mipmap) const
{
	uint w = width();
	uint h = height();
	uint d = depth();
	
	for (uint m = 0; m < mipmap; m++)
	{
		w = max(1U, w / 2);
		h = max(1U, h / 2);
		d = max(1U, d / 2);
	}

	if (header.pf.flags & DDPF_FOURCC)
	{
		// @@ How are 3D textures aligned?
		w = (w + 3) / 4;
		h = (h + 3) / 4;
		return blockSize() * w * h;
	}
	else
	{
		nvDebugCheck(header.pf.flags & DDPF_RGB);
		
		// Align pixels to bytes.
		uint byteCount = (header.pf.bitcount + 7) / 8;
		
		// Align pitch to 4 bytes.
		uint pitch = 4 * ((w * byteCount + 3) / 4);
		
		return pitch * h * d;
	}
}

uint DirectDrawSurface::faceSize() const
{
	const uint count = mipmapCount();
	uint size = 0;
	
	for (uint m = 0; m < count; m++)
	{
		size += mipmapSize(m);
	}
	
	return size;
}

uint DirectDrawSurface::offset(const uint face, const uint mipmap)
{
	uint size = sizeof(DDSHeader);
	
	if (face != 0)
	{
		size += face * faceSize();
	}
	
	for (uint m = 0; m < mipmap; m++)
	{
		size += mipmapSize(m);
	}
	
	return size;
}



