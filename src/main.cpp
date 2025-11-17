#include "Config.h"
#include "App.h"
#include "Logger.h"
#include <iostream>


int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        AppConfig appConfig = AppConfig::parseCommandLine(argc, argv);
        appConfig.validate();
        // Create and run application
        App app(appConfig);
        return app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
