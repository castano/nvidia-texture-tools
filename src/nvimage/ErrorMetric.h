
#include "nvimage.h"


namespace nv
{
    class FloatImage;

    float rmsColorError(const FloatImage * ref, const FloatImage * img, bool alphaWeight);
    float rmsAlphaError(const FloatImage * ref, const FloatImage * img);

    float cieLabError(const FloatImage * ref, const FloatImage * img);
    float cieLab94Error(const FloatImage * ref, const FloatImage * img);
    float spatialCieLabError(const FloatImage * ref, const FloatImage * img);

    float averageColorError(const FloatImage * ref, const FloatImage * img, bool alphaWeight);
    float averageAlphaError(const FloatImage * ref, const FloatImage * img);

    float averageAngularError(const FloatImage * img0, const FloatImage * img1);
    float rmsAngularError(const FloatImage * img0, const FloatImage * img1);

} // nv namespace
