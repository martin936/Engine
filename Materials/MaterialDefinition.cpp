#include "Engine/Engine.h"
#include "Engine/Maths/Maths.h"
#include "Engine/Renderer/Textures/Textures.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Misc/String.h"
#include "Engine/Device/DeviceManager.h"
#include "MaterialDefinition.h"


unsigned int			CMaterial::ms_nNbMaterials = 0;
std::vector<CMaterial*> CMaterial::ms_pMaterials;
std::vector<CTexture*>	CMaterial::ms_pTextures;

BufferId				CMaterial::ms_MaterialConstantBuffer = INVALIDHANDLE;

char CMaterial::ms_cCurrentDirectory[512] = "./";



struct SMaterialConstants
{
	float4			Color;
	float4			Fresnel;

	float			Roughness;
	float			Emissive;
	float			BumpHeight;
	float			Reflectivity;

	float			Metalness;
	float			SSSProfileID;
	float			SSSRadius;
	float			SSSThickness;

	unsigned int 	DiffuseTextureID;
	unsigned int 	NormalTextureID;
	unsigned int 	InfoTextureID;
	unsigned int	SSSTextureID;
};



CMaterial::CMaterial(const char* pcName)
{
	m_nID				= (unsigned int)ms_pMaterials.size();

	m_Color				= 0.f;
	m_Fresnel			= 0.f;

	m_Roughness			= 1.f;
	m_Emissive			= 0.f;
	m_BumpHeight		= 2.f;
	m_Reflectivity		= 0.5f;

	m_Metalness			= 0;
	m_SSSProfileID		= 0;
	m_SSSRadius			= 0.f;
	m_SSSThickness		= 0.f;

	m_DiffuseTextureID	= INVALIDHANDLE;
	m_NormalTextureID	= INVALIDHANDLE;
	m_InfoTextureID		= INVALIDHANDLE;
	m_SSSTextureID		= INVALIDHANDLE;

	m_eRenderType = e_Deferred;

	GetShaderHook("Default");

	ms_pMaterials.push_back(this);
}


CMaterial::~CMaterial()
{
}



CMaterial* CMaterial::GetMaterial(const char* pcName)
{
	std::vector<CMaterial*>::iterator it_mat;

	for (it_mat = ms_pMaterials.begin(); it_mat < ms_pMaterials.end(); it_mat++)
	{
		if (!strcmp(pcName, (*it_mat)->m_cName))
		{
			return *it_mat;
		}
	}

	return new CMaterial(pcName, pcName);
}


void CMaterial::Init()
{
	SetDirectory("../Data/Models/Misc/");
	GetMaterial("None");
}


void CMaterial::Terminate()
{
	std::vector<CMaterial*>::iterator it_mat;
	std::vector<CTexture*>::iterator it_tex;

	for (it_mat = ms_pMaterials.begin(); it_mat < ms_pMaterials.end(); it_mat++)
		delete (*it_mat);

	for (it_tex = ms_pTextures.begin(); it_tex < ms_pTextures.end(); it_tex++)
		delete (*it_tex);

	ms_pMaterials.clear();
	ms_pTextures.clear();

	ms_nNbMaterials = 0;
}


void CMaterial::BuildConstantBuffer()
{
	int numMaterials = (int)ms_pMaterials.size();
	size_t align = CResourceManager::GetConstantBufferOffsetAlignment();

	size_t bufferSize = (sizeof(SMaterialConstants) + MAX(1, align) - 1) & ~(MAX(1, align) - 1);

	char* materialData = new char[bufferSize * numMaterials];	

	for (int i = 0; i < numMaterials; i++)
	{
		SMaterialConstants buffer;
		buffer.Color			= ms_pMaterials[i]->m_Color;
		buffer.Fresnel			= ms_pMaterials[i]->m_Fresnel;
		buffer.Roughness		= ms_pMaterials[i]->m_Roughness;
		buffer.Emissive			= ms_pMaterials[i]->m_Emissive;
		buffer.BumpHeight		= ms_pMaterials[i]->m_BumpHeight;
		buffer.Reflectivity		= ms_pMaterials[i]->m_Reflectivity;
		buffer.Metalness		= ms_pMaterials[i]->m_Metalness * 1.f;
		buffer.SSSProfileID		= ms_pMaterials[i]->m_SSSProfileID * 1.f;
		buffer.SSSRadius		= ms_pMaterials[i]->m_SSSRadius;
		buffer.SSSThickness		= ms_pMaterials[i]->m_SSSThickness;
		buffer.DiffuseTextureID = ms_pMaterials[i]->m_DiffuseTextureID;
		buffer.NormalTextureID	= ms_pMaterials[i]->m_NormalTextureID;
		buffer.InfoTextureID	= ms_pMaterials[i]->m_InfoTextureID;
		buffer.SSSTextureID		= ms_pMaterials[i]->m_SSSTextureID;

		memcpy(materialData + i * bufferSize, &buffer, sizeof(SMaterialConstants));
	}

	ms_MaterialConstantBuffer = CResourceManager::CreatePermanentConstantBuffer(materialData, bufferSize * numMaterials);

	CLightField::InitRenderPasses();
}



