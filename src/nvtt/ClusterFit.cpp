// MIT license see full LICENSE text at end of file

#include "ClusterFit.h"
#include "nvmath/Fitting.h"
#include "nvmath/Vector.inl"

#include <float.h> // FLT_MAX

using namespace nv;


void ClusterFit::setColorSet(const Vector3 * colors, const float * weights, int count)
{
    // initialise the best error
#if NVTT_USE_SIMD
    m_besterror = SimdVector( FLT_MAX );
    Vector3 metric = m_metric.toVector3();
#else
    m_besterror = FLT_MAX;
    Vector3 metric = m_metric;
#endif

    m_count = count;

    // I've tried using a lower quality approximation of the principal direction, but the best fit line seems to produce best results.
    Vector3 principal = Fit::computePrincipalComponent_PowerMethod(count, colors, weights, metric);
    //Vector3 principal = Fit::computePrincipalComponent_EigenSolver(count, colors, weights, metric);

    // build the list of values
    int order[16];
    float dps[16];
    for (uint i = 0; i < m_count; ++i)
    {
        dps[i] = dot(colors[i], principal);
        order[i] = i;
    }

    // stable sort
    for (uint i = 0; i < m_count; ++i)
    {
        for (uint j = i; j > 0 && dps[j] < dps[j - 1]; --j)
        {
            swap(dps[j], dps[j - 1]);
            swap(order[j], order[j - 1]);
        }
    }

    // weight all the points
#if NVTT_USE_SIMD
    m_xxsum = SimdVector( 0.0f );
    m_xsum = SimdVector( 0.0f );
#else
    m_xxsum = Vector3(0.0f);
    m_xsum = Vector3(0.0f);
    m_wsum = 0.0f;
#endif
	
    for (uint i = 0; i < m_count; ++i)
    {
        int p = order[i];
#if NVTT_USE_SIMD
        NV_ALIGN_16 Vector4 tmp(colors[p], 1);
        m_weighted[i] = SimdVector(tmp.component) * SimdVector(weights[p]);
        m_xxsum += m_weighted[i] * m_weighted[i];
        m_xsum += m_weighted[i];
#else
        m_weighted[i] = colors[p] * weights[p];
        m_xxsum += m_weighted[i] * m_weighted[i];
        m_xsum += m_weighted[i];
        m_weights[i] = weights[p];
        m_wsum += m_weights[i];
#endif
    }
}



void ClusterFit::setColorWeights(Vector4::Arg w)
{
#if NVTT_USE_SIMD
    NV_ALIGN_16 Vector4 tmp(w.xyz(), 1);
    m_metric = SimdVector(tmp.component);
#else
    m_metric = w.xyz();
#endif
    m_metricSqr = m_metric * m_metric;
}

float ClusterFit::bestError() const
{
#if NVTT_USE_SIMD
    SimdVector x = m_xxsum * m_metricSqr;
    SimdVector error = m_besterror + x.splatX() + x.splatY() + x.splatZ();
    return error.toFloat();
#else
    return m_besterror + dot(m_xxsum, m_metricSqr);
#endif

}

#if NVTT_USE_SIMD

