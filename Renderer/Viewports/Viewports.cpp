#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Viewports.h"


unsigned int				CViewportManager::ms_nNumViewports			= 0U;
thread_local unsigned int	CViewportManager::ms_nCurrentViewport		= 0U;

std::vector<CViewportManager::SViewport>	CViewportManager::ms_Viewports[2];

std::vector<CViewportManager::SViewport>*	CViewportManager::ms_pViewportsToSet	= &CViewportManager::ms_Viewports[0];
std::vector<CViewportManager::SViewport>*	CViewportManager::ms_pViewportsToFlush	= &CViewportManager::ms_Viewports[1];



void CViewportManager::Init()
{
	ms_nNumViewports			= 0U;
	ms_nCurrentViewport			= 0U;

	ms_Viewports[0].reserve(64);
	ms_Viewports[1].reserve(64);
}


void CViewportManager::Terminate()
{

}


void CViewportManager::UpdateBeforeFlush()
{
	std::vector<CViewportManager::SViewport>* tmp = ms_pViewportsToSet;
	ms_pViewportsToSet = ms_pViewportsToFlush;
	ms_pViewportsToFlush = tmp;
}


void CViewportManager::InitFrame()
{
	ms_nNumViewports			= 0U;
	(*ms_pViewportsToSet).clear();

	int nViewportId = NewViewport();
	SetViewport(nViewportId, CRenderer::GetCurrentCamera()->GetPosition(), CRenderer::GetCurrentCamera()->GetViewProjMatrix());
}


void CViewportManager::BindViewport(unsigned int nViewportID)
{
	ms_nCurrentViewport = nViewportID;
}


unsigned int CViewportManager::GetCurrentViewport()
{
	return ms_nCurrentViewport;
}


int CViewportManager::NewViewport(int numViewports)
{
	if (ms_nNumViewports + numViewports > 64)
		return -1;

	int nViewportId = ms_nNumViewports;
	ms_nNumViewports += numViewports;

	(*ms_pViewportsToSet).resize(ms_nNumViewports);

	return nViewportId;
}


void CViewportManager::SetViewport(int nViewportId, float3& Pos, float4x4& ViewProj)
{
	(*ms_pViewportsToSet)[nViewportId].m_InvViewProj = inverse(ViewProj);
	(*ms_pViewportsToSet)[nViewportId].m_Center		= Pos;
	(*ms_pViewportsToSet)[nViewportId].m_fRadius	= -1.f;
}


void CViewportManager::SetViewportOmni(int nViewportId, float3& Center, float fRadius)
{
	(*ms_pViewportsToSet)[nViewportId].m_Center = Center;
	(*ms_pViewportsToSet)[nViewportId].m_fRadius = fRadius;
}


