# NVTT is discontinued

After more than 14 years maintaining and updating this library on my spare time, I've decided to officially discontinue it and focus my energy on other projects.

When I released NVTT there was very little public information about compression for GPU texture formats. Existing codecs were closed-source, encumbered by patents, and not particularly efficient or high quality. A lot has changed since then. NVTT doesn't support the latest formats anymore, some of its codecs are outdated, and it's source code has aged. Today most IHVs maintain their own open source codecs and several companies develop high quality commercial products around texture compression. 

If you are looking for alternative texture compression tools and processing algorithms I recommend you check these out:

## Open Source Encoders

* I still maintain [A High Quality SIMD BC1 Encoder](https://github.com/castano/icbc).
* [rgbcx](https://github.com/richgel999/bc7enc/blob/master/rgbcx.h) is another high-performance (but scalar) BC1-5 encoder.
* [stb_dxt](https://github.com/nothings/stb/blob/master/stb_dxt.h) is a single header library that provides low quality, but fast BC1-5 encoders.
* [Binomial](https://www.binomial.info/) is an image and texture compression company that develops [Basis a universal texture codec](https://github.com/BinomialLLC/basis_universal) and also has [other open source codecs](https://github.com/BinomialLLC).
* [Intel ISPC Texture Compressor](https://github.com/GameTechDev/ISPCTextureCompressor) is a set of set of open source SIMD texture encoders that are very fast, but low quality.
* [AMD Compressonator](https://gpuopen.com/compressonator/) offers various open source encoders that also run on the GPU.
* [Betsy](https://github.com/darksylinc/betsy/) is a compressor for various GPU formats using compute shaders.
* For ETC encoders see Google's [Etc2Comp](https://github.com/google/etc2comp), Bartosz Taudul's [etcpak](https://github.com/wolfpld/etcpak) and Rich Geldreich's [rg-etc1](https://github.com/richgel999/rg-etc1).
* [ARM ASTC Encoder](https://github.com/ARM-software/astc-encoder) is an excellent ASTC encoder.
* This is [a good overview of the state of texture encoders in 2020](https://aras-p.info/blog/2020/12/08/Texture-Compression-in-2020/).

## Image Processing and IO

* [stb_image_resize](https://github.com/nothings/stb/blob/master/stb_image_resize.h) provides polyphase image resize filters that are similar to what NVTT supports.
* [Dario Manesku's cube map filtering tool](https://github.com/dariomanesku/cmft) seems like a good alternative for the cube map filtering functions in NVTT.
* [CubeMapGen](https://gpuopen.com/archived/cubemapgen/) is another source of information for cubemap filtering algorithms, but is now unsupported as well.
* Deano Calver supports [three tiny libraries](https://deanoc.com/2019/09/tiny) that provide support for DDS and KTX file formats, and pixel format conversion.
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) and [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h) are two handy single header libraries to read and write images in various formats.
* https://github.com/syoyo/tinyexr
* https://lodev.org/lodepng/
* [texturec](https://bkaradzic.github.io/bgfx/tools.html#texture-compiler-texturec) is a handy command line tool similar to `nvcompress`, built on top of [bimg](https://github.com/bkaradzic/bimg).
* The venerable [ImageMagick](https://imagemagick.org/) still provides many valuable tools and utilities.

## Commercial Libraries

* [Oodle Texture](http://www.radgametools.com/oodletexture.htm) is a suite of commercial RDO texture codecs.
* [NVIDIA Texture Tools exporter](https://developer.nvidia.com/nvidia-texture-tools-exporter) is based on a private fork of this project and offers additional GPU accelerated codecs, but it's not open source.

-------------------------------

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

