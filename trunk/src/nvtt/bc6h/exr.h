/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef _EXR_H
#define _EXR_H

// exr-friendly routines

#include <string>

#include "ImfArray.h"
#include "ImfRgba.h"

using namespace std;
using namespace Imf;

class Exr
{
public:
	Exr() {};
	~Exr() {};

	static void fileinfo(const string inf, int &width, int &height);
	static void readRgba(const string inf, Array2D<Rgba> &pix, int &w, int &h);
	static void writeRgba(const string outf, const Array2D<Rgba> &pix, int w, int h);
};

#endif