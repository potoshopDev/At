#include "Logger.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <format>
#include <filesystem>
#include <print>
#include <mutex>


namespace Logger {

	inline std::ofstream logFile;
	inline std::mutex logMutex;

	inline std::string CurrentTimestamp();
	inline std::string LevelToString(Level lvl);
	inline std::string LevelToColor(Level lvl);

	void Init() {
		namespace fs = std::filesystem;
		fs::create_directories("log");
		logFile.open("log/log.log", std::ios::app);

		if (!logFile.is_open()) {
			throw std::runtime_error("Не удалось открыть файл лога");
		}
	}

	std::string CurrentTimestamp() {
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		struct tm buf;
		localtime_s(&buf, &in_time_t);

		return std::format("{:02}-{:02}-{:04} {:02}:{:02}",
			buf.tm_mday, buf.tm_mon + 1, buf.tm_year + 1900,
			buf.tm_hour, buf.tm_min);
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
		case Level::Info: return "\033[32m"; // зелёный
		case Level::Warn: return "\033[33m"; // жёлтый
		case Level::Error: return "\033[31m"; // красный
		}
		return "\033[0m";
	}

	void Log(Level lvl, const std::string& message) {
		std::lock_guard guard(logMutex);
		auto ts = CurrentTimestamp();
		auto lvlStr = LevelToString(lvl);
		auto formatted = std::format("[{}] <{}> {}", ts, lvlStr, message);

		// В файл
		logFile << formatted << std::endl;
		logFile.flush();

		// В консоль с цветом
		std::print("{}{}\033[0m\n", LevelToColor(lvl), formatted);
	}
}
