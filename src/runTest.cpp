#include "runTest.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <format>

#include "Logger.h"
#include "winsystem.h"

namespace fs = std::filesystem;

// Возвращает список PID по имени процесса
std::vector<DWORD> FindProcess(const std::wstring& processName) {
	std::vector<DWORD> pids;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE) return pids;

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(entry);

	if (Process32FirstW(snapshot, &entry)) {
		do {
			if (processName == entry.szExeFile) {
				pids.push_back(entry.th32ProcessID);
			}
		} while (Process32NextW(snapshot, &entry));
	}
	CloseHandle(snapshot);
	return pids;
}

// Завершает процесс по PID
bool KillProcess(DWORD pid) {
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if (!hProcess) return false;
	bool result = TerminateProcess(hProcess, 1);
	CloseHandle(hProcess);

	return result;
}

void _GetTest(const fs::directory_entry& regular_file, std::vector<std::string>& args);
void _recursiveGetTest(const std::filesystem::path& scriptsDir, std::vector<std::string>& args);

void _GetTest(const std::filesystem::directory_entry& regular_file, std::vector<std::string>& args)
{
	std::ifstream in(regular_file.path());
	std::string line;
	while (std::getline(in, line)) {
		if (line.starts_with("!"))
			if (line.ends_with(".txt"))
			{
				const fs::directory_entry rf{ fs::u8path(line.substr(1)) };
				_GetTest(rf, args);
			}
			else
				_recursiveGetTest(fs::u8path(line.substr(1)), args);
		else
			args.emplace_back(line);
	}
}
void _recursiveGetTest(const std::filesystem::path& scriptsDir, std::vector<std::string>& args)
{
	for (auto& p : fs::recursive_directory_iterator(scriptsDir)) {
		if (p.is_regular_file() && p.path().extension() == L".txt") {
			_GetTest(p, args);
		}
	}
}

std::vector<std::string> GetTest(const std::wstring& argv)
{
	fs::path scriptsDir = argv;

	std::vector<std::string> args;
	_recursiveGetTest(scriptsDir, args);

	return args;
}
