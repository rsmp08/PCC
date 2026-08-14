#pragma once

#include "../libs/FaultManager.hpp"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

class CommandInterpreter
{
public:
    using EventCallback =
        std::function<void(const std::string &)>;

    CommandInterpreter(
        SubsystemManager &subsystem_manager,
        FaultManager &fault_manager);

    ~CommandInterpreter();

    void start();
    void stop();

    bool isRunning() const;

    void setEventCallback(EventCallback callback);

private:
    void inputLoop();

    void processCommand(
        const std::string &command);

    FaultSeverity parseSeverity(
        const std::string &value) const;

    void printHelp();

    SubsystemManager &subsystem_manager_;
    FaultManager &fault_manager_;

    std::atomic<bool> running_;

    std::thread input_thread_;

    EventCallback event_callback_;
};
