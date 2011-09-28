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

#include "nvmath/Vector.h"

#include "nvcore/Array.h"


using namespace nv;
using namespace nvtt;




CubeSurface::CubeSurface() : m(new CubeSurface::Private())
{
    m->addRef();
}

CubeSurface::CubeSurface(const CubeSurface & cube) : m(cube.m)
{
    if (m != NULL) m->addRef();
}

CubeSurface::~CubeSurface()
{
    if (m != NULL) m->release();
    m = NULL;
}

void CubeSurface::operator=(const CubeSurface & cube)
{
    if (cube.m != NULL) cube.m->addRef();
    if (m != NULL) m->release();
    m = cube.m;
}

void CubeSurface::detach()
{
    if (m->refCount() > 1)
    {
        m->release();
        m = new CubeSurface::Private(*m);
        m->addRef();
        nvDebugCheck(m->refCount() == 1);
    }
}



bool CubeSurface::isNull() const
{
    return m->size == 0;
}

int CubeSurface::size() const
{
    return m->size;
}

int CubeSurface::countMipmaps() const
{
    return nv::countMipmaps(m->size);
}

Surface & CubeSurface::face(int f)
{
    nvDebugCheck(f >= 0 && f < 6);
    return m->face[f];
}

const Surface & CubeSurface::face(int f) const
{
    nvDebugCheck(f >= 0 && f < 6);
    return m->face[f];
}


bool CubeSurface::load(const char * fileName)
{
    // @@ TODO
    return false;
}

bool CubeSurface::save(const char * fileName) const
{
    // @@ TODO
    return false;
}


void CubeSurface::fold(const Surface & tex, CubeLayout layout)
{
    // @@ TODO
}

Surface CubeSurface::unfold(CubeLayout layout) const
{
    // @@ TODO
    return Surface();
}


CubeSurface CubeSurface::irradianceFilter(int size) const
{
    // @@ TODO
    return CubeSurface();
}


// Small solid angle table that takes into account cube map symmetry.
struct SolidAngleTable {

    SolidAngleTable(int edgeLength) : size(edgeLength/2) {
        // Allocate table.
        data.resize(size * size);

        // @@ Init table.

    }


    //
    float lookup(int x, int y) const {
        if (x >= size) x -= size;
        else if (x < size) x = size - x - 1;
        if (y >= size) y -= size;
        else if (y < size) y = size - y - 1;

        return data[y * size + x];
    }

    int size;
    nv::Array<float> data;
};


// ilen = inverse edge length.
Vector3 texelDirection(uint face, uint x, uint y, float ilen)
{
    float u = (float(x) + 0.5f) * (2 * ilen) - 1.0f;
    float v = (float(y) + 0.5f) * (2 * ilen) - 1.0f;
    nvDebugCheck(u >= 0.0f && u <= 1.0f);
    nvDebugCheck(v >= 0.0f && v <= 1.0f);

    Vector3 n;

    if (face == 0) {
        n.x = 1;
        n.y = -v;
        n.z = -u;
    }
    if (face == 1) {
        n.x = -1;
        n.y = -v;
        n.z = u;
    }

    if (face == 2) {
        n.x = u;
        n.y = 1;
        n.z = v;
    }
    if (face == 3) {
        n.x = u;
        n.y = -1;
        n.z = -v;
    }

    if (face == 4) {
        n.x = u;
        n.y = -v;
        n.z = 1;
    }
    if (face == 5) {
        n.x = -u;
        n.y = -v;
        n.z = -1;
    }

    return normalizeFast(n);
}

struct VectorTable {
    VectorTable(int edgeLength) : size(edgeLength) {
        float invEdgeLength = 1.0f / edgeLength;

        for (uint f = 0; f < 6; f++) {
            for (uint y = 0; y < size; y++) {
                for (uint x = 0; x < size; x++) {
                    data[(f * size + y) * size + x] = texelDirection(f, x, y, invEdgeLength);
                }
            }
        }
    }

    const Vector3 & lookup(uint f, uint x, uint y) {
        nvDebugCheck(f < 6 && x < size && y < size);
        return data[(f * size + y) * size + x];
    }

    int size;
    nv::Array<Vector3> data;
};


