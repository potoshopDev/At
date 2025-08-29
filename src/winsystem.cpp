#include "winsystem.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <filesystem>
#include <locale>
#include <codecvt>

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
	std::filesystem::path getFullPath(const std::wstring& filename)
	{
		const fs::path exePath = fs::current_path();
		const fs::path directory = fs::canonical(GetExePath());
		return directory / filename;
	}
	std::filesystem::path getFullPath(const std::string& filename)
	{
		const fs::path exePath = fs::current_path();
		const fs::path directory = fs::canonical(GetExePath());
		return directory / filename;
	}
std::filesystem::path getFullPath2(const std::filesystem::path& filename)
{
    const fs::path directory = fs::canonical(GetExePath());
    return directory / filename;
}
	std::wstring utf8_to_wstring(const std::string& utf8_string) {
		// Используем std::wstring_convert для преобразования UTF-8 -> wide string
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(utf8_string);
	}
	std::string to_utf8(const std::wstring& wstr) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
	}
	std::string WideToUtf8(const std::wstring& w)
	{
		if (w.empty()) return {};
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
		std::string result(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), result.data(), size_needed, nullptr, nullptr);
		return result;
	}
}
