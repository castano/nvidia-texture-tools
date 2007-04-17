// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "CudaMath.h"

#define THREAD_NUM 64		// Number of threads per block.

#if __DEVICE_EMULATION__
#define __debugsync() __syncthreads()
#else
#define __debugsync()
#endif

typedef unsigned short ushort;
typedef unsigned int uint;

template <class T> 
__device__ inline void swap(T & a, T & b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

__constant__ float3 kColorMetric = { 1.0f, 1.0f, 1.0f };


////////////////////////////////////////////////////////////////////////////////
// Round color to RGB565 and expand
////////////////////////////////////////////////////////////////////////////////
inline __device__ float3 roundAndExpand(float3 v, ushort * w)
{
    v.x = rintf(__saturatef(v.x) * 31.0f);
    v.y = rintf(__saturatef(v.y) * 63.0f);
    v.z = rintf(__saturatef(v.z) * 31.0f);
    *w = ((ushort)v.x << 11) | ((ushort)v.y << 5) | (ushort)v.z;
    v.x *= 0.03227752766457f; // approximate integer bit expansion.
    v.y *= 0.01583151765563f;
    v.z *= 0.03227752766457f;
    return v;
}


////////////////////////////////////////////////////////////////////////////////
// Evaluate permutations
////////////////////////////////////////////////////////////////////////////////
static __device__ float evalPermutation4(const float3 * colors, uint permutation, ushort * start, ushort * end)
{
    // Compute endpoints using least squares.
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);
    float3 betax_sum = make_float3(0.0f, 0.0f, 0.0f);

    // Compute alpha & beta for this permutation.
    for (int i = 0; i < 16; i++)
    {
        const uint bits = permutation >> (2*i);

        float beta = (bits & 1);
        if (bits & 2) beta = (1 + beta) / 3.0f;
        float alpha = 1.0f - beta;
    
        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i];
        betax_sum += beta * colors[i];
    }

    // alpha2, beta2, alphabeta and factor could be precomputed for each permutation, but it's faster to recompute them.
    const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

    float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
    float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
    
    // Round a, b to the closest 5-6-5 color and expand...
    a = roundAndExpand(a, start);
    b = roundAndExpand(b, end);

    // compute the error
    float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

    return dot(e, kColorMetric);
}


static __device__ float evalPermutation3(const float3 * colors, uint permutation, ushort * start, ushort * end)
{
    // Compute endpoints using least squares.
    float alpha2_sum = 0.0f;
    float beta2_sum = 0.0f;
    float alphabeta_sum = 0.0f;
    float3 alphax_sum = make_float3(0.0f, 0.0f, 0.0f);
    float3 betax_sum = make_float3(0.0f, 0.0f, 0.0f);

    // Compute alpha & beta for this permutation.
    for (int i = 0; i < 16; i++)
    {
        const uint bits = permutation >> (2*i);

        float beta = (bits & 1);
        if (bits & 2) beta = 0.5f;
        float alpha = 1.0f - beta;
    
        alpha2_sum += alpha * alpha;
        beta2_sum += beta * beta;
        alphabeta_sum += alpha * beta;
        alphax_sum += alpha * colors[i];
        betax_sum += beta * colors[i];
    }

    const float factor = 1.0f / (alpha2_sum * beta2_sum - alphabeta_sum * alphabeta_sum);

    float3 a = (alphax_sum * beta2_sum - betax_sum * alphabeta_sum) * factor;
    float3 b = (betax_sum * alpha2_sum - alphax_sum * alphabeta_sum) * factor;
    
    // Round a, b to the closest 5-6-5 color and expand...
    a = roundAndExpand(a, start);
    b = roundAndExpand(b, end);

    // compute the error
    float3 e = a * a * alpha2_sum + b * b * beta2_sum + 2.0f * (a * b * alphabeta_sum - a * alphax_sum - b * betax_sum);

    return dot(e, kColorMetric);
}


