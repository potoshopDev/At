#include "winsystem.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace win
{
	namespace fs = std::filesystem;

#define MAX_PATH          260
	std::filesystem::path GetExePath()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(nullptr, buffer, MAX_PATH);
		return std::filesystem::path(buffer).parent_path();
	}
	std::filesystem::path getFullPath(const std::string& filename)
	{
		const fs::path exePath = fs::current_path();
		const fs::path directory = fs::canonical(GetExePath());
		return directory / filename;
	}
}
