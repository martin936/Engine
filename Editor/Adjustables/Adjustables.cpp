#include "Engine/Imgui/imgui.h"
#include "Engine/Misc/FileSystem.h"
#include "Adjustables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <charconv>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <commdlg.h>
#endif


namespace
{
	// Flat registry of every adjustable. Used to commit per-frame snapshots
	// in O(n) without walking the category tree.
	std::vector<CAdjustable*>& AllAdjustables()
	{
		static std::vector<CAdjustable*> s_all;
		return s_all;
	}

	// Hash map from "<path>/<name>" full key to adjustable for O(1) loads.
	std::unordered_map<std::string, CAdjustable*>& AdjustableByKey()
	{
		static std::unordered_map<std::string, CAdjustable*> s_byKey;
		return s_byKey;
	}
}


namespace
{
	// Tracks the file last saved-to or loaded-from. Initialized lazily to the
	// absolute form of ADJUSTABLE_PATH on first access, then updated by Save As
	// and Load. Save and Reload always operate on this file.
	std::string g_sCurrentAdjustablePath;

	// Resolve a path to an absolute, native-separator string anchored against
	// the engine project root (FileSystem::GetRoot), not the process cwd.
	// Anchoring against cwd is dangerous: `Data/...` written from a process
	// launched in `Project/PotatoesArmageddon/` would create a stray `Data/`
	// folder there, and the next FileSystem::Init walk would treat that folder
	// as the project root.
	std::string MakeAbsolute(const char* pcPath)
	{
		if (pcPath == NULL || *pcPath == '\0')
			return std::string();

		// FileSystem::ResolvePath handles "Data/...", "Engine/...", "Shaders/..."
		// and absolute paths. For everything else it returns the input unchanged;
		// fall back to the process cwd in that case so user-typed paths from
		// the file dialog still work.
		const char* resolved = FileSystem::ResolvePath(pcPath);

		std::error_code ec;
		std::filesystem::path abs = std::filesystem::absolute(resolved, ec);
		if (ec)
			return std::string(resolved);

		abs.make_preferred();
		return abs.string();
	}

	const char* GetCurrentAdjustablePath()
	{
		if (g_sCurrentAdjustablePath.empty())
			g_sCurrentAdjustablePath = MakeAbsolute(ADJUSTABLE_PATH);
		return g_sCurrentAdjustablePath.c_str();
	}

	void SetCurrentAdjustablePath(const char* pcPath)
	{
		g_sCurrentAdjustablePath = MakeAbsolute(pcPath);
	}

	// Path of the current file made relative to the process working directory,
	// for compact display in the editor. Falls back to the absolute path if a
	// relative form can't be expressed (different drive, etc.).
	std::string GetDisplayablePath()
	{
		std::error_code ec;
		std::filesystem::path cwd = std::filesystem::current_path(ec);
		if (ec)
			return GetCurrentAdjustablePath();

		std::filesystem::path rel = std::filesystem::relative(GetCurrentAdjustablePath(), cwd, ec);
		if (ec || rel.empty())
			return GetCurrentAdjustablePath();

		rel.make_preferred();
		return rel.string();
	}
}


#ifdef _WIN32
namespace
{
	// Common file-filter for the OS dialogs. Each pair is a display name + a
	// pattern, separated by NULs and terminated by a double NUL.
	static const char* const kAdjustablesFilter =
		"Adjustables (*.cnf)\0*.cnf\0All Files\0*.*\0";

	// Pre-fill outPath with an absolute version of pcDefaultPath so the OS
	// dialog opens in the right directory regardless of the current cwd. For
	// the save dialog we also create the parent directory if it doesn't yet
	// exist, so the user can land directly on the intended folder.
	void PrimeDialogPath(char* outPath, size_t outPathSize, const char* pcDefaultPath, bool bCreateParent)
	{
		outPath[0] = '\0';
		if (pcDefaultPath == NULL || *pcDefaultPath == '\0')
			return;

		// Use the engine-anchored resolver so we don't inadvertently create
		// directories cwd-relative (which can land on top of the FileSystem
		// walker's project-root probe).
		const std::string abs = MakeAbsolute(pcDefaultPath);
		if (abs.empty())
			return;

		if (bCreateParent)
		{
			std::error_code ec;
			std::filesystem::path fsAbs(abs);
			if (fsAbs.has_parent_path())
				std::filesystem::create_directories(fsAbs.parent_path(), ec);
		}

		strncpy_s(outPath, outPathSize, abs.c_str(), _TRUNCATE);
	}

