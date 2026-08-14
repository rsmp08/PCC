#pragma once

#include "Subsystem.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

class SubsystemManager
{
public:
    SubsystemManager();

    void update();

    std::shared_ptr<PayloadSubsystem>
    find(std::string_view identifier) const;

    std::vector<std::shared_ptr<PayloadSubsystem>>
    all() const;

    bool rebootSubsystem(
        std::string_view subsystem_id,
        std::string &result);

    bool applyPatchFix(
        std::string_view subsystem_id,
        std::string_view patch_type,
        std::string &result);

    bool injectFault(
        std::string_view subsystem_id,
        FaultSeverity severity,
        std::string &result);

    void beginDeployment();

    bool deploymentComplete() const;

    double totalPowerDraw() const;
    double totalPowerGeneration() const;

    void enforceLowPowerMode();

private:
    std::vector<std::shared_ptr<PayloadSubsystem>>
        subsystems_;

    mutable std::mutex mutex_;

    std::size_t deployment_index_;
};