void CViewportManager::ComputeVisibility(ERenderList eRenderType)
{
	std::vector<Drawable>& pList = CPacketManager::GetDrawListToFill(eRenderType);
	std::vector<Packet>::iterator it;

	size_t nNumDrawables = pList.size();
	float4x4 World;

	float4		Corners[8] = {
								float4(-1.f, -1.f, 1.f, 1.f),
								float4(-1.f, 1.f, 1.f, 1.f),
								float4(1.f, 1.f, 1.f, 1.f),
								float4(1.f, -1.f, 1.f, 1.f),
								float4(-1.f, -1.f, 0.f, 1.f),
								float4(-1.f, 1.f, 0.f, 1.f),
								float4(1.f, 1.f, 0.f, 1.f),
								float4(1.f, -1.f, 0.f, 1.f)
							};

	float3 UnprojectedCorners[8];
	float3 PlaneNormals[5];
	float3 Forward, Center, PacketCenter;
	float Near, Far, Tan2_fov;
	float BoundingSphereRadius;
	float3 tmp;
	float3 Eye;

	float4 Point;

	for (unsigned int i = 0U; i < ms_nNumViewports; i++)
	{
		if ((*ms_pViewportsToSet)[i].m_fRadius > 0.f)
			continue;

		Eye = (*ms_pViewportsToSet)[i].m_Center;

		for (unsigned int j = 0U; j < 8U; j++)
		{
			Point = (*ms_pViewportsToSet)[i].m_InvViewProj * Corners[j];
			Point /= Point.w;

			Copy(UnprojectedCorners[j].v(), Point.v());
		}

		Point = float4(0.f, 0.f, 1.f, 1.f);

		Point = (*ms_pViewportsToSet)[i].m_InvViewProj * Point;
		Point /= Point.w;

		Forward = 0.f;
		for (unsigned int j = 0U; j < 4U; j++)
		{
			Forward += UnprojectedCorners[j + 4U] * 0.25f;
		}

		Forward -= Eye;
		Far = Forward.length();
		Forward /= Far;

		Near = MAX(0.01f, float3::dotproduct(UnprojectedCorners[0] - Eye, Forward));
		Tan2_fov = 0.5f * (UnprojectedCorners[6] - UnprojectedCorners[5]).length() / Far;
		Tan2_fov *= Tan2_fov;

		Center = 0.5f * (Near + Far) * Forward + Eye;
		BoundingSphereRadius = (UnprojectedCorners[0] - Center).length();

		for (unsigned int j = 1U; j < 8U; j++)
			BoundingSphereRadius = MAX(BoundingSphereRadius, (UnprojectedCorners[j] - Center).length());


		PlaneNormals[0] = float3::cross(UnprojectedCorners[5] - UnprojectedCorners[4], UnprojectedCorners[7] - UnprojectedCorners[4]);
		PlaneNormals[0].normalize();

		PlaneNormals[1] = float3::cross(UnprojectedCorners[5] - UnprojectedCorners[4], UnprojectedCorners[4] - UnprojectedCorners[0]);
		PlaneNormals[1].normalize();

		PlaneNormals[2] = float3::cross(UnprojectedCorners[7] - UnprojectedCorners[4], UnprojectedCorners[4] - UnprojectedCorners[0]);
		PlaneNormals[2].normalize();

		PlaneNormals[3] = float3::cross(UnprojectedCorners[6] - UnprojectedCorners[5], UnprojectedCorners[5] - UnprojectedCorners[1]);
		PlaneNormals[3].normalize();

		PlaneNormals[4] = float3::cross(UnprojectedCorners[7] - UnprojectedCorners[6], UnprojectedCorners[6] - UnprojectedCorners[2]);
		PlaneNormals[4].normalize();

		for (size_t j = 0; j < nNumDrawables; j++)
		{
			PacketList& pPacketList = pList[j].m_pPacketList;

			for (it = pPacketList.m_pPackets.begin(); it < pPacketList.m_pPackets.end(); it++)
			{
				Packet& pPacket = *it;

				if (i == 0U)
					pPacket.m_nViewportMask = 0ULL;

				Copy(Point.v(), pPacket.m_Center.v());
				Point.w = 1.f;

				Copy(PacketCenter.v(), Point.v());

				tmp = Center - PacketCenter;

				if (float3::dotproduct(tmp, tmp) > (BoundingSphereRadius + pPacket.m_fBoundingSphereRadius) * (BoundingSphereRadius + pPacket.m_fBoundingSphereRadius))
					continue;

				if (fabs(float3::dotproduct(PacketCenter - UnprojectedCorners[4], PlaneNormals[0])) < pPacket.m_fBoundingSphereRadius)
				{
					pPacket.m_nViewportMask |= (1ULL << i);
					continue;
				}

				else if (float3::dotproduct(PacketCenter - UnprojectedCorners[4], PlaneNormals[0]) * float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[0]) < 0.f)
					continue;

				if (fabs(float3::dotproduct(PacketCenter - UnprojectedCorners[4], PlaneNormals[1])) < pPacket.m_fBoundingSphereRadius)
				{
					pPacket.m_nViewportMask |= (1ULL << i);
					continue;
				}

				else if (float3::dotproduct(PacketCenter - UnprojectedCorners[4], PlaneNormals[1]) * float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[1]) < 0.f)
					continue;

				if (fabs(float3::dotproduct(PacketCenter - UnprojectedCorners[4], PlaneNormals[2])) < pPacket.m_fBoundingSphereRadius)
				{
					pPacket.m_nViewportMask |= (1ULL << i);
					continue;
				}

				else if (float3::dotproduct(PacketCenter - UnprojectedCorners[4], PlaneNormals[2]) * float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[2]) < 0.f)
					continue;

				if (fabs(float3::dotproduct(PacketCenter - UnprojectedCorners[5], PlaneNormals[3])) < pPacket.m_fBoundingSphereRadius)
				{
					pPacket.m_nViewportMask |= (1ULL << i);
					continue;
				}

				else if (float3::dotproduct(PacketCenter - UnprojectedCorners[5], PlaneNormals[3]) * float3::dotproduct(Center - UnprojectedCorners[5], PlaneNormals[3]) < 0.f)
					continue;

				if (fabs(float3::dotproduct(PacketCenter - UnprojectedCorners[6], PlaneNormals[4])) < pPacket.m_fBoundingSphereRadius)
				{
					pPacket.m_nViewportMask |= (1ULL << i);
					continue;
				}

				else if (float3::dotproduct(PacketCenter - UnprojectedCorners[6], PlaneNormals[4]) * float3::dotproduct(Center - UnprojectedCorners[6], PlaneNormals[4]) < 0.f)
					continue;

				pPacket.m_nViewportMask |= (1ULL << i);
			}
		}
	}


	for (unsigned int i = 0U; i < ms_nNumViewports; i++)
	{
		if ((*ms_pViewportsToSet)[i].m_fRadius < 0.f)
			continue;

		float viewportRadius = (*ms_pViewportsToSet)[i].m_fRadius;

		float3 viewportCenter = (*ms_pViewportsToSet)[i].m_Center;

		for (size_t j = 0; j < nNumDrawables; j++)
		{
			PacketList& pPacketList = pList[j].m_pPacketList;

			for (it = pPacketList.m_pPackets.begin(); it < pPacketList.m_pPackets.end(); it++)
			{
				float3 d = it->m_Center - viewportCenter;
				float r = viewportRadius + it->m_fBoundingSphereRadius;

				if (float3::dotproduct(d, d) < r * r)
					it->m_nViewportMask |= (1ULL << i);
			}
		}
	}
}


