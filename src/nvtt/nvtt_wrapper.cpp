
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

void nvttSetCompressionOptionsPixelFormat(NvttCompressionOptions * compressionOptions, unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask)
{
	compressionOptions->setPixelFormat(bitcount, rmask, gmask, bmask, amask);
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


// Main entrypoint of the compression library.
NvttBoolean nvttCompress(const NvttInputOptions * inputOptions, const NvttCompressionOptions * compressionOptions, const NvttOutputOptions * outputOptions)
{
	return (NvttBoolean)nvtt::compress(*inputOptions, *outputOptions, *compressionOptions);
}

// Estimate the size of compressing the input with the given options.
int nvttEstimateSize(const NvttInputOptions * inputOptions, const NvttCompressionOptions * compressionOptions)
{
	return nvtt::estimateSize(*inputOptions, *compressionOptions);
}

