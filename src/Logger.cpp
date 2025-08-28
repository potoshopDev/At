#include "Logger.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <format>
#include <filesystem>
#include <print>
#include <mutex>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "winsystem.h"

#include <string>
#include <locale>
#include <codecvt>

namespace Logger {
    inline void closeFile(std::ofstream* file)
    {
        if (file->is_open()) {
            file->close();
        }
        delete file;
    }

    using uniq_file = std::unique_ptr<std::ofstream, void(*)(std::ofstream*)>;
    uniq_file logFile(nullptr, closeFile);
    inline std::mutex logMutex;
    inline bool isLoggerOn{ true };

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
        case Level::Success: return "success";
        case Level::Warn: return "warn";
        case Level::Error: return "error";
        }
        return "unknown";
    }

    std::string LevelToColor(Level lvl) {
        switch (lvl) {
        case Level::Success: return "\033[32m";
        case Level::Warn: return "\033[33m";
        case Level::Error: return "\033[31m";
        }
        return "\033[0m";
    }

    void Init() {
        namespace fs = std::filesystem;

        const auto fullPathToApp{ win::getFullPath("") };
        const auto pathToLogFolder{ fullPathToApp / L"log" };

        fs::create_directories(pathToLogFolder);

        const auto fullPathToLog{ pathToLogFolder / L"log.log" };

        logFile = uniq_file(
            new std::ofstream(fullPathToLog, std::ios::app | std::ios::binary),
            closeFile
        );
        if (!logFile->is_open())
            throw std::runtime_error("Не удалось открыть файл лога");

        // По умолчанию используем чистый UTF-8 (без BOM)

        SetConsoleOutputCP(CP_UTF8);
    }

    void SetLogger(bool value)
    {
        isLoggerOn = value;
    }

    void Log(Level lvl, const std::string& message) {
        if (!isLoggerOn) return;

        std::lock_guard guard(logMutex);
        const auto ts = CurrentTimestamp();
        const auto lvlStr = LevelToString(lvl);
        const auto color = LevelToColor(lvl);
        const auto formatted = "[" + ts + "] <" + lvlStr + "> " + message;

        // В файл (чистый UTF-8)
        *logFile << formatted << "\n";
        logFile->flush();

        // В консоль
        std::print("{}{}\033[0m\n", color, formatted);
    }
}
