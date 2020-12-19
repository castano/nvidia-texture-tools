After more than 14 years maintaining and updating this library on my spare time, I've decided to officially discontinue it and focus my enery on other projects.

When I released NVTT there was very little public information about compression for GPU texture formats. Existing codecs were closed-source, encumbered by patents, and not particularly efficient or high quality. A lot has changed since then. Today most IHVs have their own open source codecs and several companies develop high quality commercial products around texture compression. 

NVTT doesn't support the latest formats anymore, many of its codecs are outdated, and it's source code has aged. NVIDIA doesn't support my work in any way, but its users have come to expect that support. In retrospect naming a personal project after my employer was a mistake!

If you are looking for alternative texture compression tools and algorithms I recommend you check these out:

* I still maintain [A High Quality SIMD BC1 Encoder](https://github.com/castano/icbc).
* Deano Calver supports [three tiny libraries](https://deanoc.com/2019/09/tiny) that provide support for DDS and KTX file formats, and pixel format conversion.
* [stb_image_resize](https://github.com/nothings/stb/blob/master/stb_image_resize.h) provides polyphase image resize filters that are similar to what NVTT supports.
* [Oodle Texture](http://www.radgametools.com/oodletexture.htm) is a suite of commercial RDO texture codecs.
* [Binomial](https://www.binomial.info/) is an image and texture compression company that develops [Basis a universal texture codec](https://github.com/BinomialLLC/basis_universal) and also has [other source codecs](https://github.com/BinomialLLC).
* [NVIDIA Texture Tools exporter](https://developer.nvidia.com/nvidia-texture-tools-exporter) is based on a private fork of this project and offers additional GPU accelerated codecs, but it's not open source.
* [Intel ISPC Texture Compressor](https://github.com/GameTechDev/ISPCTextureCompressor) is a set of set of open source SIMD texture codecs that are very fast, but low quality.
* [AMD Compressonator](https://gpuopen.com/compressonator/) offers a wide variety of open source compressors.
* [ARM ASTC Encoder](https://github.com/ARM-software/astc-encoder) is an excellent ASTC encoder.
* [Dario Manesku's cube map filtering tool](https://github.com/dariomanesku/cmft) seems like a good alternative for the cube map filtering functions in NVTT.


# NVIDIA Texture Tools [![Actions Status](https://github.com/castano/nvidia-texture-tools/workflows/build/badge.svg)](https://github.com/castano/nvidia-texture-tools/actions) ![MIT](https://img.shields.io/badge/license-MIT-blue.svg) [![GitHub](https://img.shields.io/badge/repo-github-green.svg)](https://github.com/castano/nvidia-texture-tools)

The NVIDIA Texture Tools is a collection of image processing and texture 
manipulation tools, designed to be integrated in game tools and asset 
processing pipelines.

The primary features of the library are mipmap and normal map generation, format 
conversion, and DXT compression.


### How to build (Windows)

Use the provided Visual Studio 2017 solution `project/vc2017/thekla.sln`.


### How to build (Linux/OSX)

Use [cmake](http://www.cmake.org/) and the provided configure script:

```bash
$ ./configure
$ make
$ sudo make install
```


### Using NVIDIA Texture Tools

To use the NVIDIA Texture Tools in your own applications you just have to
include the following header file:

[src/nvtt/nvtt.h](https://github.com/castano/nvidia-texture-tools/blob/master/src/nvtt/nvtt.h)

And include the nvtt library in your projects. 

The following file contains a simple example that shows how to use the library:

[src/nvtt/tools/compress.cpp](https://github.com/castano/nvidia-texture-tools/blob/master/src/nvtt/tools/compress.cpp)

Detailed documentation of the API can be found at:

https://github.com/castano/nvidia-texture-tools/wiki/ApiDocumentation

