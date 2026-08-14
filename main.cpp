#include "controllers/CommandInterpreter.hpp"
#include "libs/EventLog.hpp"
#include "libs/FaultManager.hpp"
#include "core/SubsystemManager.hpp"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace
{

    constexpr double PI =
        3.14159265358979323846;

    double radians(double degrees)
    {
        return degrees * PI / 180.0;
    }

    void clearScreen()
    {
        std::cout << "\033[2J\033[H";
    }

    std::string progressBar(double progress)
    {
        constexpr int width = 18;

        const int filled =
            static_cast<int>(
                progress * width);

        std::string result = "[";

        for (int i = 0; i < width; ++i)
        {
            result +=
                (i < filled) ? '#' : '.';
        }

        result += "]";

        return result;
    }

    std::string stateName(
        SubsystemState state)
    {
        switch (state)
        {

        case SubsystemState::OFFLINE:
            return "OFFLINE";

        case SubsystemState::INITIALIZING:
            return "INIT";

        case SubsystemState::NOMINAL:
            return "NOMINAL";

        case SubsystemState::DEGRADED:
            return "DEGRADED";

        case SubsystemState::FAILED:
            return "FAILED";

        case SubsystemState::REBOOTING:
            return "REBOOTING";

        case SubsystemState::PATCHING:
            return "PATCHING";
        }

        return "UNKNOWN";
    }

    void printDashboard(
        double met,
        double altitude,
        double phase,
        double signal,
        double solar,
        double draw,
        SubsystemManager &subsystem_manager,
        EventLog &event_log)
    {
        clearScreen();

        const double net_power =
            solar - draw;

        std::cout
            << "======================================================================\n"
            << "                    PAYLOAD CONTROL COMPUTER\n"
            << "                         STAGE 2 / FDIR\n"
            << "======================================================================\n";

        std::cout
            << " MET: "
            << std::fixed
            << std::setprecision(0)
            << met
            << " s";

        std::cout
            << "       ALT: "
            << std::setprecision(2)
            << altitude
            << " km";

        std::cout
            << "       PHASE: "
            << phase
            << " deg\n";

        std::cout
            << " SIGNAL: "
            << signal
            << " dBm";

        std::cout
            << "     SOLAR: "
            << solar
            << " W";

        std::cout
            << "     LOAD: "
            << draw
            << " W";

        std::cout
            << "     NET: "
            << net_power
            << " W\n";

        std::cout
            << "----------------------------------------------------------------------\n";

        std::cout
            << "                         SUBSYSTEM HEALTH\n";

        std::cout
            << "----------------------------------------------------------------------\n";

        std::cout
            << std::left
            << std::setw(25)
            << "ID"
            << std::setw(12)
            << "STATE"
            << std::setw(10)
            << "HEALTH"
            << std::setw(15)
            << "FAULT"
            << std::setw(24)
            << "TRANSITION"
            << "POWER\n";

        std::cout
            << "----------------------------------------------------------------------\n";

        for (const auto &subsystem :
             subsystem_manager.all())
        {

            double transition = 0.0;

            switch (subsystem->state())
            {

            case SubsystemState::INITIALIZING:
                transition =
                    subsystem->initializationProgress();
                break;

            case SubsystemState::REBOOTING:
                transition =
                    subsystem->rebootProgress();
                break;

            case SubsystemState::PATCHING:
                transition =
                    subsystem->patchProgress();
                break;

            default:
                transition =
                    subsystem->state() ==
                            SubsystemState::NOMINAL
                        ? 1.0
                        : 0.0;
                break;
            }

            std::cout
                << std::left
                << std::setw(25)
                << subsystem->identifier()

                << std::setw(12)
                << stateName(subsystem->state())

                << std::setw(10)
                << (std::to_string(
                        static_cast<int>(
                            subsystem->health())) +
                    "%")

                << std::setw(15)
                << subsystem->severityString()

                << std::setw(24)
                << (progressBar(transition) +
                    " " +
                    std::to_string(
                        static_cast<int>(
                            transition * 100.0)) +
                    "%")

                << std::fixed
                << std::setprecision(1)
                << subsystem->powerDraw()
                << " W\n";
        }

        std::cout
            << "----------------------------------------------------------------------\n"
            << "                              EVENT LOG\n"
            << "----------------------------------------------------------------------\n";

        for (const auto &entry :
             event_log.entries())
        {

            std::cout
                << "  "
                << entry
                << '\n';
        }

        std::cout
            << "----------------------------------------------------------------------\n"
            << " Commands: help | list | reboot <ID> | patch <ID> | "
               "inject <ID> <severity> | quit\n"
            << "----------------------------------------------------------------------\n";
    }

}

