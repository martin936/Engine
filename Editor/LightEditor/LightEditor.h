#ifndef __LIGHT_EDITOR__
#define __LIGHT_EDITOR__

#include "../Editor.h"


class CLightItem : CEditorItem
{
	friend class CLightEditor;
public:

	enum ELightType
	{
		e_LightType_Omni = 0,
		e_LightType_Spot,
		e_LightType_Sun
	};

	CLightItem();
	~CLightItem() {};

	void	Draw()		override;
	void	Select()	override;
	void	Update()	override;

	bool	ShouldGrab(float* p_fItemDist) override;

protected:

	ELightType	m_eType;
	float		m_fRadius;
	float		m_fInnerAngle;
	float		m_fOuterAngle;
	float		m_fPower;

	float3		m_Color;

	bool		m_bCastShadows;

	CLight*		m_pLight;

	static std::vector<CLightItem*>	ms_pLightList;
};



class CLightEditor
{
	friend CEditor;
	friend CLightItem;
public:

	static void Init();
	static void Terminate();

	static void Update();
	static void Draw();

	static void ClearAll();

	static void TriggerLightReload()
	{
		ms_bReloadLights = true;
	}

private:

	static void ReloadLights();

	static bool ms_bReloadLights;
	static bool ms_bActive;

	static CLightItem* ms_pSelectedItem;
};


#endif
