#include "Config.h"
#include "Logger.h"
#include "version.h"
#include <iostream>
#include <fstream>

namespace po = boost::program_options;

XRayConfig XRayConfig::parseFromFile(const std::string& filepath) {
    XRayConfig config;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open XRay config file: " + filepath);
    }

    boost::property_tree::ptree pt;
    try {
        boost::property_tree::read_json(file, pt);
    }
    catch (const boost::property_tree::json_parser_error& e) {
        throw std::runtime_error("Failed to parse XRay config JSON: " + std::string(e.what()));
    }

    // Validate API configuration
    auto apiServices = pt.get_child_optional("api.services");
    if (!apiServices || apiServices->find("StatsService") == apiServices->not_found()) {
        throw std::runtime_error("XRay config must contain api.services with StatsService");
    }

    // Parse inbounds
    config.parseInbounds(pt);

    // Parse users
    config.parseUsers(pt);

    // Parse access log path
    config.accessLogPath = pt.get("log.access", "");

    return config;
}

void XRayConfig::parseInbounds(const boost::property_tree::ptree& pt) {
    auto inbounds = pt.get_child_optional("inbounds");
    if (!inbounds) {
        throw std::runtime_error("XRay config must contain inbounds");
    }

    bool foundDokodemo = false;
    for (const auto& inbound : *inbounds) {
        auto protocol = inbound.second.get_optional<std::string>("protocol");
        if (protocol && *protocol == "dokodemo-door") {
            apiAddress = inbound.second.get("listen", "127.0.0.1");
            apiPort = inbound.second.get<int>("port", 0);
            if (apiPort == 0) {
                throw std::runtime_error("Dokodemo-door inbound must have port specified");
            }
            foundDokodemo = true;
            break;
        }
    }

    if (!foundDokodemo) {
        throw std::runtime_error("XRay config must contain dokodemo-door inbound");
    }
}

void XRayConfig::parseUsers(const boost::property_tree::ptree& pt) {
    for (const auto& inbound : pt.get_child("inbounds")) {
        auto protocol = inbound.second.get_optional<std::string>("protocol");
        if (protocol && *protocol == "vless") {
            auto clients = inbound.second.get_child_optional("settings.clients");
            if (clients) {
                for (const auto& client : *clients) {
                    User user;
                    user.id = client.second.get<std::string>("id", "");
                    user.email = client.second.get<std::string>("email", "");
                    user.connected = false;
                    user.lastSeen = 0;
                    user.downlink = 0;
                    user.uplink = 0;

                    if (!user.id.empty() && !user.email.empty()) {
                        users[user.email] = user;
                    }
                }
            }
        }
    }
}

void XRayConfig::validate() const {
    if (apiPort == 0) {
        throw std::runtime_error("Invalid API port");
    }
}

po::options_description AppConfig::createOptionsDescription() {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help message")
        ("version,v", "Show version")
        ("xray-config-path,c", po::value<std::string>(), "XRay config file path")
        ("log-level,l", po::value<std::string>(), "Log level (DEBUG, INFO, WARNING, ERROR)")
        ("log-filepath", po::value<std::string>(), "Log file path")
        ("interval,i", po::value<int>(), "Polling interval in seconds")
        ("telegram-token,t", po::value<std::string>(), "Telegram bot token")
        ("telegram-channel", po::value<std::string>(), "Telegram channel ID");

    return desc;
}

AppConfig AppConfig::parseCommandLine(int argc, char* argv[]) {
    AppConfig config;

    po::options_description desc = createOptionsDescription();

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (const po::error& e) {
        throw std::runtime_error("Command line parsing error: " + std::string(e.what()));
    }

    if (vm.count("help")) {
        config.printHelp(desc);
        exit(0);
    }

    if (vm.count("version")) {
        config.printVersion();
        exit(0);
    }

    if (vm.count("xray-config-path")) {
        config.xrayConfigPath = vm["xray-config-path"].as<std::string>();
    }

    if (vm.count("log-level")) {
        config.logLevelStr = vm["log-level"].as<std::string>();
    }

    if (vm.count("log-filepath")) {
        config.logFilePath = vm["log-filepath"].as<std::string>();
    }

    if (vm.count("interval")) {
        config.interval = vm["interval"].as<int>();
    }

    if (vm.count("telegram-token")) {
        config.telegramToken = vm["telegram-token"].as<std::string>();
    }

    if (vm.count("telegram-channel")) {
        config.telegramChannel = vm["telegram-channel"].as<std::string>();
    }

    return config;
}

void AppConfig::validate() const {
    if (xrayConfigPath.empty()) {
        throw std::runtime_error("XRay config path cannot be empty");
    }

    if (interval <= 0) {
        throw std::runtime_error("Interval must be positive");
    }

    if (!telegramToken.empty() && telegramChannel.empty()) {
        throw std::runtime_error("Telegram channel must be specified when token is provided");
    }

    if (telegramToken.empty() && !telegramChannel.empty()) {
        throw std::runtime_error("Telegram token must be specified when channel is provided");
    }

    // Validate log level
    if (logLevelStr != "DEBUG" && logLevelStr != "INFO" &&
        logLevelStr != "WARNING" && logLevelStr != "ERROR") {
        throw std::runtime_error("Invalid log level: " + logLevelStr);
    }
}

void AppConfig::printHelp(const po::options_description& desc) const {
    std::cout << "XRay Monitor - VLESS connection monitoring tool\n\n"
        << "Usage: xray_monitor [options]\n\n"
        << desc << std::endl;
}

void AppConfig::printVersion() const {
    std::cout << "XRay Monitor version " << VERSION_STRING << std::endl;
}
