#pragma once

#include "CsvStorage.h"

#include <optional>

namespace timetracker {

struct HistoryRecord {
    TimeEntry session;
    Project project;
    Task task;
};

class HistoryService {
public:
    explicit HistoryService(CsvStorage& storage);
    bool list(const std::string& projectId, std::optional<Timestamp> from,
              std::optional<Timestamp> to, std::vector<HistoryRecord>& records,
              std::string& error);
    bool edit(const std::string& entryId, Timestamp start, Timestamp end,
              Timestamp currentTime, TimeEntry& updated, std::string& error);

private:
    CsvStorage& storage_;
};

}
