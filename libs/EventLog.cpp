#include "EventLog.hpp"

EventLog::EventLog(
    std::size_t maximum_entries)
    : maximum_entries_(maximum_entries)
{
}

void EventLog::add(
    const std::string &message)
{
    std::lock_guard lock(mutex_);

    entries_.push_back(message);

    while (entries_.size() >
           maximum_entries_)
    {

        entries_.pop_front();
    }
}

std::vector<std::string>
EventLog::entries() const
{
    std::lock_guard lock(mutex_);

    return {
        entries_.begin(),
        entries_.end()};
}
