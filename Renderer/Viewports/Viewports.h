#ifndef __VIEWPORTS_H__
#define __VIEWPORTS_H__


#include <vector>


class CViewportManager
{
public:

	static void			Init();
	static void			Terminate();

	static void			InitFrame();

	static void			BindViewport(unsigned int nViewportID);
	static unsigned int GetCurrentViewport();

	static int			NewViewport(int numViewports = 1);

	static void			SetViewport(int nViewportId, float3& Pos, float4x4& ViewProj);
	static void			SetViewportOmni(int nViewportId, float3& Pos, float fRadius);

	static void			UpdateBeforeFlush();

	static void			ComputeVisibility(ERenderList ePacketList);

	static bool			IsVisible(unsigned int nViewportID, float3& Center, float fRadius);
	static bool			IsVisible(float3& viewportCenter, float4x4& frustum, float3& sphereCenter, float fRadius);

private:

	struct SViewport
	{
		float4x4	m_InvViewProj;

		float3		m_Center;
		float		m_fRadius;
	};

	static thread_local unsigned int ms_nCurrentViewport;
	static unsigned int ms_nNumViewports;

	static std::vector<SViewport>	ms_Viewports[2];

	static std::vector<SViewport>*	ms_pViewportsToSet;
	static std::vector<SViewport>*	ms_pViewportsToFlush;
};


#endif
