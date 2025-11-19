#include "App.h"
#include "logger.h"
#include "utils.h"
#include "Config.h"
#include <csignal>
#include <thread>
#include <sstream>
#include <boost/log/trivial.hpp>


// Static member for signal handling
static App* g_appInstance = nullptr;

App::App(const Config& config) : config(config) {}

int App::run() {
    initialize();

    bool firstIteration = true;

    while (!shutdownRequested) {
        try {
            if (xrayClient->queryStats()) {
                // Parse access log for IP addresses
                xrayClient->parseAccessLog();
                if (firstIteration) {
                    handleFirstIteration();
                    firstIteration = false;
                }
                else {
                    handleSubsequentIterations();
                }
            }
            // Sleep for interval, for feedback on SIGTERM, every second
            for (int i = 0; i < config.interval && !shutdownRequested; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Error in main loop: " << std::string(e.what());
        }
    }

    // Shutdown
    std::string completed = "Xray connection monitoring completed";
    BOOST_LOG_TRIVIAL(error) << completed;
    if (telegramBot->isEnabled()) {
        telegramBot->sendMessage(completed);
    }
    return 0;
}

void App::initialize() {
    initLogging(config.logFilePath, config.logLevelStr);
    BOOST_LOG_TRIVIAL(info) << "Starting XRay Monitor";

    config.parseConfigFile();
    BOOST_LOG_TRIVIAL(info)
        << "XRay config loaded successfully. API endpoint: "
        << config.apiAddress << ":"
        << std::to_string(config.apiPort);

    // Initialize components
    xrayClient = std::make_unique<XRayClient>(config, state);
    telegramBot = std::make_unique<TelegramBot>(config.telegramToken, config.telegramChannel);

    // Setup signal handlers
    setupSignalHandlers();

    // Initial state setup with users from config
    for (const auto& pair : config.users) {
        state.updateUser(pair.first, "", false);
    }
}

void App::setupSignalHandlers() {
    g_appInstance = this;
    std::signal(SIGINT, App::signalHandler);
    std::signal(SIGTERM, App::signalHandler);
}

void App::stop() {
    shutdownRequested = true;
}

void App::signalHandler(int signal) {
    if (g_appInstance) {
        g_appInstance->stop();
    }
}

void App::handleFirstIteration() {
    sendStartupMessages();
}

void App::handleSubsequentIterations() {
    auto connectedUsers = state.getConnectedUsers();
    sendNewConnectionMessages(connectedUsers);
}

void App::sendStartupMessages() {
    auto connectedUsers = state.getConnectedUsers();

    std::stringstream telegramMsg;
    std::stringstream logMsg;

    telegramMsg << "*Xray connection monitoring has been launched*\n\n";
    telegramMsg << "Users are connected to the server:\n";

    logMsg << "Xray connection monitoring has been launched. Connected users: ";

    bool firstUser = true;
    for (const auto& pair : connectedUsers) {
        const auto& user = pair.second;
        telegramMsg << "• " << user.email << " " << user.ip << " " << user.id << "\n";

        if (!firstUser) {
            logMsg << ", ";
        }
        logMsg << user.email << "(" << user.ip << ")";
        firstUser = false;
    }

    if (telegramBot->isEnabled()) {
        telegramBot->sendMessage(telegramMsg.str());
    }

    BOOST_LOG_TRIVIAL(info) << logMsg.str();
}

void App::sendNewConnectionMessages(const std::unordered_map<std::string, User>& connectedUsers) {
    for (const auto& pair : connectedUsers) {
        const auto& user = pair.second;
        auto previousUser = state.getUser(user.email);

        if (!previousUser.connected) {
            // New connection
            std::stringstream telegramMsg;
            std::stringstream logMsg;

            telegramMsg << "*Users have connected to the xray server:*\n\n";
            telegramMsg << "• " << user.email << " " << user.ip << " " << user.id << " " << utils::formatTime(user.lastSeen);

            logMsg << "New connection: " << user.email << "(" << user.ip << ") â " << utils::formatTime(user.lastSeen);

            if (telegramBot->isEnabled()) {
                telegramBot->sendMessage(telegramMsg.str());
            }

            BOOST_LOG_TRIVIAL(info) << logMsg.str();
        }
    }
}
