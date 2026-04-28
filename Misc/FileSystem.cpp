#include "FileSystem.h"

#include <string.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace
{
	char s_root[MAX_PATH] = "";
	char s_engineRoot[MAX_PATH] = "";
	char s_exeDir[MAX_PATH] = "";

	thread_local char s_resolveBuffer[MAX_PATH * 2];

	bool DirectoryExists(const char* pcPath)
	{
		DWORD attrs = GetFileAttributesA(pcPath);
		return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	bool HasPrefix(const char* pcPath, const char* pcPrefix)
	{
		size_t n = strlen(pcPrefix);
		if (strncmp(pcPath, pcPrefix, n) != 0)
			return false;
		char next = pcPath[n];
		return next == '/' || next == '\\';
	}

	bool IsAbsolute(const char* pcPath)
	{
		if (!pcPath || !pcPath[0])
			return false;
		if (pcPath[0] == '/' || pcPath[0] == '\\')
			return true;
		if (pcPath[1] == ':')
			return true;
		return false;
	}
}


void FileSystem::Init()
{
	char exePath[MAX_PATH];
	DWORD len = GetModuleFileNameA(NULL, exePath, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return;

	// Strip the exe filename to get the directory.
	for (DWORD i = len; i > 0; --i)
	{
		if (exePath[i - 1] == '\\' || exePath[i - 1] == '/')
		{
			exePath[i - 1] = '\0';
			break;
		}
	}

	strcpy_s(s_exeDir, exePath);

	// Walk up looking for a directory that contains a "Data" subfolder.
	char candidate[MAX_PATH];
	strcpy_s(candidate, exePath);

	for (int depth = 0; depth < 16; ++depth)
	{
		char probe[MAX_PATH];
		_snprintf_s(probe, _TRUNCATE, "%s\\Data", candidate);

		if (DirectoryExists(probe))
		{
			strcpy_s(s_root, candidate);
			break;
		}

		// Strip last path component.
		bool stripped = false;
		for (size_t i = strlen(candidate); i > 0; --i)
		{
			if (candidate[i - 1] == '\\' || candidate[i - 1] == '/')
			{
				candidate[i - 1] = '\0';
				stripped = true;
				break;
			}
		}
		if (!stripped)
			break;
	}

	// Engine root: prefer <root>/extern/Engine (submodule layout). If that
	// doesn't exist, fall back to <root> itself (single-tree layout).
	if (s_root[0])
	{
		char enginePath[MAX_PATH];
		_snprintf_s(enginePath, _TRUNCATE, "%s\\extern\\Engine", s_root);
		if (DirectoryExists(enginePath))
			strcpy_s(s_engineRoot, enginePath);
		else
			strcpy_s(s_engineRoot, s_root);
	}
}


const char* FileSystem::GetRoot()
{
	return s_root;
}


const char* FileSystem::GetEngineRoot()
{
	return s_engineRoot;
}


const char* FileSystem::ResolvePath(const char* pcLogicalPath)
{
	if (!pcLogicalPath)
		return "";

	if (IsAbsolute(pcLogicalPath))
		return pcLogicalPath;

	if (HasPrefix(pcLogicalPath, "Data") && s_root[0])
	{
		_snprintf_s(s_resolveBuffer, _TRUNCATE, "%s\\%s", s_root, pcLogicalPath);
		return s_resolveBuffer;
	}

	if (HasPrefix(pcLogicalPath, "Engine") && s_engineRoot[0])
	{
		// Strip the "Engine" prefix; engine root already points at extern/Engine.
		const char* rest = pcLogicalPath + 6; // strlen("Engine")
		_snprintf_s(s_resolveBuffer, _TRUNCATE, "%s%s", s_engineRoot, rest);
		return s_resolveBuffer;
	}

	if (HasPrefix(pcLogicalPath, "Shaders") && s_exeDir[0])
	{
		// Shaders ship next to the exe, so they work regardless of CWD or
		// install location. The path keeps its "Shaders/" prefix on disk.
		_snprintf_s(s_resolveBuffer, _TRUNCATE, "%s\\%s", s_exeDir, pcLogicalPath);
		return s_resolveBuffer;
	}

	return pcLogicalPath;
}


FILE* FileSystem::FOpen(const char* pcLogicalPath, const char* pcMode)
{
	return fopen(ResolvePath(pcLogicalPath), pcMode);
}


errno_t FileSystem::FOpenS(FILE** ppFile, const char* pcLogicalPath, const char* pcMode)
{
	return fopen_s(ppFile, ResolvePath(pcLogicalPath), pcMode);
}
