
#ifndef NVTT_EXPERIMENTAL_H
#define NVTT_EXPERIMENTAL_H

#include <nvtt/nvtt.h>

typedef struct NvttImage NvttImage;

NvttImage * nvttCreateImage();
void nvttDestroyImage(NvttImage * img);

void nvttSetImageData(NvttImage * img, NvttInputFormat format, uint w, uint h, void * data);

void nvttCompressImage(NvttImage * img, NvttFormat format);

// How to control the compression parameters?

// Using many arguments:
// void nvttCompressImage(img, format, quality, r, g, b, a, ...);

// Using existing compression option class:
// compressionOptions = nvttCreateCompressionOptions();
// nvttSetCompressionOptionsFormat(compressionOptions, format);
// nvttSetCompressionOptionsQuality(compressionOptions, quality);
// nvttSetCompressionOptionsQuality(compressionOptions, quality);
// nvttSetCompressionOptionsColorWeights(compressionOptions, r, g, b, a);
// ...
// nvttCompressImage(img, compressionOptions);

// Using thread local context state:
// void nvttSetCompressionFormat(format);
// void nvttSetCompressionQuality(quality);
// void nvttSetCompressionColorWeights(r, g, b, a);
// ...
// nvttCompressImage(img);

// Using thread local context state, but with GL style function arguments:
// nvttCompressorParameteri(NVTT_FORMAT, format);
// nvttCompressorParameteri(NVTT_QUALITY, quality);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_RED, r);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_GREEN, g);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_BLUE, b);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_ALPHA, a);
// or nvttCompressorParameter4f(NVTT_COLOR_WEIGHTS, r, g, b, a);
// ...
// nvttCompressImage(img);

// How do we get the compressed output?
// - Using callbacks. (via new entrypoints, or through outputOptions)
// - Return it explicitely from nvttCompressImage.
// - Store it along the image, retrieve later explicitely with 'nvttGetCompressedData(img, ...)'



#endif // NVTT_EXPERIMENTAL_H
