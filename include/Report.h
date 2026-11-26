#pragma once
#include "DateTime.h"
#include <string>
#include <vector>
#include <utility>
namespace timetracker {
struct ReportRow {
    std::string rowType;
    std::string projectId;
    std::string projectName;
    std::string taskId;
    std::string taskName;
    Timestamp durationSeconds = 0;
};
using ReportRowList = std::vector<ReportRow>;
class Report {
public:
    Report() : rows_{{"GRAND_TOTAL", {}, {}, {}, {}, 0}} {}
    explicit Report(ReportRowList rows) : rows_(std::move(rows)) {}
    const ReportRowList& rows() const { return rows_; }
    Timestamp totalSeconds() const { return rows_.empty() ? 0 : rows_.back().durationSeconds; }
private:
    ReportRowList rows_;
};
}