	bool ShowSaveAsDialog(char* outPath, size_t outPathSize, const char* pcDefaultPath)
	{
		PrimeDialogPath(outPath, outPathSize, pcDefaultPath, /*bCreateParent=*/true);

		OPENFILENAMEA ofn = {};
		ofn.lStructSize  = sizeof(ofn);
		ofn.lpstrFile    = outPath;
		ofn.nMaxFile     = (DWORD)outPathSize;
		ofn.lpstrFilter  = kAdjustablesFilter;
		ofn.nFilterIndex = 1;
		ofn.lpstrDefExt  = "cnf";
		ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
		return GetSaveFileNameA(&ofn) == TRUE;
	}

	bool ShowLoadDialog(char* outPath, size_t outPathSize, const char* pcDefaultPath)
	{
		PrimeDialogPath(outPath, outPathSize, pcDefaultPath, /*bCreateParent=*/false);

		OPENFILENAMEA ofn = {};
		ofn.lStructSize  = sizeof(ofn);
		ofn.lpstrFile    = outPath;
		ofn.nMaxFile     = (DWORD)outPathSize;
		ofn.lpstrFilter  = kAdjustablesFilter;
		ofn.nFilterIndex = 1;
		ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
		return GetOpenFileNameA(&ofn) == TRUE;
	}
}
#endif


CAdjustableCategory*	CAdjustableCategory::ms_pRoot = NULL;



CAdjustable::CAdjustable(const char* pLabel, const char* pcName, float* pVariable, float fValue, float fMin, float fMax, const char* pcSection)
{
	m_eType		= e_Float;
	m_pValue	= pVariable;
	m_pCallback	= nullptr;

	m_uMin.m_float = fMin;
	m_uMax.m_float = fMax;

	m_uDefaultValue.m_float = *(float*)m_pValue;
	m_uStaging.m_float      = *(float*)m_pValue;

	sprintf_s(m_cName, "%s", pcName);
	sprintf_s(m_cLabel, "%s", pLabel);
	m_cFullKey[0] = '\0';

	CAdjustableCategory::InsertAdjustable(this, pcSection);
}


CAdjustable::CAdjustable(const char* pLabel, const char* pcName, int* pVariable, int nValue, int nMin, int nMax, const char* pcSection)
{
	m_eType = e_Int;
	m_pValue = pVariable;
	m_pCallback = nullptr;

	m_uMin.m_int = nMin;
	m_uMax.m_int = nMax;

	m_uDefaultValue.m_int = *(int*)m_pValue;
	m_uStaging.m_int      = *(int*)m_pValue;

	sprintf_s(m_cName, "%s", pcName);
	sprintf_s(m_cLabel, "%s", pLabel);
	m_cFullKey[0] = '\0';

	CAdjustableCategory::InsertAdjustable(this, pcSection);
}


CAdjustable::CAdjustable(const char* pLabel, const char* pcName, bool* pVariable, bool bValue, bool bMin, bool bMax, const char* pcSection)
{
	m_eType = e_Bool;
	m_pValue = pVariable;
	m_pCallback = nullptr;

	m_uMin.m_bool = bMin;
	m_uMax.m_bool = bMax;

	m_uDefaultValue.m_bool = *(bool*)m_pValue;
	m_uStaging.m_bool      = *(bool*)m_pValue;

	sprintf_s(m_cName, "%s", pcName);
	sprintf_s(m_cLabel, "%s", pLabel);
	m_cFullKey[0] = '\0';

	CAdjustableCategory::InsertAdjustable(this, pcSection);
}


