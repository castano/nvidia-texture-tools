// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef NVTT_WRAPPER_H
#define NVTT_WRAPPER_H

#include <nvcore/nvcore.h>

// Function linkage
#if NVTT_SHARED
#ifdef NVTT_EXPORTS
#define NVTT_API DLL_EXPORT
#define NVTT_CLASS DLL_EXPORT_CLASS
#else
#define NVTT_API DLL_IMPORT
#define NVTT_CLASS DLL_IMPORT
#endif
#else
#define NVTT_API
#define NVTT_CLASS
#endif

#ifdef __cplusplus
typedef struct nvtt::InputOptions NvttInputOptions;
typedef struct nvtt::CompressionOptions NvttCompressionOptions;
typedef struct nvtt::OutputOptions NvttOutputOptions;
#else
typedef struct NvttInputOptions NvttInputOptions;
typedef struct NvttCompressionOptions NvttCompressionOptions;
typedef struct NvttOutputOptions NvttOutputOptions;
#endif

/// Supported compression formats.
typedef enum
{
	// No compression.
	NVTT_Format_RGB,
	NVTT_Format_RGBA = NVTT_Format_RGB,

	// DX9 formats.
	NVTT_Format_DXT1,
	NVTT_Format_DXT1a,
	NVTT_Format_DXT3,
	NVTT_Format_DXT5,
	NVTT_Format_DXT5n,
	
	// DX10 formats.
	NVTT_Format_BC1 = NVTT_Format_DXT1,
	NVTT_Format_BC1a = NVTT_Format_DXT1a,
	NVTT_Format_BC2 = NVTT_Format_DXT3,
	NVTT_Format_BC3 = NVTT_Format_DXT5,
	NVTT_Format_BC3n = NVTT_Format_DXT5n,
	NVTT_Format_BC4,
	NVTT_Format_BC5,
} NvttFormat;

/// Quality modes.
typedef enum
{
	NVTT_Quality_Fastest,
	NVTT_Quality_Normal,
	NVTT_Quality_Production,
	NVTT_Quality_Highest,
} NvttQuality;

/// Texture types.
typedef enum
{
	NVTT_TextureType_2D,
	NVTT_TextureType_Cube,
} NvttTextureType;

typedef enum
{
	NVTT_True,
	NVTT_False,
} NvttBoolean;


#ifdef __cplusplus
extern "C" {
#endif

// Input Options
NVTT_API NvttInputOptions * nvttCreateInputOptions();
NVTT_API void nvttDestroyInputOptions(NvttInputOptions * inputOptions);

NVTT_API void nvttSetInputOptionsTextureLayout(NvttInputOptions * inputOptions, NvttTextureType type, int w, int h, int d);
NVTT_API void nvttResetInputOptionsTextureLayout(NvttInputOptions * inputOptions);
NVTT_API NvttBoolean nvttSetInputOptionsMipmapData(NvttInputOptions * inputOptions, const void * data, int w, int h, int d, int face, int mipmap);


// Compression Options
NVTT_API NvttCompressionOptions * nvttCreateCompressionOptions();
NVTT_API void nvttDestroyCompressionOptions(NvttCompressionOptions * compressionOptions);

NVTT_API void nvttSetCompressionOptionsFormat(NvttCompressionOptions * compressionOptions, NvttFormat format);
NVTT_API void nvttSetCompressionOptionsQuality(NvttCompressionOptions * compressionOptions, NvttQuality quality);
NVTT_API void nvttSetCompressionOptionsPixelFormat(NvttCompressionOptions * compressionOptions, unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask);
	

// Output Options
NVTT_API NvttOutputOptions * nvttCreateOutputOptions();
NVTT_API void nvttDestroyOutputOptions(NvttOutputOptions * outputOptions);

NVTT_API void nvttSetOutputOptionsFileName(NvttOutputOptions * outputOptions, const char * fileName);


// Main entrypoint of the compression library.
NVTT_API NvttBoolean nvttCompress(NvttInputOptions * inputOptions, NvttCompressionOptions * compressionOptions, NvttOutputOptions * outputOptions);
	
// Estimate the size of compressing the input with the given options.
NVTT_API int nvttEstimateSize(NvttInputOptions * inputOptions, NvttCompressionOptions * compressionOptions);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // NVTT_WRAPPER_H
