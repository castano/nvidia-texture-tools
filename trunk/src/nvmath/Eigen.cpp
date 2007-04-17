// This code is in the public domain -- castanyo@yahoo.es

#include "Eigen.h"

using namespace nv;

static const float EPS = 0.00001f;
static const int MAX_ITER = 100;

static void semi_definite_symmetric_eigen(const float *mat, int n, float *eigen_vec, float *eigen_val);


// Use power method to find the first eigenvector.
// http://www.miislita.com/information-retrieval-tutorial/matrix-tutorial-3-eigenvalues-eigenvectors.html
Vector3 nv::firstEigenVector(float matrix[6])
{
	// Number of iterations. @@ Use a variable number of iterations.
	const int NUM = 8;

	Vector3 v(1, 1, 1);
	for(int i = 0; i < NUM; i++) {
		float x = v.x() * matrix[0] + v.y() * matrix[1] + v.z() * matrix[2];
		float y = v.x() * matrix[1] + v.y() * matrix[3] + v.z() * matrix[4];
		float z = v.x() * matrix[2] + v.y() * matrix[4] + v.z() * matrix[5];
		
		float norm = max(max(x, y), z);
		float iv = 1.0f / norm;
		if (norm == 0.0f) {
			return Vector3(zero);
		}
		
		v.set(x*iv, y*iv, z*iv);
	}

	return v;
}


/// Solve eigen system.
void Eigen::solve() {
	semi_definite_symmetric_eigen(matrix, N, eigen_vec, eigen_val);
}

/// Solve eigen system.
void Eigen3::solve() {
	// @@ Use lengyel code that seems to be more optimized.
#if 1
	float v[3*3];
	semi_definite_symmetric_eigen(matrix, 3, v, eigen_val);

	eigen_vec[0].set(v[0], v[1], v[2]);
	eigen_vec[1].set(v[3], v[4], v[5]);
	eigen_vec[2].set(v[6], v[7], v[8]);
#else
	const int maxSweeps = 32;
	const float epsilon = 1.0e-10f;
	
	float m11 = matrix[0]; // m(0,0);
	float m12 = matrix[1]; // m(0,1);
	float m13 = matrix[2]; // m(0,2);
	float m22 = matrix[3]; // m(1,1);
	float m23 = matrix[4]; // m(1,2);
	float m33 = matrix[5]; // m(2,2);
	
	//r.SetIdentity();	
	eigen_vec[0].set(1, 0, 0);
	eigen_vec[1].set(0, 1, 0);
	eigen_vec[2].set(0, 0, 1);
	
	for (int a = 0; a < maxSweeps; a++)
	{
		// Exit if off-diagonal entries small enough
		if ((fabs(m12) < epsilon) && (fabs(m13) < epsilon) && (fabs(m23) < epsilon))
		{
			break;
		}
		
		// Annihilate (1,2) entry
		if (m12 != 0.0f)
		{
			float u = (m22 - m11) * 0.5f / m12;
			float u2 = u * u;
			float u2p1 = u2 + 1.0f;
			float t = (u2p1 != u2) ? ((u < 0.0f) ? -1.0f : 1.0f) * (sqrt(u2p1) - fabs(u)) : 0.5f / u;
			float c = 1.0f / sqrt(t * t + 1.0f);
			float s = c * t;
			
			m11 -= t * m12;
			m22 += t * m12;
			m12 = 0.0f;
			
			float temp = c * m13 - s * m23;
			m23 = s * m13 + c * m23;
			m13 = temp;
			
			for (int i = 0; i < 3; i++)
			{
				float temp = c * eigen_vec[i].x - s * eigen_vec[i].y;
				eigen_vec[i].y = s * eigen_vec[i].x + c * eigen_vec[i].y;
				eigen_vec[i].x = temp;
			}
		}
		
		// Annihilate (1,3) entry
		if (m13 != 0.0f)
		{
			float u = (m33 - m11) * 0.5f / m13;
			float u2 = u * u;
			float u2p1 = u2 + 1.0f;
			float t = (u2p1 != u2) ? ((u < 0.0f) ? -1.0f : 1.0f) * (sqrt(u2p1) - fabs(u)) : 0.5f / u;
			float c = 1.0f / sqrt(t * t + 1.0f);
			float s = c * t;
			
			m11 -= t * m13;
			m33 += t * m13;
			m13 = 0.0f;
			
			float temp = c * m12 - s * m23;
			m23 = s * m12 + c * m23;
			m12 = temp;
			
			for (int i = 0; i < 3; i++)
			{
				float temp = c * eigen_vec[i].x - s * eigen_vec[i].z;
				eigen_vec[i].z = s * eigen_vec[i].x + c * eigen_vec[i].z;
				eigen_vec[i].x = temp;
			}
		}
		
		// Annihilate (2,3) entry
		if (m23 != 0.0f)
		{
			float u = (m33 - m22) * 0.5f / m23;
			float u2 = u * u;
			float u2p1 = u2 + 1.0f;
			float t = (u2p1 != u2) ? ((u < 0.0f) ? -1.0f : 1.0f) * (sqrt(u2p1) - fabs(u)) : 0.5f / u;
			float c = 1.0f / sqrt(t * t + 1.0f);
			float s = c * t;
			
			m22 -= t * m23;
			m33 += t * m23;
			m23 = 0.0f;
			
			float temp = c * m12 - s * m13;
			m13 = s * m12 + c * m13;
			m12 = temp;
			
			for (int i = 0; i < 3; i++)
			{
				float temp = c * eigen_vec[i].y - s * eigen_vec[i].z;
				eigen_vec[i].z = s * eigen_vec[i].y + c * eigen_vec[i].z;
				eigen_vec[i].y = temp;
			}
		}
	}
	
	eigen_val[0] = m11;
	eigen_val[1] = m22;
	eigen_val[2] = m33;
#endif
}


