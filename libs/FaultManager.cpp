#include "FaultManager.hpp"

#include <algorithm>
#include <sstream>

FaultManager::FaultManager(
    SubsystemManager &subsystem_manager,
    double global_failure_likelihood)
    : subsystem_manager_(subsystem_manager),
      global_failure_likelihood_(
          global_failure_likelihood),
      random_engine_(std::random_device{}()),
      probability_distribution_(0.0, 1.0)
{
}

void FaultManager::update()
{
    auto subsystems =
        subsystem_manager_.all();

    for (const auto &subsystem : subsystems)
    {

        if (!subsystem->isOperational())
        {
            continue;
        }

        /*
         * Effective fault probability.
         *
         * This is deliberately scaled by the subsystem's
         * own risk modifier.
         */

        double probability =
            global_failure_likelihood_ *
            subsystem->failureRiskModifier();

        if (probability_distribution_(random_engine_) <
            probability)
        {

            FaultSeverity severity =
                generateSeverity();

            subsystem->forceFault(severity);

            std::ostringstream message;

            message
                << "ANOMALY: "
                << subsystem->identifier()
                << " -> "
                << subsystem->severityString();

            emit(message.str());
        }
    }

    evaluateCascades();

    /*
     * Complete asynchronous recovery operations.
     */

    for (const auto &subsystem : subsystems)
    {

        if (subsystem->rebootComplete())
        {

            /*
             * Recovery probability depends on severity.
             */

            double recovery_probability = 0.20;

            switch (subsystem->faultSeverity())
            {

            case FaultSeverity::TRANSIENT_GLITCH:
                recovery_probability = 0.85;
                break;

            case FaultSeverity::COMPONENT_DEGRADATION:
                recovery_probability = 0.60;
                break;

            case FaultSeverity::CRITICAL_FAULT:
                recovery_probability = 0.20;
                break;

            case FaultSeverity::NONE:
                recovery_probability = 1.0;
                break;
            }

            bool success =
                probability_distribution_(random_engine_) <
                recovery_probability;

            subsystem->completeReboot(success);

            std::ostringstream message;

            if (success)
            {
                message
                    << "RECOVERY: "
                    << subsystem->identifier()
                    << " reboot successful";
            }
            else
            {
                message
                    << "RECOVERY FAILURE: "
                    << subsystem->identifier()
                    << " diagnostic required";
            }

            emit(message.str());
        }

        if (subsystem->patchComplete())
        {

            subsystem->completePatch();

            std::ostringstream message;

            message
                << "PATCH COMPLETE: "
                << subsystem->identifier()
                << " restored";

            emit(message.str());
        }
    }
}

void FaultManager::injectFault(
    std::string_view subsystem_id,
    FaultSeverity severity)
{
    std::string result;

    if (subsystem_manager_.injectFault(
            subsystem_id,
            severity,
            result))
    {

        std::ostringstream message;

        message
            << "OPERATOR FAULT INJECTION: "
            << subsystem_id
            << " -> "
            << static_cast<int>(severity);

        emit(message.str());
    }
    else
    {
        emit(
            "FAULT INJECTION FAILED: " +
            result);
    }
}

void FaultManager::setEventCallback(
    EventCallback callback)
{
    event_callback_ = std::move(callback);
}

double FaultManager::globalFailureLikelihood() const
{
    return global_failure_likelihood_;
}

void FaultManager::setGlobalFailureLikelihood(
    double value)
{
    global_failure_likelihood_ =
        std::clamp(value, 0.0, 1.0);
}

FaultSeverity FaultManager::generateSeverity()
{
    double value =
        probability_distribution_(random_engine_);

    if (value < 0.50)
    {
        return FaultSeverity::TRANSIENT_GLITCH;
    }

    if (value < 0.85)
    {
        return FaultSeverity::COMPONENT_DEGRADATION;
    }

    return FaultSeverity::CRITICAL_FAULT;
}

void FaultManager::evaluateCascades()
{
    auto thermal =
        subsystem_manager_.find("THERMAL_CONTROL");

    auto star_tracker =
        subsystem_manager_.find(
            "STAR_TRACKER_ALPHA");

    auto adcs =
        subsystem_manager_.find("ADCS_GYROS");

    if (thermal &&
        thermal->state() == SubsystemState::FAILED)
    {

        /*
         * Thermal failure makes the optical and
         * attitude-control systems increasingly vulnerable.
         */

        if (star_tracker)
        {
            star_tracker->setHealth(
                star_tracker->health() - 0.2);
        }

        if (adcs)
        {
            adcs->setHealth(
                adcs->health() - 0.15);
        }
    }

    /*
     * Solar array power cascade.
     */

    auto solar =
        subsystem_manager_.find("SOLAR_ARRAY");

    if (solar &&
        solar->state() == SubsystemState::FAILED)
    {

        subsystem_manager_.enforceLowPowerMode();

        emit(
            "POWER CASCADE: Solar array failure -> LOW POWER MODE");
    }
}

void FaultManager::emit(
    const std::string &message)
{
    if (event_callback_)
    {
        event_callback_(message);
    }
}
