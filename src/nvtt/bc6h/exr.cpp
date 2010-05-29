/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

// Simple .exr file reader/writer

#include <string>

#include <ImfRgbaFile.h>
#include <ImfArray.h>

#include "exr.h"

using namespace std;
using namespace Imf;
using namespace Imath;

void Exr::fileinfo(const string inf, int &width, int &height)
{
	RgbaInputFile file (inf.c_str());
    Box2i dw = file.dataWindow();

    width  = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;
}

void Exr::readRgba(const string inf, Array2D<Rgba> &pix, int &w, int &h)
{
    RgbaInputFile file (inf.c_str());
    Box2i dw = file.dataWindow();
    w  = dw.max.x - dw.min.x + 1;
    h = dw.max.y - dw.min.y + 1;
    pix.resizeErase (h, w);
    file.setFrameBuffer (&pix[0][0] - dw.min.x - dw.min.y * w, 1, w);
    file.readPixels (dw.min.y, dw.max.y);
}

void Exr::writeRgba(const string outf, const Array2D<Rgba> &pix, int w, int h)
{
	RgbaOutputFile file (outf.c_str(), w, h, WRITE_RGBA);
	file.setFrameBuffer (&pix[0][0], 1, w);
	file.writePixels (h);
}