bool ClusterFit::compress3( Vector3 * start, Vector3 * end )
{
    const int count = m_count;
    const SimdVector one = SimdVector(1.0f);
    const SimdVector zero = SimdVector(0.0f);
    const SimdVector half(0.5f, 0.5f, 0.5f, 0.25f);
    const SimdVector two = SimdVector(2.0);
    const SimdVector grid( 31.0f, 63.0f, 31.0f, 0.0f );
    const SimdVector gridrcp( 1.0f/31.0f, 1.0f/63.0f, 1.0f/31.0f, 0.0f );

    // declare variables
    SimdVector beststart = SimdVector( 0.0f );
    SimdVector bestend = SimdVector( 0.0f );
    SimdVector besterror = SimdVector( FLT_MAX );

    SimdVector x0 = zero;

    // check all possible clusters for this total order
    for (int c0 = 0; c0 <= count; c0++)
    {
        SimdVector x1 = zero;

        for (int c1 = 0; c1 <= count-c0; c1++)
        {
            const SimdVector x2 = m_xsum - x1 - x0;

            //Vector3 alphax_sum = x0 + x1 * 0.5f;
            //float alpha2_sum = w0 + w1 * 0.25f;
            const SimdVector alphax_sum = multiplyAdd(x1, half, x0); // alphax_sum, alpha2_sum
            const SimdVector alpha2_sum = alphax_sum.splatW();

            //const Vector3 betax_sum = x2 + x1 * 0.5f;
            //const float beta2_sum = w2 + w1 * 0.25f;
            const SimdVector betax_sum = multiplyAdd(x1, half, x2); // betax_sum, beta2_sum
            const SimdVector beta2_sum = betax_sum.splatW();

            //const float alphabeta_sum = w1 * 0.25f;
            const SimdVector alphabeta_sum = (x1 * half).splatW(); // alphabeta_sum

            // const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
            const SimdVector factor = reciprocal( negativeMultiplySubtract(alphabeta_sum, alphabeta_sum, alpha2_sum*beta2_sum) );

            SimdVector a = negativeMultiplySubtract(betax_sum, alphabeta_sum, alphax_sum*beta2_sum) * factor;
            SimdVector b = negativeMultiplySubtract(alphax_sum, alphabeta_sum, betax_sum*alpha2_sum) * factor;

            // clamp to the grid
            a = min( one, max( zero, a ) );
            b = min( one, max( zero, b ) );
            a = truncate( multiplyAdd( grid, a, half ) ) * gridrcp;
            b = truncate( multiplyAdd( grid, b, half ) ) * gridrcp;

            // compute the error (we skip the constant xxsum)
            SimdVector e1 = multiplyAdd( a*a, alpha2_sum, b*b*beta2_sum );
            SimdVector e2 = negativeMultiplySubtract( a, alphax_sum, a*b*alphabeta_sum );
            SimdVector e3 = negativeMultiplySubtract( b, betax_sum, e2 );
            SimdVector e4 = multiplyAdd( two, e3, e1 );

            // apply the metric to the error term
            SimdVector e5 = e4 * m_metricSqr;
            SimdVector error = e5.splatX() + e5.splatY() + e5.splatZ();

            // keep the solution if it wins
            if (compareAnyLessThan(error, besterror))
            {
                besterror = error;
                beststart = a;
                bestend = b;
            }

            x1 += m_weighted[c0+c1];
        }

        x0 += m_weighted[c0];
    }

    // save the block if necessary
    if (compareAnyLessThan(besterror, m_besterror))
    {
        *start = beststart.toVector3();
        *end = bestend.toVector3();

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::compress4( Vector3 * start, Vector3 * end )
{
    const int count = m_count;
    const SimdVector one = SimdVector(1.0f);
    const SimdVector zero = SimdVector(0.0f);
    const SimdVector half = SimdVector(0.5f);
    const SimdVector two = SimdVector(2.0);
    const SimdVector onethird( 1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f, 1.0f/9.0f );
    const SimdVector twothirds( 2.0f/3.0f, 2.0f/3.0f, 2.0f/3.0f, 4.0f/9.0f );
    const SimdVector twonineths = SimdVector( 2.0f/9.0f );
    const SimdVector grid( 31.0f, 63.0f, 31.0f, 0.0f );
    const SimdVector gridrcp( 1.0f/31.0f, 1.0f/63.0f, 1.0f/31.0f, 0.0f );

    // declare variables
    SimdVector beststart = SimdVector( 0.0f );
    SimdVector bestend = SimdVector( 0.0f );
    SimdVector besterror = SimdVector( FLT_MAX );

    SimdVector x0 = zero;

    // check all possible clusters for this total order
    for (int c0 = 0; c0 <= count; c0++)
    {
        SimdVector x1 = zero;

        for (int c1 = 0; c1 <= count-c0; c1++)
        {
            SimdVector x2 = zero;

            for (int c2 = 0; c2 <= count-c0-c1; c2++)
            {
                const SimdVector x3 = m_xsum - x2 - x1 - x0;

                //const Vector3 alphax_sum = x0 + x1 * (2.0f / 3.0f) + x2 * (1.0f / 3.0f);
                //const float alpha2_sum = w0 + w1 * (4.0f/9.0f) + w2 * (1.0f/9.0f);
                const SimdVector alphax_sum = multiplyAdd(x2, onethird, multiplyAdd(x1, twothirds, x0)); // alphax_sum, alpha2_sum
                const SimdVector alpha2_sum = alphax_sum.splatW();

                //const Vector3 betax_sum = x3 + x2 * (2.0f / 3.0f) + x1 * (1.0f / 3.0f);
                //const float beta2_sum = w3 + w2 * (4.0f/9.0f) + w1 * (1.0f/9.0f);
                const SimdVector betax_sum = multiplyAdd(x2, twothirds, multiplyAdd(x1, onethird, x3)); // betax_sum, beta2_sum
                const SimdVector beta2_sum = betax_sum.splatW();

                //const float alphabeta_sum = (w1 + w2) * (2.0f/9.0f);
                const SimdVector alphabeta_sum = twonineths*( x1 + x2 ).splatW(); // alphabeta_sum

                //const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);
                const SimdVector factor = reciprocal( negativeMultiplySubtract(alphabeta_sum, alphabeta_sum, alpha2_sum*beta2_sum) );

                SimdVector a = negativeMultiplySubtract(betax_sum, alphabeta_sum, alphax_sum*beta2_sum) * factor;
                SimdVector b = negativeMultiplySubtract(alphax_sum, alphabeta_sum, betax_sum*alpha2_sum) * factor;

                // clamp to the grid
                a = min( one, max( zero, a ) );
                b = min( one, max( zero, b ) );
                a = truncate( multiplyAdd( grid, a, half ) ) * gridrcp;
                b = truncate( multiplyAdd( grid, b, half ) ) * gridrcp;

                // compute the error (we skip the constant xxsum)
                // error = a*a*alpha2_sum + b*b*beta2_sum + 2.0f*( a*b*alphabeta_sum - a*alphax_sum - b*betax_sum );
                SimdVector e1 = multiplyAdd( a*a, alpha2_sum, b*b*beta2_sum );
                SimdVector e2 = negativeMultiplySubtract( a, alphax_sum, a*b*alphabeta_sum );
                SimdVector e3 = negativeMultiplySubtract( b, betax_sum, e2 );
                SimdVector e4 = multiplyAdd( two, e3, e1 );

                // apply the metric to the error term
                SimdVector e5 = e4 * m_metricSqr;
                SimdVector error = e5.splatX() + e5.splatY() + e5.splatZ();

                // keep the solution if it wins
                if (compareAnyLessThan(error, besterror))
                {
                    besterror = error;
                    beststart = a;
                    bestend = b;
                }

                x2 += m_weighted[c0+c1+c2];
            }

            x1 += m_weighted[c0+c1];
        }

        x0 += m_weighted[c0];
    }

    // save the block if necessary
    if (compareAnyLessThan(besterror, m_besterror))
    {
        *start = beststart.toVector3();
        *end = bestend.toVector3();

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

#else

static const float midpoints5[32] = {
    0.015686f, 0.047059f, 0.078431f, 0.111765f, 0.145098f, 0.176471f, 0.207843f, 0.241176f, 0.274510f, 0.305882f, 0.337255f, 0.370588f, 0.403922f, 0.435294f, 0.466667f, 0.5f,
    0.533333f, 0.564706f, 0.596078f, 0.629412f, 0.662745f, 0.694118f, 0.725490f, 0.758824f, 0.792157f, 0.823529f, 0.854902f, 0.888235f, 0.921569f, 0.952941f, 0.984314f, 1.0f
};

static const float midpoints6[64] = {
    0.007843f, 0.023529f, 0.039216f, 0.054902f, 0.070588f, 0.086275f, 0.101961f, 0.117647f, 0.133333f, 0.149020f, 0.164706f, 0.180392f, 0.196078f, 0.211765f, 0.227451f, 0.245098f,
    0.262745f, 0.278431f, 0.294118f, 0.309804f, 0.325490f, 0.341176f, 0.356863f, 0.372549f, 0.388235f, 0.403922f, 0.419608f, 0.435294f, 0.450980f, 0.466667f, 0.482353f, 0.500000f,
    0.517647f, 0.533333f, 0.549020f, 0.564706f, 0.580392f, 0.596078f, 0.611765f, 0.627451f, 0.643137f, 0.658824f, 0.674510f, 0.690196f, 0.705882f, 0.721569f, 0.737255f, 0.754902f,
    0.772549f, 0.788235f, 0.803922f, 0.819608f, 0.835294f, 0.850980f, 0.866667f, 0.882353f, 0.898039f, 0.913725f, 0.929412f, 0.945098f, 0.960784f, 0.976471f, 0.992157f, 1.0f
};

// This is the ideal way to round, but it's too expensive to do this in the inner loop.
inline Vector3 round565(const Vector3 & v) {
    const Vector3 grid(31.0f, 63.0f, 31.0f);
    const Vector3 gridrcp(1.0f / 31.0f, 1.0f / 63.0f, 1.0f / 31.0f);

    Vector3 q = floor(grid * v);
    q.x += (v.x > midpoints5[int(q.x)]);
    q.y += (v.y > midpoints6[int(q.y)]);
    q.z += (v.z > midpoints5[int(q.z)]);
    q *= gridrcp;
    return q;
}

bool ClusterFit::compress3(Vector3 * start, Vector3 * end)
{
    const uint count = m_count;
    const Vector3 grid( 31.0f, 63.0f, 31.0f );
    const Vector3 gridrcp( 1.0f/31.0f, 1.0f/63.0f, 1.0f/31.0f );

    // declare variables
    Vector3 beststart( 0.0f );
    Vector3 bestend( 0.0f );
    float besterror = FLT_MAX;

    Vector3 x0(0.0f);
    float w0 = 0.0f;

    int b0 = 0, b1 = 0;

    // check all possible clusters for this total order
    for (uint c0 = 0; c0 <= count; c0++)
    {
        Vector3 x1(0.0f);
        float w1 = 0.0f;

        for (uint c1 = 0; c1 <= count-c0; c1++)
        {
            float w2 = m_wsum - w0 - w1;

            // These factors could be entirely precomputed.
            float const alpha2_sum = w0 + w1 * 0.25f;
            float const beta2_sum = w2 + w1 * 0.25f;
            float const alphabeta_sum = w1 * 0.25f;
            float const factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

            Vector3 const alphax_sum = x0 + x1 * 0.5f;
            Vector3 const betax_sum = m_xsum - alphax_sum;

            Vector3 a = (alphax_sum*beta2_sum - betax_sum*alphabeta_sum) * factor;
            Vector3 b = (betax_sum*alpha2_sum - alphax_sum*alphabeta_sum) * factor;

            // clamp to the grid
            a = clamp(a, 0, 1);
            b = clamp(b, 0, 1);
#if 1
            a = floor(grid * a + 0.5f) * gridrcp;
            b = floor(grid * b + 0.5f) * gridrcp;
#else
            a = round565(a);
            b = round565(b);
#endif

            // compute the error
            Vector3 e1 = a*a*alpha2_sum + b*b*beta2_sum + 2.0f*( a*b*alphabeta_sum - a*alphax_sum - b*betax_sum );

            // apply the metric to the error term
            float error = dot(e1, m_metricSqr);

            // keep the solution if it wins
            if (error < besterror)
            {
                besterror = error;
                beststart = a;
                bestend = b;
                b0 = c0;
                b1 = c1;
            }

            x1 += m_weighted[c0+c1];
            w1 += m_weights[c0+c1];
        }

        x0 += m_weighted[c0];
        w0 += m_weights[c0];
    }

    // save the block if necessary
    if (besterror < m_besterror)
    {

        *start = beststart;
        *end = bestend;

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

bool ClusterFit::compress4(Vector3 * start, Vector3 * end)
{
    const uint count = m_count;
    const Vector3 grid( 31.0f, 63.0f, 31.0f );
    const Vector3 gridrcp( 1.0f/31.0f, 1.0f/63.0f, 1.0f/31.0f );

    // declare variables
    Vector3 beststart( 0.0f );
    Vector3 bestend( 0.0f );
    float besterror = FLT_MAX;

    Vector3 x0(0.0f);
    float w0 = 0.0f;
    int b0 = 0, b1 = 0, b2 = 0;

    // check all possible clusters for this total order
    for (uint c0 = 0; c0 <= count; c0++)
    {
        Vector3 x1(0.0f);
        float w1 = 0.0f;

        for (uint c1 = 0; c1 <= count-c0; c1++)
        {
            Vector3 x2(0.0f);
            float w2 = 0.0f;

            for (uint c2 = 0; c2 <= count-c0-c1; c2++)
            {
                float w3 = m_wsum - w0 - w1 - w2;

                float const alpha2_sum = w0 + w1 * (4.0f/9.0f) + w2 * (1.0f/9.0f);
                float const beta2_sum = w3 + w2 * (4.0f/9.0f) + w1 * (1.0f/9.0f);
                float const alphabeta_sum = (w1 + w2) * (2.0f/9.0f);
                float const factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

                Vector3 const alphax_sum = x0 + x1 * (2.0f / 3.0f) + x2 * (1.0f / 3.0f);
                Vector3 const betax_sum = m_xsum - alphax_sum;

                Vector3 a = ( alphax_sum*beta2_sum - betax_sum*alphabeta_sum )*factor;
                Vector3 b = ( betax_sum*alpha2_sum - alphax_sum*alphabeta_sum )*factor;

                // clamp to the grid
                a = clamp(a, 0, 1);
                b = clamp(b, 0, 1);
#if 1
                a = floor(a * grid + 0.5f) * gridrcp;
                b = floor(b * grid + 0.5f) * gridrcp;
#else
                a = round565(a);
                b = round565(b);
#endif
                // @@ It would be much more accurate to evaluate the error exactly. 

                // compute the error
                Vector3 e1 = a*a*alpha2_sum + b*b*beta2_sum + 2.0f*( a*b*alphabeta_sum - a*alphax_sum - b*betax_sum );

                // apply the metric to the error term
                float error = dot( e1, m_metricSqr );

                // keep the solution if it wins
                if (error < besterror)
                {
                    besterror = error;
                    beststart = a;
                    bestend = b;
                    b0 = c0;
                    b1 = c1;
                    b2 = c2;
                }

                x2 += m_weighted[c0+c1+c2];
                w2 += m_weights[c0+c1+c2];
            }

            x1 += m_weighted[c0+c1];
            w1 += m_weights[c0+c1];
        }

        x0 += m_weighted[c0];
        w0 += m_weights[c0];
    }

    // save the block if necessary
    if (besterror < m_besterror)
    {
        *start = beststart;
        *end = bestend;

        // save the error
        m_besterror = besterror;

        return true;
    }

    return false;
}

#endif // NVTT_USE_SIMD

//  Copyright (c) 2006-2020 Ignacio Castano                 icastano@nvidia.com
//  Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to	deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
