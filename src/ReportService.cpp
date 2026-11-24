#include "ReportService.h"

#include <algorithm>
#include <map>
#include <tuple>

namespace timetracker {

bool ReportService::build(const std::vector<Project>& projects,
                          const std::vector<Task>& tasks,
                          const std::vector<TimeEntry>& sessions,
                          Timestamp start, Timestamp end, const std::string& projectId,
                          ReportResult& result, std::string& error) const {
    result = {};
    if (end <= start) {
        error = "Конец периода должен быть позже его начала.";
        return false;
    }
    std::map<std::string, const Project*> projectById;
    std::map<std::string, const Task*> taskById;
    for (const auto& project : projects) projectById.emplace(project.id(), &project);
    for (const auto& task : tasks) taskById.emplace(task.id(), &task);
    if (!projectId.empty() && projectById.count(projectId) == 0) {
        error = "Проект с таким идентификатором не найден.";
        return false;
    }

    using Key = std::pair<std::string, std::string>;
    std::map<Key, ReportRow> taskTotals;
    for (const auto& session : sessions) {
        if (session.isActive() || session.startedAt() >= end || *session.endedAt() <= start) continue;
        const auto taskIt = taskById.find(session.taskId());
        if (taskIt == taskById.end()) { error = "В сессии указана отсутствующая задача."; return false; }
        const auto projectIt = projectById.find(taskIt->second->projectId());
        if (projectIt == projectById.end()) { error = "У задачи не найден проект."; return false; }
        const auto& task = *taskIt->second;
        const auto& project = *projectIt->second;
        if (!projectId.empty() && project.id() != projectId) continue;
        auto& row = taskTotals[{project.id(), task.id()}];
        row.rowType = "TASK";
        row.projectId = project.id();
        row.projectName = project.name();
        row.taskId = task.id();
        row.taskName = task.name();
        row.durationSeconds += std::min(*session.endedAt(), end) - std::max(session.startedAt(), start);
    }

    std::vector<ReportRow> taskRows;
    for (auto& item : taskTotals) taskRows.push_back(item.second);
    std::sort(taskRows.begin(), taskRows.end(), [](const ReportRow& a, const ReportRow& b) {
        return std::tie(a.projectName, a.projectId, a.taskName, a.taskId) <
               std::tie(b.projectName, b.projectId, b.taskName, b.taskId);
    });
    Timestamp grandTotal = 0;
    std::size_t index = 0;
    while (index < taskRows.size()) {
        const std::string currentProject = taskRows[index].projectId;
        ReportRow projectTotal;
        projectTotal.rowType = "PROJECT_TOTAL";
        projectTotal.projectId = taskRows[index].projectId;
        projectTotal.projectName = taskRows[index].projectName;
        while (index < taskRows.size() && taskRows[index].projectId == currentProject) {
            projectTotal.durationSeconds += taskRows[index].durationSeconds;
            grandTotal += taskRows[index].durationSeconds;
            result.rows.push_back(taskRows[index++]);
        }
        result.rows.push_back(std::move(projectTotal));
    }
    ReportRow grand;
    grand.rowType = "GRAND_TOTAL";
    grand.durationSeconds = grandTotal;
    result.rows.push_back(std::move(grand));
    return true;
}

}
