# Introduction #

## nvcompress ##

Compresses the given image and produces a DDS file.

```
usage: nvcompress [options] infile [outfile]

Input options:
  -color        The input image is a color map (default).
  -normal       The input image is a normal map.
  -tonormal     Convert input to normal map.
  -clamp        Clamp wrapping mode (default).
  -repeat       Repeat wrapping mode.
  -nomips       Disable mipmap generation.

Compression options:
  -fast         Fast compression.
  -nocuda       Do not use cuda compressor.
  -rgb          RGBA format
  -bc1          BC1 format (DXT1)
  -bc1n         BC1 normal map format (DXT1nm)
  -bc1a         BC1 format with binary alpha (DXT1a)
  -bc2          BC2 format (DXT3)
  -bc3          BC3 format (DXT5)
  -bc3n         BC3 normal map format (DXT5nm)
  -bc4          BC4 format (ATI1)
  -bc5          BC5 format (3Dc/ATI2)
```

## nvdecompress ##

Decompresses the given DDS file.

```
usage: nvdecompress ddsfile
```

## nvddsinfo ##

Outputs information about the given DDS file.

```
usage: nvddsinfo ddsfile
```

## nvimgdiff ##

```
usage: nvimgdiff [options] original_file updated_file

Diff options:
  -normal       Compare images as if they were normal maps.
  -alpha        Compare alpha weighted images.
```

## nvassemble ##

Create cubemap DDS files from multiple images.

```
usage: nvassemble [-cube|-volume|-array] 'file0' 'file1' ..
```

## nvzoom ##

Scales the given image.

```
usage: nvzoom [options] input [output]

Options:
  -s scale     Scale factor (default = 0.5)
  -g gamma     Gamma correction (default = 2.2)
  -f filter    One of the following: (default = 'box')
                * box
                * triangle
                * quadratic
                * bspline
                * mitchell
                * lanczos
                * kaiser
```