#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <vector>

class EventLog
{
public:
    explicit EventLog(std::size_t maximum_entries = 12);

    void add(const std::string &message);

    std::vector<std::string> entries() const;

private:
    std::size_t maximum_entries_;

    mutable std::mutex mutex_;

    std::deque<std::string> entries_;
};