void CMaterial::UpdateConstantBuffer(int nMatID)
{
	size_t align			= CResourceManager::GetConstantBufferOffsetAlignment();
	size_t bufferSize		= (sizeof(SMaterialConstants) + MAX(1, align) - 1) & ~(MAX(1, align) - 1);

	SMaterialConstants buffer;
	buffer.Color			= ms_pMaterials[nMatID]->m_Color;
	buffer.Fresnel			= ms_pMaterials[nMatID]->m_Fresnel;
	buffer.Roughness		= ms_pMaterials[nMatID]->m_Roughness;
	buffer.Emissive			= ms_pMaterials[nMatID]->m_Emissive;
	buffer.BumpHeight		= ms_pMaterials[nMatID]->m_BumpHeight;
	buffer.Reflectivity		= ms_pMaterials[nMatID]->m_Reflectivity;
	buffer.Metalness		= ms_pMaterials[nMatID]->m_Metalness * 1.f;
	buffer.SSSProfileID		= ms_pMaterials[nMatID]->m_SSSProfileID * 1.f;
	buffer.SSSRadius		= ms_pMaterials[nMatID]->m_SSSRadius;
	buffer.SSSThickness		= ms_pMaterials[nMatID]->m_SSSThickness;
	buffer.DiffuseTextureID = ms_pMaterials[nMatID]->m_DiffuseTextureID;
	buffer.NormalTextureID	= ms_pMaterials[nMatID]->m_NormalTextureID;
	buffer.InfoTextureID	= ms_pMaterials[nMatID]->m_InfoTextureID;
	buffer.SSSTextureID		= ms_pMaterials[nMatID]->m_SSSTextureID;

	CResourceManager::UpdateConstantBuffer(ms_MaterialConstantBuffer, &buffer, sizeof(SMaterialConstants), nMatID * bufferSize);
}


void CMaterial::BindMaterialBuffer(unsigned int nSlot)
{
	CResourceManager::SetConstantBuffer(nSlot, ms_MaterialConstantBuffer, sizeof(SMaterialConstants));
}


void CMaterial::BindMaterialTextures(unsigned int nSlot)
{
	CResourceManager::SetTextures(nSlot, ms_pTextures);
}


void CMaterial::BindMaterial(unsigned int nSlot, unsigned int nMatId)
{
	size_t align = CResourceManager::GetConstantBufferOffsetAlignment();

	size_t bufferSize = (sizeof(SMaterialConstants) + MAX(1, align) - 1) & ~(MAX(1, align) - 1);

	CResourceManager::SetConstantBufferOffset(nSlot, nMatId * bufferSize);
}


void CMaterial::GetName(char* cName)
{
	strncpy(cName, m_cName, 256);
}



void CMaterial::GetRenderType(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, pcName))
	{
		CString::CropValue(cValue, str);

		if (!strcmp(cValue, "Deferred"))
			*((CMaterial::ERenderType*)pParam) = CMaterial::e_Deferred;

		else if (!strcmp(cValue, "Forward"))
			*((CMaterial::ERenderType*)pParam) = CMaterial::e_Forward;

		else if (!strcmp(cValue, "Mixed"))
			*((CMaterial::ERenderType*)pParam) = CMaterial::e_Mixed;
	}
}


void CMaterial::ReadShaderHook(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, pcName))
	{
		CString::CropValue((char*)pParam, str);
	}
}


void CMaterial::GetTexture(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";
	char Directory[512] = "";

	CMaterial::GetDirectory(Directory);

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, "texture"))
	{
		CString::CropHead(cHead, ptr);

		if (!strcmp(cHead, pcName))
		{
			CString::CropValue(cValue, str);

			unsigned int* nID = ((unsigned int*)pParam);

			char cPath[512] = "";

			sprintf(cPath, "%s%s", Directory, cValue);

			bool bSRGB = false;

			if (nID == &m_DiffuseTextureID)
			{
				sprintf(m_cDiffuseTexturePath, cPath, 512);
				bSRGB = true;
			}

			else if (nID == &m_NormalTextureID)
				sprintf(m_cNormalTexturePath, cPath, 512);

			else if (nID == &m_InfoTextureID)
				sprintf(m_cInfoTexturePath, cPath, 512);

			FILE* pFile = fopen(cPath, "r");

			if (pFile == NULL)
			{
				char Filename[1024] = "";
				strncpy(Filename, cPath, 1024);

				char* ptr = Filename + strlen(Filename) - 1;

				while (*ptr != '.' && ptr > Filename)
					ptr--;

				ASSERT(ptr != nullptr);

				strcpy(ptr, ".dds");

				pFile = fopen(Filename, "r");

				if (pFile == NULL)
					return;
			}

			fclose(pFile);

			CTexture* pTexture = CTextureInterface::GetTexture(CTextureInterface::LoadTexture(cPath, bSRGB));

			*((unsigned int*)pParam) = static_cast<unsigned int>(CMaterial::ms_pTextures.size());

			CMaterial::ms_pTextures.push_back(pTexture);
		}
	}
}


