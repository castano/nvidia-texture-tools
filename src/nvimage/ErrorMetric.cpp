
#include "ErrorMetric.h"
#include "FloatImage.h"
#include "Filter.h"

#include "nvmath/Matrix.h"

#include <float.h> // FLT_MAX

using namespace nv;

float nv::rmsColorError(const FloatImage * img, const FloatImage * ref, bool alphaWeight)
{
    double mse = 0;

    if (img == NULL || ref == NULL || img->width() != ref->width() || img->height() != ref->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img->componentNum() == 4);
    nvDebugCheck(ref->componentNum() == 4);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++)
    {
        float r0 = img->pixel(i + count * 0);
        float g0 = img->pixel(i + count * 1);
        float b0 = img->pixel(i + count * 2);
        //float a0 = img->pixel(i + count * 3);
        float r1 = ref->pixel(i + count * 0);
        float g1 = ref->pixel(i + count * 1);
        float b1 = ref->pixel(i + count * 2);
        float a1 = ref->pixel(i + count * 3);

        float r = r0 - r1;
        float g = g0 - g1;
        float b = b0 - b1;
        //float a = a0 - a1;

        if (alphaWeight)
        {
            mse += r * r * a1;
            mse += g * g * a1;
            mse += b * b * a1;
        }
        else
        {
            mse += r * r;
            mse += g * g;
            mse += b * b;
        }
    }

    return float(sqrt(mse / count));
}

float nv::rmsAlphaError(const FloatImage * img, const FloatImage * ref)
{
    double mse = 0;

    if (img == NULL || ref == NULL || img->width() != ref->width() || img->height() != ref->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img->componentNum() == 4 && ref->componentNum() == 4);

    const uint count = img->width() * img->height();
    for (uint i = 0; i < count; i++)
    {
        float a0 = img->pixel(i + count * 3);
        float a1 = ref->pixel(i + count * 3);

        float a = a0 - a1;

        mse += a * a;
    }

    return float(sqrt(mse / count));
}

// Assumes input is in *linear* sRGB color space.
static Vector3 rgbToXyz(Vector3::Arg c)
{
    Vector3 xyz;
    xyz.x = 0.412453f * c.x + 0.357580f * c.y + 0.180423f * c.z;
    xyz.y = 0.212671f * c.x + 0.715160f * c.y + 0.072169f * c.z;
    xyz.z = 0.019334f * c.x + 0.119193f * c.y + 0.950227f * c.z;
    return xyz;
}

static Vector3 xyzToRgb(Vector3::Arg c)
{
    Vector3 rgb;
    rgb.x =  3.2404542f * c.x - 1.5371385f * c.y - 0.4985314f * c.z;
    rgb.y = -0.9692660f * c.x + 1.8760108f * c.y + 0.0415560f * c.z;
    rgb.z =  0.0556434f * c.x - 0.2040259f * c.y + 1.0572252f * c.z;
    return rgb;
}

static float toLinear(float f)
{
    return powf(f, 2.2f);
}

static float toGamma(float f)
{
    // @@ Use sRGB space?
    return powf(f, 1.0f/2.2f);
}

static Vector3 toLinear(Vector3::Arg c)
{
    return Vector3(toLinear(c.x), toLinear(c.y), toLinear(c.z));
}

static Vector3 toGamma(Vector3::Arg c)
{
    return Vector3(toGamma(c.x), toGamma(c.y), toGamma(c.z));
}

static float f(float t)
{
    const float epsilon = powf(6.0f/29.0f, 3);

    if (t < epsilon) {
        return powf(t, 1.0f/3.0f);
    }
    else {
        return 1.0f/3.0f * powf(29.0f/6.0f, 2) * t + 4.0f / 29.0f;
    }
}

static float finv(float t)
{
    const float epsilon = powf(6.0f/29.0f, 3);

    if (t > 6.0f / 29.0f) {
        return powf(t, 3.0f);
    }
    else {
        return 3.0f * powf(6.0f / 29.0f, 2) * (t - 4.0f / 29.0f);
    }
}


