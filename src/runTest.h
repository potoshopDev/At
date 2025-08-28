#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include <tlhelp32.h>

// Возвращает список PID по имени процесса
std::vector<DWORD> FindProcess(const std::wstring& processName);

// Завершает процесс по PID
bool KillProcess(DWORD pid);

std::vector<std::string> GetTest(const std::string& argv);
