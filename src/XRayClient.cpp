#include "XRayClient.h"
#include "Logger.h"
#include <cstdio>
#include <memory>
#include <fstream>
#include <regex>

XRayClient::XRayClient(const XRayConfig& config, State& state)
    : config(config), state(state) {
}

bool XRayClient::queryStats() {
    try {
        std::string command = "xray api statsquery --server=" + config.apiAddress + ":" + std::to_string(config.apiPort);
        std::string output = executeCommand(command);

        if (!output.empty()) {
            parseStatsOutput(output);
            return true;
        }
    }
    catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error querying XRay stats: " + std::string(e.what()));
    }
    return false;
}

void XRayClient::parseAccessLog() {
    if (config.accessLogPath.empty()) {
        return;
    }

    std::ifstream file(config.accessLogPath);
    if (!file.is_open()) {
        Logger::getInstance().log(LogLevel::WARNING, "Cannot open access log: " + config.accessLogPath);
        return;
    }

    std::string line;
    std::regex logPattern(R"(from (\d+\.\d+\.\d+\.\d+):\d+ accepted .*? email: ([^\s]+))");

    while (std::getline(file, line)) {
        std::smatch matches;
        if (std::regex_search(line, matches, logPattern) && matches.size() == 3) {
            std::string ip = matches[1];
            std::string email = matches[2];

            // Update user IP in state
            auto user = state.getUser(email);
            if (!user.email.empty()) {
                state.updateUser(email, ip, user.connected);
            }
        }
    }
}

std::string XRayClient::executeCommand(const std::string& command) const {
    std::array<char, 128> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed for command: " + command);
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

void XRayClient::parseStatsOutput(const std::string& output) {
    std::istringstream iss(output);
    std::string line;
    std::regex statPattern(R"("name":\s*"([^"]+)")");
        std::regex valuePattern(R"("value":\s*(\d+))");

    std::string currentStat;
    uint64_t currentValue = 0;

    while (std::getline(iss, line)) {
        std::smatch matches;

        if (std::regex_search(line, matches, statPattern) && matches.size() == 2) {
            currentStat = matches[1];
        }
        else if (std::regex_search(line, matches, valuePattern) && matches.size() == 2) {
            currentValue = std::stoull(matches[1]);

            if (!currentStat.empty()) {
                std::string email = extractEmailFromStatName(currentStat);
                if (!email.empty()) {
                    // Check if this is downlink traffic
                    if (currentStat.find("downlink") != std::string::npos) {
                        state.updateUser(email, "", true, currentValue);
                    }
                    else if (currentStat.find("uplink") != std::string::npos) {
                        state.updateUser(email, "", true, 0, currentValue);
                    }
                }
            }

            currentStat.clear();
            currentValue = 0;
        }
    }
}

std::string XRayClient::extractEmailFromStatName(const std::string& statName) const {
    size_t pos1 = statName.find(">>>");
    if (pos1 == std::string::npos) return "";

    size_t pos2 = statName.find(">>>", pos1 + 3);
    if (pos2 == std::string::npos) return "";

    return statName.substr(pos1 + 3, pos2 - pos1 - 3);
}
