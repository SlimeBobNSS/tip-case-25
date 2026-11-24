#include "ProjectTaskService.h"
#include "IdGenerator.h"

#include <algorithm>
#include <cctype>

namespace timetracker {
namespace {

bool validName(const std::string& name) {
    return !name.empty() && name.find_first_of("\r\n") == std::string::npos &&
           !std::isspace(static_cast<unsigned char>(name.front())) &&
           !std::isspace(static_cast<unsigned char>(name.back()));
}

}

ProjectTaskService::ProjectTaskService(CsvStorage& storage) : storage_(storage) {}

bool ProjectTaskService::createProject(const std::string& name, Project& created, std::string& error) {
    if (!validName(name)) { error = "Название проекта должно быть непустым и не содержать перевод строки."; return false; }
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    if (!storage_.loadAll(projects, tasks, sessions, error)) return false;
    do { created = Project(generateUuid(), name); }
    while (std::any_of(projects.begin(), projects.end(), [&](const Project& item) { return item.id() == created.id(); }));
    projects.push_back(created);
    return storage_.saveProjects(projects, error);
}

bool ProjectTaskService::createTask(const std::string& projectId, const std::string& name,
                                    Task& created, std::string& error) {
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
    tasks.push_back(created);
    return storage_.saveTasks(tasks, error);
}

bool ProjectTaskService::list(std::vector<Project>& projects, std::vector<Task>& tasks, std::string& error) {
    std::vector<TimeEntry> sessions;
    return storage_.loadAll(projects, tasks, sessions, error);
}

}