////////////////////////////////////////////////////////////////////////////////
// Sort colors
////////////////////////////////////////////////////////////////////////////////
__device__ void sortColors(float * values, float3 * colors, int * xrefs)
{
#if __DEVICE_EMULATION__

    if (threadIdx.x == 0) 
    {
        for( int i = 0; i < 16; ++i )
        {
			xrefs[i] = i;
		}
        
        // Use a sequential sort on emulation.
        for( int i = 0; i < 16; ++i )
        {
            for( int j = i; j > 0 && values[j] < values[j - 1]; --j )
            {
                swap( values[j], values[j - 1] );
                swap( xrefs[j], xrefs[j - 1] );
            //    swap( colors[j], colors[j - 1] );
            }
        }
        
        float3 tmp[16];
        for( int i = 0; i < 16; ++i ) 
        {
			tmp[i] = colors[i];
		}
        
        for( int i = 0; i < 16; ++i )
        {
            int xid = xrefs[i];
            colors[i] = tmp[xid];
        }
    }

#else
    int tid = threadIdx.x;

	xrefs[tid] = tid;

    // Parallel bitonic sort.
    for (int k = 2; k <= 16; k *= 2)
    {
        // bitonic merge:
        for (int j = k / 2; j>0; j /= 2)
        {
            int ixj = tid ^ j;
            
            if (ixj > tid) {
                // @@ Optimize these branches.
                if ((tid & k) == 0) {
                    if (values[xrefs[tid]] > values[xrefs[ixj]]) {
                    //    swap(values[tid], values[ixj]);
                        swap(colors[tid], colors[ixj]);
                        swap(xrefs[tid], xrefs[ixj]);
                    }
                }
                else {
                    if (values[xrefs[tid]] < values[xrefs[ixj]]) {
                    //    swap(values[tid], values[ixj]);
                        swap(colors[tid], colors[ixj]);
                        swap(xrefs[tid], xrefs[ixj]);
                    }
                }
            }
        }
    }
#endif

    // It would be faster to avoid color swaps during the sort, but there
    // are compiler bugs preventing that.
#if 0
	float3 tmp = colors[xrefs[tid]];
    colors[tid] = tmp;
#endif
}

// This sort is faster, but does not sort correctly elements with the same value.
__device__ void sortColors2(float * values, float3 * colors, int * cmp)
{
	int tid = threadIdx.x;

	cmp[tid] = (values[0] < values[tid]);
	cmp[tid] += (values[1] < values[tid]);
	cmp[tid] += (values[2] < values[tid]);
	cmp[tid] += (values[3] < values[tid]);
	cmp[tid] += (values[4] < values[tid]);
	cmp[tid] += (values[5] < values[tid]);
	cmp[tid] += (values[6] < values[tid]);
	cmp[tid] += (values[7] < values[tid]);
	cmp[tid] += (values[8] < values[tid]);
	cmp[tid] += (values[9] < values[tid]);
	cmp[tid] += (values[10] < values[tid]);
	cmp[tid] += (values[11] < values[tid]);
	cmp[tid] += (values[12] < values[tid]);
	cmp[tid] += (values[13] < values[tid]);
	cmp[tid] += (values[14] < values[tid]);
	cmp[tid] += (values[15] < values[tid]);
	
	float3 tmp = colors[tid];
	colors[cmp[tid]] = tmp;
}



