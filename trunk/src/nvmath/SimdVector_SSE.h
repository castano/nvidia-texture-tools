/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

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
   
#ifndef NV_SIMD_VECTOR_SSE_H
#define NV_SIMD_VECTOR_SSE_H

#include <xmmintrin.h>
#if (NV_USE_SSE > 1)
#include <emmintrin.h>
#endif

namespace nv {

    class SimdVector
    {
        __m128 vec;

        typedef SimdVector const& Arg;

        SimdVector() {}
        explicit SimdVector(float f) : vec(_mm_set1_ps(f)) {}
        explicit SimdVector(__m128 v) : vec(v) {}
        SimdVector(const SimdVector & arg) : vec(arg.vec) {}

        SimdVector & operator=(const SimdVector & arg)
        {
            vec = arg.vec;
            return *this;
        }

        SimdVector(const float * v)
        {
            vec = _mm_load_ps( v );
        }

        SimdVector(float x, float y, float z, float w)
        {
            vec = _mm_setr_ps( x, y, z, w );
        }

        float toFloat() const 
        {
            NV_ALIGN_16 float f;
            _mm_store_ss(&f, vec);
            return f;
        }

        Vector3 toVector3() const
        {
            NV_ALIGN_16 float c[4];
            _mm_store_ps( c, vec );
            return Vector3( c[0], c[1], c[2] );
        }

        Vector4 toVector4() const
        {
            NV_ALIGN_16 float c[4];
            _mm_store_ps( v.components, vec );
            return Vector4( c[0], c[1], c[2], c[3] );
        }

#define SSE_SPLAT( a ) ((a) | ((a) << 2) | ((a) << 4) | ((a) << 6))
        SimdVector splatX() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 0 ) ) ); }
        SimdVector splatY() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 1 ) ) ); }
        SimdVector splatZ() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 2 ) ) ); }
        SimdVector splatW() const { return SimdVector( _mm_shuffle_ps( vec, vec, SSE_SPLAT( 3 ) ) ); }
#undef SSE_SPLAT

        SimdVector& operator+=( Arg v )
        {
            vec = _mm_add_ps( vec, v.vec );
            return *this;
        }

        SimdVector& operator-=( Arg v )
        {
            vec = _mm_sub_ps( vec, v.vec );
            return *this;
        }

        SimdVector& operator*=( Arg v )
        {
            vec = _mm_mul_ps( vec, v.vec );
            return *this;
        }
    };


    SimdVector operator+( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( _mm_add_ps( left.vec, right.vec ) );
    }

    SimdVector operator-( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( _mm_sub_ps( left.vec, right.vec ) );
    }

    SimdVector operator*( SimdVector::Arg left, SimdVector::Arg right  )
    {
        return SimdVector( _mm_mul_ps( left.vec, right.vec ) );
    }

    // Returns a*b + c
    SimdVector multiplyAdd( SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c )
    {
        return SimdVector( _mm_add_ps( _mm_mul_ps( a.vec, b.vec ), c.vec ) );
    }

    // Returns -( a*b - c )
    SimdVector negativeMultiplySubtract( SimdVector::Arg a, SimdVector::Arg b, SimdVector::Arg c )
    {
        return SimdVector( _mm_sub_ps( c.vec, _mm_mul_ps( a.vec, b.vec ) ) );
    }

    SimdVector reciprocal( SimdVector::Arg v )
    {
        // get the reciprocal estimate
        __m128 estimate = _mm_rcp_ps( v.vec );

        // one round of Newton-Rhaphson refinement
        __m128 diff = _mm_sub_ps( _mm_set1_ps( 1.0f ), _mm_mul_ps( estimate, v.vec ) );
        return SimdVector( _mm_add_ps( _mm_mul_ps( diff, estimate ), estimate ) );
    }

    SimdVector min( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( _mm_min_ps( left.vec, right.vec ) );
    }

    SimdVector max( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( _mm_max_ps( left.vec, right.vec ) );
    }

    SimdVector truncate( SimdVector::Arg v )
    {
#if (NV_USE_SSE == 1)
        // convert to ints
        __m128 input = v.vec;
        __m64 lo = _mm_cvttps_pi32( input );
        __m64 hi = _mm_cvttps_pi32( _mm_movehl_ps( input, input ) );

        // convert to floats
        __m128 part = _mm_movelh_ps( input, _mm_cvtpi32_ps( input, hi ) );
        __m128 truncated = _mm_cvtpi32_ps( part, lo );

        // clear out the MMX multimedia state to allow FP calls later
        _mm_empty(); 
        return SimdVector( truncated );
#else
        // use SSE2 instructions
        return SimdVector( _mm_cvtepi32_ps( _mm_cvttps_epi32( v.vec ) ) );
#endif
    }

    SimdVector compareEqual( SimdVector::Arg left, SimdVector::Arg right )
    {
        return SimdVector( _mm_cmpeq_ps( left.vec, right.vec ) );
    }

    SimdVector select( SimdVector::Arg off, SimdVector::Arg on, SimdVector::Arg bits )
    {
        __m128 a = _mm_andnot_ps( bits.vec, off.vec );
        __m128 b = _mm_and_ps( bits.vec, on.vec );

        return SimdVector( _mm_or_ps( a, b ) );
    }

    bool compareAnyLessThan( SimdVector::Arg left, SimdVector::Arg right ) 
    {
        __m128 bits = _mm_cmplt_ps( left.vec, right.vec );
        int value = _mm_movemask_ps( bits );
        return value != 0;
    }

} // namespace nv

#endif // NV_SIMD_VECTOR_SSE_H
