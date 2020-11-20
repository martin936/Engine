#ifndef __MATERIAL_EDITOR_H__
#define __MATERIAL_EDITOR_H__

#include <vector>


class CMaterialEditor
{
public:

	static void				Draw();
	static void				SaveAll();
	static void				ProcessMouse();

	static void				ClearAll();

	static void				UpdateBeforeFlush();

	static bool				ms_bShouldDraw;

private:

	static void				DrawMaterialSelection();
	static void				DrawMaterialProperties();

	static void				DrawHeader();
	static void				DrawTexturesSection();
	static void				DrawSpecularSection();
	static void				DrawEmissiveSection();
	static void				DrawSSSSection();
	static void				DrawTransparencySection();

	static void				RegisterTextures();

	static bool							ms_bRayCastRequested;
	static unsigned int					ms_nCurrentMatID;
	static bool 						ms_bIsCurrentMaterialModified;

	static std::vector<unsigned int>	ms_ModifiedMaterials;

	struct STextureAssociationIds
	{
		unsigned int m_nDiffuseID;
		unsigned int m_nNormalID;
		unsigned int m_nInfoID;
		unsigned int m_nFlags;
	};

	static void AddModifiedMaterial(unsigned int nMatID);
	static bool IsMaterialModified(unsigned int nMatID);
	static void RemoveModifiedMaterial(unsigned int nMatID);

	static bool ms_bShouldReset;
	static int ms_nMaterialIndex[2048];
	static std::vector<STextureAssociationIds> ms_TextureAssociation;
};


#endif
