#include "HistoryService.h"

#include <algorithm>

namespace timetracker {

HistoryService::HistoryService(CsvStorage& storage) : storage_(storage) {}

bool HistoryService::list(const std::string& projectId, std::optional<Timestamp> from,
                          std::optional<Timestamp> to, std::vector<HistoryRecord>& records,
                          std::string& error) {
    records.clear();
    if (from && to && *to <= *from) { error = "Конец периода должен быть позже начала."; return false; }
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    if (!storage_.loadAll(projects, tasks, sessions, error)) return false;
    if (!projectId.empty() && std::none_of(projects.begin(), projects.end(), [&](const Project& p) { return p.id() == projectId; })) {
        error = "Проект с таким ID не найден.";
        return false;
    }
    for (const auto& session : sessions) {
        if (session.isActive()) continue;
        if (from && to && (*session.endedAt() <= *from || session.startedAt() >= *to)) continue;
        if (from && !to && *session.endedAt() <= *from) continue;
        if (!from && to && session.startedAt() >= *to) continue;
        const auto task = std::find_if(tasks.begin(), tasks.end(), [&](const Task& value) { return value.id() == session.taskId(); });
        if (task == tasks.end()) { error = "Для сессии не найдена задача."; return false; }
        if (!projectId.empty() && task->projectId() != projectId) continue;
        const auto project = std::find_if(projects.begin(), projects.end(), [&](const Project& value) { return value.id() == task->projectId(); });
        if (project == projects.end()) { error = "Для задачи не найден проект."; return false; }
        records.push_back({session, *project, *task});
    }
    return true;
}

bool HistoryService::edit(const std::string& entryId, Timestamp start, Timestamp end,
                          Timestamp currentTime, TimeEntry& updated, std::string& error) {
    if (start >= end || end > currentTime) { error = "Требуется начало раньше окончания и окончание не позже текущего времени."; return false; }
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    if (!storage_.loadAll(projects, tasks, sessions, error)) return false;
    auto target = std::find_if(sessions.begin(), sessions.end(), [&](const TimeEntry& value) { return value.id() == entryId; });
    if (target == sessions.end() || target->isActive()) { error = "Завершенная сессия с таким ID не найдена."; return false; }
    TimeEntry candidate = *target;
    try { candidate.edit(start, end); }
    catch (const std::exception& exception) { error = exception.what(); return false; }
    for (const auto& other : sessions) {
        if (other.id() == entryId) continue;
        const Timestamp otherEnd = other.endedAt().value_or(currentTime);
        if (candidate.startedAt() < otherEnd && other.startedAt() < *candidate.endedAt()) {
            error = "Изменение пересекается с другой сессией.";
            return false;
        }
    }
    *target = candidate;
    if (!storage_.saveSessions(sessions, error)) return false;
    updated = candidate;
    return true;
}

}
