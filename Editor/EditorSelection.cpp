#include <iostream>
#include "Editor.h"
#include "Engine/Imgui/imgui.h"
#include "Engine/Inputs/Inputs.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/DebugDraw/DebugDraw.h"


#define GIZMO_LENGTH_SCREEN 0.08f


int		CEditor::ms_nSelectedAxis = -1;
float3	CEditor::ms_SavedMouseRay;


bool CEditor::MouseSphereIntersection(const float3& SphereCenter, float ssSphereRadius, float* fDist)
{
	float4x4 ViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	float4 p = ViewProj * float4(SphereCenter, 1.f);
	p = (p / p.w) * 0.5f;
	p.x += 0.5f;
	p.y = 0.5f - p.y;

	CMouse* pMouse = CMouse::GetCurrent();

	float pos[2];
	pMouse->GetPos(&pos[0], &pos[1]);

	float3 Eye = CRenderer::GetCurrentCamera()->GetPosition();

	float d2 = (pos[0] - p.x) * (pos[0] - p.x) + (pos[1] - p.y) * (pos[1] - p.y);

	if (d2 < ssSphereRadius * ssSphereRadius)
	{
		*fDist = sqrtf(d2);
		return true;
	}

	return false;
}


bool CEditor::RayPlaneIntersection(const float3& RayOrigin, const float3& RayDir, const float3& Point, const float3& Normal, float* fDist)
{
	float UdotN = float3::dotproduct(RayDir, Normal);

	if (fabs(UdotN) < 1e-3f)
		return false;

	float t = float3::dotproduct(Point - RayOrigin, Normal) / UdotN;

	if (t > 0.f)
	{
		*fDist = t;
		return true;
	}

	return false;
}


bool CEditor::MouseSegmentIntersection(const float3& wsP1, const float3& wsP2, float ssMaxDist, float* fDist)
{
	float4x4 ViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	float4 p1 = ViewProj * float4(wsP1, 1.f);
	p1 = (p1 / p1.w) * 0.5f;
	p1.x += 0.5f;
	p1.y = 0.5f - p1.y;

	float4 p2 = ViewProj * float4(wsP2, 1.f);
	p2 = (p2 / p2.w) * 0.5f;
	p2.x += 0.5f;
	p2.y = 0.5f - p2.y;

	CMouse* pMouse = CMouse::GetCurrent();

	float pos[2];
	pMouse->GetPos(&pos[0], &pos[1]);

	float pa[2] = { pos[0] - p1.x, pos[1] - p1.y };
	float ba[2] = { p2.x - p1.x, p2.y - p1.y };

	float h = (pa[0] * pa[0] + pa[1] * pa[1]) / (ba[0] * ba[0] + ba[1] * ba[1]);
	h = CLAMP(h, 0.f, 1.f);

	float d[2] = { pa[0] - ba[0] * h, pa[1] - ba[1] * h};

	float d2 = d[0] * d[0] + d[1] * d[1];

	if (d2 < ssMaxDist * ssMaxDist)
	{
		*fDist = sqrtf(d2);
		return true;
	}

	return false;
}


bool CEditor::MouseCircleIntersection(const float3& wsCenter, const float3& wsNormal, float wsRadius, float ssMaxDist, float* fDist)
{
	float3 points[16];
	float angle = 0.f;

	float3 ex(1.f, 0.f, 0.f);

	if (float3::dotproduct(ex, wsNormal) > 0.95f)
		ex = float3(0.f, 1.f, 0.f);

	float3 ey = float3::normalize(float3::cross(wsNormal, ex));
	ex = float3::cross(ey, wsNormal);

	for (int i = 0; i < 16; i++)
	{
		angle = 2.f * i * 3.1415926f / 16.f;
		points[i] = wsCenter + wsRadius * (cosf(angle) * ex + sinf(angle) * ey);
	}

	bool bIntersect = false;
	float minDist = 1e8f;

	for (int i = 0; i < 16; i++)
	{
		float dist = 1e8f;
		bIntersect = bIntersect || MouseSegmentIntersection(points[i], points[(i + 1) % 16], ssMaxDist, &dist);

		if (dist < minDist)
			minDist = dist;
	}

	*fDist = minDist;

	return bIntersect;
}


