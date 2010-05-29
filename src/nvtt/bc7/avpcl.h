/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef _AVPCL_H
#define _AVPCL_H

#include <string>
#include <assert.h>

#include "tile.h"
#include "bits.h"

using namespace std;

#define	EXTERNAL_RELEASE	1	// define this if we're releasing this code externally
#define	DISABLE_EXHAUSTIVE	1	// define this if you don't want to spend a lot of time on exhaustive compression
#define	USE_ZOH_INTERP		1	// use zoh interpolator, otherwise use exact avpcl interpolators
#define	USE_ZOH_INTERP_ROUNDED 1	// use the rounded versions!

#define	NREGIONS_TWO	2
#define	NREGIONS_THREE	3
#define	DBL_MAX	(1.0e37)		// doesn't have to be really dblmax, just bigger than any possible squared error

class AVPCL
{
public:
	static const int BLOCKSIZE=16;
	static const int BITSIZE=128;

	// global flags
	static bool flag_premult;
	static bool flag_nonuniform;
	static bool flag_nonuniform_ati;

	// global mode
	static bool mode_rgb;		// true if image had constant alpha = 255

	static void compress(string inf, string zohf, string errf);
	static void decompress(string zohf, string outf);
	static void compress(const Tile &t, char *block, FILE *errfile);
	static void decompress(const char *block, Tile &t);

	static double compress_mode0(const Tile &t, char *block);
	static void decompress_mode0(const char *block, Tile &t);

	static double compress_mode1(const Tile &t, char *block);
	static void decompress_mode1(const char *block, Tile &t);

	static double compress_mode2(const Tile &t, char *block);
	static void decompress_mode2(const char *block, Tile &t);

	static double compress_mode3(const Tile &t, char *block);
	static void decompress_mode3(const char *block, Tile &t);

	static double compress_mode4(const Tile &t, char *block);
	static void decompress_mode4(const char *block, Tile &t);

	static double compress_mode5(const Tile &t, char *block);
	static void decompress_mode5(const char *block, Tile &t);

	static double compress_mode6(const Tile &t, char *block);
	static void decompress_mode6(const char *block, Tile &t);

	static double compress_mode7(const Tile &t, char *block);
	static void decompress_mode7(const char *block, Tile &t);

	static int getmode(Bits &in)
	{
		int mode = 0;

		if (in.read(1))			mode = 0;
		else if (in.read(1))	mode = 1;
		else if (in.read(1))	mode = 2;
		else if (in.read(1))	mode = 3;
		else if (in.read(1))	mode = 4;
		else if (in.read(1))	mode = 5;
		else if (in.read(1))	mode = 6;
		else if (in.read(1))	mode = 7;
		else mode = 8;	// reserved
		return mode;
	}
	static int getmode(const char *block)
	{
		int bits = block[0], mode = 0;

		if (bits & 1) mode = 0;
		else if ((bits&3) == 2) mode = 1;
		else if ((bits&7) == 4) mode = 2;
		else if ((bits & 0xF) == 8) mode = 3;
		else if ((bits & 0x1F) == 16) mode = 4;
		else if ((bits & 0x3F) == 32) mode = 5;
		else if ((bits & 0x7F) == 64) mode = 6;
		else if ((bits & 0xFF) == 128) mode = 7;
		else mode = 8;	// reserved
		return mode;
	}
};
#endif