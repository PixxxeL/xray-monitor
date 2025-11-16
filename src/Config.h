#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/property_tree/ptree.hpp>
#include <boost/program_options.hpp>


struct User {
    std::string id;
    std::string email;
    std::string ip;
    bool connected;
    time_t lastSeen;
    uint64_t downlink;
    uint64_t uplink;
};


struct XRayConfig {
    std::string apiAddress;
    int apiPort;
    std::string accessLogPath;
    std::unordered_map<std::string, User> users;

    static XRayConfig parseFromFile(const std::string& filepath);
    void validate() const;

private:
    void parseInbounds(const boost::property_tree::ptree& pt);
    void parseUsers(const boost::property_tree::ptree& pt);
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
