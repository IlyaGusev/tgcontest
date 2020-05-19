#include "run_server.h"

#include "controller.h"

#include <iostream>

using namespace drogon;

int RunServer() {
    std::cerr << "RUN" << std::endl;

    app()
        .setLogLevel(trantor::Logger::kDebug)
        .addListener("0.0.0.0", 1994)
        .setThreadNum(4);

    auto controllerPtr = std::make_shared<TController>();
    app().registerController(controllerPtr);

    app().run();

    return 0;
}
