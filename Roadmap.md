Here's a rough roadmap for the next releases of the NVIDIA Texture Tools:

# 2.1 #

Completed features:
  * ~~Support for linear color transforms and swizzles: [Issue 4](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=4)~~
  * ~~Migration to CUDA 2.0: [Issue 46](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=46)~~
  * ~~Support for alpha premultiplication. [Issue 30](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=30)~~
  * ~~Output AMD swizzle codes in DDS headers.~~

In progress:

  * Major refactoring and cleanup.
  * ~~Support for different alpha modes: [Issue 30](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=30)~~
  * Support for floating point formats: [Issue 27](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=27)
  * Support for YCoCg color transform: [Issue 18](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=18)

Optional features:

  * DXT1n support: [Issue 21](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=21)
  * CTX1 support: [Issue 21](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=21)
  * Add option to disable single color compressor: [Issue 53](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=53)
  * Image viewer (nvdisplay): [Issue 53](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=53)


# 2.2 #

Planned features:

  * More CUDA accelerated compressors and image processing operations. [Issue 29](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=29)
  * Support for DX11 compression formats.
  * RGBE output format: [Issue 57](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=57)
  * Simple tone mapping options: [Issue 58](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=58)
  * Better support for DX10 style DDS files: [Issue 41](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=41)

Optional features:

  * Win64 image loaders: [Issue 31](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=31)
  * Better command line parsing and more command line options.
  * Experimental lower level interface.


# 2.3 #

Planned features:

  * Use CUDA driver interface: [Issue 25](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=25)
  * Photoshop plugin: [Issue 33](http://code.google.com/p/nvidia-texture-tools/issues/detail?id=33)