#ifndef TELEGRAMBOT_H
#define TELEGRAMBOT_H

#include <string>
#include <boost/asio.hpp>
#include "Config.h"

class TelegramBot {
public:
    TelegramBot(const std::string& token, const std::string& channel);
    ~TelegramBot();

    bool sendMessage(const std::string& message);
    bool isEnabled() const { return !token.empty() && !channel.empty(); }

private:
    std::string token;
    std::string channel;
    boost::asio::io_service ioService;

    std::string escapeMessage(const std::string& text) const;
};

#endif
