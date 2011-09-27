// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
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

#include "CubeImage.h"
#include "TexImage.h"

using namespace nv;
using namespace nvtt;




CubeImage::CubeImage() : m(new CubeImage::Private())
{
    m->addRef();
}

CubeImage::CubeImage(const CubeImage & cube) : m(cube.m)
{
    if (m != NULL) m->addRef();
}

CubeImage::~CubeImage()
{
    if (m != NULL) m->release();
    m = NULL;
}

void CubeImage::operator=(const CubeImage & cube)
{
    if (cube.m != NULL) cube.m->addRef();
    if (m != NULL) m->release();
    m = cube.m;
}

void CubeImage::detach()
{
    if (m->refCount() > 1)
    {
        m->release();
        m = new CubeImage::Private(*m);
        m->addRef();
        nvDebugCheck(m->refCount() == 1);
    }
}



bool CubeImage::isNull() const
{
    return m->size == 0;
}

int CubeImage::size() const
{
    return m->size;
}

int CubeImage::countMipmaps() const
{
    return nv::countMipmaps(m->size);
}

TexImage & CubeImage::face(int f)
{
    nvDebugCheck(f >= 0 && f < 6);
    return m->face[f];
}


bool CubeImage::load(const char * fileName)
{
    return false;
}

bool CubeImage::save(const char * fileName) const
{
    return false;
}


void CubeImage::fold(const TexImage & tex, CubeLayout layout)
{
    
}

TexImage CubeImage::unfold(CubeLayout layout)
{

}



void CubeImage::toLinear(float gamma)
{
    for (int i = 0; i < 6; i++) {
        m->face[i].toLinear(gamma);
    }
}

void CubeImage::toGamma(float gamma)
{
    for (int i = 0; i < 6; i++) {
        m->face[i].toGamma(gamma);
    }
}

