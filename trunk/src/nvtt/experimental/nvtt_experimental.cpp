
#include "nvtt_experimental.h"

struct NvttImage
{
	NvttImage() :
		m_constant(false),
		m_image(NULL),
		m_floatImage(NULL)
	{
	}
	
	~NvttImage()
	{
		if (m_constant && m_image) m_image->unwrap();
		delete m_image;
		delete m_floatImage;
	}
	
	bool m_constant;
	Image * m_image;
	FloatImage * m_floatImage;
};

NvttImage * nvttCreateImage() 
{
	return new NvttImage();
}
	
void nvttDestroyImage(NvttImage * img)
{
	delete img;
}

void nvttSetImageData(NvttImage * img, NvttInputFormat format, uint w, uint h, void * data)
{
	nvCheck(img != NULL);
	
	if (format == NVTT_InputFormat_BGRA_8UB)
	{
		img->m_constant = false;
		img->m_image->allocate(w, h);
		memcpy(img->m_image->pixels(), data, w * h * 4);
	}
	else
	{
		nvCheck(false);
	}
}

void nvttCompressImage(NvttImage * img, NvttFormat format)
{
	nvCheck(img != NULL);

	// @@ Invoke appropriate compressor.
}



#endif // NVTT_EXPERIMENTAL_H
