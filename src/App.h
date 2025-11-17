#ifndef APP_H
#define APP_H

#include "Config.h"
#include "State.h"
#include "XRayClient.h"
#include "TelegramBot.h"
#include <atomic>

class App {
public:
    App(const AppConfig& appConfig);
    ~App() = default;

    int run();
    void stop();

private:
    AppConfig appConfig;
    XRayConfig xrayConfig;
    State state;
    std::unique_ptr<XRayClient> xrayClient;
    std::unique_ptr<TelegramBot> telegramBot;
    std::atomic<bool> shutdownRequested{ false };

    void initialize();
    void setupSignalHandlers();
    void handleFirstIteration();
    void handleSubsequentIterations();
    void sendStartupMessages();
    void sendNewConnectionMessages(const std::unordered_map<std::string, User>& connectedUsers);
    static void signalHandler(int signal);
};

#endif
