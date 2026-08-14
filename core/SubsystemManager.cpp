#include "SubsystemManager.hpp"

#include <algorithm>

SubsystemManager::SubsystemManager()
    : deployment_index_(0)
{
    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "SOLAR_ARRAY",
            "Solar Array",
            std::chrono::seconds(8),
            5.0,
            850.0,
            0.0005));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "ADCS_GYROS",
            "ADCS Gyroscopes",
            std::chrono::seconds(12),
            35.0,
            0.0,
            0.0008));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "STAR_TRACKER_ALPHA",
            "Star Tracker Alpha",
            std::chrono::seconds(20),
            18.0,
            0.0,
            0.0006));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "STAR_TRACKER_BETA",
            "Star Tracker Beta",
            std::chrono::seconds(20),
            18.0,
            0.0,
            0.0006));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "TELEMETRY_TRANSCEIVER",
            "Telemetry Transceiver",
            std::chrono::seconds(15),
            42.0,
            0.0,
            0.0007));

    subsystems_.push_back(
        std::make_shared<PayloadSubsystem>(
            "THERMAL_CONTROL",
            "Thermal Management",
            std::chrono::seconds(10),
            25.0,
            0.0,
            0.0004));
}

void SubsystemManager::update()
{
    for (auto &subsystem : subsystems_)
    {
        subsystem->update();
    }

    /*
     * Staggered deployment.
     */

    if (deployment_index_ >= subsystems_.size())
    {
        return;
    }

    auto current =
        subsystems_[deployment_index_];

    if (current->state() ==
        SubsystemState::OFFLINE)
    {

        current->beginInitialization();
        return;
    }

    if (current->state() ==
        SubsystemState::NOMINAL)
    {

        ++deployment_index_;

        if (deployment_index_ < subsystems_.size())
        {
            subsystems_[deployment_index_]
                ->beginInitialization();
        }
    }
}

std::shared_ptr<PayloadSubsystem>
SubsystemManager::find(
    std::string_view identifier) const
{
    for (const auto &subsystem : subsystems_)
    {

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

    const auto state =
        subsystem->state();

    if (state != SubsystemState::FAILED &&
        state != SubsystemState::DEGRADED)
    {

        result = "SUBSYSTEM IS NOT IN A RECOVERABLE FAULT STATE";
        return false;
    }

    subsystem->beginReboot();

    result =
        "REBOOT SEQUENCE INITIATED";

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

    const auto state =
        subsystem->state();

    if (state != SubsystemState::FAILED &&
        state != SubsystemState::DEGRADED)
    {

        result =
            "PATCH REQUIRES FAILED OR DEGRADED SUBSYSTEM";

        return false;
    }

    subsystem->beginPatch(patch_type);

    result =
        "PATCH SEQUENCE INITIATED";

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

    result =
        "FAULT INJECTED";

    return true;
}

void SubsystemManager::beginDeployment()
{
    deployment_index_ = 0;

    if (!subsystems_.empty())
    {
        subsystems_[0]->beginInitialization();
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
        total += subsystem->powerDraw();
    }

    return total;
}

double SubsystemManager::totalPowerGeneration() const
{
    double total = 0.0;

    for (const auto &subsystem : subsystems_)
    {
        total += subsystem->powerGeneration();
    }

    return total;
}

void SubsystemManager::enforceLowPowerMode()
{
    /*
     * Keep the spacecraft's essential hardware alive.
     *
     * Non-critical payload hardware is shut down.
     */

    for (const auto &subsystem : subsystems_)
    {

        if (subsystem->isCritical())
        {
            continue;
        }

        if (subsystem->state() ==
            SubsystemState::NOMINAL)
        {

            subsystem->forceFault(
                FaultSeverity::COMPONENT_DEGRADATION);
        }
    }
}
