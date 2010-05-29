/*
Copyright 2007 nVidia, Inc.
Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 

You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 

See the License for the specific language governing permissions and limitations under the License.
*/

#ifndef _targa_h_
#define _targa_h_

#include "ImfArray.h"
#include "rgba.h"

class Targa
{
public:
	Targa();
	~Targa();

	static void fileinfo( const std::string& filename, int& width, int& height, bool& const_alpha);
	static void read( const std::string& filename, Imf::Array2D<RGBA>& pixels, int& width, int& height );
	static void write(const std::string& filename, const Imf::Array2D<RGBA>& pixels, int width, int height );
};

#endif /* _targa_h_ */
