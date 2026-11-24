#pragma once

#include "Project.h"
#include "Task.h"
#include "TimeEntry.h"

#include <filesystem>
#include <string>
#include <vector>

namespace timetracker {

class CsvStorage {
public:
    explicit CsvStorage(std::filesystem::path directory);

    bool loadProjects(std::vector<Project>& items, std::string& error);
    bool saveProjects(const std::vector<Project>& items, std::string& error);
    bool loadTasks(std::vector<Task>& items, std::string& error);
    bool saveTasks(const std::vector<Task>& items, std::string& error);
    bool loadSessions(std::vector<TimeEntry>& items, std::string& error);
    bool saveSessions(const std::vector<TimeEntry>& items, std::string& error);
    bool loadAll(std::vector<Project>& projects, std::vector<Task>& tasks,
                 std::vector<TimeEntry>& sessions, std::string& error);

    const std::filesystem::path& directory() const;

private:
    bool readTable(const std::string& filename, const std::vector<std::string>& header,
                   std::vector<std::vector<std::string>>& rows, std::string& error);
    bool writeTable(const std::string& filename, const std::vector<std::string>& header,
                    const std::vector<std::vector<std::string>>& rows, std::string& error) const;

    std::filesystem::path directory_;
};

}
