#ifndef XRAYCLIENT_H
#define XRAYCLIENT_H

#include "Config.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <ctime>


struct Peer {
    std::string id;
    std::string email;
    std::string ip;
    std::time_t lastTime = 0;
    std::time_t prevTime = 0;
};

class XRayClient {
public:
    XRayClient(const Config& config);
    void processAccessLog();
    std::vector<Peer> getConnected();
    std::vector<Peer> getDisconnected();
    std::unordered_set<std::string> getSuspicious();

private:
    const Config& config;
    std::unordered_map<std::string, Peer> peers;
    std::vector<Peer> connected;
    std::vector<Peer> disconnected;
    std::unordered_set<std::string> suspicious;
    std::vector<std::string> parseTailAccessLog();
};

#endif
