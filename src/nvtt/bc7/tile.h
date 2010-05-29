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

#include "ImfArray.h"
#include <math.h>
#include "arvo/Vec4.h"
#include "utils.h"
#include "rgba.h"

using namespace Imf;
using namespace ArvoMath;

// extract a tile of pixels from an array

class Tile
{
public:
	static const int TILE_H = 4;
	static const int TILE_W = 4;
	static const int TILE_TOTAL = TILE_H * TILE_W;
	Vec4 data[TILE_H][TILE_W];
	int	size_x, size_y;			// actual size of tile

	Tile() {};
	~Tile(){};
	Tile(int xs, int ys) {size_x = xs; size_y = ys;}

	// pixels -> tile
	void inline insert(const Array2D<RGBA> &pixels, int x, int y)
	{
		for (int y0=0; y0<size_y; ++y0)
		for (int x0=0; x0<size_x; ++x0)
		{
			data[y0][x0].X() = (pixels[y+y0][x+x0]).r;
			data[y0][x0].Y() = (pixels[y+y0][x+x0]).g;
			data[y0][x0].Z() = (pixels[y+y0][x+x0]).b;
			data[y0][x0].W() = (pixels[y+y0][x+x0]).a;
		}
	}

	// tile -> pixels
	void inline extract(Array2D<RGBA> &pixels, int x, int y)	
	{
		for (int y0=0; y0<size_y; ++y0)
		for (int x0=0; x0<size_x; ++x0)
		{
			pixels[y+y0][x+x0].r = data[y0][x0].X();
			pixels[y+y0][x+x0].g = data[y0][x0].Y();
			pixels[y+y0][x+x0].b = data[y0][x0].Z();
			pixels[y+y0][x+x0].a = data[y0][x0].W();
		}
	}
};

#endif