CAdjustable::CAdjustable(const char* pLabel, const char* pcName, void (*pCallback)(), const char* pcSection)
{
	m_eType		= e_Button;
	m_pValue	= nullptr;
	m_pCallback	= pCallback;

	sprintf_s(m_cName, "%s", pcName);
	sprintf_s(m_cLabel, "%s", pLabel);
	m_cFullKey[0] = '\0';

	CAdjustableCategory::InsertAdjustable(this, pcSection);
}


void CAdjustable::ResetValue()
{
	// Edits go through staging — the snapshot at frame start propagates them
	// to *m_pValue.
	switch (m_eType)
	{
	case e_Float: m_uStaging.m_float = m_uDefaultValue.m_float; break;
	case e_Int:   m_uStaging.m_int   = m_uDefaultValue.m_int;   break;
	case e_Bool:  m_uStaging.m_bool  = m_uDefaultValue.m_bool;  break;
	default: break;
	}
}


void CAdjustable::SetDefault()
{
	// "Current" from the user's POV is what's in the slider, i.e. staging.
	switch (m_eType)
	{
	case e_Float: m_uDefaultValue.m_float = m_uStaging.m_float; break;
	case e_Int:   m_uDefaultValue.m_int   = m_uStaging.m_int;   break;
	case e_Bool:  m_uDefaultValue.m_bool  = m_uStaging.m_bool;  break;
	default: break;
	}
}


void CAdjustable::Draw()
{
	// ImGui binds to staging so the live variable only changes at frame
	// start — see CommitFrameSnapshot.
	switch (m_eType)
	{
	case e_Bool:
		ImGui::Checkbox(m_cLabel, &m_uStaging.m_bool);
		break;

	case e_Int:
		ImGui::SliderInt(m_cLabel, &m_uStaging.m_int, m_uMin.m_int, m_uMax.m_int);
		break;

	case e_Float:
		ImGui::SliderFloat(m_cLabel, &m_uStaging.m_float, m_uMin.m_float, m_uMax.m_float);
		break;

	case e_Button:
		if (m_pCallback != nullptr && ImGui::Button(m_cLabel))
			m_pCallback();
		break;

	default:
		break;
	}
}


void CAdjustable::CommitFrameSnapshot()
{
	for (CAdjustable* p : AllAdjustables())
	{
		switch (p->m_eType)
		{
		case e_Float: *(float*)p->m_pValue = p->m_uStaging.m_float; break;
		case e_Int:   *(int*)p->m_pValue   = p->m_uStaging.m_int;   break;
		case e_Bool:  *(bool*)p->m_pValue  = p->m_uStaging.m_bool;  break;

		case e_Button:
		default:
			break;
		}
	}
}


CAdjustable* CAdjustable::Find(const char* pcFullKey)
{
	if (pcFullKey == NULL)
		return NULL;

	auto& byKey = AdjustableByKey();
	auto it = byKey.find(pcFullKey);
	return (it != byKey.end()) ? it->second : NULL;
}


void CAdjustable::WriteValue(FILE* pFile) const
{
	// We save staging (what the user sees in the slider), and we serialize
	// without locale/printf to avoid comma-decimal corruption and to keep
	// the shortest round-trip representation for floats.
	switch (m_eType)
	{
	case e_Float:
	{
		char buf[64];
		auto res = std::to_chars(buf, buf + sizeof(buf) - 1, m_uStaging.m_float);
		*res.ptr = '\0';
		fprintf(pFile, "%s=%s\n", m_cName, buf);
		break;
	}

	case e_Int:
		fprintf(pFile, "%s=%d\n", m_cName, m_uStaging.m_int);
		break;

	case e_Bool:
		fprintf(pFile, "%s=%d\n", m_cName, m_uStaging.m_bool ? 1 : 0);
		break;

	case e_Button:
	default:
		break;
	}
}


void CAdjustable::SetValueFromString(const char* pcValue)
{
	if (pcValue == NULL)
		return;

	const char* end = pcValue + strlen(pcValue);

	switch (m_eType)
	{
	case e_Float:
	{
		float v;
		auto res = std::from_chars(pcValue, end, v);
		if (res.ec == std::errc())
			m_uStaging.m_float = v;
		break;
	}

	case e_Int:
	{
		int v;
		auto res = std::from_chars(pcValue, end, v);
		if (res.ec == std::errc())
			m_uStaging.m_int = v;
		break;
	}

	case e_Bool:
	{
		int v;
		auto res = std::from_chars(pcValue, end, v);
		if (res.ec == std::errc())
			m_uStaging.m_bool = (v != 0);
		break;
	}

	case e_Button:
	default:
		break;
	}
}


