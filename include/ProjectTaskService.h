#pragma once

#include "CsvStorage.h"

namespace timetracker {

class ProjectTaskService {
public:
    explicit ProjectTaskService(CsvStorage& storage);
    bool createProject(const std::string& name, Project& created, std::string& error);
    bool createTask(const std::string& projectId, const std::string& name,
                    Task& created, std::string& error);
    bool list(std::vector<Project>& projects, std::vector<Task>& tasks, std::string& error);

private:
    CsvStorage& storage_;
};

}
