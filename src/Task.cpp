#include "Task.h"

#include <utility>

namespace timetracker {

Task::Task(std::string id, std::string projectId, std::string name)
    : id_(std::move(id)), projectId_(std::move(projectId)), name_(std::move(name)) {}

const std::string& Task::id() const { return id_; }
const std::string& Task::projectId() const { return projectId_; }
const std::string& Task::name() const { return name_; }

}
