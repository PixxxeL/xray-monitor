# CMakeList.txt: проект CMake для xray-monitor
cmake_minimum_required(VERSION 3.8)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("xray-monitor")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(STATIC_BUILD)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
endif()

find_package(OpenSSL REQUIRED)
find_package(Boost 1.83 REQUIRED COMPONENTS system program_options json log log_setup)

add_executable(
    xray-monitor 
    "src/main.cpp" "src/version.h"
    "src/logger.h" "src/logger.cpp"
    "src/utils.h" "src/utils.cpp"
    "src/Config.h" "src/Config.cpp"
    "src/State.h" "src/State.cpp"
    "src/App.h" "src/App.cpp"
    "src/XRayClient.h" "src/XRayClient.cpp"
    "src/TelegramBot.h" "src/TelegramBot.cpp"
)

target_link_libraries(xray-monitor
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
    Boost::log
    Boost::json
    Boost::program_options
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET xray-monitor PROPERTY CXX_STANDARD 20)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(xray-monitor PROPERTIES LINK_FLAGS "-s")
endif()
