// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_IMAGEIO_H
#define NV_IMAGE_IMAGEIO_H

#include <nvimage/nvimage.h>

#include <nvcore/StrLib.h>


namespace nv
{
	class Image;
	class FloatImage;
	class Stream;

	namespace ImageIO
	{
		struct ImageMetaData
		{
			HashMap<String, String> tagMap;
		};

		NVIMAGE_API Image * load(const char * fileName);
		NVIMAGE_API Image * load(const char * fileName, Stream & s);

		NVIMAGE_API FloatImage * loadFloat(const char * fileName);
		NVIMAGE_API FloatImage * loadFloat(const char * fileName, Stream & s);
		
		NVIMAGE_API bool save(const char * fileName, const Image * img, const ImageMetaData * tags=NULL);
		NVIMAGE_API bool save(const char * fileName, Stream & s, const Image * img, const ImageMetaData * tags=NULL);

		NVIMAGE_API bool saveFloat(const char * fileName, const FloatImage * fimage, uint baseComponent, uint componentCount);
		NVIMAGE_API bool saveFloat(const char * fileName, Stream & s, const FloatImage * fimage, uint baseComponent, uint componentCount);

	} // ImageIO namespace
	
} // nv namespace


#endif // NV_IMAGE_IMAGEIO_H
