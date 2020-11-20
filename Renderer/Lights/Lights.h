#ifndef __LIGHTS_H__
#define __LIGHTS_H__

#define SHADOWMAP_DEFAULT_WIDTH 2048
#define SHADOWMAP_DEFAULT_HEIGHT 2048

#define LIGHT_DEFAULT_RADIUS 10.f

#include "Engine/Maths/Maths.h"


__declspec(align(32)) class CLight
{
	friend class CShadowRenderer;
	friend class CPreviewManager;
	friend class CLightProbe;
	friend class COIT;
	friend class CLightsManager;
	friend class CLightEditor;
	friend class CLightItem;

public:

	enum ELightType
	{
		e_Ambient,
		e_Omni,
		e_Sun,
		e_Spot
	};


	struct SLightDesc
	{
		float3	m_Pos;
		float	m_fIntensity;

		float3	m_Color;
		int		m_nType;

		float3	m_Dir;
		float	m_AreaSize;

		float3	m_Up;
		unsigned int	m_LightID;

		float	m_fMinRadius;
		float	m_fMaxRadius;
		float	m_fAngleIn;
		float	m_fAngleOut;

		int		m_nIsVolumetric;
		int		m_nCastShadows;
		int		m_nCastDynamicShadows;
		int		m_nCastStaticShadows;

		int		m_nStaticShadowMapIndex;
		int		m_nDynamicShadowMapIndex;
		int		unused1;
		int		unused2;

		SLightDesc();
	};


	static void			Init();
	static void			Terminate();

	CLight(bool bCastShadow);
	~CLight();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	inline void			SetRadius(float fRadius)
	{
		m_Desc.m_fMaxRadius = fRadius;
	}

	inline bool			CastShadow() const { return m_Desc.m_nCastShadows == 1; }
	inline unsigned int	GetID() const { return m_nID; }

	virtual void		ComputeBoundingSphere() {};

	virtual void		CreateShadowRenderer() {};

	void				RenderShadowMap() const;

	CShadowRenderer*	GetShadowRenderer();

	SLightDesc			GetDesc()
	{
		return m_Desc;
	}

	void				SetDesc(SLightDesc& desc)
	{
		m_Desc = desc;
	}

	inline bool			IsVisible() const
	{
		return m_bVisible;
	}

	inline bool			IsEnabled() const
	{
		return m_bEnabled;
	}

	inline void Enable(bool bEnable)
	{
		m_bEnabled = bEnable;
	}

	virtual void		DrawScreenShadows() {};
	virtual void		Draw() {}

	inline ELightType	GetType() const { return m_Type; }

protected:

	float4x4			m_LastFrameViewProj;
	CShadowRenderer*	m_pShadowRenderer;

	unsigned int		m_nID;
	ELightType			m_Type;

	bool				m_bEnabled;
	bool				m_bVisible;

	SLightDesc			m_Desc;
};



class COmniLight : public CLight
{
	friend class CShadowOmni;
	friend class CLightEditor;

public:

	COmniLight(bool bCastShadow) : CLight(bCastShadow) 
	{	
		m_Type = CLight::e_Omni;
	}

	void Init(float3 vPos, float fStrength, float3 vColor, float fRadius);

	void CreateShadowRenderer() override;
};



class CSunLight : public CLight
{
public:

	CSunLight(bool bCastShadow) : CLight(bCastShadow) { m_Type = CLight::e_Sun; }

	void Init(float3 vPos, float3 vDir, float fStrength, float3 vColor, float fRadius);

	void CreateShadowRenderer() override;
};



class CSpotLight : public CLight
{
	friend class CLightEditor;
	friend class CLightItem;
public:

	CSpotLight(bool bCastShadow) : CLight(bCastShadow) { m_Type = CLight::e_Spot; }

	void Init(float3 vPos, float3 vDir, float fStrength, float3 vColor, float fInAngle, float fOutAngle, float fRadius);

	void CreateShadowRenderer() override;
};


#endif
