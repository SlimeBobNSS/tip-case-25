#pragma once

#include "CsvStorage.h"
#include "Result.h"

namespace timetracker {

class ProjectTaskService {
public:
    explicit ProjectTaskService(CsvStorage& storage);
    Result<Project> createProject(const std::string& name);
    Result<Task> createTask(const std::string& projectId, const std::string& name);
    Result<std::vector<Project>> listProjects();
    Result<std::vector<Task>> listTasks();
    bool createProject(const std::string& name, Project& created, std::string& error);
    bool createTask(const std::string& projectId, const std::string& name,
                    Task& created, std::string& error);
    bool list(std::vector<Project>& projects, std::vector<Task>& tasks, std::string& error);

private:
    CsvStorage& storage_;
};

}
