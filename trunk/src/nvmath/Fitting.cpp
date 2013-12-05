// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "Fitting.h"
#include "Vector.inl"
#include "Plane.inl"

#include "nvcore/Utils.h" // max, swap

#include <float.h> // FLT_MAX
#include <vector>

using namespace nv;

// @@ Move to EigenSolver.h

// @@ We should be able to do something cheaper...
static Vector3 estimatePrincipalComponent(const float * __restrict matrix)
{
	const Vector3 row0(matrix[0], matrix[1], matrix[2]);
	const Vector3 row1(matrix[1], matrix[3], matrix[4]);
	const Vector3 row2(matrix[2], matrix[4], matrix[5]);

	float r0 = lengthSquared(row0);
	float r1 = lengthSquared(row1);
	float r2 = lengthSquared(row2);

	if (r0 > r1 && r0 > r2) return row0;
	if (r1 > r2) return row1;
	return row2;
}


static inline Vector3 firstEigenVector_PowerMethod(const float *__restrict matrix)
{
    if (matrix[0] == 0 && matrix[3] == 0 && matrix[5] == 0)
    {
        return Vector3(0.0f);
    }

    Vector3 v = estimatePrincipalComponent(matrix);

    const int NUM = 8;
    for (int i = 0; i < NUM; i++)
    {
        float x = v.x * matrix[0] + v.y * matrix[1] + v.z * matrix[2];
        float y = v.x * matrix[1] + v.y * matrix[3] + v.z * matrix[4];
        float z = v.x * matrix[2] + v.y * matrix[4] + v.z * matrix[5];

        float norm = max(max(x, y), z);

        v = Vector3(x, y, z) / norm;
    }

    return v;
}


Vector3 nv::Fit::computeCentroid(int n, const Vector3 *__restrict points)
{
    Vector3 centroid(0.0f);

    for (int i = 0; i < n; i++)
    {
        centroid += points[i];
    }
    centroid /= float(n);

    return centroid;
}

Vector3 nv::Fit::computeCentroid(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric)
{
    Vector3 centroid(0.0f);
    float total = 0.0f;

    for (int i = 0; i < n; i++)
    {
        total += weights[i];
        centroid += weights[i]*points[i];
    }
    centroid /= total;

    return centroid;
}


Vector3 nv::Fit::computeCovariance(int n, const Vector3 *__restrict points, float *__restrict covariance)
{
    // compute the centroid
    Vector3 centroid = computeCentroid(n, points);

    // compute covariance matrix
    for (int i = 0; i < 6; i++)
    {
        covariance[i] = 0.0f;
    }

    for (int i = 0; i < n; i++)
    {
        Vector3 v = points[i] - centroid;

        covariance[0] += v.x * v.x;
        covariance[1] += v.x * v.y;
        covariance[2] += v.x * v.z;
        covariance[3] += v.y * v.y;
        covariance[4] += v.y * v.z;
        covariance[5] += v.z * v.z;
    }

    return centroid;
}

Vector3 nv::Fit::computeCovariance(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric, float *__restrict covariance)
{
    // compute the centroid
    Vector3 centroid = computeCentroid(n, points, weights, metric);

    // compute covariance matrix
    for (int i = 0; i < 6; i++)
    {
        covariance[i] = 0.0f;
    }

    for (int i = 0; i < n; i++)
    {
        Vector3 a = (points[i] - centroid) * metric;
        Vector3 b = weights[i]*a;

        covariance[0] += a.x * b.x;
        covariance[1] += a.x * b.y;
        covariance[2] += a.x * b.z;
        covariance[3] += a.y * b.y;
        covariance[4] += a.y * b.z;
        covariance[5] += a.z * b.z;
    }

    return centroid;
}

Vector3 nv::Fit::computePrincipalComponent_PowerMethod(int n, const Vector3 *__restrict points)
{
    float matrix[6];
    computeCovariance(n, points, matrix);

    return firstEigenVector_PowerMethod(matrix);
}

Vector3 nv::Fit::computePrincipalComponent_PowerMethod(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric)
{
    float matrix[6];
    computeCovariance(n, points, weights, metric, matrix);

    return firstEigenVector_PowerMethod(matrix);
}



