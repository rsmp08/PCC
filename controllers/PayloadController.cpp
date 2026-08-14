#include "PayloadController.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace
{
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = PI * 2.0;

    double degreesToRadians(double degrees)
    {
        return degrees * PI / 180.0;
    }

    double radiansToDegrees(double radians)
    {
        return radians * 180.0 / PI;
    }
}

PayloadController::PayloadController(
    OrbitalParameters parameters)
    : orbital_parameters_(parameters),
      mission_phase_(MissionPhase::STATION_KEEPING_WAIT),
      running_(false),
      random_engine_(std::random_device{}()),
      random_distribution_(0.0, 1.0),
      deployment_index_(0),
      station_keeping_duration_(10.0),
      power_manager_()
{
    /*
     * Generic payload hardware configuration.
     *
     * identifier
     * label
     * initialization time
     * power draw
     * power generation
     * failure risk
     */

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "SOLAR_ARRAY",
            "Solar Array",
            std::chrono::seconds(8),
            5.0,
            850.0,
            0.00001f));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "ADCS_GYROS",
            "ADCS Gyroscopes",
            std::chrono::seconds(12),
            35.0,
            0.0,
            0.00002f));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "STAR_TRACKER_ALPHA",
            "Star Tracker Alpha",
            std::chrono::seconds(20),
            18.0,
            0.0,
            0.00001f));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "STAR_TRACKER_BETA",
            "Star Tracker Beta",
            std::chrono::seconds(20),
            18.0,
            0.0,
            0.00001f));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "TELEMETRY_TRANSCEIVER",
            "Telemetry Transceiver",
            std::chrono::seconds(15),
            42.0,
            0.0,
            0.00003f));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "THERMAL_CONTROL",
            "Thermal Management",
            std::chrono::seconds(10),
            25.0,
            0.0,
            0.00001f));

    /*
     * Power system configuration.
     *
     * These are currently simple simulation values.
     * We can make them configurable later.
     */
    power_manager_.setBatteryCapacity(1000.0);
    power_manager_.setBatteryCharge(500.0);
}

PayloadController::~PayloadController()
{
    stop();
}

void PayloadController::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    mission_start_ = std::chrono::steady_clock::now();
    deployment_start_ = mission_start_;

    mission_phase_ = MissionPhase::STATION_KEEPING_WAIT;

    simulation_thread_ =
        std::thread(&PayloadController::simulationLoop, this);

    health_thread_ =
        std::thread(&PayloadController::healthLoop, this);
}

void PayloadController::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    if (simulation_thread_.joinable())
    {
        simulation_thread_.join();
    }

    if (health_thread_.joinable())
    {
        health_thread_.join();
    }
}

bool PayloadController::running() const
{
    return running_.load();
}

MissionPhase PayloadController::missionPhase() const
{
    std::lock_guard lock(mutex_);
    return mission_phase_;
}

std::string PayloadController::missionPhaseString() const
{
    std::lock_guard lock(mutex_);

    switch (mission_phase_)
    {
    case MissionPhase::STATION_KEEPING_WAIT:
        return "STATION KEEPING WAIT";

    case MissionPhase::DEPLOYMENT_SEQUENCE:
        return "DEPLOYMENT SEQUENCE";

    case MissionPhase::NOMINAL_OPERATIONS:
        return "NOMINAL OPERATIONS";
    }

    return "UNKNOWN";
}

double PayloadController::missionElapsedSeconds() const
{
    if (!running_)
    {
        return 0.0;
    }

    return std::chrono::duration<double>(
               std::chrono::steady_clock::now() - mission_start_)
        .count();
}

Telemetry PayloadController::telemetry() const
{
    std::lock_guard lock(mutex_);
    return telemetry_;
}

std::vector<std::shared_ptr<PayloadSubsystem>>
PayloadController::subsystems() const
{
    return subsystems_;
}

const OrbitalParameters &
PayloadController::orbitalParameters() const
{
    return orbital_parameters_;
}

void PayloadController::simulationLoop()
{
    using namespace std::chrono_literals;

    while (running_)
    {
        updateMissionPhase();
        updateDeployment();
        updateOrbitalTelemetry();

        /*
         * Update subsystem state first.
         *
         * Power telemetry is calculated afterwards so the
         * current subsystem state is reflected immediately.
         */
        for (auto &subsystem : subsystems_)
        {
            subsystem->update(1s);
        }

        updatePowerTelemetry();

        std::this_thread::sleep_for(1s);
    }
}

void PayloadController::healthLoop()
{
    using namespace std::chrono_literals;

    while (running_)
    {
        performFailureChecks();

        std::this_thread::sleep_for(5s);
    }
}

void PayloadController::updateMissionPhase()
{
    double met = missionElapsedSeconds();

    std::lock_guard lock(mutex_);

    switch (mission_phase_)
    {
    case MissionPhase::STATION_KEEPING_WAIT:

        if (met >= station_keeping_duration_)
        {
            mission_phase_ = MissionPhase::DEPLOYMENT_SEQUENCE;
            deployment_start_ =
                std::chrono::steady_clock::now();
        }

        break;

    case MissionPhase::DEPLOYMENT_SEQUENCE:

        if (deployment_index_ >= subsystems_.size())
        {
            mission_phase_ =
                MissionPhase::NOMINAL_OPERATIONS;
        }

        break;

    case MissionPhase::NOMINAL_OPERATIONS:
        break;
    }
}

