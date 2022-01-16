//#include "Engine/Physics/Physics.h"
//#include "Rigidbodies.h"
//
//
//float TestSeparatingAxis(SOrientedBox& A, SOrientedBox& B, float3& Axis, float* fMinDepth)
//{
//	Axis.normalize();
//
//	if (float3::dotproduct(B.m_Position - A.m_Position, Axis) < 0.f)
//		Axis = -1.f * Axis;
//
//	float fMin = 1e8f, fMax = -1e8f;
//	float3 Point = A.m_Position;
//	int i;
//
//	for (i = 0; i < 3; i++)
//	{
//		if (float3::dotproduct(A.m_Basis[i], Axis) > 0.f)
//			Point += 0.5f * A.m_Dim[i] * A.m_Basis[i];
//
//		else
//			Point -= 0.5f * A.m_Dim[i] * A.m_Basis[i];
//	}
//
//	fMax = float3::dotproduct(Point, Axis);
//
//	Point = B.m_Position;
//
//	for (i = 0; i < 3; i++)
//	{
//		if (float3::dotproduct(B.m_Basis[i], Axis) < 0.f)
//			Point += 0.5f * B.m_Dim[i] * B.m_Basis[i];
//
//		else
//			Point -= 0.5f * B.m_Dim[i] * B.m_Basis[i];
//	}
//
//	fMin = float3::dotproduct(Point, Axis);
//
//	*fMinDepth = fMin;
//
//	return fMax - fMin;
//}
//
//
//float TestSeparatingAxis(SOrientedBox& A, SSphere& S, float3& Axis, float* fMinDepth)
//{
//	Axis.normalize();
//
//	if (float3::dotproduct(S.m_Position - A.m_Position, Axis) < 0.f)
//		Axis = -1.f * Axis;
//
//	float fMin = 1e8f, fMax = -1e8f;
//	float3 Point = A.m_Position;
//	int i;
//
//	for (i = 0; i < 3; i++)
//	{
//		if (float3::dotproduct(A.m_Basis[i], Axis) > 0.f)
//			Point += 0.5f * A.m_Dim[i] * A.m_Basis[i];
//
//		else
//			Point -= 0.5f * A.m_Dim[i] * A.m_Basis[i];
//	}
//
//	fMax = float3::dotproduct(Point, Axis);
//
//	fMin = float3::dotproduct(S.m_Position, Axis) - S.m_fRadius;
//
//	*fMinDepth = fMin;
//
//	return fMax - fMin;
//}
//
//
//float TestSeparatingAxis(SOrientedBox& A, SPartialSphere& S, float3& Axis, float* fMinDepth)
//{
//	Axis.normalize();
//
//	if (float3::dotproduct(S.m_Position - A.m_Position, Axis) < 0.f)
//		Axis = -1.f * Axis;
//
//	float fMin = 1e8f, fMax = -1e8f;
//	float3 Point = A.m_Position;
//	int i;
//
//	for (i = 0; i < 3; i++)
//	{
//		if (float3::dotproduct(A.m_Basis[i], Axis) > 0.f)
//			Point += 0.5f * A.m_Dim[i] * A.m_Basis[i];
//
//		else
//			Point -= 0.5f * A.m_Dim[i] * A.m_Basis[i];
//	}
//
//	fMax = float3::dotproduct(Point, Axis);
//
//	fMin = float3::dotproduct(S.m_Position, Axis) - S.m_fRadius;
//
//	*fMinDepth = fMin;
//
//	return fMax - fMin;
//}
//
//
//
//bool Collisions::GetMinimumTranslationVector(SOrientedBox& A, SSphere& S, float3& Direction, float* pDepth, float* fMinDepth)
//{
//	int i;
//	float fDepth = 1e8f, fTest = 0.f;
//	float fMin;
//	float3 Axis;
//	float3 MTV(0.f, 0.f, 0.f);
//
//	for (i = 0; i < 3; i++)
//	{
//		fTest = TestSeparatingAxis(A, S, A.m_Basis[i], &fMin);
//		if (fTest < 0.f)
//		{
//			Direction = A.m_Basis[i];
//			return false;
//		}
//
//		if (fTest < fDepth)
//		{
//			fDepth = fTest;
//			*fMinDepth = fMin;
//			MTV = A.m_Basis[i];
//		}
//	}
//
//	for (i = 0; i < 3; i++)
//	{
//		Axis = A.m_Basis[i] + A.m_Basis[(i + 1) % 3];
//		Axis.normalize();
//
//		fTest = TestSeparatingAxis(A, S, Axis, &fMin);
//		if (fTest < 0.f)
//		{
//			Direction = Axis;
//			return false;
//		}
//
//		if (fTest < fDepth)
//		{
//			fDepth = fTest;
//			*fMinDepth = fMin;
//			MTV = Axis;
//		}
//	}
//
//	Direction = MTV;
//	*pDepth = fDepth;
//
//	return true;
//}
//
//
//
//bool Collisions::GetMinimumTranslationVector(SOrientedBox& A, SPartialSphere& S, float3& Direction, float* pDepth, float* fMinDepth)
//{
//	int i;
//	float fDepth = 1e8f, fTest = 0.f;
//	float fMin;
//	float3 Axis;
//	float3 MTV(0.f, 0.f, 0.f);
//
//	for (i = 0; i < 3; i++)
//	{
//		fTest = TestSeparatingAxis(A, S, A.m_Basis[i], &fMin);
//		if (fTest < 0.f)
//		{
//			Direction = A.m_Basis[i];
//			return false;
//		}
//
//		if (fTest < fDepth)
//		{
//			fDepth = fTest;
//			*fMinDepth = fMin;
//			MTV = A.m_Basis[i];
//		}
//	}
//
//	for (i = 0; i < 3; i++)
//	{
//		Axis = A.m_Basis[i] + A.m_Basis[(i + 1) % 3];
//		Axis.normalize();
//
//		fTest = TestSeparatingAxis(A, S, Axis, &fMin);
//		if (fTest < 0.f)
//		{
//			Direction = Axis;
//			return false;
//		}
//
//		if (fTest < fDepth)
//		{
//			fDepth = fTest;
//			*fMinDepth = fMin;
//			MTV = Axis;
//		}
//	}
//
//	Direction = MTV;
//	*pDepth = fDepth;
//
//	return true;
//}
//
//
//
//bool Collisions::GetMinimumTranslationVector(SOrientedBox& A, SOrientedBox& B, float3& Direction, float* pDepth, float* fMinDepth)
//{
//	int i, j;
//	float fDepth = 1e8f, fTest = 0.f;
//	float fMin;
//	float3 Axis;
//	float3 MTV(0.f, 0.f, 0.f);
//
//	for (i = 0; i < 3; i++)
//	{
//		fTest = TestSeparatingAxis(A, B, A.m_Basis[i], &fMin);
//		if (fTest < 0.f)
//		{
//			Direction = A.m_Basis[i];
//			return false;
//		}
//
//		if (fTest < fDepth)
//		{
//			fDepth = fTest;
//			*fMinDepth = fMin;
//			MTV = A.m_Basis[i];
//		}
//	}
//
//	for (i = 0; i < 3; i++)
//	{
//		fTest = TestSeparatingAxis(A, B, B.m_Basis[i], &fMin);
//		if (fTest < 0.f)
//		{
//			Direction = B.m_Basis[i];
//			return false;
//		}
//
//		if (fTest < fDepth)
//		{
//			fDepth = fTest;
//			*fMinDepth = fMin;
//			MTV = B.m_Basis[i];
//		}
//	}
//
//	for (i = 0; i < 3; i++)
//	{
//		for (j = 0; j < 3; j++)
//		{
//			Axis = float3::cross(A.m_Basis[i], B.m_Basis[j]);
//			if (Axis.length() > 1e-4f)
//			{
//				fTest = TestSeparatingAxis(A, B, Axis, &fMin);
//				if (fTest < 0.f)
//				{
//					Direction = Axis;
//					return false;
//				}
//
//				if (fTest < fDepth)
//				{
//					fDepth = fTest;
//					*fMinDepth = fMin;
//					MTV = Axis;
//				}
//			}
//		}
//	}
//
//	Direction = MTV;
//	*pDepth = fDepth;
//
//	return true;
//}
//
//
