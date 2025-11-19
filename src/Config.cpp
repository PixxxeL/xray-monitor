#include <iostream>
#include <sstream>
#include <system_error>
#include <optional>
#include <boost/log/trivial.hpp>
#include "Config.h"
#include "version.h"
#include "utils.h"


namespace po = boost::program_options;
namespace json = boost::json;

// Вспомогательные функции для безопасного доступа
static std::optional<std::string>
get_optional_string(const json::object& obj, const char* key) {
    if (auto it = obj.find(key); it != obj.end() && it->value().is_string()) {
        return std::string(it->value().as_string());
    }
    return std::nullopt;
}

static std::optional<int64_t>
get_optional_int64(const json::object& obj, const char* key) {
    auto it = obj.find(key);
    if (it != obj.end() && it->value().is_int64()) {
        return it->value().as_int64();
    }
    return std::nullopt;
}

//Config& Config::getInstance() {
//    static Config instance;
//    return instance;
//}

Config Config::parseCommandLine(int argc, char* argv[]) {
    Config config;
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
        config.logLevelStr = utils::toLower(vm["log-level"].as<std::string>());
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

po::options_description Config::createOptionsDescription() {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help message")
        ("version,v", "Show version")
        ("xray-config-path,c", po::value<std::string>(), "XRay config file path")
        ("log-level,l", po::value<std::string>()->default_value("info"), "Log level (trace, debug, info, warning, error, fatal)")
        ("log-filepath", po::value<std::string>(), "Log file path")
        ("interval,i", po::value<int>()->default_value(10), "Polling interval in seconds")
        ("telegram-token,t", po::value<std::string>(), "Telegram bot token")
        ("telegram-channel", po::value<std::string>(), "Telegram channel ID");
    return desc;
}

void Config::validate() const {
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
    // get it from logger::stringToSeverity
    if (logLevelStr != "trace" && logLevelStr != "debug" && logLevelStr != "info" &&
        logLevelStr != "warning" && logLevelStr != "error" && logLevelStr != "fatal") {
        throw std::runtime_error("Invalid log level: " + logLevelStr);
    }
}

void Config::printHelp(const po::options_description& desc) const {
    std::cout << "XRay Monitor - VLESS connection monitoring tool\n\n"
        << "Usage: xray-monitor [options]\n\n"
        << desc << std::endl;
}

void Config::printVersion() const {
    std::cout << "XRay Monitor version " << VERSION_STRING << std::endl;
}

void Config::parseConfigFile() {
    auto j = utils::parseJsonFile(xrayConfigPath);
    if (!j.is_object()) {
        throw std::runtime_error("Root of XRay config must be a JSON object");
    }
    const auto& root = j.as_object();

    bool hasStatsService = false;
    auto api_it = root.find("api");
    if (api_it != root.end() && api_it->value().is_object()) {
        const auto& api_obj = api_it->value().as_object();
        auto services_it = api_obj.find("services");
        if (services_it != api_obj.end() && services_it->value().is_array()) {
            for (const auto& svc : services_it->value().as_array()) {
                if (svc.is_string() && svc.as_string() == "StatsService") {
                    hasStatsService = true;
                    break;
                }
            }
        }
    }
    if (!hasStatsService) {
        throw std::runtime_error("XRay config must contain api.services with StatsService");
    }

    parseInbounds(root);

    parseUsers(root);

    auto log_it = root.find("log");
    if (log_it != root.end() && log_it->value().is_object()) {
        const auto& log_obj = log_it->value().as_object();
        auto access_it = log_obj.find("access");
        if (access_it != log_obj.end() && access_it->value().is_string()) {
            accessLogPath = std::string(access_it->value().as_string());
        }
    }
}

void Config::parseInbounds(const json::object& root) {
    auto inbounds_it = root.find("inbounds");
    if (inbounds_it == root.end() || !inbounds_it->value().is_array()) {
        throw std::runtime_error("XRay config must contain 'inbounds' array");
    }

    const auto& inbounds = inbounds_it->value().as_array();
    bool foundDokodemo = false;

    for (const auto& inbound_val : inbounds) {
        if (!inbound_val.is_object()) continue;
        const auto& inbound = inbound_val.as_object();

        auto protocol_opt = get_optional_string(inbound, "protocol");
        if (protocol_opt && *protocol_opt == "dokodemo-door") {
            apiAddress = get_optional_string(inbound, "listen").value_or("127.0.0.1");

            auto port_opt = get_optional_int64(inbound, "port");
            if (!port_opt || *port_opt <= 0 || *port_opt > 65535) {
                throw std::runtime_error("Dokodemo-door inbound must have valid port (1-65535)");
            }
            apiPort = static_cast<int>(*port_opt);
            foundDokodemo = true;
            break;
        }
    }

    if (!foundDokodemo) {
        throw std::runtime_error("XRay config must contain dokodemo-door inbound for API access");
    }
    if (apiPort <= 0 || apiPort > 65535) {
        throw std::runtime_error("Invalid API port: " + std::to_string(apiPort));
    }
}

void Config::parseUsers(const json::object& root) {
    auto inbounds_it = root.find("inbounds");
    if (inbounds_it == root.end() || !inbounds_it->value().is_array()) return;

    const auto& inbounds = inbounds_it->value().as_array();

    for (const auto& inbound_val : inbounds) {
        if (!inbound_val.is_object()) continue;
        const auto& inbound = inbound_val.as_object();

        auto protocol_opt = get_optional_string(inbound, "protocol");
        if (!protocol_opt || *protocol_opt != "vless") continue;

        auto settings_it = inbound.find("settings");
        if (settings_it == inbound.end() || !settings_it->value().is_object()) continue;

        const auto& settings = settings_it->value().as_object();
        auto clients_it = settings.find("clients");
        if (clients_it == settings.end() || !clients_it->value().is_array()) continue;

        const auto& clients = clients_it->value().as_array();
        for (const auto& client_val : clients) {
            if (!client_val.is_object()) continue;
            const auto& client = client_val.as_object();

            auto id_opt = get_optional_string(client, "id");
            auto email_opt = get_optional_string(client, "email");

            if (id_opt && email_opt && !email_opt->empty()) {
                User user;
                user.id = *id_opt;
                user.email = *email_opt;
                users[user.email] = user;
            }
        }
    }
}
