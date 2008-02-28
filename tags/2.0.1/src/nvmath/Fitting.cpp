// License: Wild Magic License Version 3
// http://geometrictools.com/License/WildMagic3License.pdf

#include "Fitting.h"
#include "Eigen.h"

using namespace nv;


/** Fit a 3d line to the given set of points. 
 *
 * Based on code from:
 * http://geometrictools.com/
 */
Line3 Fit::bestLine(const Array<Vector3> & pointArray)
{
	nvDebugCheck(pointArray.count() > 0);
	
	Line3 line;
	
	const uint pointCount = pointArray.count();
	const float inv_num = 1.0f / pointCount;

	// compute the mean of the points
	Vector3 center(zero);
	for(uint i = 0; i < pointCount; i++) {
		center += pointArray[i];
	}
	line.setOrigin(center * inv_num);

	// compute the covariance matrix of the points
	float covariance[6] = {0, 0, 0, 0, 0, 0};
	for(uint i = 0; i < pointCount; i++) {
		Vector3 diff = pointArray[i] - line.origin();
		covariance[0] += diff.x() * diff.x();
		covariance[1] += diff.x() * diff.y();
		covariance[2] += diff.x() * diff.z();
		covariance[3] += diff.y() * diff.y();
		covariance[4] += diff.y() * diff.z();
		covariance[5] += diff.z() * diff.z();
	}
	
	line.setDirection(normalizeSafe(firstEigenVector(covariance), Vector3(zero), 0.0f));
	
	// @@ This variant is from David Eberly... I'm not sure how that works.
	/*sum_xx *= inv_num;
	sum_xy *= inv_num;
	sum_xz *= inv_num;
	sum_yy *= inv_num;
	sum_yz *= inv_num;
	sum_zz *= inv_num;

    // set up the eigensolver
	Eigen3 ES;
	ES(0,0) = sum_yy + sum_zz;
	ES(0,1) = -sum_xy;
	ES(0,2) = -sum_xz;
	ES(1,1) = sum_xx + sum_zz;
	ES(1,2) = -sum_yz;
	ES(2,2) = sum_xx + sum_yy;

    // compute eigenstuff, smallest eigenvalue is in last position
	ES.solve();

	line.setDirection(ES.eigenVector(2));
	
	nvCheck( isNormalized(line.direction()) );
	*/
	return line;
}


/** Fit a 3d plane to the given set of points. 
 *
 * Based on code from:
 * http://geometrictools.com/
 */
Vector4 Fit::bestPlane(const Array<Vector3> & pointArray)
{
	Vector3 center(zero);
	
	const uint pointCount = pointArray.count();
	const float inv_num = 1.0f / pointCount;

    // compute the mean of the points
	for(uint i = 0; i < pointCount; i++) {
		center += pointArray[i];
	}
	center *= inv_num;

    // compute the covariance matrix of the points
	float sum_xx = 0.0f;
	float sum_xy = 0.0f;
	float sum_xz = 0.0f;
	float sum_yy = 0.0f;
	float sum_yz = 0.0f;
	float sum_zz = 0.0f;
	
	for(uint i = 0; i < pointCount; i++) {
		Vector3 diff = pointArray[i] - center;
		sum_xx += diff.x() * diff.x();
		sum_xy += diff.x() * diff.y();
		sum_xz += diff.x() * diff.z();
		sum_yy += diff.y() * diff.y();
		sum_yz += diff.y() * diff.z();
		sum_zz += diff.z() * diff.z();
	}
	
	sum_xx *= inv_num;
	sum_xy *= inv_num;
	sum_xz *= inv_num;
	sum_yy *= inv_num;
	sum_yz *= inv_num;
	sum_zz *= inv_num;

    // set up the eigensolver
	Eigen3 ES;
	ES(0,0) = sum_yy + sum_zz;
	ES(0,1) = -sum_xy;
	ES(0,2) = -sum_xz;
	ES(1,1) = sum_xx + sum_zz;
	ES(1,2) = -sum_yz;
	ES(2,2) = sum_xx + sum_yy;

    // compute eigenstuff, greatest eigenvalue is in first position
	ES.solve();

	Vector3 normal = ES.eigenVector(0);
	nvCheck(isNormalized(normal));
	
	float offset = dot(normal, center);

	return Vector4(normal, offset);
}