float CEditor::ClosestDistanceBetweenLines(const float3& P0, const float3& u, const float3& Q0, const float3& v, float* t, float* s)
{
	float b = float3::dotproduct(u, v);
	float d = float3::dotproduct(u, P0 - Q0);
	float e = float3::dotproduct(v, P0 - Q0);

	*t = (b * e - d) / (1.f - b * b);
	*s = (e - b * d) / (1.f - b * b);

	return (P0 + *t * u - Q0 - *s * v).length();
}


void CEditor::GetMouseRay(float3& RayOrigin, float3& RayDir)
{
	CMouse* pMouse = CMouse::GetCurrent();

	float pos[2];
	pMouse->GetPos(&pos[0], &pos[1]);

	pos[0] = pos[0] * 2.f - 1.f;
	pos[1] = 1.f - pos[1] * 2.f;

	RayOrigin = CRenderer::GetCurrentCamera()->GetPosition();
	float4x4 InvViewProj	= CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();	

	float4 p = InvViewProj * float4(pos[0], pos[1], 0.5f, 1.f);
	p /= p.w;

	RayDir = float3(p.x, p.y, p.z) - RayOrigin;
	RayDir.normalize();
}



bool CEditor::GrabItem()
{
	float fMinDist = 1e8f;
	float t = 0.f;

	CEditorItem* pItem = nullptr;

	std::vector<CEditorItem*>::iterator it;

	for (it = CEditorItem::ms_pItemList.begin(); it < CEditorItem::ms_pItemList.end(); it++)
	{
		if ((*it)->ShouldGrab(&t) && t < fMinDist)
		{
			pItem = *it;
			fMinDist = t;
		}
	}

	if (pItem != nullptr)
	{
		pItem->Select();
		return true;
	}

	return false;
}



bool CEditor::GrabAxis(const float3& RayOrigin, const float3& RayDir)
{
	float t = 0.f;
	float minT = 1e8f;

	float3 Axis[3];

	if (ms_eTransformRef == e_TransformRef_Local || ms_eTransformType == e_Transform_Scale)
	{
		Axis[0] = ms_pSelectedItem->GetAxis(0);
		Axis[1] = ms_pSelectedItem->GetAxis(1);
		Axis[2] = ms_pSelectedItem->GetAxis(2);
	}

	else
	{
		Axis[0] = float3(1.f, 0.f, 0.f);
		Axis[1] = float3(0.f, 1.f, 0.f);
		Axis[2] = float3(0.f, 0.f, 1.f);
	}

	if (MouseSphereIntersection(ms_pSelectedItem->GetPosition(), GIZMO_LENGTH_SCREEN, &t))
	{
		if (ms_eTransformType == e_Transform_Translate || ms_eTransformType == e_Transform_Scale)
		{
			if (MouseSegmentIntersection(ms_pSelectedItem->GetPosition(), ms_pSelectedItem->GetPosition() + ms_fGizmoLength * Axis[0], 0.02f, &t))
			{
				minT = t;
				ms_nSelectedAxis = 0;
			}

			if (MouseSegmentIntersection(ms_pSelectedItem->GetPosition(), ms_pSelectedItem->GetPosition() + ms_fGizmoLength * Axis[1], 0.02f, &t))
			{
				if (t < minT)
				{
					minT = t;
					ms_nSelectedAxis = 1;
				}
			}

			if (MouseSegmentIntersection(ms_pSelectedItem->GetPosition(), ms_pSelectedItem->GetPosition() + ms_fGizmoLength * Axis[2], 0.02f, &t))
			{
				if (t < minT)
				{
					minT = t;
					ms_nSelectedAxis = 2;
				}
			}
		}

		else if (ms_eTransformType == e_Transform_Rotate)
		{
			if (MouseCircleIntersection(ms_pSelectedItem->GetPosition(), Axis[0], ms_fGizmoLength, 0.01f, &t))
			{
				if (t < minT)
				{
					minT = t;
					ms_nSelectedAxis = 0;
				}
			}

			if (MouseCircleIntersection(ms_pSelectedItem->GetPosition(), Axis[1], ms_fGizmoLength, 0.01f, &t))
			{
				if (t < minT)
				{
					minT = t;
					ms_nSelectedAxis = 1;
				}
			}

			if (MouseCircleIntersection(ms_pSelectedItem->GetPosition(), Axis[2], ms_fGizmoLength, 0.01f, &t))
			{
				if (t < minT)
				{
					minT = t;
					ms_nSelectedAxis = 2;
				}
			}
		}

		return true;
	}

	return false;
}