int main()
{
    using namespace std::chrono_literals;

    SubsystemManager subsystem_manager;

    FaultManager fault_manager(
        subsystem_manager,

        /*
         * Stage 2 global anomaly likelihood.
         *
         * The individual subsystem risk modifiers further
         * scale this value.
         */
        0.02);

    EventLog event_log(10);

    /*
     * Every fault-manager event enters the shared log.
     */

    fault_manager.setEventCallback(
        [&](const std::string &message)
        {
            event_log.add(message);
        });

    CommandInterpreter command_interpreter(
        subsystem_manager,
        fault_manager);

    command_interpreter.setEventCallback(
        [&](const std::string &message)
        {
            event_log.add(message);
        });

    event_log.add(
        "PCC BOOT: Stage 2 FDIR engine online");

    event_log.add(
        "MISSION: Payload separation detected");

    /*
     * Start the staged payload deployment.
     */

    subsystem_manager.beginDeployment();

    command_interpreter.start();

    auto start =
        std::chrono::steady_clock::now();

    double phase = 0.0;

    /*
     * Stage 2 orbital parameters.
     */

    constexpr double altitude_km = 400.0;
    constexpr double orbital_period = 5550.0;

    std::atomic<bool> running(true);

    while (command_interpreter.isRunning())
    {

        const auto now =
            std::chrono::steady_clock::now();

        const double met =
            std::chrono::duration<double>(
                now - start)
                .count();

        phase =
            std::fmod(
                (met / orbital_period) * 360.0,
                360.0);

        const double altitude =
            altitude_km +
            0.25 * std::sin(
                       radians(phase * 2.0));

        const double angular_distance =
            std::abs(
                std::remainder(
                    phase - 180.0,
                    360.0));

        const double visibility =
            std::max(
                0.0,
                std::cos(
                    radians(angular_distance)));

        const double signal =
            -120.0 +
            visibility * 70.0;

        subsystem_manager.update();

        fault_manager.update();

        double eclipse =
            0.5 +
            0.5 * std::sin(
                      radians(phase));

        eclipse =
            std::clamp(
                eclipse,
                0.0,
                1.0);

        const double solar =
            subsystem_manager.totalPowerGeneration() *
            eclipse;

        const double draw =
            subsystem_manager.totalPowerDraw();

        printDashboard(
            met,
            altitude,
            phase,
            signal,
            solar,
            draw,
            subsystem_manager,
            event_log);

        std::this_thread::sleep_for(
            std::chrono::seconds(1));
    }

    /*
     * Main loop.
     */

    bool active = true;

    while (active)
    {

        const auto now =
            std::chrono::steady_clock::now();

        const double met =
            std::chrono::duration<double>(
                now - start)
                .count();

        phase =
            std::fmod(
                (met / orbital_period) * 360.0,
                360.0);

        /*
         * Simple on-the-rails orbit.
         */

        const double altitude =
            altitude_km +
            0.25 * std::sin(
                       radians(phase * 2.0));

        /*
         * Ground station centered at 180 degrees.
         */

        const double angular_distance =
            std::abs(
                std::remainder(
                    phase - 180.0,
                    360.0));

        const double visibility =
            std::max(
                0.0,
                std::cos(
                    radians(
                        angular_distance)));

        const double signal =
            -120.0 +
            visibility * 70.0;

        /*
         * Update hardware and FDIR.
         */

        subsystem_manager.update();

        fault_manager.update();

        /*
         * Eclipse model.
         */

        double eclipse =
            0.5 +
            0.5 * std::sin(
                      radians(phase));

        eclipse =
            std::clamp(
                eclipse,
                0.0,
                1.0);

        const double solar =
            subsystem_manager.totalPowerGeneration() *
            eclipse;

        const double draw =
            subsystem_manager.totalPowerDraw();

        printDashboard(
            met,
            altitude,
            phase,
            signal,
            solar,
            draw,
            subsystem_manager,
            event_log);

        /*
         * The dashboard runs at 1 Hz.
         */

        std::this_thread::sleep_for(1s);

        /*
         * This is replaced below by the command interpreter
         * running state in a production-safe implementation.
         */
        if (!std::cin.good())
        {
            active = false;
        }
    }

    command_interpreter.stop();

    std::cout
        << "\nPCC SHUTDOWN.\n";

    return EXIT_SUCCESS;
}