static inline Vector3 firstEigenVector_EigenSolver(const float *__restrict matrix)
{
    if (matrix[0] == 0 && matrix[3] == 0 && matrix[5] == 0)
    {
        return Vector3(0.0f);
    }

    float eigenValues[3];
    Vector3 eigenVectors[3];
	if (!nv::Fit::eigenSolveSymmetric(matrix, eigenValues, eigenVectors))
	{
		return Vector3(0.0f);
	}

	return eigenVectors[0];
}

Vector3 nv::Fit::computePrincipalComponent_EigenSolver(int n, const Vector3 *__restrict points)
{
    float matrix[6];
    computeCovariance(n, points, matrix);

    return firstEigenVector_EigenSolver(matrix);
}

Vector3 nv::Fit::computePrincipalComponent_EigenSolver(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric)
{
    float matrix[6];
    computeCovariance(n, points, weights, metric, matrix);

    return firstEigenVector_EigenSolver(matrix);
}

void ArvoSVD(int rows, int cols, float * Q, float * diag, float * R);

Vector3 nv::Fit::computePrincipalComponent_SVD(int n, const Vector3 *__restrict points)
{
	// Store the points in an n x n matrix
	std::vector<float> Q(n*n, 0.0f);
	for (int i = 0; i < n; ++i)
	{
		Q[i*n+0] = points[i].x;
		Q[i*n+1] = points[i].y;
		Q[i*n+2] = points[i].z;
	}

	// Alloc space for the SVD outputs
	std::vector<float> diag(n, 0.0f);
	std::vector<float> R(n*n, 0.0f);

	ArvoSVD(n, n, &Q[0], &diag[0], &R[0]);

	// Get the principal component
	return Vector3(R[0], R[1], R[2]);
}



Plane nv::Fit::bestPlane(int n, const Vector3 *__restrict points)
{
    // compute the centroid and covariance
    float matrix[6];
    Vector3 centroid = computeCovariance(n, points, matrix);

    if (matrix[0] == 0 && matrix[3] == 0 && matrix[5] == 0)
    {
        // If no plane defined, then return a horizontal plane.
        return Plane(Vector3(0, 0, 1), centroid);
    }

    float eigenValues[3];
    Vector3 eigenVectors[3];
    if (!eigenSolveSymmetric(matrix, eigenValues, eigenVectors)) {
        // If no plane defined, then return a horizontal plane.
        return Plane(Vector3(0, 0, 1), centroid);
    }

    return Plane(eigenVectors[2], centroid);
}

bool nv::Fit::isPlanar(int n, const Vector3 * points, float epsilon/*=NV_EPSILON*/)
{
    // compute the centroid and covariance
    float matrix[6];
    computeCovariance(n, points, matrix);

    float eigenValues[3];
    Vector3 eigenVectors[3];
    if (!eigenSolveSymmetric(matrix, eigenValues, eigenVectors)) {
        return false;
    }

    return eigenValues[2] < epsilon;
}



// Tridiagonal solver from Charles Bloom. 
// Householder transforms followed by QL decomposition. 
// Seems to be based on the code from Numerical Recipes in C.

static void EigenSolver_Tridiagonal(float mat[3][3],float * diag,float * subd);
static bool EigenSolver_QLAlgorithm(float mat[3][3],float * diag,float * subd);

