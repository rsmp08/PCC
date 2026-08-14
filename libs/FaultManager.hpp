#pragma once

#include "../core/SubsystemManager.hpp"

#include <chrono>
#include <functional>
#include <random>
#include <string>

class FaultManager
{
public:
    using EventCallback =
        std::function<void(const std::string &)>;

    FaultManager(
        SubsystemManager &subsystem_manager,
        double global_failure_likelihood = 0.02);

    void update();

    void injectFault(
        std::string_view subsystem_id,
        FaultSeverity severity);

    void setEventCallback(EventCallback callback);

    double globalFailureLikelihood() const;
    void setGlobalFailureLikelihood(double value);

private:
    FaultSeverity generateSeverity();

    void evaluateCascades();

    bool attemptRebootRecovery(
        const std::shared_ptr<PayloadSubsystem> &subsystem);

    void emit(const std::string &message);

    SubsystemManager &subsystem_manager_;

    double global_failure_likelihood_;

    std::mt19937 random_engine_;

    std::uniform_real_distribution<double>
        probability_distribution_;

    EventCallback event_callback_;
};
