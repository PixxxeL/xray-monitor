#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <memory>
#include <boost/format.hpp>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& getInstance();

    void initialize(const std::string& levelStr, const std::string& filepath = "");
    void log(LogLevel level, const std::string& message);
    void setLevel(LogLevel level);

    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args... args) {
        if (level >= currentLevel) {
            std::string message = (boost::format(format) % ... % args).str();
            log(level, message);
        }
    }

private:
    Logger() = default;
    ~Logger();

    LogLevel currentLevel = LogLevel::INFO;
    std::ofstream logFile;
    std::string logFilePath;
    bool consoleOutput = true;

    std::string levelToString(LogLevel level);
    std::string getTimestamp();
    LogLevel stringToLevel(const std::string& levelStr);
};

#endif
