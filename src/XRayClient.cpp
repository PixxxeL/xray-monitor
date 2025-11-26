#include "XRayClient.h"
#include "utils.h"
#include <boost/log/trivial.hpp>
#include <regex>


const int LINES_COUNT = 100; // @TODO: move number line to options
const int TIME_DIFF_LIMIT = 60 * 60 * 2; // 2 hours @TODO: to options

XRayClient::XRayClient(const Config& config) : config(config) {}

void XRayClient::processAccessLog() {
    if (config.accessLogPath.empty()) {
        BOOST_LOG_TRIVIAL(trace) 
            << "No access log path `"
            << config.accessLogPath << "`";
        return;
    }
    
    std::vector<std::string> lines = parseTailAccessLog();
    if (lines.empty()) {
        return;
    }

    const std::regex logPattern(
        R"((\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}\.\d+) )"
        R"(from (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):\d+ )"
        R"(accepted [^\s]+ (\[vless_tls >> direct\]) )"
        R"(email: ([^\s]+))"
    );
    const int logPatCount = 5;
    const std::time_t nowTs = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::unordered_set<std::string> processedEmails;

    connected.clear();
    disconnected.clear();
    suspicious.clear();

    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        std::smatch matches;
        if (std::regex_search(*it, matches, logPattern) && matches.size() == logPatCount) {
            std::time_t logTs = utils::parseDate(matches[1]);
            std::string ip = matches[2];
            std::string type = matches[3];
            std::string email = matches[4];
            std::string id = "";
            if (type != "[vless_tls >> direct]") {
                continue;
            }
            if (logTs == 0) {
                BOOST_LOG_TRIVIAL(debug) << "Not parsed datetime: " << matches[1];
                continue;
            }
            double diffSeconds = std::difftime(nowTs, logTs);
            if (diffSeconds > TIME_DIFF_LIMIT) {
                // Because lines are reverse reading and rest lines are also later
                break;
            }
            auto userIt = config.users.find(email);
            if (userIt == config.users.end()) {
                // Unknown user
                suspicious.insert(email);
                continue;
            }
            else {
                id = userIt->second.id;
            }
            // Skip if already processed this user in this run (ensure "each user only once")
            if (processedEmails.count(email)) {
                continue;
            }
            processedEmails.insert(email);

            std::unordered_map<std::string, Peer>::iterator peerIt = peers.find(email);
            if (peerIt == peers.end()) {
                Peer peer{ id, email, ip, logTs, 0 };
                peers.emplace(email, std::move(peer));
            }
            else {
                // Update existing peer: only if this log entry is newer (should be, but safe-guard)
                peerIt->second.ip = ip;
                peerIt->second.prevTime = peerIt->second.lastTime;
                peerIt->second.lastTime = logTs;
            }
        }
    }

    for (auto& [email, peer] : peers) {
        if (processedEmails.count(peer.email) == 0) {
            peer.lastTime = 0;
        }
        if (peer.lastTime == 0 && peer.prevTime != 0) {
            peer.prevTime = 0;
            disconnected.emplace_back(peer);
        }
        else if (peer.lastTime != 0 && peer.prevTime == 0) {
            connected.emplace_back(peer);
        }
        else if (peer.lastTime != 0 && peer.prevTime != 0 &&
            peer.lastTime - peer.prevTime > TIME_DIFF_LIMIT) {
            disconnected.emplace_back(peer);
        }
    }
}

std::vector<std::string> XRayClient::parseTailAccessLog() {
    std::string output = "";
    std::vector<std::string> lines;
    try {
        std::string command = "tail -n " + std::to_string(LINES_COUNT) + " " + config.accessLogPath;
        output = utils::executeCommand(command);
        if (output.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Error reading XRay log, file empty";
            return lines;
        }
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(debug)
            << "Error reading XRay log: "
            << std::string(e.what());
        return lines;
    }
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    return lines;
}


std::vector<Peer> XRayClient::getConnected() {
    return connected;
}

std::vector<Peer> XRayClient::getDisconnected() {
    return disconnected;
}

std::unordered_set<std::string> XRayClient::getSuspicious() {
    return suspicious;
}
