#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/json.hpp>
#include <boost/program_options.hpp>

struct User {
    std::string id;
    std::string email;
    std::string ip;
    bool connected = false;
    time_t lastSeen = 0;
    uint64_t downlink = 0;
    uint64_t uplink = 0;
};

struct XRayConfig {
    std::string apiAddress = "127.0.0.1";
    int apiPort = 0;
    std::string accessLogPath;
    std::unordered_map<std::string, User> users;

    static XRayConfig parseFromFile(const std::string& filepath);
    void validate() const;

private:
    void parseInbounds(const boost::json::object& root);
    void parseUsers(const boost::json::object& root);
};

class AppConfig {
public:
    std::string xrayConfigPath = "/usr/local/etc/xray/config.json";
    std::string logLevelStr = "INFO";
    std::string logFilePath;
    int interval = 10;
    std::string telegramToken;
    std::string telegramChannel;

    static AppConfig parseCommandLine(int argc, char* argv[]);
    void validate() const;
    void printHelp(const boost::program_options::options_description& desc) const;
    void printVersion() const;

private:
    static boost::program_options::options_description createOptionsDescription();
};

#endif
