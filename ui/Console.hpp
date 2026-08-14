#pragma once

#include "../controllers/PayloadController.hpp"

#include <atomic>
#include <string>

class Console
{
public:
    explicit Console(PayloadController &controller);

    void run();
    void stop();

private:
    void printHelp();
    void printStatus();
    void printTelemetry();
    void printSubsystems();
    void printPower();
    void printOrbital();

    void handleCommand(const std::string &command);

    std::shared_ptr<PayloadSubsystem>
    findSubsystem(const std::string &identifier);

    PayloadController &controller_;
    std::atomic<bool> running_;
};
