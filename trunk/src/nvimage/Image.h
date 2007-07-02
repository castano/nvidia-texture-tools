// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_IMAGE_H
#define NV_IMAGE_IMAGE_H

#include <nvcore/Debug.h>
#include <nvimage/nvimage.h>

namespace nv
{
	class Color32;
	
	/// 32 bit RGBA image.
	class Image
	{
	public:
		
		enum Format 
		{
			Format_RGB,
			Format_ARGB,
		};
		
		NVIMAGE_API Image();
		NVIMAGE_API ~Image();
		
		NVIMAGE_API void allocate(uint w, uint h);
		NVIMAGE_API bool load(const char * name);
		
		NVIMAGE_API void wrap(void * data, uint w, uint h);
		NVIMAGE_API void unwrap();
		
		NVIMAGE_API uint width() const;
		NVIMAGE_API uint height() const;
		
		NVIMAGE_API const Color32 * scanline(uint h) const;
		NVIMAGE_API Color32 * scanline(uint h);
		
		NVIMAGE_API const Color32 * pixels() const;
		NVIMAGE_API Color32 * pixels();
		
		NVIMAGE_API const Color32 & pixel(uint idx) const;
		NVIMAGE_API Color32 & pixel(uint idx);
		
		const Color32 & pixel(uint x, uint y) const;
		Color32 & pixel(uint x, uint y);
		
		NVIMAGE_API Format format() const;
		NVIMAGE_API void setFormat(Format f);
		
		NVIMAGE_API void fill(Color32 c);

	private:
		void free();
		
	private:
		uint m_width;
		uint m_height;
		Format m_format;
		Color32 * m_data;
	};


	inline const Color32 & Image::pixel(uint x, uint y) const
	{
		nvDebugCheck(x < width() && y < height());
		return pixel(y * width() + x);
	}
	
	inline Color32 & Image::pixel(uint x, uint y)
	{
		nvDebugCheck(x < width() && y < height());
		return pixel(y * width() + x);
	}

} // nv namespace


#endif // NV_IMAGE_IMAGE_H
