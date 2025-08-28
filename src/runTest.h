#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include <tlhelp32.h>

// ���������� ������ PID �� ����� ��������
std::vector<DWORD> FindProcess(const std::wstring& processName);

// ��������� ������� �� PID
bool KillProcess(DWORD pid);

std::vector<std::string> GetTest(const std::string& argv);