bool CAdjustable::WriteAdjustables(const char* pPath)
{
	if (CAdjustableCategory::ms_pRoot == NULL || pPath == NULL)
		return false;

	// Resolve via the engine path resolver so logical paths like
	// "Data/GD/adjust.cnf" land under the project root instead of relative to
	// the process cwd.
	const std::string resolved = MakeAbsolute(pPath);
	if (resolved.empty())
		return false;

	// Make sure the parent directory exists; fopen("w") doesn't create it.
	std::error_code ec;
	std::filesystem::path fsPath(resolved);
	if (fsPath.has_parent_path())
		std::filesystem::create_directories(fsPath.parent_path(), ec);

	FILE* pFile = NULL;
	fopen_s(&pFile, resolved.c_str(), "w");
	if (pFile == NULL)
		return false;

	CAdjustableCategory::ms_pRoot->WriteRecursive(pFile, "");

	fclose(pFile);
	return true;
}


bool CAdjustable::ReadAdjustables(const char* pPath)
{
	if (pPath == NULL)
		return false;

	const std::string resolved = MakeAbsolute(pPath);
	if (resolved.empty())
		return false;

	FILE* pFile = NULL;
	fopen_s(&pFile, resolved.c_str(), "r");
	if (pFile == NULL)
		return false;

	char line[512];
	char currentPath[256] = "";

	while (fgets(line, sizeof(line), pFile) != NULL)
	{
		// Trim trailing CR/LF and surrounding whitespace.
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t'))
			line[--len] = '\0';

		char* p = line;
		while (*p == ' ' || *p == '\t') p++;

		if (*p == '\0' || *p == '#' || *p == ';')
			continue;	// blank or comment.

		if (*p == '[')
		{
			char* end = strchr(p, ']');
			if (end == NULL)
				continue;
			*end = '\0';
			strcpy_s<256>(currentPath, p + 1);
			continue;
		}

		char* eq = strchr(p, '=');
		if (eq == NULL)
			continue;

		*eq = '\0';
		const char* name  = p;
		const char* value = eq + 1;

		// Strip trailing whitespace from name.
		size_t nameLen = strlen(name);
		while (nameLen > 0 && (name[nameLen - 1] == ' ' || name[nameLen - 1] == '\t'))
			((char*)name)[--nameLen] = '\0';

		// Strip leading whitespace from value.
		while (*value == ' ' || *value == '\t') value++;

		// Build the full key for hashmap lookup: "<path>/<name>" or "<name>"
		// at root.
		char fullKey[512];
		if (currentPath[0] == '\0')
			sprintf_s(fullKey, "%s", name);
		else
			sprintf_s(fullKey, "%s/%s", currentPath, name);

		CAdjustable* pAdj = CAdjustable::Find(fullKey);
		if (pAdj != NULL)
			pAdj->SetValueFromString(value);
	}

	fclose(pFile);
	return true;
}


CAdjustableCategory::CAdjustableCategory(const char* pcName)
{
	m_pSubCategories.clear();
	m_pAdjustables.clear();

	strcpy_s<256>(m_cName, pcName);
}


CAdjustableCategory::~CAdjustableCategory()
{
	std::vector<CAdjustableCategory*>::iterator it;

	for (it = m_pSubCategories.begin(); it < m_pSubCategories.end(); it++)
		delete *it;

	m_pSubCategories.clear();
	m_pAdjustables.clear();
}


CAdjustableCategory* CAdjustableCategory::GetSubCategory(const char* pcName)
{
	std::vector<CAdjustableCategory*>::iterator it;

	for (it = m_pSubCategories.begin(); it < m_pSubCategories.end(); it++)
	{
		if (!strcmp((*it)->m_cName, pcName))
			return *it;
	}

	CAdjustableCategory* pCategory = new CAdjustableCategory(pcName);
	m_pSubCategories.push_back(pCategory);

	return pCategory;
}


