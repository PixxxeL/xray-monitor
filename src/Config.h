#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/json.hpp>
#include <boost/program_options.hpp>
#include "State.h"


namespace po = boost::program_options;
namespace json = boost::json;

class Config {
public:
    std::string xrayConfigPath = "/usr/local/etc/xray/config.json";
    std::string logLevelStr = "info";
    std::string logFilePath;
    unsigned int interval = 10;
    std::string telegramToken;
    std::string telegramChannel;
    std::string apiAddress = "127.0.0.1";
    unsigned int apiPort = 0;
    std::string accessLogPath;
    std::unordered_map<std::string, User> users;

    static Config parseCommandLine(int argc, char* argv[]);
    void validate() const;
    void printHelp(const po::options_description& desc) const;
    void printVersion() const;
    void parseConfigFile();

private:
    static po::options_description createOptionsDescription();
    void parseInbounds(const json::object& root);
    void parseUsers(const json::object& root);
};

#endif
