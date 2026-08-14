#include "SubsystemManager.hpp"

#include "../subsystems/ADCSGyros.hpp"
#include "../subsystems/SolarArray.hpp"
#include "../subsystems/StarTracker.hpp"
#include "../subsystems/TelemetryTransceiver.hpp"
#include "../subsystems/ThermalControl.hpp"

#include <algorithm>

SubsystemManager::SubsystemManager()
    : deployment_index_(0)
{
    // ------------------------------------------------------------
    // Solar Array
    // ------------------------------------------------------------

    subsystems_.push_back(
        std::make_shared<SolarArray>());

    // ------------------------------------------------------------
    // ADCS Gyroscopes
    // ------------------------------------------------------------

    subsystems_.push_back(
        std::make_shared<ADCSGyros>());

    // ------------------------------------------------------------
    // Star Tracker Alpha
    // ------------------------------------------------------------

    subsystems_.push_back(
        std::make_shared<StarTracker>(
            "STAR_TRACKER_ALPHA",
            "Star Tracker Alpha"));

    // ------------------------------------------------------------
    // Star Tracker Beta
    // ------------------------------------------------------------

    subsystems_.push_back(
        std::make_shared<StarTracker>(
            "STAR_TRACKER_BETA",
            "Star Tracker Beta"));

    // ------------------------------------------------------------
    // Telemetry Transceiver
    // ------------------------------------------------------------

    subsystems_.push_back(
        std::make_shared<TelemetryTransceiver>());

    // ------------------------------------------------------------
    // Thermal Control
    // ------------------------------------------------------------

    subsystems_.push_back(
        std::make_shared<ThermalControl>());
}

void SubsystemManager::update()
{
    // ------------------------------------------------------------
    // Update every subsystem
    // ------------------------------------------------------------

    for (auto &subsystem : subsystems_)
    {
        if (subsystem)
        {
            subsystem->update();
        }
    }

    // ------------------------------------------------------------
    // Staggered deployment
    //
    // Only one subsystem is initialized at a time.
    // The next subsystem starts once the previous subsystem
    // reaches NOMINAL.
    // ------------------------------------------------------------

    if (deployment_index_ >= subsystems_.size())
    {
        return;
    }

    auto &current = subsystems_[deployment_index_];

    if (!current)
    {
        ++deployment_index_;
        return;
    }

    // Start initialization if this subsystem is still offline.
    if (current->state() == SubsystemState::OFFLINE)
    {
        current->beginInitialization();
        return;
    }

    // Once it reaches nominal, move to the next subsystem.
    if (current->state() == SubsystemState::NOMINAL)
    {
        ++deployment_index_;

        if (deployment_index_ < subsystems_.size())
        {
            auto &next = subsystems_[deployment_index_];

            if (next)
            {
                next->beginInitialization();
            }
        }
    }
}

std::shared_ptr<PayloadSubsystem>
SubsystemManager::find(std::string_view identifier) const
{
    for (const auto &subsystem : subsystems_)
    {
        if (!subsystem)
        {
            continue;
        }

        if (subsystem->identifier() == identifier)
        {
            return subsystem;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<PayloadSubsystem>>
SubsystemManager::all() const
{
    return subsystems_;
}

bool SubsystemManager::rebootSubsystem(
    std::string_view subsystem_id,
    std::string &result)
{
    auto subsystem = find(subsystem_id);

    if (!subsystem)
    {
        result = "SUBSYSTEM NOT FOUND";
        return false;
    }

    const auto state = subsystem->state();

    if (state != SubsystemState::FAILED &&
        state != SubsystemState::DEGRADED)
    {
        result = "SUBSYSTEM IS NOT IN A RECOVERABLE FAULT STATE";
        return false;
    }

    subsystem->beginReboot();

    result = "REBOOT SEQUENCE INITIATED";

    return true;
}

bool SubsystemManager::applyPatchFix(
    std::string_view subsystem_id,
    std::string_view patch_type,
    std::string &result)
{
    auto subsystem = find(subsystem_id);

    if (!subsystem)
    {
        result = "SUBSYSTEM NOT FOUND";
        return false;
    }

    const auto state = subsystem->state();

    if (state != SubsystemState::FAILED &&
        state != SubsystemState::DEGRADED)
    {
        result = "PATCH REQUIRES FAILED OR DEGRADED SUBSYSTEM";
        return false;
    }

    subsystem->beginPatch(patch_type);

    result = "PATCH SEQUENCE INITIATED";

    return true;
}

bool SubsystemManager::injectFault(
    std::string_view subsystem_id,
    FaultSeverity severity,
    std::string &result)
{
    auto subsystem = find(subsystem_id);

    if (!subsystem)
    {
        result = "SUBSYSTEM NOT FOUND";
        return false;
    }

    subsystem->forceFault(severity);

    result = "FAULT INJECTED";

    return true;
}

void SubsystemManager::beginDeployment()
{
    deployment_index_ = 0;

    if (subsystems_.empty())
    {
        return;
    }

    auto &first = subsystems_[0];

    if (first &&
        first->state() == SubsystemState::OFFLINE)
    {
        first->beginInitialization();
    }
}

bool SubsystemManager::deploymentComplete() const
{
    return deployment_index_ >= subsystems_.size();
}

double SubsystemManager::totalPowerDraw() const
{
    double total = 0.0;

    for (const auto &subsystem : subsystems_)
    {
        if (!subsystem)
        {
            continue;
        }

        total += subsystem->powerDraw();
    }

    return total;
}

double SubsystemManager::totalPowerGeneration() const
{
    double total = 0.0;

    for (const auto &subsystem : subsystems_)
    {
        if (!subsystem)
        {
            continue;
        }

        total += subsystem->powerGeneration();
    }

    return total;
}

void SubsystemManager::enforceLowPowerMode()
{
    /*
     * Keep the spacecraft's essential hardware alive.
     *
     * Non-critical payload hardware is degraded/shut down.
     */

    for (const auto &subsystem : subsystems_)
    {
        if (!subsystem)
        {
            continue;
        }

        if (subsystem->isCritical())
        {
            continue;
        }

        if (subsystem->state() == SubsystemState::NOMINAL)
        {
            subsystem->forceFault(
                FaultSeverity::COMPONENT_DEGRADATION);
        }
    }
}