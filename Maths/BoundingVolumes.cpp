#include "Maths.h"



float ComputeSphere(float3& Center, float3 Points[4], int nNumPoints)
{
	if (nNumPoints == 3 || nNumPoints == 4)
	{
		float fRadius = (Points[0] - Points[1]).length();
		Center = 0.5f * (Points[0] + Points[1]);
		float3 e2 = Points[1] - Points[0];
		float3 e1 = Points[2] - Points[0];
		float3 e3 = Points[3] - Points[0];

		if (fabs(float3::dotproduct(e1, float3::cross(e2, e3))) < 1e-3f)
			nNumPoints = 3;

		e1 = e1 - float3::dotproduct(e2, e1) * e2;
		e1.normalize();
		e2.normalize();

		float3 v = Points[2] - Points[0];
		v.normalize();

		float3 M = 0.5f * (Points[2] + Points[0]);
		float alpha = float3::dotproduct(v, e2);
		float beta = -float3::dotproduct(v, e1);

		float t = (float3::dotproduct(Center, e1) - float3::dotproduct(M, e2)) / beta;

		Center = M + t * (alpha * e1 + beta * e2);

		if (nNumPoints == 3)
			return (Points[0] - Center).length();

		else
		{
			e1 = float3::cross(e1, e2);
			e2 = Center - Points[0];
			e1.normalize();
			e2.normalize();

			e3.normalize();

			M = 0.5f * (Points[3] + Points[0]);
			alpha = float3::dotproduct(e3, e2);
			beta = -float3::dotproduct(e3, e1);

			t = (float3::dotproduct(Center, e1) - float3::dotproduct(M, e2)) / beta;

			Center = M + t * (alpha * e1 + beta * e2);

			return (Points[0] - Center).length();
		}
	}

	if (nNumPoints == 2)
	{
		float fRadius = (Points[0] - Points[1]).length();

		Center = 0.5f * (Points[0] + Points[1]);
		return fRadius * 0.5f;
	}


	Center = Points[0];
	return 0.01f;
}



float Maths::ComputeBoundingSphere(float3& Center, float* Points, int nNumPoints, int nStride)
{
	return 0.f;
}