bool CViewportManager::IsVisible(float3& viewportCenter, float4x4& frustum, float3& Center, float fRadius)
{
	float4x4&	InvViewProj = inverse(frustum);

	float3 UnprojectedCorners[8];
	float3 PlaneNormals[5];
	float3 Forward, ViewportCenter;
	float Near, Far, Tan2_fov;
	float BoundingSphereRadius;
	float3 tmp;

	float4		Corners[8] = 
	{
		float4(-1.f, -1.f, 1.f, 1.f),
		float4(-1.f, 1.f, 1.f, 1.f),
		float4(1.f, 1.f, 1.f, 1.f),
		float4(1.f, -1.f, 1.f, 1.f),
		float4(-1.f, -1.f, 0.f, 1.f),
		float4(-1.f, 1.f, 0.f, 1.f),
		float4(1.f, 1.f, 0.f, 1.f),
		float4(1.f, -1.f, 0.f, 1.f)
	};

	float4 Point;
	float3 Eye = viewportCenter;

	for (unsigned int j = 0U; j < 8U; j++)
	{
		Point = InvViewProj * Corners[j];
		Point /= Point.w;

		Copy(UnprojectedCorners[j].v(), Point.v());
	}

	Forward = 0.f;
	for (unsigned int j = 0U; j < 4U; j++)
	{
		Forward += UnprojectedCorners[j + 4U] * 0.25f;
	}

	Forward -= Eye;
	Far = Forward.length();
	Forward /= Far;

	Near = MAX(0.01f, float3::dotproduct(UnprojectedCorners[0] - Eye, Forward));
	Tan2_fov = 0.5f * (UnprojectedCorners[6] - UnprojectedCorners[5]).length() / Far;
	Tan2_fov *= Tan2_fov;

	ViewportCenter = 0.5f * (Near + Far) * Forward + Eye;
	BoundingSphereRadius = (UnprojectedCorners[0] - ViewportCenter).length();

	for (unsigned int j = 1U; j < 8U; j++)
		BoundingSphereRadius = MAX(BoundingSphereRadius, (UnprojectedCorners[j] - ViewportCenter).length());


	PlaneNormals[0] = float3::cross(UnprojectedCorners[5] - UnprojectedCorners[4], UnprojectedCorners[7] - UnprojectedCorners[4]);
	PlaneNormals[0].normalize();

	PlaneNormals[1] = float3::cross(UnprojectedCorners[5] - UnprojectedCorners[4], UnprojectedCorners[4] - UnprojectedCorners[0]);
	PlaneNormals[1].normalize();

	PlaneNormals[2] = float3::cross(UnprojectedCorners[7] - UnprojectedCorners[4], UnprojectedCorners[4] - UnprojectedCorners[0]);
	PlaneNormals[2].normalize();

	PlaneNormals[3] = float3::cross(UnprojectedCorners[6] - UnprojectedCorners[5], UnprojectedCorners[5] - UnprojectedCorners[1]);
	PlaneNormals[3].normalize();

	PlaneNormals[4] = float3::cross(UnprojectedCorners[7] - UnprojectedCorners[6], UnprojectedCorners[6] - UnprojectedCorners[2]);
	PlaneNormals[4].normalize();


	tmp = ViewportCenter - Center;

	if (float3::dotproduct(tmp, tmp) > (BoundingSphereRadius + fRadius) * (BoundingSphereRadius + fRadius))
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[0])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[0]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[4], PlaneNormals[0]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[1])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[1]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[4], PlaneNormals[1]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[2])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[2]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[4], PlaneNormals[2]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[5], PlaneNormals[3])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[5], PlaneNormals[3]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[5], PlaneNormals[3]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[6], PlaneNormals[4])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[6], PlaneNormals[4]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[6], PlaneNormals[4]) < 0.f)
		return false;


	return true;
}