void CMaterial::GetVector(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";
	float fValue = 0.f;

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, "float4"))
	{
		CString::CropHead(cHead, ptr);

		if (!strcmp(cHead, pcName))
		{
			if (!CString::CropValue(cValue, str))
				return;

			ptr = strchr(str, '(');
			if (ptr == NULL)
				return;

			ptr++;

			for (int i = 0; i < 4; i++)
			{
				CString::CropHead(cHead, ptr);

				if (sscanf(cHead, "%f", &fValue) > 0)
					*(&((float4*)pParam)->x + i) = fValue;

				ptr = strchr(ptr, ',');
				if (ptr == NULL)
					return;

				ptr++;
			}
		}
	}
}


void CMaterial::GetFloat(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";
	float fValue = 0.f;

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, "float"))
	{
		CString::CropHead(cHead, ptr);

		if (!strcmp(cHead, pcName))
		{
			CString::CropValue(cValue, str);

			if (sscanf(cValue, "%f", &fValue) > 0)
				*((float*)pParam) = fValue;
		}
	}
}


void CMaterial::GetBool(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, "bool"))
	{
		CString::CropHead(cHead, ptr);

		if (!strcmp(cHead, pcName))
		{
			CString::CropValue(cValue, str);

			if (strstr(cValue, "true"))
				*((bool*)pParam) = true;

			else
				*((bool*)pParam) = false;
		}
	}
}


void CMaterial::GetInt(char* str, void* pParam, const char* pcName)
{
	char cHead[256] = "";
	char cValue[256] = "";
	int nValue = 0;

	char* ptr = CString::CropHead(cHead, str);

	if (!strcmp(cHead, "int"))
	{
		CString::CropHead(cHead, ptr);

		if (!strcmp(cHead, pcName))
		{
			CString::CropValue(cValue, str);

			if (sscanf(cValue, "%d", &nValue) > 0)
				*((int*)pParam) = nValue;
		}
	}
}



void CMaterial::GetParameter(char* str, void* pParam, EParamType eType, const char* pcName)
{
	switch (eType)
	{
	case e_RenderType:
		GetRenderType(str, pParam, pcName);
		break;

	case e_ShaderHook:
		ReadShaderHook(str, pParam, pcName);
		break;

	case e_TextureID:
		GetTexture(str, pParam, pcName);
		break;

	case e_float4:
		GetVector(str, pParam, pcName);
		break;

	case e_bool:
		GetBool(str, pParam, pcName);
		break;

	case e_float:
		GetFloat(str, pParam, pcName);
		break;

	case e_int:
		GetInt(str, pParam, pcName);
		break;

	default:
		break;
	}
}



CMaterial::CMaterial(const char* pcName, const char* pFileName) : CMaterial(pcName)
{
	sprintf(m_cFullPath, "%s%s.mat", ms_cCurrentDirectory, pFileName);
	sprintf(m_cName, pcName);

	printf("%s : %s\n", pcName, m_cFullPath);

	FILE* pFile = fopen(m_cFullPath, "r");
	if (pFile == NULL)
		return;

	char str[256] = "";
	char ShaderHookName[256] = "";

	while (fgets(str, 256, pFile) != NULL)
	{
		GetParameter(str, &m_eRenderType, e_RenderType, "RenderType");
		GetParameter(str, ShaderHookName, e_ShaderHook, "ShaderHook");

		GetParameter(str, &m_DiffuseTextureID,	e_TextureID,	"Diffuse");
		GetParameter(str, &m_NormalTextureID,	e_TextureID,	"Normal");
		GetParameter(str, &m_InfoTextureID,		e_TextureID,	"Info");

		GetParameter(str, &m_Color,				e_float4,		"DiffuseColor");
		GetParameter(str, &m_Fresnel,			e_float4,		"SpecColor");

		GetParameter(str, &m_Roughness,			e_float,	"Roughness");
		GetParameter(str, &m_Metalness,			e_int,		"Metalness");
		GetParameter(str, &m_SSSRadius,			e_float,	"SSSRadius");
		GetParameter(str, &m_SSSProfileID,		e_int,		"SSSProfileID");
	}

	GetShaderHook(ShaderHookName);

	fclose(pFile);
}


