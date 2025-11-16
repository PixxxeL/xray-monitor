#ifndef XRAYCLIENT_H
#define XRAYCLIENT_H

#include "Config.h"
#include "State.h"
#include <string>
#include <vector>

class XRayClient {
public:
    XRayClient(const XRayConfig& config, State& state);

    bool queryStats();
    void parseAccessLog();

private:
    XRayConfig config;
    State& state;

    std::string executeCommand(const std::string& command) const;
    void parseStatsOutput(const std::string& output);
    std::string extractEmailFromStatName(const std::string& statName) const;
};

#endif
