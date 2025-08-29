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

std::vector<std::string> GetTest(const std::wstring& argv)
{
	fs::path scriptsDir = argv;

	// Читаем все .txt файлы
	std::vector<std::string> args;
	for (auto& p : fs::recursive_directory_iterator(scriptsDir)) {
		if (p.is_regular_file() && p.path().extension() == L".txt") {
			std::ifstream in(p.path());
			std::string line;
			while (std::getline(in, line)) {
				args.emplace_back(line);
			}
		}
	}

	return args;
}
