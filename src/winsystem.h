#pragma once

#include <filesystem>
#include <string>

namespace win
{
	std::filesystem::path getFullPath(const std::wstring& filename);
	std::filesystem::path getFullPath(const std::string& filename);
	std::filesystem::path getFullPath2(const std::filesystem::path& filename);
	std::wstring utf8_to_wstring(const std::string& utf8_string);
	std::string to_utf8(const std::wstring& wstr);
	std::string WideToUtf8(const std::wstring& w);
}
