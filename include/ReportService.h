#pragma once

#include "Project.h"
#include "Task.h"
#include "TimeEntry.h"

#include <string>
#include <vector>

namespace timetracker {

struct ReportRow {
    std::string rowType;
    std::string projectId;
    std::string projectName;
    std::string taskId;
    std::string taskName;
    Timestamp durationSeconds = 0;
};

struct ReportResult {
    std::vector<ReportRow> rows;
};

class ReportService {
public:
    bool build(const std::vector<Project>& projects,
               const std::vector<Task>& tasks,
               const std::vector<TimeEntry>& sessions,
               Timestamp start,
               Timestamp end,
               const std::string& projectId,
               ReportResult& result,
               std::string& error) const;
};

}
