#include "screenshot.h"

#include <vector>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <windows.h>
#include <chrono>
#include <format>
#include <iostream>
#include <filesystem>

#include "winsystem.h"

void CaptureFullScreenAndSave()
{
	// 1. Получаем размеры экрана
	const int w = GetSystemMetrics(SM_CXSCREEN);
	const int h = GetSystemMetrics(SM_CYSCREEN);

	// 2. DC всего экрана
	HDC hdcScreen = GetDC(nullptr);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);

	// 3. Bitmap нужного размера
	HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, w, h);
	HGDIOBJ old = SelectObject(hdcMem, hbm);

	// 4. Копируем экран в bitmap
	BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY | CAPTUREBLT);

	// 5. Читаем пиксели (BGRA → RGBA)
	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = -h; // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	std::vector<unsigned char> pixels(w * h * 4);
	GetDIBits(hdcMem, hbm, 0, h, pixels.data(), &bmi, DIB_RGB_COLORS);

	// Конвертируем BGRA → RGBA (чтобы цвета не искажались)
	for (int i = 0; i < w * h; i++) {
		std::swap(pixels[i * 4 + 0], pixels[i * 4 + 2]); // B <-> R
	}

	// 6. Имя файла с датой/временем (C++23 std::chrono + std::format)
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm local_tm;
	localtime_s(&local_tm, &t);

	namespace fs = std::filesystem;

	const auto fullPathToApp{ win::getFullPath(L"") };
	const auto pathToScreenshots{ fullPathToApp / L"log" / L"screenshots" };

	fs::create_directories(pathToScreenshots);

	std::wstring filename = std::format(L"{}\\screenshot_{:02d}-{:02d}-{:04d}_{:02d}-{:02d}-{:02d}.png",
		pathToScreenshots.wstring(),
		local_tm.tm_mday,
		local_tm.tm_mon + 1,
		local_tm.tm_year + 1900,
		local_tm.tm_hour,
		local_tm.tm_min,
		local_tm.tm_sec);

	// Конвертируем в UTF-8 для stbi_write_png
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8Filename(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, utf8Filename.data(), sizeNeeded, nullptr, nullptr);

	// 7. Сохраняем в PNG
	if (!stbi_write_png(utf8Filename.c_str(), w, h, 4, pixels.data(), w * 4)) {
		MessageBoxW(nullptr, L"Не удалось сохранить PNG", L"Error", MB_ICONERROR);
	}

	// 8. Чистим ресурсы
	SelectObject(hdcMem, old);
	DeleteObject(hbm);
	DeleteDC(hdcMem);
	ReleaseDC(nullptr, hdcScreen);
}

