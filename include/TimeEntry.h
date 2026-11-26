#pragma once

#include "DateTime.h"

#include <optional>
#include <string>

namespace timetracker {

class TimeEntry {
public:
    TimeEntry() = default;
    TimeEntry(std::string id, std::string taskId, Timestamp startedAt,
              std::optional<Timestamp> endedAt = std::nullopt);

    const std::string& id() const;
    const std::string& taskId() const;
    Timestamp startedAt() const;
    const std::optional<Timestamp>& endedAt() const;
    bool isActive() const;
    std::optional<Timestamp> durationSeconds() const;
    void finish(Timestamp endedAt);
    void edit(Timestamp startedAt, Timestamp endedAt);

private:
    std::string id_;
    std::string taskId_;
    Timestamp startedAt_ = 0;
    std::optional<Timestamp> endedAt_;
};

}