/*---------------------------------------------------------------------------
	Functions
---------------------------------------------------------------------------*/
    

/** @@ I don't remember where did I get this function.
 * computes the eigen values and eigen vectors   
 * of a semi definite symmetric matrix
 *
 * -  matrix is stored in column symmetric storage, i.e.
 *     matrix = { m11, m12, m22, m13, m23, m33, m14, m24, m34, m44 ... }
 *     size = n(n+1)/2
 *
 * - eigen_vectors (return) = { v1, v2, v3, ..., vn } where vk = vk0, vk1, ..., vkn
 *     size = n^2, must be allocated by caller
 *
 * - eigen_values  (return) are in decreasing order
 *     size = n,   must be allocated by caller
 */

void semi_definite_symmetric_eigen(
    const float *mat, int n, float *eigen_vec, float *eigen_val
) {
    float *a,*v;
    float a_norm,a_normEPS,thr,thr_nn;
    int nb_iter = 0;
    int jj;
    int i,j,k,ij,ik,l,m,lm,mq,lq,ll,mm,imv,im,iq,ilv,il,nn;
    int *index;
    float a_ij,a_lm,a_ll,a_mm,a_im,a_il;
    float a_lm_2;
    float v_ilv,v_imv;
    float x;
    float sinx,sinx_2,cosx,cosx_2,sincos;
    float delta;
        
    // Number of entries in mat
        
    nn = (n*(n+1))/2;
        
    // Step 1: Copy mat to a
        
    a = new float[nn];
        
    for( ij=0; ij<nn; ij++ ) {
        a[ij] = mat[ij];
    }
        
    // Ugly Fortran-porting trick: indices for a are between 1 and n
    a--;
        
    // Step 2 : Init diagonalization matrix as the unit matrix
    v = new float[n*n];
        
    ij = 0;
    for( i=0; i<n; i++ ) {
        for( j=0; j<n; j++ ) {
            if( i==j ) {
                v[ij++] = 1.0;
            } else {
                v[ij++] = 0.0;
            }
        }
    }
        
    // Ugly Fortran-porting trick: indices for v are between 1 and n
    v--;
        
    // Step 3 : compute the weight of the non diagonal terms 
    ij     = 1  ;
    a_norm = 0.0;
    for( i=1; i<=n; i++ ) {
        for( j=1; j<=i; j++ ) {
            if( i!=j ) {
                a_ij    = a[ij];
                a_norm += a_ij*a_ij;
            }
            ij++;
        }
    }
        
    if( a_norm != 0.0 ) {
        
        a_normEPS = a_norm*EPS;
        thr       = a_norm    ;
    
        // Step 4 : rotations
        while( thr > a_normEPS   &&   nb_iter < MAX_ITER ) {
        
            nb_iter++;
            thr_nn = thr / nn;
            
            for( l=1  ; l< n; l++ ) {
                for( m=l+1; m<=n; m++ ) {
                    
                    // compute sinx and cosx 
                    
                    lq   = (l*l-l)/2;
                    mq   = (m*m-m)/2;
                    
                    lm     = l+mq;
                    a_lm   = a[lm];
                    a_lm_2 = a_lm*a_lm;
                    
                    if( a_lm_2 < thr_nn ) {
                        continue ;
                    }
                    
                    ll   = l+lq;
                    mm   = m+mq;
                    a_ll = a[ll];
                    a_mm = a[mm];
                    
                    delta = a_ll - a_mm;
                    
                    if( delta == 0.0 ) {
                        x = - PI/4 ; 
                    } else {
                        x = - atan( (a_lm+a_lm) / delta ) / 2.0 ;
                    }

                    sinx    = sin(x)   ;
                    cosx    = cos(x)   ;
                    sinx_2  = sinx*sinx;
                    cosx_2  = cosx*cosx;
                    sincos  = sinx*cosx;
                    
                    // rotate L and M columns 
       
                    ilv = n*(l-1);
                    imv = n*(m-1);
                    
                    for( i=1; i<=n;i++ ) {
                        if( (i!=l) && (i!=m) ) {
                            iq = (i*i-i)/2;
                            
                            if( i<m )  { 
                                im = i + mq; 
                            } else {
                                im = m + iq;
                            }
                            a_im = a[im];
                            
                            if( i<l ) {
                                il = i + lq; 
                            } else {
                                il = l + iq;
                            }
                            a_il = a[il];
                            
                            a[il] =  a_il*cosx - a_im*sinx;
                            a[im] =  a_il*sinx + a_im*cosx;
                        }
                        
                        ilv++;
                        imv++;
                        
                        v_ilv = v[ilv];
                        v_imv = v[imv];
                        
                        v[ilv] = cosx*v_ilv - sinx*v_imv;
                        v[imv] = sinx*v_ilv + cosx*v_imv;
                    } 
                    
                    x = a_lm*sincos; x+=x;
                    
                    a[ll] =  a_ll*cosx_2 + a_mm*sinx_2 - x;
                    a[mm] =  a_ll*sinx_2 + a_mm*cosx_2 + x;
                    a[lm] =  0.0;
                    
                    thr = fabs( thr - a_lm_2 );
                }
            }
        }         
    }
        
    // Step 5: index conversion and copy eigen values 
        
    // back from Fortran to C++
    a++;
        
    for( i=0; i<n; i++ ) {
        k = i + (i*(i+1))/2;
        eigen_val[i] = a[k];
    }
     
    delete[] a;
        
    // Step 6: sort the eigen values and eigen vectors 
        
    index = new int[n];
    for( i=0; i<n; i++ ) {
        index[i] = i;
    }
        
    for( i=0; i<(n-1); i++ ) {
        x = eigen_val[i];
        k = i;
            
        for( j=i+1; j<n; j++ ) {
            if( x < eigen_val[j] ) {
                k = j;
                x = eigen_val[j];
            }
        }
            
        eigen_val[k] = eigen_val[i];
        eigen_val[i] = x;
          
        jj       = index[k];
        index[k] = index[i];
        index[i] = jj;
    }


    // Step 7: save the eigen vectors 
    
    v++; // back from Fortran to to C++
        
    ij = 0;
    for( k=0; k<n; k++ ) {
        ik = index[k]*n;
        for( i=0; i<n; i++ ) {
            eigen_vec[ij++] = v[ik++];
        }
    }
    
    delete[] v    ;
    delete[] index;
    return;
}
    
