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

#include "CubeSurface.h"
#include "Surface.h"

#include "nvimage/DirectDrawSurface.h"

#include "nvmath/Vector.h"

#include "nvcore/Array.h"
#include "nvcore/StrLib.h"


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
    return m->edgeLength == 0;
}

int CubeSurface::edgeLength() const
{
    return m->edgeLength;
}

int CubeSurface::countMipmaps() const
{
    return nv::countMipmaps(m->edgeLength);
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


bool CubeSurface::load(const char * fileName, int mipmap)
{
    if (strcmp(Path::extension(fileName), ".dds") == 0) {
        nv::DirectDrawSurface dds(fileName);

        if (!dds.isValid()/* || !dds.isSupported()*/) {
            return false;
        }

        if (!dds.isTextureCube()) {
            return false;
        }

        // Make sure it's a valid cube.
        if (dds.header.width != dds.header.height) return false;
        //if ((dds.header.caps.caps2 & DDSCAPS2_CUBEMAP_ALL_FACES) != DDSCAPS2_CUBEMAP_ALL_FACES) return false;

        if (mipmap < 0) {
            mipmap = dds.mipmapCount() - 1 - mipmap;
        }
        if (mipmap < 0 || mipmap > toI32(dds.mipmapCount())) return false;
        

        nvtt::InputFormat inputFormat = nvtt::InputFormat_RGBA_16F;

        if (dds.header.hasDX10Header()) {
            if (dds.header.header10.dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) inputFormat = nvtt::InputFormat_RGBA_16F;
            else if (dds.header.header10.dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) inputFormat = nvtt::InputFormat_RGBA_32F;
            else return false;
        }
        else {
            if ((dds.header.pf.flags & DDPF_FOURCC) != 0) {
                if (dds.header.pf.fourcc == D3DFMT_A16B16G16R16F) inputFormat = nvtt::InputFormat_RGBA_16F;
                else if (dds.header.pf.fourcc == D3DFMT_A32B32G32R32F) inputFormat = nvtt::InputFormat_RGBA_32F;
                else return false;
            }
            else {
                if (dds.header.pf.bitcount == 32 /*&& ...*/) inputFormat = nvtt::InputFormat_BGRA_8UB;
                else return false;  // @@ Do pixel format conversions!
            }
        }
        
        uint edgeLength = dds.surfaceWidth(mipmap);
        uint size = dds.surfaceSize(mipmap);

        void * data = malloc(size);

        for (int f = 0; f < 6; f++) {
            dds.readSurface(f, mipmap, data, size);
            m->face[f].setImage(inputFormat, edgeLength, edgeLength, 1, data);
        }

        m->edgeLength = edgeLength;

        free(data);

        return true;
    }

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



// Solid angle of an axis aligned quad from (0,0,1) to (x,y,1)
// See: http://www.fizzmoll11.com/thesis/ for a derivation of this formula.
static float areaElement(float x, float y) {
    return atan2(x*y, sqrtf(x*x + y*y + 1));
}

// Solid angle of a hemicube texel.
static float solidAngleTerm(uint x, uint y, float inverseEdgeLength) {
    // Transform x,y to [-1, 1] range, offset by 0.5 to point to texel center.
    float u = (float(x) + 0.5f) * (2 * inverseEdgeLength) - 1.0f;
    float v = (float(y) + 0.5f) * (2 * inverseEdgeLength) - 1.0f;
    nvDebugCheck(u >= -1.0f && u <= 1.0f);
    nvDebugCheck(v >= -1.0f && v <= 1.0f);

#if 1   
    // Exact solid angle:
    float x0 = u - inverseEdgeLength;
    float y0 = v - inverseEdgeLength;
    float x1 = u + inverseEdgeLength;
    float y1 = v + inverseEdgeLength;
    float solidAngle = areaElement(x0, y0) - areaElement(x0, y1) - areaElement(x1, y0) + areaElement(x1, y1);
    nvDebugCheck(solidAngle > 0.0f);
    
    return solidAngle;
#else
    // This formula is equivalent, but not as precise.
    float pixel_area = nv::square(2.0f * inverseEdgeLength);
    float dist_square = 1.0f + nv::square(u) + nv::square(v);
    float cos_theta = 1.0f / sqrt(dist_square);
    float cos_theta_d2 = cos_theta / dist_square; // Funny this is just 1/dist^3 or cos(tetha)^3

    return pixel_area * cos_theta_d2;
#endif
}


// Small solid angle table that takes into account cube map symmetry.
SolidAngleTable::SolidAngleTable(uint edgeLength) : size(edgeLength/2) {
    // Allocate table.
    data.resize(size * size);

    // Init table.
    const float inverseEdgeLength = 1.0f / edgeLength;

    for (uint y = 0; y < size; y++) {
        for (uint x = 0; x < size; x++) {
            data[y * size + x] = solidAngleTerm(128+x, 128+y, inverseEdgeLength);
        }
    }
}

float SolidAngleTable::lookup(uint x, uint y) const {
    if (x >= size) x -= size;
    else if (x < size) x = size - x - 1;
    if (y >= size) y -= size;
    else if (y < size) y = size - y - 1;

    return data[y * size + x];
}


static Vector3 texelDirection(uint face, uint x, uint y, float inverseEdgeLength)
{
    // Transform x,y to [-1, 1] range, offset by 0.5 to point to texel center.
    float u = (float(x) + 0.5f) * (2 * inverseEdgeLength) - 1.0f;
    float v = (float(y) + 0.5f) * (2 * inverseEdgeLength) - 1.0f;
    nvDebugCheck(u >= -1.0f && u <= 1.0f);
    nvDebugCheck(v >= -1.0f && v <= 1.0f);

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


VectorTable::VectorTable(uint edgeLength) : size(edgeLength) {
    float invEdgeLength = 1.0f / edgeLength;

    data.resize(size*size*6);

    for (uint f = 0; f < 6; f++) {
        for (uint y = 0; y < size; y++) {
            for (uint x = 0; x < size; x++) {
                data[(f * size + y) * size + x] = texelDirection(f, x, y, invEdgeLength);
            }
        }
    }
}

const Vector3 & VectorTable::lookup(uint f, uint x, uint y) const {
    nvDebugCheck(f < 6 && x < size && y < size);
    return data[(f * size + y) * size + x];
}



// We want to find the alpha such that:
// cos(alpha)^cosinePower = epsilon
// That's: acos(epsilon^(1/cosinePower))

// We can cull texels in two different ways:
// - culling faces that do not touch the cone.
// - computing one rectangle per face, find intersection between cone and face.
// -

// Other speedups:
// - parallelize.
// - use ISPC?

static Vector3 faceNormals[6] = {
    Vector3(1, 0, 0),
    Vector3(-1, 0, 0),
    Vector3(0, 1, 0),
    Vector3(0, -1, 0),
    Vector3(0, 0, 1),
    Vector3(0, 0, -1),
};


// Convolve filter against this cube.
Vector3 CubeSurface::Private::applyCosinePowerFilter(const Vector3 & filterDir, float cosineConeAngle, float cosinePower)
{
    const float coneAngle = acos(cosineConeAngle);

    Vector3 color(0);
    float sum = 0;

    // For each texel of the input cube.
    for (uint f = 0; f < 6; f++) {

        // Test face cone agains filter cone.
        float cosineFaceAngle = dot(filterDir, faceNormals[f]);

        if (cosineFaceAngle > cos(coneAngle + atan(sqrt(2)))) { // @@ Simplify this with cos(a+b) = cos(a)cos(b) - sin(a)sin(b) formula?
            // Skip face.
            continue;
        }

        // @@ We could do a less conservative test and test the face frustum against the cone...

        // @@ Compute bounding box of cone intersection against face.
        // The intersection of the cone with the face is an elipse, we want the extents of that elipse.
        // Hmm... we could even rasterize an elipse! Sounds like FUN!
        uint x0 = 0, x1 = edgeLength-1;
        uint y0 = 0, y1 = edgeLength-1;

        const Surface & inputFace = face[f];
        const FloatImage * inputImage = inputFace.m->image;

        for (uint y = y0; y <= y1; y++) {
            for (uint x = x0; x <= x1; x++) {

                Vector3 dir = vectorTable->lookup(f, x, y);
                float cosineAngle = dot(dir, filterDir);

                if (cosineAngle > cosineConeAngle) {
                    float solidAngle = solidAngleTable->lookup(x, y);
                    float scale = powf(saturate(cosineAngle), cosinePower);
                    float contribution = solidAngle * scale;

                    sum += contribution;
                    color.x += contribution * inputImage->pixel(0, x, y, 0);
                    color.y += contribution * inputImage->pixel(1, x, y, 0);
                    color.z += contribution * inputImage->pixel(2, x, y, 0);
                }
            }
        }
    }

    color *= (1.0f / sum);

    return color;
}


CubeSurface CubeSurface::cosinePowerFilter(int size, float cosinePower) const
{
    const uint edgeLength = m->edgeLength;

    // Allocate output cube.
    CubeSurface filteredCube;
    filteredCube.m->allocate(size);

    // Store these tables along with the surface. Compute them only once!
    if (m->solidAngleTable == NULL) {
        m->solidAngleTable = new SolidAngleTable(edgeLength);
    }
    if (m->vectorTable == NULL) {
        m->vectorTable = new VectorTable(edgeLength);
    }

    const float threshold = 0.0001f;
    const float cosineConeAngle = pow(threshold, 1/cosinePower);
    //const float coneAngle = acos(cosineConeAngle);


#if 0
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

                Vector3 texelDir = texelDirection(f, x, y, 1.0f / edgeLength);

                for (uint ff = 0; ff < 6; ff++) {
                    FloatImage * filteredFace = filteredCube.m->face[ff].m->image;

                    for (uint yy = 0; yy < uint(size); yy++) {
                        for (uint xx = 0; xx < uint(size); xx++) {

                            Vector3 filterDir = texelDirection(ff, xx, yy, 1.0f / size);

                            float scale = powf(saturate(dot(texelDir, filterDir)), cosinePower);

                            if (scale > threshold) {
                                filteredFace->pixel(0, xx, yy, 0) += r * scale;
                                filteredFace->pixel(1, xx, yy, 0) += g * scale;
                                filteredFace->pixel(2, xx, yy, 0) += b * scale;
                                filteredFace->pixel(3, xx, yy, 0) += solidAngle * scale;
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

        for (uint y = 0; y < uint(size); y++) {
            for (uint x = 0; x < uint(size); x++) {

                const Vector3 filterDir = texelDirection(f, x, y, 1.0f / size);

                // Convolve filter against cube.
                Vector3 color = m->applyCosinePowerFilter(filterDir, cosineConeAngle, cosinePower);

                filteredImage->pixel(0, x, y, 0) = color.x;
                filteredImage->pixel(1, x, y, 0) = color.y;
                filteredImage->pixel(2, x, y, 0) = color.z;
            }
        }
    }

    /*int jobCount = 6 * size * size;
    for (int i = 0; i < jobCount; i++) {
        int f = i / (size * size);
        int idx = i % (size * size);
        int y = idx / size;
        int x = idx % size;

        nvtt::Surface filteredFace = filteredCube.m->face[f];
        FloatImage * filteredImage = filteredFace.m->image;

        const Vector3 filterDir = texelDirection(f, x, y, 1.0f / size);

        // Convolve filter against cube.
        Vector3 color = m->applyCosinePowerFilter(filterDir, coneAngle, cosinePower);

        filteredImage->pixel(0, idx) = color.x;
        filteredImage->pixel(1, idx) = color.y;
        filteredImage->pixel(2, idx) = color.z;
    }*/

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

