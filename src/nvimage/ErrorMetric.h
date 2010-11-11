
#include "nvimage.h"


namespace nv
{
    class FloatImage;

    float rmsColorError(const FloatImage * img, const FloatImage * ref, bool alphaWeight);
    float rmsAlphaError(const FloatImage * img, const FloatImage * ref);

    float cieLabError(const FloatImage * img, const FloatImage * ref);
    float spatialCieLabError(const FloatImage * img, const FloatImage * ref);

} // nv namespace