void CAdjustableCategory::AddAdjustable(CAdjustable* pAdjust)
{
	m_pAdjustables.push_back(pAdjust);
}


void CAdjustableCategory::InsertAdjustable(CAdjustable* pAdjust, const char* pcPath)
{
	if (ms_pRoot == NULL)
		ms_pRoot = new CAdjustableCategory("Root");

	const bool bAtRoot = (pcPath == NULL || *pcPath == '\0');

	// Build the full key on the adjustable and register it for O(1) lookups
	// and per-frame snapshots.
	if (bAtRoot)
		sprintf_s(pAdjust->m_cFullKey, "%s", pAdjust->m_cName);
	else
		sprintf_s(pAdjust->m_cFullKey, "%s/%s", pcPath, pAdjust->m_cName);

	AdjustableByKey()[pAdjust->m_cFullKey] = pAdjust;
	AllAdjustables().push_back(pAdjust);

	// Tree insertion (used by the editor to render nested categories).
	CAdjustableCategory* pCurrentCategory = ms_pRoot;

	if (!bAtRoot)
	{
		char path[256];
		strcpy_s<256>(path, pcPath);

		char* start = path;
		char* end;

		do
		{
			end = strchr(start, '/');

			if (end != NULL)
				*end = '\0';

			pCurrentCategory = pCurrentCategory->GetSubCategory(start);

			if (end != NULL)
				start = end + 1;

		} while (end != NULL);
	}

	pCurrentCategory->AddAdjustable(pAdjust);
}


void CAdjustableCategory::DrawAll()
{
	if (ms_pRoot == NULL)
		return;

	// Set when a Save/Load operation fails so we can surface a modal dialog.
	// Static so it survives across frames until the user dismisses the popup.
	static char       s_errorMsg [768] = {};
	static const char kErrorPopupId[]  = "Adjustables I/O error";

	auto ReportError = [&](const char* op, const char* path)
	{
		_snprintf_s(s_errorMsg, sizeof(s_errorMsg), _TRUNCATE,
		            "%s failed for:\n%s\n\nCheck the path is writable and not opened by another process.",
		            op, path);
		ImGui::OpenPopup(kErrorPopupId);
	};

	if (ImGui::Button("Save"))
	{
		const char* path = GetCurrentAdjustablePath();
		if (!CAdjustable::WriteAdjustables(path))
			ReportError("Save", path);
	}
	ImGui::SameLine();

#ifdef _WIN32
	if (ImGui::Button("Save As"))
	{
		char path[MAX_PATH] = {};
		if (ShowSaveAsDialog(path, MAX_PATH, GetCurrentAdjustablePath()))
		{
			SetCurrentAdjustablePath(path);
			if (!CAdjustable::WriteAdjustables(path))
				ReportError("Save", path);
		}
	}
	ImGui::SameLine();
#endif

	if (ImGui::Button("Reload"))
	{
		const char* path = GetCurrentAdjustablePath();
		if (!CAdjustable::ReadAdjustables(path))
			ReportError("Reload", path);
	}

#ifdef _WIN32
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		char path[MAX_PATH] = {};
		if (ShowLoadDialog(path, MAX_PATH, GetCurrentAdjustablePath()))
		{
			SetCurrentAdjustablePath(path);
			if (!CAdjustable::ReadAdjustables(path))
				ReportError("Load", path);
		}
	}
