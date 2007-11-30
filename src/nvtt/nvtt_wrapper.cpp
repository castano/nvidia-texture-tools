


// Input Options
NvttInputOptions nvttCreateInputOptions()
{
	return (NvttInputOptions *) new nvtt::InputOptions();
}

void nvttDestroyInputOptions(NvttInputOptions inputOptions)
{
	delete (nvtt::InputOptions *) inputOptions;
}

void nvttSetInputOptionsTextureLayout(NvttInputOptions inputOptions, NvttTextureType type, int w, int h, int d)
{
	((nvtt::InputOptions *)inputOptions)->setTextureLayout(type, w, h, d);
}

void nvttResetInputOptionsTextureLayout(NvttInputOptions inputOptions)
{
	((nvtt::InputOptions *)inputOptions)->resetTextureLayout();
}

NvttBoolean nvttSetInputOptionsMipmapData(NvttInputOptions inputOptions, const void * data, int w, int h, int d, int face, int mipmap)
{
	return ((nvtt::InputOptions *)inputOptions)->setMipmapData(data, w, h, d, face, mipmap);
}


// Compression Options
NvttCompressionOptions nvttCreateCompressionOptions()
{
	return (NvttCompressionOptions *) new nvtt::CompressionOptions();
}

void nvttDestroyCompressionOptions(NvttCompressionOptions compressionOptions)
{
	delete (nvtt::CompressionOptions *) compressionOptions;
}

void nvttSetCompressionOptionsFormat(NvttCompressionOptions compressionOptions, NvttFormat format)
{
	((nvtt::CompressionOptions *)compressionOptions)->setFormat(format);
}

void nvttSetCompressionOptionsQuality(NvttCompressionOptions compressionOptionso, NvttQuality quality)
{
	((nvtt::CompressionOptions *)compressionOptions)->setQuality(quality);
}

void nvttSetCompressionOptionsPixelFormat(unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask)
{
	((nvtt::CompressionOptions *)compressionOptions)->setPixelFormat(bitcount, rmask, gmask, bmask, amask);
}
	

// Output Options
NvttOutputOptions nvttCreateOutputOptions()
{
	return (NvttOutputOptions *) new nvtt::OutputOptions();
}

void nvttDestroyOutputOptions(NvttOutputOptions outputOptions)
{
	delete (nvtt::OutputOptions *) outputOptions;
}

void nvttSetOutputOptionsFileName(NvttOutputOptions outputOptions, const char * fileName)
{
	((nvtt::OutputOptions *)outputOptions)->setFileName(fileName);
}


// Main entrypoint of the compression library.
NvttBoolean nvttCompress(NvttInputOptions inputOptions, NvttOutputOptions outputOptions, NvttCompressionOptions compressionOptions)
{
	return nvtt::compress((nvtt::InputOptions *)inputOptions, (nvtt::OutputOptions *)outputOptions, (nvtt::CompressionOptions *)compressionOptions);
}

// Estimate the size of compressing the input with the given options.
int nvttEstimateSize(NvttInputOptions inputOptions, NvttCompressionOptions compressionOptions)
{
	return nvtt::estimateSize((nvtt::InputOptions *)inputOptions, (nvtt::CompressionOptions *)compressionOptions);
}

