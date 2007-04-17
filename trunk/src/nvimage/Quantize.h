// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_IMAGE_QUANTIZE_H
#define NV_IMAGE_QUANTIZE_H

namespace nv
{
	class Image;

	namespace Quantize
	{
		void RGB16(Image * img);
		void BinaryAlpha(Image * img, int alpha_threshold = 127);
		void Alpha4(Image * img);
		
		void FloydSteinberg_RGB16(Image * img);
		void FloydSteinberg_BinaryAlpha(Image * img, int alpha_threshold = 127);
		void FloydSteinberg_Alpha4(Image * img);

		// @@ Add palette quantization algorithms!
	}
}


#endif // NV_IMAGE_QUANTIZE_H
