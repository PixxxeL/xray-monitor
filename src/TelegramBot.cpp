#include "TelegramBot.h"
#include "Logger.h"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

TelegramBot::TelegramBot(const std::string& token, const std::string& channel)
    : token(token), channel(channel) {
}

TelegramBot::~TelegramBot() {
}

bool TelegramBot::sendMessage(const std::string& message) {
    if (!isEnabled()) {
        Logger::getInstance().log(LogLevel::WARNING, "Telegram bot not configured, message not sent");
        return false;
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        tcp::socket socket(ioc);

        auto const results = resolver.resolve("api.telegram.org", "443");

        // For simplicity, we'll use HTTP here. In production, consider using HTTPS
        beast::tcp_stream stream(ioc);
        stream.connect(results);

        std::string escapedMessage = escapeMessage(message);
        std::string requestBody = "chat_id=" + channel + "&text=" + escapedMessage + "&parse_mode=Markdown";

        http::request<http::string_body> req{ http::verb::post, "/bot" + token + "/sendMessage", 11 };
        req.set(http::field::host, "api.telegram.org");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/x-www-form-urlencoded");
        req.set(http::field::content_length, std::to_string(requestBody.length()));
        req.body() = requestBody;

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        stream.socket().shutdown(tcp::socket::shutdown_both);

        if (res.result() == http::status::ok) {
            Logger::getInstance().log(LogLevel::DEBUG, "Telegram message sent successfully");
            return true;
        }
        else {
            Logger::getInstance().log(LogLevel::ERROR,
                "Failed to send Telegram message: " + std::to_string(res.result_int()) + " " + res.reason().to_string());
            return false;
        }
    }
    catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, "Error sending Telegram message: " + std::string(e.what()));
        return false;
    }
}

std::string TelegramBot::escapeMessage(const std::string& text) const {
    std::string result;
    for (char c : text) {
        switch (c) {
        case '_': case '*': case '[': case ']': case '(': case ')':
        case '~': case '`': case '>': case '#': case '+': case '-':
        case '=': case '|': case '{': case '}': case '.': case '!':
            result += '\\';
            result += c;
            break;
        default:
            result += c;
        }
    }
    return result;
}