//_________________________________________________________


// Eric Lengyel code:
// http://www.terathon.com/code/linear.html
#if 0 

const float epsilon = 1.0e-10F;
const int maxSweeps = 32;


struct Matrix3D
{
	float n[3][3];
	
	float& operator()(int i, int j)
	{
		return (n[j][i]);
	}
	
	const float& operator()(int i, int j) const
	{
		return (n[j][i]);
	}
	
	void SetIdentity(void)
	{
		n[0][0] = n[1][1] = n[2][2] = 1.0F;
		n[0][1] = n[0][2] = n[1][0] = n[1][2] = n[2][0] = n[2][1] = 0.0F;
	}
};


void CalculateEigensystem(const Matrix3D& m, float *lambda, Matrix3D& r)
{
	float m11 = m(0,0);
	float m12 = m(0,1);
	float m13 = m(0,2);
	float m22 = m(1,1);
	float m23 = m(1,2);
	float m33 = m(2,2);
	
	r.SetIdentity();
	for (int a = 0; a < maxSweeps; a++)
	{
		// Exit if off-diagonal entries small enough
		if ((Fabs(m12) < epsilon) && (Fabs(m13) < epsilon) &&
			(Fabs(m23) < epsilon)) break;
		
		// Annihilate (1,2) entry
		if (m12 != 0.0F)
		{
			float u = (m22 - m11) * 0.5F / m12;
			float u2 = u * u;
			float u2p1 = u2 + 1.0F;
			float t = (u2p1 != u2) ?
					((u < 0.0F) ? -1.0F : 1.0F) * (sqrt(u2p1) - fabs(u)) : 0.5F / u;
			float c = 1.0F / sqrt(t * t + 1.0F);
			float s = c * t;
			
			m11 -= t * m12;
			m22 += t * m12;
			m12 = 0.0F;
			
			float temp = c * m13 - s * m23;
			m23 = s * m13 + c * m23;
			m13 = temp;
			
			for (int i = 0; i < 3; i++)
			{
				float temp = c * r(i,0) - s * r(i,1);
				r(i,1) = s * r(i,0) + c * r(i,1);
				r(i,0) = temp;
			}
		}
		
		// Annihilate (1,3) entry
		if (m13 != 0.0F)
		{
			float u = (m33 - m11) * 0.5F / m13;
			float u2 = u * u;
			float u2p1 = u2 + 1.0F;
			float t = (u2p1 != u2) ?
				((u < 0.0F) ? -1.0F : 1.0F) * (sqrt(u2p1) - fabs(u)) : 0.5F / u;
			float c = 1.0F / sqrt(t * t + 1.0F);
			float s = c * t;
			
			m11 -= t * m13;
			m33 += t * m13;
			m13 = 0.0F;
			
			float temp = c * m12 - s * m23;
			m23 = s * m12 + c * m23;
			m12 = temp;
			
			for (int i = 0; i < 3; i++)
			{
				float temp = c * r(i,0) - s * r(i,2);
				r(i,2) = s * r(i,0) + c * r(i,2);
				r(i,0) = temp;
			}
		}
		
		// Annihilate (2,3) entry
		if (m23 != 0.0F)
		{
			float u = (m33 - m22) * 0.5F / m23;
			float u2 = u * u;
			float u2p1 = u2 + 1.0F;
			float t = (u2p1 != u2) ?
				((u < 0.0F) ? -1.0F : 1.0F) * (sqrt(u2p1) - fabs(u)) : 0.5F / u;
			float c = 1.0F / sqrt(t * t + 1.0F);
			float s = c * t;
			
			m22 -= t * m23;
			m33 += t * m23;
			m23 = 0.0F;
			
			float temp = c * m12 - s * m13;
			m13 = s * m12 + c * m13;
			m12 = temp;
			
			for (int i = 0; i < 3; i++)
			{
				float temp = c * r(i,1) - s * r(i,2);
				r(i,2) = s * r(i,1) + c * r(i,2);
				r(i,1) = temp;
			}
		}
	}
	
	lambda[0] = m11;
	lambda[1] = m22;
	lambda[2] = m33;
}


#endif
