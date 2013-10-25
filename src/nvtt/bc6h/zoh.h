/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/
#pragma once
#ifndef _ZOH_H
#define _ZOH_H

#include "tile.h"

// UNUSED ZOH MODES are 0x13, 0x17, 0x1b, 0x1f

#define	EXTERNAL_RELEASE	1	// define this if we're releasing this code externally

#define	NREGIONS_TWO	2
#define	NREGIONS_ONE	1
#define	NCHANNELS	3

// Note: this code only reads OpenEXR files, which are only in F16 format.
// if unsigned is selected, the input is clamped to >= 0.
// if f16 is selected, the range is clamped to 0..0x7bff.

struct FltEndpts
{
    nv::Vector3 A;
    nv::Vector3 B;
};

struct IntEndpts
{
	int A[NCHANNELS];
	int B[NCHANNELS];
};

struct ComprEndpts
{
	uint A[NCHANNELS];
	uint B[NCHANNELS];
};

class ZOH
{
public:
	static const int BLOCKSIZE=16;
	static const int BITSIZE=128;
	static Format FORMAT;

	static void compress(const Tile &t, char *block);
	static void decompress(const char *block, Tile &t);

	static float compressone(const Tile &t, char *block);
	static float compresstwo(const Tile &t, char *block);
	static void decompressone(const char *block, Tile &t);
	static void decompresstwo(const char *block, Tile &t);

	static float refinetwo(const Tile &tile, int shapeindex_best, const FltEndpts endpts[NREGIONS_TWO], char *block);
	static float roughtwo(const Tile &tile, int shape, FltEndpts endpts[NREGIONS_TWO]);

	static float refineone(const Tile &tile, int shapeindex_best, const FltEndpts endpts[NREGIONS_ONE], char *block);
	static float roughone(const Tile &tile, int shape, FltEndpts endpts[NREGIONS_ONE]);

	static bool isone(const char *block);
};

#endif // _ZOH_H