#endif

	ImGui::TextDisabled("Current: %s", GetDisplayablePath().c_str());

	if (ImGui::BeginPopupModal(kErrorPopupId, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("%s", s_errorMsg);
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::Separator();

	std::vector<CAdjustableCategory*>::iterator it_cat;
	std::vector<CAdjustable*>::iterator it_adj;

	for (it_cat = ms_pRoot->m_pSubCategories.begin(); it_cat < ms_pRoot->m_pSubCategories.end(); it_cat++)
		(*it_cat)->Draw();

	if (ms_pRoot->m_pAdjustables.size() > 0)
		ImGui::Separator();

	for (it_adj = ms_pRoot->m_pAdjustables.begin(); it_adj < ms_pRoot->m_pAdjustables.end(); it_adj++)
		(*it_adj)->Draw();
}


void CAdjustableCategory::Draw()
{
	if (ImGui::TreeNode(m_cName))
	{
		std::vector<CAdjustableCategory*>::iterator it_cat;
		std::vector<CAdjustable*>::iterator it_adj;

		for (it_cat = m_pSubCategories.begin(); it_cat < m_pSubCategories.end(); it_cat++)
			(*it_cat)->Draw();

		if (m_pAdjustables.size() > 0)
			ImGui::Separator();

		for (it_adj = m_pAdjustables.begin(); it_adj < m_pAdjustables.end(); it_adj++)
			(*it_adj)->Draw();

		ImGui::TreePop();

		if (m_pAdjustables.size() > 0)
			ImGui::Separator();
	}
}


CAdjustable* CAdjustableCategory::FindAdjustable(const char* pcName)
{
	for (std::vector<CAdjustable*>::iterator it = m_pAdjustables.begin(); it < m_pAdjustables.end(); it++)
	{
		if (strcmp((*it)->m_cName, pcName) == 0)
			return *it;
	}
	return NULL;
}


CAdjustableCategory* CAdjustableCategory::FindSubCategoryByPath(const char* pcPath)
{
	if (pcPath == NULL || *pcPath == '\0')
		return this;

	char path[256];
	strcpy_s<256>(path, pcPath);

	CAdjustableCategory* pCurrent = this;

	char* start = path;
	char* end;

	do
	{
		end = strchr(start, '/');
		if (end != NULL)
			*end = '\0';

		CAdjustableCategory* pNext = NULL;
		for (std::vector<CAdjustableCategory*>::iterator it = pCurrent->m_pSubCategories.begin();
		     it < pCurrent->m_pSubCategories.end(); it++)
		{
			if (strcmp((*it)->m_cName, start) == 0)
			{
				pNext = *it;
				break;
			}
		}

		if (pNext == NULL)
			return NULL;

		pCurrent = pNext;

		if (end != NULL)
			start = end + 1;

	} while (end != NULL);

	return pCurrent;
}


CAdjustable* CAdjustableCategory::FindByPathAndName(const char* pcPath, const char* pcName)
{
	// Forwards to the O(1) hash-map lookup. Kept for source-compat with any
	// external caller that already uses this signature.
	if (pcName == NULL)
		return NULL;

	char fullKey[512];
	if (pcPath == NULL || *pcPath == '\0')
		sprintf_s(fullKey, "%s", pcName);
	else
		sprintf_s(fullKey, "%s/%s", pcPath, pcName);

	return CAdjustable::Find(fullKey);
}


void CAdjustableCategory::WriteRecursive(FILE* pFile, const char* pcPathPrefix)
{
	// Build the path under which our adjustables will be written.
	// Root has no header — its adjustables go into a leading "[]" block.
	char path[512];
	if (this == ms_pRoot)
		path[0] = '\0';
	else if (pcPathPrefix == NULL || *pcPathPrefix == '\0')
		strcpy_s<512>(path, m_cName);
	else
		sprintf_s(path, "%s/%s", pcPathPrefix, m_cName);

	// Emit header + adjustables (skipping buttons, which have no state).
	bool bHaveAny = false;
	for (std::vector<CAdjustable*>::iterator it = m_pAdjustables.begin(); it < m_pAdjustables.end(); it++)
	{
		if ((*it)->m_eType == CAdjustable::e_Button)
			continue;

		if (!bHaveAny)
		{
			if (this != ms_pRoot)
				fprintf(pFile, "[%s]\n", path);
			bHaveAny = true;
		}

		(*it)->WriteValue(pFile);
	}
	if (bHaveAny)
		fprintf(pFile, "\n");

	for (std::vector<CAdjustableCategory*>::iterator it = m_pSubCategories.begin(); it < m_pSubCategories.end(); it++)
		(*it)->WriteRecursive(pFile, path);
}
