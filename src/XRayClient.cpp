#include "XRayClient.h"
#include "utils.h"
#include <boost/log/trivial.hpp>
#include <boost/json.hpp>
#include <cstdio>
#include <fstream>
#include <regex>


namespace json = boost::json;

XRayClient::XRayClient(const Config& config, State& state)
    : config(config), state(state) {
}

bool XRayClient::queryStats() {
    try {
        std::string command = "xray api statsquery --server=" + config.apiAddress + ":" + std::to_string(config.apiPort);
        std::string output = utils::executeCommand(command);

        if (!output.empty()) {
            parseStatsOutput(output);
            return true;
        }
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Error querying XRay stats: " + std::string(e.what());
    }
    return false;
}

void XRayClient::parseAccessLog() {
    if (config.accessLogPath.empty() || state.getUsers().empty()) {
        BOOST_LOG_TRIVIAL(trace) 
            << "No access log path `" << config.accessLogPath
            << "` or connected users " << state.getUsers().size();
        return;
    }
    std::string output = "";
    try {
        // @TODO: move number line to options
        std::string command = "tail -n 1000 " + config.accessLogPath;
        output = utils::executeCommand(command);
        if (output.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Error reading XRay log, file empty";
            return;
        }
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(debug) << "Error reading XRay log: " + std::string(e.what());
        return;
    }
    std::istringstream iss(output);
    std::vector<std::string> lines;
    std::string line;
    std::regex logPattern(R"(from (\d+\.\d+\.\d+\.\d+):\d+ accepted .*? email: ([^\s]+))");
    std::unordered_map<std::string, std::string> ips;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        std::smatch matches;
        if (std::regex_search(*it, matches, logPattern) && matches.size() == 3) {
            std::string ip = matches[1];
            std::string email = matches[2];
            if (ips.find(email) == ips.end()) {
                ips[email] = ip;
            }
        }
    }
    for (const auto& pair : state.getUsers()) {
        std::string email = pair.second.email;
        if (!ips.count(email)) {
            continue;
        }
        std::string ip = ips[email];
        state.updateUser(email, ip, pair.second.connected);
    }
}

void XRayClient::parseStatsOutput(const std::string& output) {
    json::value j;
    try {
        j = json::parse(output);
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Wrong API JSON: " << std::string(e.what());
        return;
    }
    json::object root = j.as_object();
    if (auto it = root.find("stat"); it != root.end() && it->value().is_array()) {
        json::array statArray = it->value().as_array();
        state.disconnectAllUsers();
        for (auto& item : statArray) {
            if (!item.is_object()) continue;
            json::object obj = item.as_object();
            if (auto nameIt = obj.find("name"); nameIt != obj.end() && nameIt->value().is_string()) {
                std::string name = nameIt->value().as_string().c_str();
                if (name.substr(0, 4) == "user") {
                    std::vector<std::string> parts;
                    size_t start = 0;
                    size_t pos;
                    while ((pos = name.find(">>>", start)) != std::string::npos) {
                        parts.push_back(name.substr(start, pos - start));
                        start = pos + 3;
                    }
                    parts.push_back(name.substr(start));
                    if (parts.size() >= 4) {
                        std::string email = parts[1];
                        std::string trafficType = parts.back();
                        int64_t downlink = 0;
                        int64_t uplink = 0;
                        int64_t value = 0;
                        if (trafficType == "downlink" || trafficType == "uplink") {
                            if (auto valueIt = obj.find("value"); valueIt != obj.end() && valueIt->value().is_int64()) {
                                value = valueIt->value().as_int64();
                            }
                        }
                        if (trafficType == "downlink") {
                            downlink = value;
                        }
                        else if (trafficType == "uplink") {
                            uplink = value;
                        }
                        if (downlink || uplink) {
                            state.updateUser(email, "", true, downlink, uplink);
                            BOOST_LOG_TRIVIAL(trace) << "User: " << email << " " << trafficType << " = " << value;
                        }
                    }
                }
            }
        }
    }
}
