
#include "nvtt.h"
#include "nvtt_wrapper.h"


// Input Options
NvttInputOptions * nvttCreateInputOptions()
{
	return new nvtt::InputOptions();
}

void nvttDestroyInputOptions(NvttInputOptions * inputOptions)
{
	delete inputOptions;
}

void nvttSetInputOptionsTextureLayout(NvttInputOptions * inputOptions, NvttTextureType type, int w, int h, int d)
{
	inputOptions->setTextureLayout((nvtt::TextureType)type, w, h, d);
}

void nvttResetInputOptionsTextureLayout(NvttInputOptions * inputOptions)
{
	inputOptions->resetTextureLayout();
}

NvttBoolean nvttSetInputOptionsMipmapData(NvttInputOptions * inputOptions, const void * data, int w, int h, int d, int face, int mipmap)
{
	return (NvttBoolean)inputOptions->setMipmapData(data, w, h, d, face, mipmap);
}

void nvttSetInputOptionsFormat(NvttInputOptions * inputOptions, NvttInputFormat format)
{
	inputOptions->setFormat((nvtt::InputFormat)format);
}

void nvttSetInputOptionsAlphaMode(NvttInputOptions * inputOptions, NvttAlphaMode alphaMode)
{
	inputOptions->setAlphaMode((nvtt::AlphaMode)alphaMode);
}

void nvttSetInputOptionsGamma(NvttInputOptions * inputOptions, float inputGamma, float outputGamma)
{
	inputOptions->setGamma(inputGamma, outputGamma);
}


void nvttSetInputOptionsWrapMode(NvttInputOptions * inputOptions, NvttWrapMode mode)
{
	inputOptions->setWrapMode((nvtt::WrapMode)mode);
}

void nvttSetInputOptionsMipmapping(NvttInputOptions * inputOptions, NvttBoolean generateMipmaps, NvttMipmapFilter filter, int maxLevel)
{
	inputOptions->setMipmapping(generateMipmaps != NVTT_False, (nvtt::MipmapFilter)filter, maxLevel);
}

void nvttSetInputOptionsKaiserParameters(NvttInputOptions * inputOptions, float width, float alpha, float stretch)
{
	inputOptions->setKaiserParameters(width, alpha, stretch);
}

void nvttSetInputOptionsNormalMap(NvttInputOptions * inputOptions, NvttBoolean b)
{
	inputOptions->setNormalMap(b != NVTT_False);
}

void nvttSetInputOptionsConvertToNormalMap(NvttInputOptions * inputOptions, NvttBoolean convert)
{
	inputOptions->setConvertToNormalMap(convert != NVTT_False);
}

void nvttSetInputOptionsHeightEvaluation(NvttInputOptions * inputOptions, float redScale, float greenScale, float blueScale, float alphaScale)
{
	inputOptions->setHeightEvaluation(redScale, greenScale, blueScale, alphaScale);
}

void nvttSetInputOptionsNormalFilter(NvttInputOptions * inputOptions, float small, float medium, float big, float large)
{
	inputOptions->setNormalFilter(small, medium, big, large);
}

void nvttSetInputOptionsNormalizeMipmaps(NvttInputOptions * inputOptions, NvttBoolean b)
{
	inputOptions->setNormalizeMipmaps(b != NVTT_False);
}

void nvttSetInputOptionsColorTransform(NvttInputOptions * inputOptions, NvttColorTransform t)
{
	inputOptions->setColorTransform((nvtt::ColorTransform)t);
}

void nvttSetInputOptionsLinearTransfrom(NvttInputOptions * inputOptions, int channel, float w0, float w1, float w2, float w3)
{
	inputOptions->setLinearTransform(channel, w0, w1, w2, w3);
}

void nvttSetInputOptionsMaxExtents(NvttInputOptions * inputOptions, int dim)
{
	inputOptions->setMaxExtents(dim);
}

void nvttSetInputOptionsRoundMode(NvttInputOptions * inputOptions, NvttRoundMode mode)
{
	inputOptions->setRoundMode((nvtt::RoundMode)mode);
}


// Compression Options
NvttCompressionOptions * nvttCreateCompressionOptions()
{
	return new nvtt::CompressionOptions();
}

void nvttDestroyCompressionOptions(NvttCompressionOptions * compressionOptions)
{
	delete compressionOptions;
}

void nvttSetCompressionOptionsFormat(NvttCompressionOptions * compressionOptions, NvttFormat format)
{
	compressionOptions->setFormat((nvtt::Format)format);
}

void nvttSetCompressionOptionsQuality(NvttCompressionOptions * compressionOptions, NvttQuality quality)
{
	compressionOptions->setQuality((nvtt::Quality)quality);
}

void nvttSetCompressionOptionsColorWeights(NvttCompressionOptions * compressionOptions, float red, float green, float blue, float alpha)
{
	compressionOptions->setColorWeights(red, green, blue, alpha);
}

void nvttEnableCompressionOptionsCudaCompression(NvttCompressionOptions * compressionOptions, NvttBoolean enable)
{
	compressionOptions->enableCudaCompression(enable != NVTT_False);
}

void nvttSetCompressionOptionsPixelFormat(NvttCompressionOptions * compressionOptions, unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask)
{
	compressionOptions->setPixelFormat(bitcount, rmask, gmask, bmask, amask);
}

void nvttSetCompressionOptionsQuantization(NvttCompressionOptions * compressionOptions, NvttBoolean colorDithering, NvttBoolean alphaDithering, NvttBoolean binaryAlpha, int alphaThreshold)
{
	compressionOptions->setQuantization(colorDithering != NVTT_False, alphaDithering != NVTT_False, binaryAlpha != NVTT_False, alphaThreshold);
}


// Output Options
NvttOutputOptions * nvttCreateOutputOptions()
{
	return new nvtt::OutputOptions();
}

void nvttDestroyOutputOptions(NvttOutputOptions * outputOptions)
{
	delete outputOptions;
}

void nvttSetOutputOptionsFileName(NvttOutputOptions * outputOptions, const char * fileName)
{
	outputOptions->setFileName(fileName);
}

void nvttSetOutputOptionsOutputHeader(NvttOutputOptions * outputOptions, NvttBoolean b)
{
	outputOptions->setOutputHeader(b != NVTT_False);
}
/*
void nvttSetOutputOptionsErrorHandler(NvttOutputOptions * outputOptions, nvttErrorHandler errorHandler)
{
	outputOptions->setErrorHandler(errorHandler);
}

void nvttSetOutputOptionsOutputHandler(NvttOutputOptions * outputOptions, nvttOutputHandler outputHandler, nvttImageHandler imageHandler)
{
}
*/


// Main entrypoints.
NvttBoolean nvttCompress(const NvttInputOptions * inputOptions, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions)
{
	return (NvttBoolean)nvtt::compress(*inputOptions, *outputOptions, *compressionOptions);
}

int nvttEstimateSize(const NvttInputOptions * inputOptions, const NvttCompressionOptions * compressionOptions)
{
	return nvtt::estimateSize(*inputOptions, *compressionOptions);
}

const char * nvttErrorString(NvttError e)
{
	return nvtt::errorString((nvtt::Error)e);
}
