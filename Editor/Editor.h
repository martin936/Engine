#ifndef __EDITOR_INC__
#define __EDITOR_INC__

#include <vector>
#include "Externalized/Externalized.h"
#include "LevelDesign/Toolbox.h"


class CEditorItem
{
	friend class CEditor;
public:

	CEditorItem();
	~CEditorItem();

	virtual void Update();
	virtual void Draw();
	virtual void Select();
	virtual void Add() {};
	virtual void Delete() {};
	virtual void Duplicate() {};

	virtual bool ShouldGrab(float* p_fItemDist) { return false; };

	virtual void Copy(const CEditorItem& pItem);

	inline void SetPosition(const float3& p_Pos)
	{
		m_Position = p_Pos;
	}

	inline void SetScale(const float3& p_Scale)
	{
		m_Scale = p_Scale;
	}

	inline void SetRotation(const float3 p_Axis[3])
	{
		for (int i = 0; i < 3; i++)
			m_Axis[i] = p_Axis[i];
	}

	inline float3 GetPosition() const
	{
		return m_Position;
	}

	inline float3 GetAxis(int index) const
	{
		return m_Axis[index];
	}

	inline float3 GetScale() const
	{
		return m_Scale;
	}

	inline void GetRotation(float3* p_Axis) const
	{
		for (int i = 0; i < 3; i++)
			p_Axis[i] = m_Axis[i];
	}

protected:

	float3	m_Position;
	float3	m_Scale;
	float3	m_Axis[3];

	bool	m_bIsMovable;
	bool	m_bIsScalable;
	bool	m_bHidden;

	static std::vector<CEditorItem*>	ms_pItemList;
};


class CEditor
{
	friend CEditorItem;
	friend CLightItem;
public:

	static void Init();
	static void Terminate();

	static void ClearAll();
	static void ClearHistory();
	static void Process();

	static void Draw();
	static void DrawFeatures();

	static void UpdateBeforeFlush();

	enum EUndoEventType
	{
		e_UndoEvent_Modify
	};

	struct SUndoEvent
	{
		EUndoEventType	m_eType;
		CEditorItem&	m_ItemRef;
		CEditorItem*	m_SavedState;

		SUndoEvent(CEditorItem& ItemRef, const CEditorItem& pStateToSave) : m_ItemRef(ItemRef)
		{
			m_eType = e_UndoEvent_Modify;
			m_SavedState = new CEditorItem(pStateToSave);
		};
	};

	enum ETransformReferential
	{
		e_TransformRef_Global,
		e_TransformRef_Local
	};

	enum ETransformType
	{
		e_Transform_Translate,
		e_Transform_Rotate,
		e_Transform_Scale,
		e_Last
	};

protected:

	static ETransformType				ms_eTransformType;
	static ETransformReferential		ms_eTransformRef;

	static std::vector<SUndoEvent>		ms_History;

	static CEditorItem*					ms_pSelectedItem;
	static CEditorItem					ms_pSelectedItemSavedState;

	static int							ms_nSelectedAxis;
	static float3						ms_SavedMouseRay;

	static float						ms_fGizmoLength;
	static bool							ms_bIsUnderModification;
	static bool							ms_bIsAxisGrabbed;

	static void							Undo();

	static void							ProcessMouse();
	static void							ProcessKeyboard();

	static bool							HasSelectionChanged();

	static bool							GrabItem();
	static bool							ProcessSelectionGizmo();
	static void							DrawSelectionGizmo();

	static void							DrawTransformHelper();

	static bool							GrabAxis(const float3& RayOrigin, const float3& RayDir);
	static void							TranslateSelection(const float3& RayOrigin, const float3& RayDir);
	static void							RotateSelection(const float3& RayOrigin, const float3& RayDir);
	static void							ScaleSelection(const float3& RayOrigin, const float3& RayDir);

	static void							DrawTranslationGizmo();
	static void							DrawRotationGizmo();
	static void							DrawScaleGizmo();

	static bool							MouseSphereIntersection(const float3& wsSphereCenter, float ssSphereRadius, float* fDist);
	static bool							MouseSegmentIntersection(const float3& wsP1, const float3& wsP2, float ssMaxDist, float* fDist);
	static bool							MouseCircleIntersection(const float3& wsCenter, const float3& wsNormal, float wsRadius, float ssMaxDist, float* fDist);
	static bool							RayPlaneIntersection(const float3& RayOrigin, const float3& RayDir, const float3& Point, const float3& Normal, float* fDist);
	static float						ClosestDistanceBetweenLines(const float3& P0, const float3& u, const float3& Q0, const float3& v, float* t, float* s);
	static void							GetMouseRay(float3& RayOrigin, float3& RayDir);

};


#endif
