#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include "logger.h"


namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace fs = boost::dll::fs;

static logging::trivial::severity_level stringToSeverity(const std::string& levelStr) {
    if (levelStr == "trace") return logging::trivial::trace;
    if (levelStr == "debug") return logging::trivial::debug;
    if (levelStr == "info") return logging::trivial::info;
    if (levelStr == "warning") return logging::trivial::warning;
    if (levelStr == "error") return logging::trivial::error;
    if (levelStr == "fatal") return logging::trivial::fatal;
    return logging::trivial::info;
}

void initLogging(const std::string fileName, const std::string level) {
    // to console
    auto consoleHandler = logging::add_console_log();
    logging::trivial::severity_level levelSeverity = stringToSeverity(level);
    consoleHandler->set_filter(logging::trivial::severity >= levelSeverity);
    // to file
    if (fileName != "") {
        fs::path exePath = boost::dll::program_location();
        boost::filesystem::path filePath = exePath / fileName;
        auto fileHandler = logging::add_file_log(
            keywords::file_name = filePath.string(),
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::max_size = 50 * 1024 * 1024,
            keywords::format = (
                expr::stream
                << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
                << " [" << logging::trivial::severity
                << "] " << expr::smessage
                )
        );
        fileHandler->set_filter(logging::trivial::severity >= levelSeverity);
    }
    logging::add_common_attributes();
}
