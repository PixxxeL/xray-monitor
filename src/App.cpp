#include "App.h"
#include "logger.h"
#include "utils.h"
#include "Config.h"
#include "version.h"
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
            // Parse access log for IP addresses
            xrayClient->processAccessLog();
            if (firstIteration) {
                sendStartupMessage();
                firstIteration = false;
            }
            else {
                sendNewConnectionMessage();
                sendDisconnectionMessage();
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
    std::string completed = "🏁 Xray connection monitoring completed";
    BOOST_LOG_TRIVIAL(error) << completed;
    if (telegramBot->isEnabled()) {
        telegramBot->sendMessage(completed);
    }
    return 0;
}

void App::initialize() {
    initLogging(config.logFilePath, config.logLevelStr);
    BOOST_LOG_TRIVIAL(info) << "Starting XRay Monitor " << VERSION_STRING;

    config.parseConfigFile();
    BOOST_LOG_TRIVIAL(info)
        << "XRay config loaded successfully. API endpoint: "
        << config.apiAddress << ":"
        << std::to_string(config.apiPort);

    // Initialize components
    xrayClient = std::make_unique<XRayClient>(config);
    telegramBot = std::make_unique<TelegramBot>(config.telegramToken, config.telegramChannel);

    // Setup signal handlers
    setupSignalHandlers();
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

void App::sendStartupMessage() {
    auto users = xrayClient->getConnected();

    std::stringstream telegramMsg;
    std::stringstream logMsg;

    telegramMsg << "🚀 *Xray connection monitoring has been launched*\n\n";
    if (users.empty()) {
        telegramMsg << "No users are connected to the server.";
        logMsg << "No users are connected to the server";
    }
    else {
        telegramMsg << "Users are already connected to the server:\n";
        logMsg << "Xray connection monitoring has been launched. Connected users: ";
    }

    bool firstUser = true;
    for (const auto& user : users) {
        telegramMsg << utils::escapeMDv2(user.email) << utils::escapeMDv2(" | ")
            << utils::escapeMDv2(user.id) << utils::escapeMDv2(" | ")
            << utils::escapeMDv2(user.ip) << "\n";
        if (!firstUser) {
            logMsg << ", ";
        }
        logMsg << user.email;
        firstUser = false;
    }

    if (telegramBot->isEnabled()) {
        telegramBot->sendMessage(telegramMsg.str());
    }

    BOOST_LOG_TRIVIAL(info) << logMsg.str();
}

void App::sendNewConnectionMessage() {
    auto users = xrayClient->getConnected();
    std::stringstream telegramMsg;
    std::stringstream logMsg;
    telegramMsg << "🔗 *Users have connected to the xray server:*\n";
    for (const auto& user : users) {
        telegramMsg << utils::escapeMDv2(user.email) << utils::escapeMDv2(" | ")
            << utils::escapeMDv2(user.id) << utils::escapeMDv2(" | ")
            << utils::escapeMDv2(user.ip) << utils::escapeMDv2(" | ")
            << utils::escapeMDv2(utils::formatTime(user.lastTime)) << "\n";

        logMsg << "New connection: "
            << user.email
            << " (" << user.ip << ") "
            << utils::formatTime(user.lastTime);

    }
    if (!users.empty()) {
        if (telegramBot->isEnabled()) {
            telegramBot->sendMessage(telegramMsg.str());
        }
        BOOST_LOG_TRIVIAL(info) << logMsg.str();
    }
}

void App::sendDisconnectionMessage() {
    auto users = xrayClient->getDisconnected();
    std::stringstream telegramMsg;
    std::stringstream logMsg;
    telegramMsg << "❌ *Users have disconnected from the xray server:*\n";
    for (const auto& user : users) {
        telegramMsg << utils::escapeMDv2(user.email) << utils::escapeMDv2(" | ")
            << utils::escapeMDv2(user.id) << utils::escapeMDv2(" | ")
            << utils::escapeMDv2(user.ip) << "\n";

        logMsg << "Discconnection: "
            << user.email
            << " (" << user.ip << ") "
            << utils::formatTime(user.lastTime);

    }
    if (!users.empty()) {
        if (telegramBot->isEnabled()) {
            telegramBot->sendMessage(telegramMsg.str());
        }
        BOOST_LOG_TRIVIAL(info) << logMsg.str();
    }
}
