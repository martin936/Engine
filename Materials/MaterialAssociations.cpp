#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "MaterialAssociations.h"
#include "MaterialDefinition.h"

#include <string.h>

SMaterialAssociation gs_MaterialAssociations[] =
{
	{ "Default", CDeferredRenderer::UpdateShader, CDeferredRenderer::UpdateShader }
};



void CMaterial::GetShaderHook(const char* pcName)
{
	int nSize = sizeof(gs_MaterialAssociations) / sizeof(SMaterialAssociation);

	for (int i = 0; i < nSize; i++)
	{
		if (!strcmp(gs_MaterialAssociations[i].m_cName, pcName))
		{
			m_DeferredShaderHook = gs_MaterialAssociations[i].m_DeferredShaderHook;
			m_ForwardShaderHook = gs_MaterialAssociations[i].m_ForwardShaderHook;
			
			break;
		}
	}
}