bool CEditor::ProcessSelectionGizmo()
{
	float3 RayOrigin, RayDir;

	GetMouseRay(RayOrigin, RayDir);

	if (ms_nSelectedAxis < 0)
		ms_bIsAxisGrabbed = false;

	if (!ms_bIsAxisGrabbed)
	{
		if (!GrabAxis(RayOrigin, RayDir))
		{
			ms_nSelectedAxis = -1;
			return false;
		}

		ms_bIsAxisGrabbed = true;
		ms_SavedMouseRay = RayDir;

		return true;
	}

	else
	{
		switch (ms_eTransformType)
		{
		case e_Transform_Translate:
			TranslateSelection(RayOrigin, RayDir);
			break;

		case e_Transform_Rotate:
			RotateSelection(RayOrigin, RayDir);
			break;

		case e_Transform_Scale:
			ScaleSelection(RayOrigin, RayDir);
			break;

		default:
			break;
		}

		return true;
	}
}



void CEditor::TranslateSelection(const float3& RayOrigin, const float3& RayDir)
{
	float3 Axis = float3(0.f, 0.f, 0.f);

	if (ms_eTransformRef == e_TransformRef_Local)
		Axis = ms_pSelectedItem->GetAxis(ms_nSelectedAxis);

	else
		Axis.v()[ms_nSelectedAxis] = 1.f;

	float3 Center = ms_pSelectedItemSavedState.GetPosition();

	float t, s, s_ref;

	ClosestDistanceBetweenLines(RayOrigin, RayDir, Center, Axis, &t, &s);
	ClosestDistanceBetweenLines(RayOrigin, ms_SavedMouseRay, Center, Axis, &t, &s_ref);

	Center += (s - s_ref) * Axis;

	ms_pSelectedItem->SetPosition(Center);
}


void CEditor::RotateSelection(const float3& RayOrigin, const float3& RayDir)
{
	float fAngle = 0.f;

	float3 Axis = float3(0.f, 0.f, 0.f);

	if (ms_eTransformRef == e_TransformRef_Local)
		Axis = ms_pSelectedItem->GetAxis(ms_nSelectedAxis);

	else
		Axis.v()[ms_nSelectedAxis] = 1.f;

	float3 Center = ms_pSelectedItemSavedState.GetPosition();

	float t = 0.f;

	if (!RayPlaneIntersection(RayOrigin, ms_SavedMouseRay, Center, Axis, &t))
		return;

	float3 RefVector = float3::normalize(RayOrigin + t * ms_SavedMouseRay - Center);

	if (!RayPlaneIntersection(RayOrigin, RayDir, Center, Axis, &t))
		return;

	float3 Vector = float3::normalize(RayOrigin + t * RayDir - Center);

	fAngle = sign(float3::dotproduct(float3::cross(RefVector, Vector), Axis)) * acosf(clamp(float3::dotproduct(RefVector, Vector), -0.999999f, 0.999999f));

	float3 NewAxis[3];

	if (ms_eTransformRef == e_TransformRef_Local)
	{
		float3 XToRotate = ms_pSelectedItemSavedState.GetAxis((ms_nSelectedAxis + 1) % 3);
		float3 YToRotate = ms_pSelectedItemSavedState.GetAxis((ms_nSelectedAxis + 2) % 3);

		float3 X = cosf(fAngle) * XToRotate + sinf(fAngle) * YToRotate;
		float3 Y = cosf(fAngle) * YToRotate - sinf(fAngle) * XToRotate;

		NewAxis[ms_nSelectedAxis] = Axis;
		NewAxis[(ms_nSelectedAxis + 1) % 3] = X;
		NewAxis[(ms_nSelectedAxis + 2) % 3] = Y;
	}

	else
	{
		float3 XToRotate = float3(0.f, 0.f, 0.f);
		float3 YToRotate = float3(0.f, 0.f, 0.f);

		XToRotate.v()[(ms_nSelectedAxis + 1) % 3] = 1.f;
		YToRotate.v()[(ms_nSelectedAxis + 2) % 3] = 1.f;

		float3 X = cosf(fAngle) * XToRotate + sinf(fAngle) * YToRotate;
		float3 Y = cosf(fAngle) * YToRotate - sinf(fAngle) * XToRotate;

		float3 localAxis;
		for (int i = 0; i < 3; i++)
		{
			localAxis = ms_pSelectedItem->GetAxis(i);
			NewAxis[i] = float3::dotproduct(localAxis, Axis) * Axis + float3::dotproduct(localAxis, X) * X + float3::dotproduct(localAxis, Y) * Y;
		}
	}

	ms_pSelectedItem->SetRotation(NewAxis);
}