bool nv::Fit::eigenSolveSymmetric(const float matrix[6], float eigenValues[3], Vector3 eigenVectors[3])
{
    nvDebugCheck(matrix != NULL && eigenValues != NULL && eigenVectors != NULL);

    float subd[3];
    float diag[3];
    float work[3][3];

    work[0][0] = matrix[0];
    work[0][1] = work[1][0] = matrix[1];
    work[0][2] = work[2][0] = matrix[2];
    work[1][1] = matrix[3];
    work[1][2] = work[2][1] = matrix[4];
    work[2][2] = matrix[5];

    EigenSolver_Tridiagonal(work, diag, subd);
    if (!EigenSolver_QLAlgorithm(work, diag, subd))
    {
        for (int i = 0; i < 3; i++) {
            eigenValues[i] = 0;
            eigenVectors[i] = Vector3(0);
        }
        return false;
    }

    for (int i = 0; i < 3; i++) {
        eigenValues[i] = (float)diag[i];
    }

    // eigenvectors are the columns; make them the rows :

    for (int i=0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            eigenVectors[j].component[i] = (float) work[i][j];
        }
    }

    // shuffle to sort by singular value :
    if (eigenValues[2] > eigenValues[0] && eigenValues[2] > eigenValues[1])
    {
        swap(eigenValues[0], eigenValues[2]);
        swap(eigenVectors[0], eigenVectors[2]);
    }
    if (eigenValues[1] > eigenValues[0])
    {
        swap(eigenValues[0], eigenValues[1]);
        swap(eigenVectors[0], eigenVectors[1]);
    }
    if (eigenValues[2] > eigenValues[1])
    {
        swap(eigenValues[1], eigenValues[2]);
        swap(eigenVectors[1], eigenVectors[2]);
    }

    nvDebugCheck(eigenValues[0] >= eigenValues[1] && eigenValues[0] >= eigenValues[2]);
    nvDebugCheck(eigenValues[1] >= eigenValues[2]);

    return true;
}

static void EigenSolver_Tridiagonal(float mat[3][3],float * diag,float * subd)
{
    // Householder reduction T = Q^t M Q
    //   Input:   
    //     mat, symmetric 3x3 matrix M
    //   Output:  
    //     mat, orthogonal matrix Q
    //     diag, diagonal entries of T
    //     subd, subdiagonal entries of T (T is symmetric)
    const float epsilon = 1e-08f;

    float a = mat[0][0];
    float b = mat[0][1];
    float c = mat[0][2];
    float d = mat[1][1];
    float e = mat[1][2];
    float f = mat[2][2];

    diag[0] = a;
    subd[2] = 0.f;
    if ( fabs(c) >= epsilon )
    {
        const float ell = sqrtf(b*b+c*c);
        b /= ell;
        c /= ell;
        const float q = 2*b*e+c*(f-d);
        diag[1] = d+c*q;
        diag[2] = f-c*q;
        subd[0] = ell;
        subd[1] = e-b*q;
        mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0;
        mat[1][0] = 0; mat[1][1] = b; mat[1][2] = c;
        mat[2][0] = 0; mat[2][1] = c; mat[2][2] = -b;
    }
    else
    {
        diag[1] = d;
        diag[2] = f;
        subd[0] = b;
        subd[1] = e;
        mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0;
        mat[1][0] = 0; mat[1][1] = 1; mat[1][2] = 0;
        mat[2][0] = 0; mat[2][1] = 0; mat[2][2] = 1;
    }
}

static bool EigenSolver_QLAlgorithm(float mat[3][3],float * diag,float * subd)
{
    // QL iteration with implicit shifting to reduce matrix from tridiagonal
    // to diagonal
    const int maxiter = 32;

    for (int ell = 0; ell < 3; ell++)
    {
        int iter;
        for (iter = 0; iter < maxiter; iter++)
        {
            int m;
            for (m = ell; m <= 1; m++)
            {
                float dd = fabs(diag[m]) + fabs(diag[m+1]);
                if ( fabs(subd[m]) + dd == dd )
                    break;
            }
            if ( m == ell )
                break;

            float g = (diag[ell+1]-diag[ell])/(2*subd[ell]);
            float r = sqrtf(g*g+1);
            if ( g < 0 )
                g = diag[m]-diag[ell]+subd[ell]/(g-r);
            else
                g = diag[m]-diag[ell]+subd[ell]/(g+r);
            float s = 1, c = 1, p = 0;
            for (int i = m-1; i >= ell; i--)
            {
                float f = s*subd[i], b = c*subd[i];
                if ( fabs(f) >= fabs(g) )
                {
                    c = g/f;
                    r = sqrtf(c*c+1);
                    subd[i+1] = f*r;
                    c *= (s = 1/r);
                }
                else
                {
                    s = f/g;
                    r = sqrtf(s*s+1);
                    subd[i+1] = g*r;
                    s *= (c = 1/r);
                }
                g = diag[i+1]-p;
                r = (diag[i]-g)*s+2*b*c;
                p = s*r;
                diag[i+1] = g+p;
                g = c*r-b;

                for (int k = 0; k < 3; k++)
                {
                    f = mat[k][i+1];
                    mat[k][i+1] = s*mat[k][i]+c*f;
                    mat[k][i] = c*mat[k][i]-s*f;
                }
            }
            diag[ell] -= p;
            subd[ell] = g;
            subd[m] = 0;
        }

        if ( iter == maxiter )
            // should not get here under normal circumstances
            return false;
    }

    return true;
}




