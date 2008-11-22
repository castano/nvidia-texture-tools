// This code is in the public domain -- icastano@gmail.com

using namespace nv;


Vector3 nv::ComputeCentroid(int n, const Vec3 * points, const float * weights, Vector3::Arg metric, float * covariance)
{
	Vector3 centroid(zero);
	float total = 0.0f;

	for (int i = 0; i < n; i++)
	{
		total += weights[i];
		centroid += weights[i]*points[i];
	}
	centroid /= total;

	return centroid;
}


void nv::ComputeCovariance(int n, const Vec3 * points, const float * weights, Vector3::Arg metric, float * covariance)
{
	// compute the centroid
	Vector3 centroid = ComputeCentroid(n, points, weights, metric);

	// compute covariance matrix
	for (int i = 0; i < 6; i++)
	{
		covariance[i] = 0.0f;
	}

	for (int i = 0; i < n; i++)
	{
		Vector3 a = (points[i] - centroid) * metric;
		Vector3 b = weights[i]*a;
		
		covariance[0] += a.X()*b.X();
		covariance[1] += a.X()*b.Y();
		covariance[2] += a.X()*b.Z();
		covariance[3] += a.Y()*b.Y();
		covariance[4] += a.Y()*b.Z();
		covariance[5] += a.Z()*b.Z();
	}
}

Vector3 nv::ComputePrincipalComponent(int n, const Vec3 * points, const float * weights, Vector3::Arg metric)
{
	float matrix[6];
	ComputeCovariance(n, points, weights, metric, matrix);

	if (covariance[0] == 0 || covariance[3] == 0 || covariance[5] == 0)
	{
		return Vector3(zero);
	}
	
	const int NUM = 8;

	Vector3 v(1, 1, 1);
	for (int i = 0; i < NUM; i++)
	{
		float x = v.x() * matrix[0] + v.y() * matrix[1] + v.z() * matrix[2];
		float y = v.x() * matrix[1] + v.y() * matrix[3] + v.z() * matrix[4];
		float z = v.x() * matrix[2] + v.y() * matrix[4] + v.z() * matrix[5];
		
		float norm = std::max(std::max(x, y), z);
	
		v = Vector3(x, y, z) / norm;
	}

	return v;	
}



void nv::Compute4Means(int n, const Vec3 * points, const float * weights, Vector3::Arg metric, Vector3 * cluster)
{
	Vector3 centroid = ComputeCentroid(n, points, weights, metric);
	
	// Compute principal component.
	Vector3 principal = ComputePrincipalComponent(n, points, weights, metric);
	
	// Pick initial solution.
	int mini, maxi;
	mini = maxi = 0;
	
	float mindps, maxdps;
	mindps = maxdps = dot(points[0], principal);
	
	for (int i = 1; i < count; ++i)
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
	cluster[3] = centroid + maxdps * principal;
	cluster[1] = (2 * cluster[0] + cluster[1]) / 3;
	cluster[2] = (2 * cluster[1] + cluster[0]) / 3;

	// Now we have to iteratively refine the clusters.
	while(true)
	{
		Vector3 newCluster[4] = { Vector3(zero), Vector3(zero), Vector3(zero), Vector3(zero) };
		float total[4] = {0, 0, 0, 0};
		
		for (int i = 0; i < count; ++i)
		{
			// Find nearest cluster.
			int nearest = 0;
			float mindist = FLT_MAX;
			for (int j = 0; j < 4; j++)
			{
				float dist = lengthSquared(cluster[j] - points[i]);
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
			newCluster[j] /= total[j];
		}

		if (equal(cluster[0], newCluster[0]) && equal(cluster[3], newCluster[3]))
		{
			break;
		}
		
		// @@ We should choose the optimal assignment.
		cluster[0] = newCluster[0];
		cluster[3] = newCluster[3];
		cluster[1] = (2 * cluster[0] + cluster[1]) / 3;
		cluster[2] = (2 * cluster[1] + cluster[0]) / 3;
	}
}