////////////////////////////////////////////////////////////////////////////////
// Find index with minimum error
////////////////////////////////////////////////////////////////////////////////
__device__ void minimizeError(float * errors, int * indices)
{
	const int idx = threadIdx.x;

#if __DEVICE_EMULATION__

	for(int d = THREAD_NUM/2; d > 0; d >>= 1)
	{
		__syncthreads();

		if (idx < d)
		{
			float err0 = errors[idx];
			float err1 = errors[idx + d];
			
			if (err1 < err0) {
				errors[idx] = err1;
				indices[idx] = indices[idx + d];
			}
		}
	}

#else

	for(int d = THREAD_NUM/2; d > 32; d >>= 1)
	{
		__syncthreads();

		if (idx < d)
		{
			float err0 = errors[idx];
			float err1 = errors[idx + d];
			
			if (err1 < err0) {
				errors[idx] = err1;
				indices[idx] = indices[idx + d];
			}
		}
	}

	// unroll last 6 steps 
	if (idx <= 32)
	{
		if (errors[idx + 32] < errors[idx]) {
			errors[idx] = errors[idx + 32];
			indices[idx] = indices[idx + 32];
		}
		if (errors[idx + 16] < errors[idx]) {
			errors[idx] = errors[idx + 16];
			indices[idx] = indices[idx + 16];
		}
		if (errors[idx + 8] < errors[idx]) {
			errors[idx] = errors[idx + 8];
			indices[idx] = indices[idx + 8];
		}
		if (errors[idx + 4] < errors[idx]) {
			errors[idx] = errors[idx + 4];
			indices[idx] = indices[idx + 4];
		}
		if (errors[idx + 2] < errors[idx]) {
			errors[idx] = errors[idx + 2];
			indices[idx] = indices[idx + 2];
		}
		if (errors[idx + 1] < errors[idx]) {
			errors[idx] = errors[idx + 1];
			indices[idx] = indices[idx + 1];
		}
	}
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Compress color block
////////////////////////////////////////////////////////////////////////////////
__global__ void compress(const uint * permutations, const uint * image, uint * result)
{
	const int bid = blockIdx.x;
	const int idx = threadIdx.x;
	
	__shared__ float3 colors[16];
	__shared__ float dps[16];
	__shared__ int xrefs[16];
	
	if (idx < 16)
	{
		// Read color.
		uint c = image[(bid) * 16 + idx];
	
		// No need to synchronize, 16 < warp size.
#if __DEVICE_EMULATION__
		} __debugsync(); if (idx < 16) {
#endif
		
		// Copy color to shared mem.
		colors[idx].z = ((c >> 0) & 0xFF) * (1.0f / 255.0f);
		colors[idx].y = ((c >> 8) & 0xFF) * (1.0f / 255.0f);
		colors[idx].x = ((c >> 16) & 0xFF) * (1.0f / 255.0f);
		
#if __DEVICE_EMULATION__
		} __debugsync(); if (idx < 16) {
#endif

		// Sort colors along the best fit line.
		float3 axis = bestFitLine(colors);
		
		dps[idx] = dot(colors[idx], axis);
		
#if __DEVICE_EMULATION__
		} __debugsync(); if (idx < 16) {
#endif
		
		sortColors(dps, colors, xrefs);
	}
	
	ushort bestStart, bestEnd;
	uint bestPermutation;
	float bestError = FLT_MAX;
	
	__syncthreads();
	
	for(int i = 0; i < 16; i++)
	{
		if (i == 15 && idx >= 32) break;
		
		ushort start, end;
		uint permutation = permutations[idx + THREAD_NUM * i];
		float error = evalPermutation4(colors, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
		}
	}

	if (bestStart < bestEnd)
	{
		swap(bestEnd, bestStart);
		bestPermutation ^= 0x55555555;	// Flip indices.
	}

	for(int i = 0; i < 3; i++)
	{
		if (i == 2 && idx >= 32) break;
		
		ushort start, end;
		uint permutation = permutations[idx + THREAD_NUM * i];
		float error = evalPermutation3(colors, permutation, &start, &end);
		
		if (error < bestError)
		{
			bestError = error;
			bestPermutation = permutation;
			bestStart = start;
			bestEnd = end;
			
			if (bestStart > bestEnd)
			{
				swap(bestEnd, bestStart);
				bestPermutation ^= (~bestPermutation >> 1) & 0x55555555;	// Flip indices.
			}
		}
	}
	
	if (bestStart == bestEnd)
	{
		bestPermutation = 0;
	}
	
	__syncthreads();
	
	// Use a parallel reduction to find minimum error.
	__shared__ float errors[THREAD_NUM];
	__shared__ int indices[THREAD_NUM];
	
	errors[idx] = bestError;
	indices[idx] = idx;
	
	minimizeError(errors, indices);
	
	__syncthreads();
	
	// Only write the result of the winner thread.
	if (idx == indices[0])
	{
		// Reorder permutation.
		uint perm = 0;
		for(int i = 0; i < 16; i++)
		{
			int ref = xrefs[i];
			perm |= ((bestPermutation >> (2 * i)) & 3) << (2 * ref);
		}
		
		// Write endpoints. (bestStart, bestEnd)
		result[2 * bid + 0] = (bestEnd << 16) | bestStart;
		
		// Write palette indices (permutation).
		result[2 * bid + 1] = perm;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Launch kernel
////////////////////////////////////////////////////////////////////////////////
extern "C" void compressKernel(uint blockNum, uint * d_data, uint * d_result, uint * d_bitmaps, float weights[3])
{
	// Set constants.
	cudaMemcpyToSymbol(kColorMetric, weights, sizeof(float) * 3, 0);

	compress<<<blockNum, THREAD_NUM>>>(d_bitmaps, d_data, d_result);
}