void PayloadController::updateDeployment()
{
    std::lock_guard lock(mutex_);

    if (mission_phase_ != MissionPhase::DEPLOYMENT_SEQUENCE)
    {
        return;
    }

    if (deployment_index_ >= subsystems_.size())
    {
        mission_phase_ = MissionPhase::NOMINAL_OPERATIONS;
        return;
    }

    auto now = std::chrono::steady_clock::now();

    /*
     * Each subsystem gets activated only after the
     * previous subsystem has reached NOMINAL.
     */

    if (deployment_index_ == 0)
    {
        if (subsystems_[0]->state() ==
            SubsystemState::OFFLINE)
        {
            subsystems_[0]->beginInitialization();
        }
    }

    if (subsystems_[deployment_index_]->state() ==
        SubsystemState::NOMINAL)
    {
        deployment_index_++;

        if (deployment_index_ < subsystems_.size())
        {
            subsystems_[deployment_index_]->beginInitialization();
        }
        else
        {
            mission_phase_ =
                MissionPhase::NOMINAL_OPERATIONS;
        }
    }

    /*
     * Safety timeout: if a subsystem remains stuck
     * initializing for several times its nominal
     * initialization period, mark it degraded.
     */

    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(
            now - deployment_start_)
            .count();

    if (elapsed > 120 &&
        deployment_index_ < subsystems_.size())
    {
        if (subsystems_[deployment_index_]->state() ==
            SubsystemState::INITIALIZING)
        {
            subsystems_[deployment_index_]->degrade();
        }
    }
}

void PayloadController::updateOrbitalTelemetry()
{
    double met = missionElapsedSeconds();

    double orbital_period =
        orbital_parameters_.orbital_period_seconds;

    double phase =
        orbital_parameters_.initial_phase_deg +
        (met / orbital_period) * 360.0;

    phase = std::fmod(phase, 360.0);

    if (phase < 0.0)
    {
        phase += 360.0;
    }

    /*
     * Stage 1 assumes a circular orbit.
     *
     * A tiny deterministic altitude variation is introduced
     * to make telemetry feel alive without needing a physics
     * engine.
     */

    double altitude_variation =
        0.25 * std::sin(degreesToRadians(phase * 2.0));

    double altitude =
        orbital_parameters_.initial_altitude_km +
        altitude_variation;

    /*
     * Ground station visibility.
     *
     * Maximum signal around phase 180 degrees.
     */

    double angular_distance =
        std::abs(std::remainder(
            phase - 180.0,
            360.0));

    double visibility =
        std::cos(
            degreesToRadians(
                angular_distance));

    visibility = std::max(0.0, visibility);

    /*
     * Base space-ground link.
     *
     * -120 dBm = effectively invisible
     * -50 dBm  = excellent
     */

    double signal =
        -120.0 +
        visibility * 70.0;

    std::lock_guard lock(mutex_);

    telemetry_.altitude_km = altitude;
    telemetry_.orbital_phase_deg = phase;
    telemetry_.signal_dbm = signal;
}

void PayloadController::updatePowerTelemetry()
{
    double solar_generation = 0.0;
    double subsystem_draw = 0.0;

    for (const auto &subsystem : subsystems_)
    {
        if (subsystem->isActive())
        {
            subsystem_draw += subsystem->powerDraw();
        }

        if (subsystem->isOperational())
        {
            solar_generation +=
                subsystem->powerGeneration();
        }
    }

    /*
     * Solar arrays only generate power when illuminated.
     *
     * A simple orbital eclipse model:
     * approximately 60% of the orbit is sunlit.
     */

    double phase;

    {
        std::lock_guard lock(mutex_);
        phase = telemetry_.orbital_phase_deg;
    }

    double eclipse_factor =
        0.5 +
        0.5 * std::sin(
                  degreesToRadians(phase));

    eclipse_factor =
        std::clamp(eclipse_factor, 0.0, 1.0);

    solar_generation *= eclipse_factor;

    /*
     * Feed the current spacecraft power state
     * into PowerManager.
     */
    power_manager_.setGeneration(solar_generation);
    power_manager_.setConsumption(subsystem_draw);

    /*
     * One simulation tick = one second.
     */
    power_manager_.update(1.0);

    double net =
        power_manager_.getPowerMargin();

    double battery_charge =
        power_manager_.getBatteryCharge();

    double battery_capacity =
        power_manager_.getBatteryCapacity();

    double battery_percentage =
        power_manager_.getBatteryChargePercentage();

    std::lock_guard lock(mutex_);

    telemetry_.solar_generation_w =
        solar_generation;

    telemetry_.subsystem_draw_w =
        subsystem_draw;

    telemetry_.net_power_w =
        net;

    telemetry_.battery_charge_wh =
        battery_charge;

    telemetry_.battery_capacity_wh =
        battery_capacity;

    telemetry_.battery_charge_percentage =
        battery_percentage;
}

void PayloadController::performFailureChecks()
{
    /*
     * Background stochastic health model.
     *
     * This deliberately uses very small probabilities so
     * failures are unusual rather than constant.
     */

    for (auto &subsystem : subsystems_)
    {
        if (!subsystem->isOperational())
        {
            continue;
        }

        double chance =
            static_cast<double>(
                subsystem->failureRiskModifier());

        if (random_distribution_(random_engine_) < chance)
        {
            /*
             * 50/50 chance of degradation or complete failure.
             */

            if (random_distribution_(random_engine_) < 0.5)
            {
                subsystem->degrade();
            }
            else
            {
                subsystem->fail();
            }
        }
    }
}