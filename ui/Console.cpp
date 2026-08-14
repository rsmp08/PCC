#include "Console.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

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
                return static_cast<char>(std::toupper(c));
            });

        return value;
    }

    std::string stateName(SubsystemState state)
    {
        switch (state)
        {
        case SubsystemState::OFFLINE:
            return "OFFLINE";

        case SubsystemState::INITIALIZING:
            return "INITIALIZING";

        case SubsystemState::NOMINAL:
            return "NOMINAL";

        case SubsystemState::DEGRADED:
            return "DEGRADED";

        case SubsystemState::FAILED:
            return "FAILED";

        case SubsystemState::REBOOTING:
            return "REBOOTING";
        }

        return "UNKNOWN";
    }

    std::string progressBar(double progress)
    {
        constexpr int width = 25;

        int filled =
            static_cast<int>(progress * width);

        std::string result = "[";

        for (int i = 0; i < width; ++i)
        {
            result += (i < filled) ? '#' : '-';
        }

        result += "]";

        return result;
    }

    void printSeparator()
    {
        std::cout
            << "------------------------------------------------------------\n";
    }

}

Console::Console(PayloadController &controller)
    : controller_(controller),
      running_(true)
{
}

void Console::run()
{
    printHelp();

    std::string input;

    while (running_)
    {

        std::cout << "\nPCC> ";
        std::cout.flush();

        if (!std::getline(std::cin, input))
        {
            break;
        }

        if (input.empty())
        {
            continue;
        }

        handleCommand(input);
    }

    running_ = false;
}

void Console::stop()
{
    running_ = false;
}

void Console::handleCommand(
    const std::string &command)
{
    std::stringstream stream(command);

    std::string command_name;
    stream >> command_name;

    command_name = uppercase(command_name);

    if (command_name == "HELP" ||
        command_name == "?")
    {

        printHelp();
        return;
    }

    if (command_name == "STATUS")
    {
        printStatus();
        return;
    }

    if (command_name == "TELEMETRY" ||
        command_name == "TEL")
    {

        printTelemetry();
        return;
    }

    if (command_name == "SUBSYSTEMS" ||
        command_name == "SUBS")
    {

        printSubsystems();
        return;
    }

    if (command_name == "POWER")
    {
        printPower();
        return;
    }

    if (command_name == "ORBIT")
    {
        printOrbital();
        return;
    }

    if (command_name == "CLEAR")
    {
        std::cout << "\033[2J\033[H";
        return;
    }

    if (command_name == "QUIT" ||
        command_name == "EXIT")
    {

        running_ = false;
        return;
    }

    if (command_name == "REBOOT")
    {

        std::string identifier;
        stream >> identifier;

        if (identifier.empty())
        {
            std::cout
                << "Usage: reboot <SUBSYSTEM_ID>\n";
            return;
        }

        auto subsystem =
            findSubsystem(identifier);

        if (!subsystem)
        {
            std::cout
                << "Subsystem not found: "
                << identifier
                << '\n';

            return;
        }

        subsystem->reboot();

        std::cout
            << "Reboot command sent to "
            << subsystem->identifier()
            << '\n';

        return;
    }

    if (command_name == "FAIL")
    {

        std::string identifier;
        stream >> identifier;

        if (identifier.empty())
        {
            std::cout
                << "Usage: fail <SUBSYSTEM_ID>\n";

            return;
        }

        auto subsystem =
            findSubsystem(identifier);

        if (!subsystem)
        {
            std::cout
                << "Subsystem not found: "
                << identifier
                << '\n';

            return;
        }

        subsystem->fail();

        std::cout
            << "FAULT INJECTED: "
            << subsystem->identifier()
            << '\n';

        return;
    }

    if (command_name == "DEGRADE")
    {

        std::string identifier;
        stream >> identifier;

        if (identifier.empty())
        {
            std::cout
                << "Usage: degrade <SUBSYSTEM_ID>\n";

            return;
        }

        auto subsystem =
            findSubsystem(identifier);

        if (!subsystem)
        {
            std::cout
                << "Subsystem not found: "
                << identifier
                << '\n';

            return;
        }

        subsystem->degrade();

        std::cout
            << "DEGRADATION INJECTED: "
            << subsystem->identifier()
            << '\n';

        return;
    }

    std::cout
        << "Unknown command: "
        << command_name
        << '\n'
        << "Type 'help' for available commands.\n";
}

void Console::printHelp()
{
    std::cout << "Available commands:\n";
    std::cout << "  help, ?         Show this help\n";
    std::cout << "  status          Show system status\n";
    std::cout << "  telemetry (tel) Show telemetry values\n";
    std::cout << "  subsystems (subs)  List subsystems\n";
    std::cout << "  power           Show power summary\n";
    std::cout << "  orbit           Show orbital telemetry\n";
    std::cout << "  reboot <id>     Reboot subsystem\n";
    std::cout << "  fail <id>       Inject critical fault\n";
    std::cout << "  degrade <id>    Inject degradation\n";
}

void Console::printStatus()
{
    std::cout << "Mission phase: " << controller_.missionPhaseString() << '\n';
    std::cout << "Elapsed: " << controller_.missionElapsedSeconds() << " s\n";
    std::cout << "Controller running: " << (controller_.running() ? "yes" : "no") << '\n';
}

void Console::printTelemetry()
{
    auto t = controller_.telemetry();
    std::cout << "Altitude: " << t.altitude_km << " km\n";
    std::cout << "Orbital phase: " << t.orbital_phase_deg << " deg\n";
    std::cout << "Signal: " << t.signal_dbm << " dBm\n";
    std::cout << "Solar generation: " << t.solar_generation_w << " W\n";
    std::cout << "Subsystem draw: " << t.subsystem_draw_w << " W\n";
}

void Console::printSubsystems()
{
    auto subs = controller_.subsystems();
    printSeparator();
    std::cout << "ID\tLABEL\tSTATE\tHEALTH\tSEVERITY\n";

    for (const auto &s : subs)
    {
        std::cout
            << s->identifier() << '\t'
            << s->label() << '\t'
            << stateName(s->state()) << '\t'
            << static_cast<int>(s->health()) << "%\t"
            << s->severityString() << '\n';
    }

    printSeparator();
}

void Console::printPower()
{
    auto t = controller_.telemetry();
    std::cout << "Solar generation: " << t.solar_generation_w << " W\n";
    std::cout << "Subsystem draw: " << t.subsystem_draw_w << " W\n";
    std::cout << "Net: " << (t.solar_generation_w - t.subsystem_draw_w) << " W\n";
}

void Console::printOrbital()
{
    auto t = controller_.telemetry();
    std::cout << "Altitude: " << t.altitude_km << " km\n";
    std::cout << "Orbital phase: " << t.orbital_phase_deg << " deg\n";
}

std::shared_ptr<PayloadSubsystem>
Console::findSubsystem(const std::string &identifier)
{
    auto subs = controller_.subsystems();

    for (const auto &s : subs)
    {
        if (s->identifier() == identifier)
            return s;

        if (uppercase(s->label()) == uppercase(identifier))
            return s;
    }

    return nullptr;
}
