#pragma once

#include "../core/Subsystem.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

enum class MissionPhase
{
    STATION_KEEPING_WAIT,
    DEPLOYMENT_SEQUENCE,
    NOMINAL_OPERATIONS
};

struct OrbitalParameters
{
    double initial_altitude_km = 400.0;
    double orbital_period_seconds = 5550.0;
    double inclination_deg = 51.6;
    double initial_phase_deg = 0.0;
};

struct Telemetry
{
    double altitude_km = 400.0;
    double orbital_phase_deg = 0.0;
    double signal_dbm = -120.0;
    double solar_generation_w = 0.0;
    double subsystem_draw_w = 0.0;
    double net_power_w = 0.0;
};

class PayloadController
{
public:
    explicit PayloadController(
        OrbitalParameters parameters = {});

    ~PayloadController();

    void start();
    void stop();

    MissionPhase missionPhase() const;
    std::string missionPhaseString() const;

    double missionElapsedSeconds() const;

    Telemetry telemetry() const;

    std::vector<std::shared_ptr<PayloadSubsystem>>
    subsystems() const;

    const OrbitalParameters &orbitalParameters() const;

    bool running() const;

private:
    void simulationLoop();
    void healthLoop();

    void updateMissionPhase();
    void updateDeployment();

    void updateOrbitalTelemetry();
    void updatePowerTelemetry();

    void performFailureChecks();

    std::vector<std::shared_ptr<PayloadSubsystem>> subsystems_;

    OrbitalParameters orbital_parameters_;

    mutable std::mutex mutex_;

    MissionPhase mission_phase_;

    Telemetry telemetry_;

    std::atomic<bool> running_;

    std::thread simulation_thread_;
    std::thread health_thread_;

    std::chrono::steady_clock::time_point mission_start_;

    std::mt19937 random_engine_;

    std::uniform_real_distribution<double> random_distribution_;

    // Deployment schedule
    std::size_t deployment_index_;

    std::chrono::steady_clock::time_point deployment_start_;

    double station_keeping_duration_;
};
