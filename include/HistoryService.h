#pragma once

#include "CsvStorage.h"
#include "Period.h"
#include "Result.h"

#include <optional>

namespace timetracker {

struct HistoryRow {
    TimeEntry session;
    Project project;
    Task task;
};

using HistoryRecord = HistoryRow;
using HistoryRowList = std::vector<HistoryRow>;

class HistoryService {
public:
    explicit HistoryService(CsvStorage& storage);
    Result<HistoryRowList> list(std::optional<std::string> projectId = std::nullopt,
                                std::optional<Period> period = std::nullopt);
    Result<TimeEntry> edit(const std::string& entryId, Timestamp start, Timestamp end);
    bool list(const std::string& projectId, std::optional<Timestamp> from,
              std::optional<Timestamp> to, std::vector<HistoryRecord>& records,
              std::string& error);
    bool edit(const std::string& entryId, Timestamp start, Timestamp end,
              Timestamp currentTime, TimeEntry& updated, std::string& error);

private:
    CsvStorage& storage_;
};

}