void CMaterial::ReloadMaterial(int nMatID)
{
	ms_pMaterials[nMatID]->Reload();
}



void CMaterial::Reload()
{
	FILE* pFile = fopen(m_cFullPath, "r");
	if (pFile == NULL)
		return;

	char str[256] = "";
	char ShaderHookName[256] = "";

	while (fgets(str, 256, pFile) != NULL)
	{
		GetParameter(str, &m_eRenderType, e_RenderType, "RenderType");
		GetParameter(str, ShaderHookName, e_ShaderHook, "ShaderHook");

		GetParameter(str, &m_DiffuseTextureID, e_TextureID, "Diffuse");
		GetParameter(str, &m_NormalTextureID, e_TextureID, "Normal");
		GetParameter(str, &m_InfoTextureID, e_TextureID, "Info");

		GetParameter(str, &m_Color, e_float4, "DiffuseColor");
		GetParameter(str, &m_Fresnel, e_float4, "SpecColor");

		GetParameter(str, &m_Roughness, e_float, "Roughness");
		GetParameter(str, &m_Metalness, e_int, "Metalness");
		GetParameter(str, &m_SSSRadius, e_float, "SSSRadius");
		GetParameter(str, &m_SSSProfileID, e_int, "SSSProfileID");
	}

	GetShaderHook(ShaderHookName);

	fclose(pFile);
}



const char* CMaterial::GetMaterialName(int nMatID)
{
	return ms_pMaterials[nMatID]->m_cName;
}



const char* CMaterial::GetMaterialFullPath(int nMatID)
{
	return ms_pMaterials[nMatID]->m_cFullPath;
}



void CMaterial::WriteParameter(FILE* pFile, void* pParam, EParamType eType, const char* pcName)
{
	switch (eType)
	{
	case e_TextureID:
		fprintf(pFile, "texture %s = %s\n", pcName, (char*)pParam);
		break;

	case e_float4:
		fprintf(pFile, "float4 %s = (%f, %f, %f, %f)\n", pcName, ((float4*)pParam)->x, ((float4*)pParam)->y, ((float4*)pParam)->z, ((float4*)pParam)->w);
		break;

	case e_float:
		fprintf(pFile, "float %s = %f\n", pcName, *((float*)pParam));
		break;

	case e_bool:
		fprintf(pFile, "bool %s = %s\n", pcName, *((bool*)pParam) ? "true" : "false");
		break;

	case e_int:
		fprintf(pFile, "int %s = %d\n", pcName, *((int*)pParam));
		break;

	case e_RenderType:
		if ((*(EParamType*)pParam) == e_Forward)
			fprintf(pFile, "RenderType = Forward\n");

		else if ((*(EParamType*)pParam) == e_Mixed)
			fprintf(pFile, "RenderType = Mixed\n");

		else if ((*(EParamType*)pParam) == e_Deferred)
			fprintf(pFile, "RenderType = Deferred\n");
		break;

	default:
		break;
	}
}



void CMaterial::ExportMaterial(int nMatID, const char* pDirectory)
{
	ms_pMaterials[nMatID]->Export(pDirectory);
}



void CMaterial::Export(const char* pDirectory)
{
	char cFullPath[1024] = "";

	if (pDirectory != NULL)
		sprintf_s(cFullPath, "%s%s.mat", pDirectory, m_cName);

	else
		strcpy(cFullPath, m_cFullPath);

	FILE* pFile;
	fopen_s(&pFile, cFullPath, "w+");
	if (pFile == NULL)
		return;

	char cName[512] = "";

	CString::GetFileName(cName, m_cDiffuseTexturePath);

	if (strnlen_s(cName, 256) > 1)
		WriteParameter(pFile, &cName, e_TextureID, "Diffuse");

	CString::GetFileName(cName, m_cNormalTexturePath);

	if (strnlen_s(cName, 256) > 1)
		WriteParameter(pFile, &cName, e_TextureID, "Normal");

	CString::GetFileName(cName, m_cInfoTexturePath);

	if (strnlen_s(cName, 256) > 1)
		WriteParameter(pFile, &cName, e_TextureID, "Info");

	WriteParameter(pFile, &m_DiffuseTextureID, e_float4, "DiffuseColor");
	WriteParameter(pFile, &m_Fresnel, e_float4, "SpecColor");

	WriteParameter(pFile, &m_Roughness, e_float, "Roughness");
	WriteParameter(pFile, &m_Metalness, e_int, "Metalness");

	WriteParameter(pFile, &m_SSSProfileID, e_int, "SSSProfileID");

	WriteParameter(pFile, &m_SSSRadius, e_float, "SSSRadius");

	WriteParameter(pFile, &m_eRenderType, e_RenderType, "RenderType");

	fclose(pFile);
}

