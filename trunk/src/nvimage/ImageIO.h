// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_IMAGEIO_H
#define NV_IMAGE_IMAGEIO_H

#include <nvimage/nvimage.h>

namespace nv
{
	class Image;
	class FloatImage;
	class Stream;

	namespace ImageIO
	{
		NVIMAGE_API Image * load(const char * name);
		NVIMAGE_API Image * load(const char * name, Stream & s);
		
		NVIMAGE_API Image * loadTGA(Stream & s);
		NVIMAGE_API bool saveTGA(Stream & s, const Image * img);

		NVIMAGE_API Image * loadPSD(Stream & s);

#if defined(HAVE_PNG)
		NVIMAGE_API Image * loadPNG(Stream & s);
		NVIMAGE_API FloatImage * loadFloatPNG(Stream & s);
#endif

#if defined(HAVE_JPEG)
		NVIMAGE_API Image * loadJPG(Stream & s);
#endif
		
#if defined(HAVE_TIFF)
		// Hacks!
		NVIMAGE_API FloatImage * loadFloatTIFF(const char * fileName);
		NVIMAGE_API bool saveFloatTIFF(const char * fileName, FloatImage *fimage);

		NVIMAGE_API FloatImage * loadFloatTIFF(Stream & s);
#endif

	} // ImageIO namespace
	
} // nv namespace


#endif // NV_IMAGE_IMAGEIO_H
