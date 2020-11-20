#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Editor/LightEditor/LightEditor.h"
#include "Engine/Renderer/Window/Window.h"
#include "Config.h"
#include "String.h"


enum EParamType
{
	e_Bool,
	e_Float,
	e_float3,
	e_LightType
};


struct SNewLightDesc
{
	CLight::ELightType	m_eType;
	float				m_fStrength;
	float				m_fInAngle;
	float				m_fOutAngle;

	float				m_fAreaSize;
	float3				m_Position;

	float				m_fRadius;
	float3				m_Direction;

	float3				m_Color;
	bool				m_bCastShadows;

	char				Padding[3];

	void Reset()
	{
		m_eType			= CLight::e_Omni;
		m_fStrength		= 1.f;
		m_fInAngle		= 30.f;
		m_fOutAngle		= 60.f;
		m_fAreaSize		= 1.f;
		m_fRadius		= 1.f;
		m_Position		= 0.f;
		m_Direction		= float3(0.f, 0.f, -1.f);
		m_Color			= float3(1.f, 1.f, 1.f);
		m_bCastShadows	= false;
	}
};


struct SNewCameraDesc
{
	float3	m_Position;
	float3	m_Target;
	float3	m_Up;

	float	m_fNearPlane;
	float	m_fFarPlane;
	float	m_fFOV;

	void Reset()
	{
		m_Position		= 0.f;
		m_Target		= float3(0.f, 1.f, 0.f);
		m_Up			= float3(0.f, 0.f, 1.f);

		m_fNearPlane	= 0.1f;
		m_fFarPlane		= 1000.f;
		m_fFOV			= 50.f;
	}
};



void GetLightType(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, pcName))
	{
		CString::CropValue(cValue, str);

		if (!strcmp(cValue, "Sun"))
			*(CLight::ELightType*)pParam = CLight::e_Sun;

		else if (!strcmp(cValue, "Point"))
			*(CLight::ELightType*)pParam = CLight::e_Omni;

		else if (!strcmp(cValue, "Spot"))
			*(CLight::ELightType*)pParam = CLight::e_Spot;
	}
}



void GetFloat(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";
	float fValue = 0.f;

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, pcName))
	{
		CString::CropValue(cValue, str);

		if (sscanf(cValue, "%f", &fValue) > 0)
			*((float*)pParam) = fValue;
	}
}



void GetBool(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";
	float fValue = 0.f;

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, pcName))
	{
		CString::CropValue(cValue, str);

		if (strstr(cValue, "true"))
			*((bool*)pParam) = true;

		else
			*((bool*)pParam) = false;
	}
}



void Getfloat3(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";
	float fValue = 0.f;

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, pcName))
	{
		if (!CString::CropValue(cValue, str))
			return;

		ptr = strchr(str, '(');
		if (ptr == NULL)
			return;

		ptr++;

		for (int i = 0; i < 3; i++)
		{
			CString::CropHead(cHead, ptr);

			if (sscanf(cHead, "%f", &fValue) > 0)
				((float3*)pParam)->v()[i] = fValue;

			ptr = strchr(ptr, ',');
			if (ptr == NULL)
				return;

			ptr++;
		}
	}
}



void GetParameter(char* str, void* pParam, EParamType eType, const char* pcName)
{
	switch (eType)
	{
	case e_LightType:
		GetLightType(str, pParam, pcName);
		break;

	case e_float3:
		Getfloat3(str, pParam, pcName);
		break;

	case e_Float:
		GetFloat(str, pParam, pcName);
		break;

	case e_Bool:
		GetBool(str, pParam, pcName);
		break;

	default:
		break;
	}
}



