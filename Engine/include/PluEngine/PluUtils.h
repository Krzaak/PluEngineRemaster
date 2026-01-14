//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_PLUUTILS_H
#define PLUENGINE_PLUUTILS_H

#include "Core.h"
#include "PluSTL_FWD.h"
#ifdef PLU_PLATFORM_WINDOWS
#error "Add mena and lean"
	#include <windows.h>
#endif


namespace Plu
{
	inline PathW GetExePath()
	{
#if defined(PLU_PLATFORM_WINDOWS)

		wchar_t buffer[MAX_PATH];
		DWORD size = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

		if (size == 0) {
			throw std::runtime_error("GetModuleFileNameW failed");
		}

		return std::filesystem::path(buffer);

#elif defined(PLU_PLATFORM_LINUX)

		char buffer[PATH_MAX];
		ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

		if (len == -1) {
			throw std::runtime_error("readlink(/proc/self/exe) failed");
		}

		buffer[len] = '\0';
		return PathW(StringW::FromNarrow(buffer));

#endif
	}
}

#endif //PLUENGINE_PLUUTILS_H