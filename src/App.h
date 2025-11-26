#ifndef APP_H
#define APP_H

#include "Config.h"
#include "XRayClient.h"
#include "TelegramBot.h"
#include <atomic>


class App {
public:
    App(const Config& config);
    ~App() = default;

    int run();
    void stop();

private:
    Config config;
    std::unique_ptr<XRayClient> xrayClient;
    std::unique_ptr<TelegramBot> telegramBot;
    std::atomic<bool> shutdownRequested{ false };

    void initialize();
    void setupSignalHandlers();
    void sendStartupMessage();
    void sendNewConnectionMessage();
    void sendDisconnectionMessage();
    static void signalHandler(int signal);
};

#endif
