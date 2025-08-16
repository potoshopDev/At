#include "Logger.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <format>
#include <filesystem>
#include <print>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "winsystem.h"


#include <string>
#include <locale>
#include <codecvt>

namespace Logger {
	inline std::ofstream logFile;
	inline std::mutex logMutex;


	std::string CurrentTimestamp() {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		struct tm buf;
		localtime_s(&buf, &in_time_t);
		return std::format("{:02}-{:02}-{:04} {:02}:{:02}", buf.tm_mday, buf.tm_mon + 1, buf.tm_year + 1900, buf.tm_hour, buf.tm_min);
	}

	std::string LevelToString(Level lvl) {
		switch (lvl) {
		case Level::Info: return "info";
		case Level::Warn: return "warn";
		case Level::Error: return "error";
		}
		return "unknown";
	}

	std::string LevelToColor(Level lvl) {
		switch (lvl) {
		case Level::Info: return "\033[32m";
		case Level::Warn: return "\033[33m";
		case Level::Error: return "\033[31m";
		}
		return "\033[0m";
	}

	void Init() {
		namespace fs = std::filesystem;
		const auto fullPathToApp{ fs::current_path() };
		fs::create_directories(fullPathToApp / "log");

		const auto pathToLog("log/log.log");
		const auto fullPathToLog{ fullPathToApp / pathToLog };

		// Открываем файл в бинарном режиме и записываем BOM для UTF-8
		logFile.open(fullPathToLog, std::ios::app | std::ios::binary);
		if (!logFile.is_open()) {
			throw std::runtime_error("Не удалось открыть файл лога");
		}

		// Если файл пустой, записываем BOM
		logFile.seekp(0, std::ios::end);
		if (logFile.tellp() == 0) {
			const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
			logFile.write(reinterpret_cast<const char*>(bom), sizeof(bom));
		}

		SetConsoleOutputCP(CP_UTF8);
	}

	void Log(Level lvl, const std::string& message) {
		std::lock_guard guard(logMutex);
		auto ts = CurrentTimestamp();
		auto lvlStr = LevelToString(lvl);
		auto color = LevelToColor(lvl);
		auto formatted = "[" + ts + "] <" + lvlStr + "> " + message;

		// В файл
		logFile << formatted << "\n";
		logFile.flush();

		// В консоль
		std::cout << color << formatted << "\033[0m" << std::endl;
	}
}



//namespace Logger {
//
//	inline std::ofstream logFile;
//	inline std::mutex logMutex;
//
//	inline std::string CurrentTimestamp();
//	inline std::string LevelToString(Level lvl);
//	inline std::string LevelToColor(Level lvl);
//
//	void Init() {
//		namespace fs = std::filesystem;
//		const auto fullPathToApp{ win::getFullPath("") };
//
//		fs::create_directories(fullPathToApp / "log");
//
//		const auto pathToLog("log/log.log");
//		const auto fullPathToLog{ win::getFullPath(pathToLog) };
//
//		logFile.open(fullPathToLog, std::ios::app);
//
//		if (!logFile.is_open()) {
//			throw std::runtime_error("Не удалось открыть файл лога");
//		}
//
//		// Записываем BOM только если файл пустой
//		if (fs::file_size(fullPathToLog) == 0) {
//			unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
//			logFile.write(reinterpret_cast<char*>(bom), sizeof(bom));
//		}
//
//		// Включаем поддержку UTF-8 в консоли Windows
//		SetConsoleOutputCP(CP_UTF8);
//	}
//
//	std::string CurrentTimestamp() {
//		auto now = std::chrono::system_clock::now();
//		auto in_time_t = std::chrono::system_clock::to_time_t(now);
//		struct tm buf;
//		localtime_s(&buf, &in_time_t);
//
//		return std::format("{:02}-{:02}-{:04} {:02}:{:02}",
//			buf.tm_mday, buf.tm_mon + 1, buf.tm_year + 1900,
//			buf.tm_hour, buf.tm_min);
//	}
//
//	std::string LevelToString(Level lvl) {
//		switch (lvl) {
//		case Level::Info: return "info";
//		case Level::Warn: return "warn";
//		case Level::Error: return "error";
//		}
//		return "unknown";
//	}
//
//	std::string LevelToColor(Level lvl) {
//		switch (lvl) {
//		case Level::Info: return "\033[32m"; // зелёный
//		case Level::Warn: return "\033[33m"; // жёлтый
//		case Level::Error: return "\033[31m"; // красный
//		}
//		return "\033[0m";
//	}
//
//	void Log(Level lvl, const std::string& message) {
//		std::lock_guard guard(logMutex);
//		auto ts = CurrentTimestamp();
//		auto lvlStr = LevelToString(lvl);
//		auto formatted = std::format("[{}] <{}> {}", ts, lvlStr, message);
//
//		// В файл
//		logFile << formatted << std::endl;
//		logFile.flush();
//
//		// В консоль с цветом
//		std::print("{}{}\033[0m\n", LevelToColor(lvl), formatted);
//	}
//}
