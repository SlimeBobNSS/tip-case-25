#include "TimerController.h"

#include "DateTime.h"
#include "IdGenerator.h"

#include <algorithm>

namespace timetracker {

TimerController::TimerController(CsvStorage& storage, std::function<Timestamp()> clock)
    : storage_(storage), clock_(std::move(clock)) {}

OperationResult TimerController::start(const std::string& taskId) {
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    std::string error;
    if (!storage_.loadAll(projects, tasks, sessions, error)) {
        return {false, "Ошибка чтения данных: " + error};
    }

    if (std::any_of(sessions.begin(), sessions.end(), [](const TimeEntry& item) { return item.isActive(); })) {
        return {false, "Уже есть активная сессия. Сначала остановите ее."};
    }
    const auto task = std::find_if(tasks.begin(), tasks.end(), [&](const Task& item) {
        return item.id() == taskId;
    });
    if (task == tasks.end()) {
        return {false, "Задача с таким идентификатором не найдена."};
    }
    const auto projectExists = std::any_of(projects.begin(), projects.end(), [&](const Project& item) {
        return item.id() == task->projectId();
    });
    if (!projectExists) {
        return {false, "Для задачи не найден связанный проект. Проверьте файлы данных."};
    }
    const auto startedAt = clock_();
    for (const auto& session : sessions) {
        if (session.endedAt() && startedAt < *session.endedAt()) {
            return {false, "Текущее время раньше окончания последней сессии."};
        }
    }
    std::string id;
    do { id = generateUuid(); }
    while (std::any_of(sessions.begin(), sessions.end(), [&](const TimeEntry& item) { return item.id() == id; }));
    sessions.emplace_back(id, taskId, startedAt);
    if (!storage_.saveSessions(sessions, error)) {
        return {false, "Не удалось сохранить сессию: " + error};
    }
    return {true, "Учет запущен для задачи «" + task->name() + "» с " + formatDateTime(startedAt) + "."};
}

OperationResult TimerController::stop() {
    std::vector<TimeEntry> sessions;
    std::string error;
    std::vector<Project> projects;
    std::vector<Task> tasks;
    if (!storage_.loadAll(projects, tasks, sessions, error)) {
        return {false, "Ошибка чтения сессий: " + error};
    }
    auto active = std::find_if(sessions.begin(), sessions.end(), [](const TimeEntry& item) {
        return item.isActive();
    });
    if (active == sessions.end()) {
        return {false, "Активной сессии нет."};
    }
    const auto endedAt = clock_();
    if (endedAt <= active->startedAt()) {
        return {false, "Текущее время не позже времени начала. Сессия не остановлена."};
    }
    try {
        active->finish(endedAt);
    } catch (const std::exception& exception) {
        return {false, exception.what()};
    }
    const auto duration = active->durationSeconds();
    if (!storage_.saveSessions(sessions, error)) {
        return {false, "Не удалось сохранить сессию: " + error};
    }
    return {true, "Учет остановлен. Продолжительность: " + formatDuration(duration) + "."};
}

bool TimerController::getActive(TimeEntry& entry, Task& task, Project& project, std::string& error) {
    std::vector<TimeEntry> sessions;
    std::vector<Task> tasks;
    std::vector<Project> projects;
    if (!storage_.loadAll(projects, tasks, sessions, error)) {
        return false;
    }
    const auto active = std::find_if(sessions.begin(), sessions.end(), [](const TimeEntry& item) {
        return item.isActive();
    });
    if (active == sessions.end()) {
        error = "Активной сессии нет.";
        return false;
    }
    const auto foundTask = std::find_if(tasks.begin(), tasks.end(), [&](const Task& item) {
        return item.id() == active->taskId();
    });
    if (foundTask == tasks.end()) {
        error = "Для активной сессии не найдена задача.";
        return false;
    }
    const auto foundProject = std::find_if(projects.begin(), projects.end(), [&](const Project& item) {
        return item.id() == foundTask->projectId();
    });
    if (foundProject == projects.end()) {
        error = "Для активной задачи не найден проект.";
        return false;
    }
    entry = *active;
    task = *foundTask;
    project = *foundProject;
    return true;
}

}
