#pragma once

#include <filesystem>

namespace win
{
	std::filesystem::path getFullPath(const std::string& filename);
}
