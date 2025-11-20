#include "TelegramBot.h"
#include <openssl/ssl.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/log/trivial.hpp>
#include <boost/json.hpp>


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;


TelegramBot::TelegramBot(const std::string& token, const std::string& channel) : token(token), channel(channel) {}

bool TelegramBot::sendMessage(const std::string& message) {
    if (!isEnabled()) {
        BOOST_LOG_TRIVIAL(warning) << "Telegram bot not configured, message not sent";
        return false;
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        ssl::context ctx{ ssl::context::tlsv12_client }; // or tls v1.3
        ssl::stream<tcp::socket> ssl_stream(ioc, ctx);

        auto const results = resolver.resolve("api.telegram.org", "443");
        net::connect(ssl_stream.next_layer(), results);

        // Set SNI hostname
        SSL_set_tlsext_host_name(ssl_stream.native_handle(), "api.telegram.org");

        // Make SSL handshake
        ssl_stream.handshake(ssl::stream_base::client);

        boost::json::object jsonBody{
            {"chat_id", channel},
            {"text", message},
            {"parse_mode", "MarkdownV2"}
        };
        std::string jsonStr = boost::json::serialize(jsonBody);

        http::request<http::string_body> req{
            http::verb::post,
            "/bot" + token + "/sendMessage",
            11
        };
        req.set(http::field::host, "api.telegram.org");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::content_length, std::to_string(jsonStr.size()));
        req.body() = jsonStr;

        http::write(ssl_stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(ssl_stream, buffer, res);

        boost::system::error_code ec;
        ssl_stream.shutdown(ec);
        if (ec && ec != boost::asio::ssl::error::stream_truncated) {
            throw boost::system::system_error(ec);
        }

        if (res.result() == http::status::ok) {
            BOOST_LOG_TRIVIAL(debug) << "Telegram message sent successfully";
            return true;
        }
        else {
            BOOST_LOG_TRIVIAL(error)
                << "Failed to send Telegram message: "
                << std::to_string(res.result_int())
                << " " << std::string(res.reason());
            return false;
        }
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error)
            << "Error sending Telegram message: "
            << std::string(e.what());
        return false;
    }
}
