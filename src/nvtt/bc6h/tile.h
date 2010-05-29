/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef _TILE_H
#define _TILE_H

#include <ImfArray.h>
#include <ImfRgba.h>
#include <half.h>
#include <math.h>
#include "arvo/Vec3.h"

#include "utils.h"

#define	DBL_MAX	(1.0e37)		// doesn't have to be really dblmax, just bigger than any possible squared error

using namespace Imf;
using namespace ArvoMath;

//#define	USE_IMPORTANCE_MAP	1		// define this if you want to increase importance of some pixels in tile
class Tile
{
private:
	// NOTE: this returns the appropriately-clamped BIT PATTERN of the half as an INTEGRAL float value
	static float half2float(half h)
	{
		return (float) Utils::ushort_to_format(h.bits());
	}
	// NOTE: this is the inverse of the above operation
	static half float2half(float f)
	{
		half h;
		h.setBits(Utils::format_to_ushort((int)f));
		return h;
	}
	// look for adjacent pixels that are identical. if there are enough of them, increase their importance
	void generate_importance_map()
	{
		// initialize
		for (int y=0; y<size_y; ++y)
		for (int x=0; x<size_x; ++x)
		{
			// my importance is increased if I am identical to any of my 4-neighbors
			importance_map[y][x] = match_4_neighbor(x,y) ? 5.0f : 1.0f;
		}
	}
	bool is_equal(int x, int y, int xn, int yn)
	{
		if (xn < 0 || xn >= size_x || yn < 0 || yn >= size_y)
			return false;
		return( (data[y][x].X() == data[yn][xn].X()) &&
				(data[y][x].Y() == data[yn][xn].Y()) &&
				(data[y][x].Z() == data[yn][xn].Z()) );
	}
#ifdef USE_IMPORTANCE_MAP
	bool match_4_neighbor(int x, int y)
	{
		return is_equal(x,y,x-1,y) || is_equal(x,y,x+1,y) || is_equal(x,y,x,y-1) || is_equal(x,y,x,y+1);
	}
#else
	bool match_4_neighbor(int x, int y)
	{
		return false;
	}
#endif

public:
	Tile() {};
	~Tile(){};
	Tile(int xs, int ys) {size_x = xs; size_y = ys;}

	static const int TILE_H = 4;
	static const int TILE_W = 4;
	static const int TILE_TOTAL = TILE_H * TILE_W;
	Vec3 data[TILE_H][TILE_W];
	float importance_map[TILE_H][TILE_W];
	int	size_x, size_y;			// actual size of tile

	// pixels -> tile
	void inline insert(const Array2D<Rgba> &pixels, int x, int y)
	{
		for (int y0=0; y0<size_y; ++y0)
		for (int x0=0; x0<size_x; ++x0)
		{
			data[y0][x0].X() = half2float((pixels[y+y0][x+x0]).r);
			data[y0][x0].Y() = half2float((pixels[y+y0][x+x0]).g);
			data[y0][x0].Z() = half2float((pixels[y+y0][x+x0]).b);
		}
		generate_importance_map();
	}

	// tile -> pixels
	void inline extract(Array2D<Rgba> &pixels, int x, int y)	
	{
		for (int y0=0; y0<size_y; ++y0)
		for (int x0=0; x0<size_x; ++x0)
		{
			pixels[y+y0][x+x0].r = float2half(data[y0][x0].X());
			pixels[y+y0][x+x0].g = float2half(data[y0][x0].Y());
			pixels[y+y0][x+x0].b = float2half(data[y0][x0].Z());
			pixels[y+y0][x+x0].a = 0;		// set it to a known value
		}
	}
};

#endif