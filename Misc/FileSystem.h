#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <stdio.h>


namespace FileSystem
{
	// Initialize the file system. Walks up from the exe location to find the
	// project root (a directory containing a "Data" subfolder). Call once at
	// program startup, before any file loading.
	void Init();

	// Returns the project root (directory containing Data/) without trailing slash.
	// Returns an empty string until Init() has been called successfully.
	const char* GetRoot();

	// Returns the directory containing Engine source/data (defaults to
	// <root>/extern/Engine, which matches the submodule layout used by
	// PotatoesArmageddon). Without trailing slash.
	const char* GetEngineRoot();

	// Resolves a logical path to an absolute filesystem path.
	//   - Paths starting with "Data/" are resolved against the project root.
	//   - Paths starting with "Engine/" are resolved against the engine root.
	//   - Paths starting with "Shaders/" are resolved against the exe directory
	//     (shaders ship next to the exe, so the binary is self-contained).
	//   - Absolute paths (drive letter or leading slash) are returned unchanged.
	//   - All other paths are returned unchanged (caller-managed / local working files).
	//
	// The returned pointer is valid until the next call to ResolvePath on the
	// same thread. Callers that need to keep the value should copy it.
	const char* ResolvePath(const char* pcLogicalPath);

	// fopen wrapper that calls ResolvePath on the path.
	FILE* FOpen(const char* pcLogicalPath, const char* pcMode);

	// fopen_s wrapper that calls ResolvePath on the path.
	errno_t FOpenS(FILE** ppFile, const char* pcLogicalPath, const char* pcMode);
};


#endif
