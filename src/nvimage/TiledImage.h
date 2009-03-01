// This code is in the public domain -- castano@gmail.com

#ifndef NV_IMAGE_TILEDIMAGE_H
#define NV_IMAGE_TILEDIMAGE_H

#include <nvcore/Debug.h>
#include <nvcore/StrLib.h>

#include <nvimage/nvimage.h>

// For simplicity the tile size is fixed at compile time.
#define TILE_SIZE 256

// 256 * 256 * 4 = 2^(8+8+2) = 2^18 = 256 KB
// 512 * 512 * 4 = 2^(9+9+2) = 2^20 = 1 MB


namespace nv
{
#if 0
	struct ImageConcept
	{
		float pixel(uint x, uint y) const;
	};

	enum WrapMode {
		WrapMode_Clamp,
		WrapMode_Repeat,
		WrapMode_Mirror
	};

	template <class T>
	class Sampler
	{
		// ...
	};
#endif


	class Tile
	{
		Tile(uint x, uint y, uint w, uint h) : xoffset(x), yoffset(y), w(w), h(h)
		{
			data = new float[w*h];
		}
		~Tile()
		{
			delete [] data;
		}

		uint size() const
		{
			return w * h * sizeof(float);
		}
		
		float pixel(uint x, uint y) const
		{
			x -= xoffset;
			y -= yoffset;
			
			nvDebugCheck (x < w);
			nvDebugCheck (y < h);
			
			return data[y * w + x];
		}

		bool load(const char * name);
		void unload(const char * name);


		uint xoffset, yoffset;
		uint w, h;
		float * data;
	};


	class TiledImage
	{
	public:
		
		TiledImage();

		void allocate(uint c, uint w, uint h, uint pageCount);

		uint componentCount() const { return m_componentCount; }
		uint width() const { return m_width; }
		uint height() const { return m_height; }
		uint pageCount() const { return m_residentArray.count(); }

		void prefetch(uint c, uint x, uint y);
		void prefetch(uint c, uint x, uint y, uint w, uint h);

		float pixel(uint c, uint x, uint y);

	private:
		Tile * tileAt(uint c, uint x, uint y);
		Tile * tileAt(uint idx);

		uint loadPage(uint x, uint y);
		void unloadPage(Tile *);

		uint addAndReplace(uint newPage);
		
	private:
		uint m_componentCount;
		uint m_width;
		uint m_height;

		struct Page {
			Page() : tile(NULL) {}

			String tmpFileName;
			Tile * tile;
		};

		mutable Array<Page> m_pageMap;
		mutable Array<uint> m_residentArray; // MRU
	};

	inline float TiledImage::pixel(uint c, uint x, uint y)
	{
		nvDebugCheck (c < m_componentCount);
		nvDebugCheck (x < m_width);
		nvDebugCheck (y < m_height);

		uint px = x / TILE_SIZE;
		uint py = y / TILE_SIZE;

		Tile * tile = tileAt(c, px, py);

		if (tile == NULL) {
			tile = loadPage(c, px, py);
		}

		return tile->pixel(x, y);
	}
	
	inline Tile * TiledImage::tileAt(uint c, uint x, uint y)
	{
		uint idx = (c * h  + y) * w + x;
		return tileAt(idx);
	}
	inline Tile * TiledImage::tileAt(uint idx)
	{
		return m_pageMap[idx].tile;
	}
	
} // nv namespace



#endif // NV_IMAGE_TILEDIMAGE_H