bool CViewportManager::IsVisible(unsigned int nViewportID, float3& Center, float fRadius)
{
	if ((*ms_pViewportsToSet)[nViewportID].m_fRadius > 0.f)
	{
		if (((*ms_pViewportsToSet)[nViewportID].m_Center - Center).length() < fRadius + (*ms_pViewportsToSet)[nViewportID].m_fRadius)
			return true;

		return false;
	}

	float4x4&	InvViewProj = (*ms_pViewportsToSet)[nViewportID].m_InvViewProj;


	float3 UnprojectedCorners[8];
	float3 PlaneNormals[5];
	float3 Forward, ViewportCenter;
	float Near, Far, Tan2_fov;
	float BoundingSphereRadius;
	float3 tmp;

	float4		Corners[8] = 
	{
		float4(-1.f, -1.f, 1.f, 1.f),
		float4(-1.f, 1.f, 1.f, 1.f),
		float4(1.f, 1.f, 1.f, 1.f),
		float4(1.f, -1.f, 1.f, 1.f),
		float4(-1.f, -1.f, 0.f, 1.f),
		float4(-1.f, 1.f, 0.f, 1.f),
		float4(1.f, 1.f, 0.f, 1.f),
		float4(1.f, -1.f, 0.f, 1.f)
	};

	float4 Point;
	float3 Eye = (*ms_pViewportsToSet)[nViewportID].m_Center;

	for (unsigned int j = 0U; j < 8U; j++)
	{
		Point = InvViewProj * Corners[j];
		Point /= Point.w;

		Copy(UnprojectedCorners[j].v(), Point.v());
	}

	Forward = 0.f;
	for (unsigned int j = 0U; j < 4U; j++)
	{
		Forward += UnprojectedCorners[j + 4U] * 0.25f;
	}

	Forward -= Eye;
	Far = Forward.length();
	Forward /= Far;

	Near = MAX(0.01f, float3::dotproduct(UnprojectedCorners[0] - Eye, Forward));
	Tan2_fov = 0.5f * (UnprojectedCorners[6] - UnprojectedCorners[5]).length() / Far;
	Tan2_fov *= Tan2_fov;

	ViewportCenter = 0.5f * (Near + Far) * Forward + Eye;
	BoundingSphereRadius = (UnprojectedCorners[0] - ViewportCenter).length();

	for (unsigned int j = 1U; j < 8U; j++)
		BoundingSphereRadius = MAX(BoundingSphereRadius, (UnprojectedCorners[j] - ViewportCenter).length());


	PlaneNormals[0] = float3::cross(UnprojectedCorners[5] - UnprojectedCorners[4], UnprojectedCorners[7] - UnprojectedCorners[4]);
	PlaneNormals[0].normalize();

	PlaneNormals[1] = float3::cross(UnprojectedCorners[5] - UnprojectedCorners[4], UnprojectedCorners[4] - UnprojectedCorners[0]);
	PlaneNormals[1].normalize();

	PlaneNormals[2] = float3::cross(UnprojectedCorners[7] - UnprojectedCorners[4], UnprojectedCorners[4] - UnprojectedCorners[0]);
	PlaneNormals[2].normalize();

	PlaneNormals[3] = float3::cross(UnprojectedCorners[6] - UnprojectedCorners[5], UnprojectedCorners[5] - UnprojectedCorners[1]);
	PlaneNormals[3].normalize();

	PlaneNormals[4] = float3::cross(UnprojectedCorners[7] - UnprojectedCorners[6], UnprojectedCorners[6] - UnprojectedCorners[2]);
	PlaneNormals[4].normalize();


	tmp = ViewportCenter - Center;

	if (float3::dotproduct(tmp, tmp) > (BoundingSphereRadius + fRadius) * (BoundingSphereRadius + fRadius))
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[0])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[0]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[4], PlaneNormals[0]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[1])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[1]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[4], PlaneNormals[1]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[2])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[4], PlaneNormals[2]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[4], PlaneNormals[2]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[5], PlaneNormals[3])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[5], PlaneNormals[3]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[5], PlaneNormals[3]) < 0.f)
		return false;

	if (fabs(float3::dotproduct(Center - UnprojectedCorners[6], PlaneNormals[4])) < fRadius)
		return true;

	else if (float3::dotproduct(Center - UnprojectedCorners[6], PlaneNormals[4]) * float3::dotproduct(ViewportCenter - UnprojectedCorners[6], PlaneNormals[4]) < 0.f)
		return false;


	return true;
}
