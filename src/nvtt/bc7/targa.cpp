/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

// Quick and dirty Targa file I/O -- doesn't handle compressed format targa files, though.

#include <stdexcept>
#include <iostream>

#include "ImfArray.h"
#include "targa.h"
#include "rgba.h"

Targa::Targa() {}

Targa::~Targa() {}

// read either RGB or RGBA files
static int readTgaHeader(FILE *fp, int& width, int& height, int &bpp, int &origin)
{
	unsigned char hdr[18];

	if (fread(hdr, sizeof(hdr), 1, fp ) != 1)
		return 0;

	if (hdr[2] != 2)
		return 0;

	bpp = hdr[16];
	if (bpp != 24 && bpp != 32)
		return 0;

	int alphabpp = hdr[17] & 0xF;
	origin = (hdr[17] >> 4) & 0x3;

	if (bpp == 24 && alphabpp != 0)
		return 0;
	if (bpp == 32 && alphabpp != 8)
		return 0;

	width = (hdr[13] << 8) | hdr[12];
	height = (hdr[15] << 8) | hdr[14];

	// skip image ID field
	int idsize = hdr[0];
	for (; idsize; --idsize)
		(void) getc(fp);

	return 1;
}

static void read_file(FILE *fp, Imf::Array2D<RGBA>& pixels, int width, int height, int bpp, int origin)
{
	pixels.resizeErase(height, width);

	// bottom to top order
	for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	{
		float b = float(getc(fp));
		float g = float(getc(fp));
		float r = float(getc(fp));
		float a = (bpp == 24) ? RGBA_MAX : float(getc(fp));

		int xt, yt;

		// transform based on origin
		switch (origin)
		{
		case 0:	xt = x; yt = height-1-y; break;		// bottom left
		case 1:	xt = width-1-x; yt = y;	break;		// bottom right
		case 2:	xt = x; yt = y;	break;				// top left
		case 3: xt = width-1-x; yt = height-1-y; break;	// top right
		default:  throw "impossible origin value";
		}

		pixels[yt][xt].a = a;
		pixels[yt][xt].r = r;
		pixels[yt][xt].g = g;
		pixels[yt][xt].b = b;
	}
}

void Targa::fileinfo(const std::string& filename, int& width, int& height, bool& const_alpha)
{
	int bpp, origin;

	FILE *fp = fopen(filename.c_str(), "rb");

	if (fp == (FILE *) 0)
		throw "Unable to open infile";

	if (readTgaHeader(fp, width, height, bpp, origin) == 0)
		throw "Invalid or unimplemented format for infile, needs to be a 24 or 32 bit uncompressed TGA file";

	if (bpp == 24)
		const_alpha = true;
	else
	{
		// even if file is 32bpp the alpha may still be constant. so read file and check
		Imf::Array2D<RGBA> pixels;
		
		read_file(fp, pixels, width, height, bpp, origin);

		const_alpha = true;

		for (int y=0; y<height && const_alpha; ++y)
		for (int x=0; x<width && const_alpha; ++x)
			if (pixels[y][x].a != 255.0)
				const_alpha = false;
	}

	fclose(fp);
}


void Targa::read(const std::string& filename, Imf::Array2D<RGBA>& pixels, int& width, int& height)
{
	int bpp, origin;

	FILE *fp = fopen(filename.c_str(), "rb");

	if (fp == (FILE *) 0)
		throw "Unable to open infile";

	if (readTgaHeader(fp, width, height, bpp, origin) == 0)
		throw "Invalid or unimplemented format for infile, needs to be a 24 or 32 bit uncompressed TGA file";

	read_file(fp, pixels, width, height, bpp, origin);

	fclose(fp);
}

void Targa::write(const std::string& filename, const Imf::Array2D<RGBA>& pixels, int width, int height)
{
	FILE *fp = fopen(filename.c_str(), "wb");

	if (fp == (FILE *) 0)
		throw "Unable to open outfile";

	unsigned char hdr[18];

	// we're lazy, always write this as a 32bpp file, even if the alpha is constant 255

	memset(hdr, 0, sizeof(hdr));
	hdr[2]  = 2;
	hdr[12] = width & 0xFF;
	hdr[13] = width >> 8;
	hdr[14] = height & 0xFF;
	hdr[15] = height >> 8;
	hdr[16] = 32;
	hdr[17] = 0x28;

	fwrite( hdr, sizeof(hdr), 1, fp );

	// top to bottom order
	for (int y = 0; y < height; ++y)
	for (int x = 0; x < width; ++x)
	{
		int a = int((pixels[y][x]).a + 0.5f);
		int r = int((pixels[y][x]).r + 0.5f);
		int g = int((pixels[y][x]).g + 0.5f);
		int b = int((pixels[y][x]).b + 0.5f);

		if (b < RGBA_MIN) b = RGBA_MIN; if (b > RGBA_MAX) b = RGBA_MAX; fputc(b, fp);
		if (g < RGBA_MIN) g = RGBA_MIN; if (g > RGBA_MAX) g = RGBA_MAX; fputc(g, fp);
		if (r < RGBA_MIN) r = RGBA_MIN; if (r > RGBA_MAX) r = RGBA_MAX; fputc(r, fp);
		if (a < RGBA_MIN) a = RGBA_MIN; if (a > RGBA_MAX) a = RGBA_MAX; fputc(a, fp);
	}
	fclose(fp);
}