int nv::Fit::compute4Means(int n, const Vector3 *__restrict points, const float *__restrict weights, Vector3::Arg metric, Vector3 *__restrict cluster)
{
    // Compute principal component.
    float matrix[6];
    Vector3 centroid = computeCovariance(n, points, weights, metric, matrix);
    Vector3 principal = firstEigenVector_PowerMethod(matrix);

    // Pick initial solution.
    int mini, maxi;
    mini = maxi = 0;

    float mindps, maxdps;
    mindps = maxdps = dot(points[0] - centroid, principal);

    for (int i = 1; i < n; ++i)
    {
        float dps = dot(points[i] - centroid, principal);

        if (dps < mindps) {
            mindps = dps;
            mini = i;
        }
        else {
            maxdps = dps;
            maxi = i;
        }
    }

    cluster[0] = centroid + mindps * principal;
    cluster[1] = centroid + maxdps * principal;
    cluster[2] = (2.0f * cluster[0] + cluster[1]) / 3.0f;
    cluster[3] = (2.0f * cluster[1] + cluster[0]) / 3.0f;

    // Now we have to iteratively refine the clusters.
    while (true)
    {
        Vector3 newCluster[4] = { Vector3(0.0f), Vector3(0.0f), Vector3(0.0f), Vector3(0.0f) };
        float total[4] = {0, 0, 0, 0};

        for (int i = 0; i < n; ++i)
        {
            // Find nearest cluster.
            int nearest = 0;
            float mindist = FLT_MAX;
            for (int j = 0; j < 4; j++)
            {
                float dist = lengthSquared((cluster[j] - points[i]) * metric);
                if (dist < mindist)
                {
                    mindist = dist;
                    nearest = j;
                }
            }

            newCluster[nearest] += weights[i] * points[i];
            total[nearest] += weights[i];
        }

        for (int j = 0; j < 4; j++)
        {
            if (total[j] != 0)
                newCluster[j] /= total[j];
        }

        if (equal(cluster[0], newCluster[0]) && equal(cluster[1], newCluster[1]) && 
            equal(cluster[2], newCluster[2]) && equal(cluster[3], newCluster[3]))
        {
            return (total[0] != 0) + (total[1] != 0) + (total[2] != 0) + (total[3] != 0);
        }

        cluster[0] = newCluster[0];
        cluster[1] = newCluster[1];
        cluster[2] = newCluster[2];
        cluster[3] = newCluster[3];

        // Sort clusters by weight.
        for (int i = 0; i < 4; i++)
        {
            for (int j = i; j > 0 && total[j] > total[j - 1]; j--)
            {
                swap( total[j], total[j - 1] );
                swap( cluster[j], cluster[j - 1] );
            }
        }
    }
}



// Adaptation of James Arvo's SVD code, as found in ZOH.

inline float Sqr(float x) { return x*x; }

inline float svd_pythag( float a, float b )
{
	float at = fabsf(a);
	float bt = fabsf(b);
	if( at > bt )
		return at * sqrtf( 1.0f + Sqr( bt / at ) );
	else if( bt > 0.0f )
		return bt * sqrtf( 1.0f + Sqr( at / bt ) );
	else return 0.0f;
}

inline float SameSign( float a, float b ) 
{
	float t;
	if( b >= 0.0f ) t = fabsf( a );
	else t = -fabsf( a );
	return t;
}

