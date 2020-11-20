#ifndef __MATERIAL_DEFINITION_H__
#define __MATERIAL_DEFINITION_H__

#include "Engine/Renderer/Textures/TextureInterface.h"
#include "Engine/Device/ResourceManager.h"
#include <vector>


_declspec(align(32)) class Packet;


#define MATERIAL_PATH "../Data/Materials/"

__declspec(align(32)) class CMaterial
{
	friend class CMaterialEditor;
public:

	CMaterial(const char* pcName);
	CMaterial(const char* pcName, const char* pFileName);
	~CMaterial();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void Export(const char* pDirectory = nullptr);

	static void ExportMaterial(int nMatID, const char* pDirectory = nullptr);

	void GetName(char* cName);

	enum ERenderType
	{
		e_Deferred = 1,
		e_Forward = 2,
		e_Mixed = 4
	};

	static std::vector<CTexture*>	ms_pTextures;
	static std::vector<CMaterial*>	ms_pMaterials;

	static void Init();
	static void Terminate();

	static void ReloadMaterial(int nMatID);

	static const char* GetMaterialName(int nMatID);
	static const char* GetMaterialFullPath(int nMatID);

	static CMaterial* GetMaterial(const char* pcName);

	static void BuildConstantBuffer();
	static void UpdateConstantBuffer(int nMatID);

	static void BindMaterialBuffer(unsigned int nSlot);
	static void BindMaterial(unsigned int nSlot, unsigned int nMatId);

	static void BindMaterialTextures(unsigned int nSlot);

	static BufferId GetMaterialBuffer()
	{
		return ms_MaterialConstantBuffer;
	}

	static int	GetNumTextures()
	{
		return static_cast<int>(ms_pTextures.size());
	}

	inline static void SetDirectory(const char* pcDirectory)
	{
		strncpy(ms_cCurrentDirectory, pcDirectory, 512);
	}

	inline static void GetDirectory(char* pcDirectory)
	{
		strncpy(pcDirectory, ms_cCurrentDirectory, 512);
	}

	inline ERenderType GetRenderType() const
	{
		return m_eRenderType;
	}

	inline void SetRenderType(ERenderType eType)
	{
		m_eRenderType = eType;
	}

	inline unsigned int GetID() const
	{
		return m_nID;
	}

	void Reload();

	int(*m_DeferredShaderHook)(Packet* packet, void* p_pShaderData);
	int(*m_ForwardShaderHook)(Packet* packet, void* p_pShaderData);

private:

	enum EParamType
	{
		e_int,
		e_float,
		e_bool,
		e_float4,
		e_TextureID,
		e_RenderType,
		e_ShaderHook
	};

	void			GetShaderHook(const char* pcName);

	void			WriteParameter(FILE* pFile, void* pParam, EParamType eType, const char* pcName);
	void			GetParameter(char* str, void* pParam, EParamType eType, const char* pcName);
	void			GetInt(char* str, void* pParam, const char* pcName);
	void			GetFloat(char* str, void* pParam, const char* pcName);
	void			GetBool(char* str, void* pParam, const char* pcName);
	void			GetVector(char* str, void* pParam, const char* pcName);
	void			GetTexture(char* str, void* pParam, const char* pcName);
	void			ReadShaderHook(char* str, void* pParam, const char* pcName);
	void			GetRenderType(char* str, void* pParam, const char* pcName);

	ERenderType		m_eRenderType;

	static unsigned int				ms_nNbMaterials;

	unsigned int	m_nID;
	char			m_cName[256];
	char			m_cFullPath[1024];

	static char		ms_cCurrentDirectory[512];

	char			m_cDiffuseTexturePath[512];
	char			m_cNormalTexturePath[512];
	char			m_cInfoTexturePath[512];

	float4			m_Color;
	float4			m_Fresnel;

	float			m_Roughness;
	float			m_Emissive;
	float			m_BumpHeight;
	float			m_Reflectivity;

	unsigned int	m_Metalness;
	unsigned int	m_SSSProfileID;
	float			m_SSSRadius;
	float			m_SSSThickness;

	unsigned int 	m_DiffuseTextureID;
	unsigned int 	m_NormalTextureID;
	unsigned int 	m_InfoTextureID;
	unsigned int	m_SSSTextureID;

	static BufferId	ms_MaterialConstantBuffer;
};



#endif
