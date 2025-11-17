#include "App.h"
#include "Logger.h"
#include "utils.h"
#include <csignal>
#include <thread>
#include <sstream>

// Static member for signal handling
static App* g_appInstance = nullptr;

App::App(const AppConfig& appConfig) : appConfig(appConfig) {
}

int App::run() {
    try {
        initialize();

        bool firstIteration = true;

        // Main loop
        while (!shutdownRequested) {
            try {
                // Query XRay stats
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

                // Sleep for interval
                for (int i = 0; i < appConfig.interval && !shutdownRequested; ++i) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

            }
            catch (const std::exception& e) {
                Logger::getInstance().log(LogLevel::ERROR, "Error in main loop: " + std::string(e.what()));
            }
        }

        // Shutdown
        std::string completed = "Xray connection monitoring completed";
        Logger::getInstance().log(LogLevel::INFO, completed);
        if (telegramBot->isEnabled()) {
            telegramBot->sendMessage(completed);
        }
        return 0;
    }
    catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Fatal error in App::run: " + std::string(e.what()));
        return 1;
    }
}

void App::initialize() {
    Logger::getInstance().initialize(appConfig.logLevelStr, appConfig.logFilePath);
    Logger::getInstance().log(LogLevel::INFO, "Starting XRay Monitor");

    // Parse XRay config
    xrayConfig = XRayConfig::parseFromFile(appConfig.xrayConfigPath);
    xrayConfig.validate();

    Logger::getInstance().log(LogLevel::INFO,
        "XRay config loaded successfully. API endpoint: " + xrayConfig.apiAddress + ":" + std::to_string(xrayConfig.apiPort));

    // Initialize components
    xrayClient = std::make_unique<XRayClient>(xrayConfig, state);
    telegramBot = std::make_unique<TelegramBot>(appConfig.telegramToken, appConfig.telegramChannel);

    // Setup signal handlers
    setupSignalHandlers();

    // Initial state setup with users from config
    for (const auto& pair : xrayConfig.users) {
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

    Logger::getInstance().log(LogLevel::INFO, logMsg.str());
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

            logMsg << "New connection: " << user.email << "(" << user.ip << ") в " << utils::formatTime(user.lastSeen);

            if (telegramBot->isEnabled()) {
                telegramBot->sendMessage(telegramMsg.str());
            }

            Logger::getInstance().log(LogLevel::INFO, logMsg.str());
        }
    }
}
