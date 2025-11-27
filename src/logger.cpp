#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <ios>
#include "logger.h"
#include "utils.h"


namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

static logging::trivial::severity_level stringToSeverity(const std::string& levelStr) {
    if (levelStr == "trace") return logging::trivial::trace;
    if (levelStr == "debug") return logging::trivial::debug;
    if (levelStr == "info") return logging::trivial::info;
    if (levelStr == "warning") return logging::trivial::warning;
    if (levelStr == "error") return logging::trivial::error;
    if (levelStr == "fatal") return logging::trivial::fatal;
    return logging::trivial::info;
}

void initLogging(const std::string filePath, const std::string level) {
    logging::add_common_attributes();
    // to console
    auto consoleHandler = logging::add_console_log();
    logging::trivial::severity_level levelSeverity = stringToSeverity(level);
    consoleHandler->set_filter(logging::trivial::severity >= levelSeverity);
    // to file
    if (filePath != "") {
        utils::ensurePathExists(filePath);
        auto fileHandler = logging::add_file_log(
            keywords::file_name = filePath,
            keywords::auto_flush = true,
            keywords::open_mode = std::ios::app,
            // @TODO: add smart method making filePath with placeholder
            // then log rotation may on back
            //keywords::rotation_size = 10 * 1024 * 1024,
            //keywords::max_size = 50 * 1024 * 1024,
            keywords::format = (
                expr::stream
                << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
                << " [" << logging::trivial::severity
                << "] " << expr::smessage
            )
        );
        fileHandler->set_filter(logging::trivial::severity >= levelSeverity);
    }
}
