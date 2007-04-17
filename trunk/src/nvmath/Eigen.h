// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_EIGEN_H
#define NV_MATH_EIGEN_H

#include <nvcore/Containers.h> // swap
#include <nvmath/nvmath.h>
#include <nvmath/Vector.h>

namespace nv
{

	// Compute first eigen vector using the power method.
	Vector3 firstEigenVector(float matrix[6]);
	
	/// Generic eigen-solver.
	class Eigen
	{
	public:
		
		/// Ctor.
		Eigen(uint n) : N(n)
		{
			uint size = n * (n + 1) / 2;
			matrix = new float[size];
			eigen_vec = new float[N*N];
			eigen_val = new float[N];
		}
		
		/// Dtor.
		~Eigen()
		{
			delete [] matrix;
			delete [] eigen_vec;
			delete [] eigen_val;
		}
		
		NVMATH_API void solve();
		
		/// Matrix accesor.
		float & operator()(uint x, uint y)
		{
			if( x > y ) {
				swap(x, y);
			}
			return matrix[y * (y + 1) / 2 + x];
		}
		
		/// Matrix const accessor.
		float operator()(uint x, uint y) const
		{
			if( x > y ) {
				swap(x, y);
			}
			return matrix[y * (y + 1) / 2 + x];
		}
		
		Vector3 eigenVector3(uint i) const
		{
			nvCheck(3 == N);
			nvCheck(i < N);
			return Vector3(eigen_vec[i*N+0], eigen_vec[i*N+1], eigen_vec[i*N+2]);
		}
		
		Vector4 eigenVector4(uint i) const
		{
			nvCheck(4 == N);
			nvCheck(i < N);
			return Vector4(eigen_vec[i*N+0], eigen_vec[i*N+1], eigen_vec[i*N+2], eigen_vec[i*N+3]);
		}
		
		float eigenValue(uint i) const
		{
			nvCheck(i < N);
			return eigen_val[i];
		}
	
	private:
		const uint N;
		float * matrix;
		float * eigen_vec;
		float * eigen_val;
	};
	
	
	/// 3x3 eigen-solver. 
	/// Based on Eric Lengyel's code:
	/// http://www.terathon.com/code/linear.html
	class Eigen3
	{
	public:
		
		/** Ctor. */
		Eigen3() {}
		
		NVMATH_API void solve();
		
		/// Matrix accesor.
		float & operator()(uint x, uint y)
		{
			nvDebugCheck( x < 3 && y < 3 );
			if( x > y ) {
				swap(x, y);
			}
			return matrix[y * (y + 1) / 2 + x];
		}
		
		/// Matrix const accessor.
		float operator()(uint x, uint y) const
		{
			nvDebugCheck( x < 3 && y < 3 );
			if( x > y ) {
				swap(x, y);
			}
			return matrix[y * (y + 1) / 2 + x];
		}
		
		/// Get ith eigen vector.
		Vector3 eigenVector(uint i) const
		{
			nvCheck(i < 3);
			return eigen_vec[i];
		}
		
		/** Get ith eigen value. */
		float eigenValue(uint i) const
		{
			nvCheck(i < 3);
			return eigen_val[i];
		}
	
	private:
		float matrix[3+2+1];
		Vector3 eigen_vec[3];
		float eigen_val[3];
	};

} // nv namespace

#endif // NV_MATH_EIGEN_H
