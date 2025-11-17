#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::initialize(const std::string& levelStr, const std::string& filepath) {
    currentLevel = stringToLevel(levelStr);

    if (!filepath.empty()) {
        logFile.open(filepath, std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("Cannot open log file: " + filepath);
        }
        logFilePath = filepath;
        consoleOutput = false;
    }
    else {
        consoleOutput = true;
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < currentLevel) return;
    if (consoleOutput) {
        std::cout << message << std::endl;
    }
    if (logFile.is_open()) {
        std::string logEntry = getTimestamp() + " [" + levelToString(level) + "] " + message;
        logFile << logEntry << std::endl;
        logFile.flush();
    }
}

LogLevel Logger::stringToLevel(const std::string& levelStr) {
    if (levelStr == "DEBUG") return LogLevel::DEBUG;
    if (levelStr == "INFO") return LogLevel::INFO;
    if (levelStr == "WARNING") return LogLevel::WARNING;
    if (levelStr == "ERROR") return LogLevel::ERROR;
    throw std::runtime_error("Unknown log level: " + levelStr);
}

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG: return "DEBUG";
    case LogLevel::INFO: return "INFO";
    case LogLevel::WARNING: return "WARNING";
    case LogLevel::ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}
