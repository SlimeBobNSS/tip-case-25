#include "TimeEntry.h"

#include <stdexcept>
#include <utility>

namespace timetracker {

TimeEntry::TimeEntry(std::string id, std::string taskId, Timestamp startedAt,
                     std::optional<Timestamp> endedAt)
    : id_(std::move(id)), taskId_(std::move(taskId)), startedAt_(startedAt), endedAt_(endedAt) {}

const std::string& TimeEntry::id() const { return id_; }
const std::string& TimeEntry::taskId() const { return taskId_; }
Timestamp TimeEntry::startedAt() const { return startedAt_; }
const std::optional<Timestamp>& TimeEntry::endedAt() const { return endedAt_; }
bool TimeEntry::isActive() const { return !endedAt_.has_value(); }

Timestamp TimeEntry::durationSeconds() const {
    return endedAt_ ? *endedAt_ - startedAt_ : now() - startedAt_;
}

void TimeEntry::finish(Timestamp endedAt) {
    if (endedAt <= startedAt_) {
        throw std::invalid_argument("Время окончания должно быть позже времени начала.");
    }
    endedAt_ = endedAt;
}

void TimeEntry::edit(Timestamp startedAt, Timestamp endedAt) {
    if (endedAt <= startedAt) {
        throw std::invalid_argument("Время окончания должно быть позже времени начала.");
    }
    startedAt_ = startedAt;
    endedAt_ = endedAt;
}

}
