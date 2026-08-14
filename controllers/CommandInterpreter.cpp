#include "CommandInterpreter.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace
{

    std::string uppercase(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(
                    std::toupper(c));
            });

        return value;
    }

}

CommandInterpreter::CommandInterpreter(
    SubsystemManager &subsystem_manager,
    FaultManager &fault_manager)
    : subsystem_manager_(subsystem_manager),
      fault_manager_(fault_manager),
      running_(false)
{
}

CommandInterpreter::~CommandInterpreter()
{
    stop();
}

void CommandInterpreter::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    input_thread_ =
        std::thread(
            &CommandInterpreter::inputLoop,
            this);
}

void CommandInterpreter::stop()
{
    /*
     * std::getline is blocking. The thread will terminate
     * when stdin reaches EOF or the operator exits.
     */

    running_ = false;

    if (input_thread_.joinable())
    {
        input_thread_.detach();
    }
}

bool CommandInterpreter::isRunning() const
{
    return running_.load();
}

void CommandInterpreter::setEventCallback(
    EventCallback callback)
{
    event_callback_ = std::move(callback);
}

void CommandInterpreter::inputLoop()
{
    std::string line;

    while (running_)
    {

        if (!std::getline(std::cin, line))
        {
            running_ = false;
            break;
        }

        if (!line.empty())
        {
            processCommand(line);
        }
    }
}

void CommandInterpreter::processCommand(
    const std::string &command)
{
    std::stringstream stream(command);

    std::string operation;
    stream >> operation;

    operation = uppercase(operation);

    if (operation == "HELP")
    {
        printHelp();
        return;
    }

    if (operation == "LIST")
    {

        auto subsystems =
            subsystem_manager_.all();

        std::cout
            << "\nSUBSYSTEM DIAGNOSTICS\n";

        for (const auto &subsystem : subsystems)
        {

            std::cout
                << subsystem->identifier()
                << " | "
                << subsystem->stateString()
                << " | Health "
                << subsystem->health()
                << "% | Fault "
                << subsystem->severityString()
                << '\n';
        }

        return;
    }

    if (operation == "REBOOT")
    {

        std::string identifier;
        stream >> identifier;

        std::string result;

        if (subsystem_manager_.rebootSubsystem(
                identifier,
                result))
        {

            if (event_callback_)
            {
                event_callback_(
                    "CMD reboot " +
                    identifier +
                    " -> " +
                    result);
            }
        }
        else
        {
            if (event_callback_)
            {
                event_callback_(
                    "CMD reboot FAILED: " +
                    result);
            }
        }

        return;
    }

    if (operation == "PATCH")
    {

        std::string identifier;
        stream >> identifier;

        std::string result;

        if (subsystem_manager_.applyPatchFix(
                identifier,
                "AUTO_DIAGNOSTIC",
                result))
        {

            if (event_callback_)
            {
                event_callback_(
                    "CMD patch " +
                    identifier +
                    " -> " +
                    result);
            }
        }
        else
        {
            if (event_callback_)
            {
                event_callback_(
                    "CMD patch FAILED: " +
                    result);
            }
        }

        return;
    }

    if (operation == "INJECT")
    {

        std::string identifier;
        std::string severity_string;

        stream >> identifier;
        stream >> severity_string;

        FaultSeverity severity =
            parseSeverity(
                severity_string);

        if (severity == FaultSeverity::NONE)
        {

            if (event_callback_)
            {
                event_callback_(
                    "CMD inject FAILED: invalid severity");
            }

            return;
        }

        fault_manager_.injectFault(
            identifier,
            severity);

        return;
    }

    if (operation == "EXIT" ||
        operation == "QUIT")
    {

        running_ = false;

        if (event_callback_)
        {
            event_callback_(
                "CMD shutdown requested");
        }

        return;
    }

    if (event_callback_)
    {
        event_callback_(
            "UNKNOWN COMMAND: " +
            command);
    }
}

FaultSeverity CommandInterpreter::parseSeverity(
    const std::string &value) const
{
    const std::string severity =
        uppercase(value);

    if (severity == "GLITCH" ||
        severity == "TRANSIENT_GLITCH")
    {

        return FaultSeverity::TRANSIENT_GLITCH;
    }

    if (severity == "DEGRADATION" ||
        severity == "COMPONENT_DEGRADATION")
    {

        return FaultSeverity::COMPONENT_DEGRADATION;
    }

    if (severity == "CRITICAL" ||
        severity == "CRITICAL_FAULT")
    {

        return FaultSeverity::CRITICAL_FAULT;
    }

    return FaultSeverity::NONE;
}

void CommandInterpreter::printHelp()
{
    std::cout
        << "\n"
        << "PAYLOAD CONTROL COMPUTER COMMANDS\n"
        << "--------------------------------\n"
        << "help\n"
        << "list\n"
        << "reboot <subsystem_id>\n"
        << "patch <subsystem_id>\n"
        << "inject <subsystem_id> <severity>\n"
        << "\n"
        << "SEVERITIES\n"
        << "  glitch\n"
        << "  degradation\n"
        << "  critical\n"
        << "\n"
        << "EXAMPLES\n"
        << "  reboot ADCS_GYROS\n"
        << "  patch STAR_TRACKER_ALPHA\n"
        << "  inject TELEMETRY_TRANSCEIVER critical\n"
        << '\n';
}
