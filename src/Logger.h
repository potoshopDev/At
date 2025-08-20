#pragma once

#include <string>

namespace Logger {

	enum class Level {
		Info,
		Warn,
		Error
	};
	

	void Init();
	void SetLogger(bool value);
    void Log(Level lvl, const std::string& message);
}

#define LOG_INFO(msg) Logger::Log(Logger::Level::Info, msg)
#define LOG_WARN(msg) Logger::Log(Logger::Level::Warn, msg)
#define LOG_ERROR(msg) Logger::Log(Logger::Level::Error, msg)
