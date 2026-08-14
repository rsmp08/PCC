#include <StarTracker.hpp>

StarTracker::StarTracker(
    std::string identifier,
    std::string label)
    : PayloadSubsystem(
          identifier,
          label,
          std::chrono::seconds(20),
          18.0,
          0.0,
          0.0006),
      attitude_accuracy_(0.0)
{
}

void StarTracker::update()
{
    PayloadSubsystem::update();

    // Star-tracker-specific processing
}

double StarTracker::attitudeAccuracy() const
{
    return attitude_accuracy_;
}