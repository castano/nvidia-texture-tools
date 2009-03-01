// This code is in the public domain -- castano@gmail.com

#include "TiledImage.h"

#include <nvcore/StdStream.h>


using namespace nv;

namespace
{
	// MRU helpers:
	// ...


}


bool Tile::load(const char * name)
{
	StdInputStream stream(name);

	if (stream.isError()) {
		return false;
	}

	uint header;
	stream << header;

	if (header == 'NVTC') {
		return false;
	}

	uint count;
	stream << count;

	if (count != w*h) {
		return false;
	}

	const uint size = count * sizeof(float);

	return stream.serialize(data, size) == size;
}


bool Tile::unload(const char * name)
{
	StdOutputStream stream(name);

	if (stream.isError()) {
		return false;
	}

	uint header = 'NVTC';
	uint count = w * h;
	const uint size = w * h * sizeof(float);

	stream << header << count;

	return stream.serialize(data, size) == size;
}





TiledImage::TiledImage()
{
}

void TiledImage::allocate(uint c, uint w, uint h, uint pageCount)
{
	// Allocate page map:
	const uint pw = ((w + TILE_SIZE - 1) / TILE_SIZE);
	const uint ph = ((h + TILE_SIZE - 1) / TILE_SIZE);
	const uint size = c * pw * ph;
	m_pageMap.resize(size);

	m_residentArray.resize(pageCount, ~0);
}

void TiledImage::prefetch(uint c, uint x, uint y)
{
}

void TiledImage::prefetch(uint c, uint x, uint y, uint w, uint h)
{
}

void TiledImage::loadPage(uint x, uint y)
{
	const uint pw = ((w + TILE_SIZE - 1) / TILE_SIZE);
	const uint ph = ((h + TILE_SIZE - 1) / TILE_SIZE);

	nvDebugCheck(x < pw);
	nvDebugCheck(y < ph);


}


