// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef NV_TT_TEXTURE_H
#define NV_TT_TEXTURE_H

#include "nvtt.h"

#include <nvcore/Containers.h>
#include <nvcore/RefCounted.h>
#include <nvcore/Ptr.h>

#include <nvimage/Image.h>
#include <nvimage/FloatImage.h>

namespace nvtt
{

	struct TexImage::Private : public nv::RefCounted
	{
		Private()
		{
			type = TextureType_2D;
			wrapMode = WrapMode_Mirror;
			alphaMode = AlphaMode_None;
			isNormalMap = false;

			imageArray.resize(1, NULL);
		}
		Private(const Private & p)
		{
			type = p.type;
			wrapMode = p.wrapMode;
			alphaMode = p.alphaMode;
			isNormalMap = p.isNormalMap;

			imageArray = p.imageArray;
		}
		~Private()
		{
			// @@ Free images.
		}

		TextureType type;
		WrapMode wrapMode;
		AlphaMode alphaMode;
		bool isNormalMap;

		nv::Array<nv::FloatImage *> imageArray;
	};

	
} // nvtt namespace


#endif // NV_TT_TEXTURE_H