static Vector3 xyzToCieLab(Vector3::Arg c)
{
    // Normalized white point.
    const float Xn = 0.950456f;
    const float Yn = 1.0f;
    const float Zn = 1.088754;

    float Xr = c.x / Xn;
    float Yr = c.y / Yn;
    float Zr = c.z / Zn;

    float fx = f(Xr);
    float fy = f(Yr);
    float fz = f(Zr);

    float L = 116 * fx - 16;
    float a = 500 * (fx - fy);
    float b = 200 * (fy - fz);
}

static Vector3 rgbToCieLab(Vector3::Arg c)
{
    return xyzToCieLab(rgbToXyz(toLinear(c)));
}

static void rgbToCieLab(const FloatImage * rgbImage, FloatImage * LabImage)
{
    nvDebugCheck(rgbImage != NULL && LabImage != NULL);
    nvDebugCheck(rgbImage->width() == LabImage->width() && rgbImage->height() == LabImage->height());
    nvDebugCheck(rgbImage->componentNum() >= 3 && LabImage->componentNum() >= 3);

    const uint w = rgbImage->width();
    const uint h = LabImage->height();

    const float * R = rgbImage->channel(0);
    const float * G = rgbImage->channel(1);
    const float * B = rgbImage->channel(2);

    float * L = LabImage->channel(0);
    float * a = LabImage->channel(1);
    float * b = LabImage->channel(2);

    const uint count = w*h;
    for (uint i = 0; i < count; i++)
    {
        Vector3 Lab = rgbToCieLab(Vector3(R[i], G[i], B[i]));
        L[i] = Lab.x;
        a[i] = Lab.y;
        b[i] = Lab.z;
    }
}


// Assumes input images are in linear sRGB space.
float nv::cieLabError(const FloatImage * img0, const FloatImage * img1)
{
    if (img0 == NULL || img1 == NULL || img0->width() != img1->width() || img0->height() != img1->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img0->componentNum() == 4 && img0->componentNum() == 4);

    uint w = img0->width();
    uint h = img0->height();

    const float * r0 = img0->channel(0);
    const float * g0 = img0->channel(1);
    const float * b0 = img0->channel(2);

    const float * r1 = img1->channel(0);
    const float * g1 = img1->channel(1);
    const float * b1 = img1->channel(2);

    double error = 0.0f;

    const uint count = w*h;
    for (uint i = 0; i < count; i++)
    {
        Vector3 lab0 = rgbToCieLab(Vector3(r0[i], g0[i], b0[i]));
        Vector3 lab1 = rgbToCieLab(Vector3(r1[i], g1[i], b1[i]));

        // @@ Measure Delta E.
    }

    return float(error / count);
}

float nv::spatialCieLabError(const FloatImage * img0, const FloatImage * img1)
{
    if (img0 == NULL || img1 == NULL || img0->width() != img1->width() || img0->height() != img1->height()) {
        return FLT_MAX;
    }
    nvDebugCheck(img0->componentNum() == 4 && img0->componentNum() == 4);

    uint w = img0->width();
    uint h = img0->height();

    FloatImage lab0, lab1; // Original images in CIE-Lab space.
    lab0.allocate(3, w, h);
    lab1.allocate(3, w, h);

    // Convert input images to CIE-Lab.
    rgbToCieLab(img0, &lab0);
    rgbToCieLab(img1, &lab1);

    // @@ Convolve each channel by the corresponding filter.
    /*
    GaussianFilter LFilter(5);
    GaussianFilter aFilter(5);
    GaussianFilter bFilter(5);

    lab0.convolve(0, LFilter);
    lab0.convolve(1, aFilter);
    lab0.convolve(2, bFilter);

    lab1.convolve(0, LFilter);
    lab1.convolve(1, aFilter);
    lab1.convolve(2, bFilter);
    */
    // @@ Measure Delta E between lab0 and lab1.

}




