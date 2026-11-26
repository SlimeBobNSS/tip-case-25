#pragma once
#include "CsvStorage.h"
#include "Period.h"
#include "Report.h"
#include "Result.h"
namespace timetracker {
class ReportService {
public:
    explicit ReportService(CsvStorage& storage) : storage_(storage) {}
    Result<Report> build(Period period, std::optional<std::string> projectId = std::nullopt) const;
private:
    CsvStorage& storage_;
    bool calculate(const std::vector<Project>& projects, const std::vector<Task>& tasks,
                   const std::vector<TimeEntry>& sessions, Timestamp start, Timestamp end,
                   const std::string& projectId, ReportRowList& rows, std::string& error) const;
};
}