void AddLamp(SNewLightDesc* pDesc)
{
	CLight* pLight = CLightsManager::AddLight(pDesc->m_eType, pDesc->m_bCastShadows);

	switch (pDesc->m_eType)
	{
	case CLight::e_Sun:
		((CSunLight*)pLight)->Init(pDesc->m_Position, pDesc->m_Direction, pDesc->m_fStrength, pDesc->m_Color, pDesc->m_fRadius);
		break;

	case CLight::e_Omni:
		((COmniLight*)pLight)->Init(pDesc->m_Position, pDesc->m_fStrength, pDesc->m_Color, pDesc->m_fRadius);
		break;

	case CLight::e_Spot:
		((CSpotLight*)pLight)->Init(pDesc->m_Position, pDesc->m_Direction, pDesc->m_fStrength, pDesc->m_Color, pDesc->m_fInAngle, pDesc->m_fOutAngle, pDesc->m_fRadius);
		break;

	default:
		break;
	}
}


void AddCamera(SNewCameraDesc* pDesc)
{
	float fAspectRatio = CWindow::GetMainWindow()->GetAspectRatio();

	CRenderer::GetCurrentCamera()->SetCamera(pDesc->m_fFOV, fAspectRatio, pDesc->m_fNearPlane, pDesc->m_fFarPlane, pDesc->m_Up, pDesc->m_Position, pDesc->m_Target);
}


void Config::LoadLights(const char* pcFile)
{
	SNewLightDesc lightDesc;

	bool bLightPending	= false;

	FILE* pFile = fopen(pcFile, "r");
	if (pFile == NULL)
		return;

	char str[1024] = "";

	while (fgets(str, 1024, pFile) != NULL)
	{
		if (strstr(str, "New Light") != NULL)
		{
			if (bLightPending)
			{
				AddLamp(&lightDesc);
			}

			lightDesc.Reset();
			bLightPending = true;
		}

		GetParameter(str, &lightDesc.m_eType,			e_LightType,	"Type");
		GetParameter(str, &lightDesc.m_Position,		e_float3,		"Position");
		GetParameter(str, &lightDesc.m_Direction,		e_float3,		"Direction");
		GetParameter(str, &lightDesc.m_Color,			e_float3,		"Color");
		GetParameter(str, &lightDesc.m_fRadius,			e_Float,		"Radius");
		GetParameter(str, &lightDesc.m_fStrength,		e_Float,		"Strength");
		GetParameter(str, &lightDesc.m_fInAngle,		e_Float,		"InAngle");
		GetParameter(str, &lightDesc.m_fOutAngle,		e_Float,		"OutAngle");
		GetParameter(str, &lightDesc.m_fAreaSize,		e_Float,		"AreaSize");
		GetParameter(str, &lightDesc.m_bCastShadows,	e_Bool,			"CastShadows");
	}

	if (bLightPending)
		AddLamp(&lightDesc);

	CLightEditor::TriggerLightReload();

	fclose(pFile);
}


void Config::LoadCameras(const char* pcFile)
{
	SNewCameraDesc camDesc;

	bool bCamPending = false;

	FILE* pFile = fopen(pcFile, "r");
	if (pFile == NULL)
		return;

	char str[1024] = "";

	while (fgets(str, 1024, pFile) != NULL)
	{
		if (strstr(str, "New Camera") != NULL)
		{
			if (bCamPending)
			{
				AddCamera(&camDesc);
			}

			camDesc.Reset();
			bCamPending = true;
		}

		GetParameter(str, &camDesc.m_Position,		e_float3,	"Position");
		GetParameter(str, &camDesc.m_Target,		e_float3,	"Target");
		GetParameter(str, &camDesc.m_Up,			e_float3,	"Up");
		GetParameter(str, &camDesc.m_fNearPlane,	e_Float,	"NearPlane");
		GetParameter(str, &camDesc.m_fFarPlane,		e_Float,	"FarPlane");
		GetParameter(str, &camDesc.m_fFOV,			e_Float,	"FOV");
	}

	if (bCamPending)
		AddCamera(&camDesc);

	fclose(pFile);
}