
#include "nvimage.h"


namespace nv
{
    class FloatImage;

    float rmsColorError(const FloatImage * img, const FloatImage * ref, bool alphaWeight);
    float rmsAlphaError(const FloatImage * img, const FloatImage * ref);

    float cieLabError(const FloatImage * img, const FloatImage * ref);
    float cieLab94Error(const FloatImage * img, const FloatImage * ref);
    float spatialCieLabError(const FloatImage * img, const FloatImage * ref);

    float averageColorError(const FloatImage * img, const FloatImage * ref, bool alphaWeight);
    float averageAlphaError(const FloatImage * img, const FloatImage * ref);

    float averageAngularError(const FloatImage * img0, const FloatImage * img1);
    float rmsAngularError(const FloatImage * img0, const FloatImage * img1);

} // nv namespace
