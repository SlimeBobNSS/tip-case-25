#include "ProjectTaskService.h"
#include "IdGenerator.h"

#include <algorithm>
#include <cctype>

namespace timetracker {
namespace {

std::string trimName(const std::string& text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c); });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return first < last ? std::string(first, last) : std::string{};
}

bool validName(const std::string& name) {
    return !name.empty() && name.find_first_of("\r\n") == std::string::npos &&
           !std::isspace(static_cast<unsigned char>(name.front())) &&
           !std::isspace(static_cast<unsigned char>(name.back()));
}

}

ProjectTaskService::ProjectTaskService(CsvStorage& storage) : storage_(storage) {}

bool ProjectTaskService::createProject(const std::string& inputName, Project& created, std::string& error) {
    if (inputName.find_first_of("\r\n") != std::string::npos) { error = "Название не должно содержать перевод строки."; return false; }
    const auto name = trimName(inputName);
    if (!validName(name)) { error = "Название проекта должно быть непустым и не содержать перевод строки."; return false; }
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    if (!storage_.loadAll(projects, tasks, sessions, error)) return false;
    do { created = Project(generateUuid(), name); }
    while (std::any_of(projects.begin(), projects.end(), [&](const Project& item) { return item.id() == created.id(); }));
    return storage_.saveProject(created, error);
}

bool ProjectTaskService::createTask(const std::string& projectId, const std::string& inputName,
                                    Task& created, std::string& error) {
    if (inputName.find_first_of("\r\n") != std::string::npos) { error = "Название не должно содержать перевод строки."; return false; }
    const auto name = trimName(inputName);
    if (!validName(name)) { error = "Название задачи должно быть непустым и не содержать перевод строки."; return false; }
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    if (!storage_.loadAll(projects, tasks, sessions, error)) return false;
    if (std::none_of(projects.begin(), projects.end(), [&](const Project& item) { return item.id() == projectId; })) {
        error = "Проект с таким ID не найден.";
        return false;
    }
    do { created = Task(generateUuid(), projectId, name); }
    while (std::any_of(tasks.begin(), tasks.end(), [&](const Task& item) { return item.id() == created.id(); }));
    return storage_.saveTask(created, error);
}

bool ProjectTaskService::list(std::vector<Project>& projects, std::vector<Task>& tasks, std::string& error) {
    std::vector<TimeEntry> sessions;
    return storage_.loadAll(projects, tasks, sessions, error);
}

Result<Project> ProjectTaskService::createProject(const std::string& name) {
    Project item; std::string error;
    if (!createProject(name, item, error)) return Result<Project>::failure(error);
    return Result<Project>::success(std::move(item));
}
Result<Task> ProjectTaskService::createTask(const std::string& projectId, const std::string& name) {
    Task item; std::string error;
    if (!createTask(projectId, name, item, error)) return Result<Task>::failure(error);
    return Result<Task>::success(std::move(item));
}
Result<std::vector<Project>> ProjectTaskService::listProjects() {
    std::vector<Project> projects; std::vector<Task> tasks; std::string error;
    if (!list(projects, tasks, error)) return Result<std::vector<Project>>::failure(error);
    return Result<std::vector<Project>>::success(std::move(projects));
}
Result<std::vector<Task>> ProjectTaskService::listTasks() {
    std::vector<Project> projects; std::vector<Task> tasks; std::string error;
    if (!list(projects, tasks, error)) return Result<std::vector<Task>>::failure(error);
    return Result<std::vector<Task>>::success(std::move(tasks));
}

}
