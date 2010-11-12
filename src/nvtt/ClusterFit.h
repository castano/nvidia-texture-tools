/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2006 Ignacio Castano                      icastano@nvidia.com

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#ifndef NVTT_CLUSTERFIT_H
#define NVTT_CLUSTERFIT_H

#define NVTT_USE_SIMD 0

#include "nvmath/SimdVector.h"
#include "nvmath/Vector.h"

namespace nv {

    struct ColorSet;

    class ClusterFit
    {
    public:
	    ClusterFit();

	    void setColourSet(const ColorSet * set);
    	
        void setMetric(Vector4::Arg w);
	    float bestError() const;

	    bool compress3(Vector3 * start, Vector3 * end);
	    bool compress4(Vector3 * start, Vector3 * end);
    	
    private:

        uint count;
	    //ColorSet const* m_colours;

        Vector3 m_principle;

    #if NVTT_USE_SIMD
        SimdVector m_weighted[16];
	    SimdVector m_metric;
	    SimdVector m_metricSqr;
	    SimdVector m_xxsum;
	    SimdVector m_xsum;
	    SimdVector m_besterror;
    #else
	    Vector3 m_weighted[16];
	    float m_weights[16];
	    Vector3 m_metric;
	    Vector3 m_metricSqr;
	    Vector3 m_xxsum;
	    Vector3 m_xsum;
	    float m_wsum;
	    float m_besterror;
    #endif

	    int m_order[16];
    };

} // nv namespace

#endif // NVTT_CLUSTERFIT_H
