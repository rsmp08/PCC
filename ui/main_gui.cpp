#include "PccGui.hpp"

#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>

int main_gui()
{
    using namespace std::chrono_literals;

    PccRuntimeState runtime;

    SubsystemManager subsystem_manager;
    FaultManager fault_manager(subsystem_manager);
    EventLog event_log(10);

    // Forward fault manager events to the shared event log
    fault_manager.setEventCallback(
        [&](const std::string &msg)
        {
            event_log.add(msg);
        });

    event_log.add("PCC BOOT: Stage 2 FDIR engine online");
    event_log.add("MISSION: Payload separation detected");

    // Begin staggered deployment
    subsystem_manager.beginDeployment();

    // Background simulation thread — advances time, telemetry, and FDIR
    auto sim_start = std::chrono::steady_clock::now();

    std::thread sim_thread([&]()
                           {
        const double orbital_period = 5550.0;

        while (runtime.running)
        {
            const auto now = std::chrono::steady_clock::now();
            const double met = std::chrono::duration<double>(now - sim_start).count();

            // Advance hardware state
            subsystem_manager.update();
            fault_manager.update();

            // Compute orbital telemetry
            double phase = std::fmod((met / orbital_period) * 360.0, 360.0);
            if (phase < 0.0)
                phase += 360.0;

            double altitude = 400.0 + 0.25 * std::sin(phase * 2.0 * 3.141592653589793 / 180.0);

            double angular_distance = std::abs(std::remainder(phase - 180.0, 360.0));
            double visibility = std::max(0.0, std::cos(angular_distance * 3.141592653589793 / 180.0));
            double signal = -120.0 + visibility * 70.0;

            double solar = subsystem_manager.totalPowerGeneration() * (0.5 + 0.5 * std::sin(phase * 3.141592653589793 / 180.0));
            solar = std::clamp(solar, 0.0, 1e9);

            double draw = subsystem_manager.totalPowerDraw();

            // Update shared runtime snapshot
            {
                std::lock_guard lock(runtime.mutex);
                runtime.met_seconds = met;
                runtime.altitude_km = altitude;
                runtime.orbital_phase_deg = phase;
                runtime.signal_dbm = signal;
                runtime.solar_generation_w = solar;
                runtime.subsystem_draw_w = draw;

                if (subsystem_manager.deploymentComplete())
                    runtime.mission_phase = "NOMINAL";
                else
                    runtime.mission_phase = "DEPLOYMENT";
            }

            std::this_thread::sleep_for(1s);
        } });

    PccGui gui(runtime, subsystem_manager, fault_manager, event_log);
    const int rc = gui.run();

    // Ensure simulation thread exits
    runtime.running = false;
    if (sim_thread.joinable())
        sim_thread.join();

    return rc;
}