void CEditor::ScaleSelection(const float3& RayOrigin, const float3& RayDir)
{
	float3 Axis = ms_pSelectedItem->GetAxis(ms_nSelectedAxis);
	float3 Scale = ms_pSelectedItemSavedState.GetScale();

	float3 Center = ms_pSelectedItemSavedState.GetPosition();

	float t, s, s_ref;

	ClosestDistanceBetweenLines(RayOrigin, RayDir, Center, Axis, &t, &s);
	ClosestDistanceBetweenLines(RayOrigin, ms_SavedMouseRay, Center, Axis, &t, &s_ref);

	Scale.v()[ms_nSelectedAxis] *= s / MAX(1e-3f, s_ref);

	ms_pSelectedItem->SetScale(Scale);
}


void CEditor::DrawSelectionGizmo()
{
	DrawTransformHelper();

	float3 Eye = CRenderer::GetCurrentCamera()->GetPosition();
	float fov = CRenderer::GetCurrentCamera()->GetFOV();

	float3 pos = ms_pSelectedItem->GetPosition();

	float d = (Eye - pos).length();

	ms_fGizmoLength = GIZMO_LENGTH_SCREEN * d * tanf(fov * 3.1415926f / 360.f);

	switch (ms_eTransformType)
	{
	case e_Transform_Translate:
		DrawTranslationGizmo();
		break;

	case e_Transform_Rotate:
		DrawRotationGizmo();
		break;

	case e_Transform_Scale:
		DrawScaleGizmo();
		break;

	default:
		break;
	}
}


