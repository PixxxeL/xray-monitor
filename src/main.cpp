#include <iostream>
#include "Config.h"
#include "App.h"


int main(int argc, char* argv[]) {
	try {
		Config config = Config::parseCommandLine(argc, argv);
		config.validate();
		App app(config);
		return app.run();
	}
	catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
