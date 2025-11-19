#ifndef XRAYCLIENT_H
#define XRAYCLIENT_H

#include "Config.h"
#include "State.h"
#include <string>
#include <vector>


class XRayClient {
public:
    XRayClient(const Config& config, State& state);
    bool queryStats();
    void parseAccessLog();

private:
    const Config& config;
    State& state;
    void parseStatsOutput(const std::string& output);
};

#endif