void CEditor::DrawTransformHelper()
{
	ImGui::SetNextWindowBgAlpha(0.3f);

	if (ImGui::Begin("Transform Helper"))
	{
		ImGui::Text("Referential : ");
		ImGui::SameLine();
		ImGui::RadioButton("Global", (int*)&ms_eTransformRef, e_TransformRef_Global);
		ImGui::SameLine();
		ImGui::RadioButton("Local", (int*)&ms_eTransformRef, e_TransformRef_Local);

		ImGui::Separator();

		ImGui::Text("Axis : ");
		ImGui::SameLine();
		ImGui::RadioButton("X", &ms_nSelectedAxis, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Y", &ms_nSelectedAxis, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Z", &ms_nSelectedAxis, 2);

		float fValue = 0.f;

		static bool bModified = false;

		if (ms_eTransformType == e_Transform_Translate)
		{
			float3 Pos = ms_pSelectedItem->GetPosition();
			float3 SavedPos = ms_pSelectedItemSavedState.GetPosition();
			float3 Axis = float3(0.f, 0.f, 0.f);

			if (ms_nSelectedAxis >= 0)
			{
				if (ms_eTransformRef == e_TransformRef_Local)
					Axis = ms_pSelectedItem->GetAxis(ms_nSelectedAxis);

				else
					Axis.v()[ms_nSelectedAxis] = 1.f;

				fValue = float3::dotproduct(Pos, Axis) - float3::dotproduct(SavedPos, Axis);
			}

			if (ImGui::InputFloat("Translation", &fValue) && ms_nSelectedAxis >= 0)
			{
				Pos = SavedPos + fValue * Axis;

				ms_pSelectedItem->SetPosition(Pos);
				ms_pSelectedItem->Update();

				bModified = true;
			}

			else if (bModified && !ImGui::GetIO().WantCaptureKeyboard)
			{
				if (fValue != 0.f)
				{
					SUndoEvent Event(*ms_pSelectedItem, ms_pSelectedItemSavedState);

					ms_History.push_back(Event);

					ms_pSelectedItemSavedState.Copy(*ms_pSelectedItem);
				}

				bModified = false;
			}
		}

		else if (ms_eTransformType == e_Transform_Rotate)
		{
			float3	Normal;
			float3	XAxis;
			float3	SavedXAxis;
			float3	SavedYAxis;

			if (ms_nSelectedAxis >= 0)
			{
				Normal = ms_pSelectedItem->GetAxis(ms_nSelectedAxis);
				XAxis = ms_pSelectedItem->GetAxis((ms_nSelectedAxis + 1) % 3);
				SavedXAxis = ms_pSelectedItemSavedState.GetAxis((ms_nSelectedAxis + 1) % 3);
				SavedYAxis = ms_pSelectedItemSavedState.GetAxis((ms_nSelectedAxis + 2) % 3);

				fValue = sign(float3::dotproduct(float3::cross(SavedXAxis, XAxis), Normal)) * acosf(clamp(float3::dotproduct(SavedXAxis, XAxis), -0.999999f, 0.999999f)) * (180.f / 3.1415926f);
			}

			if (ImGui::InputFloat("Angle", &fValue) && ms_nSelectedAxis >= 0)
			{
				fValue *= (3.1415926f / 180.f);

				float3 X = cosf(fValue) * SavedXAxis + sinf(fValue) * SavedYAxis;
				float3 Y = cosf(fValue) * SavedYAxis - sinf(fValue) * SavedXAxis;

				float3 NewAxis[3];
				NewAxis[ms_nSelectedAxis] = Normal;
				NewAxis[(ms_nSelectedAxis + 1) % 3] = X;
				NewAxis[(ms_nSelectedAxis + 2) % 3] = Y;

				ms_pSelectedItem->SetRotation(NewAxis);
				ms_pSelectedItem->Update();
				bModified = true;
			}

			else if (bModified && !ImGui::GetIO().WantCaptureKeyboard)
			{
				if (fValue != 0.f)
				{
					SUndoEvent Event(*ms_pSelectedItem, ms_pSelectedItemSavedState);

					ms_History.push_back(Event);

					ms_pSelectedItemSavedState.Copy(*ms_pSelectedItem);
				}

				bModified = false;
			}
		}

		else if (ms_eTransformType == e_Transform_Scale)
		{
			float3 Scale = ms_pSelectedItem->GetScale();
			float3 SavedScale = ms_pSelectedItemSavedState.GetScale();

			fValue = Scale.v()[ms_nSelectedAxis] / MAX(1e-3f, SavedScale.v()[ms_nSelectedAxis]);

			if (ImGui::InputFloat("Scale", &fValue) && ms_nSelectedAxis >= 0)
			{
				Scale.v()[ms_nSelectedAxis] = fValue * SavedScale.v()[ms_nSelectedAxis];

				ms_pSelectedItem->SetScale(Scale);
				ms_pSelectedItem->Update();

				bModified = true;
			}

			else if (bModified && !ImGui::GetIO().WantCaptureKeyboard)
			{
				if (fValue != 0.f)
				{
					SUndoEvent Event(*ms_pSelectedItem, ms_pSelectedItemSavedState);

					ms_History.push_back(Event);

					ms_pSelectedItemSavedState.Copy(*ms_pSelectedItem);
				}

				bModified = false;
			}
		}
	}

	ImGui::End();
}


void CEditor::DrawTranslationGizmo()
{
	float3 Center = ms_pSelectedItem->GetPosition();
	float3 XAxis;
	float3 YAxis;
	float3 ZAxis;

	if (ms_eTransformRef == e_TransformRef_Local)
	{
		XAxis = ms_pSelectedItem->GetAxis(0);
		YAxis = ms_pSelectedItem->GetAxis(1);
		ZAxis = ms_pSelectedItem->GetAxis(2);
	}

	else
	{
		XAxis = float3(1.f, 0.f, 0.f);
		YAxis = float3(0.f, 1.f, 0.f);
		ZAxis = float3(0.f, 0.f, 1.f);
	}

	float4 colorX = ms_nSelectedAxis == 0 ? float4(1.f, 1.f, 1.f, 1.f) : float4(1.f, 0.f, 0.f, 1.f);
	float4 colorY = ms_nSelectedAxis == 1 ? float4(1.f, 1.f, 1.f, 1.f) : float4(0.f, 1.f, 0.f, 1.f);
	float4 colorZ = ms_nSelectedAxis == 2 ? float4(1.f, 1.f, 1.f, 1.f) : float4(0.f, 0.f, 1.f, 1.f);

	CDebugDraw::DrawVector(Center, XAxis, ms_fGizmoLength, colorX);
	CDebugDraw::DrawVector(Center, YAxis, ms_fGizmoLength, colorY);
	CDebugDraw::DrawVector(Center, ZAxis, ms_fGizmoLength, colorZ);
}


