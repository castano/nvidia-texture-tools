The compressors available in the NVIDIA Texture Tools are not designed for real-time compression. If you need to compress textures in real-time, I'd recommend to look at the following sources:

[J.M.P. van Waveren](http://mrelusive.com) was the first one to describe a [real-time DXT compressor](http://www.intel.com/cd/ids/developer/asmo-na/eng/dc/index.htm). This compressor is part of the of [real-time texture streaming pipeline](http://softwarecommunity.intel.com/UserFiles/en-us/Image/1221/Real-Time%20Texture%20Streaming%20&%20Decompression.pdf) used in some [id Software](http://www.idsoftware.com) games. He obtains the following results with his SSE2 optimized implementation:

| **CPU** | **DXT1** | **DXT5** |
|:--------|:---------|:---------|
| Intel 2.8 GHz Dual Core Xeon | 112.05 MP/s | 66.43 MP/s |
| Intel 2.9 GHz Core 2 Extreme | 200.62 MP/s | 127.55 MP/s |

The same algorithm described by Waveren can also be adapted easily to the GPU. This is what Simon Green did in [this OpenGL SDK example](http://developer.download.nvidia.com/SDK/10/opengl/samples.html#compress_DXT). The performance on the GPU is much more impressive:

| **GPU** | **DXT1** | **DXT5** |
|:--------|:---------|:---------|
| GeForce 8800 GTX | 1,547 MP/s | - |
| GeForce 8600 GTS | 461 MP/s | - |

A later [whitepaper](http://developer.nvidia.com/object/real-time-ycocg-dxt-compression.html) by Waveren and [Ignacio Castaño](http://castano.ludicon.com) provides even higher results on the GPU:

| **GPU** | **DXT1** | **YCoCg-DXT5** |
|:--------|:---------|:---------------|
| GeForce 8800 GTX | 1,690 MP/s | 939 MP/s |
| GeForce 8600 GTS | 520 MP/s | 279 MP/s |

Peter Uliciansky optimized Waveren's algorithm further and published his results in this thread: [Extreme DXT Compression, New algorithm for real-time DXT compression](http://developer.nvidia.com/forums/index.php?showtopic=1737).

| **CPU**                   | **DXT1**     | **DXT5**     |
|:--------------------------|:-------------|:-------------|
| Intel Pentium 4 2.8 GHz | 241.2 MP/s | 206.5 MP/s |
| Intel Core 2 3.0 GHz    | 910.0 MP/s | 620.4 MP/s |

These numbers are very impressive and start to get closer to the results of the GPU implementation. However, as pointed by [Charles Bloom](http://www.cbloom.com/) this implementation has [some errors](http://cbloomrants.blogspot.com/2008/11/11-18-08-dxtc.html).

Moreover, the latest GPU implementation also offers higher performance.
  * http://developer.download.nvidia.com/SDK/10/opengl/samples.html#compress_YCoCgDXT
  * http://developer.download.nvidia.com/SDK/10/opengl/samples.html#compress_NormalDXT

| **GPU**             | **DXT1**     | **YCoCg-DXT5** | **BC5**      | **DXT5n**    |
|:--------------------|:-------------|:---------------|:-------------|:-------------|
| GeForce 8800 GTX | 3450 MP/s  | 1880 MP/s    | 5665 MP/s  | 5804 MP/s  |
| GeForce GTX 280  | 9920 MP/s  | 7900 MP/s    | 12850 MP/s | 13150 MP/s |



Other links:
  * https://mollyrocket.com/forums/viewtopic.php?t=392
  * http://www.evl.uic.edu/cavern/fastdxt/