void ArvoSVD(int rows, int cols, float * Q, float * diag, float * R)
{
	static const int MaxIterations = 30;

	int    i, j, k, l, p, q, iter;
	float  c, f, h, s, x, y, z;
	float  norm  = 0.0f;
	float  g     = 0.0f;
	float  scale = 0.0f;

	std::vector<float> temp(cols, 0.0f);

	for( i = 0; i < cols; i++ ) 
	{
		temp[i] = scale * g;
		scale   = 0.0f;
		g       = 0.0f;
		s       = 0.0f;
		l       = i + 1;

		if( i < rows )
		{
			for( k = i; k < rows; k++ ) scale += fabsf( Q[k*cols+i] );
			if( scale != 0.0f ) 
			{
				for( k = i; k < rows; k++ ) 
				{
					Q[k*cols+i] /= scale;
					s += Sqr( Q[k*cols+i] );
				}
				f = Q[i*cols+i];
				g = -SameSign( sqrtf(s), f );
				h = f * g - s;
				Q[i*cols+i] = f - g;
				if( i != cols - 1 )
				{
					for( j = l; j < cols; j++ ) 
					{
						s = 0.0f;
						for( k = i; k < rows; k++ ) s += Q[k*cols+i] * Q[k*cols+j];
						f = s / h;
						for( k = i; k < rows; k++ ) Q[k*cols+j] += f * Q[k*cols+i];
					}
				}
				for( k = i; k < rows; k++ ) Q[k*cols+i] *= scale;
			}
		}

		diag[i] = scale * g;
		g       = 0.0f;
		s       = 0.0f;
		scale   = 0.0f;

		if( i < rows && i != cols - 1 ) 
		{
			for( k = l; k < cols; k++ ) scale += fabsf( Q[i*cols+k] );
			if( scale != 0.0f ) 
			{
				for( k = l; k < cols; k++ ) 
				{
					Q[i*cols+k] /= scale;
					s += Sqr( Q[i*cols+k] );
				}
				f = Q[i*cols+l];
				g = -SameSign( sqrtf(s), f );
				h = f * g - s;
				Q[i*cols+l] = f - g;
				for( k = l; k < cols; k++ ) temp[k] = Q[i*cols+k] / h;
				if( i != rows - 1 ) 
				{
					for( j = l; j < rows; j++ ) 
					{
						s = 0.0f;
						for( k = l; k < cols; k++ ) s += Q[j*cols+k] * Q[i*cols+k];
						for( k = l; k < cols; k++ ) Q[j*cols+k] += s * temp[k];
					}
				}
				for( k = l; k < cols; k++ ) Q[i*cols+k] *= scale;
			}
		}
		norm = max( norm, fabsf( diag[i] ) + fabsf( temp[i] ) );
	}


	for( i = cols - 1; i >= 0; i-- ) 
	{
		if( i < cols - 1 ) 
		{
			if( g != 0.0f ) 
			{
				for( j = l; j < cols; j++ ) R[i*cols+j] = ( Q[i*cols+j] / Q[i*cols+l] ) / g;
				for( j = l; j < cols; j++ ) 
				{
					s = 0.0f;
					for( k = l; k < cols; k++ ) s += Q[i*cols+k] * R[j*cols+k];
					for( k = l; k < cols; k++ ) R[j*cols+k] += s * R[i*cols+k];
				}
			}
			for( j = l; j < cols; j++ ) 
			{
				R[i*cols+j] = 0.0f;
				R[j*cols+i] = 0.0f;
			}
		}
		R[i*cols+i] = 1.0f;
		g = temp[i];
		l = i;
	}


	for( i = cols - 1; i >= 0; i-- ) 
	{
		l = i + 1;
		g = diag[i];
		if( i < cols - 1 ) for( j = l; j < cols; j++ ) Q[i*cols+j] = 0.0f;
		if( g != 0.0f ) 
		{
			g = 1.0f / g;
			if( i != cols - 1 ) 
			{
				for( j = l; j < cols; j++ ) 
				{
					s = 0.0f;
					for( k = l; k < rows; k++ ) s += Q[k*cols+i] * Q[k*cols+j];
					f = ( s / Q[i*cols+i] ) * g;
					for( k = i; k < rows; k++ ) Q[k*cols+j] += f * Q[k*cols+i];
				}
			}
			for( j = i; j < rows; j++ ) Q[j*cols+i] *= g;
		} 
		else 
		{
			for( j = i; j < rows; j++ ) Q[j*cols+i] = 0.0f;
		}
		Q[i*cols+i] += 1.0f;
	}


	for( k = cols - 1; k >= 0; k-- ) 
	{
		for( iter = 1; iter <= MaxIterations; iter++ ) 
		{
			int jump;

			for( l = k; l >= 0; l-- )
			{
				q = l - 1;
				if( fabsf( temp[l] ) + norm == norm ) { jump = 1; break; }
				if( fabsf( diag[q] ) + norm == norm ) { jump = 0; break; }
			}

			if( !jump )
			{
				c = 0.0f;
				s = 1.0f;
				for( i = l; i <= k; i++ )
				{
					f = s * temp[i];
					temp[i] *= c;
					if( fabsf( f ) + norm == norm ) break;
					g = diag[i];
					h = svd_pythag( f, g );
					diag[i] = h;
					h = 1.0f / h;
					c = g * h;
					s = -f * h;
					for( j = 0; j < rows; j++ ) 
					{
						y = Q[j*cols+q];
						z = Q[j*cols+i];
						Q[j*cols+q] = y * c + z * s;
						Q[j*cols+i] = z * c - y * s;
					}
				}
			}

			z = diag[k];
			if( l == k ) 
			{
				if( z < 0.0f ) 
				{
					diag[k] = -z;
					for( j = 0; j < cols; j++ ) R[k*cols+j] *= -1.0f; 
				}
				break;
			}
			if( iter >= MaxIterations ) return;
			x = diag[l];
			q = k - 1;
			y = diag[q];
			g = temp[q];
			h = temp[k];
			f = ( ( y - z ) * ( y + z ) + ( g - h ) * ( g + h ) ) / ( 2.0f * h * y );
			g = svd_pythag( f, 1.0f );
			f = ( ( x - z ) * ( x + z ) + h * ( ( y / ( f + SameSign( g, f ) ) ) - h ) ) / x;
			c = 1.0f;
			s = 1.0f;
			for( j = l; j <= q; j++ ) 
			{
				i = j + 1;
				g = temp[i];
				y = diag[i];
				h = s * g;
				g = c * g;
				z = svd_pythag( f, h );
				temp[j] = z;
				c = f / z;
				s = h / z;
				f = x * c + g * s;
				g = g * c - x * s;
				h = y * s;
				y = y * c;
				for( p = 0; p < cols; p++ ) 
				{
					x = R[j*cols+p];
					z = R[i*cols+p];
					R[j*cols+p] = x * c + z * s;
					R[i*cols+p] = z * c - x * s;
				}
				z = svd_pythag( f, h );
				diag[j] = z;
				if( z != 0.0f ) 
				{
					z = 1.0f / z;
					c = f * z;
					s = h * z;
				}
				f = c * g + s * y;
				x = c * y - s * g;
				for( p = 0; p < rows; p++ ) 
				{
					y = Q[p*cols+j];
					z = Q[p*cols+i];
					Q[p*cols+j] = y * c + z * s;
					Q[p*cols+i] = z * c - y * s;
				}
			}
			temp[l] = 0.0f;
			temp[k] = f;
			diag[k] = x;
		}
	}

	// Sort the singular values into descending order.

	for( i = 0; i < cols - 1; i++ )
	{
		float biggest = diag[i];  // Biggest singular value so far.
		int   bindex  = i;        // The row/col it occurred in.
		for( j = i + 1; j < cols; j++ )
		{
			if( diag[j] > biggest ) 
			{
				biggest = diag[j];
				bindex  = j;
			}            
		}
		if( bindex != i )  // Need to swap rows and columns.
		{
			// Swap columns in Q.
			for (int j = 0; j < rows; ++j)
				swap(Q[j*cols+i], Q[j*cols+bindex]);

			// Swap rows in R.
			for (int j = 0; j < rows; ++j)
				swap(R[i*cols+j], R[bindex*cols+j]);

			// Swap elements in diag.
			swap(diag[i], diag[bindex]);
		}
	}
}