void CEditor::DrawScaleGizmo()
{
	float3 Center = ms_pSelectedItem->GetPosition();
	float3 XAxis = ms_pSelectedItem->GetAxis(0);
	float3 YAxis = ms_pSelectedItem->GetAxis(1);
	float3 ZAxis = ms_pSelectedItem->GetAxis(2);

	float4 colorX = ms_nSelectedAxis == 0 ? float4(1.f, 1.f, 1.f, 1.f) : float4(1.f, 0.f, 0.f, 1.f);
	float4 colorY = ms_nSelectedAxis == 1 ? float4(1.f, 1.f, 1.f, 1.f) : float4(0.f, 1.f, 0.f, 1.f);
	float4 colorZ = ms_nSelectedAxis == 2 ? float4(1.f, 1.f, 1.f, 1.f) : float4(0.f, 0.f, 1.f, 1.f);

	CDebugDraw::DrawVector(Center, XAxis, ms_fGizmoLength, colorX);
	CDebugDraw::DrawCircle(Center + ms_fGizmoLength * XAxis, XAxis, ms_fGizmoLength * 0.25f, colorX);

	CDebugDraw::DrawVector(Center, YAxis, ms_fGizmoLength, colorY);
	CDebugDraw::DrawCircle(Center + ms_fGizmoLength * YAxis, YAxis, ms_fGizmoLength * 0.25f, colorY);

	CDebugDraw::DrawVector(Center, ZAxis, ms_fGizmoLength, colorZ);
	CDebugDraw::DrawCircle(Center + ms_fGizmoLength * ZAxis, ZAxis, ms_fGizmoLength * 0.25f, colorZ);
}


void CEditor::DrawRotationGizmo()
{
	float3 Center = ms_pSelectedItem->GetPosition();
	float3 XAxis;
	float3 YAxis;
	float3 ZAxis;

	if (ms_eTransformRef == e_TransformRef_Local)
	{
		XAxis = ms_pSelectedItem->GetAxis(0);
		YAxis = ms_pSelectedItem->GetAxis(1);
		ZAxis = ms_pSelectedItem->GetAxis(2);
	}

	else
	{
		XAxis = float3(1.f, 0.f, 0.f);
		YAxis = float3(0.f, 1.f, 0.f);
		ZAxis = float3(0.f, 0.f, 1.f);
	}

	float4 colorX = ms_nSelectedAxis == 0 ? float4(1.f, 1.f, 1.f, 1.f) : float4(1.f, 0.f, 0.f, 1.f);
	float4 colorY = ms_nSelectedAxis == 1 ? float4(1.f, 1.f, 1.f, 1.f) : float4(0.f, 1.f, 0.f, 1.f);
	float4 colorZ = ms_nSelectedAxis == 2 ? float4(1.f, 1.f, 1.f, 1.f) : float4(0.f, 0.f, 1.f, 1.f);

	CDebugDraw::DrawVector(Center, XAxis, ms_fGizmoLength, colorX);
	CDebugDraw::DrawCircle(Center, XAxis, ms_fGizmoLength, colorX);

	CDebugDraw::DrawVector(Center, YAxis, ms_fGizmoLength, colorY);
	CDebugDraw::DrawCircle(Center, YAxis, ms_fGizmoLength, colorY);

	CDebugDraw::DrawVector(Center, ZAxis, ms_fGizmoLength, colorZ);
	CDebugDraw::DrawCircle(Center, ZAxis, ms_fGizmoLength, colorZ);
}