CubeSurface CubeSurface::cosinePowerFilter(int size, float cosinePower) const
{
    const uint edgeLength = m->size;

    // Allocate output cube.
    CubeSurface filteredCube;
    filteredCube.m->allocate(size);

    SolidAngleTable solidAngleTable(edgeLength);
    VectorTable vectorTable(edgeLength);

#if 1
    // Scatter approach.

    // For each texel of the input cube.
    // - Lookup our solid angle.
    // - Determine to what texels of the output cube we contribute.
    // - Add our contribution to the texels whose power is above threshold.

    for (uint f = 0; f < 6; f++) {
        const Surface & face = m->face[f];

        for (uint y = 0; y < edgeLength; y++) {
            for (uint x = 0; x < edgeLength; x++) {
                float solidAngle = solidAngleTable.lookup(x, y);
                float r = face.m->image->pixel(0, x, y, 0) * solidAngle;;
                float g = face.m->image->pixel(1, x, y, 0) * solidAngle;;
                float b = face.m->image->pixel(2, x, y, 0) * solidAngle;;

                Vector3 texelDir = texelDirection(f, x, y, edgeLength);

                for (uint ff = 0; ff < 6; ff++) {
                    FloatImage * filteredFace = filteredCube.m->face[ff].m->image;

                    for (uint yy = 0; yy < size; yy++) {
                        for (uint xx = 0; xx < size; xx++) {

                            Vector3 filterDir = texelDirection(ff, xx, yy, size);

                            float power = powf(saturate(dot(texelDir, filterDir)), cosinePower);

                            if (power > 0.01) {
                                filteredFace->pixel(0, xx, yy, 0) += r * power;
                                filteredFace->pixel(1, xx, yy, 0) += g * power;
                                filteredFace->pixel(2, xx, yy, 0) += b * power;
                                filteredFace->pixel(3, xx, yy, 0) += solidAngle * power;
                            }
                        }
                    }
                }
            }
        }
    }

    // Normalize contributions.
    for (uint f = 0; f < 6; f++) {
        FloatImage * filteredFace = filteredCube.m->face[f].m->image;

        for (int i = 0; i < size*size; i++) {
            float & r = filteredFace->pixel(0, i);
            float & g = filteredFace->pixel(1, i);
            float & b = filteredFace->pixel(2, i);
            float & sum = filteredFace->pixel(3, i);
            float isum = 1.0f / sum;
            r *= isum;
            g *= isum;
            b *= isum;
            sum = 1;
        }
    }

#else

    // Gather approach. This should be easier to parallelize, because there's no contention in the filtered output.

    // For each texel of the output cube.
    // - Determine what texels of the input cube contribute to it.
    // - Add weighted contributions. Normalize.

    // For each texel of the output cube. @@ Parallelize this loop.
    for (uint f = 0; f < 6; f++) {
        nvtt::Surface filteredFace = filteredCube.m->face[f];
        FloatImage * filteredImage = filteredFace.m->image;

        for (uint y = 0; y < size; y++) {
            for (uint x = 0; x < size; x++) {

                const Vector3 filterDir = texelDirection(f, x, y, size);

                Vector3 color(0);
                float sum = 0;

                // For each texel of the input cube.
                for (uint ff = 0; ff < 6; ff++) {
                    const Surface & inputFace = m->face[ff];
                    const FloatImage * inputImage = inputFace.m->image;

                    for (uint yy = 0; yy < edgeLength; yy++) {
                        for (uint xx = 0; xx < edgeLength; xx++) {

                            // @@ We should probably store solid angle and direction together.
                            Vector3 inputDir = vectorTable.lookup(ff, xx, yy);

                            float power = powf(saturate(dot(inputDir, filterDir)), cosinePower);

                            if (power > 0.01f) {    // @@ Adjustable threshold.
                                float solidAngle = solidAngleTable.lookup(xx, yy);
                                float contribution = solidAngle * power;

                                sum += contribution;

                                float r = inputImage->pixel(0, xx, yy, 0);
                                float g = inputImage->pixel(1, xx, yy, 0);
                                float b = inputImage->pixel(2, xx, yy, 0);

                                color.r += r * contribution;
                                color.g += g * contribution;
                                color.b += b * contribution;
                            }
                        }
                    }
                }

                color *= (1.0f / sum);

                filteredImage->pixel(0, x, y, 0) = color.x;
                filteredImage->pixel(1, x, y, 0) = color.y;
                filteredImage->pixel(2, x, y, 0) = color.z;
            }
        }
    }

#endif

    return filteredCube;
}


void CubeSurface::toLinear(float gamma)
{
    if (isNull()) return;

    detach();

    for (int i = 0; i < 6; i++) {
        m->face[i].toLinear(gamma);
    }
}

void CubeSurface::toGamma(float gamma)
{
    if (isNull()) return;

    detach();

    for (int i = 0; i < 6; i++) {
        m->face[i].toGamma(gamma);
    }
}

